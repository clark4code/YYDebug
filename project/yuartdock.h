#ifndef YUARTDOCK_H
#define YUARTDOCK_H

#include "ycommdock.h"
#include <QComboBox>
#include <QPushButton>
#include <QSerialPort>

class YUartDock : public YCommDock
{
    Q_OBJECT
public:
    enum ComboBoxT{CB_UART, CB_BAUDRATE, CB_DATA_BITS, CB_STOP_BITS, CB_PARITY, CB_LENGHT};
    enum PushButtonT{PB_OPEN, PB_LENGTH};

    explicit YUartDock(SObject *pObj, YMainWindow *parent = 0);
    bool commOpen() override;
    int commWrite(const void* pData, uint uSize, uint uTimeOut = uint(~0)) override;
    bool commIsOpen() override;
    void commClose() override;
    void updateSObject() override;
    void updateUartStatus();

    static void initSObject(SObject *pObj);
protected:
    void timerEvent(QTimerEvent *evt);
public slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError err);
    void clickedOpenBt();
    void resetParameters(int index);
private:
    QSerialPort m_serialPort;
    QComboBox* m_combos[CB_LENGHT];
    QPushButton* m_buttons[PB_LENGTH];
};

#endif // YUARTDOCK_H
