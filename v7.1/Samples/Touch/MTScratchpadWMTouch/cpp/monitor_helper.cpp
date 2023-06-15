#include "monitor_helper.h"

BOOL XDD_GetActiveAttachedMonitor(OUT DISPLAY_DEVICE& ddMonitor)
{
	// 初始化输出参数  
	ZeroMemory(&ddMonitor, sizeof(ddMonitor));

	// 枚举Adapter下Monitor用变量  
	DWORD dwMonitorIndex = 0;
	DISPLAY_DEVICE ddMonTmp;

	// 枚举Adapter  
	DWORD dwAdapterIndex = 0;
	DISPLAY_DEVICE ddAdapter;
	ddAdapter.cb = sizeof(ddAdapter);
	while (::EnumDisplayDevices(0, dwAdapterIndex, &ddAdapter, 0) != FALSE)
	{
		// 枚举该Adapter下的Monitor  
		dwMonitorIndex = 0;
		ZeroMemory(&ddMonTmp, sizeof(ddMonTmp));
		ddMonTmp.cb = sizeof(ddMonTmp);
		bool isMasterFlag = ddAdapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP && !(ddAdapter.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) && ddAdapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE;
		while (::EnumDisplayDevices(ddAdapter.DeviceName, dwMonitorIndex, &ddMonTmp, 0) != FALSE)
		{
			// 判断状态是否正确  
			//if ((ddMonTmp.StateFlags & DISPLAY_DEVICE_ACTIVE) == DISPLAY_DEVICE_ACTIVE
			//	&& (ddMonTmp.StateFlags & DISPLAY_DEVICE_ATTACHED) == DISPLAY_DEVICE_ATTACHED
			//	)
			//{
			//	ddMonitor = ddMonTmp;
			//	return TRUE;
			//}
			if (isMasterFlag)
			{
				ddMonitor = ddMonTmp;
				return TRUE;
			}

			// 下一个Monitor  
			dwMonitorIndex += 1;
			ZeroMemory(&ddMonTmp, sizeof(ddMonTmp));
			ddMonTmp.cb = sizeof(ddMonTmp);
		}

		// 下一个Adapter  
		dwAdapterIndex += 1;
		ZeroMemory(&ddAdapter, sizeof(ddAdapter));
		ddAdapter.cb = sizeof(ddAdapter);
	}

	// 未枚举到满足条件的Monitor  
	return FALSE;
}

BOOL XDD_GetModelDriverFromDeviceID(IN LPCWSTR lpDeviceID, OUT wstring& strModel, OUT wstring& strDriver)
{
	// 初始化输出参数  
	strModel = L"";
	strDriver = L"";

	// 参数有效性  
	if (lpDeviceID == NULL)
	{
		return FALSE;
	}

	// 查找第一个斜杠后的开始位置  
	LPCWSTR lpBegin = wcschr(lpDeviceID, L'\\');
	if (lpBegin == NULL)
	{
		return FALSE;
	}
	lpBegin += 1;

	// 查找开始后的第一个斜杠  
	LPCWSTR lpSlash = wcschr(lpBegin, L'\\');
	if (lpSlash == NULL)
	{
		return FALSE;
	}

	// 得到Model，最长为7个字符  
	wchar_t wcModelBuf[8] = { 0 };
	size_t szLen = lpSlash - lpBegin;
	if (szLen >= 8)
	{
		szLen = 7;
	}
	wcsncpy_s(wcModelBuf, lpBegin, szLen);

	// 得到输出参数  
	strModel = wstring(wcModelBuf);
	strDriver = wstring(lpSlash + 1);

	// 解析成功  
	return TRUE;
}

BOOL XDD_IsCorrectEDID(IN const BYTE* pEDIDBuf, IN DWORD dwcbBufSize, IN LPCWSTR lpModel)
{
	// 参数有效性  
	if (pEDIDBuf == NULL || dwcbBufSize < 24 || lpModel == NULL)
	{
		return FALSE;
	}

	// 判断EDID头  
	if (pEDIDBuf[0] != 0x00
		|| pEDIDBuf[1] != 0xFF
		|| pEDIDBuf[2] != 0xFF
		|| pEDIDBuf[3] != 0xFF
		|| pEDIDBuf[4] != 0xFF
		|| pEDIDBuf[5] != 0xFF
		|| pEDIDBuf[6] != 0xFF
		|| pEDIDBuf[7] != 0x00
		)
	{
		return FALSE;
	}

	// 厂商名称 2个字节 可表三个大写英文字母  
	// 每个字母有5位 共15位不足一位 在第一个字母代码最高位补 0” 字母 A”至 Z”对应的代码为00001至11010  
	// 例如 MAG”三个字母 M代码为01101 A代码为00001 G代码为00111 在M代码前补0为001101   
	// 自左向右排列得2字节 001101 00001 00111 转化为十六进制数即为34 27  
	DWORD dwPos = 8;
	wchar_t wcModelBuf[9] = { 0 };
	char byte1 = pEDIDBuf[dwPos];
	char byte2 = pEDIDBuf[dwPos + 1];
	wcModelBuf[0] = ((byte1 & 0x7C) >> 2) + 64;
	wcModelBuf[1] = ((byte1 & 3) << 3) + ((byte2 & 0xE0) >> 5) + 64;
	wcModelBuf[2] = (byte2 & 0x1F) + 64;
	swprintf_s(wcModelBuf + 3, sizeof(wcModelBuf) / sizeof(wchar_t) - 3, L"%X%X%X%X", (pEDIDBuf[dwPos + 3] & 0xf0) >> 4, pEDIDBuf[dwPos + 3] & 0xf, (pEDIDBuf[dwPos + 2] & 0xf0) >> 4, pEDIDBuf[dwPos + 2] & 0x0f);

	// 比较MODEL是否匹配  
	return (_wcsicmp(wcModelBuf, lpModel) == 0) ? TRUE : FALSE;
}

BOOL XDD_GetDeviceEDID(IN LPCWSTR lpModel, IN LPCWSTR lpDriver, OUT BYTE* pDataBuf, IN DWORD dwcbBufSize, OUT DWORD* pdwGetBytes)
{
	// 初始化输出参数  
	if (pdwGetBytes != NULL)
	{
		*pdwGetBytes = 0;
	}

	// 参数有效性  
	if (lpModel == NULL
		|| lpDriver == NULL
		|| pDataBuf == NULL
		|| dwcbBufSize == 0
		)
	{
		return FALSE;
	}

	// 打开设备注册表子键  
	wchar_t wcSubKey[MAX_PATH] = L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\";
	wcscat_s(wcSubKey, lpModel);
	HKEY hSubKey;
	if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, wcSubKey, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	// 存放EDID数据  
	BOOL bGetEDIDSuccess = FALSE;
	BYTE EDIDBuf[256] = { 0 };
	DWORD dwEDIDSize = sizeof(EDIDBuf);

	// 枚举该子键下的键  
	DWORD dwIndex = 0;
	DWORD dwSubKeyLen = sizeof(wcSubKey) / sizeof(wchar_t);
	FILETIME ft;
	while (bGetEDIDSuccess == FALSE
		&& ::RegEnumKeyEx(hSubKey, dwIndex, wcSubKey, &dwSubKeyLen, NULL, NULL, NULL, &ft) == ERROR_SUCCESS
		)
	{
		// 打开枚举到的键  
		HKEY hEnumKey;
		if (::RegOpenKeyEx(hSubKey, wcSubKey, 0, KEY_READ, &hEnumKey) == ERROR_SUCCESS)
		{
			// 打开的键下查询Driver键的值  
			dwSubKeyLen = sizeof(wcSubKey) / sizeof(wchar_t);
			if (::RegQueryValueEx(hEnumKey, L"Driver", NULL, NULL, (LPBYTE)&wcSubKey, &dwSubKeyLen) == ERROR_SUCCESS
				&& _wcsicmp(wcSubKey, lpDriver) == 0 // Driver匹配  
				)
			{
				// 打开键Device Parameters  
				HKEY hDevParaKey;
				if (::RegOpenKeyEx(hEnumKey, L"Device Parameters", 0, KEY_READ, &hDevParaKey) == ERROR_SUCCESS)
				{
					// 读取EDID  
					memset(EDIDBuf, 0, sizeof(EDIDBuf));
					dwEDIDSize = sizeof(EDIDBuf);
					if (::RegQueryValueEx(hDevParaKey, L"EDID", NULL, NULL, (LPBYTE)&EDIDBuf, &dwEDIDSize) == ERROR_SUCCESS
						&& XDD_IsCorrectEDID(EDIDBuf, dwEDIDSize, lpModel) == TRUE // 正确的EDID数据  
						)
					{
						// 得到输出参数  
						DWORD dwRealGetBytes = min(dwEDIDSize, dwcbBufSize);
						if (pdwGetBytes != NULL)
						{
							*pdwGetBytes = dwRealGetBytes;
						}
						memcpy(pDataBuf, EDIDBuf, dwRealGetBytes);

						// 成功获取EDID数据  
						bGetEDIDSuccess = TRUE;
					}

					// 关闭键Device Parameters  
					::RegCloseKey(hDevParaKey);
				}
			}

			// 关闭枚举到的键  
			::RegCloseKey(hEnumKey);
		}

		// 下一个子键  
		dwIndex += 1;
	}

	// 关闭设备注册表子键  
	::RegCloseKey(hSubKey);

	// 返回获取EDID数据结果  
	return bGetEDIDSuccess;
}

BOOL XDD_GetActiveMonitorPhysicalSize(OUT DWORD& dwWidth, OUT DWORD& dwHeight)
{
	// 初始化输出参数  
	dwWidth = 0;
	dwHeight = 0;

	// 取得当前Monitor的DISPLAY_DEVICE数据  
	DISPLAY_DEVICE ddMonitor;
	if (XDD_GetActiveAttachedMonitor(ddMonitor) == FALSE)
	{
		return FALSE;
	}

	// 解析DeviceID得到Model和Driver  
	wstring strModel = L"";
	wstring strDriver = L"";
	if (XDD_GetModelDriverFromDeviceID(ddMonitor.DeviceID, strModel, strDriver) == FALSE)
	{
		return FALSE;
	}

	// 取得设备EDID数据  
	BYTE EDIDBuf[256] = { 0 };
	DWORD dwRealGetBytes = 0;
	if (XDD_GetDeviceEDID(strModel.c_str(), strDriver.c_str(), EDIDBuf, sizeof(EDIDBuf), &dwRealGetBytes) == FALSE
		|| dwRealGetBytes < 23
		)
	{
		return FALSE;
	}

	// EDID结构中第22和23个字节为宽度和高度  
	dwWidth = EDIDBuf[21];
	dwHeight = EDIDBuf[22];

	// 成功获取显示器物理尺寸  
	return TRUE;
}

bool CMonitorHelper::GetMaterMonitorPhysicalSize(DWORD& dwWidth, DWORD& dwHeight)
{
	return XDD_GetActiveMonitorPhysicalSize(dwWidth, dwHeight);
}
