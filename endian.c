四、如何判断机器的字节序 (重点)
#include <stdio.h>
int main (void)
{
    union
    {	
		short i;	
		char a[2];
    }u;
    u.a[0] = 0x11;
    u.a[1] = 0x22;
    printf ( "0x%x\n", u.i);  
    //0x2211 为小端  0x1122 为大端
    return 0;
}
//输出结果：0x2211
   
   
 /*“共用”类型“FOO”*/ 
union foo{    
    int i;  /*“整数”类型“i”*/     
    char c;    /*“字符”类型“C”*/   
    double k; /*“双”精度类型“K”*/     
};
    
/*“共用”类型“FOO”*/
union foo     
{
    char s[10];    /*“字符”类型的数组“S”下面有“10”个元素*/
    int i;   /*“整数”类型i*/    
};


#include <stdio.h>
int main (void)
{
    short i = 0x1122;
    char *a = (char*)(&i);
    printf ("0x%x\n", *(a +0)); 
    //大端为 0x11 小端为 0x22
    printf ("0x%x\n", *(a +1));
    return 0;
}
// 输出结果：
// 0x22
// 0x11
 
六、如何进行大小端转换（重点）///
// 第一种方法：位操作 
#include<stdio.h>    
typedef unsigned int uint_32 ;     
typedef unsigned short uint_16 ; 

//16位
#define BSWAP_16(x) \
    (uint_16)((((uint_16)(x) & 0x00ff) << 8) | \
    (((uint_16)(x) & 0xff00) >> 8) \
)    

//32位  
#define BSWAP_32(x) \
    (uint_32)((((uint_32)(x) & 0xff000000) >> 24) | \
    (((uint_32)(x) & 0x00ff0000) >> 8) | \
    (((uint_32)(x) & 0x0000ff00) << 8) | \
    (((uint_32)(x) & 0x000000ff) << 24) \
) 

//无符号整型16位    
uint_16 bswap_16(uint_16 x)  
{  
    return (((uint_16)(x) & 0x00ff) << 8) | \
    (((uint_16)(x) & 0xff00) >> 8) ;
}

//无符号整型32位
uint_32 bswap_32(uint_32 x)  
{
    return (((uint_32)(x) & 0xff000000) >> 24) | \
			(((uint_32)(x) & 0x00ff0000) >> 8) | \
			(((uint_32)(x) & 0x0000ff00) << 8) | \
			(((uint_32)(x) & 0x000000ff) << 24) ;  
} 
	 
int main(int argc,char *argv[])  
{  
    printf("------------带参宏-------------\n"); 
    printf("%#x\n",BSWAP_16(0x1234)) ; 
    printf("%#x\n",BSWAP_32(0x12345678)); 
	
    printf("------------函数调用-----------\n"); 
    printf("%#x\n",bswap_16(0x1234)) ;  
    printf("%#x\n",bswap_32(0x12345678)); 
	
    return 0 ;  
}  
// 输出结果：
// ------------带参宏-------------
// 0x3412
// 0x78563412
// ------------函数调用-----------
// 0x3412
// 0x78563412
 

// 第二种方法：从软件的角度理解端模式 使用 htonl, htons, ntohl, ntohs 等函数 
NAME htonl, htons, ntohl, ntohs - convert values between host and network byte orderSYNOPSIS  
 #include <arpa/inet.h> 
 uint32_t htonl(uint32_t hostlong); 
 
 uint16_t htons(uint16_t hostshort); 
 
 uint32_t ntohl(uint32_t netlong);
 
 uint16_t ntohs(uint16_t netshort);
 
DESCRIPTION
    The htonl() function converts the unsigned integer hostlong from host byte order to network byte order.
	
    The htons() function converts the unsigned short integer hostshort from host byte order to network byte order.
	
	The ntohl() function converts the unsigned integer netlong from network byte order to host byte order.
	
	The ntohs() function converts the unsigned short integer netshort from network byte order to host byte order.On  the  i386  the host byte order is Least Significant Byte first, whereas the network byte order, as used on the Internet, is MostSignificant Byte first.
【翻译】		
	htonl()     //32位无符号整型的主机字节顺序到网络字节顺序的转换（小端->>大端）
	
	htons()     //16位无符号短整型的主机字节顺序到网络字节顺序的转换  （小端->>大端）
	
	ntohl()     //32位无符号整型的网络字节顺序到主机字节顺序的转换  （大端->>小端）
	
	ntohs()     //16位无符号短整型的网络字节顺序到主机字节顺序的转换  （大端->>小端）

//注，主机字节顺序，X86一般多为小端（little-endian），网络字节顺序，即大端(big-endian)；

//示例一
#include <stdio.h>
#icnlude <arpa/inet.h>
int main (void)
{	
	union
	{	
		short i;	
		char a[2];
	}u;

    u.a[0] = 0x11;
    u.a[1] = 0x22;
	
    printf ("0x%x\n", u.i); //0x2211 为小端  0x1122 为大端 
    printf ("0x%.x\n", htons (u.i)); //大小端转换 
    
    return 0;
}
// 输出结果：
// 0x2211
// 0x1122

 //示例二
#include <stdio.h>    
#include <arpa/inet.h> 

struct ST{
    short val1;  
    short val2; 
};

union U{  
    int val;
    struct ST st;  
 };
 
int main(void)  
{  
    int a = 0; 
    union U u1, u2;  
	
    a = 0x12345678;  
    u1.val = a; 
	
    printf("u1.val is 0x%x\n", u1.val);  // u1.val is 0x12345678
    printf("val1 is 0x%x\n", u1.st.val1);  // val1 is 0x5678
    printf("val2 is 0x%x\n", u1.st.val2);	// val2 is 0x1234
    printf("after first convert is: 0x%x\n", htonl(u1.val));// after first convert is: 0x78563412
	
    u2.st.val2 = htons(u1.st.val1); 
    u2.st.val1 = htons(u1.st.val2);
    printf("after second convert is: 0x%x\n", u2.val); // after second convert is: 0x78563412
	
    return 0; 
}
//注：作者的输出我已经转到对应的输出语句之后

