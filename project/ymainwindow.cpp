#include "ymainwindow.h"
#include "ynetdock.h"
#include "yuartdock.h"
#include "yusbdock.h"
#include <stdlib.h>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QInputDialog>
#include <QSplitter>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QStatusBar>
#include <QScrollArea>
#include <sobject.h>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimerEvent>
#include <QSettings>
#include <QCoreApplication>
#include <QVariant>
#include <QApplication>

YMainWindow::YMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_scriptEngine(this)
{
    m_scriptEngine.globalObject().setProperty("_m", m_scriptEngine.newQObject(this));
    m_scriptEngine.globalObject().setProperty("log", m_scriptEngine.newFunction(jsLog, this));
    createAction();
    createMenuBar();
    createCentral();
    createStatusBar();
    closeProject();
    setWindowTitle(tr("YYDebug"));
}

YMainWindow::~YMainWindow()
{

}

QLabel *YMainWindow::label(YMainWindow::LabelT id)
{
    return m_labels[id];
}

bool YMainWindow::openProject(SObject *pObj)
{
    SObject* pObjs[5];
    if(pObj == NULL
            || pObj->property("type").toString() != "debug"
            || (pObjs[0] = pObj->findChild<SObject*>("comm")) == NULL)
        return false;
    QString strPortType;
    YCommDock* pCommDock = NULL;
    strPortType = pObjs[0]->property("type").toString();
    if(strPortType == "uart")
        pCommDock = new YUartDock(pObjs[0], this);
    else if(strPortType == "usb")
        pCommDock = new YUSBDock(pObjs[0], this);
    else if(strPortType == "network")
        pCommDock = new YNetDock(pObjs[0], this);
    if(pCommDock == NULL)
        return false;
    if(m_commDock != NULL)
        closeProject();
    addDockWidget(Qt::LeftDockWidgetArea, pCommDock);
    m_scriptEngine.globalObject().setProperty("_d", m_scriptEngine.newQObject(pCommDock));

    //QMenuBar*pMB = menuBar();
    //pMB->insertMenu()
    connect(pCommDock, SIGNAL(connectedStatus(bool)), m_buttons[PB_TX_SEND], SLOT(setEnabled(bool)));
    connect(pCommDock, SIGNAL(connectedStatus(bool)), m_buttons[PB_TX_SEND_FILE], SLOT(setEnabled(bool)));
    connect(pCommDock, SIGNAL(connectedStatus(bool)), m_checkBoxs[CK_TX_AUTO], SLOT(setEnabled(bool)));
    connect(pCommDock, SIGNAL(connectedStatus(bool)), m_checkBoxs[CK_TX_EXT_AUTO], SLOT(setEnabled(bool)));
    emit pCommDock->connectedStatus(false);
    connect(pCommDock, SIGNAL(receivedData(const uchar*,uint)), SLOT(receivedData(const uchar*,uint)));
    m_commDock = pCommDock;
    m_projectObj = pObj;
    m_actions[A_CLOSE]->setEnabled(true);
    m_actions[A_SAVE]->setEnabled(true);
    m_actions[A_SAVE_AS]->setEnabled(true);
    updateRecentProjectMenu();
    // initailize ui by sobject
    if((pObjs[0] = pObj->findChild<SObject*>("main")) == NULL)
        return true;
    setWindowTitle("YYDebug-" + pObjs[0]->property("name").toString());

    if((pObjs[1] = pObjs[0]->findChild<SObject*>("rx")) == NULL)
        return true;
    m_mainWidget[W_RX]->setVisible(pObjs[1]->property("visible").toBool());
//    m_comboBoxs[CB_RX_WORD_LEN]->setCurrentText(pObjs[1]->property("word_bytes").toString());
    m_comboBoxs[CB_RX_FORMAT]->setCurrentText(pObjs[1]->property("format").toString());
    if((pObjs[2] = pObjs[1]->findChild<SObject*>("ext")) == NULL)
        return true;
    m_checkBoxs[CK_RX_EXT]->setChecked(pObjs[2]->property("visible").toBool());
    m_textEdits[TE_RX_SCRIPT]->setText(pObjs[2]->property("script").toString());

    if((pObjs[1] = pObjs[0]->findChild<SObject*>("tx")) == NULL)
        return true;
    m_mainWidget[W_TX]->setVisible(pObjs[1]->property("visible").toBool());
//    m_comboBoxs[CB_TX_WORD_LEN]->setCurrentText(pObjs[1]->property("word_bytes").toString());
    m_comboBoxs[CB_TX_FORMAT]->setCurrentText(pObjs[1]->property("format").toString());
    m_lineEdits[LE_TX_INTERVAL]->setText(pObjs[1]->property("interval").toString());
    m_lineEdits[LE_TX_TIMES]->setText(pObjs[1]->property("times").toString());
    m_textEdits[TE_TX]->setText(pObjs[1]->property("value").toString());
    if((pObjs[2] = pObjs[1]->findChild<SObject*>("ext")) == NULL)
        return true;
    m_checkBoxs[CK_TX_EXT]->setChecked(pObjs[2]->property("visible").toBool());
    m_lineEdits[LE_TX_FILE_NAME]->setText(pObjs[2]->property("file_name").toString());
    m_lineEdits[LE_TX_EXT_ITEM_INTERVAL]->setText(pObjs[2]->property("item_interval").toString());
    m_lineEdits[LE_TX_EXT_FRAME_INTERVAL]->setText(pObjs[2]->property("frame_interval").toString());
    for(int nIndex = 0; nIndex < 20; nIndex++){
        m_cmdItems[nIndex].name->setText(
                    pObjs[2]->property(tr("item%1_name").arg(nIndex + 1).toUtf8().data()).toString());
        m_cmdItems[nIndex].value->setText(
                    pObjs[2]->property(tr("item%1_value").arg(nIndex + 1).toUtf8().data()).toString());
    }
    QByteArray baLayout = pObjs[0]->property("layout").toByteArray();
    if(!baLayout.isEmpty())
        ((QSplitter*)m_mainWidget[W_MAIN_SPLITTER])->restoreState(baLayout);
    showMaximized();
    return true;
}

template <typename T>
void convertInt(QVector<T>& vecDes, const QString& strSrc, int nBase){
    QStringList lstValue = strSrc.split(' ');

    foreach (QString strItem, lstValue) {
        if(strItem.isEmpty())
            continue;
        vecDes.append((T)strItem.toLongLong(0, nBase));
    }
}


int YMainWindow::sendData(const QString &strValue, const QString &strFormat)
{
    if(strValue.isEmpty())
        return 0;

    if(strFormat == "ansi"){
        QByteArray ba = strValue.toLatin1();
        return m_commDock->commWrite(ba.data(), ba.size());
    }else if(strFormat == "hex8"){
        QVector<quint8> baValue;
        convertInt(baValue, strValue, 16);
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(quint8));
    }else if(strFormat == "hex16"){
        QVector<quint16> baValue;
        convertInt(baValue, strValue, 16);
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(quint16));
    }else if(strFormat == "hex32"){
        QVector<quint32> baValue;
        convertInt(baValue, strValue, 16);
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(quint32));
    }else if(strFormat == "hex64"){
        QVector<quint64> baValue;
        convertInt(baValue, strValue, 16);
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(quint64));
    }else if(strFormat == "uint8"){
        QVector<quint8> baValue;
        convertInt(baValue, strValue, 10);
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(quint8));
    }else if(strFormat == "uint16"){
        QVector<quint16> baValue;
        convertInt(baValue, strValue, 10);
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(quint16));
    }else if(strFormat == "uint32"){
        QVector<quint32> baValue;
        convertInt(baValue, strValue, 10);
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(quint32));
    }else if(strFormat == "uint64"){
        QVector<quint64> baValue;
        convertInt(baValue, strValue, 10);
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(quint64));
    }else if(strFormat == "binary8"){
        QVector<quint8> baValue;
        convertInt(baValue, strValue, 2);
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(quint8));
    }else if(strFormat == "binary16"){
        QVector<quint16> baValue;
        convertInt(baValue, strValue, 2);
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(quint16));
    }else if(strFormat == "binary32"){
        QVector<quint32> baValue;
        convertInt(baValue, strValue, 2);
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(quint32));
    }else if(strFormat == "binary64"){
        QVector<quint64> baValue;
        convertInt(baValue, strValue, 2);
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(quint64));
    }else if(strFormat == "float"){
        QVector<float> baValue;
        QStringList lstValue = strValue.split(' ');

        foreach (QString strItem, lstValue) {
            if(strItem.isEmpty())
                continue;
            baValue.append(strItem.toFloat());
        }
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(float));
    }else if(strFormat == "double"){
        QVector<double> baValue;
        QStringList lstValue = strValue.split(' ');

        foreach (QString strItem, lstValue) {
            if(strItem.isEmpty())
                continue;
            baValue.append(strItem.toDouble());
        }
        return m_commDock->commWrite(baValue.data(), baValue.size() * sizeof(double));
    }
    return 0;
}

QScriptValue YMainWindow::jsLog(QScriptContext *context, QScriptEngine *engine, void* pArgs)
{
    Q_UNUSED(engine)
    if(context->argumentCount() < 1)
        return QScriptValue();
    YMainWindow* pMW = (YMainWindow*)pArgs;
    pMW->appendLog(context->argument(0).toString());
    return QScriptValue();
}

void YMainWindow::timerEvent(QTimerEvent *evt)
{
    if(evt->timerId() == m_timerIDs[T_TX_AUTO]){
        sendTxValue();
        if((--m_counters[C_TX_AUTO]) == 0){
            autoSend(false);
        }
        m_lineEdits[LE_TX_TIMES]->setText(QString::number(m_counters[C_TX_AUTO]));
    }
}

void YMainWindow::createAction()
{
    m_actions[A_NEW] = new QAction(tr("&New"), this);
    connect(m_actions[A_NEW], SIGNAL(triggered()), SLOT(newProject()));
    m_actions[A_OPEN] = new QAction(tr("&Open"), this);
    connect(m_actions[A_OPEN], SIGNAL(triggered()), SLOT(openProject()));
    m_actions[A_SAVE] = new QAction(tr("&Save"), this);
    connect(m_actions[A_SAVE], SIGNAL(triggered()), SLOT(saveProject()));
    m_actions[A_SAVE_AS] = new QAction(tr("Save &As"), this);
    connect(m_actions[A_SAVE_AS], SIGNAL(triggered()), SLOT(saveProjectAs()));
    m_actions[A_CLOSE] = new QAction(tr("&Close"), this);
    m_actions[A_CLOSE]->setEnabled(false);
    connect(m_actions[A_CLOSE], SIGNAL(triggered()), SLOT(closeProject()));
    m_actions[A_EXIT] = new QAction(tr("&Exit"), this);
    connect(m_actions[A_EXIT], SIGNAL(triggered()), SLOT(close()));
    m_actions[A_ABOUT] = new QAction(tr("&About"), this);
    connect(m_actions[A_ABOUT], SIGNAL(triggered()), SLOT(about()));
}

void YMainWindow::createMenuBar()
{
    QMenuBar* pMenuBar = menuBar();
    QMenu* arrpMenu[2];

    arrpMenu[0] = pMenuBar->addMenu(tr("&File"));
    arrpMenu[0]->addAction(m_actions[A_NEW]);
    arrpMenu[0]->addAction(m_actions[A_OPEN]);
    arrpMenu[0]->addAction(m_actions[A_SAVE]);
    arrpMenu[0]->addAction(m_actions[A_SAVE_AS]);
    arrpMenu[0]->addAction(m_actions[A_CLOSE]);
    m_menus[M_RECENT] = arrpMenu[0]->addMenu(tr("&Recent projects"));
    updateRecentProjectMenu();
    arrpMenu[0]->addAction(m_actions[A_EXIT]);

    arrpMenu[0] = pMenuBar->addMenu(tr("&Help"));
    arrpMenu[0]->addAction(m_actions[A_ABOUT]);
}

void YMainWindow::createCentral()
{
    QSplitter* arrpSplitter[3];
    QGroupBox* arrpGroup[3];
    QGridLayout* arrpLayout[3];
    QHBoxLayout* arrpHLayout[3];
    int nCol,nRow;

    m_mainWidget[W_MAIN_SPLITTER] = arrpSplitter[1] = new QSplitter(Qt::Vertical, this);
    // received
    m_mainWidget[W_RX] = arrpGroup[0] = new QGroupBox(tr("Received"), this);
    m_textEdits[TE_RX] = new QTextEdit(this);
    m_buttons[PB_RX_CLEAR] = new QPushButton(tr("Clear"), this);
    m_buttons[PB_RX_SAVE] = new QPushButton(tr("Save"), this);
//    m_comboBoxs[CB_RX_WORD_LEN] = new QComboBox(this);
//    m_comboBoxs[CB_RX_WORD_LEN]->addItem("1", 1);
//    m_comboBoxs[CB_RX_WORD_LEN]->addItem("2", 2);
//    m_comboBoxs[CB_RX_WORD_LEN]->addItem("4", 4);
//    m_comboBoxs[CB_RX_WORD_LEN]->addItem("8", 8);
    m_comboBoxs[CB_RX_FORMAT] = new QComboBox(this);
    m_comboBoxs[CB_RX_FORMAT]->addItems(
                QStringList() << "hex8" << "hex16" << "hex32" << "hex64"
                                << "uint8" << "uint16" << "uint32" << "uint64"
                                << "binary8" << "binary16" << "binary32" << "binary64"
                                << "float" << "double" << "ansi" << "unicode" << "utf-8" << "none");
    m_checkBoxs[CK_RX_EXT] = new QCheckBox(tr("Extend"), this);

    arrpHLayout[0] = new QHBoxLayout();
    arrpHLayout[0]->addWidget(m_textEdits[TE_RX], 1);
    m_mainWidget[W_RX_EXT] = createRecvExt(arrpGroup[0]);
    arrpHLayout[0]->addWidget(m_mainWidget[W_RX_EXT]);

    arrpLayout[0] = new QGridLayout(arrpGroup[0]);
    nCol = 0;
    nRow = 1;
    arrpLayout[0]->addWidget(m_buttons[PB_RX_CLEAR], nRow, nCol++);
    arrpLayout[0]->addWidget(m_buttons[PB_RX_SAVE], nRow, nCol++);
//    arrpLayout[0]->addWidget(new QLabel(tr("Word(Byte)")), nRow, nCol++);
//    arrpLayout[0]->addWidget(m_comboBoxs[CB_RX_WORD_LEN], nRow, nCol++);
    arrpLayout[0]->addWidget(new QLabel(tr("Format")), nRow, nCol++);
    arrpLayout[0]->addWidget(m_comboBoxs[CB_RX_FORMAT], nRow, nCol++);
    arrpLayout[0]->setColumnStretch(nCol++, 1);
    arrpLayout[0]->addWidget(m_checkBoxs[CK_RX_EXT], nRow, nCol++);
    arrpLayout[0]->addLayout(arrpHLayout[0], --nRow, 0, 1, nCol);
    //arrpLayout[0]->setColumnStretch(nCol, 1);
    arrpSplitter[1]->addWidget(arrpGroup[0]);

    // send
    m_mainWidget[W_TX] = arrpGroup[0] = new QGroupBox(tr("Send"), this);
    m_textEdits[TE_TX] = new QTextEdit(this);
    m_buttons[PB_TX_SEND] = new QPushButton(tr("Send"), this);
    m_buttons[PB_TX_CLEAR] = new QPushButton(tr("Clear"), this);
//    m_comboBoxs[CB_TX_WORD_LEN] = new QComboBox(this);
//    m_comboBoxs[CB_TX_WORD_LEN]->addItem("1", 1);
//    m_comboBoxs[CB_TX_WORD_LEN]->addItem("2", 2);
//    m_comboBoxs[CB_TX_WORD_LEN]->addItem("4", 4);
//    m_comboBoxs[CB_TX_WORD_LEN]->addItem("8", 8);
    m_comboBoxs[CB_TX_FORMAT] = new QComboBox(this);
    m_comboBoxs[CB_TX_FORMAT]->addItems(QStringList() << "hex8" << "hex16" << "hex32" << "hex64"
                                        << "uint8" << "uint16" << "uint32" << "uint64"
                                        << "binary8" << "binary16" << "binary32" << "binary64"
                                        << "float" << "double" << "ansi");
    m_checkBoxs[CK_TX_AUTO] = new QCheckBox(tr("Auto"), this);
    m_lineEdits[LE_TX_INTERVAL] = new QLineEdit(tr("1000"), this);
    m_lineEdits[LE_TX_TIMES] = new QLineEdit(tr("0"), this);
    m_checkBoxs[CK_TX_EXT] = new QCheckBox(tr("Extend"), this);
    m_checkBoxs[CK_TX_EXT]->hide();

    arrpHLayout[0] = new QHBoxLayout();
    arrpHLayout[0]->addWidget(m_textEdits[TE_TX], 1);
    m_mainWidget[W_TX_EXT] = createSendExt(arrpGroup[0]);
    arrpHLayout[0]->addWidget(m_mainWidget[W_TX_EXT]);

    arrpLayout[0] = new QGridLayout(arrpGroup[0]);
    nCol = 0;
    nRow = 0;
    arrpLayout[0]->addWidget(m_buttons[PB_TX_SEND], nRow, nCol++);
    arrpLayout[0]->addWidget(m_buttons[PB_TX_CLEAR], nRow, nCol++);
//    arrpLayout[0]->addWidget(new QLabel(tr("Word(Byte)")), nRow, nCol++);
//    arrpLayout[0]->addWidget(m_comboBoxs[CB_TX_WORD_LEN], nRow, nCol++);
    arrpLayout[0]->addWidget(new QLabel(tr("Format")), nRow, nCol++);
    arrpLayout[0]->addWidget(m_comboBoxs[CB_TX_FORMAT], nRow, nCol++);
    arrpLayout[0]->addWidget(m_checkBoxs[CK_TX_AUTO], nRow, nCol++);
    arrpLayout[0]->addWidget(new QLabel(tr("Interval(ms)")), nRow, nCol++);
    arrpLayout[0]->addWidget(m_lineEdits[LE_TX_INTERVAL], nRow, nCol++);
    arrpLayout[0]->addWidget(new QLabel(tr("Times")), nRow, nCol++);
    arrpLayout[0]->addWidget(m_lineEdits[LE_TX_TIMES], nRow, nCol++);
    arrpLayout[0]->setColumnStretch(nCol++, 1);
    arrpLayout[0]->addWidget(m_checkBoxs[CK_TX_EXT], nRow, nCol++);

    arrpLayout[0]->addLayout(arrpHLayout[0], ++nRow, 0, 1, nCol);
    //arrpLayout[0]->setColumnStretch(nCol, 1);
    arrpSplitter[1]->addWidget(arrpGroup[0]);
    setCentralWidget(arrpSplitter[1]);

    m_mainWidget[W_RX_EXT]->hide();
    m_mainWidget[W_TX_EXT]->hide();
    connect(m_checkBoxs[CK_RX_EXT], SIGNAL(toggled(bool)), m_mainWidget[W_RX_EXT], SLOT(setVisible(bool)));
    connect(m_buttons[PB_RX_CLEAR], SIGNAL(clicked()), SLOT(clearRxCounter()));
    connect(m_buttons[PB_RX_SAVE], SIGNAL(clicked()), SLOT(saveRx()));
    connect(m_checkBoxs[CK_TX_EXT], SIGNAL(toggled(bool)), m_mainWidget[W_TX_EXT], SLOT(setVisible(bool)));
    connect(m_buttons[PB_TX_SEND], SIGNAL(clicked()), SLOT(sendTxValue()));
    connect(m_buttons[PB_TX_CLEAR], SIGNAL(clicked()), SLOT(clearTxCounter()));
    connect(m_checkBoxs[CK_TX_AUTO], SIGNAL(toggled(bool)), SLOT(autoSend(bool)));
}

void YMainWindow::createStatusBar()
{
    QStatusBar* pStatusBar = new QStatusBar(this);
    m_labels[L_COMM_STATUS] = new QLabel(tr("No port has been connected."), this);
    m_labels[L_DATA_STATS] = new QLabel(tr("Rx:0 Tx:0"), this);
    pStatusBar->addWidget(m_labels[L_COMM_STATUS], 1);
    pStatusBar->addWidget(m_labels[L_DATA_STATS]);
    pStatusBar->addPermanentWidget(new QLabel(tr("\tYYDebug v1.0.0.0")));
    setStatusBar(pStatusBar);
}

QWidget *YMainWindow::createSendExt(QWidget *parent)
{
    QGroupBox* arrpGroup[3];
    QGridLayout* arrpLayout[3];
    QScrollArea* pScrollArea = new QScrollArea(this);
    QWidget* pScrollWidget = new QWidget(this);
    int nRow,nCol;

    arrpGroup[0] = new QGroupBox(tr("Extend"), parent);
    arrpLayout[0] = new QGridLayout(arrpGroup[0]);
    arrpGroup[1] = new QGroupBox(tr("File"), arrpGroup[0]);
    arrpLayout[1] = new QGridLayout(arrpGroup[1]);
    m_lineEdits[LE_TX_FILE_NAME] = new QLineEdit(this);
    m_buttons[PB_TX_SEL_FILE] = new QPushButton(tr("Select"), this);
    m_buttons[PB_TX_SEND_FILE] = new QPushButton(tr("Send"), this);
    nRow = nCol = 0;
    arrpLayout[1]->addWidget(m_lineEdits[LE_TX_FILE_NAME], nRow, nCol++);
    arrpLayout[1]->addWidget(m_buttons[PB_TX_SEL_FILE], nRow, nCol++);
    arrpLayout[1]->addWidget(m_buttons[PB_TX_SEND_FILE], nRow, nCol++);
    arrpLayout[1]->setColumnStretch(0, 1);

    m_checkBoxs[CK_TX_EXT_AUTO] = new QCheckBox(tr("Auto"), this);
    m_lineEdits[LE_TX_EXT_ITEM_INTERVAL] = new QLineEdit(this);
    m_lineEdits[LE_TX_EXT_FRAME_INTERVAL] = new QLineEdit(this);


    arrpLayout[2] = new QGridLayout(pScrollWidget);
    nRow = nCol = 0;
    arrpLayout[2]->addWidget(new QLabel(tr("Name"), this), nRow, nCol++, Qt::AlignHCenter);
    arrpLayout[2]->addWidget(new QLabel(tr("Data"), this), nRow, nCol++, Qt::AlignHCenter);
    arrpLayout[2]->addWidget(new QLabel(tr("Send"), this), nRow++, nCol++, Qt::AlignHCenter);
    for(int nIndex = 0; nIndex < 20; nIndex++){
        nCol = 0;
        m_cmdItems[nIndex].name = new QLineEdit(this);
        m_cmdItems[nIndex].value = new QLineEdit(this);
        m_cmdItems[nIndex].send = new QPushButton(QString::number(nIndex + 1), this);
        arrpLayout[2]->addWidget(m_cmdItems[nIndex].name, nRow, nCol++,Qt::AlignHCenter);
        arrpLayout[2]->addWidget(m_cmdItems[nIndex].value, nRow, nCol++, Qt::AlignHCenter);
        arrpLayout[2]->addWidget(m_cmdItems[nIndex].send, nRow++, nCol++, Qt::AlignHCenter);
    }
    arrpLayout[2]->setColumnStretch(1, 1);
    pScrollArea->setWidget(pScrollWidget);

    nRow = 0;
    arrpLayout[0]->addWidget(arrpGroup[1], nRow, 0, 1, 5);
    nCol = 0;
    arrpLayout[0]->addWidget(m_checkBoxs[CK_TX_EXT_AUTO], ++nRow, nCol++);
    arrpLayout[0]->addWidget(new QLabel(tr("Item(ms)")), nRow, nCol++);
    arrpLayout[0]->addWidget(m_lineEdits[LE_TX_EXT_ITEM_INTERVAL], nRow, nCol++);
    arrpLayout[0]->addWidget(new QLabel(tr("Frame(ms)")), nRow, nCol++);
    arrpLayout[0]->addWidget(m_lineEdits[LE_TX_EXT_FRAME_INTERVAL], nRow, nCol++);
    arrpLayout[0]->addWidget(pScrollArea, ++nRow, 0, 1, 5);
    return arrpGroup[0];
}

QWidget *YMainWindow::createRecvExt(QWidget *parent)
{
    QGroupBox* arrpGroup[3];
    QVBoxLayout* arrpVLayout[3];
    arrpGroup[0] = new QGroupBox(tr("Extend"), parent);
    arrpVLayout[0] = new QVBoxLayout(arrpGroup[0]);
    arrpGroup[1] = new QGroupBox(tr("JavaScript"), this);
    arrpVLayout[1] = new QVBoxLayout(arrpGroup[1]);
    m_textEdits[TE_RX_SCRIPT] = new QTextEdit(this);
    arrpVLayout[1]->addWidget(m_textEdits[TE_RX_SCRIPT]);
    arrpGroup[2] = new QGroupBox(tr("Log"), this);
    arrpVLayout[2] = new QVBoxLayout(arrpGroup[2]);
    m_textEdits[TE_LOG] = new QTextEdit(this);
    arrpVLayout[2]->addWidget(m_textEdits[TE_LOG]);
    arrpVLayout[0]->addWidget(arrpGroup[1]);
    arrpVLayout[0]->addWidget(arrpGroup[2]);

    connect(m_textEdits[TE_RX_SCRIPT], SIGNAL(textChanged()), SLOT(scriptTxtChanged()));
    return arrpGroup[0];
}

void YMainWindow::newProject()
{
    QDialog dlgNew(this);
    QGridLayout* pLayout = new QGridLayout(&dlgNew);
    QLineEdit* arrpEdits[4];
    QComboBox* pTypeCombo = new QComboBox(&dlgNew);
    QPushButton* arrpBt[3];
    QHBoxLayout* pHLayout = new QHBoxLayout();
    int nIndex,nRow = 0;

    dlgNew.setWindowTitle(tr("New project"));
    arrpEdits[0] = new QLineEdit(&dlgNew);
    arrpEdits[1] = new QLineEdit(&dlgNew);
    arrpBt[0] = new QPushButton(tr("..."), this);
    arrpBt[1] = new QPushButton(tr("Create"), this);
    arrpBt[2] = new QPushButton(tr("Cancel"), this);
    pHLayout->addWidget(arrpBt[1]);
    pHLayout->addWidget(arrpBt[2]);
    pTypeCombo->addItems(QStringList() << "UART" << "USB" << "Network");
    pLayout->addWidget(new QLabel(tr("Name"), this), nRow, 0, 0);
    pLayout->addWidget(arrpEdits[0], nRow, 1, 1, 2);
    pLayout->addWidget(new QLabel(tr("Type"), this), ++nRow, 0, 0);
    pLayout->addWidget(pTypeCombo, nRow, 1);
    pLayout->addWidget(new QLabel(tr("Path"), this), ++nRow, 0, 0);
    pLayout->addWidget(arrpEdits[1], nRow, 1);
    pLayout->addWidget(arrpBt[0], nRow, 2);
    pLayout->addLayout(pHLayout, ++nRow, 0, 1, 3, Qt::AlignHCenter);

    setProperty("file_name_edit", QVariant::fromValue((void*)arrpEdits[1]));
    connect(arrpBt[0], SIGNAL(clicked()), SLOT(selectSaveFile()));
    connect(arrpBt[1], SIGNAL(clicked()), &dlgNew, SLOT(accept()));
    connect(arrpBt[2], SIGNAL(clicked()), &dlgNew, SLOT(reject()));
    forever{
        if(dlgNew.exec() != QDialog::Accepted)
            return;
        if(arrpEdits[0]->text().isEmpty()
                || arrpEdits[1]->text().isEmpty()){
            QMessageBox::warning(this, tr("Error"), tr("The name and path can't be empty."));
        }else
            break;
    }

    SObject* pObjs[4];
    pObjs[0] = new SObject();
    pObjs[0]->setObjectName("yy");
    pObjs[0]->setProperty("type", "debug");
    pObjs[0]->setProperty("path", arrpEdits[1]->text());

    pObjs[1] = new SObject(pObjs[0]);
    pObjs[1]->setObjectName("main");
    pObjs[1]->setProperty("name", arrpEdits[0]->text());
    pObjs[1]->setProperty("layout", QByteArray());
    pObjs[2] = new SObject(pObjs[1]);
    pObjs[2]->setObjectName("rx");
    pObjs[2]->setProperty("visible", true);
//    pObjs[2]->setProperty("word_bytes", "1");
    pObjs[2]->setProperty("format", "hex");
    pObjs[3] = new SObject(pObjs[2]);
    pObjs[3]->setObjectName("ext");
    pObjs[3]->setProperty("visible", false);
    pObjs[3]->setProperty("script", "");

    pObjs[2] = new SObject(pObjs[1]);
    pObjs[2]->setObjectName("tx");
    pObjs[2]->setProperty("visible", true);
//    pObjs[2]->setProperty("word_bytes", "1");
    pObjs[2]->setProperty("format", "hex");
    pObjs[2]->setProperty("interval", 1000);
    pObjs[2]->setProperty("times", 0);
    pObjs[2]->setProperty("value", "");
    pObjs[3] = new SObject(pObjs[2]);
    pObjs[3]->setObjectName("ext");
    pObjs[3]->setProperty("visible", false);
    pObjs[3]->setProperty("file_name", "");
    pObjs[3]->setProperty("item_interval", 500);
    pObjs[3]->setProperty("frame_interval", 1000);
    for(nIndex = 0; nIndex < 20; nIndex++){
        pObjs[3]->setProperty(tr("item%1_name").arg(nIndex + 1).toUtf8().data(), "");
        pObjs[3]->setProperty(tr("item%1_value").arg(nIndex + 1).toUtf8().data(), "");
    }

    pObjs[1] = new SObject(pObjs[0]);
    pObjs[1]->setObjectName("comm");
    if(pTypeCombo->currentText() == "UART"){
        YUartDock::initSObject(pObjs[1]);
    }else if(pTypeCombo->currentText() == "USB"){
        YUSBDock::initSObject(pObjs[1]);
    }else if(pTypeCombo->currentText() == "Network"){
        YNetDock::initSObject(pObjs[1]);
    }
    pObjs[0]->saveFile(arrpEdits[1]->text());
    openProject(pObjs[0]);
}

void YMainWindow::openProject(const QString& strFileName)
{
    QString strFileName2;
    strFileName2 = strFileName;
    if(strFileName.isEmpty())
        strFileName2 =  QFileDialog::getOpenFileName(this, tr("Open project"), "../examples",
                                                     tr("YYDebug Project (*.ydproj)"));
    else
        strFileName2 = strFileName;
    if(strFileName2.isEmpty())
        return;
    SObject *pProObj = new SObject();
    if(!pProObj->openFile(strFileName2)
            || !openProject(pProObj)){
        delete pProObj;
        return;
    }
    pProObj->setAsRoot(true);
    pProObj->setProperty("path", strFileName);

    QString strPros;
    QSettings setting;
    QStringList lstPro = setting.value("recent_projects").toString().split(',');
    int nIndex = lstPro.indexOf(strFileName2);
    if(nIndex >= 0)
        lstPro.removeAt(nIndex);
    while (lstPro.size() > 5) {
        lstPro.removeLast();
    }
    strPros = strFileName2;
    if(lstPro.size() > 0
            && !lstPro[0].isEmpty()){
        foreach (QString strPro, lstPro) {
            strPros += "," + strPro;
        }
    }
    setting.setValue("recent_projects", strPros);
}

void YMainWindow::closeProject()
{
    m_mainWidget[W_RX]->hide();
    m_mainWidget[W_TX]->hide();
    m_actions[A_CLOSE]->setEnabled(false);
    m_actions[A_SAVE]->setEnabled(false);
    m_actions[A_SAVE_AS]->setEnabled(false);
    if(m_commDock){
        delete m_commDock;
        m_commDock = NULL;
    }
    if(m_projectObj){
       delete m_projectObj;
        m_projectObj = NULL;
    }
}

void YMainWindow::saveProject()
{
    if(m_projectObj){
        m_commDock->updateSObject();
        updateProjectObject();
        m_projectObj->saveFile(m_projectObj->property("path").toString());
    }
}

void YMainWindow::saveProjectAs()
{
    if(m_projectObj == NULL)
        return;
    QString strFileName = QFileDialog::getSaveFileName(this, tr("Save as"),
                                                       m_projectObj->property("path").toString(),
                                                       tr("YYDebug Project (*.ydproj)"));
    if(strFileName.isEmpty())
        return;
    m_commDock->updateSObject();
    updateProjectObject();
    m_projectObj->saveFile(strFileName);
}

void YMainWindow::selectSaveFile()
{
    QLineEdit* pEdit = (QLineEdit*)property("file_name_edit").value<void*>();
    if(pEdit == NULL)
        return;
    QString strFileName = QFileDialog::getSaveFileName(this, tr("Save file"), QString(),
                                                   tr("YYDebug Project (*.ydproj)"));
    if(strFileName.isEmpty())
        return;
    pEdit->setText(strFileName);
}

void YMainWindow::sendTxValue()
{
    sendData(m_textEdits[TE_TX]->toPlainText(), m_comboBoxs[CB_TX_FORMAT]->currentText());
}

void YMainWindow::updateStatusBar()
{
    m_labels[L_DATA_STATS]->setText(tr("Rx:%1 Tx:%2").arg(m_counters[C_RX]).arg(m_counters[C_TX]));
}

void YMainWindow::clearRxCounter()
{
    m_counters[C_RX] = 0;
    m_textEdits[TE_RX]->setText("");
    updateStatusBar();
}

void YMainWindow::clearTxCounter()
{
    m_counters[C_TX] = 0;
    m_textEdits[TE_TX]->setText("");
    updateStatusBar();
}

void YMainWindow::updateProjectObject()
{
    SObject* pObjs[4];
    if((pObjs[0] = m_projectObj->findChild<SObject*>("main")) == NULL){
        pObjs[0] = new SObject(m_projectObj);
        pObjs[0]->setObjectName("main");
    }
    pObjs[0]->setProperty("layout", ((QSplitter*)m_mainWidget[W_MAIN_SPLITTER])->saveState());
    if((pObjs[1] = pObjs[0]->findChild<SObject*>("rx")) == NULL){
        pObjs[1] = new SObject(pObjs[0]);
        pObjs[1]->setObjectName("rx");
    }
    pObjs[1]->setProperty("visible", m_mainWidget[W_RX]->isVisible());
    pObjs[1]->setProperty("format", m_comboBoxs[CB_RX_FORMAT]->currentText());
    if((pObjs[2] = pObjs[1]->findChild<SObject*>("ext")) == NULL){
        pObjs[2] = new SObject(pObjs[1]);
        pObjs[2]->setObjectName("ext");
    }
    pObjs[2]->setProperty("visible", m_mainWidget[W_RX_EXT]->isVisible());
    pObjs[2]->setProperty("script", m_textEdits[TE_RX_SCRIPT]->toPlainText());

    if((pObjs[1] = pObjs[0]->findChild<SObject*>("tx")) == NULL){
        pObjs[1] = new SObject(pObjs[0]);
        pObjs[1]->setObjectName("tx");
    }
    pObjs[1]->setProperty("visible", m_mainWidget[W_TX]->isVisible());
    pObjs[1]->setProperty("format", m_comboBoxs[CB_TX_FORMAT]->currentText());
    pObjs[1]->setProperty("interval", m_lineEdits[LE_TX_INTERVAL]->text().toInt());
    pObjs[1]->setProperty("times", m_lineEdits[LE_TX_TIMES]->text().toInt());
    pObjs[1]->setProperty("value", m_textEdits[TE_TX]->toPlainText());
    if((pObjs[2] = pObjs[1]->findChild<SObject*>("ext")) == NULL){
        pObjs[2] = new SObject(pObjs[1]);
        pObjs[2]->setObjectName("ext");
    }
    pObjs[2]->setProperty("visible", m_mainWidget[W_TX_EXT]->isVisible());
    pObjs[2]->setProperty("script", m_textEdits[TE_RX_SCRIPT]->toPlainText());
    pObjs[2]->setProperty("file_name", m_lineEdits[LE_TX_FILE_NAME]->text());
    pObjs[2]->setProperty("item_interval", m_lineEdits[LE_TX_EXT_ITEM_INTERVAL]->text().toInt());
    pObjs[2]->setProperty("frame_interval", m_lineEdits[LE_TX_EXT_FRAME_INTERVAL]->text().toInt());

    for(int nIndex = 0; nIndex < 20; nIndex++){
        pObjs[2]->setProperty(tr("item%1_name").arg(nIndex + 1).toUtf8().data(),
                              m_cmdItems[nIndex].name->text());
        pObjs[2]->setProperty(tr("item%1_value").arg(nIndex + 1).toUtf8().data(),
                              m_cmdItems[nIndex].value->text());
    }
}

template <typename T>
void convert2String(const uchar *pData, uint uSize, QString& strRes, const char* cformat){
    QString strTemp;
    const T* pOffset, *pEnd;
    pOffset = (T*)pData;
    pEnd = pOffset + uSize / sizeof(T);
    while(pOffset < pEnd){
        strRes.append(strTemp.sprintf(cformat, *(pOffset++)));
    }
}

template <typename T>
void convert2Binary(const uchar *pData, uint uSize, QString& strRes){
    const T* pOffset, *pEnd;
    char *cOffset,*cEnd,cBin[66];
    uint uBitLen;
    T tVal;

    pOffset = (T*)pData;
    pEnd = pOffset + uSize / sizeof(T);
    uBitLen = sizeof(T) * 8;
    cBin[uBitLen] = '\0';
    cEnd = &cBin[0];
    while(pOffset < pEnd){
        tVal = *(pOffset++);
        cOffset = &cBin[uBitLen - 1];
        while(cOffset >= cEnd){
            *(cOffset--) = (tVal & 0x1) ? '1' : '0';
            tVal >>= 1;
        }
        strRes.append(cBin);
    }
}


void YMainWindow::receivedData(const uchar *pData, uint uSize)
{
    if(uSize == 0)
        return;

    QString strRes,strFormat;

    //m_scriptEngine.evaluate()
    strFormat = m_comboBoxs[CB_RX_FORMAT]->currentText();
    if(strFormat == "ansi"){
        strRes = QString::fromLatin1((char*)pData, uSize);
    }else if(strFormat == "unicode"){
        strRes = QString::fromWCharArray((wchar_t*)pData, uSize);
    }else if(strFormat == "utf-8"){
        strRes = QString::fromUtf8((char*)pData, uSize);
    }else if(strFormat == "hex8"){
        convert2String<quint8>(pData, uSize, strRes, "%02X ");
    }else if(strFormat == "hex16"){
        convert2String<quint16>(pData, uSize, strRes, "%04X ");
    }else if(strFormat == "hex32"){
        convert2String<quint32>(pData, uSize, strRes, "%08X ");
    }else if(strFormat == "hex64"){
        convert2String<quint64>(pData, uSize, strRes, "%016X ");
    }else if(strFormat == "uint8"){
        convert2String<quint8>(pData, uSize, strRes, "%u ");
    }else if(strFormat == "uint16"){
        convert2String<quint16>(pData, uSize, strRes, "%u ");
    }else if(strFormat == "uint32"){
        convert2String<quint32>(pData, uSize, strRes, "%u ");
    }else if(strFormat == "uint64"){
        convert2String<quint64>(pData, uSize, strRes, "%u ");
    }else if(strFormat == "binary8"){
        convert2Binary<quint8>(pData, uSize, strRes);
    }else if(strFormat == "binary16"){
        convert2Binary<quint16>(pData, uSize, strRes);
    }else if(strFormat == "binary32"){
        convert2Binary<quint32>(pData, uSize, strRes);
    }else if(strFormat == "binary64"){
        convert2Binary<quint64>(pData, uSize, strRes);
    }else if(strFormat == "float"){
        convert2String<float>(pData, uSize, strRes, "%g ");
    }else if(strFormat == "double"){
        convert2String<double>(pData, uSize, strRes, "%g ");
    }

    if(!strRes.isEmpty()){
        QString strRxTxt = m_textEdits[TE_RX]->toPlainText();
        if(strRxTxt.size() > (20 * 1024)){
            strRxTxt.clear();
        }
        strRxTxt += strRes;
        m_textEdits[TE_RX]->setText(strRxTxt);
        if(!m_scriptProgram.isNull()){
            m_scriptEngine.globalObject().setProperty("_rd", strRes);
            m_scriptEngine.evaluate(m_scriptProgram);
            if(m_scriptEngine.hasUncaughtException()){
                appendLog("Script error:" + m_scriptEngine.uncaughtException().toString());
            }
        }
    }
    m_counters[C_RX] += uSize;
    updateStatusBar();
}

void YMainWindow::autoSend(bool bAuto)
{
    if(bAuto){
        m_counters[C_TX_AUTO] = m_lineEdits[LE_TX_TIMES]->text().toLongLong();
        if(m_counters[C_TX_AUTO] == 0)
            m_counters[C_TX_AUTO] = ~0;
        m_timerIDs[T_TX_AUTO] = startTimer(m_lineEdits[LE_TX_INTERVAL]->text().toInt());
        m_lineEdits[LE_TX_INTERVAL]->setEnabled(false);
        m_lineEdits[LE_TX_TIMES]->setEnabled(false);
    }else{
        killTimer(m_timerIDs[T_TX_AUTO]);
        m_checkBoxs[CK_TX_AUTO]->setCheckState(Qt::Unchecked);
        m_lineEdits[LE_TX_INTERVAL]->setEnabled(true);
        m_lineEdits[LE_TX_TIMES]->setEnabled(true);
    }
}

void YMainWindow::saveRx()
{
    QString strFileName = QFileDialog::getSaveFileName(this, tr("Save"), "rx.txt",
                                                       tr("Received data (*.txt)"));
    if(strFileName.isEmpty())
        return;
    QFile file(strFileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    file.write(m_textEdits[TE_RX]->toPlainText().toUtf8());
    file.close();
}

void YMainWindow::appendLog(const QString &strMsg)
{
    m_textEdits[TE_LOG]->append(strMsg);
}

void YMainWindow::about()
{
    QMessageBox::about(
                this,
                tr("About YYDebug"),
                tr("YYDebug 1.0.0.0\nCopyrights 2015-2016 MrY. All rights reserved.\n"
                   "E-mail: mryboy@gmail.com"));
}

void YMainWindow::updateRecentProjectMenu()
{
    auto lstActions = m_menus[M_RECENT]->actions();
    foreach (QAction* pAct, lstActions) {
        delete pAct;
    }
    QAction* pItem;
    QStringList lstPro;
    QSettings setting;
    lstPro = setting.value("recent_projects").toString().split(',');
    foreach (QString strPro, lstPro) {
        if(strPro.isEmpty())
            continue;
        pItem = m_menus[M_RECENT]->addAction(strPro);
        connect(pItem, SIGNAL(triggered()), SLOT(triggeredRecentProject()));
    }
}

void YMainWindow::triggeredRecentProject()
{
    QAction* pAct = qobject_cast<QAction*>(sender());
    if(pAct == nullptr)
        return;
    openProject(pAct->text());
}

void YMainWindow::scriptTxtChanged()
{
    QString strCode = m_textEdits[TE_RX_SCRIPT]->toPlainText();
    QScriptSyntaxCheckResult sscr = QScriptEngine::checkSyntax(strCode);
    if(sscr.state() != QScriptSyntaxCheckResult::Error){
        m_scriptProgram = QScriptProgram(strCode);
        m_textEdits[TE_LOG]->clear();
    }else{
        if(!m_scriptProgram.isNull()){
            m_scriptProgram = QScriptProgram();
        }
        m_textEdits[TE_LOG]->setText(tr("Script error:%1").arg(sscr.errorMessage()));
    }
}

void YMainWindow::addTxCounter(int uSize)
{
    if(uSize < 1)
        return;
    m_counters[C_TX] += (quint64)uSize;
    updateStatusBar();
}
