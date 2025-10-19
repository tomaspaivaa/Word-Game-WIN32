#include "../arbitro/utils.h"
#include "../funcoes/funcoes.h"

COMMAND_TYPE getCommandType(TCHAR* str) {
	if (_tcscmp(str, _T(":pont")) == 0) return PONT;
	if (_tcscmp(str, _T(":jogs")) == 0) return JOGS;
	if (_tcscmp(str, _T(":sair")) == 0) return EXITGAME;
	return INVALID;
}

int _tmain(int argc, LPTSTR argv[]) {
	HANDLE hPipe, hThreadLePipe, hThreadShowLetters, hConsoleIn, hEventExit;
	DWORD numBytes, result;
	TCHAR buf[256];
	BYTE b[256];
	MESSAGE_PLAYER messagePlayer;
	BOOL registerSuccess;
	THREAD_READ_PIPE threadReadPipe;
	THREAD_SHOW_LETTERS threadShowLetters;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	if (argc != 2) {
		_tprintf_s(_T("[JOGADOR] ERRO exemplo de formatação: 'jogoui <nomeJogador>'. \n"));
		return -1;
	}
	_tprintf_s(_T("[JOGADOR] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), PIPE_NAME);
	if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(_T("[JOGADOR] ERRO a ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_NAME);
		exit(-1);
	}
	_tprintf_s(_T("[JOGADOR] Ligação ao pipe do arbitro... (CreateFile)\n"));
	hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		_tprintf_s(_T("[JOGADOR] ERRO a ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME);
		exit(-1);
	}

	DWORD modo = PIPE_READMODE_MESSAGE;
	SetNamedPipeHandleState(hPipe, &modo, NULL, NULL);

	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);

	messagePlayer.commandType = REGISTER;
	_tcscpy_s(messagePlayer.message, PLAYER_NAME_TAM, argv[1]);

	ZeroMemory(&ov, sizeof(OVERLAPPED));
	ov.hEvent = hEv;
	result = WriteFile(hPipe, &messagePlayer, sizeof(messagePlayer), &numBytes, &ov);
	if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
		_tprintf_s(_T("[JOGADOR] ERRO! %d (%d bytes)... (WriteFile)\n"), result, numBytes);
		exit(-1);
	}
	if (GetLastError() == ERROR_IO_PENDING) {
		WaitForSingleObject(hEv, INFINITE);
		GetOverlappedResult(hPipe, &ov, &numBytes, FALSE);
	}

	_tprintf_s(_T("[JOGADOR] Enviada tentativa de registo com o nome '%s'.\n"), messagePlayer.message);

	ZeroMemory(&ov, sizeof(OVERLAPPED));
	ov.hEvent = hEv;
	result = ReadFile(hPipe, &registerSuccess, sizeof(registerSuccess), &numBytes, &ov);
	if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
		_tprintf_s(_T("[JOGADOR] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (ReadFile)\n"), messagePlayer.message, result, numBytes);
		exit(-1);;
	}
	if (GetLastError() == ERROR_IO_PENDING) {
		WaitForSingleObject(hEv, INFINITE);
		GetOverlappedResult(hPipe, &ov, &numBytes, FALSE);
	}
	if (!registerSuccess) {
		_tprintf_s(_T("[JOGADOR] O username '%s' já está em uso. Tenta com outro!\n"), messagePlayer.message);
		exit(-1);
	}
	_tprintf_s(_T("[JOGADOR] Registo efetuado com sucesso.\n"));
	hEventExit = CreateEvent(NULL, FALSE, FALSE, NULL);

	threadReadPipe.hPipe = hPipe;
	threadReadPipe.hMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
	threadReadPipe.hEventExit = hEventExit;
	hThreadLePipe = CreateThread(NULL, 0, readPipe, (LPVOID)&threadReadPipe, 0, NULL);

	threadShowLetters.hEventExit = hEventExit;
	hThreadShowLetters = CreateThread(NULL, 0, showLetters, (LPVOID)&threadShowLetters, 0, NULL);

	hConsoleIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	HANDLE evToWait[2] = { threadReadPipe.hEventExit, hEv };
	do {
		ZeroMemory(&ov, sizeof(OVERLAPPED));
		ov.hEvent = hEv;
		result = ReadFile(hConsoleIn, &b, sizeof(b), &numBytes, &ov);
		if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
			_tprintf_s(_T("[JOGADOR] Erro na thread main: %d (%d bytes)... (ReadFile)\n"), result, numBytes);
			break;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			result = WaitForMultipleObjects(2, evToWait, FALSE, INFINITE);
			if (result == WAIT_OBJECT_0) {
				break;
			}
			else if (result == WAIT_OBJECT_0 + 1) {
				GetOverlappedResult(hConsoleIn, &ov, &numBytes, FALSE);
			}
		}
		b[numBytes - 2] = ('\0');
		MultiByteToWideChar(CP_UTF8, 0, b, numBytes + 2, buf, 256);
		if (buf[0] == ':') {
			messagePlayer.commandType = getCommandType(buf);
			if (messagePlayer.commandType == INVALID) {
				_tprintf_s(_T("[JOGADOR] Comando ':' inválido, usa um dos seguintes:\n\t - :jogs \n\t - :pont \n\t - :sair\n"));
				continue;
			}
		}
		else {
			messagePlayer.commandType = WORDGUESS;
		}
		_tcscpy_s(messagePlayer.message, MSG_TAM, buf);

		ZeroMemory(&ov, sizeof(OVERLAPPED));
		ov.hEvent = hEv;
		result = WriteFile(hPipe, &messagePlayer, sizeof(messagePlayer), &numBytes, &ov);
		if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
			_tprintf_s(_T("[JOGADOR] ERRO a escrever mensagem %d (%d bytes)... (WriteFile)\n"), result, numBytes);
			continue;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEv, INFINITE);
			GetOverlappedResult(hPipe, &ov, &numBytes, FALSE);
		}
		_tprintf_s(_T("[JOGADOR] Enviei a seguinte palavra: '%s' (%d bytes) (WriteFile)\n"), buf, numBytes);
	} while (1);

	WaitForSingleObject(hThreadLePipe, INFINITE);
	WaitForSingleObject(hThreadShowLetters, INFINITE);
	CloseHandle(hThreadLePipe);
	CloseHandle(hThreadShowLetters);
	CloseHandle(hEv);
	CloseHandle(hPipe);
	_tprintf_s(_T("[JOGADOR] A Thread main a desligar-se.\n"));
	ExitThread(0);
}
