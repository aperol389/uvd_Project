
#include "common.h"
#include "distortionWindow.h"

extern "C" pthread_mutex_t gMutex;
extern "C" DistortionPlayer gDistortionPlayer;

int distortionWindow::init(int width, int height)
{
    this->win_width = width;
    this->win_height = height;

    this->sdlRect.x = 0;
    this->sdlRect.y = 0;
    this->sdlRect.w = this->win_width;
    this->sdlRect.h = this->win_height;

    this->pDeRenderFrameBufferARGB = (unsigned char *)malloc(VIDEO_FRAME_SIZE_RGBA);
    this->pDeRenderFrameBufferRGB = (unsigned char *)malloc(VIDEO_FRAME_SIZE_RGB);
    this->pDistortionFrameBuffer = (unsigned char *)malloc(1920 * 1080 * 3);

    this->sdlWindow = SDL_CreateWindow(
        "Utopia Debug Window - Distortion Window",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        this->win_width,
        this->win_height,
        SDL_WINDOW_OPENGL);
    if (this->sdlWindow == NULL)
    {
        SDL_Log("create distortion window failed, error info: %s", SDL_GetError());
        return -1;
    }
    
    this->sdlRender = SDL_CreateRenderer(
        this->sdlWindow,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (this->sdlRender == NULL)
    {
        SDL_Log("create render failed, error info: %s", SDL_GetError());
        return -1;
    }

    this->videoTexture = SDL_CreateTexture(
        this->sdlRender,
        SDL_PIXELFORMAT_NV12,
        SDL_TEXTUREACCESS_STREAMING,
        this->win_width,
        this->win_height);
    if (this->videoTexture == NULL)
    {
        SDL_Log("create video texture failed, error info: %s", SDL_GetError());
        return -1;
    }

    this->rulerTexture = SDL_CreateTexture(
        this->sdlRender,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        this->win_width,
        this->win_height);
    if (this->rulerTexture == NULL)
    {
        SDL_Log("create ruler texture failed, error info: %s", SDL_GetError());
        return -1;
    }
    SDL_SetTextureBlendMode(this->rulerTexture, SDL_BLENDMODE_BLEND);


	this->faceTexture = SDL_CreateTexture(
		this->sdlRender,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_STREAMING,
		this->win_width,
		this->win_height);
	if (this->faceTexture == NULL)
	{
		SDL_Log("create face texture failed, error info: %s", SDL_GetError());
		return -1;
	}
    SDL_SetTextureBlendMode(this->faceTexture, SDL_BLENDMODE_BLEND);

	this->audioTexture = SDL_CreateTexture(
		this->sdlRender,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_STREAMING,
		this->win_width,
		this->win_height);
	if (this->audioTexture == NULL)
	{
		SDL_Log("create audio texture failed, error info: %s", SDL_GetError());
		return -1;
	}
    SDL_SetTextureBlendMode(this->audioTexture, SDL_BLENDMODE_BLEND);

	this->cropTexture = SDL_CreateTexture(
		this->sdlRender,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_STREAMING,
		this->win_width,
		this->win_height);
	if (this->cropTexture == NULL)
	{
		SDL_Log("create crop texture failed, error info: %s", SDL_GetError());
		return -1;
	}
    SDL_SetTextureBlendMode(this->cropTexture, SDL_BLENDMODE_BLEND);

	this->distortionTexture = SDL_CreateTexture(
		this->sdlRender,
		SDL_PIXELFORMAT_RGB24,
		SDL_TEXTUREACCESS_STREAMING,
        1920,
		1080);
	if (this->distortionTexture == NULL)
	{
		SDL_Log("create ditortion texture failed, error info: %s", SDL_GetError());
		return -1;
    }

    SDL_RenderClear(this->sdlRender);

    return 0;
}

int distortionWindow::handleEvent(
    SDL_Event event,
    void *pVideoFrameBuffer,
    void *pRulerFrameBufferRGBA,
    void *pFaceFrameBuffer,
    void *pAudioFrameBuffer,
    void *pCropFrameBuffer
    )
{
    if (event.type == SDL_WINDOWEVENT)
    {
        switch(event.window.event)
        {
            case SDL_WINDOWEVENT_FOCUS_GAINED:
            SDL_Log("SDL_WINDOWEVENT_FOCUS_GAINED");
            break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
            SDL_Log("SDL_WINDOWEVENT_FOCUS_LOST");
            break;
        }
    }
    else if (event.type == REFRESH_EVENT)
    {
        SDL_Log("REFRESH_EVENT");
        this->refreshWindow(
            pVideoFrameBuffer,
            pRulerFrameBufferRGBA,
            pFaceFrameBuffer,
            pAudioFrameBuffer,
            pCropFrameBuffer 
        );
    }
    else
    {
        SDL_Log("Unknown SDL Event.");
    }

    return 0;
}

int distortionWindow::refreshWindow(
    void *pVideoFrameBuffer,
    void *pRulerFrameBufferRGBA,
    void *pFaceFrameBuffer,
    void *pAudioFrameBuffer,
    void *pCropFrameBuffer
    )
{
    SDL_RenderClear(this->sdlRender);

    pthread_mutex_lock(&gMutex);
    SDL_UpdateTexture(this->videoTexture, NULL, pVideoFrameBuffer, this->win_width);
    pthread_mutex_unlock(&gMutex);
    SDL_RenderCopy(this->sdlRender, this->videoTexture, NULL, &this->sdlRect);

    // update ruler
    SDL_UpdateTexture(this->rulerTexture, NULL, pRulerFrameBufferRGBA, this->win_width * 4);
    SDL_RenderCopy(this->sdlRender, this->rulerTexture, NULL, &this->sdlRect);

    // update face
    SDL_UpdateTexture(this->faceTexture, NULL, pFaceFrameBuffer, this->win_width * 4);
    SDL_RenderCopy(this->sdlRender, this->faceTexture, NULL, &this->sdlRect);

    // update audio
    SDL_UpdateTexture(this->audioTexture, NULL, pAudioFrameBuffer, this->win_width * 4);
    SDL_RenderCopy(this->sdlRender, this->audioTexture, NULL, &this->sdlRect);

    // update crop
	SDL_UpdateTexture(this->cropTexture, NULL, pCropFrameBuffer, this->win_width * 4);
    SDL_RenderCopy(this->sdlRender, this->cropTexture, NULL, &this->sdlRect);

    //
    SDL_RenderReadPixels(this->sdlRender, &this->sdlRect, SDL_PIXELFORMAT_ARGB8888, this->pDeRenderFrameBufferARGB, this->win_width * 4);
    this->convertARGBtoRGB(this->pDeRenderFrameBufferARGB, this->pDeRenderFrameBufferRGB, this->win_width, this->win_height);
    gDistortionPlayer.CorrectImageRGB(this->pDeRenderFrameBufferRGB, this->win_width, this->win_height, this->pDistortionFrameBuffer, 1920, 1080);
    SDL_UpdateTexture(this->distortionTexture, NULL, this->pDistortionFrameBuffer, 1920 * 3);

    SDL_RenderClear(this->sdlRender);
    SDL_RenderCopy(this->sdlRender, this->distortionTexture, NULL, &this->sdlRect);

    // show
    SDL_RenderPresent(this->sdlRender);

    return 0;
}

int distortionWindow::convertARGBtoRGB(
    unsigned char *pArgb,
    unsigned char *pRgb,
    int w,
    int h
    )
{
    for (int i=0; i< w*h; i++)
    {
        pRgb[3*i] = pArgb[4*i+2];
        pRgb[3*i + 1] = pArgb[4*i+1];
        pRgb[3*i + 2] = pArgb[4*i];
    }
    return 0;
}