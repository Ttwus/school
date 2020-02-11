#include "main.h"
char timec[30]={0};

/*
功能：初始化
参数：无
返回值：0 成功
        -1 失败
*/
int kuai_init()
{
	//初始化
	Init_Font();
	lcd_fd=open(LCD_PATH,O_RDWR);
	if(-1 == lcd_fd)
	{
		perror("打开屏幕失败\n");
		return -1;
	}
	ts_fd=open(TS_PATH,O_RDWR);
	if(-1 == ts_fd)
	{
		perror("打开触摸屏失败\n");
		return -1;
	}
	
	lcdp=mmap(0,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcd_fd,0);
	lcdp1=(int (*)[800])lcdp;
/***************************************************************/
    close(lcd_fd);
/*****************************************************************/
	return 0;
}

/*
功能：关闭文件描述符
参数：无
返回值：0
*/
int kuai_exit()
{
	close(ts_fd);
	//close(lcd_fd);
	
    /***************************************************************/
    munmap(lcdp, 800*480*4);
    /***************************************************************/    
	UnInit_Font();
	return 0;
}

/*
功能：获取触摸屏坐标
参数：获取横坐标 x
        纵坐标 y
返回值：-1 失败
*/
int get_x_y(int *x,int *y)
{
	//获取触摸屏触摸时的x，y坐标
	struct input_event buf;
	int ret;
	*x=-1;
	*y=-1;
	
	while(1)
	{
		ret = read(ts_fd,&buf,sizeof(buf));
		if(ret < 0)
		{
			perror("read file fail");
			return -1;
		}
		if(buf.type == EV_ABS)
		{
			if(buf.code == ABS_X)
			{
				*x = buf.value;
				//printf("x=%d\n",buf.value);
				//printf("x=%d\n",*x);
			}
			if(buf.code == ABS_Y)
			{
				*y = buf.value;
			//	printf("y=%d\n",buf.value);
				//printf("y=%d\n",*y);
			}
		}
		if(*x>-1 && *y>-1)
			break;
		
	}

}

/*
功能：显示bmp图片
参数：bmppathname bmp文件位置
        x1   横轴偏移
        y1   纵轴偏移
        z    显示模式
返回值：-1      失败
*/
int show_bmp(char *bmppathname,int x1,int y1,int z)
{
	//显示图片
	int i,x,y,k;
	char bmp_head[54]={0};
	unsigned short w,h;
	int bmp_fd;
	int ret;
	
	bmp_fd=open(bmppathname,O_RDWR);
	if(bmp_fd == -1)
	{
		perror("open bmp fail\n");
		return -1;
	}
	
	read(bmp_fd,bmp_head,54);
	w = bmp_head[19] << 8 | bmp_head[18];
	h = bmp_head[23] << 8 | bmp_head[22];
	
	if(x1+w>800 || y1+h>480)
	{
		printf("over area\n");
		return -1;
	}
	
	char bmp_buf[w*h*3];
	int lcd_buf[w*h];
	ret = read(bmp_fd,bmp_buf,w*h*3);
	if(ret == -1)
	{
		printf("read bmp data fail\n");
		return -1;
	}
	
	for(i=0;i<w*h;i++)
	{
		lcd_buf[i] = bmp_buf[3*i+0] | bmp_buf[3*i+1]<<8 | bmp_buf[3*i+2]<<16 | 0x00<<24;
	}
	switch(z)
	{
		case 0://竖百叶窗显示
		for(x=0;x<(w/8);x++)
		{
			for(y=0;y<h;y++)
			{
				for(k=0;k<8;k++)
				{
					*(lcdp+800*(y+y1)+x+x1+k*w/8)= lcd_buf[(h-1-y)*w+x+k*w/8];
				}
			}
			usleep(2000);			
		}
		break;
		case 1://直接显示
		for(y=0;y<h;y++)
		{
			for(x=0;x<w;x++)
			{
				*(lcdp+800*(y+y1)+x+x1)= lcd_buf[(h-1-y)*w+x];
			}
		}
		usleep(10000);	
		break;
		
	}
	
}
void *showtime()
{
	//左上角显示时间
	while(1)
	{
		GetSysTime(timec);
		Clean_Area(0,0,330,35,0x00ffffff); 
		Display_characterX( 5, 5, timec, 0x00000000 , 2); 
		usleep(5000);
	} 
}
/*
功能：http   返回报文解析
参数：httpres        返回报文
返回值：报文信息长度
*/
int httpres_parse(char *httpres)
{
	int retcode = atoi(httpres + strlen("HTTP/1.x "));

	switch(retcode)
	{
	case 200:
		printf("success\n");
		break;
	case 206:
		printf("success\n");
		break;
	case 400 ... 499:
		printf("HTTP的请求报文有问题\n");
		show_bmp("./pho/fail.bmp",0,0,1);
		break;
	case 500 ... 599:
		printf("HTTP服务器有问题\n");
		show_bmp("./pho/fail.bmp",0,0,1);
		break;
	}

		return atoi(strstr(httpres, "Content-Length: ") + strlen("Content-Length: "));
}
int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;
	
	cd = iconv_open("gb2312","utf-8");
	if((iconv_t)-1 == cd)
	{
		perror("失败1\n");
		return -1;
	}
	memset(outbuf,0,outlen);
	if(iconv(cd,pin,&inlen,pout,&outlen)== -1)
	{
		perror("失败2\n");
		return -1;
	}
	iconv_close(cd);
	return 0;
}

/*
功能：把 fromcode 编码的字符串转换成 tocode 编码
参数：输入字符串 inbuf
        输入长度  inlen
        输出字符串  outbuf
        输出长度  outlen
返回值：0 成功
        -1 失败
*/
int ug2(char *inbuf,int inlen,char *outbuf,int outlen)
{
		return code_convert("UTF-8","GB2312",inbuf,inlen,outbuf,outlen);
}


/****************************************************************************************/
/* 
功能：显示api的ip
参数：hostent结构体
返回值：无
*/
void remote_info(struct hostent *he)
{
	assert(he);

	printf("主机的官方名称：%s\n", he->h_name);

	int i;
	for(i=0; he->h_aliases[i]!=NULL; i++)
	{
		printf("别名[%d]：%s\n", i+1, he->h_aliases[i]);
	}

	printf("IP地址长度：%d\n", he->h_length);

	for(i=0; he->h_addr_list[i]!=NULL; i++)
	{
		printf("IP地址[%d]：%s\n", i+1,
			inet_ntoa(*((struct in_addr **)he->h_addr_list)[i]));
	}
}

/*
功能：填充报文信息
参数：   ynum      快递单号
      company   快递公司
      buf       存储位置  
      size      buf大小
返回值：无
*/
void http_request(char *ynum, char *company, char*buf, int size)
{
    assert(buf);
    printf("/*****************************************/\n");
    printf("快递单号:%s\t快递:%s\n",ynum,company);

    // 构建一条 HTPP 请求报文
    bzero(buf, size);
    snprintf(buf, size, "GET /express/trace/query?comid=%s&number=%s HTTP/1.1\r\n"
                "Host:api09.aliyun.venuscn.com\r\n"
                "Authorization:APPCODE 02f9bc1466434e48907af7dcd9c75bac\r\n\r\n",strtok(company, "\n"), strtok(ynum, "\n"));
}

/*
功能：解析显示Json数据
参数：报文返回信息buf
返回值：无
*/
/*void show_weather_info(char *json)
{
	//解析JSON字符串
	cJSON *root     = cJSON_Parse(json);//从给定的json字符串中得到cjson对象
	//获取json对象（获取对应的值）
	////在主体中找到“show_res_body”这个大结构体
	cJSON *data   = cJSON_GetObjectItem(root, "data");
	//在大结构体总中找到数组“dayList” 
	cJSON *traces    = cJSON_GetObjectItem(data, "traces");

    cJSON *number    = cJSON_GetObjectItem(data, "number");
    cJSON *com       = cJSON_GetObjectItem(data, "com");

	//printf("快递：%s\n单号：%s\n", cJSON_GetObjectItem(data, "com")->valuestring, cJSON_GetObjectItem(data, "number")->valuestring);
    printf("快递：%s\n单号：%s\n", com->valuestring, number->valuestring);
	//提取json对象数组成员的个数（dayList这个数组中的元素个数）
	int size = cJSON_GetArraySize(traces);
	cJSON *a = NULL;
    int j=0;
	cJSON *jiedian;
	cJSON *time;
	cJSON *stat;
	int yyy1;
	int yyy2;
	int xxx;
	char out1[OUTLEN];
	char out2[OUTLEN];
	int rc;
                                
	for(int i=0; i<size; i++)
	{	
		//根据下标获取cjson对象数组中的对象
		a = cJSON_GetArrayItem(traces, i);//获取数组中的元素
		//获取字符串型json对象的值  （一组）
		printf("时间：%s\n", cJSON_GetObjectItem(a, "time")->valuestring);
		printf("地点：%s\n", cJSON_GetObjectItem(a, "desc")->valuestring);
        
		rc = ug2(time->valuestring,strlen(time->valuestring),out1,OUTLEN);
		Display_characterX( xxx, yyy1,out1, 0x00000000 , 1);
		rc = ug2(desc->valuestring,strlen(desc->valuestring),out2,OUTLEN);
		Display_characterX( xxx, yyy2, out2, 0x00000000 , 1);
	}
}*/
void show_weather_info(char *json)
{
    //解析JSON字符串
        cJSON *root     = cJSON_Parse(json);//从给定的json字符串中得到cjson对象
        //获取json对象（获取对应的值）
        ////在主体中找到“data”这个大结构体
        cJSON *data   = cJSON_GetObjectItem(root, "data");
        //在大结构体总中找到数组“traces” 
        cJSON *traces    = cJSON_GetObjectItem(data, "traces");
    
        cJSON *number    = cJSON_GetObjectItem(data, "number");
        cJSON *com       = cJSON_GetObjectItem(data, "com");
        
        int arraySize = cJSON_GetArraySize(traces);
        int j=0;
        cJSON *jiedian;
        cJSON *time;
        cJSON *desc;
        int yyy1;
        int yyy2;
        int xxx;
        char out1[OUTLEN];
        char out2[OUTLEN];
        int rc;

        while(1)//退出键跳出物流打印
        {
            int n=0;
            int m=0;
            show_bmp("./pho/wuliu1.bmp",0,0,1);
            //for (j = 0; j <= arraySize; j++)
            for (j = 0; j < arraySize; j++)
            {
                if( arraySize==0 )
                {
                    Display_characterX( 330, 80,"暂无物流信息", 0x00000000 , 2);
                    goto lab1;
                }
                //rc = ug2(com->valuestring,strlen(com->valuestring),out1,OUTLEN);
                //Display_characterX( 330, 80,out1, 0x00000000 , 2);
                 rc = ug2(com->valuestring,strlen(com->valuestring),out1,OUTLEN);
                Display_characterX( 200, 80,out1, 0x00000000 , 2);
                 rc = ug2(number->valuestring,strlen(number->valuestring),out2,OUTLEN);
                Display_characterX( 330, 80,out2, 0x00000000 , 2);
                n++;
                if(j<8)
                {
                    //第一页快递信息
                    xxx=60;
                    yyy1=70+n*40;
                    yyy2=yyy1+20;
                    jiedian=cJSON_GetArrayItem(traces,j);
                    time=cJSON_GetObjectItem(jiedian,"time");
                    desc=cJSON_GetObjectItem(jiedian,"desc");
                    //printf("%s\n",time->valuestring );
                    //printf("%s\n",stat->valuestring );
                    rc = ug2(time->valuestring,strlen(time->valuestring),out1,OUTLEN);
                    Display_characterX( xxx, yyy1,out1, 0x00000000 , 1);
                    rc = ug2(desc->valuestring,strlen(desc->valuestring),out2,OUTLEN);
                    Display_characterX( xxx, yyy2, out2, 0x00000000 , 1);
                    if( (j+1)==arraySize )//表示已经显示完所有物流信息
                    {
                        goto lab1;
                    }
                }
                else
                {
                    get_x_y(&x,&y);//一直等待触摸显示
                    if(x>663*1.28 && y>90*1.25 && x<746*1.28 && y<117*1.25)//第二页快递信息
                    {
                        goto lab;
                    }
                    else
                        break;
                }
            }

            lab1:
                get_x_y(&x,&y);
                if(x>552*1.28 && y>90*1.25 && x<647*1.28 && y<120*1.25 && arraySize>8)//上一页（只在有第二页是存在）
                {
                    continue;
                }
                if(x>670*1.28 && y>410*1.25 && x<778*1.28 && y<458*1.25)//退出
                {
                    break;
                }

            /****************************************/
                //位置需要重新修改（想法：添加跳转条件）
                if(x>663*1.28 && y>90*1.25 && x<746*1.28 && y<117*1.25 && arraySize>8)//第二页快递信息
                {
                    lab:
                        show_bmp("./pho/wuliu3.bmp",0,0,1);
                        //show_bmp("./pho/wuliu2.bmp",0,0,1);
                        for(j=8;j<arraySize;j++)
                        {
                            m++;
                            xxx=60;
                            yyy1=70+m*40;
                            yyy2=yyy1+20;
                            jiedian=cJSON_GetArrayItem(traces,j);
                            time=cJSON_GetObjectItem(jiedian,"time");
                            desc=cJSON_GetObjectItem(jiedian,"desc");
                            //printf("%s\n",time->valuestring );
                            //printf("%s\n",stat->valuestring );
                            rc = ug2(time->valuestring,strlen(time->valuestring),out1,OUTLEN);
                            Display_characterX( xxx, yyy1,out1, 0x00000000 , 1);
                            rc = ug2(desc->valuestring,strlen(desc->valuestring),out2,OUTLEN);
                            Display_characterX( xxx, yyy2, out2, 0x00000000 , 1);
                        }
                    m=0;
                }
                else
                {
                    goto  lab1;
                }

        }
                
                return;

}
/*******************************************************************************************/

/*
功能：发送查询请求报文
参数：无
返回值：0 成功
*/
int get_k_message()
{
	/******************************************************************************************/
    
    // 1，根据所选择的API获取其服务器IP
	char *host = "api09.aliyun.venuscn.com";
	struct hostent *he = gethostbyname(host); //用域名或主机名获取IP地址
	if(he == NULL)
	{
		errno = h_errno;
		perror("gethostbyname() failed");
		exit(0);
	}

    struct in_addr **addr_list = (struct in_addr **)(he->h_addr_list);

    remote_info(he);
    
    /******************************************************************************************/
    struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	bzero(&addr, len);

	addr.sin_family = AF_INET;
	//addr.sin_addr.s_addr   =inet_addr("112.124.225.197");//地址需要修改
	addr.sin_port   = htons(80);//port == 80
	
    /******************************************************************************************/
	addr.sin_addr   = *addr_list[0];
    /******************************************************************************************/
	
	// 创建TCP套接字(因为HTTP是基于TCP的)，并发起连接
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(connect(fd, (struct sockaddr *)&addr, len) == 0)
	{
		printf("连接服务器成功！\n");
	}
    else
    {
        perror("connect() failed");
        show_bmp("./pho/fail.bmp",0,0,1);
		sleep(2);
    }
	int k_bg_flag=1;//快递标志位
	int if_flag=0;//快递选取标志位
	int dh_flag=1;//单号标志位
	char *company;//临时存放物流商家
	while(1)
	{
		//int if_flag=0;//快递选取标志位
		if_flag=0;
		if(k_bg_flag==1)
		{
			show_bmp("./pho/k_bg.bmp",0,0,0);//快递选择按钮
			k_bg_flag=0;
		}
		//printf("你要查哪个快递公司？\n");
		//char *company;
		get_x_y(&x,&y);
		if(x>100*1.28 && y>100*1.25 && x<200*1.28 && y<200*1.25)//中通
		{
			if_flag=1;
			//company="zto";
            company="ZTO";
		}
		if(x>250*1.28 && y>100*1.25 && x<350*1.28 && y<200*1.25)//申通
		{
			if_flag=2;
			company="STO";
		}
		if(x>400*1.28 && y>100*1.25 && x<500*1.28 && y<200*1.25)//天天快递
		{
			if_flag=3;
			company="TTKDEX";
		}
		if(x>550*1.28 && y>100*1.25 && x<650*1.28 && y<200*1.25)//圆通
		{
			if_flag=4;
			company="YTO";
		}
		if(x>100*1.28 && y>250*1.25 && x<200*1.28 && y<350*1.25)//韵达
		{
			if_flag=5;
			company="YUNDA";
		}
		if(x>250*1.28 && y>250*1.25 && x<350*1.28 && y<350*1.25)//京东
		{
			if_flag=6;
			company="JD";
		}
		if(x>400*1.28 && y>250*1.25 && x<500*1.28 && y<350*1.25)//其他
		{
			if_flag=7;
			company="";
		}
		if(x>650*1.28 && y>400*1.25 && x<750*1.28 && y<450*1.25)//退出
		{
			if_flag=0;
			break;
		}
		if(if_flag>0)
		{

				//int dh_flag=1;//单号标志位
				dh_flag=1;//单号标志位
				lab2:
				if(dh_flag==1)
				{
					show_bmp("./pho/danhao.bmp",0,0,0);
					dh_flag=0;
				}
				//printf("请输入快递单号:\n");
				char num[20];	
				char *lan="输入单号错误，重新输入";
				char lan1[100]={0};
				ug2(lan,strlen(lan),lan1,100);
				bzero(num, 20);
				int i=0;
				while(1)
				{
					show_bmp("./pho/danhao.bmp",0,0,1);
					Display_characterX( 51, 120, num, 0x00000000 , 2);
					get_x_y(&x,&y);//坐标获取
					//if((x>60*1.28&&y>200*1.25&&x<340*1.28 && y<404*1.25)||(x>150*1.28 && y>404*1.25 && x<240*1.28 && y<470*1.25))
						
						if(x>60*1.28 && y>200*1.25 && x<150*1.28 && y<270*1.25)//1
						{
							num[i]='1';
							i++;
							//continue;
						}
						if(x>150*1.28 && y>200*1.25 && x<240*1.28 && y<270*1.25)//2
						{
							num[i]='2';
							i++;
							//continue;
						}
						if(x>240*1.28 && y>200*1.25 && x<340*1.28 && y<270*1.25)//3
						{
							num[i]='3';
							i++;
							//continue;
						}
						if(x>60*1.28 && y>270*1.25 && x<150*1.28 && y<340*1.25)//4
						{
							num[i]='4';
							i++;
							//continue;
						}
						if(x>150*1.28 && y>270*1.25 && x<240*1.28 && y<340*1.25)//5
						{
							num[i]='5';
							i++;
							//continue;
						}
						if(x>240*1.28 && y>270*1.25 && x<340*1.28 && y<340*1.25)//6
						{
							num[i]='6';
							i++;
							//continue;
						}
						if(x>60*1.28 && y>340*1.25 && x<150*1.28 && y<404*1.25)//7
						{
							num[i]='7';
							i++;
							//continue;
						}
						if(x>150*1.28 && y>340*1.25 && x<240*1.28 && y<404*1.25)//8
						{
							num[i]='8';
							i++;
							//continue;
						}
						if(x>240*1.28 && y>340*1.25 && x<340*1.28 && y<404*1.25)//9
						{
							num[i]='9';
							i++;
							//continue;
						}
						if(x>150*1.28 && y>404*1.25 && x<240*1.28 && y<470*1.25)//0
						{
							num[i]='0';
							i++;
							//continue;
						}
						if(x>240*1.28 && y>404*1.25 && x<340*1.28 && y<470*1.25)
						{
							i--;
							num[i]='\0';
							if(i<0)
								i=0;
							continue;
						}
						
						if(i>15)
						{
							Display_characterX( 65, 150, lan1, 0x00000000 , 2);
							
							get_x_y(&x,&y);
							dh_flag=1;
							goto lab2;
						}
						
					
						if(x>670*1.28 && y>410*1.25 && x<778*1.28 && y<460*1.25)//退出
						{
							k_bg_flag=1;
							break;
						}
						if(x>60*1.28 && y>404*1.25 && x<150*1.28 && y<470*1.25)//重新输入
						{
							i=0;
							bzero(num,20);
							continue;
						}
						
						if(x>370*1.28 && y>110*1.25 && x<450*1.28 && y<145*1.25)//确认
						{
							char ynum[50]={0};
							while(1)
							{
								if(if_flag==6)
								{
									sprintf(ynum,"JD%s",num);
									//printf("%s\n",ynum);
								}
								else
								{
									strcpy(ynum,num);
									//printf("%s\n",ynum);
								}
								show_bmp("./pho/wuliu1.bmp",0,0,1);
                           #if 0
								static char request[1024];

								snprintf(request, 1024, "GET /kdi?no=%s&type=%s HTTP/1.1\r\n"
									"Host:wuliu.market.alicloudapi.com\r\n"
								"Authorization:APPCODE cd8422b163114052820cd129cd4745d7\r\n\r\n",strtok(ynum, "\n"),strtok(company, "\n"));
                                // 准备好HTTP的请求报文
								//char *s = httprequest();
								bzero(ynum, 50);
								write(fd, request, strlen(request));
								//printf("%s\n",request );
                           #endif
                                /****************************************************************/                                
                                
                                // 编辑HTTP请求报文
                                static char request[1024];
                                memset(request, 0, sizeof(request));
                                http_request(ynum, company, request, 1024);  
                                bzero(ynum, 50);
                                
								//write(fd, request, strlen(request));
                                send(fd, request, strlen(request), 0);

                                char *recvbuf = calloc(1, 4096);
                            	int m = 0; // 头部的大小

                            	// 等待接收 HTTP 响应头部（头部一定以\r\n\r\n结束）
                            	while(1)	
                            	{
                            	    int n = recv(fd, recvbuf+m, 1, 0);
                            		if(n == -1)
                            		{
                            			perror("recv() failed");
                                        show_bmp("./pho/fail_1.bmp",0,0,1);
										sleep(2);
										return 0;
                            		}
                            		m += n;

                            		if(strstr(recvbuf, "\r\n\r\n"))
                            			break;
                            	}

                                // 分析头部信息，比如响应码、正文长度……
                                int totalsize = httpres_parse(recvbuf);
                            	char *s = "Content-Length: ";
                            	char *p = strstr(recvbuf, s);
                            	int json_len;
                            	if(p != NULL)
                            	{
                            		json_len = atoi(p+strlen(s));
                            	}
                                free(recvbuf);//释放申请内存
                            	printf("正文的长度: %d\n", json_len);

                            	// 接收正文（JSON数据）
                            	char *json = calloc(1, json_len);	

                            	int total = 0;
                            	while(json_len > 0)
                            	{
                            		int n = read(fd, json+total, json_len);
                            		json_len -= n;
                            		total += n;
                            	}
                                /****************************************************************/                                
                           #if 0
								// 接收对方的响应头部
								char res[2048];
								int total = 0;
								while(1)
								{
									int n = read(fd, res+total, 1);
			
									if(n <= 0)
									{
										perror("读取HTTP头部失败");
										show_bmp("./pho/fail.bmp",0,0,1);
										sleep(2);
										return 0;
									}
	
									total += n;

									if(strstr(res, "\r\n\r\n"))
									break;
								}

								//printf("响应报文头部: %s\n", res);
		
                           
								// 分析正文的长度
								int totalsize = httpres_parse(res);
								total = 0;
								int ret =0;
	
								bzero(res,2048);
								while(totalsize)
								{
		
									ret = read (fd, res+total, 2048);
			
									totalsize -=ret ;
									total +=ret ;
								}
                           #endif
                                /********************************************************/
                                // 解析 JSON 数据，输出物流信息
                                show_weather_info(json);
                                free(json);
                                /**********************************************************/
                                #if   0
								cJSON *root=cJSON_Parse(res);//JSON数据解析
								//获取json对象（获取对应的值）
								cJSON *result = cJSON_GetObjectItem(root,"result");//在主体中找到“result”这个大结构体
								cJSON *expName = cJSON_GetObjectItem(result,"expName");//在大结构体总中找到数组“expName” 
								cJSON *list = cJSON_GetObjectItem(result,"list");//在大结构体中找到数组“list” 
                                
                                //提取json对象数组成员的个数（dayList这个数组中的元素个数）
								int i = cJSON_GetArraySize(list);
								//printf("list=%d\n",i);
								int j=0;
								cJSON *jiedian;
								cJSON *time;
								cJSON *stat;
								int yyy1;
								int yyy2;
								int xxx;
								char out1[OUTLEN];
								char out2[OUTLEN];
								int rc;
						
								while(1)//退出键跳出物流打印
								{
									int n=0;
									int m=0;
									show_bmp("./pho/wuliu1.bmp",0,0,1);
									for (j = 0; j <= i; j++)
									{
										if( i==0 )
										{
										    Display_characterX( 330, 80,"暂无物流信息", 0x00000000 , 2);
											goto lab1;
										}
										rc = ug2(expName->valuestring,strlen(expName->valuestring),out1,OUTLEN);
										Display_characterX( 330, 80,out1, 0x00000000 , 2);
										n++;
										if(j<8)
										{
										
											//第一页快递信息
											xxx=60;
											yyy1=70+n*40;
											yyy2=yyy1+20;
											jiedian=cJSON_GetArrayItem(list,j);
											time=cJSON_GetObjectItem(jiedian,"time");
											stat=cJSON_GetObjectItem(jiedian,"status");
											//printf("%s\n",time->valuestring );
											//printf("%s\n",stat->valuestring );
											rc = ug2(time->valuestring,strlen(time->valuestring),out1,OUTLEN);
											Display_characterX( xxx, yyy1,out1, 0x00000000 , 1);
											rc = ug2(stat->valuestring,strlen(stat->valuestring),out2,OUTLEN);
											Display_characterX( xxx, yyy2, out2, 0x00000000 , 1);
											if( j==i )//表示已经显示完所有物流信息
											{
												goto lab1;
											}
										}
										else
										{
											get_x_y(&x,&y);//一直等待触摸显示
											if(x>663*1.28 && y>90*1.25 && x<746*1.28 && y<117*1.25)//第二页快递信息
											{
												goto lab;
											}
                                            else
                                                break;
										}
									}
							        /****************************************/
                                    /*
                                    //想法：lab 跳转需要一定条件  
									lab:
										show_bmp("./pho/wuliu3.bmp",0,0,1);
                                        //show_bmp("./pho/wuliu2.bmp",0,0,1);
										for(j=8;j<i;j++)
										{
											m++;
											xxx=60;
											yyy1=70+m*40;
											yyy2=yyy1+20;
											jiedian=cJSON_GetArrayItem(list,j);
											time=cJSON_GetObjectItem(jiedian,"time");
											stat=cJSON_GetObjectItem(jiedian,"status");
											//printf("%s\n",time->valuestring );
											//printf("%s\n",stat->valuestring );
											rc = ug2(time->valuestring,strlen(time->valuestring),out1,OUTLEN);
											Display_characterX( xxx, yyy1,out1, 0x00000000 , 1);
											rc = ug2(stat->valuestring,strlen(stat->valuestring),out2,OUTLEN);
											Display_characterX( xxx, yyy2, out2, 0x00000000 , 1);
										}
									m=0;
                                        */
                                       /****************************************/
									lab1:
										get_x_y(&x,&y);
										if(x>552*1.28 && y>90*1.25 && x<647*1.28 && y<120*1.25 && i>8)//上一页（只在有第二页是存在）
										{
											continue;
										}
    									if(x>670*1.28 && y>410*1.25 && x<778*1.28 && y<458*1.25)//退出
    									{
    										break;
    									}

                                    /****************************************/
                                        //位置需要重新修改（想法：添加跳转条件）
                                        if(x>663*1.28 && y>90*1.25 && x<746*1.28 && y<117*1.25 && i>8)//第二页快递信息
                                        {
                                            lab:
        										show_bmp("./pho/wuliu3.bmp",0,0,1);
                                                //show_bmp("./pho/wuliu2.bmp",0,0,1);
        										for(j=8;j<i;j++)
        										{
        											m++;
        											xxx=60;
        											yyy1=70+m*40;
        											yyy2=yyy1+20;
        											jiedian=cJSON_GetArrayItem(list,j);
        											time=cJSON_GetObjectItem(jiedian,"time");
        											stat=cJSON_GetObjectItem(jiedian,"status");
        											//printf("%s\n",time->valuestring );
        											//printf("%s\n",stat->valuestring );
        											rc = ug2(time->valuestring,strlen(time->valuestring),out1,OUTLEN);
        											Display_characterX( xxx, yyy1,out1, 0x00000000 , 1);
        											rc = ug2(stat->valuestring,strlen(stat->valuestring),out2,OUTLEN);
        											Display_characterX( xxx, yyy2, out2, 0x00000000 , 1);
        										}
        									m=0;
                                        }
                                        else
                                        {
                                            goto  lab1;
                                        }
                                    /***************************************/
                                    //goto lab;
								}
                                #endif
								break;//跳出物流报文显示
							}
							break;//跳出单号填写
						}
						
						continue;
				}
					
				//dh_flag=1;
                //这里应该不需要
				//continue;
		}
		k_bg_flag=1;
		continue;	
	}
	return 0;
}
int main()
{
	
	int num,bg_flag=1;
	pthread_t thread;
	int ret;
	char p_path[50]={0};
	kuai_init();//初始化
	//开机动画
	Clean_Area(0,0,800,480,0x00000000); //开机画面清空
	for(num=1;num<106;num++)//时间（大概3s）
	{
	//图片位置修改，开机画面图片存放,画面修改，中心画面放图标大小200*200
		sprintf(p_path,"/liuchen/star/%d.bmp",num);

        /*sprintf(p_path,"/liuchen/fiveday/RFID/pho/star/%d.bmp",num);//图片位置修改*/
		show_bmp(p_path,310,150,1);
        //参数配置：namephotopath，x偏移位置，y偏移位置，显示mode选择（0：百叶窗）
	}
	
	pthread_create(&thread,NULL,showtime,NULL);//个人认为这个线程不太需要（可能是为了美化）
	
	while(1)
	{
		if(bg_flag==1)
		{
			show_bmp("./pho/bg.bmp",0,0,0); //图片显示
			bg_flag=0;			
		}
		get_x_y(&x,&y);
		if(x>200*1.28 && y>140*1.25 && x<600*1.28 && y<340*1.25)
		{
			 ret=get_k_message();//socket套接字修改，主执行内容
			 if(ret == 0)
			 {
				 bg_flag=1;
				 continue;
			 }
		}
        //直接做位置判断（退出按键位置）
		if(x>650*1.28 && y>30*1.25 && x<760*1.28 && y<70*1.25)
		{
			break;
		}
		else
		{
			bg_flag=1;
			continue;
		}
	
	}

	//关机动画
	Clean_Area(0,0,800,480,0x00000000); //清空界面信息
	for(num=1;num<84;num++)
	{
	//位置修改，文件图片大小寻找
		sprintf(p_path,"/liuchen/end/%d.bmp",num);
		show_bmp(p_path,300,140,1);//关机图片大小设置
	}
	Clean_Area(0,0,800,480,0x00000000); 
	
	kuai_exit();//退出前关闭所有
	return 0;
}
