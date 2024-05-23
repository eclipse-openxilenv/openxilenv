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


#include "Platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "Config.h"
#include "ConfigurablePrefix.h"
#include "Files.h"
#include "InterfaceToScript.h"
#include "ScriptHtmlFunctions.h"


static int NoLevel1 = 0;
static int NoLevel2 = 0;
static int NoLevel3 = 0;
static int NoLevel4 = 0;
static int NoLevel5 = 0;
static int NoLevel6 = 0;


#define MAX_ANZAHL_KEYWORDS 10

// For the different keywords
#define HTML_GREEN          1
#define HTML_RED            2
#define HTML_BLUE           3
#define HTML_BLACK          4
#define HTML_YELLOW         5

#define HTML_BOLD           10
#define HTML_LINE           11

#define HTML_HEADER_1       20
#define HTML_HEADER_2       21
#define HTML_HEADER_3       22
#define HTML_HEADER_4       23
#define HTML_HEADER_5       24
#define HTML_HEADER_6       25

void init_html_strings(void)
{
    NoLevel1 = 0;
    NoLevel2 = 0;
    NoLevel3 = 0;
    NoLevel4 = 0;
    NoLevel5 = 0;
    NoLevel6 = 0;
}

int get_html_strings(char *st1,char *st2,char *keywordstring)
{
    int Keyword[MAX_ANZAHL_KEYWORDS];
    char Separator[]= " ";
    char *SingleName;
    char StringBefore[1024];
    char StringBehind[1024];
    char string_bef[1024];
    char string_beh[1024];
    char StringWithNumber[1024];
    char StringNoLevel1[1024];
    char StringNoLevel2[1024];
    char StringNoLevel3[1024];
    char StringNoLevel4[1024];
    char StringNoLevel5[1024];
    char StringNoLevel6[1024];

    int i;
    int KeywordCount=0;
    int ret = 0;

    SingleName= strtok(keywordstring, Separator);

    // Delete the keywords from the last REPORT command
    for(i=0;i<MAX_ANZAHL_KEYWORDS;i++) {
        Keyword[i]=0;
    }

    // Loop over the keywords
    while(SingleName!=NULL) {
        if(stricmp("green",SingleName)==0) {
            Keyword[KeywordCount] = HTML_GREEN;
        } else if(stricmp("red",SingleName)==0) {
            Keyword[KeywordCount] = HTML_RED;
        } else if(stricmp("blue",SingleName)==0) {
            Keyword[KeywordCount] = HTML_BLUE;
        } else if(stricmp("black",SingleName)==0) {
            Keyword[KeywordCount] = HTML_BLACK;
        } else if(stricmp("yellow",SingleName)==0) {
            Keyword[KeywordCount] = HTML_YELLOW;
        } else if(stricmp("bold",SingleName)==0) {
            Keyword[KeywordCount] = HTML_BOLD;
        } else if(stricmp("line",SingleName)==0) {
            Keyword[KeywordCount] = HTML_LINE;
        } else if(stricmp("header1",SingleName)==0) {
            Keyword[KeywordCount]=HTML_HEADER_1;
        } else if(stricmp("header2",SingleName)==0) {
            Keyword[KeywordCount]=HTML_HEADER_2;
        } else if(stricmp("header3",SingleName)==0) {
            Keyword[KeywordCount]=HTML_HEADER_3;
        } else if(stricmp("header4",SingleName)==0) {
            Keyword[KeywordCount]=HTML_HEADER_4;
        } else if(stricmp("header5",SingleName)==0) {
            Keyword[KeywordCount]=HTML_HEADER_5;
        } else if(stricmp("header6",SingleName)==0) {
            Keyword[KeywordCount]=HTML_HEADER_6;
        } else if(stricmp("msg",SingleName)==0) {
            ret = 1;
        }
        SingleName= strtok(NULL, Separator);
        KeywordCount++;
    }

    // Delete string before and behind
    memset(StringBefore,'\0',sizeof(StringBefore));
    memset(StringBehind,'\0',sizeof(StringBehind));


    for(i=0;i<KeywordCount;i++) {
        memset(string_bef,'\0',sizeof(string_bef));
        memset(string_beh,'\0',sizeof(string_beh));
        memset(StringWithNumber,'\0',sizeof(StringWithNumber));
        memset(StringNoLevel1,'\0',sizeof(StringNoLevel1));
        memset(StringNoLevel2,'\0',sizeof(StringNoLevel1));
        memset(StringNoLevel3,'\0',sizeof(StringNoLevel1));
        memset(StringNoLevel4,'\0',sizeof(StringNoLevel1));
        memset(StringNoLevel5,'\0',sizeof(StringNoLevel1));
        memset(StringNoLevel6,'\0',sizeof(StringNoLevel1));

        switch(Keyword[i]) {
        case HTML_GREEN:
            strcpy(string_bef,"\n<font color=\\#00C000\\>");
            strcpy(string_beh,"</font><br>");
            break;
        case HTML_RED:
            strcpy(string_bef,"\n<font color=\"#FF0000\">");
            strcpy(string_beh,"</font><br>");
            break;
        case HTML_BLUE:
            strcpy(string_bef,"\n<font color=\"#0000FF\">");
            strcpy(string_beh,"</font><br>");
            break;
        case HTML_BLACK:
            strcpy(string_bef,"\n<font color=\"#000000\">");
            strcpy(string_beh,"</font><br>");
            break;
        case HTML_YELLOW:
            strcpy(string_bef,"\n<font color=\"#FFFF00\">");
            strcpy(string_beh,"</font><br>");
            break;
        case HTML_BOLD:
            strcpy(string_bef,"\n<strong>");
            strcpy(string_beh,"</strong>");
            break;
        case HTML_LINE:
            strcpy(string_bef,"\n<hr />");
            strcpy(string_beh,"\n");
            break;
        case HTML_HEADER_1:
            NoLevel1++;
            strcpy(string_bef,"\n<h1>\n");
            sprintf(StringNoLevel1,"%d.",NoLevel1);
            strcat(string_bef,StringNoLevel1);
            strcpy(string_beh,"</h1>\n");
            NoLevel2=0;
            NoLevel3=0;
            NoLevel4=0;
            NoLevel5=0;
            NoLevel6=0;
            break;
        case HTML_HEADER_2:
            NoLevel2++;
            strcpy(string_bef,"\n<h2>");
            sprintf(StringNoLevel1,"%d.",NoLevel1);
            sprintf(StringNoLevel2,"%d.",NoLevel2);
            strcat(string_bef,StringNoLevel1);
            strcat(string_bef,StringNoLevel2);
            strcpy(string_beh,"</h2>\n");
            NoLevel3=0;
            NoLevel4=0;
            NoLevel5=0;
            NoLevel6=0;
            break;
        case HTML_HEADER_3:
            NoLevel3++;
            strcpy(string_bef,"\n<h3>");
            sprintf(StringNoLevel1,"%d.",NoLevel1);
            sprintf(StringNoLevel2,"%d.",NoLevel2);
            sprintf(StringNoLevel3,"%d.",NoLevel3);
            strcat(string_bef,StringNoLevel1);
            strcat(string_bef,StringNoLevel2);
            strcat(string_bef,StringNoLevel3);
            strcpy(string_beh,"</h3>\n");
            NoLevel4=0;
            NoLevel5=0;
            NoLevel6=0;
            break;
        case HTML_HEADER_4:
            NoLevel4++;
            strcpy(string_bef,"\n<h4>");
            strcpy(string_beh,"</h4>\n");
            sprintf(StringNoLevel1,"%d.",NoLevel1);
            sprintf(StringNoLevel2,"%d.",NoLevel2);
            sprintf(StringNoLevel3,"%d.",NoLevel3);
            sprintf(StringNoLevel4,"%d.",NoLevel4);
            strcat(string_bef,StringNoLevel1);
            strcat(string_bef,StringNoLevel2);
            strcat(string_bef,StringNoLevel3);
            strcat(string_bef,StringNoLevel4);
            NoLevel5=0;
            NoLevel6=0;
            break;
        case HTML_HEADER_5:
            NoLevel5++;
            strcpy(string_bef,"\n<h5>");
            strcpy(string_beh,"</h5>\n");
            sprintf(StringNoLevel1,"%d.",NoLevel1);
            sprintf(StringNoLevel2,"%d.",NoLevel2);
            sprintf(StringNoLevel3,"%d.",NoLevel3);
            sprintf(StringNoLevel4,"%d.",NoLevel4);
            sprintf(StringNoLevel5,"%d.",NoLevel5);
            strcat(string_bef,StringNoLevel1);
            strcat(string_bef,StringNoLevel2);
            strcat(string_bef,StringNoLevel3);
            strcat(string_bef,StringNoLevel4);
            strcat(string_bef,StringNoLevel5);
            NoLevel6=0;
            break;
        case HTML_HEADER_6:
            NoLevel6++;
            strcpy(string_bef,"\n<h6>");
            strcpy(string_beh,"</h6>\n");
            sprintf(StringNoLevel1,"%d.",NoLevel1);
            sprintf(StringNoLevel2,"%d.",NoLevel2);
            sprintf(StringNoLevel3,"%d.",NoLevel3);
            sprintf(StringNoLevel4,"%d.",NoLevel4);
            sprintf(StringNoLevel5,"%d.",NoLevel5);
            sprintf(StringNoLevel6,"%d.",NoLevel6);
            strcat(string_bef,StringNoLevel1);
            strcat(string_bef,StringNoLevel2);
            strcat(string_bef,StringNoLevel3);
            strcat(string_bef,StringNoLevel4);
            strcat(string_bef,StringNoLevel5);
            strcat(string_bef,StringNoLevel6);
            break;
            default:
            break;
        }
        strcat(StringBefore, string_bef);
        strcat(StringBehind, string_beh);
    }
    strcpy(st1,StringBefore);
    strcpy(st2,StringBehind);

    return ret;
}


/*******************************************************************************
**
**    Function    : GenerateHTMLReportFile
**
**    Description : Erzeugt ein HTML Script-Report-File
**                  Ueber Report-Befehle wird diese Datei vom laufenden Script
**                  erweitert.
**
**    Parameter   : -
**
**    Returnvalues:  0 = o.k.
**                  !0 = irgendwas ging schief
**
*******************************************************************************/
FILE *GenerateHTMLReportFile (char *HTML_ReportFilename)
{
 char string_mit_dem_aktuellen_pfad[1024] = {0};
 FILE *HTML_ReportFile;

 // Messagefilename um Pfad und Laufwerk zu vervollstaendigen
 // Wichtig, falls Script-Prozess den Pfad abaendert.
 ScriptIdentifyPath (HTML_ReportFilename);

 // vorhandenes File von letztem Aufruf oder Script-Lauf loeschen
 remove(HTML_ReportFilename);

 // Evtl. vorhandene Datei wird ueberschrieben
 HTML_ReportFile = open_file (HTML_ReportFilename, "w+");
 /* Pruefen, ob Fehler aufgetreten */
 if (HTML_ReportFile == NULL)
 {
    // Failed
    return NULL;
 }

 // ****************
 // Kopf schreiben
 // ****************
 fprintf (HTML_ReportFile, "\n <HTML> ");
 // Basis-Pfad fuer alle verlinkten Dokumente eintragen
 ScriptIdentifyPath(string_mit_dem_aktuellen_pfad);
 fprintf (HTML_ReportFile, "<BASE HREF=\"HTTP://%s\">",string_mit_dem_aktuellen_pfad); //Pfad hinter dem alle Links stehen
 // Titel erstellen
 fprintf (HTML_ReportFile, " \n<HEAD>\
 \n<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=windows-1252\">\
 \n<TITLE>HTML_ReportFile</TITLE>\
 \n</HEAD>\
 \n<P><!-- weisser Dateihintergrund --></P>\
 \n<H1>%s-HTML-Report-File</H1><DIR>\
 \n<DIR>\
 \n<DIR>\
 \n<DIR>\
 \n</DIR>\
 \n</DIR>\
 \n</DIR> ", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME));
 // Und jetzt nen Rahmen beginnen
 fprintf (HTML_ReportFile, "\n<P STYLE=\"border: double #CCFFCC 20px\">");
 // und auf Courier umschalten
 fprintf (HTML_ReportFile, "\n<FONT FACE=\"courier\">");
 fprintf (HTML_ReportFile, "\n<br>Filename              : %s", HTML_ReportFilename);
 fprintf (HTML_ReportFile, "\n<br>%s-Version (used): %.3lf",
          GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME),
          XILENV_VERSION/1000.0 );
 /* DATUM & ZEIT!!! */
 {
  char *datum;            /* aktuelles Datum                       */
  time_t datum_und_zeit;  /* Struct in time.h definiert            */
  time (&datum_und_zeit);           /* Datum und Zeit einlesen         */
  datum   = ctime(&datum_und_zeit);
  fprintf (HTML_ReportFile, "\n<br>date                      : %s", datum);
 }
 // Courier beenden
 fprintf (HTML_ReportFile, "\n</FONT>");
 //Rahmen beenden
 fprintf (HTML_ReportFile, "\n</P>");

 // alles o.k.
 return HTML_ReportFile;
}

/*******************************************************************************
**
**    Function    : CloseHTMLReportFile
**
**    Description : HTML-Script-Report-File schliessen und die
**                  HTML-Fussinformationen ausgeben
**
**    Parameter   : -
**
**    Returnvalues:  0 = o.k.
**                  !0 = irgendwas ging schief
**
*******************************************************************************/
int CloseHTMLReportFile (FILE *HTML_ReportFile)
{
     // ********************
     // Fussinfos schreiben
     // ********************
     if(HTML_ReportFile != NULL) {
        // HTML beenden
        fprintf (HTML_ReportFile, "\n</body> </html>");
        // Datei schliessen
        close_file(HTML_ReportFile);
     }

     // alles o.k.
     return(0);
}




