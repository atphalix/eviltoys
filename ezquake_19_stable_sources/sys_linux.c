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

	$Id: sys_linux.c,v 1.32 2007-10-16 15:52:17 dkure Exp $

*/
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#ifndef __FreeBSD__
#include <linux/rtc.h>
#endif
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sched.h>
#include <errno.h>
#include <dirent.h>

#include <pthread.h>

#include "quakedef.h"

// BSD only defines FNDELAY:
#ifndef O_NDELAY
#  define O_NDELAY	FNDELAY
#endif


/* needed for RTC timer */
#define RTC_RATE 1024.00
static int rtc_fd;  /* file descriptor for rtc device */

int noconinput = 0;


qbool stdin_ready;
int do_stdin = 1;


cvar_t sys_yieldcpu = {"sys_yieldcpu", "0"};
cvar_t sys_nostdout = {"sys_nostdout", "0"};
cvar_t sys_extrasleep = {"sys_extrasleep", "0"};


void Sys_Printf (char *fmt, ...) {
	va_list argptr;
	char text[2048];
	unsigned char *p;

#ifdef NDEBUG
	if (!dedicated)
		return;
#endif

	va_start (argptr,fmt);
	vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	if (sys_nostdout.value)
		return;

	for (p = (unsigned char *) text; *p; p++)
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
}

void Sys_Quit (void) {
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~O_NDELAY);

	if (rtc_fd)
	    close(rtc_fd);

	exit(0);
}

void Sys_Init(void) {
	if (dedicated) {
		Cvar_Register (&sys_nostdout);
		Cvar_Register (&sys_extrasleep);
	}
	else {
    Cvar_SetCurrentGroup(CVAR_GROUP_SYSTEM_SETTINGS);
    Cvar_Register (&sys_yieldcpu);
    Cvar_ResetCurrentGroup();
	}
}

void Sys_Error (char *error, ...) {
        extern FILE *qconsole_log;
	va_list argptr;
	char string[1024];

	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~O_NDELAY);	//change stdin to non blocking

	if (rtc_fd)
		close(rtc_fd);

	va_start (argptr, error);
	vsnprintf (string, sizeof(string), error, argptr);
	va_end (argptr);
	fprintf(stderr, "Error: %s\n", string);
	if (qconsole_log)
	    fprintf(qconsole_log, "Error: %s\n", string);

	Host_Shutdown ();
	exit (1);
}

void Sys_mkdir (const char *path) {
    mkdir (path, 0777);
}

/*
================
Sys_remove
================
*/
int Sys_remove (char *path)
{
	return unlink(path);
}

// kazik -->
int Sys_chdir (const char *path)
{
    return (chdir(path) == 0);
}

char * Sys_getcwd (char *buf, int bufsize)
{
    return getcwd(buf, bufsize);
}
// kazik <--

double Sys_DoubleTime (void) {
    /* rtc timer vars */
    unsigned long curticks = 0;
    struct pollfd pfd;
    static unsigned long totalticks;

    /* old timer vars */
    struct timeval tp;
    struct timezone tzp;
    static int secbase;

    if (rtc_fd) {  /* rtc timer is enabled */
	pfd.fd = rtc_fd;
	pfd.events = POLLIN | POLLERR;
again:
	if (poll(&pfd, 1, 100000) < 0) {
	    if ((errno = EINTR)) {
		/* happens with gdb or signal exiting */
		goto again;
	    }
	    Sys_Error("Poll call on RTC timer failed!\n");
	}
	read(rtc_fd, &curticks, sizeof(curticks));
	curticks = curticks >> 8; /* knock out info byte */
	totalticks += curticks;
	return totalticks / RTC_RATE;
    } else { /* old timer */
	gettimeofday(&tp, &tzp);

	if (!secbase) {
	    secbase = tp.tv_sec;
            return tp.tv_usec/1000000.0;
	}

	return (tp.tv_sec - secbase) + tp.tv_usec / 1000000.0;
    }
}

void floating_point_exception_handler (int whatever) {
	signal(SIGFPE, floating_point_exception_handler);
}

char *Sys_ConsoleInput (void) {
	static char text[256];
	int len;

	if (!dedicated)
		return NULL;

	if (!stdin_ready || !do_stdin)
		return NULL; // the select didn't say it was ready
	stdin_ready = false;

	len = read (0, text, sizeof(text));
	if (len == 0) { // end of file
		do_stdin = 0;
		return NULL;
	}
	if (len < 1)
		return NULL;
	text[len - 1] = 0; // rip off the /n and terminate

	return text;
}

/* 0 is the highest. -1 may be a better setting. */
#define PRIORITYLEVEL -1

int set_realtime(void)
{
    struct sched_param schp;

    memset(&schp, 0, sizeof(schp));
    schp.sched_priority = sched_get_priority_max(SCHED_FIFO) + PRIORITYLEVEL;

    if (sched_setscheduler(0, SCHED_FIFO, &schp) != 0)
	return 0;

    return 1;
}

#ifndef id386
void Sys_HighFPPrecision (void) {}

void Sys_LowFPPrecision (void) {}
#endif

int main (int argc, char **argv) {
	double time, oldtime, newtime;
	int i;

	COM_InitArgv (argc, argv);

	// let me use -condebug C:\condebug.log before Quake FS init, so I get ALL messages before quake fully init
	if ((i = COM_CheckParm("-condebug")) && i < COM_Argc() - 1) {
		extern FILE *qconsole_log;
		char *s = COM_Argv(i + 1);
		if (*s != '-' && *s != '+')
			qconsole_log = fopen(s, "a");
	}

#if !defined(CLIENTONLY)
	dedicated = COM_CheckParm ("-dedicated");
#endif

	if (!dedicated) {
		signal(SIGFPE, SIG_IGN);

		// we need to check for -noconinput and -nostdout before Host_Init is called
		if (!(noconinput = COM_CheckParm("-noconinput")))
			fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

		if (COM_CheckParm("-nostdout"))
			sys_nostdout.value = 1;

#ifndef __FreeBSD__
		/* also check for -rtctimer before Host_Init is called */
		if (COM_CheckParm("-rtctimer")) {
		    int retval;
		    unsigned long tmpread;

		    /* try accessing rtc */
		    rtc_fd = open("/dev/rtc", O_RDONLY);
		    if (rtc_fd < 0)
			Sys_Error("Cannot open /dev/rtc! Exiting..\n");

		    /* make sure RTC is set to RTC_RATE (1024Hz) */
		    retval = ioctl(rtc_fd, RTC_IRQP_READ, &tmpread);
		    if (retval < 0)
			Sys_Error("Error with ioctl!\n");
		    if (tmpread != RTC_RATE)
			Sys_Error("RTC is not set to 1024Hz! Please use the command 'echo 1024 > /proc/sys/dev/rtc/max-user-freq' as root, or 'echo dev.rtc.max-user-freq=1024 >> /etc/sysctl.conf' as root to keep the change through reboots.\n");

		    /* take ownership of rtc */
		    retval = fcntl(rtc_fd, F_SETOWN, getpid());
		    if (retval < 0)
			Sys_Error("Cannot set ownership of /dev/rtc!\n");

		    /* everything is nice - now turn on the RTC's periodic timer */
		    retval = ioctl(rtc_fd, RTC_PIE_ON, 0);
		    if (retval == -1)
			Sys_Error("Error activating RTC timer!\n");

		    /* to make the most of the timer, want real time priority */
		    /* will probably only work if run q as root */
		    if (!set_realtime())
			Com_Printf("Realtime Priority not enabled..\n");
		    else
			Com_Printf("Realtime Priority enabled!\n");

		    Com_Printf("RTC Timer Enabled.\n");
		}
#endif
		#ifdef id386
			Sys_SetFPCW();
		#endif
	}

    Host_Init (argc, argv, 32 * 1024 * 1024);

	oldtime = Sys_DoubleTime ();
	while (1) {
		if (dedicated)
			NET_Sleep (10);

		// find time spent rendering last frame
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;
		oldtime = newtime;

		Host_Frame(time);

		if (dedicated) {
			if (sys_extrasleep.value)
				usleep (sys_extrasleep.value);
		}
    }
}

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length) {
	int r;
	unsigned long addr;
	int psize = getpagesize();

	addr = (startaddr & ~(psize - 1)) - psize;
	r = mprotect((char*) addr, length + startaddr - addr + psize, 7);
	if (r < 0)
    		Sys_Error("Protection change failed");
}

int  Sys_CreateThread(DWORD WINAPI (*func)(void *), void *param)
{
    pthread_t thread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setschedpolicy(&attr, SCHED_OTHER);   // ale gowno

    pthread_create(&thread, &attr, (void *)func, param);
    return 1;
}

// kazik -->
// directory listing functions
int CopyDirent(sys_dirent *ent, struct dirent *tmpent)
{
    struct stat statbuf;
    struct tm * lintime;
    if (stat(tmpent->d_name, &statbuf) != 0)
        return 0;

    strncpy(ent->fname, tmpent->d_name, MAX_PATH_LENGTH);
    ent->fname[MAX_PATH_LENGTH-1] = 0;

    lintime = localtime(&statbuf.st_mtime);
    UnixtimeToWintime(&ent->time, lintime);

    ent->size = statbuf.st_size;

    ent->directory = (statbuf.st_mode & S_IFDIR);

    return 1;
}

unsigned long Sys_ReadDirFirst(sys_dirent *ent)
{
    struct dirent *tmpent;
    DIR *dir = opendir(".");
    if (dir == NULL)
        return 0;

    tmpent = readdir(dir);
    if (tmpent == NULL)
    {
        closedir(dir);
        return 0;
    }

    if (!CopyDirent(ent, tmpent))
    {
        closedir(dir);
        return 0;
    }

    return (unsigned long)dir;
}

int Sys_ReadDirNext(unsigned long search, sys_dirent *ent)
{
    struct dirent *tmpent;

    tmpent = readdir((DIR*)search);

    if (tmpent == NULL)
        return 0;

    if (!CopyDirent(ent, tmpent))
        return 0;

    return 1;
}

void Sys_ReadDirClose(unsigned long search)
{
    closedir((DIR *)search);
}

void _splitpath(const char *path, char *drive, char *dir, char *file, char *ext)
{
    const char *f, *e;

    if (drive)
    drive[0] = 0;

    f = path;
    while (strchr(f, '/'))
    f = strchr(f, '/') + 1;

    if (dir)
    {
    strncpy(dir, path, min(f-path, _MAX_DIR));
        dir[_MAX_DIR-1] = 0;
    }

    e = f;
    while (*e == '.')   // skip dots at beginning
    e++;
    if (strchr(e, '.'))
    {
    while (strchr(e, '.'))
        e = strchr(e, '.')+1;
    e--;
    }
    else
    e += strlen(e);

    if (file)
    {
        strncpy(file, f, min(e-f, _MAX_FNAME));
    file[min(e-f, _MAX_FNAME-1)] = 0;
    }

    if (ext)
    {
    strncpy(ext, e, _MAX_EXT);
    ext[_MAX_EXT-1] = 0;
    }
}

// full path
char *Sys_fullpath(char *absPath, const char *relPath, int maxLength)
{
    // too small buffer, copy in tmp[] and then look is enough space in output buffer aka absPath
    if (maxLength-1 < PATH_MAX)	{
			 char tmp[PATH_MAX+1];
			 if (realpath(relPath, tmp) && absPath && strlen(tmp) < maxLength+1) {
					strlcpy(absPath, tmp, maxLength+1);
          return absPath;
			 }

       return NULL;
		}

    return realpath(relPath, absPath);
}
// kazik <--

#ifdef WITH_FTE_VFS
int Sys_EnumerateFiles (char *gpath, char *match, int (*func)(char *, int, void *), void *parm)
{
	DIR *dir, *dir2;
	char apath[MAX_OSPATH];
	char file[MAX_OSPATH];
	char truepath[MAX_OSPATH];
	char *s;
	struct dirent *ent;

	//printf("path = %s\n", gpath);
	//printf("match = %s\n", match);

	if (!gpath)
		gpath = "";
	*apath = '\0';

	strncpy(apath, match, sizeof(apath));
	for (s = apath+strlen(apath)-1; s >= apath; s--)
	{
		if (*s == '/')
		{
			s[1] = '\0';
			match += s - apath+1;
			break;
		}
	}
	if (s < apath)  //didn't find a '/'
		*apath = '\0';

	snprintf(truepath, sizeof(truepath), "%s/%s", gpath, apath);


	//printf("truepath = %s\n", truepath);
	//printf("gamepath = %s\n", gpath);
	//printf("apppath = %s\n", apath);
	//printf("match = %s\n", match);
	dir = opendir(truepath);
	if (!dir)
	{
		Com_DPrintf("Failed to open dir %s\n", truepath);
		return true;
	}
	do
	{
		ent = readdir(dir);
		if (!ent)
			break;
		if (*ent->d_name != '.')
			if (wildcmp(match, ent->d_name))
			{
				snprintf(file, sizeof(file), "%s/%s", gpath, ent->d_name);
				//would use stat, but it breaks on fat32.

				if ((dir2 = opendir(file)))
				{
					closedir(dir2);
					snprintf(file, sizeof(file), "%s%s/", apath, ent->d_name);
					//printf("is directory = %s\n", file);
				}
				else
				{
					snprintf(file, sizeof(file), "%s%s", apath, ent->d_name);
					//printf("file = %s\n", file);
				}

				if (!func(file, -2, parm))
				{
					closedir(dir);
					return false;
				}
			}
	} while(1);
	closedir(dir);

	return true;
}

#endif /* WITH_FTE_VFS */

/********************************* CLIPBOARD *********************************/

#define SYS_CLIPBOARD_SIZE		256
static char clipboard_buffer[SYS_CLIPBOARD_SIZE] = {0};

wchar *Sys_GetClipboardTextW(void) {
	return NULL;
}

char *Sys_GetClipboardData(void) {
	return clipboard_buffer;
}

void Sys_CopyToClipboard(char *text) {
	strlcpy(clipboard_buffer, text, SYS_CLIPBOARD_SIZE);
}

/*************************** INTER PROCESS CALLS *****************************/
#define PIPE_BUFFERSIZE		1024

FILE *fifo_pipe;

static char *Sys_PipeFile(void) {
	static char pipe[MAX_PATH] = {0};

	if (*pipe)
		return pipe;

	snprintf(pipe, sizeof(pipe), "/tmp/ezquake_fifo_%s", getlogin());
	return pipe;
}

void Sys_InitIPC()
{
	int fd;
	mode_t old;

	/* Don't use the user's umask, make sure we set the proper access */
	old = umask(0);
	if (mkfifo(Sys_PipeFile(), 0600)) {
		umask(old);
		// We failed ... 
		return;
	}
	umask(old); // Reset old mask

	/* Open in non blocking mode */
	if ((fd = open(Sys_PipeFile(), O_RDONLY | O_NONBLOCK)) == -1) {
		// We failed ...
		return;
	}
	if (!(fifo_pipe = fdopen(fd, "r"))) {
		// We failed ...
		return;
	}
}

void Sys_CloseIPC()
{
	if (fifo_pipe) {
		fclose(fifo_pipe);
		unlink(Sys_PipeFile());
	}
}

void Sys_ReadIPC()
{
	char buf[PIPE_BUFFERSIZE] = {0};
	int num_bytes_read = 0;

	if (!fifo_pipe)
		return;

	num_bytes_read = fread(buf, sizeof(buf), 1, fifo_pipe);

	COM_ParseIPCData(buf, num_bytes_read);
}

unsigned int Sys_SendIPC(const char *buf)
{
	FILE *fifo_out;
	int nelms_written;
	
	if (!(fifo_out = fopen(Sys_PipeFile(), "w")))
		return true;

	nelms_written = fwrite(buf, strlen(buf) + 1, 1, fifo_out);

	fclose(fifo_out);

	if (nelms_written != 1)
		return false;
	
	return true;
}


/********************************** SEMAPHORES *******************************/
/* Sys_Sem*() returns 0 on success; on error, -1 is returned */
int Sys_SemInit(sem_t *sem, int value, int max_value) 
{
	return sem_init(sem, 0, value); // Don't share between processes
}

int Sys_SemWait(sem_t *sem) 
{
	return sem_wait(sem);
}

int Sys_SemPost(sem_t *sem)
{
	return sem_post(sem);
}

int Sys_SemDestroy(sem_t *sem)
{
	return sem_destroy(sem);
}

