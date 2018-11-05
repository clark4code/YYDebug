#ifndef STHREAD_H
#define STHREAD_H
#include <QThread>
typedef int (*SThreadFunc)(void* pParam, const bool& bRunning);

class SThread : public QThread
{
    Q_OBJECT
public:
    explicit SThread(QObject *parent = 0);

    SThreadFunc userFunction() const;
    void setUserFunction(const SThreadFunc &fnUserFunction);

    void *userParam() const;
    void setUserParam(void *pUserParam);
    void stop();

protected:
    virtual void run();

    SThreadFunc m_userFunction;
    void* m_userParam;
    bool m_running = false;
};

#endif // STHREAD_H
