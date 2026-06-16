#include <libc/sysfunc.h>
#include <libc/string.h>
#include <libc/print.h>
#include <libc/file.h>


#define MAX_ARGS    16

struct cmd_parse {
    char *raw;
    char *args[MAX_ARGS];
    int   cnt;
};

typedef void (*builtin_fn)(struct cmd_parse *);

struct builtin {
    char      *name;
    builtin_fn fn;
};

static char cwd[256];
static char input_buf[256];
static char path_env[256] = "/bin";


static void env_parse(char **envp) {
    if (!envp) return;
    for (char **e = envp; *e; e++) {
        if (!strncmp(*e, "PATH=", 5)) {
            size_t len = strlen(*e + 5);
            if (len >= sizeof(path_env)) len = sizeof(path_env) - 1;
            memcpy(path_env, *e + 5, len);
            path_env[len] = '\0';
            break;
        }
    }
}

static const char *get_path(void) {
    return path_env;
}

static void resolve_path(const char *cmd, char *out, size_t outsz) {
    if (strchr(cmd, '/')) {
        memcpy(out, cmd, strlen(cmd) + 1);
        return;
    }

    const char *p = get_path();
    while (*p) {
        const char *end = strchr(p, ':');
        if (!end) end = p + strlen(p);

        size_t dirlen = (size_t)(end - p);
        if (dirlen + strlen(cmd) + 2 > outsz) goto next;

        memcpy(out, p, dirlen);
        out[dirlen] = '/';
        memcpy(out + dirlen + 1, cmd, strlen(cmd) + 1);

        FILE *fh = open(out, O_READ);
        if (fh) {
            close(fh);
            return;
        }

next:
        p = *end ? end + 1 : end;
    }

    snprintf(out, outsz, "/bin/%s", cmd);
}

static char *get_cwd(void) {
    memset(cwd, 0, sizeof(cwd));
    sys_getcwd(cwd, sizeof(cwd));
    return cwd;
}


static void builtin_pwd(struct cmd_parse *p) {
    (void)p;
    printf("%s\n", cwd);
}

static void builtin_cd(struct cmd_parse *p) {
    if (p->cnt < 2) return;
    sys_chdir(p->args[1]);
    get_cwd();
}

static void builtin_ls(struct cmd_parse *p) {
    (void)p;
    DIR *dp = opendir(cwd);
    if (!dp) {
        dprintf(STDERR, "ls: cannot open '%s'\n", cwd);
        return;
    }
    struct dirent *entries = readdir(dp);
    if (entries) {
        for (int i = 0; i < dp->cnt; i++)
            printf("%s  ", entries[i].name);
        printf("\n");
    }
    closedir(dp);
}

static void builtin_help(struct cmd_parse *p) {
    (void)p;
    printf("builtins: cd  ls  pwd  echo  help\n");
}

static void builtin_echo(struct cmd_parse *p) {
    for (int i = 1; i < p->cnt; i++)
        printf("%s ", p->args[i]);
    printf("\n");
}

static const struct builtin builtins[] = {
    {"cd",   builtin_cd},
    {"ls",   builtin_ls},
    {"pwd",  builtin_pwd},
    {"echo", builtin_echo},
    {"help", builtin_help},
};

static builtin_fn find_builtin(const char *name) {
    size_t n = sizeof(builtins) / sizeof(builtins[0]);
    for (size_t i = 0; i < n; i++)
        if (strcmp(builtins[i].name, name) == 0)
            return builtins[i].fn;
    return null;
}


static void parse_free(struct cmd_parse *p) {
    sys_vfree(p->raw);
    for (int i = 0; i < p->cnt; i++)
        sys_vfree(p->args[i]);
    memset(p, 0, sizeof(*p));
}

static void parse_cmd(const char *s, struct cmd_parse *p) {
    memset(p, 0, sizeof(*p));

    size_t slen = strlen(s);
    if (!slen) return;

    p->raw = sys_vmalloc(null, slen + 1);
    memset(p->raw, 0, slen + 1);
    memcpy(p->raw, s, slen);

    char *tok = p->raw;
    for (size_t i = 0; i <= slen; i++) {
        if (p->raw[i] == ' ' || p->raw[i] == '\0') {
            p->raw[i] = '\0';
            size_t tlen = strlen(tok);
            if (tlen > 0) {
                p->args[p->cnt] = sys_vmalloc(null, tlen + 1);
                memset(p->args[p->cnt], 0, tlen + 1);
                memcpy(p->args[p->cnt], tok, tlen);
                p->cnt++;
                if (p->cnt >= MAX_ARGS) break;
            }
            tok = &p->raw[i + 1];
        }
    }
}

static char *read_line(void) {
    memset(input_buf, 0, sizeof(input_buf));
    scanf("%s", input_buf);
    return input_buf;
}

static void run_external(struct cmd_parse *p) {
    char path[256];
    resolve_path(p->args[0], path, sizeof(path));

    u32 pid = sys_fork();
    if (pid == 0) {
        sys_execve(path, p->cnt, p->args);
        dprintf(STDERR, "exec '%s' failed\n", path);
        sys_user_exit();
    }
    sys_wait(pid);
}


int main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv;
    env_parse(envp);
    struct cmd_parse parse;

    while (1) {
        printf("%s$: ", get_cwd());

        char *line = read_line();
        if (!strlen(line)) continue;

        parse_cmd(line, &parse);
        if (!parse.cnt) continue;

        builtin_fn fn = find_builtin(parse.args[0]);
        if (fn) {
            fn(&parse);
        } else {
            run_external(&parse);
        }

        parse_free(&parse);
    }
}
