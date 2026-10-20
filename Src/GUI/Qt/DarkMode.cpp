#include "DarkMode.h"

#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>

static void ApplyDarkPalette(QApplication *App)
{
    QPalette DarkPalette;
    QColor BaseColor(15, 15, 15);
    QColor DarkColor(40, 40, 40);
    QColor DisabledColor(127, 127, 127);
    QColor MdiBackground(35, 35, 35);
    QColor LinkColor(42, 130, 218);
    QColor HighlightColor(42, 130, 218);

    DarkPalette.setColor(QPalette::Dark,            MdiBackground);
    DarkPalette.setColor(QPalette::Link,            LinkColor);
    DarkPalette.setColor(QPalette::Window,          DarkColor);
    DarkPalette.setColor(QPalette::WindowText,      Qt::white);
    DarkPalette.setColor(QPalette::Base,            BaseColor);
    DarkPalette.setColor(QPalette::AlternateBase,   DarkColor);
    DarkPalette.setColor(QPalette::ToolTipBase,     Qt::black);
    DarkPalette.setColor(QPalette::ToolTipText,     Qt::white);
    DarkPalette.setColor(QPalette::Text,            Qt::white);
    DarkPalette.setColor(QPalette::Disabled, QPalette::Text,            DisabledColor);
    DarkPalette.setColor(QPalette::Disabled, QPalette::ButtonText,      DisabledColor);
    DarkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, DisabledColor);
    DarkPalette.setColor(QPalette::Button,          DarkColor);
    DarkPalette.setColor(QPalette::ButtonText,      Qt::white);
    DarkPalette.setColor(QPalette::BrightText,      Qt::red);
    DarkPalette.setColor(QPalette::Highlight,       HighlightColor);
    DarkPalette.setColor(QPalette::HighlightedText, Qt::black);
    App->setPalette(DarkPalette);
}

// Fusion's standardPalette() follows the system color scheme, so a light palette must be set explicitly.
static void ApplyLightPalette(QApplication *App)
{
    QPalette LightPalette(QColor(239, 239, 239));
    QColor DisabledColor(120, 120, 120);

    LightPalette.setColor(QPalette::Window,          QColor(239, 239, 239));
    LightPalette.setColor(QPalette::WindowText,      Qt::black);
    LightPalette.setColor(QPalette::Base,            Qt::white);
    LightPalette.setColor(QPalette::AlternateBase,   QColor(247, 247, 247));
    LightPalette.setColor(QPalette::ToolTipBase,     QColor(255, 255, 220));
    LightPalette.setColor(QPalette::ToolTipText,     Qt::black);
    LightPalette.setColor(QPalette::Text,            Qt::black);
    LightPalette.setColor(QPalette::Button,          QColor(239, 239, 239));
    LightPalette.setColor(QPalette::ButtonText,      Qt::black);
    LightPalette.setColor(QPalette::BrightText,      Qt::white);
    LightPalette.setColor(QPalette::Link,            QColor(0, 0, 255));
    LightPalette.setColor(QPalette::Highlight,       QColor(48, 140, 198));
    LightPalette.setColor(QPalette::HighlightedText, Qt::white);
    LightPalette.setColor(QPalette::Disabled, QPalette::Text,       DisabledColor);
    LightPalette.setColor(QPalette::Disabled, QPalette::ButtonText, DisabledColor);
    LightPalette.setColor(QPalette::Disabled, QPalette::WindowText, DisabledColor);
    App->setPalette(LightPalette);
}

bool IsDarkModeActive()
{
    bool isDarkMode = QGuiApplication::palette().color(QPalette::Window).value() < 128;
    return isDarkMode;
}

void SetDarkMode(bool par_DarkMode)
{
    QApplication *App = static_cast<QApplication*>(QApplication::instance());
    if (App == nullptr) return;

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // native Qt dark mode support
    QGuiApplication::styleHints()->setColorScheme(par_DarkMode ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
#endif

    // The native styles (e.g. on Windows) ignore the palette, and Fusion's standardPalette()
    // follows the system color scheme. Use Fusion with an explicit palette for a reliable result.
    App->setStyle(QStyleFactory::create("Fusion"));
    if (par_DarkMode && !IsDarkModeActive()) {
        ApplyDarkPalette(App);
    } else if (!par_DarkMode && IsDarkModeActive()) {
        ApplyLightPalette(App);
    }
}
