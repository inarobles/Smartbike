#ifndef UI_WIFI_H
#define UI_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void ui_open_wifi_list(void);
void ui_loading_complete(void);
void ui_upload_complete(bool success);

#ifdef __cplusplus
}
#endif

#endif // UI_WIFI_H
