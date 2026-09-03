/* dd_assets.h -- boot-time detection and validation of the game's files.
 *
 * Call order in main(), after nx_resolve_data_root() and before anything
 * touches the asset tree:
 *
 *     dd_assets_detect();              // stat the manifest, work out layout
 *     dd_assets_log();                 // one block in debug.log
 *     dd_assets_fatal_if_incomplete(); // actionable error, or return
 *     ... if (dd_assets_should_pack()) ...
 *
 * dd_assets_log() runs BEFORE the fatal check on purpose: when the layout is
 * wrong the log is the only place the reason survives, and a player who hits
 * the error screen can be asked for debug.log.
 */
#ifndef DD_ASSETS_H
#define DD_ASSETS_H

#include <stdint.h>

/* Pack below this many loose files and you spend more than you save. Zookeeper
 * DX (~4,200 files) needed it; Data Defense (9) does not. The number is a
 * threshold rather than a per-game flag so a future content update that starts
 * shipping many small files turns packing back on by itself. */
#define DD_PACK_THRESHOLD 400

#define DD_REPORT_MAX 8

enum {
    DD_LIBS_MISSING = 0,   /* not found anywhere we looked   */
    DD_LIBS_IN_LIB_DIR,    /* still in lib/arm64-v8a/        */
    DD_LIBS_IN_ABI_DIR,    /* still in arm64-v8a/            */
};

typedef struct {
    int      layout;            /* index into the known-layouts table, -1 none */
    char     prefix[64];        /* asset prefix that actually worked           */

    int      libs_ok;
    int      libs_where;        /* DD_LIBS_* when !libs_ok                     */

    int      n_found;
    uint64_t bytes;
    unsigned loose_files;       /* capped count under the asset tree           */

    int         n_missing;
    const char *missing[DD_REPORT_MAX];

    int         n_absent_optional;
    const char *absent_optional[DD_REPORT_MAX];

    int         n_odd_size;
    int         n_truncated;
    const char *odd_size[DD_REPORT_MAX];
    uint64_t    odd_expected[DD_REPORT_MAX];
    uint64_t    odd_actual[DD_REPORT_MAX];
    int         odd_truncated[DD_REPORT_MAX];
} DdAssetReport;

/* Returns 1 when every REQUIRED file was found. */
int dd_assets_detect(void);

const DdAssetReport *dd_assets_report(void);
void dd_assets_log(void);
void dd_assets_fatal_if_incomplete(void);
int  dd_assets_should_pack(void);

#endif /* DD_ASSETS_H */
