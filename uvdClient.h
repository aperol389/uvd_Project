
#ifndef UVDCLIENT_H
#define UVDCLIENT_H

#include "common.h"

class uvdClient
{
private:
	char serverIP[256];

    int videoFrameBufferNumber;
    unsigned char *pVideoFrameBufferTemp;

    // RGB -> RGB_After -> RGBA
    unsigned char *pRulerFrameBufferRGB;
    unsigned char *pRulerFrameBufferRGB_After;

    unsigned char *pAudioFrameBufferRGB;

    FaceFrame faceFrame;
    int audioPosition;
    int cropPosition[4];

    originWindow myOriginWindow;
    distortionWindow myDistortionWindow;

    int currentFocusWindow; // current focus window indicate which window user indicate.

    int dropFrameNumber;

	int drawRulerFrame();
    int drawFaceFrame();
    int drawAudioFrame();
    int drawCropFrame();

    static int getVideoFrameThread(void *para);
    static int getFaceFrameThread(void *para);
    static int getAudioFrameThread(void *para);
    static int getCropFrameThread(void *para);

public:

	unsigned char *pVideoFrameBuffer;

	unsigned char *pRulerFrameBufferRGBA;

	unsigned char *pFaceFrameBuffer;

	unsigned char *pAudioFrameBuffer;

	unsigned char *pCropFrameBuffer;

	int start(char **argv);
};

#endif