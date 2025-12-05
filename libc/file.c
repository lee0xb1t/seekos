#include <libc/file.h>
#include <libc/string.h>


FILE *open(const char *path, int mode) {
    int r = sys_open(path, mode);
    if (r < 0) {
        return null;
    }
    return (FILE *)r;
}

int read(FILE *fh, int len, void *buff) {
    return sys_read((i32)fh, len, buff);
}

int write(FILE *fh, int len, void *buff) {
    return sys_write((i32)fh, len, buff);
}

void close(FILE *fh) {
    sys_close((i32)fh);
}

DIR *opendir(const char *path) {
    DIR *dirp = null;

    dirp = (DIR *)sys_vmalloc(null, sizeof(DIR));

    dirp->fh = open(path, O_READWRITE);

    if (dirp->fh == null) {
        sys_vfree(dirp);
        return null;
    }

    dirp->allocation = 4096;
    dirp->data = sys_vmalloc(null, dirp->allocation);
    memset(dirp->data, 0, 4096);
    
    return dirp;
}

struct dirent *readdir(DIR * dirp) {
    int r = sys_readdir(dirp->fh, dirp->data, dirp->allocation);
    if (r < 0) {
        return null;
    }
    dirp->cnt = r;
    return (struct dirent *)dirp->data;
}

void closedir(DIR * dirp) {
    close(dirp->fh);
    sys_vfree(dirp->data);
    sys_vfree(dirp);
}
