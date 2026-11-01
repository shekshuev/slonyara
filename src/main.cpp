#include "ui/windows/introwindow.h"
#include "version.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QLocale>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setApplicationVersion(QString::fromLatin1(Version::STRING));

    QCoreApplication::setOrganizationName("Slonyara");
    QCoreApplication::setOrganizationDomain("slonyara.app");
    QCoreApplication::setApplicationName("Slonyara");

    QFontDatabase::addApplicationFont(":/fonts/Manrope-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Manrope-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Xolonium-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Regular.ttf");

    QFile styleFile(":/styles/theme.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        a.setStyleSheet(styleFile.readAll());
    }

    QGuiApplication::styleHints()->unsetColorScheme();

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Slonyara_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

#if defined(Q_OS_WIN)
    QString winConfigDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, winConfigDir);
    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif

    IntroWindow w;
    w.show();
    return QApplication::exec();
}
