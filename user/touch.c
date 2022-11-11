#include "lib.h"
int flag[256];
void touch(char *path, char *prefix) {
	 int faid = syscall_getfaid(syscall_getfaid(syscall_getenvid()));  
	char curpath[MAXPATHLEN] = {0}; 
	int r = curpath_get(faid, curpath); 
	if (r < 0) {
		fwritef(1, "mkdir: Error: get curpath\n"); 
		return; 
	}
	if (path[0] == '/') {
		strcpy(curpath, path);  		
	}
	else {
		if (curpath[strlen(curpath) - 1] != '/')
			strcat(curpath, "/");
		strcat(curpath, path); 
	}
	r = open(curpath, O_CREAT|O_RDWR); 
	if (r < 0) {
		fwritef(1, "mkdir: Error: create file\n"); 
		return; 
	}
	fwritef(1, "File %s created!", curpath);
}

void
usage(void) {
    fwritef(1, "usage: touch [file...]\n");
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

	int faid = syscall_getfaid(syscall_getfaid(syscall_getenvid())); 
    char curpath[MAXPATHLEN];
    curpath_get(faid, curpath);
    if (argc == 0)
        return; 
    else {
        for (i = 0; i < argc; i++)
            touch(argv[i], argv[i]); 
			//ls(argv[i], argv[i]);
    }
}

