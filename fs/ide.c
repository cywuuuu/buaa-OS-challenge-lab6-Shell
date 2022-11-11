/*
 * operations on IDE disk.
 */

#include "fs.h"
#include "lib.h"
#include <mmu.h>

// Overview:
// 	read data from IDE disk. First issue a read request through
// 	disk register and then copy data from disk buffer
// 	(512 bytes, a sector) to destination array.
//
// Parameters:
//	diskno: disk number.   IDE 的id
// 	secno: start sector number.   sector number 扇区号*扇区块大小0x200B -> offset
// 	dst: destination for data read from IDE disk.
// 	nsecs: the number of sectors to read. 要读几个扇区
//
// Post-Condition:
// 	If error occurrs during the read of the IDE disk, panic.
//
// Hint: use syscalls to access device registers and buffers
/*** exercise 5.2 ***/
void
ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs)
{
	// 0x200: the size of a sector: 512 bytes.
	int offset_begin = secno * 0x200;
	int offset_end = offset_begin + nsecs * 0x200;
	int offset = 0;

	while (offset_begin + offset < offset_end) {
		// Your code here 0x1300 0000
		int cur = offset_begin + offset; 
		// 选择IDE id 写哪个磁盘 write 0x10 
		int r = syscall_write_dev(&diskno, 0x13000010, 4); //把diskno写入1300 0010
		if (r < 0) user_panic("ERR in fs/ide.c, ide_read, set IDE ID \n"); 
		// 读哪个位置由 cur指定 写道磁盘 0x0
		r = syscall_write_dev(&cur, 0x13000000, 4); 
		if (r < 0) user_panic("ERR in fs/ide.c, ide_read, set offset \n");
		int tmp = 0; //在0x20 写入0， 代表读取开始
		r = syscall_write_dev(&tmp, 0x13000020, 4);  
		if (r < 0) user_panic("ERR in fs/ide.c, ide_read, set start \n");
		int result = 0; //到0x30 获取此次读取情况信息
		r = syscall_read_dev(&result, 0x13000030, 4); 
		if (r < 0) user_panic("ERR in fs/ide.c, ide_read, get state \n");      
		if (result == 0) user_panic("ERR in fs/ide.c, ide_read, failed \n");
		// 0x4000 缓冲区中获取内容
		r = syscall_read_dev(dst, 0x13004000, 0x200); 
		if (r < 0) user_panic("ERR in fs/ide.c, ide_read, get 512B from buffer\n"); 
		// error occurred, then panic.
		offset += 0x200;
		dst += 0x200; 
	}
}
/*void
ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs)
{
	// 0x200: the size of a sector: 512 bytes.
	int offset_begin = secno * 0x200;
	int offset_end = offset_begin + nsecs * 0x200;
	int offset = 0, curoffset;
	while (offset_begin + offset < offset_end) {
		curoffset = offset_begin + offset;
		if( syscall_write_dev(&diskno, 0x13000010, 4) < 0) {
			user_panic("Error in ide.c/ide_read 1\n");
		}
		if( syscall_write_dev(&curoffset, 0x13000000, 4) < 0) {
			user_panic("Error in ide.c/ide_read 2\n");
		}
		//char tmp = 0;
		int temp = 0;
		if( syscall_write_dev(&temp, 0x13000020, 4) < 0) {
			user_panic("Error in ide.c/ide_read 3\n");
		}
		int val;
		if( syscall_read_dev(&val, 0x13000030, 4) < 0){
			user_panic("Error in ide.c/ide_read 4\n");
		}
		if( val == 0 ) {//失败了
			user_panic("Error in ide.c/ide_read 4 val = 0\n");
		}
		if( syscall_read_dev(dst, 0x13004000, 512) < 0){
			user_panic("Error in ide.c/ide_read 5\n");
		}
		offset += 512;
		dst += 512;
		//offset_begin += 512;
	}
}
*/

// Overview:
// 	write data to IDE disk.
//
// Parameters:
//	diskno: disk number.
//	secno: start sector number.
// 	src: the source data to write into IDE disk.
//	nsecs: the number of sectors to write.
//
// Post-Condition:
//	If error occurrs during the read of the IDE disk, panic.
//
// Hint: use syscalls to access device registers and buffers
/*** exercise 5.2 ***/
/*void
ide_write(u_int diskno, u_int secno, void *src, u_int nsecs)
{
	// Your code here
	int offset_begin = secno * 0x200;
	int offset_end = offset_begin + nsecs * 0x200;
	int offset = 0;

	// DO NOT DELETE WRITEF !!!
	writef("diskno: %d\n", diskno);
    while (offset_begin + offset < offset_end) {
       // 写入缓冲
        int cur = offset_begin + offset;
		int r = syscall_write_dev(src, 0x13004000, 0x200);
        if (r < 0) user_panic("ERR in fs/ide.c, ide_write, write 512B to buffer\n");
        // 选择IDE id 写哪个磁盘 write 0x10 
        r = syscall_write_dev(&diskno, 0x13000010, 4); //把diskno写入1300 0010
        if (r < 0) user_panic("ERR in fs/ide.c, ide_write, set IDE ID \n");
        // 读哪个位置由 cur指定 写道磁盘 0x0
        r = syscall_write_dev(&cur, 0x13000000, 4);
        if (r < 0) user_panic("ERR in fs/ide.c, ide_write, set offset \n");
        int tmp = 1; //在0x20 写入1， 代表写入开始
        r = syscall_write_dev(&tmp, 0x13000020, 4); 
        if (r < 0) user_panic("ERR in fs/ide.c, ide_write, set start \n");
        int result = 0; //到0x30 获取此次读取情况信息
        r = syscall_read_dev(&result, 0x13000030, 4);
        if (r < 0) user_panic("ERR in fs/ide.c, ide_write, get info \n");
        if (result == 0) user_panic("ERR in fs/ide.c, ide_write, failed \n");
        // 0x4000 缓冲区中获取内容
        offset += 0x200;
        src += 0x200; 
    }
	
}*/
void
ide_write(u_int diskno, u_int secno, void *src, u_int nsecs)
{
    // Your code here
	int offset_begin = secno * 512;
	int offset_end = offset_begin + nsecs*512;
	// int offset = ;
	//writef("diskno: %d\n", diskno);
	while (offset_begin <  offset_end) {
		/* 1. 将数据写入缓冲区*/
		if(syscall_write_dev(src, 0x13004000, 512) < 0) {
			user_panic("Error in ide.c/ide_write 1\n");
		}
		/* 2. 选择磁盘号*/
		if(syscall_write_dev(&diskno, 0x13000010, 4) < 0) {
			user_panic("Error in ide.c/ide_write 2\n");
		}
		/* 3. 设置磁盘写入位置的偏移 */
		if(syscall_write_dev(&offset_begin, 0x13000000, 4) < 0) {
			user_panic("Error in ide.c/ide_write 3\n");
		}
		/* 4. 写入1, 表示开始把缓冲区内容写入磁盘 */
		//char tmp = 1;
		int temp = 1;
		if(syscall_write_dev(&temp, 0x13000020, 4) < 0) {
			user_panic("Error in ide.c/ide_write 4\n");
		}
		/* 5. 读取磁盘操作结果 */
		int val;
		if( syscall_read_dev(&val, 0x13000030, 4) < 0 ){
			user_panic("Error in ide.c/ide_write 5\n");
		}
		if(val == 0) {
			user_panic("Error in ide.c/ide_write 5 val = 0\n");
		}
		src += 512;
		offset_begin += 512;
	}
}

