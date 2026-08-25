#include "assetroute89_posix.h"
#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

static int ar89_p_exists(void *user, const char *path)
{
    struct stat st;
    (void)user;
    return path && stat(path,&st)==0 ? AR89_PROVIDER_FOUND : AR89_PROVIDER_MISSING;
}

static int ar89_p_join(char *out, unsigned int cap, const char *a, const char *b)
{
    unsigned int n=0U,i=0U;
    while (a[i] && n+1U<cap) out[n++]=a[i++];
    if (n && out[n-1U]!='/' && n+1U<cap) out[n++]='/';
    i=0U; while (b[i] && n+1U<cap) out[n++]=b[i++];
    out[n]='\0'; return b[i]=='\0';
}
static int ar89_p_enum_impl(const char *root, int recursive, ar89_enum_sink_fn sink, void *sink_user)
{
    DIR *d;
    struct dirent *de;
    struct stat st;
    char path[AR89_PATH_CAP];
    int r;
    d=opendir(root); if(!d) return AR89_PROVIDER_ERROR;
    while((de=readdir(d))!=0){
        if(strcmp(de->d_name,".")==0||strcmp(de->d_name,"..")==0) continue;
        if(!ar89_p_join(path,AR89_PATH_CAP,root,de->d_name)) continue;
        if(stat(path,&st)!=0) continue;
        if(S_ISDIR(st.st_mode)) { if(recursive){r=ar89_p_enum_impl(path,recursive,sink,sink_user); if(r==AR89_PROVIDER_ERROR){closedir(d);return r;}} }
        else { if(!sink(sink_user,path)){closedir(d);return AR89_PROVIDER_ERROR;} }
    }
    closedir(d); return AR89_PROVIDER_FOUND;
}
static int ar89_p_enum(void *user,const char *root,int recursive,ar89_enum_sink_fn sink,void *sink_user)
{ (void)user; if(!root||!sink) return AR89_PROVIDER_ERROR; return ar89_p_enum_impl(root,recursive,sink,sink_user); }
void ar89_posix_make_provider(AR89_FileProvider *p){ if(!p)return; p->exists=ar89_p_exists;p->enumerate=ar89_p_enum;p->user=0; }
#else
void ar89_posix_make_provider(AR89_FileProvider *p){ if(!p)return; p->exists=0;p->enumerate=0;p->user=0; }
#endif
