#pragma once
#include <Windows.h>  
#include <string>  
using namespace std;

BOOL
XDD_GetActiveAttachedMonitor(
	OUT DISPLAY_DEVICE& ddMonitor                // ���ddMonitor��Ϣ  
);


BOOL
XDD_GetModelDriverFromDeviceID(
	IN  LPCWSTR lpDeviceID,                      // DeviceID  
	OUT wstring& strModel,                       // ����ͺţ�����LEN0028  
	OUT wstring& strDriver                       // ���������Ϣ������{4d36e96e-e325-11ce-bfc1-08002be10318}\0001  
);

BOOL
XDD_IsCorrectEDID(
	IN  const BYTE* pEDIDBuf,                    // EDID���ݻ�����  
	IN  DWORD dwcbBufSize,                       // �����ֽڴ�С  
	IN  LPCWSTR lpModel                          // �ͺ�  
);


BOOL
XDD_GetDeviceEDID(
	IN  LPCWSTR lpModel,                         // �ͺ�  
	IN  LPCWSTR lpDriver,                        // Driver  
	OUT BYTE* pDataBuf,                          // ���EDID���ݻ�����  
	IN DWORD dwcbBufSize,                        // ����������ֽڴ�С������С��256  
	OUT DWORD* pdwGetBytes = NULL                // ʵ�ʻ���ֽ���  
);

BOOL
XDD_GetActiveMonitorPhysicalSize(
	OUT DWORD& dwWidth,                          // �����ȣ���λCM  
	OUT DWORD& dwHeight                          // ����߶ȣ���λCM  
);



class CMonitorHelper
{
public:
	static bool GetMaterMonitorPhysicalSize(DWORD& dwWidth, DWORD& dwHeight);

};