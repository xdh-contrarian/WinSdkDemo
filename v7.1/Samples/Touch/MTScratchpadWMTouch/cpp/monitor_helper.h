#pragma once
#include <Windows.h>  
#include <string>  
using namespace std;

BOOL
XDD_GetActiveAttachedMonitor(
	OUT DISPLAY_DEVICE& ddMonitor                // 输出ddMonitor信息  
);


BOOL
XDD_GetModelDriverFromDeviceID(
	IN  LPCWSTR lpDeviceID,                      // DeviceID  
	OUT wstring& strModel,                       // 输出型号，比如LEN0028  
	OUT wstring& strDriver                       // 输出驱动信息，比如{4d36e96e-e325-11ce-bfc1-08002be10318}\0001  
);

BOOL
XDD_IsCorrectEDID(
	IN  const BYTE* pEDIDBuf,                    // EDID数据缓冲区  
	IN  DWORD dwcbBufSize,                       // 数据字节大小  
	IN  LPCWSTR lpModel                          // 型号  
);


BOOL
XDD_GetDeviceEDID(
	IN  LPCWSTR lpModel,                         // 型号  
	IN  LPCWSTR lpDriver,                        // Driver  
	OUT BYTE* pDataBuf,                          // 输出EDID数据缓冲区  
	IN DWORD dwcbBufSize,                        // 输出缓冲区字节大小，不可小于256  
	OUT DWORD* pdwGetBytes = NULL                // 实际获得字节数  
);

BOOL
XDD_GetActiveMonitorPhysicalSize(
	OUT DWORD& dwWidth,                          // 输出宽度，单位CM  
	OUT DWORD& dwHeight                          // 输出高度，单位CM  
);



class CMonitorHelper
{
public:
	static bool GetMaterMonitorPhysicalSize(DWORD& dwWidth, DWORD& dwHeight);

};