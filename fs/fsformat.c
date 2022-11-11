/*
 * BUAA MIPS OS Kernel file system format
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

/* Prevent inc/types.h, included from inc/fs.h,
 * From attempting to redefine types defined in the host's inttypes.h. */
#define _INC_TYPES_H_
#define BY2PG       4096        // bytes to a page
#include "../include/fs.h"

#define nelem(x)    (sizeof(x) / sizeof((x)[0]))
typedef struct Super Super;
typedef struct File File;

#define NBLOCK 1024 // The number of blocks in the disk. 块数
uint32_t nbitblock; // the number of bitmap blocks.
uint32_t nextbno;   // next availiable block.

struct Super super; // super block.
/*
struct Super {
    u_int s_magic;      // Magic number: FS_MAGIC
    u_int s_nblocks;    // Total number of blocks on disk
    struct File s_root; // Root directory node
};
struct File {
    u_char f_name[MAXNAMELEN];  // filename
    u_int f_size;           // file size in bytes
    u_int f_type;           // file type
    u_int f_direct[NDIRECT];
    u_int f_indirect;

    struct File *f_dir;     // the pointer to the dir where this file is in, valid only in memory.
    u_char f_pad[BY2FILE - MAXNAMELEN - 4 - 4 - NDIRECT * 4 - 4 - 4];
};
*/
enum {
    BLOCK_FREE  = 0,
    BLOCK_BOOT  = 1,
    BLOCK_BMAP  = 2,
    BLOCK_SUPER = 3,
    BLOCK_DATA  = 4,
    BLOCK_FILE  = 5,
    BLOCK_INDEX = 6,
};

struct Block {
    uint8_t data[BY2BLK];
    uint32_t type;
} disk[NBLOCK]; // 许多个(NBLOCK = 1024个)Block组成了一个disk 数组

// reverse: mutually transform between little endian and big endian.
void reverse(uint32_t *p) {
    uint8_t *x = (uint8_t *) p;
    uint32_t y = *(uint32_t *) x;
    x[3] = y & 0xFF;
    x[2] = (y >> 8) & 0xFF;
    x[1] = (y >> 16) & 0xFF;
    x[0] = (y >> 24) & 0xFF;
}

// reverse_block: reverse proper filed in a block.
void reverse_block(struct Block *b) {
    int i, j;
    struct Super *s;
    struct File *f, *ff;
    uint32_t *u;

    switch (b->type) {
    case BLOCK_FREE:
    case BLOCK_BOOT:
        break; // do nothing.
    case BLOCK_SUPER:
        s = (struct Super *)b->data;
        reverse(&s->s_magic);
        reverse(&s->s_nblocks);

        ff = &s->s_root;
        reverse(&ff->f_size);
        reverse(&ff->f_type);
        for(i = 0; i < NDIRECT; ++i) {
            reverse(&ff->f_direct[i]);
        }
        reverse(&ff->f_indirect);
        break;
    case BLOCK_FILE:
        f = (struct File *)b->data;
        for(i = 0; i < FILE2BLK; ++i) {
            ff = f + i;
            if(ff->f_name[0] == 0) {
                break;
            }
            else {
                reverse(&ff->f_size);
                reverse(&ff->f_type);
                for(j = 0; j < NDIRECT; ++j) {
                    reverse(&ff->f_direct[j]);
                }
                reverse(&ff->f_indirect);
            }
        }
        break;
    case BLOCK_INDEX:
    case BLOCK_BMAP:
        u = (uint32_t *)b->data;
        for(i = 0; i < BY2BLK/4; ++i) {
            reverse(u+i);
        }
        break;
    }
}

// Initial the disk. Do some work with bitmap and super block.
void init_disk() {
    int i, r, diff;

    // Step 1: Mark boot sector block.
    disk[0].type = BLOCK_BOOT;//起初block

    // Step 2: Initialize boundary.
    nbitblock = (NBLOCK + BIT2BLK - 1) / BIT2BLK;
	//BIT2BLOCK 就是说 每个BLOCK多少BIT(1024*8 bit)
	//位图共有（NBLOCK'1024' + BIT2BLK - 1）个bit的信息，(每个BLOCK 由一个bit代表， 向上取整
	// nbitblock 就是由这么多个block来存位图
    
	nextbno = 2 + nbitblock;//正经储存的首位置
	
    // Step 2: Initialize bitmap blocks.
    for(i = 0; i < nbitblock; ++i) { 
        disk[2+i].type = BLOCK_BMAP;//block2 以上部分
    }
    for(i = 0; i < nbitblock; ++i) {
        memset(disk[2+i].data, 0xff, BY2BLK); //设置位图全为1  
    }
    if(NBLOCK != nbitblock * BIT2BLK) {
        diff = NBLOCK % BIT2BLK / 8;
        memset(disk[2+(nbitblock-1)].data+diff, 0x00, BY2BLK - diff);//位图中不存在的设置为0
    }

    // Step 3: Initialize super block.
    disk[1].type = BLOCK_SUPER;
    super.s_magic = FS_MAGIC;
    super.s_nblocks = NBLOCK;
    super.s_root.f_type = FTYPE_DIR;
    strcpy(super.s_root.f_name, "/");
}

// Get next block id, and set `type` to the block's type.
int next_block(int type) {
    disk[nextbno].type = type;
    return nextbno++;
}

// Flush disk block usage to bitmap.
void flush_bitmap() {
    int i;
    // update bitmap, mark all bit where corresponding block is used.
    for(i = 0; i < nextbno; ++i) {
        ((uint32_t *)disk[2+i/BIT2BLK].data)[(i%BIT2BLK)/32] &= ~(1<<(i%32));
    }
}

// Finish all work, dump block array into physical file.
void finish_fs(char *name) {
    int fd, i, k, n, r;
    uint32_t *p;

    // Prepare super block.
    memcpy(disk[1].data, &super, sizeof(super));

    // Dump data in `disk` to target image file.
    fd = open(name, O_RDWR|O_CREAT, 0666);
    for(i = 0; i < 1024; ++i) {
        reverse_block(disk+i);
        write(fd, disk[i].data, BY2BLK);
    }

    // Finish.
    close(fd);
}

// Save block link.
void save_block_link(struct File *f, int nblk, int bno)
{
    assert(nblk < NINDIRECT); // if not, file is too large !

    if(nblk < NDIRECT) {//
        f->f_direct[nblk] = bno;
    }
    else {
        if(f->f_indirect == 0) {
            // create new indirect block.
            f->f_indirect = next_block(BLOCK_INDEX);//
        }
        ((uint32_t *)(disk[f->f_indirect].data))[nblk] = bno;
    }
}

// Make new block contians link to files in a directory.
int make_link_block(struct File *dirf, int nblk) {
    int bno = next_block(BLOCK_FILE);
    save_block_link(dirf, nblk, bno);
    dirf->f_size += BY2BLK;
    return bno;
}

// Overview:
//      Create new block pointer for a file under sepcified directory.
//      Notice that when we delete a file, we do not re-arrenge all
//      other file pointers, so we should be careful of existing empty
//      file pointers
//
// Post-Condition:
//      We ASSUME that this function will never fail
//
// Return:
//      Return a unused struct File pointer
// Hint:
//      use make_link_block function
/*** exercise 5.4 ***/
/*struct File *create_file(struct File *dirf) {
    // 函数的目标在于在dirf目录文件控制块下， 找到一个可以存文件的地方，
	// 返回这个地方的指针
	struct File *dirblk;
    int i, bno, found;
    int nblk = dirf->f_size / BY2BLK;
	// dirf 目录文件占据了多少磁盘块
    // Your code here
    // Step1: According to different range of nblk, make classified discussion to
    //        calculate the correct block number.
	for (i = 0; i < nblk; i++) {
		if (i < NDIRECT) {
			bno = dirf->f_direct[i]; 
		} else {
			bno = ((uint32_t *)(disk[dirf->f_indirect].data))[i]; 
		}
		// bno对应着目录文件dirf的第i个盘的实际盘号
		dirblk = (struct File *) (disk[bno].data); 
		// 目录文件的盘内容肯定是文件控制块
		// dirblk就是第i个块的文件控制块数组的基地址，
		// 因为一个块由FILE2BLK 个文件控制块组成（4个)
		int j = 0;  
		for (j = 0; j < FILE2BLK; j++) {
			if (dirblk[j].f_name[0] == '\0') {
				return &dirblk[j]; 
				//被删除的文件，此处可放文件控制块
				// 或者是以前申请了block盘块，用了一部分FILE 
				// 覆盖掉就好了
			}
		}
		// 整个盘块全放满了FILE控制块
		// 新盘号在bnp
		bno = make_link_block(dirf, nblk); 
		return (struct File *)disk[bno].data; //新盘第一个放File文件
	
	}

    // Step2: Find an unused pointer


}*/
struct File *create_file(struct File *dirf) {
    struct File *dirblk;
    int i, j, bno, found;
    int nblk = dirf->f_size / BY2BLK;
    //一个FILE 就占了512B, 一个块4096B, sector 512B
    // Your code here
    // Step1: According to different range of nblk, make classified discussion to
    //        calculate the correct block number.
	for (i = 0; i < nblk; i++) {
		if (i < NDIRECT) {
			bno = dirf->f_direct[i]; //得到占据的磁盘块
		} else {
			bno = ((uint32_t *)(disk[dirf->f_indirect].data))[i]; //到另一个4096B全是次级指针的磁盘块上找映射
            //bno = ((uint32_t *)(disk[dirf->f_indirect].data))[i];
        }
        // bno就是文件的第几块
		dirblk = (struct File *)(disk[bno].data);
        // 一个文件第几块, FILE2BLK
		for (j = 0; j < FILE2BLK; j++) {
			if (dirblk[j].f_name[0] == '\0') {
				return &dirblk[j];
			}
		}
	}

    // Step2: Find an unused pointer
	bno = make_link_block(dirf, nblk);
	return (struct File *)disk[bno].data;
}

/*struct File *create_file(struct File *dirf) {
    struct File *dirblk;
    int i, bno, found;
    int nblk = dirf->f_size / BY2BLK;
    if(nblk == 0) {
        return (struct File *)(disk[(make_link_block(dirf, nblk))].data);//相当于返回第0个位置
    }
    if(nblk <= NDIRECT) {
        bno = dirf -> f_direct[nblk - 1];
    }else{
        bno = ((uint32_t *)(disk[dirf->f_indirect].data))[nblk - 1];
    }
    dirblk = (struct File *)disk[bno].data;
    for(i = 0; i < FILE2BLK; ++ i){
        if(dirblk[i].f_name[0] == '\0') {
            return &dirblk[i];
        }
    }
   
    return (struct File *)(disk[(make_link_block(dirf, nblk))].data);//再申请一个盘块
}*/
// Write file to disk under specified dir.
void write_file(struct File *dirf, const char *path) {
    int iblk = 0, r = 0, n = sizeof(disk[0].data);
    uint8_t buffer[n+1], *dist;
    struct File *target = create_file(dirf);

    /* in case `create_file` is't filled */
    if (target == NULL) return;

    int fd = open(path, O_RDONLY);
 
    // Get file name with no path prefix.
    const char *fname = strrchr(path, '/');
    if(fname)
        fname++;
    else
        fname = path;
    strcpy(target->f_name, fname);
 
    target->f_size = lseek(fd, 0, SEEK_END);
    target->f_type = FTYPE_REG;
 
    // Start reading file.
    lseek(fd, 0, SEEK_SET);
    while((r = read(fd, disk[nextbno].data, n)) > 0) {
        save_block_link(target, iblk++, next_block(BLOCK_DATA));
    }
    close(fd); // Close file descriptor.
}

// Overview:
//      Write directory to disk under specified dir.
//      Notice that we may use standard library functions to operate on
//      directory to get file infomation.
//
// Post-Condition:
//      We ASSUME that this funcion will never fail
void write_directory(struct File *dirf, char *name) {
    // Your code here
	    int r;
    struct File *target = create_file(dirf);

    const char *fname = strrchr(name, '/');
    if (fname)
        fname++;
    else
        fname = name;
    strcpy(target->f_name, fname);

    target->f_size = 4096;
    target->f_type = FTYPE_DIR;
}

int main(int argc, char **argv) {
    int i;

    init_disk();//初始化

    if(argc < 3 || (strcmp(argv[2], "-r") == 0 && argc != 4)) {
        fprintf(stderr, "\
Usage: fsformat gxemul/fs.img files...\n\
       fsformat gxemul/fs.img -r DIR\n");
        exit(0);
    }

    if(strcmp(argv[2], "-r") == 0) {
        for (i = 3; i < argc; ++i) {
            write_directory(&super.s_root, argv[i]);
        }
    }
    else {
        for(i = 2; i < argc; ++i) {
            write_file(&super.s_root, argv[i]);
        }
    }

    flush_bitmap();
    finish_fs(argv[1]);

    return 0;
}
