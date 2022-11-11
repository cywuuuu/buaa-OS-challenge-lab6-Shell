#include "lib.h"
int flag[256];

void mkdir(char *path, char *prefix) {
	char curpath[MAXPATHLEN] = {0};
	int id = syscall_getenvid(); 
	int shellid = syscall_getfaid(syscall_getfaid(id)); 	
	int r = curpath_get(shellid, curpath); 
	if (r < 0) user_panic("mkdir: Error: curpath get\n");
	if (path[0] == '/') {
		strcat(curpath, path); 
	} else {
		if (curpath[strlen(curpath) - 1] != '/')
            strcat(curpath, "/");
        strcat(curpath, path);
	}

	r = open(curpath, O_RDWR|O_MKDIR); 
	if (r < 0) user_panic("Directory %s created error\n", curpath); 
	fwritef(1, "%s created successfully\n", curpath);

}

void
usage(void) {
    fwritef(1, "usage: mkdir [-dFl] [file...]\n");
    exit();
}

void
umain(int argc, char **argv) {
    int i;
    ARGBEGIN
    {
        default:
            usage();
        case 'd':
        case 'F':
        case 'l':
            flag[(u_char) ARGC()]++;
        break;
    }
    ARGEND

    if (argc == 0) {
        return;
    } else {
        for (i = 0; i < argc; i++)
            mkdir(argv[i], argv[i]);
    }
}


