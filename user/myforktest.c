#include "lib.h"
void umain()
{
    int a = 0;
    int id = 0;

/*    if ((id = fork()) == 0) {
        writef("heyhey"); //
        if ((id = fork()) == 0) {
            a += 3;
                writef("\t\tthis is child2 :a:%d\n", a);
        }

        a += 2;

       	writef("\tthis is child :a:%d\n", a);
    }

    a++;

    writef("this is father: a:%d\n", a);
*/
	syscall_mem_alloc(3, 3, 3); 
}
