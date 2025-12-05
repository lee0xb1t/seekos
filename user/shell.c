#include <libc/sysfunc.h>
#include <libc/string.h>
#include <libc/print.h>
#include <libc/file.h>


#define MAX_ARGS    16

struct builtin_cmd_parse {
    char *cmd;
    char *args[MAX_ARGS];
    int cnt;
};

typedef void (*builtin_callback)(struct builtin_cmd_parse *);

struct builtin_cmd {
    char *cmd;
    builtin_callback callback;
};


static char cwd[256];
static char cmd[256];



char *get_cwd() {
    memset(cwd, 0, 256);
    sys_getcwd(cwd, sizeof(cwd));
    return cwd;
}

void print_cwd(struct builtin_cmd_parse *parse) {
    printf("%s\n", cwd);
}

void list_dir(struct builtin_cmd_parse *parse) {
    DIR* dirp = opendir(cwd);
    struct dirent *dp = readdir(dirp);
    for (int i = 0; i < dirp->cnt; i++) {
        printf("%s\t", dp[i].name);
    }
    printf("\n");
    closedir(dirp);
}

void change_dir(struct builtin_cmd_parse *parse) {
    // cd
    if (parse->cnt < 2) {
        return;
    }

    // char *temp = cmd;

    // for (int i = 0; i < strlen(cmd); i++) {
    //     if (temp[i] == ' ') {
    //         temp = (char*)&temp[i];
    //     }
    // }
    sys_chdir(parse->args[1]);
    get_cwd();
}

void parse_cmd(const char *s, struct builtin_cmd_parse *parse) {
    parse->cmd = sys_vmalloc(null, strlen(s)+1);
    memset(parse->cmd, 0, strlen(s)+1);
    memcpy(parse->cmd, s, strlen(s));

    char *prev = parse->cmd;

    int len = strlen(s)+1;
    for (int i = 0; i < len; i++) {
        if (parse->cmd[i] == ' ' || parse->cmd[i] == '\0') {
            parse->cmd[i] = '\0';

            if (strlen(prev) > 0) {
                parse->args[parse->cnt] = sys_vmalloc(null, strlen(prev)+1);
                memset(parse->args[parse->cnt], 0, strlen(prev)+1);
                memcpy(parse->args[parse->cnt], prev, strlen(prev));

                prev = &parse->cmd[i+1];
                
                parse->cnt++;

                if (parse->cnt >= MAX_ARGS) {
                    break;
                }
            }
        }
    }
}

void free_parse(struct builtin_cmd_parse *parse) {
    sys_vfree(parse->cmd);
    for (int i = 0; i < parse->cnt; i++) {
        sys_vfree(parse->args[i]);
        parse->args[i] = null;
    }
    parse->cmd = null;
    parse->cnt = 0;
}

char *read_cmd() {
    memset(cmd, 0, 256);
    scanf("%s", cmd);
    return cmd;
}


static struct builtin_cmd cmds[] = {
    {"ls", list_dir},
    {"cd", change_dir},
    {"pwd", print_cwd},
};

builtin_callback is_builtin(const char *cmd) {
    int cnt = sizeof(cmds) / sizeof(struct builtin_cmd);
    for (int i = 0; i < cnt; i++) {
        if (strcmp(cmds[i].cmd, cmd) == 0) {
            return cmds[i].callback;
        }
    }
    return null;
}

int main(int argc, char **argv) {
    builtin_callback builtin_cb;

    while (true) {
        printf("%s$: ", get_cwd());

        char *cmd = read_cmd();

        struct builtin_cmd_parse parse;

        if (strlen(cmd)>0) {
            parse_cmd(cmd, &parse);
            if (builtin_cb = is_builtin(parse.args[0])) {
                builtin_cb(&parse);
            } else {
                // printf("%s\n", parse.args[0]);
                int pid = sys_spawn(parse.args[0], parse.cnt, parse.args);
                // sys_wait(pid);
                //printf("command not found\n");
            }
            free_parse(&parse);
        }

        // int pid = sys_fork();
        // if (pid == 0) {
        //     printf("I am child!\n");
        //     sys_execve("/bin/test1", null, null);
        // }

        // printf("I am parent!\n");
        // sys_wait(pid);
    }
}