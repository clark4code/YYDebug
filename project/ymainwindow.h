#ifndef YMAINWINDOW_H
#define YMAINWINDOW_H

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QScriptEngine>
#include <QScriptEngine>
#include <QTextEdit>

class SObject;
class YCommDock;

class YMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    struct CmdItemT{
        QLineEdit* name;
        QLineEdit* value;
        QPushButton* send;
    };

    enum ActionT{A_NEW, A_OPEN, A_SAVE, A_SAVE_AS, A_CLOSE, A_EXIT, A_ABOUT, A_LENGTH};
    enum TextEditT{TE_RX, TE_TX,TE_RX_SCRIPT, TE_LOG, TE_LENGTH};
    enum PushButtonT{PB_RX_CLEAR, PB_RX_SAVE, PB_TX_CLEAR, PB_TX_SEND, PB_TX_SEL_FILE, PB_TX_SEND_FILE, PB_LENGTH};
    enum ComboBoxT{/*CB_RX_WORD_LEN,*/ CB_RX_FORMAT, /*CB_TX_WORD_LEN,*/ CB_TX_FORMAT, CB_LENGTH};
    enum CheckBoxT{CK_RX_EXT, CK_TX_AUTO, CK_TX_EXT, CK_TX_EXT_AUTO, CK_LENGTH};
    enum LineEditT{LE_TX_INTERVAL, LE_TX_TIMES, LE_TX_FILE_NAME, LE_TX_EXT_ITEM_INTERVAL
                   , LE_TX_EXT_FRAME_INTERVAL, LE_LENGTH};
    enum LabelT{L_COMM_STATUS, L_DATA_STATS, L_LENGTH};
    enum WidgetT{W_MAIN_SPLITTER, W_RX, W_RX_EXT, W_TX, W_TX_EXT, W_LENGTH};
    enum CounterT{C_RX, C_TX, C_TX_AUTO, C_LENGTH};
    enum TimerT{T_TX_AUTO, T_LENGTH};
    enum MenuT{M_RECENT, M_LENGTH};

    YMainWindow(QWidget *parent = 0);
    ~YMainWindow();

    QLabel* label(LabelT id);

    bool openProject(SObject* pObj);
    int sendData(const QString& strValue, const QString& strFormat);
    static QScriptValue jsLog(QScriptContext *context, QScriptEngine *engine, void* pArgs);
protected:
    void timerEvent(QTimerEvent *evt);
private:
    void createAction();
    void createMenuBar();
    void createCentral();
    void createStatusBar();
    QWidget* createSendExt(QWidget* parent);
    QWidget *createRecvExt(QWidget* parent);
public slots:
    void newProject();
    void openProject(const QString& strFileName = "");
    void closeProject();
    void saveProject();
    void saveProjectAs();
    void selectSaveFile();
    void sendTxValue();
    void updateStatusBar();
    void clearRxCounter();
    void clearTxCounter();
    void updateProjectObject();
    void receivedData(const uchar* pData, uint uSize);
    void autoSend(bool bAuto);
    void saveRx();
    void appendLog(const QString& strMsg);
    void about();
    // update recent projects menu
    void updateRecentProjectMenu();
    // triggered a recent project item
    void triggeredRecentProject();
    void scriptTxtChanged();
    void addTxCounter(int uSize);
private:
    QAction* m_actions[A_LENGTH] = {0};
    QTextEdit* m_textEdits[TE_LENGTH] = {0};
    QPushButton* m_buttons[PB_LENGTH] = {0};
    QComboBox* m_comboBoxs[CB_LENGTH] = {0};
    QCheckBox* m_checkBoxs[CK_LENGTH] = {0};
    QLineEdit* m_lineEdits[LE_LENGTH] = {0};
    QLabel* m_labels[L_LENGTH] = {0};
    QWidget* m_mainWidget[W_LENGTH] = {0};
    QMenu* m_menus[M_LENGTH] = {0};
    CmdItemT m_cmdItems[20];
    SObject* m_projectObj = NULL;
    YCommDock* m_commDock = NULL;
    quint64 m_counters[C_LENGTH] = {0};
    int m_timerIDs[T_LENGTH] = {0};
    QScriptEngine m_scriptEngine;
    QScriptProgram m_scriptProgram;
};

#endif // YMAINWINDOW_H
