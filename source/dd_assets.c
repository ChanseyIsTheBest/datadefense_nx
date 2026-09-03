/* dd_assets.c -- find and validate the game's files at boot, and decide
 * whether packing them is worth doing.
 *
 * WHAT THIS REPLACES
 * ------------------
 * The inherited tree did three things here that are wrong for Data Defense:
 *
 *  1. Its "have the splits already been joined?" fast path probed a file list
 *     from Killer Bean, including sharedassets1.resource. Data Defense has no
 *     such file, so the probe failed EVERY boot and fell through to a full
 *     recursive readdir of assets/ looking for .splitN parts that do not
 *     exist. Wasted work on every launch, and a confusing log.
 *
 *  2. It built an asset pack unconditionally, then DELETED the loose assets/
 *     tree with nx_rmtree(). The packer exists because Zookeeper DX ships
 *     ~4,200 loose files and per-file open costs far more on Switch than on
 *     Android -- its first scene load took 20+ minutes unpacked. Data Defense
 *     ships NINE files, two of which are 29 MB and 30 MB. Packing them saves
 *     nothing, needs ~60 MB of extra SD space while it runs, and ends by
 *     deleting the files the player copied over. That is all cost.
 *
 *  3. When something was missing it named one file and gave up. The actual
 *     mistakes people make are structural -- the assets/ level dropped, the
 *     whole APK extracted verbatim, an interrupted copy -- and naming one file
 *     does not point at any of them.
 *
 * WHAT IT DOES INSTEAD
 * --------------------
 * Detects where the data actually is, says exactly how to fix it when the
 * layout is wrong, distinguishes "missing" from "wrong size", and only packs
 * when packing would help.
 *
 * ON SIZE CHECKING. Sizes come from Data Defense 1.3.11 and a mismatch is a
 * WARNING, never fatal: a different version of the game is a perfectly normal
 * thing to have, and refusing to boot over it would be obnoxious. But a size
 * that is *smaller* than expected is worth shouting about, because a truncated
 * copy -- SD card pulled early, a copy that ran out of space -- presents later
 * as a corrupt-asset crash deep inside the engine with nothing pointing back
 * at the real cause.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "dd_assets.h"
#include "error.h"
#include "nx_data_root.h"

extern void debugPrintf(const char *fmt, ...);

/* ------------------------------------------------------------------------- */

typedef struct {
    const char *rel;        /* path relative to the game folder */
    uint64_t    size;       /* expected bytes in 1.3.11; 0 = do not check */
    int         required;   /* 0 = nice to have, 1 = cannot boot without it */
} DdFile;

/* Data Defense 1.3.11, com.IIBlocks.DataDefense, Unity 6000.3.9f1.
 * Sizes are the APK's uncompressed entry sizes. */
static const DdFile DD_FILES[] = {
    /* native code -- never packed, always loose */
    { "libmain.so",                                          6696, 1 },
    { "libunity.so",                                     28079064, 1 },
    { "libil2cpp.so",                                    42992840, 1 },

    /* the engine's data */
    { "assets/bin/Data/data.unity3d",                    29327787, 1 },
    { "assets/bin/Data/resources.resource",              30651168, 1 },
    { "assets/bin/Data/Managed/Metadata/global-metadata.dat",
                                                          5530176, 1 },
    { "assets/bin/Data/Resources/unity default resources", 3783412, 1 },
    { "assets/bin/Data/boot.config",                          222, 1 },

    /* present in the APK, and the engine copes without them -- but their
     * absence means a partial copy, so say so. */
    { "assets/bin/Data/Managed/Resources/mscorlib.dll-resources.dat",
                                                           337563, 0 },
    { "assets/bin/Data/ScriptingAssemblies.json",            3194, 0 },
    { "assets/bin/Data/RuntimeInitializeOnLoads.json",        703, 0 },
    { "assets/bin/Data/unity_app_guid",                        36, 0 },
};
#define DD_FILES_N ((int)(sizeof(DD_FILES) / sizeof(DD_FILES[0])))

/* The file we look for to decide "is the asset tree here, and where?".
 * Every layout mistake below is expressed as a prefix relative to the game
 * folder that would make this path resolve. */
#define DD_BEACON "bin/Data/data.unity3d"

/* Layouts people actually end up with, and what each one means. Order matters:
 * the correct layout is checked first so the normal case costs one stat(). */
static const struct { const char *prefix; const char *diagnosis; } LAYOUTS[] = {
    { "assets",
      NULL },                                   /* correct */
    { "",
      "The 'assets' level is missing -- you copied the CONTENTS of assets/ "
      "instead of the folder itself." },
    { "assets/assets",
      "There is an extra 'assets' level -- the folder was copied into an "
      "assets/ folder that already existed." },
    { "Data",
      "You copied bin/Data's contents as 'Data/'. The engine needs the full "
      "assets/bin/Data path." },
    { "bin",
      "The 'assets' level is missing above 'bin'." },
};
#define LAYOUTS_N ((int)(sizeof(LAYOUTS) / sizeof(LAYOUTS[0])))

/* ------------------------------------------------------------------------- */

static int stat_size(const char *abs, uint64_t *size)
{
    struct stat st;
    if (stat(abs, &st) < 0) return 0;
    if (size) *size = (uint64_t)st.st_size;
    return 1;
}

static int probe(const char *prefix, const char *rel, uint64_t *size)
{
    char sub[768];
    if (prefix && *prefix)
        snprintf(sub, sizeof sub, "/%s/%s", prefix, rel);
    else
        snprintf(sub, sizeof sub, "/%s", rel);
    return stat_size(nx_path(sub), size);
}

/* Count the files under a directory, stopping once the count passes `cap`.
 * The number itself does not matter beyond the pack decision, and a full walk
 * of a large tree is exactly the cost this module exists to avoid. */
static unsigned count_files(const char *abs, unsigned cap, int depth)
{
    if (depth > 8) return 0;
    DIR *d = opendir(abs);
    if (!d) return 0;
    unsigned n = 0;
    struct dirent *e;
    char child[768];
    while (n < cap && (e = readdir(d)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        snprintf(child, sizeof child, "%s/%s", abs, e->d_name);
        struct stat st;
        if (stat(child, &st) == 0 && S_ISDIR(st.st_mode))
            n += count_files(child, cap - n, depth + 1);
        else
            n++;
    }
    closedir(d);
    return n;
}

/* ------------------------------------------------------------------------- */

static DdAssetReport g_report;

const DdAssetReport *dd_assets_report(void) { return &g_report; }

int dd_assets_detect(void)
{
    memset(&g_report, 0, sizeof g_report);
    g_report.layout = -1;

    for (int i = 0; i < LAYOUTS_N; i++) {
        if (probe(LAYOUTS[i].prefix, DD_BEACON, NULL)) {
            g_report.layout = i;
            snprintf(g_report.prefix, sizeof g_report.prefix, "%s",
                     LAYOUTS[i].prefix);
            break;
        }
    }

    /* Are the three .so files where they should be? A very common mistake is
     * copying lib/arm64-v8a/ across as a folder rather than its contents. */
    g_report.libs_ok = probe("", "libunity.so", NULL) &&
                       probe("", "libil2cpp.so", NULL) &&
                       probe("", "libmain.so", NULL);
    if (!g_report.libs_ok) {
        if (probe("lib/arm64-v8a", "libunity.so", NULL))
            g_report.libs_where = DD_LIBS_IN_LIB_DIR;
        else if (probe("arm64-v8a", "libunity.so", NULL))
            g_report.libs_where = DD_LIBS_IN_ABI_DIR;
        else
            g_report.libs_where = DD_LIBS_MISSING;
    }

    if (g_report.layout < 0) return 0;

    /* Per-file check against the manifest. */
    for (int i = 0; i < DD_FILES_N; i++) {
        const DdFile *f = &DD_FILES[i];
        const char *rel = f->rel;
        const char *prefix = "";
        /* Manifest paths are written with the assets/ prefix; strip it and
         * re-apply whatever prefix this card actually uses. */
        if (!strncmp(rel, "assets/", 7)) { rel += 7; prefix = g_report.prefix; }

        uint64_t got = 0;
        if (!probe(prefix, rel, &got)) {
            if (f->required) {
                if (g_report.n_missing < DD_REPORT_MAX)
                    g_report.missing[g_report.n_missing] = f->rel;
                g_report.n_missing++;
            } else {
                if (g_report.n_absent_optional < DD_REPORT_MAX)
                    g_report.absent_optional[g_report.n_absent_optional] = f->rel;
                g_report.n_absent_optional++;
            }
            continue;
        }
        g_report.n_found++;
        g_report.bytes += got;
        if (f->size && got != f->size) {
            if (g_report.n_odd_size < DD_REPORT_MAX) {
                g_report.odd_size[g_report.n_odd_size] = f->rel;
                g_report.odd_expected[g_report.n_odd_size] = f->size;
                g_report.odd_actual[g_report.n_odd_size] = got;
                g_report.odd_truncated[g_report.n_odd_size] = (got < f->size);
            }
            g_report.n_odd_size++;
            if (got < f->size) g_report.n_truncated++;
        }
    }

    /* How many loose files are under the ASSET TREE? Only used for the pack
     * decision, so stop counting well before a full walk.
     *
     * Count bin/Data, not the game folder: the folder also holds the three
     * .so files and the log, and the packer never touches those. Counting from
     * the root also meant the empty-prefix layout counted everything on the
     * card under the game folder. */
    char sub[768];
    if (g_report.prefix[0])
        snprintf(sub, sizeof sub, "/%s/bin/Data", g_report.prefix);
    else
        snprintf(sub, sizeof sub, "/bin/Data");
    g_report.loose_files = count_files(nx_path(sub), DD_PACK_THRESHOLD * 4, 0);

    return g_report.n_missing == 0;
}

void dd_assets_log(void)
{
    const DdAssetReport *r = &g_report;
    debugPrintf("[assets] root: %s\n", DATA_ROOT);

    if (r->layout < 0) {
        debugPrintf("[assets] NO ASSET TREE FOUND (looked for %s under each of "
                    "%d known layouts)\n", DD_BEACON, LAYOUTS_N);
        return;
    }
    if (LAYOUTS[r->layout].diagnosis)
        debugPrintf("[assets] layout: NON-STANDARD (\"%s/\") -- %s\n",
                    r->prefix, LAYOUTS[r->layout].diagnosis);
    else
        debugPrintf("[assets] layout: standard (assets/bin/Data/...)\n");

    debugPrintf("[assets] %d of %d manifest files present, %llu MB, "
                "%u loose file(s) under the asset tree\n",
                r->n_found, DD_FILES_N,
                (unsigned long long)(r->bytes >> 20), r->loose_files);

    for (int i = 0; i < r->n_absent_optional && i < DD_REPORT_MAX; i++)
        debugPrintf("[assets] optional file absent: %s\n",
                    r->absent_optional[i]);

    for (int i = 0; i < r->n_odd_size && i < DD_REPORT_MAX; i++)
        debugPrintf("[assets] %s: %s -- have %llu bytes, "
                    "Data Defense 1.3.11 has %llu%s\n",
                    r->odd_truncated[i] ? "TRUNCATED?" : "size differs",
                    r->odd_size[i],
                    (unsigned long long)r->odd_actual[i],
                    (unsigned long long)r->odd_expected[i],
                    r->odd_truncated[i]
                        ? "  *** smaller than expected: re-copy this file ***"
                        : "  (probably just a different game version)");

    if (r->n_odd_size && !r->n_truncated)
        debugPrintf("[assets] sizes differ but nothing is short -- most likely "
                    "a game version other than 1.3.11. Re-run "
                    "tools/scan_granularity.py against YOUR libunity.so if the "
                    "patch table reports a word mismatch.\n");
}

/* ------------------------------------------------------------------------- */

int dd_assets_should_pack(void)
{
#if !KB_ASSET_PACK
    /* Say so rather than returning silently. "No [pack] line in the log" is
     * indistinguishable from "the pack code never ran because something threw
     * earlier", and that ambiguity costs more than one line of output. */
    debugPrintf("[pack] compiled out (KB_ASSET_PACK 0). Data Defense ships %u "
                "loose asset file(s); the packer exists for games that ship "
                "thousands of small ones.\n", g_report.loose_files);
    return 0;
#else
    if (g_report.layout < 0) return 0;
    if (g_report.loose_files >= DD_PACK_THRESHOLD) {
        debugPrintf("[pack] %u loose asset files (>= %d): packing is worth it\n",
                    g_report.loose_files, DD_PACK_THRESHOLD);
        return 1;
    }
    debugPrintf("[pack] %u loose asset files -- below the %d-file threshold, "
                "so packing is skipped. It would cost ~%llu MB of scratch "
                "space and end by deleting the loose tree, to save nothing: "
                "the packer exists for games that ship thousands of small "
                "files, and this one ships a handful of large ones.\n",
                g_report.loose_files, DD_PACK_THRESHOLD,
                (unsigned long long)(g_report.bytes >> 20));
    return 0;
#endif
}

void dd_assets_fatal_if_incomplete(void)
{
    const DdAssetReport *r = &g_report;

    if (r->layout < 0) {
        if (r->libs_where == DD_LIBS_IN_LIB_DIR ||
            r->libs_where == DD_LIBS_IN_ABI_DIR)
            fatal_error(
                "Game files are in the wrong place.\n\n"
                "Found the .so files inside a subfolder, and no asset tree.\n"
                "It looks like the APK was extracted here as-is.\n\n"
                "Expected, directly in:\n%s\n\n"
                "  datadefense_nx.nro\n"
                "  libmain.so  libunity.so  libil2cpp.so   (from lib/arm64-v8a/)\n"
                "  assets/bin/Data/...                     (the assets folder)\n\n"
                "Move the three .so files up out of lib/arm64-v8a/, and copy\n"
                "the assets folder itself (not its contents).", DATA_ROOT);

        fatal_error(
            "No game data found.\n\n"
            "Looked in:\n%s\n\n"
            "Expected to find:\n  assets/bin/Data/data.unity3d\n\n"
            "Copy these out of your Data Defense APK (it is a zip):\n"
            "  lib/arm64-v8a/*.so  ->  next to the .nro\n"
            "  assets/            ->  next to the .nro\n\n"
            "If the files ARE there, check the folder name: this build\n"
            "resolves its root from where the .nro was launched.", DATA_ROOT);
    }

    if (!r->libs_ok) {
        const char *where =
            r->libs_where == DD_LIBS_IN_LIB_DIR  ? "lib/arm64-v8a/" :
            r->libs_where == DD_LIBS_IN_ABI_DIR  ? "arm64-v8a/"     : NULL;
        if (where)
            fatal_error(
                "The game libraries are one folder too deep.\n\n"
                "Found them in:\n%s%s\n\n"
                "Move libmain.so, libunity.so and libil2cpp.so up so they sit\n"
                "directly beside datadefense_nx.nro.", DATA_ROOT, where);
        fatal_error(
            "Missing game libraries.\n\n"
            "Need all three, directly in:\n%s\n\n"
            "  libmain.so  libunity.so  libil2cpp.so\n\n"
            "They are in your APK under lib/arm64-v8a/.\n"
            "An armeabi-v7a build will NOT work -- this loader needs arm64.",
            DATA_ROOT);
    }

    if (r->n_missing) {
        const char *fix = LAYOUTS[r->layout].diagnosis;
        fatal_error(
            "Game data is incomplete: %d required file(s) missing.\n\n"
            "First missing: %s\n\n"
            "Looked in: %s\n%s%s\n"
            "Re-copy the assets folder from your APK.",
            r->n_missing, r->missing[0], DATA_ROOT,
            fix ? "\nLayout problem detected: " : "",
            fix ? fix : "");
    }

    if (r->n_truncated)
        debugPrintf("[assets] *** %d file(s) are SMALLER than expected. If the "
                    "game crashes while loading, re-copy them -- a copy that "
                    "was interrupted shows up as corrupt assets deep inside "
                    "the engine, nowhere near the real cause. ***\n",
                    r->n_truncated);
}
