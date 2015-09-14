/**********************************************************************
* File:        tessedit.cpp  (Formerly tessedit.c)
* Description: Main program for merge of tess and editor.
* Author:                  Ray Smith
* Created:                 Tue Jan 07 15:21:46 GMT 1992
*
* (C) Copyright 1992, Hewlett-Packard Ltd.
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
** http://www.apache.org/licenses/LICENSE-2.0
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*
**********************************************************************/

// Include automatically generated configuration file if running autoconf
#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif
 
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif  // _WIN32
#include <iostream>

#include "allheaders.h"
#include "baseapi.h"
#include "basedir.h"
#include "renderer.h"
#include "strngs.h"
#include "tprintf.h"
#include "openclwrapper.h"
#include "osdetect.h"
#include <time.h>
#include <sys/time.h> 
#include <pthread.h>   
#include <vector>   
#include <string>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <unistd.h>  
#include <signal.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/prctl.h>

#include "consoleappender.h"
#include "layout.h"
#include "logger.h"
#include "fileappender.h"
#include "loggingmacros.h"

#define OCR_THREAD_NUM 4  //预初始化的识别线程池线程数
#define SERVERVERSION  true  //忽略
using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;
 
pthread_mutex_t ImgVecLock;  /****请求队列（图片队列）的同步锁，
                                  1、用于同步多个处理线程之间对于请求队列的写入图片操作；
                                  2、用于同步多个识别线程之间对于请求队列的读出图片操作；
                                  3、用于同步处理线程与识别线程之间的写入、读出操作；
                             *****/

pthread_mutex_t RespVecLock;  /****响应队列（识别结果队列）的同步锁，
                                  1、用于同步多个识别线程之间对于响应队列的写入识别结果操作；
                                  2、用于同步多个处理线程之间对于响应队列的读出识别结果操作；
                                  3、用于同步识别线程与处理线程之间的写入、读出操作；
                             *****/

pthread_cond_t ImgVecCond;  /****请求队列（图片队列）的条件变量，
                                 1、用于在处理线程往请求队列写入新的图片请求时，通知识别线程（有新请求到，请作识别）；
                                 2、用于在请求队列为空的时候，挂起识别线程，以免线程跑空循环；
                             *****/ 
                             
pthread_cond_t RespVecCond; /****响应队列（识别结果队列）的条件变量，
                                 1、用于在识别线程往响应队列写入新的识别结果时，通知处理线程（有新结果到，请对应的处理线程查收）；
                                 2、用于在识别结果队列为空的时候，挂起处理线程，以免线程跑空循环；
                             *****/ 
Logger _logger ;      //第三方库log4plus日志对象，全局变量，在main函数中设置及初始化
char * TessDataPath;  //全局变量，存储当前程序所在目录，用于语言包及日志存放目录的寻径



/****
请求结果体
ImgID  请求唯一标识ID,用于与响应队列中的识别结果进行匹配，现取线程ID；
ImgPixs 图片解码后的像素对象；
Language 用于尝试识别的语言包，可选为全部、英文、中文；
****/
struct ImgInfo
{
	unsigned long int ImgID;
	Pix*   ImgPixs;
	int    Language;
};


/****
ImgID  响应唯一标示ID，对应于ImgInfo的ImgID；
Resp   识别结果字符串；
****/
struct RespInfo
{
	unsigned long int ImgID;
	char   *Resp;
};



vector<ImgInfo>  VecImg;  //请求队列（ImgInfo队列）
vector<RespInfo> VecResp; //响应队列（RespInfo队列）


//将当前进程变成守护进程
void init_daemon()
{
    int pid, i;
    if(pid = fork())
    {
        exit(0);
    }else if(pid < 0)
    {
        perror("fork error!\n");
        exit(EXIT_FAILURE);
    }
    
    setsid();
   
    for(i = 0; i < NOFILE; i++)
        close(i);
    chdir("/");
    umask(0);
    signal(SIGCHLD,SIG_IGN);
    return;
}


//将识别结果打包成HTTP报文
char * Creat_json(int aiErrorCode,const char * apcMessage)
{
    
    char *send_str=(char*) malloc(2048*64);
    memset(send_str,0,sizeof(send_str));
    //头信息
    strcat(send_str, "HTTP/1.1 200 OK\r\n");
    strcat(send_str, "Date: Thu, 25 Sep 2014 06:45:28 GMT\r\n");
    strcat(send_str, "Server: Apache/2.4.10 (Linux) OpenSSL/1.0.1h PHP/5.4.31\r\n");
    strcat(send_str, "X-Powered-By: PHP/5.4.31\r\n");
    char content_header[100];
    char json_str[2048*64] = {0};
    memset(json_str,0,sizeof(json_str));
    sprintf(json_str, "{\"errorcode\":%d,\"msg\":\"%s\"}",aiErrorCode,apcMessage);
    sprintf(content_header,"Content-Length: %d\r\n", strlen(json_str));
    strcat(send_str, content_header);
    strcat(send_str, "Keep-Alive: timeout=5, max=100\r\n");
    strcat(send_str, "Connection: Keep-Alive\r\n");
    strcat(send_str, "Content-Type: text/html\r\n");
    strcat(send_str, "\r\n");
    strcat(send_str, json_str);
    return send_str;
}



//处理线程，操作包括接收数据、解析HTTP报文、解码图片、写请求队列、读响应队列、返回识别结果
void * RecvImg(void * inputbuffer){
	pthread_detach(pthread_self());
    

	/***将监听线程传入的网络通信句柄保存起来，释放指针***/
    int *lpiClient_sockfd=static_cast<int *>(inputbuffer);
    int client_sockfd=*lpiClient_sockfd;  
    free(lpiClient_sockfd); 
    lpiClient_sockfd=NULL; 
    
	/***设置接收缓冲区大小***/
    int liRecvBufSize=32*1024;//设置为32K
    setsockopt(client_sockfd,SOL_SOCKET,SO_RCVBUF,(const char*)&liRecvBufSize,sizeof(int));

    
    
    int liRecvSize=0;
    char recv_str[256]={0};
    char recv_tmp[2268]={0};
    memset(recv_str,'\0',sizeof(recv_str));
    memset(recv_tmp,'\0',sizeof(recv_tmp));
   
    struct timeval t_start,t_end; 
    
    //get start time 
    gettimeofday(&t_start, NULL); 
    

	/***先接收头512个字节，为了解释出此HTTP包的总大小***/
    int liTotalSize=0;
    liRecvSize=0;
    while(liTotalSize<512)
    {
    	  if((liRecvSize=recv(client_sockfd,recv_str,sizeof(recv_str), 0))<=0) //异常处理
    	  {
    	  	    char * lpcResult_json=Creat_json(1,"Content-Length network ERROR!!");
		        signal( SIGPIPE, SIG_IGN );
		        //LOG4CPLUS_ERROR(_logger,"send lpcResult_json=\n"<<lpcResult_json);
		        send(client_sockfd,lpcResult_json,strlen(lpcResult_json)+1, 0);
		        if(lpcResult_json)
		        {
		            free(lpcResult_json);
		            lpcResult_json=NULL;
		        }
		        close(client_sockfd);
		        
                FILE *lpoHandleRecv=NULL;
                char writefilename[512];
                    
                struct timeval timestamp; 
                gettimeofday(&timestamp, NULL); 
                sprintf(writefilename,"%s/%lu_%lu_Content_Length_ERROR",TessDataPath,pthread_self(),((long)timestamp.tv_sec)*1000+(long)timestamp.tv_usec/1000);
                lpoHandleRecv=fopen(writefilename,"w+");
                fwrite(recv_tmp,liTotalSize,1,lpoHandleRecv);
                fclose(lpoHandleRecv);
		        
	            pthread_exit((void*)pthread_self());
		        return NULL;
    	  }
    	  
        memcpy(recv_tmp+liTotalSize,recv_str,liRecvSize);
        liTotalSize+=liRecvSize;
        memset(recv_str,0,sizeof(recv_str));
        liRecvSize=0;
    }
    
    
    char lpcLanguage[2];
    lpcLanguage[0]=recv_tmp[9];
    lpcLanguage[1]='\0';
    ImgInfo loImgInfo;
    loImgInfo.Language=atoi(lpcLanguage);
    loImgInfo.ImgID=pthread_self();
    
    
    int  liHTTPHeaderLenth=0;
    char *lpcTmp=recv_tmp;
    char lpcImgLabel[256];
    char lpcContentLength[32];
    memset(lpcImgLabel,'\0',sizeof(lpcImgLabel));
    memset(lpcContentLength,'\0',sizeof(lpcContentLength));
    

	/***寻找Content-Length的位置***/
    while((lpcTmp-recv_tmp)<liTotalSize&&(!(lpcTmp[0]=='C'&&lpcTmp[1]=='o'&&lpcTmp[2]=='n'
         &&lpcTmp[3]=='t'&&lpcTmp[4]=='e'&&lpcTmp[5]=='n'&&lpcTmp[6]=='t'&&lpcTmp[7]=='-'&&lpcTmp[8]=='L'
         &&lpcTmp[9]=='e'&&lpcTmp[10]=='n'&&lpcTmp[11]=='g'&&lpcTmp[12]=='t'&&lpcTmp[13]=='h')))
    {
        lpcTmp++;
    }
    
    if((lpcTmp-recv_tmp)>=liTotalSize) //异常处理
    {
        LOG4CPLUS_ERROR(_logger,recv_tmp<<"  Content-Length Format ERROR!!");
        
        char * lpcResult_json=Creat_json(1,"格式错误");
        signal( SIGPIPE, SIG_IGN );
		//LOG4CPLUS_ERROR(_logger,"send lpcResult_json=\n"<<lpcResult_json);
        send(client_sockfd,lpcResult_json,strlen(lpcResult_json)+1, 0);
        if(lpcResult_json)
        {
            free(lpcResult_json);
            lpcResult_json=NULL;
        }
        close(client_sockfd);
        
        FILE *lpoHandleRecv=NULL;
        char writefilename[512];
            
        struct timeval timestamp; 
        gettimeofday(&timestamp, NULL); 
        sprintf(writefilename,"%s/%lu_%lu",TessDataPath,pthread_self(),((long)timestamp.tv_sec)*1000+(long)timestamp.tv_usec/1000);
        lpoHandleRecv=fopen(writefilename,"w+");
        fwrite(recv_tmp,liTotalSize,1,lpoHandleRecv);
        fclose(lpoHandleRecv);
        
	    pthread_exit((void*)pthread_self());
        return NULL;
    }else
    {
        lpcTmp+=16;
        int i=0;
        while((!(lpcTmp[i]==0x0D&&lpcTmp[i+1]==0x0A))
                &&(lpcTmp+i-recv_tmp)<liTotalSize
                &&i<32)
        {
            lpcContentLength[i]=lpcTmp[i];
            ++i;
        }
        lpcTmp+=(i+2);
    }
	/***得到HTTP包body的大小***/
    int liContentLength=atoi(lpcContentLength);
    

	/***寻找boundary标识***/
    while((lpcTmp-recv_tmp)<liTotalSize&&(!(lpcTmp[0]=='b'&&lpcTmp[1]=='o'&&lpcTmp[2]=='u'
         &&lpcTmp[3]=='n'&&lpcTmp[4]=='d'&&lpcTmp[5]=='a'&&lpcTmp[6]=='r'&&lpcTmp[7]=='y'&&lpcTmp[8]=='=')))
    {
        lpcTmp++;
    }
    
    if((lpcTmp-recv_tmp)>=liTotalSize) //异常处理
    {
        LOG4CPLUS_ERROR(_logger,recv_tmp<<" Label Format ERROR!!");
        
        char * lpcResult_json=Creat_json(1,"格式错误");
        signal( SIGPIPE, SIG_IGN );
		//LOG4CPLUS_ERROR(_logger,"send lpcResult_json=\n"<<lpcResult_json);
        send(client_sockfd,lpcResult_json,strlen(lpcResult_json)+1, 0);
        if(lpcResult_json)
        {
            free(lpcResult_json);
            lpcResult_json=NULL;
        }
        close(client_sockfd);
        
        FILE *lpoHandleRecv=NULL;
        char writefilename[512];            
        struct timeval timestamp; 
        gettimeofday(&timestamp, NULL); 
        sprintf(writefilename,"%s/%lu_%lu",TessDataPath,pthread_self(),((long)timestamp.tv_sec)*1000+(long)timestamp.tv_usec/1000);
        lpoHandleRecv=fopen(writefilename,"w+");
        fwrite(recv_tmp,liTotalSize,1,lpoHandleRecv);
        fclose(lpoHandleRecv);
        
	    pthread_exit((void*)pthread_self());
        return NULL;
    }else
    {
        lpcTmp+=9;
        int i=0;
        while((!(lpcTmp[i]==0x0D&&lpcTmp[i+1]==0x0A))
                &&(lpcTmp-recv_tmp)<liTotalSize
                &&i<256)
        {
            lpcImgLabel[i]=lpcTmp[i];
            ++i;
        }
        lpcTmp+=(i+4);
    }
    
    liHTTPHeaderLenth=lpcTmp-recv_tmp;

	/***根据前面解析出来的liContentLength开辟响应大小的存储空间lpcRevContent***/
    char *lpcRevContent;
    lpcRevContent=(char *)malloc(liHTTPHeaderLenth+liContentLength);
    memset(lpcRevContent,'0',liHTTPHeaderLenth+liContentLength);
	/***将开始接收的512字节拷到lpcRevContent***/
    memcpy(lpcRevContent,recv_tmp,liTotalSize);
    
	lpcTmp=lpcRevContent+(lpcTmp-recv_tmp);
    memset(recv_str,0,sizeof(recv_str));
    liRecvSize=0;
	/***将HTTP报文余下的所有字节接收完毕***/
    while(liTotalSize<liHTTPHeaderLenth+liContentLength&&((liRecvSize=recv(client_sockfd,recv_str,sizeof(recv_str), 0))>0))
    {
        memcpy(lpcRevContent+liTotalSize,recv_str,liRecvSize);
        liTotalSize+=liRecvSize;
        //printf("liTotalSize=%d\tliRecvSize=%d\tliHTTPHeaderLenth+liContentLength=%d\n",liTotalSize,liRecvSize,liHTTPHeaderLenth+liContentLength);
        memset(recv_str,0,sizeof(recv_str));
        liRecvSize=0;
    }
    
        
    //finding Content-Type:
	/***寻找PNG图片格式开始的标志***/
    while((lpcTmp-lpcRevContent)<liTotalSize&&(!((lpcTmp[0]&0xFF)==0x89&&((lpcTmp[1]&0xFF)==0x50)&&((lpcTmp[2]&0xFF)==0x4E)
                                                &&((lpcTmp[3]&0xFF)==0x47)&&((lpcTmp[4]&0xFF)==0x0D)&&((lpcTmp[5]&0xFF)==0x0A))))
    {
        lpcTmp++;
    }
    //printf("lpcTmp=%X %X %X %X %X %X %X %X %X %X\n",lpcTmp[0]&0xFF,lpcTmp[1]&0xFF,lpcTmp[2]&0xFF,lpcTmp[3]&0xFF,lpcTmp[4]&0xFF,lpcTmp[5]&0xFF,lpcTmp[6]&0xFF,lpcTmp[7]&0xFF,lpcTmp[8],lpcTmp[9]);

    if((lpcTmp-lpcRevContent)>=liTotalSize) //异常处理
    {
        LOG4CPLUS_ERROR(_logger,recv_tmp<<" PNG Header ERROR!!");
        
        char * lpcResult_json=Creat_json(1,"格式错误");
        signal( SIGPIPE, SIG_IGN );
		//LOG4CPLUS_ERROR(_logger,"send lpcResult_json=\n"<<lpcResult_json);
        send(client_sockfd,lpcResult_json,strlen(lpcResult_json)+1, 0);
        if(lpcResult_json)
        {
            free(lpcResult_json);
            lpcResult_json=NULL;
        }
        close(client_sockfd);
        
        FILE *lpoHandleRecv=NULL;
        char writefilename[512];            
        struct timeval timestamp; 
        gettimeofday(&timestamp, NULL); 
        sprintf(writefilename,"%s/%lu_%lu",TessDataPath,pthread_self(),((long)timestamp.tv_sec)*1000+(long)timestamp.tv_usec/1000);
        lpoHandleRecv=fopen(writefilename,"w+");
        fwrite(lpcRevContent,liTotalSize,1,lpoHandleRecv);
        fclose(lpoHandleRecv);
        
	    pthread_exit((void*)pthread_self());
        return NULL;
    }    

    while((lpcTmp-recv_tmp)<liTotalSize&&(!(*(lpcTmp-4)==0x0D&&*(lpcTmp-3)==0x0A&&*(lpcTmp-2)==0x0D&&*(lpcTmp-1)==0x0A)))
    {
    	  lpcTmp++;
    }
    
    int  liFoundEndCount=0;
    char *lpcRevContentEnd=lpcRevContent+liTotalSize-9;
    
	/***寻找PNG图片格式结束的标志***/
    while((liFoundEndCount<liTotalSize/2)&&(!((lpcRevContentEnd[0]&0xFF)==0x49&&((lpcRevContentEnd[1]&0xFF)==0x45)&&((lpcRevContentEnd[2]&0xFF)==0x4E)
                                              &&((lpcRevContentEnd[3]&0xFF)==0x44)&&((lpcRevContentEnd[4]&0xFF)==0xAE)&&((lpcRevContentEnd[5]&0xFF)==0x42)
                                              &&((lpcRevContentEnd[6]&0xFF)==0x60)&&((lpcRevContentEnd[7]&0xFF)==0x82))))
    {
        liFoundEndCount++;
        lpcRevContentEnd--;
    }
    
    //printf("lpcRevContentEnd=%X %X %X %X %X %X %X %X %X %X\n",lpcRevContentEnd[0]&0xFF,lpcRevContentEnd[1]&0xFF,lpcRevContentEnd[2]&0xFF,lpcRevContentEnd[3]&0xFF,lpcRevContentEnd[4]&0xFF,lpcRevContentEnd[5]&0xFF,lpcRevContentEnd[6]&0xFF,lpcRevContentEnd[7]&0xFF,lpcRevContentEnd[8]&0xFF,lpcRevContentEnd[9]&0xFF);
    
    lpcRevContentEnd+=7;
    
    
    if(liFoundEndCount>=liTotalSize/2) //异常处理
    {
        LOG4CPLUS_ERROR(_logger,"in  RecvImg  strlen(lpcImgLabel)="<<strlen(lpcImgLabel)<<"  lpcImgLabel="<<lpcImgLabel);
        LOG4CPLUS_ERROR(_logger,"ERR:"<<lpcRevContent<<" Found End Lable Format ERROR!!");
        
        char * lpcResult_json=Creat_json(1,"格式错误");
        signal( SIGPIPE, SIG_IGN );
		//LOG4CPLUS_ERROR(_logger,"send lpcResult_json=\n"<<lpcResult_json);
        send(client_sockfd,lpcResult_json,strlen(lpcResult_json)+1, 0);
        if(lpcResult_json)
        {
            free(lpcResult_json);
            lpcResult_json=NULL;
        }
        close(client_sockfd);
        
        FILE *lpoHandleRecv=NULL;
        char writefilename[512];            
        struct timeval timestamp; 
        gettimeofday(&timestamp, NULL); 
        sprintf(writefilename,"%s/%lu_%lu",TessDataPath,pthread_self(),((long)timestamp.tv_sec)*1000+(long)timestamp.tv_usec/1000);
        lpoHandleRecv=fopen(writefilename,"w+");
        fwrite(lpcRevContent,liTotalSize,1,lpoHandleRecv);
        fclose(lpoHandleRecv);
        
        free(lpcRevContent);  
        lpcRevContent=NULL; 
	    pthread_exit((void*)pthread_self());
        return NULL;
    }
    
	/***根据PNG开始标志的地址及结束标志的地址，计算出图片的字节长度***/
    int liImgSize=lpcRevContentEnd-lpcTmp+1;
    
    //get end time 
    gettimeofday(&t_end, NULL); 
    LOG4CPLUS_DEBUG(_logger,"in  RecvImg   recv take: "
                 <<float((((long)t_end.tv_sec)*1000+(long)t_end.tv_usec/1000)-(((long)t_start.tv_sec)*1000+(long)t_start.tv_usec/1000))/1000<<" s");
                 
    Pix* pixs = NULL;
	/***对图片进行解码***/
    pixs=pixReadMem ( (unsigned char*)lpcTmp,liImgSize );
    
        /*FILE *lpoHandleRecv=NULL;
        char writefilename[512];        
        struct timeval timestamp; 
        gettimeofday(&timestamp, NULL); 
        sprintf(writefilename,"%s/%lu_%lu",TessDataPath,pthread_self(),((long)timestamp.tv_sec)*1000+(long)timestamp.tv_usec/1000);
        lpoHandleRecv=fopen(writefilename,"w+");
        fwrite(lpcRevContent,liTotalSize,1,lpoHandleRecv);
        fclose(lpoHandleRecv);*/
         
    //get start time 
    gettimeofday(&t_start, NULL); 
    
    if (!pixs) { //异常处理
        LOG4CPLUS_ERROR(_logger,"in  RecvImg  pixReadMem Fail!!");
                
        FILE *lpoHandleRecv=NULL;
        char writefilename[512];            
        struct timeval timestamp; 
        gettimeofday(&timestamp, NULL); 
        sprintf(writefilename,"%s/%lu_%lu",TessDataPath,pthread_self(),((long)timestamp.tv_sec)*1000+(long)timestamp.tv_usec/1000);
        lpoHandleRecv=fopen(writefilename,"w+");
        fwrite(lpcRevContent,liTotalSize,1,lpoHandleRecv);
        fclose(lpoHandleRecv);
        
        char * lpcResult_json=Creat_json(1,"格式错误");
        signal( SIGPIPE, SIG_IGN );
		//LOG4CPLUS_ERROR(_logger,"send lpcResult_json=\n"<<lpcResult_json);
        send(client_sockfd,lpcResult_json,strlen(lpcResult_json)+1, 0);
        if(lpcResult_json)
        {
            free(lpcResult_json);
            lpcResult_json=NULL;
        }
        close(client_sockfd);
        
        free(lpcRevContent);  
        lpcRevContent=NULL; 
	    pthread_exit((void*)pthread_self());
	    return NULL;
    }
    
    loImgInfo.ImgPixs=pixs;
    free(lpcRevContent);  
    lpcRevContent=NULL; 
    
	/***加锁，准备将图片存入请求队列***/
    bool Imgtoomuch=false;
    pthread_mutex_lock(&ImgVecLock); //获得ImgVecLock锁，与其他线程竞争，阻塞直到获得锁成功
    LOG4CPLUS_DEBUG(_logger,"in  RecvImg  recv a img ID="<<loImgInfo.ImgID<<"  Language="<<loImgInfo.Language<<"  now push back into VecImg");
	/***如果请求队列已经超过15在等待，则直接返回服务器忙***/
    if(VecImg.size()>15)
    {
        Imgtoomuch=true;
    }else
    {    
        if(VecImg.size()==0)
        {
            pthread_cond_signal(&ImgVecCond);
        }
        VecImg.push_back(loImgInfo);
    }
    
    pthread_mutex_unlock(&ImgVecLock);
    if(Imgtoomuch)
    {
        LOG4CPLUS_ERROR(_logger,"in  RecvImg  Imgtoomuch!!");
        pixDestroy(&pixs);
        pixs=NULL;
        
        char * lpcResult_json=Creat_json(2,"服务器繁忙");
        signal( SIGPIPE, SIG_IGN );
		//LOG4CPLUS_ERROR(_logger,"send lpcResult_json=\n"<<lpcResult_json);
        send(client_sockfd,lpcResult_json,strlen(lpcResult_json)+1, 0);
        if(lpcResult_json)
        {
            free(lpcResult_json);
            lpcResult_json=NULL;
        }
        close(client_sockfd);
        
	    pthread_exit((void*)pthread_self());
        return NULL;
    }
    
    

	/***开始通过条件变量wait结果队列，一旦有新结果存入（识别线程发来信号），
	则遍历结果队列判断是否有与存入的请求对应ImgID的识别结果
	有则取出，跳出循环，没有则等待下一个新结果的存入***/
    RespInfo loRespInfo;
    loRespInfo.ImgID=0;
    loRespInfo.Resp=NULL;
    vector<RespInfo>::iterator  lpoiterResp;
    bool lbNotMine=false;
    while(true)
    {
        pthread_mutex_lock(&RespVecLock);  //获得RespVecLock锁，与其他线程竞争，阻塞直到获得锁成功
        while(VecResp.size()==0||lbNotMine)
        {
            lbNotMine=false;
            pthread_cond_wait(&RespVecCond,&RespVecLock); //RespVecCond对应有信号发过来，而且获得RespVecLock锁才返回，不然一直阻塞，阻塞过程会释放RespVecLock
        }
        
        for(lpoiterResp=VecResp.begin();lpoiterResp!=VecResp.end();lpoiterResp++)
        {
            if(lpoiterResp->ImgID==loImgInfo.ImgID)
            {
                break;
            }
        }
        
        if(lpoiterResp!=VecResp.end())
        {
            loRespInfo.ImgID=lpoiterResp->ImgID;
            loRespInfo.Resp=lpoiterResp->Resp;
            VecResp.erase(lpoiterResp);
            pthread_cond_signal(&RespVecCond);
            pthread_mutex_unlock(&RespVecLock);
            break;
        }else
        {
            lbNotMine=true;
            pthread_cond_signal(&RespVecCond);
            pthread_mutex_unlock(&RespVecLock);
            //usleep(30000);
            continue;
        }
    }
    
	/***将识别结果打包成HTTP报文***/
    char * lpcResult_json=NULL;
    lpcResult_json=Creat_json(0,loRespInfo.Resp);
    
	//LOG4CPLUS_ERROR(_logger,"send lpcResult_json=\n"<<lpcResult_json);
    signal( SIGPIPE, SIG_IGN ); //屏蔽socket发送失败的信号，因为此信号会导致程序coredump
	/***返回结果到客户端***/
    if(-1 == send(client_sockfd,lpcResult_json,strlen(lpcResult_json)+1, 0)) //异常处理
    {
        LOG4CPLUS_ERROR(_logger,"in  RecvImg  Send Fail!!");
        perror("Send OCR Result");
    }else
    {
        LOG4CPLUS_DEBUG(_logger,"in  RecvImg  Send  loRespInfo.ImgID="<<loRespInfo.ImgID);
        
        //get end time 
        gettimeofday(&t_end, NULL); 
        LOG4CPLUS_DEBUG(_logger,"in  RecvImg   Wait for OCR take: "
                     <<float((((long)t_end.tv_sec)*1000+(long)t_end.tv_usec/1000)-(((long)t_start.tv_sec)*1000+(long)t_start.tv_usec/1000))/1000<<" s");
    }


	/***释放资源***/
    close(client_sockfd);
    if(loRespInfo.Resp)
    {
        delete[] (loRespInfo.Resp);
        loRespInfo.Resp=NULL;
    }
    if(lpcResult_json)
    {
        free(lpcResult_json);
        lpcResult_json=NULL;
    }
    pixs=NULL;
	pthread_exit((void*)pthread_self());
    return NULL;
}


//监听线程，操作包括监听端口、接受请求、对于每个请求新建处理线程进行处理、进入下一次监听等待请求
void * ListenForImg(void * inputbuffer)
{ 
	pthread_detach(pthread_self());
    int listenfd;
    int *lpiConnectfd;
    struct sockaddr_in server;
    struct sockaddr_in client;
    socklen_t sin_size;
    
    if((listenfd=socket(AF_INET,SOCK_STREAM,0))==-1){ //申请一个socket句柄
	    LOG4CPLUS_ERROR(_logger,"in  ListenForImg   socket  fail!");
        perror("Creating socket failed.");
        exit(1);
    }
    

	/***构造网络通信结构体，包括TCP/UDP协议，接收的目标IP地址（现为INADDR_ANY即全部），监听的端口***/
    bzero(&server,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=htonl(INADDR_ANY);
    server.sin_port=htons(45758);

    
	/***设置接收缓冲区大小***/
    int liRecvBuf=32*1024;//设置为32K
    setsockopt(listenfd,SOL_SOCKET,SO_RCVBUF,(const char*)&liRecvBuf,sizeof(int));
    
    /***设置端口可重用、超时时间***/
	int bReuseaddr=1; 
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(bReuseaddr));
    struct timeval timeout={10,0};//10s
    int ret=setsockopt(listenfd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
    ret=setsockopt(listenfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
    
	/***将网络通信结构体与申请的socket句柄绑定***/
    if(bind(listenfd,(struct sockaddr*)&server,sizeof(struct sockaddr))==-1){
	    LOG4CPLUS_ERROR(_logger,"in  ListenForImg   bind  fail!");
        perror("bind() error");
        exit(1);
    }
    
    sin_size=sizeof(struct sockaddr_in);
    if (listen(listenfd,45758)<0)  //开始监听端口
    {
	    LOG4CPLUS_ERROR(_logger,"in  ListenForImg   listen  fail!");
        perror("listen() error\n");
        exit(1);
    }
    
    /***开始循环接收请求，每个请求新建一个处理线程进行处理***/
    while(true)
    {  
        lpiConnectfd = (int *)malloc(sizeof(int));  //为了让每个新建的处理线程独享一个句柄内存空间，需要每次都malloc开辟一块空间然后传入pthread_create
        if((*lpiConnectfd=accept(listenfd,(struct sockaddr*)&client,&sin_size))==-1)
        {
		    //LOG4CPLUS_ERROR(_logger,"in  ListenForImg   accept  fail!");
            //perror("ERR:accept");
            free(lpiConnectfd);  
            lpiConnectfd=NULL; 
            continue;
        }else
        {
            LOG4CPLUS_DEBUG(_logger,"in  ListenForImg   accept a connection from "<<inet_ntoa(client.sin_addr));
            //printf("in  ListenForImg   accept a connection from %s\n",inet_ntoa(client.sin_addr));
        }
        
        pthread_t  recvimg_arrpthread_t;
        if(pthread_create(&recvimg_arrpthread_t, NULL, RecvImg, (void*)lpiConnectfd)!=0)//创建处理线程  
        {  
		    LOG4CPLUS_ERROR(_logger,"in  ListenForImg   pthread_create  fail!");
            perror("ERR:pthread_create");  
            close(*lpiConnectfd);
            free(lpiConnectfd);
            lpiConnectfd=NULL; 
        }
    } 
	pthread_exit((void*)pthread_self());
    
}


//识别闹钟(计时器)识别开始前启动的线程，当识别时间超过15秒，即在此线程中对识别进行中断
void * OCRAlarm(void * inputbuffer)
{
	 tesseract::TessBaseAPI *api=static_cast<tesseract::TessBaseAPI*>(inputbuffer);
	 sleep(15);
	 LOG4CPLUS_ERROR(_logger,"in  OCRAlarm time out!");
	 if(api)
	 {
	    api->Stop();
	 }
	 pthread_exit((void*)pthread_self());
	 return NULL;
	 
}


//识别线程，操作包括等待图片队列信号、取出图片、启动识别闹钟线程、识别、关闭识别闹钟、识别结果过滤、识别结果写入响应队列、进入下一次等待图片队列信号
void * OCRImg(void * inputbuffer)  
{  
	pthread_detach(pthread_self());
    struct timeval t_start,t_end; 
  
    tesseract::TessBaseAPI *api=static_cast<tesseract::TessBaseAPI*>(inputbuffer);  //由于线程只接受void *类型的入参，所以需要传入时强制转换
																					//成（void *）再做安全类型转换成TessBaseAPI
    int liOCRCount=0;  //识别计数器
    

	//开始循环wait请求队列，当有新的图片存入请求队列（处理线程存入后发信号）时，激活一条识别线程取出一张图片进行识别
    while(true)
    {
        LOG4CPLUS_DEBUG(_logger,"in  OCRImg   liOCRCount="<<liOCRCount);
        
        LOG4CPLUS_DEBUG(_logger,"in  OCRImg   before get ImgVecLock");
		/***加锁、wait信号、取出图片、解锁***/
        pthread_mutex_lock(&ImgVecLock);//获得ImgVecLock锁，与其他线程竞争，阻塞直到获得锁成功
        while(VecImg.size()==0)
        {  
            pthread_cond_wait(&ImgVecCond,&ImgVecLock);  //ImgVecCond对应有信号发过来，而且获得ImgVecLock锁才返回，不然一直阻塞，阻塞过程会释放ImgVecLock
        }
        
        ImgInfo loImgInfo;
        loImgInfo.ImgID=VecImg.begin()->ImgID;
        loImgInfo.ImgPixs=VecImg.begin()->ImgPixs;
        loImgInfo.Language=VecImg.begin()->Language;
        LOG4CPLUS_DEBUG(_logger,"in  OCRImg   VecImg.size()="<<VecImg.size());
        VecImg.erase(VecImg.begin());
        pthread_mutex_unlock(&ImgVecLock);

        //get start time 
        gettimeofday(&t_start, NULL); 
        
        /***tesstwo的基本识别步骤，SetImage->GetUTF8Text***/
	    api->SetImage(loImgInfo.ImgPixs);
 
		/***GetUTF8Text前启动一个识别闹钟，对识别时间进行检测，超时则会中断识别***/
 		pthread_t  ocralarm_pthread_t;
 		int liPthCreatRet=0;
 		if((liPthCreatRet=pthread_create(&ocralarm_pthread_t, NULL, OCRAlarm, inputbuffer))!=0)//创建子线程  
        {  
            LOG4CPLUS_ERROR(_logger,"in  OCRImg   pthread_create Fail!(errno:"<<liPthCreatRet<<"-"<<strerror(liPthCreatRet)<<")");  
        }
        api->Start();
        //public static final int USE_ALL = 0;
        //public static final int CHI_ONLY = 1;
        //public static final int ENG_ONLY = 2;
        //public static final int CHI_ENG  = 3;
	    char * text=api->GetUTF8TextWith(loImgInfo.Language);  //识别
        
		/***取消识别闹钟线程***/
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
	    if(pthread_cancel(ocralarm_pthread_t)!=0)
	    {
	      LOG4CPLUS_ERROR(_logger,"in  OCRImg   pthread_cancel Fail!");  
	    }
        pthread_detach(ocralarm_pthread_t);

		/***清楚识别状态，销毁图片数据***/
		api->Start();
        api->Clear();
        pixDestroy(&loImgInfo.ImgPixs);
        //char * text=NULL;

		/***对识别结果作判断、过滤操作***/
	    if(text==NULL)
	    {
		    LOG4CPLUS_ERROR(_logger,"in  OCRImg   GetUTF8Text() fail");
		    text=new char[2];
		    strcpy(text," ");
	    }else if(text[0]=='('||text[1]=='('||text[2]=='('||text[3]=='('||text[4]=='(')
	    {
	        int i=0;
	        while(text[i]!=')'&&text[i]!='\0')
	        {
	            i++;
	        }
	        if(text[i]!='\0'&&i<strlen(text)/2)
	        {
	            LOG4CPLUS_DEBUG(_logger,"in  OCRImg   delete topic head-info");
	            sprintf(text,"%s",text+i+1);
	        }    
	    }
	    
	    int i=0;
	    while(text[i]!='\0')
	    {
	        if(text[i]=='"'||text[i]=='\\'||text[i]=='\n')
	        {
	            text[i]=' ';
	        }
	        i++;
	    }
	    
	    
        //get end time 
        gettimeofday(&t_end, NULL); 
        LOG4CPLUS_DEBUG(_logger,"in  OCRImg   GetUTF8Text take: "
                    <<float((((long)t_end.tv_sec)*1000+(long)t_end.tv_usec/1000)-(((long)t_start.tv_sec)*1000+(long)t_start.tv_usec/1000))/1000<<" s");
	
        RespInfo loRespInfo;
        loRespInfo.ImgID=loImgInfo.ImgID;
        loRespInfo.Resp=text;
        
		/***加锁，发信号，识别结果存入响应队列，解锁***/
        pthread_mutex_lock(&RespVecLock);
        pthread_cond_signal(&RespVecCond);
        LOG4CPLUS_DEBUG(_logger,"in  OCRImg   push back loRespInfo.ImgID="<<loRespInfo.ImgID<<"   loRespInfo.Resp="<<endl<<loRespInfo.Resp);
        VecResp.push_back(loRespInfo);
        pthread_mutex_unlock(&RespVecLock);
        liOCRCount++;
    }
  
	pthread_exit((void*)pthread_self());
    return NULL;
}



//主函数，操作包括解析程序所在目录、拼接语言包路径、拼接日志路径、初始化日志对象、初始化N个识别对象、启动N个识别线程、启动监听线程、挂起等待所有线程的退出
int main(int argc, char **argv) {
    if(argc!=2)
    {
        printf("ERROR:argc!=2!\n");
        return -1;
    }
    
	/***获取程序全路径***/
    char exec_name [512];
    char lpcTessDataPath [512];
    exec_name[readlink ("/proc/self/exe", exec_name, 512)-9]='\0'; //得到全路径存储在exec_name中，然后将/home/...../bin/Tesseract最后面的"Tesseract"去掉，得到文件夹路径
  
    sprintf(lpcTessDataPath,"%stessdatas",exec_name); //通过上面得到的路径exec_name拼接出语言包路径
    TessDataPath=lpcTessDataPath;

    init_daemon(); //守护进程
    prctl(PR_SET_DUMPABLE, 1); //允许程序coredump时生成core文件方便问题定位及调试
    
    /* step 1: Instantiate an appender object */
    char logfilename[512];
    sprintf(logfilename,"%s%s",exec_name,argv[1]); //通过上面得到的路径exec_name拼接出日志文件全路径
    SharedAppenderPtr _append(new RollingFileAppender(logfilename, 50*1024*1024, 10));
    _append->setName("file log test");
    /* step 4: Instantiate a logger object */
    std::string pattern = "%D{%m/%d/%y %H:%M:%S}[%t][%L]%p - %m%n";
    std::auto_ptr<log4cplus::Layout> _layout(new PatternLayout(pattern));
    /* step 3: Attach the layout object to the appender */
    _append->setLayout( _layout );
    _logger = Logger::getInstance("test.subtestof_filelog");
    /* step 5: Attach the appender object to the logger  */
    _logger.addAppender(_append);  //初始化日志对象完成
    /* log activity */
    
    LOG4CPLUS_DEBUG(_logger,"LOGFile: "<<logfilename); //第一条日志
    
    int i; 
	pthread_t  listenforimg_arrpthread_t;   //监听线程结构体，用于存储线程的运行状态等信息
	pthread_t  ocr_arrpthread_t[OCR_THREAD_NUM];//识别线程结构体队列，用于存储线程的运行状态等信息
	
    tesseract::TessBaseAPI *baseapi[OCR_THREAD_NUM];  //识别对象的队列

	struct timeval t_start,t_end; 
	

	//for循环初始化OCR_THREAD_NUM个的识别对象
    for ( i = 0; i < (OCR_THREAD_NUM)&&SERVERVERSION; i++)  
	{
      //get start time 
      gettimeofday(&t_start, NULL); 
      baseapi[i]=new tesseract::TessBaseAPI();
	  int rc = baseapi[i]->Init(TessDataPath, "libpinyin+libchi_sim+libeng");
	  if (rc) {
		LOG4CPLUS_DEBUG(_logger, "Could not initialize tesseract.\n");
		exit(1);
	  }		  
      //get end time 
      gettimeofday(&t_end, NULL); 
      LOG4CPLUS_DEBUG(_logger,"baseapi["<<i<<"]"<<" Init take time: "
                    <<float((((long)t_end.tv_sec)*1000+(long)t_end.tv_usec/1000)-(((long)t_start.tv_sec)*1000+(long)t_start.tv_usec/1000))/1000<<" s");
	}
	

	//创建OCR_THREAD_NUM个的识别线程
    for ( i = 0; i < OCR_THREAD_NUM&&SERVERVERSION; i++)  
	{
        if(pthread_create(&ocr_arrpthread_t[i], NULL, OCRImg, (void*)(baseapi[i]))!=0)//创建子线程  
        {  
            perror("OCRImg pthread_create");  
        }
	}
	

	//创建监听线程
	if(SERVERVERSION)
	{
        if(pthread_create(&listenforimg_arrpthread_t, NULL,ListenForImg,NULL)!=0)//创建子线程  
        {  
            LOG4CPLUS_ERROR(_logger,"ERR:ListenForImg Fail!");
        }
    }
    

	//通过线程结构体等待线程退出
    for ( i = 0; i < OCR_THREAD_NUM&&SERVERVERSION; i++)  
	{
		pthread_join (ocr_arrpthread_t[i], NULL);  //ocr_arrpthread_[i]对应的线程退出则返回，不然一直阻塞
	}

	pthread_join (listenforimg_arrpthread_t, NULL);  //listenforimg_arrpthread_t对应的线程退出则返回，不然一直阻塞
    return 0;

}


