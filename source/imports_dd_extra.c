/* imports_dd_extra.c -- the import surface DATA DEFENSE needs on top of the
 * loader core's tables.
 *
 * DERIVATION (tools/imports_check.py reproduces this exactly)
 *   distinct undefined dynamic symbols across the three modules ... 500
 *     (libunity 430, libil2cpp 276, libmain 12, heavily overlapping)
 *   already resolved by imports.c / unity_imports.c / libc_shim.c /
 *   opensles.c / ndk_stubs.c / cr3_stubs.c / firebase_stub.c ...... 480
 *   remainder, handled here ........................................ 20
 *
 * Twenty is a remarkably small gap for an engine three Unity majors newer than
 * the tree this was forked from, and it is the main reason this port is worth
 * attempting at all. They fall into three groups:
 *
 *  1. NDK CHOREOGRAPHER (4 syms). The only group that is real work, and the
 *     only one that can silently kill the boot. Unity 6 drives frame timing
 *     through the NDK Choreographer instead of the Java one that jni_fake.c
 *     answers for the older ports. Implemented for real in ndk_choreographer.c
 *     -- see the long comment there for why a stub is not acceptable.
 *
 *  2. PLAIN LIBC (13 syms). newlib already has every one of these; the loader's
 *     table simply never listed them because no previous game referenced them.
 *     Passthroughs, nothing more.
 *
 *  3. ANDROID-ONLY ODDS AND ENDS (3 syms). Logging, an NDK media getter on a
 *     path this game cannot reach (no WebCamTexture / VideoPlayer in a 2D
 *     tower defense), and process_vm_readv, which only the Android crash
 *     reporter uses. Stubbed, each logging once if it is ever actually called
 *     so debug.log tells you an assumption was wrong.
 *
 * Wired into the combined table in imports.c the same way the Killer Bean
 * extras were -- the *_functions / *_numfunctions naming is the loader core's
 * convention, not a choice.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

/* strcasestr is a GNU extension; newlib only declares it under __GNU_VISIBLE.
 * Must come before any include. */
#define _GNU_SOURCE 1

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>   /* also provides pid_t / ssize_t; <sys/types.h> is the
                       * one header nothing else in this tree includes, so it
                       * is not worth relying on here. */

#include "so_util.h"

/* devkitA64's newlib has no <sys/uio.h>. libc_shim.c already hit this and
 * declares the layout locally; use the same shape rather than inventing a
 * second one. The kernel iovec is just {ptr, len}, and bionic's matches. */
struct nx_iovec { void *iov_base; size_t iov_len; };

extern void debugPrintf(const char *fmt, ...);

/* Log once per stub, so a stub that turns out to be live is visible in
 * debug.log without flooding it. */
#define NOTSUP(name)                                                          \
    do {                                                                      \
        static int _warned = 0;                                               \
        if (!_warned) {                                                       \
            _warned = 1;                                                      \
            debugPrintf("[dd-extra] %s() called -- stubbed. If the game "     \
                        "misbehaves near this, it needs a real one.\n", name);\
        }                                                                     \
    } while (0)

/* ---- group 1: NDK Choreographer (implemented in ndk_choreographer.c) ----- */

void *AChoreographer_getInstance(void);
void  AChoreographer_postFrameCallback(void *, void *, void *);
void  AChoreographer_postFrameCallback64(void *, void *, void *);
void  AChoreographer_postFrameCallbackDelayed(void *, void *, void *, long);

/* ---- group 2: plain libc passthroughs ----------------------------------- */
/* Declared here rather than pulled from headers in every case because a few
 * (fseeko64/ftello64) are glibc LFS aliases newlib spells without the suffix. */

static int nx_fseeko64(FILE *f, int64_t off, int whence)
{
    return fseek(f, (long)off, whence);
}

static int64_t nx_ftello64(FILE *f)
{
    return (int64_t)ftell(f);
}

/* execl: there is no process to exec into. Fail like a sandboxed Android app
 * rather than pretending to succeed. */
static int nx_execl(const char *path, const char *arg, ...)
{
    (void)path; (void)arg;
    NOTSUP("execl");
    errno = ENOSYS;
    return -1;
}

/* strcasestr: the only GNU-only name in this file. newlib exposes it solely
 * under __GNU_VISIBLE, and if a given newlib build does not carry it the
 * failure is an unresolved symbol at link time -- after a full rebuild, with
 * nothing pointing at this file. Implementing it is ten lines and removes the
 * question entirely. */
static char *nx_strcasestr(const char *hay, const char *needle)
{
    if (!hay || !needle) return NULL;
    if (!*needle) return (char *)hay;
    for (; *hay; hay++) {
        const char *h = hay, *n = needle;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++; n++;
        }
        if (!*n) return (char *)hay;
    }
    return NULL;
}

/* GNU strerror_r returns char*, unlike the POSIX int-returning one newlib
 * exposes. Getting this backwards yields a garbage error string, which is a
 * miserable thing to debug from a log. */
static char *nx_gnu_strerror_r(int err, char *buf, size_t buflen)
{
    const char *s = strerror(err);
    if (buf && buflen) {
        strncpy(buf, s ? s : "unknown", buflen - 1);
        buf[buflen - 1] = '\0';
        return buf;
    }
    return (char *)s;
}

static ssize_t nx_writev(int fd, const struct nx_iovec *iov, int iovcnt)
{
    ssize_t total = 0;
    for (int i = 0; i < iovcnt; i++) {
        ssize_t n = write(fd, iov[i].iov_base, iov[i].iov_len);
        if (n < 0) return total ? total : -1;
        total += n;
        if ((size_t)n != iov[i].iov_len) break;
    }
    return total;
}

/* ---- group 3: Android-only ---------------------------------------------- */

static int nx_android_log_buf_write(int bufID, int prio, const char *tag,
                                    const char *text)
{
    (void)bufID; (void)prio;
    debugPrintf("[andlog] %s: %s\n", tag ? tag : "?", text ? text : "");
    return 0;
}

/* AImage_getWidth: the Camera2/AImageReader capture path. A 2D tower defense
 * has no WebCamTexture, so this is link-satisfying only. Returns an NDK media
 * error rather than 0, so any caller takes its failure branch immediately
 * instead of proceeding with a zero-width image. */
static int nx_AImage_getWidth(void *image, int32_t *width)
{
    (void)image;
    NOTSUP("AImage_getWidth");
    if (width) *width = 0;
    return -10000;                     /* AMEDIA_ERROR_BASE */
}

/* ALooper_removeFd: our looper has no real fds registered, so there is nothing
 * to remove. 1 means "removed", which is the answer that keeps callers happy. */
static int nx_ALooper_removeFd(void *looper, int fd)
{
    (void)looper; (void)fd;
    return 1;
}

/* process_vm_readv -- NOT a stub, deliberately.
 *
 * The first draft of this file stubbed it to ENOSYS on the reasoning that
 * there is no other process to read from. libc_shim.c had already been through
 * that and its comment records the outcome: "Stubbing it to ENOSYS made the
 * caller spin once per frame (a process_vm_readv flood), wedging the boot
 * path." The engine uses it as a fault-safe read probe on its OWN memory, so
 * failing it does not make the caller give up -- it makes it retry forever.
 *
 * syscall_fake() already implements the self-process case properly: it
 * validates every remote range with svcQueryMemory and copies the readable
 * parts. Route the exported symbol into that rather than reimplementing it,
 * so there is one behaviour and one place to fix it. */
long syscall_fake(long number, ...);
#define NX_SYS_PROCESS_VM_READV 270

static ssize_t nx_process_vm_readv(pid_t pid, const struct nx_iovec *lv,
                                   unsigned long ln, const struct nx_iovec *rv,
                                   unsigned long rn, unsigned long flags)
{
    (void)flags;
    return (ssize_t)syscall_fake(NX_SYS_PROCESS_VM_READV,
                                 (long)pid, lv, ln, rv, rn);
}

/* ------------------------------------------------------------------------- */


/* ---- AAudio (aaudio_shim.c) ---------------------------------------------
 * FMOD dlopen()s libaaudio.so and dlsym()s these. dlsym_fake falls through to
 * this table, so registering them here serves both the dlsym path and any
 * direct import. The list is exactly the 27 AAudio* strings present in this
 * libunity.so -- nothing speculative. */
int32_t AAudio_createStreamBuilder(void **);
void AAudioStreamBuilder_setSampleRate(void *, int32_t);
void AAudioStreamBuilder_setChannelCount(void *, int32_t);
void AAudioStreamBuilder_setFormat(void *, int32_t);
void AAudioStreamBuilder_setDirection(void *, int32_t);
void AAudioStreamBuilder_setPerformanceMode(void *, int32_t);
void AAudioStreamBuilder_setSharingMode(void *, int32_t);
void AAudioStreamBuilder_setDeviceId(void *, int32_t);
void AAudioStreamBuilder_setSessionId(void *, int32_t);
void AAudioStreamBuilder_setBufferCapacityInFrames(void *, int32_t);
void AAudioStreamBuilder_setFramesPerDataCallback(void *, int32_t);
void AAudioStreamBuilder_setDataCallback(void *, void *, void *);
void AAudioStreamBuilder_setErrorCallback(void *, void *, void *);
void AAudioStreamBuilder_delete(void *);
int32_t AAudioStreamBuilder_openStream(void *, void **);
int32_t AAudioStream_requestStart(void *);
int32_t AAudioStream_requestStop(void *);
int32_t AAudioStream_close(void *);
int32_t AAudioStream_waitForStateChange(void *, int32_t, int32_t *, int64_t);
int32_t AAudioStream_getBufferCapacityInFrames(void *);
int32_t AAudioStream_getBufferSizeInFrames(void *);
int32_t AAudioStream_setBufferSizeInFrames(void *, int32_t);
int32_t AAudioStream_getFramesPerBurst(void *);
int32_t AAudioStream_getXRunCount(void *);
int32_t AAudioStream_getDeviceId(void *);
int32_t AAudioStream_getSessionId(void *);
int32_t AAudioStream_isMMapUsed(void *);
int32_t AAudioStream_getSampleRate(void *);
int32_t AAudioStream_getChannelCount(void *);
int32_t AAudioStream_getFormat(void *);

DynLibFunction dd_extra_functions[] = {
    /* 1. NDK Choreographer -- Unity 6 frame pacing. Do not stub these. */
    { "AChoreographer_getInstance",           (uintptr_t)&AChoreographer_getInstance },
    { "AChoreographer_postFrameCallback",     (uintptr_t)&AChoreographer_postFrameCallback },
    { "AChoreographer_postFrameCallback64",   (uintptr_t)&AChoreographer_postFrameCallback64 },
    { "AChoreographer_postFrameCallbackDelayed",
                                              (uintptr_t)&AChoreographer_postFrameCallbackDelayed },

    /* AAudio -- the output FMOD actually has a plugin for on this build */
    { "AAudio_createStreamBuilder", (uintptr_t)&AAudio_createStreamBuilder },
    { "AAudioStreamBuilder_setSampleRate", (uintptr_t)&AAudioStreamBuilder_setSampleRate },
    { "AAudioStreamBuilder_setChannelCount", (uintptr_t)&AAudioStreamBuilder_setChannelCount },
    { "AAudioStreamBuilder_setFormat", (uintptr_t)&AAudioStreamBuilder_setFormat },
    { "AAudioStreamBuilder_setDirection", (uintptr_t)&AAudioStreamBuilder_setDirection },
    { "AAudioStreamBuilder_setPerformanceMode", (uintptr_t)&AAudioStreamBuilder_setPerformanceMode },
    { "AAudioStreamBuilder_setSharingMode", (uintptr_t)&AAudioStreamBuilder_setSharingMode },
    { "AAudioStreamBuilder_setDeviceId", (uintptr_t)&AAudioStreamBuilder_setDeviceId },
    { "AAudioStreamBuilder_setSessionId", (uintptr_t)&AAudioStreamBuilder_setSessionId },
    { "AAudioStreamBuilder_setBufferCapacityInFrames", (uintptr_t)&AAudioStreamBuilder_setBufferCapacityInFrames },
    { "AAudioStreamBuilder_setFramesPerDataCallback", (uintptr_t)&AAudioStreamBuilder_setFramesPerDataCallback },
    { "AAudioStreamBuilder_setDataCallback", (uintptr_t)&AAudioStreamBuilder_setDataCallback },
    { "AAudioStreamBuilder_setErrorCallback", (uintptr_t)&AAudioStreamBuilder_setErrorCallback },
    { "AAudioStreamBuilder_delete", (uintptr_t)&AAudioStreamBuilder_delete },
    { "AAudioStreamBuilder_openStream", (uintptr_t)&AAudioStreamBuilder_openStream },
    { "AAudioStream_requestStart", (uintptr_t)&AAudioStream_requestStart },
    { "AAudioStream_requestStop", (uintptr_t)&AAudioStream_requestStop },
    { "AAudioStream_close", (uintptr_t)&AAudioStream_close },
    { "AAudioStream_waitForStateChange", (uintptr_t)&AAudioStream_waitForStateChange },
    { "AAudioStream_getBufferCapacityInFrames", (uintptr_t)&AAudioStream_getBufferCapacityInFrames },
    { "AAudioStream_getBufferSizeInFrames", (uintptr_t)&AAudioStream_getBufferSizeInFrames },
    { "AAudioStream_setBufferSizeInFrames", (uintptr_t)&AAudioStream_setBufferSizeInFrames },
    { "AAudioStream_getFramesPerBurst", (uintptr_t)&AAudioStream_getFramesPerBurst },
    { "AAudioStream_getXRunCount", (uintptr_t)&AAudioStream_getXRunCount },
    { "AAudioStream_getDeviceId", (uintptr_t)&AAudioStream_getDeviceId },
    { "AAudioStream_getSessionId", (uintptr_t)&AAudioStream_getSessionId },
    { "AAudioStream_isMMapUsed", (uintptr_t)&AAudioStream_isMMapUsed },
    { "AAudioStream_getSampleRate", (uintptr_t)&AAudioStream_getSampleRate },
    { "AAudioStream_getChannelCount", (uintptr_t)&AAudioStream_getChannelCount },
    { "AAudioStream_getFormat", (uintptr_t)&AAudioStream_getFormat },

    /* 2. libc passthroughs newlib already provides */
    { "exp2",        (uintptr_t)&exp2        },
    { "hypotf",      (uintptr_t)&hypotf      },
    { "nearbyintf",  (uintptr_t)&nearbyintf  },
    { "nextafter",   (uintptr_t)&nextafter   },
    { "remainderf",  (uintptr_t)&remainderf  },
    { "tanhf",       (uintptr_t)&tanhf       },
    { "strcasestr",  (uintptr_t)&nx_strcasestr },
    { "strncat",     (uintptr_t)&strncat     },
    { "fseeko64",    (uintptr_t)&nx_fseeko64 },
    { "ftello64",    (uintptr_t)&nx_ftello64 },
    { "writev",      (uintptr_t)&nx_writev   },
    { "execl",       (uintptr_t)&nx_execl    },
    { "__gnu_strerror_r", (uintptr_t)&nx_gnu_strerror_r },

    /* 3. Android-only */
    { "__android_log_buf_write", (uintptr_t)&nx_android_log_buf_write },
    { "AImage_getWidth",         (uintptr_t)&nx_AImage_getWidth       },
    { "ALooper_removeFd",        (uintptr_t)&nx_ALooper_removeFd      },
    { "process_vm_readv",        (uintptr_t)&nx_process_vm_readv      },
};

size_t dd_extra_numfunctions =
    sizeof(dd_extra_functions) / sizeof(dd_extra_functions[0]);
