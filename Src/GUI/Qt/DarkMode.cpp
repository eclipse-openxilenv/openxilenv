#include "DarkMode.h"

#include <QStyleFactory>
#include <QProxyStyle>
#include <QStyleOption>
#include <QPainter>
#include <QPen>
#include <QPalette>

extern "C" {
#include "MainValues.h"
}

static QPalette *SavePaette;

class DarkModeProxyStyle : public QProxyStyle
{

  public:
    void drawPrimitive(PrimitiveElement par_Elem,
                       const QStyleOption *par_Option,
                       QPainter *par_Painter, const QWidget *par_Widget) const override
    {
        switch (par_Elem) {
        case PE_FrameWindow:
           {
               QColor Color1 = QColor(40, 40, 40);
               QColor Color2 = QColor(80, 80, 80);
               QColor Color3 = QColor(20, 20, 20);
               QRect Rect = par_Option->rect;
               int Left = Rect.left();
               int Right = Rect.right();
               int Top = Rect.top();
               int Bottom = Rect.bottom();

               par_Painter->save();
               par_Painter->setPen(QPen(Color1));
               par_Painter->drawRect(Left, Top, Right - Left - 1, Bottom - Top - 1);
               par_Painter->setPen(QPen(Color2));
               par_Painter->drawLine(Left + 1, Top + 1, Left + 1, Bottom - 1);
               par_Painter->setPen(QPen(Color3));
               par_Painter->drawLine(Left + 1, Bottom - 1, Right - 2, Bottom - 1);
               par_Painter->drawLine(Right - 1, Top + 1, Right - 1, Bottom - 1);
               par_Painter->restore();
           }
           break;
        default:
            baseStyle()->drawPrimitive(par_Elem,
                                       par_Option,
                                       par_Painter, par_Widget);
            break;
        }
    }
};

void SetDarkMode(QApplication *par_Application, bool par_DarkMode)
{
    static bool DarkModeCurrent;

    if (DarkModeCurrent != par_DarkMode) {
        DarkModeCurrent = par_DarkMode;
        if (par_DarkMode) {
            if (SavePaette == nullptr) {
                SavePaette = new QPalette(par_Application->palette());
            }
            par_Application->setStyle(QStyleFactory::create("Fusion"));
            QStyle *BaseStyle = QStyleFactory::create("Fusion");
            DarkModeProxyStyle *Style = new DarkModeProxyStyle;
            Style->setBaseStyle(BaseStyle);
            par_Application->setStyle(Style);

            QPalette darkPalette;

            QColor BaseColor = QColor(15, 15, 15);
            QColor DarkColor = QColor(40, 40, 40);
            QColor DisabledColor = QColor(127, 127, 127);
            QColor MdiBackground = QColor(35, 35, 35);
            QColor LinkColor = QColor(42, 130, 218);
            QColor HighlightColor = QColor(42, 130, 218);

            darkPalette.setColor(QPalette::Dark, MdiBackground);
            darkPalette.setColor(QPalette::Link, LinkColor);
            darkPalette.setColor(QPalette::Window, DarkColor);
            darkPalette.setColor(QPalette::WindowText, Qt::white);
            darkPalette.setColor(QPalette::Base, BaseColor);
            darkPalette.setColor(QPalette::AlternateBase, DarkColor);
            darkPalette.setColor(QPalette::ToolTipBase, Qt::black);
            darkPalette.setColor(QPalette::ToolTipText, Qt::white);
            darkPalette.setColor(QPalette::Text, Qt::white);
            darkPalette.setColor(QPalette::Disabled, QPalette::Text, DisabledColor);
            darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, DisabledColor);
            darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, DisabledColor);
            darkPalette.setColor(QPalette::Button, DarkColor);
            darkPalette.setColor(QPalette::ButtonText, Qt::white);
            darkPalette.setColor(QPalette::BrightText, Qt::red);
            darkPalette.setColor(QPalette::Highlight, HighlightColor);
            darkPalette.setColor(QPalette::HighlightedText, Qt::black);
            par_Application->setPalette(darkPalette);
        } else {
            if (SavePaette != nullptr) {
                par_Application->setPalette(*SavePaette);
            }
        }
    }
}
