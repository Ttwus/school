#include "main.h"
char timec[30]={0};

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
	return 0;
}
int kuai_exit()
{
	close(ts_fd);
	close(lcd_fd);
	UnInit_Font();
	return 0;
}
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
int ug2(char *inbuf,int inlen,char *outbuf,int outlen)
{
		return code_convert("UTF-8","GB2312",inbuf,inlen,outbuf,outlen);
}

int get_k_message()
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	bzero(&addr, len);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr   =inet_addr("112.124.225.197");
	addr.sin_port   = htons(80);

	// 创建TCP套接字(因为HTTP是基于TCP的)，并发起连接
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(connect(fd, (struct sockaddr *)&addr, len) == 0)
	{
		printf("连接服务器成功！\n");
	}
	int k_bg_flag=1;
	while(1)
	{
		int if_flag=0;
		if(k_bg_flag==1)
		{
			show_bmp("./pho/k_bg.bmp",0,0,0);
			k_bg_flag=0;
		}
		//printf("你要查哪个快递公司？\n");
		char *company;	
		get_x_y(&x,&y);
		if(x>100*1.28 && y>100*1.25 && x<200*1.28 && y<200*1.25)//中通
		{
			if_flag=1;
			company="zto";
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
			
				
				int dh_flag=1;
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
					get_x_y(&x,&y);
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
								static char request[1024];

								snprintf(request, 1024, "GET /kdi?no=%s&type=%s HTTP/1.1\r\n"
									"Host:wuliu.market.alicloudapi.com\r\n"
								"Authorization:APPCODE cd8422b163114052820cd129cd4745d7\r\n\r\n",strtok(ynum, "\n"),strtok(company, "\n"));
								//// 准备好HTTP的请求报文广州
								//char *s = httprequest();
								bzero(ynum, 50);
								write(fd, request, strlen(request));
								//printf("%s\n",request );
			

								// // 接收对方的响应头部
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
								cJSON *root=cJSON_Parse(res);//JSON数据解析
								cJSON *result = cJSON_GetObjectItem(root,"result");
								cJSON *expName = cJSON_GetObjectItem(result,"expName");
								cJSON *list = cJSON_GetObjectItem(result,"list");
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
						
								while(1)
								{
									int n=0;
									int m=0;
									show_bmp("./pho/wuliu1.bmp",0,0,1);
									for (j = 0; j <= i; j++)
									{
										if( i==0 )
										{
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
											if( j==i )
											{
												goto lab1;
											}
										}
										else
										{
											get_x_y(&x,&y);
											if(x>663*1.28 && y>90*1.25 && x<746*1.28 && y<117*1.25)//第二页快递信息
											{
											
												goto lab;
											}
										}
									}
							
									lab:
										show_bmp("./pho/wuliu2.bmp",0,0,1);
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
									lab1:
										get_x_y(&x,&y);
										if(x>552*1.28 && y>90*1.25 && x<647*1.28 && y<120*1.25)//上一页
										{
											continue;
										}
									if(x>670*1.28 && y>410*1.25 && x<778*1.28 && y<458*1.25)//退出
									{
										break;
									}
									goto lab;
								}
								break;
							}
							break;
						}
						
						continue;
						
				}
					
				dh_flag=1;
				continue;
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
	Clean_Area(0,0,800,480,0x00000000); 
	for(num=0;num<105;num++)
	{
		sprintf(p_path,"/liuchen/fiveday/RFID/pho/star/%d.bmp",num);
		show_bmp(p_path,310,150,1);
	}
	
	pthread_create(&thread,NULL,showtime,NULL);
	
	while(1)
	{
		if(bg_flag==1)
		{
			show_bmp("./pho/bg.bmp",0,0,0);
			bg_flag=0;			
		}
		get_x_y(&x,&y);
		if(x>200*1.28 && y>140*1.25 && x<600*1.28 && y<340*1.25)
		{
			 ret=get_k_message();
			 if(ret == 0)
			 {
				 bg_flag=1;
				 continue;
			 }
		}
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
	Clean_Area(0,0,800,480,0x00000000); 
	for(num=0;num<83;num++)
	{
		sprintf(p_path,"/liuchen/fiveday/RFID/pho/lion/%d.bmp",num);
		show_bmp(p_path,300,140,1);
	}
	Clean_Area(0,0,800,480,0x00000000); 
	
	kuai_exit();//退出前关闭所有
	return 0;
}