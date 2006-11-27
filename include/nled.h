#ifndef __NLED_H
#define __NLED_H

// The following definitions are from:
//
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/mobilesdk5/html/mob5grfNotificationLEDReference.asp
//

struct NLED_COUNT_INFO {
  UINT cLeds;
};

struct NLED_SETTINGS_INFO {
  UINT LedNum;
  INT OffOnBlink;
  LONG TotalCycleTime;
  LONG OnTime;
  LONG OffTime;
  INT MetaCycleOn;
  INT MetaCycleOff;
};

struct NLED_SUPPORTS_INFO {
  UINT LedNum;
  LONG lCycleAdjust;
  BOOL fAdjustTotalCycleTime;
  BOOL fAdjustOnTime;
  BOOL fAdjustOffTime;
  BOOL fMetaCycleOn;
  BOOL fMetaCycleOff;
};

#endif // nled.h
