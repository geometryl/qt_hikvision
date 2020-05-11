#pragma once

#include <QtWidgets/QWidget>
#include "ui_qt_hikvision.h"

#include "HCNetSDK.h"
#include "plaympeg4.h"

class qt_hikvision : public QWidget
{
    Q_OBJECT

public:
    explicit qt_hikvision(QWidget *parent = Q_NULLPTR);
    ~qt_hikvision();

private Q_SLOTS:
    void on_pushButton_play_clicked();
    void on_pushButton_pause_clicked();

private:
    Ui::qt_hikvisionClass ui;

    LONG lUserID = -1;
    NET_DVR_USER_LOGIN_INFO struLoginInfo = { 0 };
    LONG lRealPlayHandle = -1;
};
