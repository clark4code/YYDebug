#include "yuartdock.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QSerialPortInfo>
#include <QVBoxLayout>
#include <QDebug>
#include <sobject.h>
#include <ymainwindow.h>
#define addLog(msg) qDebug()<<(msg)

YUartDock::YUartDock(SObject *pObj, YMainWindow *parent) :
    YCommDock(pObj, parent)
{
    QString strPortName;
    QScrollArea* pScrollArea = new QScrollArea(this);
    QGroupBox* pGroups[2];
    QFormLayout* pFLayout[2];
    QVBoxLayout* pVLayout[1];
    QWidget* pScrollWidget = new QWidget(this);

    pGroups[0] = new QGroupBox(tr("Port"), this);
    m_combos[CB_UART] = new QComboBox(this);
    m_buttons[PB_OPEN] = new QPushButton(tr("Open"), this);
    m_buttons[PB_OPEN]->setAutoFillBackground(true);
    pFLayout[0] = new QFormLayout(pGroups[0]);
    pFLayout[0]->addRow(m_combos[CB_UART], m_buttons[PB_OPEN]);
    pGroups[0]->setLayout(pFLayout[0]);

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        m_combos[CB_UART]->addItem(info.portName());
    }
    strPortName = pObj->property("port_name").toString();
    if(!strPortName.isEmpty()
            && m_combos[CB_UART]->findText(strPortName) >= 0)
        m_combos[CB_UART]->setCurrentText(strPortName);
    if(m_combos[CB_UART]->count() == 0)
        m_buttons[PB_OPEN]->setEnabled(false);

    QPalette pal = m_buttons[PB_OPEN]->palette();
    pal.setColor(QPalette::Button, Qt::red);
    m_buttons[PB_OPEN]->setPalette(pal);

    pGroups[1] = new QGroupBox(tr("Parameters"), this);
    m_combos[CB_BAUDRATE] = new QComboBox(this);
    foreach (qint32 nRate, QSerialPortInfo::standardBaudRates()) {
        m_combos[CB_BAUDRATE]->addItem(QString::number(nRate), nRate);
    }

    m_combos[CB_BAUDRATE]->setCurrentText(pObj->property("baud_rate").toString());
    m_combos[CB_DATA_BITS] = new QComboBox(this);
    m_combos[CB_DATA_BITS]->addItem("5", 5);
    m_combos[CB_DATA_BITS]->addItem("6", 6);
    m_combos[CB_DATA_BITS]->addItem("7", 7);
    m_combos[CB_DATA_BITS]->addItem("8", 8);
    m_combos[CB_DATA_BITS]->setCurrentText(pObj->property("data_bits").toString());
    m_combos[CB_STOP_BITS] = new QComboBox(this);
    m_combos[CB_STOP_BITS]->addItem("1", 1);
    m_combos[CB_STOP_BITS]->addItem("1.5", 3);
    m_combos[CB_STOP_BITS]->addItem("2", 2);
    m_combos[CB_STOP_BITS]->setCurrentText(pObj->property("stop_bits").toString());
    m_combos[CB_PARITY] = new QComboBox(this);
    m_combos[CB_PARITY]->addItem("No", 0);
    m_combos[CB_PARITY]->addItem("Even", 2);
    m_combos[CB_PARITY]->addItem("Odd", 3);
    m_combos[CB_PARITY]->addItem("Space", 4);
    m_combos[CB_PARITY]->addItem("Mark", 5);
    m_combos[CB_PARITY]->setCurrentText(pObj->property("parity").toString());
    pFLayout[1] = new QFormLayout(pGroups[1]);
    pFLayout[1]->addRow(new QLabel(tr("Baud Rate"), this), m_combos[CB_BAUDRATE]);
    pFLayout[1]->addRow(new QLabel(tr("Data Bits"), this), m_combos[CB_DATA_BITS]);
    pFLayout[1]->addRow(new QLabel(tr("Stop Bits"), this), m_combos[CB_STOP_BITS]);
    pFLayout[1]->addRow(new QLabel(tr("Parity"), this), m_combos[CB_PARITY]);
    pGroups[1]->setLayout(pFLayout[1]);

    pVLayout[0] = new QVBoxLayout(pScrollWidget);
    pVLayout[0]->addWidget(pGroups[0]);
    pVLayout[0]->addWidget(pGroups[1]);
    pScrollWidget->setLayout(pVLayout[0]);
    pScrollArea->setWidget(pScrollWidget);
    setWidget(pScrollArea);
    startTimer(1000);

    setWindowTitle(tr("Uart"));

    connect(m_buttons[PB_OPEN], SIGNAL(clicked()), SLOT(clickedOpenBt()));
    connect(m_combos[CB_BAUDRATE], SIGNAL(currentIndexChanged(int)), SLOT(resetParameters(int)));
    connect(m_combos[CB_DATA_BITS], SIGNAL(currentIndexChanged(int)), SLOT(resetParameters(int)));
    connect(m_combos[CB_STOP_BITS], SIGNAL(currentIndexChanged(int)), SLOT(resetParameters(int)));
    connect(m_combos[CB_PARITY], SIGNAL(currentIndexChanged(int)), SLOT(resetParameters(int)));
    updateUartStatus();
}

bool YUartDock::commOpen()
{
    if(m_serialPort.isOpen()){
        return true;
    }
    QMutexLocker locker(&mutex());
    QString serialPortName = m_combos[CB_UART]->currentText();
    m_serialPort.setPortName(serialPortName);
    if (!m_serialPort.open(QIODevice::ReadWrite)) {
        addLog(QObject::tr("Failed to open port %1, error: %2").arg(serialPortName).arg(m_serialPort.error()));
        return false;
    }
    int nBaudRate = m_combos[CB_BAUDRATE]->currentData().toInt();
    if (!m_serialPort.setBaudRate(nBaudRate)) {
        addLog(QObject::tr("Failed to set %1 baud for port %2, error: %3").arg(nBaudRate).arg(serialPortName).arg(m_serialPort.errorString()));
    }

    if (!m_serialPort.setDataBits((QSerialPort::DataBits)m_combos[CB_DATA_BITS]->currentData().toInt())) {
        addLog(QObject::tr("Failed set 8 data bits for port %1, error: %2").arg(serialPortName).arg(m_serialPort.errorString()));
    }

    if (!m_serialPort.setParity((QSerialPort::Parity)m_combos[CB_PARITY]->currentData().toInt())) {
        addLog(QObject::tr("Failed to set no parity for port %1, error: %2").arg(serialPortName).arg(m_serialPort.errorString()));
    }

    if (!m_serialPort.setStopBits((QSerialPort::StopBits)m_combos[CB_STOP_BITS]->currentData().toInt())) {
        addLog(QObject::tr("Failed to set 1 stop bit for port %1, error: %2").arg(serialPortName).arg(m_serialPort.errorString()));
    }
    connect(&m_serialPort, SIGNAL(readyRead()), SLOT(handleReadyRead()));
    connect(&m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), SLOT(handleError(QSerialPort::SerialPortError)));
    emit connectedStatus(true);
    m_buttons[PB_OPEN]->setText(tr("Close"));
    QPalette pal = m_buttons[PB_OPEN]->palette();
    pal.setColor(QPalette::Button, Qt::green);
    m_buttons[PB_OPEN]->setPalette(pal);
    return true;
}

int YUartDock::commWrite(const void *pData, uint uSize, uint uTimeOut)
{
    if(!m_serialPort.isOpen())
        return -1;

    qint64 nWritten = m_serialPort.write((char*)pData, (qint64)uSize);

    int nTimeOut;
    if(uTimeOut == ~(uint(0)))
        nTimeOut = -1;
    else
        nTimeOut = (int)uTimeOut;
    if(nWritten == -1
            || nWritten != (qint64)uSize
            || !m_serialPort.waitForBytesWritten(nTimeOut))
        return false;
    mainWindow()->addTxCounter(nWritten);
    return (int)nWritten;
}

bool YUartDock::commIsOpen()
{
    return m_serialPort.isOpen();
}

void YUartDock::commClose()
{
    if(m_serialPort.isOpen()){
        emit connectedStatus(false);
        m_serialPort.close();
        m_buttons[PB_OPEN]->setText(tr("Open"));
        QPalette pal = m_buttons[PB_OPEN]->palette();
        pal.setColor(QPalette::Button, Qt::red);
        m_buttons[PB_OPEN]->setPalette(pal);
    }
}

void YUartDock::updateSObject()
{
    SObject* pObj = sobject();
    pObj->setProperty("port_name", m_combos[CB_UART]->currentText());
    pObj->setProperty("baud_rate", m_combos[CB_BAUDRATE]->currentText());
    pObj->setProperty("data_bits", m_combos[CB_DATA_BITS]->currentText());
    pObj->setProperty("stop_bits", m_combos[CB_STOP_BITS]->currentText());
    pObj->setProperty("parity", m_combos[CB_PARITY]->currentText());
    YCommDock::updateSObject();
}

void YUartDock::updateUartStatus()
{
    QString strMsg;

    if(m_serialPort.isOpen()){
        strMsg = tr("UART: %1 has been opened. %2bps %3 %4 %5")
                .arg(m_serialPort.portName())
                .arg(m_serialPort.baudRate())
                .arg(m_serialPort.dataBits())
                .arg(m_combos[CB_STOP_BITS]->currentText())
                .arg(m_combos[CB_PARITY]->currentText());
    }else{
        strMsg = tr("UART: No port has been connected.");
    }
    updateStatusBar(strMsg);
}

void YUartDock::initSObject(SObject *pObj)
{
    YCommDock::initSObject(pObj);
    pObj->setProperty("type", "uart");
    pObj->setProperty("port_name", "");
    pObj->setProperty("baud_rate", "115200");
    pObj->setProperty("data_bits", "8");
    pObj->setProperty("stop_bits", "1");
    pObj->setProperty("parity", "No");
}

void YUartDock::timerEvent(QTimerEvent *evt)
{
    Q_UNUSED(evt)

    QStringList lstUart;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        lstUart << info.portName();
    }

    for(int nIndex = 0; nIndex < m_combos[CB_UART]->count();){
        if(lstUart.contains(m_combos[CB_UART]->itemText(nIndex))){
            nIndex++;
            continue;
        }
        m_combos[CB_UART]->removeItem(nIndex);
    }
    foreach (QString strName, lstUart) {
        if(m_combos[CB_UART]->findText(strName) >= 0)
            continue;
        m_combos[CB_UART]->addItem(strName);
    }
}

void YUartDock::handleReadyRead()
{
    QByteArray baData = m_serialPort.readAll();
    emit receivedData((uchar*)baData.data(), baData.size());
}

void YUartDock::handleError(QSerialPort::SerialPortError err)
{
    const char* szErr[] = {
            "NoError",
            "DeviceNotFoundError",
            "PermissionError",
            "OpenError",
            "ParityError",
            "FramingError",
            "BreakConditionError",
            "WriteError",
            "ReadError",
            "ResourceError",
            "UnsupportedOperationError",
            "UnknownError",
            "TimeoutError",
            "NotOpenError"
        };
    if(err != 0)
        addLog(tr("UART error: %1").arg(szErr[err]));
}

void YUartDock::clickedOpenBt()
{
    if(m_serialPort.isOpen())
        commClose();
    else
        commOpen();
    updateUartStatus();
}

void YUartDock::resetParameters(int)
{
    if(!m_serialPort.isOpen())
        return;
    m_serialPort.setBaudRate(m_combos[CB_BAUDRATE]->currentData().toInt());
    m_serialPort.setDataBits((QSerialPort::DataBits)m_combos[CB_DATA_BITS]->currentData().toInt());
    m_serialPort.setParity((QSerialPort::Parity)m_combos[CB_PARITY]->currentData().toInt());
    m_serialPort.setStopBits((QSerialPort::StopBits)m_combos[CB_STOP_BITS]->currentData().toInt());
    updateUartStatus();
}
