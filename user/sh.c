#include "sh.h"
#include "lib.h"
#include <args.h>

int debug_ = 0;

void his_init()
{
    int r;
    if ((r = open("/.history", O_CREAT | O_RDWR)) < 0)
        user_panic("His : Init Failed: %d.", r);
}

void his_save(char *s)
{
    int r;
    if ((r = open("/.history", O_CREAT | O_RDWR | O_APPEND)) < 0)
    {
        user_panic("His : Open failed");
    }
    fwritef(r, s);
    fwritef(r, "\n");
    close(r);
}

int his_read(char cmd[][128])
{
    int r, fd;
    char buf[128 * 128];
    if ((fd = open("/.history", O_RDONLY)) < 0)
    {
        user_panic("His : Open failed");
    }
    if ((r = read(fd, buf, sizeof(buf))) < 0)
    {
        user_panic("His : Read failed");
    }
    close(fd);

    int i = 0, ii = 0;
    while (buf[i])
    {
        int jj = 0;
        while (buf[i] && buf[i] != '\n')
            cmd[ii][jj++] = buf[i++];
        if (!buf[i])
            break;
        ++i;
        ++ii;
    }
    return ii;
}

//
// get the next token from string s
// set *p1 to the beginning of the token and
// *p2 just past the token.
// return:
//	0 for end-of-string
//	> for >
//	| for |
//	w for a word
//
// eventually (once we parse the space where the nul will go),
// words get nul-terminated.
#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

int _gettoken(char *s, char **p1, char **p2)
{
    int t;

    if (*s == 0)
        return 0;
    *p1 = 0;
    *p2 = 0;

    while (strchr(WHITESPACE, *s))
        *s++ = 0;
    if (*s == 0)
        return 0;

    if (*s == '\"') // easy 支持引号
    {
        *p1 = ++s;
        while (*s && !(*s == '\"' && *(s - 1) != '\\'))
        { // 一旦读到到 ", 且不是 \" 就停止, 其余跳过
            ++s;
        }
        *s++ = 0;
        *p2 = s;
        return 'w';
    }

    if (*s == '>' && *(s + 1) == '>')
    {
        *p1 = s;
        *s++ = 0;
        *s++ = 0;
        *p2 = s;
        return 'a';
    }

    if (strchr(SYMBOLS, *s))
    {
        t = *s;
        *p1 = s;
        *s++ = 0;
        *p2 = s;
        return t;
    }
    *p1 = s;
    while (*s && !strchr(WHITESPACE SYMBOLS, *s))
        s++;
    *p2 = s;
    if (debug_ > 1)
    {
        t = **p2;
        **p2 = 0;
        **p2 = t;
    }
    return 'w';
}

int gettoken(char *s, char **p1)
{
    static int c, nc;
    static char *np1, *np2;
    // 如果有s
    if (s)
    {
        nc = _gettoken(s, &np1, &np2);
        return 0;
    }
    c = nc; // type
    *p1 = np1;
    nc = _gettoken(np2, &np1, &np2);
    return c;
}

#define MAXARGS 16

void runcmd(char *s)
{
    his_save(s); // s is the command
    char *argv[MAXARGS], *t;
    int argc, c, i, r, p[2], fd, rightpipe;
    int fdnum, hang = 0, pid, shellid;
    int input = -1, output = -1;
    rightpipe = 0;
    gettoken(s, 0); //初始化s
again:
    argc = 0;
    for (;;)
    {
        c = gettoken(0, &t);
        switch (c)
        {
        case 0:
            goto runit; //结束了可以润
        case 'w':
            if (argc == MAXARGS)
            {
                writef("too many arguments\n");
                exit();
            }
            argv[argc++] = t;
            break;
        case '<':
            if (gettoken(0, &t) != 'w')
            {
                writef("syntax error: < not followed by word\n");
                exit();
            }
            if ((r = open(t, O_RDONLY)) < 0)
                user_panic("< open failed");
            fd = r;
            input = fd;
            break;
        case '>':
            if (gettoken(0, &t) != 'w')
            {
                writef("syntax error: > not followed by word\n");
                exit();
            }
            if ((r = open(t, O_WRONLY | O_CREAT)) < 0)
                user_panic("> open failed");
            fd = r;
            output = fd;
            break;
        case '|':
            pipe(p);
            if ((rightpipe = fork()) == 0)
            {
                input = p[0];
                close(p[1]);
                goto again;
            }
            else
            {
                output = p[1];
                close(p[0]);
                goto runit;
            }
            break;
        case '&':
            hang = 1;
            break;
        case ';':
            
            if ((pid = fork()) == 0) //子进程
            {
				//hang = 0; 
				//input = -1;  //
                //output = -1;  //
                goto runit;  // runit
            }
			wait(pid); 
			input = -1; 
			output = -1; 
			argc = 0;
            hang = 0;
            argc = 0;
            rightpipe = 0; //父进程跳出switch， 继续解析下一个
            break;
		}

    }

runit:
    if (input != -1)
    {
        dup(input, 0);
        close(input);
    }
    if (output != -1)
    {
        dup(output, 1);
        close(output);
    }
    if (argc == 0)
    {
        if (debug_)
            writef("EMPTY COMMAND\n");
        return;
    }

    argv[argc] = 0;

    char prog_name[64];
    int prog_name_len = strlen(argv[0]);
    strcpy(prog_name, argv[0]);
    strcat(prog_name, ".b");
    prog_name_len += 2;
    prog_name[prog_name_len] = '\0';

    // spawn
    int child_envid;
    if ((child_envid = spawn(prog_name, argv)) < 0)
    {
        writef("Super Shell: command not found " RED([% s]) "\n", prog_name);
    }
    int idd = syscall_getenvid();
    int faid = syscall_getfaid(idd);
    if (hang)
    {
        for (i = 0; i < argc; ++i)
            writef("%s ", argv[i]);
        writef("\n");
    }

    if ((r = syscall_set_env_status(child_envid, ENV_RUNNABLE)) < 0)
    {
        writef("set child runnable is wrong\n");
    }
    r = child_envid;
    // -- spawn
    close_all();
    if (r >= 0)
    {
        if (!hang)
            wait(r); //等子进程结束
        else
        {
            pid = fork();
            if (pid == 0) //创建cmd子进程
            {
                wait(r); //等待新加载的程序运行完
                writef("\n[%d] DONE\t", r);
                for (i = 0; i < argc; ++i)
                    writef("%s ", argv[i]);
				  char curpath[MAXPATHLEN];
                 int envid = syscall_getenvid();
                 int shellid = syscall_getfaid(envid);
                 curpath_get(shellid, curpath);
                 fwritef(1, "\n" RED(% s) ":" CYAN($) " ", curpath);
				 writef("\b \b");
					exit();
                    
                
            }
        }
    }
    if (rightpipe)
    {
        if (debug_)
            writef("[%08x] WAIT right-pipe %08x\n", env->env_id, rightpipe);
        wait(rightpipe);
    }

    exit();
}

void readline(char *buf, u_int n)
{
    int i, r, ii;
    char his[128][128];
    int nn = ii = his_read(his);

    r = 0;
    for (i = 0; i < n; i++)
    {
        if ((r = read(0, buf + i, 1)) != 1)
        {
            if (r < 0)
            exit();
        }

        if (buf[i] == '\r' || buf[i] == '\n')
        {
            buf[i] = 0;
            return;
        }

        if (buf[i] == 127)
        {
            if (i > 0)
            {
                buf[i] = 0;
                int j;
                for (j = 0; j < strlen(buf); j++)
                    writef("\b \b"); //
                buf[i - 1] = 0;
                buf = strcat(buf, &buf[i]);
                writef("%s", buf);
                i -= 2;
            }
            else
            {
                buf[i] = 0;
                i -= 1;
            }
        }

        if (i >= 2 && buf[i - 2] == 27 && buf[i - 1] == 91 && buf[i] == 65)
        {
            writef("%c%c%c", 27, 91, 66);
            for (i -= 2; i; --i)
                writef("\b \b");

            if (ii)
                strcpy(buf, his[--ii]);
            else
                strcpy(buf, his[ii]);

            writef("%s", buf);
            i = strlen(buf) - 1;
        }
        else if (i >= 2 && buf[i - 2] == 27 && buf[i - 1] == 91 && buf[i] == 66)
        {

            if (ii < nn - 1)
            {
                for (i -= 2; i; --i)
                    writef("\b \b");
                strcpy(buf, his[++ii]);
                writef("%s", buf);
            }
            else
            {
                buf[i - 2] = buf[i - 1] = buf[i] = '\0';
            }

            i = strlen(buf) - 1;
        }
    }
    writef("line too long\n");
    while ((r = read(0, buf, 1)) == 1 && buf[0] != '\n')
        ;
    buf[0] = 0;
}

void usage(void)
{
    writef("usage: sh [-dix] [command-file]\n");
    exit();
}

char buf[1024];

void umain(int argc, char **argv)
{
    int r, interactive, echocmds;
    interactive = '?';
    echocmds = 0;
    writef("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    writef("::                                                         ::\n");
    writef("::              " LIGHT_BLUE(Super Shell V0 .0.0) "                         ::\n");
    writef("::                                                         ::\n");
    writef(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    ARGBEGIN
    {
    case 'd':
        debug_++;
        break;
    case 'i':
        interactive = 1;
        break;
    case 'x':
        echocmds = 1;
        break;
    default:
        usage();
    }
    ARGEND

    if (argc > 1)
        usage();
    if (argc == 1)
    {
        close(0);
        if ((r = open(argv[1], O_RDONLY)) < 0)
            user_panic("open %s: %e", r);
        user_assert(r == 0);
    }
    if (interactive == '?')
        interactive = iscons(0);
    int envid = syscall_getenvid();
    his_init();
    curpath_init(envid, "/");

    for (;;)
    {
        char curpath[MAXPATHLEN];
        curpath_get(envid, curpath);
        if (interactive)
            fwritef(1, "\n" RED(%s) ":" CYAN($) " ", curpath);
        readline(buf, sizeof buf);

        if (buf[0] == '#')
            continue;
        if (echocmds)
            fwritef(1, "# %s\n", buf);
        if ((r = fork()) < 0)
            user_panic("fork: %e", r);
        if (r == 0)
        {
            runcmd(buf);
            exit();
            return;
        }
        else
            wait(r);
    }
}

