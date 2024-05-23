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


#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>
#include <QPixmap>

class StatusBar : public QStatusBar
{
     Q_OBJECT
public:
    explicit StatusBar(QWidget *parent = nullptr);
    void SetSpeedPerCent(double par_Speed);
    void SetEnableCar(bool par_Enable);
    bool IsEnableCar();

private slots:
    void MoveCar();

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private:
    void BuildPixmaps();

    QPixmap m_Pixmaps[4];
    enum {MOVING_CAR_IMAGES, MOVING_XMAS_IMAGES} m_ImagesType;
    int m_ImageNo;
    double m_Speed;
    int m_Width;
    int m_Height;
    double m_CurrentPos;
    bool m_CarIsVisable;
    int m_CarHeight;
    int m_CarWidth;
    int m_ImageSwitchCounter;
    bool m_FirstPaint;
};

#endif // STATUSBAR_H
