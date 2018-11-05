#include "sobject.h"
#include <QBuffer>
#include <QFile>
#include <QPixmap>
#include <QStringList>
#include <QTextStream>
#include <qcoreevent.h>

SObject::SObject(SObject *parent) :
    QObject(parent)
{
}

SObject::~SObject()
{
    if(m_rootParam){
        killTimer(m_rootParam->timerIDForCheck);
        delete m_rootParam;
    }
}

bool SObject::setProperty(const char *name, const QVariant &value)
{
    if(value.isValid()){
        m_propertyInfo[name].version++;
        SObject* pRoot = root();
        if(pRoot != NULL){
            pRoot->m_rootParam->lock.lockForWrite();
            pRoot->m_rootParam->changed[this][name] = this;
            pRoot->m_rootParam->lock.unlock();
        }
    }else{
        m_propertyInfo.remove(name);
    }
    return QObject::setProperty(name, value);
}

QVariant SObject::propertyEx(const QString &strPath)
{
    QStringList lstPath = strPath.split('@');
    if(lstPath.isEmpty())
        return QVariant();
    QObject* pChild = this;
    int nIndex,nPathLen;
    nPathLen = lstPath.size() - 1;
    for(nIndex = 0; nIndex < nPathLen; nIndex++) {
        pChild = pChild->findChild<QObject*>(lstPath[nIndex]);
        if(pChild == NULL)
            return QVariant();
    }
    return pChild->property(lstPath.last().toUtf8().data());
}

bool SObject::setPropertyEx(const QString &strPath, const QVariant &varValue)
{
    QStringList lstPath = strPath.split('@');
    if(lstPath.isEmpty())
        return false;
    QObject* pChild = this;
    int nIndex,nPathLen;
    nPathLen = lstPath.size() - 1;
    for(nIndex = 0; nIndex < nPathLen; nIndex++) {
        pChild = pChild->findChild<QObject*>(lstPath[nIndex]);
        if(pChild == NULL){
            pChild = new QObject(this);
            pChild->setObjectName(lstPath[nIndex]);
        }
    }
    return pChild->setProperty(lstPath.last().toUtf8().data(), varValue);
}

SObject* SObject::find(const QString &strPath)
{
    QStringList lstPath = strPath.split('@');
    QObject* pChild = this;
    int nIndex,nPathLen;
    nPathLen = lstPath.size();
    for(nIndex = 0; nIndex < nPathLen; nIndex++) {
        pChild = pChild->findChild<QObject*>(lstPath[nIndex]);
        if(pChild == NULL)
            break;
    }
    return (SObject*)pChild;
}

QDomElement SObject::toXML(const QString &strRootName)
{
    QDomDocument doc;
    QDomElement eRoot;

    eRoot = doc.createElement(strRootName);
    eRoot.setAttribute("name", objectName());
    toXML(eRoot, *this);
    return eRoot;
}

void SObject::fromXML(const QDomElement &eRoot)
{
    fromXML(*this, eRoot);
}

void SObject::setObjectName(const QString &name)
{
    if(name.isEmpty()
            || name.indexOf('@') != -1)
        return;
    QObject::setObjectName(name);
}

bool SObject::saveFile(const QString &strFileName)
{
    QFile file(strFileName);
    if(!file.open(QFile::WriteOnly | QFile::Text))
        return false;
    QTextStream outStream(&file);
    QDomElement eRoot = toXML();
    eRoot.save(outStream,1);
    file.close();
    return true;
}

bool SObject::openFile(const QString &strFileName)
{
    QFile file(strFileName);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }
    QDomDocument domDocument;
    QDomElement eRoot;
    QString strErrorMsg;
    int nErrorLine,nErrorColumn;
    if(!domDocument.setContent(&file, &strErrorMsg, &nErrorLine, &nErrorColumn))
    {
        //qDebug()<< __FUNCTION__<<strFileName << strErrorMsg<< nErrorLine<< nErrorColumn;
        file.close();
        return false;
    }
    file.close();
    eRoot = domDocument.documentElement();
    fromXML(*this, eRoot);
    return true;
}

void SObject::clear()
{

}

SObject *SObject::clone() const
{
    SObject* pNew;
    pNew = new SObject();
    clone(*pNew, *this);
    return pNew;
}

void SObject::toXML(QDomElement &eParent, const SObject& obj)
{
    QString strName,strType;
    QVariant varValue;
    QDomElement eParamRoot, eChildRoot,eParam,eChild;
    QDomDocument doc = eParent.toDocument();
    auto lstNames = obj.dynamicPropertyNames();
    eParamRoot = doc.createElement("property");
    for(auto iterParam = lstNames.begin(); iterParam != lstNames.end(); iterParam++){
        strName = iterParam->data();
        varValue = obj.property(strName.toUtf8().data());
        if(!varValue.isValid())
            continue;
        eParam = doc.createElement("item");
        auto iterEnd = obj.m_propertyInfo[strName].attribute.end();
        for(auto iterName = obj.m_propertyInfo[strName].attribute.begin(); iterName != iterEnd; iterName++){
            eParam.setAttribute(iterName.key(), iterName.value());
        }
        eParam.setAttribute("value", variantToString(varValue, strType));
        eParam.setAttribute("name", strName);
        eParam.setAttribute("type", strType);
        eParamRoot.appendChild(eParam);
    }
    eParent.appendChild(eParamRoot);
    eChildRoot = doc.createElement("children");
    QObjectList lstChild = obj.children();
    for(auto iterChild = lstChild.begin(); iterChild != lstChild.end(); iterChild++){
        eChild = doc.createElement("item");
        eChild.setAttribute("name", (*iterChild)->objectName());
        toXML(eChild, *((SObject*)(*iterChild)));
        eChildRoot.appendChild(eChild);
    }
    eParent.appendChild(eChildRoot);
}

void SObject::fromXML(SObject &obj, const QDomElement &eParent)
{
    QDomElement eChild,eProp;
    QString strType,strName,strValue;
    SObject* pNew;
    int nIndex;
    QDomNamedNodeMap attrs;
    QDomAttr attr;

    obj.setObjectName(eParent.attribute("name"));
    eProp = eParent.firstChildElement("property");
    eProp = eProp.firstChildElement();
    while(!eProp.isNull()){
        strName = eProp.attribute("name");
        nIndex = 0;
        attrs = eProp.attributes();
        for(nIndex = 0; nIndex < attrs.count(); nIndex++){
            attr = attrs.item(nIndex).toAttr();
            if(attr.name() == "name"){
                continue;
            }else if(attr.name() == "type"){
                strType = attr.value();
                continue;
            }else if(attr.name() == "value"){
                strValue = attr.value();
                continue;
            }
            obj.m_propertyInfo[strName].attribute[attr.name()] = attr.value();
        }
        obj.setProperty(strName.toUtf8().data(), stringToVariant(strType, strValue));
        eProp = eProp.nextSiblingElement();
    }
    eChild = eParent.firstChildElement("children");
    eChild = eChild.firstChildElement();
    while(!eChild.isNull()){
        pNew = new SObject(&obj);
        fromXML(*pNew, eChild);
        eChild = eChild.nextSiblingElement();
    }
}

QVariant SObject::stringToVariant(const QString &strTypeName, const QString &strValue)
{
    if(strTypeName == "bool"){
        return (strValue == "true") ? true : false;
    }else if(strTypeName == "int"){
        return strValue.toInt();
    }else if(strTypeName == "uint"){
        return strValue.toUInt();
    }else if(strTypeName == "double"){
        return strValue.toDouble();
    }else if(strTypeName == "char"
             || strTypeName == "SChar"){
        return (char)strValue.toInt();
    }else if(strTypeName == "QString"){
        return strValue;
    }else if(strTypeName == "float"){
        return strValue.toFloat();
    }else if(strTypeName == "qlonglong"){
        return strValue.toLongLong();
    }else if(strTypeName == "short"){
        return strValue.toShort();
    }else if(strTypeName == "qulonglong"){
        return strValue.toULongLong();
    }else if(strTypeName == "UShort"){
        return strValue.toUShort();
    }else if(strTypeName == "UChar"){
        return (uchar)strValue.toUInt();
    }else if(strTypeName == "QImage"){
        QImage img;
        if(img.loadFromData(QByteArray::fromBase64(strValue.toLatin1())))
            return img;
        return img;
    }else if(strTypeName == "QPixmap"){
        QPixmap img;
        if(img.loadFromData(QByteArray::fromBase64(strValue.toLatin1())))
            return img;
        return img;
    }else if(strTypeName == "QByteArray"){
        return QByteArray::fromBase64(strValue.toLatin1());
    }
    return QVariant();
}

QString SObject::variantToString(const QVariant &varValue, QString &strTypeName)
{
    strTypeName = varValue.typeName();
    if(strTypeName == "QPixmap"){
        QByteArray ba;
        QBuffer buffer(&ba);
        QPixmap image = varValue.value<QPixmap>();
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG"); // writes image into ba in PNG format

        return QString::fromLatin1(ba.toBase64());
    }else if(strTypeName == "QImage"){
        QByteArray ba;
        QBuffer buffer(&ba);
        QImage image = varValue.value<QImage>();
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG"); // writes image into ba in PNG format

        return QString::fromLatin1(ba.toBase64());
    }else if(strTypeName == "QByteArray"){
        return QString::fromLatin1(varValue.toByteArray().toBase64());
    }
    return varValue.toString();
}

void SObject::clone(SObject& des, const SObject& src)
{
    SObject *pChild;
    des.setObjectName(src.objectName());
    des.m_propertyInfo = src.m_propertyInfo;
    auto lstNames = src.dynamicPropertyNames();
    for(auto iterParam = lstNames.begin(); iterParam != lstNames.end(); iterParam++){
        des.setProperty(iterParam->data(), src.property(iterParam->data()));
    }
    QObjectList lstChild = src.children();
    for(auto iterChild = lstChild.begin(); iterChild != lstChild.end(); iterChild++){
        pChild = new SObject(&des);
        clone(*pChild, *((SObject*)(*iterChild)));
    }
}
QMap<QString, SObject::PropertyInfo>& SObject::propertyInfo()
{
    return m_propertyInfo;
}

void SObject::setPropertyInfo(const QMap<QString, PropertyInfo> &propertyInfo)
{
    m_propertyInfo = propertyInfo;
}

QReadWriteLock &SObject::lock()
{
    return m_lock;
}

void SObject::setAsRoot(bool bRoot)
{
    if(( bRoot && m_rootParam != NULL)
            || ( !bRoot && m_rootParam == NULL)
            )
        return;
    if(bRoot){
        m_rootParam = new RootParamT();
        m_rootParam->timerIDForCheck = startTimer(500);
    }else{
        killTimer(m_rootParam->timerIDForCheck);
    }
}

void SObject::updatedProperty(const QString &strPropName)
{
    SObject* pRoot = root();
    if(pRoot != NULL){
        pRoot->m_rootParam->lock.lockForWrite();
        pRoot->m_rootParam->changed[this][strPropName] = this;
        pRoot->m_rootParam->lock.unlock();
    }
}

void SObject::changeProperty(QObject *pFrom, const QString &strPropName)
{
    if(pFrom == NULL
            || strPropName.isEmpty()
            || pFrom == (QObject*)this)
        return;
    SObject* pRoot = root();
    QVariant varValue;
    if(pRoot != NULL){
        pRoot->m_rootParam->lock.lockForWrite();
        pRoot->m_rootParam->changed[this][strPropName] = pFrom;
        pRoot->m_rootParam->lock.unlock();
    }else{
        varValue = pFrom->property(strPropName.toUtf8().data());
        if(varValue.isNull())
            return;
        setProperty(strPropName.toUtf8().data(), varValue);
    }
}

QStringList SObject::supportedTypes()
{
    return QStringList() << "bool" << "int" << "uint" << "double" << "char" << "SChar" << "QString"
                         << "float" << "qlonglong" << "short" << "qulonglong" << "ushort"
                         << "uchar" << "QImage" << "QPixmap" << "QByteArray";
}

void SObject::timerEvent(QTimerEvent *evt)
{
    if(m_rootParam != NULL
            && evt->timerId() == m_rootParam->timerIDForCheck) {
        SObject* pRoot = root();

        if(pRoot == NULL
                || pRoot->m_rootParam->changed.isEmpty())
            return;
        SObject* pChangedObj;
        QVariant varValue;
        QObject* pFromObj;
        QString strName;
        QByteArray baNameUtf;

        pRoot->m_rootParam->lock.lockForWrite();
        for(auto iterObj = pRoot->m_rootParam->changed.begin();
            iterObj != pRoot->m_rootParam->changed.end();
            iterObj++){
            pChangedObj = iterObj.key();
            for(auto iterProp = iterObj->begin(); iterProp != iterObj->end(); iterProp++){
                pFromObj = iterProp.value();
                strName = iterProp.key();
                baNameUtf = strName.toUtf8();
                if(pFromObj == (QObject*)pChangedObj){
                    varValue = pChangedObj->property(baNameUtf.data());
                }else{
                    varValue = pFromObj->property(baNameUtf.data());
                    if(varValue.isNull())
                        continue;
                    pChangedObj->QObject::setProperty(baNameUtf.data(), varValue);
                    pChangedObj->propertyInfo()[strName].version++;
                }
                emit pChangedObj->propertyChanged(pChangedObj, strName, varValue, pFromObj);
            }
        }
        pRoot->m_rootParam->changed.clear();
        pRoot->m_rootParam->lock.unlock();
    }
}

SObject *SObject::root()
{
    SObject* pRoot = this;
    while(pRoot != NULL){
        if(pRoot->m_rootParam != NULL)
            return pRoot;
        pRoot = (SObject*)pRoot->parent();
    }
    return NULL;
}

