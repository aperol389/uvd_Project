/*
 * Copyright (c) 2018 Polycom Inc
 *
 * Distortion Player for Utopia Debug Window
 *
 * Author: SONGYI (yi.song@polycom.com)
 * Date Created: 20180816
 */

#include <stdio.h>
#include <assert.h>
#include <sys/time.h> // gettimeofday, select
#include <math.h>
#include <string.h>
#include <unistd.h> // getpid, readlink, usleep
#include <cerrno> // errno
#include <utility> // make_pair
#include "DistortionPlayer.h"

#define MAX_QUE 5
#define IMAGE_WIDTH 1280
#define IMAGE_HEIGHT 720
#define IMAGE_BUFSIZE 2764800 // RGB24: w*h*3 = 1280*720*3
#define SCALE 3 // 2160/720  (4K resolution UHD: 3840 x 2160 pixels)

#define PLAYBACK_EVENT (SDL_USEREVENT + 100)
#define PLAYBACK_FRAME_RATE 25 // PAL 25 fps
#define DISTORTION_GAP_TIME 100

#define SCREEN_WIDTH 1920 //1600 //1920          //3840 //2560 //1280
#define SCREEN_HEIGHT 1080 //900 //1080         //2160 // 1440 //720
#define SCREEN_BUFSIZE 6220800 //4320000 //6220800   //24883200 //11059200 //2764800  RGB24

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#define Pythagorean(x,y) (sqrt(pow((x), 2) + pow((y), 2)))
const int endian = 1;
#define BIGENDIAN() ((*(char *)&endian) == 0)

enum Axis
{
    AXIS_X,
    AXIS_Y
};

struct MapEntry
{
    float first;
    float second;
};

struct Pic
{
    unsigned char *buf;
    int w;
    int h;
    int pitch;
    int ox; // origin offset
    int oy;
    float x; // coordinate
    float y;
    float r; // radius
    Pic(unsigned char *buffer, int width, int height, const int PIXELSIZE=3) // RGB24
    {
        buf = buffer;
        w = width;
        h = height;
        pitch = w * PIXELSIZE;
        ox = w / 2;
        oy = h / 2;
    }
    Pic()
    {
        buf = NULL;
        w = h = pitch = ox = oy = 0;
        x = y = r = 0.0F;
    }
    Pic& operator=(const Pic& other)
    {
        if(this != &other)
        {
            buf = other.buf;
            w = other.w;
            h = other.h;
            pitch = other.pitch;
            ox = other.ox;
            oy = other.oy;
            x = other.x;
            y = other.y;
            r = other.r;
            //memcpy(this, &other, sizeof(struct Pic));
        }
        return *this;
    }
};

struct ArgbColor
{
    unsigned char alpha;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    ArgbColor(int x)
    {
        if(BIGENDIAN())
        {
            alpha = (x >> 24) & 0xff;
            red   = (x >> 16) & 0xff;
            green = (x >> 8) & 0xff;
            blue  = x & 0xff;
            printf("IS BIG ENDIAN A%x R%x G%x B%x\n", alpha, red, green, blue);
        }
        else
        {
            alpha = x & 0xff;
            red   = (x >> 8) & 0xff;
            green = (x >> 16) & 0xff;
            blue  = (x >> 24) & 0xff;
            printf("IS LITTLE ENDIAN A%x R%x G%x B%x\n", alpha, red, green, blue);
        }
    }
};

void *DistortionProcess(void *context);
void *PlaybackVideo(void *context);


DistortionPlayer::DistortionPlayer(int workmode)
 : webcamque(MAX_QUE, IMAGE_BUFSIZE),
   distortionque(MAX_QUE, SCREEN_BUFSIZE),
   distortion_thread(DistortionProcess, this),
   playback_thread(PlaybackVideo, this)
{
    // init members
    mode = workmode;
    image_width = IMAGE_WIDTH;
    image_height = IMAGE_HEIGHT;
    screen_width = SCREEN_WIDTH;
    screen_height = SCREEN_HEIGHT;

    InitDistortionMap();

    //... add anything else later ...
    rgbtmpbuf.resize(IMAGE_BUFSIZE);

    playback_time = 1000 / PLAYBACK_FRAME_RATE;
    distortion_gap_time = DISTORTION_GAP_TIME;

    // create sdl window
    sdlwnd = NULL;
    sdlrender = NULL;
    sdltexture = NULL;

    if(mode == PLAYERWND)
        CreateSDLWindow();

    // start up working threads
    if(mode == ASYNCHRO || mode == PLAYERWND)
    {
        distortion_thread.start();
        playback_thread.start();
    }
}

DistortionPlayer::~DistortionPlayer()
{
    // stop working threads
    if(mode == ASYNCHRO || mode == PLAYERWND)
    {
        distortion_thread.stop();
        playback_thread.stop();
    }

    // destory sdl window
    if(mode == PLAYERWND)
        DestroySDLWindow();
}

bool DistortionPlayer::CreateSDLWindow()
{
    SDL_Init(SDL_INIT_VIDEO);

    sdlwnd = SDL_CreateWindow("Utopia Distortion Player",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        screen_width,
        screen_height,
        SDL_WINDOW_OPENGL);

    if(NULL == sdlwnd)
    {
        printf("error: failed to create SDL window: %s\n", SDL_GetError());
        goto exit_with_err;
    }

    sdlrender = SDL_CreateRenderer(sdlwnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if(NULL == sdlrender)
    {
        printf("error: failed to create SDL render: %s\n", SDL_GetError());
        goto exit_with_err;
    }

    sdltexture = SDL_CreateTexture(sdlrender, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_TARGET, screen_width, screen_height);

    if(NULL == sdltexture)
    {
        printf("error: failed to create SDL texture: %s\n", SDL_GetError());
        goto exit_with_err;
    }

    return true;

exit_with_err:
    DestroySDLWindow();
    return false;
}

void DistortionPlayer::DestroySDLWindow()
{
    if(sdltexture)
    {
        SDL_DestroyTexture(sdltexture);
        sdltexture = NULL;
    }

    if(sdlrender)
    {
        SDL_DestroyRenderer(sdlrender);
        sdlrender = NULL;
    }

    if(sdlwnd)
    {
        SDL_DestroyWindow(sdlwnd);
        sdlwnd = NULL;
    }

    SDL_Quit();
}

bool DistortionPlayer::CorrectImage(unsigned char *src_buf,
                                    int src_width,
                                    int src_height,
                                    unsigned char *dst_buf,
                                    int dst_width,
                                    int dst_height)
{
    if(NULL == src_buf || NULL == dst_buf)
        return false;
    NV12_to_RGB24(src_buf, &rgbtmpbuf[0], src_width, src_height);
    distortion_correction(&rgbtmpbuf[0], src_width, src_height, dst_buf, dst_width, dst_height, REVERSE);
    return true;
}

bool DistortionPlayer::CorrectImageRGB(unsigned char *src_buf,
                                      int src_width,
                                      int src_height,
                                      unsigned char *dst_buf,
                                      int dst_width,
                                      int dst_height)
{
    if(NULL == src_buf || NULL == dst_buf)
        return false;
    distortion_correction(src_buf, src_width, src_height, dst_buf, dst_width, dst_height, REVERSE);
    return true;
}

bool DistortionPlayer::DistortImageRGB(unsigned char *src_buf,
                                      int src_width,
                                      int src_height,
                                      unsigned char *dst_buf,
                                      int dst_width,
                                      int dst_height)
{
    if(NULL == src_buf || NULL == dst_buf)
        return false;
    distortion_correction(src_buf, src_width, src_height, dst_buf, dst_width, dst_height, FORWARD);
    return true;
}

bool DistortionPlayer::CorrectXLine(unsigned char *buf, int pos, int pixelwidth, int color)
{
    if(!(NULL != buf && pos >= 0 && pixelwidth > 0))
        return false;
    line_correction(buf, pos, pixelwidth, color, AXIS_X, FORWARD);
    return true;
}

bool DistortionPlayer::DistortXLine(unsigned char *buf, int pos, int pixelwidth, int color)
{
    if(!(NULL != buf && pos >= 0 && pixelwidth > 0))
        return false;
    line_correction(buf, pos, pixelwidth, color, AXIS_X, REVERSE);
    return true;
}

bool DistortionPlayer::CorrectYLine(unsigned char *buf, int pos, int pixelwidth, int color)
{
    if(!(NULL != buf && pos >= 0 && pixelwidth > 0))
        return false;
    line_correction(buf, pos, pixelwidth, color, AXIS_Y, FORWARD);
    return true;
}

bool DistortionPlayer::DistortYLine(unsigned char *buf, int pos, int pixelwidth, int color)
{
    if(!(NULL != buf && pos >= 0 && pixelwidth > 0))
        return false;
    line_correction(buf, pos, pixelwidth, color, AXIS_Y, REVERSE);
    return true;
}

bool DistortionPlayer::PushImage(unsigned char *buf)
{
    /*// Active dropping frame ???
    if(webcamque.IsFull())
    {
        webcamque.Deque();
        printf("error: drop frame! %s\n", __FUNCTION__);
    }*/

    unsigned char *surface = webcamque.GetEnqueSurface();
    if(surface == NULL)
    {
        printf("warning: failed to get enque surface!fed too many images~\n");
        return false;
    }

    if(!NV12_to_RGB24(buf, surface, image_width, image_height))
    {
        printf("error: failed to convert nv12 to rgb24!\n");
        return false;
    }

    webcamque.Enque(surface);

    return true;
}

void DistortionPlayer::Play()
{
    distortion_thread.resume();
    playback_thread.resume();
}

void DistortionPlayer::Pause()
{
    distortion_thread.suspend();
    playback_thread.suspend();
}

void *DistortionProcess(void *context)
{
    DistortionPlayer *thisptr = (DistortionPlayer *)context;
    SDL_Event event;
    unsigned long tick1, tick2;
    static unsigned int i = 0;
    struct timeval tv;
    unsigned char *buf = NULL;
    unsigned char *surface = NULL;

    ++i;
    tick1 = thisptr->GetTickCount();

    // get a webcam buffer
    while((buf = thisptr->webcamque.GetDequeSurface()) == NULL)
    {
        printf("warning: no any webcam image to process...\n");
        tv.tv_sec = 0;
        tv.tv_usec = thisptr->distortion_gap_time;
        select(0, NULL, NULL, NULL, &tv);
    }

    // get a distortion surface
    while((surface = thisptr->distortionque.GetEnqueSurface()) == NULL)
    {
        thisptr->distortionque.Deque(); // drop one frame not played
        printf("warning: dropping unplayed frame at %u, Playback is stuck or too slow~\n", i);
    }

    tick2 = thisptr->GetTickCount();

    // distortion
    if(buf != NULL && surface != NULL)
    {
        // do convert
        thisptr->distortion_correction(buf,
            thisptr->image_width,
            thisptr->image_height,
            surface,
            thisptr->screen_width,
            thisptr->screen_height,
            REVERSE);

        // release webcam que surface
        thisptr->webcamque.Deque();
        // release distortion que surface
        thisptr->distortionque.Enque(surface);
        // send play event
        if(thisptr->mode == PLAYERWND)
        {
            event.type = PLAYBACK_EVENT;
            SDL_PushEvent(&event);
        }
    }

    // statistics of CPU time
    printf("info: distortion loop %u wait surface %u correction cost %u\n",
            i,
            (unsigned int)(tick2 - tick1),
            (unsigned int)(thisptr->GetTickCount() - tick2));

    return NULL;
}

void *PlaybackVideo(void *context)
{
    DistortionPlayer *thisptr = (DistortionPlayer *)context;
    int pitch = 0;
    unsigned char *buf = NULL;
    SDL_Event event;
    SDL_Rect target_rect;
    int ret;

    //ret = SDL_WaitEvent(&event);
    while(ret = SDL_WaitEventTimeout(&event, thisptr->playback_time))
    {
        if(ret == 1)
        {
            if(event.type == PLAYBACK_EVENT)
                break;
            else
            {
                printf("warning: received unkonwn SDL event: %d\n", event.type);
                continue;
            }
        }
        else // 0
        {
            printf("failed to wait SDL event or timeout: %s\n", SDL_GetError());
            break;
        }
    }

    // try to get a corrected frame
    // consider timestamp ? - expending buffer structure
    if((buf = thisptr->distortionque.GetDequeSurface()) != NULL)
    {
        pitch = thisptr->screen_width * 3; // window may resize

        target_rect.x = 0;
        target_rect.y = 0;
        target_rect.w = thisptr->screen_width;
        target_rect.h = thisptr->screen_height;

        // display frame
        SDL_RenderClear(thisptr->sdlrender);
        SDL_UpdateTexture(thisptr->sdltexture, NULL, buf, pitch);
        SDL_RenderCopy(thisptr->sdlrender, thisptr->sdltexture, NULL, &target_rect);
        SDL_RenderPresent(thisptr->sdlrender);

        // release surface
        thisptr->distortionque.Deque();
    }

    return NULL;
}

bool DistortionPlayer::InitDistortionMap()
{
    float A[] = {
        0.000,           0.000,
        21.89988272, 19.30226677,
        43.80081481, 38.61107981,
        65.70383951, 57.93299426,
        87.61,       77.27458305,
        109.5203457, 96.64244588,
        131.4359321, 116.0432182,
        153.3578148, 135.4835805,
        175.287037,  154.9702674,
        197.2246728, 174.5100771,
        219.1717778, 194.1098811,
        241.1294198, 213.776634,
        263.098679,  233.5173836,
        285.0806296, 253.3392812,
        307.0763642, 273.2495923,
        329.0869691, 293.2557075,
        351.1135494, 313.3651542,
        373.157216,  333.585608,
        395.2190864, 353.9249054,
        417.3002901, 374.3910563,
        439.401963,  394.9922575,
        461.5252654, 415.7369064,
        483.6713519, 436.6336158,
        505.8413951, 457.6912293,
        528.0365988, 478.9188372,
        550.2581605, 500.3257931,
        572.5073025, 521.9217325,
        594.7852593, 543.7165909,
        617.093284,  565.7206239,
        639.4326543, 587.944428,
        661.8046543, 610.3989635,
        684.2105926, 633.0955772,
        706.6518086, 656.0460281,
        729.1296543, 679.2625139,
        751.6454938, 702.7576991,
        774.2007284, 726.5447459,
        796.7967778, 750.6373455,
        819.4350802, 775.0497536,
        842.1171111, 799.7968265,
        864.8443519, 824.8940609,
        887.6183272, 850.3576362,
        910.4405741, 876.2044597,
        933.3126605, 902.4522156,
        956.2361728, 929.1194173,
        979.2127284, 956.2254637,
        1002.243969, 983.7907006,
        1025.331549, 1011.836485,
        1048.477167, 1040.385259,
        1071.682512, 1069.460621,
        1080,        1079.995658,
        1094.949321, 1098.34762,
        1118.27934,  1128.466731,
        1141.674327, 1158.591998,
        1165.136056, 1188.739045,
        1188.666315, 1218.9248,
        1212.266901, 1249.167619,
        1235.939611, 1279.487408,
        1259.686259, 1309.905759,
        1283.508636, 1340.446119,
        1307.408537, 1371.133948,
        1331.387747, 1401.996915,
        1355.448025, 1433.065108,
        1379.591111, 1464.371265,
        1403.818722, 1495.951034,
        1428.132519, 1527.84326,
        1452.534136, 1560.090304,
        1477.025142, 1592.738398,
        1501.607043, 1625.838048,
        1526.281278, 1659.444467,
        1551.049198, 1693.618081,
        1575.912062, 1728.425077,
        1600.871031, 1763.938027,
        1625.927142, 1800.23659,
        1651.08129,  1837.408311,
        1676.334259, 1875.549474,
        1701.686642, 1914.766151,
        1727.138883, 1955.175304,
        1752.691235, 1996.906087,
        1778.343753, 2040.101307,
        1804.096278, 2084.919101,
        1829.948407, 2131.534849,
        1855.899519, 2180.14334,
        1867.037037, 2201.682771,
        1881.94871,  2230.961317,
        1908.094802, 2284.230378,
        1934.336346, 2340.220304,
        1960.671568, 2399.233009,
        1987.098383, 2461.607067,
        2013.614389, 2527.723027,
        2040.216815, 2598.00974,
        2066.902568, 2672.95169,
        2093.668173, 2753.097859,
        2120.509802, 2839.072179,
        2147.423241, 2931.586184,
        2174.403914, 3031.454187,
        2201.446858, 3139.611827,
        2228.546741, 3257.138696,
        2255.69787,  3385.286236,
        2282.894191, 3525.512444,
        2310.12929,  3679.52533,
        2337.396451, 3849.337772,
        2364.688623, 4037.337716,
        2391.998488, 4246.378565
    };

    int i;
    int N = sizeof(A) / sizeof(float);
    std::vector<float> first;
    std::vector<float> second;

    N /= 2;
    first.resize(N);
    second.resize(N);

    for(i = 0; i < N; i++)
    {
        first[i] = A[i*2]/SCALE;
        second[i] = A[i*2+1]/SCALE;
    }

#ifdef _BINARY_SEARCH_TREE_
    distortion_tree.SmartInsert(&second[0], &first[0], N);
    reverse_distortion_tree.SmartInsert(&first[0], &second[0], N);
    printf("Tree depth: %d\n", distortion_tree.Depth());
    distortion_tree.Draw();
#endif

    for(i = 0; i < N; i++)
    {
        distortion_map.insert(std::pair<float, float>(first[i], second[i]));
        reverse_distortion_map.insert(std::pair<float, float>(second[i], first[i]));
    }

    return true;
}

void BilinearInterpolation(unsigned char *dst, Pic &src)
{
    int x1, x2;
    int y1, y2;
    //float coeff;
    float k[4];

    // source image coordinate translation
    float x = src.x + src.ox;
    float y = src.y + src.oy;

    if(x < 0 || x > src.w || y < 0 || y > src.h)
        return; // out of range

    x1 = (int)x; // x1 must be less than x
    x2 = x1 +1;

    y1 = (int)y;
    y2 = y1 + 1;

    //coeff = 1.0 / ((x2 - x1) * (y2 - y1));
    k[0] = (x2 - x) * (y2 - y);
    k[1] = (x - x1) * (y2 - y);
    k[2] = (x2 - x) * (y - y1);
    k[3] = (x - x1) * (y - y1);

    //r = coeff*(src[pitch*y1+3*x1]*k[0] + src[pitch*y1+3*x2]*k[1] + src[pitch*y2+3*x1]*k[2] + src[pitch*y2+3*x2]*k[3]);
    dst[0] = src.buf[src.pitch*y1+3*x1]*k[0] + src.buf[src.pitch*y1+3*x2]*k[1] + src.buf[src.pitch*y2+3*x1]*k[2] + src.buf[src.pitch*y2+3*x2]*k[3];
    dst[1] = src.buf[src.pitch*y1+3*x1+1]*k[0] + src.buf[src.pitch*y1+3*x2+1]*k[1] + src.buf[src.pitch*y2+3*x1+1]*k[2] + src.buf[src.pitch*y2+3*x2+1]*k[3];
    dst[2] = src.buf[src.pitch*y1+3*x1+2]*k[0] + src.buf[src.pitch*y1+3*x2+2]*k[1] + src.buf[src.pitch*y2+3*x1+2]*k[2] + src.buf[src.pitch*y2+3*x2+2]*k[3];
}

void DistortionPlayer::distortion_correction(unsigned char *src_buf, int src_w, int src_h, unsigned char *dst_buf, int dst_w, int dst_h, int maptype)
{
    int i, j;
    float slope;
    std::map<float, float>::iterator itlow, itup;
    std::map<float, float>& rallymap = (maptype == FORWARD) ? distortion_map : reverse_distortion_map;
    struct MapEntry low, up;

    std::map<float, float>::iterator itlasthit = rallymap.end(); // initial
    bool positive = false;

    struct Pic src(src_buf, src_w, src_h), dst(dst_buf, dst_w, dst_h);
    int half_dw = dst.w / 2;
    int half_dh = dst.h / 2;

    memset(&low, 0, sizeof(low));
    memset(&up, 0, sizeof(up));
    memset(dst.buf, 0, dst.pitch*dst.h); // init 0

    // look up table and translate pixels
    for(i = 0; i < half_dh; i++)// dh
    {
        for(j = 0; j < half_dw; j++) // dw
        {
            // #coordinate translation
            dst.x = j - dst.ox;
            dst.y = i - dst.oy;

            // #circle radius
            dst.r = Pythagorean(dst.x, dst.y);

            // #scale up // did in map
            //r *= SCALE;

            //#look up table
            if(dst.r < low.first || dst.r > up.first) // optimization 1, by reducing queries
            {
#ifdef _BINARY_SEARCH_TREE_
                distortion_tree.Search(dst.r, low.first, low.second, up.first, up.second);
#else
                if(itlasthit == itup) // optimization 2, forecast the next query (save 10 milliseconds)
                {
                    positive ? itlasthit++ : itlasthit--;
                    itup = itlasthit;
                    if(itup == rallymap.begin())
                        itup++;
                    itlow = itup;
                    itlow--;

                    low.first = itlow->first;
                    low.second = itlow->second;
                    up.first = itup->first;
                    up.second = itup->second;
                }

                if(dst.r < low.first || dst.r > up.first)
                {
                    //[[ BEGIN to look up
                    itup = rallymap.upper_bound(dst.r);  // return map::end if failed
                    if(itup == rallymap.begin())
                        itup++;
                    itlow = itup;
                    itlow--;

                    low.first = itlow->first;
                    low.second = itlow->second;
                    up.first = itup->first;
                    up.second = itup->second;
                    //]] END

                    positive = itup->first > itlasthit->first ? true : false;
                    itlasthit = itup;
                }
#endif
                //printf("info: %s mapping entry [%f, %f] - [%f, %f]\n", __FUNCTION__, low.first, low.second, up.first, up.second);

            }

            // #linear interpolation as approximation
            //r2 = (itup->second*(r-itlow->first)+itlow->second*(itup->first-r))/(itup->first-itlow->first);
            src.r = (up.second * (dst.r - low.first) + low.second * (up.first - dst.r)) / (up.first - low.first);

            // #calculating the slope
            slope = src.r / dst.r;
            src.x = dst.x * slope;
            src.y = dst.y * slope;
            //if(i==0&&j==0) printf("slope=%f, r2=%f, x2=%f, y2=%f\n",  slope, r2, x2, y2);

            /*
            // #coordinate translation
            x2 += ox;
            y2 += oy;
            // #check range
            x2 += 0.5;
            x2 = min(x2, w);
            x2 = max(0, x2);
            y2 += 0.5;
            y2 = min(y2, h);
            y2 = max(0, y2);

            // #storage 888 pixels
            for(k = 0; k < 3; k++)
                dst[pitch*i + 3*j + k] = src[pitch*(int)y2 + 3*(int)x2 + k];
            */

            // #bilinear interpolation
            BilinearInterpolation(&dst.buf[dst.pitch*i+3*j], src);

            // optimizationn 3, symmetry point
            src.y *= (-1.0F);
            BilinearInterpolation(&dst.buf[dst.pitch*(dst.h - i - 1)+3*j], src);

            src.x *= (-1.0F);
            BilinearInterpolation(&dst.buf[dst.pitch*(dst.h - i - 1)+3*(dst.w - j - 1)], src);

            src.y *= (-1.0F);
            BilinearInterpolation(&dst.buf[dst.pitch*i+3*(dst.w - j - 1)], src);
        }
    }

    return;
}

bool DistortionPlayer::NV12_to_RGB24(unsigned char* yuv,unsigned char* rgb,int width,int height)
{
    if (width < 1 || height < 1 || yuv == NULL || rgb == NULL)
        return false;
    const long len = width * height;
    unsigned char* yData = yuv;
    unsigned char* uData = &yData[len];
    unsigned char* vData = &yData[len];
    int i, j, k;
    int buf[3];
    int yIdx,uIdx,vIdx,idx;
    for (i = 0;i < height;i++){
        for (j = 0;j < width;j++){
            yIdx = i * width + j;
            uIdx = (i/2) * (width/2) + (j/2);
            uIdx *= 2;
            vIdx = uIdx+1;

            buf[0] = (int)(yData[yIdx] + 1.370705 * (vData[uIdx] - 128));
            buf[1] = (int)(yData[yIdx] - 0.698001 * (uData[uIdx] - 128) - 0.703125 * (vData[vIdx] - 128));
            buf[2] = (int)(yData[yIdx] + 1.732446 * (uData[vIdx] - 128));

            for (k = 0;k < 3;k++){
                idx = (i * width + j) * 3 + k;
                if(buf[k] >= 0 && buf[k] <= 255)
                    rgb[idx] = buf[k];
                else
                    rgb[idx] = (buf[k] < 0)?0:255;
            }
        }
    }
    return true;
}

void DistortionPlayer::WriteBmp(const char *path, unsigned char *buf, int w, int h)
{
    typedef struct {
        short twobyte;
        char bfType[2];
        unsigned int bfSize;
        unsigned int bfReserved1;
        unsigned int bfOffBits;
    } BmpHeader;

    typedef struct {
        unsigned int biSize;
        int biWidth;
        int biHeight;
        unsigned short biPlanes;
        unsigned short biBitCount;
        unsigned int biCompression;
        unsigned int biSizeImage;
        int biXPelsPerMeter;
        int biYPelsPerMeter;
        unsigned int biClrUsed;
        unsigned int biClrImportant;
    } BmpInfo;

    FILE *fp = 0;
    BmpHeader head;
    BmpInfo info;
    int len = 0, i = 0, picture_size = 0;
    const char pad = 0;

    do
    {
        if (path == NULL || w <= 0 || h <= 0 || buf == NULL)
        {
            printf("<%s> bad parameters\n", __FUNCTION__);
            break;
        }

        if ((fp = fopen(path, "w")) == NULL)
        {
            printf("<%s> failed to open %s error: %s\n", __FUNCTION__, path, strerror(errno));
            break;
        }

        picture_size = w * h * 3; // RGB24
        len = picture_size;
        memset(&head, 0, sizeof(head));
        memset(&info, 0, sizeof(info));
        head.bfType[0] = 'B';
        head.bfType[1] = 'M';
        head.bfOffBits = 14 + sizeof(info);
        head.bfSize = head.bfOffBits + len;
        head.bfSize = (head.bfSize + 3) & ~3;
        len = head.bfSize - head.bfOffBits;

        info.biSize = sizeof(BmpInfo);
        info.biWidth = w;
        info.biHeight = -h;
        info.biPlanes = 1;
        info.biBitCount = 24;
        info.biCompression = 0L; //BI_RGB;
        info.biSizeImage = len;

        fwrite(head.bfType, sizeof(char), 14, fp);
        fwrite(&info, sizeof(char), sizeof(info), fp);
        fwrite(buf, sizeof(char), picture_size, fp);
        for(i = 0; i < len - picture_size; i++)
	    fwrite(&pad, 1, 1, fp);
    } while (0);

    if(fp)
    {
        fclose(fp);
        fp = NULL;
    }
}

unsigned long DistortionPlayer::GetTickCount()
{
    struct timespec now;
    if(clock_gettime(CLOCK_MONOTONIC, &now))
        return 0;
    return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

int DistortionPlayer::GetExeDir(char *buf, int len)
{
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "/proc/%d/exe", getpid());
    int bytes = readlink(tmp, buf, len);
    bytes = min(bytes, len - 1);
    if(bytes >= 0)
        buf[bytes] = '\0';
    char * pch;
    pch=strrchr(buf,'/');
    *pch = '\0';
    return bytes;
}

float DistortionPlayer::fast_sqrt(float x)
{
    unsigned int x_bits = 0;
    x_bits = *(unsigned int *)&x;
    x_bits = (x_bits >> 1) + 532369198;
    return *((float *)&x_bits);
}

float DistortionPlayer::Q_rsqrt( float number )
{
    long i;
    float x2, y;
    const float threehalfs = 1.5F;

    x2 = number * 0.5F;
    y  = number;
    i  = * ( long * ) &y;                       // evil floating point bit level hacking
    i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
    //y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

    return y;
}

float SquareRoot(float number)
{
    long i;
    float x, y;
    const float f = 1.5F;

    x = number * 0.5F;
    y = number;
    i = *(long *)&y;
    i = 0x5f3759df - (i>>1);
    y = *(float *)&i;
    y = y*(f - (x*y*y));
    y = y*(f - (x*y*y));
    return number*y;
}

float DistortionPlayer::Pythagorean2(float x, float y)
{
    return fast_sqrt(pow(x, 2) + pow(y, 2));
    return 1.0F/Q_rsqrt(pow(x, 2) + pow(y, 2));
}

void DistortionPlayer::line_correction(unsigned char *buf, int pos, int pixelwidth, int color, int axis, int maptype)
{
    int i, j;
    float slope;
    std::map<float, float>::iterator itlow, itup;
    std::map<float, float>& rallymap = (maptype == FORWARD) ? distortion_map : reverse_distortion_map;
    struct MapEntry low, up;

    struct Pic src, dst;
    struct ArgbColor col(color);

    int hbegin, hend;
    int wbegin, wend;

    typedef struct segment_point{
        int x;
        int y;
        segment_point():x(0),y(0){}
    } Point;

    Point segment_start, segment_end;
    int k, offset, pt;
    int tmp;

    memset(&low, 0, sizeof(low));
    memset(&up, 0, sizeof(up));

    if(maptype == FORWARD) {
        src = Pic(NULL, image_width, image_height);
        dst = Pic(buf, screen_width, screen_height);
        memset(dst.buf, 0, dst.pitch*dst.h);
    } else {
        src = Pic(NULL, screen_width, screen_height);
        dst = Pic(buf, image_width, image_height);
        memset(dst.buf, 0, dst.pitch*dst.h);
    }
    
    for (pt = 0; pt < pixelwidth; pt++)
    {
        if(axis == AXIS_X)
        {
            hbegin = 0;
            hend = src.h / 2; // half of height, use symmetry
            wbegin = pos + pt;
            wend = pos + pt + 1;
        }
        else
        {
            hbegin = pos + pt;
            hend = pos + pt + 1;
            wbegin = 0;
            wend = src.w / 2;
        }

        for(j = hbegin; j < hend; j++)
        {
            for(i = wbegin; i < wend; i++)
            {
                src.x = i - src.ox;
                src.y = j - src.oy;
                src.r = Pythagorean(src.x, src.y);

                //printf("-src...................... %f, %f\n", src.x, src.y);

                if(src.r < low.first || src.r > up.first)
                {
                    itup = rallymap.upper_bound(src.r);
                    if(itup == rallymap.begin())
                        itup++;
                    itlow = itup;
                    itlow--;

                    low.first = itlow->first;
                    low.second = itlow->second;
                    up.first = itup->first;
                    up.second = itup->second;
                }

                dst.r = (up.second * (src.r - low.first) + low.second * (up.first - src.r)) / (up.first - low.first);
                slope = dst.r / src.r;
                dst.x = src.x * slope + dst.ox;
                dst.y = src.y * slope + dst.oy;

                if(dst.x < 0.0F || dst.x > (float)dst.ox || dst.y < 0.0F || dst.y > (float)dst.oy)
                    continue;

                //printf("-dst...................... %f, %f\n", dst.x, dst.y);

                if(axis == AXIS_X)
                {
                    /*
                    if(j == 0 && i == 0)
                    {
                        segment_start.x = floor(dst.x);
                        segment_start.y = floor(dst.y);
                        continue;
                    }

                    tmp = floor(dst.x);

                    if(segment_start.x != tmp)
                    {
                        // keep end point
                        segment_end.y = floor(dst.y);
                        printf("................. [%d, %d] ~ [%d, %d], new x=%f\n", 
                            segment_start.x, segment_start.y, segment_end.x, segment_end.y, floor(dst.x));

                        //if(abs(segment_end.y - hend) < 5)
                            //segment_end.y = hend;
                        // draw line segment
                        for(k = segment_start.y; k < segment_end.y; k++)
                        {
                            offset = dst.pitch * k + 3 * segment_start.x;
                            dst.buf[offset] = col.red;
                            dst.buf[offset + 1] = col.green;
                            dst.buf[offset + 2] = col.blue;
                            // symmetrical
                            offset = dst.pitch * (dst.h - 1 - k) + 3 * segment_start.x;
                            dst.buf[offset] = col.red;
                            dst.buf[offset + 1] = col.green;
                            dst.buf[offset + 2] = col.blue;
                        }

                        // save new starting point
                        segment_start.x = floor(dst.x);
                        segment_start.y = floor(dst.y);
                    }
*/
                    
                    offset = dst.pitch * (int)(dst.y + 0.5F) + 3 * (int)(dst.x + 0.5F);
                    dst.buf[offset] = col.red;
                    dst.buf[offset + 1] = col.green;
                    dst.buf[offset + 2] = col.blue;
                    // symmetrical
                    offset = dst.pitch * (dst.h - 1 - (int)(dst.y + 0.5F)) + 3 * (int)(dst.x + 0.5F);
                    dst.buf[offset] = col.red;
                    dst.buf[offset + 1] = col.green;
                    dst.buf[offset + 2] = col.blue;
                    
                }
                else
                {
                    /*
                    if(segment_start.y != (floor(dst.y) + dst.oy))
                    {
                        segment_end.x = floor(dst.x) + dst.ox;
                        if(abs(segment_end.x - wend) < 5)
                            segment_end.x = wend;
                        for(k = segment_start.x; k < segment_end.x; k++)
                        {
                            offset = dst.pitch * segment_start.y + 3 * k;
                            dst.buf[offset] = col.red;
                            dst.buf[offset + 1] = col.green;
                            dst.buf[offset + 2] = col.blue;
                            // symmetrical
                            offset = dst.pitch * segment_start.y + 3 * (dst.w - 1 - k);
                            dst.buf[offset] = col.red;
                            dst.buf[offset + 1] = col.green;
                            dst.buf[offset + 2] = col.blue;
                        }
                        segment_start.x = floor(dst.x) + dst.ox;
                        segment_start.y = floor(dst.y) + dst.oy;
                    }
                    */

                    offset = dst.pitch * (int)(dst.y + 0.5F) + 3 * (int)(dst.x + 0.5F);
                    dst.buf[offset] = col.red;
                    dst.buf[offset + 1] = col.green;
                    dst.buf[offset + 2] = col.blue;
                    // symmetrical
                    offset = dst.pitch * (int)(dst.y + 0.5F) + 3 * (dst.w - 1 - (int)(dst.x + 0.5F));
                    dst.buf[offset] = col.red;
                    dst.buf[offset + 1] = col.green;
                    dst.buf[offset + 2] = col.blue;
                }
            }
        }
    }
}


/****************************************************/
/* Unit Test */

/*
#define UNIT_TEST
#ifdef UNIT_TEST

int main(int argc, char *argv[])
{
    const int MAX_PATH = 260;
    const int NV12BUFSIZE = IMAGE_BUFSIZE/2;

    int i, bytes;

    char cwd[MAX_PATH];
    char path[MAX_PATH];
    memset(cwd, 0, sizeof(cwd));
    memset(path, 0, sizeof(path));

    std::vector<unsigned char> image;
    std::vector<unsigned char> bmpbuf;
    image.resize(NV12BUFSIZE);
    bmpbuf.resize(SCREEN_BUFSIZE);

    DistortionPlayer player(BLOCKING); //PLAYERWND

    player.GetExeDir(cwd, MAX_PATH);
    printf("current working directory: %s\n", cwd);

    std::vector<unsigned char> tmp;
    tmp.resize(IMAGE_BUFSIZE);

#ifdef IGNORE
    // test player
    snprintf(path, sizeof(path), "%s/1280_720_utopia.nv12", cwd);
    FILE *fp = fopen(path, "r");

    for(i = 1; (fp != NULL) && (feof(fp) == 0); i++)
    {
        printf("processing frame number: %d\n", i);

        if((bytes = fread(&image[0], sizeof(unsigned char), NV12BUFSIZE, fp)) != NV12BUFSIZE)
        {
            printf("warning: failed to read file %s, numread=%d\n", path, bytes);
            break;
        }

        while(!player.PushImage(&image[0]))
            usleep(200000);

        ////////////////////////////////
        if(i == 1)
        {
            player.CorrectImage(&image[0], IMAGE_WIDTH, IMAGE_HEIGHT, &bmpbuf[0], SCREEN_WIDTH, SCREEN_HEIGHT);
            snprintf(path, sizeof(path), "%s/test.bmp", cwd);
            player.WriteBmp(path, &bmpbuf[0], SCREEN_WIDTH, SCREEN_HEIGHT);
        }

        if(i == 3)
            player.Play();
    }
   
    if(fp != NULL)
    {
        fclose(fp);
        fp = NULL;
    }

    // test reverse convertion
    snprintf(path, sizeof(path), "%s/test.bmp", cwd);
    FILE *h = fopen(path, "r");
    bytes = fread(&bmpbuf[0], sizeof(unsigned char), SCREEN_BUFSIZE, h);
    fclose(h);
    h = NULL;
    player.DistortImageRGB(&bmpbuf[0], SCREEN_WIDTH, SCREEN_HEIGHT, &tmp[0], IMAGE_WIDTH, IMAGE_HEIGHT);
    snprintf(path, sizeof(path), "%s/test2.bmp", cwd);
    player.WriteBmp(path, &tmp[0], IMAGE_WIDTH, IMAGE_HEIGHT);
#endif

    // test line corrcetion
    snprintf(path, sizeof(path), "%s/test3.bmp", cwd);
    if(1) {
        player.DistortXLine(&tmp[0], SCREEN_WIDTH/2, 1, 0xff000000);
        //player.DistortYLine(&tmp[0], SCREEN_HEIGHT/4, 1, 0xff000000);
        player.WriteBmp(path, &tmp[0], IMAGE_WIDTH, IMAGE_HEIGHT);
    } else {
        //player.CorrectXLine(&bmpbuf[0], IMAGE_WIDTH/4, 1, 0xff000000);
        player.CorrectYLine(&bmpbuf[0], IMAGE_HEIGHT/4, 1, 0xff000000);
        player.WriteBmp(path, &bmpbuf[0], SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    return 0;
}

#endif
*/