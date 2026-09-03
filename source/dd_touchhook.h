/* dd_touchhook.h -- UnityEngine.Input hooks; see dd_touchhook.c for why this
 * replaces the nativeInjectEvent path entirely. */
#ifndef DD_TOUCHHOOK_H
#define DD_TOUCHHOOK_H
#include "so_util.h"
int  dd_touchhook_install(so_module *il2cpp);  /* returns hooks applied */
void dd_touchhook_tick(void);                  /* once per frame, before render */
#endif
