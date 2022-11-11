#include "lib.h"
#include "error.h"
int flag[256];


void
set(char *name, char *value, int perm) {
    int r;
     int faid = syscall_getfaid(syscall_getfaid(syscall_getenvid())); 
	if ((r = syscall_var_set(faid, name, value, 0, perm)) < 0) {
       	if (r == -E_VAR_FULL) fwritef(1, "declare: no var available\n", name);
        if (r == -E_VAR_RDONLY) fwritef(1, "declare: [%s] is readonly\n", name);
        return;
    }
}

void
usage(void) {
    fwritef(1, "usage: declare [-xr] [NAME [=VALUE]]\n");
    exit();
}
void declare_nothing() {
	 int faid = syscall_getfaid(syscall_getfaid(syscall_getenvid())); 
	int r; 
	char strlist[4096]; 
	if ( r = syscall_var_get(faid, NULL, strlist, 0) < 0) {
		if (r == E_VAR_NOT_FOUND) fwritef(1, "declare: no vars\n");
		return; 
	}
//	fwritef(1, "Vars Are:\n"); 
//	fwritef(1, "%s\n", strlist);
	
}
void
umain(int argc, char **argv) {
    int i;
    ARGBEGIN
    {
        default:
            usage();
        case 'r':
            flag[(u_char) ARGC()]++;
		case 'x':
			flag[(u_char) ARGC()]++;
        break;
    }
    ARGEND

	int perm = 0; 
	if (flag['r']) {
		perm |= VAR_RDONLY; 
	} 
	if (flag['x']) {
		perm |= VAR_LOCAL; 
	}
	char *arg2 = argv[1]; 
	while(*arg2 == '=') {
		arg2++; 
	}
	
    if (argc == 0) declare_nothing(); 
	else if (argc == 1) set(argv[0], "", perm);
    else if (argc == 2) set(argv[0], arg2, perm);
  	else writef(1, "too many args");
	//writef("declare ok**"); 
}

