#include "joyhid.h"
#include <windev.h>
#include <svsutil.hxx>

static void ProcessJoystickReport (PHID_JOYSTICK pHidJoystick,PCHAR pbHidPacket,DWORD cbHidPacket);

static BOOL
AllocateUsageLists(PHID_JOYSTICK pHidJoystick, size_t cbUsages );

// Dll entry function.
extern "C" 
BOOL
DllEntry
(
	HANDLE	hDllHandle, 
	DWORD	dwReason, 
	LPVOID	lpReserved
)
{
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls((HMODULE) hDllHandle);
            break;
            
        case DLL_PROCESS_DETACH:
            break;
            
        default:
            break;
    }
    return TRUE ;
}

// Get interrupt reports from the device. Thread exits when the device has
// been removed.
static 
DWORD 
WINAPI
JoystickThreadProc( LPVOID lpParameter )
{
    PHID_JOYSTICK pHidJoystick = (PHID_JOYSTICK) lpParameter;
    PCHAR pbHidPacket;
    DWORD cbHidPacket;
    DWORD cbBuffer;
    DWORD dwErr;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    cbBuffer = pHidJoystick->hidpCaps.InputReportByteLength;
    pbHidPacket = (PCHAR) LocalAlloc(LMEM_FIXED, cbBuffer);
    if (pbHidPacket == NULL) return 0;
    while (TRUE) 
    {
        // Get an interrupt report from the device.
        dwErr = pHidJoystick->pHidFuncs->lpGetInterruptReport
		(
            pHidJoystick->hDevice,
            pbHidPacket,
            cbBuffer,
            &cbHidPacket,
            NULL,
            INFINITE
		);
        if (dwErr == ERROR_DEVICE_REMOVED) break; // Exit thread 
        ProcessJoystickReport(pHidJoystick, pbHidPacket, cbHidPacket);
    }
    if (pbHidPacket != NULL) LocalFree(pbHidPacket);
    return 0;
}

// Free the memory used by pHidKbd
void
FreeHidJoystick
(
    PHID_JOYSTICK pHidJoystick
)
{
    if (pHidJoystick->puPrevUsages != NULL) LocalFree(pHidJoystick->puPrevUsages);
    LocalFree(pHidJoystick);    
}

// Entry point for the HID driver. Initializes the structures for this 
// keyboard and starts the thread that will receive interrupt reports.
extern 
"C" 
BOOL
HIDDeviceAttach
(
    HID_HANDLE                 hDevice, 
    PCHID_FUNCS                pHidFuncs,
    const HID_DRIVER_SETTINGS *pDriverSettings,
    PHIDP_PREPARSED_DATA       phidpPreparsedData,
    PVOID                     *ppvNotifyParameter,
    DWORD                      dwUnused
)
{
    PHID_JOYSTICK pHidJoystick;
    size_t cbUsages;

    pHidJoystick = (PHID_JOYSTICK) LocalAlloc(LPTR, sizeof(HID_JOYSTICK));

    if (pHidJoystick == NULL) return false;

    pHidJoystick->dwSig = HID_JOYSTICK_SIG;
    pHidJoystick->hDevice = hDevice;
    pHidJoystick->pHidFuncs = pHidFuncs;
    pHidJoystick->phidpPreparsedData = phidpPreparsedData;
	pHidJoystick->prevDirX = 2048;
	pHidJoystick->prevDirY = 2048;
	pHidJoystick->prevDirZ = 1024;
    HidP_GetCaps(pHidJoystick->phidpPreparsedData, &pHidJoystick->hidpCaps);


    // Get the total number of usages that can be returned in an input packet.
    pHidJoystick->dwMaxUsages = HidP_MaxUsageListLength
								(
									HidP_Input, 
									HID_USAGE_PAGE_BUTTON, 
									phidpPreparsedData
								);
    cbUsages = pHidJoystick->dwMaxUsages * sizeof(USAGE);

    if (AllocateUsageLists(pHidJoystick, cbUsages) == FALSE)
	{ 
		FreeHidJoystick(pHidJoystick);
		return false;
	}

    // Create the thread that will receive reports from this device
    pHidJoystick->hThread = CreateThread
							(
								NULL, 
								0, 
								JoystickThreadProc, 
								pHidJoystick, 
								0, 
								NULL
							);
    if (pHidJoystick->hThread == NULL) return false;

    *ppvNotifyParameter = pHidJoystick;

/////////////////////////////////////////////////////////////////////////////////////////////
		wchar_t* msg = (wchar_t *)malloc(300 * sizeof (wchar_t));
		//swprintf(msg, L"Joystick Attached");
		//swprintf_s(
		MessageBox (NULL, msg, L"Dr.Modi's Joystick driver v0.1", MB_OK);
		free(msg);
/////////////////////////////////////////////////////////////////////////////////////////////

	return true;
}

//AllocateUsageLists
static 
BOOL
AllocateUsageLists
(
    PHID_JOYSTICK pHidJoystick,
    size_t cbUsages
)
{
    size_t cbTotal;
    PBYTE pbStart;
    DWORD dwIdx;
    PUSAGE *rgppUsages[] = 
	{
        &pHidJoystick->puPrevUsages,
        &pHidJoystick->puCurrUsages,
        &pHidJoystick->puBreakUsages,
        &pHidJoystick->puMakeUsages
    };


    // Allocate a block of memory for all the usage lists
	cbTotal = cbUsages * dim(rgppUsages);
    pbStart = (PBYTE) LocalAlloc(LPTR, cbTotal);
    
    if (pbStart == NULL) return false;

    // Divide the block.
    for (dwIdx = 0; dwIdx < dim(rgppUsages); ++dwIdx) {
        *rgppUsages[dwIdx] = (PUSAGE) (pbStart + (cbUsages * dwIdx));
    }


    return true;
}

// Entry point for the HID driver to give us notifications.
extern "C" 
BOOL 
WINAPI
HIDDeviceNotifications(
    DWORD  dwMsg,
    WPARAM wParam, // Message parameter
    PVOID  pvNotifyParameter
    )
{
	PHID_JOYSTICK pHidJoystick = (PHID_JOYSTICK) pvNotifyParameter;
    DWORD dwFlags = 0;

    if (VALID_HID_JOYSTICK(pHidJoystick) == FALSE) return false;

  
    switch(dwMsg) {
        case HID_CLOSE_DEVICE:
            // Free all of our resources.
            WaitForSingleObject(pHidJoystick->hThread, INFINITE);
            CloseHandle(pHidJoystick->hThread);
            pHidJoystick->hThread = NULL;

            // Send ups for each button that is still down.
                        
            FreeHidJoystick(pHidJoystick);
            return true;
            break;
        default:
            break;
    };

    return true;
}

static void
SendButtonMessages
(
    bool upOrDown,
    PUSAGE pUsages,
    DWORD dwMaxUsages
)
{
    DWORD dwIdx;
	DWORD charToSend = 0;
    for (dwIdx = 0; dwIdx < dwMaxUsages; ++dwIdx) 
	{
        const USAGE usage = pUsages[dwIdx];

		if (usage == 0) break;// No more set usages
		switch (usage)
		{
		case 1:
			keybd_event(VK_NUMPAD1 ,MapVirtualKey(VK_NUMPAD1, 1), upOrDown ? KEYEVENTF_KEYUP : 0, 0);
			break;
		case 2:
			keybd_event(VK_NUMPAD2 ,MapVirtualKey(VK_NUMPAD2, 1), upOrDown ? KEYEVENTF_KEYUP : 0, 0);
			break;
		case 3:
			keybd_event(VK_NUMPAD3 ,MapVirtualKey(VK_NUMPAD3, 1), upOrDown ? KEYEVENTF_KEYUP : 0, 0);
			break;
		case 4:
			keybd_event(VK_NUMPAD4 ,MapVirtualKey(VK_NUMPAD4, 1), upOrDown ? KEYEVENTF_KEYUP : 0, 0);
			break;
		case 5:
			keybd_event(VK_NUMPAD5 ,MapVirtualKey(VK_NUMPAD5, 1), upOrDown ? KEYEVENTF_KEYUP : 0, 0);
			break;
		case 6:
			keybd_event(VK_NUMPAD6 ,MapVirtualKey(VK_NUMPAD6, 1), upOrDown ? KEYEVENTF_KEYUP : 0, 0);
			break;
		case 7:
			keybd_event(VK_NUMPAD7 ,MapVirtualKey(VK_NUMPAD7, 1), upOrDown ? KEYEVENTF_KEYUP : 0, 0);
			break;
		case 8:
			keybd_event(VK_NUMPAD8 ,MapVirtualKey(VK_NUMPAD8, 1), upOrDown ? KEYEVENTF_KEYUP : 0, 0);
			break;
		}
    }    
}


static void
SendDirMessages
(
    USAGE usage,
    ULONG value,
	ULONG *pPrevDirValue
)
{
	ULONG prevDirValue = *pPrevDirValue;
	switch (usage)
	{
	case HID_USAGE_GENERIC_X:
		if (value == prevDirValue) break;
		switch (prevDirValue)
		{
		case (LEFT):
			keybd_event(VK_LEFT, MapVirtualKey(VK_LEFT, 0), KEYEVENTF_KEYUP, 0);
			break;
		case (RIGHT):
			keybd_event(VK_RIGHT, MapVirtualKey(VK_RIGHT, 0), KEYEVENTF_KEYUP, 0);
			break;
		}
		switch (value)
		{
		case (LEFT):
			keybd_event(VK_LEFT, MapVirtualKey(VK_LEFT, 0), 0, 0);
			break;
		case (RIGHT):
			keybd_event(VK_RIGHT, MapVirtualKey(VK_RIGHT, 0), 0, 0);
			break;
		}
		break;
	case HID_USAGE_GENERIC_Y:
		if (value == prevDirValue) break;
		switch (prevDirValue)
		{
		case (UP):
			keybd_event(VK_UP, MapVirtualKey(VK_UP, 0), KEYEVENTF_KEYUP, 0);
			break;
		case (DOWN):
			keybd_event(VK_DOWN, MapVirtualKey(VK_DOWN, 0), KEYEVENTF_KEYUP, 0);
			break;
		}
		switch (value)
		{
		case (UP):
			keybd_event(VK_UP, MapVirtualKey(VK_UP, 0), 0, 0);
			break;
		case (DOWN):
			keybd_event(VK_DOWN, MapVirtualKey(VK_DOWN, 0), 0, 0);
			break;
		}
		break;
	case HID_USAGE_GENERIC_Z:
		if (value == prevDirValue) break;
		switch (prevDirValue)
		{
		case (LEFT_Z):
			keybd_event(VK_LEFT, MapVirtualKey(VK_LEFT, 0), KEYEVENTF_KEYUP, 0);
			break;
		case (RIGHT_Z):
			keybd_event(VK_RIGHT, MapVirtualKey(VK_RIGHT, 0), KEYEVENTF_KEYUP, 0);
			break;
		}
		switch (value)
		{
		case (LEFT_Z):
			keybd_event(VK_LEFT, MapVirtualKey(VK_LEFT, 0), 0, 0);
			break;
		case (RIGHT_Z):
			keybd_event(VK_RIGHT, MapVirtualKey(VK_RIGHT, 0), 0, 0);
			break;
		}

	}
	*pPrevDirValue = value;
}

static void
ProcessJoystickReport
(
    PHID_JOYSTICK pHidJoystick,
    PCHAR pbHidPacket,
    DWORD cbHidPacket
)
{

    NTSTATUS status;
    ULONG uCurrUsages;
    DWORD cbUsageList;
    DWORD dwFlags;
    LONG dx, dy, dz;
    
    dwFlags = dx = dy = dz = 0;

    uCurrUsages = pHidJoystick->dwMaxUsages;
    cbUsageList = uCurrUsages * sizeof(USAGE);

    //
    // Handle mouse buttons
    //

    // Get the usage list from this packet.
    status = HidP_GetUsages(
        HidP_Input,
        HID_USAGE_PAGE_BUTTON,
        0,
        pHidJoystick->puCurrUsages,
        &uCurrUsages, // IN OUT parameter
        pHidJoystick->phidpPreparsedData,
        pbHidPacket,
        cbHidPacket);

    status = HidP_UsageListDifference
		(
			pHidJoystick->puPrevUsages,
			pHidJoystick->puCurrUsages,
			pHidJoystick->puBreakUsages,
			pHidJoystick->puMakeUsages,
			pHidJoystick->dwMaxUsages
		);

    // Determine which buttons went up and down.
    SendButtonMessages(true, pHidJoystick->puBreakUsages, pHidJoystick->dwMaxUsages);
    SendButtonMessages(false, pHidJoystick->puMakeUsages, pHidJoystick->dwMaxUsages);

	ULONG dirPressed = 0;

	status = HidP_GetUsageValue(
        HidP_Input,
        HID_USAGE_PAGE_GENERIC,
        0,
        HID_USAGE_GENERIC_X,
        &dirPressed,
        pHidJoystick->phidpPreparsedData,
        pbHidPacket,
        cbHidPacket);

	SendDirMessages( HID_USAGE_GENERIC_X, dirPressed, &pHidJoystick->prevDirX);

	status = HidP_GetUsageValue(
        HidP_Input,
        HID_USAGE_PAGE_GENERIC,
        0,
        HID_USAGE_GENERIC_Y,
        &dirPressed,
        pHidJoystick->phidpPreparsedData,
        pbHidPacket,
        cbHidPacket);

	SendDirMessages( HID_USAGE_GENERIC_Y, dirPressed, &pHidJoystick->prevDirY);


    // Save the current usages.
    memcpy(pHidJoystick->puPrevUsages, pHidJoystick->puCurrUsages, cbUsageList);
}