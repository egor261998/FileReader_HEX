#include <windows.h>
#include "locale.h"
#include <string.h>
#include <wchar.h>

#define iEdit 1
#define iEdit2 2
#define iButton 3
#define iButton2 4

// объ€вление переменных

boolean g_bMouseLUp = FALSE;

HWND hMyWnd;
HWND hEdit;
HWND hEdit2;
HWND hButton;
HWND hButton2;
HWND hScroll;

HANDLE hFile=INVALID_HANDLE_VALUE;

DWORD g_nBytesRead = 0;
DWORD g_nBytesToRead = 16;
DWORD g_nSizeFile = { 0 };
DWORD g_nSizeX = 800;
DWORD g_nSizeY = 400;

OVERLAPPED g_ov = { 0 };

WNDPROC wpOrigEditProc;

wchar_t g_szPath[128] = L"c:\\all\\MyWork\\Cryptosoft\\Release\\MyFile.txt";

int g_nDiff = 0;

int g_iPosScrollPercent = 0;
int g_iMinScrollPercent = 0;
int g_iMaxScrollPercent = 100;

int g_iPosScrollVar = 0;
int g_iMinScrollVar = 0;
int g_iMaxScrollVar = 100;

int g_nStringsInText = 23;
const int g_nStepScrollPage = 10;
const int g_nStepScrollLine = 1;
const int g_iSize = 20;

// объ€вление функций
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY EditSubclassProc(HWND, UINT, WPARAM, LPARAM);
ATOM RegMyWindowClass(HINSTANCE, LPCTSTR);

int GetLineFromFile(wchar_t *szBufferOut, DWORD dwByte, boolean *flagEnd)
{
	boolean bResult = FALSE;
	DWORD dwError;
	char szBufferRead[256] = { 0 };
	wchar_t szBuffer[256] = { 0 };
	wchar_t szBufferHEX[256] = { 0 };

	ReadFile(hFile, szBufferRead, dwByte, &g_nBytesRead, &g_ov);
	bResult = GetOverlappedResult(hFile, &g_ov, &g_nBytesRead, TRUE);// проверка ошибок

	if (!bResult || g_nBytesRead == 0)
	{
		switch (dwError = GetLastError())
		{
			case ERROR_HANDLE_EOF:
			{
				return 0;
				(*flagEnd) = TRUE;
				break;
			}
			case ERROR_IO_PENDING:
			{
				return 0;
				break;
			}
			default:
				break;
		}
	}

	MultiByteToWideChar(CP_THREAD_ACP, MB_ERR_INVALID_CHARS, szBufferRead, g_nBytesToRead, szBuffer, g_nBytesToRead);
	if (dwByte==0)return 0;
	int i;
	for (i = 0; i < (int)dwByte; i++)
	{
		if ((szBufferRead[i] & 0xff) <= 0x0f)wsprintf(szBufferHEX, TEXT("%s 0%X"), szBufferHEX, szBufferRead[i] & 0xff);
		else wsprintf(szBufferHEX, TEXT("%s %X"), szBufferHEX, szBufferRead[i] & 0xff);
		if ((szBufferRead[i] & 0xff) <= 0x1F)szBuffer[i] = '.';
	}
	if (i < (int)g_nBytesToRead)
	{
		for (int n = i; n < (int)g_nBytesToRead; n++)  wsprintf(szBufferHEX, TEXT("%s   "), szBufferHEX);
	}

	if (g_ov.Offset < 0xF)wsprintf(szBufferOut, TEXT("0000000%X: %s\t|\t%s\r\n"), g_ov.Offset, szBufferHEX, szBuffer);
	else if (g_ov.Offset < 0xFF)wsprintf(szBufferOut, TEXT("000000%X: %s\t|\t%s\r\n"), g_ov.Offset, szBufferHEX, szBuffer);
	else if (g_ov.Offset < 0xFFF)wsprintf(szBufferOut, TEXT("00000%X: %s\t|\t%s\r\n"), g_ov.Offset, szBufferHEX, szBuffer);
	else if (g_ov.Offset < 0xFFFF)wsprintf(szBufferOut, TEXT("0000%X: %s\t|\t%s\r\n"), g_ov.Offset, szBufferHEX, szBuffer);
	else if (g_ov.Offset < 0xFFFFF)wsprintf(szBufferOut, TEXT("000%X: %s\t|\t%s\r\n"), g_ov.Offset, szBufferHEX, szBuffer);
	else if (g_ov.Offset < 0xFFFFFF)wsprintf(szBufferOut, TEXT("00%X: %s\t|\t%s\r\n"), g_ov.Offset, szBufferHEX, szBuffer);
	else if (g_ov.Offset < 0xFFFFFFF)wsprintf(szBufferOut, TEXT("0%X: %s\t|\t%s\r\n"), g_ov.Offset, szBufferHEX, szBuffer);
	else if (g_ov.Offset < 0xFFFFFFFF)wsprintf(szBufferOut, TEXT("%X: %s\t|\t%s\r\n"), g_ov.Offset, szBufferHEX, szBuffer);
	return 1;
}

void ShowFile(int iPointScroll)
{
	boolean bFlagEnd = FALSE;
	wchar_t szOutBuf2[8192] = { 0 };

	g_ov.Offset = (DWORD)iPointScroll;
	if (g_ov.Offset > g_nSizeFile) return;
	int nStringsInTextRead = 0;
	//SetWindowText(hEdit, NULL);
	while (!bFlagEnd && g_nStringsInText > nStringsInTextRead && !(hFile == INVALID_HANDLE_VALUE))
	{
		DWORD dwByte = 0;
		wchar_t szOutBuf[1024] = { 0 };

		if (g_nSizeFile > (g_nBytesToRead + g_ov.Offset) || g_ov.Offset == 0) dwByte = g_nBytesToRead;
		else dwByte = g_nSizeFile - g_ov.Offset;
		
		if(GetLineFromFile(szOutBuf, dwByte, &bFlagEnd)) swprintf(szOutBuf2, sizeof(szOutBuf2), TEXT("%s %s"), szOutBuf2, szOutBuf);
		g_ov.Offset += dwByte;
		nStringsInTextRead++;
	}
	SetWindowText(hEdit, szOutBuf2);
}

void ChangeSizeWindow()
{
	if ((int)g_nSizeY > 4 * g_iSize)
	{
		g_nSizeY -= 2 * g_iSize;
		if (g_nSizeY % 2 != 0) g_nSizeY--;
		g_nStringsInText = (g_nSizeY - 2) / g_iSize;
		g_iMaxScrollVar = g_nSizeFile / 16 - g_nStringsInText + 1;
		g_iPosScrollPercent = 0;
		g_iPosScrollVar = 0;
		if (g_iMaxScrollVar < 100) g_iMaxScrollPercent = g_iMaxScrollVar;
		else g_iMaxScrollPercent = 100;
		SetScrollPos(hScroll, SB_CTL, g_iPosScrollPercent, TRUE);
		SetScrollRange(hScroll, SB_CTL, g_iMinScrollPercent, g_iMaxScrollPercent, TRUE);

		MoveWindow(hEdit, 0, 0, 900, g_nSizeY, TRUE);
		MoveWindow(hEdit2, 0, g_nSizeY, 400, 20, TRUE);
		MoveWindow(hButton, 400, g_nSizeY, 100, 20, TRUE);
		MoveWindow(hButton2, 520, g_nSizeY, 100, 20, TRUE);
		MoveWindow(hScroll, 900, 0, 16, g_nSizeY, TRUE);
		ShowFile(g_iPosScrollVar);
	}
}

void OpenNewFile()
{
	CloseHandle(hFile);
	hFile = CreateFile(g_szPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, L"‘аил не найден", L"ќшибка", MB_OK);
		SetWindowText(hEdit, L"ќЎ»Ѕ ј 10+12=120");
		return;
	}
	SetWindowText(hEdit2, g_szPath);
	g_nSizeFile = GetFileSize(hFile, NULL);

	RECT screen_rect;
	GetWindowRect(GetDesktopWindow(), &screen_rect); // разрешение экрана
	int ix = screen_rect.right / 2 - 150;
	int iy = screen_rect.bottom / 2 - 75;
	MoveWindow(hMyWnd, ix, iy, 1000, 440, TRUE);

	g_nSizeX = 800;
	g_nSizeY = 400;
	ChangeSizeWindow();
}

void MoveScroll(WPARAM wParam)
{
	boolean bFlag = FALSE;
	int iLastPosScrollVar = g_iPosScrollVar;
	switch (LOWORD(wParam))
	{
		case SB_PAGERIGHT:
		{
			g_iPosScrollVar += g_nStepScrollPage;
			break;
		}
		case SB_LINERIGHT:
		{
			g_iPosScrollVar += g_nStepScrollLine;
			break;
		}
		case SB_PAGELEFT:
		{
			g_iPosScrollVar -= g_nStepScrollPage;
			break;
		}
		case SB_LINELEFT:
		{
			g_iPosScrollVar -= g_nStepScrollLine;
			break;
		}
		case SB_TOP:
		{
			g_iPosScrollVar = g_iPosScrollVar;
			break;
		}
		case SB_BOTTOM:
		{
			g_iPosScrollVar = g_iMaxScrollVar;
			break;
		}
		break;
		case SB_THUMBTRACK:
		{
			g_iPosScrollPercent = HIWORD(wParam);
			bFlag = TRUE;
		}
		break;
		default:
			break;
	}
	if (bFlag)
	{
		g_iPosScrollVar = (int)(((float)g_iPosScrollPercent / (float)g_iMaxScrollPercent) * g_iMaxScrollVar);
		if (g_iPosScrollPercent > g_iMaxScrollPercent)g_iPosScrollPercent = g_iMaxScrollPercent;
		if (g_iPosScrollPercent < g_iMinScrollPercent)g_iPosScrollPercent = g_iMinScrollPercent;
		SetScrollPos(hScroll, SB_CTL, g_iPosScrollPercent, TRUE);
		if (g_iPosScrollVar > g_iMaxScrollVar) g_iPosScrollVar = g_iMaxScrollVar;
		if (g_iPosScrollVar < g_iMinScrollVar) g_iPosScrollVar = g_iMinScrollVar;

		if (g_iPosScrollPercent >= g_iMaxScrollPercent) g_iPosScrollVar = g_iMaxScrollVar;
		if (g_iPosScrollVar <= g_iMinScrollVar) g_iPosScrollVar = g_iMinScrollVar;

		if (iLastPosScrollVar != g_iPosScrollVar)ShowFile(g_iPosScrollVar * 16);
	}
	else
	{
		g_iPosScrollPercent = (int)(((float)g_iPosScrollVar / (float)g_iMaxScrollVar) * g_iMaxScrollPercent);
		if (g_iPosScrollPercent > g_iMaxScrollPercent)g_iPosScrollPercent = g_iMaxScrollPercent;
		if (g_iPosScrollPercent < g_iMinScrollPercent)g_iPosScrollPercent = g_iMinScrollPercent;
		SetScrollPos(hScroll, SB_CTL, g_iPosScrollPercent, TRUE);
		if (g_iPosScrollVar > g_iMaxScrollVar) g_iPosScrollVar = g_iMaxScrollVar;
		if (g_iPosScrollVar < g_iMinScrollVar) g_iPosScrollVar = g_iMinScrollVar;

		if (iLastPosScrollVar != g_iPosScrollVar)ShowFile(g_iPosScrollVar * 16);
	}
}

DWORD WINAPI ThreadFunc()
{
	while (1==1)
	{
		if (g_bMouseLUp)
		{
			int iLastPosScrollVar = g_iPosScrollVar;
			g_iPosScrollVar += g_nDiff;
			g_iPosScrollPercent = (int)(((float)(g_iPosScrollVar) / (float)g_iMaxScrollVar) * g_iMaxScrollPercent);
			if (g_iPosScrollPercent > g_iMaxScrollPercent)g_iPosScrollPercent = g_iMaxScrollPercent;
			if (g_iPosScrollPercent < g_iMinScrollPercent)g_iPosScrollPercent = g_iMinScrollPercent;
			SetScrollPos(hScroll, SB_CTL, g_iPosScrollPercent, TRUE);
			if (g_iPosScrollVar > g_iMaxScrollVar) g_iPosScrollVar = g_iMaxScrollVar;
			if (g_iPosScrollVar < g_iMinScrollVar) g_iPosScrollVar = g_iMinScrollVar;

			if (g_iPosScrollPercent > g_iMaxScrollPercent) g_iPosScrollVar = g_iMaxScrollVar;
			if (g_iPosScrollVar < g_iMinScrollVar) g_iPosScrollVar = g_iMinScrollVar;
			if (iLastPosScrollVar != g_iPosScrollVar)ShowFile(g_iPosScrollVar * 16);
		}
		else return 0;
		Sleep(50);
	}
	
	return 0;
}

////////////////////////////////////////////////////////////////////////// 
// функци€ вхождений программы WinMain
int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE         hPrevInstance,
	LPSTR             lpCmdLine,
	int               nCmdShow)
{
	//setlocale(LC_ALL, "Russian");
	// им€ будущего класса
	LPCTSTR lpzClass = TEXT("My Window Class!");

	// регистраци€ класса
	if (!RegMyWindowClass(hInstance, lpzClass)) return 1;

	// вычисление координат центра экрана
	RECT screen_rect;
	GetWindowRect(GetDesktopWindow(), &screen_rect); // разрешение экрана
	int ix = screen_rect.right / 2 - 150;
	int iy = screen_rect.bottom / 2 - 75;

	// создание диалогового окна
	hMyWnd = CreateWindow(lpzClass, TEXT("HEX TABLE"), WS_OVERLAPPEDWINDOW | WS_VISIBLE, ix, iy, 1000, 440, NULL, NULL, hInstance, NULL);
	
	// если окно не создано, описатель будет равен 0
	if (!hMyWnd) return 2;

	// цикл сообщений приложени€
	MSG msg = { 0 };    // структура сообщени€
	int iGetOk = 0;   // переменна€ состо€ни€

	while ((iGetOk = GetMessage(&msg, NULL, 0, 0)) != 0) // цикл сообщений
	{
		if (iGetOk == -1) return 3;  // если GetMessage вернул ошибку - выход
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;  // возвращаем код завершени€ программы
}

////////////////////////////////////////////////////////////////////////// 
// функци€ регистрации класса окон
ATOM RegMyWindowClass(HINSTANCE hInst, LPCTSTR lpzClassName)
{
	WNDCLASS wcWindowClass = { 0 };
	// адрес ф-ции обработки сообщений
	wcWindowClass.lpfnWndProc = (WNDPROC)WndProc;
	// стиль окна
	wcWindowClass.style = CS_HREDRAW | CS_VREDRAW;
	// дискриптор экземпл€ра приложени€
	wcWindowClass.hInstance = hInst;
	// название класса
	wcWindowClass.lpszClassName = lpzClassName;
	// загрузка курсора
	wcWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	// загрузка цвета окон
	wcWindowClass.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
	return RegisterClass(&wcWindowClass); // регистраци€ класса
}

////////////////////////////////////////////////////////////////////////// 
// функци€ обработки сообщений


LRESULT APIENTRY EditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_MOUSEWHEEL:
		{
			int iLastPosScrollVar = g_iPosScrollVar;
			g_iPosScrollVar -= GET_WHEEL_DELTA_WPARAM(wParam) / 16;
			g_iPosScrollPercent = (int)(((float)(g_iPosScrollVar) / (float)g_iMaxScrollVar) * g_iMaxScrollPercent);
			if (g_iPosScrollPercent > g_iMaxScrollPercent)g_iPosScrollPercent = g_iMaxScrollPercent;
			if (g_iPosScrollPercent < g_iMinScrollPercent)g_iPosScrollPercent = g_iMinScrollPercent;
			SetScrollPos(hScroll, SB_CTL, g_iPosScrollPercent, TRUE);
			if (g_iPosScrollVar > g_iMaxScrollVar) g_iPosScrollVar = g_iMaxScrollVar;
			if (g_iPosScrollVar < g_iMinScrollVar) g_iPosScrollVar = g_iMinScrollVar;

			if (g_iPosScrollPercent >= g_iMaxScrollPercent) g_iPosScrollVar = g_iMaxScrollVar;
			if (g_iPosScrollVar <= g_iMinScrollVar) g_iPosScrollVar = g_iMinScrollVar;
			if (iLastPosScrollVar != g_iPosScrollVar)ShowFile(g_iPosScrollVar * 16);
		}
		break;
		case WM_MOUSEMOVE:
		{
			int iy = HIWORD(lParam);

			if ((int)(g_nSizeY) < iy)
			{
				if (iy > 10000) g_nDiff = -(1+(65536 - iy)/100);
				else g_nDiff = (1 + (iy - g_nSizeY) / 80);				
				SetFocus(hMyWnd);
			}
			else
			{
				g_nDiff = 0;
			}
		}
		break;
		case WM_LBUTTONDOWN:
		{
			g_bMouseLUp = TRUE;
			HANDLE hThread;
			hThread = CreateThread(NULL, 0, &ThreadFunc, NULL, 0, NULL);
		}
		break;
		case WM_LBUTTONUP:
		{
			g_bMouseLUp = FALSE;
		}
		break;	
	}
	return CallWindowProc(wpOrigEditProc, hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(
	HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		break;
		case WM_CREATE:
		{
			hEdit = CreateWindow(L"edit", NULL, WS_BORDER | WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY , 0, 0, g_nSizeX, g_nSizeY, hWnd, iEdit, 0, 0);
			hEdit2 = CreateWindow(L"edit", NULL, WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 800, 20, 200, hWnd, iEdit2, 0, 0);
			hScroll = CreateWindow(L"scrollbar", NULL, WS_CHILD | WS_VISIBLE | SBS_VERT, 800, 0, 16, 400, hWnd, NULL, NULL, NULL);
			hButton = CreateWindow(L"BUTTON", L"ќтркыть фаил", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 800, 200, 20, hWnd, iButton, 0, 0);
			hButton2 = CreateWindow(L"BUTTON", L"ќбзор", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 800, 200, 20, hWnd, iButton2, 0, 0);
			SetWindowText(hEdit2, g_szPath);
			HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH, L"Consolas");
			SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, NULL);
			
			wpOrigEditProc = (WNDPROC)GetWindowLongPtr(hEdit, GWLP_WNDPROC);
			SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LPARAM)EditSubclassProc);
		}
		break;
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case iButton:
				{
					GetWindowText(hEdit2, g_szPath, sizeof(g_szPath));
					OpenNewFile();
				}
				break;
				case iButton2:
				{
					OPENFILENAME ofn;
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = g_szPath;
					ofn.nMaxFile = 128;
					ofn.nMaxCustFilter = 0;
					GetOpenFileName(&ofn);
					OpenNewFile();
				}
				break;
				default:
					break;
			}
		}
		break;
		case WM_SIZE:
		{
			g_nSizeX = (DWORD)LOWORD(lParam);
			g_nSizeY = (DWORD)HIWORD(lParam);
			ChangeSizeWindow();
		}
		break;
		case WM_MOUSEWHEEL:
		{
				g_iPosScrollVar -= GET_WHEEL_DELTA_WPARAM(wParam)/16;
				g_iPosScrollPercent = (int)(((float)(g_iPosScrollVar) / (float)g_iMaxScrollVar) * g_iMaxScrollPercent);
				if (g_iPosScrollPercent > g_iMaxScrollPercent)g_iPosScrollPercent = g_iMaxScrollPercent;
				if (g_iPosScrollPercent < g_iMinScrollPercent)g_iPosScrollPercent = g_iMinScrollPercent;
				SetScrollPos(hScroll, SB_CTL, g_iPosScrollPercent, TRUE);
				if (g_iPosScrollVar > g_iMaxScrollVar) g_iPosScrollVar = g_iMaxScrollVar;
				if (g_iPosScrollVar < g_iMinScrollVar) g_iPosScrollVar = g_iMinScrollVar;

				if (g_iPosScrollPercent >= g_iMaxScrollPercent) g_iPosScrollVar = g_iMaxScrollVar;
				if (g_iPosScrollVar <= g_iMinScrollVar) g_iPosScrollVar = g_iMinScrollVar;
				ShowFile(g_iPosScrollVar * 16);
		}
		break;
		case WM_DESTROY:
		{
			CloseHandle(hFile);
			PostQuitMessage(0);  
		}
		break;
		case WM_VSCROLL:
		{
			MoveScroll(wParam);
		}
		break;
		default:
			// все сообщени€ не обработанные ¬ами обработает сама Windows
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}