//
// Created by liuxiang on 2017/10/15.
//

#ifndef DNRECORDER_SAFE_QUEUE_H
#define DNRECORDER_SAFE_QUEUE_H

#include <queue>
#include <pthread.h>
#include "logutil.h"

#include "macro.h"
//todo 是否使用c++11,玩玩而已。还是习惯posix标准的线程。。。。
//#define C11
#ifdef C11
#include <thread>
#endif


using namespace std;


template<typename T>
class SafeQueue {
    typedef void (*ReleaseHandle)(T &);

    typedef void (*SyncHandle)(queue<T> &);

public:
    SafeQueue() {
#ifdef C11

#else
        pthread_mutex_init(&mutex, nullptr);
        pthread_cond_init(&cond, nullptr);
#endif

    }

    ~SafeQueue() {
#ifdef C11
#else
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex);
#endif

    }


    void enQueue(T new_value) {
#ifdef C11
        lock_guard<mutex> lk(mt);
        if (work) {
            q.push(new_value);
            cv.notify_one();
        }
#else
        pthread_mutex_lock(&mutex);
        if (work) {
            q.push(new_value);
            pthread_cond_signal(&cond);
        } else {
            slogd("无法加入数据====:%d", q.size());
            if (!releaseHandle) {
                sloge("releaseHandle 为null");
                return;
            }
            releaseHandle(new_value);
        }
        pthread_mutex_unlock(&mutex);
#endif

    }


    int deQueue(T &value) {
        int ret = 0;
        if (size() <= 0) {
            sloge("q size : %d", size());
            return 0;
        }
#ifdef C11
        //占用空间相对lock_guard 更大一点且相对更慢一点，但是配合条件必须使用它，更灵活
        unique_lock<mutex> lk(mt);
        //false则不阻塞 往下走
        cv.wait(lk,[this]{return !work || !empty();});
        if (!empty()) {
            value = q.front();
            q.pop();
            ret = 1;
        }
#else
        pthread_mutex_lock(&mutex);
        //在多核处理器下 由于竞争可能虚假唤醒 包括jdk也说明了
        while (work && empty()) {
            pthread_cond_wait(&cond, &mutex);
            pthread_mutex_unlock(&mutex);
        }
        if (!empty()) {
            value = q.front();
            q.pop();
            ret = 1;
        }
        pthread_mutex_unlock(&mutex);
#endif

        return ret;
    }

    void setWork(int work) {
#ifdef C11
        lock_guard<mutex> lk(mt);
        this->work = work;
#else
        pthread_mutex_lock(&mutex);
        this->work = work;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
#endif

    }

    int empty() {
        return q.empty();
    }

    int size() {
        return q.size();
    }

    void clear() {
#ifdef C11
        lock_guard<mutex> lk(mt);
        int size = q.size();
        for (int i = 0; i < size; ++i) {
            T value = q.front();
            if (!releaseHandle){
                sloge("releaseHandle 为null");
                return;
            }
            releaseHandle(value);
            q.pop();
        }
#else
        pthread_mutex_lock(&mutex);
        int size = q.size();
        for (int i = 0; i < size; ++i) {
            T value = q.front();
            releaseHandle(value);
            q.pop();
        }
        slogd("清空数据====:%d", q.size());
        pthread_mutex_unlock(&mutex);
#endif

    }

    void sync() {
#ifdef C11
        lock_guard<mutex> lk(mt);
        syncHandle(q);
#else
        pthread_mutex_lock(&mutex);
        syncHandle(q);
        pthread_mutex_unlock(&mutex);
#endif

    }

    void setReleaseHandle(ReleaseHandle r) {
        releaseHandle = r;
    }

    void setSyncHandle(SyncHandle s) {
        syncHandle = s;
    }

private:

#ifdef C11
    mutex mt;
    condition_variable cv;
#else
    pthread_cond_t cond;
    pthread_mutex_t mutex;
#endif

    queue<T> q;
    int work;
    ReleaseHandle releaseHandle;
    SyncHandle syncHandle;

};


#endif //DNRECORDER_SAFE_QUEUE_H
