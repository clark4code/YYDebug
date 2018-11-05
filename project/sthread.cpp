#include "sthread.h"


SThread::SThread(QObject *parent) :
    QThread(parent)
{
    m_userFunction = NULL;
    m_userParam = NULL;
}

SThreadFunc SThread::userFunction() const
{
    return m_userFunction;
}

void SThread::setUserFunction(const SThreadFunc &fnUserThread)
{
    m_userFunction = fnUserThread;
}

void *SThread::userParam() const
{
    return m_userParam;
}

void SThread::setUserParam(void *pUserParam)
{
    m_userParam = pUserParam;
}

void SThread::stop()
{
    m_running = false;
}

void SThread::run()
{
    if(m_userFunction != NULL)
    {
        m_running = true;
        m_userFunction(m_userParam, m_running);
        m_running = false;
    }
}

