# Word Identification Game — WIN32 Multi-Process System

A multi-process word identification game developed in **C** using the **WIN32 API**, featuring **inter-process communication**, **shared memory** and **synchronization** mechanisms.

This project simulates a dynamic multiplayer environment where users (and bots) attempt to guess words based on randomly revealed letters.

## Overview

The system is composed of **five independent processes** that interact through **named pipes**, **shared memory** with **synchronization mechanisms** such as **mutexes**, **events** and **semaphores**.

### Main Components
- **Arbitro (Referee)** – Central server managing the game state and player communication.  
- **Jogoui (Player)** – Console-based client that allows a human player to join and interact with the game.  
- **Bot** – Automated player that attempts to guess words at random intervals.  
- **Funcoes (Library)** – Static library containing shared functions used by both *Jogoui* and *Bot*.  
- **Painel (Panel)** – Optional graphical interface built with **Win32 GUI**, displaying real-time game data.

## Technologies Used

- **C Language (WIN32 API)** – Core system implementation  
- **Threads** – Concurrent processing of players, timers and admin commands  
- **Named Pipes** – Bidirectional communication between processes  
- **Shared Memory** – Real-time exchange of visible letters and scores  
- **Mutexes, Semaphores, Events, Critical Sections** – Synchronization and resource control  
- **Win32 GUI** – Visual interface for the game panel  

## System Architecture

Each program plays a specific role within the global system architecture:

### 1. Arbitro (Referee)
The main controller responsible for:
- Managing player registration and limits (via semaphore)
- Handling **named pipe** connections with each player
- Creating **threads** for:
  - Player communication (`playerAttend`)
  - Letter generation timer (`wordsTimer`)
  - Admin command processing
- Updating **shared memory** with new letters and notifying clients via **events**
- Maintaining a global shared structure (`appContext`) containing:
  - Synchronization handles (mutexes, events, semaphores)
  - Player list and scores
  - Game parameters (cadence, maximum visible letters, etc.)

All access to shared data is protected by a **Critical Section**.


### 2. Jogoui (Player)
A console-based player client that:
- Registers with the referee via **named pipes**
- Starts two **threads**:
  - Listening to referee messages  
  - Reading shared memory when triggered by an event  
- The main thread accepts user input (`stdin`) for:
  - Guessing words  
  - Checking scores (`:pont`)  
  - Viewing players (`:jogs`)  
  - Exiting (`:sair`)  

When disconnected, the player terminates safely.

### 3. Bot
An automated player sharing most of the *Jogoui* logic, but instead of reading user input:
- Periodically guesses random words from the dictionary  
- Communicates through named pipes just like a regular player  
- Is interpreted by the referee as a standard player  

### 4. Painel (Panel)
A **Win32 graphical interface** that:
- Reads data from shared memory only  
- Displays:
  - Last successfully guessed word  
  - Currently visible letters  
  - Leaderboard (top-scoring players)  
- Responds to **event signals** triggered by the referee  
- Supports dynamic updates using a **dedicated thread** that repaints the interface after data changes  
- Includes a configuration menu to change how many players are displayed  


### 5. Funcoes (Library)
A static library (`.lib`) that stores shared logic between *Jogoui* and *Bot*, mainly for:

## Communication Design

The system uses **named pipes** for all message exchange between the referee and clients.

Each message has a defined type:

```c
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
  INVALID
} COMMAND_TYPE;
```

## Communication Protocol

Communication follows a strict **two-step protocol**:

1. **Write 1:** The command type  
2. **Write 2:** The corresponding data structure 


This ensures clean, extensible communication without redundancy and improves message clarity between processes.

Each command has its own **dedicated data structure**, even when it contains only one field, this was made for keeping the design modular and easy to expand.

## Safely Shutdown

- When a player sends the **`:sair`** command, it triggers an **EXITGAME** message to the referee.  
- The referee responds with the same command, signaling the player’s event and unblocking any active `ReadFile()` operations safely.  
- On **expulsion (`:excluir`)**, the same logic applies, ensuring controlled termination.  
- When the referee ends the session (**`encerrar`**), all players receive an **EXPELLED** message and terminate safely.


---

*This work was completed as part of the “Operating Systems II” course during the 2024/2025 academic year in a group of 2 members. (Grade: 10 out of 10)*
