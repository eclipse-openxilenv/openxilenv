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

bool IsDarkModeActive()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    Qt::ColorScheme Scheme = QGuiApplication::styleHints()->colorScheme();
    return Scheme == Qt::ColorScheme::Dark;
#else
    return QGuiApplication::palette().color(QPalette::Window).value() < 128;
#endif
}

void SetDarkMode(bool par_DarkMode)
{
    QApplication *App = static_cast<QApplication*>(QApplication::instance());
    if (App == nullptr) return;

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // native Qt dark mode support
    QGuiApplication::styleHints()->setColorScheme(par_DarkMode ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);

    App->setStyle(QStyleFactory::create("Fusion"));
    App->setPalette(App->style()->standardPalette());

    if (par_DarkMode) {
        // Check whether the platform generated an actual dark palette.
        // On Linux/xcb the platform has no dark colors to provide, so the
        // palette stays light even after setColorScheme(Dark); apply one
        // explicitly in that case.
        const bool PaletteIsDark = App->palette().color(QPalette::Window).value() < 128;
        if (!PaletteIsDark) {
            ApplyDarkPalette(App);
        }
    }
#else
    // Qt < 6.8 has no public setColorScheme(): emulate it with the Fusion style and an explicit palette.
    static const QString OriginalStyleName = App->style()->objectName();
    static const QPalette OriginalPalette = App->palette();
    if (par_DarkMode) {
        App->setStyle(QStyleFactory::create("Fusion"));
        ApplyDarkPalette(App);
    } else {
        QStyle *OriginalStyle = QStyleFactory::create(OriginalStyleName);
        if (OriginalStyle != nullptr) {
            App->setStyle(OriginalStyle);
        }
        App->setPalette(OriginalPalette);
    }
#endif
}
