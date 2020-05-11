#include "qt_hikvision.h"

#include <stdio.h>
#include <iostream>
#include <Windows.h>
#include <time.h>

using namespace std;

typedef HWND (WINAPI* PROCGETCONSOLEWINDOW)();
PROCGETCONSOLEWINDOW GetConsoleWindowAPI;

LONG lPort = -1; //全局的播放库port号

void CALLBACK g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* dwUser)
{
    printf("Real play callback.\n");
    HWND hWnd = GetConsoleWindowAPI();

    switch (dwDataType)
    {
    case NET_DVR_SYSHEAD: //系统头
        if (lPort >= 0)
        {
            break;  //该通道取流之前已经获取到句柄，后续接口不需要再调用
        }

        if (!PlayM4_GetPort(&lPort))  //获取播放库未使用的通道号
        {
            break;
        }
        //m_iPort = lPort; //第一次回调的是系统头，将获取的播放库port号赋值给全局port，下次回调数据时即使用此port号播放
        if (dwBufSize > 0)
        {
            if (!PlayM4_SetStreamOpenMode(lPort, STREAME_REALTIME))  //设置实时流播放模式
            {
                break;
            }

            if (!PlayM4_OpenStream(lPort, pBuffer, dwBufSize, 1024 * 1024)) //打开流接口
            {
                break;
            }

            if (!PlayM4_Play(lPort, hWnd)) //播放开始
            {
                break;
            }
        }
        break;
    case NET_DVR_STREAMDATA:   //码流数据
        if (dwBufSize > 0 && lPort != -1)
        {
            if (!PlayM4_InputData(lPort, pBuffer, dwBufSize))
            {
                break;
            }
        }
        break;
    default: //其他数据
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
    case EXCEPTION_RECONNECT:    //预览时重连
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
    // 初始化
    //NET_DVR_Init();
    if (false == NET_DVR_Init()) {
        printf("Camera initial failed : %d\n", NET_DVR_GetLastError());
        return;
    }
    //设置连接时间与重连时间
    NET_DVR_SetConnectTime(2000, 1);
    NET_DVR_SetReconnect(10000, true);

    //---------------------------------------
    //设置异常消息回调函数
    NET_DVR_SetExceptionCallBack_V30(0, NULL, g_ExceptionCallBack, NULL);

    //---------------------------------------
    // 获取控制台窗口句柄
    //HMODULE hKernel32 = GetModuleHandle("kernel32");
    //GetConsoleWindowAPI = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32,"GetConsoleWindow");

    //---------------------------------------
    // 注册设备
    //登录参数，包括设备地址、登录用户、密码等
    struLoginInfo.bUseAsynLogin = 0; //同步登录方式
    strcpy(struLoginInfo.sDeviceAddress, "172.20.64.2"); //设备IP地址
    struLoginInfo.wPort = 8000; //设备服务端口
    strcpy(struLoginInfo.sUserName, "admin"); //设备登录用户名
    strcpy(struLoginInfo.sPassword, "1qaz2wsx"); //设备登录密码

    //设备信息, 输出参数
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

    //注销用户
    NET_DVR_Logout(lUserID);
    //释放SDK资源
    NET_DVR_Cleanup();
    printf("Stop real play.\n");
}

void qt_hikvision::on_pushButton_play_clicked() {
    if (-1 != lRealPlayHandle) {
        return;
    }

    //---------------------------------------
    //启动预览并设置回调数据流
    NET_DVR_PREVIEWINFO struPlayInfo = { 0 };
    //HWND hWnd = GetConsoleWindowAPI();     //获取窗口句柄
    HWND hWnd = (HWND)ui.label->winId();
    struPlayInfo.hPlayWnd = hWnd;         //需要SDK解码时句柄设为有效值，仅取流不解码时可设为空
    //struPlayInfo.hPlayWnd = NULL;         //需要SDK解码时句柄设为有效值，仅取流不解码时可设为空
    struPlayInfo.lChannel = 1;       //预览通道号
    struPlayInfo.dwStreamType = 0;       //0-主码流，1-子码流，2-码流3，3-码流4，以此类推
    struPlayInfo.dwLinkMode = 0;       //0- TCP方式，1- UDP方式，2- 多播方式，3- RTP方式，4-RTP/RTSP，5-RSTP/HTTP
    struPlayInfo.bBlocked = 1;       //0- 非阻塞取流，1- 阻塞取流

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
    //关闭预览
    NET_DVR_StopRealPlay(lRealPlayHandle);
    lRealPlayHandle = -1;
    printf("NET_DVR_StopRealPlay.\n");
    //释放播放库资源
    //PlayM4_Stop(lPort);
    //PlayM4_CloseStream(lPort);
    //PlayM4_FreePort(lPort);
}