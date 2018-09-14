/*
 * Copyright (c) 2018 Polycom Inc
 *
 * Customized Circle Queue
 *
 * 1. Divide enque and deque action into two parts
 *     for further more buffer operation.
 *
 * 2. Considering thread safe.
 *
 * Author: SONGYI (yi.song@polycom.com)
 * Date Created: 20180823
 */

#ifndef _MY_CIRCLE_QUE_H_
#define _MY_CIRCLE_QUE_H_

#include "circleque.h"
#include "autolock.h"

class MyCircleQue : public CircleQue
{
public:
    MyCircleQue(int maxque, int bufsize)
     : CircleQue(maxque, bufsize)
    {
    }

    virtual ~MyCircleQue()
    {
    }

    unsigned char *GetEnqueSurface()
    {
        Autolock lock(&mtx);
        if(IsFull())
        {
            //printf("error: full queue! %s\n", __FUNCTION__);
            return NULL;
        }
        else
        {
            unsigned char *surface = &buffers[rear][0];
            return surface;
        }
    }

    virtual bool Enque(unsigned char *surface)
    {
        Autolock lock(&mtx);
        if(IsFull())
        {
            printf("error: full queue! capacity=%d, size=%d, front=%d, rear=%d", 
                capacity, size, front, rear);
            //throw std::invalid_argument("full queue");
            return false;
        }
        else
        {
            if(surface == &buffers[rear][0])
            {
                rear = succ(rear);
                size++;
                return true;
            }
            else
            {
                printf("error: bad enque surface!\n");
                return false;
            }
        }
    }

    unsigned char *GetDequeSurface()
    {
        Autolock lock(&mtx);
        if(IsEmpty())
        {
            //printf("error: empty queue! %s\n", __FUNCTION__);
            return NULL;
        }
        else
        {
            unsigned char *surface = &buffers[front][0];
            return surface;
        }
    }

    // interface limitation:
    // ignore the return value and the surface should have been inputted
    // because we have to keep the same parameters in virtual function
    virtual unsigned char *Deque()
    {
        Autolock lock(&mtx);
        if(IsEmpty())
        {
            printf("error: empty queue! capacity=%d, size=%d, front=%d, rear=%d",
                capacity, size, front, rear);
            //throw std::invalid_argument("empty queue");
            return NULL;
        }
        else
        {
            unsigned char *buf = &buffers[front][0];
            front = succ(front);
            size--;
            return buf;
        }
    }
    
private:
    MutexLock mtx; // protect queue knowledge (front, rear and size)
};

#endif