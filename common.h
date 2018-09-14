
/*
*
* aperol.chen@polycom.com
* 2018-09-11
*
*/

#ifndef COMMON_H
#define COMMON_H

#include "string"
#include "sys/socket.h"
#include "sys/unistd.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "SDL2/SDL.h"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"

#include "originWindow.h"
#include "distortionWindow.h"
#include "DistortionPlayer.h"

using namespace std;
using namespace cv;

#define PIXEL_W 1280
#define PIXEL_H 720
#define VIDEO_FRAME_BUFFER_NUMBER 5
#define VIDEO_FRAME_SIZE_NV12 1382400	// 1280 * 720 * 1.5
#define VIDEO_FRAME_SIZE_RGB 2764800	// 1280 * 720 * 3
#define VIDEO_FRAME_SIZE_RGBA 3686400	// 1280 * 720 * 4

#define VIDEO_PORT 5881
#define FACE_PORT 5882
#define AUDIO_PORT 5883
#define CROP_PORT 5884

#define MAX_FACE 256

#define REFRESH_EVENT (SDL_USEREVENT + 1)

typedef struct FaceFrame {
	int faceNumber;
	int facePosition[MAX_FACE][4];
}_FaceFrame;

#endif