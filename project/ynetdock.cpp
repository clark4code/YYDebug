#include "ynetdock.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QUdpSocket>
#include <sobject.h>
#include <ymainwindow.h>

YNetDock::YNetDock(SObject *pObj, YMainWindow *parent) :
    YCommDock(pObj, parent)
{
    QScrollArea* pScrollArea = new QScrollArea(this);
    QGroupBox* pGroups[2];
    QFormLayout* pFLayout[2];
    QVBoxLayout* pVLayout[1];
    QStackedLayout* pParamStackW;
    QWidget* pWidget[5];
    int nCurrentType;

    pWidget[0] = new QWidget(this);
    pGroups[0] = new QGroupBox(tr("Type"), this);
    m_combos[CB_TYPE] = new QComboBox(this);
    m_combos[CB_TYPE]->addItem(tr("UDP"), 0);
    m_combos[CB_TYPE]->addItem(tr("TCP client"), 1);
    m_combos[CB_TYPE]->addItem(tr("TCP server"), 2);
    nCurrentType = m_combos[CB_TYPE]->findText(pObj->property("net_type").toString());
    m_buttons[PB_OPEN] = new QPushButton(tr("Open"), this);
    m_buttons[PB_OPEN]->setAutoFillBackground(true);
    QPalette pal = m_buttons[PB_OPEN]->palette();
    pal.setColor(QPalette::Button, Qt::red);
    m_buttons[PB_OPEN]->setPalette(pal);

    pFLayout[0] = new QFormLayout(pGroups[0]);
    pFLayout[0]->addRow(m_combos[CB_TYPE], m_buttons[PB_OPEN]);
    pGroups[0]->setLayout(pFLayout[0]);

    pGroups[1] = new QGroupBox(tr("Parameters"), this);
    pParamStackW = new QStackedLayout(pGroups[1]);
    pWidget[1] = new QWidget(this);
    pFLayout[1] = new QFormLayout(pWidget[1]);
    m_lineEdits[LE_UDP_L_PORT] = new QLineEdit(pObj->property("udp_lport").toString(), this);
    m_lineEdits[LE_UDP_T_IP] = new QLineEdit(pObj->property("udp_tip").toString(), this);
    m_lineEdits[LE_UDP_T_PORT] = new QLineEdit(pObj->property("udp_tport").toString(), this);
    pFLayout[1]->addRow(new QLabel(tr("Local port"), this), m_lineEdits[LE_UDP_L_PORT]);
    pFLayout[1]->addRow(new QLabel(tr("Target IP"), this), m_lineEdits[LE_UDP_T_IP]);
    pFLayout[1]->addRow(new QLabel(tr("Target port"), this), m_lineEdits[LE_UDP_T_PORT]);
    pParamStackW->addWidget(pWidget[1]);

    pWidget[1] = new QWidget(this);
    pFLayout[1] = new QFormLayout(pWidget[1]);
    m_lineEdits[LE_TC_IP] = new QLineEdit(pObj->property("tcpc_server_ip").toString(), this);
    m_lineEdits[LE_TC_PORT] = new QLineEdit(pObj->property("tcpc_server_port").toString(), this);
    pFLayout[1]->addRow(new QLabel(tr("Server IP"), this), m_lineEdits[LE_TC_IP]);
    pFLayout[1]->addRow(new QLabel(tr("Server port"), this), m_lineEdits[LE_TC_PORT]);
    pParamStackW->addWidget(pWidget[1]);

    pWidget[1] = new QWidget(this);
    pFLayout[1] = new QFormLayout(pWidget[1]);
    m_lineEdits[LE_TS_PORT] = new QLineEdit(pObj->property("tcps_port").toString(), this);
    m_combos[CB_TS_TO] = new QComboBox(this);
    pFLayout[1]->addRow(new QLabel(tr("Server port"), this), m_lineEdits[LE_TS_PORT]);
    pFLayout[1]->addRow(new QLabel(tr("Current client"), this), m_combos[CB_TS_TO]);
    pParamStackW->addWidget(pWidget[1]);

    pVLayout[0] = new QVBoxLayout(pWidget[0]);
    pVLayout[0]->addWidget(pGroups[0]);
    pVLayout[0]->addWidget(pGroups[1]);
    pWidget[0]->setLayout(pVLayout[0]);
    pScrollArea->setWidget(pWidget[0]);
    setWidget(pScrollArea);

    setWindowTitle(tr("Network"));

    connect(m_buttons[PB_OPEN], SIGNAL(clicked()), SLOT(clickedOpenBt()));
    connect(m_combos[CB_TYPE], SIGNAL(currentIndexChanged(int)), pParamStackW, SLOT(setCurrentIndex(int)));
    if(nCurrentType > 0)
        m_combos[CB_TYPE]->setCurrentIndex(nCurrentType);
    updateStatus();
}

YNetDock::~YNetDock()
{
    commClose();
}

bool YNetDock::commOpen()
{
    YMainWindow* pMW = mainWindow();
    QString strNetType = m_combos[CB_TYPE]->currentText();
    if(strNetType == tr("UDP")){
        m_udpSkt = new QUdpSocket(this);
        if(!m_udpSkt->bind((quint16)m_lineEdits[LE_UDP_L_PORT]->text().toInt())){
            delete m_udpSkt;
            m_udpSkt = NULL;
            pMW->appendLog(tr("UDP: The port of %1 has been used."));
            return false;
        }
        connect(m_udpSkt, SIGNAL(readyRead()), SLOT(readUdpDatagrams()));

    }else if(strNetType == tr("TCP server")){
        m_tcpserver = new QTcpServer(this);
        if(!m_tcpserver->listen(QHostAddress::Any, (quint16)m_lineEdits[LE_TS_PORT]->text().toInt())){
            delete m_tcpserver;
            m_tcpserver = NULL;
            pMW->appendLog(tr("TCP server: The port of %1 has been used."));
            return false;
        }
        connect(m_tcpserver, SIGNAL(newConnection()), SLOT(newConnectionOnTcpServer()));
    }else if(strNetType == tr("TCP client")){
        m_tcpSkt = new QTcpSocket(this);
        m_tcpSkt->connectToHost(
                    QHostAddress(m_lineEdits[LE_TC_IP]->text()),
                    (quint16)m_lineEdits[LE_TC_PORT]->text().toInt());
        if(!m_tcpSkt->waitForConnected(3000)){
            delete m_tcpSkt;
            m_tcpSkt = NULL;
            pMW->appendLog(tr("TCP client: It was failed to connect the server of %1:%2.")
                           .arg(m_lineEdits[LE_TC_IP]->text())
                           .arg(m_lineEdits[LE_TC_PORT]->text()));
            return false;
        }
        // connect(m_tcpSkt, SIGNAL(disconnected()), SLOT(clickedOpenBt()));
        connect(m_tcpSkt, SIGNAL(readyRead()), SLOT(readTcpSktData()));
    }
    emit connectedStatus(true);
    m_buttons[PB_OPEN]->setText(tr("Close"));
    QPalette pal = m_buttons[PB_OPEN]->palette();
    pal.setColor(QPalette::Button, Qt::green);
    m_buttons[PB_OPEN]->setPalette(pal);
    return true;
}

int YNetDock::commWrite(const void *pData, uint uSize, uint uTimeOut)
{
    Q_UNUSED(uTimeOut)
    qint64 nRet = 0;
    if(m_udpSkt){
        nRet = m_udpSkt->writeDatagram(
                    (char*)pData,
                    uSize,
                    QHostAddress(m_lineEdits[LE_UDP_T_IP]->text()),
                    (quint16)m_lineEdits[LE_UDP_T_PORT]->text().toUInt());

    }else if(m_tcpserver){
        if(m_combos[CB_TS_TO]->count() < 1)
            return 0;
        nRet = ((QTcpSocket*)m_combos[CB_TS_TO]->currentData().value<void*>())
                ->write((char*)pData, uSize);
    }else if(m_tcpSkt){
        nRet = m_tcpSkt->write((char*)pData, uSize);
    }
    if(nRet < 0)
        nRet = 0;
    mainWindow()->addTxCounter(nRet);
    return (int)nRet;
}

bool YNetDock::commIsOpen()
{
    if(m_udpSkt != NULL
            || m_tcpserver != NULL
            || m_tcpSkt != NULL)
        return true;
    return false;
}

void YNetDock::commClose()
{
    if(m_udpSkt != NULL){
        emit connectedStatus(false);
        delete m_udpSkt;
        m_udpSkt = NULL;
    }else if(m_tcpserver != NULL){
        delete m_tcpserver;
        m_tcpserver = NULL;
        m_connectedTcpSkts.clear();
    }else if(m_tcpSkt != NULL){
        delete m_tcpSkt;
        m_tcpSkt = NULL;
    }else
        return;
    emit connectedStatus(false);
    m_buttons[PB_OPEN]->setText(tr("Open"));
    QPalette pal = m_buttons[PB_OPEN]->palette();
    pal.setColor(QPalette::Button, Qt::red);
    m_buttons[PB_OPEN]->setPalette(pal);
}

void YNetDock::updateSObject()
{
    SObject* pObj = sobject();
    pObj->setProperty("net_type", m_combos[CB_TYPE]->currentText());
    pObj->setProperty("udp_lport", m_lineEdits[LE_UDP_L_PORT]->text());
    pObj->setProperty("udp_tip", m_lineEdits[LE_UDP_T_IP]->text());
    pObj->setProperty("udp_tport", m_lineEdits[LE_UDP_T_PORT]->text());
    pObj->setProperty("tcpc_server_ip", m_lineEdits[LE_TC_IP]->text());
    pObj->setProperty("tcpc_server_port", m_lineEdits[LE_TC_PORT]->text());
    pObj->setProperty("tcps_port", m_lineEdits[LE_TS_PORT]->text());
    YCommDock::updateSObject();
}

void YNetDock::initSObject(SObject *pObj)
{
    YCommDock::initSObject(pObj);
    pObj->setProperty("type", "network");
    pObj->setProperty("net_type", "UDP");
    pObj->setProperty("udp_lport", "1000");
    pObj->setProperty("udp_tip", "127.0.0.1");
    pObj->setProperty("udp_tport", "1001");
    pObj->setProperty("tcpc_server_ip", "127.0.0.1");
    pObj->setProperty("tcpc_server_port", "1000");
    pObj->setProperty("tcps_port", "1000");
}

void YNetDock::updateStatus()
{
    QString strMsg;

    if(commIsOpen()){
        strMsg = tr("Network: Connected by %1 mode.")
                .arg(m_combos[CB_TYPE]->currentText());
    }else{
        strMsg = tr("Network: Disconnected.");
    }
    updateStatusBar(strMsg);
}

void YNetDock::clickedOpenBt()
{
    if(commIsOpen())
        commClose();
    else
        commOpen();
    updateStatus();
}

void YNetDock::readUdpDatagrams()
{
    QByteArray datagram;
    QHostAddress sender;
    quint16 senderPort;
    YMainWindow* pMW = mainWindow();
    while (m_udpSkt->hasPendingDatagrams()) {
        datagram.resize(m_udpSkt->pendingDatagramSize());
        m_udpSkt->readDatagram(datagram.data(), datagram.size(),
                                &sender, &senderPort);
        pMW->appendLog(tr("UDP recv %1 bytes data from %2:%3")
                       .arg(datagram.size())
                       .arg(sender.toString())
                       .arg(senderPort));
        emit receivedData((uchar*)datagram.data(), datagram.size());
    }
}

void YNetDock::newConnectionOnTcpServer()
{
    QTcpSocket* pTcpSkt;
    while(m_tcpserver->hasPendingConnections()){
        pTcpSkt = m_tcpserver->nextPendingConnection();
        m_connectedTcpSkts.append(pTcpSkt);
        connect(pTcpSkt, SIGNAL(readyRead()), SLOT(readTcpSktData()));
        connect(pTcpSkt, SIGNAL(disconnected()), SLOT(tcpSktDisconnected()));
        m_combos[CB_TS_TO]->addItem(tr("%1:%2")
                                    .arg(pTcpSkt->peerAddress().toString())
                                    .arg(pTcpSkt->peerPort()),
                                    QVariant::fromValue((void*)pTcpSkt));
    }
}

void YNetDock::readTcpSktData()
{
    QTcpSocket* pTcpSkt = qobject_cast<QTcpSocket*>(sender());
    QByteArray baIn;
    QDataStream dsIn(pTcpSkt);
    int nRet, nSum = 0;

    dsIn.setVersion(QDataStream::Qt_5_2);
    while(pTcpSkt->bytesAvailable() > 0){
        baIn.resize(pTcpSkt->bytesAvailable());
        nRet = dsIn.readRawData(baIn.data(), pTcpSkt->bytesAvailable());
        if(nRet > 0){
            nSum += nRet;
            emit receivedData((uchar*)baIn.data(), (uint)nRet);
        }
    }
    if(nSum > 0){
        YMainWindow* pMW = mainWindow();
        pMW->appendLog(tr("TCP recived %1 bytes data from %2:%3")
                       .arg(nSum)
                       .arg(pTcpSkt->peerAddress().toString())
                       .arg(pTcpSkt->peerPort()));
    }
}

void YNetDock::tcpSktDisconnected()
{
    QTcpSocket* pTcpSkt = qobject_cast<QTcpSocket*>(sender());
    m_combos[CB_TS_TO]->removeItem(
                m_combos[CB_TS_TO]->findText(tr("%1:%2")
                                .arg(pTcpSkt->peerAddress().toString())
                                .arg(pTcpSkt->peerPort())));
    m_connectedTcpSkts.removeOne(pTcpSkt);
    delete pTcpSkt;
}
