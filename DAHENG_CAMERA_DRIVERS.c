#include "DAHENG_CAMERA_DRIVERS.h"

/***************************************************************************************************
Camera Private Functions And Variables
****************************************************************************************************/

// Helper macros
#define MAKE_COLOR(r, g, b)  ( ((b) << 16) | ((g) << 8) | (r) )


void GX_STDC OnFrameCallbackFun(GX_FRAME_CALLBACK_PARAM *pFrame); // Frame callback function for GxIAPI

GX_STATUS SetPixelFormat8bit(struct camera_s *cam);
GX_STATUS GX_STDC GXInitLib(void);
void UnPrepareForShowImg(struct camera_s *cam);
int PrepareForShowImg(struct camera_s *cam);
int PrepareForShowColorImg(struct camera_s *cam);
int PrepareForShowMonoImg(struct camera_s *cam);
int SaveBufferAsBMP(const char* fileName, struct camera_s *cam);

/***************************************************************************************************
Return Defines
****************************************************************************************************/

#define TRUE        1
#define FALSE       0
#define CANCEL      -1
#define OK			1

/***************************************************************************************************
Camera open and close functions.


GX_STRING_VALUE stStringValue;
emStatus = GXGetStringValue(m_hDevice, "DeviceSerialNumber", &stStringValue);
VERIFY_STATUS_RET(emStatus);
****************************************************************************************************/

GX_STATUS OpenDevice(struct camera_s *cam) {
	
    GX_STATUS   emStatus      = GX_STATUS_SUCCESS;
    uint32_t    nDevNum       = 0;
    GX_OPEN_PARAM stOpenParam = {0};
    GX_DEV_HANDLE hTempDevice = NULL;
    int         foundDevice   = 0;

    // Initialize GxIAPI library
    emStatus = GXInitLib();
    if (emStatus != GX_STATUS_SUCCESS)
    {
        ShowErrorString(emStatus);
        return CANCEL;
    }

    // Update/Enumerate the device list
    emStatus = GXUpdateDeviceList(&nDevNum, 1000);
    if (emStatus != GX_STATUS_SUCCESS)
    {
        ShowErrorString(emStatus);
        MessagePopup("Camera Error", "Failed to enumerate devices.");
        return CANCEL;
    }

    // Check how many devices we found
    if (nDevNum == 0)
    {
        MessagePopup("Camera Error", "No cameras detected!");
        return CANCEL;
    }

    // If our camera struct is already open, close it first
    if (cam->Device != NULL)
    {
        emStatus = GXCloseDevice(cam->Device);
        if (emStatus != GX_STATUS_SUCCESS)
        {
            ShowErrorString(emStatus);
            MessagePopup("Camera Error", "Failed to close previously open camera handle.");
            return CANCEL;
        }
        cam->Device = NULL;
    }

    // We want to match this camera's serial number
    // (Ensure cam->SerialNumber is set to the desired serial before calling!)
    const char* desiredSerial = (const char*)cam->SerialNumber;

    // Loop over each camera index, 1 through nDevNum
    // (Daheng cameras are 1-based for GX_OPEN_INDEX)
    for (uint32_t i = 1; i <= nDevNum; i++)
    {
        char indexStr[64];
        sprintf(indexStr, "%u", i);

        stOpenParam.accessMode = GX_ACCESS_EXCLUSIVE;
        stOpenParam.openMode   = GX_OPEN_INDEX;
        stOpenParam.pszContent = indexStr;

        // Try opening device #i
        emStatus = GXOpenDevice(&stOpenParam, &hTempDevice);
        if (emStatus != GX_STATUS_SUCCESS)
        {
            // Could not open this device; skip it
            // (e.g., device might be in use, or some error)
            continue;
        }

        // Once open, query the device for its serial number
        // (We do it in two steps: first get length, then get the string)
        size_t len = 0;
        emStatus = GXGetStringLength(hTempDevice, GX_STRING_DEVICE_SERIAL_NUMBER, &len);
        if (emStatus != GX_STATUS_SUCCESS || len == 0)
        {
            // If we can't read the serial number, close and keep looking
            GXCloseDevice(hTempDevice);
            hTempDevice = NULL;
            continue;
        }

        // Allocate a buffer for the serial number and read it
        char* deviceSerial = (char*)malloc(len + 1);
        if (!deviceSerial)
        {
            GXCloseDevice(hTempDevice);
            hTempDevice = NULL;
            MessagePopup("Memory Error", "Failed to allocate memory for serial reading!");
            return CANCEL;
        }
        memset(deviceSerial, 0, len + 1);

        // Actually retrieve the serial string
        emStatus = GXGetString(hTempDevice, GX_STRING_DEVICE_SERIAL_NUMBER, deviceSerial, &len);
        if (emStatus != GX_STATUS_SUCCESS)
        {
            free(deviceSerial);
            GXCloseDevice(hTempDevice);
            hTempDevice = NULL;
            continue;
        }

        // Compare with the desiredSerial
        if (strcmp(deviceSerial, desiredSerial) == 0)
        {
            // It's a match! Assign this device to our camera struct
            cam->Device    = hTempDevice;
            cam->DevOpened = 1;
            foundDevice    = 1;

            // Store the actual serial we opened (for reference or logging)
            strncpy((char*)cam->SerialNumber, deviceSerial, 2047);
            cam->SerialNumber[2047] = '\0';

            free(deviceSerial);
            deviceSerial = NULL;
            break;  // We found the one we want; stop searching
        }
        else
        {
            // Not the one we need; close and move on
            free(deviceSerial);
            deviceSerial = NULL;
            GXCloseDevice(hTempDevice);
            hTempDevice = NULL;
        }
    }

    // If we never found the requested serial number, show an error
    if (!foundDevice)
    {
        MessagePopup("Camera Error", 
                     "No camera with the specified serial number was found!");
        return CANCEL;
    }

    return GX_STATUS_SUCCESS;
}

void CloseDevice(struct camera_s *cam) {
	
    GX_STATUS emStatus;

    // Kill the timer if running
    if (cam->timerId > 0)
        DiscardAsyncTimer(cam->timerId);

    // If snapping, stop
    if (cam->IsSnap) {
		
        emStatus = GXSendCommand(cam->Device, GX_COMMAND_ACQUISITION_STOP);
        GXUnregisterCaptureCallback(cam->Device);
        cam->IsSnap = 0;
        UnPrepareForShowImg(cam);
    }

    // If open, close
    if (cam->DevOpened) {
		
        emStatus = GXCloseDevice(cam->Device);
        cam->DevOpened = 0;
        cam->Device    = NULL;
    }
}

/***************************************************************************************************
Start / Stop Image Acquisition Functions.
****************************************************************************************************/

void StartCameraAcquisition (struct camera_s *cam) {
	
    GX_STATUS emStatus = GX_STATUS_ERROR;

    // PrepareForShowImg allocates RawBuffer & ImgBuffer, 
    // plus creates the LabWindows bitmap handle 
    if (PrepareForShowImg(cam) != OK) {
		
        MessagePopup("Camera Error", "Fail to allocate resources for image!");
        return;
    }

    // Register frame callback with cameraOne as user pointer
    emStatus = GXRegisterCaptureCallback(cam->Device, cam, OnFrameCallbackFun);
    if (emStatus != GX_STATUS_SUCCESS) {
		
        UnPrepareForShowImg(cam);
        ShowErrorString(emStatus);
        return;
    }

    // Set stream buffer handling to OLDEST_FIRST
    emStatus = GXSetEnum(cam->Device, GX_DS_ENUM_STREAM_BUFFER_HANDLING_MODE, GX_DS_STREAM_BUFFER_HANDLING_MODE_OLDEST_FIRST);
	
    if (emStatus != GX_STATUS_SUCCESS) {
		
        UnPrepareForShowImg(cam);
        ShowErrorString(emStatus);
        return;
    }

    // Send AcquisitionStart command
    emStatus = GXSendCommand(cam->Device, GX_COMMAND_ACQUISITION_START);
	
    if (emStatus != GX_STATUS_SUCCESS) {
        UnPrepareForShowImg(cam);
        ShowErrorString(emStatus);
        return;
    }

    cam->IsSnap = 1; // true
}


void StopCameraAcquisition (struct camera_s *cam) {
	
    GX_STATUS emStatus = GX_STATUS_SUCCESS;

    // Send AcquisitionStop command
    emStatus = GXSendCommand(cam->Device, GX_COMMAND_ACQUISITION_STOP);
    GX_VERIFY(emStatus);

    // Unregister frame callback
    emStatus = GXUnregisterCaptureCallback(cam->Device);
    GX_VERIFY(emStatus);
	
    // Stop the timer FIRST, so no callbacks can run
	// Kill the timer 
    if (cam->timerId > 0)
    {
        DiscardAsyncTimer(cam->timerId);
        cam->timerId = 0;
    }
	
    cam->IsSnap = 0;

    // Release memory
    UnPrepareForShowImg(cam);
}

/***************************************************************************************************
Function to save a frame to a BMP. Caution, these files are large!

Helper function to write a BMP file from a buffer that already 
has the correct row alignment/padding. 
For 24-bit color: rowBytes = ((width * 3) + 3) & ~3
For 8-bit mono:  rowBytes = (width + 3) & ~3, plus a grayscale palette.

Return 0 on success, -1 on failure
****************************************************************************************************/

int SaveBufferAsBMP(const char* fileName, struct camera_s *cam) {
	
    int width  = (int)cam->ImageWidth; 
    int height = (int)cam->ImageHeight; 
    int bitsPerPixel = cam->IsColorFilter ? 24 : 8;
    int rowBytes = cam->IsColorFilter ? (((width * 3) + 3) & ~3) : ((width + 3) & ~3);

    FILE *fp = fopen(fileName, "wb");
    if (!fp) return -1;

    uint32_t imageSize = rowBytes * height;

    // Prepare file header
    cam->BmpFile.bfType    = 0x4D42; // 'BM'
    cam->BmpFile.bfOffBits = sizeof(cam->BmpFile) + 40; // 40 = size of standard BITMAPINFOHEADER
    uint32_t paletteSize   = 0;

    // If 8-bit, we need a 256-color palette
    if (bitsPerPixel == 8) {
        paletteSize = 256 * 4;
        cam->BmpFile.bfOffBits += paletteSize; 
    }

    cam->BmpFile.bfSize      = cam->BmpFile.bfOffBits + imageSize;
    cam->BmpFile.bfReserved1 = 0;
    cam->BmpFile.bfReserved2 = 0;

    // Prepare the standard 40-byte info header
    // (remove that extra int32_t biColorTable[256] from your struct or do not rely on it)
    BmpInfoHeader infoHeader;
    memset(&infoHeader, 0, sizeof(infoHeader));
    infoHeader.biSize        = 40;        // *** Must be exactly 40 for BITMAPINFOHEADER ***
    infoHeader.biWidth       = width;
    infoHeader.biHeight      = height;
    infoHeader.biPlanes      = 1;
    infoHeader.biBitCount    = (uint16_t)bitsPerPixel;
    infoHeader.biCompression = 0;         // BI_RGB
    infoHeader.biSizeImage   = imageSize;

    // Write file header
    fwrite(&cam->BmpFile, 1, sizeof(cam->BmpFile), fp);
    // Write the standard 40-byte info header
    fwrite(&infoHeader, 1, 40, fp);

    // If 8-bit, write a grayscale palette (256 BGRA entries)
    if (bitsPerPixel == 8) {
        for (int i = 0; i < 256; i++) {
            unsigned char bgra[4];
            bgra[0] = (unsigned char)i; // B
            bgra[1] = (unsigned char)i; // G
            bgra[2] = (unsigned char)i; // R
            bgra[3] = 0;                // A (unused)
            fwrite(bgra, 1, 4, fp);
        }
    }

    // Finally write pixel data
    fwrite(cam->ImgBuffer, 1, imageSize, fp);

    fclose(fp);
    return 0;
}

/***************************************************************************************************
The GxIAPI frame callback. This is called from the camera driver thread
whenever a new frame is ready. We decode/copy the data into our 
cameraOne.ImgBuffer holds the BMP image, i.e. can be drawn to a canvas, saved to file, ETC.
****************************************************************************************************/

void GX_STDC OnFrameCallbackFun(GX_FRAME_CALLBACK_PARAM *pFrame) {
	
    if (pFrame == NULL || pFrame->status != 0) return;

    struct camera_s *cam = (struct camera_s*)pFrame->pUserParam;
    if (!cam) return; // Camera object is not passed correctly
	
    // Ensure that RawBuffer is allocated before copying
    if (cam->RawBuffer == NULL || pFrame->pImgBuf == NULL) {
        // Log the error
        MessagePopup("Error", "RawBuffer or pImgBuf is NULL. Check memory allocation.");
        return;
    }

    // Copy from driver to RawBuffer
    if (pFrame->nImgSize > 0) {
        memcpy(cam->RawBuffer, pFrame->pImgBuf, pFrame->nImgSize);
    }
	else {
        // Log an error if image size is zero or negative
        MessagePopup("Error", "Invalid image size in frame callback.");
        return;
	}
    

    int width  = (int)cam->ImageWidth; 
    int height = (int)cam->ImageHeight; 
	int bitsPerPixel = cam->IsColorFilter ? 24 : 8;
    int rowBytes = cam->IsColorFilter ? (((width * 3) + 3) & ~3) : ((width + 3) & ~3);

    if (cam->IsColorFilter) {
        // If the acquired image is color format,convert it to RGB
        DxRaw8toRGB24 (cam->RawBuffer, cam->ImgBuffer, (VxUint32)width, (VxUint32)height, RAW2RGB_NEIGHBOUR, (DX_PIXEL_COLOR_FILTER)cam->PixelColorFilter, TRUE);
    }
	
    else {
        // If the acquired image is mono format,you must flip the image data for showing.
        for (int y = 0; y < height; y++) {
            unsigned char *src = cam->RawBuffer + (size_t)(height - 1 - y) * width;
            unsigned char *dst = cam->ImgBuffer + (size_t)y * rowBytes;
            memcpy(dst, src, width);
        }
    }
	
	//static int g_frameIndex = 0;  // keeps incrementing on each callback
    //char filename[256];
    //sprintf(filename, "C:\\temp\\frame_%04d.bmp", g_frameIndex++);
	
	//int rc = SaveBufferAsBMP(filename, cam);
}

/***************************************************************************************************
Prepare/unprepare to show image
****************************************************************************************************/

int PrepareForShowImg(struct camera_s *cam) {

	//Allocate memory for getting image
    // Always allocate RawBuffer = PayLoadSize for the raw bytes
    // (which is 8 bits/pixel for color Bayer or mono).
    cam->RawBuffer = (unsigned char*)malloc((size_t)cam->PayLoadSize);
    if (!cam->RawBuffer) {
        return CANCEL; // error
    }
	
	if (cam->IsColorFilter) {
		// Allocate buffer for showing color image.
		if (PrepareForShowColorImg(cam) != OK) {
			UnPrepareForShowImg(cam);
			return CANCEL;
		}
	}
	else {
		// Allocate buffer for showing mono image.
		if (PrepareForShowMonoImg(cam) != OK) {
			UnPrepareForShowImg(cam);
			return CANCEL;
		}
	}
	
    // Compute rowBytes
    int rowBytes = 0;
    if (cam->BmpInfo.biBitCount == 24) rowBytes = ((cam->BmpInfo.biWidth * 3) + 3) & ~3;
    else rowBytes = (cam->BmpInfo.biWidth + 3) & ~3; // 8-bit
	
    size_t totalBytes = (size_t)rowBytes * cam->BmpInfo.biHeight;

    // Create the LabWindows/CVI bitmap
    int error = NewBitmap(
        rowBytes,           		// bytesPerRow (24-bit color => 3×width, aligned)
        cam->BmpInfo.biBitCount,	// 24 for color, 8 for mono
        cam->BmpInfo.biWidth,
        cam->BmpInfo.biHeight,
        cam->BmpInfo.biColorTable,  // grayscale palette if mono
        cam->ImgBuffer,     		// pointer to image data
        NULL,               		// no mask
        &cam->BitmapHandle
    );
	
    if (error < 0) {
        free(cam->RawBuffer);
        free(cam->ImgBuffer);
        cam->RawBuffer    = NULL;
        cam->ImgBuffer    = NULL;
        cam->BitmapHandle = 0;
        return CANCEL; // error
    }

    return OK; // success
}

//allocate memory for showing color image.
int PrepareForShowColorImg(struct camera_s *cam) {
	
	memset(cam->BmpBuf, 0, sizeof(cam->BmpBuf));
	
	//Initialize bitmap header
	//cam->BmpInfo					= (BITMAPINFO *)cam->BmpBuf;
	cam->BmpInfo.biSize				= sizeof(BITMAPINFOHEADER);
	cam->BmpInfo.biWidth			= (LONG)cam->ImageWidth;
	cam->BmpInfo.biHeight			= (LONG)cam->ImageHeight;	

	cam->BmpInfo.biPlanes			= 1;
	cam->BmpInfo.biBitCount			= 24; 
	cam->BmpInfo.biCompression		= BI_RGB;
	cam->BmpInfo.biSizeImage		= 0;
	cam->BmpInfo.biXPelsPerMeter	= 0;
	cam->BmpInfo.biYPelsPerMeter	= 0;
	cam->BmpInfo.biClrUsed			= 0;
	cam->BmpInfo.biClrImportant		= 0;
	
	int width = (int)cam->ImageWidth;
	int height = (int)cam->ImageHeight;
	
	// Allocate memory for showing converted color images. 3 bytes per pixel.
	cam->ImgBuffer = (unsigned char *)malloc((size_t)width * height * 3);
	
    if (cam->ImgBuffer == NULL) {
        // If allocation fails, free RawBuffer (if previously allocated)
        if (cam->RawBuffer) {
            free(cam->RawBuffer);
            cam->RawBuffer = NULL;
        }
        return CANCEL;
    }

    // Initialize the new buffer to zero
	memset(cam->ImgBuffer, 0, (size_t)(width * height * 3));

	return OK;
}

//allocate memory for showing mono image.
int PrepareForShowMonoImg(struct camera_s *cam) {
	
	memset(cam->BmpBuf, 0, sizeof(cam->BmpBuf));
	
	//Initialize bitmap header
	//cam->BmpInfo					= (BITMAPINFO *)cam->BmpBuf;
	cam->BmpInfo.biSize				= sizeof(BITMAPINFOHEADER);
	cam->BmpInfo.biWidth			= (LONG)cam->ImageWidth;
	cam->BmpInfo.biHeight			= (LONG)cam->ImageHeight;	

	cam->BmpInfo.biPlanes			= 1;
	cam->BmpInfo.biBitCount			= 8;  
	cam->BmpInfo.biCompression		= BI_RGB;
	cam->BmpInfo.biSizeImage		= 0;
	cam->BmpInfo.biXPelsPerMeter	= 0;
	cam->BmpInfo.biYPelsPerMeter	= 0;
	cam->BmpInfo.biClrUsed			= 0;
	cam->BmpInfo.biClrImportant		= 0;

	if (!cam->IsColorFilter) {
		// If using mono cameras,you need to initialize color palette first.
		for(int i = 0; i < 256; i++) {
			// MAKE_COLOR(r, g, b) -> uses 0x00BBGGRR in CVI
			cam->BmpInfo.biColorTable[i] = MAKE_COLOR(i, i, i);
		}
	}
	
	int width = (int)cam->ImageWidth;
	int height = (int)cam->ImageHeight;
	
	// Allocate memory for showing converted mono images
	cam->ImgBuffer = (unsigned char *)malloc((size_t)width * height);
	
    if (cam->ImgBuffer == NULL) {
        // If allocation fails, free RawBuffer (if previously allocated)
        if (cam->RawBuffer) {
            free(cam->RawBuffer);
            cam->RawBuffer = NULL;
        }
        return CANCEL;
    }

    // Initialize the new buffer to zero
	memset(cam->ImgBuffer, 0, (size_t)(width * height));
	
	return OK;
}

void UnPrepareForShowImg(struct camera_s *cam) {
	
    if (cam->RawBuffer) {
		
        free(cam->RawBuffer);
        cam->RawBuffer = NULL;
    }

    if (cam->ImgBuffer) {
		
        free(cam->ImgBuffer);
        cam->ImgBuffer = NULL;
    }

    // If you want, you can also discard the BitmapHandle here:
    if (cam->BitmapHandle) {
		
        DiscardBitmap(cam->BitmapHandle);
        cam->BitmapHandle = 0;
    }
}

/***************************************************************************************************
Camera Initilization.

Initilizes camera to a preset known state. Prepares it for proper image capturing.

Loads parameters from device to struct.

Loads parameters Gain and Exposure from struct to device.
****************************************************************************************************/

GX_STATUS InitDevice(struct camera_s *cam) {
	
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

    // AcquisitionMode = Continuous
    emStatus = GXSetEnum(cam->Device, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);
    VERIFY_STATUS_RET(emStatus);

    // TriggerMode = Off
    emStatus = GXSetEnum(cam->Device, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_OFF);
    VERIFY_STATUS_RET(emStatus);

    // Force camera to 8-bit pixel format
	emStatus = SetPixelFormat8bit(cam);
	VERIFY_STATUS_RET(emStatus);
	
    // Payload size
    emStatus = GXGetInt(cam->Device, GX_INT_PAYLOAD_SIZE, &cam->PayLoadSize);
    VERIFY_STATUS_RET(emStatus);

    // Width, Height
    emStatus = GXGetInt(cam->Device, GX_INT_WIDTH, &cam->ImageWidth);
    VERIFY_STATUS_RET(emStatus);

    emStatus = GXGetInt(cam->Device, GX_INT_HEIGHT, &cam->ImageHeight);
    VERIFY_STATUS_RET(emStatus);

    // IsColorFilter?
    emStatus = GXIsImplemented(cam->Device, GX_ENUM_PIXEL_COLOR_FILTER, &cam->IsColorFilter);
    VERIFY_STATUS_RET(emStatus);
	
    // ROI
    GXGetInt(cam->Device, GX_INT_AAROI_OFFSETX, &cam->RoiX);
    GXGetInt(cam->Device, GX_INT_AAROI_OFFSETY, &cam->RoiY);
    GXGetInt(cam->Device, GX_INT_AAROI_WIDTH,   &cam->RoiW);
    GXGetInt(cam->Device, GX_INT_AAROI_HEIGHT,  &cam->RoiH);

    // Gray
    GXGetInt(cam->Device, GX_INT_GRAY_VALUE, &cam->Gray);

    // If color filter, get it
    if (cam->IsColorFilter) GXGetEnum(cam->Device, GX_ENUM_PIXEL_COLOR_FILTER, &cam->PixelColorFilter);
	
	// Configure Gain Mode
	if (cam->GainMode == 0) emStatus = GXSetEnum(cam->Device, GX_ENUM_GAIN_AUTO, GX_GAIN_AUTO_OFF);
	else if (cam->GainMode == 1) emStatus = GXSetEnum(cam->Device, GX_ENUM_GAIN_AUTO, GX_GAIN_AUTO_CONTINUOUS);
	else if (cam->GainMode == 2) emStatus = GXSetEnum(cam->Device, GX_ENUM_GAIN_AUTO, GX_GAIN_AUTO_ONCE);
	else return CANCEL;
	VERIFY_STATUS_RET(emStatus);
	
	emStatus = GXSetFloat(cam->Device,GX_FLOAT_GAIN,cam->Gain);
	VERIFY_STATUS_RET(emStatus);
	
	emStatus = GXSetFloat(cam->Device, GX_FLOAT_AUTO_GAIN_MIN, cam->AutoGainMin);
	VERIFY_STATUS_RET(emStatus);
	
	emStatus = GXSetFloat(cam->Device, GX_FLOAT_AUTO_GAIN_MAX, cam->AutoGainMax);
	VERIFY_STATUS_RET(emStatus);
	
	// Configure Exposure Time Mode 
	if (cam->ExposureTimeMode == 0) emStatus = GXSetEnum(cam->Device, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_OFF);
	else if (cam->ExposureTimeMode == 1) emStatus = GXSetEnum(cam->Device, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_CONTINUOUS);
	else if (cam->ExposureTimeMode == 2) emStatus = GXSetEnum(cam->Device, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_ONCE);
	else return CANCEL;
	VERIFY_STATUS_RET(emStatus);
	
	emStatus = GXSetFloat(cam->Device, GX_FLOAT_EXPOSURE_TIME, cam->ExposureTime);
	VERIFY_STATUS_RET(emStatus);
	
	emStatus =GXSetFloat(cam->Device, GX_FLOAT_AUTO_EXPOSURE_TIME_MIN, cam->AutoExposureTimeMin);
	VERIFY_STATUS_RET(emStatus);
	
	emStatus =GXSetFloat(cam->Device, GX_FLOAT_AUTO_EXPOSURE_TIME_MAX, cam->AutoExposureTimeMax);
	VERIFY_STATUS_RET(emStatus);
	
    return emStatus;
}

/***************************************************************************************************
Set pixel format to 8-Bit
If you don't know the device supports which kind of pixel format,
you could use following function to set the pixel format to 8-bit.
****************************************************************************************************/

GX_STATUS SetPixelFormat8bit(struct camera_s *cam) {
	
    GX_STATUS emStatus   = GX_STATUS_SUCCESS;
    int64_t   nPixelSize = 0;
	uint32_t  nEnumEntry  = 0;
    size_t    nBufferSize = 0;
	
	GX_ENUM_DESCRIPTION *pEnumDescription = NULL;

    // PixelSize is bits per pixel
	// Get the feature PixelSize, this feature indicates the depth of the pixel values in the acquired images in bits per pixel
    emStatus = GXGetEnum(cam->Device, GX_ENUM_PIXEL_SIZE, &nPixelSize);
    VERIFY_STATUS_RET(emStatus);

    // If the PixelSize is 8bit then return, or set the PixelSize to 8bit.
    if (nPixelSize == GX_PIXEL_SIZE_BPP8) return GX_STATUS_SUCCESS;
	
	else {
		
	    // We want to find an 8-bit format among the supported pixel formats
		// Get the enumeration entry of the pixel format the device supports.
	    emStatus = GXGetEnumEntryNums(cam->Device, GX_ENUM_PIXEL_FORMAT, &nEnumEntry);
	    VERIFY_STATUS_RET(emStatus);

		// Allocate memory for getting the enumeration entry of the pixel format.
	    nBufferSize = nEnumEntry * sizeof(GX_ENUM_DESCRIPTION);
	    pEnumDescription = (GX_ENUM_DESCRIPTION*) malloc(nBufferSize);
	    
		if (!pEnumDescription) return CANCEL;

	    emStatus = GXGetEnumDescription(cam->Device, GX_ENUM_PIXEL_FORMAT, pEnumDescription, &nBufferSize);
	
	    if (emStatus != GX_STATUS_SUCCESS) {
			if (pEnumDescription != NULL) {
	        	free(pEnumDescription);
			}
			return emStatus;
	    }

	    // Try each supported format to see if it's 8-bit
		// A loop to visit every enumeration entry node once to find which enumeration entry node is 8bit and then set pixel format to its value
	    for (uint32_t i = 0; i < nEnumEntry; i++) {
		
	        if ((pEnumDescription[i].nValue & GX_PIXEL_8BIT) == GX_PIXEL_8BIT)
	        {
	            emStatus = GXSetEnum(cam->Device, GX_ENUM_PIXEL_FORMAT, pEnumDescription[i].nValue);
	            break;
	        }
	    }
	}

    free(pEnumDescription);
    return emStatus;
}



/***************************************************************************************************
Camera Error Handling Functions.
****************************************************************************************************/

int VERIFY_STATUS_RET (GX_STATUS emStatus) {
	
    if (emStatus != GX_STATUS_SUCCESS) return emStatus;
	
    else return GX_STATUS_SUCCESS;
}

// Show error info from GxIAPI
void ShowErrorString(GX_STATUS emErrorStatus) {
	
	char*  pchErrorInfo = NULL;
    size_t nSize        = 0;
    GX_STATUS emStatus  = GX_STATUS_ERROR;

    // Get length
    emStatus = GXGetLastError(&emErrorStatus, NULL, &nSize);
    if (emStatus != GX_STATUS_SUCCESS || nSize == 0) {
		
        MessagePopup("Error", "Failed to retrieve error info length.");
        return;
    }

    pchErrorInfo = (char*)malloc(nSize);
    if (!pchErrorInfo) {
		
        MessagePopup("Error", "Memory allocation failed for error string.");
        return;
    }

    // Get actual error info
    emStatus = GXGetLastError(&emErrorStatus, pchErrorInfo, &nSize);
    if (emStatus != GX_STATUS_SUCCESS) MessagePopup("Error", "Fail to call GXGetLastError!");
    else MessagePopup("GxIAPI Error", pchErrorInfo);

    free(pchErrorInfo);
}

/***************************************************************************************************
The async timer callback: draws the latest image in cam->ImgBuffer.

The correct panelHandle and canvasControl must be set in order to display the image!
****************************************************************************************************/

void CVICALLBACK UpdateCameraCallback(int reserved, int timerId, int event, struct camera_s *cam, int eventData1, int eventData2) {
	
    if (cam->IsSnap && cam->BitmapHandle) {
		
		// Canvas Control Information
    	int panelHandle      = cam->panelHandle;
    	int canvasControl    = cam->canvasControl;
		
        // Grab local copies (cast 64-bit to int if safe)
        int width        = (int)cam->BmpInfo.biWidth;
        int height       = (int)cam->BmpInfo.biHeight;
        int bitsPerPixel = (int)cam->BmpInfo.biBitCount;
		
        // Use GetBitmapInfo to check the nandle is truly valid
        int error = GetBitmapInfo(cam->BitmapHandle, NULL, NULL, NULL);

        if (GetBitmapInfo(cam->BitmapHandle, NULL, NULL, NULL) < 0) {
			
            // If GetBitmapInfo fails, the handle is invalid (or something is wrong)
            printf("GetBitmapInfo failed for camera's bitmap handle. Error = %d\n", error);
            return;
        }
		
        // Compute rowBytes
        int rowBytes = (bitsPerPixel == 24) ? (((width * 3) + 3) & ~3) : ((width + 3) & ~3);  // For 8-bit

        // For 8-bit images, use cameraOne.BmpInfo.biColorTable;
        // For 24-bit images, pass NULL for the colorTable parameter.
        int *colorTablePtr = (bitsPerPixel == 8) ? cam->BmpInfo.biColorTable : NULL;

        // Update the existing bitmap handle with new frame data
        // SetBitmapData signature:
        //     int SetBitmapData(int bitmapID,
        //                       int bytesPerRow,
        //                       int pixelDepth,
        //                       int colorTable[],    // or NULL for 24-bit
        //                       unsigned char bits[], // pointer to pixel data
        //                       unsigned char mask[]); // or NULL for no mask
        error = SetBitmapData(
                        cam->BitmapHandle,
                        rowBytes,
                        bitsPerPixel,
                        colorTablePtr,
                        cam->ImgBuffer,
                        NULL   // no mask
                    );
        if (error < 0)
        {
            // Not fatal, but we can log something
            printf("SetBitmapData failed: %d\n", error);
        }

        // Now draw it onto the canvas
        CanvasDrawBitmap(panelHandle,
                         canvasControl,
                         cam->BitmapHandle,
                         VAL_ENTIRE_OBJECT, // source rect
                         VAL_ENTIRE_OBJECT  // dest rect
        );
		
		
        // Draw the crosshair lines
		if (cam->UseCrosshair == 1) {
			
			int width, height, scaled_crosshair_x, scaled_crosshair_y;
			
			// Set the pen width and color.
			SetCtrlAttribute(panelHandle, canvasControl, ATTR_PEN_COLOR, cam->CrosshairColor);
			SetCtrlAttribute(panelHandle, canvasControl, ATTR_PEN_WIDTH, cam->CrosshairThickness);

		    // 1) Get the canvas width and height
		    GetCtrlAttribute (panelHandle, canvasControl, ATTR_WIDTH,  &width);
		    GetCtrlAttribute (panelHandle, canvasControl, ATTR_HEIGHT, &height);

		    // 2) Make sure the cameraOne.CrosshairX / CrosshairY are within the valid image range
		    if (cam->CrosshairX > cam->ImageWidth) cam->CrosshairX = (int)cam->ImageWidth;
		    if (cam->CrosshairY > cam->ImageHeight) cam->CrosshairY = (int)cam->ImageHeight;

		    // 3) Scale from full-resolution space [0 .. ImageWidth] to canvas space [0 .. width]
		    //    and [0 .. ImageHeight] to [0 .. height].
		    //    (Use a float cast to avoid integer division issues.)
		    scaled_crosshair_x = (int)( (float)cam->CrosshairX / (float)cam->ImageWidth  * (float)width );
		    scaled_crosshair_y = (int)( (float)cam->CrosshairY / (float)cam->ImageHeight * (float)height );

		    // 4) Clip scaled coordinates again just to ensure they are not outside the Canvas
		    if (scaled_crosshair_x < 0)        scaled_crosshair_x = 0;
		    else if (scaled_crosshair_x > width)  scaled_crosshair_x = width;

		    if (scaled_crosshair_y < 0)        scaled_crosshair_y = 0;
		    else if (scaled_crosshair_y > height) scaled_crosshair_y = height;
			
			
			// Horizontal line in cameraOne's canvas:
			CanvasDrawLine (panelHandle, canvasControl, MakePoint(scaled_crosshair_x - cam->CrosshairSize, scaled_crosshair_y), MakePoint(scaled_crosshair_x + cam->CrosshairSize, scaled_crosshair_y));
			// Vertical line in cameraOne's canvas:
			CanvasDrawLine (panelHandle, canvasControl, MakePoint(scaled_crosshair_x, scaled_crosshair_y - cam->CrosshairSize), MakePoint(scaled_crosshair_x, scaled_crosshair_y + cam->CrosshairSize));
			
			
		}
    }
}
