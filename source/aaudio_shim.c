/* aaudio_shim.c -- AAudio over libnx audout, because this build of FMOD has
 * nothing else to offer.
 *
 * WHY THIS EXISTS
 * ---------------
 * Audio was silent, and the log said:
 *
 *     dlopen(libaaudio.so) -> NULL [not provided; caller takes its library-absent path]
 *     Unity: FMOD failed to initialize the output device.: "Error initializing
 *            output device. " (60)          <- FMOD_ERR_OUTPUT_INIT
 *
 * The earlier reading of this port was that FMOD would probe AAudio, find it
 * absent, and fall back to OpenSL ES -- which opensles.c implements. That was
 * wrong, and the binary says so plainly. The FMOD compiled into this libunity
 * carries exactly these output backends:
 *
 *     ../src/fmod_output.cpp              ../src/fmod_output_software.cpp
 *     ../src/fmod_output_nosound.cpp      ../src/fmod_output_nosound_nrt.cpp
 *     ../src/fmod_output_wavwriter_nrt.cpp  ../src/fmod_output_emulated.cpp
 *     ../android/src/fmod_output_aaudio.cpp
 *     ../android/src/fmod_output_audiotrack.cpp
 *
 * There is NO fmod_output_opensl.cpp. FMOD dropped OpenSL ES for Android in
 * this generation. So the two real choices are AAudio (a C API, dlopen'd) and
 * AudioTrack (a Java API, over JNI). Forcing FMOD_OUTPUTTYPE to OpenSL -- the
 * fix every earlier port in this lineage carries -- cannot work here: the
 * plugin is not in the binary. That is why the "force OpenSL" patch was
 * correctly reported as NOT NEEDED and audio was still silent.
 *
 * AAudio is the tractable one. It is plain C, FMOD reaches it through dlopen +
 * dlsym, and it maps onto libnx audout almost directly. AudioTrack would mean
 * implementing android.media.AudioTrack in jni_fake.c and marshalling PCM
 * through a fake JNI, which is far more surface for the same result.
 *
 * HOW FMOD DRIVES IT
 * ------------------
 *   AAudio_createStreamBuilder(&b)
 *   AAudioStreamBuilder_setSampleRate / setChannelCount / setFormat /
 *     setPerformanceMode / setFramesPerDataCallback / setDataCallback(cb, user)
 *   AAudioStreamBuilder_openStream(b, &stream)
 *   AAudioStream_requestStart(stream)
 *       -> from here a callback thread must call
 *          cb(stream, user, audioData, numFrames) to FILL the buffer, and
 *          keep calling it for as long as the stream runs.
 *   AAudioStream_requestStop / close
 *
 * So the shim owns a thread. That thread asks FMOD to fill a buffer and hands
 * it to audout. Nothing else drives it -- if the thread stops calling back,
 * audio stops, with no error anywhere.
 *
 * WHAT IS DELIBERATELY SIMPLE
 * ---------------------------
 * One stream. FMOD opens exactly one output stream, and pretending to support
 * several would mean sharing one audout device between them with no way to
 * mix. Opening a second returns an error rather than silently aliasing the
 * first.
 *
 * Format is forced to 16-bit PCM at the device rate. AAUDIO_FORMAT_PCM_FLOAT is
 * accepted at the builder and then reported back as I16, which is the same
 * thing a real device does when it does not support float: FMOD asks, reads the
 * actual format back, and converts. Lying about the format instead would make
 * FMOD hand us floats we would play as integers -- loud noise, not silence,
 * which is worse.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>   /* memalign: audout buffers must be 0x1000-aligned */
#include <string.h>
#include <switch.h>

#include "config.h"
#include "util.h"     /* install_bionic_tls */

/* debugPrintf comes from util.h -- `int debugPrintf(char *, ...)`. A local
 * `void debugPrintf(const char *, ...)` conflicts with it and is a hard
 * error once this file includes util.h, which it now does for
 * install_bionic_tls. */

/* ---- the slice of the AAudio ABI FMOD actually dlsyms --------------------
 * Taken from the 27 AAudio* strings in this libunity.so; anything not in that
 * list is not implemented, because an unused stub is one more thing to keep
 * honest. */

#define AAUDIO_OK                        0
#define AAUDIO_ERROR_INVALID_STATE      (-895)
#define AAUDIO_ERROR_UNAVAILABLE        (-891)
#define AAUDIO_ERROR_NO_FREE_HANDLES    (-890)

#define AAUDIO_FORMAT_PCM_I16            1
#define AAUDIO_FORMAT_PCM_FLOAT          2

#define AAUDIO_DIRECTION_OUTPUT          0
#define AAUDIO_STREAM_STATE_STARTED      5
#define AAUDIO_STREAM_STATE_STOPPED      7

#define AAUDIO_CALLBACK_RESULT_CONTINUE  0
#define AAUDIO_CALLBACK_RESULT_STOP      1

typedef int32_t aaudio_result_t;
typedef int32_t aaudio_format_t;

typedef int32_t (*AAudioStream_dataCallback)(void *stream, void *userData,
                                             void *audioData, int32_t numFrames);
typedef void    (*AAudioStream_errorCallback)(void *stream, void *userData,
                                              aaudio_result_t error);

/* AUDOUT_BUFFERS: two is the minimum that keeps the device fed while we build
 * the next block. More would only add latency. */
#define AUDOUT_BUFFERS   2
#define FRAMES_PER_CB    240        /* 5 ms at 48 kHz; FMOD may lower it */

typedef struct {
    int32_t sample_rate, channels, format;
    int32_t frames_per_cb, buffer_capacity, device_id, session_id;
    int32_t perf_mode, sharing, direction;
    AAudioStream_dataCallback  cb;
    AAudioStream_errorCallback err_cb;
    void *user;
} Builder;

/* util.h defines BIONIC_TLS_SIZE (0x400); use it rather than a guess, so this
 * block tracks the layout install_bionic_tls actually writes. */

typedef struct {
    Builder  cfg;            /* what FMOD asked for -- what the callback fills */
    int32_t  dev_rate, dev_channels;   /* what audout actually plays           */
    void    *scratch;        /* callback target, in the REQUESTED format       */
    size_t   scratch_bytes;
    int      open, running;
    Thread   thr;
    uint8_t  tls[BIONIC_TLS_SIZE] __attribute__((aligned(16)));
    volatile int quit;
    AudioOutBuffer buf[AUDOUT_BUFFERS];
    void    *mem[AUDOUT_BUFFERS];
    size_t   bytes;        /* allocation: frames*frame_bytes rounded to 0x1000 */
    size_t   data_bytes;   /* what a callback actually fills, and all we play  */
    uint64_t frames_played;
    uint32_t xruns;
} Stream;

static Stream g_stream;
static int    g_audout_up = 0;

/* ------------------------------------------------------------------------- */

/* Convert one callback's worth of audio from what FMOD wrote into what audout
 * plays, and return the number of bytes produced.
 *
 * THIS IS NOT OPTIONAL, and the reason is worth stating: FMOD does not read the
 * stream's format back. The 27 AAudio symbols this libunity dlsyms contain
 * setFormat/setSampleRate/setChannelCount but NO getFormat, getSampleRate or
 * getChannelCount. So whatever it asked the builder for is what it writes --
 * here 24000 Hz stereo FLOAT -- regardless of what openStream reports.
 *
 * The first version of this shim reported I16 at 48000 and played the buffer
 * as-is. That plays float bit patterns as integers at double speed: loud noise,
 * which is a worse failure than silence because it sounds like a working
 * audio path that is broken somewhere else.
 *
 * Resampling is linear interpolation. For the 24k -> 48k case that is exact
 * midpoint interpolation; it is not a high-quality resampler and does not need
 * to be, because FMOD is already mixing at its own rate and this is the last
 * hop to the device. */
static size_t aaudio_convert(Stream *s, const void *src, int32_t in_frames,
                             int16_t *dst, size_t dst_bytes)
{
    const int  ch      = s->cfg.channels;
    const int  dch     = s->dev_channels;
    const int  is_flt  = (s->cfg.format == AAUDIO_FORMAT_PCM_FLOAT);
    const int64_t in_r = s->cfg.sample_rate;
    const int64_t out_r= s->dev_rate;

    int32_t out_frames = (int32_t)(((int64_t)in_frames * out_r) / in_r);
    size_t  need = (size_t)out_frames * (size_t)dch * sizeof(int16_t);
    if (need > dst_bytes) {                     /* never write past the buffer */
        out_frames = (int32_t)(dst_bytes / ((size_t)dch * sizeof(int16_t)));
        need = (size_t)out_frames * (size_t)dch * sizeof(int16_t);
    }

    for (int32_t o = 0; o < out_frames; o++) {
        /* position in input frames, 16.16 fixed point */
        int64_t pos = ((int64_t)o * in_r << 16) / out_r;
        int32_t i0  = (int32_t)(pos >> 16);
        int32_t i1  = i0 + 1;
        if (i0 >= in_frames) i0 = in_frames - 1;
        if (i1 >= in_frames) i1 = in_frames - 1;
        int32_t fr  = (int32_t)(pos & 0xFFFF);

        for (int c = 0; c < dch; c++) {
            int sc = (c < ch) ? c : ch - 1;      /* mono source -> both ears */
            float a, b;
            if (is_flt) {
                const float *f = (const float *)src;
                a = f[(size_t)i0 * ch + sc];
                b = f[(size_t)i1 * ch + sc];
            } else {
                const int16_t *p = (const int16_t *)src;
                a = p[(size_t)i0 * ch + sc] / 32768.0f;
                b = p[(size_t)i1 * ch + sc] / 32768.0f;
            }
            float v = a + (b - a) * ((float)fr / 65536.0f);
            if (v >  1.0f) v =  1.0f;            /* clip rather than wrap */
            if (v < -1.0f) v = -1.0f;
            dst[(size_t)o * dch + c] = (int16_t)(v * 32767.0f);
        }
    }
    return need;
}

static void aaudio_thread(void *arg)
{
    Stream *s = (Stream *)arg;

    /* GIVE THIS THREAD A BIONIC TLS BLOCK BEFORE CALLING INTO FMOD.
     *
     * Every function libunity compiles with -fstack-protector begins
     *     mrs x21, tpidr_el0 ; ldr x9, [x21, #0x28]
     * to read its stack canary. A libnx thread made with raw threadCreate()
     * has tpidr_el0 = 0, so that load faults at address 0x28 -- which is
     * exactly the crash this shim caused on its first boot:
     *     pc=libunity+0xc85bec far=0x0000000000000028 esr=92000005
     *
     * Engine threads never hit it because they are born through
     * pthread_create_fake(), whose thread_trampoline() calls
     * install_bionic_tls() first (imports.c). The block must outlive the
     * thread, so it lives in the Stream rather than on the stack. */
    install_bionic_tls(s->tls);

    /* Prime every buffer before waiting on any. Waiting first means the first
     * audoutWaitPlayFinish has nothing queued and burns its whole timeout, once
     * per buffer, before audio starts. */
    for (int i = 0; i < AUDOUT_BUFFERS && !s->quit; i++) {
        AudioOutBuffer *b = &s->buf[i];
        memset(b->buffer, 0, s->bytes);
        int32_t frames = s->cfg.frames_per_cb;
        int32_t r = AAUDIO_CALLBACK_RESULT_CONTINUE;
        if (s->cfg.cb) {
            memset(s->scratch, 0, s->scratch_bytes);
            r = s->cfg.cb(s, s->cfg.user, s->scratch, frames);
        }
        if (r == AAUDIO_CALLBACK_RESULT_STOP) { s->quit = 1; break; }
        b->data_size = aaudio_convert(s, s->scratch, frames,
                                      (int16_t *)b->buffer, s->bytes);
        s->frames_played += (uint64_t)frames;
        if (R_FAILED(audoutAppendAudioOutBuffer(b))) s->xruns++;
    }

    while (!s->quit) {
        AudioOutBuffer *released = NULL;
        uint32_t count = 0;
        /* 100ms is a liveness bound, not a timing one: if audout stops
         * returning buffers we want to notice rather than block forever in a
         * thread nobody is watching. */
        if (R_FAILED(audoutWaitPlayFinish(&released, &count, 100000000ULL))
            || !count || !released) {
            s->xruns++;
            continue;
        }
        AudioOutBuffer *b = released;

        int32_t frames = s->cfg.frames_per_cb;
        int32_t r = AAUDIO_CALLBACK_RESULT_CONTINUE;
        if (s->cfg.cb) {
            memset(s->scratch, 0, s->scratch_bytes);
            r = s->cfg.cb(s, s->cfg.user, s->scratch, frames);
        }
        if (r == AAUDIO_CALLBACK_RESULT_STOP) {
            debugPrintf("[aaudio] data callback asked to STOP\n");
            break;
        }
        s->frames_played += (uint64_t)frames;

        b->data_size = aaudio_convert(s, s->scratch, frames,
                                      (int16_t *)b->buffer, s->bytes);
        if (R_FAILED(audoutAppendAudioOutBuffer(b))) s->xruns++;

        { static uint64_t last; uint64_t now = armTicksToNs(armGetSystemTick());
          if (now - last > 10000000000ULL) {
              last = now;
              debugPrintf("[aaudio] %llu frames in, %u xrun(s)\n",
                          (unsigned long long)s->frames_played, s->xruns); } }
    }
    debugPrintf("[aaudio] callback thread exiting (%llu frames)\n",
                (unsigned long long)s->frames_played);
}

/* ---- builder ------------------------------------------------------------- */

aaudio_result_t AAudio_createStreamBuilder(void **out)
{
    if (!out) return AAUDIO_ERROR_UNAVAILABLE;
    Builder *b = calloc(1, sizeof *b);
    if (!b) return AAUDIO_ERROR_NO_FREE_HANDLES;
    b->sample_rate   = 48000;
    b->channels      = 2;
    b->format        = AAUDIO_FORMAT_PCM_I16;
    b->frames_per_cb = FRAMES_PER_CB;
    *out = b;
    debugPrintf("[aaudio] createStreamBuilder -> %p\n", (void *)b);
    return AAUDIO_OK;
}

#define B(x) Builder *b = (Builder *)(x); if (!b) return
void AAudioStreamBuilder_setSampleRate(void *x, int32_t v)            { B(x); b->sample_rate = v; }
void AAudioStreamBuilder_setChannelCount(void *x, int32_t v)          { B(x); b->channels = v; }
void AAudioStreamBuilder_setFormat(void *x, int32_t v)                { B(x); b->format = v; }
void AAudioStreamBuilder_setDirection(void *x, int32_t v)             { B(x); b->direction = v; }
void AAudioStreamBuilder_setPerformanceMode(void *x, int32_t v)       { B(x); b->perf_mode = v; }
void AAudioStreamBuilder_setSharingMode(void *x, int32_t v)           { B(x); b->sharing = v; }
void AAudioStreamBuilder_setDeviceId(void *x, int32_t v)              { B(x); b->device_id = v; }
void AAudioStreamBuilder_setSessionId(void *x, int32_t v)             { B(x); b->session_id = v; }
void AAudioStreamBuilder_setBufferCapacityInFrames(void *x, int32_t v){ B(x); b->buffer_capacity = v; }
void AAudioStreamBuilder_setFramesPerDataCallback(void *x, int32_t v) { B(x); if (v > 0) b->frames_per_cb = v; }
void AAudioStreamBuilder_setDataCallback(void *x, void *cb, void *u)  { B(x); b->cb = (AAudioStream_dataCallback)cb; b->user = u; }
void AAudioStreamBuilder_setErrorCallback(void *x, void *cb, void *u) { B(x); b->err_cb = (AAudioStream_errorCallback)cb; (void)u; }
#undef B

void AAudioStreamBuilder_delete(void *x) { free(x); }

/* ---- stream -------------------------------------------------------------- */

aaudio_result_t AAudioStreamBuilder_openStream(void *xb, void **out)
{
    Builder *b = (Builder *)xb;
    if (!b || !out) return AAUDIO_ERROR_UNAVAILABLE;
    if (g_stream.open) {
        /* One device, one stream. Aliasing the open one would give two
         * producers for a single audout queue and interleave their frames. */
        debugPrintf("[aaudio] openStream refused: a stream is already open\n");
        return AAUDIO_ERROR_NO_FREE_HANDLES;
    }

    if (!g_audout_up) {
        if (R_FAILED(audoutInitialize())) {
            debugPrintf("[aaudio] audoutInitialize FAILED -- no audio\n");
            return AAUDIO_ERROR_UNAVAILABLE;
        }
        g_audout_up = 1;
    }
    if (R_FAILED(audoutStartAudioOut())) {
        debugPrintf("[aaudio] audoutStartAudioOut FAILED\n");
        return AAUDIO_ERROR_UNAVAILABLE;
    }

    Stream *s = &g_stream;
    memset(s, 0, sizeof *s);
    s->cfg = *b;                       /* keep what FMOD ASKED for, verbatim */

    /* The device's geometry is separate from the request. FMOD never reads the
     * stream's format back (no getFormat/getSampleRate/getChannelCount among
     * the symbols it dlsyms), so it will keep writing in the format it asked
     * for and aaudio_convert bridges the two. Reporting the device's values and
     * hoping FMOD adapts -- which the first version of this file did -- just
     * means the two sides disagree silently. */
    s->dev_rate     = (int32_t)audoutGetSampleRate();
    s->dev_channels = (int32_t)audoutGetChannelCount();
    if (s->dev_rate <= 0)     s->dev_rate = 48000;
    if (s->dev_channels <= 0) s->dev_channels = 2;
    if (s->cfg.sample_rate <= 0)   s->cfg.sample_rate = s->dev_rate;
    if (s->cfg.channels <= 0)      s->cfg.channels = 2;
    if (s->cfg.frames_per_cb <= 0) s->cfg.frames_per_cb = FRAMES_PER_CB;

    /* Scratch: what the callback writes, in the REQUESTED format. */
    const size_t in_sample = (s->cfg.format == AAUDIO_FORMAT_PCM_FLOAT) ? 4u : 2u;
    s->scratch_bytes = (size_t)s->cfg.frames_per_cb * (size_t)s->cfg.channels * in_sample;
    s->scratch = malloc(s->scratch_bytes);
    if (!s->scratch) {
        debugPrintf("[aaudio] scratch alloc (%u B) failed\n", (unsigned)s->scratch_bytes);
        audoutStopAudioOut();
        return AAUDIO_ERROR_NO_FREE_HANDLES;
    }

    /* Output: worst case is every input frame becoming dev_rate/req_rate output
     * frames. Round UP, then align the ALLOCATION to 0x1000 as audout requires;
     * data_size is set per block by aaudio_convert and is not rounded. */
    int64_t max_out = ((int64_t)s->cfg.frames_per_cb * s->dev_rate
                       + s->cfg.sample_rate - 1) / s->cfg.sample_rate;
    s->data_bytes = (size_t)max_out * (size_t)s->dev_channels * sizeof(int16_t);
    s->bytes = (s->data_bytes + 0xFFFu) & ~(size_t)0xFFFu;

    for (int i = 0; i < AUDOUT_BUFFERS; i++) {
        s->mem[i] = memalign(0x1000, s->bytes);
        if (!s->mem[i]) {
            debugPrintf("[aaudio] buffer alloc failed\n");
            for (int j = 0; j < i; j++) free(s->mem[j]);
            free(s->scratch); s->scratch = NULL;
            audoutStopAudioOut();
            return AAUDIO_ERROR_NO_FREE_HANDLES;
        }
        memset(s->mem[i], 0, s->bytes);
        s->buf[i].next        = NULL;
        s->buf[i].buffer      = s->mem[i];
        s->buf[i].buffer_size = s->bytes;
        s->buf[i].data_size   = s->data_bytes;
        s->buf[i].data_offset = 0;
    }

    s->open = 1;
    debugPrintf("[aaudio] openStream: FMOD writes %d Hz %d ch %s, %d frames/cb "
                "(%u B scratch); audout plays %d Hz %d ch s16, <=%u B/block "
                "(%u B alloc)\n",
                s->cfg.sample_rate, s->cfg.channels,
                s->cfg.format == AAUDIO_FORMAT_PCM_FLOAT ? "float" : "s16",
                s->cfg.frames_per_cb, (unsigned)s->scratch_bytes,
                s->dev_rate, s->dev_channels,
                (unsigned)s->data_bytes, (unsigned)s->bytes);
    *out = s;
    return AAUDIO_OK;
}

#define S(x) Stream *s = (Stream *)(x); if (!s || !s->open) return AAUDIO_ERROR_INVALID_STATE

aaudio_result_t AAudioStream_requestStart(void *x)
{
    S(x);
    if (s->running) return AAUDIO_OK;
    s->quit = 0;
    Result rc = threadCreate(&s->thr, aaudio_thread, s, NULL, 64 * 1024, 0x28, -2);
    if (R_SUCCEEDED(rc)) rc = threadStart(&s->thr);
    if (R_FAILED(rc)) {
        debugPrintf("[aaudio] callback thread start FAILED rc=0x%x\n", rc);
        return AAUDIO_ERROR_UNAVAILABLE;
    }
    s->running = 1;
    debugPrintf("[aaudio] stream started\n");
    return AAUDIO_OK;
}

aaudio_result_t AAudioStream_requestStop(void *x)
{
    S(x);
    if (!s->running) return AAUDIO_OK;
    s->quit = 1;
    threadWaitForExit(&s->thr);
    threadClose(&s->thr);
    s->running = 0;
    debugPrintf("[aaudio] stream stopped\n");
    return AAUDIO_OK;
}

aaudio_result_t AAudioStream_close(void *x)
{
    Stream *s = (Stream *)x;
    if (!s || !s->open) return AAUDIO_ERROR_INVALID_STATE;
    AAudioStream_requestStop(x);
    audoutStopAudioOut();
    for (int i = 0; i < AUDOUT_BUFFERS; i++) { free(s->mem[i]); s->mem[i] = NULL; }
    free(s->scratch); s->scratch = NULL;
    s->open = 0;
    debugPrintf("[aaudio] stream closed\n");
    return AAUDIO_OK;
}

aaudio_result_t AAudioStream_waitForStateChange(void *x, int32_t cur,
                                                int32_t *next, int64_t timeout)
{
    Stream *s = (Stream *)x; (void)cur; (void)timeout;
    if (!s) return AAUDIO_ERROR_INVALID_STATE;
    if (next) *next = s->running ? AAUDIO_STREAM_STATE_STARTED
                                 : AAUDIO_STREAM_STATE_STOPPED;
    return AAUDIO_OK;
}

int32_t AAudioStream_getBufferCapacityInFrames(void *x)
{ Stream *s = x; return s && s->open ? s->cfg.frames_per_cb * AUDOUT_BUFFERS : 0; }
int32_t AAudioStream_getBufferSizeInFrames(void *x)
{ Stream *s = x; return s && s->open ? s->cfg.frames_per_cb * AUDOUT_BUFFERS : 0; }
int32_t AAudioStream_setBufferSizeInFrames(void *x, int32_t n)
{ (void)n; return AAudioStream_getBufferSizeInFrames(x); }   /* fixed here */
int32_t AAudioStream_getFramesPerBurst(void *x)
{ Stream *s = x; return s && s->open ? s->cfg.frames_per_cb : FRAMES_PER_CB; }
int32_t AAudioStream_getXRunCount(void *x)
{ Stream *s = x; return s ? (int32_t)s->xruns : 0; }
int32_t AAudioStream_getDeviceId(void *x)   { (void)x; return 0; }
int32_t AAudioStream_getSessionId(void *x)  { (void)x; return 0; }
/* MMap is a shared-memory fast path we do not have; saying so keeps FMOD on
 * its ordinary buffered path rather than expecting a mapping. */
int32_t AAudioStream_isMMapUsed(void *x)    { (void)x; return 0; }
int32_t AAudioStream_getSampleRate(void *x)
{ Stream *s = x; return s && s->open ? s->cfg.sample_rate : 48000; }
int32_t AAudioStream_getChannelCount(void *x)
{ Stream *s = x; return s && s->open ? s->cfg.channels : 2; }
int32_t AAudioStream_getFormat(void *x)
{ Stream *s = x; return s && s->open ? s->cfg.format : AAUDIO_FORMAT_PCM_I16; }

#undef S
