#include "qt_hikvision.h"

#include <stdio.h>
#include <iostream>
#include <Windows.h>
#include <time.h>

using namespace std;

typedef HWND (WINAPI* PROCGETCONSOLEWINDOW)();
PROCGETCONSOLEWINDOW GetConsoleWindowAPI;

LONG lPort = -1; //ȫ�ֵĲ��ſ�port��

void CALLBACK g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* dwUser)
{
    printf("Real play callback.\n");
    HWND hWnd = GetConsoleWindowAPI();

    switch (dwDataType)
    {
    case NET_DVR_SYSHEAD: //ϵͳͷ
        if (lPort >= 0)
        {
            break;  //��ͨ��ȡ��֮ǰ�Ѿ���ȡ������������ӿڲ���Ҫ�ٵ���
        }

        if (!PlayM4_GetPort(&lPort))  //��ȡ���ſ�δʹ�õ�ͨ����
        {
            break;
        }
        //m_iPort = lPort; //��һ�λص�����ϵͳͷ������ȡ�Ĳ��ſ�port�Ÿ�ֵ��ȫ��port���´λص�����ʱ��ʹ�ô�port�Ų���
        if (dwBufSize > 0)
        {
            if (!PlayM4_SetStreamOpenMode(lPort, STREAME_REALTIME))  //����ʵʱ������ģʽ
            {
                break;
            }

            if (!PlayM4_OpenStream(lPort, pBuffer, dwBufSize, 1024 * 1024)) //�����ӿ�
            {
                break;
            }

            if (!PlayM4_Play(lPort, hWnd)) //���ſ�ʼ
            {
                break;
            }
        }
        break;
    case NET_DVR_STREAMDATA:   //��������
        if (dwBufSize > 0 && lPort != -1)
        {
            if (!PlayM4_InputData(lPort, pBuffer, dwBufSize))
            {
                break;
            }
        }
        break;
    default: //��������
        if (dwBufSize > 0 && lPort != -1)
        {
            if (!PlayM4_InputData(lPort, pBuffer, dwBufSize))
            {
                break;
            }
        }
        break;
    }
}

void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void* pUser)
{
    char tempbuf[256] = { 0 };
    switch (dwType)
    {
    case EXCEPTION_RECONNECT:    //Ԥ��ʱ����
        printf("----------reconnect--------%d\n", time(NULL));
        break;
    default:
        break;
    }
}

qt_hikvision::qt_hikvision(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    printf("program start.\n");

    //---------------------------------------
    // ��ʼ��
    //NET_DVR_Init();
    if (false == NET_DVR_Init()) {
        printf("Camera initial failed : %d\n", NET_DVR_GetLastError());
        return;
    }
    //��������ʱ��������ʱ��
    NET_DVR_SetConnectTime(2000, 1);
    NET_DVR_SetReconnect(10000, true);

    //---------------------------------------
    //�����쳣��Ϣ�ص�����
    NET_DVR_SetExceptionCallBack_V30(0, NULL, g_ExceptionCallBack, NULL);

    //---------------------------------------
    // ��ȡ����̨���ھ��
    //HMODULE hKernel32 = GetModuleHandle("kernel32");
    //GetConsoleWindowAPI = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32,"GetConsoleWindow");

    //---------------------------------------
    // ע���豸
    //��¼�����������豸��ַ����¼�û��������
    struLoginInfo.bUseAsynLogin = 0; //ͬ����¼��ʽ
    strcpy(struLoginInfo.sDeviceAddress, "172.20.64.2"); //�豸IP��ַ
    struLoginInfo.wPort = 8000; //�豸����˿�
    strcpy(struLoginInfo.sUserName, "admin"); //�豸��¼�û���
    strcpy(struLoginInfo.sPassword, "1qaz2wsx"); //�豸��¼����

    //�豸��Ϣ, �������
    NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = { 0 };

    lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
    //    lUserID = NET_DVR_Login_V30("172.20.64.1", 8000, "admin", "1qaz2wsx", &struDeviceInfoV40);
    if (lUserID < 0)
    {
        printf("Login failed, error code: %d\n", NET_DVR_GetLastError());
        NET_DVR_Cleanup();
        return;
    }
}

qt_hikvision::~qt_hikvision() {
    if (-1 != lRealPlayHandle) {
        on_pushButton_pause_clicked();
    }

    //ע���û�
    NET_DVR_Logout(lUserID);
    //�ͷ�SDK��Դ
    NET_DVR_Cleanup();
    printf("Stop real play.\n");
}

void qt_hikvision::on_pushButton_play_clicked() {
    if (-1 != lRealPlayHandle) {
        return;
    }

    //---------------------------------------
    //����Ԥ�������ûص�������
    NET_DVR_PREVIEWINFO struPlayInfo = { 0 };
    //HWND hWnd = GetConsoleWindowAPI();     //��ȡ���ھ��
    HWND hWnd = (HWND)ui.label->winId();
    struPlayInfo.hPlayWnd = hWnd;         //��ҪSDK����ʱ�����Ϊ��Чֵ����ȡ��������ʱ����Ϊ��
    //struPlayInfo.hPlayWnd = NULL;         //��ҪSDK����ʱ�����Ϊ��Чֵ����ȡ��������ʱ����Ϊ��
    struPlayInfo.lChannel = 1;       //Ԥ��ͨ����
    struPlayInfo.dwStreamType = 0;       //0-��������1-��������2-����3��3-����4���Դ�����
    struPlayInfo.dwLinkMode = 0;       //0- TCP��ʽ��1- UDP��ʽ��2- �ಥ��ʽ��3- RTP��ʽ��4-RTP/RTSP��5-RSTP/HTTP
    struPlayInfo.bBlocked = 1;       //0- ������ȡ����1- ����ȡ��

    lRealPlayHandle = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, NULL, NULL);
    //lRealPlayHandle = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, g_RealDataCallBack_V30, NULL);
    if (lRealPlayHandle < 0)
    {
        printf("NET_DVR_RealPlay_V40 error: %d\n", NET_DVR_GetLastError());
        NET_DVR_Logout(lUserID);
        NET_DVR_Cleanup();
        return;
    }
    printf("NET_DVR_RealPlay_V40.\n");
    //Sleep(10000);
}

void qt_hikvision::on_pushButton_pause_clicked() {
    if (-1 == lRealPlayHandle) {
        return;
    }

    //---------------------------------------
    //�ر�Ԥ��
    NET_DVR_StopRealPlay(lRealPlayHandle);
    lRealPlayHandle = -1;
    printf("NET_DVR_StopRealPlay.\n");
    //�ͷŲ��ſ���Դ
    //PlayM4_Stop(lPort);
    //PlayM4_CloseStream(lPort);
    //PlayM4_FreePort(lPort);
}