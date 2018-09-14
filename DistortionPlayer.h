/*
 * Copyright (c) 2018 Polycom Inc
 *
 * Distortion Player for Utopia Debug Window
 *
 * Author: SONGYI (yi.song@polycom.com)
 * Date Created: 20180816
 */

#ifndef _DISTORTION_PLAYER_H_
#define _DISTORTION_PLAYER_H_

#include <map>
#include <vector>
#include "mycircleque.h"
#include "mythread.h"
//#include "bst.h"
#include "SDL2/SDL.h"

enum MapType
{
    FORWARD,
    REVERSE
};

enum WorkMode
{
    BLOCKING = 1,
    ASYNCHRO,
    PLAYERWND
};

class DistortionPlayer
{
public:
    DistortionPlayer(int workmode = BLOCKING);
    virtual ~DistortionPlayer();

public:
    /*
     * Correct a distorted image
     * blocking mode
     *
     *  src_buf:        source NV12 image buffer
     *  src_width:     width of source image e.g. 1280
     *  src_height:    height of source image e.g. 720
     *  dst_buf:        dest RGB24 bitmap buffer
     *  dst_width:     width of dest image e.g. 1920
     *  dst_height:    height of dest image e.g. 1080
     *
     *  support a different w*h resolution
     *
     *  returns:  true if success, false otherwise
     */
    bool CorrectImage(unsigned char *src_buf,
                      int src_width,
                      int src_height,
                      unsigned char *dst_buf,
                      int dst_width,
                      int dst_height);

    /*
     * Correct a RGB image
     * blocking mode
     *
     * the same as CorrectImage, except input RGB24
     */
    bool CorrectImageRGB(unsigned char *src_buf,
                        int src_width,
                        int src_height,
                        unsigned char *dst_buf,
                        int dst_width,
                        int dst_height);

    /*
     * Reverse convertion of CorrctImage
     * blocking mode
     *
     *  src_buf: source RGB24 buffer
     *  src_width: width of source image e.g. 1920
     *  src_height: height of source iamge e.g. 1080
     *  dst_buf: dest RGB24 buffer
     *  dst_width: widht of dest image e.g. 1280
     *  dst_height: height of dest image e.g. 720
     *
     *  returns:  true if success, false otherwise
     */
    bool DistortImageRGB(unsigned char *src_buf,
                        int src_width,
                        int src_height,
                        unsigned char *dst_buf,
                        int dst_width,
                        int dst_height);

    /*
     * lines correction
     * 
     *  buf: dest buffer
     *  pos: start position (left-top is zero)
     *  pixelwidth: how many pixels are in the line
     *  color: bit operation 4 bytes = [alpha|red|green|blue]
     */
    bool CorrectXLine(unsigned char *buf, int pos, int pixelwidth, int color);
    bool DistortXLine(unsigned char *buf, int pos, int pixelwidth, int color);
    bool CorrectYLine(unsigned char *buf, int pos, int pixelwidth, int color);
    bool DistortYLine(unsigned char *buf, int pos, int pixelwidth, int color);

    /*
     * Push back a image into queue
     * asynchronous mode
     *
     *  buf:        NV12 image buffer (only support 1280*720 for now)
     *
     *  if necessary, we can add a callback to return result (RGB24,1080p)
     *
     *  returns:  true if success, false otherwise
     */
    bool PushImage(unsigned char *buf);

    /*
     * Control to start processing distortion correction
     */
    void Play();

    /*
     * Control to pause processing distortion correction
     *  note: you can not stop and let it running
     */
    void Pause();

public:
    bool NV12_to_RGB24(unsigned char* yuv,unsigned char* rgb,int width,int height);
    void WriteBmp(const char *path, unsigned char *buf, int w, int h);
    unsigned long GetTickCount(void);
    int GetExeDir(char *buf, int len);

protected:
    bool InitDistortionMap();
    bool CreateSDLWindow();
    void DestroySDLWindow();

    friend void *DistortionProcess(void *context);
    friend void *PlaybackVideo(void *context);

private:
    void distortion_correction(unsigned char *src_buf, int src_w, int src_h, unsigned char *dst_buf, int dst_w, int dst_h, int maptype);
    void line_correction(unsigned char *buf, int pos, int pixelwidth, int color, int axis, int maptype);
    
    float fast_sqrt(float x);
    float Q_rsqrt(float number);
    float Pythagorean2(float x, float y);

private:
    std::map<float, float> distortion_map;
    std::map<float, float> reverse_distortion_map;
    std::vector<unsigned char> rgbtmpbuf;

#ifdef _BINARY_SEARCH_TREE_
    BinarySearchTree distortion_tree;
    BinarySearchTree reverse_distortion_tree;
#endif

    MyCircleQue webcamque;  // input buffer fed by socket
    MyCircleQue distortionque; // output buffer of distorted image

    MyThread distortion_thread;
    MyThread playback_thread;

    SDL_Window *sdlwnd;
    SDL_Renderer *sdlrender;
    SDL_Texture *sdltexture;

    int mode;

    int image_width;  // source image
    int image_height;
    int screen_width; // destination screen
    int screen_height;

    int playback_time; // milliseconds to wait
    int distortion_gap_time;
};

#endif
