/*
 * Copyright 2023 ZF Friedrichshafen AG
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef KNOBORTACHOWIDGET_H
#define KNOBORTACHOWIDGET_H

#include "MdiWindowWidget.h"
#include "BlackboardObserver.h"
#include "QLabel"
#include "QWidget"

#include <QPoint>
#include <QAction>
#include <QVBoxLayout>

class KnobOrTachoWidget : public MdiWindowWidget {
    Q_OBJECT
public:
    enum KnobOrTachoType {KnobType, TachoType, SliderType, BargraphType};
    enum BitmapWindowSizeBehaviour {FixedWindowSize = 0, DontScaleBitmapNorFixedWindowSize, ScaleBitMap};

    KnobOrTachoWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type,
                      enum KnobOrTachoType par_KnobOrTachoType, QWidget *parent = nullptr);
    ~KnobOrTachoWidget() Q_DECL_OVERRIDE;

    virtual bool writeToIni() Q_DECL_OVERRIDE;
    virtual bool readFromIni() Q_DECL_OVERRIDE;
    virtual CustomDialogFrame* dialogSettings(QWidget *arg_parent) Q_DECL_OVERRIDE;

    enum KnobOrTachoType GetType();

    // Basic
    bool getUseBlackboardConfig();
    void setUseBlackboardConfig(bool arg_useConfig);
    double getMin();
    void setMin(double arg_min);
    double getMax();
    void setMax(double arg_max);
    char isPhys();
    void setPhys(char arg_isPhys);
    QColor getColor();
    void setColor(QColor par_Color);
    QFont getFont();
    void setFont(QFont par_Font);

    // Scale
    int getScaleFlag();
    void setScaleFlag(int arg_showScale);
    double getMaxAngle();
    void setMaxAngle(double arg_maxAngle);
    double getMinAngle();
    void setMinAngle(double arg_minAngle);
    bool getScaleStepFlag();
    void setScaleStepFlag(bool arg_stepFlag);
    double getScaleStep();
    void setScaleStep(double arg_step);
    double getScalePosX();
    void setScalePosX(double arg_posX);
    double getScalePosY();
    void setScalePosY(double arg_posY);
    bool getScaleSizeFlag();
    void setScaleSizeFlag(bool arg_sizeFlag);
    double getScaleSize();
    void setScaleSize(double arg_length);
    double scaleStepSize();
    void setScaleColorFlag(bool arg_colorFlag);
    bool getScaleColorFlag();
    void setScaleColor(QColor arg_color);
    QColor getScaleColor();

    // Knob or Needle
    int getNeedleFlag();
    void setNeedleFlag(int arg_showNeedle);
    double getNeedlePosX();
    void setNeedlePosX(double arg_posX);
    double getNeedlePosY();
    void setNeedlePosY(double arg_posY);
    bool getNeedleSizeFlag();
    void setNeedleSizeFlag(bool arg_SizeFlag);

    double getNeedleLength();
    void setNeedleLength(double arg_length);
    bool getNeedleLineColorFlag();
    void setNeedleLineColorFlag(bool arg_colorFlag);
    QColor getNeedleLineColor();
    void setNeedleLineColor(QColor arg_color);
    bool getNeedleFillColorFlag();
    void setNeedleFillColorFlag(bool arg_colorFlag);
    QColor getNeedleFillColor();
    void setNeedleFillColor(QColor arg_color);

    // Value
    int getTextFlag();
    void setTextFlag(int arg_showText);
    double getTextPosX();
    void setTextPosX(double arg_posX);
    double getTextPosY();
    void setTextPosY(double arg_posY);
    bool getTextFontFlag();
    void setTextFontFlag(bool par_FontFlag);
    QFont getTextFont();
    void setTextFont(QFont &par_Font);
    int getTextAlign();
    void SetTextAlign(int par_Align);

    // Name
    int getNameFlag();
    void setNameFlag(int arg_showName);
    double getNamePosX();
    void setNamePosX(double arg_posX);
    double getNamePosY();
    void setNamePosY(double arg_posY);
    bool getNameFontFlag();
    void setNameFontFlag(bool par_FontFlag);
    QFont getNameFont();
    void setNameFont(QFont &par_Font);
    int getNameAlign();
    void SetNameAlign(int par_Align);

    // Bitmap
    int getBitmapFlag();
    void setBitmapFlag(int arg_showBitmap);
    QString getBitmapPath();
    void setBitmapPath(QString arg_path);
    BitmapWindowSizeBehaviour getBitmapSizeBehaviour();
    void setBitmapSizeBehaviour(BitmapWindowSizeBehaviour arg_behaviour);

    // If release fallback to
    bool getIfReleaseFallbackToFlag();
    void setIfReleaseFallbackToFlag(bool arg_Flag);
    double getIfReleaseFallbackToValue();
    void setIfReleaseFallbackToValue(double arg_Value);

    // Drag status
    bool getDragStatusFlag();
    void setDragStatusFlag(bool arg_Flag);
    QString getDragStatusSignal();
    void setDragStatusSignal(QString arg_Name);

    void CopyAttributesFrom(KnobOrTachoWidget *par_From);
    void CheckChangeOfDragStatusSignal(bool par_DragStatusFlagOld, QString par_DragStatusNameOld);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *arg_event) Q_DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;

private slots:
    virtual void CyclicUpdate() Q_DECL_OVERRIDE;
    void blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag);
    void changeBlackboardValue(double arg_value);
    virtual void changeColor(QColor arg_color) Q_DECL_OVERRIDE;
    virtual void changeFont(QFont arg_font) Q_DECL_OVERRIDE;
    virtual void changeWindowName(QString arg_name) Q_DECL_OVERRIDE;
    virtual void changeVariable(QString arg_variable, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void changeVaraibles(QStringList arg_variables, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void resetDefaultVariables(QStringList arg_variables) Q_DECL_OVERRIDE;
    void setBBConfigDirect(bool arg_value);
    void setVariablePhysical(bool arg_value);
    void setBitmapVisible(bool arg_value);
    void ConfigureSlot();

private:
    QString SearchAndReplaceEnvironmentVariables(QString &par_String);
    QColor getTextColorForBackground(const QColor &arg_color) const;
    double minArcAngleForDial(const double arg_minArcIni) const;
    double maxArcAngleForDial(const double arg_minArcIni, const double arg_maxArcIni) const;
    double minArcAngleForIni(const double arg_minArcDial) const;
    double maxArcAngleForIni(const double arg_minArcDial, const double arg_maxArcDial) const;
    void paintEvent(QPaintEvent *arg_event) Q_DECL_OVERRIDE;
    void openDialog() Q_DECL_OVERRIDE;

    void UpdateNameAndValue();

    void ReadValueFromBlackboard();

    void ResetWindowSize();

    QRect CalcSliderRect(int par_Width, int par_Height);
    void CheckBitmapWindowSizeBehaviour();
    void ChangeLayout();
    void LoadBackgroundPixmap();

    enum KnobOrTachoType m_KnobOrTachoType;
    bool m_FirstUpdate;
    QSize m_Size;
    int m_ConversionType;
    bool m_DisplayUnit;
    int m_DataType;
    double m_DragValue;
    int m_VidLastUpdate;

    class KnobAttributes {
    public:
        KnobAttributes();
        void writeToIni(QString &par_SectionPath, KnobOrTachoType par_KnobOrTachoType);
        void readFromIni(QString &ar_SectionPath, KnobOrTachoType par_KnobOrTachoType);

        bool m_Physical;
        bool m_BBCfgDirectFlag;
        double m_LowerBound;
        double m_UpperBound;
        QFont m_Font;
        QColor m_Color;

        // Scale
        bool m_ScaleFlag;
        double m_MinAngle;
        double m_MaxAngle;
        bool m_ScaleStepFlag;
        double m_ScaleStep;
        double m_ScalePosX;
        double m_ScalePosY;
        bool m_ScaleSizeFlag;
        double m_ScaleSize;
        bool m_ScaleColorFlag;
        QColor m_ScaleColor;

        // Knob
        bool m_NeedleFlag;
        double m_NeedlePosX;
        double m_NeedlePosY;
        bool m_NeedleSizeFlag;
        double m_NeedleSize;
        bool m_NeedleLineColorFlag;
        QColor m_NeedleLineColor;
        bool m_NeedleFillColorFlag;
        QColor m_NeedleFillColor;

        // Value
        bool m_TextFlag;
        double m_TextPosX;
        double m_TextPosY;
        bool m_TextFontFlag;
        QFont m_TextFont;
        int m_TextAlign;

        // Name
        bool m_NameFlag;
        int m_NamePosX;
        int m_NamePosY;
        bool m_NameFontFlag;
        QFont m_NameFont;
        int m_NameAlign;

        // Bitmap
        bool m_BitmapFlag;
        QString m_bitmap;
        BitmapWindowSizeBehaviour m_BitmapWindowSizeBehaviour;

        // If release fallback to
        bool m_IfReleaseFallbackFlag;
        double m_IfReleaseFallbackValue;

        // Drag status
        bool m_DragStateVarFlag;
        QString m_DragStateVariableName;
    };

    KnobAttributes m_Attributes;
    KnobAttributes m_SavedAttributes;
    QSize m_WindowSizeStored;

    class DrawArea : public QWidget {
    public:
        DrawArea(enum KnobOrTachoType par_KnobOrTachoType, KnobAttributes *par_Attributes, QWidget* parent = nullptr);
        double PaintRoundScale(QPainter &Painter, int par_Width, int par_Height);
        void PaintKnob(QPainter &Painter, int par_Width, int par_Height, double par_Radius);
        void PaintNeedle(QPainter &Painter, int par_Width, int par_Height, double par_Radius);
        void PaintLinearSlopeScale(QPainter &Painter, int par_Width, int par_Height);
        void PaintSliderKnob(QPainter &Painter, int par_Width, int par_Height);
        void PaintSliderLine(QPainter &Painter, int par_Width, int par_Height);
        void PaintBargraph(QPainter &Painter, int par_Width, int par_Height);
        void SetVid(int par_Vid);
        int GetVid();
        void SetDragStateVid(int par_Vid);
        int GetDragStateVid();
        double GetCurrentValue();
        void SetCurrentValue(double par_Value);
        void SetVariableName(QString par_Name);
        QString GetVariableName();
        void SetValueString(QString par_Name);
        QString GetValueString();

    protected:
        void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
        void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
        void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    private:
        void paintEvent(QPaintEvent *arg_event) Q_DECL_OVERRIDE;
        void WriteSliderValue(double arg_Value);
        int CalcSliderLen(int par_Height);

        enum KnobOrTachoType m_KnobOrTachoType;
        KnobAttributes *m_Attributes;
        int maxLabelSize;
        int maxLabelSizey;
        int m_FontSizeDyn;
        QFont m_ScaleFont;

        double m_CurrentValue;
        double m_DragValue;
        int m_Vid;
        int m_DragStateVid;

        QPoint m_MoveStartPoint;
        bool m_MoveFlag;
        QPoint m_startDragPosition;

        bool m_Overwind;
        bool m_OverwindDir;

        int m_LastXpos;
        int m_LastYpos;

        QString m_VariableName;
        QString m_ValueString;
    };

    DrawArea *m_Drawarea;
    QLabel *m_VariableNameLabel;
    QLabel *m_ValueLabel;
    QVBoxLayout *m_Layout;

    bool m_bitmapPathExist;
    bool m_NewPixmapIsLoaded;
    QPixmap m_PixmapOrginalSize;
    QPixmap m_PixmapResized;

    BlackboardObserverConnection m_ObserverConnection;

    QAction *m_ConfigAct;
};

#endif // KNOBORTACHOWIDGET_H
