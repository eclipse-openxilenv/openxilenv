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


#include <math.h>
#include "A2LCalMap3DView.h"

#include <QPainter>
#include <QMouseEvent>

extern "C" {
#include "Config.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
}

#include "GetEventPos.h"

#define X_AXIS_NO 0
#define Y_AXIS_NO 1
#define MAP_NO    2


A2LCalMap3DView::A2LCalMap3DView(A2LCalMapData *par_Data, QWidget *parent) : QWidget(parent)
{
    m_Data = par_Data;
    m_x = 0;
    m_y = 0;
    init_trans3d ();
}

A2LCalMap3DView::~A2LCalMap3DView()
{

}


void A2LCalMap3DView::LineTo (QPainter *Painter, int x, int y)
{
    Painter->drawLine (m_x, m_y, x, y);
    m_x = x;
    m_y = y;
}

void A2LCalMap3DView::MoveToEx (int x, int y, POINT2D *p)
{
    if (p != nullptr) {
        p->x = m_x;
        p->y = m_y;
    }
    m_x = x;
    m_y = y;
}


const A2LCalMap3DView::POINT3D A2LCalMap3DView::xaxis = {1.0, 0.0, 0.0};
const A2LCalMap3DView::POINT3D A2LCalMap3DView::yaxis = {0.0, 1.0, 0.0};
const A2LCalMap3DView::POINT3D A2LCalMap3DView::zaxis = {0.0, 0.0, 1.0};
const A2LCalMap3DView::POINT3D A2LCalMap3DView::nullp = {0.0, 0.0, 0.0};


void A2LCalMap3DView::update_sin_cos (TRANS3D *trans3d)
{
    trans3d->ax -= floor (trans3d->ax / (2*M_PI)) * (2*M_PI);
    trans3d->sin_ax = sin (trans3d->ax);
    trans3d->cos_ax = cos (trans3d->ax);
    trans3d->ay -= floor (trans3d->ay / (2*M_PI)) * (2*M_PI);
    trans3d->sin_ay = sin (trans3d->ay);
    trans3d->cos_ay = cos (trans3d->ay);
    trans3d->az -= floor (trans3d->az / (2*M_PI)) * (2*M_PI);
    trans3d->sin_az = sin (trans3d->az);
    trans3d->cos_az = cos (trans3d->az);
}

void A2LCalMap3DView::init_trans3d (void)
{
    trans3d.ax = 0.0;
    trans3d.ay = 0.0;
    trans3d.az = M_PI;
    update_sin_cos (&trans3d);
    trans3d.k_depth = 0.5;
    trans3d.scale = 1.0;
    trans3d.x_off = 0.4;
    trans3d.y_off = 0.0;
    trans3d.z_off = -0.3;
}

void A2LCalMap3DView::init_trans2d (void)
{
    trans3d.ax = 0.0;
    trans3d.ay = 0.0;
    trans3d.az = M_PI;
    update_sin_cos (&trans3d);
    trans3d.k_depth = 0.5;
    trans3d.scale = 1.6;
    trans3d.x_off = 0.8;
    trans3d.y_off = 0.0;
    trans3d.z_off = -0.8;
}

void A2LCalMap3DView::update_sin_cos()
{
    update_sin_cos(&trans3d);
}

void A2LCalMap3DView::add_x_offset(double par_Offset)
{
    trans3d.x_off += par_Offset;
}

void A2LCalMap3DView::add_y_offset(double par_Offset)
{
    trans3d.y_off += par_Offset;
}

void A2LCalMap3DView::add_z_offset(double par_Offset)
{
    trans3d.z_off += par_Offset;
}

void A2LCalMap3DView::scale_factor(double par_Factor)
{
    trans3d.scale *= par_Factor;
}

void A2LCalMap3DView::rotate_x_angle(double par_Angle)
{
    trans3d.ax += par_Angle;
}

void A2LCalMap3DView::rotate_y_angle(double par_Angle)
{
    trans3d.ay += par_Angle;
}

void A2LCalMap3DView::rotate_z_angle(double par_Angle)
{
    trans3d.az += par_Angle;
}

double A2LCalMap3DView::get_x_angle()
{
    return trans3d.ax;
}

double A2LCalMap3DView::get_y_angle()
{
    return trans3d.ay;
}

double A2LCalMap3DView::get_z_angle()
{
    return trans3d.az;
}

double A2LCalMap3DView::get_x_offset ()
{
    return trans3d.x_off;
}

double A2LCalMap3DView::get_y_offset ()
{
    return trans3d.y_off;
}

double A2LCalMap3DView::get_z_offset ()
{
    return trans3d.z_off;
}

double A2LCalMap3DView::get_depth ()
{
    return trans3d.k_depth;
}

double A2LCalMap3DView::get_scale ()
{
    return trans3d.scale;
}


void A2LCalMap3DView::set_x_angle (double par_Angle)
{
    trans3d.ax = par_Angle;
}

void A2LCalMap3DView::set_y_angle (double par_Angle)
{
    trans3d.ay = par_Angle;
}

void A2LCalMap3DView::set_z_angle (double par_Angle)
{
    trans3d.az = par_Angle;
}

void A2LCalMap3DView::set_x_offset (double par_Offset)
{
    trans3d.x_off = par_Offset;
}

void A2LCalMap3DView::set_y_offset (double par_Offset)
{
    trans3d.y_off = par_Offset;
}

void A2LCalMap3DView::set_z_offset (double par_Offset)
{
    trans3d.z_off = par_Offset;
}

void A2LCalMap3DView::set_depth (double par_Depth)
{
    trans3d.k_depth = par_Depth;
}

void A2LCalMap3DView::set_scale (double par_Scale)
{
    trans3d.scale = par_Scale;
}



void A2LCalMap3DView::transformate3d_2d (POINT3D *point3d, POINT2D *point2d, int xdim, int ydim, TRANS3D *trans3d)
{
    double x2d;
    double y2d;
    double help;

    help = 1.0 + point3d->y * trans3d->k_depth;
    if (help != 0.0) {
        x2d = point3d->x / help;
        y2d = point3d->z / help;
    } else {
        x2d = 100000000000.0;
        y2d = 100000000000.0;
    }
    point2d->x = xdim - static_cast<int>(x2d * xdim);
    point2d->y = ydim - static_cast<int>(y2d * ydim);
}


void A2LCalMap3DView::rotate3d (POINT3D *point, TRANS3D *trans3d)
{
    double sin_a, cos_a;
    POINT3D help;
    // X-Rotation
    sin_a = trans3d->sin_ax;
    cos_a = trans3d->cos_ax;
    help.x = point->x;
    help.y = point->y * cos_a - point->z * sin_a;
    help.z = point->y * sin_a + point->z * cos_a;
    // Y-Rotation
    sin_a = trans3d->sin_ay;
    cos_a = trans3d->cos_ay;
    point->x = help.x * cos_a + help.z * sin_a;
    point->y = help.y;
    point->z = help.z * cos_a - help.x * sin_a;
    // Z-Rotation
    sin_a = trans3d->sin_az;
    cos_a = trans3d->cos_az;
    help.x = point->x * cos_a - point->y * sin_a;
    help.y = point->y * cos_a + point->x * sin_a;
    help.z = point->z;
    *point = help;
}

void A2LCalMap3DView::scale3d (POINT3D *point, TRANS3D *trans3d)
{
    point->x = point->x * trans3d->scale;
    point->y = point->y * trans3d->scale;
    point->z = point->z * trans3d->scale;
}

void A2LCalMap3DView::move3d (POINT3D *point, TRANS3D *trans3d)
{
    point->x = point->x + trans3d->x_off;
    point->y = point->y + trans3d->y_off;
    point->z = point->z + trans3d->z_off;
}

void A2LCalMap3DView::axis_txt (QPainter *Painter, int xp, int yp, const char *input, const char *unit)
{
    char help[BBVARI_NAME_SIZE + BBVARI_UNIT_SIZE + 10];

    if (input != nullptr) {
        STRING_COPY_TO_ARRAY (help, input);
    } else {
        STRING_COPY_TO_ARRAY(help, "no input");
    }
    if ((unit != nullptr) && (strlen(unit))) {
        STRING_APPEND_TO_ARRAY (help, " [");
        STRING_APPEND_TO_ARRAY (help, unit);
        STRING_APPEND_TO_ARRAY (help, "]");
    }

    Painter->drawText (xp, yp, help);
}

#define POINTERWIDTH   0.03
#define POINTERHIGHT   0.1

void A2LCalMap3DView::axis_pointer (QPainter *Painter, int xm, int ym, const POINT3D *base,
                   TRANS3D *trans3d, int dir)
{
    POINT3D a, b, c;
    POINT2D point2d;
    QPoint pp[4];

    c = b = a = *base;
    switch (dir) {
    case 0:  // x
        a.x -= POINTERHIGHT;
        b.x -= POINTERHIGHT;
        a.y -= POINTERWIDTH;
        b.y += POINTERWIDTH;
        break;
    case 1:  // y
        a.y -= POINTERHIGHT;
        b.y -= POINTERHIGHT;
        a.x -= POINTERWIDTH;
        b.x += POINTERWIDTH;
        break;
    case 2:  // z
        a.z -= POINTERHIGHT;
        b.z -= POINTERHIGHT;
        a.y -= POINTERWIDTH;
        b.y += POINTERWIDTH;
        break;
    }
    rotate3d (&a, trans3d);
    scale3d (&a, trans3d);
    move3d (&a, trans3d);
    transformate3d_2d (&a, &point2d, xm, ym, trans3d);
    pp[0].setX (point2d.x);
    pp[0].setY (point2d.y);
    rotate3d (&b, trans3d);
    scale3d (&b, trans3d);
    move3d (&b, trans3d);
    transformate3d_2d (&b, &point2d, xm, ym, trans3d);
    pp[1].setX (point2d.x);
    pp[1].setY (point2d.y);
    rotate3d (&c, trans3d);
    scale3d (&c, trans3d);
    move3d (&c, trans3d);
    transformate3d_2d (&c, &point2d, xm, ym, trans3d);
    pp[2].setX (point2d.x);
    pp[2].setY (point2d.y);
    Painter->drawPolygon (pp, 3);
}

void A2LCalMap3DView::paint_axis (QPainter *Painter, int xm, int ym, TRANS3D *trans3d, int axis_flag)
{
    POINT3D point3d;
    POINT2D point2d;

    QBrush brush_save = Painter->brush();
    QBrush brush = QBrush (QColor (210, 210, 210));
    Painter->setBrush (brush);

    // X axis
    if ((axis_flag & 0x01) == 0x01) {
        point3d = nullp;
        rotate3d (&point3d, trans3d);
        scale3d (&point3d, trans3d);
        move3d (&point3d, trans3d);
        transformate3d_2d (&point3d, &point2d, xm, ym, trans3d);
        MoveToEx (point2d.x, point2d.y, nullptr);
        point3d = xaxis;
        rotate3d (&point3d, trans3d);
        scale3d (&point3d, trans3d);
        move3d (&point3d, trans3d);
        transformate3d_2d (&point3d, &point2d, xm, ym, trans3d);
        LineTo (Painter, point2d.x, point2d.y);
        axis_pointer (Painter, xm, ym, &xaxis, trans3d, 0);

        axis_txt (Painter, point2d.x, point2d.y, m_Data->GetXInput(), m_Data->GetXUnit());
    }
    if (m_Data->GetType() == 2) {
        // Y axis
        if ((axis_flag & 0x02) == 0x02) {
            point3d = nullp;
            rotate3d (&point3d, trans3d);
            scale3d (&point3d, trans3d);
            move3d (&point3d, trans3d);
            transformate3d_2d (&point3d, &point2d, xm, ym, trans3d);
            MoveToEx (point2d.x, point2d.y, nullptr);
            point3d = yaxis;
            rotate3d (&point3d, trans3d);
            scale3d (&point3d, trans3d);
            move3d (&point3d, trans3d);
            transformate3d_2d (&point3d, &point2d, xm, ym, trans3d);
            LineTo (Painter, point2d.x, point2d.y);
            axis_pointer (Painter, xm, ym, &yaxis, trans3d, 1);
            axis_txt (Painter, point2d.x, point2d.y, m_Data->GetYInput(), m_Data->GetYUnit());
        }
    }

    // Z axis
    if ((axis_flag & 0x04) == 0x04) {
        point3d = nullp;
        rotate3d (&point3d, trans3d);
        scale3d (&point3d, trans3d);
        move3d (&point3d, trans3d);
        transformate3d_2d (&point3d, &point2d, xm, ym, trans3d);
        MoveToEx (point2d.x, point2d.y, nullptr);
        point3d = zaxis;
        rotate3d (&point3d, trans3d);
        scale3d (&point3d, trans3d);
        move3d (&point3d, trans3d);
        transformate3d_2d (&point3d, &point2d, xm, ym, trans3d);
        LineTo (Painter, point2d.x, point2d.y);
        axis_pointer (Painter, xm, ym, &zaxis, trans3d, 2);
        axis_txt (Painter, point2d.x, point2d.y, "", "ZUnit");
    }
    Painter->setBrush (brush_save);
}

double A2LCalMap3DView::RedrawMap (QPainter *Painter, int xm, int ym, int xpick, int ypick)
{
    Q_UNUSED(xpick)
    Q_UNUSED(ypick)
    int x, y;
    POINT3D point3d;
    POINT2D point2d;
    POINT2D old_pos;
    QPoint pp[4];
    double value = 0.0;
    int xstart, xend, dx;
    int ystart, yend, dy;
    int axis_mask;
    double zxa, zya, zza, z0p;

    int XDim = m_Data->GetXDim();
    int YDim = m_Data->GetYDim();

    // Find the order to paint
    // This is only necessary for maps
    if ((m_Data->GetType() == 2) && ((XDim >= 2) || (YDim >= 2))) {
        // Calculate the depth of the axis
        point3d = xaxis;
        rotate3d (&point3d, &trans3d);
        scale3d (&point3d, &trans3d);
        move3d (&point3d, &trans3d);
        zxa = point3d.y;
        point3d = yaxis;
        rotate3d (&point3d, &trans3d);
        scale3d (&point3d, &trans3d);
        move3d (&point3d, &trans3d);
        zya = point3d.y;
        point3d = zaxis;
        rotate3d (&point3d, &trans3d);
        scale3d (&point3d, &trans3d);
        move3d (&point3d, &trans3d);
        zza = point3d.y;
        point3d = nullp;
        rotate3d (&point3d, &trans3d);
        scale3d (&point3d, &trans3d);
        move3d (&point3d, &trans3d);
        z0p = point3d.y;

        // Calculate the depth of the 4 corner points
        double depth1, depth2, depth3, depth4;
        point3d.x = m_Data->GetValueNorm (X_AXIS_NO, 0, 0);
        point3d.y = m_Data->GetValueNorm (Y_AXIS_NO, 0, 0);
        point3d.z = m_Data->GetValueNorm (MAP_NO, 0, 0);
        rotate3d (&point3d, &trans3d);
        scale3d (&point3d, &trans3d);
        move3d (&point3d, &trans3d);
        depth1 = point3d.y;
        point3d.x = m_Data->GetValueNorm (X_AXIS_NO, 0, 0);
        point3d.y = m_Data->GetValueNorm (Y_AXIS_NO, 0, YDim-1);
        point3d.z = m_Data->GetValueNorm (MAP_NO, 0, YDim-1);
        rotate3d (&point3d, &trans3d);
        scale3d (&point3d, &trans3d);
        move3d (&point3d, &trans3d);
        depth2 = point3d.y;
        point3d.x = m_Data->GetValueNorm (X_AXIS_NO, XDim-1, 0);
        point3d.y = m_Data->GetValueNorm (Y_AXIS_NO, YDim-1, 0);
        point3d.z = m_Data->GetValueNorm (MAP_NO, XDim-1, YDim-1);
        rotate3d (&point3d, &trans3d);
        scale3d (&point3d, &trans3d);
        move3d (&point3d, &trans3d);
        depth3 = point3d.y;
        point3d.x = m_Data->GetValueNorm (X_AXIS_NO, XDim-1, 0);
        point3d.y = m_Data->GetValueNorm (Y_AXIS_NO, 0, 0);
        point3d.z = m_Data->GetValueNorm (MAP_NO, XDim-1, 0);
        rotate3d (&point3d, &trans3d);
        scale3d (&point3d, &trans3d);
        move3d (&point3d, &trans3d);
        depth4 = point3d.y;

        // Search the corner nearest to the viewer. The remotely corner must be paint first.
        // Check if the axis are behinde the map
        axis_mask = 0x00;
        if (depth1 > depth2) {
            if (depth1 > depth3) {
                if (depth1 > depth4) {
                    xstart = 0; xend = XDim-1; dx = +1; ystart = 0; yend = YDim-1; dy = +1;     // depth1 is the remotely corner
                    if ((depth1 < zxa) || (depth1 < z0p)) axis_mask |= 0x01;
                    if ((depth1 < zya) || (depth1 < z0p)) axis_mask |= 0x02;
                    if ((depth1 < zza) || (depth1 < z0p)) axis_mask |= 0x04;
                } else {
                    xstart = XDim-2; xend = -1; dx = -1; ystart = 0; yend = YDim-1; dy = +1;     // depth4 is the remotely corner
                    if ((depth4 < zxa) || (depth4 < z0p)) axis_mask |= 0x01;
                    if ((depth4 < zya) || (depth4 < z0p)) axis_mask |= 0x02;
                    if ((depth4 < zza) || (depth4 < z0p)) axis_mask |= 0x04;
                }
            } else {
                if (depth2 > depth4) {
                    xstart = 0; xend = XDim-1; dx = +1; ystart = YDim-2; yend = -1; dy = -1;     // depth2 is the remotely corner
                    if ((depth2 < zxa) || (depth2 < z0p)) axis_mask |= 0x01;
                    if ((depth2 < zya) || (depth2 < z0p)) axis_mask |= 0x02;
                    if ((depth2 < zza) || (depth2 < z0p)) axis_mask |= 0x04;
                } else {
                    xstart = XDim-2; xend = -1; dx = -1; ystart = 0; yend = YDim-1; dy = +1;     // depth4 is the remotely corner
                    if ((depth4 < zxa) || (depth4 < z0p)) axis_mask |= 0x01;
                    if ((depth4 < zya) || (depth4 < z0p)) axis_mask |= 0x02;
                    if ((depth4 < zza) || (depth4 < z0p)) axis_mask |= 0x04;
                }
            }
        } else {
            if (depth2 > depth3) {
                if (depth2 > depth4) {
                    xstart = 0; xend = XDim-1; dx = +1; ystart = YDim-2; yend = -1; dy = -1;     // depth2 is the remotely corner
                    if ((depth2 < zxa) || (depth2 < z0p)) axis_mask |= 0x01;
                    if ((depth2 < zya) || (depth2 < z0p)) axis_mask |= 0x02;
                    if ((depth2 < zza) || (depth2 < z0p)) axis_mask |= 0x04;
                } else {
                    xstart = XDim-2; xend = -1; dx = -1; ystart = 0; yend = YDim-1; dy = +1;     // depth4 is the remotely corner
                    if ((depth4 < zxa) || (depth4 < z0p)) axis_mask |= 0x01;
                    if ((depth4 < zya) || (depth4 < z0p)) axis_mask |= 0x02;
                    if ((depth4 < zza) || (depth4 < z0p)) axis_mask |= 0x04;
                }
            } else {
                if (depth3 > depth4) {
                    xstart = XDim-2; xend = -1; dx = -1; ystart = YDim-2; yend = -1; dy = -1;     // depth3 is the remotely corner
                    if ((depth3 < zxa) || (depth3 < z0p)) axis_mask |= 0x01;
                    if ((depth3 < zya) || (depth3 < z0p)) axis_mask |= 0x02;
                    if ((depth3 < zza) || (depth3 < z0p)) axis_mask |= 0x04;
                } else {
                    xstart = XDim-2; xend = -1; dx = -1; ystart = 0; yend = YDim-1; dy = +1;     // depth4 is the remotely corner
                    if ((depth4 < zxa) || (depth4 < z0p)) axis_mask |= 0x01;
                    if ((depth4 < zya) || (depth4 < z0p)) axis_mask |= 0x02;
                    if ((depth4 < zza) || (depth4 < z0p)) axis_mask |= 0x04;
                }
            }
        }
    } else {
        xstart = 0; xend = XDim-1; dx = +1;
        ystart = 0; yend = YDim-1; dy = +1;
        axis_mask = 0xFF;
    }
    // Paint the axis behind the map
    paint_axis (Painter, xm, ym, &trans3d, axis_mask);

    // Paint the map
    if (m_Data->GetType() == 2) {
        QBrush brush = QBrush (QColor (111, 210, 143));
        QBrush brush_save = Painter->brush();
        Painter->setBrush (brush);

        // Paint green surface, line and points from behind to front
        for (x = xstart; x != xend; x += dx)  {
            for (y = ystart; y != yend; y += dy) {
                point3d.x = m_Data->GetValueNorm (X_AXIS_NO, x, 0);
                point3d.y = m_Data->GetValueNorm (Y_AXIS_NO, y, 0);
                point3d.z = m_Data->GetValueNorm (MAP_NO, x, y);
                rotate3d (&point3d, &trans3d);
                scale3d (&point3d, &trans3d);
                move3d (&point3d, &trans3d);
                transformate3d_2d (&point3d, &point2d, xm, ym, &trans3d);
                pp[0].setX (point2d.x);
                pp[0].setY (point2d.y);

                point3d.x = m_Data->GetValueNorm (X_AXIS_NO, x+1, 0);
                point3d.y = m_Data->GetValueNorm (Y_AXIS_NO, y, 0);
                point3d.z = m_Data->GetValueNorm (MAP_NO, x+1, y);
                rotate3d (&point3d, &trans3d);
                scale3d (&point3d, &trans3d);
                move3d (&point3d, &trans3d);
                transformate3d_2d (&point3d, &point2d, xm, ym, &trans3d);
                pp[1].setX (point2d.x);
                pp[1].setY (point2d.y);

                point3d.x = m_Data->GetValueNorm (X_AXIS_NO, x+1, 0);
                point3d.y = m_Data->GetValueNorm (Y_AXIS_NO, y+1, 0);
                point3d.z = m_Data->GetValueNorm (MAP_NO, x+1, y+1);
                rotate3d (&point3d, &trans3d);
                scale3d (&point3d, &trans3d);
                move3d (&point3d, &trans3d);
                transformate3d_2d (&point3d, &point2d, xm, ym, &trans3d);
                pp[2].setX (point2d.x);
                pp[2].setY (point2d.y);

                point3d.x = m_Data->GetValueNorm (X_AXIS_NO, x, 0);
                point3d.y = m_Data->GetValueNorm (Y_AXIS_NO, y+1, 0);
                point3d.z = m_Data->GetValueNorm (MAP_NO, x, y+1);
                rotate3d (&point3d, &trans3d);
                scale3d (&point3d, &trans3d);
                move3d (&point3d, &trans3d);
                transformate3d_2d (&point3d, &point2d, xm, ym, &trans3d);
                pp[3].setX (point2d.x);
                pp[3].setY (point2d.y);

                Painter->drawPolygon (pp, 4);

                Painter->drawRect (pp[0].x()-2, pp[0].y()-2, 4, 4);
                if (x == (XDim-2)) Painter->drawRect (pp[1].x()-2, pp[1].y()-2, 4, 4);
                if ((x == (XDim-2)) && (y == (YDim-2))) Painter->drawRect (pp[2].x()-2, pp[2].y()-2, 4, 4);
                if (y == (YDim-2)) Painter->drawRect (pp[3].x()-2, pp[3].y()-2, 4, 4);
            }
        }
        Painter->setBrush (brush_save);
    } else if (m_Data->GetType() == 1) {
        for (x = 0; x !=XDim; x++)  {
            point3d.x = m_Data->GetValueNorm (X_AXIS_NO, x, 0);
            point3d.y = 0.0;
            point3d.z = m_Data->GetValueNorm (MAP_NO, x, 0);
            rotate3d (&point3d, &trans3d);
            scale3d (&point3d, &trans3d);
            move3d (&point3d, &trans3d);
            transformate3d_2d (&point3d, &point2d, xm, ym, &trans3d);
            if (!x) {
                MoveToEx (point2d.x, point2d.y, &old_pos);
            } else {
                LineTo (Painter, point2d.x, point2d.y);
            }
            Painter->drawRect (point2d.x-2, point2d.y-2, 4, 4);
        }
    }
    // Paint axis behind the map
    paint_axis (Painter, xm, ym, &trans3d, ~axis_mask);
    // Paint picking crosses
    for (y = 0; y < YDim; y++) {
        for (x = 0; x < XDim; x++) {

            if (m_Data->IsPicked(x, y)) {
                point3d.x = m_Data->GetValueNorm (X_AXIS_NO, x, 0);
                point3d.y = m_Data->GetValueNorm (Y_AXIS_NO, y, 0);
                point3d.z = m_Data->GetValueNorm (MAP_NO, x, y);
                rotate3d (&point3d, &trans3d);
                scale3d (&point3d, &trans3d);
                move3d (&point3d, &trans3d);
                transformate3d_2d (&point3d, &point2d, xm, ym, &trans3d);
                QPen Pen = QPen (QColor (255, 0, 0));
                QPen PenSave = Painter->pen ();
                Painter->setPen (Pen);
                MoveToEx (point2d.x-6, point2d.y, &old_pos);
                LineTo (Painter, point2d.x+6, point2d.y);
                MoveToEx (point2d.x, point2d.y-6, nullptr);
                LineTo (Painter, point2d.x, point2d.y+6);
                Painter->setPen (PenSave);
                MoveToEx (old_pos.x, old_pos.y, nullptr);
            }
        }
    }
    // Paint map access
    if ((XDim > 0) && (YDim > 0)) {
        if (((m_Data->GetType() == 2) && (m_Data->GetYVid() > 0) && (m_Data->GetXVid() > 0)) ||
            ((m_Data->GetType() == 1) && (m_Data->GetXVid() > 0))) {
            value = m_Data->MapAccess(&(point3d.x), &(point3d.y), &(point3d.z));
            rotate3d (&point3d, &trans3d);
            scale3d (&point3d, &trans3d);
            move3d (&point3d, &trans3d);
            transformate3d_2d (&point3d, &point2d, xm, ym, &trans3d);
            QPen PenSave = Painter->pen();
            QPen Pen = QPen (QColor (0, 0, 255));
            Painter->setPen(Pen);
            MoveToEx (point2d.x, point2d.y, nullptr);
            LineTo (Painter, point2d.x-3, point2d.y-8);
            LineTo (Painter, point2d.x+3, point2d.y-8);
            LineTo (Painter, point2d.x, point2d.y);
            Painter->setPen(PenSave);
        }
    }
    return value;
}


void A2LCalMap3DView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    char BottonLine[1024];
    double value;

    QPainter Painter(this);
    if (m_Data->IsValid()) {
        int xm = this->width() / 2;
        int ym = this->height() / 2;
        int ipick, jpick;
        m_Data->GetLastPick(&ipick, &jpick);
        value = RedrawMap (&Painter, xm, ym, ipick, jpick);
        Painter.drawText (QRect(0, 0, this->width(), 20), Qt::AlignCenter, QString (m_Data->GetCharacteristicName()));
        //Painter.drawText (QRect(0, 0, this->width(), 45), Qt::AlignCenter, QString (m_Data->Desc));
        double x, y, z;
        const char *s;
        if (m_Data->IsPhysical()) {
            s = "(phys)";
        } else {
            s = "(raw)";
        }
        x = m_Data->GetRawValue (X_AXIS_NO, ipick, 0);
        y = m_Data->GetRawValue (Y_AXIS_NO, jpick, 0);
        z = m_Data->GetRawValue (MAP_NO, ipick, jpick);
        PrintFormatToString (BottonLine, sizeof(BottonLine), "%s  X = %g %s  Y = %g %s  Z = %g %s",
                 s,
                 x, m_Data->GetXUnit(),
                 y, m_Data->GetYUnit(),
                 z, m_Data->GetMapUnit());
        if (((m_Data->GetType() == 2) && (m_Data->GetYVid() > 0) && (m_Data->GetXVid() > 0)) ||
            ((m_Data->GetType() == 1) && (m_Data->GetXVid() > 0))) {
            PrintFormatToString (BottonLine + strlen(BottonLine), sizeof(BottonLine) - strlen(BottonLine), "  AP = %g %s",
                     value, m_Data->GetMapUnit());
        }
        Painter.drawText (QRect(0, height() - 20, width(), 20), Qt::AlignCenter, QString (BottonLine));
    }
}

void A2LCalMap3DView:: mousePressEvent(QMouseEvent * event)
{
    int AddFlag;
    switch (event->button ()) {
    case Qt::LeftButton:
        if ((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier) {
            AddFlag = 1;
        } else {
            AddFlag = 0;
        }
        PickingMap (width()/2, height()/2, GetEventXPos(event), GetEventYPos(event), AddFlag);
        update();
        break;
    default:
        break;
    }
}

void A2LCalMap3DView::wheelEvent(QWheelEvent *event)
{
    if ((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier) {
        // If Ctrl key is pressed -> zoom
        // do nothing
    } else {
        QPoint numPixels = event->pixelDelta();
        if (!numPixels.isNull()) {
            // ottherwise change the value
            emit ChangeValueByWheel (numPixels.ry());
        }
    }
    event->accept();
}

int A2LCalMap3DView::PickingMap (int xm, int ym, int xpos, int ypos, int add)
{
    int x, y, apick, help;
    POINT3D point3d;
    POINT2D point2d;
    int XDim = m_Data->GetXDim();
    int YDim = m_Data->GetYDim();
    int x_pick, y_pick;

    // Picking
    apick = 0x7FFFFFFF;
    for (x = 0; x < XDim; x++)  {
        for (y = 0; y < YDim; y++) {
            if (!add) m_Data->SetPick(x, y, 2); // if not CTRL key pressed delete all picks
            point3d.x = m_Data->GetValueNorm (X_AXIS_NO, x, 0);
            point3d.y = m_Data->GetValueNorm (Y_AXIS_NO, y, 0);
            point3d.z = m_Data->GetValueNorm (MAP_NO, x, y);
            rotate3d (&point3d, &trans3d);
            scale3d (&point3d, &trans3d);
            move3d (&point3d, &trans3d);
            transformate3d_2d (&point3d, &point2d, xm, ym, &trans3d);
            point2d.x -= xpos;
            point2d.y -= ypos;
            help = point2d.x * point2d.x + point2d.y * point2d.y;
            if (help < 0) {
                help = 0x7FFFFFFF;
            } else {
                help = static_cast<int>(sqrt (static_cast<double>(help)));
            }
            if (help < apick) {
                apick = help;
                x_pick = x;
                y_pick = y;
            }
        }
    }
    if (apick != 0x7FFFFFFF) {
        if (add) {
            if (m_Data->IsPicked(x_pick, y_pick)) {
                m_Data->SetPick(x_pick, y_pick, 2);    // reset
            } else {
                m_Data->SetPick(x_pick, y_pick, 1);    // set
            }
        } else {
            m_Data->SetPick(x_pick, y_pick, 3);    // toggle
        }
        m_Data->SetNoRecursiveSetPickMapFlag(true);
        emit SelectionChanged();
        m_Data->SetNoRecursiveSetPickMapFlag(false);
    }
    return 0;
}
