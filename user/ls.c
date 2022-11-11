#include "lib.h"
#include "sh.h"
int flag[256];

void lsdir(char*, char*);
void ls1(char*, u_int, u_int, char*);

void
ls(char *path, char *prefix)
{
	int r;
	struct Stat st;
	if ((r=stat(path, &st)) < 0)
		user_panic("stat %s: %e", path, r);
	if (st.st_isdir && !flag['d'])
		lsdir(path, prefix);
	else
		ls1(0, st.st_isdir, st.st_size, path);

}

void
lsdir(char *path, char *prefix)
{
	int fd, n;
	struct File f;

	if ((fd = open(path, O_RDONLY)) < 0)
		user_panic("open %s: %e", path, fd);
	while ((n = readn(fd, &f, sizeof f)) == sizeof f)
		if (f.f_name[0])
			ls1(prefix, f.f_type==FTYPE_DIR, f.f_size, f.f_name);
	if (n > 0)
		user_panic("short read in directory %s", path);
	if (n < 0)
		user_panic("error reading directory %s: %e", path, n);
}

void
ls1(char *prefix, u_int isdir, u_int size, char *name)
{
	char *sep;

	if(flag['l'])
		fwritef(1, "%11d %c ", size, isdir ? 'd' : '-');
	if(prefix) {
		if (prefix[0] && prefix[strlen(prefix)-1] != '/')
			sep = "/";
		else
			sep = "";
		fwritef(1, "%s%s", prefix, sep);
	}
	if (isdir) {
		fwritef(1, LIGHT_GREEN(%s), name); 
	}
	else fwritef(1, LIGHT_PURPLE(%s), name);
	if(flag['F'] && isdir)
		fwritef(1, LIGHT_GREEN(/));
	fwritef(1, " \t");
}

void
usage(void)
{
	fwritef(1, "usage: ls [-dFl] [file...]\n");
	exit();
}

void
umain(int argc, char **argv)
{
//	writef("wahtttttt\n"); 
	int i;
	int envid = syscall_getenvid(); 
	int faid = syscall_getfaid(syscall_getfaid(envid)); 
	//writef("\n**running ls id:%d, shellid:%d\n", envid, faid); 
	ARGBEGIN{
	default:
		usage();
	case 'd':
	case 'F':
	case 'l':
		flag[(u_char)ARGC()]++;
		break;
	}ARGEND

	if (argc == 0){
		char path[256] = {0};   
        int faid = syscall_getfaid(syscall_getfaid(syscall_getenvid())); 
        curpath_get(faid, path); 
//        writef("ls at %s\n", path) ;
        ls(path, path);
	}
	else {
		
		for (i=0; i<argc; i++) {
			char path[256]; 
			int faid = syscall_getfaid(syscall_getfaid(syscall_getenvid())); 
			curpath_get(faid, path); 
			strcat(path, argv[i]); 
//			writef("ls at %s", path) ;
			ls(path, path);
		}
	}
}


