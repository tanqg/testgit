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

#define OCR_THREAD_NUM 4  //Ԥ��ʼ����ʶ���̳߳��߳���
#define SERVERVERSION  true  //����
using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;
 
pthread_mutex_t ImgVecLock;  /****������У�ͼƬ���У���ͬ������
                                  1������ͬ����������߳�֮�����������е�д��ͼƬ������
                                  2������ͬ�����ʶ���߳�֮�����������еĶ���ͼƬ������
                                  3������ͬ�������߳���ʶ���߳�֮���д�롢����������
                             *****/

pthread_mutex_t RespVecLock;  /****��Ӧ���У�ʶ�������У���ͬ������
                                  1������ͬ�����ʶ���߳�֮�������Ӧ���е�д��ʶ����������
                                  2������ͬ����������߳�֮�������Ӧ���еĶ���ʶ����������
                                  3������ͬ��ʶ���߳��봦���߳�֮���д�롢����������
                             *****/

pthread_cond_t ImgVecCond;  /****������У�ͼƬ���У�������������
                                 1�������ڴ����߳����������д���µ�ͼƬ����ʱ��֪ͨʶ���̣߳��������󵽣�����ʶ�𣩣�
                                 2���������������Ϊ�յ�ʱ�򣬹���ʶ���̣߳������߳��ܿ�ѭ����
                             *****/ 
                             
pthread_cond_t RespVecCond; /****��Ӧ���У�ʶ�������У�������������
                                 1��������ʶ���߳�����Ӧ����д���µ�ʶ����ʱ��֪ͨ�����̣߳����½���������Ӧ�Ĵ����̲߳��գ���
                                 2��������ʶ��������Ϊ�յ�ʱ�򣬹������̣߳������߳��ܿ�ѭ����
                             *****/ 
Logger _logger ;      //��������log4plus��־����ȫ�ֱ�������main���������ü���ʼ��
char * TessDataPath;  //ȫ�ֱ������洢��ǰ��������Ŀ¼���������԰�����־���Ŀ¼��Ѱ��



/****
��������
ImgID  ����Ψһ��ʶID,��������Ӧ�����е�ʶ��������ƥ�䣬��ȡ�߳�ID��
ImgPixs ͼƬ���������ض���
Language ���ڳ���ʶ������԰�����ѡΪȫ����Ӣ�ġ����ģ�
****/
struct ImgInfo
{
	unsigned long int ImgID;
	Pix*   ImgPixs;
	int    Language;
};


/****
ImgID  ��ӦΨһ��ʾID����Ӧ��ImgInfo��ImgID��
Resp   ʶ�����ַ�����
****/
struct RespInfo
{
	unsigned long int ImgID;
	char   *Resp;
};



vector<ImgInfo>  VecImg;  //������У�ImgInfo���У�
vector<RespInfo> VecResp; //��Ӧ���У�RespInfo���У�


//����ǰ���̱���ػ�����
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


//��ʶ���������HTTP����
char * Creat_json(int aiErrorCode,const char * apcMessage)
{
    
    char *send_str=(char*) malloc(2048*64);
    memset(send_str,0,sizeof(send_str));
    //ͷ��Ϣ
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



//�����̣߳����������������ݡ�����HTTP���ġ�����ͼƬ��д������С�����Ӧ���С�����ʶ����
void * RecvImg(void * inputbuffer){
	pthread_detach(pthread_self());
    

	/***�������̴߳��������ͨ�ž�������������ͷ�ָ��***/
    int *lpiClient_sockfd=static_cast<int *>(inputbuffer);
    int client_sockfd=*lpiClient_sockfd;  
    free(lpiClient_sockfd); 
    lpiClient_sockfd=NULL; 
    
	/***���ý��ջ�������С***/
    int liRecvBufSize=32*1024;//����Ϊ32K
    setsockopt(client_sockfd,SOL_SOCKET,SO_RCVBUF,(const char*)&liRecvBufSize,sizeof(int));

    
    
    int liRecvSize=0;
    char recv_str[256]={0};
    char recv_tmp[2268]={0};
    memset(recv_str,'\0',sizeof(recv_str));
    memset(recv_tmp,'\0',sizeof(recv_tmp));
   
    struct timeval t_start,t_end; 
    
    //get start time 
    gettimeofday(&t_start, NULL); 
    

	/***�Ƚ���ͷ512���ֽڣ�Ϊ�˽��ͳ���HTTP�����ܴ�С***/
    int liTotalSize=0;
    liRecvSize=0;
    while(liTotalSize<512)
    {
    	  if((liRecvSize=recv(client_sockfd,recv_str,sizeof(recv_str), 0))<=0) //�쳣����
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
    

	/***Ѱ��Content-Length��λ��***/
    while((lpcTmp-recv_tmp)<liTotalSize&&(!(lpcTmp[0]=='C'&&lpcTmp[1]=='o'&&lpcTmp[2]=='n'
         &&lpcTmp[3]=='t'&&lpcTmp[4]=='e'&&lpcTmp[5]=='n'&&lpcTmp[6]=='t'&&lpcTmp[7]=='-'&&lpcTmp[8]=='L'
         &&lpcTmp[9]=='e'&&lpcTmp[10]=='n'&&lpcTmp[11]=='g'&&lpcTmp[12]=='t'&&lpcTmp[13]=='h')))
    {
        lpcTmp++;
    }
    
    if((lpcTmp-recv_tmp)>=liTotalSize) //�쳣����
    {
        LOG4CPLUS_ERROR(_logger,recv_tmp<<"  Content-Length Format ERROR!!");
        
        char * lpcResult_json=Creat_json(1,"��ʽ����");
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
	/***�õ�HTTP��body�Ĵ�С***/
    int liContentLength=atoi(lpcContentLength);
    

	/***Ѱ��boundary��ʶ***/
    while((lpcTmp-recv_tmp)<liTotalSize&&(!(lpcTmp[0]=='b'&&lpcTmp[1]=='o'&&lpcTmp[2]=='u'
         &&lpcTmp[3]=='n'&&lpcTmp[4]=='d'&&lpcTmp[5]=='a'&&lpcTmp[6]=='r'&&lpcTmp[7]=='y'&&lpcTmp[8]=='=')))
    {
        lpcTmp++;
    }
    
    if((lpcTmp-recv_tmp)>=liTotalSize) //�쳣����
    {
        LOG4CPLUS_ERROR(_logger,recv_tmp<<" Label Format ERROR!!");
        
        char * lpcResult_json=Creat_json(1,"��ʽ����");
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

	/***����ǰ�����������liContentLength������Ӧ��С�Ĵ洢�ռ�lpcRevContent***/
    char *lpcRevContent;
    lpcRevContent=(char *)malloc(liHTTPHeaderLenth+liContentLength);
    memset(lpcRevContent,'0',liHTTPHeaderLenth+liContentLength);
	/***����ʼ���յ�512�ֽڿ���lpcRevContent***/
    memcpy(lpcRevContent,recv_tmp,liTotalSize);
    
	lpcTmp=lpcRevContent+(lpcTmp-recv_tmp);
    memset(recv_str,0,sizeof(recv_str));
    liRecvSize=0;
	/***��HTTP�������µ������ֽڽ������***/
    while(liTotalSize<liHTTPHeaderLenth+liContentLength&&((liRecvSize=recv(client_sockfd,recv_str,sizeof(recv_str), 0))>0))
    {
        memcpy(lpcRevContent+liTotalSize,recv_str,liRecvSize);
        liTotalSize+=liRecvSize;
        //printf("liTotalSize=%d\tliRecvSize=%d\tliHTTPHeaderLenth+liContentLength=%d\n",liTotalSize,liRecvSize,liHTTPHeaderLenth+liContentLength);
        memset(recv_str,0,sizeof(recv_str));
        liRecvSize=0;
    }
    
        
    //finding Content-Type:
	/***Ѱ��PNGͼƬ��ʽ��ʼ�ı�־***/
    while((lpcTmp-lpcRevContent)<liTotalSize&&(!((lpcTmp[0]&0xFF)==0x89&&((lpcTmp[1]&0xFF)==0x50)&&((lpcTmp[2]&0xFF)==0x4E)
                                                &&((lpcTmp[3]&0xFF)==0x47)&&((lpcTmp[4]&0xFF)==0x0D)&&((lpcTmp[5]&0xFF)==0x0A))))
    {
        lpcTmp++;
    }
    //printf("lpcTmp=%X %X %X %X %X %X %X %X %X %X\n",lpcTmp[0]&0xFF,lpcTmp[1]&0xFF,lpcTmp[2]&0xFF,lpcTmp[3]&0xFF,lpcTmp[4]&0xFF,lpcTmp[5]&0xFF,lpcTmp[6]&0xFF,lpcTmp[7]&0xFF,lpcTmp[8],lpcTmp[9]);

    if((lpcTmp-lpcRevContent)>=liTotalSize) //�쳣����
    {
        LOG4CPLUS_ERROR(_logger,recv_tmp<<" PNG Header ERROR!!");
        
        char * lpcResult_json=Creat_json(1,"��ʽ����");
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
    
	/***Ѱ��PNGͼƬ��ʽ�����ı�־***/
    while((liFoundEndCount<liTotalSize/2)&&(!((lpcRevContentEnd[0]&0xFF)==0x49&&((lpcRevContentEnd[1]&0xFF)==0x45)&&((lpcRevContentEnd[2]&0xFF)==0x4E)
                                              &&((lpcRevContentEnd[3]&0xFF)==0x44)&&((lpcRevContentEnd[4]&0xFF)==0xAE)&&((lpcRevContentEnd[5]&0xFF)==0x42)
                                              &&((lpcRevContentEnd[6]&0xFF)==0x60)&&((lpcRevContentEnd[7]&0xFF)==0x82))))
    {
        liFoundEndCount++;
        lpcRevContentEnd--;
    }
    
    //printf("lpcRevContentEnd=%X %X %X %X %X %X %X %X %X %X\n",lpcRevContentEnd[0]&0xFF,lpcRevContentEnd[1]&0xFF,lpcRevContentEnd[2]&0xFF,lpcRevContentEnd[3]&0xFF,lpcRevContentEnd[4]&0xFF,lpcRevContentEnd[5]&0xFF,lpcRevContentEnd[6]&0xFF,lpcRevContentEnd[7]&0xFF,lpcRevContentEnd[8]&0xFF,lpcRevContentEnd[9]&0xFF);
    
    lpcRevContentEnd+=7;
    
    
    if(liFoundEndCount>=liTotalSize/2) //�쳣����
    {
        LOG4CPLUS_ERROR(_logger,"in  RecvImg  strlen(lpcImgLabel)="<<strlen(lpcImgLabel)<<"  lpcImgLabel="<<lpcImgLabel);
        LOG4CPLUS_ERROR(_logger,"ERR:"<<lpcRevContent<<" Found End Lable Format ERROR!!");
        
        char * lpcResult_json=Creat_json(1,"��ʽ����");
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
    
	/***����PNG��ʼ��־�ĵ�ַ��������־�ĵ�ַ�������ͼƬ���ֽڳ���***/
    int liImgSize=lpcRevContentEnd-lpcTmp+1;
    
    //get end time 
    gettimeofday(&t_end, NULL); 
    LOG4CPLUS_DEBUG(_logger,"in  RecvImg   recv take: "
                 <<float((((long)t_end.tv_sec)*1000+(long)t_end.tv_usec/1000)-(((long)t_start.tv_sec)*1000+(long)t_start.tv_usec/1000))/1000<<" s");
                 
    Pix* pixs = NULL;
	/***��ͼƬ���н���***/
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
    
    if (!pixs) { //�쳣����
        LOG4CPLUS_ERROR(_logger,"in  RecvImg  pixReadMem Fail!!");
                
        FILE *lpoHandleRecv=NULL;
        char writefilename[512];            
        struct timeval timestamp; 
        gettimeofday(&timestamp, NULL); 
        sprintf(writefilename,"%s/%lu_%lu",TessDataPath,pthread_self(),((long)timestamp.tv_sec)*1000+(long)timestamp.tv_usec/1000);
        lpoHandleRecv=fopen(writefilename,"w+");
        fwrite(lpcRevContent,liTotalSize,1,lpoHandleRecv);
        fclose(lpoHandleRecv);
        
        char * lpcResult_json=Creat_json(1,"��ʽ����");
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
    
	/***������׼����ͼƬ�����������***/
    bool Imgtoomuch=false;
    pthread_mutex_lock(&ImgVecLock); //���ImgVecLock�����������߳̾���������ֱ��������ɹ�
    LOG4CPLUS_DEBUG(_logger,"in  RecvImg  recv a img ID="<<loImgInfo.ImgID<<"  Language="<<loImgInfo.Language<<"  now push back into VecImg");
	/***�����������Ѿ�����15�ڵȴ�����ֱ�ӷ��ط�����æ***/
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
        
        char * lpcResult_json=Creat_json(2,"��������æ");
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
    
    

	/***��ʼͨ����������wait������У�һ�����½�����루ʶ���̷߳����źţ���
	�������������ж��Ƿ��������������ӦImgID��ʶ����
	����ȡ��������ѭ����û����ȴ���һ���½���Ĵ���***/
    RespInfo loRespInfo;
    loRespInfo.ImgID=0;
    loRespInfo.Resp=NULL;
    vector<RespInfo>::iterator  lpoiterResp;
    bool lbNotMine=false;
    while(true)
    {
        pthread_mutex_lock(&RespVecLock);  //���RespVecLock�����������߳̾���������ֱ��������ɹ�
        while(VecResp.size()==0||lbNotMine)
        {
            lbNotMine=false;
            pthread_cond_wait(&RespVecCond,&RespVecLock); //RespVecCond��Ӧ���źŷ����������һ��RespVecLock���ŷ��أ���Ȼһֱ�������������̻��ͷ�RespVecLock
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
    
	/***��ʶ���������HTTP����***/
    char * lpcResult_json=NULL;
    lpcResult_json=Creat_json(0,loRespInfo.Resp);
    
	//LOG4CPLUS_ERROR(_logger,"send lpcResult_json=\n"<<lpcResult_json);
    signal( SIGPIPE, SIG_IGN ); //����socket����ʧ�ܵ��źţ���Ϊ���źŻᵼ�³���coredump
	/***���ؽ�����ͻ���***/
    if(-1 == send(client_sockfd,lpcResult_json,strlen(lpcResult_json)+1, 0)) //�쳣����
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


	/***�ͷ���Դ***/
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


//�����̣߳��������������˿ڡ��������󡢶���ÿ�������½������߳̽��д���������һ�μ����ȴ�����
void * ListenForImg(void * inputbuffer)
{ 
	pthread_detach(pthread_self());
    int listenfd;
    int *lpiConnectfd;
    struct sockaddr_in server;
    struct sockaddr_in client;
    socklen_t sin_size;
    
    if((listenfd=socket(AF_INET,SOCK_STREAM,0))==-1){ //����һ��socket���
	    LOG4CPLUS_ERROR(_logger,"in  ListenForImg   socket  fail!");
        perror("Creating socket failed.");
        exit(1);
    }
    

	/***��������ͨ�Žṹ�壬����TCP/UDPЭ�飬���յ�Ŀ��IP��ַ����ΪINADDR_ANY��ȫ�����������Ķ˿�***/
    bzero(&server,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=htonl(INADDR_ANY);
    server.sin_port=htons(45758);

    
	/***���ý��ջ�������С***/
    int liRecvBuf=32*1024;//����Ϊ32K
    setsockopt(listenfd,SOL_SOCKET,SO_RCVBUF,(const char*)&liRecvBuf,sizeof(int));
    
    /***���ö˿ڿ����á���ʱʱ��***/
	int bReuseaddr=1; 
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(bReuseaddr));
    struct timeval timeout={10,0};//10s
    int ret=setsockopt(listenfd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
    ret=setsockopt(listenfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
    
	/***������ͨ�Žṹ���������socket�����***/
    if(bind(listenfd,(struct sockaddr*)&server,sizeof(struct sockaddr))==-1){
	    LOG4CPLUS_ERROR(_logger,"in  ListenForImg   bind  fail!");
        perror("bind() error");
        exit(1);
    }
    
    sin_size=sizeof(struct sockaddr_in);
    if (listen(listenfd,45758)<0)  //��ʼ�����˿�
    {
	    LOG4CPLUS_ERROR(_logger,"in  ListenForImg   listen  fail!");
        perror("listen() error\n");
        exit(1);
    }
    
    /***��ʼѭ����������ÿ�������½�һ�������߳̽��д���***/
    while(true)
    {  
        lpiConnectfd = (int *)malloc(sizeof(int));  //Ϊ����ÿ���½��Ĵ����̶߳���һ������ڴ�ռ䣬��Ҫÿ�ζ�malloc����һ��ռ�Ȼ����pthread_create
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
        if(pthread_create(&recvimg_arrpthread_t, NULL, RecvImg, (void*)lpiConnectfd)!=0)//���������߳�  
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


//ʶ������(��ʱ��)ʶ��ʼǰ�������̣߳���ʶ��ʱ�䳬��15�룬���ڴ��߳��ж�ʶ������ж�
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


//ʶ���̣߳����������ȴ�ͼƬ�����źš�ȡ��ͼƬ������ʶ�������̡߳�ʶ�𡢹ر�ʶ�����ӡ�ʶ�������ˡ�ʶ����д����Ӧ���С�������һ�εȴ�ͼƬ�����ź�
void * OCRImg(void * inputbuffer)  
{  
	pthread_detach(pthread_self());
    struct timeval t_start,t_end; 
  
    tesseract::TessBaseAPI *api=static_cast<tesseract::TessBaseAPI*>(inputbuffer);  //�����߳�ֻ����void *���͵���Σ�������Ҫ����ʱǿ��ת��
																					//�ɣ�void *��������ȫ����ת����TessBaseAPI
    int liOCRCount=0;  //ʶ�������
    

	//��ʼѭ��wait������У������µ�ͼƬ����������У������̴߳�����źţ�ʱ������һ��ʶ���߳�ȡ��һ��ͼƬ����ʶ��
    while(true)
    {
        LOG4CPLUS_DEBUG(_logger,"in  OCRImg   liOCRCount="<<liOCRCount);
        
        LOG4CPLUS_DEBUG(_logger,"in  OCRImg   before get ImgVecLock");
		/***������wait�źš�ȡ��ͼƬ������***/
        pthread_mutex_lock(&ImgVecLock);//���ImgVecLock�����������߳̾���������ֱ��������ɹ�
        while(VecImg.size()==0)
        {  
            pthread_cond_wait(&ImgVecCond,&ImgVecLock);  //ImgVecCond��Ӧ���źŷ����������һ��ImgVecLock���ŷ��أ���Ȼһֱ�������������̻��ͷ�ImgVecLock
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
        
        /***tesstwo�Ļ���ʶ���裬SetImage->GetUTF8Text***/
	    api->SetImage(loImgInfo.ImgPixs);
 
		/***GetUTF8Textǰ����һ��ʶ�����ӣ���ʶ��ʱ����м�⣬��ʱ����ж�ʶ��***/
 		pthread_t  ocralarm_pthread_t;
 		int liPthCreatRet=0;
 		if((liPthCreatRet=pthread_create(&ocralarm_pthread_t, NULL, OCRAlarm, inputbuffer))!=0)//�������߳�  
        {  
            LOG4CPLUS_ERROR(_logger,"in  OCRImg   pthread_create Fail!(errno:"<<liPthCreatRet<<"-"<<strerror(liPthCreatRet)<<")");  
        }
        api->Start();
        //public static final int USE_ALL = 0;
        //public static final int CHI_ONLY = 1;
        //public static final int ENG_ONLY = 2;
        //public static final int CHI_ENG  = 3;
	    char * text=api->GetUTF8TextWith(loImgInfo.Language);  //ʶ��
        
		/***ȡ��ʶ�������߳�***/
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
	    if(pthread_cancel(ocralarm_pthread_t)!=0)
	    {
	      LOG4CPLUS_ERROR(_logger,"in  OCRImg   pthread_cancel Fail!");  
	    }
        pthread_detach(ocralarm_pthread_t);

		/***���ʶ��״̬������ͼƬ����***/
		api->Start();
        api->Clear();
        pixDestroy(&loImgInfo.ImgPixs);
        //char * text=NULL;

		/***��ʶ�������жϡ����˲���***/
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
        
		/***���������źţ�ʶ����������Ӧ���У�����***/
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



//����������������������������Ŀ¼��ƴ�����԰�·����ƴ����־·������ʼ����־���󡢳�ʼ��N��ʶ���������N��ʶ���̡߳����������̡߳�����ȴ������̵߳��˳�
int main(int argc, char **argv) {
    if(argc!=2)
    {
        printf("ERROR:argc!=2!\n");
        return -1;
    }
    
	/***��ȡ����ȫ·��***/
    char exec_name [512];
    char lpcTessDataPath [512];
    exec_name[readlink ("/proc/self/exe", exec_name, 512)-9]='\0'; //�õ�ȫ·���洢��exec_name�У�Ȼ��/home/...../bin/Tesseract������"Tesseract"ȥ�����õ��ļ���·��
  
    sprintf(lpcTessDataPath,"%stessdatas",exec_name); //ͨ������õ���·��exec_nameƴ�ӳ����԰�·��
    TessDataPath=lpcTessDataPath;

    init_daemon(); //�ػ�����
    prctl(PR_SET_DUMPABLE, 1); //�������coredumpʱ����core�ļ��������ⶨλ������
    
    /* step 1: Instantiate an appender object */
    char logfilename[512];
    sprintf(logfilename,"%s%s",exec_name,argv[1]); //ͨ������õ���·��exec_nameƴ�ӳ���־�ļ�ȫ·��
    SharedAppenderPtr _append(new RollingFileAppender(logfilename, 50*1024*1024, 10));
    _append->setName("file log test");
    /* step 4: Instantiate a logger object */
    std::string pattern = "%D{%m/%d/%y %H:%M:%S}[%t][%L]%p - %m%n";
    std::auto_ptr<log4cplus::Layout> _layout(new PatternLayout(pattern));
    /* step 3: Attach the layout object to the appender */
    _append->setLayout( _layout );
    _logger = Logger::getInstance("test.subtestof_filelog");
    /* step 5: Attach the appender object to the logger  */
    _logger.addAppender(_append);  //��ʼ����־�������
    /* log activity */
    
    LOG4CPLUS_DEBUG(_logger,"LOGFile: "<<logfilename); //��һ����־
    
    int i; 
	pthread_t  listenforimg_arrpthread_t;   //�����߳̽ṹ�壬���ڴ洢�̵߳�����״̬����Ϣ
	pthread_t  ocr_arrpthread_t[OCR_THREAD_NUM];//ʶ���߳̽ṹ����У����ڴ洢�̵߳�����״̬����Ϣ
	
    tesseract::TessBaseAPI *baseapi[OCR_THREAD_NUM];  //ʶ�����Ķ���

	struct timeval t_start,t_end; 
	

	//forѭ����ʼ��OCR_THREAD_NUM����ʶ�����
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
	

	//����OCR_THREAD_NUM����ʶ���߳�
    for ( i = 0; i < OCR_THREAD_NUM&&SERVERVERSION; i++)  
	{
        if(pthread_create(&ocr_arrpthread_t[i], NULL, OCRImg, (void*)(baseapi[i]))!=0)//�������߳�  
        {  
            perror("OCRImg pthread_create");  
        }
	}
	

	//���������߳�
	if(SERVERVERSION)
	{
        if(pthread_create(&listenforimg_arrpthread_t, NULL,ListenForImg,NULL)!=0)//�������߳�  
        {  
            LOG4CPLUS_ERROR(_logger,"ERR:ListenForImg Fail!");
        }
    }
    

	//ͨ���߳̽ṹ��ȴ��߳��˳�
    for ( i = 0; i < OCR_THREAD_NUM&&SERVERVERSION; i++)  
	{
		pthread_join (ocr_arrpthread_t[i], NULL);  //ocr_arrpthread_[i]��Ӧ���߳��˳��򷵻أ���Ȼһֱ����
	}

	pthread_join (listenforimg_arrpthread_t, NULL);  //listenforimg_arrpthread_t��Ӧ���߳��˳��򷵻أ���Ȼһֱ����
    return 0;

}


