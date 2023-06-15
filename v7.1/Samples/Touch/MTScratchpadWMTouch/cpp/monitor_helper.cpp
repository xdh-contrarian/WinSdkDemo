#include "monitor_helper.h"

BOOL XDD_GetActiveAttachedMonitor(OUT DISPLAY_DEVICE& ddMonitor)
{
	// ��ʼ���������  
	ZeroMemory(&ddMonitor, sizeof(ddMonitor));

	// ö��Adapter��Monitor�ñ���  
	DWORD dwMonitorIndex = 0;
	DISPLAY_DEVICE ddMonTmp;

	// ö��Adapter  
	DWORD dwAdapterIndex = 0;
	DISPLAY_DEVICE ddAdapter;
	ddAdapter.cb = sizeof(ddAdapter);
	while (::EnumDisplayDevices(0, dwAdapterIndex, &ddAdapter, 0) != FALSE)
	{
		// ö�ٸ�Adapter�µ�Monitor  
		dwMonitorIndex = 0;
		ZeroMemory(&ddMonTmp, sizeof(ddMonTmp));
		ddMonTmp.cb = sizeof(ddMonTmp);
		bool isMasterFlag = ddAdapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP && !(ddAdapter.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) && ddAdapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE;
		while (::EnumDisplayDevices(ddAdapter.DeviceName, dwMonitorIndex, &ddMonTmp, 0) != FALSE)
		{
			// �ж�״̬�Ƿ���ȷ  
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

			// ��һ��Monitor  
			dwMonitorIndex += 1;
			ZeroMemory(&ddMonTmp, sizeof(ddMonTmp));
			ddMonTmp.cb = sizeof(ddMonTmp);
		}

		// ��һ��Adapter  
		dwAdapterIndex += 1;
		ZeroMemory(&ddAdapter, sizeof(ddAdapter));
		ddAdapter.cb = sizeof(ddAdapter);
	}

	// δö�ٵ�����������Monitor  
	return FALSE;
}

BOOL XDD_GetModelDriverFromDeviceID(IN LPCWSTR lpDeviceID, OUT wstring& strModel, OUT wstring& strDriver)
{
	// ��ʼ���������  
	strModel = L"";
	strDriver = L"";

	// ������Ч��  
	if (lpDeviceID == NULL)
	{
		return FALSE;
	}

	// ���ҵ�һ��б�ܺ�Ŀ�ʼλ��  
	LPCWSTR lpBegin = wcschr(lpDeviceID, L'\\');
	if (lpBegin == NULL)
	{
		return FALSE;
	}
	lpBegin += 1;

	// ���ҿ�ʼ��ĵ�һ��б��  
	LPCWSTR lpSlash = wcschr(lpBegin, L'\\');
	if (lpSlash == NULL)
	{
		return FALSE;
	}

	// �õ�Model���Ϊ7���ַ�  
	wchar_t wcModelBuf[8] = { 0 };
	size_t szLen = lpSlash - lpBegin;
	if (szLen >= 8)
	{
		szLen = 7;
	}
	wcsncpy_s(wcModelBuf, lpBegin, szLen);

	// �õ��������  
	strModel = wstring(wcModelBuf);
	strDriver = wstring(lpSlash + 1);

	// �����ɹ�  
	return TRUE;
}

BOOL XDD_IsCorrectEDID(IN const BYTE* pEDIDBuf, IN DWORD dwcbBufSize, IN LPCWSTR lpModel)
{
	// ������Ч��  
	if (pEDIDBuf == NULL || dwcbBufSize < 24 || lpModel == NULL)
	{
		return FALSE;
	}

	// �ж�EDIDͷ  
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

	// �������� 2���ֽ� �ɱ�������дӢ����ĸ  
	// ÿ����ĸ��5λ ��15λ����һλ �ڵ�һ����ĸ�������λ�� 0�� ��ĸ A���� Z����Ӧ�Ĵ���Ϊ00001��11010  
	// ���� MAG��������ĸ M����Ϊ01101 A����Ϊ00001 G����Ϊ00111 ��M����ǰ��0Ϊ001101   
	// �����������е�2�ֽ� 001101 00001 00111 ת��Ϊʮ����������Ϊ34 27  
	DWORD dwPos = 8;
	wchar_t wcModelBuf[9] = { 0 };
	char byte1 = pEDIDBuf[dwPos];
	char byte2 = pEDIDBuf[dwPos + 1];
	wcModelBuf[0] = ((byte1 & 0x7C) >> 2) + 64;
	wcModelBuf[1] = ((byte1 & 3) << 3) + ((byte2 & 0xE0) >> 5) + 64;
	wcModelBuf[2] = (byte2 & 0x1F) + 64;
	swprintf_s(wcModelBuf + 3, sizeof(wcModelBuf) / sizeof(wchar_t) - 3, L"%X%X%X%X", (pEDIDBuf[dwPos + 3] & 0xf0) >> 4, pEDIDBuf[dwPos + 3] & 0xf, (pEDIDBuf[dwPos + 2] & 0xf0) >> 4, pEDIDBuf[dwPos + 2] & 0x0f);

	// �Ƚ�MODEL�Ƿ�ƥ��  
	return (_wcsicmp(wcModelBuf, lpModel) == 0) ? TRUE : FALSE;
}

BOOL XDD_GetDeviceEDID(IN LPCWSTR lpModel, IN LPCWSTR lpDriver, OUT BYTE* pDataBuf, IN DWORD dwcbBufSize, OUT DWORD* pdwGetBytes)
{
	// ��ʼ���������  
	if (pdwGetBytes != NULL)
	{
		*pdwGetBytes = 0;
	}

	// ������Ч��  
	if (lpModel == NULL
		|| lpDriver == NULL
		|| pDataBuf == NULL
		|| dwcbBufSize == 0
		)
	{
		return FALSE;
	}

	// ���豸ע����Ӽ�  
	wchar_t wcSubKey[MAX_PATH] = L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\";
	wcscat_s(wcSubKey, lpModel);
	HKEY hSubKey;
	if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, wcSubKey, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	// ���EDID����  
	BOOL bGetEDIDSuccess = FALSE;
	BYTE EDIDBuf[256] = { 0 };
	DWORD dwEDIDSize = sizeof(EDIDBuf);

	// ö�ٸ��Ӽ��µļ�  
	DWORD dwIndex = 0;
	DWORD dwSubKeyLen = sizeof(wcSubKey) / sizeof(wchar_t);
	FILETIME ft;
	while (bGetEDIDSuccess == FALSE
		&& ::RegEnumKeyEx(hSubKey, dwIndex, wcSubKey, &dwSubKeyLen, NULL, NULL, NULL, &ft) == ERROR_SUCCESS
		)
	{
		// ��ö�ٵ��ļ�  
		HKEY hEnumKey;
		if (::RegOpenKeyEx(hSubKey, wcSubKey, 0, KEY_READ, &hEnumKey) == ERROR_SUCCESS)
		{
			// �򿪵ļ��²�ѯDriver����ֵ  
			dwSubKeyLen = sizeof(wcSubKey) / sizeof(wchar_t);
			if (::RegQueryValueEx(hEnumKey, L"Driver", NULL, NULL, (LPBYTE)&wcSubKey, &dwSubKeyLen) == ERROR_SUCCESS
				&& _wcsicmp(wcSubKey, lpDriver) == 0 // Driverƥ��  
				)
			{
				// �򿪼�Device Parameters  
				HKEY hDevParaKey;
				if (::RegOpenKeyEx(hEnumKey, L"Device Parameters", 0, KEY_READ, &hDevParaKey) == ERROR_SUCCESS)
				{
					// ��ȡEDID  
					memset(EDIDBuf, 0, sizeof(EDIDBuf));
					dwEDIDSize = sizeof(EDIDBuf);
					if (::RegQueryValueEx(hDevParaKey, L"EDID", NULL, NULL, (LPBYTE)&EDIDBuf, &dwEDIDSize) == ERROR_SUCCESS
						&& XDD_IsCorrectEDID(EDIDBuf, dwEDIDSize, lpModel) == TRUE // ��ȷ��EDID����  
						)
					{
						// �õ��������  
						DWORD dwRealGetBytes = min(dwEDIDSize, dwcbBufSize);
						if (pdwGetBytes != NULL)
						{
							*pdwGetBytes = dwRealGetBytes;
						}
						memcpy(pDataBuf, EDIDBuf, dwRealGetBytes);

						// �ɹ���ȡEDID����  
						bGetEDIDSuccess = TRUE;
					}

					// �رռ�Device Parameters  
					::RegCloseKey(hDevParaKey);
				}
			}

			// �ر�ö�ٵ��ļ�  
			::RegCloseKey(hEnumKey);
		}

		// ��һ���Ӽ�  
		dwIndex += 1;
	}

	// �ر��豸ע����Ӽ�  
	::RegCloseKey(hSubKey);

	// ���ػ�ȡEDID���ݽ��  
	return bGetEDIDSuccess;
}

BOOL XDD_GetActiveMonitorPhysicalSize(OUT DWORD& dwWidth, OUT DWORD& dwHeight)
{
	// ��ʼ���������  
	dwWidth = 0;
	dwHeight = 0;

	// ȡ�õ�ǰMonitor��DISPLAY_DEVICE����  
	DISPLAY_DEVICE ddMonitor;
	if (XDD_GetActiveAttachedMonitor(ddMonitor) == FALSE)
	{
		return FALSE;
	}

	// ����DeviceID�õ�Model��Driver  
	wstring strModel = L"";
	wstring strDriver = L"";
	if (XDD_GetModelDriverFromDeviceID(ddMonitor.DeviceID, strModel, strDriver) == FALSE)
	{
		return FALSE;
	}

	// ȡ���豸EDID����  
	BYTE EDIDBuf[256] = { 0 };
	DWORD dwRealGetBytes = 0;
	if (XDD_GetDeviceEDID(strModel.c_str(), strDriver.c_str(), EDIDBuf, sizeof(EDIDBuf), &dwRealGetBytes) == FALSE
		|| dwRealGetBytes < 23
		)
	{
		return FALSE;
	}

	// EDID�ṹ�е�22��23���ֽ�Ϊ��Ⱥ͸߶�  
	dwWidth = EDIDBuf[21];
	dwHeight = EDIDBuf[22];

	// �ɹ���ȡ��ʾ������ߴ�  
	return TRUE;
}

bool CMonitorHelper::GetMaterMonitorPhysicalSize(DWORD& dwWidth, DWORD& dwHeight)
{
	return XDD_GetActiveMonitorPhysicalSize(dwWidth, dwHeight);
}
