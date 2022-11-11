#include "lib.h"
#include "error.h"

void usage(void)
{
    fwritef(1, "usage: cd [directory]\n");
    exit();
}

void umain(int argc, char **argv)
{
    int i, r;
    int path[256];
    int faid = syscall_getfaid(syscall_getfaid(syscall_getenvid()));
    if (argc == 1)
    {
        fwritef(1, "cd: too few args\n");
        return;
    }
    else
    {
        for (i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], ".") == 0)
                return;

            if (strcmp(argv[i], "..") == 0)
            {
                curpath_get_parent(faid, path);
                curpath_set(faid, path);
                fwritef(1, "cd: %s", path);
                return;
            }
            else
            {

                if ((r = curpath_get(faid, path)) < 0)
                {
                    fwritef(1, "cd: get environment var failed");
                    return;
                }
                strcat(path, argv[i]);
                int len = strlen(path);
                if (path[len - 1] != '/')
                    strcat(path, "/");

                struct Stat st;
                r = stat(path, &st);

                if (r == -E_VAR_NOT_FOUND)
                    fwritef(1, "cd: %s not found\n", path);
                else if (r < 0)
                    fwritef(1, "cd: cannot cd %s\n", path);
                else if (!st.st_isdir)
                    fwritef(1, "cd: %s is not directory\n", path);
                else
                {
                    if ((r = curpath_set(faid, path)) < 0)
                        fwritef(1, "Environment var not found");
                    fwritef(1, "curpath: %s", path);
                }
                return;
            }
        }
    }
}

