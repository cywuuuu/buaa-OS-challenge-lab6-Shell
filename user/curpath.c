#include "lib.h"
#include "mmu.h"
const char *CURPATH_KEY = "curpath";

void curpath_init(int envid, char *path) {
    int r;
	 
    // set op 0 declare with perm
	if ((r = syscall_var_set(envid, CURPATH_KEY, path, 0, 0)) < 0) user_panic("Init curpath failed: %u.", r);
	return ; 
}

int curpath_get(int envid, char *path) {
    int r;
	//op1 get one
	//writef("get from env : %d", envid); 
    if ((r = syscall_var_get(envid, CURPATH_KEY, path, 1)) < 0) return r;
	return 0; 
}

int curpath_set(int envid, char *path) {
    int r;
	// op0 declare with perm
    if ((r = syscall_var_set(envid, CURPATH_KEY, path, 0, 0)) < 0) return r;
	return r; 
}

int curpath_get_parent(int envid, char *path) {
    int r, i;
    if ((r = curpath_get(envid, path)) < 0) return r;
    if (strlen(path) == 1) return 0;
    for (i = strlen(path) - 2; path[i - 1] != '/'; i--);
    path[i] = 0;
	return r; 
}
