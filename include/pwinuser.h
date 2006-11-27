#ifndef __PWINUSER_H
#define __PWINUSER_H

// The following definitions are from:
//
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/mobilesdk5/html/mob5grfNotificationLEDReference.asp
//

#ifdef __cplusplus
extern "C" {
#endif

#define NLED_SETTINGS_INFO_ID 2

BOOL WINAPI NLedSetDevice(UINT nDeviceId, void* pInput);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // pwinuser.h
