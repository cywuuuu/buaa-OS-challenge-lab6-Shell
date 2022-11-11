#include "lib.h"


char *msg = "Now is the time for all good men to come to the aid of their party.";

void
umain(void)
{
	char buf[100];
	int i, pid, p[2];

	if ((i=pipe(p)) < 0)
		user_panic("pipe: %e", i);

	if ((pid=fork()) < 0)
		user_panic("fork: %e", i);

	if (pid == 0) {
		writef("%x: cyw check here?0\n", env->env_id);//
		writef("[%08x] pipereadeof close %d\n", env->env_id, p[1]);
		close(p[1]);
		writef("[%08x] pipereadeof readn %d\n", env->env_id, p[0]);
		i = readn(p[0], buf, sizeof buf-1);
		if (i < 0)
			user_panic("read: %e", i);
		buf[i] = 0;
		writef("%x:read %s\n", env->env_id, buf); //
		if (strcmp(buf, msg) == 0)
			writef("\npipe read closed properly\n");
		else
			writef("\ngot %d bytes: %s\n", i, buf);
		exit();
	} else {
		writef("[%08x] pipereadeof close %d\n", env->env_id, p[0]);
		close(p[0]);
		writef("[%08x] pipereadeof write %d\n", env->env_id, p[1]);
		if ((i=write(p[1], msg, strlen(msg))) != strlen(msg))
			user_panic("write: %e", i);
		writef("%x:pipe write done\n", env->env_id); 
		close(p[1]);
	}
	wait(pid);//father wait son until son runs out
	writef("%x: cyw check here?1\n", env->env_id);

	writef("new pipe! =================================\n"); 
	if ((i=pipe(p)) < 0)
		user_panic("pipe: %e", i);

	if ((pid=fork()) < 0)
		user_panic("fork: %e", i);
	struct Env *e = &envs[ENVX(pid)]; 
	writef("son status:%d %d", e->env_id == pid, e->env_status); 
	writef("%x: cyw check here?2\n", env->env_id); 
	if (pid == 0) {
		writef("are you ok?"); 
		close(p[0]);
		for(;;){
			writef(".");
			if(write(p[1], "x", 1) != 1)
				break;
		}
		writef("\npipe write closed properly\n");
	}

	writef("%x: cyw check here?3\n", env->env_id);
	close(p[0]);
	close(p[1]);
	writef("%x: cyw check here?3.5\n", env->env_id);
	wait(pid);
	writef("%x: cyw check here?4\n", env->env_id);
	writef("pipe tests passed\n");
}
