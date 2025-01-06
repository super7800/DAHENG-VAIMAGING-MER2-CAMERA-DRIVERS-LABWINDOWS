#include <windows.h>
#include <stdio.h>
#include <stdlib.h>   
#include <ansi_c.h>
#include <string.h>
#include <math.h>
#include <analysis.h>
#include <gpib.h>
#include <time.h>
#include <userint.h>
#include <formatio.h>
#include <utility.h>
#include <lowlvlio.h>
#include "toolbox.h"
#include "CALLBACK.h"
#include <time.h>
#include "camera_test_program.h"
#include <cvirte.h>
#include <asynctmr.h> // For NewAsyncTimer, DiscardAsyncTimer
#include <utility.h> // For Timer callbacks

#include "DAHENG_CAMERA_DRIVERS.h"

/***************************************************************************************************
Main Defines
****************************************************************************************************/

// Global Return Defines
#define TRUE        1
#define FALSE       0
#define CANCEL      -1
#define OK			1
