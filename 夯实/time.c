#include "main.h"

char* GetSysTime(char *out)
{
	time_t timep;
	struct tm *p;
	
	time(&timep);
	p=localtime(&timep);
	
	int year=(1900+p->tm_year);
	int month=(1+p->tm_mon);
	int day=p->tm_mday;
	int hour=p->tm_hour;
	int minute=p->tm_min;
	int second=p->tm_sec;
	
	//2019-07-09-09-07-30
	sprintf(out,"%d-",year);
	if(month<10)  
		sprintf(out+5,"0%d-",month);   
	else 
		sprintf(out+5,"%d-",month);
	if(day<10)    
		sprintf(out+8,"0%d ",day);    
 	else 
		sprintf(out+8,"%d ",day);
	if(hour<10)  
		sprintf(out+11,"0%d:",hour);   
	else 
		sprintf(out+11,"%d:",hour);
	if(minute<10) 
		sprintf(out+14,"0%d:",minute); 
	else 
		sprintf(out+14,"%d:",minute);
	if(second<10) 
		sprintf(out+17,"0%d",second);  
	else 
		sprintf(out+17,"%d",second);
	return out;
}