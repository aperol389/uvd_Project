/*
 * Copyright (c) 2018 Polycom Inc
 *
 * General Thread Class
 * which name is better? GThread, GenThread, MyThread ...
 *
 * Author: SONGYI (yi.song@polycom.com)
 * Date Created: 20180827
 */

#ifndef _MY_THREAD_H_
#define _MY_THREAD_H_

#include <pthread.h>
#include <sys/types.h> // pthread_attr_t
#include <sys/time.h> // gettimeofday
#include <cerrno> // ETIMEDOUT

typedef void *(*THREAD_PROC_CB)(void *);

void *thread_proc(void *ctx);

class MyThread
{
public:
    MyThread(THREAD_PROC_CB pfnCb, void *param, bool init_suspend=true, int exit_timeout=3)
    {
        cb = pfnCb;
        arg = param;
        run = false;
        pause = init_suspend;
        maxsec = exit_timeout;
        pthread_mutex_init(&exit_mtx, NULL);
        pthread_cond_init(&exit_cond, NULL);
        pthread_mutex_init(&suspend_mtx, NULL);
        pthread_cond_init(&suspend_cond, NULL);
    }

    virtual ~MyThread()
    {
        pthread_mutex_destroy(&exit_mtx);
        pthread_cond_destroy(&exit_cond);
        pthread_mutex_destroy(&suspend_mtx);
        pthread_cond_destroy(&suspend_cond);
    }

    friend void *thread_proc(void *ctx)
    {
        MyThread *pthis = (MyThread *)ctx;
        while(pthis->run)
        {
            // cancel point
            pthread_testcancel();

            // suspend
            pthread_mutex_lock(&pthis->suspend_mtx);
            while(pthis->pause)
                pthread_cond_wait(&pthis->suspend_cond, &pthis->suspend_mtx);
            pthread_mutex_unlock(&pthis->suspend_mtx);

            // do something ...
            (*pthis->cb)(pthis->arg);
        }

        // signal waiting threads that this thread is about to terminate
        pthread_mutex_lock(&pthis->exit_mtx);
        pthread_cond_broadcast(&pthis->exit_cond);
        pthread_mutex_unlock(&pthis->exit_mtx);

        return NULL;

    }

    bool start()
    {
        pthread_attr_t attr;

        if(0 != pthread_attr_init(&attr))
        {
            printf("error: failed to init pthread_attr.\n");
            return false;
        }

        run = true;

        if(0 != pthread_create(&tid, &attr, thread_proc, this))
        {
            printf("error: failed to create distortion thread.\n");
            pthread_attr_destroy(&attr);
            return false;
        }

        pthread_attr_destroy(&attr);
        return true;
    }

    void stop()
    {
        int ret = ETIMEDOUT;
        struct timespec ts;
        struct timeval cur;

        // set exit flag
        run = false;

        // convert from timeval to timespec
        if(0 == gettimeofday(&cur, NULL))
        {
            ts.tv_sec  = cur.tv_sec;
            ts.tv_nsec = cur.tv_usec * 1000;
            ts.tv_sec += maxsec; // max timeout seconds

            // wait (with timeout) until thread has finished
            pthread_mutex_lock(&exit_mtx);
            ret = pthread_cond_timedwait(&exit_cond, &exit_mtx, &ts);
            pthread_mutex_unlock(&exit_mtx);
        }
        else
            printf("error: failed to get time!\n");


        // wait until thread has -really- finished
        if (ret == ETIMEDOUT)
        {
            printf("warning: wait distortion thread exit time out!\n");
            pthread_cancel(tid); // try to forcefully stop it at a cancellation point
        }
        else
        {
            pthread_join(tid, NULL);
        }
    }

    void suspend()
    {
        pthread_mutex_lock(&suspend_mtx);
        pause = true;
        pthread_mutex_unlock(&suspend_mtx);
    }

    void resume()
    {
        pthread_mutex_lock(&suspend_mtx);
        pause = false;
        pthread_cond_signal(&suspend_cond);
        pthread_mutex_unlock(&suspend_mtx);
    }

private:
    pthread_t tid;
    pthread_mutex_t exit_mtx;
    pthread_cond_t exit_cond;
    pthread_mutex_t suspend_mtx;
    pthread_cond_t suspend_cond;
    bool run;
    bool pause;
    int maxsec;
    THREAD_PROC_CB cb;
    void *arg;
};

#endif
