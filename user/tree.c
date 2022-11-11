#include "lib.h"
#include "sh.h"
void walk(char *path, int len) {
	int dir_fd = open(path, O_RDONLY); 
	int i; 
	if (dir_fd < 0) user_panic("failed: open %s: %e", path, dir_fd);  
	struct File f;
	while (readn(dir_fd, &f, sizeof (struct File)) == sizeof (struct File)) {
		
		if (f.f_name[0] != '\0') {
			struct File *dir = &f; // get FCB for each dir
			
			fwritef(1, "\n");	
			for (i = 0; i < len; i++) {
				fwritef(1, "   "); 
			}
			
			fwritef(1, "|-- ");  
			
			if (dir->f_type == FTYPE_REG) {
				fwritef(1, PURPLE(%s), dir->f_name); 
			} else if (dir->f_type == FTYPE_DIR) {
				fwritef(1, LIGHT_GREEN(%s), dir->f_name); 
				char npath[MAXPATHLEN] = {0}; 
				strcat(npath, path); 
				strcat(npath, dir->f_name); 
				strcat(npath, "/"); 
				walk(npath, len + 1);
			}

		}
	}
}

int flag[256];
void
tree(char *path, char *prefix) {    
    struct Stat statt;
    int r = stat(path, &statt); 
	if (r < 0) user_panic("stat %s: %e", path, r);
    walk(path, 0);
        //ls1(0, st.st_isdir, st.st_size, path);    
}

void
usage(void) {
    fwritef(1, "usage: tree [-dFl] [file...]\n");
    exit();
}

void
umain(int argc, char **argv) {
    int i;
    char curpath[MAXPATHLEN];
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
    //syscall_curpath(curpath, 0);
    if (argc == 0) {
        //tree(curpath, "");
        char path[256] = {0};   
        int faid = syscall_getfaid(syscall_getfaid(syscall_getenvid())); 
        curpath_get(faid, path); 
		tree(path, "");
    } else {
        for (i = 0; i < argc; i++)
            tree(argv[i], argv[i]);
    }

}
