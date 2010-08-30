/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sys.h -- non-portable functions

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

#ifdef _WIN32
#define Sys_MSleep(x) Sleep(x)
#else
#define Sys_MSleep(x) usleep((x) * 1000)
#endif

#ifndef _WIN32
#define DWORD unsigned int
#define WINAPI
#ifndef MAX_PATH
	#define MAX_PATH (1024)
#endif
#define _MAX_FNAME 1024
#define _MAX_EXT 64
#define _MAX_DIR 1024
#endif

#include "localtime.h"

// create thread (process under linux)
int  Sys_CreateThread(DWORD (WINAPI *func)(void *), void *param);

#define MAX_PATH_LENGTH 1024

typedef struct sys_dirent_s
{
    int directory;
    int hidden;
    char fname[MAX_PATH_LENGTH];
    unsigned int size;
    SYSTEMTIME time;
} sys_dirent; 

char *Sys_fullpath(char *absPath, const char *relPath, int maxLength);
unsigned long  Sys_ReadDirFirst(sys_dirent *);               // 0 if failed
int            Sys_ReadDirNext(unsigned long, sys_dirent *); // 0 if failed (EOF)
void           Sys_ReadDirClose(unsigned long);
int    Sys_chdir (const char *path);
char * Sys_getcwd (char *buf, int bufsize);


// file IO
void Sys_mkdir (const char *path);
int Sys_remove (char *path);
int Sys_EnumerateFiles (char *gpath, char *match, int (*func)(char *, int, void *), void *parm);

// memory protection
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length);

// an error will cause the entire program to exit
void Sys_Error (char *error, ...);

// send text to the console
void Sys_Printf (char *fmt, ...);

void Sys_Quit (void);

double Sys_DoubleTime (void);

char *Sys_ConsoleInput (void);

// Perform Key_Event () callbacks until the input que is empty
void Sys_SendKeyEvents (void);

void Sys_LowFPPrecision (void);
void Sys_HighFPPrecision (void);
void Sys_SetFPCW (void);

void Sys_Init (void);

wchar *Sys_GetClipboardTextW(void);
void Sys_CopyToClipboard(char *);

void Sys_GetFullExePath(char *path, unsigned int path_length, int long_name);

// Inter Process Call functions.
void Sys_InitIPC();
void Sys_ReadIPC();
void Sys_CloseIPC();
unsigned int Sys_SendIPC(const char *buf);

// Semaphore functions
#ifdef _WIN32
typedef HANDLE sem_t;
#else
#include <semaphore.h>
#endif
int Sys_SemInit(sem_t *sem, int value, int max_value);
int Sys_SemWait(sem_t *sem);
int Sys_SemPost(sem_t *sem);
int Sys_SemDestroy(sem_t *sem);
