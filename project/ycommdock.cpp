#include "ycommdock.h"

#include <sobject.h>
#include <ymainwindow.h>

YCommDock::YCommDock(SObject *pObj, YMainWindow *parent) :
    QDockWidget(parent)
  , m_sobject(pObj)
  , m_mainWindow(parent)
{
    setVisible(m_sobject->property("visible").toBool());
}

YCommDock::~YCommDock()
{

}

void YCommDock::updateSObject()
{
    m_sobject->setProperty("visible", isVisible());
}

YMainWindow *YCommDock::mainWindow()
{
    return m_mainWindow;
}

SObject *YCommDock::sobject() const
{
    return m_sobject;
}

QMutex &YCommDock::mutex()
{
    return m_mutex;
}

void YCommDock::initSObject(SObject *pObj)
{
    pObj->setProperty("visible", true);
}

void YCommDock::updateStatusBar(const QString &strMsg)
{
    m_mainWindow->label(YMainWindow::L_COMM_STATUS)->setText(strMsg);
}

int YCommDock::write(const QVariantList &data, unsigned int nTimeOutMs)
{
    QByteArray ba;
    vl2ba(data,ba);
    return commWrite(ba.data(), (unsigned int)ba.size(), nTimeOutMs);
}


void YCommDock::vl2ba(const QVariantList &vl, QByteArray &ba)
{
    ba.clear();
    for(auto iterItem = vl.begin(); iterItem != vl.end(); iterItem++){
        ba.append((char)iterItem->toInt());
    }
}
