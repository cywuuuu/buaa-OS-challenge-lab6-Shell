#include "lib.h"
#include "sh.h"
void
umain(int argc, char **argv)
{
    int i, nflag;

    nflag = 0;
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        nflag = 1;
        argc--;
        argv++;
    }   
    for (i = 1; i < argc; i++) {
        if (i > 1)
            write(1, " ", 1); 
        if (argv[i][0] == '$') {//第一个参数
			int shellid = syscall_getfaid(syscall_getfaid(syscall_getenvid())); 
			char val[256]; 
			syscall_var_get(shellid, &argv[i][1], val, 1); 
			write(1, val, strlen(val)); 
		} else {
			write(1, argv[i], strlen(argv[i]));
		}
    }   
    if (!nflag)
        write(1, "\n", 1); 
}
