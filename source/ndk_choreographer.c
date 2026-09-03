/* ndk_choreographer.c -- AChoreographer for the Switch loader.
 *
 * WHY THIS FILE EXISTS (and why neither reference port has one)
 * ------------------------------------------------------------
 * Killer Bean (Unity 2021.3) and Deus Ex GO (Unity 2022.3) drive frame timing
 * through the JAVA Choreographer: libunity calls
 * android.view.Choreographer.getInstance().postFrameCallback(cb) over JNI, and
 * jni_fake.c answers those calls. Data Defense is Unity 6000.3, and Unity 6
 * moved that path to the NDK C API instead:
 *
 *     AChoreographer_getInstance()
 *     AChoreographer_postFrameCallback(c, cb, data)
 *     AChoreographer_postFrameCallback64(c, cb64, data)
 *
 * These are UNDEFINED symbols in the game's libunity.so, so they must be
 * resolved by the loader's import table or the module will not load. And they
 * cannot be no-op stubs: a frame callback that is registered and never fired
 * is the classic "boots to a black screen and sits there" failure. Unity posts
 * a callback, waits for it to deliver the frame's timestamp, and posts the next
 * one from inside the callback. Break the chain anywhere and the render loop
 * simply stops advancing.
 *
 * WHAT THIS DOES
 * --------------
 * A single dispatcher thread ticks at the display refresh period and drains a
 * small pending-callback queue, passing each one a monotonically increasing
 * frame time in nanoseconds -- which is precisely the contract the real
 * Choreographer offers. Callbacks are one-shot on Android; they are one-shot
 * here too, so a callback that re-posts itself (what Unity does) keeps running
 * and one that does not, stops.
 *
 * Timestamps come from armGetSystemTick, the same clock nx_now_ns() uses, so
 * the frame times handed to Unity agree with everything else in the tree
 * rather than forming a second, drifting timebase.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#include "config.h"
#include "util.h"   /* install_bionic_tls, BIONIC_TLS_SIZE */

/* debugPrintf lives in main.c/util.c depending on the tree revision. */
/* debugPrintf comes from util.h -- `int debugPrintf(char *, ...)`. A local
 * `void debugPrintf(const char *, ...)` conflicts with it and is a hard
 * error once this file includes util.h, which it now does for
 * install_bionic_tls. */

/* ------------------------------------------------------------------------- */

typedef void (*AChoreographer_frameCallback)(long frameTimeNanos, void *data);
typedef void (*AChoreographer_frameCallback64)(int64_t frameTimeNanos, void *data);

typedef struct {
    void *fn;        /* frameCallback or frameCallback64 */
    void *data;
    int   is64;
    int64_t due_ns;  /* 0 = next frame */
} ChoreoEntry;

#define CHOREO_MAX 32

static struct {
    Mutex        lock;
    ChoreoEntry  q[CHOREO_MAX];
    int          n;
    Thread       thread;
    volatile int running;
    int          started;
    uint64_t     period_ns;
} g_ch;

/* Opaque handle handed back to Unity. Never dereferenced by the engine -- it
 * only ever passes it straight back to us -- but it must be non-NULL, because
 * Unity checks for NULL and falls back to a path we do not want. */
static int g_choreo_token;

static uint64_t ch_now_ns(void)
{
    return armTicksToNs(armGetSystemTick());
}

static void choreo_thread(void *arg)
{
    (void)arg;

    /* This thread invokes Unity's frame callbacks -- module code -- so it needs
     * a bionic TLS block for the same reason aaudio_shim.c's does: every
     * -fstack-protector function in libunity opens with
     * `mrs x21, tpidr_el0 ; ldr x9, [x21, #0x28]`, and a raw threadCreate()
     * thread has tpidr_el0 = 0. That faults at address 0x28.
     *
     * This was latent here, not theoretical: the audio thread hit exactly that
     * crash, and this thread has been calling into the engine since the NDK
     * Choreographer path went live. It survived only because the callbacks it
     * happened to reach were not stack-protected. */
    static uint8_t tls[BIONIC_TLS_SIZE] __attribute__((aligned(16)));
    install_bionic_tls(tls);

    uint64_t next = ch_now_ns();

    while (g_ch.running) {
        next += g_ch.period_ns;
        uint64_t now = ch_now_ns();
        if (next > now)
            svcSleepThread((int64_t)(next - now));
        else
            next = now;                       /* we fell behind; do not spiral */

        /* Drain under the lock into a local copy, then fire OUTSIDE the lock:
         * Unity re-posts from inside its callback, and calling back into
         * postFrameCallback while we still hold the mutex would deadlock. */
        ChoreoEntry local[CHOREO_MAX];
        int n = 0;
        mutexLock(&g_ch.lock);
        n = g_ch.n;
        if (n) {
            memcpy(local, g_ch.q, (size_t)n * sizeof(ChoreoEntry));
            g_ch.n = 0;
        }
        mutexUnlock(&g_ch.lock);

        int64_t t = (int64_t)ch_now_ns();
        for (int i = 0; i < n; i++) {
            if (!local[i].fn) continue;
            if (local[i].is64)
                ((AChoreographer_frameCallback64)local[i].fn)(t, local[i].data);
            else
                ((AChoreographer_frameCallback)local[i].fn)((long)t, local[i].data);
        }
    }
}

static void choreo_start(void)
{
    if (g_ch.started) return;
    g_ch.started = 1;
    mutexInit(&g_ch.lock);

    /* 60 Hz, the Switch panel rate.
     *
     * There is no config.framerate to read: config.c accepts a `framerate`
     * line in config.txt and deliberately discards it (it sits in the
     * known-but-ignored list alongside `widescreen` and `language`), and the
     * Config struct has only handheld_res and docked_res. An earlier draft of
     * this file guarded a read of config.framerate behind an #ifdef that was
     * never defined -- which compiled, and would have quietly meant the guard
     * could never do anything even if the field were added later.
     *
     * A wrong period here does not break rendering. It paces Unity's own
     * timing loop faster or slower than the display, so the game would run
     * smoothly but measure time slightly wrong. */
    const unsigned fps = 60;
    g_ch.period_ns = 1000000000ULL / fps;
    g_ch.running = 1;

    /* Priority just above the main thread so a busy frame cannot starve the
     * tick; 0x2C is the libnx default for the main thread. */
    Result rc = threadCreate(&g_ch.thread, choreo_thread, NULL, NULL,
                             16 * 1024, 0x2B, -2);
    if (R_SUCCEEDED(rc))
        rc = threadStart(&g_ch.thread);
    if (R_FAILED(rc)) {
        g_ch.running = 0;
        debugPrintf("[choreo] thread start FAILED rc=0x%x -- frame callbacks "
                    "will never fire, expect a black screen\n", rc);
    } else {
        debugPrintf("[choreo] dispatcher up at %u Hz (period %llu ns)\n",
                    fps, (unsigned long long)g_ch.period_ns);
    }
}

static int choreo_post(void *fn, void *data, int is64)
{
    if (!fn) return 0;
    choreo_start();
    mutexLock(&g_ch.lock);
    if (g_ch.n >= CHOREO_MAX) {
        /* Should never happen: Unity keeps at most a couple outstanding. If it
         * does, dropping is better than growing without bound. */
        mutexUnlock(&g_ch.lock);
        debugPrintf("[choreo] queue full (%d) -- dropping a frame callback\n",
                    CHOREO_MAX);
        return 0;
    }
    g_ch.q[g_ch.n].fn   = fn;
    g_ch.q[g_ch.n].data = data;
    g_ch.q[g_ch.n].is64 = is64;
    g_ch.n++;
    mutexUnlock(&g_ch.lock);
    return 1;
}

/* ---- the exported NDK surface ------------------------------------------- */

void *AChoreographer_getInstance(void)
{
    choreo_start();
    return &g_choreo_token;
}

void AChoreographer_postFrameCallback(void *choreographer,
                                      AChoreographer_frameCallback cb,
                                      void *data)
{
    (void)choreographer;
    choreo_post((void *)cb, data, 0);
}

void AChoreographer_postFrameCallback64(void *choreographer,
                                        AChoreographer_frameCallback64 cb,
                                        void *data)
{
    (void)choreographer;
    choreo_post((void *)cb, data, 1);
}

/* Delayed variants exist in newer NDK headers. Unity does not reference them
 * in this build (they are not in the undefined-symbol set), but they cost two
 * lines and having them means a game update that starts using them does not
 * fail to load. The delay is ignored -- firing a frame callback early is
 * harmless, never firing it is not. */
void AChoreographer_postFrameCallbackDelayed(void *c,
                                             AChoreographer_frameCallback cb,
                                             void *data, long delayMs)
{
    (void)c; (void)delayMs;
    choreo_post((void *)cb, data, 0);
}

void AChoreographer_postFrameCallbackDelayed64(void *c,
                                               AChoreographer_frameCallback64 cb,
                                               void *data, uint32_t delayMs)
{
    (void)c; (void)delayMs;
    choreo_post((void *)cb, data, 1);
}

void nx_choreographer_stop(void)
{
    if (!g_ch.started || !g_ch.running) return;
    g_ch.running = 0;
    threadWaitForExit(&g_ch.thread);
    threadClose(&g_ch.thread);
    debugPrintf("[choreo] dispatcher stopped\n");
}
