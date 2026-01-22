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


#include <stdint.h>
#include <limits.h>

#include <ColorHelpers.h>

//#define DEBUG_COLOR_SELECT

static QColor GetDiffColor(const QColor &par_Color1, const QColor &par_Color2, bool par_Clockwise)
{
    int Hue1, Saturation1, Lightness1;
    int Hue2, Saturation2, Lightness2;
    int Hue3, Saturation3, Lightness3;
    par_Color1.getHsl(&Hue1, &Saturation1, &Lightness1);
    par_Color2.getHsl(&Hue2, &Saturation2, &Lightness2);

#ifdef DEBUG_COLOR_SELECT

    FILE *fh = fopen("c:\\temp\\color.txt", "at");
    fprintf(fh, "\nGetDiffColor:\n");
    fprintf(fh, "color1 %i, %i , %i:\n", Hue1, Saturation1, Lightness1);
    fprintf(fh, "color2 %i, %i , %i:\n", Hue2, Saturation2, Lightness2);
#endif
    if (par_Clockwise) {
        int HueDiff = Hue2 - Hue1;
        Hue3 = Hue1 + HueDiff/2;

    } else {
        int HueDiff = 360 - Hue1 + Hue2;
        Hue3 = Hue1 + HueDiff/2;
    }
    if (Hue3 > 359) Hue3 -= 360;
    if (Saturation1 > Saturation2) {
        Saturation3 = Saturation2 + (Saturation1 - Saturation2) / 2;
    } else {
        Saturation3 = Saturation1 + (Saturation2 - Saturation1) / 2;
    }
    if (Lightness1 > Lightness2) {
        Lightness3 = Lightness2 + (Lightness1 - Lightness2) / 2;
    } else {
        Lightness3 = Lightness1 + (Lightness2 - Lightness1) / 2;
    }
#ifdef DEBUG_COLOR_SELECT
    fprintf(fh, "color3 %i, %i , %i:\n", Hue3, Saturation3, Lightness3);
    fclose(fh);
#endif
    QColor Ret;
    Ret.setHsl(Hue3, Saturation3, Lightness3);
    return Ret;
}

QColor GetColorProposal(QList<QColor> &par_UsedColors)
{
    // Sort
    QList<QColor> SortedUsedColors;
    int Hue1, Saturation1, Lightness1;
    int Hue2, Saturation2, Lightness2;

    for (int x = 0; x < (par_UsedColors.count()); x++) {
        QColor Color1 =  par_UsedColors.at(x);
        Color1.getHsl(&Hue1, &Saturation1, &Lightness1);
        if (Hue1 >= 0) { // don't add black, gray to whith
            int y;
            if (SortedUsedColors.count() == 0) {
                SortedUsedColors.insert(0, Color1);
            } else {
                for (y = 0; y < (SortedUsedColors.count()); y++) {
                    QColor Color2 = SortedUsedColors.at(y);
                    Color2.getHsl(&Hue2, &Saturation2, &Lightness2);
                    if (Hue1 <= Hue2) {
                        break;
                    }
                }
                // don't add same
                if (Hue1 != Hue2) {
                    SortedUsedColors.insert(y, Color1);
                }
            }
        }
    }

    if (SortedUsedColors.count() >= 2) {
        int HueDiff;
        int HueDiffMax;
        int HueDiffMaxX;
        HueDiffMax = 0;
        HueDiffMaxX = -1;

#ifdef DEBUG_COLOR_SELECT
        FILE *fh = fopen("c:\\temp\\color.txt", "wt");
        fprintf (fh, "List of all sorted colors:\n");
        for (int x = 0; x < (SortedUsedColors.count()); x++) {
            QColor Color1 =  SortedUsedColors.at(x);
            Color1.getHsl(&Hue1, &Saturation1, &Lightness1);
            fprintf (fh, "%i: %i, %i, %i:\n", x, Hue1, Saturation1, Lightness1);
        }
        fprintf (fh, "Now search:\n");
#endif
        for (int x = 0; x < (SortedUsedColors.count()); x++) {
            QColor Color2;
            QColor Color1 = SortedUsedColors.at(x);
            Color1.getHsl(&Hue1, &Saturation1, &Lightness1);
            if (x < (SortedUsedColors.count() - 1)) {
                Color2 = SortedUsedColors.at(x + 1);
                Color2.getHsl(&Hue2, &Saturation2, &Lightness2);
                HueDiff = Hue2 - Hue1;
            } else {
                Color2 = SortedUsedColors.at(0);
                Color2.getHsl(&Hue2, &Saturation2, &Lightness2);
                HueDiff = 360 - Hue1 + Hue2;
            }
#ifdef DEBUG_COLOR_SELECT
            fprintf (fh, "  %i: %i, diff=%i:\n", x, Hue2, HueDiff);
#endif
            if (HueDiff > HueDiffMax) {
                HueDiffMax = HueDiff;
                HueDiffMaxX = x;
#ifdef DEBUG_COLOR_SELECT
                fprintf (fh, "    new max hue diff = %i\n", HueDiffMax);
#endif
            }
        }
#ifdef DEBUG_COLOR_SELECT
        fprintf (fh, "Max hue %i at %in", HueDiffMax, HueDiffMaxX);
        fclose(fh);
#endif
        if(HueDiffMaxX >= 0) {
            if (HueDiffMaxX < (SortedUsedColors.count() - 1)) {
                return GetDiffColor(SortedUsedColors.at(HueDiffMaxX), SortedUsedColors.at(HueDiffMaxX+1), true);
            } else {
                return GetDiffColor(SortedUsedColors.at(HueDiffMaxX), SortedUsedColors.at(0), false);
            }
        } else {
            // this should not happen
            QColor Ret(Qt::red);
            return Ret;
        }
    } else if (SortedUsedColors.count() == 1){
        int Hue, Saturation, Lightness;
        SortedUsedColors.at(0).getHsl(&Hue, &Saturation, &Lightness);
        Hue += 180;
        if (Hue > 359) Hue -= 360;
        QColor Ret;
        Ret.setHsl(Hue, Saturation, Lightness);
        return Ret;
    } else {
        // if no color chooce red!
        QColor Ret(Qt::red);
        return Ret;
    }
}
