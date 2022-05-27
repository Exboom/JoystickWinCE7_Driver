#ifndef _JOYHID_H_
#define _JOYSTICKHID_H_

#include "hidpi.h"
#include "hiddi.h"
#include <cedrv_guid.h>

#define HID_JOYSTICK_SIG 'maGH' // "HGam"

typedef struct _HID_JOYSTICK {
	DWORD dwSig;
    HID_HANDLE hDevice;
    PCHID_FUNCS pHidFuncs;
    PHIDP_PREPARSED_DATA phidpPreparsedData;
    HIDP_CAPS hidpCaps;
    HANDLE hThread;

	DWORD dwMaxUsages;
    PUSAGE puPrevUsages;
    PUSAGE puCurrUsages;
    PUSAGE puBreakUsages;
    PUSAGE puMakeUsages;

	ULONG prevDirX;
	ULONG prevDirY;
	ULONG prevDirZ;
} HID_JOYSTICK, *PHID_JOYSTICK;

#define VALID_HID_JOYSTICK( p ) \
   ( p && (HID_JOYSTICK_SIG == p->dwSig) )

#define dim(x) (sizeof(x)/sizeof(x[0]))

#define LEFT 0
#define CENTER 2048
#define RIGHT 4095
#define DOWN 4095
#define UP 0

#define CENTER_Z 1024
#define LEFT_Z 0
#define RIGHT_Z 2047

#endif