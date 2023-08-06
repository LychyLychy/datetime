#include <Windows.h>
#include <CommCtrl.h>
#include <tchar.h>
#include <windowsx.h>
#include "resource.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

enum dateMode_e
{
	MODIFIED,
	CREATED,
	ACCESSED,
	LAST_DATEMODE
};

struct dateMode_t
{
	LPCTSTR string;
	FILETIME time;
	BOOL enabled;
};

struct dateMode_t dateModes[LAST_DATEMODE] = 
{
	{TEXT("Modified")  },
	{TEXT("Created")   },
	{TEXT("Accessed")  },
};

SYSTEMTIME fileSystemTime;

void CheckButtons(HWND hWnd);
void ApplyDates(HWND hWnd);
void ResetFileTime(HWND hWnd);
void CheckErrors(HWND hWnd);
LRESULT CALLBACK DateTimeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LPTSTR filePath;
DWORD fileAtrributes;

int _tWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
	filePath = lpCmdLine;
	if (filePath[0] == TEXT('\"'))
	{
		filePath = _tcsinc(filePath);
	}

	TCHAR* lastQuote =  _tcsrchr(filePath, TEXT('\"'));
	if (lastQuote)
	{
		lastQuote[0] = TEXT('\0');
	}

	DWORD attribs = GetFileAttributes(filePath);
	if (attribs == INVALID_FILE_ATTRIBUTES)
	{
		CheckErrors(NULL);
		PostQuitMessage(0);
	}
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DateTimeProc);
	return 0;
}

LRESULT CALLBACK DateTimeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &monitorInfo);
		LONG middleX = (monitorInfo.rcWork.left + monitorInfo.rcWork.right) / 2;
		LONG middleY = (monitorInfo.rcWork.top + monitorInfo.rcWork.bottom) / 2;

		RECT windowRect;
		GetWindowRect(hWnd, &windowRect);
		LONG middleWndX = (windowRect.left + windowRect.right) / 2;
		LONG middleWndY = (windowRect.top + windowRect.bottom) / 2;

		middleX -= middleWndX;
		middleY -= middleWndY;
		SetWindowPos(hWnd, 0, middleX, middleY, 0, 0, SWP_NOSIZE);

		HWND listBox = GetDlgItem(hWnd, IDC_DATEMODES);

		for (int i = 0; i < sizeof(dateModes) / sizeof(struct dateMode_t); i++)
		{
			SendMessage(listBox, LB_ADDSTRING, 0, (LPARAM)dateModes[i].string);
		}
		SendMessage(listBox, LB_SELITEMRANGE, TRUE, 0);
		dateModes[MODIFIED].enabled = TRUE;

		ResetFileTime(hWnd);

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		int wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDC_DATEMODES:
			if (wmEvent == LBN_SELCHANGE)
			{
				CheckButtons(hWnd);
			}
			break;
		case IDAPPLY:
			ApplyDates(hWnd);
			break;
		case IDOK:
			ApplyDates(hWnd);
		case IDCANCEL:
			PostQuitMessage(0);
			break;
		case IDC_RESET:
			ResetFileTime(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

void ApplyDates(HWND hWnd)
{
	HWND dateHwnd = GetDlgItem(hWnd, IDC_DATE);
	HWND timeHwnd = GetDlgItem(hWnd, IDC_TIME);

	SYSTEMTIME dateAndTime;
	SYSTEMTIME time;
	DateTime_GetSystemtime(dateHwnd, &dateAndTime);
	DateTime_GetSystemtime(timeHwnd, &time);
	dateAndTime.wHour = time.wHour;
	dateAndTime.wMinute = time.wMinute;
	dateAndTime.wSecond = time.wSecond;
	dateAndTime.wMilliseconds = time.wMilliseconds;
	fileSystemTime = dateAndTime;

	FILETIME fileTime;
	SystemTimeToFileTime(&dateAndTime, &fileTime);


	FILETIME* filetimes[LAST_DATEMODE];
	for (int i = 0; i < LAST_DATEMODE; i++)
	{
		filetimes[i] = dateModes[i].enabled ? &fileTime : NULL;
	}
	HANDLE hFile = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	CheckErrors(hWnd);
	SetFileTime(hFile, filetimes[CREATED], filetimes[ACCESSED], filetimes[MODIFIED]);
	CheckErrors(hWnd);
	CloseHandle(hFile);
}

void CheckButtons(HWND hWnd)
{
	HWND listBox = GetDlgItem(hWnd, IDC_DATEMODES);
	LRESULT count = SendMessage(listBox, LB_GETSELCOUNT, 0, 0);

	HWND okButton = GetDlgItem(hWnd, IDOK);
	HWND applyButton = GetDlgItem(hWnd, IDAPPLY);
	HWND resetButton = GetDlgItem(hWnd, IDC_RESET);
	if (count < 1)
	{
		Button_Enable(okButton, FALSE);
		Button_Enable(applyButton, FALSE);
		Button_Enable(resetButton, FALSE);
	}
	else if (count == 1)
	{
		Button_Enable(okButton, TRUE);
		Button_Enable(applyButton, TRUE);
		Button_Enable(resetButton, TRUE);
	}
	else // count > 1
	{
		Button_Enable(okButton, TRUE);
		Button_Enable(applyButton, TRUE);
		Button_Enable(resetButton, FALSE);
	}

	for (int i = 0; i < LAST_DATEMODE; i++)
	{
		LRESULT enabled = SendMessage(listBox, LB_GETSEL, i, 0);
		dateModes[i].enabled = !!enabled;
	}
}

void ResetFileTime(HWND hWnd)
{
	HANDLE file = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	CheckErrors(hWnd);
	FILETIME theTimes[LAST_DATEMODE];
	GetFileTime(file, &theTimes[CREATED], &theTimes[ACCESSED], &theTimes[MODIFIED]);
	CheckErrors(hWnd);
	CloseHandle(file);
	for (int i = 0; i < LAST_DATEMODE; i++)
	{
		if (dateModes[i].enabled)
		{
			FileTimeToSystemTime(&theTimes[i], &fileSystemTime);
			HWND dateBox = GetDlgItem(hWnd, IDC_DATE);
			HWND timeBox = GetDlgItem(hWnd, IDC_TIME);
			DateTime_SetSystemtime(dateBox, GDT_VALID, &fileSystemTime);
			DateTime_SetSystemtime(timeBox, GDT_VALID, &fileSystemTime);
		}
	}

}

void CheckErrors(HWND hWnd)
{
	DWORD result = GetLastError();
	if (result != 0)
	{
		LPTSTR buffer = NULL;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER , 0, result, LANG_USER_DEFAULT, (LPTSTR)&buffer, 0, NULL);
		MessageBox(hWnd, buffer, TEXT("Error!"), MB_ICONERROR | MB_OK);
	}
}