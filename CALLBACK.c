#include <ansi_c.h>
#define WIN32_LEAN_AND_MEAN    // Exclude rarely-used APIs from Windows headers
#include <windows.h>           // Include Windows headers first
#include <userint.h>           // CVI User Interface headers
#include <utility.h>           // CVI Utility functions
#include "callback.h"
#include "DAHENG_CAMERA_DRIVERS.h"

//==============================================================================
// UI callback function prototypes
//==============================================================================

// Callback function for the smart switch panel
int CVICALLBACK MAIN_CALLBACK(int panel, int event, void *callbackData, int eventData1, int eventData2) {
    switch (event) {
        case EVENT_GOT_FOCUS:
            // Handle got focus event if needed
            break;
        case EVENT_LOST_FOCUS:
            // Handle lost focus event if needed
            break;
        case EVENT_CLOSE:
            // Actions to perform when the close button (X) is pressed
            HidePanel(panel); // Hide the panel
			
			CloseDevice (&cameraOne);
			CloseDevice (&cameraTwo);

            // Signal the loop to stop running
            if (callbackData) {
                int *continueRunning = (int *)callbackData;
                *continueRunning = 0;
            }
			
			exit(0);
			
            break;
    }
    return 0;
}


