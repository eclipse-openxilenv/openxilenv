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


#ifndef A2LCALMAP3DVIEW_H
#define A2LCALMAP3DVIEW_H

#include <QWidget>

#include "A2LCalMapData.h"

class A2LCalMap3DView : public QWidget
{
    Q_OBJECT
public:
    A2LCalMap3DView(A2LCalMapData *par_Data, QWidget *parent = nullptr);
    ~A2LCalMap3DView() Q_DECL_OVERRIDE;

    void init_trans3d (void);
    void init_trans2d (void);

    void update_sin_cos ();

    void add_x_offset (double par_Offset);
    void add_y_offset (double par_Offset);
    void add_z_offset (double par_Offset);

    void scale_factor (double par_Factor);

    void rotate_x_angle (double par_Angle);
    void rotate_y_angle (double par_Angle);
    void rotate_z_angle (double par_Angle);

    double get_x_angle (void);
    double get_y_angle (void);
    double get_z_angle (void);

    double get_x_offset (void);
    double get_y_offset (void);
    double get_z_offset (void);

    double get_depth (void);
    double get_scale (void);

    void set_x_angle (double par_Angle);
    void set_y_angle (double par_Angle);
    void set_z_angle (double par_Angle);

    void set_x_offset (double par_Offset);
    void set_y_offset (double par_Offset);
    void set_z_offset (double par_Offset);

    void set_depth (double par_Depth);
    void set_scale (double par_Scale);

signals:
    void ChangeValueByWheel(int delta);
    void SelectionChanged();

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

private:

    typedef struct {
        double ax; // = 0.0;
        double sin_ax;
        double cos_ax;
        double ay; // = 0.0;
        double sin_ay;
        double cos_ay;
        double az; // = -M_PI; //0.0;
        double sin_az;
        double cos_az;
        double k_depth; // = 0.5;
        double scale; // = 1.0;
        double x_off; // = 0.4;
        double y_off; // = 0.0;
        double z_off; // = -0.3;
    } TRANS3D;

    typedef struct {
        double x;
        double y;
        double z;
    } POINT3D;

    typedef struct {
        int x;
        int y;
    } POINT2D;

    static const POINT3D xaxis;
    static const POINT3D yaxis;
    static const POINT3D zaxis;
    static const POINT3D nullp;


    A2LCalMapData *m_Data;

    double RedrawMap (QPainter *Painter, int xm, int ym, int xpick, int ypick);
    void paint_axis (QPainter *Painter, int xm, int ym, TRANS3D *trans3d, int axis_flag);

    void update_sin_cos (TRANS3D *trans3d);
    void transformate3d_2d (POINT3D *point3d, POINT2D *point2d, int xdim, int ydim, TRANS3D *trans3d);
    void rotate3d (POINT3D *point, TRANS3D *trans3d);
    void scale3d (POINT3D *point, TRANS3D *trans3d);
    void move3d (POINT3D *point, TRANS3D *trans3d);
    TRANS3D trans3d;

    void axis_txt (QPainter *Painter, int xp, int yp, const char *input, const char *unit);
    void axis_pointer (QPainter *Painter, int xm, int ym, const POINT3D *base,
                       TRANS3D *trans3d, int dir);


    void LineTo (QPainter *Painter, int x, int y);
    void MoveToEx (int x, int y, POINT2D *p);
    int m_x;
    int m_y;

    int PickingMap (int xm, int ym, int xpos, int ypos, int add);

};

#endif // A2LCALMAP3DVIEW_H
