#include "yusbdock.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <sobject.h>
#include <QDebug>
#include "ymainwindow.h"

YUSBDock::YUSBDock(SObject *pObj, YMainWindow *parent) :
    YCommDock(pObj, parent)
{
    QScrollArea* pScrollArea = new QScrollArea(this);
    QGroupBox* pGroups[3];
    QVBoxLayout* pVLayout[2];
    QFormLayout* pFLayout[2];
    QWidget* pScrollWidget = new QWidget(this);
    QString strFmt;

    if(libusb_init(&m_context) != 0){
        qDebug() << "It's failed to initialize libusb.";
    }
    pVLayout[0] = new QVBoxLayout(pScrollWidget);
    pGroups[0] = new QGroupBox(tr("Devices"), this);
    m_combos[CB_DEVICES] = new QComboBox(this);
    connect(m_combos[CB_DEVICES], SIGNAL(currentIndexChanged(int)),
            SLOT(currentIndexChangedByDevs(int)));
    m_buttons[PB_OPEN] = new QPushButton(tr("Open"), this);
    m_buttons[PB_OPEN]->setAutoFillBackground(true);
    QPalette pal = m_buttons[PB_OPEN]->palette();
    pal.setColor(QPalette::Button, Qt::red);
    m_buttons[PB_OPEN]->setPalette(pal);

    pGroups[1] = new QGroupBox(tr("Parameter"), this);
    m_combos[CB_IN_EP] = new QComboBox(this);
    m_combos[CB_OUT_EP] = new QComboBox(this);

    //m_labels[L_DEV_INFO] = new QLabel(tr("Dev info"),this);
    m_treeWidget[T_DEV_INFO] = new QTreeWidget(this);
    m_treeWidget[T_DEV_INFO]->setColumnCount(2);
    m_treeWidget[T_DEV_INFO]->setHeaderLabels(QStringList() << tr("Property") << tr("Value"));
    m_treeWidget[T_DEV_INFO]->setColumnWidth(0, 180);

    struct libusb_device **devs;
    struct libusb_device *dev;
    size_t nIndex = 0;
    int nRet;

    if (libusb_get_device_list(m_context, &devs) < 0)
        return;

    while ((dev = devs[nIndex++]) != NULL) {
        struct libusb_device_descriptor desc;
        nRet = libusb_get_device_descriptor(dev, &desc);
        if (nRet < 0)
            continue;
        strFmt.sprintf("%04X:%04X", desc.idVendor, desc.idProduct);
        m_combos[CB_DEVICES]->addItem(strFmt, QVariant::fromValue((void*)dev));
    }

    pFLayout[0] = new QFormLayout(pGroups[0]);
    pFLayout[0]->addRow(m_combos[CB_DEVICES], m_buttons[PB_OPEN]);

    pFLayout[1] = new QFormLayout(pGroups[1]);
    pFLayout[1]->addRow(new QLabel(tr("Input EndPoint"), this), m_combos[CB_IN_EP]);
    pFLayout[1]->addRow(new QLabel(tr("Output EndPoint"), this), m_combos[CB_OUT_EP]);

    pVLayout[0]->addWidget(pGroups[0]);
    pVLayout[0]->addWidget(pGroups[1]);
    //pVLayout[0]->addWidget(m_labels[L_DEV_INFO]);
    pVLayout[0]->addWidget(new QLabel(tr("Device info")));
    pVLayout[0]->addWidget(m_treeWidget[T_DEV_INFO]);
    pScrollArea->setWidget(pScrollWidget);
    setWidget(pScrollArea);
    setWindowTitle(tr("USB"));
    connect(m_buttons[PB_OPEN], SIGNAL(clicked()), SLOT(clickedOpenBt()));
    updateUsbStatus();

    m_rxThread.setUserFunction(rxThread);
    m_rxThread.setUserParam(this);
}

YUSBDock::~YUSBDock()
{
    if(m_device)
        libusb_close(m_device);
    if(m_context)
        libusb_exit(m_context);
}

bool YUSBDock::commOpen()
{
    int nRet;
    libusb_device_handle *handle = NULL;
    libusb_device *dev = (libusb_device*)m_combos[CB_DEVICES]->currentData().value<void*>();
    if ((nRet = libusb_open(dev, &handle)) < 0){
        reportError(nRet);
        return false;
    }

    if(/*((nRet = libusb_detach_kernel_driver(handle, 0)) != 0
        && nRet != LIBUSB_ERROR_NOT_SUPPORTED)
            ||*/ (nRet = libusb_claim_interface(handle, 0)) < 0){
        libusb_close(handle);
        reportError(nRet);
        return false;
    }
    m_device = handle;
    emit connectedStatus(true);
    m_buttons[PB_OPEN]->setText(tr("Close"));
    QPalette pal = m_buttons[PB_OPEN]->palette();
    pal.setColor(QPalette::Button, Qt::green);
    m_buttons[PB_OPEN]->setPalette(pal);
    m_rxThread.start();
    return true;
}

int YUSBDock::commWrite(const void *pData, uint uSize, uint uTimeOut)
{
    if(m_combos[CB_OUT_EP]->count() < 1)
        return -1;
    int uActualLen = 0;
    QStringList lstEpInfo = m_combos[CB_OUT_EP]->currentText().split('.');
    if(lstEpInfo.size() < 3)
        return -2;
    uchar uAddr;
    //uint uMaxPktSize;
    uAddr = (uchar)lstEpInfo[0].toUInt();
    //uMaxPktSize = lstEpInfo[2].toUInt();
    if(lstEpInfo[1] == "bulk"){
        if(libusb_bulk_transfer(m_device, uAddr, (uchar*)pData, (int)uSize, &uActualLen, uTimeOut) != 0)
            return -2;
    }else if(lstEpInfo[1] == "interrupt"){
        if(libusb_interrupt_transfer(m_device, uAddr, (uchar*)pData, (int)uSize, &uActualLen, uTimeOut) != 0)
            return -2;
    }else if(lstEpInfo[1] == "control"){
        uchar* cData = (uchar*)pData;
        if(uSize < 6)
            return -2;
        if(libusb_control_transfer(m_device, cData[0], cData[1], *((quint16*)(cData + 2)),
                                   *((quint16*)(cData + 4)), cData + 6, quint16(uSize - 6), uTimeOut) != 0)
            return -2;
        uActualLen = uSize;
    }
    mainWindow()->addTxCounter(uActualLen);
    return uActualLen;
}

bool YUSBDock::commIsOpen()
{
    return m_device != NULL ? true : false;
}

void YUSBDock::commClose()
{
    if(!commIsOpen())
        return;
    m_rxThread.stop();
    m_rxThread.wait();
    emit connectedStatus(false);
    libusb_release_interface(m_device, 0);
    libusb_attach_kernel_driver(m_device, 0);
    libusb_close(m_device);
    m_device = NULL;
    m_buttons[PB_OPEN]->setText(tr("Open"));
    QPalette pal = m_buttons[PB_OPEN]->palette();
    pal.setColor(QPalette::Button, Qt::red);
    m_buttons[PB_OPEN]->setPalette(pal);
}

void YUSBDock::updateSObject()
{
    YCommDock::updateSObject();
    SObject* pObj = sobject();
    pObj->setProperty("id", m_combos[CB_DEVICES]->currentText());
    pObj->setProperty("out_ep", m_combos[CB_OUT_EP]->currentText());
    pObj->setProperty("in_ep", m_combos[CB_IN_EP]->currentText());
}

void YUSBDock::updateUsbStatus()
{
    QString strMsg;

    if(commIsOpen()){
        strMsg = tr("USB: %1 has been opened.")
                .arg(m_combos[CB_DEVICES]->currentText());
    }else{
        strMsg = tr("USB: No port has been connected.");
    }
    updateStatusBar(strMsg);
}

void YUSBDock::reportError(int nError)
{
    QString strErr = "USB Error:";
    switch (nError) {
    case LIBUSB_ERROR_NO_MEM:
        strErr += "on memory allocation failure";
        break;
    case LIBUSB_ERROR_ACCESS:
        strErr += "the user has insufficient permissions";
        break;
    case LIBUSB_ERROR_NO_DEVICE:
        strErr += "the device has been disconnected";
        break;
    case LIBUSB_ERROR_NOT_FOUND:
        strErr += "the requested interface does not exist";
        break;
    case LIBUSB_ERROR_BUSY:
        strErr += "another program or driver has claimed the interface";
        break;
    case LIBUSB_ERROR_INVALID_PARAM:
        strErr += " the interface does not exist";
        break;
    case LIBUSB_ERROR_NOT_SUPPORTED:
        strErr += "on platforms where the functionality is not available";
        break;
    default:
        strErr += "unknown";
        break;
    }
    mainWindow()->appendLog(strErr);
}

void YUSBDock::initSObject(SObject *pObj)
{
    YCommDock::initSObject(pObj);
    pObj->setProperty("type", "usb");
    pObj->setProperty("id", "");
    pObj->setProperty("out_ep", "");
    pObj->setProperty("in_ep", "");
}

int YUSBDock::rxThread(void *pParam, const bool &bRunning)
{
    YUSBDock* pUsbDock = (YUSBDock*)pParam;

    if(pUsbDock->m_combos[CB_IN_EP]->count() < 1)
        return -1;
    uchar uAddr;
    int uActualLen = 0;
    QStringList lstEpInfo;
    QByteArray baBuffer(1024, 0);
    while(bRunning){
        uActualLen = 0;
        lstEpInfo = pUsbDock->m_combos[CB_IN_EP]->currentText().split('.');
        if(lstEpInfo.size() < 3)
            return -2;
        uAddr = (uchar)(0x80 | lstEpInfo[0].toUInt());
        if(lstEpInfo[1] == "bulk"){
            libusb_bulk_transfer(pUsbDock->m_device, uAddr, (uchar*)baBuffer.data(), 1024, &uActualLen, 10);
        }else if(lstEpInfo[1] == "interrupt"){
            libusb_interrupt_transfer(pUsbDock->m_device, uAddr, (uchar*)baBuffer.data(), 1024, &uActualLen, 10);
        }else if(lstEpInfo[1] == "control"){
        }
        if(uActualLen > 0)
            emit pUsbDock->receivedData((uchar*)baBuffer.data(), uActualLen);
    }
    return 0;
}

void YUSBDock::currentIndexChangedByDevs(int nIndex)
{
    if(nIndex < 0)
        return;
    libusb_device_descriptor devDesc;
    libusb_config_descriptor* cfgDesc;
    const libusb_interface_descriptor* infDesc;
    const libusb_endpoint_descriptor* epDesc;
    libusb_device *dev = (libusb_device*)m_combos[CB_DEVICES]->itemData(nIndex).value<void*>();
    // QString strInfo;
    int nComboForEP,nRet;
    uint8_t uIndex,uIndex2,uIndex3;
    QTreeWidgetItem* pItems[5];
    const char* arrSpeed[] ={"unknown", "low", "full", "high", "super"};
    const char* arrEpTrType[] ={"control", "syn", "bulk", "interrupt"};

    m_treeWidget[T_DEV_INFO]->clear();
    m_combos[CB_IN_EP]->clear();
    m_combos[CB_OUT_EP]->clear();
    libusb_get_device_descriptor(dev,&devDesc);
    pItems[0] = new QTreeWidgetItem(m_treeWidget[T_DEV_INFO], QStringList() << m_combos[CB_DEVICES]->currentText());
    pItems[1] = new QTreeWidgetItem(pItems[0],
            QStringList() << "Bus" << QString::number(libusb_get_bus_number (dev)));
    pItems[1] = new QTreeWidgetItem(pItems[0],
            QStringList() << "Address" << QString::number(libusb_get_device_address(dev)));
    pItems[1] = new QTreeWidgetItem(pItems[0],
            QStringList() << "Speed" << arrSpeed[libusb_get_device_speed(dev)]);
    pItems[1] = new QTreeWidgetItem(pItems[0],
            QStringList() << "Hotplug" << (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) ? "valid" : "invalid"));

//    strInfo = tr("Bus=%1,Address=%2,Speed=%3\nhotplug=%4,configuration=%5")
//            .arg(libusb_get_bus_number (dev))
//            .arg(libusb_get_device_address(dev))
//            .arg(arrSpeed[libusb_get_device_speed(dev)])
//            .arg(libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) ? "valid" : "invalid")
//            .arg(devDesc.bNumConfigurations);
    for(uIndex = 0; uIndex < devDesc.bNumConfigurations; uIndex++){
        if((nRet = libusb_get_config_descriptor(dev, uIndex, &cfgDesc)) != 0){
            reportError(nRet);
            continue;
        }
        //strInfo += tr("\ncfg.%1,interface=%2").arg(uIndex).arg(cfgDesc->bNumInterfaces);
        pItems[1] = new QTreeWidgetItem(pItems[0],
                QStringList() << tr("Configuration.%1").arg(uIndex));
        for(uIndex2 = 0; uIndex2 < cfgDesc->bNumInterfaces; uIndex2++){
            infDesc = cfgDesc->interface[uIndex2].altsetting;// + cfgDesc->interface[uIndex2].num_altsetting;
            //strInfo += tr("\ninf.%1,endpoint=%2").arg(uIndex2).arg(infDesc->bNumEndpoints);
            pItems[2] = new QTreeWidgetItem(pItems[1],
                    QStringList() << tr("Interface.%1").arg(uIndex2));
            for(uIndex3 = 0; uIndex3 < infDesc->bNumEndpoints; uIndex3++){
                epDesc = infDesc->endpoint + uIndex3;
                nComboForEP = (epDesc->bEndpointAddress & 0x80) ? CB_IN_EP : CB_OUT_EP;
                m_combos[nComboForEP]->addItem(
                            tr("%1.%2.%3")
                            .arg(epDesc->bEndpointAddress & 0x7f)
                            .arg(arrEpTrType[epDesc->bmAttributes & 0x03])
                            .arg(QString::number(epDesc->wMaxPacketSize)),
                            epDesc->bEndpointAddress);
                pItems[3] = new QTreeWidgetItem(pItems[2],
                        QStringList() << tr("EndPoint.%1").arg(uIndex3));
                pItems[4] = new QTreeWidgetItem(pItems[3],
                        QStringList() << "Direction" << (epDesc->bEndpointAddress & 0x80 ? "in" : "out"));
                pItems[4] = new QTreeWidgetItem(pItems[3],
                        QStringList() << "Address" << QString::number(epDesc->bEndpointAddress & 0x7f));
                pItems[4] = new QTreeWidgetItem(pItems[3],
                        QStringList() << "Type" << arrEpTrType[epDesc->bmAttributes & 0x03]);
                pItems[4] = new QTreeWidgetItem(pItems[3],
                        QStringList() << "MaxPacket" << QString::number(epDesc->wMaxPacketSize));
//                strInfo += tr("\nep.%1,%2,address=%3,%4,max_pkt=%5")
//                        .arg(uIndex3)
//                        .arg(epDesc->bEndpointAddress & 0x80 ? "IN" : "OUT")
//                        .arg(epDesc->bEndpointAddress & 0x7f)
//                        .arg(arrEpTrType[epDesc->bmAttributes & 0x03])
//                        .arg(epDesc->wMaxPacketSize);
            }
        }
    }
    m_treeWidget[T_DEV_INFO]->expandAll();
    //m_labels[L_DEV_INFO]->setText(strInfo);
}

void YUSBDock::clickedOpenBt()
{
    if(commIsOpen())
        commClose();
    else
        commOpen();
    updateUsbStatus();
}
