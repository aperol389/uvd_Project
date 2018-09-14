
/*
* Distortion Window Class
* aperol
* 2018-09-12
*/

#ifndef DISTORTION_WINDOW_H
#define DISTORTION_WINDOW_H

class distortionWindow
{
private:
    int win_width;                      // width of origin window
    int win_height;                     // height of origin window

    unsigned char *pDeRenderFrameBufferRGB;   // deRenderer buffer
    unsigned char *pDeRenderFrameBufferARGB;   // deRenderer buffer
    unsigned char *pDistortionFrameBuffer;   // deRenderer buffer

    SDL_Window *sdlWindow;
    SDL_Renderer *sdlRender;

    SDL_Texture *videoTexture;          // layer 1
    SDL_Texture *rulerTexture;          // layer 2
	SDL_Texture *faceTexture;
	SDL_Texture *audioTexture;
	SDL_Texture *cropTexture;

    SDL_Texture *distortionTexture;

    SDL_Rect sdlRect;                  // display position of window

    int refreshWindow(
        void *pVideoFrameBuffer,
        void *pRulerFrameBufferRGBA,
        void *pFaceFrameBuffer,
        void *pAudioFrameBuffer,
        void *CropFrameBuffer
    );

    int convertARGBtoRGB(
        unsigned char *pArgb,
        unsigned char *pRgb,
        int w,
        int h
    );

public:
    int init(int width, int height);
    int handleEvent(
        SDL_Event event,
        void *pVideoFrameBuffer,
        void *pRulerFrameBufferRGBA,
        void *pFaceFrameBuffer,
        void *pAudioFrameBuffer,
        void *CropFrameBuffer
        );
    // int deInit();
};

#endif