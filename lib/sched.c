#include <env.h>
#include <pmap.h>
#include <printf.h>

/*** exercise 3.14 ***/
#define cur_list env_sched_list[point]
#define nxt_list env_sched_list[!point]

void sched_yield(void) {
    static int count = 0; // remaining time slices of current env
    static int point = 0; // current env_sched_list index
    struct Env *nxt_env;

    if (--count <= 0 || curenv == NULL || curenv->env_status != ENV_RUNNABLE) {
        int has_nxt_env = 0;

        // find in cur_list
        while (!LIST_EMPTY(&cur_list)) {
            nxt_env = LIST_FIRST(&cur_list);
            if (nxt_env->env_status == ENV_RUNNABLE) {
                has_nxt_env = 1;
                break;
            }
            LIST_REMOVE(nxt_env, env_sched_link);
            LIST_INSERT_TAIL(&nxt_list, nxt_env, env_sched_link);
        }

        if (LIST_EMPTY(&cur_list)) point = !point;

        // find in nxt_list
        if (!has_nxt_env) {
            while (!LIST_EMPTY(&cur_list)) {
                nxt_env = LIST_FIRST(&cur_list);
                if (nxt_env->env_status == ENV_RUNNABLE) {
                    has_nxt_env = 1;
                    break;
                }
                LIST_REMOVE(nxt_env, env_sched_link);
                LIST_INSERT_TAIL(&nxt_list, nxt_env, env_sched_link);
            }

            if (LIST_EMPTY(&cur_list)) {
                panic("^^^^^No ENV Runnable!^^^^^");
            }
        }

        LIST_REMOVE(nxt_env, env_sched_link);
        LIST_INSERT_TAIL(&nxt_list, nxt_env, env_sched_link);
        count = nxt_env->env_pri;
		//printf("nxtenv id:%d\n", nxt_env->env_id); 

		env_run(nxt_env);
    } else {
        env_run(curenv);
    }


}

#undef cur_list
#undef nxt_list



//breakpoint add sched_yield
/* Overview:
 *  Implement simple round-robin scheduling.
 *
 *
 * Hints:
 *  1. The variable which is for counting should be defined as 'static'.
 *  2. Use variable 'env_sched_list', which is a pointer array.
 *  3. CANNOT use `return` statement!
 */
/*** exercise 3.15 ***/
/*
void sched_yield(void)
{
    static int count = 0; // remaining time slices of current env剩余执行时间
    static int point = 0; // current env_sched_list index
    static struct Env *cur = NULL;  
	 
	//	调用 sched_yield函数时，先判断当前时间片是否用完。如果用完，将其插入另一个进程调度链表的尾部。
	//	之后判断当前进程调度链表是否为空。如果为空，切换到另一个进程调度链表。
	 
	//cur作废的条件就是 1. 当前进程时间片为0 或者最开始嘛也没有cur==NULL(count也是0)
	//         		    2. 或者cur不是RUNNABLE
	// 代表了当前cur是废的，要换一个， 另请高明
	while (count<= 0 || cur->env_status != ENV_RUNNABLE) {
		//另请高明
		count = 0; //那必然， cur报废了， 重新计数
		if (cur){// +前提是有东西, 要不然咋移除嘞

		LIST_REMOVE(cur, env_sched_link);//报废的cur去另外队列等着去， 等我这队列空了，我再切换去你那里
		if (cur->env_status != ENV_FREE)
			LIST_INSERT_TAIL(&env_sched_list[point^1], cur, env_sched_link);
		}
		if (LIST_EMPTY(&env_sched_list[point])) point = point ^ 1; //此队列，换个地方请
		if (LIST_EMPTY(&env_sched_list[point])) continue; //另一个队列没有那算了, 不切换了
		cur = LIST_FIRST(&env_sched_list[point]); //ok, 我们取新的env
		if (cur) // +防止异常
		count = cur->env_pri; 
		//不算完， 我们重回while那里， 去看看你是不是真的RUNNABLE符合运行，还是又是一个废物
	}
	--count; 
	env_run(cur); 
}*/
/*
int getbits(int u, int low, int high); 
int setbits(int o, int u, int low, int high); 
int func1(int u) {
	return getbits(u, 8, 15); 
}
int func2(int u) {
	return getbits(u, 16, 23); 
}
int func3(int u) {
	return getbits(u, 24, 31); 
}
int get_pri(int u) {
	return getbits(u, 0, 7); 
}
int set_pri(int o, int pri) {
	setbits(o, pri, 0, 7); 
}
int getbits(int u, int low, int high) {
	return (u>>low) & (1 << (high - low) - 1); 
}
int setbits(int o, int u, int low, int high) {
	o = o | ((u & (1 << (high - low) - 1)) << low); 
}
void sched_yield(void) {
    static int count = 0; // remaining time slices of current env剩余执行时间
    static int point = 0; // current env_sched_list index
    static struct Env *cur = NULL;
	struct Env *item = cur;
	// while (cur->env_status != ENV_RUNNABLE) {
	printf("???"); 
	int value = 2;
	if (value > 0) {
		if (cur) {
       		if (cur->env_pri < value) {
				cur->env_pri = 0;
				printf("setting ENV: id-%d, pri-%d\n", cur->env_id, cur->env_pri);
			} else {
				cur->env_pri -= value;
			}
		}
	}
	int max_pri = 0;
	LIST_FOREACH(item, &env_sched_list[point], env_sched_link) {

		if (item->env_status != ENV_RUNNABLE) {
			continue;
		}

		if (item->env_pri >= max_pri) {
			cur = item;
			max_pri = item->env_pri;
		}
	}
	printf("running ENV: id-%d, pri-%d\n", cur->env_id, cur->env_pri); 
	env_run(cur);
}*/

