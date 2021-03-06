//把系统中常用的头文件全部都包含，然后丢到系统头文件的环境变量中/usr/include，那么以后就可以将myhead.h当成系统提供的一个头文件

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/input.h>//跟输入子系统模型有关的头文件
#include <sys/mman.h>
#include <sys/wait.h>//跟wait有关
#include <signal.h>//跟信号有关的头文件
#include <sys/shm.h>//跟共享内存有关的头文件
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <poll.h>
#include <termios.h>

#include <stddef.h>
#include <iconv.h>

#include "sqlite3.h"
#include "font.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "cJSON.h"

#define LCD_PATH "/dev/fb0"
#define TS_PATH "/dev/input/event0"
#define RFID_DB "rfid.db"
#define OUTLEN 1024
int *lcdp;
int (*lcdp1)[800];
int lcd_fd,ts_fd;
int x,y;

int httpres_parse(char *httpres);
int ug2(char *inbuf,int inlen,char *outbuf,int outlen);
int get_k_message();
int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen);