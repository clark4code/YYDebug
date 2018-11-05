#ifndef YNETDOCK_H
#define YNETDOCK_H

#include "ycommdock.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTcpServer>
#include <QUdpSocket>

class YNetDock : public YCommDock
{
    Q_OBJECT
public:
    enum ComboBoxT{CB_TYPE, CB_TS_TO, CB_LENGHT};
    enum PushButtonT{PB_OPEN, PB_LENGTH};
    enum LineEditT{LE_UDP_L_PORT, LE_UDP_T_IP, LE_UDP_T_PORT, LE_TC_IP, LE_TC_PORT, LE_TS_PORT, LE_LENGTH};

    explicit YNetDock(SObject *pObj, YMainWindow *parent = 0);
    ~YNetDock();

    bool commOpen() override;
    int commWrite(const void* pData, uint uSize, uint uTimeOut = uint(~0)) override;
    bool commIsOpen() override;
    void commClose() override;
    void updateSObject() override;

    static void initSObject(SObject *pObj);
public slots:
    void updateStatus();
    void clickedOpenBt();
    void readUdpDatagrams();
    void newConnectionOnTcpServer();
    void readTcpSktData();
    void tcpSktDisconnected();
private:
    QComboBox* m_combos[CB_LENGHT];
    QPushButton* m_buttons[PB_LENGTH];
    QLineEdit* m_lineEdits[LE_LENGTH] = {0};
    QUdpSocket* m_udpSkt = NULL;
    QTcpServer* m_tcpserver = NULL;
    QTcpSocket* m_tcpSkt = NULL;
    QList<QTcpSocket*> m_connectedTcpSkts;
};

#endif // YNETDOCK_H
