#include "ymainwindow.h"
#include <QApplication>
#include <QDir>
#include <QSettings>

int main(int argc, char *argv[])
{
    // QDir::setCurrent(QApplication::applicationDirPath());
    QCoreApplication::setOrganizationName("freeman");
    QCoreApplication::setApplicationName("yydebuf");
    QSettings::setDefaultFormat(QSettings::NativeFormat);
    QApplication a(argc, argv);
    QDir::setCurrent(QApplication::applicationDirPath());
    YMainWindow w;
    w.show();
    if(argc > 1)
        w.openProject(argv[1]);
    else{
        QStringList lstPro;
        QSettings setting;
        lstPro = setting.value("recent_projects").toString().split(',');
        if(!lstPro.isEmpty()
                && !lstPro.first().isEmpty())
            w.openProject(lstPro.first());
    }
    return a.exec();
}
