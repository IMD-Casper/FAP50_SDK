// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the FAP50PLUSSDKPANEL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// FAP50PLUSSDKPANEL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifndef FAP50PLUSSDKPANEL_H
#define FAP50PLUSSDKPANEL_H

#include <stdint.h>
#include <stdbool.h>
#include "InterfaceImdFap50Method.h"

#ifdef __cplusplus
extern "C" {
#endif

    // Exported functions
    IMD_STD_API bool connect_fap50_panel(const char* host, uint16_t port);
    IMD_STD_API bool send_jpg_fap50_panel(const uint8_t* buffer, uint32_t size);
    IMD_STD_API void disconnect_fap50_panel();

#ifdef __cplusplus
}
#endif

#endif //FAP50PLUSSDKPANEL_H