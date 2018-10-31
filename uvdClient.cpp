
#include "uvdClient.h"

pthread_mutex_t gMutex;
DistortionPlayer gDistortionPlayer;

int uvdClient::getVideoFrameThread(void *para)
{
    uvdClient *pUvdClient = (uvdClient *)para;
    
    struct sockaddr_in video_address;
    int video_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    video_address.sin_family = AF_INET;
    video_address.sin_addr.s_addr = inet_addr(pUvdClient->serverIP);
    video_address.sin_port = htons(VIDEO_PORT);

    SDL_Event event;

    if (connect(video_sockfd, (struct sockaddr *)&video_address, sizeof(sockaddr_in)) == -1)
    {
        SDL_Log("video socket connect error.");
        event.type = SDL_QUIT;
        SDL_PushEvent(&event);
        return -1;
    }
    else
    {
        SDL_Log("video socket connect success.");
    }

    while(1)
    {
        if (recv(video_sockfd, (char *)pUvdClient->pVideoFrameBufferTemp, VIDEO_FRAME_SIZE_NV12, MSG_WAITALL) == VIDEO_FRAME_SIZE_NV12)
        {
            pthread_mutex_lock(&gMutex);
            memcpy(pUvdClient->pVideoFrameBuffer, pUvdClient->pVideoFrameBufferTemp, VIDEO_FRAME_SIZE_NV12);
            pthread_mutex_unlock(&gMutex);
            event.type = REFRESH_EVENT;
            SDL_PushEvent(&event);
        }
        else
        {
            SDL_Log("video socket, do not receive enough bytes, error.");
            event.type = SDL_QUIT;
            SDL_PushEvent(&event);
            break;
        }
    }

    return 0;
}

int uvdClient::drawCropFrame()
{
    int left, top, right, bottom;
    memset(this->pCropFrameBuffer, 0x00, VIDEO_FRAME_SIZE_RGBA);
    
    if (this->cropPosition[3] != 0)
    {
        left = this->cropPosition[0];
        top = this->cropPosition[1];
        right = this->cropPosition[2];
        bottom = this->cropPosition[3];

        for (int j = top; j < bottom; j++)
        {
            for (int k = left; k < right; k++)
            {
                if (k - left < 4 || right - k < 4 || j - top < 4 || bottom - j < 4)
                {
                    this->pCropFrameBuffer[(j * PIXEL_W + k) * 4 + 0] = 0xff;
                    this->pCropFrameBuffer[(j * PIXEL_W + k) * 4 + 1] = 0xff;
                    this->pCropFrameBuffer[(j * PIXEL_W + k) * 4 + 2] = 0x00;
                    this->pCropFrameBuffer[(j * PIXEL_W + k) * 4 + 3] = 0x00;
                }
            }
        }
    }

    return 0;
}

int uvdClient::drawAudioFrame()
{
    // draw audio
    memset(this->pAudioFrameBufferRGB, 0x00, VIDEO_FRAME_SIZE_RGB);
    memset(this->pAudioFrameBuffer, 0x00, VIDEO_FRAME_SIZE_RGBA);
    
    // this->audioPosition = 500;

/*
    gDistortionPlayer.DistortXLine(this->pAudioFrameBufferRGB, this->audioPosition * 3 / 2, 4, 0xffff00ff);

    for (int i = 0; i < PIXEL_W; i++)
    {
        for (int j = 0; j < PIXEL_H; j++)
        {
            if (this->pAudioFrameBufferRGB[(j * PIXEL_W + i) * 3 + 0] > 40 ||
                this->pAudioFrameBufferRGB[(j * PIXEL_W + i) * 3 + 1] > 40 ||
                this->pAudioFrameBufferRGB[(j * PIXEL_W + i) * 3 + 2] > 40)
            {
                this->pAudioFrameBuffer[(j * PIXEL_W + i) * 4 + 0] = 0xa0;
            }
            else
            {
                this->pAudioFrameBuffer[(j * PIXEL_W + i) * 4 + 0] = 0x00;
            }
            this->pAudioFrameBuffer[(j * PIXEL_W + i) * 4 + 1] = this->pAudioFrameBufferRGB[(j * PIXEL_W + i) * 3 + 0];
            this->pAudioFrameBuffer[(j * PIXEL_W + i) * 4 + 2] = this->pAudioFrameBufferRGB[(j * PIXEL_W + i) * 3 + 1];
            this->pAudioFrameBuffer[(j * PIXEL_W + i) * 4 + 3] = this->pAudioFrameBufferRGB[(j * PIXEL_W + i) * 3 + 2];
        }
    }
    */

    if (this->audioPosition < 3 || this->audioPosition > 1280)
    {
        SDL_Log("Audio Position < 3 or > 1280");
        return -1;
    }

    for (int i=(this->audioPosition - 2); i<this->audioPosition + 2; i++)
    {
        for (int j=0; j<PIXEL_H; j++)
        {
            this->pAudioFrameBuffer[(j * PIXEL_W + i) * 4 + 0] = 0x7f;
            this->pAudioFrameBuffer[(j * PIXEL_W + i) * 4 + 1] = 0xff;
            this->pAudioFrameBuffer[(j * PIXEL_W + i) * 4 + 2] = 0xff;
            this->pAudioFrameBuffer[(j * PIXEL_W + i) * 4 + 3] = 0x00;
        }
    }

    return 0;
}

int uvdClient::drawFaceFrame()
{
    // draw face
    memset(this->pFaceFrameBuffer, 0x00, VIDEO_FRAME_SIZE_RGBA);

    int left, top, right, bottom;
    for (int i = 0; i < this->faceFrame.faceNumber; i++)
    {
        left = this->faceFrame.facePosition[i][0];
        top = this->faceFrame.facePosition[i][1];
        right = this->faceFrame.facePosition[i][2];
        bottom = this->faceFrame.facePosition[i][3];

        for (int j = top; j < bottom; j++)
        {
            for (int k = left; k < right; k++)
            {
                this->pFaceFrameBuffer[(j * PIXEL_W + k) * 4 + 1] = 0x00;
                this->pFaceFrameBuffer[(j * PIXEL_W + k) * 4 + 2] = 0xff;
                this->pFaceFrameBuffer[(j * PIXEL_W + k) * 4 + 3] = 0x00;
                if (this->pFaceFrameBuffer[(j * PIXEL_W + k) * 4 + 0] == 0xff)
                {
                    this->pFaceFrameBuffer[(j * PIXEL_W + k) * 4 + 0] = 0xff;
                }
                else
                {
                    this->pFaceFrameBuffer[(j * PIXEL_W + k) * 4 + 0] = 0x44;
                }

                if (k - left < 2 || right - k < 2 || j - top < 2 || bottom - j < 2)
                {
                    this->pFaceFrameBuffer[(j * PIXEL_W + k) * 4 + 0] = 0xff;
                    this->pFaceFrameBuffer[(j * PIXEL_W + k) * 4 + 1] = 0x00;
                    this->pFaceFrameBuffer[(j * PIXEL_W + k) * 4 + 2] = 0xff;
                    this->pFaceFrameBuffer[(j * PIXEL_W + k) * 4 + 3] = 0x00;
                }
            }
        }
    }

    Mat src(PIXEL_H, PIXEL_W, CV_8UC4, this->pFaceFrameBuffer);

    for (int i = 0; i < this->faceFrame.faceNumber; i ++)
    {
        string strInfo  = "(";
        strInfo += std::to_string(this->faceFrame.facePosition[i][0]);
        strInfo += ",";
        strInfo += std::to_string(this->faceFrame.facePosition[i][1]);
        strInfo += ",";
        strInfo += std::to_string(this->faceFrame.facePosition[i][2] - this->faceFrame.facePosition[i][0]);
        strInfo += ",";
        strInfo += std::to_string(this->faceFrame.facePosition[i][3] - this->faceFrame.facePosition[i][1]);
        strInfo += ")";
        putText(src, strInfo, Point(this->faceFrame.facePosition[i][0], this->faceFrame.facePosition[i][1] - 10), FONT_HERSHEY_SIMPLEX, 0.5, cvScalar(255, 0, 255, 0), 2, 4);
    }

    return 0;
}

int uvdClient::drawRulerFrame()
{
    // draw ruler
    int left, top, right, bottom;
    for (int i=0; i<11; i++)
    {
        left = PIXEL_W / 2 - 1 + 112 * (i-5);
        top = 0;
        right = PIXEL_W / 2 + 112 * (i-5);
        bottom = PIXEL_H;
        for (int j=top; j<bottom; j++)
        {
            for (int k=left; k<right; k++)
            {
                this->pRulerFrameBufferRGB[(j * PIXEL_W + k) * 3 + 0] = 0xff;
                this->pRulerFrameBufferRGB[(j * PIXEL_W + k) * 3 + 1] = 0x00;
                this->pRulerFrameBufferRGB[(j * PIXEL_W + k) * 3 + 2] = 0xff;
            }
        }
    }

    for (int i=0; i<11; i++)
    {
        left = 0;
        top = PIXEL_H / 2 - 1 + 63 * (i-5);
        right = PIXEL_W;
        bottom = PIXEL_H / 2 + 63 * (i-5);
        for (int j=top; j<bottom; j++)
        {
            for (int k=left; k<right; k++)
            {
                this->pRulerFrameBufferRGB[(j * PIXEL_W + k) * 3 + 0] = 0xff;
                this->pRulerFrameBufferRGB[(j * PIXEL_W + k) * 3 + 1] = 0x00;
                this->pRulerFrameBufferRGB[(j * PIXEL_W + k) * 3 + 2] = 0xff;
            }
        }
    }

    left = PIXEL_W / 2 - 4;
    top = PIXEL_H / 2 - 4;
    right = PIXEL_W / 2 + 4;
    bottom = PIXEL_H / 2 + 4;
    for (int j=top; j<bottom; j++)
    {
        for (int k=left; k<right; k++)
        {
            this->pRulerFrameBufferRGB[(j * PIXEL_W + k) * 3 + 0] = 0xff;
            this->pRulerFrameBufferRGB[(j * PIXEL_W + k) * 3 + 1] = 0x00;
            this->pRulerFrameBufferRGB[(j * PIXEL_W + k) * 3 + 2] = 0xff;
        }
    }

    gDistortionPlayer.DistortImageRGB(this->pRulerFrameBufferRGB, PIXEL_W, PIXEL_H, this->pRulerFrameBufferRGB_After, PIXEL_W, PIXEL_H);
    for (int i=0; i<PIXEL_H; i++)
    {
        for (int j=0; j<PIXEL_W; j++)
        {
            this->pRulerFrameBufferRGBA[(i * PIXEL_W + j) * 4 + 1] = this->pRulerFrameBufferRGB_After[(i * PIXEL_W + j) * 3 + 0];
            this->pRulerFrameBufferRGBA[(i * PIXEL_W + j) * 4 + 2] = this->pRulerFrameBufferRGB_After[(i * PIXEL_W + j) * 3 + 1];
            this->pRulerFrameBufferRGBA[(i * PIXEL_W + j) * 4 + 3] = this->pRulerFrameBufferRGB_After[(i * PIXEL_W + j) * 3 + 2];

            if ( this->pRulerFrameBufferRGB_After[(i * PIXEL_W + j) * 3 + 0] > 0x40  ||
                 this->pRulerFrameBufferRGB_After[(i * PIXEL_W + j) * 3 + 1] > 0x40  ||
                 this->pRulerFrameBufferRGB_After[(i * PIXEL_W + j) * 3 + 2] > 0x40 )
            {   
                this->pRulerFrameBufferRGBA[(i * PIXEL_W + j) * 4 + 0] = 0x80;
            }
            else
            {
                this->pRulerFrameBufferRGBA[(i * PIXEL_W + j) * 4 + 0] = 0x00;
            }
        }
    }

    return 0;
}

int uvdClient::getFaceFrameThread(void *para)
{
    uvdClient *pUvdClient = (uvdClient *)para;
    struct sockaddr_in face_address;
    int face_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    face_address.sin_family = AF_INET;
    face_address.sin_addr.s_addr = inet_addr(pUvdClient->serverIP);
    face_address.sin_port = htons(FACE_PORT);

    SDL_Event event;
    
    if (connect(face_sockfd, (struct sockaddr *)&face_address, sizeof(sockaddr_in)) == -1)
    {
        SDL_Log("face socket connect error.");
        event.type = SDL_QUIT;
        SDL_PushEvent(&event);
        return -1;
    }
    else
    {
        SDL_Log("face socket connect success.");
    }

    while(1)
    {
        if (recv(face_sockfd, (char *)&(pUvdClient->faceFrame), sizeof(FaceFrame), MSG_WAITALL) == sizeof(FaceFrame))
        {
            SDL_Log("faceNumber: %d, facePosition[0][0]: %d", pUvdClient->faceFrame.faceNumber, pUvdClient->faceFrame.facePosition[0][0]);
            pUvdClient->drawFaceFrame();
            event.type = REFRESH_EVENT;
            SDL_PushEvent(&event);
        }
    }
    
    return 0;
}

int uvdClient::getAudioFrameThread(void *para)
{
    uvdClient *pUvdClient = (uvdClient *)para;
    struct sockaddr_in audio_address;
    int audio_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    audio_address.sin_family = AF_INET;
    audio_address.sin_addr.s_addr = inet_addr(pUvdClient->serverIP);
    audio_address.sin_port = htons(AUDIO_PORT);

    SDL_Event event;

    if (connect(audio_sockfd, (struct sockaddr *)&audio_address, sizeof(sockaddr_in)) == -1)
    {
        SDL_Log("audio socket connect error.");
        event.type = SDL_QUIT;
        SDL_PushEvent(&event);
        return -1;
    }
    else
    {
        SDL_Log("audio socket connect success.");
    }

    while(1)
    {
        if (recv(audio_sockfd, (char *)&(pUvdClient->audioPosition), sizeof(int), MSG_WAITALL) == sizeof(int))
        {
            SDL_Log("audio position: %d", pUvdClient->audioPosition);
            pUvdClient->drawAudioFrame();
            event.type = REFRESH_EVENT;
            SDL_PushEvent(&event);
        }
    }

    return 0;
}

int uvdClient::getCropFrameThread(void *para)
{
	uvdClient *pUvdClient = (uvdClient *)para;
	struct sockaddr_in crop_address;
	int crop_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	crop_address.sin_family = AF_INET;
	crop_address.sin_addr.s_addr = inet_addr(pUvdClient->serverIP);
	crop_address.sin_port = htons(CROP_PORT);

	SDL_Event event;
	
	if (connect(crop_sockfd, (struct sockaddr *)&crop_address, sizeof(sockaddr_in)) == -1)
	{
		SDL_Log("crop socket connect error.");
		event.type = SDL_QUIT;
		SDL_PushEvent(&event);
		return -1;
	}
	else
	{
		SDL_Log("crop socket connect success");
	}
	
	while (1)
	{
		if (recv(crop_sockfd, (char *)pUvdClient->cropPosition, sizeof(int) * 4, MSG_WAITALL) == sizeof(int) * 4)
		{
            SDL_Log("cropPosition[0]: %d", pUvdClient->cropPosition[0]);
			pUvdClient->drawCropFrame();
			event.type = REFRESH_EVENT;
			SDL_PushEvent(&event);
		}
	}
}

int uvdClient::start(char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
        SDL_Log("SDL_Init() Error, error info: %s", SDL_GetError());
        return -1;
    }

    pthread_mutex_init(&gMutex, NULL);

    memset(this->serverIP, 0x00, 256);
	memcpy(this->serverIP, argv[1], strlen(argv[1]));

    this->faceFrame.faceNumber = 0;
    this->audioPosition = 0;
    this->cropPosition[0] = 0;

    this->currentFocusWindow = 0;
    this->dropFrameNumber = 0;

    this->pVideoFrameBuffer = (unsigned char *)malloc(VIDEO_FRAME_SIZE_NV12 * VIDEO_FRAME_BUFFER_NUMBER);
    if (this->pVideoFrameBuffer == NULL)
    {
        SDL_Log("malloc video frame buffer error.");
        return -1;
    }

    this->pVideoFrameBufferTemp = (unsigned char *)malloc(VIDEO_FRAME_SIZE_NV12);
    if (this->pVideoFrameBufferTemp == NULL)
    {
        SDL_Log("malloc video frame buffer temp error.");
        return -1;
    }

    this->pRulerFrameBufferRGB = (unsigned char *)malloc(VIDEO_FRAME_SIZE_RGB);
    if (this->pRulerFrameBufferRGB == NULL)
    {
        SDL_Log("malloc ruler frame buffer RGB error.");
        return -1;
    }

    this->pRulerFrameBufferRGB_After = (unsigned char *)malloc(VIDEO_FRAME_SIZE_RGB);
    if (this->pRulerFrameBufferRGB_After == NULL)
    {
        SDL_Log("malloc ruler frame buffer RGB After error.");
        return -1;
    }

    this->pRulerFrameBufferRGBA = (unsigned char *)malloc(VIDEO_FRAME_SIZE_RGBA);
    if (this->pRulerFrameBufferRGBA == NULL)
    {
        SDL_Log("malloc ruler frame buffer RGBA error.");
        return -1;
    }

    this->pFaceFrameBuffer = (unsigned char *)malloc(VIDEO_FRAME_SIZE_RGBA);
    if (this->pFaceFrameBuffer == NULL)
    {
        SDL_Log("malloc face frame buffer error.");
        return -1;
    }
    
    this->pAudioFrameBuffer = (unsigned char *)malloc(VIDEO_FRAME_SIZE_RGBA);
    if (this->pAudioFrameBuffer == NULL)
    {
        SDL_Log("malloc audio frame buffer error.");
        return -1;
    }

    this->pAudioFrameBufferRGB = (unsigned char *)malloc(VIDEO_FRAME_SIZE_RGB);
    if (this->pAudioFrameBufferRGB == NULL)
    {
        SDL_Log("malloc audio frame buffer rgb error.");
        return -1;
    }

    this->pCropFrameBuffer = (unsigned char *)malloc(VIDEO_FRAME_SIZE_RGBA);
    if (this->pCropFrameBuffer == NULL)
    {
        SDL_Log("malloc crop frame buffer error.");
        return -1;
    }

    this->myOriginWindow.init(PIXEL_W, PIXEL_H);
    this->myDistortionWindow.init(PIXEL_W, PIXEL_H);

    this->drawRulerFrame();
    // unsigned long tick1 = gDistortionPlayer.GetTickCount();
    // this->drawAudioFrame();
    // SDL_Log("audio draw cost time: %d", gDistortionPlayer.GetTickCount() - tick1);

    SDL_Thread *video_socket_thread = SDL_CreateThread(this->getVideoFrameThread, NULL, this);
    SDL_Thread *face_socket_thread = SDL_CreateThread(this->getFaceFrameThread, NULL, this);
    SDL_Thread *audio_socket_thread = SDL_CreateThread(this->getAudioFrameThread, NULL, this);
    SDL_Thread *crop_socket_thread = SDL_CreateThread(this->getCropFrameThread, NULL, this);

    SDL_Event event;

    // event.type = REFRESH_EVENT;
    // SDL_PushEvent(&event);

    while(1)
    {
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT)
        {
            SDL_Log("SDL_QUIT.");
            break;
        }
        else if (event.type == REFRESH_EVENT)
        {
            switch(this->currentFocusWindow)
            {
                case 0:
                this->myOriginWindow.handleEvent(
                    event, 
                    this->pVideoFrameBuffer,
                    this->pRulerFrameBufferRGBA,
                    this->pFaceFrameBuffer,
                    this->pAudioFrameBuffer,
                    this->pCropFrameBuffer
                    );
                this->myDistortionWindow.handleEvent(
                    event,
                    this->pVideoFrameBuffer,
                    this->pRulerFrameBufferRGBA,
                    this->pFaceFrameBuffer,
                    this->pAudioFrameBuffer,
                    this->pCropFrameBuffer
                    );
                break;

                case 1:
                this->myOriginWindow.handleEvent(
                    event, 
                    this->pVideoFrameBuffer,
                    this->pRulerFrameBufferRGBA,
                    this->pFaceFrameBuffer,
                    this->pAudioFrameBuffer,
                    this->pCropFrameBuffer
                    );
                this->myDistortionWindow.handleEvent(
                    event,
                    this->pVideoFrameBuffer,
                    this->pRulerFrameBufferRGBA,
                    this->pFaceFrameBuffer,
                    this->pAudioFrameBuffer,
                    this->pCropFrameBuffer
                    );
                break;
        }
        }
        else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
        {
            SDL_Log("SDL_WINDOWEVENT_FOCUS_GAINED, which window: %d", event.window.windowID);
            this->currentFocusWindow = event.window.windowID - 2;
        }
        else
        {
            // SDL_Log("Other SDL Event, Do not care.");
        }
    }

    return 0;
}