#include "GxIAPI.h" // Camera Includes 
#include "DxImageProc.h" // Camera Includes 
#include "camera_test_program.h"
#include <ansi_c.h>
#include <cvirte.h>      
#include <userint.h>
#include "toolbox.h"
#include "CALLBACK.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "asynctmr.h"


/***************************************************************************************************
The BMP File Struct. Used to display and save BMP images.
****************************************************************************************************/

#pragma pack(push, 1)  // ensure structures are byte-aligned

// BMP File Header
typedef struct bmp_file_header{
    uint16_t bfType;        // "BM"
    uint32_t bfSize;        // file size in bytes
    uint16_t bfReserved1;   // reserved, must be 0
    uint16_t bfReserved2;   // reserved, must be 0
    uint32_t bfOffBits;     // offset in bytes to bitmap data
} BmpFileHeader;

// BMP Info Header (a.k.a. DIB header)
typedef struct bmp_info_header {
    uint32_t biSize;          	// size of this header (40 bytes)
    int32_t  biWidth;         	// image width
    int32_t  biHeight;        	// image height
    uint16_t biPlanes;        	// # of color planes, must be 1
    uint16_t biBitCount;      	// bits per pixel (24 = RGB24, 8 = mono)
    uint32_t biCompression;   	// 0 = BI_RGB (no compression)
    uint32_t biSizeImage;     	// size of raw bitmap data
    int32_t  biXPelsPerMeter; 	// pixels per meter in X
    int32_t  biYPelsPerMeter; 	// pixels per meter in Y
	int32_t  biColorTable[256];	// MAKE_COLOR(r, g, b) -> uses 0x00BBGGRR in CVI. If 8-bit, build a grayscale color table for LabWindows.
    uint32_t biClrUsed;       	// # of colors in the palette
    uint32_t biClrImportant;  	// # of important colors
} BmpInfoHeader;

#pragma pack(pop)

/***************************************************************************************************
Camera Struct. This struct holds everything about the camera: connection, buffers, etc.
****************************************************************************************************/

struct camera_s {
	
    // Flags
    int DevOpened;       // 0=FALSE, 1=TRUE
    int IsSnap;          // 0=not snapping, 1=snap started
    int IsColorFilter;   // 0=mono, 1=color
    int ImplementAutoGain;
    int ImplementAutoShutter;
    int ImplementLight;

    // Timer ID from LabWindows async timer
    int timerId;
	
	// Camera ID
	unsigned char SerialNumber[2048];

    // Camera parameters
    int64_t Gray;
    int64_t RoiX;
    int64_t RoiY;
    int64_t RoiW;
    int64_t RoiH;
	
	// Camera Gain parameters
	double  Gain;
	int GainMode; // 0 for disabled, 1 for enabled
    double  AutoGainMin;
    double  AutoGainMax;
	
	// Camera Exposure parameters
	double  ExposureTime;
	int ExposureTimeMode; // 0 for disabled, 1 for enabled
    double  AutoExposureTimeMin;
    double  AutoExposureTimeMax;
	
    double  AutoShutterMin;
    double  AutoShutterMax;
    int64_t ImageWidth;
    int64_t ImageHeight;
    int64_t PayLoadSize;
    int64_t PixelColorFilter;
	
	// Crosshair Settings
	int UseCrosshair; 			// 0=FALSE, 1=TRUE
	int CrosshairSize;  		// length from center
	int CrosshairThickness;  		// Thickness of the crosshair line
	int CrosshairColor; 		// RGB value is a 4-byte integer with the hexadecimal format 0x00RRGGBB.
	unsigned int CrosshairX;	// Position On Image (x)
	unsigned int CrosshairY;	// Position On Image (y)
	
	// Variables for real-time image display
	int panelHandle;
    int canvasControl;
	

    // Buffers for image data
    unsigned char *RawBuffer;  	// Received from camera
    unsigned char *ImgBuffer;   // Color-converted/flipped data
	unsigned char BmpBuf[2048]; // The buffer for showing image

    // LabWindows/CVI Bitmap handle for drawing
    int BitmapHandle;

    // Windows GxIAPI handle
    GX_DEV_HANDLE Device;
	
	// Bmp Header And Info Structs 
	BmpInfoHeader BmpInfo;
	BmpFileHeader BmpFile;
    
    // Auto modes
    GX_EXPOSURE_AUTO_ENTRY AutoShutterMode;
    GX_GAIN_AUTO_ENTRY     AutoGainMode;
} cameraOne, cameraTwo;

/***************************************************************************************************
Camera Public Functions And Variables
****************************************************************************************************/

GX_STATUS OpenDevice   (struct camera_s *cam);
void CloseDevice  (struct camera_s *cam);
GX_STATUS InitDevice   (struct camera_s *cam);
void StartCameraAcquisition (struct camera_s *cam); // Called when the start acquisition button pressed
void StopCameraAcquisition (struct camera_s *cam); // Called when the stop acquisition button pressed
int VERIFY_STATUS_RET (GX_STATUS emStatus);
void ShowErrorString(GX_STATUS emErrorStatus);
void CVICALLBACK UpdateCameraCallback(int reserved, int timerId, int event, struct camera_s *cam, int eventData1, int eventData2); // Display image on canvas





