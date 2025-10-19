#include "../arbitro/utils.h"
#include "../funcoes/funcoes.h"

TCHAR* getRandomWordFromDictionary() {
	static BOOL initialized = FALSE;
	if (!initialized) {
		srand((unsigned int)time(NULL));
		initialized = TRUE;
	}

	int randomIndex = rand() % DICT_SIZE;

	return dictionary[randomIndex];
}

int _tmain(int argc, LPTSTR argv[]) {
	HANDLE hPipe, hThreadLePipe, hThreadShowLetters, hConsoleIn, hEventExit;
	DWORD numBytes, result, reactionTime;
	TCHAR buf[256];
	TCHAR* randomWord;
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

	if (argc != 3) {
		_tprintf_s(_T("[BOT] ERRO exemplo de formatação: 'bot <nomeBOT> <tempoReacao>'. \n"));
		return -1;
	}
	reactionTime = _ttoi(argv[2]) * 1000;
	if (reactionTime < 5000 || reactionTime > 30000) {
		_tprintf(_T("[BOT] Tempo de reação deve estar entre 5 e 30 segundos\n"));
		exit(-1);
	}
	_tprintf_s(_T("[BOT] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), PIPE_NAME);
	if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(_T("[BOT] ERRO a ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_NAME);
		exit(-1);
	}
	_tprintf_s(_T("[BOT] Ligação ao pipe do arbitro... (CreateFile)\n"));
	hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		_tprintf_s(_T("[BOT] ERRO a ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME);
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
		_tprintf_s(_T("[BOT] ERRO! %d (%d bytes)... (WriteFile)\n"), result, numBytes);
		exit(-1);
	}
	if (GetLastError() == ERROR_IO_PENDING) {
		WaitForSingleObject(hEv, INFINITE);
		GetOverlappedResult(hPipe, &ov, &numBytes, FALSE);
	}

	_tprintf_s(_T("[BOT] Enviada tentativa de registo com o nome '%s'.\n"), messagePlayer.message);

	ZeroMemory(&ov, sizeof(OVERLAPPED));
	ov.hEvent = hEv;
	result = ReadFile(hPipe, &registerSuccess, sizeof(registerSuccess), &numBytes, &ov);
	if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
		_tprintf_s(_T("[BOT] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (ReadFile)\n"), messagePlayer.message, result, numBytes);
		exit(-1);;
	}
	if (GetLastError() == ERROR_IO_PENDING) {
		WaitForSingleObject(hEv, INFINITE);
		GetOverlappedResult(hPipe, &ov, &numBytes, FALSE);
	}
	if (!registerSuccess) {
		_tprintf_s(_T("[BOT] O username '%s' já está em uso. Tenta com outro!\n"), messagePlayer.message);
		exit(-1);
	}
	_tprintf_s(_T("[BOT] Registo efetuado com sucesso.\n"));
	hEventExit = CreateEvent(NULL, FALSE, FALSE, NULL);

	threadReadPipe.hPipe = hPipe;
	threadReadPipe.hMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
	threadReadPipe.hEventExit = hEventExit;
	hThreadLePipe = CreateThread(NULL, 0, readPipe, (LPVOID)&threadReadPipe, 0, NULL);

	threadShowLetters.hEventExit = hEventExit;
	hThreadShowLetters = CreateThread(NULL, 0, showLetters, (LPVOID)&threadShowLetters, 0, NULL);

	do {
		if (WaitForSingleObject(hEventExit, reactionTime) == WAIT_OBJECT_0) {
			break;
		}
		randomWord = getRandomWordFromDictionary();
		messagePlayer.commandType = WORDGUESS;
		_tcscpy_s(messagePlayer.message, MSG_TAM, randomWord);

		ZeroMemory(&ov, sizeof(OVERLAPPED));
		ov.hEvent = hEv;
		result = WriteFile(hPipe, &messagePlayer, sizeof(messagePlayer), &numBytes, &ov);
		if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
			_tprintf_s(_T("[BOT] ERRO a escrever mensagem %d (%d bytes)... (WriteFile)\n"), result, numBytes);
			continue;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEv, INFINITE);
			GetOverlappedResult(hPipe, &ov, &numBytes, FALSE);
		}
	} while (1);

	WaitForSingleObject(hThreadLePipe, INFINITE);
	WaitForSingleObject(hThreadShowLetters, INFINITE);
	CloseHandle(hThreadLePipe);
	CloseHandle(hThreadShowLetters);
	CloseHandle(hEv);
	CloseHandle(hPipe);
	_tprintf_s(_T("[BOT] A Thread main a desligar-se.\n"));
	ExitThread(0);
}
