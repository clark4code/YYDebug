#ifndef YCOMMDOCK_H
#define YCOMMDOCK_H

#include <QDockWidget>
#include <QMutex>

class SObject;
class YMainWindow;

class YCommDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit YCommDock(SObject *pObj, YMainWindow *parent = 0);
    virtual ~YCommDock();

    virtual bool commOpen() = 0;
    virtual int commWrite(const void* pData, uint uSize, uint uTimeOut = uint(~0)) = 0;
    virtual bool commIsOpen() = 0;
    virtual void commClose() = 0;
    virtual void updateSObject();

    YMainWindow* mainWindow();
    SObject *sobject() const;
    QMutex& mutex();

    static void initSObject(SObject* pObj);
    static void vl2ba(const QVariantList &vl, QByteArray &ba);
protected:
    void updateStatusBar(const QString& strMsg);
signals:
    void receivedData(const uchar* pData, uint uSize);
    void connectedStatus(bool bOpen);
public slots:
    int write(const QVariantList& data, unsigned int nTimeOutMs = ~0);
private:
    SObject* m_sobject;
    QMutex m_mutex;
    YMainWindow* m_mainWindow;
};

#endif // YCOMMDOCK_H
