#include <iostream>

using namespace std;

#include <opencv/cv.h>
#include <opencv/highgui.h>
//#include "opencv/opencv2.20/include/opencv2/imgproc/imgproc.hpp"

#define soobel_xcoder 1
#define soobel_ycoder 0
#define soobel_aperture_size 7
#define canny_threold1 120
#define canny_threold2 180
#define canny_aperture_size 3

#define sign 0x7fffffff
#define signed int INT

int scale = 1;
int delta = 0;
int ddepth = CV_16S;

/*
int main()

{
    IplImage *frame,*gray,*sobel,*canny,*laplace,*laplaceRe,*frameInt;

    double M,m;
/*
    IplImage *frame1,*frame2,*gray1,*gray2,*sobel1,*sobel2,*canny1,*canny2;

    frame1=cvLoadImage("F:\\1436871654.2412.jpg");
    frame2=cvLoadImage("F:\\1436871704.1238.jpg");
    gray1=cvCreateImage(cvGetSize(frame1),frame1->depth,1);
    gray2=cvCreateImage(cvGetSize(frame2),frame2->depth,1);
    sobel1=cvCreateImage(cvGetSize(frame1),IPL_DEPTH_16S,1);
    sobel2=cvCreateImage(cvGetSize(frame2),IPL_DEPTH_16S,1);
    canny1=cvCreateImage(cvSize(frame1->width,frame1->height),8,1);
    canny2=cvCreateImage(cvSize(frame2->width,frame2->height),8,1);

    cvCvtColor(frame1,gray1,CV_BGR2GRAY);//转为灰度
    cvSobel(gray1,sobel1,1,0,7);//sobel是输出图像
    printf("%s:%d\n",__FILE__,__LINE__);
    cvCvtColor(sobel1,frame1,CV_GRAY2RGB);
    printf("%s:%d\n",__FILE__,__LINE__);
    cvCvtColor(frame1,gray1,CV_RGB2GRAY);
    printf("%s:%d\n",__FILE__,__LINE__);
    cvCanny(gray1,canny1,50,75,3);
    cvNamedWindow("sobel1");
    cvShowImage("sobel1",sobel1);
    cvNamedWindow("canny1");
    cvShowImage("canny1",canny1);

    cvCvtColor(frame2,gray2,CV_BGR2GRAY);//转为灰度
    cvSobel(gray2,sobel2,1,0,7);//sobel是输出图像
    printf("%s:%d\n",__FILE__,__LINE__);
    cvCvtColor(sobel2,frame2,CV_GRAY2RGB);
    printf("%s:%d\n",__FILE__,__LINE__);
    cvCvtColor(frame2,gray2,CV_RGB2GRAY);
    printf("%s:%d\n",__FILE__,__LINE__);
    cvCanny(gray2,canny2,50,75,3);
    cvNamedWindow("sobel2");
    cvShowImage("sobel2",sobel2);
    cvNamedWindow("canny2");
    cvShowImage("canny2",canny2);
    cvWaitKey(0);

    cvReleaseImage(&frame1);
    cvReleaseImage(&gray1);
    cvReleaseImage(&sobel1);
    cvReleaseImage(&canny1);
    cvReleaseImage(&frame2);
    cvReleaseImage(&gray2);
    cvReleaseImage(&sobel2);
    c01.int main(int argc, char ** argv)
02.{
03.    IplImage* img = cvLoadImage("lena.bmp",CV_LOAD_IMAGE_GRAYSCALE);
04.    IplImage* tmp = cvCloneImage(img);
05.    IplImage* dst = cvCreateImage(cvGetSize(img),IPL_DEPTH_64F,img->nChannels);
06.    double M,m;
07.    cvScale(img,dst,1.0,0.0);
08.    cvAdd(dst,dst,dst);
09.    cvMinMaxLoc(dst, &m, &M, NULL, NULL, NULL);
10.    cout<<"M="<<M<<endl<<"m="<<m<<endl;
11.
12.    cvScale(dst,tmp,1.0,0.0);
13.
14.    cvScale(dst, dst, 1.0/(M-m), 1.0*(-m)/(M-m));//图像数据转换到[0,1]区间
15.
16.    cvMinMaxLoc(dst, &m, &M, NULL, NULL, NULL);
17.    cout<<"M="<<M<<endl<<"m="<<m<<endl;
18.
19.    cvNamedWindow("img",CV_WINDOW_AUTOSIZE);
20.    cvShowImage("img",img);
21.
22.    cvNamedWindow("tmp",CV_WINDOW_AUTOSIZE);
23.    cvShowImage("tmp",tmp);
24.
25.    cvNamedWindow("dst",CV_WINDOW_AUTOSIZE);
26.    cvShowImage("dst",dst);
27.
28.    cvWaitKey(-1);
29.    cvDestroyAllWindows();
30.    cvReleaseImage(&dst);
31.    return 0;
32.}
vReleaseImage(&canny2);
    cvDestroyWindow("sobel1");
    cvDestroyWindow("sobel2");
*/
/*
        int  i=1;
        char path[100];
while(true){


        memset(path,0,sizeof(path));
        sprintf(path,"C:\\Users\\mywork\\Desktop\\ClearSample\\Clear%d.png",i);
        //sprintf(path,"C:\\Users\\mywork\\Desktop\\ClearSample\\Clear%d.png",i);
        if(i<12)
            i++;
        else
            i=1;
        frame=cvLoadImage(path,0);//加载图像

        gray=cvCreateImage(cvGetSize(frame),IPL_DEPTH_8U,1);//分配图像空间

        sobel=cvCreateImage(cvGetSize(frame),IPL_DEPTH_16S,1);

        canny=cvCreateImage(cvSize(frame->width,frame->height),IPL_DEPTH_8U,1);

        laplace = cvCreateImage(cvGetSize(frame),IPL_DEPTH_16S,frame->nChannels) ;

        laplaceRe = cvCreateImage(cvGetSize(frame),IPL_DEPTH_64F,frame->nChannels) ;

        frameInt = cvCreateImage(cvGetSize(frame),IPL_DEPTH_64F,frame->nChannels) ;

 //      cvNamedWindow("frame");

       cvNamedWindow("gray");

 //       cvNamedWindow("sobel");

        cvNamedWindow("laplace");

         cvNamedWindow("laplacebefore");

       // cvCvtColor(frame,gray,CV_BGR2GRAY);//转为灰度
        //cvConvertScale(frame,gray,32767+32768);
 //       cvSobel(gray,sobel,soobel_xcoder,soobel_ycoder,soobel_aperture_size);//sobel是输出图像
 //       cvSobel( gray, sobel, ddepth, 0, 1, 3, scale, delta);//, BORDER_DEFAULT );

 //       cvMinMaxLoc(sobel, &m, &M, NULL, NULL, NULL);

 //       cout<<M<<" "<<m<<endl;
 //       cvScale(sobel, gray,  255.0/(M-m),(double)255.0*(-m)/(M-m));//);//图像数据转换到[0,1]区间

//        cvCanny(gray,canny,canny_threold1,canny_threold2,canny_aperture_size);

        cvLaplace(frame, laplace ,7) ;

        cvShowImage("laplacebefore",laplace);

        double minVal = 0.0;
        double maxVal = 0.0;

        cvMinMaxLoc( frame, &minVal, &maxVal );

        printf("minVal=%g,maxVal=%g\n",minVal,maxVal);

        cvConvertScale(frame, frameInt, 65535/(maxVal-minVal),-32768);

        for(int i=0;i<laplace->height;i++)
        {
            for(int j=0;j<laplace->width;j++)
            {
                for(int k=0;k<laplace->nChannels;k++)
                {
               //     printf("imageData=%d\n",*(laplace->imageData+i*laplace->widthStep+j*laplace->nChannels*((sign&laplace->depth)>>3)+k*((sign&laplace->depth)>>3)) );
                    *(laplace->imageData+i*laplace->widthStep+j*laplace->nChannels*((sign&laplace->depth)>>3)+k*((sign&laplace->depth)>>3)) =*(laplace->imageData+i*laplace->widthStep+j*laplace->nChannels*((sign&laplace->depth)>>3)+k*((sign&laplace->depth)>>3))+ *(frameInt->imageData+i*frameInt->widthStep+j*frameInt->nChannels*((sign&frameInt->depth)>>3)+k*((sign&frameInt->depth)>>3));
               //     printf("imageData1=%d\n",*(laplace->imageData+i*laplace->widthStep+j*laplace->nChannels*((sign&laplace->depth)>>3)+k*((sign&laplace->depth)>>3)) );
                }
            }
        }
        /*cvConvertScale(laplace, laplace, (32768+32767)/(maxVal-minVal), -32768-(minVal)*(32768+32767)/(maxVal-minVal));
        cvAbs(laplace,laplace);
        cvMinMaxLoc( laplace, &minVal, &maxVal );*/

        /*
        cvConvertScale(laplace, laplaceRe, 1.0,0.0);
        cvPow(laplaceRe,laplaceRe,2);
        cvPow(laplaceRe,laplaceRe,0.5);
        double minVal = 0.0;
        double maxVal = 0.0;
        cvMinMaxLoc( laplaceRe, &minVal, &maxVal );
        printf("minVal=%g,maxVal=%g\n",minVal,maxVal);
        cvConvertScale(laplaceRe, laplaceRe, 1.0/(maxVal-minVal),0.0);
        cvMinMaxLoc( laplaceRe, &minVal, &maxVal );
        printf("minVal=%g,maxVal=%g\n",minVal,maxVal);
        */
//        printf("minVal=%g,maxVal=%g\n",minVal,maxVal);

 //       cvCanny(gray,canny,90,190,3);

   //     cvShowImage("frame",frame);//显示图像
/*
        cvShowImage("gray",frameInt);

  //      cvShowImage("sobel",sobel);

  //      cvShowImage("canny",canny);

        cvShowImage("laplace",laplace);

        cvWaitKey(0);//等待




    cvReleaseImage(&frame);//释放空间（对视频处理很重要，不释放会造成内存泄露）

    cvReleaseImage(&gray);

    cvReleaseImage(&sobel);

     cvReleaseImage(&canny);
      cvReleaseImage(&laplace);

  //  cvDestroyWindow("frame");

    cvDestroyWindow("gray");

 //   cvDestroyWindow("sobel");

  //   cvDestroyWindow("canny");

     cvDestroyWindow("laplace");


/*

        frame=cvLoadImage("F:\\1436871704.1238.jpg");//加载图像

        gray=cvCreateImage(cvGetSize(frame),IPL_DEPTH_8U,1);//分配图像空间

        sobel=cvCreateImage(cvGetSize(frame),IPL_DEPTH_16S,1);

        canny=cvCreateImage(cvSize(frame->width,frame->height),IPL_DEPTH_8U,1);

 //      cvNamedWindow("frame");

       cvNamedWindow("gray");

        cvNamedWindow("sobel");

        cvNamedWindow("canny");

        cvCvtColor(frame,gray,CV_BGR2GRAY);//转为灰度

        cvSobel(gray,sobel,soobel_xcoder,soobel_ycoder,soobel_aperture_size);//sobel是输出图像
 //       cvSobel( gray, sobel, ddepth, 0, 1, 3, scale, delta);//, BORDER_DEFAULT );

        cvMinMaxLoc(sobel, &m, &M, NULL, NULL, NULL);

        cout<<M<<" "<<m<<endl;
        cvScale(sobel, gray,  256.0/(M-m),(double)256.0*(-m)/(M-m));//);//图像数据转换到[0,1]区间

        cvCanny(gray,canny,canny_threold1,canny_threold2,canny_aperture_size);

//        cvCanny(sobel,canny,40,5,3);

   //     cvShowImage("frame",frame);//显示图像

        cvShowImage("gray",gray);

        cvShowImage("sobel",sobel);

        cvShowImage("canny",canny);

        cvWaitKey(0);//等待




    cvReleaseImage(&frame);//释放空间（对视频处理很重要，不释放会造成内存泄露）

    cvReleaseImage(&gray);

    cvReleaseImage(&sobel);

     cvReleaseImage(&canny);

  //  cvDestroyWindow("frame");

    cvDestroyWindow("gray");

    cvDestroyWindow("sobel");

     cvDestroyWindow("canny");

     */
  //   return 0;

/*
}










    return 0;

}



/*
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdlib.h>
#include <stdio.h>

using namespace cv;

int main( int argc, char** argv )
{

  Mat src, src_gray;
  Mat grad;
  char* window_name = "Sobel Demo - Simple Edge Detector";
  int scale = 1;
  int delta = 0;
  int ddepth = CV_16S;

  int c;



  // 装载图像
  src = imread( "F:\\52829DEAEDE661A8D17AA605CC5D905C.jpg" );

  if( !src.data )
  { return -1; }

  GaussianBlur( src, src, Size(3,3), 0, 0, BORDER_DEFAULT );

  // 转换为灰度图
  cvtColor( src, src_gray, CV_RGB2GRAY );

  //创建显示窗口
  namedWindow( window_name, CV_WINDOW_AUTOSIZE );

  // 创建 grad_x 和 grad_y 矩阵
  Mat grad_x, grad_y;
  Mat abs_grad_x, abs_grad_y;

  // 求 X方向梯度
  //Scharr( src_gray, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );
  Sobel( src_gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT );
  convertScaleAbs( grad_x, abs_grad_x );

  // 求Y方向梯度
  //Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
  Sobel( src_gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT );
  convertScaleAbs( grad_y, abs_grad_y );

  // 合并梯度(近似)
  addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad );

  imshow( window_name, grad );

  waitKey(0);

  return 0;
  }*/

//#define thresholdnum 30
//#define blocksize 5

//#include <cv.h>
//#include <highgui.h>

/*
int Fuzzy_or_Clear_Judge(IplImage *src,int threshold1,int blocksize=3)
{
    double  graycounter=0,graycounter1=0,graycounter2=0;//全局平均灰度和边缘平均灰度
    int thresholdnum=0;
//    memset(graycount,0,sizeof(graycount));
    double judge=0;
    double minVal = 0.0;
    double maxVal = 0.0;
    CvPoint MinLocation;
    CvPoint MaxLocation;
//    IplImage *frame,*gray,*sobelx,*sobely,*sobel,*sobelxABS,*sobelyABS;
//    IplImage *frame=cvLoadImage(path);//加载图像
    IplImage *gray=cvCreateImage(cvGetSize(src),src->depth,1);//分配图像空间
    IplImage *sobelx=cvCreateImage(cvGetSize(src),IPL_DEPTH_16S,1);
    IplImage *sobely=cvCreateImage(cvGetSize(src),IPL_DEPTH_16S,1);
    IplImage *sobel=cvCreateImage(cvGetSize(src),IPL_DEPTH_16S,1);
//    IplImage *sobelx8u=cvCreateImage(cvGetSize(sobelx),IPL_DEPTH_8U,1);
//    IplImage *sobely8u=cvCreateImage(cvGetSize(sobely),IPL_DEPTH_8U,1);
//    IplImage *sobel8u=cvCreateImage(cvGetSize(sobely),IPL_DEPTH_8U,1);
//    IplImage *threshold=cvCreateImage(cvGetSize(src),IPL_DEPTH_8U,1);
    cvZero(gray);
    cvZero(sobelx);
    cvZero(sobely);
    cvZero(sobel);

    cvCvtColor(src,gray,CV_BGR2GRAY);//转为灰度

    cvSobel(gray,sobely,0,1,blocksize);//y方向的sobel

    cvSobel(gray,sobelx,1,0,blocksize);//x方向的sobel

//    cvMinMaxLoc( sobely, &minVal, &maxVal ,&MinLocation ,&MaxLocation );

//    cvMinMaxLoc( sobelx, &minVal, &maxVal ,&MinLocation ,&MaxLocation );




    for(int i=0;i<sobelx->height;i++)
    {
       for(int j=0;j<sobelx->width;j++)
       {
            for(int k=0;k<sobelx->nChannels;k++)
            {
                int16_t * ldtmp=(int16_t *)(sobel->imageData+i*sobel->widthStep+j*sobel->nChannels*((sign&sobel->depth)>>3)+k*((sign&sobel->depth)>>3));
                int16_t * ldtmp1=(int16_t *)(sobelx->imageData+i*sobelx->widthStep+j*sobelx->nChannels*((sign&sobelx->depth)>>3)+k*((sign&sobelx->depth)>>3));
                int16_t * ldtmp2=(int16_t *)(sobely->imageData+i*sobely->widthStep+j*sobely->nChannels*((sign&sobely->depth)>>3)+k*((sign&sobely->depth)>>3));
                if(* ldtmp1<0)
                {

                    * ldtmp1=-1*(* ldtmp1);
                }
                if(* ldtmp2<0)
                {
                    * ldtmp2=-1*(* ldtmp2);
                }

                if(* ldtmp1<* ldtmp2)
                {

                    * ldtmp=* ldtmp2;

                }
                else
                {
                    * ldtmp=* ldtmp1;
                }
                graycounter=graycounter+(*ldtmp);

            }
        }
    }
/*
    for(int i=0;i<sobel->height;i++)
    {
       for(int j=0;j<sobel->width;j++)
       {
            for(int k=0;k<sobel->nChannels;k++)
            {
                int * ldtmp=(int *)(sobel->imageData+i*sobel->widthStep+j*sobel->nChannels*((sign&sobel->depth)>>3)+k*((sign&sobel->depth)>>3));
                int16_t * ldtmp1=(int16_t *)(sobelx->imageData+i*sobelx->widthStep+j*sobelx->nChannels*((sign&sobelx->depth)>>3)+k*((sign&sobelx->depth)>>3));
                int16_t * ldtmp2=(int16_t *)(sobely->imageData+i*sobely->widthStep+j*sobely->nChannels*((sign&sobely->depth)>>3)+k*((sign&sobely->depth)>>3));

                if(* ldtmp1<* ldtmp2)
                {

                    * ldtmp=* ldtmp2;

                }
                else
                {
                    * ldtmp=* ldtmp1;
                }
                graycounter=graycounter+(*ldtmp);
           //     *ldtmp=pow(* ldtmp,2);//做平方放大运算
           /*
                if(*ldtmp>thresholdnum)
                {
                    graycounter1+=*ldtmp;
                    graycounter2++;
                }
            */

             //   cout<<pow(*ldtmp,2)<<endl;
             //   printf("ldtmp=%d,ldtmp1=%d,ldtmp2=%d,pow(ldtmp)=%g\n",*ldtmp,*ldtmp1,* ldtmp2,pow(*ldtmp,2));
              //  graycount[* ldtmp]++;

//            }
//        }
//    }
/*
    thresholdnum=graycounter/(sobel->height*sobel->width)*2.5;
    for(int i=0;i<sobel->height;i++)
    {
       for(int j=0;j<sobel->width;j++)
       {
            for(int k=0;k<sobel->nChannels;k++)
            {
                int16_t * ldtmp=(int16_t *)(sobel->imageData+i*sobel->widthStep+j*sobel->nChannels*((sign&sobel->depth)>>3)+k*((sign&sobel->depth)>>3));
                if(*ldtmp>thresholdnum)
                {
                    graycounter1+=*ldtmp;
                    graycounter2++;
                }
            }
       }
    }
    /*
    minVal=0.0;
    maxVal=0.0;
    cvMinMaxLoc( sobel, &minVal, &maxVal ,&MinLocation ,&MaxLocation );

    printf("minVal=%g,maxVal=%g  ",minVal,maxVal);

    //   cvConvertScale(sobel,sobel8u,255.0/(maxVal-minVal),0);
    printf("globalaverage=%g   ",graycounter/(sobel->height*sobel->width));*/
 /*   cvReleaseImage(&gray);
    cvReleaseImage(&sobelx);
    cvReleaseImage(&sobely);
    cvReleaseImage(&sobel);


    if(graycounter2!=0)
    {
        judge=(double)graycounter1/(double)graycounter2;
        judge=pow(judge,2);
    }
    else
        judge=0;
 //   printf("judge=%g   ",judge);

    if((graycounter2==0)||(judge<threshold1))
        return 0;
    else
        return 1;

    return -1;




}
*/

int Fuzzy_or_Clear_Judge(IplImage *src,int thresholdnum,int threshold1,int blocksize)
{
    double  graycounter=0,graycounter1=0,graycounter2=0;//全局平均灰度和边缘平均灰度
//    memset(graycount,0,sizeof(graycount));
    double judge=0;
    double minVal = 0.0;
    double maxVal = 0.0;
    CvPoint MinLocation;
    CvPoint MaxLocation;

    IplImage *gray=cvCreateImage(cvGetSize(src),src->depth,1);//分配图像空间

    IplImage *sobel=cvCreateImage(cvGetSize(src),IPL_DEPTH_16S,1);

    IplImage *sobelx=cvCreateImage(cvGetSize(src),IPL_DEPTH_16S,1);

    IplImage *sobely=cvCreateImage(cvGetSize(src),IPL_DEPTH_16S,1);

    IplImage *sobel8u=cvCreateImage(cvGetSize(src),IPL_DEPTH_8U,1);


    cvZero(gray);

    cvZero(sobel);

    cvZero(sobelx);

    cvZero(sobely);

    cvZero(sobel8u);


    cvCvtColor(src,gray,CV_BGR2GRAY);//转为灰度


    cvSobel(gray,sobelx,1,0,blocksize);


	cvSobel(gray,sobely,0,1,blocksize);


    /*
    minVal=0.0;
    maxVal=0.0;
    cvMinMaxLoc( sobel, &minVal, &maxVal ,&MinLocation ,&MaxLocation );

    printf("minVal=%g,maxVal=%g  ",minVal,maxVal);*/

//    cvAbs(sobel,sobel);//取像素绝对值



    for(int i=0;i<sobel->height;i++)
    {
       for(int j=0;j<sobel->width;j++)
       {
            for(int k=0;k<sobel->nChannels;k++)
            {
                int16_t * ldtmp=(int16_t *)(sobel->imageData+i*sobel->widthStep+j*sobel->nChannels*((sign&sobel->depth)>>3)+k*((sign&sobel->depth)>>3));

                int16_t * ldtmp1=(int16_t *)(sobelx->imageData+i*sobelx->widthStep+j*sobelx->nChannels*((sign&sobelx->depth)>>3)+k*((sign&sobelx->depth)>>3));

                int16_t * ldtmp2=(int16_t *)(sobely->imageData+i*sobely->widthStep+j*sobely->nChannels*((sign&sobely->depth)>>3)+k*((sign&sobely->depth)>>3));

                if(* ldtmp1<0) * ldtmp1=-(* ldtmp1);

				if(* ldtmp2<0) * ldtmp2=-(* ldtmp2);

                * ldtmp=* ldtmp1+* ldtmp2;

                if(*ldtmp>thresholdnum)
                {
                    graycounter1+=*ldtmp;
                    graycounter2++;
                }

            }
        }
    }



    int width=sobelx->width;
    int height=sobelx->height;

    minVal=0.0;
    maxVal=0.0;
    cvMinMaxLoc( sobel, &minVal, &maxVal ,&MinLocation ,&MaxLocation );

    printf("minVal=%g,maxVal=%g  ",minVal,maxVal);

    cvConvertScale(sobel,sobel8u,255.0/(maxVal-minVal),0);

    cvNamedWindow("sobel");

    cvShowImage("sobel",sobel8u);//显示图像
//    printf("globalaverage=%g   ",graycounter/(sobel->height*sobel->width));*/
    cvReleaseImage(&gray);
    cvReleaseImage(&sobel);
    cvReleaseImage(&sobelx);
    cvReleaseImage(&sobely);
    cvReleaseImage(&sobel8u);


    if(graycounter2!=0)
    {
        judge=(double)graycounter1/(double)graycounter2;
        judge=pow(judge,2);
    }
    else
        judge=0;

    int area=(width*height)/60;
    printf("judge=%g   graycounter2=%g  ",judge,graycounter2);
    printf("area=%d  ",area);
    if((graycounter2==0)||((judge<threshold1)||(graycounter2<area)))
        return 0;
    else
        return 1;

    return -1;




}


int main()
{
    char path[128];

    int i=1,judge=-1;
    while(i<17)
    {
        memset(path,0,sizeof(path));
        sprintf(path,"C:\\Users\\mywork\\Desktop\\Fuzzy_or_Clear\\ClearSample\\%d.png",i);
 //       printf("%s,%d\n",__FILE__,__LINE__);
        //sprintf(path,"C:\\Users\\mywork\\Desktop\\ClearSample\\Clear%d.png",i);
        /*
        if(i<60)
            i++;
        else
            i=1;
        */
        IplImage *frame=cvLoadImage(path);//加载图像

        judge=Fuzzy_or_Clear_Judge(frame,50,12000,3);
        printf("%d.png    ",i);
        if(!judge)
            printf("True\n");
        else if(judge==1)
            printf("False\n");
        else
            printf("ERROR\n");



        cvNamedWindow("frame");

        cvShowImage("frame",frame);//显示图像


        cvWaitKey(0);//等待

        cvReleaseImage(&frame);//释放空间（对视频处理很重要，不释放会造成内存泄露）
        cvDestroyWindow("frame");
        i++;
    }
    return 0;
}




