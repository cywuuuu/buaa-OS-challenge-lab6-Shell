#include    <print.h>
/* macros */
#define     IsDigit(x)  ( ((x) >= '0') && ((x) <= '9') )
#define     Ctod(x)     ( (x) - '0')


/* forward declaration */
extern int PrintChar(char *, char, int, int);
extern int PrintString(char *, char *, int, int);
extern int PrintNum(char *, unsigned long, int, int, int, int, char, int);

/* private variable */
static const char theFatalMsg[] = "fatal error in lp_Print!";
struct s1 {
	int a;
	char b;
	char c;
	int d;
};

struct s2 {
	int size;
	int c[10000];
};
/* -*-
 * A low level printf() function.
 */
void
lp_Print(void (*output)(void *, char *, int),
     void * arg,
     char *fmt,
     va_list ap)
{

    #define     OUTPUT(arg, s, l)  \
    { if (((l) < 0) || ((l) > LP_MAX_BUF)) { \
        (*output)(arg, (char*)theFatalMsg, sizeof(theFatalMsg)-1); for(;;); \
        } else { \
        (*output)(arg, s, l); \
        } \
    }
    char buf[LP_MAX_BUF];

    char c;
    char *s;
    long int num;
    
    int longFlag;
    int negFlag;
    int width;
    int prec;
    int ladjust;
    char padc;

    int length;

    /*
        Exercise 1.5. Please fill in two parts in this file.
    */

    for(;;) 
    {
        /* Part1: your code here */
            
            {   
                
                /* scan for the next '%' 找到了待解析格式串，或者结束*/
                char *p = fmt;  
                while (1) {
                    if (*p == '\0' || *p == '%') {
                        break;
                    }
                    p++;
					//printf("----"); //
                }
                
                /* flush the string found so far 把以前这段读到的普通串 */
                OUTPUT(arg, fmt, p-fmt); 
                fmt = p; 

                /* check "are we hitting the end?" */
                if(*fmt == '\0') break; // 如果是上面是因为\0 而退出while(1),则跳出外面的for(; ;)循环
            }

			            
            /* we found a '%' */
            fmt ++; //pass %
            /* check for format flag */
            padc = ' ';
            ladjust = 0;
            /*if (*fmt == '-') {
           
                    ladjust = 1;
                    fmt ++;
                
                if (*fmt == '0') {
                    padc = '0';
                    fmt ++;
                 }  
            } else if(*fmt == '0') {

                    padc = '0';
                    fmt ++; 
                
                if(*fmt == '-'){
                    ladjust = 1; 
                    fmt ++; 
                }
            }*/
			while(*fmt == '-' || *fmt == '0') {
				if(*fmt == '0') {
					padc = '0'; 
				} 
				if(*fmt == '-') {
					ladjust = 1; 
				}
				fmt++; 
			}
            /* check for other prefixes */
            /* check for width number */
            width = 0;//每次循环初始化！             
            while(IsDigit(*fmt)) {
                width = width*10 + Ctod(*fmt); 
                fmt++; 
            }

            /* check for [.precision] */
            prec = 6; 
            if(*fmt == '.') {
                fmt++;
                prec = 0; 
                while(IsDigit(*fmt)) {
                    prec = prec*10 + Ctod(*fmt); 
                    fmt++;
                }
            } else {
                prec = 6; 
            }
            /* check for long */
            longFlag = 0; 
            if(*fmt == 'l') {
                longFlag = 1; 
                fmt++; //printf("nihaol");//                                                                      
            } 
			// new specifier
            int type = 0; 
			if(*fmt == '$') {
				fmt++; 
				if(*fmt == '1') {
					type = 1; 
				} else {
					type = 2;
//					printf("here!"); 
				}
				fmt++; 
			}

            /* check for specifier */
            negFlag = 0;
            switch (*fmt) {
             case 'b':
                if (longFlag) { // long int 以二进制数输出
                num = va_arg(ap, long int); //找到下一个long int
                } else {  
                num = va_arg(ap, int);// 下一个普通int
                }
                /* PrintNum(char * buf, unsigned long u, int base, int negFlag,
                          int length, int ladjust, char padc, int upcase)
                          */
                length = PrintNum(buf, num, 2, 0, width, ladjust, padc, 0);
                // 2: 进制，0: 负数符, 打印总长，要不要左对齐， 怎么补全， 大小写?
                OUTPUT(arg, buf, length);
                break;
			 case 'T': // 注意case和break
			 //	printf("here at T"); 
				if(type == 1) {
					
					// 0. 获取参数 va_arg 注意转型
					// 打印数字套路，1.  注意负数 2.length =  printNum 3. OUTPUT(arg, buf , length)
					struct s1 *s1_addr = (struct s1*)va_arg(ap, struct s1*); 
					length = PrintChar(buf, '{', width, ladjust);
                    OUTPUT(arg, buf, length);
			//		printf("here!%d", s1_addr->a); //
		//			printf("%d%d%d%d", s1_addr->a, s1_addr->b, s1_addr->c, s1_addr->d); 
					num = s1_addr->a; 
			 		if(num < 0) {
                    	num = - num;
                    	negFlag = 1;
                	}
					//ladjust = 0; 
					//padc = ' '; 默认 
					//num = va_arg(ap, int); 
					length = PrintNum(buf, num, 10,	negFlag, width, ladjust, padc, 0); 
					OUTPUT(arg, buf, length);
                  	
					// 打印 , 
					length = PrintChar(buf, ',', width, ladjust); 
                    OUTPUT(arg, buf, length);
					// 打印Char 
					char cc = s1_addr->b; 
					//c = (char)va_arg(ap, int);
                	length = PrintChar(buf, cc, width, ladjust);
                	OUTPUT(arg, buf, length);
					// 打印 ,
            		length = PrintChar(buf, ',', width, ladjust);
                    OUTPUT(arg, buf, length);
					// 打印 Char
					c = s1_addr->c;
					//c = (char)va_arg(ap, int);
                	length = PrintChar(buf, c, width, ladjust);
                	OUTPUT(arg, buf, length);
					// 打印 , 
            		length = PrintChar(buf, ',', width, ladjust);
                    OUTPUT(arg, buf, length);
					// 打印 Char
					num = s1_addr->d; 
		            if(num < 0) {    
                    	num = - num;
                    	negFlag = 1;
                	}
					//num = va_arg(ap, int);
                    length = PrintNum(buf, num, 10, negFlag, width, ladjust, padc, 0);
                    OUTPUT(arg, buf, length);

				} else if(type == 2) {
				
					struct s2 *s2_addr = (struct s2*)va_arg(ap,struct s2*);
					num = s2_addr->size;
					int *arr = s2_addr->c; 
					//p size
					if(num < 0) {
                        num = - num;
                        negFlag = 1;
                    } 
                    length = PrintNum(buf, num, 10, negFlag, width, ladjust, padc, 0);
                    OUTPUT(arg, buf, length); 
					//p comma 
					length = PrintChar(buf, ',', width, 0);
                	OUTPUT(arg, buf, length);

					long int cnt = num;
					int i; 
					for(i = 0; i < cnt; i++) {
						num = arr[i]; 
						if(num < 0) {
                        	num = - num;
                        	negFlag = 1;
                    	}
                    	length = PrintNum(buf, num, 10, negFlag, width, ladjust, padc, 0);
                    	OUTPUT(arg, buf, length); 
						if(i != num-1) {
						   //p comma 
                   		 length = PrintChar(buf, ',', width, 0); 
                    	OUTPUT(arg, buf, length);	
						}
					}
				}
				break;
			 /*case 'f':
			 	//printf("nih"); //
			 	if (prec == 0) prec = 6; 
				if (longFlag) {
					double para = va_arg(ap, double);
					if(para < 0) {
						printf("-"); 
						para = -para; 
					}
					int double_int = (int) para; 
					printf("%d", double_int); 
					printf("."); 
					para = para - double_int; 
					int i = 0;
					for ( i = 0; i < prec; i++) {
						para = para * 10; 
						double_int = (int) para; 
						printf("%d", double_int);
						para = para - double_int; 
					}
				} else {
					
				}
			 	break;*/
			 case 'd':
             case 'D':
                if (longFlag) { 
                num = va_arg(ap, long int);
                } else { 
                num = va_arg(ap, int); 
                }
                if(num < 0) {
                    num = - num; 
                    negFlag = 1; 
                }
                length = PrintNum(buf, num, 10, negFlag, width, ladjust, padc, 0);
                OUTPUT(arg, buf, length); 
                /*  Part2:
                    your code here.
                    Refer to other part (case 'b',case 'o' etc.) and func PrintNum to complete this part.
                    Think the difference between case 'd' and others. (hint: negFlag).
                */
                
                break;

             case 'o':
             case 'O':
                if (longFlag) { 
                num = va_arg(ap, long int);
                } else { 
                num = va_arg(ap, int); 
                }
                length = PrintNum(buf, num, 8, 0, width, ladjust, padc, 0);
                OUTPUT(arg, buf, length);
                break;
             case 'u':
             case 'U':                                                                       
                if (longFlag) {
                num = va_arg(ap, long int);
                } else { 
                num = va_arg(ap, int); 
                }
                length = PrintNum(buf, num, 10, 0, width, ladjust, padc, 0);
                OUTPUT(arg, buf, length);
                break;
                
             case 'x':
                if (longFlag) { 
                num = va_arg(ap, long int);
                } else { 
                num = va_arg(ap, int); 
                }
                length = PrintNum(buf, num, 16, 0, width, ladjust, padc, 0);
                OUTPUT(arg, buf, length);
                break;

             case 'X':
                if (longFlag) { 
                num = va_arg(ap, long int);
                } else { 
                num = va_arg(ap, int); 
                }
                length = PrintNum(buf, num, 16, 0, width, ladjust, padc, 1);
                OUTPUT(arg, buf, length);
                break;

             case 'c':
                c = (char)va_arg(ap, int);
                length = PrintChar(buf, c, width, ladjust);
                OUTPUT(arg, buf, length);
                break;

                        case 's':
                s = (char*)va_arg(ap, char *);
                length = PrintString(buf, s, width, ladjust);
                OUTPUT(arg, buf, length);
                break;

             case '\0':
                fmt --;
                break;

             default:
                /* output this char as it is */
                OUTPUT(arg, fmt, 1);
            }   /* switch (*fmt) */

            fmt ++;
    }       /* for(;;) */

    /* special termination call */                                                           
    OUTPUT(arg, "\0", 1);
}
/* --------------- local help functions --------------------- */
int
PrintChar(char * buf, char c, int length, int ladjust)
{
    int i;
    
    if (length < 1) length = 1;
    if (ladjust) {
	*buf = c;
	for (i=1; i< length; i++) buf[i] = ' ';
    } else {
	for (i=0; i< length-1; i++) buf[i] = ' ';
	buf[length - 1] = c;
    }
    return length;
}

int
PrintString(char * buf, char* s, int length, int ladjust)
{
    int i;
    int len=0;
    char* s1 = s;
    while (*s1++) len++;
    if (length < len) length = len;

    if (ladjust) {
	for (i=0; i< len; i++) buf[i] = s[i];
	for (i=len; i< length; i++) buf[i] = ' ';
    } else {
	for (i=0; i< length-len; i++) buf[i] = ' ';
	for (i=length-len; i < length; i++) buf[i] = s[i-length+len];
    }
    return length;
}

int
PrintNum(char * buf, unsigned long u, int base, int negFlag, 
	 int length, int ladjust, char padc, int upcase)
{
    /* algorithm :
     *  1. prints the number from left to right in reverse form.
     *  2. fill the remaining spaces with padc if length is longer than
     *     the actual length
     *     TRICKY : if left adjusted, no "0" padding.
     *		    if negtive, insert  "0" padding between "0" and number.
     *  3. if (!ladjust) we reverse the whole string including paddings
     *  4. otherwise we only reverse the actual string representing the num.
     */

    int actualLength =0;
    char *p = buf;
    int i;

    do {
	int tmp = u %base;
	if (tmp <= 9) {
	    *p++ = '0' + tmp;
	} else if (upcase) {
	    *p++ = 'A' + tmp - 10;
	} else {
	    *p++ = 'a' + tmp - 10;
	}
	u /= base;
    } while (u != 0);

    if (negFlag) {
	*p++ = '-';
    }

    /* figure out actual length and adjust the maximum length */
    actualLength = p - buf;
    if (length < actualLength) length = actualLength;

    /* add padding */
    if (ladjust) {
	padc = ' ';
    }
    if (negFlag && !ladjust && (padc == '0')) {
	for (i = actualLength-1; i< length-1; i++) buf[i] = padc;
	buf[length -1] = '-';
    } else {
	for (i = actualLength; i< length; i++) buf[i] = padc;
    }
	    

    /* prepare to reverse the string */
    {
	int begin = 0;
	int end;
	if (ladjust) {
	    end = actualLength - 1;
	} else {
	    end = length -1;
	}

	while (end > begin) {
	    char tmp = buf[begin];
	    buf[begin] = buf[end];
	    buf[end] = tmp;
	    begin ++;
	    end --;
	}
    }

    /* adjust the string pointer */
    return length;
}

