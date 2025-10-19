#include "funcoes.h"

DWORD WINAPI readPipe(LPVOID dataArg) {
	THREAD_READ_PIPE* pThreadReadPipe = (THREAD_READ_PIPE*)dataArg;
	DWORD result, numBytes, i;
	COMMAND_TYPE expelled = EXPELLED;
	BOOL continuing = TRUE;


	COMMAND_TYPE commandType;
	PONT_MESSAGE pontMessage;
	JOGS_MESSAGE jogsMessage;
	WORDGUESS_MESSAGE wordGuessMessage;
	JOIN_MESSAGE joinMessage;
	LEFT_MESSAGE leftMessage;
	GUESSED_MESSAGE guessedMessage;
	LEADER_MESSAGE leaderMessage;
	MESSAGE_PLAYER messagePlayer;

	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
	do {
		ZeroMemory(&ov, sizeof(OVERLAPPED));
		ov.hEvent = hEv;
		result = ReadFile(pThreadReadPipe->hPipe, &commandType, sizeof(commandType), &numBytes, &ov);
		if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
			_tprintf_s(_T("[JOGADOR] Erro na thread readPipe!: %d (%d bytes)... (ReadFile)\n"), result, numBytes);
			continue;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEv, INFINITE);
			GetOverlappedResult(pThreadReadPipe->hPipe, &ov, &numBytes, FALSE);
		}
		_tprintf_s(_T("[JOGADOR] Recebi o comando '%d' (%d bytes) (ReadFile)\n"), commandType, numBytes);
		switch (commandType) {
		case PONT:
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = ReadFile(pThreadReadPipe->hPipe, &pontMessage, sizeof(pontMessage), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[JOGADOR] Erro na thread readPipe: %d (%d bytes)... (ReadFile)\n"), result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(pThreadReadPipe->hPipe, &ov, &numBytes, FALSE);
			}
			_tprintf_s(_T("[JOGADOR] A tua pontuação é %.2f Recebi %d bytes: '%d'... (ReadFile)\n"), pontMessage.score, result, numBytes);
			break;

		case JOGS:
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = ReadFile(pThreadReadPipe->hPipe, &jogsMessage, sizeof(jogsMessage), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[JOGADOR] Erro na thread readPipe: %d (%d bytes)... (ReadFile)\n"), result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(pThreadReadPipe->hPipe, &ov, &numBytes, FALSE);
			}
			_tprintf_s(_T("[JOGADOR] A lista de jogadores:\n"));
			for (i = 0; i < jogsMessage.numPlayers; i++) {
				_tprintf_s(_T("\t - [%d] %s: %0.2f\n"), i, jogsMessage.playersList[i].name, jogsMessage.playersList[i].score);
			}
			break;

		case EXITGAME:
			continuing = 0;
			break;

		case WORDGUESS:
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = ReadFile(pThreadReadPipe->hPipe, &wordGuessMessage, sizeof(wordGuessMessage), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[JOGADOR] Erro na thread readPipe: %d (%d bytes)... (ReadFile)\n"), result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(pThreadReadPipe->hPipe, &ov, &numBytes, FALSE);
			}
			if (wordGuessMessage.wordSuccess == 3) {
				_tprintf_s(_T("[JOGADOR] O jogo ainda não começou.\n"));
			}
			else if (wordGuessMessage.wordSuccess == 0) {
				_tprintf_s(_T("[JOGADOR] A palavra que tentaste não usa apenas as letras disponiveis.\n"));
			}
			else if (wordGuessMessage.wordSuccess == 1) {
				_tprintf_s(_T("[JOGADOR] A palavra que tentaste usa apenas as letras disponiveis, mas não está no dicionário.\n"));
			}
			else if (wordGuessMessage.wordSuccess == 2) {
				_tprintf_s(_T("[JOGADOR] Parabéns! Acertaste uma palavra! Já que usa apenas letras disponíveis e está no dicionário.\n"));
			}
			break;

		case JOIN:
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = ReadFile(pThreadReadPipe->hPipe, &joinMessage, sizeof(joinMessage), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[JOGADOR] Erro na thread readPipe: %d (%d bytes)... (ReadFile)\n"), result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(pThreadReadPipe->hPipe, &ov, &numBytes, FALSE);
			}
			_tprintf_s(_T("[JOGADOR] O jogador '%s' juntou-se ao jogo.\n"), joinMessage.username);
			break;

		case LEFT:
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = ReadFile(pThreadReadPipe->hPipe, &leftMessage, sizeof(leftMessage), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[JOGADOR] Erro na thread readPipe: %d (%d bytes)... (ReadFile)\n"), result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(pThreadReadPipe->hPipe, &ov, &numBytes, FALSE);
			}
			_tprintf_s(_T("[JOGADOR] O jogador '%s' saiu do jogo.\n"), leftMessage.username);
			break;

		case GUESSED:
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = ReadFile(pThreadReadPipe->hPipe, &guessedMessage, sizeof(guessedMessage), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[JOGADOR] Erro na thread readPipe: %d (%d bytes)... (ReadFile)\n"), result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(pThreadReadPipe->hPipe, &ov, &numBytes, FALSE);
			}
			_tprintf_s(_T("[JOGADOR] O jogador '%s' adivinhou a palavra '%s'.\n"), guessedMessage.username, guessedMessage.word);
			break;

		case LEADER:
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = ReadFile(pThreadReadPipe->hPipe, &leaderMessage, sizeof(leaderMessage), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[JOGADOR] Erro na thread readPipe: %d (%d bytes)... (ReadFile)\n"), result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(pThreadReadPipe->hPipe, &ov, &numBytes, FALSE);
			}
			_tprintf_s(_T("[JOGADOR] O jogador '%s' é agora o lider de pontuação com '%.2f' pontos.\n"), leaderMessage.newLeader.name, leaderMessage.newLeader.score);
			break;

		case EXPELLED:
			messagePlayer.commandType = EXPELLED;
			_tcscpy_s(messagePlayer.message, MSG_TAM, _T("expelled"));
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = WriteFile(pThreadReadPipe->hPipe, &messagePlayer, sizeof(messagePlayer), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[JOGADOR] Erro na thread readPipe, a escrever mensagem %d (%d bytes)... (WriteFile)\n"), result, numBytes);
				continue;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(pThreadReadPipe->hPipe, &ov, &numBytes, FALSE);
			}
			_tprintf_s(_T("[JOGADOR] Foste removido pelo arbitro!\n"));
			continuing = 0;
			break;

		case INVALID:
			break;
		}
	} while (continuing);
	_tprintf_s(_T("[JOGADOR] A thread readPipe a desligar-se.\n"));
	SetEvent(pThreadReadPipe->hEventExit);
	SetEvent(pThreadReadPipe->hEventExit);
	CloseHandle(hEv);
	ExitThread(0);
}

DWORD WINAPI showLetters(LPVOID dataArg) {
	THREAD_SHOW_LETTERS* pThreadShowLetters = (THREAD_SHOW_LETTERS*)dataArg;
	DWORD result;
	HANDLE hEvent, hMutex, hSharedMemory;
	TCHAR str[MAX_MAXLETTERS + 1];
	TCHAR* pStr;


	hEvent = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, EVENT_SHM);
	if (hEvent == NULL) {
		_tprintf(_T("[JOGADOR] Erro ao abrir o evento! Código: %lu\n"), GetLastError());
		ExitThread(1);
	}
	else {
		_tprintf(_T("[JOGADOR] Evento aberto com sucesso!\n"));
	}

	hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MUTEX_SHM);
	if (hMutex == NULL) {
		_tprintf(_T("[JOGADOR] Erro ao abrir o mutex! Código: %lu\n"), GetLastError());
		ExitThread(1);
	}
	else {
		_tprintf(_T("[JOGADOR] Mutex aberto com sucesso!\n"));
	}

	hSharedMemory = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM);
	if (hSharedMemory == NULL) {
		_tprintf(_T("[JOGADOR] Erro ao abrir a memória partilhada! Código: %lu\n"), GetLastError());
		ExitThread(1);
	}
	else {
		_tprintf(_T("[JOGADOR] Memória partilhada aberta com sucesso!\n"));

	}

	pStr = (TCHAR*)MapViewOfFile(hSharedMemory, FILE_MAP_READ, 0, 0, (MAX_MAXLETTERS + 1) * sizeof(TCHAR));
	if (pStr == NULL) {
		_tprintf(_T("[BOT] Erro ao mapear a memória! Código: %lu\n"), GetLastError());
		ExitThread(1);
	}

	HANDLE evToWait[2] = { hEvent, pThreadShowLetters->hEventExit };

	do {
		result = WaitForMultipleObjects(2, evToWait, FALSE, INFINITE);
		if (result == WAIT_OBJECT_0 + 1) {
			break;
		}
		WaitForSingleObject(hMutex, INFINITE);
		_tcscpy_s(str, MAX_MAXLETTERS + 1, pStr);
		ReleaseMutex(hMutex);
		_tprintf(_T("[JOGADOR] Letras recebidas: %s\n"), str);
	} while (1);

	_tprintf_s(_T("[JOGADOR] A thread showLetters a desligar-se.\n"));
	UnmapViewOfFile(pStr);
	CloseHandle(hSharedMemory);
	CloseHandle(hMutex);
	CloseHandle(hEvent);
	ExitThread(0);
}