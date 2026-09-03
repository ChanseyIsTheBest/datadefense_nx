/* dd_touchhook.c -- expose the Switch pointer through UnityEngine.Input.
 *
 * WHY NOT nativeInjectEvent
 * -------------------------
 * This port spent several rounds feeding synthesised Android MotionEvents to
 * UnityPlayer.nativeInjectEvent, and touch never reached the game. The reasons
 * were real but endless:
 *
 *   - the engine screens every InputEvent through a KeyEvent filter first, so a
 *     MotionEvent's getKeyCode() is compared against KEYCODE_VOLUME_DOWN,
 *     VOLUME_UP, ZOOM_OUT and whatever else it happens to check;
 *   - jni_fake.c answers every method on every object, so nothing distinguishes
 *     a MotionEvent from a KeyEvent to the engine;
 *   - a fake MotionEvent has to satisfy every getter Unity calls back into, and
 *     a malformed one faults inside libunity;
 *   - nativeInjectEvent's return value is its own int argument echoed back
 *     (+0xaaddb8 `mov w20, w3`), so there is no signal about whether the event
 *     was even used.
 *
 * clayjam_nx reached the same conclusion on the same engine and took the other
 * route: hook UnityEngine.Input directly in libil2cpp. That is what this file
 * does. It removes the entire JNI surface from the input path -- no MotionEvent,
 * no keycode filter, no class resolution -- and replaces it with seven function
 * pointers whose contract is a C signature.
 *
 * WHY THIS GAME NEEDS BOTH MOUSE AND TOUCH
 * ----------------------------------------
 * Counting call sites in THIS game's libil2cpp (bl targets, not guesses):
 *
 *     Input.get_mousePosition      6
 *     Input.GetTouch               3
 *     Input.GetMouseButtonDown     1
 *     Input.GetMouseButton         0
 *     Input.GetMouseButtonUp       0
 *     Input.get_mousePresent       0
 *     Input.get_touchCount         0
 *     Input.get_touchSupported     0
 *
 * The mouse path dominates, so that is the one that has to work. But GetTouch
 * has callers too, and clayjam records learning this the hard way in reverse:
 * it shipped touchSupported=0 first, and a game whose own code reads touches
 * takes its no-touch branch and never asks for a finger. Denying is not the
 * same as not answering. So both paths are served from one pointer state.
 *
 * The zero-call-site entries are still hooked. An unhooked get_touchSupported
 * returns the engine's native Android answer, and anything reached indirectly
 * -- through uGUI's input modules, which live in this same libil2cpp -- would
 * then disagree with what we report everywhere else.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <stdint.h>
#include <string.h>
#include <switch.h>   /* armGetSystemTick / armTicksToNs for the poll report */

#include "so_util.h"
#include "util.h"
#include "config.h"
#include "nx_pointer.h"
#include "nx_patch_datadefense.h"

/* UnityEngine.Vector2/3 come back in the FP registers; declaring them as C
 * structs of floats gets that right without any assembly. */
typedef struct { float x, y; }    V2;
typedef struct { float x, y, z; } V3;

/* UnityEngine.Touch, 68 bytes. Layout taken from the Il2CppInspector types
 * dump for this build, not guessed. It is returned BY VALUE, which on AArch64
 * means an indirect result in x8 -- declaring the C function as returning the
 * struct makes the compiler do that correctly. */
typedef struct {
    int32_t m_FingerId;                /* 0x00 */
    V2      m_Position;                /* 0x04 */
    V2      m_RawPosition;             /* 0x0c */
    V2      m_PositionDelta;           /* 0x14 */
    float   m_TimeDelta;               /* 0x1c */
    int32_t m_TapCount;                /* 0x20 */
    int32_t m_Phase;                   /* 0x24  TouchPhase */
    int32_t m_Type;                    /* 0x28  TouchType  */
    float   m_Pressure;                /* 0x2c */
    float   m_maximumPossiblePressure; /* 0x30 */
    float   m_Radius;                  /* 0x34 */
    float   m_RadiusVariance;          /* 0x38 */
    float   m_AltitudeAngle;           /* 0x3c */
    float   m_AzimuthAngle;            /* 0x40 */
} UTouch;                              /* 0x44 */

enum { UTP_BEGAN = 0, UTP_MOVED = 1, UTP_STATIONARY = 2, UTP_ENDED = 3 };

/* ---- pointer state, refreshed once per frame ---------------------------- */

static float g_x, g_y;              /* Unity screen space, bottom-left origin */
static float g_prev_x, g_prev_y;
static int   g_down, g_down_prev;
static int   g_installed;
static unsigned g_n_touchcount_calls, g_n_touchcount_nonzero, g_n_gettouch_calls;

void dd_touchhook_tick(void)
{
    if (!g_installed) return;

    NxpEvent ev[16];
    int n = nxp_poll(ev, (int)(sizeof ev / sizeof ev[0]));

    g_down_prev = g_down;
    g_prev_x = g_x; g_prev_y = g_y;

    for (int i = 0; i < n; i++) {
        /* An UP is reported for one frame after release and must not count as
         * still-down, or a tap never produces a ButtonUp / Ended. */
        if (ev[i].phase == NXP_UP) { g_down = 0; continue; }
        g_x = ev[i].x;
        /* Unity's screen origin is BOTTOM-left; the pointer layer and the
         * touchscreen both use top-left. Getting this wrong does not fail
         * loudly -- the game simply responds in the wrong half of the screen. */
        g_y = (float)screen_height - 1.0f - ev[i].y;
        g_down = (ev[i].phase != NXP_UP);
        break;                       /* primary pointer only */
    }

    /* How hard is the game actually polling? Once every ~5s, cheap. */
    { static uint64_t last_ns;
      uint64_t now = armTicksToNs(armGetSystemTick());
      if (now - last_ns > 5000000000ull) {
        last_ns = now;
        debugPrintf("[touch] game polled: touchCount x%u (x%u returned 1), "
                    "GetTouch x%u | pointer down=%d at (%.0f, %.0f)\n",
                    g_n_touchcount_calls, g_n_touchcount_nonzero,
                    g_n_gettouch_calls, g_down, (double)g_x, (double)g_y);
        g_n_touchcount_calls = g_n_touchcount_nonzero = g_n_gettouch_calls = 0; } }

    if (g_down != g_down_prev) {
        static unsigned n_edge;
        if (n_edge < 8) { n_edge++;
            debugPrintf("[touch] pointer %s at (%.0f, %.0f) -- reached the hook layer\n",
                        g_down ? "DOWN" : "UP", (double)g_x, (double)g_y); }
    }
}

/* Report once per distinct API the game actually reads. Three links have to
 * work for a tap to land -- HID reaches the tick, the hooks install on the
 * right addresses, and the GAME calls them -- and "touch does nothing" looks
 * identical whichever is broken. The tick reports the first, the installer the
 * second, this the third. */
static void note_read(const char *who)
{
    static const char *seen[8]; static unsigned n;
    for (unsigned i = 0; i < n; i++) if (seen[i] == who) return;
    if (n < 8) { seen[n++] = who;
        debugPrintf("[touch] game called Input.%s -- it IS reading this API\n", who); }
}

/* ---- the hooks ---------------------------------------------------------- */

static V3 hk_mousePosition(void)
{ note_read("mousePosition"); V3 v; v.x = g_x; v.y = g_y; v.z = 0.0f; return v; }

static uint8_t hk_mousePresent(void)     { note_read("mousePresent");  return 1; }
static uint8_t hk_GetMouseButton(int b)  { note_read("GetMouseButton");
                                           return (b == 0 && g_down) ? 1 : 0; }
static uint8_t hk_GetMouseButtonDown(int b)
{ note_read("GetMouseButtonDown"); return (b == 0 && g_down && !g_down_prev) ? 1 : 0; }
static uint8_t hk_GetMouseButtonUp(int b)
{ return (b == 0 && !g_down && g_down_prev) ? 1 : 0; }

static uint8_t hk_touchSupported(void)   { note_read("touchSupported"); return 1; }

/* The finger must survive its release frame: Unity reports an ending touch for
 * exactly one frame so the game can observe Ended. Dropping to 0 the instant it
 * lifts loses every tap-release. */
static int32_t hk_touchCount(void)
{
    note_read("touchCount");
    int32_t c = (g_down || g_down_prev) ? 1 : 0;
    /* Count how often the game ASKS, not just that it did once. A game polling
     * touchCount every frame and a game that asked twice at startup look
     * identical through note_read(), and they need completely different fixes. */
    g_n_touchcount_calls++;
    if (c) g_n_touchcount_nonzero++;
    return c;
}

static UTouch hk_GetTouch(int index)
{
    note_read("GetTouch");
    g_n_gettouch_calls++;
    UTouch t;
    memset(&t, 0, sizeof t);
    if (index != 0) return t;                 /* one pointer only */
    t.m_FingerId               = 0;
    t.m_Position.x             = g_x;
    t.m_Position.y             = g_y;
    t.m_RawPosition.x          = g_x;
    t.m_RawPosition.y          = g_y;
    t.m_PositionDelta.x        = g_x - g_prev_x;
    t.m_PositionDelta.y        = g_y - g_prev_y;
    t.m_TimeDelta              = 1.0f / 60.0f;
    t.m_TapCount               = 1;
    t.m_Pressure               = 1.0f;
    t.m_maximumPossiblePressure = 1.0f;
    t.m_Type                   = 0;           /* Direct */
    if (g_down && !g_down_prev)      t.m_Phase = UTP_BEGAN;
    else if (!g_down && g_down_prev) t.m_Phase = UTP_ENDED;
    else if (g_x != g_prev_x || g_y != g_prev_y) t.m_Phase = UTP_MOVED;
    else                             t.m_Phase = UTP_STATIONARY;
    /* Report the value handed over on a phase EDGE. Everything upstream of here
     * is now verified -- hooks install, the game calls them, the pointer
     * arrives with the right coordinates -- so if touch still does nothing, the
     * question is what the game receives, and this is the only place that can
     * answer it. Bounded, and edges only. */
    { static int last = -1; static unsigned n;
      if (t.m_Phase != last && n < 24) { n++; last = t.m_Phase;
        static const char *ph[] = { "BEGAN", "MOVED", "STATIONARY", "ENDED" };
        debugPrintf("[touch] GetTouch(%d) -> %s at (%.0f, %.0f) delta (%.0f, %.0f) "
                    "id=%d taps=%d\n", index, ph[t.m_Phase],
                    (double)t.m_Position.x, (double)t.m_Position.y,
                    (double)t.m_PositionDelta.x, (double)t.m_PositionDelta.y,
                    t.m_FingerId, t.m_TapCount); } }
    return t;
}

/* ---- install ------------------------------------------------------------ */

typedef struct { const char *name; uint32_t rva, guard; void *fn; } TouchHook;

int dd_touchhook_install(so_module *il2cpp)
{
    const TouchHook H[] = {
      { "get_mousePosition",  DD_IL2_Input_get_mousePosition,  0xd10083ffu, (void *)hk_mousePosition },
      { "get_mousePresent",   DD_IL2_Input_get_mousePresent,   0xa9bf4ffeu, (void *)hk_mousePresent },
      { "get_touchSupported", DD_IL2_Input_get_touchSupported, 0xa9bf4ffeu, (void *)hk_touchSupported },
      { "get_touchCount",     DD_IL2_Input_get_touchCount,     0xa9bf4ffeu, (void *)hk_touchCount },
      { "GetMouseButton",     DD_IL2_Input_GetMouseButton,     0xf81e0ffeu, (void *)hk_GetMouseButton },
      { "GetMouseButtonDown", DD_IL2_Input_GetMouseButtonDown, 0xf81e0ffeu, (void *)hk_GetMouseButtonDown },
      { "GetMouseButtonUp",   DD_IL2_Input_GetMouseButtonUp,   0xf81e0ffeu, (void *)hk_GetMouseButtonUp },
      { "GetTouch",           DD_IL2_Input_GetTouch,           0xd101c3ffu, (void *)hk_GetTouch },
    };
    const int n = (int)(sizeof H / sizeof H[0]);
    uintptr_t base = (uintptr_t)il2cpp->load_virtbase;
    int applied = 0;

    for (int i = 0; i < n; i++) {
        if (H[i].rva == 0u) {
            /* Absent from this build's metadata, so the game cannot be calling
             * it. Reading base+0 would compare against the ELF header and
             * report a confusing guard mismatch instead. */
            debugPrintf("[touch] %-20s absent from this build -- nothing to hook\n",
                        H[i].name);
            continue;
        }
        volatile uint32_t *site = (uint32_t *)(base + H[i].rva);
        uint32_t got = site[0];
        if (got == 0x58000051u) {          /* ldr x17,#8 -- already hooked */
            debugPrintf("[touch] %-20s already hooked\n", H[i].name);
            applied++;
            continue;
        }
        if (got != H[i].guard) {
            debugPrintf("[touch] %-20s SKIP (guard %08x, found %08x) -- re-derive\n",
                        H[i].name, H[i].guard, got);
            continue;
        }
        /* NOT hook_arm64(): it stores straight into module text, which
         * so_finalize has already mapped RX. Writing there is a permission
         * fault, and that is exactly how the first version of this file
         * crashed:
         *
         *     pc=...  far=<il2cpp_base + 0x2366984>  esr=9200004f
         *     DFSC 0x0f = permission fault, WnR=1 = write
         *
         * The stack even held the table being walked. Build the same 16-byte
         * thunk hook_arm64 would and write it through so_patch_code's writable
         * alias -- the pattern main.c already uses for the il2cpp liveness
         * guard, and for the same reason. */
        uint32_t thunk[4];
        const uint64_t dst = (uint64_t)(uintptr_t)H[i].fn;
        thunk[0] = 0x58000051u;            /* ldr x17, #8 */
        thunk[1] = 0xd61f0220u;            /* br  x17     */
        memcpy(&thunk[2], &dst, sizeof dst);
        if (so_patch_code((void *)site, thunk, sizeof thunk) != 0) {
            debugPrintf("[touch] %-20s so_patch_code FAILED\n", H[i].name);
            continue;
        }
        applied++;
    }
    if (applied) so_flush_caches(il2cpp);   /* the thunks are code we just wrote */
    g_installed = (applied > 0);
    debugPrintf("[touch] %d/%d Input hooks applied (pointer served as BOTH mouse "
                "and single touch)\n", applied, n);
    return applied;
}
