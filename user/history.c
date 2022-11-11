#include "lib.h"
#include "sh.h"
int flag[256]; 
void
history() {                                                                                                          
    
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
	writef("his cmds:\n"); 
    writef(YELLOW(%s), buf); 

}

void
usage(void) {
    fwritef(1, "usage: history \n");
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
        history(); 
    } else {
        writef("too many args\n"); 
    }
}
