/* This is a simplefied ELF reader.
 * You can contact me if you find any bugs.
 *
 * Luming Wang<wlm199558@126.com>
 */


#include "kerelf.h"
#include <stdio.h>
/* Overview:
 *   Check whether it is a ELF file.
 *
 * Pre-Condition:
 *   binary must longer than 4 byte.
 *
 * Post-Condition:
 *   Return 0 if `binary` isn't an elf. Otherwise
 * return 1.
 */
int is_elf_format(u_char *binary)
{
        Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;
        if (ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
                ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
                ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
                ehdr->e_ident[EI_MAG3] == ELFMAG3) {
                return 1;
        }

        return 0;
}
#define REV16(x) 	(((((unsigned short)(x)) & 0x00ff)<<8) | ((((unsigned short)(x)) & 0xff00)>>8)) 
#define REV32(x) 	(((((unsigned int)(x)) & 0x000000ff) << 24)|\
((((unsigned int)(x)) & 0x0000ff00) << 8)|\
((((unsigned int)(x)) & 0x00ff0000) >> 8)|\
((((unsigned int)(x)) & 0xff000000) >> 24))

unsigned short rev16(unsigned short x) {
	return ((x & 0x00ff) << 8) | ((x & 0xff00) >> 8); 
}
unsigned int rev32(unsigned int x) {
	return ((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24); 
}
/* Overview:
 *   read an elf format binary file. get ELF's information
 *
 * Pre-Condition:
 *   `binary` can't be NULL and `size` is the size of binary.
 *
 * Post-Condition:
 *   Return 0 if success. Otherwise return < 0.
 *   If success, output address of every section in ELF.
 */

/*
    Exercise 1.2. Please complete func "readelf". 
*/
int readelf(u_char *binary, int size)
{
        Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;

        int Nr;

        Elf32_Shdr *shdr = NULL;
		Elf32_Shdr *shdr_b = NULL; 
        u_char *ptr_sh_table = NULL;
		u_char *ptr_sh_table_b = NULL; 
        Elf32_Half sh_entry_count;
        Elf32_Half sh_entry_count_b; 

		Elf32_Half sh_entry_size;
		Elf32_Half sh_entry_size_b; 
        // check whether `binary` is a ELF file.
        if (size < 4 || !is_elf_format(binary)) {
                printf("not a standard elf format\n");
                return 0;
        }
		//printf("%x", ehdr->e_type); 
        // get section table addr, section header number and section header size.
		ptr_sh_table = ehdr->e_shoff + binary;//binary char*指针, off是一个int代表偏移多少char
		sh_entry_count = ehdr->e_shnum; 
		sh_entry_size = ehdr->e_shentsize; 
		
		ptr_sh_table_b = (rev32(ehdr->e_shoff)) + binary;
		sh_entry_count_b = rev16(ehdr->e_shnum); 
		sh_entry_size_b = rev16(ehdr->e_shentsize); 
		if(ehdr->e_ident[5] == 0x02) {
       		for(Elf32_Half i = 0; i < sh_entry_count_b; i++) {
           		shdr_b = (Elf32_Shdr *) (ptr_sh_table_b + i * sh_entry_size_b); 
            	printf("%d:0x%x\n", i, rev32(shdr_b->sh_addr)); 
        	}
		}
		else if(ehdr->e_ident[5] == 0x01){
			 for(Elf32_Half i = 0; i < sh_entry_count; i++) {
            	shdr = (Elf32_Shdr *) (ptr_sh_table + i * sh_entry_size);  
            	printf("%d:0x%x\n", i, shdr->sh_addr);  
        	} 
		} 


        // for each section header, output section number and section addr.
        // hint: section number starts at 0.
		// find ph
		u_char *ptr_ph_table = ehdr->e_phoff + binary;//别忘了binary 
		Elf32_Half ph_entry_count = ehdr->e_phnum; 
		Elf32_Half ph_entry_size = ehdr->e_phentsize;
		Elf32_Phdr *phdr = (Elf32_Phdr *)ptr_ph_table; 
		/*for(Elf32_Half i = 0; i < ehdr->e_phnum; i++) {
			//phdr = (Elf32_Phdr)(ptr_ph_table + i * ph_entry_size);// ()里面u_char+ i*ph_entry_size个char
			printf("size:%d\n", ph_entry_size);
			printf("size:%d\n", sizeof(*phdr)); 
			printf("%d:0x%x;align:%x\n\n", i, phdr->offset, phdr->p_align); 
			phdr++; 
		}*/

        return 0;
}

