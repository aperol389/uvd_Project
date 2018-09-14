/*
 * Copyright (c) 2018 Polycom Inc
 *
 * Circle Queue
 *
 * Author: SONGYI (yi.song@polycom.com)
 * Date Created: 20180816
 */

#ifndef _CIRCLE_QUEUE_H_
#define _CIRCLE_QUEUE_H_

#include <stdlib.h>
#include <vector>
#include <stdexcept> // throw
#include <cstring> // memcpy

class CircleQue
{
public:
    CircleQue(int maxque, int bufsize)
    {
        capacity = maxque;
        buffers.resize(maxque, std::vector<unsigned char>(bufsize)); // a[M][N]
        make_empty();
    }

    virtual ~CircleQue()
    {
    }

    bool IsEmpty()
    {
        return (0 == size);
    }

    bool IsFull()
    {
        return (size == capacity);
    }

    int GetSize()
    {
        return size;
    }

    virtual bool Enque(unsigned char *data)
    {
        if(IsFull())
        {
            printf("error: full queue! capacity=%d, size=%d, front=%d, rear=%d", 
                capacity, size, front, rear);
            //throw std::invalid_argument("full queue");
            return false;
        }
        else
        {
            // default copying action ?
            memcpy(&buffers[rear][0], data, buffers[rear].size());
            rear = succ(rear);
            size++;
            return true;
        }
    }

    virtual unsigned char *Deque()
    {
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

protected:
    int succ(int index)
    {
        if(++index == capacity)
            index = 0;
        return index;
        //return (index + 1) % capacity; // avoid to use / and %
    }

    void make_empty()
    { 
        front = 0;
        rear = 0;
        size = 0;
    }

protected:
    int capacity;
    int front;
    int rear;
    int size;
    std::vector<std::vector<unsigned char> > buffers;
};

#endif