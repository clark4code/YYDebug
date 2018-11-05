#ifndef SOBJECT_H
#define SOBJECT_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QDomElement>
#include <QReadWriteLock>

class SObject : public QObject
{
    Q_OBJECT
public:
    struct PropertyInfo{
        uint version = 0;
        QMap<QString, QString> attribute;
        QObject* changedBy = NULL;
    };

    struct RootParamT{
        int timerIDForCheck = 0;
        QMap<SObject*, QMap<QString, QObject*> > changed;
        QReadWriteLock lock;
    };

    explicit SObject(SObject *parent = 0);
    ~SObject();

    virtual bool setProperty(const char *name, const QVariant &value);
    virtual QVariant propertyEx(const QString& strPath);
    virtual bool setPropertyEx(const QString& strPath, const QVariant& varValue);
    virtual SObject* find(const QString& strPath);
    virtual QDomElement toXML(const QString &strRootName = "object");
    virtual void fromXML(const QDomElement& eRoot);
    virtual SObject* clone() const;

    static void toXML(QDomElement& eParent, const SObject& obj);
    static void fromXML(SObject& obj, const QDomElement& eParent);
    static QVariant stringToVariant(const QString& strTypeName, const QString& strValue);
    static QString variantToString(const QVariant& varValue, QString& strTypeName);
    static void clone(SObject& des, const SObject& src);

    QMap<QString, PropertyInfo>& propertyInfo();
    void setPropertyInfo(const QMap<QString, PropertyInfo> &propertyInfo);

    QReadWriteLock &lock();
    void changeProperty(QObject* pFrom, const QString& strPropName);

    static QStringList supportedTypes();
protected:
    void timerEvent(QTimerEvent *evt);
    SObject* root();

signals:
    void propertyChanged(SObject* pObj, const QString& strPropName, const QVariant& varValue, QObject* pChangedBy);
public slots:
    void setObjectName(const QString & name);
    virtual bool saveFile(const QString & strFileName);
    virtual bool openFile(const QString & strFileName);
    virtual void clear();
    void setAsRoot(bool bRoot);
    void updatedProperty(const QString& strPropName);
private:
    QMap<QString, PropertyInfo> m_propertyInfo;
    QReadWriteLock m_lock;
    RootParamT* m_rootParam = NULL;
};

#endif // SOBJECT_H
