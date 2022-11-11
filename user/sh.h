/* Define color modes */
#define BOLD_GREEN(str) "\033[0;32;32m\033[1m" # str "\033[m"

#define RED(str) "\033[0;32;31m" # str "\033[m"

#define LIGHT_RED(str) "\033[1;31m" # str "\033[m"

#define GREEN(str) "\033[0;32;32m" # str "\033[m"

#define LIGHT_GREEN(str) "\033[1;32m" # str "\033[m"

#define BLUE(str) "\033[0;32;34m" # str "\033[m"

#define LIGHT_BLUE(str) "\033[1;34m" # str "\033[m"

#define DARK_GRAY(str) "\033[1;30m" # str "\033[m"

#define CYAN(str) "\033[0;36m" # str "\033[m"

#define LIGHT_CYAN(str) "\033[1;36m" # str "\033[m"

#define PURPLE(str) "\033[0;35m" # str "\033[m"

#define LIGHT_PURPLE(str) "\033[1;35m" # str "\033[m"

#define BROWN(str) "\033[0;33m" # str "\033[m"

#define YELLOW(str) "\033[1;33m" # str "\033[m"

#define LIGHT_GRAY(str) "\033[0;37m" # str "\033[m"

#define WHITE(str) "\033[1;37m" # str "\033[m"

#define MOVEDOWN(x) writef("\033[%dB", (x))

// 左移光标

#define MOVELEFT(y) writef("\033[%dD", (y))

// 右移光标

#define MOVERIGHT(y) writef("\033[%dC",(y))

// 定位光标

#define MOVETO(x,y) writef("\033[%d;%dH", (x), (y))

// 光标复位

#define RESET_CURSOR() writef("\033[H")

// 隐藏光标

#define HIDE_CURSOR() writef("\033[?25l")

// 显示光标

#define SHOW_CURSOR() writef("\033[?25h")

//反显

#define HIGHT_LIGHT() writef("\033[7m")

#define UN_HIGHT_LIGHT() writef("\033[27m")
