#include "assetroute89_win32.h"
#ifdef _WIN32
#include <windows.h>
#include <string.h>
static int ar89_w_exists(void *user,const char *path){DWORD a;(void)user;if(!path)return AR89_PROVIDER_MISSING;a=GetFileAttributesA(path);return a==INVALID_FILE_ATTRIBUTES?AR89_PROVIDER_MISSING:AR89_PROVIDER_FOUND;}
static int ar89_w_join(char*out,unsigned int cap,const char*a,const char*b){unsigned int n=0U,i=0U;while(a[i]&&n+1U<cap)out[n++]=a[i++];if(n&&out[n-1U]!='/'&&out[n-1U]!='\\'&&n+1U<cap)out[n++]='\\';i=0U;while(b[i]&&n+1U<cap)out[n++]=b[i++];out[n]='\0';return b[i]=='\0';}
static int ar89_w_enum_impl(const char*root,int recursive,ar89_enum_sink_fn sink,void*sink_user){WIN32_FIND_DATAA fd;HANDLE h;char mask[AR89_PATH_CAP];char path[AR89_PATH_CAP];int r;if(!ar89_w_join(mask,AR89_PATH_CAP,root,"*"))return AR89_PROVIDER_ERROR;h=FindFirstFileA(mask,&fd);if(h==INVALID_HANDLE_VALUE)return AR89_PROVIDER_ERROR;do{if(strcmp(fd.cFileName,".")==0||strcmp(fd.cFileName,"..")==0)continue;if(!ar89_w_join(path,AR89_PATH_CAP,root,fd.cFileName))continue;if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){if(recursive){r=ar89_w_enum_impl(path,recursive,sink,sink_user);if(r==AR89_PROVIDER_ERROR){FindClose(h);return r;}}}else if(!sink(sink_user,path)){FindClose(h);return AR89_PROVIDER_ERROR;}}while(FindNextFileA(h,&fd));FindClose(h);return AR89_PROVIDER_FOUND;}
static int ar89_w_enum(void*user,const char*root,int recursive,ar89_enum_sink_fn sink,void*sink_user){(void)user;if(!root||!sink)return AR89_PROVIDER_ERROR;return ar89_w_enum_impl(root,recursive,sink,sink_user);}
void ar89_win32_make_provider(AR89_FileProvider*p){if(!p)return;p->exists=ar89_w_exists;p->enumerate=ar89_w_enum;p->user=0;}
#else
void ar89_win32_make_provider(AR89_FileProvider*p){if(!p)return;p->exists=0;p->enumerate=0;p->user=0;}
#endif
