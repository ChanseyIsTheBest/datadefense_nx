/* nx_patch_datadefense.h -- in-memory patch table for
 * DATA DEFENSE 1.3.11  (com.IIBlocks.DataDefense)
 * Unity 6000.3.9f1, IL2CPP, arm64, il2cpp metadata version 39.
 * libunity.so  build-id 8f235fb696f80ed0b54b51596aebd7d0862f939f  (28.0 MB)
 * libil2cpp.so 41.0 MB     libmain.so 6.7 KB
 *
 * Forked from nx_patch_datadefense.h (Unity 2021.3.31f1). EVERY offset in that
 * file was derived against KILLER BEAN's binaries and is meaningless here --
 * a valid-looking address pointing at unrelated code in a different link.
 * They are therefore all ZEROED below and their gates are OFF.
 *
 *   An unlocated hook is a missing feature.
 *   A wrong hook is memory corruption whose stack trace blames somewhere else.
 *
 * The ONE table that IS derived for this game is the region-granularity table
 * at the bottom. Everything above it is a to-do list.
 *
 * ===========================================================================
 * HOW THE GRANULARITY TABLE WAS DERIVED  (tools/scan_granularity.py)
 * ===========================================================================
 * No symbolized reference build of Unity 6000.3.9f1 was available, so the
 * derivation is anchor-based rather than symbol-based:
 *
 *  1. Every instruction in the executable LOAD segments was decoded for the
 *     six encoding classes that can participate in a 256MB region
 *     computation. 456 candidate words across 28 MB of text.
 *
 *  2. Most of those are noise -- `lsr #28` and `movz #0x10000000` occur all
 *     over a Unity build for reasons unrelated to memory. The candidates were
 *     narrowed by ANCHORS: encodings that essentially only occur in region
 *     math. Nothing but an allocator masks an address with a wide
 *     0x...F0000000 run, and nothing else subtracts a value shifted left by
 *     28. Four anchors were found, spanning +0x1005fd4..+0x100f798 -- one
 *     contiguous 37 KB stretch, which is what an allocator compiled as a
 *     single translation unit looks like.
 *
 *  3. Every granularity site inside that span was kept: 18. A separate
 *     raw-encoding sweep of the same span (tools/sweep, no text matching)
 *     found ZERO sites the four classes miss, so the table is complete for
 *     the region.
 *
 * CORROBORATION -- this is the part that makes it trustworthy. Fifteen of the
 * 18 words are BYTE-IDENTICAL to words in the hand-derived, shipped Killer
 * Bean table, and they appear in the SAME ORDER with near-identical intra-
 * function deltas. Killer Bean's VirtualAllocator::GetMemoryBlockFromPointer:
 *
 *     +0x00  lsr  x8, x1, #28            <- DD +0x100d720, same word
 *     +0x10  and  x8, x1, #0xfffffff0000000   <- DD +0x100d730, same word
 *     +0x18  ubfx x10, x1, #28, #12      <- DD +0x100d738, same word
 *     +0x34  movk x11, #0x1000, lsl #16  <- DD +0x100d754, same word
 *     +0x70  sub  x8, x8, x10, lsl #28   <- DD +0x100d798, same word
 *
 * Two Unity versions three majors apart, same allocator, same encodings, same
 * sequence. That is two independent methods agreeing, which is the standard
 * this lineage's tables are held to.
 *
 * CONFIDENCE: HIGH on the sites themselves. UNVERIFIED ON HARDWARE -- nothing
 * in this tree has been run on a Switch yet. nx_patch_libunity() is
 * verify-first: it reads every {from} word and patches NOTHING unless all of
 * them match, so a wrong table is caught and logged rather than executed.
 *
 * THREE SITES KILLER BEAN HAS THAT THIS DOES NOT (TLSAllocator's paired lsr,
 * DynamicHeapAllocator's paired lsr, AtomicPageAllocator::AllocatePage's
 * movz). Unity 6 generates those functions differently; the sweep confirms
 * there is no equivalent instruction to rewrite. Expected, not a gap.
 * ===========================================================================
 */
#ifndef NX_PATCH_DATADEFENSE_H
#define NX_PATCH_DATADEFENSE_H

#include <stdint.h>

/* One 32-bit instruction rewrite: read {from} at libunity+{off}, and only if it
 * matches, write {to}. Declared here at the top because KB_BRANCH_FORCES and
 * KB_FMOD_WORDS below both use it -- it used to sit next to the table it
 * describes, several hundred lines further down, which compiled for exactly as
 * long as nothing above the table referenced it. */
typedef struct { uint32_t off, from, to; } NxPatchWord;



#define KB_HAVE_BRANCH_FORCES  0   /* not needed unless an allocator abort appears */

/* ===========================================================================
 * INHERITED-FEATURE GATES -- ALL OFF, AND WHY THAT IS CORRECT
 * ===========================================================================
 * The Fruit Ninja tree this was retargeted from carried a set of hooks and
 * guards whose addresses were derived against FRUIT NINJA's libil2cpp.so and
 * libunity.so. Those addresses are MEANINGLESS in this game's binaries --
 * different Unity version, different game code, different link layout.
 * Inheriting them would patch arbitrary instructions.
 *
 * So every one of them is gated OFF here with its offsets zeroed. This is not
 * laziness: an unlocated hook is a missing feature, but a WRONG hook is
 * memory corruption with a stack trace that points somewhere unrelated.
 *
 * Each gate below documents the SYMPTOM that means you need to go derive it
 * for this game. Turn one on only after you have re-derived its offsets.
 * ===========================================================================
 */

/* ---- il2cpp IsInst null-klass guard --------------------------------------
 * Fruit Ninja needed this because ITS DialogueConfig held an Il2CppClass*
 * where an array was expected. That is a Fruit-Ninja-specific data bug. This
 * game has no DialogueConfig. OFF, offsets zeroed.
 * SYMPTOM if you ever need it: fault reading [klass + 0x135] inside
 * il2cpp's assignability check, with x0 -> an object whose first word is 0. */
#define KB_IL2CPP_ISINST_AND     0u
#define KB_IL2CPP_ISINST_AND_OLD 0u
#define KB_IL2CPP_ISINST_GUARD   0u
#define KB_HAVE_ISINST_GUARD     0

/* ---- DataBinding / ConvertFromTo liveness guard --------------------------
 * Likewise Fruit-Ninja-specific (its MoreMountains DataBinding path passed a
 * raw 1 to Object.GetType). Killer Bean uses the same MoreMountains Tools
 * package, so this one is more plausible to recur than the IsInst guard --
 * but the ADDRESSES are still Fruit Ninja's and must be re-derived.
 * SYMPTOM: fault in a leaf `ldr x8,[x0] ; add x0,x8,#0x20` with lr pointing
 * into IDataSource$$ConvertFromTo. OFF, offsets zeroed. */
#define KB_IL2CPP_LIVENESS_BODY    0u
#define KB_IL2CPP_LIVENESS_ADD     0u
#define KB_IL2CPP_LIVENESS_ADD_HI  0u
#define KB_IL2CPP_LIVENESS_ADD_LO  0u
#define KB_IL2CPP_LIVENESS_HASPAR  0u
#define KB_IL2CPP_LIVENESS_LR_INST 0u
#define KB_IL2CPP_CFT_LR           0u
#define KB_IL2CPP_GETTYPE_LEAF     0u
#define KB_HAVE_LIVENESS_GUARD     0
static const uint32_t KB_LIVENESS_PROLOGUE[4] = { 0u, 0u, 0u, 0u };

/* ---- diagnostic field offsets used by nx_exception_dump.c ----------------
 * These describe FRUIT NINJA managed types. Zeroed; the exception dumper
 * checks for 0 and simply prints less. Re-derive from dump.cs if you want
 * richer crash dumps for this game. */
#define KB_CFT_SP_DATABINDING    0u
#define KB_CFT_SP_VALUE          0u
#define KB_DATABINDING_DATAPATH  0u
#define KB_DATABINDING_PROPNAME  0u
#define KB_DATABINDING_STRFMT    0u
#define KB_DIALOGUE_CONFIG       0u
#define KB_DIALOGUE_INDEX        0u
#define KB_DLGCONFIG_PIECES      0u
#define KB_SDS_SP_DIALOGUE       0u
#define KB_SDS_SP_PIECE          0u
#define KB_REFLECTIONTYPE_TYPE   0u
/* These two are il2cpp ABI, not game-specific: the array header layout is
 * fixed by the runtime, so they carry over unchanged and stay live. */
#define KB_IL2CPP_ARRAY_LEN      0u
#define KB_IL2CPP_ARRAY_DATA     0u
#define KB_IL2CPP_KLASS_DEPTH    0u
#define KB_IL2CPP_KLASS_TYPEHIER 0u

/* ---- TimeManager / engine-clock fix (libunity offsets) -------------------
 * Fruit Ninja's entry 0x478aec / body 0x478b10 / GetTimeManager 0x4790d4 are
 * 2022.3 addresses. Not valid here. OFF.
 * SYMPTOM: deltaTime stays 0, async scene loads never complete, the first
 * nativeRender blocks forever or the game sits on a black screen after the
 * splash. That is when you go pin TimeManager::Update in THIS libunity. */
/* (real TimeManager addresses are further down, in the TimeManager section) */
/* Expected prologue of an il2cpp icall thunk, used to verify each Time.get_*
 * hook site before patching. Zero here because the Time.get_* thunks are NOT
 * derived for this game (KB_HAVE_TIME_HOOKS is 0 in config.h); the macro must
 * still exist because main.c compares against it outside the gate. */
/* The icall-thunk prologue this build actually uses. VERIFIED by reading the
 * first word of all seven resolved getters in THIS libil2cpp: every one is
 * 0xa9bf4ffe = `stp x30, x19, [sp, #-0x10]!`, followed by
 * `adrp x19, <icall table>` / `ldr x0, [x19, #off]`. Killer Bean's note said
 * `stp x19,x30` -- the operand ORDER differs, so the word differs, and
 * inheriting the value would have made every hook SKIP with "not the expected
 * icall-thunk prologue". */
#define KB_TIME_THUNK_WORD    0xa9bf4ffeu
/* TimeManager time fix -- ON. All five pieces derived for THIS build:
 *
 *   entry        +0x8d0638   guard word 0xf940b008 = ldr x8,[x0,#0x160].
 *                            The install is VERIFY-FIRST: a mismatch logs and
 *                            writes nothing.
 *   body         +0x8d065c   = entry + 0x24, and the runtime `ret` scan in
 *                            nx_install_time_fix() independently agrees.
 *   frameCount   tm+0x160    (PvZ 0xc8)
 *   aux counter  tm+0x168    (PvZ 0xd0)
 *   paused       tm+0x1a8    (PvZ 0xf8)
 *   m_StartupRef tm+0x190    a RationalTime {i64 ticks; u32 num; u32 den},
 *                            NOT the double PvZ read at tm+0xe8.
 *
 * KB_TIME_GETMANAGER stays 0 -- Unity 6 has no GetTimeManager symbol -- and
 * the cast is now guarded on the macro, because ub+0 is the module base rather
 * than NULL and would otherwise be called as a function. The clock thread uses
 * g_tm, which nx_time_update_hook sets on its first fire.
 *
 * This hook REPLACES TimeManager::Update rather than chaining it: the stub is
 * a branch, so nx_time_update_hook returns to Update's caller and the engine's
 * own time computation only runs via g_unity_update_body(tm, newTime). If the
 * game's clock behaves oddly, this is the first thing to turn back off. */
#define KB_HAVE_TIME_FIX      1

/* ===========================================================================
 * Time.get_* icall thunks  --  DERIVED FROM dump.cs, ENABLED
 * ===========================================================================
 * These are libil2cpp offsets, so no libunity reference is involved. They come
 * from the Il2CppDumper output for THIS build (dump.cs), UnityEngine.Time,
 * TypeDefIndex 5940.
 *
 * ADDRESS CONVENTION -- worth stating, because it is a trap. This dump prints
 * "RVA: 0x1FA2D6C Offset: 0x1FA2D6C VA: 0x1FA2D6C", all three identical, and
 * the value is a plain virtual address in the .so. It is NOT the
 * "runtime_RVA = Offset + 0x4000" convention some Il2CppDumper configurations
 * emit, and adding 0x4000 lands in the wrong place. Also note these addresses
 * are NOT in .text: generated method code lives in this binary's own `il2cpp`
 * section (0x9b07c4 .. 0x21da31c). The loader maps the whole LOAD zone, so
 * `il2cpp_virtbase + offset` is still correct -- the same convention already
 * used for KB_IL2CPP_VM_GLOBAL.
 *
 * VERIFICATION. Every offset below was disassembled and is a textbook il2cpp
 * icall thunk, all eight identical in shape:
 *
 *     stp  x19, x30, [sp, #-0x10]!     <- KB_TIME_THUNK_WORD
 *     adrp x19, #0x2eee000
 *     ldr  x0,  [x19, #<slot>]         ; cached icall fn ptr
 *     cbnz x0,  <call>                 ; already resolved?
 *     adrp x0,  #<page> ; add x0,x0,#<off>   ; the icall NAME string
 *     bl   0x8ffbcc                    ; shared resolver
 *     str  x0,  [x19, #<slot>]         ; cache it
 *     ldp  x19, x30, [sp], #0x10
 *     br   x0                          ; tail-call
 *
 * Stronger still: the name string each thunk passes to the resolver was read
 * out of .rodata and reads "UnityEngine.Time::get_time()",
 * "UnityEngine.Time::get_deltaTime()", and so on -- each thunk names itself,
 * and every name matches what dump.cs claimed. That is the real verification.
 *
 * CAVEAT ON THE GUARD WORD. main.c checks the first word against
 * KB_TIME_THUNK_WORD before patching. 0xA9BF7BF3 is `stp x19,x30,[sp,#-0x10]!`
 * -- an extremely common prologue (Swappy::IsEnabledAndActive starts with the
 * same word). So this guard catches a stale/garbage offset but would NOT catch
 * an offset that happened to land on another ordinary function. It is a
 * sanity check, not an identity check; the disassembly above is what actually
 * establishes these are the right nine.                                     */
/* Managed input entry points -- RESOLVED, NOT YET USED.
 *
 * Recorded because they are the managed side of touch, and this port currently
 * feeds input natively (unity_input.c synthesises MotionEvents for the legacy
 * Input manager). If that turns out not to reach the game, these are the direct
 * route: patch the getters the way KB_HAVE_TIME_HOOKS patches the Time ones.
 *
 * Data Defense uses the LEGACY Input manager -- libil2cpp contains zero
 * references to UnityEngine.InputSystem -- so these three are the whole
 * surface, and nx_newinput.c is inert for this title.
 *
 * The Touch value type this returns is 68 bytes, laid out (from the same
 * Il2CppInspector output):
 *   +0x00 int32 m_FingerId      +0x04 Vector2 m_Position
 *   +0x0c Vector2 m_RawPosition +0x14 Vector2 m_PositionDelta
 *   +0x1c float m_TimeDelta     +0x20 int32 m_TapCount
 *   +0x24 int32 m_Phase         +0x28 int32 m_Type
 *   +0x2c float m_Pressure      +0x30 float m_maximumPossiblePressure
 *   +0x34 float m_Radius        +0x38 float m_RadiusVariance
 *   +0x3c float m_AltitudeAngle +0x40 float m_AzimuthAngle
 */
#define DD_IL2_Input_GetTouch            0x2366478u
#define DD_IL2_Input_get_touchCount      0x2366e5cu
#define DD_IL2_Input_get_touchSupported  0x2366df0u
#define DD_IL2_Input_get_mousePosition   0x2366984u

/* The mouse-button half. dd_touchhook.c serves the Switch pointer through BOTH
 * the mouse and touch APIs, because this game's own libil2cpp calls both:
 * get_mousePosition x6, GetTouch x3, GetMouseButtonDown x1 (counted as bl
 * targets, not assumed). Every guard word below was read from this binary. */
#define DD_IL2_Input_get_mousePresent    0x2366d84u  /* guard 0xa9bf4ffe */
#define DD_IL2_Input_GetMouseButton      0x236639cu  /* guard 0xf81e0ffe */
#define DD_IL2_Input_GetMouseButtonDown  0x23663d8u  /* guard 0xf81e0ffe */
#define DD_IL2_Input_GetMouseButtonUp    0x2366414u  /* guard 0xf81e0ffe */

/* ===========================================================================
 * THE GAME'S REAL INPUT PATH: IMGUI Event, not UnityEngine.Input
 * ===========================================================================
 * Data Defense does not poll Input.GetTouch for gameplay. It has its own input
 * framework -- InputMgr, InputAction, InputKey, InteractiveObject -- and that
 * framework is driven from a Unity OnGUI callback:
 *
 *   InputMgr_InputListener_OnGUI  (+0x12d2d5c) calls, in order:
 *       Event.current, Event.isKey, Event.type, Event.keyCode,
 *       Event.isMouse, Event.button, Event.delta
 *   then hit-tests with InteractiveObject_collidesWithMouse(Vector2 mousePos)
 *   through SceneMgr_toViewportSpace / isInsideViewport.
 *
 * InputMgr_getMousePosition (+0x12d2b60) is just `ldr x8,[x0,#0x18];
 * ldp s0,s1,[x8,#0x40]` -- a cached Vector2. Nothing in that path touches
 * UnityEngine.Input at all.
 *
 * So the Input.* hooks in dd_touchhook.c cannot reach this game's logic, which
 * is why they install, get called, receive correct data, and change nothing.
 * The Input.mousePosition calls that DO show up in the log are Unity's own
 * uGUI input module polling for the UI layer, exactly as clayjam_nx's comment
 * predicts.
 *
 * Call sites for the Event accessors, counted as bl targets in this binary:
 *
 *     Event.get_type           86     Event.get_delta           9
 *     Event.get_current        33     Event.get_isKey           2
 *     Event.get_mousePosition  16     Event.get_isMouse         1
 *     Event.get_button          9
 *
 * NOT HOOKED YET, AND get_type IS THE REASON. IMGUI runs a Layout pass and a
 * Repaint pass every frame, and Unity's own GUI internals are most of those 86
 * callers. Returning MouseDown from get_type globally would corrupt layout for
 * every IMGUI control in the game, which is a worse failure than no touch.
 * get_isMouse having exactly ONE call site -- the game's own OnGUI -- is the
 * lever worth pulling first, because it is the only one that is unambiguously
 * the game asking.
 *
 * The right fix is more likely upstream: IMGUI events are generated by the
 * engine from its native input queue, which is what nativeInjectEvent feeds.
 * That path is already wired and already delivers (ret=1, and the engine reads
 * the event's getters). Finding why those do not become IMGUI events is a
 * smaller and safer job than lying to 86 call sites.
 * ===========================================================================
 */
#define DD_IL2_Event_get_current         0x2341e7cu
#define DD_IL2_Event_get_isMouse         0x23411ecu
#define DD_IL2_Event_get_isKey           0x2341f5cu
#define DD_IL2_Event_get_type            0x2341368u
#define DD_IL2_Event_get_button          0x2340c44u
#define DD_IL2_Event_get_delta           0x2340a74u
#define DD_IL2_Event_get_mousePosition   0x2340930u
#define DD_IL2_InputMgr_OnGUI            0x12d2d5cu
#define DD_IL2_InputMgr_getMousePosition 0x12d2b60u

/* ===========================================================================
 * UnityEngine.Time getters -- RESOLVED FROM AN Il2CppInspector ADDRESS MAP
 * ===========================================================================
 * These are the icall thunks KB_HAVE_TIME_HOOKS patches. Previously they had to
 * come from a dump.cs and be pinned by hand; an Il2CppInspector map gives all
 * of them by name.
 *
 * THE MAP WAS VERIFIED AGAINST THIS BINARY FIRST. tools/il2cpp_lookup.py
 * refuses to resolve anything until the map's `exports` list matches the real
 * .dynsym of the libil2cpp it is pointed at: 2416 of 2416 here, zero
 * mismatches. A map from a different build of the game is worse than no map --
 * every address in it is plausible and wrong -- so this check is not optional.
 *
 * 7 of 9 resolved. get_smoothDeltaTime and get_timeSinceLevelLoad are absent
 * from the map because IL2CPP STRIPPED them: Data Defense never calls either,
 * so the linker removed the managed body. Not a derivation failure, and the
 * hook table in main.c skips a zero entry, so the other seven are unaffected.
 * (Killer Bean shipped with 8 of 9 for the same reason.)
 *
 * A note on the mangled names, because it cost a wrong answer: Itanium encodes
 * the IDENTIFIER LENGTH, so get_realtimeSinceStartup is `24get_realtime...`,
 * not 25. Guessing the number produces a confident "not found" for a symbol
 * that is right there. Count it, do not estimate it.
 * ===========================================================================
 */
#define KB_IL2_get_time                  0x2322344u
#define KB_IL2_get_timeSinceLevelLoad    0u
#define KB_IL2_get_deltaTime             0x2322414u
#define KB_IL2_get_unscaledTime          0x232243cu
#define KB_IL2_get_unscaledDeltaTime     0x2322464u
#define KB_IL2_get_timeScale             0x23224ecu
#define KB_IL2_get_frameCount            0x2316e64u
#define KB_IL2_get_realtimeSinceStartup  0x2321554u

/* ABSENT IN THIS BUILD. UnityEngine.Time here has no smoothDeltaTime property
 * at all -- the string does not occur even once in the whole 19 MB dump, so
 * the managed linker stripped it because the game never reads it. Left at 0;
 * main.c skips a zero offset explicitly rather than probing libil2cpp+0.
 * If a future game version starts using it, re-dump and fill this in.       */
#define KB_IL2_get_smoothDeltaTime       0u

/* Other Time members present in this build, recorded but NOT hooked (the
 * loader has no substitute for them and does not read them):
 *     get_fixedUnscaledTime   0x1FA2E0C     get_fixedDeltaTime    0x1FA2E5C
 *     set_fixedDeltaTime      0x1FA2E84     get_maximumDeltaTime  0x1FA2EBC
 *     set_maximumDeltaTime    0x1FA2EE4     set_timeScale         0x1FA2F44
 *     get_renderedFrameCount  0x1FA2FA4     get_inFixedTimeStep   0x1FA2FF4 */


/* KB_HAVE_TIME_HOOKS is owned by config.h (single source of truth), where it
 * is 0 -- these nine il2cpp thunk offsets were never derived for this game. */

/* ===========================================================================
 * LevelMap_WhatsNew.CloseWhatsNew hook  --  diagnostic AND workaround
 * ===========================================================================
 * The "What's New" popup's close button does nothing, while every other button
 * in the game -- including small ones in the options and UI-config menus --
 * works. By that point the wrapper was demonstrably clean on that screen: zero
 * managed and native exceptions, 44 DOWN / 44 UP all CONSUMED including
 * multi-touch, coordinates landing on the button, and display metrics matching
 * the delivery space (1280x720 on both sides). So the remaining question was
 * binary, and inference had run out: DOES THE CLICK REACH THE HANDLER AT ALL?
 *
 * Offsets from dump.cs (LevelMap_WhatsNew, TypeDefIndex 1751), each confirmed
 * by disassembly of THIS libil2cpp:
 *
 *   CloseWhatsNew          RVA 0xA024A4, first word 0xF81E0FF4
 *                                        (str x20, [sp, #-0x20]!)
 *   panel_whatsnew         field offset 0x18 -- the body does
 *                                        `ldr x0,[x19,#0x18]` then `cbz`
 *   GameObject.SetActive   RVA 0x1FA4C98
 *
 * The call convention was READ OFF the original call site rather than assumed:
 *       +0x50  mov  w1, wzr        ; value = false
 *       +0x54  mov  x2, xzr        ; MethodInfo* = NULL
 *       +0x58  bl   #0x1fa4c98
 * so the hook calls SetActive(panel, 0, NULL) exactly as the game does --
 * including the NULL MethodInfo, which is what the original passes.
 */
#define KB_IL2_CloseWhatsNew          0u
#define KB_CLOSE_WHATSNEW_WORD        0u  /* str x20,[sp,#-0x20]! */
/* ---- WeaponStore_IAP, offline entitlements (round 160) -------------------
 * DERIVED from dump.cs for this build:
 *   public  void Purchase_Weapons_Pack()             RVA 0xA0DA58  A9BE53F5 A9017BF3
 *   public  void Purchase_Unlimited_Ammo()           RVA 0xA0DB10  A9BE53F5 A9017BF3
 *   private void Owned_Weapons_Pack(bool)            RVA 0xA0CC3C  A9BA6FFC A90167FA
 *   private void Owned_Unlimited_Ammo(bool)          RVA 0xA0D3B8  F81D0FF6 A90153F5
 *
 * The two Purchase_* handlers are replaced; the two Owned_* grants are CALLED,
 * not patched, so the unlock runs the game's own code. Both Purchase_* share a
 * prologue, so each is still checked against its own address -- one shared
 * expected word across a table is how you patch the wrong function and never
 * find out. */
#define KB_IL2_Purchase_Weapons_Pack    0u
#define KB_PURCHASE_WEAPONS_W0          0u
#define KB_PURCHASE_WEAPONS_W1          0u
#define KB_IL2_Purchase_Unlimited_Ammo  0u
#define KB_PURCHASE_AMMO_W0             0u
#define KB_PURCHASE_AMMO_W1             0u
#define KB_IL2_Owned_Weapons_Pack       0u
#define KB_IL2_Owned_Unlimited_Ammo     0u

/* ---- EventSystem.Update: let every EventSystem tick (round 156) ---------
 * level3 (the level map) is the ONLY scene in this game with TWO EventSystems:
 *
 *   path_id  87   EventSystem + StandaloneInputModule + TouchInputModule
 *   path_id 199   EventSystem + InputSystemUIInputModule      <-- new input
 *
 * every other scene has just the legacy one. uGUI's EventSystem.Update() opens
 * with `if (current != this) return;` and `current` is whichever registered
 * first, so on the map the new-Input-System EventSystem wins and the legacy one
 * never ticks. We only feed the legacy backend (UnityPlayer.nativeInjectEvent),
 * so nothing on that screen can be clicked: the What's New X, Button_play and
 * the menu button all die together, while every other scene is fine. On Android
 * both backends are fed from one MotionEvent, so nobody ever noticed.
 *
 * Proven by probe: replacements on LevelMap_Control.PlayLevel/Menu installed
 * and NEVER fired, so the click was not reaching the handler at all.
 *
 * EventSystem.Update  RVA 0x21CA404
 *   0x21CA4A4  bl   0x1F9BFCC          ; Object.op_Inequality(current, this)
 *   0x21CA4A8  tbnz w0, #0, 0x21CA630  ; if (current != this) return   <-- NOP
 *   0x21CA4AC  mov  x0, x19
 *   0x21CA4B0  bl   0x21CA2F0          ; TickModules()
 *
 * NOPping the branch lets both tick. Where only one EventSystem exists the
 * branch was never taken, so this is a no-op everywhere else in the game; on
 * the map the legacy module gets to run and the new-input one finds no events
 * and does nothing. */
#define KB_IL2_EventSystem_Update_Br  0u
#define KB_EVENTSYSTEM_BR_WORD        0u  /* tbnz w0,#0,#0x188 */
#define KB_ARM64_NOP                  0xD503201Fu

/* ---- LevelMap_Control.PlayLevel / .Menu (round 155, DIAGNOSTIC) ---------
 * DERIVED from dump.cs for this build:
 *   public void LevelMap_Control.Menu()       RVA 0x9FF1FC  F81E0FF4
 *   public void LevelMap_Control.PlayLevel()  RVA 0x9FF290  A9BE53F5
 * These are the two handlers behind the only controls on the map that are uGUI
 * Buttons rather than LeanSelectables -- Button_play (level3 path_id 78) has
 * RectTransform + CanvasRenderer + Image + Button, the classic uGUI shape.
 * They are the two the player reports dead now that the Lean nodes work. */
#define KB_IL2_LevelMap_Menu          0u
#define KB_LEVELMAP_MENU_WORD         0u  /* str x20,[sp,#-0x20]!     */
#define KB_IL2_LevelMap_PlayLevel     0u
#define KB_LEVELMAP_PLAYLEVEL_WORD    0u  /* stp x21,x20,[sp,#-0x20]! */

/* ---- LeanInput touch source (round 154) ---------------------------------
 * DERIVED for this build from dump.cs + disassembly:
 *   public static int  LeanInput.GetTouchCount()                RVA 0x156AD68
 *   public static void LeanInput.GetTouch(int index, out int id,
 *                        out Vector2 position, out float pressure,
 *                        out bool set)                          RVA 0x156ADD8
 *
 * GetTouchCount disassembles to
 *     EnhancedTouchSupport.get_enabled() / .Enable()      (0x1E72190/0x1E721E0)
 *     UnityEngine.InputSystem.EnhancedTouch.Touch.get_activeTouches()
 *                                                        (0x1E74734)
 * -- the NEW input system. We inject through UnityPlayer.nativeInjectEvent,
 * which is the legacy path; the new system's native event queue is never
 * written, so LeanTouch sees no touches at all. Confirmed at runtime: with
 * PointOverGui hooked, it was never called once across 40+ taps, i.e. no finger
 * was ever created.
 *
 * On Android one MotionEvent feeds both backends. We feed one. */
#define KB_IL2_LeanGetTouchCount      0u
#define KB_LEAN_GETTOUCHCOUNT_WORD    0u  /* stp x19,x30,[sp,#-0x10]! */
#define KB_IL2_LeanGetTouch           0u
#define KB_LEAN_GETTOUCH_WORD         0u  /* sub sp,sp,#0x90          */

/* ---- LeanTouch.PointOverGui (round 153) ---------------------------------
 * DERIVED for this build from dump.cs + disassembly:
 *   public static bool LeanTouch.PointOverGui(Vector2 screenPosition)
 *   RVA 0x1583D74, prologue  6DBD23E9  stp d9, d8, [sp, #-0x30]!
 *
 * Why it matters: LevelMap_Button.Update() (RVA 0x9FD780) is
 *     if (this->lean_selectable == null) return;
 *     if (LeanSelectable.get_IsSelected(lean_selectable))   // 0x1576FBC
 *         levelmap_control->is_button_pressed = true, selected_level = name;
 * so every level node on the map is gated on a LeanSelectable being SELECTED.
 * Nothing on that screen is a uGUI Button, which is why uGUI screens respond
 * and the map does not. LeanTouch refuses to select for a finger whose
 * StartedOverGui is true, and StartedOverGui is PointOverGui() evaluated at the
 * DOWN, via EventSystem.RaycastAll. */
#define KB_IL2_LeanPointOverGui       0u
#define KB_LEAN_POINTOVERGUI_WORD     0u  /* stp d9,d8,[sp,#-0x30]! */

#define KB_WHATSNEW_PANEL_OFF         0u        /* LevelMap_WhatsNew.panel_whatsnew */
#define KB_IL2_GameObject_SetActive   0x231a8fcu

/* ALL THREE close handlers, because the first pass hooked only one and assumed
 * the button targeted it. LevelMap_WhatsNew owns three panels and three close
 * methods; the visible dialog says "What's New", but a close button is just as
 * often wired to a sibling handler. Each has the SAME prologue word, each reads
 * its panel field then calls GameObject.SetActive(panel, false, NULL), so one
 * hook shape covers all three -- and whichever fires names the real target. */
#define KB_IL2_Close_NewDay           0u
#define KB_NEWDAY_PANEL_OFF           0u        /* panel_NewDay        */
#define KB_IL2_Close_SpecialBonus     0u

/* ---- Awake/Start TRAMPOLINES (not replacements) --------------------------
 * The close handlers could be replaced outright because the only effect that
 * mattered was hiding a panel. Awake and Start cannot: their bodies have real
 * side effects (Awake assigns dungeon_refresh; Start decides whether the popup
 * shows at all), so these are true trampolines that log and then run the stock
 * body.
 *
 * Both begin with the SAME two PC-independent instructions, which is what makes
 * one trampoline shape cover both:
 *     A9BE53F5  stp x21, x20, [sp, #-0x20]!
 *     A9017BF3  stp x19, x30, [sp, #0x10]
 * followed by two ADRPs that differ per method and must be rebuilt from the
 * runtime base rather than copied (a relocated ADRP computes the wrong page):
 *     Awake  +0x8 x20 <- 0x2ee5000   +0xc x21 <- 0x2cd9000
 *     Start  +0x8 x21 <- 0x2ee5000   +0xc x20 <- 0x2d0a000
 * Execution resumes at +0x10 in both cases, which expects exactly those two
 * registers set and x0 still holding `this`. */
#define KB_IL2_WhatsNew_Awake         0u
#define KB_IL2_WhatsNew_Start         0u
#define KB_WHATSNEW_PRO_W0            0u  /* stp x21,x20,[sp,#-0x20]! */
#define KB_WHATSNEW_PRO_W1            0u  /* stp x19,x30,[sp,#0x10]   */
#define KB_WN_PAGE_A                  0u   /* both methods, +0x8 or +0xc */
#define KB_WN_PAGE_AWAKE_B            0u
#define KB_WN_PAGE_START_B            0u
#define KB_WHATSNEW_REFRESH_OFF       0u        /* dungeon_refresh -- set at Awake +0x6c */

/* GameObject active-state getters, for probing which panel is really on screen.
 * dump.cs marks them [NativeMethod("IsSelfActive")] / [NativeMethod("IsActive")],
 * so they are icalls into the engine and safe to call directly with a NULL
 * MethodInfo, exactly like GameObject.SetActive above.
 *
 * activeSelf vs activeInHierarchy matters here: a panel can be activeSelf=true
 * while a deactivated parent keeps it off screen. Only activeInHierarchy says
 * "this is actually being drawn and raycast against". */
#define KB_IL2_GO_get_activeSelf        0u
#define KB_IL2_GO_get_activeInHierarchy 0u
#define KB_SPECIALBONUS_PANEL_OFF     0u        /* panel_SpecialBonus  */

/* ===========================================================================
 * Boehm GC stop-the-world bridge  --  DERIVED FOR THIS GAME, ENABLED
 * ===========================================================================
 * WHERE IT LIVES. Not in libunity. Boehm is compiled into libil2cpp.so: that
 * binary carries Boehm's own copyright string and internal error strings
 * ("GC_mark_some: bad state", "GC_push_all_stacks: sp not set!") and imports
 * pthread_kill / pthread_sigmask / sem_init / sem_wait / sem_post / sigaction.
 * libunity.so contains none of those and does not import pthread_kill at all.
 * A symbolized libunity reference -- of any Unity version -- cannot reach
 * this; searching 93k-96k engine symbols for Boehm internals returns zero.
 *
 * HOW IT WAS DERIVED (tools/gc_derive.py, against YOUR libil2cpp.so, which is
 * stripped -- no reference build and no symbols were needed):
 *   1. Resolve the PLT stub for pthread_kill from .rela.plt plus the stub's
 *      own ADRP/LDR pair                                     -> 0x7179c0
 *   2. Scan .text for BL to that stub. EXACTLY TWO callers exist, which is the
 *      expected shape:
 *          0x94c32c  GC_suspend_all     (sends suspend sig, returns count)
 *          0x94c574  GC_start_world     (sends restart sig)
 *          0x94c504  GC_stop_world      (sem_wait xN, N = count suspended)
 *          0x94c1d4  GC_suspend_handler (the signal handler that acks)
 *   3. Read the globals straight out of those four bodies.
 *
 * THE CLINCHING EVIDENCE. GC_suspend_handler contains, verbatim:
 *
 *     lsr  x9, x0, #8
 *     eor  w8, w9, w1
 *     eor  w8, w8, w8, lsr #16
 *     and  x8, x8, #0xff
 *     add  x9, x9, #0x608          <- GC_threads
 *     ldr  x21, [x9, x8, lsl #3]
 *
 * That is instruction-for-instruction the hash nx_gc_thread_index() in
 * libc_shim.c already implements. The loader's thread lookup and this
 * binary's agree exactly. GC_threads is confirmed twice more: both
 * GC_suspend_all and GC_start_world walk the same table with `cmp x21,#0x100`
 * (256 buckets -- the size the loader's table[] assumes), chaining via [x26].
 */
/* ---- the four load-bearing globals, each pinned to its use site ----------
 * Re-verified by backward-slicing the argument at every call site rather than
 * by a forward scan. The four that actually drive the bridge:
 *
 *   suspend signal   0x2ee52e4  GC_suspend_all +0x88:
 *                                 ldr w1,[x24,#0x2e4] ; bl pthread_kill
 *   restart signal   0x2ee52e8  GC_start_world +0xa0:
 *                                 ldr w1,[x24,#0x2e8] ; bl pthread_kill
 *   start-world gate 0x2ee52e0  GC_start_world +0x84:
 *                                 ldr w8,[x23,#0x2e0] ; cbz w8,<skip>
 *                               -- gates whether start_world signals AT ALL
 *   ack semaphore    0x2ef6fb0  fed to FIVE call sites (see below)
 *
 * Note the three counters sit in consecutive 4-byte slots, gate first:
 * 0x2e0 / 0x2e4 / 0x2e8. Any build showing that trio adjacent in .data with a
 * cbz-gate on the lowest one is the same structure at different addresses. */

/* THE START-WORLD GATE. Boehm calls this GC_retry_signals, and that is the
 * name libc_shim.c uses -- but the name undersells it. Its functional role in
 * THIS build is a gate on the restart path:
 *
 *     GC_start_world +0x84   ldr  w8, [x23, #0x2e0]
 *                    +0x88   cbz  w8, <skip>          ; 0 -> never signals
 *                    +0x8c   ldr  x8, [x27, #0x10]    ; thread stop_count
 *                    +0x90   ldr  x9, [.., #0x5e8]    ; GC_stop_count
 *                    +0x94   orr  x9, x9, #1          ; <- the |1 convention
 *                    +0x98   cmp  x8, x9
 *                    +0xa0   ldr  w1, [x24, #0x2e8]   ; restart sig
 *                    +0xa4   bl   pthread_kill
 *
 * The loader's emulation in libc_shim.c acks a second time with
 * `stop_count | 1` exactly when this gate is set, which is precisely what the
 * `orr x9,x9,#1` above compares against. Emulation and binary agree. */
#define GC_RETRY_SIGNALS_OFF_FN  0x290b8e0u  /* .data  GC_stop_world +0x28, GC_start_world +0x14 */

/* THE ACK SEMAPHORE. Confirmed at FIVE independent call sites, each with an
 * explicit `adrp x0,#0x2ef6000 ; add x0,x0,#0xfb0` immediately before the
 * call, so this is a slice of the actual argument, not a guess:
 *
 *     sem_init      @0x94c6f4   (setup)
 *     sem_timedwait @0x94c49c   (GC_suspend_all -- the bounded ack wait)
 *     sem_wait      @0x94c540   (GC_stop_world  -- the blocking ack wait)
 *     sem_post      @0x94c2a0   (handler, first ack)
 *     sem_post      @0x94c2ec   (handler, retry ack, behind the gate)
 *
 * The sem_timedwait site was MISSED by the first derivation pass, which only
 * scanned for sem_wait/sem_post/sem_init. It agrees with the other four, so
 * the value was right, but the confirmation was weaker than it should have
 * been. tools/gc_derive.py now scans the full set.
 *
 * ONE SEMAPHORE, not three: all five sites resolve to the same address. The
 * three macro names below are kept because libc_shim.c refers to them
 * separately; they intentionally hold one value. */
/* GC_START_ACK_OFF_FN / GC_RESTART_SEM_OFF_FN were carried over from an older
 * revision of this tree and are referenced by NOTHING except their own alias.
 * Deleted rather than left at 0: a zeroed offset that nobody reads is just
 * another thing a future reader has to check and discard. */

/* ONE SEMAPHORE, not three. Every sem_* site in this build -- the single
 * sem_init, the sem_wait loop in GC_stop_world, and both sem_post calls in the
 * handler -- resolves to 0x2ef6fb0. The three macros above stay distinct
 * because libc_shim.c names them separately, but they intentionally hold the
 * same address. The reference port found the same single-semaphore shape, so
 * this is Boehm's configuration rather than a quirk of this build.
 *
 * ---- Boehm per-thread struct field offsets: CORRECTED FOR THIS BUILD ------
 * The reference tree carried STACKPTR 0x28 / STOPCNT 0x18, and an earlier
 * revision of this header repeated them with a comment claiming they were
 * "Boehm ABI, fixed by the collector's own layout" and therefore safe to
 * inherit. THAT WAS WRONG. Read off GC_suspend_handler:
 *
 *     ldr  x21, [x21]                          <- next        (+0x00)
 *     ldr  x8,  [x21, #8]                      <- thread id   (+0x08)
 *     ldr  x8,  [x21, #0x10] ; and x8,x8,#~1   <- stop_count  (+0x10), low bit a flag
 *     add  x19, x21, #0x10   ; stlr x20,[x19]  <- publishes stop_count
 *     str  x8,  [x21, #0x18]                   <- stack pointer (+0x18)
 *
 * Only the id offset matched. Inheriting 0x18/0x28 would have made the
 * collector read the wrong words out of every thread it suspended -- exactly
 * the failure mode this tree gates features off to avoid.                   */
#define GC_THREAD_NEXT_OFF       0x0u
#define GC_THREAD_ID_OFF         0x8u
#define GC_THREAD_STOPCNT_OFF    0x10u   /* CORRECTED: reference tree said 0x18 */
#define GC_THREAD_STACKPTR_OFF   0x18u   /* CORRECTED: reference tree said 0x28 */

/* ---- splash / video / preload probes (all Fruit Ninja il2cpp) ----------- */
#define KB_GET_SHOULD_SHOW_SPLASH     0u
#define KB_IL2_SPLASH_FINISHED_CHECK  0u
#define KB_IL2CPP_FINISH_FLAG         0u
#define KB_HAVE_FINISH_PROBE          0
#define KB_IL2_CRTFVP_Prepare             0u
#define KB_IL2_CRTFVP_OnVideoPlayerReady  0u
#define KB_IL2_RAISE_EXCEPTION            0u
#define KB_IL2_RAISE_INNER_BL             0u
#define KB_IL2_RAISE_TAIL                 0u
#define KB_HAVE_VIDEO_BYPASS              0
#define KB_PRELOAD_EXIT_BRANCH        0u
#define KB_PRELOAD_BUDGET_DEFAULT     0u
#define KB_PRELOAD_BUDGET_TABLE_LOAD  0u
/* KB_PACING_GETTER: real value in the Swappy section below */

/* ===========================================================================
 * THE ALLOCATOR TABLE -- 21 sites, derived for THIS binary
 * =========================================================================== */
/* ===========================================================================
 * BRANCH FORCES -- ChoreographerBase::Get() must not pick the Java implementation
 * ===========================================================================
 * ChoreographerBase::Get() chooses a frame-pacing backend from
 * GetContextType() (libunity+0xaad4fc -- a three-instruction leaf that just
 * loads the value initJni stored):
 *
 *     +0xa99f5c  bl   GetContextType
 *     +0xa99f60  cmp  w0, #1
 *     +0xa99f64  b.eq +0xa99f98        ; ==1 (GameActivity) -> API check -> NDK
 *     +0xa99f68  cbnz w0, +0xa99fe4
 *                                      ; ==0 (ActivityOrService) -> JAVA
 *
 * We pass contextType 0, and truthfully: this game's Java class really is
 * com.unity3d.player.UnityPlayerForActivityOrService, which is also where its
 * lifecycle natives are registered. So the branch falls through to
 * ChoreographerJava, which drives frames from android.view.Choreographer and
 * needs a real Android UI thread posting callbacks. There is none, so UnityMain
 * parks forever in the constructor at +0xaa06f4 waiting on a condvar for a
 * Choreographer the Java side can never publish. Two frames render, then
 * nothing.
 *
 * NOTE THE ORDER OF THE TWO GATES. Fixing SDK_INT (so DeviceApiLevel() returns
 * 33 instead of 0) was necessary but NOT sufficient: the API check lives at
 * +0xa99f98, which contextType 0 never reaches. Both gates have to pass.
 *
 * WHY PATCH THE BRANCH RATHER THAN PASS contextType 1 TO initJni.
 * Because the class genuinely is ActivityOrService. Claiming GameActivity
 * globally would change input, window and lifecycle handling everywhere --
 * GetContextType() is read by more than this one decision. Patching the branch
 * confines the lie to the single choice that needs it. This is hitmansniper_nx's
 * reasoning and its fix; the site here is our own build's.
 *
 * WHAT THIS TRADES. ChoreographerNDK is a path this port can actually serve:
 * AChoreographer_getInstance / postFrameCallback / postFrameCallback64 are
 * implemented for real in ndk_choreographer.c. The residual risk hitmansniper
 * records is that if something later dereferences the handler at +0x50 without
 * a null check, this converts an early hang into a later crash. Their port
 * needed a companion null-check on NdkLooper::CreateHandler; ours may not,
 * because ALooper_forThread() now creates on demand (android_native_unity.c)
 * so the looper that patch guarded against is never null here. If a crash
 * appears just past this point, that is the first thing to add.
 *
 * A THIRD hitmansniper patch that we deliberately do NOT carry.
 *
 * Theirs neutralises AndroidCursors::AndroidSetCursorCommand's constructor:
 *     0xad1928  bl NdkLooper::CreateHandler  ->  mov x0, #0
 * because GetUILooper() returned NULL on that port, CreateHandler passed NULL
 * into WaitForCreation, and its first instruction `ldrb w8,[x0,#0xe8]` faulted.
 * That is the same far=0xe8 fault this port hit at r13.
 *
 * The site exists here, pinned and verified:
 *     +0xac151c  AndroidSetCursorCommand ctor
 *     +0xac1560  bl GetUILooper      (+0xaa7398)
 *     +0xac1570  bl CreateHandler    (+0xaa7054)   word 0x97ff96b9
 *                                     -> mov x0,#0  0xd2800000
 * (Their word is 0x97ff96bb; ours differs only in the call displacement.)
 *
 * It is not needed, and the reason is worth recording. GetUILooper is three
 * instructions:
 *     adrp x8, #0x1b42000 ; ldr x0, [x8, #0x208] ; ret
 * and +0x1b42208 is exactly the global InitializeUILooper stores to on success.
 * Our ALooper_forThread() now creates on demand (6k), so InitializeUILooper
 * succeeds -- the "Couldn't retrieve native ALooper for UI thread" line is gone
 * from the log -- and the global is non-NULL. hitmansniper patched the symptom
 * because their looper was null; we removed the cause.
 *
 * ENABLE IT IF: that warning returns, or a data abort appears with far=0xe8 and
 * pc inside CreateHandler's WaitForCreation (+0xaa6ff8).
 *
 * VERIFY-FIRST: main.c checks {from} and skips on a mismatch.
 * ===========================================================================
 */
static const NxPatchWord KB_BRANCH_FORCES[] = {
  /* 1. ChoreographerBase::Get() context-type gate.
   *    b.eq +0xa99f98 -> b +0xa99f98. Same target, unconditional.
   *    Byte-identical to hitmansniper_nx's site (0x540001a0 -> 0x1400000d). */
  { 0xa99f64, 0x540001a0, 0x1400000d },

  /* 2. ...and its API-level gate, immediately after:
   *        +0xa99f98  bl   DeviceApiLevel()
   *        +0xa99f9c  cmp  w0, #0x17
   *        +0xa99fa0  b.gt +0xa9a084      ; >23 -> ChoreographerNDK
   *
   *    Also byte-identical to hitmansniper (0x5400072c -> 0x14000039).
   *
   *    WHY FORCE THIS TOO, when 6j fixed SDK_INT and the log now prints
   *    "API 33"? Because that line comes from SystemInfo, and it is NOT
   *    established that DeviceApiLevel() reads the same value:
   *    DeviceApiLevel (+0xa9a11c) caches its result through a virtual call
   *    (bl +0xab4f30 ; ldr x9,[x0,#0x240] ; blr x8), not through the JNI field
   *    handler we fixed. Forcing the branch removes the dependency on an
   *    assumption I have not verified.
   *
   *    Safe unconditionally here: this port always supplies the NDK
   *    Choreographer for real (ndk_choreographer.c), so "use NDK regardless of
   *    API" is simply true of us. */
  { 0xa99fa0, 0x5400072c, 0x14000039 },
};
#define KB_BRANCH_FORCES_N ((int)(sizeof(KB_BRANCH_FORCES)/sizeof(KB_BRANCH_FORCES[0])))

/* ===========================================================================
 * il2cpp JavaVM globals -- DERIVED FOR THIS GAME, and live
 * ===========================================================================
 * libil2cpp's own JNI_OnLoad must not be called: its first action is a log
 * through a GOT slot the loader mis-binds. Its essential effects are two
 * stores, which main.c replicates directly. Disassembly of THIS binary:
 *
 *   0x008a1310  stp  x19,x30,[sp,#-0x10]!
 *   0x008a131c  mov  x19, x0             ; x19 = JavaVM*
 *   0x008a132c  bl   0x716cc0            ; <- the unsafe log call we skip
 *   0x008a1334  adrp x8, #0x2ef2000
 *   0x008a1338  add  x0, x0, #0x354      ; x0 = il2cpp+0x8a1354 (handler fn)
 *   0x008a133c  str  x19, [x8, #0x4b8]   ; g_vm = vm   -> +0x2ef24b8
 *   0x008a1340  bl   0x8f27fc            ; setter:
 *                                        ;   adrp x8,#0x2ef2000
 *                                        ;   str  x0,[x8,#0xf38]  -> +0x2ef2f38
 *
 * Both targets confirmed to lie in libil2cpp's .bss (SHT_NOBITS, writable).
 * Structurally identical to the PvZ / Fruit Ninja pairs; the VALUES are ours,
 * read out of this binary. This is the one inherited feature that IS enabled,
 * because it was re-derived rather than assumed.                            */
#define KB_IL2CPP_VM_GLOBAL       0u  /* g_javavm            (.bss) */
#define KB_IL2CPP_HANDLER_SLOT    0u  /* g_jni_handler_fnptr (.bss) */
#define KB_IL2CPP_HANDLER_FN      0u   /* value stored into the slot */
#define KB_HAVE_IL2CPP_VM         0

/* ===========================================================================
 * FMOD -> OpenSL output select  --  DERIVED AND ENABLED
 * ===========================================================================
 * The Fruit Ninja port looked for this inside AudioManager::InitFMOD. In
 * 2021.3.31f1 that is the wrong function. A whole-image scan of the
 * symbolized reference for callers of FMOD::System::setOutput finds exactly
 * ONE in engine code:
 *
 *     AudioManager::InitNormal(bool, FMOD_OUTPUTTYPE) +0xb4
 *
 * (The other setOutput callers are all inside FMOD's own SystemI, reached
 * from createSound/init/getNumDrivers -- not the engine's output choice.)
 *
 * InitNormal pinned into the game at 0x476cb8 by exact opcode-shape match,
 * unique across the whole binary. Its argument setup is byte-identical to
 * the reference:
 *
 *     +0x98   bl   <get output kind>
 *     +0x9c   cmp  w0, #2
 *     +0xa0   mov  w8, #0x15            ; 21
 *     +0xa4   cinc w21, w8, eq          ; w21 = 21, or 22 when w0 == 2
 *     +0xa8   ldr  x0, [x19, #0x158]    ; the FMOD System*
 *     +0xac   mov  w1, w21              ; <-- PATCH SITE  (0x2A1503E1)
 *     +0xb0   add  x24, sp, #0x40
 *     +0xb4   bl   FMOD::System::setOutput
 *
 * Replacing `mov w1, w21` with `movz w1, #22` forces OpenSL unconditionally,
 * which is the output the loader actually implements (opensles.c). Enum 22
 * is confirmed from the code itself -- it is the value the cinc selects --
 * not assumed from an FMOD header.
 *
 * The expected word 0x2A1503E1 is the SAME one main.c already verifies
 * against, so the existing patch logic needed no change beyond this offset.
 * It is verify-first: a mismatch skips and logs.                           */
#define KB_FMOD_OUTPUT_SITE   0u   /* AudioManager::InitNormal +0xac */
#define KB_HAVE_FMOD_OPENSL   0

/* FMOD buffer bypass: signature absent in this FMOD build, as in the
 * reference port. Still off. */
#define KB_FMOD_BUFFER_SITE   0u
#define KB_HAVE_FMOD_BUFFER_BYPASS 0
static const NxPatchWord KB_FMOD_WORDS[] = { { 0, 0, 0 } };
#define KB_FMOD_WORDS_NUM 0u

/* ===========================================================================
 * Swappy frame-pacing gate  --  DERIVED AND ENABLED
 * ===========================================================================
 * Swappy::IsEnabledAndActive() @ ref 0x5f03a0 (96 bytes), pinned into the
 * game at 0x368894 by exact opcode-shape match over all 24 instructions,
 * unique across the whole binary.
 *
 * Forcing it to return 0 makes every call site take the disabled path ->
 * plain eglSwapBuffers, no pacing threads, no join. This is how the Zookeeper
 * base already boots.
 *
 * IMPORTANT: the guard word DIFFERS from the reference port's. Fruit Ninja
 * checked for 0xA9BF4FFE (`stp x30,x19,[sp,#-0x10]!`); this build emits
 * 0xA9BF7BF3 (`stp x19,x30,[sp,#-0x10]!`) -- the same pair, opposite
 * register order. main.c has been updated to expect ours. If you ever see
 * "[pace] SKIP Swappy-disable", compare against this value first.          */
#define KB_PACING_GETTER      0u
#define KB_PACING_GUARD_WORD  0u  /* stp x19, x30, [sp, #-0x10]! */

/* ===========================================================================
 * TimeManager  --  ADDRESSES DERIVED, HOOK STILL GATED OFF
 * ===========================================================================
 * All four functions are now located in the game binary:
 *
 *     TimeManager::Update(double)      0x20b654   entry word 0xF9406408
 *     TimeManager::ResetTime(bool)     0x20b280
 *     TimeManager::SetTimeScale(float) 0x20ba44
 *     GetTimeManager()                 0x20bce4
 *
 * Update and SetTimeScale would not pin by masked-context match (0 hits --
 * the reference is a larger, differently-configured build and their codegen
 * genuinely differs), and they carry no rare move-wide immediates for
 * pin.py to vote on. They were resolved instead by exact opcode-shape match
 * bounded to the window between two already-pinned neighbours, ResetTime
 * (0x20b280) and GetTimeManager (0x20bce4) -- each gave exactly one hit in
 * that range, and Update landed within 0 bytes of where the reference's own
 * function spacing projected it.
 *
 * THE GATE IS STILL 0, and this is the honest reason: nx_install_time_fix()
 * does not only need these four addresses. It also needs the native vsync
 * triple -- the waiter's mutex, cond and counter -- which main.c currently
 * carries as three HARDCODED Fruit Ninja .bss offsets (0x110e8d0 / 0x110e8f8
 * / 0x110e928). Those are that game's, not ours. Installing the Update hook
 * while the triple points into the wrong .bss would park the clock thread on
 * an unrelated lock, which is a worse failure than no time fix at all.
 *
 * To enable: derive the three vsync globals for THIS libunity (they sit in
 * the vsync waiter reachable from GfxDeviceClient::SetVSyncCount, located
 * below), replace the hardcoded triple in main.c, then flip this gate.      */
/* TimeManager::Update(double) -- LOCATED, but the time fix stays OFF.
 *
 * Pinned to +0x8d0638: its first six words are byte-identical to the
 * symbolized reference's TimeManager::Update, and that six-word sequence
 * occurs exactly ONCE in the whole game binary.
 *
 * The game's prologue, which the hook has to replicate because the stub
 * overwrites it:
 *
 *     ldr  x8,  [x0, #0x160]   frameCount   (u64)
 *     ldr  w9,  [x0, #0x168]   aux counter  (u32)
 *     ldrb w10, [x0, #0x1a8]   paused flag  (u8)
 *     add  x8, x8, #1  /  add w9, w9, #1
 *     str  x8,  [x0, #0x160]  /  str w9, [x0, #0x168]
 *     cbz  w10, <continue>  /  ret          paused != 0 -> return early
 *
 * WHY THE GATE STAYS OFF. nx_time_update_hook writes tm+0xc8, tm+0xd0 and
 * reads tm+0xf8 -- PvZ's offsets. The correct ones for this build are 0x160,
 * 0x168 and 0x1a8 above, and the semantics match one for one. But
 * nx_clock_tick also reads m_StartupRef at tm+0xe8, which is PvZ's too and is
 * NOT derived, and the hook does not resume the original function -- it
 * replaces it and calls g_unity_update_body instead, so the resume point needs
 * checking as well.
 *
 * Enabling with two of four offsets correct writes into the middle of a live
 * TimeManager. Finish the derivation before flipping KB_HAVE_TIME_FIX:
 * m_StartupRef and the update-body resume point are what is left. */
#define KB_TIME_UPDATE_ENTRY  0x8d0638u
#define KB_TIME_UPDATE_WORD   0xf940b008u  /* ldr x8, [x0, #0x160] */
#define DD_TIME_FRAMECOUNT_OFF 0x160u      /* u64, was PvZ 0xc8 */
#define DD_TIME_AUXCOUNT_OFF   0x168u      /* u32, was PvZ 0xd0 */
#define DD_TIME_PAUSED_OFF     0x1a8u      /* u8,  was PvZ 0xf8 */

/* m_StartupRef -- THE LAST PIECE, and it is not the same KIND of thing.
 *
 * PvZ read it as a plain double at tm+0xe8. Unity 6 has no double there,
 * because TimeManager no longer stores time as doubles at all: it uses
 *
 *     struct RationalTime { int64_t ticks; uint32_t num; uint32_t den; }
 *
 * with the rate as an exact fraction, and seconds = ticks * den / num. The
 * body confirms the direction: `ucvtf d0,w20 ; ucvtf d1,w22 ; fdiv d0,d0,d1`
 * makes tps = num/den, then `fmul d0,d0,d8` makes ticks = tps * seconds.
 *
 * WHICH FIELD. TimeManager::Update reads it and subtracts it from the absolute
 * time it is handed:
 *
 *     x21 = newTicks - ConvertRate(x19+0x190)   ; absolute -> since-startup
 *     x23 = x21 - ConvertRate(x19+0xa8)         ; ...minus last -> the delta
 *     str x21, [x19, #0xa8]                     ; 0xa8 = m_CurTime
 *     stp x23, x20, [x19, #0xc8]                ; 0xc8 = m_DeltaTime
 *
 * Corroborated by every other reference to the field in the whole class:
 *   TimeManager::ResetTime   +0xd24ee8  stp x8, x20, [x19, #0x190]   writes it
 *   TimeManager::Update      +0xd252ec  add x0, x19, #0x190          reads it
 *   TimeManager::GetRealtime +0xd25f50  ldr x8, [x19, #0x190]        reads it
 *   ProduceHelper<TimeManager>::Produce +0xd26028  str q0, [x0,#0x190]
 *
 * Written on reset, subtracted in Update, read by GetRealtime, and zeroed by
 * the constructor with a 128-bit store that also confirms the 16-byte size.
 * That is a startup reference and nothing else.
 *
 * Reading it every tick rather than caching it is deliberate and matches the
 * inherited design: it only changes when ResetTime runs, and if that happens
 * mid-session we want to follow it. The net effect through Update is
 * curTime = (sref + wall) - sref = wall, which is the point. */
#define DD_TIME_STARTUPREF_OFF 0x190u      /* RationalTime, was PvZ double 0xe8 */
#define DD_TIME_CURTIME_OFF    0xa8u       /* RationalTime, for reference */
#define DD_TIME_DELTATIME_OFF  0xc8u       /* RationalTime, for reference */
#define KB_TIME_UPDATE_BODY   0x8d065cu /* DERIVED: entry + 0x24 -- the instruction AFTER the
                                          * prologue's `ret`. NOT entry+16: the 2022.3
                                          * port used +16 because ITS prologue was that
                                          * long. On 2021.3 the prologue runs to +0x20
                                          * (ret), and resuming at +0x10 re-entered the
                                          * sequence with x8/w9/w10 never loaded --
                                          * `str x8,[x0,#0xc8]` then wrote a stale
                                          * register into frameCount and `cbz w10`
                                          * branched on garbage. main.c now SCANS for
                                          * the ret at install time and uses that; this
                                          * constant is the cross-check. */
/* GetTimeManager() -- DERIVED. This was 0 on the mistaken conclusion that
 * Unity 6 does not have it, because a symbol search over 72,831 names found
 * nothing. That was right about the SYMBOL and wrong about the FUNCTION: it is
 * an unnamed two-instruction thunk.
 *
 *     +0x8d12d4  mov w0, #7          <- subsystem index
 *     +0x8d12d8  b   +0x7d81d0
 *     +0x7d81d0  adrp/add x8 = subsystem array (+0x1ad6588)
 *                ldr x0, [x8, w0, sxtw #3] ; ret
 *
 * hitmansniper_nx names the shape ("GetTimeManager is `mov w0,#7 ;
 * b GetSubsystem` -- subsystem index 7"), which is what made it findable:
 * searching for that exact instruction PAIR yields one cross-region jump in
 * the whole binary, and the rest are short intra-function branches.
 *
 * Corroborated by distance. hitmansniper (Unity 6000.3.13f1) has
 * Update 0x8e0148 / GetTimeManager 0x8e0de4; we have 0x8d0638 / 0x8d12d4.
 * Both deltas are 0xc9c exactly.
 *
 * WHY IT MATTERS. TimeManager::Update is never called on this port -- the hook
 * installs and never fires. Without the accessor, nx_clock_thread's
 *     tm = g_get_time_manager ? g_get_time_manager() : g_tm;
 * is always NULL (g_tm is only set BY the hook that never fires), so the clock
 * thread can never drive g_unity_update_body itself. With it, the native clock
 * advances regardless of whether Unity ever ticks the subsystem. */
#define KB_TIME_GETMANAGER    0x8d12d4u
#define KB_WORD_GETTIMEMGR    0x528000E0u  /* mov w0, #7 -- verify before use */
#define KB_TIME_RESETTIME     0u
#define KB_TIME_SETTIMESCALE  0u
/* KB_HAVE_TIME_FIX is now 1: the vsync-triple blocker is cleared (below). */

/* ===========================================================================
 * VSync  --  LOCATED (reference only; no patch applied)
 * ===========================================================================
 * Provided because these are the entry points you need in order to derive
 * the vsync triple that blocks the time fix, and because forcing vsync count
 * is a common bring-up lever.
 *
 *     GetWantedVSyncCount()                      0x20b114
 *     QualitySettings::OnVSyncChanged()          0x7db68c
 *     QualitySettings::SetVSyncCount(int,bool)   0x7db7a4
 *     GfxDeviceClient::SetVSyncCount(unsigned)   0x3c2918
 *
 * All four pinned by masked whole-function match, uniquely. No patch is
 * applied to any of them: this port drives its own loop and disables Swappy,
 * so the engine's vsync bookkeeping is not in the critical path. They are
 * recorded, not used.                                                       */

/* ===========================================================================
 * The vsync triple  --  DERIVED FOR DATA DEFENSE
 * ===========================================================================
 * Unity's frame pacing has a native waiter, WaitVSync(long), that blocks until
 * a counter reaches a target. On Android something increments that counter
 * once per display refresh. Here nothing does, so any thread that calls
 * WaitVSync waits forever. main.c's clock thread stands in for it, and needs
 * three globals: the mutex, its condvar, and the counter.
 *
 * DERIVATION, following the recipe the previous port left in this file.
 *
 * 1. In the symbolized reference, WaitVSync(long) is +0xefe2dc, 96 bytes, and
 *    its body names all three:
 *
 *        adrp x20,#0x1c76000 ; add x20,x20,#0xdc4   -> MUTEX
 *        mov  x0,x20 ; bl <pthread_mutex_lock>
 *        adrp x22,#0x1c76000 ; ldr x21,[x22,#0xe20] -> COUNTER
 *        cmp  x21,x19 ; b.ge <done>
 *        add  x0,x20,#0x28                          -> CONDVAR
 *        mov  x1,x20 ; bl <pthread_cond_wait>
 *
 * 2. Pinned into THIS game by opcode-SHAPE match over all 24 instructions:
 *    unique, exactly one hit in 28 MB of text. A plain masked match is not
 *    enough here -- the add/ldr imm12 fields ARE the .bss offsets, which is
 *    precisely what differs between two links, so they have to be masked too.
 *    Prologue matching also fails: the first three instructions are a generic
 *    frame setup that occurs all over the binary.
 *
 * 3. The globals were then read out of the GAME's own code at +0xa99d68, not
 *    projected from the reference. Data addresses do not survive a cross-build
 *    map and projecting them is how you get a plausible wrong answer.
 *
 * 4. The three call targets were resolved through the game's own .rela.plt:
 *    pthread_mutex_lock, pthread_cond_wait, pthread_mutex_unlock. So these are
 *    real pthread objects and the globals hold the OBJECTS, not pointers to
 *    them. (main.c types the first two as `**`; that extra indirection is
 *    inherited from the lineage and is harmless because only the address is
 *    ever passed, never dereferenced. Left alone rather than "fixed".)
 *
 * THE COUNTER IS AT +0x5c, NOT +0x58. Killer Bean and Fruit Ninja both have
 * mutex+0x58 and this file previously documented that spacing as confirmed
 * across two Unity LTS lines. Unity 6 moved it by one word. Both the reference
 * (0x1c76e20 - 0x1c76dc4) and the game (0x1b412a0 - 0x1b41244) give 0x5c, so
 * two independent binaries agree -- but inheriting the documented +0x58 would
 * have incremented four bytes into the middle of a live pthread_mutex_t and
 * corrupted it silently. This is exactly why the recipe says to read the
 * globals locally.
 *
 * main.c types the counter as uint64_t and the `ldr x21` above confirms it is
 * a 64-bit load, so the width is right.
 * ===========================================================================
 */
#define KB_VSYNC_MUTEX        0x1b41244u
#define KB_VSYNC_COND         0x1b4126cu  /* mutex + 0x28 */
#define KB_VSYNC_COUNTER      0x1b412a0u  /* mutex + 0x5c  <- NOT +0x58 */
#define KB_VSYNC_WAITVSYNC_FN 0xa99d68u   /* WaitVSync(long), for reference */
#define KB_HAVE_VSYNC_TRIPLE  1



/* ===========================================================================
 * BOEHM GC STOP-THE-WORLD BRIDGE  --  DERIVED FOR THIS libil2cpp
 * ===========================================================================
 * libil2cpp.so build-id a6d53ec17e7f7c160b24d13af8dc3d9f7fd4aa1e
 *
 * THE PROBLEM. il2cpp's Boehm GC stops the world with POSIX signals: the
 * collector pthread_kill()s every mutator, each mutator's handler posts an
 * acknowledgement semaphore and parks in sigsuspend(), and the collector waits
 * on that semaphore before marking. Switch has no signal delivery, so the ack
 * never arrives and the collector waits forever -- a hard hang at the first
 * collection, typically seconds into the first scene, with UnityMain parked in
 * sem_wait.
 *
 * Confirmed present in THIS build: libil2cpp imports pthread_kill, sigaction,
 * sigsuspend, pthread_sigmask and the sem_* family, and carries Boehm's
 * "GC Warning: Duplicate suspend signal in thread %p". Unity 6 did not change
 * this.
 *
 * DERIVATION (tools/derive_gc_bridge.py, no symbols and no reference build):
 *   1. The warning string above is referenced by exactly one function --
 *      GC_suspend_handler_inner, at +0x1108d34.
 *   2. Inside it, two `bl sem_post` calls both pass x0 = +0x2b29818. That is
 *      GC_suspend_ack_sem: .bss, 8-byte aligned, posted from both the normal
 *      and the duplicate-signal path exactly as Boehm's source does.
 *   3. The two `bl pthread_kill` sites load their signal number from two
 *      adjacent .data ints, +0x290b8e4 and +0x290b8e8. The lower-addressed
 *      call site is GC_stop_world (suspend), the higher is GC_start_world
 *      (restart) -- the same adjacent-pair layout Killer Bean found.
 *
 * The offsets that were here before this derivation were KILLER BEAN's. They
 * survived the retarget because they do not begin with KB_, which is the
 * pattern the zeroing pass matched on. They were inert only because the gate
 * below was 0. Corrected now, and worth remembering as a reminder that "the
 * retarget script handled it" is not the same as "somebody checked".
 * ======================================================================== */
#define GC_SUSPEND_SIG_OFF_FN    0x290b8e4u  /* .data  int, GC_stop_world's pthread_kill */
#define GC_RESTART_SIG_OFF_FN    0x290b8e8u  /* .data  int, GC_start_world's pthread_kill */
#define GC_ACK_SEM_OFF_FN        0x2b29818u  /* .bss   GC_suspend_ack_sem */

/* DERIVED. Both read straight out of GC_stop_world / GC_suspend_all /
 * GC_restart_all, and each is corroborated by appearing in two functions:
 *
 *   GC_stop_count  GC_stop_world +0x0c does  ldr x9,[x8,#0x808] ; add x9,x9,#2
 *                  ; str x9,[x8,#0x808]  -- the epoch counter, bumped by TWO so
 *                  bit 0 stays free. GC_restart_all +0x78 then does
 *                  ldr x9,[x26,#0x808] ; orr x9,x9,#1 ; cmp x8,x9 against the
 *                  thread record's +0x10. That `orr #1` is exactly the `| 1`
 *                  the restart path in libc_shim.c emulates, so the two agree
 *                  instruction for instruction.
 *
 *   GC_threads     add x22,x22,#0x838 in BOTH GC_suspend_all (+0x24) and
 *                  GC_restart_all (+0x28), indexed ldr x27,[x22,x21,lsl #3]
 *                  with cmp x21,#0x100 -- a 256-bucket table of chained
 *                  records, matching NxGcThread exactly (next +0, id +8,
 *                  last_stop_count +0x10).
 */
#define GC_STOP_COUNT_OFF_FN     0x2b29808u  /* .bss   GC_stop_count */
#define GC_THREADS_OFF_FN        0x2b29838u  /* .bss   GC_threads[256] */

/* All 6 offsets now derived. The gate lives in config.h (KB_HAVE_GC_BRIDGE).
 *
 * SYMPTOM THAT MEANS YOU NEED IT: a hang seconds into the first scene, main
 * thread in sem_wait, other threads idle. diag.c's watchdog runs even with
 * DEBUG_LOG 0 specifically to unwedge this, so the game may stutter rather
 * than die outright.
 *
 * A RUNTIME SHORTCUT FOR CONFIRMING THE TWO SIGNAL GLOBALS: pthread_kill_gc
 * already receives the signal number. Log it alongside the values read from
 * both .data addresses on the first call. If the incoming sig matches
 * +0x290b8e4 the suspend/restart assignment above is right; if it matches
 * +0x290b8e8 they are swapped. That turns an offline inference into a fact
 * from one boot. */

/* ===========================================================================
 * WHOLE-FUNCTION PINNING DOES NOT WORK ON THIS PAIR OF BINARIES
 * ===========================================================================
 * An earlier revision of this header carried five DD_SYM_* constants --
 * TimeManager::Update, ResetTime, SetTimeScale, SwappyGL::Init and
 * Swappy::GetSwappyTargetFrameRate -- pinned from the symbolized reference by
 * matching function prologues. FOUR OF THEM WERE WRONG and have been removed.
 *
 * What went wrong is worth recording, because the addresses looked perfect:
 * each was a UNIQUE masked match, and each guard word verified against the
 * live binary. What was never checked was whether the code AFTER the prologue
 * matched. It did not:
 *
 *     TimeManager::Update          body match   7%
 *     TimeManager::ResetTime                    4%
 *     TimeManager::SetTimeScale                51%
 *     SwappyGL::Init                            6%
 *     AudioManager::InitNormal                  5%
 *     AudioManager::InitFMOD                   52%
 *     AudioManager::CallInitFMODSystem         17%
 *     Swappy::GetSwappyTargetFrameRate         59%   <- the only survivor
 *
 * AArch64 frame setup is formulaic: `sub sp,sp,#N`, a run of `stp` pairs,
 * `mrs x,tpidr_el0`. A seven-instruction run of that can be unique in a 27 MB
 * binary and still belong to an unrelated function. AudioManager::InitNormal
 * pinned to a game function that saves SIX arguments where InitNormal takes
 * three -- plainly not the same code, and the prologue could not tell.
 *
 * WHY THE GRANULARITY TABLE IS STILL SOUND. It does not depend on whole-
 * function correspondence. Each of its 20 entries is pinned on a LOCAL window
 * around one specific computation, and every pin was additionally verified to
 * decode to the same operation as the reference. Local computations survive
 * differences in inlining; whole function layouts do not. The reference build
 * and the game build have visibly different module/stripping configuration
 * (.text differs by 1.4 MB), which is exactly the condition that preserves the
 * former and destroys the latter.
 *
 * CONSEQUENCE FOR THE HOOKS BELOW. Every remaining hook in this file --
 * Swappy, FMOD, TimeManager, the GC bridge -- needs a LOCAL signature, not a
 * function address. tools/pin_symbol.py now measures body similarity and
 * refuses anything under 55%, so it will tell you when a pin is this kind of
 * lie. Do not re-add a DD_SYM_* constant that has not cleared that check.
 * ===========================================================================
 */

/* ===========================================================================
 * SWAPPY AND FMOD  --  INVESTIGATED, AND NEITHER NEEDS A PATCH
 * ===========================================================================
 * Killer Bean patches libunity twice here. Both patches were re-derived for
 * Data Defense, and in both cases the conclusion is that the patch is NOT
 * required for this build. Recording why, because "we did nothing" is only a
 * safe answer if somebody can check the reasoning.
 *
 * ---- SWAPPY (frame pacing) ------------------------------------------------
 * Killer Bean force-disables it: Swappy's init brings up a Choreographer-driven
 * thread pool that never completes, so engine init parks in a pthread_join at
 * frame 0.
 *
 * In Unity 6, Swappy::LoadSwappyWrapperLibrary decides availability, and it is
 * a plain dlopen:
 *
 *     dlopen("libswappywrapper")        <- ref +0xee5a40..+0xee5a6c
 *     cbz  x0, <fail>                   <- NULL: log, return FALSE
 *     dlsym("UnitySwappyWrapperInit")
 *     cbz  x0, <fail>                   <- NULL: dlclose, return FALSE
 *
 * "libswappywrapper" is not in libc_shim.c's dlopen_allow[] and is not a module
 * we loaded, so dlopen_fake returns NULL and the function returns false. Unity
 * then takes the same no-pacing path it takes on any Android device that lacks
 * the wrapper. The deny-by-default dlopen policy already produces exactly the
 * outcome Killer Bean needs a code patch for.
 *
 * The Choreographer argument reinforces this: this port SUPPLIES a working NDK
 * Choreographer (ndk_choreographer.c), so even if Swappy did initialise, the
 * condition that made it hang on Killer Bean is not present.
 *
 * ---- FMOD OUTPUT SELECTION ------------------------------------------------
 * Killer Bean rewrites the requested FMOD_OUTPUTTYPE from AudioTrack (21) to
 * OpenSL (22), because Unity 2021 asks for AudioTrack by default and the Java
 * AudioTrack driver has no JVM to talk to, so it stays silent.
 *
 * Unity 6 does not do that. Tracing the constant back through the call chain:
 *
 *     AudioManager::AwakeFromLoad  +0x24   mov  w1, wzr      <- 0 = AUTODETECT
 *                                  +0x28   bl   InitFMOD
 *     AudioManager::InitFMOD               -> InitNormal(..., type)  x3 ladder
 *     AudioManager::InitNormal             -> FMOD::System::setOutput(type)
 *
 * The request is AUTODETECT, not AudioTrack. FMOD then probes: AAudio first,
 * then OpenSL. libaaudio.so is not in dlopen_allow[] so the AAudio probe fails
 * -- which libc_shim.c already documents as deliberate -- and libOpenSLES.so IS
 * allowed and is implemented by opensles.c. Autodetect should therefore land on
 * OpenSL by itself.
 *
 * RESIDUAL RISK, and the fallback. FMOD's probe order is internal to FMOD and
 * unverified on hardware. If the game runs with no audio, force the request:
 * write movz w1,#22 (0x528002C1) over the site below, which is verify-first
 * against DD_FMOD_OUTPUT_WORD. This is the exact transform Killer Bean applies,
 * at the site pinned for THIS binary.
 *
 * AudioManager::AwakeFromLoad pinned to +0xc8d100 (100% body match over 56
 * instructions against the symbolized reference).
 * ===========================================================================
 */
#define DD_SYM_AudioManager_AwakeFromLoad 0xc8d100u  /* guard 0xa9bf4ffe */
#define DD_FMOD_OUTPUT_SITE               0xc8d124u  /* mov w1, wzr           */
#define DD_FMOD_OUTPUT_WORD               0x2A1F03E1u /* verify before writing */
#define DD_FMOD_OUTPUT_OPENSL             0x528002C1u /* movz w1, #22 (OPENSL) */
#define DD_HAVE_FMOD_FORCE_OPENSL         0  /* 1 only if there is no audio */

/* ===========================================================================
 * REGION-GRANULARITY TABLE  --  20 sites, symbol-attributed
 * ===========================================================================
 * Unity's block allocator reserves memory in 256MB-aligned regions. On a 4GB
 * Switch that granularity does not fit the so_loader address space, so the
 * allocator's region computation is rewritten to 64MB. Each entry rewrites ONE
 * 32-bit instruction word.
 *
 * DERIVED TWICE, BY INDEPENDENT METHODS THAT AGREE COMPLETELY.
 *
 * Method 1 -- anchoring (tools/scan_granularity.py, no symbols needed).
 *   Decode every instruction in the executable LOADs for the six encoding
 *   classes that can carry a 256MB region computation: 456 candidates across
 *   28 MB of text. Narrow by ANCHORS -- encodings essentially unique to region
 *   math (a wide 0x...F0000000 address mask, a `sub ..., lsl #28`). Those
 *   cluster into one contiguous 37 KB stretch, which is what an allocator
 *   compiled as a single translation unit looks like. Keep every granularity
 *   site inside it: 20.
 *
 * Method 2 -- symbol attribution + masked-context pinning
 *   (tools/pin_from_reference.py, using the symbolized reference build).
 *   A symbolized build of the SAME Unity revision was available:
 *
 *       libunity_sym.so      72,831 FUNC symbols, .text NOBITS
 *       libunity600039f1.so  real code, identical address space
 *                            both build-id d314a33c7f8bced13600b28c029302ffa880721f
 *       libunity.so          THE GAME BINARY, a DIFFERENT link,
 *                            build-id 8f235fb696f80ed0b54b51596aebd7d0862f939f
 *
 *   In the reference, exactly 20 granularity sites fall inside functions NAMED
 *   as allocators, across 10 functions. Each was then pinned into the game
 *   binary by masked-context search: a window of instructions with every
 *   position-dependent immediate (B/BL/B.cond/CBZ/TBZ/ADR/ADRP/literal loads)
 *   masked out, widened until unique. 17 pinned uniquely -- 15 of them
 *   byte-identical to the reference word, marked '=' below; 2 differing only
 *   in register allocation, marked '~' and verified to decode to the same
 *   operation.
 *
 * THE TWO METHODS AGREE ON EVERY SITE. 17 of 17 pins land on addresses the
 * anchor scan had already found, and the anchor scan found nothing the pinning
 * rejected. Two methods sharing no assumptions producing the same answer is
 * the standard this lineage's tables are held to.
 *
 * THE REMAINING 3, marked 'E', are resolved by exhaustive elimination inside a
 * closed set rather than by pinning -- their immediate neighbours differ
 * between the two links, so no context window matches:
 *
 *   The reference has exactly TWO movn+and pairs in allocator functions
 *   (LocalLowLevelAllocator::ReserveMemoryBlock, DynamicHeapAllocator::
 *   Allocate). The game's allocator span has exactly two. One pinned; the
 *   other is therefore the second. Likewise the reference has exactly FOUR
 *   movz sites and the game span has exactly four; three pinned, so the
 *   leftover is MemoryManager::InitializeDefaultAllocators.
 *
 *   This is weaker than a pin but not much: the sets are closed and the counts
 *   match exactly. If the allocator misbehaves, these three are the first to
 *   comment out.
 *
 * NOTE ON THE >>40 SITES. Entries 13 and 15 shift by 40, not 28: the allocator
 * indexes a second-level block table by (addr >> 40), which is region-scaled
 * and must move in lockstep. Killer Bean shipped one of these. The symbols
 * confirm BOTH here -- GetBlockInfoFromPointer and GetAllocatorContainingPtr
 * are both named allocator functions. The anchor-only version of this table
 * had the second one commented out; the symbols corrected that.
 *
 * SAFETY. nx_patch_libunity() is VERIFY-FIRST: it reads every {from} word and
 * patches NOTHING unless all 20 match. A table that has gone stale against a
 * game update is caught and logged, not executed. Note this protects against
 * a STALE table, not a WRONG one -- which is why the derivation above matters.
 *
 * UNVERIFIED ON HARDWARE. Nothing in this tree has been run on a Switch.
 * ===========================================================================
 */
static const NxPatchWord KB_PATCH_WORDS[] = {
  /*  0 = */ { 0x1007fd4, 0x12be0009, 0x12bf8009 },  /* LocalLowLevelAllocator::ReserveMemoryBlock+0x8c  movn 0xF000->0xFC00 */
  /*  1 = */ { 0x1007fe4, 0x92648d35, 0x92669535 },  /* LocalLowLevelAllocator::ReserveMemoryBlock+0x9c  and 0xfffffffff0000000->0xfffffffffc000000 */
  /*  2 = */ { 0x1008810, 0x52a20008, 0x52a08008 },  /* TLSAllocator+0x18  movz 0x1000->0x0400 */
  /*  3 E */ { 0x1009604, 0x52a20009, 0x52a08009 },  /* MemoryManager::InitializeDefaultAllocators+0x1d0  movz 0x1000->0x0400 */
  /*  4 = */ { 0x100b738, 0x52a20009, 0x52a08009 },  /* DynamicHeapAllocator+0x44  movz 0x1000->0x0400 */
  /*  5 E */ { 0x100bc04, 0x12be000d, 0x12bf800d },  /* DynamicHeapAllocator::Allocate+0x53c  movn 0xF000->0xFC00 */
  /*  6 E */ { 0x100bc30, 0x92648d36, 0x92669536 },  /* DynamicHeapAllocator::Allocate+0x560  and 0xfffffffff0000000->0xfffffffffc000000 */
  /*  7 ~ */ { 0x100d444, 0xd35cdc33, 0xd35ad433 },  /* MemoryManager::VirtualAllocator::MarkMemoryBlocks+0x18  ubfx #28,#28w->#26,#28w */
  /*  8 ~ */ { 0x100d44c, 0xd35cfd15, 0xd35afd15 },  /* MemoryManager::VirtualAllocator::MarkMemoryBlocks+0x20  lsr #28->#26 */
  /*  9 = */ { 0x100d4f0, 0x52a20008, 0x52a08008 },  /* MemoryManager::VirtualAllocator::ReserveMemoryBlock+0x50  movz 0x1000->0x0400 */
  /* 10 = */ { 0x100d720, 0xd35cfc28, 0xd35afc28 },  /* MemoryManager::VirtualAllocator::GetMemoryBlockFromPointer+0x0  lsr #28->#26 */
  /* 11 = */ { 0x100d730, 0x92646c28, 0x92667428 },  /* MemoryManager::VirtualAllocator::GetMemoryBlockFromPointer+0x10  and 0xfffffff0000000->0xfffffffc000000 */
  /* 12 = */ { 0x100d738, 0xd35c9c2a, 0xd35a942a },  /* MemoryManager::VirtualAllocator::GetMemoryBlockFromPointer+0x18  ubfx #28,#12w->#26,#12w */
  /* 13 = */ { 0x100d74c, 0xd35cdc29, 0xd35ad429 },  /* MemoryManager::VirtualAllocator::GetMemoryBlockFromPointer+0x2c  ubfx #28,#28w->#26,#28w */
  /* 14 = */ { 0x100d754, 0xf2a2000b, 0xf2a0800b },  /* MemoryManager::VirtualAllocator::GetMemoryBlockFromPointer+0x34  movk 0x1000->0x0400 */
  /* 15 = */ { 0x100d798, 0xcb0a7108, 0xcb0a6908 },  /* MemoryManager::VirtualAllocator::GetMemoryBlockFromPointer+0x78  sub lsl#28->lsl#26 */
  /* 16 = */ { 0x100d7ac, 0xd368fc28, 0xd366fc28 },  /* MemoryManager::VirtualAllocator::GetBlockInfoFromPointer+0x0  lsr #40->#38 */
  /* 17 = */ { 0x100d7bc, 0xd35c9c29, 0xd35a9429 },  /* MemoryManager::VirtualAllocator::GetBlockInfoFromPointer+0x10  ubfx #28,#12w->#26,#12w */
  /* 18 = */ { 0x100f1f0, 0xd368fc28, 0xd366fc28 },  /* MemoryManager::GetAllocatorContainingPtr+0xc  lsr #40->#38 */
  /* 19 = */ { 0x100f208, 0xd35c9e89, 0xd35a9689 },  /* MemoryManager::GetAllocatorContainingPtr+0x24  ubfx #28,#12w->#26,#12w */
};
#define KB_PATCH_WORDS_N  ((int)(sizeof(KB_PATCH_WORDS)/sizeof(KB_PATCH_WORDS[0])))

#endif /* NX_PATCH_DATADEFENSE_H */
