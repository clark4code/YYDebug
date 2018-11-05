#ifndef YUSBDOCK_H
#define YUSBDOCK_H

#include "sthread.h"
#include "ycommdock.h"
#include <libusb-1.0/libusb.h>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>

class YUSBDock : public YCommDock
{
    Q_OBJECT
public:
    enum ComboBoxT{CB_DEVICES, CB_IN_EP, CB_OUT_EP, CB_LENGHT};
    enum PushButtonT{PB_OPEN, PB_LENGTH};
    enum LabelT{L_DEV_INFO, L_LENGTH};
    enum TreeWidgetT{T_DEV_INFO, T_LENGTH};

    explicit YUSBDock(SObject *pObj, YMainWindow *parent = 0);
    ~YUSBDock();
    bool commOpen() override;
    int commWrite(const void* pData, uint uSize, uint uTimeOut = uint(~0)) override;
    bool commIsOpen() override;
    void commClose() override;
    void updateSObject() override;
    void updateUsbStatus();
    void reportError(int nError);
    static void initSObject(SObject *pObj);
    static int rxThread(void* pParam, const bool& bRunning);
public slots:
    void currentIndexChangedByDevs(int nIndex);
    void clickedOpenBt();
private:
    // 设备句柄
    libusb_context* m_context = NULL;
    libusb_device_handle* m_device = NULL;
    QComboBox* m_combos[CB_LENGHT];
    QPushButton* m_buttons[PB_LENGTH];
    //QLabel* m_labels[L_LENGTH];
    QTreeWidget* m_treeWidget[T_LENGTH];
    SThread m_rxThread;
};

#endif // YUSBDOCK_H
