#include "ui/windows/introwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QStyleHints>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#if defined(Q_OS_WIN)
    QGuiApplication::styleHints()->unsetColorScheme();
#else
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Slonyara_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    IntroWindow w;
    w.show();
    return QApplication::exec();
}
