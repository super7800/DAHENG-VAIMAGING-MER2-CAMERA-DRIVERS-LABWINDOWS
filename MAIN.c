#include "MAIN.h"

/***************************************************************************************************
Panels, global variables, functions 
****************************************************************************************************/

// Panels
static int p_main;         // Handle to main panel
static int p_title;        // Handle to the "TITLEPANEL" (loading panel)

// Functions
int setupCameras(void); // Set up the three cameras
void camera_run(void); // Run the camera UI

/***************************************************************************************************
LabWindows/CVI entry point. The main function, Runs at program start.
****************************************************************************************************/

void main (void)  {
	
    if ((p_main = LoadPanel (0, "camera_test_program.uir", MAIN)) < 0)
        return;
    
    if ((p_title = LoadPanel (0, "camera_test_program.uir", TITLEPANEL)) < 0)
        return;
    
    // Show loading panel
    DisplayPanel (p_title);
	
	SetCtrlVal(p_title, TITLEPANEL_loading_msg, "Configuring Cameras...");

    // Set up cameras
    if (setupCameras() == CANCEL) {
		
        MessagePopup("Fatal Error", "Failed to initialize the camera. Closing.");
        QuitUserInterface(0);
        return;
    }
    
    // Hide loading panel, show main panel
    HidePanel (p_title);
    DisplayPanel (p_main);
	SetPanelAttribute (p_main, ATTR_WINDOW_ZOOM, VAL_MAXIMIZE); // center and maximize

    // Start the main UI loop (which calls camera_run logic)
    camera_run();
    RunUserInterface ();
}

/***************************************************************************************************
Camera SETUP function. Called at program start by the MAIN function. 
****************************************************************************************************/

int setupCameras(void) {
	
    GX_STATUS emStatus = GX_STATUS_SUCCESS;
	
	// Settings Camera 1
	strcpy((char *)cameraOne.SerialNumber, "FCU24100XXX");
	cameraOne.Gain = 0;
	cameraOne.GainMode = 0;
	cameraOne.AutoGainMin = 0;
	cameraOne.AutoGainMax = 24;
	cameraOne.ExposureTime = 10000;
	cameraOne.ExposureTimeMode = 0;
	cameraOne.AutoExposureTimeMin = 10;
	cameraOne.AutoExposureTimeMax = 1000000;
	cameraOne.UseCrosshair = 1;
	cameraOne.CrosshairSize = 10;
	cameraOne.CrosshairThickness = 3;
	cameraOne.CrosshairColor = 16711680;
	cameraOne.CrosshairX = 2012;
	cameraOne.CrosshairY = 1518;
	
	// Settings Camera 2
	strcpy((char *)cameraTwo.SerialNumber, "FCU24100XXX");
	cameraTwo.Gain = 0;
	cameraTwo.GainMode = 0;
	cameraTwo.AutoGainMin = 0;
	cameraTwo.AutoGainMax = 24;
	cameraTwo.ExposureTime = 10000;
	cameraTwo.ExposureTimeMode = 0;
	cameraTwo.AutoExposureTimeMin = 10;
	cameraTwo.AutoExposureTimeMax = 1000000;
	cameraTwo.UseCrosshair = 1;
	cameraTwo.CrosshairSize = 10;
	cameraTwo.CrosshairThickness = 3;
	cameraTwo.CrosshairColor = 16711680;
	cameraTwo.CrosshairX = 2012;
	cameraTwo.CrosshairY = 1518;

    // Open device
    if (OpenDevice(&cameraOne) != GX_STATUS_SUCCESS) {
		
		MessagePopup("Fatal Error", "Failed to open the camera.");
        return CANCEL;
	}
	
    if (OpenDevice(&cameraTwo) != GX_STATUS_SUCCESS) {
		
		MessagePopup("Fatal Error", "Failed to open the camera.");
        return CANCEL;
	}

    // Init camera parameters
    emStatus = InitDevice(&cameraOne);
    if (VERIFY_STATUS_RET(emStatus) != GX_STATUS_SUCCESS) return CANCEL;
    emStatus = InitDevice(&cameraTwo);
    if (VERIFY_STATUS_RET(emStatus) != GX_STATUS_SUCCESS) return CANCEL;

    return OK;
}

/***************************************************************************************************
Camera RUN function. Called at program start by the MAIN function. Handles the camera UI,
which at this time only consists of a start and stop aquiring images button.
****************************************************************************************************/

void camera_run(void) {
	
    int control, panelHandle;
	
	// Set the canvas panel and control
	cameraOne.panelHandle   = p_main;
	cameraOne.canvasControl = MAIN_CANVAS_CAM_1;
	cameraTwo.panelHandle   = p_main;
	cameraTwo.canvasControl = MAIN_CANVAS_CAM_2;
	
	SetCtrlVal(p_main, MAIN_DOT_X_CAM_1, cameraOne.CrosshairX);
	SetCtrlVal(p_main, MAIN_DOT_Y_CAM_1, cameraOne.CrosshairY);
	SetCtrlVal(p_main, MAIN_DOT_X_CAM_2, cameraTwo.CrosshairX);
	SetCtrlVal(p_main, MAIN_DOT_Y_CAM_2, cameraTwo.CrosshairY);
	
	

    while (p_main >= 0) {
		
        // Wait for user events on this panel
        GetUserEvent(1, &panelHandle, &control);

        switch (control) {
			
            case MAIN_acquisition_start:
                StartCameraAcquisition(&cameraOne);
				StartCameraAcquisition(&cameraTwo);
				
			    // Create an async timer that fires every 1ms (0.5)
			    // and calls UpdateCameraCallback
			    cameraOne.timerId = NewAsyncTimer(0.5, -1, 1, (AsyncTimerCallbackPtr)UpdateCameraCallback, &cameraOne);
				cameraTwo.timerId = NewAsyncTimer(0.5, -1, 1, (AsyncTimerCallbackPtr)UpdateCameraCallback, &cameraTwo);
				
                break;
            
            case MAIN_acquisition_stop:
                StopCameraAcquisition(&cameraOne);
				StopCameraAcquisition(&cameraTwo);
                break;
				
            case MAIN_DOT_X_CAM_1:
				if (cameraOne.UseCrosshair == 1) if (GetCtrlVal(panelHandle, MAIN_DOT_X_CAM_1, &cameraOne.CrosshairX) < 0) MessagePopup("Error", "Unable to read value from the textbox.");
                break;
				
            case MAIN_DOT_Y_CAM_1:
				if (cameraOne.UseCrosshair == 1) if (GetCtrlVal(panelHandle, MAIN_DOT_Y_CAM_1, &cameraOne.CrosshairY) < 0) MessagePopup("Error", "Unable to read value from the textbox.");
                break;
				
            case MAIN_DOT_X_CAM_2:
				if (cameraTwo.UseCrosshair == 1) if (GetCtrlVal(panelHandle, MAIN_DOT_X_CAM_2, &cameraTwo.CrosshairX) < 0) MessagePopup("Error", "Unable to read value from the textbox.");
                break;
				
            case MAIN_DOT_Y_CAM_2:
				if (cameraTwo.UseCrosshair == 1) if (GetCtrlVal(panelHandle, MAIN_DOT_Y_CAM_2, &cameraTwo.CrosshairY) < 0) MessagePopup("Error", "Unable to read value from the textbox.");
                break;
            
            default:
                break;
        }
    }
}
