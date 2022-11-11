#include "mmu.h"
#include "pmap.h"
#include "printf.h"
#include "env.h"
#include "error.h"


/* These variables are set by mips_detect_memory() */
u_long maxpa;            /* Maximum physical address 64M*/
u_long npage;            /* Amount of memory(in pages) */
u_long basemem;          /* Amount of base memory(in bytes) */
u_long extmem;           /* Amount of extended memory(in bytes) */

Pde *boot_pgdir;
struct Page *pages;
static u_long freemem;

////////////////////////////////////////////////////
static void *alloc(u_int n, u_int align, int clear)
{
	extern char end[];
	u_long alloced_mem;
		// . = 0x80400000;
    // end = . ;
	/**
	 * end数组的初始地址在 8040 0000
	 * 虚拟地址 0x80400000，根据映射规则，对应的物理地址是0x00400000，
	 * 因此我们所管理的物理地址区间为 [0x00400000, maxpa − 1]
	 * 物理地址区间和虚拟地址区间 [0x80400000, 0x83ffffff] 一一对应，
	 * 因此当虚拟地址被分配出去，代表对应的物理地址也被分配了出去
	*/
	/* Initialize `freemem` if this is the first time. The first virtual address that the
	 * linker did *not* assign to any kernel code or global variables. */
	if (freemem == 0) {
		freemem = (u_long)end;
	}

	freemem = ROUND(freemem, align);
	// allocedmem是分配这段mem的首地址
	// freemem是分配段的末地址
	alloced_mem = freemem;

	freemem = freemem + n;
	//printf("allocating\n"); //
	/* Check if we're out of memory. If we are, PANIC !! */
	if (PADDR(freemem) >= maxpa) {
		panic("out of memory\n");
		return (void *)-E_NO_MEM;
	}
	// kseg0区 kernel不用通过页表整, kseg1 3g-4g 也是 是直接转换到外设的
	// PADDR(va) 使用前提就是虚拟地址x在va kseg0区8000-9ffff（2g + 0-1g）内，内核区， 
	// 我们最初需要的空间 就是内核区
	// 受限于内存64M,内核本身也不大，肯定在kseg0哈, 但很有可能你申请超过了64M
	// 下面就会报错
	// 8000.. +xxx =  freemem -> 000 + xxx 
	/* Step 4: Clear allocated chunk if parameter `clear` is set. */
	
	if (clear) {
		//printf("clear?"); //
		bzero((void *)alloced_mem, n);
	}

	/* Step 5: return allocated chunk. */
	return (void *)alloced_mem;
}
struct node
{
	int isDistr; 
	int l; 
	int r; 
	int lpa; 
	int rpa; 
	struct node *left; 
	struct node *right; 
};
struct node *tree; 

struct node *build_tree(int l, int r, int lpa, int rpa, int depth) {
	if(depth == 0) return NULL; 
	depth--;
	printf("what s wrong?"); 
	struct node *tree = (struct node *)alloc(sizeof(struct node), 1, 1);
	printf("%d, %x : %x\n", depth, l, r); 
	tree->l = l; 
	tree->r = r-1; 
	tree->lpa = lpa; 
	tree->rpa = rpa; 
	tree->isDistr = 0; 
	tree->left = build_tree(l, (l+r)/2, 0, 0, depth); 
	tree->right =  build_tree((l+r)/2, r, 0, 0, depth);
}
int get_distr(int l, int r, struct node* tree) {
	int y = tree->r - tree->l;// suppply 
	int x = r - l;//demand 
	printf("[%x:%x] [%x:%x]", tree->r, tree->l, r, l); 
	if (y < x || y < 4*1024) {
		return -1; 
	}
	if (y/2 < x || y == 4*1024) {
		tree->isDistr = 1; 
		tree->lpa = l; 
		tree->rpa = r; 
	} else {
		if(tree->left == NULL)
		tree->left = build_tree(tree->l, (tree->l + tree->r)/2, 0, 0, 1); 
		if(tree->left == NULL)
		tree->right = build_tree((tree->l + tree->r)/2, tree->r, 0, 0, 1); 
		return get_distr(l, r, tree->left); 
	}
}
struct node* find_and_cancel(int pa, struct node* tree) {
	int y = tree->r - tree->l;// suppply 
	
	if ((pa >= tree->lpa && pa <= tree->rpa) || y == 4*1024) {
		tree->isDistr = 0; 
		return tree; 
	} 
	tree->left = find_and_cancel(pa, tree->left); 
	tree->right = find_and_cancel(pa, tree->right); 
	if ((tree->left)->isDistr == 0 && (tree->right)->isDistr == 0) {
		tree->isDistr = 0; 
		tree->left = NULL; 
		tree->right = NULL; 
	} 
	return tree; 

}
int cal_page(int taskKind, u_long va, int n, Pde *pgdir) {
	if (taskKind == 0) {
		return 39; 
	} else if (taskKind == 1) {
		return va + ((va>>22) << 12); 
	} else if (taskKind == 2) {
		(va>>22)<<22 + (n<<12);  
	} else if (taskKind == 3) {
		Pde *pg_dir_entry = pgdir + PDX(va);
		*pg_dir_entry = PADDR(va) |PTE_R | PTE_V; 

	}
}
void count_page(Pde *pgdir, int *cnt,int size) {
	cnt[page2ppn(pgdir)]++; 
	int i = 0; 
	for(i = 0; i < 1024; i++) {
		Pde* pgdir_entry = pgdir + i; 
		if((*pgdir_entry & PTE_V) != 0) {
			Pte *pgtableBase = KADDR(PTE_ADDR(*pgdir_entry)); 
			cnt[page2ppn(pgtableBase)]++; 
			int j = 0; 
			for (j = 0; j < 1024; j++) {
				Pte *pgtable_entry = pgtableBase + j; 
				if ((*pgtable_entry & PTE_V) != 0) {
					cnt[page2ppn(KADDR(PTE_ADDR(*pgtable_entry)))]++; 
				}
			}
		}

	}

}
////////////////////////////////////////////////////
static struct Page_list page_free_list;	/* Free list of physical pages */
unsigned int page_bitmap[512]; 

/* Exercise 2.1 */
/* Overview:
   Initialize basemem and npage.
   Set basemem to be 64MB, and calculate corresponding npage value.*/
void mips_detect_memory()
{
	/* Step 1: Initialize basemem.
	 * (When use real computer, CMOS tells us how many kilobytes there are). */
	maxpa = 0x4000000; //
	npage = maxpa >> PGSHIFT;
	basemem = maxpa; 
	extmem = 0; 

	// Step 2: Calculate corresponding npage value.

	printf("Physical memory: %dK available, ", (int)(maxpa / 1024));
	printf("base = %dK, extended = %dK\n", (int)(basemem / 1024),
			(int)(extmem / 1024));
}
int times = 0; 
void get_page_status(int pa) {
	struct Page *outpage = pa2page(pa);
	int status = 0; 
	struct Page *list_item = 0; 
	if (outpage->pp_ref > 0) status = 1; 
	else if (outpage->pp_ref == 0) {
		status = 2; 
		LIST_FOREACH(list_item, &page_free_list, pp_link) {
			if(list_item == outpage) {
				status = 3; 
				break; 
			}
		}
	}
	times++; 
	printf("times:%d, page status:%d\n", times , status); 

}
/* Overview:
   Allocate `n` bytes physical memory with alignment `align`, if `clear` is set, clear the
   allocated memory.
   This allocator is used only while setting up virtual memory system.

   Post-Condition:
   If we're out of memory, should panic, else return this address of memory we have allocated.*/

/* Exercise 2.6 */
/* Overview:
   Get the page table entry for virtual address `va` in the given
   page directory `pgdir`.
   If the page table is not exist and the parameter `create` is set to 1,
   then create it.*/
/*
返回一级页表基地址 pgdir 对应的两级页表结构中，va 这个虚拟地址所在的二级页表项的虚地址
*/
static Pte *boot_pgdir_walk(Pde *pgdir, u_long va, int create)
{
// va->页目录项在页目录中的偏移(虚)->为页表申请内存(虚地址)将其转化为实地址存入页目录项
// 页目录项提纯->页表的实地址->KADDR 页表的虚地址 -> + PTX(va) 页表项虚地址
	Pde *pgdir_entry = pgdir + PDX(va); //虚地址
    // check whether the page table exists
	Pte *test = 0; 
    if ((*pgdir_entry & PTE_V) == 0) {
        if (create) {
            /**
            * use `alloc` to allocate a page for the page table
            * set permission: `PTE_V | PTE_R`
            * hint: `PTE_V` <==> valid ; `PTE_R` <==> writable
            */
			test = alloc(BY2PG, BY2PG, 1); 
			*pgdir_entry = PADDR(test); 
			//printf("test:%x\n", test);//
			//physical address , 页表项里存入的都是实地址
			// of pgtable head [PFN of pgtable, flags]
			*pgdir_entry = (*pgdir_entry) | PTE_V | PTE_R;
        } else return 0; // exception
    }
	
	Pte *pgtable_entry = (Pte*) KADDR(PTE_ADDR(*pgdir_entry)); //[PFN of pgtable, 0s] tells headaddr of pgtab
	//二级页表表头实地址
	//对应的虚地址
    // return the address of entry of page table
    return ((Pte *)/* II. Kernel Virtual Address of PTBase */ pgtable_entry) + PTX(va);

}

/* Exercise 2.7 */
/*Overview:
  Map [va, va+size) of virtual address space to physical [pa, pa+size) in the page
  table rooted at pgdir.
  Use permission bits `perm | PTE_V` for the entries.
  Pre-Condition:
  Size is a multiple of BY2PG.*/
void boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm)
{
	/* Step 1: Check if `size` is a multiple of BY2PG. */
	/* Step 2: Map virtual address space to physical address. */
	/* Hint: Use `boot_pgdir_walk` to get the page table entry of virtual address `va`. */
    int i;
    Pte *pgtable_entry;
	//将虚拟地址区间 [va, va + size − 1] 映射到物理地址区间 [pa, pa + size − 1]
	//实质就是在创建并遍历一个二级页表，并填写对应的物理页框号PTE_ADDR(pa+i)
    for (i = 0, size = ROUND(size, BY2PG); i < size; i += BY2PG) {
        /* Step 1. use `boot_pgdir_walk` to "walk" the page directory */
        pgtable_entry = boot_pgdir_walk( 
            pgdir,
            va + i,
            1 /* create if entry of page directory not exists yet */
        );//虚地址实地址涨一页， 页表项才往后跳一个,页表项记录的是
        /* Step 2. fill in the page table */
        *pgtable_entry = (/* III. Physical Frame Address of `pa + i` */ PTE_ADDR(pa+i))
                        | perm | PTE_V;
		//printf("boot_map_seg_i:%d\n", i);// 
    }

}

/* Overview:
   Set up two-level page table.

Hint:
You can get more details about `UPAGES` and `UENVS` in include/mmu.h. */
void mips_vm_init()
{
	extern char end[];
	extern int mCONTEXT;
	extern struct Env *envs;

	Pde *pgdir;
	u_int n;

	/* Step 1: Allocate a page for page directory(first level page table). */
	pgdir = alloc(BY2PG, BY2PG, 1);
	printf("to memory %x for struct page directory.\n", freemem);
	mCONTEXT = (int)pgdir;

	boot_pgdir = pgdir;

	/* Step 2: Allocate proper size of physical memory for global array `pages`,
	 * for physical memory management. Then, map virtual address `UPAGES` to
	 * physical address `pages` allocated before. In consideration of alignment,
	 * you should round up the memory size before map. */
	/**
	* 我们用把管理内存的page数组的空间申请一下
	*/

	pages = (struct Page *)alloc(npage * sizeof(struct Page), BY2PG, 1);
	printf("to memory %x for struct Pages.\n", freemem);
	n = ROUND(npage * sizeof(struct Page), BY2PG);
	//UPAGE, UPAGE + n -> pages结构体数组 虚拟地址分配给UPAGE，映射
	boot_map_segment(pgdir, UPAGES, n, PADDR(pages), PTE_R);

	/* Step 3, Allocate proper size of physical memory for global array `envs`,
	 * for process management. Then map the physical address to `UENVS`. */
	envs = (struct Env *)alloc(NENV * sizeof(struct Env), BY2PG, 1);
	n = ROUND(NENV * sizeof(struct Env), BY2PG);
	boot_map_segment(pgdir, UENVS, n, PADDR(envs), PTE_R);

	printf("pmap.c:\t mips vm init success\n");
}

/* Exercise 2.3 */
/*Overview:
  Initialize page structure and memory free list.
  The `pages` array has one `struct Page` entry per physical page. Pages
  are reference counted, and free pages are kept on a linked list.
page_init()，在启动过程中提到过，它实现了以下三个功能：
首先利用链表相关宏初始化 page_free_list,空闲页表链。
接着将 mips_vm_init() 中用到的空间对应的物理页面的内存控制块的引用次数全部标为 1。
最后将剩下的物理页面的引用次数全部标为 0，并将它们对应的内存控制块插入到 page_free_list。
Hint:
Use `LIST_INSERT_HEAD` to insert something to list.*/
void page_init(void)
{
	/* Step 1: Initialize page_free_list.(struct Page_list, a head [ Page hdr*]) */
	/* Hint: Use macro `LIST_INIT` defined in include/queue.h. */
	struct Page *item; 
	LIST_INIT(&page_free_list); 
	//所有链表宏 输入都是 地址，或者指针/ 还有就是field是域名 
	// Page 结构体视角下
	// struct Page {
	//  Page_LIST_entry_t pp_link;	/* free list link */
	//  u_short pp_ref;
	// }
	// &Page_free_list 就是指针head
	/* Step 2: Align `freemem` up to multiple of BY2PG. */
	ROUND(freemem, BY2PG); 
	/**
	 * freemem 的含义是 [0x80400000, freemem − 1] 的虚拟地址都已经被分配了
	 * 一开始freemem = end 地址在8040， 虚拟地址一个都没有分配
	 * 内核虚地址, 物理页号和kseg0区的地址是有一一对应的,  
	 * page <->(page2pa/pa2page)<-> pa <-> (PADDR(kva)/KADDR(pa))  <-> kva 
	 * page2kva可以直通，但是差不多
	*/
	
	/* Step 3: Mark all memory blow `freemem` as used(set `pp_ref`
	 * filed to 1) */
	int i = 0; 
	for ( ; page2kva(&pages[i]) < freemem ; i++ ) {
		pages[i].pp_ref = 1; 
	}
	
	/* Step 4: Mark the other memory as free. */
	for ( ; i < npage; i++) {
		if (i == PPN(PADDR(TIMESTACK-BY2PG))) continue;
		
		pages[i].pp_ref = 0; 
		
		LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link); 
	}
	
}

/* Exercise 2.4 */
/*Overview:
  Allocates a physical page from free memory, and clear this page.

  Post-Condition:
  If failing to allocate a new page(out of memory(there's no free page)),
  return -E_NO_MEM.
  Else, set the address of allocated page to *pp, and return 0.

Note:
DO NOT increment the reference count of the page - the caller must do
these if necessary (either explicitly or via page_insert).
page_alloc(struct Page **pp)，它的作用是将 page_free_list 空闲链表头部内存控制块
对应的物理页面分配出去，将其从空闲链表中移除，并清空对应的物理页面，最后将 pp 指向的
空间赋值为这个内存控制块的地址
#define LIST_EMPTY(head)        ((head)->lh_first == NULL) 
#define LIST_FIRST(head)        ((head)->lh_first)
#define LIST_INSERT_AFTER(listelm, elm, field)
#define LIST_INSERT_BEFORE(listelm, elm, field)
#define LIST_INSERT_HEAD(head, elm, field)
#define LIST_INSERT_TAIL(head, elm, field)
#define LIST_REMOVE(elm, field)
Hint:
Use LIST_FIRST and LIST_REMOVE defined in include/queue.h .*/
int page_alloc(struct Page **pp){
	struct Page *ppage_temp;
	struct Page *tmp; 
	/* Step 1: Get a page from free memory. If fail, return the error code.*/
	if (LIST_EMPTY(&page_free_list)) return -E_NO_MEM; 
	
	tmp = LIST_FIRST(&page_free_list);
	LIST_REMOVE(tmp, pp_link); 
	/* Step 2: Initialize this page.
	 * Hint: use `bzero`. */
	bzero(page2kva(tmp), BY2PG);//初始化 
	// 最后将pp赋值为内存控制块地址
	*pp = tmp;
	return 0; //别忘了return 0 
}
int page_alloc_diy(struct Page **pp){
    struct Page *ppage_temp;
    struct Page *tmp;
    /* Step 1: Get a page from free memory. If fail, return the error code.*/
    int i = 0; 
	for (i = 0; i < (npage); i++ ) {
		if((page_bitmap[i/32] & (1<<(i%32))) == 0) { //第i页有空， 分配好
			page_bitmap[i/32] = page_bitmap[i/32] | (1<<(i%32)); 
			break; 
		}
	}
	
	if (i == npage) return -E_NO_MEM;
    tmp = &pages[i]; 
	bzero(page2kva(tmp), BY2PG);//初始化 
    // 最后将pp赋值为内存控制块地址
    *pp = tmp;
    return 0; //别忘了return 0 
}
/* Exercise 2.5 */
/*Overview:
  Release a page, mark it as free if it's `pp_ref` reaches 0.
Hint:
When you free a page, just insert it to the page_free_list.*/
void page_free(struct Page *pp)
{
	/* Step 1: If there's still virtual address referring to this page, do nothing. */
	if (pp->pp_ref == 0) {
		// 插入 page_free_list中
		LIST_INSERT_HEAD(&page_free_list, pp, pp_link); 
		return; 
	} else if (pp->pp_ref > 0) { //仍在使用中
		return; 
	}
	
	/* Step 2: If the `pp_ref` reaches 0, mark this page as free and return. */

	/* If the value of `pp_ref` is less than 0, some error must occurr before,
	 * so PANIC !!! */
	panic("cgh:pp->pp_ref is less than zero\n");
}

/* Exercise 2.8 */
/*Overview:
  Given `pgdir`, a pointer to a page directory, pgdir_walk returns a pointer
  to the page table entry (with permission PTE_R|PTE_V) for virtual address 'va'.
  Pre-Condition:
  The `pgdir` should be two-level page table structure.

  Post-Condition:
  If we're out of memory, return -E_NO_MEM.
  Else, we get the page table entry successfully, store the value of page table
  entry to *ppte, and return 0, indicating success.
Hint:
We use a two-level pointer to store page table entry and return a state code to indicate
whether this function execute successfully or not.
This function has something in common with function `boot_pgdir_walk`.*/
int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte)
{

	/* Step 1: Get the corresponding page directory entry and page table. */
	/* Step 2: If the corresponding page table is not exist(valid) and parameter `create`
	 * is set, create one. And set the correct permission bits for this new page table.
	 * When creating new page table, maybe out of memory. */
	/* Step 3: Set the page table entry to `*ppte` as return value. */
    Pde *pgdir_entry = pgdir + PDX(va); 
	struct Page *page;
    int ret;
    
    // check whether the page table exists
    if ((*pgdir_entry & PTE_V) == 0) { // 不valid,即不存在该二级页表
        if (create) {
		// API explain: alloc 返回的是二级页表基虚地址, page_alloc直接给出修改Page指针
            if ((ret = page_alloc(&page)) < 0) return ret;//没有空闲free page了
            *pgdir_entry = (/* Physical Address of `page` */page2pa(page))
                          | PTE_V | PTE_R;
			page->pp_ref++;// 
        } else {
            *ppte = 0;
            return 0;
        }
    }
    *ppte = ((Pte *)(/*Kernel Virtual Address of PTBase */KADDR(PTE_ADDR(*pgdir_entry)))) + PTX(va);

	return 0;
}

/* Exercise 2.9 */
/*Overview:
  Map the physical page 'pp' at virtual address 'va'.
  The permissions (the low 12 bits) of the page table entry should be set to 'perm|PTE_V'.

  Post-Condition:
  Return 0 on success
  Return -E_NO_MEM, if page table couldn't be allocated
这个函数的作用是将一级页表基地址 pgdir 对应的两级页表结构中 va 这一虚拟地址
映射到内存控制块 pp 对应的物理页面，并将页表项权限为设置为 perm。
建立连接
Hint:
If there is already a page mapped at `va`, call page_remove() to release this mapping.
The `pp_ref` should be incremented if the insertion succeeds.*/
int page_insert(Pde *pgdir, struct Page *pp, u_long va, u_int perm)
{

	/* Step 1: Get corresponding page table entry. */
	/* Step 2: Update TLB. */
	/* hint: use tlb_invalidate function */
	/* Step 3: Do check, re-get page table entry to validate the insertion. */
	/* Step 3.1 Check if the page can be insert, if can’t return -E_NO_MEM */
	/* Step 3.2 Insert page and increment the pp_ref */
	Pte *pgtable_entry;
    int ret;
    perm = perm | PTE_V;//valid

    // Step 0. check whether `va` is already mapping to `pa`
    pgdir_walk(pgdir, va, 0 /* for check */, &pgtable_entry);//找找看有没有二级页表项
    if (pgtable_entry != 0 && (*pgtable_entry & PTE_V) != 0) { // 若存在pte, 且 pte valid
        // check whether `va` is mapping to another physical frame
        if (pa2page(*pgtable_entry) != pp) { 
		//pgtable_entry 该二级页表项内的对应物理页如果对不上我想要的pp话
            page_remove(pgdir, va); 
			// unmap it! 取消原来该二级页表项的物理页， 可以去看下page_remove
        } else { //如果对上了, 要更新perm
            tlb_invalidate(pgdir, va);              // <~~
            *pgtable_entry = page2pa(pp) | perm;    // update the permission
            return 0;
        }
    }
    tlb_invalidate(pgdir, va);                      // <~~,删除对应的TLB, 诱发充填
    /* Step 1. use `pgdir_walk` to "walk" the page directory */
    if ((ret = pgdir_walk(pgdir, va, 1, &pgtable_entry)) < 0) 
	// 刚刚不是remove了，再创建一个页表项，返回 va对应的pgtable_entry
        return ret; // freelist 不够， exception
    /* Step 2. fill in the page table */
	// PTE_ADDR 就是说取PTE中PFN地址部分
    *pgtable_entry = (/*Physical Frame Address of `pp` */page2pa(pp)) | perm;
    pp->pp_ref++;
	return 0;
}

/*Overview:
  Look up the Page that virtual address `va` map to.

  Post-Condition:
  Return a pointer to corresponding Page, and store it's page table entry to *ppte.
  If `va` doesn't mapped to any Page, return NULL.*/
struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte)
{
	struct Page *ppage;
	Pte *pte;

	/* Step 1: Get the page table entry. */
	pgdir_walk(pgdir, va, 0, &pte);

	/* Hint: Check if the page table entry doesn't exist or is not valid. */
	if (pte == 0) {
		return 0;
	}
	if ((*pte & PTE_V) == 0) {
		return 0;    //the page is not in memory.
	}

	/* Step 2: Get the corresponding Page struct. */

	/* Hint: Use function `pa2page`, defined in include/pmap.h . */
	ppage = pa2page(*pte);
	if (ppte) {
		*ppte = pte;
	}

	return ppage;
}

// Overview:
// 	Decrease the `pp_ref` value of Page `*pp`, if `pp_ref` reaches to 0, free this page.
void page_decref(struct Page *pp) {
	if(--pp->pp_ref == 0) {
		page_free(pp);
	}
}

// Overview:
// 	Unmaps the physical page at virtual address `va`.
void page_remove(Pde *pgdir, u_long va)
{
	Pte *pagetable_entry;
	struct Page *ppage;

	/* Step 1: Get the page table entry, and check if the page table entry is valid. */

	ppage = page_lookup(pgdir, va, &pagetable_entry);

	if (ppage == 0) {
		return;
	}

	/* Step 2: Decrease `pp_ref` and decide if it's necessary to free this page. */

	/* Hint: When there's no virtual address mapped to this page, release it. */
	ppage->pp_ref--;
	if (ppage->pp_ref == 0) {
		page_free(ppage);
	}

	/* Step 3: Update TLB. */
	*pagetable_entry = 0;
	tlb_invalidate(pgdir, va);
	return;
}

// Overview:
// 	Update TLB.
void tlb_invalidate(Pde *pgdir, u_long va)
{
	if (curenv) {
		tlb_out(PTE_ADDR(va) | GET_ENV_ASID(curenv->env_id));
	} else {
		tlb_out(PTE_ADDR(va));
	}
}

void physical_memory_manage_check(void)
{
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;
	int *temp;

	// should be able to allocate three pages
	//printf("what?"); //
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);//取一个pp0
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);



	// temporarily steal the rest of the free pages
	fl = page_free_list;
	// now this page_free list must be empty!!!!
	LIST_INIT(&page_free_list);//取消pageinit分配的freepage
	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM); //没有空闲的了

	temp = (int*)page2kva(pp0);
	// kva 有了
	//printf("pp0'skva':%x\n", temp);//
	//write 1000 to pp0
	*temp = 1000;
	// free pp0
	//printf("pp0'ref'%d\n", pp0->pp_ref);//
	page_free(pp0);
	printf("The number in address temp is %d\n",*temp);

	// alloc again
	assert(page_alloc(&pp0) == 0);
	assert(pp0);
	//printf("alloc again\n");//
	// pp0 should not change
	assert(temp == (int*)page2kva(pp0));
	// pp0 should be zero
	assert(*temp == 0);

	page_free_list = fl;
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);
	struct Page_list test_free;
	struct Page *test_pages;
	test_pages= (struct Page *)alloc(10 * sizeof(struct Page), BY2PG, 1);
	LIST_INIT(&test_free);
	//LIST_FIRST(&test_free) = &test_pages[0];
	int i,j=0;
	struct Page *p, *q;
	//test inert tail
	for(i=0;i<10;i++) {
		//printf("HEY I AM HERE!"); //
		test_pages[i].pp_ref=i;
		//test_pages[i].pp_link=NULL;
		//printf("bef:0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);//
		LIST_INSERT_TAIL(&test_free,&test_pages[i],pp_link);
		//printf("0x%x  0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next, *(test_pages[i].pp_link.le_prev));//

	}
	p = LIST_FIRST(&test_free);
	int answer1[]={0,1,2,3,4,5,6,7,8,9};
	assert(p!=NULL);
	while(p!=NULL)
	{
		//printf("%d %d\n",p->pp_ref,answer1[j]);//
		assert(p->pp_ref==answer1[j++]);
		//printf("???\n"); //
		//printf("ptr: 0x%x v: %d\n",(p->pp_link).le_next,((p->pp_link).le_next)->pp_ref);//
		p=LIST_NEXT(p,pp_link);
		//printf("%d***\n", p); //
	}
	//printf("HEYEYEYE\n"); //
	// insert_after test
	int answer2[]={0,1,2,3,4,20,5,6,7,8,9};
	q=(struct Page *)alloc(sizeof(struct Page), BY2PG, 1);
	q->pp_ref = 20;
	//printf("q's next %x\n", LIST_NEXT(q, pp_link)); //
	//printf("---%d\n",test_pages[4].pp_ref);//
	LIST_INSERT_AFTER(&test_pages[4], q, pp_link);
	//printf("---%d\n",LIST_NEXT(&test_pages[4],pp_link)->pp_ref);//
	p = LIST_FIRST(&test_free);
	j=0;
	//printf("into test\n");//
	while(p!=NULL){
		 // printf("%d %d\n",p->pp_ref,answer2[j]);//
		assert(p->pp_ref==answer2[j++]);
		p=LIST_NEXT(p,pp_link);
		//printf("p:%d\n", p); //
	}
	//printf("WTF!!"); //


	printf("physical_memory_manage_check() succeeded\n");
}


void page_check(void)
{
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	// now this page_free list must be empty!!!!
	LIST_INIT(&page_free_list);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// there is no free memory, so we can't allocate a page table
	// 创建va 0 和 pp1的映射, 先要看下当前va上的二级页表项内容， 可能不存在，就需要创建该页表
	// 可是申请不到页表page
	assert(page_insert(boot_pgdir, pp1, 0x0, 0) < 0);

	// free pp0 and try again: pp0 should be used for page table
	page_free(pp0);
	//建立物理页pp1 与 va:0x0映射
	assert(page_insert(boot_pgdir, pp1, 0x0, 0) == 0);
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));

	//printf("pp0?%d\n", pp0->pp_ref); //
	//在insert和walk中为在freelist中的pp0拿过来alloc成为页表page
	printf("va2pa(boot_pgdir, 0x0) is %x\n",va2pa(boot_pgdir, 0x0));
	printf("page2pa(pp1) is %x\n",page2pa(pp1));

	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	//page2pa物理地址pp1 page -> ppn ->  pa
	//va ->通过boot_pgdir二级页表查找到了PFN-> pa       映射成功
	// 因为insert过程就是重新填表的过程
	assert(pp1->pp_ref == 1);

	// should be able to map pp2 at BY2PG because pp0 is already allocated for page table
	assert(page_insert(boot_pgdir, pp2, BY2PG, 0) == 0);

	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	printf("start page_insert\n");
	// should be able to map pp2 at BY2PG because it's already there
	assert(page_insert(boot_pgdir, pp2, BY2PG, 0) == 0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// pp2 should NOT be on the free list
	// could happen in ref counts are handled sloppily in page_insert
	assert(page_alloc(&pp) == -E_NO_MEM);

	// should not be able to map at PDMAP because need free page for page table
	assert(page_insert(boot_pgdir, pp0, PDMAP, 0) < 0);

	// insert pp1 at BY2PG (replacing pp2)
	assert(page_insert(boot_pgdir, pp1, BY2PG, 0) == 0);

	// should have pp1 at both 0 and BY2PG, pp2 nowhere, ...
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp1));
	// ... and ref counts should reflect this
	assert(pp1->pp_ref == 2);
	printf("pp2->pp_ref %d\n",pp2->pp_ref);
	assert(pp2->pp_ref == 0);
	printf("end page_insert\n");

	// pp2 should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp2);

	// unmapping pp1 at 0 should keep pp1 at BY2PG
	page_remove(boot_pgdir, 0x0);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 0);

	// unmapping pp1 at BY2PG should free it
	page_remove(boot_pgdir, BY2PG);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, BY2PG) == ~0);
	assert(pp1->pp_ref == 0);
	assert(pp2->pp_ref == 0);

	// so it should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// forcibly take pp0 back
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	boot_pgdir[0] = 0;
	//printf("pp0?%d\n", pp0->pp_ref); //
	assert(pp0->pp_ref == 1);
	pp0->pp_ref = 0;

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);

	printf("page_check() succeeded!\n");
}

void pageout(int va, int context)
{
	u_long r;
	struct Page *p = NULL;

	if (context < 0x80000000) {
		panic("tlb refill and alloc error!");
	}

	if ((va > 0x7f400000) && (va < 0x7f800000)) {
		panic(">>>>>>>>>>>>>>>>>>>>>>it's env's zone");
	}

	if (va < 0x10000) {
		panic("^^^^^^TOO LOW^^^^^^^^^");
	}

	if ((r = page_alloc(&p)) < 0) {
		panic ("page alloc error!");
	}

	p->pp_ref++;

	page_insert((Pde *)context, p, VA2PFN(va), PTE_R);
//	printf("pageout:\t@@@___0x%x___@@@  ins a page \n", va);
}
