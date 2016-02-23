// frame_diff.cpp : �������̨Ӧ�ó������ڵ㡣
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
#define threshold_diff 20 //���ü�֡���ֵ
#define WinSize 40

using namespace cv;
using namespace std;


CInitSock initSock; //��ʼ��Winsock��


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

	// ����  
	cv::dilate(dBkg, dBkg, cv::Mat(), Point(-1, -1), 1);
	// ��ʴ  
	cv::erode(dBkg, dBkg, cv::Mat(), Point(-1, -1), 2);

	centor = GetCenterPoint(&IplImage(dBkg));	

	imshow("dbkg", dBkg);
	return centor;
}

int main()
{
	


	#pragma region ��������
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
	 
	#pragma endregion ��������

#ifdef USECOMM
	#pragma region ͨѶ��ʼ��
	//�����׽���
	SOCKET sListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET)
	{
		printf("Failed socket()\n");
		return 0;
	}
	//���sockaddr_in�ṹ
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(13000);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	//������׽��ֵ�һ�����ص�ַ
	if (::bind(sListen, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf("Failed bind()\n");
		return 0;
	}
	//�������ģʽ
	if (::listen(sListen, 2) == SOCKET_ERROR)
	{
		printf("Failed listen()\n");
		return 0;
	}

	//ѭ�����ܿͻ�����������
	sockaddr_in remoteAddr;
	int nAddrLen = sizeof(remoteAddr);
	printf("Length of sockaddr_in is: %d\n", nAddrLen);
	SOCKET sClient;
	char szText[] = "TCP Server Inited!\r\n";

	#pragma endregion ͨѶ��ʼ��

	while (TRUE)
	{		
		sClient = ::accept(sListen, (SOCKADDR *)&remoteAddr, &nAddrLen);
		if (sClient == INVALID_SOCKET)
		{
			printf("Failed accapt()");
			continue;
		}
		//printf("���ܵ�һ������:%s\r\n", Inet_Ntop(remoteAddr.sin_addr));	
#endif // USECOMM

		for (int i = 352; i < 450; i++)
		{
			#pragma region ����ͼ��

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
			//�������ͼ��ʧ��
			if (img1.empty() || img2.empty())
			{
				fprintf(stderr, "Can not load image %s\n", imagename);
				getchar();
				return -1;
			}

#pragma endregion ����ͼ��

#ifdef USECOMM
			//��������
			char buff[256];
			int nRev = ::recv(sClient, buff, 256, 0);
			if (nRev > 0)
			{
				buff[nRev] = '\0';
				printf("���ܵ�����:%s\n", buff);
			}
#endif // USECOMM

			double t = (double)getTickCount(); //��ʱ


			if (winflag == false) //��ʱ���±���
			{
				img_bkg = img_bkg*0.9 + 0.1 * img2;
			}
			else  // ѡȡС����
			{
				img1 = img1(rect);
				img2 = img2(rect);
				imshow("image_wnd", img1);
			}


			#pragma region ��֡&Ԥ����

			//GaussianBlur(img1, img1, Size(3, 3), 0, 0, BORDER_DEFAULT);
			//GaussianBlur(img2, img2, Size(3, 3), 0, 0, BORDER_DEFAULT);

			absdiff(img2, img1, dframe1);

			cvtColor(dframe1, dframe1, CV_BGR2GRAY);
			threshold(dframe1, dframe1, 30, 255, CV_THRESH_BINARY);

			// ����  
			cv::dilate(dframe1, dframe1, cv::Mat(), Point(-1, -1), 1);
			// ��ʴ  
			cv::erode(dframe1, dframe1, cv::Mat(), Point(-1, -1), 3);

#pragma endregion ��֡&Ԥ����	

			#pragma region ��֡������

			src = &IplImage(dframe1);
			centor = GetCenterPoint(src);

#pragma endregion ������


			if (centor.x == 0 || centor.y == 0) //û�ҵ��㣬������
			{
				rect.x = 0;
				rect.y = 0;

				winflag = false;
			}
			else //�ҵ���󣬸��´���
			{
				#pragma region С����������

				if (winflag) //����������
				{
					Mat bkgSmall = img_bkg(rect);
					centor = findInSmallWnd(img2, bkgSmall);

				}


				//�õ�ƹ�����ͼ������
				centor.x += rect.x; //��ʱ��rect������һʱ�̵Ĵ���
				centor.y += rect.y;

				//���´���λ�ã�Ϊ�´�������׼��
				rect.x = centor.x - WinSize;
				rect.y = centor.y - WinSize;

				rect.x = (rect.x > 0 ? rect.x : 0);
				rect.y = (rect.y > 0 ? rect.y : 0);
				rect.x = (rect.x < (640 - WinSize) ? rect.x : (640 - WinSize));
				rect.y = (rect.y < (480 - WinSize) ? rect.y : (480 - WinSize));

#pragma endregion С����������

				winflag = true;
			}

			//��ʱ
			t = (double)getTickCount() - t;
			cout << "the time is :" << t / getTickFrequency() << endl;

#ifdef USECOMM
			//��ͻ��˷�������
			::send(sClient, szText, strlen(szText), 0);
#endif USECOMM

			#pragma region ��ʾ

			if (centor.x != 0 && centor.y != 0)
			{
				cout << "x: " << centor.x << endl << "y: " << centor.y << endl;
				circle(img2Src, centor, 3, CV_RGB(0, 255, 0), 1, 8, 3);
			}
			imshow("image_readin", img2Src);
			imshow("image_bkg", img_bkg);

#pragma endregion ��ʾ

			waitKey(25);

			if (i == 449)i = 352;
		}

#ifdef USECOMM
		printf("waitting for new connect\n");
	
	}
#endif USECOMM
	return 0;
}