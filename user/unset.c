#include "lib.h"
#include "error.h"
int flag[256];


void
unset(char *name) {
    int r;
     int faid = syscall_getfaid(syscall_getfaid(syscall_getenvid())); 
	if ((r = syscall_var_set(faid, name, 0, 1, 0)) < 0) {
        if (r == -E_VAR_NOT_FOUND)fwritef(1, "Environment var %s Not Exists\n", name);
		else if (r == -E_VAR_RDONLY) fwritef(1, "Environment var %s Read Only\n", name);
        else fwritef(1, "Other Error code: %d\n", r);
		return;
    }
}

void
usage(void) {
    fwritef(1, "usage: unset [vars...]\n");
    exit();
}

void
umain(int argc, char **argv) {
    int i;
    if (argc == 0) {
        return;
    } else {
        for (i = 1; i < argc; i++)
            unset(argv[i]);
    }
}

