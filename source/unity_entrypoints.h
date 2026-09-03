/* unity_entrypoints.h -- UnityPlayer native methods recovered from
 * libunity.so's own JNINativeMethod tables.
 *
 *   DATA DEFENSE 1.3.11  --  Unity 6000.3.9f1, arm64, IL2CPP
 *   libunity.so build-id 8f235fb696f80ed0b54b51596aebd7d0862f939f
 *
 * Regenerate with:
 *     python3 tools/extract_entrypoints.py libunity.so --header
 *
 * These offsets are LINK-TIME addresses for THIS EXACT libunity.so. Runtime
 * address = unity_mod.load_virtbase + offset.
 *
 * ===========================================================================
 * WHY THIS FILE MATTERS MORE THAN ANY OTHER OFFSET TABLE
 * ===========================================================================
 * This is the drive surface. The loader calls nativeRender once per frame,
 * nativeResume and nativePause on focus changes, initJni at startup,
 * nativeInjectEvent for every touch. They are not exported symbols -- libunity
 * registers them with the JVM inside JNI_OnLoad -- so the only way to reach
 * them is by raw offset.
 *
 * An offset inherited from a different game's libunity.so is a jump into
 * arbitrary code, executed sixty times a second. This file previously carried
 * KILLER BEAN's offsets (build-id 73132d02...), which survived the retarget
 * because they are named OFF_* rather than KB_*. tools/retarget.py now has a
 * leak check that flags any non-zero offset macro outside the verified patch
 * table, specifically so that cannot happen again silently.
 *
 * ===========================================================================
 * TWO DIFFERENCES FROM THE KILLER BEAN TREE, BOTH READ OUT OF THIS BINARY
 * ===========================================================================
 *   1. nativeInjectEvent TAKES THE deviceId.
 *          Killer Bean (2021.3):  (Landroid/view/InputEvent;)Z
 *          Data Defense (6000.3): (Landroid/view/InputEvent;I)Z
 *      The signature comes straight from this binary's JNINativeMethod table,
 *      so it is fact. KB_INJECT_TAKES_DEVICE_ID is 1 here, where the Killer
 *      Bean tree had it 0.
 *
 *      This matters in a direction that would not have crashed. The loader
 *      calls through a 4-argument typedef either way; on Killer Bean the
 *      callee simply ignored w3. Here the callee READS w3, so leaving the gate
 *      at 0 would feed it whatever happened to be in that register as a device
 *      id -- input that works erratically rather than not at all, which is the
 *      worst kind to debug.
 *
 *   2. nativeGetNoWindowMode EXISTS in this build. Killer Bean's 2021.3 did
 *      not register it, so that tree gated it off. KB_HAVE_NO_WINDOW_MODE is 1.
 *
 * NOT REGISTERED in this build, resolved to 0 and must not be called:
 *      nativeLowMemory, nativeRestartActivityIndicator
 */
#ifndef UNITY_ENTRYPOINTS_H
#define UNITY_ENTRYPOINTS_H

#include <stdint.h>
#include "so_util.h"

/* ---- UnityPlayer native method offsets (link-time vaddr) ---------------- */
/* JNI_OnLoad */
#define OFF_JNI_OnLoad                    0xaaea78 /* (JavaVM*,reserved)->jint  caches VM, registers natives */

/* drive-critical */
#define OFF_initJni                       0xaad7cc /* (env,thiz,Context,int type,String cmdline)
                                                   * CONFIRMED for this build: the runtime
                                                   * RegisterNatives table reported initJni at
                                                   * libunity+0xaad7cc, matching this exactly. */
#define OFF_nativeRecreateGfxState        0xaadaa8 /* (env,thiz,int,Surface)  set surface*/
#define OFF_nativeSendSurfaceChangedEvent 0xaadb10 /* (env,thiz)                         */
#define OFF_nativeRender                  0xaadd48 /* (env,thiz)->Z   per-frame; false=stop */
#define OFF_nativeInjectEvent             0xaadda8 /* (env,thiz,InputEvent)->Z  NO deviceId */
#define OFF_nativePause                   0xaad930 /* (env,thiz)->Z                      */
#define OFF_nativeResume                  0xaad994 /* (env,thiz)                         */
#define OFF_nativeFocusChanged            0xaada44 /* (env,thiz,Z)                       */
#define OFF_nativeDone                    0xaad850 /* (env,thiz)->Z   shutdown           */
#define OFF_nativeApplicationUnload       0xaad9f4 /* (env,thiz)                         */
#define OFF_nativeLowMemory               0x0 /* (env,thiz)                         */
#define OFF_nativeOrientationChanged      0xaae8bc /* (env,thiz,int,int)                 */

/* secondary / usually unused for a port */
#define OFF_nativeUnitySendMessage        0xaae3b8 /* (env,thiz,String,String,byte[])    */
#define OFF_nativeMuteMasterAudio         0xaae5e4 /* (env,thiz,Z)                       */
#define OFF_nativeIsAutorotationOn        0xaae584 /* (env,thiz)->Z                      */
#define OFF_nativeSetLaunchURL            0xaae648 /* (env,thiz,String)                  */
#define OFF_nativeRestartActivityIndicator 0x0 /* (env,thiz)                        */

/* soft keyboard (route via SoftInputProvider stub; not needed for first boot) */
#define OFF_nativeSetInputArea            0xaae068 /* (env,thiz,I,I,I,I)                 */
#define OFF_nativeSetKeyboardIsVisible    0xaae0f0
#define OFF_nativeSetInputString          0xaae150
#define OFF_nativeSetInputSelection       0xaae1f8
#define OFF_nativeSoftInputClosed         0xaae360
#define OFF_nativeSoftInputCanceled       0xaae268
#define OFF_nativeSoftInputLostFocus      0xaae2c0
#define OFF_nativeReportKeyboardConfigChanged 0xaae318

/* ---- 2021.3.31f1 compatibility shims ----------------------------------- */
/* This engine registers nativeSendSurfaceChangedEvent, not the later
 * nativeSendSurfaceChanged. The core only ever calls the *Event form; the
 * alias keeps any stray reference compiling. */
#define OFF_nativeSendSurfaceChanged      OFF_nativeSendSurfaceChangedEvent

/* nativeHidePreservedContent does not exist in 2021.3. The loader core
 * declares it but never calls it. Resolving to 0 makes an accidental call
 * fail loudly instead of jumping into the middle of an unrelated function. */
#define OFF_nativeHidePreservedContent    0xaae804 /* ()V  present in this build */
#define KB_HAVE_HIDE_PRESERVED_CONTENT    0

/* nativeGetNoWindowMode is likewise ABSENT in 2021.3 (2022.3 has it). Same
 * treatment: resolve to 0 and gate. Nothing in the loader calls it, but the
 * gate is here so a future edit that does call it fails loudly rather than
 * jumping to libunity_base+0. */
#define OFF_nativeGetNoWindowMode         0xaae970 /* ()Z  present in this build */
#define KB_HAVE_NO_WINDOW_MODE            1

/* nativeInjectEvent's real arity, for anyone auditing the call. 0 == the
 * 2021.3 one-argument form. The loader intentionally calls through the
 * 4-argument typedef anyway; see the VERSION NOTE for why that is safe. */
#define KB_INJECT_TAKES_DEVICE_ID         1

/* ===========================================================================
 * OFFSETS VERIFIED AGAINST A REAL BOOT
 * ===========================================================================
 * Every OFF_ below was checked against the RegisterNatives table the engine
 * built at runtime (jni_fake.c logs it as [natives] lines). 25 of 25 matched
 * exactly. The three that are not registered -- JNI_OnLoad, nativeLowMemory,
 * nativeRestartActivityIndicator -- are not JNI natives at all or are absent
 * from this build, and are zeroed where absent.
 *
 * WHERE THE LIFECYCLE NATIVES LIVE IN UNITY 6. They are no longer on
 * UnityPlayer. nativeRender, nativeRecreateGfxState, nativeSendSurfaceChanged-
 * Event, nativeResume, nativePause, nativeFocusChanged and nativeDone are all
 * registered by:
 *
 *     com/unity3d/player/UnityPlayerForActivityOrService
 *
 * which is a useful confirmation in its own right: it is the class Unity picks
 * for context type 0, the value initJni is now passed below. UnityPlayer keeps
 * only initJni, nativeInjectEvent, nativeApplicationUnload and the odds and
 * ends.
 * ===========================================================================
 */

/* ---- JNI native signatures: ret (*)(JNIEnv*, jobject thiz, args...) ----- */

/* initJni GAINED TWO PARAMETERS IN UNITY 6. This is what crashed the first
 * boot, and it is worth spelling out because nothing about it looks wrong.
 *
 *   Unity 2021.3 (Killer Bean):  initJni(Landroid/content/Context;)V
 *   Unity 6000.3 (this game):    initJni(Landroid/content/Context;ILjava/lang/String;)V
 *
 * The extra int is a CONTEXT TYPE and the String is a command line. Calling
 * through the old three-argument typedef left w3 and x4 holding whatever the
 * previous code had put there, so Unity read a random integer as the context
 * type:
 *
 *     Unity: Context Type: Unknown (541296588)
 *
 * and took its unknown-context error path, which re-entered and overflowed the
 * main thread's 1 MB stack after ~8,000 rounds.
 *
 * The AArch64 calling convention is why this was silent rather than an
 * immediate fault: surplus arguments simply are not passed, and missing ones
 * are read as garbage from whatever the registers held. There is no
 * diagnostic. The three-arg call compiled and ran; it just lied.
 *
 * Values, read out of initJni's own dispatch at reference +0xf11ba0:
 *     w3 == 0  ->  "Context Type: ActivityOrService"   <- what we are
 *     w3 == 1  ->  "Context Type: GameActivity"
 *     else     ->  "Context Type: Unknown (%d)"        <- the crash path
 *
 * The jstring is the command line; it reaches DVM::SetupCommandline, which
 * null-checks it at +0x2c (`cbz x19`) and skips the append. NULL is therefore
 * both safe and correct -- we have no command line to pass. */
typedef void     (*fn_initJni)(void*,void*,void*,int32_t,void*);
typedef void     (*fn_gfxstate)(void*,void*,int32_t,void*);
typedef void     (*fn_v)(void*,void*);
typedef uint8_t  (*fn_z)(void*,void*);
typedef void     (*fn_vz)(void*,void*,int32_t);
typedef uint8_t  (*fn_inject)(void*,void*,void*,int32_t); /* 4th arg ignored by 2021.3 */
typedef void     (*fn_orient)(void*,void*,int32_t,int32_t);

#define UNITY_RESOLVE(mod, off) ((void*)((uintptr_t)(mod).load_virtbase + (off)))

/* ===========================================================================
 * Drive sequence (what the Java UnityPlayer does; main.c does it here):
 *
 *   initJni(env, thiz, fake_context);                  // early init
 *   nativeRecreateGfxState(env, thiz, 0, fake_surface);// give it the surface
 *   nativeSendSurfaceChangedEvent(env, thiz);          // engine builds GL state
 *   for (;;) {
 *       // input: nativeInjectEvent(env, thiz, motionEvent);
 *       if (!nativeRender(env, thiz)) break;           // false == engine wants out
 *   }
 *   nativeApplicationUnload(env, thiz);  nativeDone(env, thiz);
 *
 * NOTE on input: nativeInjectEvent takes a Java InputEvent/MotionEvent jobject,
 * which the engine then queries back via JNI (getActionMasked/getX/getY/
 * getPointerId/getPointerCount...). unity_input.c fabricates that object.
 *
 * Killer Bean Unleashed drives gameplay through Corgi Engine's on-screen
 * MMTouchButton / MMTouchJoystick widgets, which are ordinary uGUI elements
 * fed by this same MotionEvent path. So this entry point is as load-bearing
 * here as it was for Fruit Ninja -- but the requirement differs: Fruit Ninja
 * needed sub-frame swipe sampling for blade trails, whereas this game needs
 * accurate, *sustained* press state on small on-screen widgets. One pointer
 * sample per frame is fine; a dropped ACTION_UP is not, because a stuck
 * "fire" or "move right" is unrecoverable without another press.
 * See PORTING_KILLERBEAN.md sec 6.
 * =========================================================================== */

/* ---- Non-UnityPlayer native tables also present in this build (FYI) -------
 * We do NOT register/drive these; listed only so nobody re-hunts them.
 *   choreographer   nOnChoreographer                     @0x98d6f4
 *   swappy          nOnRefreshPeriodChanged              @0x987558
 *                   nSetSupportedRefreshPeriods          @0x987378
 *   ARCore          initializeARCore/pause/resume        @0x3924f4/0x392558/0x3925a0
 *   Camera2         initCamera2Jni/deinit                @0x362d94/0x362de0
 *                   nativeFrameReady/SurfaceTextureReady @0x369d34/0x369be0
 *   HFP audio       initHFPStatusJni/deinit              @0x38d26c/0x38d2b8
 *   audio volume    onAudioVolumeChanged                 @0x38f83c
 *   query status    nativeStatusQueryResult              @0x378ae0
 *   orient lock     nativeUpdateOrientationLockState     @0x36c9f8
 * -------------------------------------------------------------------------- */

#endif /* UNITY_ENTRYPOINTS_H */
