#ifndef UTILS_H
#define UTILS_H

#pragma once
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include <windows.h>

#define MSG_TAM 20
#define MAX_PLAYERS 20
#define TAM 256
#define PLAYER_NAME_TAM 20
#define PIPE_NAME _T("\\\\.\\pipe\\pipetp")
#define MAX_MAXLETTERS 13
#define DEFAULT_MAXLETTERS 6
#define DEFAULT_RHYTHM 3

#define MUTEX_SHM _T("MUTEX_SHM")
#define SHM _T("SHM")

#define EVENT_SHM _T("EVENT_SHM")
#define EVENT_SHM_PAINEL _T("EVENT_SHM_PAINEL")
#define SHM_PANEL_GUI _T("SHM_PAINEL")
#define MUTEX_SHM_PANEL _T("MUTEX_SHM_PANEL")

#define KEY_RHYTHM _T("RITMO")
#define KEY_MAXLETTERS _T("MAXLETRAS")
#define KEY_PATH _T("Software\\TrabSO2")
#define DICT_SIZE 47

static const TCHAR* dictionary[DICT_SIZE] = {
	_T("a"),        // A
	_T("baba"),     // B, A
	_T("casa"),     // C, A, S
	_T("dado"),     // D, A, O
	_T("eco"),      // E, C, O
	_T("faca"),     // F, A, C
	_T("gato"),     // G, A, T, O
	_T("haste"),    // H, A, S, T, E
	_T("idade"),    // I, D, A, E
	_T("ja"),       // J, A
	_T("kilo"),     // K, I, L, O
	_T("lua"),      // L, U, A
	_T("mama"),     // M, A
	_T("nada"),     // N, A, D
	_T("ola"),      // O, L, A
	_T("papa"),     // P, A
	_T("que"),      // Q, U, E
	_T("rua"),      // R, U, A
	_T("saco"),     // S, A, C, O
	_T("tatu"),     // T, A, U
	_T("uva"),      // U, V, A
	_T("vaso"),     // V, A, S, O
	_T("wa"),       // W, A 
	_T("xale"),     // X, A, L, E
	_T("ya"),       // Y, A 
	_T("zoo"),      // Z, O
	_T("eu"),       // E, U
	_T("oi"),       // O, I
	_T("sol"),      // S, O, L
	_T("mar"),      // M, A, R
	_T("paz"),      // P, A, Z
	_T("rede"),     // R, E, D
	_T("voo"),      // V, O
	_T("leve"),     // L, E, V
	_T("fim"),      // F, I, M
	_T("no"),       // N, O
	_T("se"),       // S, E
	_T("te"),       // T, E
	_T("mel"),      // M, E, L
	_T("sal"),      // S, A, L
	_T("pau"),      // P, A, U
	_T("rei"),      // R, E, I
	_T("ver"),      // V, E, R
	_T("luz"),      // L, U, Z
	_T("cor"),      // C, O, R
	_T("dor"),      // D, O, R
	_T("rio")       // R, I, O
};

typedef enum {
	LIST,
	REMOVE,
	STARTBOT,
	ACELERATE,
	BRAKE,
	CLOSE,
	INCORRECT
} COMMAND_ADMIN;

typedef enum {
	REGISTER,
	PONT,
	JOGS,
	EXITGAME,
	WORDGUESS,
	JOIN,
	LEFT,
	GUESSED,
	LEADER,
	EXPELLED,
	INVALID,
} COMMAND_TYPE;

typedef struct {
	TCHAR letter;
	DWORD time;
} LETTER;

typedef struct {
	LETTER letters[MAX_MAXLETTERS];
} LETTERS_SHM;

typedef struct {
	HANDLE hPipe;
	TCHAR name[PLAYER_NAME_TAM];
	FLOAT score;
} PLAYER;

typedef struct {
	RECT dim;
	HWND hWnd;
	BOOL continuing;
	HANDLE hMutex, hThread;
	HINSTANCE hInst;
	TCHAR lastWordRight[MSG_TAM];
	TCHAR availableLetters[MAX_MAXLETTERS];
	DWORD numberActivePlayers;
	DWORD numberTopPlayers;
	PLAYER activePlayers[MAX_PLAYERS];
	TCHAR top10[MAX_PLAYERS][256];
} PANEL_DATA; // PAINEL

typedef struct {
	DWORD numActivePlayers;
	TCHAR lastWordRight[MSG_TAM];
	TCHAR lettersAvailable[MAX_MAXLETTERS];
	PLAYER activePlayers[MAX_PLAYERS];
} SHM_PANEL;

typedef struct {
	COMMAND_TYPE commandType;
	TCHAR message[MSG_TAM];
} MESSAGE_PLAYER;

typedef struct {
	FLOAT score;
} PONT_MESSAGE;

typedef struct {
	PLAYER playersList[MAX_PLAYERS];
	DWORD numPlayers;
} JOGS_MESSAGE;

typedef struct {
	COMMAND_TYPE commandType;
	DWORD wordSuccess;
} WORDGUESS_MESSAGE;

typedef struct {
	TCHAR username[PLAYER_NAME_TAM];
} JOIN_MESSAGE;

typedef struct {
	TCHAR username[PLAYER_NAME_TAM];
} LEFT_MESSAGE;

typedef struct {
	TCHAR username[PLAYER_NAME_TAM];
	TCHAR word[MSG_TAM];
} GUESSED_MESSAGE;

typedef struct {
	PLAYER newLeader;
} LEADER_MESSAGE;

typedef struct {
	PLAYER activePlayers[MAX_PLAYERS];
	DWORD numActivePlayers;
	BOOL hasGameStarted;
	BOOL continuing;
	HANDLE hMutexShm;
	HANDLE hSemaphore;
	HANDLE hSharedMemory;
	HANDLE hEventShm;
	HANDLE hEventExit;
	HANDLE hEventStartGame;
	HANDLE hEventPanel; // PAINEL
	HANDLE hSharedMemoryPanel; // PAINEL
	HANDLE hMutexShmPanel; // PAINEL
	SHM_PANEL* pShmPanel; // PAINEL
	CRITICAL_SECTION criticalSection;
	DWORD rhythm;
	DWORD maxLetters;
} APP_CONTEXT;

typedef struct {
	APP_CONTEXT* pAppContext;
	PLAYER thisPlayer;
} THREAD_ATTEND;

typedef struct {
	APP_CONTEXT* pAppContext;
} THREAD_TIMER;

typedef struct {
	APP_CONTEXT* pAppContext;
} THREAD_ADMIN;

typedef struct {
	HANDLE hPipe;
	HANDLE hMainThread;
	BOOL* continuing;
	HANDLE hEventExit;
} THREAD_READ_PIPE;

typedef struct {
	HANDLE hEventExit;
	BOOL* continuing;
} THREAD_SHOW_LETTERS;


#endif //UTILS_H