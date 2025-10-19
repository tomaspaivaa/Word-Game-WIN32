#include "./utils.h"

COMMAND_ADMIN getCommandAdmin(TCHAR* str) {
	if (_tcscmp(str, _T("listar")) == 0) return LIST;
	if (_tcscmp(str, _T("excluir")) == 0) return REMOVE;
	if (_tcscmp(str, _T("iniciarbot")) == 0) return STARTBOT;
	if (_tcscmp(str, _T("acelerar")) == 0) return ACELERATE;
	if (_tcscmp(str, _T("travar")) == 0) return BRAKE;
	if (_tcscmp(str, _T("encerrar")) == 0) return CLOSE;
	return INCORRECT;
}

PLAYER* findPlayerByName(TCHAR* name, APP_CONTEXT* pAppContext) {
	for (DWORD i = 0; i < pAppContext->numActivePlayers; i++) {
		if (_tcscmp(pAppContext->activePlayers[i].name, name) == 0) {
			return &(pAppContext->activePlayers[i]);
		}
	}
	return NULL;
}

void commandToText(TCHAR* str, DWORD* j, TCHAR* user) {
	int k = 0;
	(*j)++; // salta o espaço msg(este espaço)textodamensagem   
	for (; str[*j] != ' ' && str[*j] != '\0'; (*j)++) {
		user[k] = str[*j];
		k++;
	}
	user[k] = '\0';
}

BOOL isUsernameAvailable(TCHAR* username, APP_CONTEXT* pAppContext) {
	for (DWORD i = 0; i < pAppContext->numActivePlayers; i++) {
		if (_tcscmp(pAppContext->activePlayers[i].name, username) == 0) {
			return FALSE;
		}
	}
	return TRUE;
}

PLAYER* verifyScores(APP_CONTEXT* appContext, FLOAT oldScore, TCHAR name[PLAYER_NAME_TAM]) {
	FLOAT highestScore = 0;
	DWORD iPreviousHighestScore = 0;
	DWORD iPostHighestScore = 0;
	for (DWORD i = 0; i < appContext->numActivePlayers; i++) {
		if (_tcscmp(appContext->activePlayers[i].name, name) == 0) {
			if (oldScore > highestScore) {
				iPreviousHighestScore = i;
				highestScore = appContext->activePlayers[i].score;
			}
			continue;
		}
		if (appContext->activePlayers[i].score > highestScore) {
			highestScore = appContext->activePlayers[i].score;
			iPreviousHighestScore = i;
		}
	}
	highestScore = 0;
	for (DWORD i = 0; i < appContext->numActivePlayers; i++) {
		if (appContext->activePlayers[i].score > highestScore) {
			highestScore = appContext->activePlayers[i].score;
			iPostHighestScore = i;
		}
	}
	if (iPreviousHighestScore == iPostHighestScore) { // NÃO HOUVE MUDANÇA NO LIDER
		return NULL;
	}
	return &(appContext->activePlayers[iPostHighestScore]);
}

BOOL verifyWordIsCorrect(TCHAR* sharedLetters, TCHAR* word) {
	TCHAR tempBuf[MAX_MAXLETTERS + 1];
	size_t i, j;
	size_t wordLen = _tcslen(word);
	_tcscpy_s(tempBuf, MAX_MAXLETTERS + 1, sharedLetters);

	for (i = 0; i < wordLen; i++) {
		BOOL found = FALSE;
		TCHAR currentChar = _totupper(word[i]);

		for (j = 0; j < MAX_MAXLETTERS; j++) {
			if (_totupper(tempBuf[j]) == currentChar) {
				tempBuf[j] = _T('*'); // marca como usada
				found = TRUE;
				break;
			}
		}
		if (!found) {
			return FALSE; // letra não encontrada ou já usada
		}
	}

	return TRUE;
}


BOOL verifyWordIsOnDictionary(TCHAR* word) {
	if (word == NULL || _tcslen(word) == 0) {
		return FALSE;
	}

	DWORD i;
	for (i = 0; i < DICT_SIZE; i++) {
		if (_tcsicmp(dictionary[i], word) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

void removeLettersInSharedMemory(HANDLE shm, TCHAR* string, DWORD maxLetters, HANDLE event, DWORD numActivePlayers, HANDLE eventPanel, SHM_PANEL* pShmPanel) {
	if (shm == NULL || string == NULL) return;

	TCHAR* pStr = (TCHAR*)MapViewOfFile(shm, FILE_MAP_WRITE, 0, 0, MAX_MAXLETTERS * sizeof(TCHAR));
	if (pStr == NULL) {
		_tprintf(_T("[ERRO] Não foi possível mapear a memória partilhada.\n"));
		return;
	}

	for (DWORD i = 0; string[i] != _T('\0'); i++) {
		TCHAR letra = string[i];
		for (DWORD j = 0; j < maxLetters; j++) {
			if (pStr[j] == letra) {
				pStr[j] = _T('_');  //substituir letral por '_'
				break;
			}
		}
	}

	_tcscpy_s(pShmPanel->lettersAvailable, MAX_MAXLETTERS, pStr);

	for (DWORD i = 0; i < numActivePlayers; i++) {
		SetEvent(event);
	}
	SetEvent(eventPanel);

	UnmapViewOfFile(pStr);
}

void readOrCreateRegistryValues(APP_CONTEXT* appContext) {
	HKEY hKey;
	DWORD size = sizeof(DWORD);
	DWORD maxLetters = DEFAULT_MAXLETTERS, rhythm = DEFAULT_RHYTHM;
	DWORD type;
	LSTATUS res, state;

	res = RegCreateKeyEx(HKEY_CURRENT_USER, KEY_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &state);

	size = sizeof(DWORD);
	res = RegQueryValueEx(hKey, KEY_MAXLETTERS, NULL, &type, (LPBYTE)&maxLetters, &size);
	if (res != ERROR_SUCCESS) {
		maxLetters = DEFAULT_MAXLETTERS;
		RegSetValueEx(hKey, KEY_MAXLETTERS, 0, REG_DWORD, (LPBYTE)&maxLetters, sizeof(DWORD));
	}
	if (maxLetters > MAX_MAXLETTERS - 1) {
		maxLetters = MAX_MAXLETTERS - 1;
		RegSetValueEx(hKey, KEY_MAXLETTERS, 0, REG_DWORD, (LPBYTE)&maxLetters, sizeof(DWORD));
	}
	size = sizeof(DWORD);
	res = RegQueryValueEx(hKey, KEY_RHYTHM, NULL, &type, (LPBYTE)&rhythm, &size);
	if (res != ERROR_SUCCESS) {
		rhythm = DEFAULT_RHYTHM;
		RegSetValueEx(hKey, KEY_RHYTHM, 0, REG_DWORD, (LPBYTE)&rhythm, sizeof(DWORD));
	}
	RegCloseKey(hKey);

	appContext->maxLetters = maxLetters;
	appContext->rhythm = rhythm;
}

void generateRandomLetter(LETTERS_SHM* pLettersShm, DWORD n) {
	// Garante semente apenas uma vez
	static BOOL seeded = FALSE;
	if (!seeded) {
		srand((unsigned)time(NULL));
		seeded = TRUE;
	}
	for (DWORD i = 0; i < n; ++i) {
		if (pLettersShm->letters[i].letter == '_') {
			pLettersShm->letters[i].letter = (TCHAR)(_T('a') + (rand() % 26));
			pLettersShm->letters[i].time = GetTickCount(); // Atualiza o tempo
			return;
		}
	}

	DWORD oldestIndex = 0;
	DWORD oldestTime = pLettersShm->letters[0].time;

	for (DWORD i = 1; i < n; ++i) {
		if (pLettersShm->letters[i].time < oldestTime) {
			oldestTime = pLettersShm->letters[i].time;
			oldestIndex = i;
		}
	}

	pLettersShm->letters[oldestIndex].letter = (TCHAR)(_T('a') + (rand() % 26));
	pLettersShm->letters[oldestIndex].time = GetTickCount(); // Atualiza o tempo
}

BOOL removeActiveUser(APP_CONTEXT* pAppContext, TCHAR nameToRemove[PLAYER_NAME_TAM]) {
	DWORD iToRemove = -1;
	for (DWORD i = 0; i < pAppContext->numActivePlayers; i++) {
		if (_tcscmp(pAppContext->activePlayers[i].name, nameToRemove) == 0) {
			iToRemove = i;
		}
	}
	if (iToRemove == -1) {
		_tprintf_s(_T("[ARBITRO] Não existe ninguém na lista dos jogadores ativos com o nome '%s'.\n"), nameToRemove);
		return FALSE;
	}
	DWORD* numActivePlayers = &(pAppContext->numActivePlayers);
	pAppContext->activePlayers[iToRemove] = pAppContext->activePlayers[*numActivePlayers - 1];
	ZeroMemory(&pAppContext->activePlayers[*numActivePlayers - 1], sizeof(PLAYER));
	(*numActivePlayers)--;
	return TRUE;
}

FLOAT getPlayerScore(APP_CONTEXT* pAppContext, TCHAR name[PLAYER_NAME_TAM]) {
	PLAYER* player = findPlayerByName(name, pAppContext);
	return player->score;	
}

void changePlayerScore(APP_CONTEXT* pAppContext, TCHAR name[PLAYER_NAME_TAM], FLOAT scoreToChange) {
	PLAYER* player = findPlayerByName(name, pAppContext);
	
	player->score += scoreToChange;

	if ((player->score) <= 0) {
		(player->score) = 0;
	}
}

DWORD WINAPI playerAttend(LPVOID dataArg) {
	THREAD_ATTEND* pThreadAttend = (THREAD_ATTEND*)dataArg;
	BOOL result, continuing = TRUE;
	DWORD numBytes, i;
	FLOAT oldScore;
	TCHAR* sharedLetters;
	TCHAR buffer[TAM];
	MESSAGE_PLAYER messagePlayer;
	TCHAR letters[MAX_MAXLETTERS];
	PLAYER playerToAttend = pThreadAttend->thisPlayer;
	DWORD* numActivePlayers = &(pThreadAttend->pAppContext->numActivePlayers);
	size_t wordLength;


	LEADER_MESSAGE leaderMessage;
	PONT_MESSAGE pontMessage;
	JOGS_MESSAGE jogsMessage;
	WORDGUESS_MESSAGE wordGuessMessage;
	LEFT_MESSAGE leftMessage;
	GUESSED_MESSAGE guessedMessage;

	CRITICAL_SECTION criticalSection = pThreadAttend->pAppContext->criticalSection;

	// Overlapped
	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
	_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s' iniciado.\n"), playerToAttend.name);
	do {
		ZeroMemory(&ov, sizeof(OVERLAPPED));
		ov.hEvent = hEv;
		result = ReadFile(playerToAttend.hPipe, &messagePlayer, sizeof(messagePlayer), &numBytes, &ov);
		if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
			_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (ReadFile)\n"), playerToAttend.name, result, numBytes);
			break;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEv, INFINITE);
			GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
		}
		_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s', pedido: '%s' (%d bytes)... (ReadFile)\n"), playerToAttend.name, messagePlayer.message, numBytes);
		// COMANDOS
		switch (messagePlayer.commandType) {
		case PONT:
			EnterCriticalSection(&criticalSection);
			pontMessage.score = getPlayerScore(pThreadAttend->pAppContext, playerToAttend.name);
			LeaveCriticalSection(&criticalSection);
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = WriteFile(playerToAttend.hPipe, &messagePlayer.commandType, sizeof(messagePlayer.commandType), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
			}
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = WriteFile(playerToAttend.hPipe, &pontMessage, sizeof(pontMessage), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
			}
			_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s', pontuação: %.2f (%d bytes)... (WriteFile)\n"), playerToAttend.name, pontMessage.score, numBytes);
			break;
		case JOGS:
			EnterCriticalSection(&criticalSection);
			jogsMessage.numPlayers = pThreadAttend->pAppContext->numActivePlayers;
			for (i = 0; i < *numActivePlayers; i++) {
				jogsMessage.playersList[i] = pThreadAttend->pAppContext->activePlayers[i];
			}
			LeaveCriticalSection(&criticalSection);
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = WriteFile(playerToAttend.hPipe, &messagePlayer.commandType, sizeof(messagePlayer.commandType), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
			}
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = WriteFile(playerToAttend.hPipe, &jogsMessage, sizeof(jogsMessage), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
			}
			_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s', enviou jogadores ativos. (%d bytes)... (WriteFile)\n"), playerToAttend.name, numBytes);
			break;

		case EXITGAME:
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = WriteFile(playerToAttend.hPipe, &messagePlayer.commandType, sizeof(messagePlayer.commandType), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[ARBITRO] ERRO fds na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
			}

			EnterCriticalSection(&criticalSection);
			result = removeActiveUser(pThreadAttend->pAppContext, playerToAttend.name);
			LeaveCriticalSection(&criticalSection);
			if (!result) {
				_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! Ao tentar remover o jogador.\n"), playerToAttend.name);
				break;
			}

			EnterCriticalSection(&criticalSection); // NA DUVIDA!!
			for (i = 0; i < pThreadAttend->pAppContext->numActivePlayers; i++) {
				if (_tcscmp(pThreadAttend->pAppContext->activePlayers[i].name, playerToAttend.name) == 0) {
					continue;
				}

				COMMAND_TYPE left = LEFT;
				result = WriteFile(pThreadAttend->pAppContext->activePlayers[i].hPipe, &left, sizeof(left), &numBytes, &ov);
				if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
					LeaveCriticalSection(&criticalSection);
					_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
					break;
				}
				if (GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEv, INFINITE);
					GetOverlappedResult(pThreadAttend->pAppContext->activePlayers[i].hPipe, &ov, &numBytes, FALSE);
				}
				_tcscpy_s(leftMessage.username, PLAYER_NAME_TAM, playerToAttend.name);
				result = WriteFile(pThreadAttend->pAppContext->activePlayers[i].hPipe, &leftMessage.username, sizeof(leftMessage.username), &numBytes, &ov);
				if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
					LeaveCriticalSection(&criticalSection);
					_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
					break;
				}
				if (GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEv, INFINITE);
					GetOverlappedResult(pThreadAttend->pAppContext->activePlayers[i].hPipe, &ov, &numBytes, FALSE);
				}
			}
			if (pThreadAttend->pAppContext->numActivePlayers == 1) {
				ResetEvent(pThreadAttend->pAppContext->hEventStartGame);
				pThreadAttend->pAppContext->hasGameStarted = FALSE;
				_tprintf_s(_T("[ARBITRO] O jogo terminou, só existe 1 jogador.\n"));
			}
			LeaveCriticalSection(&criticalSection);

			WaitForSingleObject(pThreadAttend->pAppContext->hMutexShmPanel, INFINITE); // PAINEL
			EnterCriticalSection(&criticalSection);
			pThreadAttend->pAppContext->pShmPanel->numActivePlayers = pThreadAttend->pAppContext->numActivePlayers; // PAINEL
			for (i = 0; i < pThreadAttend->pAppContext->numActivePlayers; i++) {
				pThreadAttend->pAppContext->pShmPanel->activePlayers[i] = pThreadAttend->pAppContext->activePlayers[i];
			} // PAINEL
			LeaveCriticalSection(&criticalSection);
			ReleaseMutex(pThreadAttend->pAppContext->hMutexShmPanel); // PAINEL
			SetEvent(pThreadAttend->pAppContext->hEventPanel); // PAINEL

			_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s', o jogador foi removido. (%d bytes)... (WriteFile)\n"), playerToAttend.name, numBytes);
			ReleaseSemaphore(pThreadAttend->pAppContext->hSemaphore, 1, NULL);
			continuing = FALSE;
			break;

		case WORDGUESS:
			WaitForSingleObject(pThreadAttend->pAppContext->hMutexShm, INFINITE);
			sharedLetters = (TCHAR*)MapViewOfFile(pThreadAttend->pAppContext->hSharedMemory, FILE_MAP_READ, 0, 0, (pThreadAttend->pAppContext->maxLetters + 1) * sizeof(TCHAR));
			if (sharedLetters == NULL) {
				_tprintf(_T("Erro ao mapear memória partilhada: %lu\n"), GetLastError());
				break;
			}
			EnterCriticalSection(&criticalSection);
			_tcscpy_s(letters, (pThreadAttend->pAppContext->maxLetters + 1), sharedLetters);
			LeaveCriticalSection(&criticalSection);
			ReleaseMutex(pThreadAttend->pAppContext->hMutexShm);
			EnterCriticalSection(&criticalSection);
			oldScore = getPlayerScore(pThreadAttend->pAppContext, playerToAttend.name);  // MERDA
			LeaveCriticalSection(&criticalSection);
			wordLength = _tcslen(messagePlayer.message);
			_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s', as letras disponiveis são: %s.\n"), playerToAttend.name, sharedLetters);
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = WriteFile(playerToAttend.hPipe, &messagePlayer.commandType, sizeof(messagePlayer.commandType), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
			}
			if (!(pThreadAttend->pAppContext->hasGameStarted)) {
				wordGuessMessage.wordSuccess = 3;
				ZeroMemory(&ov, sizeof(OVERLAPPED));
				ov.hEvent = hEv;
				result = WriteFile(playerToAttend.hPipe, &wordGuessMessage, sizeof(wordGuessMessage), &numBytes, &ov);
				if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
					_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
					break;
				}
				if (GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEv, INFINITE);
					GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
				}
				_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s', a palavra '%s' usa letras disponíveis, mas não está no dicionário.\n"), playerToAttend.name, messagePlayer.message);
			}
			if (!verifyWordIsCorrect(letters, messagePlayer.message)) {
				wordGuessMessage.wordSuccess = 0;
				ZeroMemory(&ov, sizeof(OVERLAPPED));
				ov.hEvent = hEv;
				result = WriteFile(playerToAttend.hPipe, &wordGuessMessage, sizeof(wordGuessMessage), &numBytes, &ov);
				if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
					_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
					break;
				}
				if (GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEv, INFINITE);
					GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
				}
				_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s', a palavra '%s' não usa apenas as letras disponiveis.\n"), playerToAttend.name, messagePlayer.message);
				EnterCriticalSection(&criticalSection);
				changePlayerScore(pThreadAttend->pAppContext, playerToAttend.name, - (FLOAT)wordLength / 2);
				LeaveCriticalSection(&criticalSection);
			}
			else if (verifyWordIsCorrect(letters, messagePlayer.message) && !verifyWordIsOnDictionary(messagePlayer.message)) {
				wordGuessMessage.wordSuccess = 1;
				ZeroMemory(&ov, sizeof(OVERLAPPED));
				ov.hEvent = hEv;
				result = WriteFile(playerToAttend.hPipe, &wordGuessMessage, sizeof(wordGuessMessage), &numBytes, &ov);
				if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
					_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
					break;
				}
				if (GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEv, INFINITE);
					GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
				}
				_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s', a palavra '%s' usa letras disponíveis, mas não está no dicionário.\n"), playerToAttend.name, messagePlayer.message);
				EnterCriticalSection(&criticalSection);
				changePlayerScore(pThreadAttend->pAppContext, playerToAttend.name, -(FLOAT)wordLength / 2);
				LeaveCriticalSection(&criticalSection);
			}
			else if (verifyWordIsCorrect(letters, messagePlayer.message) && verifyWordIsOnDictionary(messagePlayer.message)) {
				wordGuessMessage.wordSuccess = 2;
				ZeroMemory(&ov, sizeof(OVERLAPPED));
				ov.hEvent = hEv;
				result = WriteFile(playerToAttend.hPipe, &wordGuessMessage, sizeof(wordGuessMessage), &numBytes, &ov);
				if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
					_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
					break;
				}
				if (GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEv, INFINITE);
					GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
				}

				WaitForSingleObject(pThreadAttend->pAppContext->hMutexShm, INFINITE);
				EnterCriticalSection(&criticalSection);
				removeLettersInSharedMemory(pThreadAttend->pAppContext->hSharedMemory, messagePlayer.message, pThreadAttend->pAppContext->maxLetters, pThreadAttend->pAppContext->hEventShm, pThreadAttend->pAppContext->numActivePlayers, pThreadAttend->pAppContext->hMutexShmPanel, pThreadAttend->pAppContext->pShmPanel);
				LeaveCriticalSection(&criticalSection);
				ReleaseMutex(pThreadAttend->pAppContext->hMutexShm);

				_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s', a palavra '%s' usa letras disponíveis e está no dicionário.\n"), playerToAttend.name, messagePlayer.message);

				WaitForSingleObject(pThreadAttend->pAppContext->hMutexShmPanel, INFINITE); // PAINEL
				EnterCriticalSection(&criticalSection);
				_tcscpy_s(pThreadAttend->pAppContext->pShmPanel->lastWordRight, MSG_TAM, messagePlayer.message); // PAINEL
				LeaveCriticalSection(&criticalSection);
				ReleaseMutex(pThreadAttend->pAppContext->hMutexShmPanel); // PAINEL
				SetEvent(pThreadAttend->pAppContext->hEventPanel); // PAINEL

				EnterCriticalSection(&criticalSection);
				changePlayerScore(pThreadAttend->pAppContext, playerToAttend.name, (FLOAT)wordLength);
				for (i = 0; i < pThreadAttend->pAppContext->numActivePlayers; i++) {
					if (_tcscmp(pThreadAttend->pAppContext->activePlayers[i].name, playerToAttend.name) == 0) {
						continue;
					}
					COMMAND_TYPE guessed = GUESSED;
					ZeroMemory(&ov, sizeof(OVERLAPPED));
					ov.hEvent = hEv;
					result = WriteFile(pThreadAttend->pAppContext->activePlayers[i].hPipe, &guessed, sizeof(guessed), &numBytes, &ov);
					if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
						LeaveCriticalSection(&criticalSection);
						_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
						break;
					}
					if (GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEv, INFINITE);
						GetOverlappedResult(pThreadAttend->pAppContext->activePlayers[i].hPipe, &ov, &numBytes, FALSE);
					}
					_tcscpy_s(guessedMessage.username, PLAYER_NAME_TAM, playerToAttend.name);
					_tcscpy_s(guessedMessage.word, MSG_TAM, messagePlayer.message);
					ZeroMemory(&ov, sizeof(OVERLAPPED));
					ov.hEvent = hEv;
					result = WriteFile(pThreadAttend->pAppContext->activePlayers[i].hPipe, &guessedMessage, sizeof(guessedMessage), &numBytes, &ov);
					if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
						LeaveCriticalSection(&criticalSection);
						_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
						break;
					}
					if (GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEv, INFINITE);
						GetOverlappedResult(pThreadAttend->pAppContext->activePlayers[i].hPipe, &ov, &numBytes, FALSE);
					}
					_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s', enviado informações de que o jogador acertou uma palavra '%s' para '%s'.\n"), playerToAttend.name, messagePlayer.message, pThreadAttend->pAppContext->activePlayers[i].name);
				}
				LeaveCriticalSection(&criticalSection);
			}
			EnterCriticalSection(&criticalSection);
			PLAYER* newLeader = verifyScores(pThreadAttend->pAppContext, oldScore, playerToAttend.name);
			LeaveCriticalSection(&criticalSection);
			if (newLeader != NULL) {
				_tprintf_s(_T("[ARBITRO] Novo lider!.\n"));
				leaderMessage.newLeader = *(newLeader);
				EnterCriticalSection(&criticalSection);

				for (i = 0; i < pThreadAttend->pAppContext->numActivePlayers; i++) {
					COMMAND_TYPE leader = LEADER;
					ZeroMemory(&ov, sizeof(OVERLAPPED));
					ov.hEvent = hEv;
					result = WriteFile(pThreadAttend->pAppContext->activePlayers[i].hPipe, &leader, sizeof(leader), &numBytes, &ov);
					if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
						LeaveCriticalSection(&criticalSection);
						_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
						break;
					}
					if (GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEv, INFINITE);
						GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
					}
					ZeroMemory(&ov, sizeof(OVERLAPPED));
					ov.hEvent = hEv;
					result = WriteFile(pThreadAttend->pAppContext->activePlayers[i].hPipe, &leaderMessage, sizeof(leaderMessage), &numBytes, &ov);
					if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
						LeaveCriticalSection(&criticalSection);
						_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! %d (%d bytes)... (WriteFile)\n"), playerToAttend.name, result, numBytes);
						break;
					}
					if (GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEv, INFINITE);
						GetOverlappedResult(playerToAttend.hPipe, &ov, &numBytes, FALSE);
					}
					_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s', enviado informações do novo lider ao jogador '%s'.\n"), playerToAttend.name, pThreadAttend->pAppContext->activePlayers[i].name);
				}
				LeaveCriticalSection(&criticalSection);
			}

			WaitForSingleObject(pThreadAttend->pAppContext->hMutexShmPanel, INFINITE); // PAINEL
			EnterCriticalSection(&criticalSection);
			pThreadAttend->pAppContext->pShmPanel->numActivePlayers = pThreadAttend->pAppContext->numActivePlayers; // PAINEL
			for (i = 0; i < pThreadAttend->pAppContext->numActivePlayers; i++) {
				pThreadAttend->pAppContext->pShmPanel->activePlayers[i] = pThreadAttend->pAppContext->activePlayers[i];
			} // PAINEL
			LeaveCriticalSection(&criticalSection);
			ReleaseMutex(pThreadAttend->pAppContext->hMutexShmPanel); // PAINEL
			SetEvent(pThreadAttend->pAppContext->hEventPanel); // PAINEL

			break;
		case EXPELLED:
			WaitForSingleObject(pThreadAttend->pAppContext->hMutexShmPanel, INFINITE); // PAINEL
			EnterCriticalSection(&criticalSection);
			pThreadAttend->pAppContext->pShmPanel->numActivePlayers = pThreadAttend->pAppContext->numActivePlayers; // PAINEL
			for (i = 0; i < pThreadAttend->pAppContext->numActivePlayers; i++) {
				pThreadAttend->pAppContext->pShmPanel->activePlayers[i] = pThreadAttend->pAppContext->activePlayers[i];
			} // PAINEL
			LeaveCriticalSection(&criticalSection);
			ReleaseMutex(pThreadAttend->pAppContext->hMutexShmPanel); // PAINEL
			SetEvent(pThreadAttend->pAppContext->hEventPanel); // PAINEL

			continuing = FALSE;
			break;

		case INVALID:
			_tprintf_s(_T("[ARBITRO] ERRO na Thread para atender o cliente '%s'! O comando '%s' é inválido.\n"), playerToAttend.name, buffer);
			break;
		}
	} while (continuing);
	CloseHandle(hEv);
	_tprintf_s(_T("[ARBITRO] Thread para atender o cliente '%s' a desligar-se.\n"), playerToAttend.name);
	ExitThread(0);
}

DWORD WINAPI admin(LPVOID dataArg) {
	THREAD_ADMIN* pThreadAdmin = (THREAD_ADMIN*)dataArg;
	DWORD i, j, k, numBytes;
	PLAYER* player;
	TCHAR buf[256], commandAdmin[15], user[PLAYER_NAME_TAM];
	BOOL result;
	HKEY hKey;

	LEFT_MESSAGE leftMessage;

	CRITICAL_SECTION criticalSection = pThreadAdmin->pAppContext->criticalSection;

	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
	do {
		_fgetts(buf, 256, stdin);
		buf[_tcslen(buf) - 1] = _T('\0');
		for (i = 0; buf[i] != ' ' && buf[i] != '\0'; i++);
		for (j = 0; j < i; j++) {
			commandAdmin[j] = buf[j];
		}
		commandAdmin[j] = '\0';
		switch (getCommandAdmin(commandAdmin)) {
		case LIST:
			EnterCriticalSection(&criticalSection);
			if (pThreadAdmin->pAppContext->numActivePlayers == 0) {
				_tprintf_s(_T("[ARBITRO] A lista de jogadores está vazia!\n"));
				break;
			}
			_tprintf_s(_T("[ARBITRO] A lista de %d jogadores é:\n"), pThreadAdmin->pAppContext->numActivePlayers);
			for (k = 0; k < pThreadAdmin->pAppContext->numActivePlayers; k++) {
				_tprintf_s(_T("\t - [%d] %s: %0.2f\n"), k, pThreadAdmin->pAppContext->activePlayers[k].name, pThreadAdmin->pAppContext->activePlayers[k].score);
			}
			LeaveCriticalSection(&criticalSection);
			break;

		case REMOVE:
			commandToText(buf, &i, user);
			EnterCriticalSection(&criticalSection);
			player = findPlayerByName(user, pThreadAdmin->pAppContext);
			LeaveCriticalSection(&criticalSection);
			if (player == NULL) {
				_tprintf_s(_T("[ARBITRO] ERRO na Thread admin '%s'! Ao tentar remover o jogador.\n"), user);
				break;
			}
			HANDLE hPipeToRemove = player->hPipe;

			EnterCriticalSection(&criticalSection);
			result = removeActiveUser(pThreadAdmin->pAppContext, user);
			LeaveCriticalSection(&criticalSection);
			if (!result) {
				_tprintf_s(_T("[ARBITRO] ERRO na Thread admin! Ao tentar remover o jogador '%s'.\n"), user);
				break;
			}

			COMMAND_TYPE expelled = EXPELLED;
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = WriteFile(hPipeToRemove, &expelled, sizeof(expelled), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[ARBITRO] ERRO na Thread admin! %d (%d bytes)... (WriteFile)\n"), result, numBytes);
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(hPipeToRemove, &ov, &numBytes, FALSE);
			}
			EnterCriticalSection(&criticalSection);
			for (i = 0; i < pThreadAdmin->pAppContext->numActivePlayers; i++) {
				if (_tcscmp(pThreadAdmin->pAppContext->activePlayers[i].name, user) == 0) {
					continue;
				}
				COMMAND_TYPE left = LEFT;
				result = WriteFile(pThreadAdmin->pAppContext->activePlayers[i].hPipe, &left, sizeof(left), &numBytes, &ov);
				if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
					LeaveCriticalSection(&criticalSection);
					_tprintf_s(_T("[ARBITRO] ERRO na Thread admin! %d (%d bytes)... (WriteFile)\n"), result, numBytes);
					break;
				}
				if (GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEv, INFINITE);
					GetOverlappedResult(pThreadAdmin->pAppContext->activePlayers[i].hPipe, &ov, &numBytes, FALSE);
				}
				_tcscpy_s(leftMessage.username, PLAYER_NAME_TAM, user);
				result = WriteFile(pThreadAdmin->pAppContext->activePlayers[i].hPipe, &leftMessage.username, sizeof(leftMessage.username), &numBytes, &ov);
				if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
					LeaveCriticalSection(&criticalSection);
					_tprintf_s(_T("[ARBITRO] ERRO na Thread admin! %d (%d bytes)... (WriteFile)\n"), result, numBytes);
					break;
				}
				if (GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEv, INFINITE);
					GetOverlappedResult(pThreadAdmin->pAppContext->activePlayers[i].hPipe, &ov, &numBytes, FALSE);
				}
			}
			if (pThreadAdmin->pAppContext->numActivePlayers == 1) {
				ResetEvent(pThreadAdmin->pAppContext->hEventStartGame);
				pThreadAdmin->pAppContext->hasGameStarted = FALSE;
				_tprintf_s(_T("[ARBITRO] O jogo terminou, só existe 1 jogador.\n"));
			}
			LeaveCriticalSection(&criticalSection);
			_tprintf_s(_T("[ARBITRO] Thread admin, o jogador '%s' foi removido. (%d bytes)... (WriteFile)\n"), user, numBytes);
			ReleaseSemaphore(pThreadAdmin->pAppContext->hSemaphore, 1, NULL);
			break;

		case STARTBOT:
			commandToText(buf, &i, user);
			int tempoReacao = (rand() % 26) + 5; // 5 a 30 inclusive

			TCHAR cmdLine[MAX_PATH];
			_stprintf_s(cmdLine, MAX_PATH, _T("bot.exe %s %d"), user, tempoReacao);

			STARTUPINFO si = { sizeof(si) };
			PROCESS_INFORMATION pi;

			// criar o processo do bot
			if (CreateProcess(
				NULL,         // Nome do executável
				cmdLine,      // Linha de comando completa
				NULL, NULL,   // Segurança
				FALSE,        // Herança de handles
				CREATE_NO_WINDOW,            // Flags
				NULL, NULL,   // Ambiente e diretório
				&si, &pi)) {

				_tprintf(_T("Bot '%s' iniciado com tempo de reação %d segundos (PID: %lu)\n"),
					user, tempoReacao, pi.dwProcessId);

				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
			else {
				_tprintf(_T("Erro ao iniciar o bot (%lu).\n"), GetLastError());
			}

			break;

		case ACELERATE:
			EnterCriticalSection(&criticalSection);
			if ((pThreadAdmin->pAppContext->rhythm - 1) == 0) {
				LeaveCriticalSection(&criticalSection);
				_tprintf_s(_T("[ARBITRO] Thread admin, não podes diminuir, pois já se encontra no minimo de 1.\n"));
				break;
			}
			pThreadAdmin->pAppContext->rhythm--;
			LeaveCriticalSection(&criticalSection);
			_tprintf_s(_T("[ARBITRO] Thread admin, diminuiste o ritmo para: %d, e portanto, aceleraste.\n"), pThreadAdmin->pAppContext->rhythm);
			break;

		case BRAKE:
			EnterCriticalSection(&criticalSection);
			if (pThreadAdmin->pAppContext->rhythm == 12) {
				LeaveCriticalSection(&criticalSection);
				_tprintf_s(_T("[ARBITRO] Thread admin, não podes travar mais, ou seja, aumentar o ritmo mais de '%d'.\n"), pThreadAdmin->pAppContext->rhythm);
				break;
			}
			pThreadAdmin->pAppContext->rhythm++;
			LeaveCriticalSection(&criticalSection);
			_tprintf_s(_T("[ARBITRO] Thread admin, aumentaste o ritmo para: %d, e portanto, travaste.\n"), pThreadAdmin->pAppContext->rhythm);
			break;

		case CLOSE:
			if (RegCreateKeyEx(HKEY_CURRENT_USER, KEY_PATH, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {
				EnterCriticalSection(&criticalSection);
				DWORD rhythmValue = (DWORD)pThreadAdmin->pAppContext->rhythm;
				DWORD maxLettersValue = (DWORD)pThreadAdmin->pAppContext->maxLetters;
				LeaveCriticalSection(&criticalSection);

				RegSetValueEx(hKey, KEY_RHYTHM, 0, REG_DWORD, (LPBYTE)&rhythmValue, sizeof(DWORD));
				RegSetValueEx(hKey, KEY_MAXLETTERS, 0, REG_DWORD, (LPBYTE)&maxLettersValue, sizeof(DWORD));

				RegCloseKey(hKey);
				_tprintf_s(_T("[ARBITRO] Valores guardados no registo: %s = %d, %s = %d\n"),
					KEY_RHYTHM, rhythmValue, KEY_MAXLETTERS, maxLettersValue);
			}
			else {
				_tprintf_s(_T("[ARBITRO] ERRO ao abrir/criar a chave '%s' no registo.\n"), KEY_PATH);
			}

			EnterCriticalSection(&criticalSection);
			for (i = 0; i < pThreadAdmin->pAppContext->numActivePlayers; i++) {
				COMMAND_TYPE expelled = EXPELLED;
				ZeroMemory(&ov, sizeof(OVERLAPPED));
				ov.hEvent = hEv;
				result = WriteFile(pThreadAdmin->pAppContext->activePlayers[i].hPipe, &expelled, sizeof(expelled), &numBytes, &ov);
				if (!result && GetLastError() != ERROR_IO_PENDING) {
					LeaveCriticalSection(&criticalSection);
					_tprintf_s(_T("[ARBITRO] ERRO na Thread admin! %d (%d bytes)... (WriteFile)\n"), result, numBytes);
					break;
				}
				if (GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEv, INFINITE);
					GetOverlappedResult(pThreadAdmin->pAppContext->activePlayers[i].hPipe, &ov, &numBytes, FALSE);
				}
			}
			LeaveCriticalSection(&criticalSection);

			pThreadAdmin->pAppContext->continuing = 0;
			break;


		case INCORRECT:
			_tprintf_s(_T("[ARBITRO] Comando inválido, usa um dos seguintes:\n\t - listar \n\t - excluir <username> \n\t - iniciarbot <username> \n\t - acelerar \n\t - travar \n\t - encerrar\n"));
			break;
		}
	} while (pThreadAdmin->pAppContext->continuing);
	SetEvent(pThreadAdmin->pAppContext->hEventStartGame);
	SetEvent(pThreadAdmin->pAppContext->hEventExit);
	SetEvent(pThreadAdmin->pAppContext->hEventExit);
	CloseHandle(hEv);
	_tprintf_s(_T("[ARBITRO] Thread admin a desligar-se.\n"));
	ExitThread(0);
}

DWORD WINAPI wordsTimer(LPVOID dataArg) {
	THREAD_TIMER* threadTimer = (THREAD_TIMER*)dataArg;
	DWORD result, i;
	DWORD* timer = &threadTimer->pAppContext->rhythm;
	DWORD maxLetters = threadTimer->pAppContext->maxLetters;
	TCHAR str[MAX_MAXLETTERS];
	TCHAR* pStr;
	SHM_PANEL* pShmPanel;
	LETTERS_SHM lettersShm;

	CRITICAL_SECTION criticalSection = threadTimer->pAppContext->criticalSection;

	HANDLE evToWait[2] = { threadTimer->pAppContext->hMutexShm, threadTimer->pAppContext->hEventExit };

	pStr = (TCHAR*)MapViewOfFile(threadTimer->pAppContext->hSharedMemory, FILE_MAP_WRITE, 0, 0, MAX_MAXLETTERS * sizeof(TCHAR));
	if (pStr == NULL) {
		_tprintf(_T("[ARBITRO] Erro ao mapear a memória! Código: %lu\n"), GetLastError());
		ExitThread(1);
	}
	WaitForSingleObject(threadTimer->pAppContext->hMutexShm, INFINITE);
	for (i = 0; i < maxLetters; i++) {
		pStr[i] = '_';
	}
	ReleaseMutex(threadTimer->pAppContext->hMutexShm);

	pShmPanel = (SHM_PANEL*)MapViewOfFile(threadTimer->pAppContext->hSharedMemoryPanel, FILE_MAP_WRITE, 0, 0, sizeof(SHM_PANEL));
	if (pShmPanel == NULL) {
		_tprintf(_T("[ARBITRO] Erro ao mapear a memória! Código: %lu\n"), GetLastError());
		ExitThread(1);
	}
	do {
		WaitForSingleObject(threadTimer->pAppContext->hEventStartGame, INFINITE);
		WaitForSingleObject(threadTimer->pAppContext->hMutexShm, INFINITE);
		for (i = 0; i < MAX_MAXLETTERS; i++) {
			lettersShm.letters[i].letter = pStr[i];
		}
		ReleaseMutex(threadTimer->pAppContext->hMutexShm);
		generateRandomLetter(&lettersShm, maxLetters);
		for (i = 0; i < maxLetters; i++) {
			str[i] = lettersShm.letters[i].letter;
		}
		str[i] = _T('\0');
		result = WaitForMultipleObjects(2, evToWait, FALSE, INFINITE);
		if (result == WAIT_OBJECT_0 + 1) {
			break;
		}
		_tcscpy_s(pStr, maxLetters + 1, str);
		ReleaseMutex(threadTimer->pAppContext->hMutexShm);
		for (i = 0; i < threadTimer->pAppContext->numActivePlayers; i++) {
			SetEvent(threadTimer->pAppContext->hEventShm);
		}

		WaitForSingleObject(threadTimer->pAppContext->hMutexShmPanel, INFINITE); // PAINEL
		_tcscpy_s(pShmPanel->lettersAvailable, maxLetters + 1, str); // PAINEL
		ReleaseMutex(threadTimer->pAppContext->hMutexShmPanel); // PAINEL
		SetEvent(threadTimer->pAppContext->hEventPanel); // PAINEL

		EnterCriticalSection(&criticalSection);
		Sleep((*timer) * 1000);
		LeaveCriticalSection(&criticalSection);
	} while (threadTimer->pAppContext->continuing);
	UnmapViewOfFile(pShmPanel);
	UnmapViewOfFile(pStr);
	_tprintf_s(_T("[ARBITRO] Thread wordsTimer a desligar-se.\n"));
	ExitThread(0);
}

int _tmain(int argc, TCHAR* argv[]) {
	HANDLE hPipe, hThreadTimer, hThreadAdmin;
	HANDLE hThreadAttend[MAX_PLAYERS] = { 0 };
	THREAD_ADMIN threadAdmin;
	THREAD_TIMER threadTimer;
	THREAD_ATTEND threadAttend[MAX_PLAYERS];
	APP_CONTEXT appContext = { 0 };
	TCHAR buffer[TAM];
	DWORD i, j, numBytes;
	BOOL result, registerSuccess;

	MESSAGE_PLAYER messagePlayer;
	JOIN_MESSAGE joinMessage;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif 

	readOrCreateRegistryValues(&appContext);

	appContext.continuing = TRUE;
	appContext.hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MAX_MAXLETTERS * sizeof(TCHAR), SHM);
	appContext.hSemaphore = CreateSemaphore(NULL, MAX_PLAYERS, MAX_PLAYERS, NULL);
	appContext.hMutexShm = CreateMutex(NULL, FALSE, MUTEX_SHM);
	appContext.hEventStartGame = CreateEvent(NULL, TRUE, FALSE, NULL);
	appContext.hasGameStarted = FALSE;
	appContext.hEventShm = CreateEvent(NULL, FALSE, FALSE, EVENT_SHM);
	appContext.hEventExit = CreateEvent(NULL, FALSE, FALSE, NULL);
	appContext.hEventPanel = CreateEvent(NULL, FALSE, TRUE, EVENT_SHM_PAINEL); // PAINEL
	appContext.hSharedMemoryPanel = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SHM_PANEL), SHM_PANEL_GUI); // PAINEL
	appContext.hMutexShmPanel = CreateMutex(NULL, FALSE, MUTEX_SHM_PANEL); // PAINEL
	appContext.pShmPanel = (SHM_PANEL*)MapViewOfFile(appContext.hSharedMemoryPanel, FILE_MAP_WRITE, 0, 0, sizeof(SHM_PANEL));
	if (appContext.pShmPanel == NULL) {
		_tprintf(_T("[ARBITRO] Erro ao mapear a memória do painel! Código: %lu\n"), GetLastError());
		ExitThread(1);
	} // PAINEL
	_tcscpy_s(appContext.pShmPanel->lastWordRight, MSG_TAM, _T("NENHUMA\n"));
	InitializeCriticalSection(&appContext.criticalSection); // PAINEL

	threadAdmin.pAppContext = &(appContext);
	hThreadAdmin = CreateThread(NULL, 0, admin, (LPVOID)&threadAdmin, 0, NULL);

	threadTimer.pAppContext = &(appContext);
	hThreadTimer = CreateThread(NULL, 0, wordsTimer, (LPVOID)&threadTimer, 0, NULL);

	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
	do {
		WaitForSingleObject(appContext.hSemaphore, INFINITE);
		_tprintf_s(_T("[ARBITRO] Criar uma cópia do pipe '%s' ... (CreateNamedPipe)\n"), PIPE_NAME);
		hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, MAX_PLAYERS, sizeof(buffer), sizeof(buffer), 3000, NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf_s(_T("[ARBITRO] ERRO! Criar Named Pipe! (CreateNamedPipe)\n"));
			exit(-1);
		}
		_tprintf_s(_T("[ARBITRO] Esperar ligação de um jogador.\n"));

		HANDLE evToWait[2] = { appContext.hEventExit, hEv };
		ZeroMemory(&ov, sizeof(OVERLAPPED));
		ov.hEvent = hEv;
		result = ConnectNamedPipe(hPipe, &ov);
		if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
			_tprintf_s(_T("[ARBITRO] ERRO! Ligação ao jogador. (ConnectNamedPipe\n"));
			break;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			result = WaitForMultipleObjects(2, evToWait, FALSE, INFINITE);
			if (result == WAIT_OBJECT_0) {
				break;
			}
			else if (result == WAIT_OBJECT_0 + 1) {
				GetOverlappedResult(hPipe, &ov, &numBytes, FALSE);
			}
		}

		ZeroMemory(&ov, sizeof(OVERLAPPED));
		ov.hEvent = hEv;
		result = ReadFile(hPipe, &messagePlayer, sizeof(messagePlayer), &numBytes, &ov);
		if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
			_tprintf_s(_T("[ARBITRO] ERRO ao tentar ler registo! %d (%d bytes)... (ReadFile)\n"), result, numBytes);
			continue;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEv, INFINITE);
			GetOverlappedResult(hPipe, &ov, &numBytes, FALSE);
		}
		EnterCriticalSection(&appContext.criticalSection);
		for (i = 0; i < MAX_PLAYERS && appContext.activePlayers[i].hPipe != NULL; i++);
		BOOL isUsernameAvailablee = isUsernameAvailable(messagePlayer.message, &appContext);
		LeaveCriticalSection(&appContext.criticalSection);
		if (i < MAX_PLAYERS && isUsernameAvailablee) {
			EnterCriticalSection(&appContext.criticalSection);
			appContext.activePlayers[i].hPipe = hPipe;
			appContext.activePlayers[i].score = 0;
			_tcscpy_s(appContext.activePlayers[i].name, PLAYER_NAME_TAM, messagePlayer.message);
			LeaveCriticalSection(&appContext.criticalSection);
			registerSuccess = TRUE;
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = WriteFile(hPipe, &registerSuccess, sizeof(registerSuccess), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[ARBITRO] ERRO ao enviar o sucesso do registo do utilizador '%s'! %d (%d bytes)... (WriteFile)\n"), messagePlayer.message, result, numBytes);
				continue;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(hPipe, &ov, &numBytes, FALSE);
			}
			_tprintf_s(_T("[ARBITRO] Registo do jogador '%s' concluido.\n"), messagePlayer.message);

			EnterCriticalSection(&appContext.criticalSection);
			if (appContext.numActivePlayers != 0) {
				for (j = 0; j < appContext.numActivePlayers; j++) {
					COMMAND_TYPE join = JOIN;
					ZeroMemory(&ov, sizeof(OVERLAPPED));
					ov.hEvent = hEv;
					result = WriteFile(appContext.activePlayers[j].hPipe, &join, sizeof(join), &numBytes, &ov);
					if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
						_tprintf_s(_T("[ARBITRO] ERRO ao enviar mensagem do tipo join ao player '%s'. (WriteFile)\n"), appContext.activePlayers[j].name);
						continue;
					}
					if (GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEv, INFINITE);
						GetOverlappedResult(hPipe, &ov, &numBytes, FALSE);
					}
					_tcscpy_s(joinMessage.username, PLAYER_NAME_TAM, messagePlayer.message);
					result = WriteFile(appContext.activePlayers[j].hPipe, &joinMessage, sizeof(joinMessage), &numBytes, &ov);
					if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
						_tprintf_s(_T("[ARBITRO] ERRO ao enviar mensagem do tipo join ao player '%s'. (WriteFile)\n"), appContext.activePlayers[j].name);
						continue;
					}
					if (GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEv, INFINITE);
						GetOverlappedResult(hPipe, &ov, &numBytes, FALSE);
					}
				}
			}
			appContext.numActivePlayers++;
			WaitForSingleObject(appContext.hMutexShmPanel, INFINITE); // PAINEL
			appContext.pShmPanel->numActivePlayers = appContext.numActivePlayers; // PAINEL
			for (j = 0; j < appContext.numActivePlayers; j++) {
				appContext.pShmPanel->activePlayers[j] = appContext.activePlayers[j];
			} // PAINEL
			ReleaseMutex(appContext.hMutexShmPanel); // PAINEL
			SetEvent(appContext.hEventPanel); // PAINEL

			LeaveCriticalSection(&appContext.criticalSection);
		}
		else {
			registerSuccess = FALSE;
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			result = WriteFile(hPipe, &registerSuccess, sizeof(registerSuccess), &numBytes, &ov);
			if (!result && GetLastError() != ERROR_IO_PENDING) { // EOF (n == 0)
				_tprintf_s(_T("[ARBITRO] ERRO ao enviar o sucesso do registo do utilizador '%s'! %d (%d bytes)... (WriteFile)\n"), messagePlayer.message, result, numBytes);
				continue;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE);
				GetOverlappedResult(hPipe, &ov, &numBytes, FALSE);
			}
			_tprintf_s(_T("[ARBITRO] ERRO no registo do jogador '%s', username já existe.\n"), messagePlayer.message);
			continue;
		}
		EnterCriticalSection(&appContext.criticalSection);
		threadAttend[i].pAppContext = &appContext;
		threadAttend[i].thisPlayer = appContext.activePlayers[i];
		if (appContext.numActivePlayers == 2) {
			SetEvent(appContext.hEventStartGame);
			appContext.hasGameStarted = TRUE;
			_tprintf_s(_T("[ARBITRO] Jogo começou, existem pelo menos 2 jogadores.\n"));
		}
		LeaveCriticalSection(&appContext.criticalSection);
		hThreadAttend[i] = CreateThread(NULL, 0, playerAttend, (LPVOID)&threadAttend[i], 0, NULL);
	} while (appContext.continuing);

	for (i = 0; i < MAX_PLAYERS; i++) {
		if (hThreadAttend[i] == NULL) {
			continue;
		}
		WaitForSingleObject(hThreadAttend[i], INFINITE);

		CloseHandle(hThreadAttend[i]);
	}

	WaitForSingleObject(hThreadAdmin, INFINITE);
	WaitForSingleObject(hThreadTimer, INFINITE);
	CloseHandle(hEv);
	CloseHandle(appContext.hSharedMemory);
	CloseHandle(appContext.hSemaphore);
	CloseHandle(appContext.hMutexShm);
	CloseHandle(appContext.hEventShm);
	CloseHandle(appContext.hEventExit);
	CloseHandle(hThreadAdmin);
	CloseHandle(hThreadTimer);
	DeleteCriticalSection(&appContext.criticalSection);
	_tprintf_s(_T("[ARBITRO] Thread main a desligar-se.\n"));
	ExitThread(0);
}
