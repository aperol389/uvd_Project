/*
 * Autolock class
 * Copyright (C) 2017 Polycom Incorporated, all rights reserved.
 * Author: SONGYI (yi.song@polycom.com)
 */

#ifndef _AUTOLOCK_H_
#define _AUTOLOCK_H_

#include <pthread.h>

class MutexLock
{
public:
    MutexLock()
    {
        pthread_mutex_init(&mtx, NULL);
    }

    ~MutexLock()
    {
        pthread_mutex_destroy(&mtx);
    }

    int Lock()
    {
        return pthread_mutex_lock(&mtx);
    }

    int Unlock()
    {
        return pthread_mutex_unlock(&mtx);
    }
    
private:
    pthread_mutex_t mtx;
};

class Autolock
{
public:
    Autolock(MutexLock *lockptr)
    {
        lock = lockptr;
        if(lock != NULL)
            lock->Lock();
    }

    ~Autolock()
    {
        if(lock != NULL)
            lock->Unlock();
    }
    
private:
    MutexLock *lock;
};

#endif 
