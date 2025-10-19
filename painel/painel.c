#include "../arbitro/utils.h"
#include "resource.h"
#include <tchar.h>
#include <windows.h>

LRESULT CALLBACK trataEventos(HWND, UINT, WPARAM, LPARAM);

TCHAR szProgName[] = TEXT("Base");

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR lpCmdLine, int nCmdShow) {
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);
	freopen_s(&fp, "CONIN$", "r", stdin);
	HWND hWnd;
	MSG lpMsg;
	WNDCLASSEX wcApp;

	wcApp.cbSize = sizeof(WNDCLASSEX);
	wcApp.hInstance = hInst;
	wcApp.lpszClassName = szProgName;
	wcApp.lpfnWndProc = trataEventos;
	wcApp.style = CS_HREDRAW | CS_VREDRAW;
	wcApp.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcApp.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcApp.lpszMenuName = (LPCWSTR)IDR_MENU2;
	wcApp.cbClsExtra = 0;
	wcApp.cbWndExtra = sizeof(PANEL_DATA*);
	wcApp.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

	if (!RegisterClassEx(&wcApp))
		return 0;

	hWnd = CreateWindow(
		szProgName,
		TEXT("Jogo - Interface Principal"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		500,
		800,
		HWND_DESKTOP,
		NULL,
		hInst,
		NULL);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg);
		DispatchMessage(&lpMsg);
	}

	return (int)lpMsg.wParam;
}

DWORD WINAPI readShm(LPVOID data) {
	PANEL_DATA* ptd = (PANEL_DATA*)data;
	SHM_PANEL* pShmPanel;
	HANDLE hEvent, hSharedMemory, hMutexShm;

	_tprintf(_T("Entrou na thread\n"));

	hEvent = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_SHM_PAINEL);
	if (hEvent == NULL) {
		ExitThread(1);
	}

	hSharedMemory = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_PANEL_GUI);
	if (hSharedMemory == NULL) {
		ExitThread(1);
	}

	hMutexShm = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MUTEX_SHM_PANEL);
	if (hMutexShm == NULL) {
		ExitThread(1);
	}

	pShmPanel = (SHM_PANEL*)MapViewOfFile(hSharedMemory, FILE_MAP_READ, 0, 0, sizeof(SHM_PANEL));
	if (pShmPanel == NULL) {
		ExitThread(1);
	}
	do {
		WaitForSingleObject(hEvent, INFINITE);
		WaitForSingleObject(hMutexShm, INFINITE);
		WaitForSingleObject(ptd->hMutex, INFINITE);

		_tcscpy_s(ptd->lastWordRight, MSG_TAM, pShmPanel->lastWordRight);
		_tcscpy_s(ptd->availableLetters, MAX_MAXLETTERS, pShmPanel->lettersAvailable);
		ptd->numberActivePlayers = pShmPanel->numActivePlayers;

		for (DWORD i = 0; i < pShmPanel->numActivePlayers; i++) {
			ptd->activePlayers[i] = pShmPanel->activePlayers[i];
		}

		ReleaseMutex(ptd->hMutex);
		ReleaseMutex(hMutexShm);

		InvalidateRect(ptd->hWnd, NULL, TRUE);
	} while (ptd->continuing);

	ExitThread(0);
}

int comparePlayers(PLAYER* a, PLAYER* b) {
	if (a->score > b->score) return -1;
	if (a->score <= b->score) return 1;
	return 0;
}

LRESULT CALLBACK trataDlg(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam) {
	TCHAR str[TAM];
	PANEL_DATA* ptd;
	HANDLE hWnd;

	hWnd = GetParent(hDlg);
	ptd = (PANEL_DATA*)GetWindowLongPtr(hWnd, 0);

	switch (messg) {
	case WM_INITDIALOG:
		WaitForSingleObject(ptd->hMutex, INFINITE);
		_stprintf_s(str, TAM, _T("%d"), ptd->numberTopPlayers); // MUTEX!!!!!!!!!!!!                    INT --->  STR
		ReleaseMutex(ptd->hMutex);
		SetDlgItemText(hDlg, IDC_EDIT1, str);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OK:
			GetDlgItemText(hDlg, IDC_EDIT1, str, TAM);
			WaitForSingleObject(ptd->hMutex, INFINITE);
			_stscanf_s(str, _T("%d"), &ptd->numberTopPlayers); // MUTEX!!!!!!!!!!!! (WAITFOR....)        //STR --->  INT
			ReleaseMutex(ptd->hMutex);
			EndDialog(hDlg, 0);
			InvalidateRect(ptd->hWnd, NULL, TRUE);

			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
		}
		return TRUE;   //  TRATEI DO EVENTO
	}
	return FALSE; // NÃO TRATEI DO EVENTO
}

LRESULT CALLBACK trataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	PANEL_DATA* ptd;
	HDC hdc;
	TCHAR str[TAM];
	PAINTSTRUCT ps;

	switch (messg) {
	case WM_CREATE:
		ptd = (PANEL_DATA*)malloc(sizeof(PANEL_DATA));
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)ptd);
		GetClientRect(hWnd, &ptd->dim);
		ptd->continuing = TRUE;
		ptd->hWnd = hWnd;
		ptd->hMutex = CreateMutex(NULL, FALSE, NULL);
		ptd->hThread = NULL;
		ptd->numberTopPlayers = 10;
		ptd->hInst = ((LPCREATESTRUCT)lParam)->hInstance;

		_tcscpy_s(ptd->lastWordRight, MSG_TAM, _T("NENHUMA"));
		_tcscpy_s(ptd->availableLetters, MAX_MAXLETTERS, _T("NÃO EXISTE"));

		ptd->hThread = CreateThread(NULL, 0, readShm, (LPVOID)ptd, 0, NULL);
		break;

	case WM_COMMAND:
		ptd = (PANEL_DATA*)GetWindowLongPtr(hWnd, 0);
		if (HIWORD(wParam) == 0) { //MENU
			switch (LOWORD(wParam)) {
			case ID_FICHEIRO_DEFINIR:
				DialogBox(ptd->hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, trataDlg);
				break;
			case ID_FICHEIRO_SAIR:
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				break;
			case ID_SOBRE_AUTORES:
				MessageBox(hWnd, _T("Autores: \n - Nuno Tomás Paiva \n - Rui Santos"), _T("INFORMACAO"), MB_OK);
				break;
			}
		}

	case WM_PAINT:
		ptd = (PANEL_DATA*)GetWindowLongPtr(hWnd, 0);
		hdc = BeginPaint(hWnd, &ps);
		GetClientRect(hWnd, &ptd->dim);

		WaitForSingleObject(ptd->hMutex, INFINITE);
		int h1 = (ptd->dim.bottom - ptd->dim.top) * 20 / 100;  
		int h2 = (ptd->dim.bottom - ptd->dim.top) * 20 / 100;  
		int h3 = (ptd->dim.bottom - ptd->dim.top) * 60 / 100; 
		ReleaseMutex(ptd->hMutex);


		HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Arial"));
		HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

		WaitForSingleObject(ptd->hMutex, INFINITE);
		Rectangle(hdc, 0, 0, ptd->dim.right, h1);
		TextOut(hdc, 10, 10, _T("Última Palavra Acertada:"), (int)_tcslen(_T("Última Palavra Acertada:")));
		TextOut(hdc, 250, 10, ptd->lastWordRight, (int)_tcslen(ptd->lastWordRight));

		Rectangle(hdc, 0, h1, ptd->dim.right, h1 + h2);
		TextOut(hdc, 10, h1 + 10, _T("Letras Disponíveis:"), (int)_tcslen(_T("Letras Disponíveis:")));
		SetTextCharacterExtra(hdc, 7);
		TextOut(hdc, 200, h1 + 10, ptd->availableLetters, (int)_tcslen(ptd->availableLetters));
		SetTextCharacterExtra(hdc, 0);

		qsort(ptd->activePlayers, ptd->numberActivePlayers, sizeof(PLAYER), comparePlayers);
		Rectangle(hdc, 0, h1 + h2, ptd->dim.right, h1 + h2 + h3);
		_stprintf_s(str, TAM, _T("Top %d Jogadores:"), ptd->numberTopPlayers);
		TextOut(hdc, 10, h1 + h2 + 10, str, (int)_tcslen(str));

		int lineHeight = 25;
		for (int i = 0; i < (ptd->numberActivePlayers); i++) {
			if (i >= ptd->numberTopPlayers) {
				break;
			}
			_stprintf_s(ptd->top10[i], 256, _T("%d. Jogador %s (%.2f pts)"), i + 1, ptd->activePlayers[i].name, ptd->activePlayers[i].score);
			TextOut(hdc, 30, h1 + h2 + 30 + i * lineHeight, ptd->top10[i], (int)_tcslen(ptd->top10[i]));
		}
		ReleaseMutex(ptd->hMutex);

		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		ptd = (PANEL_DATA*)GetWindowLongPtr(hWnd, 0);
		if (ptd != NULL) {
			CloseHandle(ptd->hMutex);
			CloseHandle(ptd->hThread);
			free(ptd);
		}
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, messg, wParam, lParam);
	}

	return 0;
}
