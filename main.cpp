// frame_diff.cpp : 定义控制台应用程序的入口点。
//

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv/cv.h>

#include <cxcore.hpp>

#include <string>


#include "InitSock.h"

//#define USECOMM
#define threshold_diff 20 //设置简单帧差法阈值
#define WinSize 40

using namespace cv;
using namespace std;


CInitSock initSock; //初始化Winsock库


CvPoint GetCenterPoint(IplImage *src)
{
	int i, j;
	int x0 = 0, y0 = 0, sum = 0;
	CvPoint center;
	CvScalar pixel;
	for (i = 0; i<src->width; i++)
		for (j = 0; j<src->height; j++)
		{
			pixel = cvGet2D(src, j, i);
			if (pixel.val[0] == 255)
			{
				x0 = x0 + i;
				y0 = y0 + j;
				sum = sum + 1;
			}
		}
	if (sum == 0) return center;
	center.x = x0 / sum;
	center.y = y0 / sum;
	return center;
}

CvPoint findInSmallWnd(Mat src, Mat bkg)
{
	Mat dBkg;
	CvPoint centor;
	absdiff(src, bkg, dBkg);

	cvtColor(dBkg, dBkg, CV_BGR2GRAY);
	threshold(dBkg, dBkg, 30, 255, CV_THRESH_BINARY);

	// 膨胀  
	cv::dilate(dBkg, dBkg, cv::Mat(), Point(-1, -1), 1);
	// 腐蚀  
	cv::erode(dBkg, dBkg, cv::Mat(), Point(-1, -1), 2);

	centor = GetCenterPoint(&IplImage(dBkg));	

	imshow("dbkg", dBkg);
	return centor;
}

int main()
{
	


	#pragma region 变量定义
	string imagename;
	string imagename1;
	string imagename2;
	string name;
	char buf[10];
	
	Mat dframe1;

	CvPoint centor;
	
	IplImage *src;
	
	bool winflag = false;

	Rect rect(0, 0, 2 * WinSize, 2 * WinSize);

	Mat img_bkg = imread("..\\L1\\L352.bmp");
	 
	#pragma endregion 变量定义

#ifdef USECOMM
	#pragma region 通讯初始化
	//创建套接字
	SOCKET sListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET)
	{
		printf("Failed socket()\n");
		return 0;
	}
	//填充sockaddr_in结构
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(13000);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	//绑定这个套接字到一个本地地址
	if (::bind(sListen, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf("Failed bind()\n");
		return 0;
	}
	//进入监听模式
	if (::listen(sListen, 2) == SOCKET_ERROR)
	{
		printf("Failed listen()\n");
		return 0;
	}

	//循环接受客户的连接请求
	sockaddr_in remoteAddr;
	int nAddrLen = sizeof(remoteAddr);
	printf("Length of sockaddr_in is: %d\n", nAddrLen);
	SOCKET sClient;
	char szText[] = "TCP Server Inited!\r\n";

	#pragma endregion 通讯初始化

	while (TRUE)
	{		
		sClient = ::accept(sListen, (SOCKADDR *)&remoteAddr, &nAddrLen);
		if (sClient == INVALID_SOCKET)
		{
			printf("Failed accapt()");
			continue;
		}
		//printf("接受到一个连接:%s\r\n", Inet_Ntop(remoteAddr.sin_addr));	
#endif // USECOMM

		for (int i = 352; i < 450; i++)
		{
			#pragma region 读入图像

			imagename = "E:\\ppqColor\\L1\\L";
			sprintf(buf, "%d", i);
			imagename += buf;
			imagename += ".bmp";
			Mat img1 = imread(imagename);

			imagename = "E:\\ppqColor\\L1\\L";
			sprintf(buf, "%d", i + 1);
			imagename += buf;
			imagename += ".bmp";
			Mat img2Src = imread(imagename);
			Mat img2 = img2Src;
			//如果读入图像失败
			if (img1.empty() || img2.empty())
			{
				fprintf(stderr, "Can not load image %s\n", imagename);
				getchar();
				return -1;
			}

#pragma endregion 读入图像

#ifdef USECOMM
			//接收数据
			char buff[256];
			int nRev = ::recv(sClient, buff, 256, 0);
			if (nRev > 0)
			{
				buff[nRev] = '\0';
				printf("接受到数据:%s\n", buff);
			}
#endif // USECOMM

			double t = (double)getTickCount(); //计时


			if (winflag == false) //大窗时更新背景
			{
				img_bkg = img_bkg*0.9 + 0.1 * img2;
			}
			else  // 选取小窗口
			{
				img1 = img1(rect);
				img2 = img2(rect);
				imshow("image_wnd", img1);
			}


			#pragma region 差帧&预处理

			//GaussianBlur(img1, img1, Size(3, 3), 0, 0, BORDER_DEFAULT);
			//GaussianBlur(img2, img2, Size(3, 3), 0, 0, BORDER_DEFAULT);

			absdiff(img2, img1, dframe1);

			cvtColor(dframe1, dframe1, CV_BGR2GRAY);
			threshold(dframe1, dframe1, 30, 255, CV_THRESH_BINARY);

			// 膨胀  
			cv::dilate(dframe1, dframe1, cv::Mat(), Point(-1, -1), 1);
			// 腐蚀  
			cv::erode(dframe1, dframe1, cv::Mat(), Point(-1, -1), 3);

#pragma endregion 差帧&预处理	

			#pragma region 差帧找中心

			src = &IplImage(dframe1);
			centor = GetCenterPoint(src);

#pragma endregion 找中心


			if (centor.x == 0 || centor.y == 0) //没找到点，不处理
			{
				rect.x = 0;
				rect.y = 0;

				winflag = false;
			}
			else //找到点后，更新窗口
			{
				#pragma region 小窗背景剪除

				if (winflag) //背景找中心
				{
					Mat bkgSmall = img_bkg(rect);
					centor = findInSmallWnd(img2, bkgSmall);

				}


				//得到乒乓球的图像坐标
				centor.x += rect.x; //此时的rect还是上一时刻的窗口
				centor.y += rect.y;

				//更新窗口位置，为下次找球做准备
				rect.x = centor.x - WinSize;
				rect.y = centor.y - WinSize;

				rect.x = (rect.x > 0 ? rect.x : 0);
				rect.y = (rect.y > 0 ? rect.y : 0);
				rect.x = (rect.x < (640 - WinSize) ? rect.x : (640 - WinSize));
				rect.y = (rect.y < (480 - WinSize) ? rect.y : (480 - WinSize));

#pragma endregion 小窗背景剪除

				winflag = true;
			}

			//计时
			t = (double)getTickCount() - t;
			cout << "the time is :" << t / getTickFrequency() << endl;

#ifdef USECOMM
			//向客户端发送数据
			::send(sClient, szText, strlen(szText), 0);
#endif USECOMM

			#pragma region 显示

			if (centor.x != 0 && centor.y != 0)
			{
				cout << "x: " << centor.x << endl << "y: " << centor.y << endl;
				circle(img2Src, centor, 3, CV_RGB(0, 255, 0), 1, 8, 3);
			}
			imshow("image_readin", img2Src);
			imshow("image_bkg", img_bkg);

#pragma endregion 显示

			waitKey(25);

			if (i == 449)i = 352;
		}

#ifdef USECOMM
		printf("waitting for new connect\n");
	
	}
#endif USECOMM
	return 0;
}