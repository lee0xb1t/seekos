#ifndef LIBC_FILE_H
#define LIBC_FILE_H
#include <libc/sysfunc.h>
#include <libc/types.h>


typedef void * FILE;

typedef struct _DIR{
    FILE *fh;
    int allocation;
    int cnt;
    void *data;
} DIR;

#define O_READ              0
#define O_WRITE             1
#define O_READWRITE         2

struct dirent {
    char name[256];
#define DIRENT_DIRECTORY        0
#define DIRENT_FILE             1
    int type;
    int sz;
};


FILE *open(const char *path, int mode);
int read(FILE *fh, int len, void *buff);
int write(FILE *fh, int len, void *buff);
void close(FILE *fh);

DIR *opendir(const char *path);
struct dirent *readdir(DIR * dirp);
void closedir(DIR * dirp);

#endif