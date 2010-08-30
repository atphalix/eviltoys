#include "quakedef.h"
#include "fs.h"

#ifdef _WIN32
#include "winquake.h"
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#endif

#include "EX_browser.h"

// sources table
source_data *sources[MAX_SOURCES];
int sourcesn;

source_data * Create_Source(void)
{
    source_data *s;
    s = (source_data *) Q_malloc(sizeof(source_data));
    s->serversn = 0;
    s->last_update.wYear = 0;
    s->name[0] = 0;
    s->checked = 0;
    s->servers = NULL;
    s->unique = source_unique++;
    return s;
}

void Reset_Source(source_data *s)
{
    int i;
    if (servers != NULL)
    {
        for (i=0; i < s->serversn; i++)
            Q_free(s->servers[i]);
        Q_free(s->servers);
    }
    s->serversn = 0;
    s->last_update.wYear = 0;
    //s->name[0] = 0;
}

void Delete_Source(source_data *s)
{
    Reset_Source(s);
    Q_free(s);
}


// returns true, if there were some problems (like domain-name addresses)
// which require the source to be dumped to file in corrected form
qbool Update_Source_From_File(source_data *s, char *fname, server_data **servers, int *pserversn)
{
#ifndef WITH_FTE_VFS
    FILE *f;
#else
	vfsfile_t *f;
	char line[2048];
#endif
    qbool should_dump = false;

    //length = COM_FileOpenRead (fname, &f);
#ifndef WITH_FTE_VFS
    FS_FOpenFile(fname, &f);

    if (f) {
		while (!feof(f))
		{
			char c = 'A';
			char line[2048];
			netadr_t addr;
			int ret;

			ret = fscanf(f, "%s", line);
			while (!feof(f)  &&  c != '\n')
				fscanf(f, "%c", &c);

			if (ret != 1)
				continue;
#else
	f = FS_OpenVFS(fname, "rb", FS_ANY);

    if (f) {
		while (VFS_GETS(f, line, sizeof(line)))
		{
			netadr_t addr;
#endif
			if (!strchr(line, ':'))
				strlcat (line, ":27000", sizeof (line));
			if (!NET_StringToAdr(line, &addr))
				continue;

			servers[(*pserversn)++] = Create_Server2(addr);
			if (line[0] <= '0'  ||  line[0] >= '9')
				should_dump = true;
		}
#ifndef WITH_FTE_VFS
		fclose(f);
#else
		VFS_CLOSE(f);
#endif
    } else {
        //Com_Printf ("Updating %15.15s failed: file not found\n", s->name);
        // ?????? should_dump = true;
	}

    return should_dump;
}

void Precache_Source(source_data *s)
{
    int i;
    char name[1024];
    server_data *servers[MAX_SERVERS];
    int serversn = 0;

    if (s->type != type_master)
        return;
    
    snprintf(name, sizeof (name), "sb/cache/%d_%d_%d_%d_[%d].txt",
            s->address.address.ip[0], s->address.address.ip[1],
            s->address.address.ip[2], s->address.address.ip[3],
            ntohs(s->address.address.port));
    Update_Source_From_File(s, name, servers, &serversn);

    if (serversn > 0)
    {
        SYSTEMTIME tm;
        if (GetFileLocalTime(va("%s/%s", com_filesearchpath, name), &tm))
        {
            Reset_Source(s);
            s->servers = (server_data **) Q_malloc(serversn * sizeof(server_data *));
            for (i=0; i < serversn; i++)
                s->servers[i] = servers[i];
            s->serversn = serversn;

            if (s->checked)
                rebuild_servers_list = 1;

            memcpy(&s->last_update, &tm, sizeof(SYSTEMTIME));
        }
    }
}

void Update_Source(source_data *s)
{
    int i;
    qbool should_dump = false;
    server_data *servers[MAX_SERVERS];
    int serversn = 0;

	if (s->type == type_dummy)
        return;

    if (s->type == type_file)
    {
        // read servers from file
        char name[1024];
        snprintf(name, sizeof (name), "sb/%s", s->address.filename);
        should_dump = Update_Source_From_File(s, name, servers, &serversn);
        GetLocalTime(&(s->last_update));
    }

    if (s->type == type_master)
    {
        // get servers from master server
        char request[] = {'c', '\n', '\0'};

        int newsocket;
		struct sockaddr_storage server;
        int ret, i;
        unsigned char answer[10000];
        fd_set fd;
        struct timeval tv;

        newsocket = UDP_OpenSocket(PORT_ANY);
        // so we have a socket

        // send status request

        NetadrToSockadr (&(s->address.address), &server);
        ret = sendto (newsocket, request, sizeof(request), 0,
                      (struct sockaddr *)&server, sizeof(server) );
        if (ret < 0)
            return;

        //fd.fd_count = 1;
        //fd.fd_array[0] = newsocket;
    FD_ZERO(&fd);
  FD_SET(newsocket, &fd);
        tv.tv_sec = 0;
        tv.tv_usec = 1000 * 1.5 * sb_mastertimeout.value; // multiply timeout by 1.5
        ret = select(newsocket+1, &fd, NULL, NULL, &tv);

        // get answer
        if (ret > 0)
            ret = recvfrom (newsocket, answer, 10000, 0, NULL, NULL);

        if (ret > 0  &&  ret < 10000)
        {
            answer[ret] = 0;

            if (memcmp(answer, "\xff\xff\xff\xff\x64\x0a", 6))
            {
                closesocket(newsocket);
                return;
            }

            for (i=6; i+5 < ret; i+=6)
            {
                char buf[32];
                snprintf(buf, sizeof (buf), "%u.%u.%u.%u:%u",
                    (int)answer[i+0], (int)answer[i+1],
                    (int)answer[i+2], (int)answer[i+3],
                    256 * (int)answer[i+4] + (int)answer[i+5]);

                servers[serversn++] = Create_Server(buf);
            }
        }
 
        closesocket(newsocket);
        
    }

    // copy all servers to source list
    if (serversn > 0)
    {
        Reset_Source(s);
        s->servers = (server_data **) Q_malloc((serversn + (s->type==type_file ? MAX_UNBOUND : 0)) * sizeof(server_data *));
        for (i=0; i < serversn; i++)
            s->servers[i] = servers[i];
        s->serversn = serversn;

        if (s->checked)
            rebuild_servers_list = 1;

        if (sb_mastercache.value)
            DumpSource(s);

        GetLocalTime(&(s->last_update));
    }
    else
        if (s->type == type_file)
        {
            Reset_Source(s);
            s->servers = (server_data **) Q_malloc((serversn + (s->type==type_file ? MAX_UNBOUND : 0)) * sizeof(server_data *));
        }
    
    if (should_dump)
        DumpSource(s);
    //Com_Printf ("Updating %15.15s: %d servers\n", s->name, serversn);
}

int updating_sources = 0;
int source_full_update = 0;

typedef struct infohost_s
{
    double lastsenttime;
    int phase;
} infohost;

source_data **psources;
int psourcesn;

DWORD WINAPI Update_Multiple_Sources_Proc(void * lpParameter)
{
    // get servers from master server
    SYSTEMTIME lt;
    char request[] = {'c', '\n', '\0'};

    int newsocket;
	struct sockaddr_storage server;
    int ret, i, sourcenum;
    unsigned char answer[10000];
    fd_set fd;
    struct timeval tv;
    int total_masters = 0;
    int updated = 0;
    int d1, d2;

    GetLocalTime(&lt);
    d1 = lt.wSecond + 60*(lt.wMinute + 60*(lt.wHour + 24*(lt.wDay)));
    // update file sources - this should be a flash
    for (sourcenum = 0; sourcenum < psourcesn; sourcenum++)
        if (psources[sourcenum]->checked)
        {
            if (psources[sourcenum]->type == type_file)
                Update_Source(psources[sourcenum]);
            else if (psources[sourcenum]->type == type_master)
            {
                source_data *s = psources[sourcenum];
                if (s->last_update.wYear != 0  &&  !source_full_update)
                {
                    d2 = s->last_update.wSecond + 60*(s->last_update.wMinute + 60*(s->last_update.wHour + 24*(s->last_update.wDay)));

                    if (d1 > d2  &&  d1 < d2 + sb_sourcevalidity.value*60)
                    continue;
                }
                total_masters++;
            }
        }

    // update master sources
    newsocket = UDP_OpenSocket(PORT_ANY);

    for (sourcenum = 0; sourcenum < psourcesn  &&  !abort_ping; sourcenum++)
    {
        server_data *servers[MAX_SERVERS];
        int serversn = 0;
        int trynum = 0;
        source_data *s = psources[sourcenum];

        if (psources[sourcenum]->type != type_master  ||  !psources[sourcenum]->checked)
            continue;

        if (s->last_update.wYear != 0  &&  !source_full_update)
        {
            d2 = s->last_update.wSecond + 60*(s->last_update.wMinute + 60*(s->last_update.wHour + 24*(s->last_update.wDay)));

            if (d1 > d2  &&  d1 < d2 + sb_sourcevalidity.value*60)
                continue;
        }

        while (trynum < sb_masterretries.value)
        {
			struct sockaddr_storage hostaddr;
            netadr_t from;

            trynum++;

            NetadrToSockadr (&(s->address.address), &server);
            ret = sendto (newsocket, request, sizeof(request), 0,
                          (struct sockaddr *)&server, sizeof(server) );
            if (ret == -1)
                continue;

            //fd.fd_count = 1;
            //fd.fd_array[0] = newsocket;
        FD_ZERO(&fd);
      FD_SET(newsocket, &fd);
            tv.tv_sec = 0;
            tv.tv_usec = 1000 * sb_mastertimeout.value;
            ret = select(newsocket+1, &fd, NULL, NULL, &tv);

            // get answer
            i = sizeof(hostaddr);
            if (ret > 0)
                ret = recvfrom (newsocket, answer, 10000, 0,
				(struct sockaddr *)&hostaddr, (socklen_t *)&i);

            if (ret > 0  &&  ret < 10000)
            {
                SockadrToNetadr (&hostaddr, &from);

                if (from.ip[0] == s->address.address.ip[0] &&
                    from.ip[1] == s->address.address.ip[1] &&
                    from.ip[2] == s->address.address.ip[2] &&
                    from.ip[3] == s->address.address.ip[3] &&
                    from.port == s->address.address.port)
                {
                    answer[ret] = 0;

                    if (memcmp(answer, "\xff\xff\xff\xff\x64\x0a", 6))
                    {
                        continue;
                    }

                    for (i=6; i+5 < ret; i+=6)
                    {
                        char buf[32];
                        snprintf (buf, sizeof (buf), "%u.%u.%u.%u:%u",
                            (int)answer[i+0], (int)answer[i+1],
                            (int)answer[i+2], (int)answer[i+3],
                            256 * (int)answer[i+4] + (int)answer[i+5]);

                        servers[serversn++] = Create_Server(buf);
                    }

                    updated++;
                    break;
                }
            }
        }
        // copy all servers to source list
        if (serversn > 0)
        {
            Reset_Source(s);
            s->servers = (server_data **) Q_malloc(serversn * sizeof(server_data *));
            for (i=0; i < serversn; i++)
                s->servers[i] = servers[i];
            s->serversn = serversn;

            if (s->checked)
                rebuild_servers_list = 1;

            GetLocalTime(&(s->last_update));

            if (sb_mastercache.value)
                DumpSource(s);
        }

        ping_pos = updated / (double)total_masters;
    }

    closesocket(newsocket);

    if (abort_ping)
        Sys_MSleep(500);

    updating_sources = 0;
    return 0;    
}

void Update_Multiple_Sources(source_data *s[], int sn)
{

    psources = s;
    psourcesn = sn;

    abort_ping = 0;
    updating_sources = 1;
    ping_pos = 0;

    Sys_CreateThread(Update_Multiple_Sources_Proc, NULL);

}

void Toggle_Source(source_data *s)
{
    s->checked = !(s->checked);
    rebuild_servers_list = 1;
}

void Mark_Source(source_data *s)
{
    if (!s->checked)
    {
        s->checked = 1;
        rebuild_servers_list = 1;
    }
}

void Unmark_Source(source_data *s)
{
    if (s->checked)
    {
        s->checked = 0;
        rebuild_servers_list = 1;
    }
}

char * next_space(char *s)
{
    char *ret = s;
    while (*ret  &&  !isspace2(*ret))
        ret++;
    return ret;
}

char * next_nonspace(char *s)
{
    char *ret = s;
    while (*ret  &&  isspace2(*ret))
        ret++;
    return ret;
}

char * next_quote(char *s)
{
    char *ret = s;
    while (*ret  &&  *ret != '\"')
        ret++;
    return ret;
}

void Reload_Sources(void)
{
    int i;
#ifndef WITH_FTE_VFS
    FILE *f;
    int length;
#else
	vfsfile_t *f;
	char ln[2048];
#endif
    source_data *s;

    for (i=0; i < sourcesn; i++)
        Delete_Source(sources[i]);
    sourcesn = 0;

#ifndef WITH_FTE_VFS
    //length = COM_FileOpenRead (SOURCES_PATH, &f);
    length = FS_FOpenFile("sb/sources.txt", &f);
    if (length < 0)
    {
        //Com_Printf ("sources file not found: %s\n", SOURCES_PATH);
        return;
    }

#else
	f = FS_OpenVFS("sb/sources.txt", "rb", FS_ANY);
	if (!f) 
	{
        //Com_Printf ("sources file not found: %s\n", SOURCES_PATH);
		return;
	}
#endif

    // create dummy unbound source
    sources[0] = Create_Source();
    sources[0]->type = type_dummy;
    strlcpy (sources[0]->name, "Unbound", sizeof (sources[0]->name));
    sources[0]->servers = (server_data **) Q_malloc(MAX_UNBOUND*sizeof(server_data *));
    sourcesn = 1;

    s = Create_Source();
#ifndef WITH_FTE_VFS
    while (!feof(f))
    {
        char c = 'A';
        char line[2048];
        char *p, *q;

        if (fscanf(f, "%[ -~	]s", line) != 1)
		{
			while (!feof(f)  &&  c != '\n')
				fscanf(f, "%c", &c);
			continue;
		}
        while (!feof(f)  &&  c != '\n')
            fscanf(f, "%c", &c);
#else
    while (VFS_GETS(f, ln, sizeof(ln)))
    {
		char line[2048];
        char *p, *q;

        if (sscanf(ln, "%[ -~	]s", line) != 1) {
			continue;
		}
#endif

        p = next_nonspace(line);
        if (*p == '/')
            continue;   // comment
        q = next_space(p);

        if (!strncmp(p, "master", q-p))
            s->type = type_master;
        else
            if (!strncmp(p, "file", q-p))
                s->type = type_file;
            else
                continue;

        p = next_nonspace(q);
        q = (*p == '\"') ? next_quote(++p) : next_space(p);

        if (q-p <= 0)
            continue;

        strlcpy (s->name, p, min(q-p+1, MAX_SOURCE_NAME+1));

        p = next_nonspace(q+1);
        q = next_space(p);
        *q = 0;

        if (q-p <= 0)
            continue;

        if (s->type == type_file)
            strlcpy (s->address.filename, p, sizeof (s->address.filename));
        else
            if (!NET_StringToAdr(p, &(s->address.address)))
                continue;

        sources[sourcesn] = Create_Source();
        i = sources[sourcesn]->unique;
        memcpy(sources[sourcesn], s, sizeof(source_data));
        sources[sourcesn]->unique = i;
        sourcesn++;
    }

    Delete_Source(s);

#ifndef WITH_FTE_VFS
    fclose(f);
#else
	VFS_CLOSE(f);
#endif
    //Com_Printf("Read %d sources for Server Browser\n", sourcesn);

    // update all file sources
    for (i=0; i < sourcesn; i++)
        if (sources[i]->type == type_file)
            Update_Source(sources[i]);
        else if (sources[i]->type == type_master)
            Precache_Source(sources[i]);

    rebuild_servers_list = 1;
    resort_sources = 1;
}

int rebuild_servers_list = 0;

void Rebuild_Servers_List(void)
{
    int i;
    serversn = 0;
    for (i=0; i < sourcesn; i++)
        if (sources[i]->checked)
        {
            int j;
            for (j=0; j < sources[i]->serversn; j++)
            {
                int k;
                for (k=0; k < serversn; k++)
                    if (!memcmp(&(servers[k]->address),
                        &(sources[i]->servers[j]->address),
                        sizeof(netadr_t)))
                        break;
                if (k == serversn)  // if not on list yet
                    servers[serversn++] = sources[i]->servers[j];
            }
        }
    resort_servers = 1;
    rebuild_all_players = 1;
    Servers_pos = 0;
    rebuild_servers_list = 0;
    serversn_passed = serversn;
}

void DumpSource(source_data *s)
{
    FILE *f;
    int i;
    char buf[1024];

    if (s->type == type_file)
        snprintf(buf, sizeof (buf), "sb/%s", s->address.filename);
    else if (s->type == type_master)
    {
        Sys_mkdir("sb/cache");
        snprintf(buf, sizeof (buf), "sb/cache/%d_%d_%d_%d_[%d].txt",
                s->address.address.ip[0], s->address.address.ip[1],
                s->address.address.ip[2], s->address.address.ip[3],
                ntohs(s->address.address.port));
    }
    else
        return;

//    f = fopen(buf, "wt");
//    if (f == NULL)
//        return;
    if (!FS_FCreateFile(buf, &f, "ezquake", "wt"))
        return;

    for (i=0; i < s->serversn; i++)
        fprintf(f, "%d.%d.%d.%d:%d\n",
        s->servers[i]->address.ip[0], s->servers[i]->address.ip[1],
        s->servers[i]->address.ip[2], s->servers[i]->address.ip[3],
        ntohs(s->servers[i]->address.port));

    fclose(f);
}

void AddUnbound(server_data *s)
{
    if (sources[0]->serversn >= MAX_UNBOUND)
        return;

    if (IsInSource(sources[0], s))
        return;

    sources[0]->servers[sources[0]->serversn] = s;
    (sources[0]->serversn) ++;
    rebuild_servers_list = true;
}

void RemoveFromFileSource(source_data *source, server_data *serv)
{
    int i;
    
    if (source->serversn <= 0)
        return;

    for (i=0; i < source->serversn; i++)
        if (!memcmp(&source->servers[i]->address, &serv->address, 6))
        {
            // add to unbound
            AddUnbound(serv);

            // remove from source
            if (i != source->serversn - 1)
            {
                memmove(source->servers+i,
                        source->servers+i+1,
                        (source->serversn - i - 1) * sizeof(source_data *));
            }
            (source->serversn) --;
            DumpSource(source);
            Mark_Source(sources[0]);
            return;
        }
}

void AddToFileSource(source_data *source, server_data *serv)
{
    if (IsInSource(source, serv))
        return;

    source->servers[source->serversn] = serv;
    (source->serversn) ++;
    rebuild_servers_list = true;

    DumpSource(source);
    Mark_Source(sources[0]);
}
