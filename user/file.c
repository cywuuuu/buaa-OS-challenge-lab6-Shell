#include "lib.h"
#include <fs.h>

#define debug 0

static int file_close(struct Fd *fd);
static int file_read(struct Fd *fd, void *buf, u_int n, u_int offset);
static int file_write(struct Fd *fd, const void *buf, u_int n, u_int offset);
static int file_stat(struct Fd *fd, struct Stat *stat);


// Dot represents choosing the variable of the same name within struct declaration
// to assign, and no need to consider order of variables.
struct Dev devfile = {
	.dev_id =	'f',
	.dev_name =	"file",
	.dev_read =	file_read,
	.dev_write =	file_write,
	.dev_close =	file_close,
	.dev_stat =	file_stat,
};
/*
struct Fd
{
	u_int fd_dev_id;
	u_int fd_offset;
	u_int fd_omode;
};
*/

// Overview:
//	Open a file (or directory).
//
// Returns:
//	the file descriptor onsuccess,
//	< 0 on failure.
/*** exercise 5.8 ***/
int
open(const char *path, int mode)
{
	struct Fd *fd;
	struct Filefd *ffd;
	u_int size, fileid;
	int r;
	u_int va;
	u_int i;
	// 用户进程向文件系统发送打开文件的请求
	// 文件系统进程会将这些基本信息记录在内存返回
	// 然后由操作系统将用户进程请求的地址映射到同一个物理页上
/*
struct Fd {
    u_int fd_dev_id; //外设id，也就是外设类型
    u_int fd_offset; // 读或写的当前位置（偏移量），类似于“流”的当前位置
    u_int fd_omode; // 打开方式，比如只读，只写，均可blabla
};
*/
	// Step 1: Alloc a new Fd, return error code when fail to alloc.
	// Hint: Please use fd_alloc.
	r = fd_alloc(&fd); //fd获取了一页 
	//writef("fd alloc ok\n"); 
	if (r < 0) return r; 
	// Step 2: Get the file descriptor of the file to open.
	// Hint: Read fsipc.c, and choose a function.
	//writef("opening...\n"); 
	r = fsipc_open(path, mode, fd);
	//writef("opened ok %d\n", r); 
	// 函数内部会把path包裹进Fsreq_open req, 向server发送
	// server  返回一个fd 文件描述块
	if (r < 0) return r; 
	// Step 3: Set the start address storing the file's content. 
	// Set size and fileid correctly.
	// Hint: Use fd2data to get the start address.
/*
struct Filefd { //单纯的内存数据；
struct Fd f_fd; //文件描述符
u_int f_fileid; //文件系统为打开的文件进行的编号
struct File f_file; //对应文件的文件控制块
};
*/	// ffd filefd 可以获取动态的id, f_fileid, 文件控制块f_file
	ffd = (struct Filefd *)fd; 
	// va 得到fd的数据块首地址
	va = fd2data(fd); 
	fileid = ffd->f_fileid; 
	size = ffd->f_file.f_size; 
	// Step 4: Alloc memory, map the file content into memory.
	// 映射文件内容到进程
	// 找到File以后， 进行映射
	if ((mode & O_APPEND) != 0) {
		ffd->f_fd.fd_offset = size; 
	} 
	for (i = 0; i < size; i += BY2BLK ) {
		
		//	向server发map请求, 发送fsipc_map(fileid,offset, va)
		//	sever返回一个page, page中有block
		r = fsipc_map(fileid, i, va+i); //va+i就是说要映射block到我这个虚地址哦
		if (r < 0) return r; 
	}
	//writef("map ok"); 
	// Step 5: Return the number of file descriptor.
	// 完成映射，返回file file descriptor id
	//writef("fd ok %d",fd); 	
	return fd2num(fd); 

}

// Overview:
//	Close a file descriptor
int
file_close(struct Fd *fd)
{
	int r;
	struct Filefd *ffd;
	u_int va, size, fileid;
	u_int i;

	ffd = (struct Filefd *)fd;
	fileid = ffd->f_fileid;
	size = ffd->f_file.f_size;

	// Set the start address storing the file's content.
	va = fd2data(fd);

	// Tell the file server the dirty page.
	for (i = 0; i < size; i += BY2PG) {
		fsipc_dirty(fileid, i);
	}

	// Request the file server to close the file with fsipc.
	if ((r = fsipc_close(fileid)) < 0) {
		writef("cannot close the file\n");
		return r;
	}

	// Unmap the content of file, release memory.
	if (size == 0) {
		return 0;
	}
	for (i = 0; i < size; i += BY2PG) {
		if ((r = syscall_mem_unmap(0, va + i)) < 0) {
			writef("cannont unmap the file.\n");
			return r;
		}
	}
	return 0;
}

// Overview:
//	Read 'n' bytes from 'fd' at the current seek position into 'buf'. Since files
//	are memory-mapped, this amounts to a user_bcopy() surrounded by a little red
//	tape to handle the file size and seek pointer.
static int
file_read(struct Fd *fd, void *buf, u_int n, u_int offset)
{
	u_int size;
	struct Filefd *f;
	f = (struct Filefd *)fd;

	// Avoid reading past the end of file.
	size = f->f_file.f_size;

	if (offset > size) {
		return 0;
	}

	if (offset + n > size) {
		n = size - offset;
	}

	user_bcopy((char *)fd2data(fd) + offset, buf, n);
	return n;
}

// Overview:
//	Find the virtual address of the page that maps the file block
//	starting at 'offset'.
int
read_map(int fdnum, u_int offset, void **blk)
{
	int r;
	u_int va;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0) {
		return r;
	}

	if (fd->fd_dev_id != devfile.dev_id) {
		return -E_INVAL;
	}

	va = fd2data(fd) + offset;

	if (offset >= MAXFILESIZE) {
		return -E_NO_DISK;
	}

	if (!((* vpd)[PDX(va)]&PTE_V) || !((* vpt)[VPN(va)]&PTE_V)) {
		return -E_NO_DISK;
	}

	*blk = (void *)va;
	return 0;
}

// Overview:
//	Write 'n' bytes from 'buf' to 'fd' at the current seek position.
static int
file_write(struct Fd *fd, const void *buf, u_int n, u_int offset)
{
	int r;
	u_int tot;
	struct Filefd *f;

	f = (struct Filefd *)fd;

	// Don't write more than the maximum file size.
	tot = offset + n;

	if (tot > MAXFILESIZE) {
		return -E_NO_DISK;
	}

	// Increase the file's size if necessary
	if (tot > f->f_file.f_size) {
		if ((r = ftruncate(fd2num(fd), tot)) < 0) {
			return r;
		}
	}

	// Write the data
	user_bcopy(buf, (char *)fd2data(fd) + offset, n);
	return n;
}

static int
file_stat(struct Fd *fd, struct Stat *st)
{
	struct Filefd *f;

	f = (struct Filefd *)fd;

	strcpy(st->st_name, (char *)f->f_file.f_name);
	st->st_size = f->f_file.f_size;
	st->st_isdir = f->f_file.f_type == FTYPE_DIR;
	return 0;
}

// Overview:
//	Truncate or extend an open file to 'size' bytes
int
ftruncate(int fdnum, u_int size)
{
	int i, r;
	struct Fd *fd;
	struct Filefd *f;
	u_int oldsize, va, fileid;

	if (size > MAXFILESIZE) {
		return -E_NO_DISK;
	}

	if ((r = fd_lookup(fdnum, &fd)) < 0) {
		return r;
	}

	if (fd->fd_dev_id != devfile.dev_id) {
		return -E_INVAL;
	}

	f = (struct Filefd *)fd;
	fileid = f->f_fileid;
	oldsize = f->f_file.f_size;
	f->f_file.f_size = size;

	if ((r = fsipc_set_size(fileid, size)) < 0) {
		return r;
	}

	va = fd2data(fd);

	// Map any new pages needed if extending the file
	for (i = ROUND(oldsize, BY2PG); i < ROUND(size, BY2PG); i += BY2PG) {
		if ((r = fsipc_map(fileid, i, va + i)) < 0) {
			fsipc_set_size(fileid, oldsize);
			return r;
		}
	}

	// Unmap pages if truncating the file
	for (i = ROUND(size, BY2PG); i < ROUND(oldsize, BY2PG); i += BY2PG)
		if ((r = syscall_mem_unmap(0, va + i)) < 0) {
			user_panic("ftruncate: syscall_mem_unmap %08x: %e", va + i, r);
		}

	return 0;
}

int
create(const char *path, int t) {
    int r;
    if ((r = fsipc_create(path, t)) < 0) {
        return r;
    }
    return 0;
}
// Overview:
//	Delete a file or directory.
/*** exercise 5.10 ***/
int
remove(const char *path)
{
	// Your code here.
	// Call fsipc_remove.
	return fsipc_remove(path); 
}

// Overview:
//	Synchronize disk with buffer cache
int
sync(void)
{
	return fsipc_sync();
}
