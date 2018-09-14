
/*
 * Origen Window Class
 * aperol
 * 2018-09-07
*/

#ifndef ORIGIN_WINDOW_H
#define ORIGIN_WINDOW_H

class originWindow
{
private:
    int win_width;                      // width of origin window
    int win_height;                     // height of origin window

    SDL_Window *sdlWindow;
    SDL_Renderer *sdlRender;

    SDL_Texture *videoTexture;          // layer 1
    SDL_Texture *rulerTexture;          // layer 2
	SDL_Texture *faceTexture;
	SDL_Texture *audioTexture;
	SDL_Texture *cropTexture;

    SDL_Rect sdlRect;                  // display position of window

    int refreshWindow(
        void *pVideoFrameBuffer,
        void *pRulerFrameBufferRGBA,
        void *pFaceFrameBuffer,
        void *pAudioFrameBuffer,
        void *CropFrameBuffer
    );                

public:
    int init(int width, int height);     // initialize origin window, create window & render
    int handleEvent(
        SDL_Event event,
        void *pVideoFrameBuffer,
        void *pRulerFrameBufferRGBA,
        void *pFaceFrameBuffer,
        void *pAudioFrameBuffer,
        void *CropFrameBuffer
        );                           // handle sdl event
    // int deInit();                                               // clear resource of origin window
};

#endif