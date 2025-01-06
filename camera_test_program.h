/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2025. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  MAIN                             1       /* callback function: MAIN_CALLBACK */
#define  MAIN_acquisition_start           2       /* control type: command, callback function: (none) */
#define  MAIN_acquisition_stop            3       /* control type: command, callback function: (none) */
#define  MAIN_CANVAS_CAM_1                4       /* control type: canvas, callback function: (none) */
#define  MAIN_CANVAS_CAM_2                5       /* control type: canvas, callback function: (none) */
#define  MAIN_DOT_Y_CAM_2                 6       /* control type: numeric, callback function: (none) */
#define  MAIN_DOT_X_CAM_2                 7       /* control type: numeric, callback function: (none) */
#define  MAIN_DOT_Y_CAM_1                 8       /* control type: numeric, callback function: (none) */
#define  MAIN_DOT_X_CAM_1                 9       /* control type: numeric, callback function: (none) */
#define  MAIN_DECORATION                  10      /* control type: deco, callback function: (none) */

#define  TITLEPANEL                       2
#define  TITLEPANEL_NVELOGO               2       /* control type: picture, callback function: (none) */
#define  TITLEPANEL_VERSION               3       /* control type: textMsg, callback function: (none) */
#define  TITLEPANEL_VERSION_3             4       /* control type: textMsg, callback function: (none) */
#define  TITLEPANEL_VERSION_2             5       /* control type: textMsg, callback function: (none) */
#define  TITLEPANEL_PICTURE               6       /* control type: picture, callback function: (none) */
#define  TITLEPANEL_loading_msg           7       /* control type: textMsg, callback function: (none) */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK MAIN_CALLBACK(int panel, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
