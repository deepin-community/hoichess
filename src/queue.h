/* Copyright (C) 2005-2015 Holger Ruckdeschel <holger@hoicher.de>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

#include <list>

#if defined(HAVE_PTHREAD)
# include <errno.h>
# include <pthread.h>
# include <sys/time.h>
#elif defined(WIN32)
# include <windows.h>
#else
# error no thread support is available
#endif


template <typename T>
class Queue {
      private:
#if defined(HAVE_PTHREAD)
	pthread_mutex_t mtx;
	pthread_cond_t cv;
#elif defined(WIN32)
	CRITICAL_SECTION cs;
	HANDLE sem;
#endif

	std::list<T> queue;

      public:
	Queue();
	~Queue();

      public:
	void put(const T& elem);
	T get();
	bool wait(unsigned long long timeout_us);
	void clear();
};


template <typename T>
Queue<T>::Queue()
{
#if defined(HAVE_PTHREAD)
	int res = pthread_mutex_init(&mtx, NULL);
	ASSERT(res == 0);
	res = pthread_cond_init(&cv, NULL);
	ASSERT(res == 0);
#elif defined(WIN32)
	InitializeCriticalSection(&cs); /* void */
	sem = CreateSemaphore(NULL, 0, INT_MAX, NULL);
	ASSERT(sem != NULL);
#endif
}

template <typename T>
Queue<T>::~Queue()
{
#if defined(HAVE_PTHREAD)
	int res = pthread_mutex_destroy(&mtx);
	ASSERT(res == 0);
	res = pthread_cond_destroy(&cv);
	ASSERT(res == 0);
#elif defined(WIN32)
	DeleteCriticalSection(&cs); /* void */
	BOOL res = CloseHandle(sem);
	ASSERT(res); 
#endif
}

template <typename T>
void Queue<T>::put(const T& elem)
{
#if defined(HAVE_PTHREAD)
	int res = pthread_mutex_lock(&mtx);
	ASSERT(res == 0);
	queue.push_back(elem);
	res = pthread_cond_signal(&cv);
	ASSERT(res == 0);
	res = pthread_mutex_unlock(&mtx);
	ASSERT(res == 0);
#elif defined(WIN32)
	EnterCriticalSection(&cs); /* void */
	queue.push_back(elem);
	BOOL res = ReleaseSemaphore(sem, 1, NULL);
	ASSERT(res);
	LeaveCriticalSection(&cs); /* void */
#endif
}

template <typename T>
T Queue<T>::get()
{
#if defined(HAVE_PTHREAD)
	int res = pthread_mutex_lock(&mtx);
	ASSERT(res == 0);
	while (queue.empty()) {
		res = pthread_cond_wait(&cv, &mtx);
		ASSERT(res == 0);
	}
	T elem = queue.front();
	queue.pop_front();
	res = pthread_mutex_unlock(&mtx);
	ASSERT(res == 0);
	return elem;
#elif defined(WIN32)
	DWORD res = WaitForSingleObject(sem, INFINITE);
	ASSERT(res == WAIT_OBJECT_0);
	EnterCriticalSection(&cs); /* void */
	ASSERT(!queue.empty());
	T elem = queue.front();
	queue.pop_front();
	LeaveCriticalSection(&cs); /* void */
	return elem;
#endif
}

template <typename T>
bool Queue<T>::wait(unsigned long long timeout_us)
{
#if defined(HAVE_PTHREAD)
	/* Ugh. pthread_cond_timedwait() needs absolute time. */
	struct timeval now;
	struct timespec timeout;
	int res = gettimeofday(&now, NULL);
	ASSERT(res == 0);
	unsigned long long us_tmp = now.tv_usec + timeout_us;
	timeout.tv_nsec = us_tmp % (int) 1E6 * 1000;
	timeout.tv_sec = now.tv_sec + us_tmp / (int) 1E6;

	res = pthread_mutex_lock(&mtx);
	ASSERT(res == 0);
	bool ret = true;
	while (queue.empty()) {
		res = pthread_cond_timedwait(&cv, &mtx, &timeout);
		if (res == ETIMEDOUT) {
			ret = false;
			break;
		}
		ASSERT(res == 0);
	}
	res = pthread_mutex_unlock(&mtx);
	ASSERT(res == 0);
	return ret;
#elif defined(WIN32)
	DWORD res = WaitForSingleObject(sem, timeout_us / (int) 1E3);
	if (res == WAIT_TIMEOUT) {
		return false;
	}
	ASSERT(res == WAIT_OBJECT_0);
	return true;
#endif
}

template <typename T>
void Queue<T>::clear()
{
#if defined(HAVE_PTHREAD)
	int res = pthread_mutex_lock(&mtx);
	ASSERT(res == 0);
	queue.erase(queue.begin(), queue.end());
	res = pthread_mutex_unlock(&mtx);
	ASSERT(res == 0);
#elif defined(WIN32)
	EnterCriticalSection(&cs); /* void */
	/* There seems to be no way to simply set the semaphore to zero,
	 * thus we cannot just clear the queue. Instead we remove each
	 * element individually, decrementing the semaphore each time
	 * (like the normal get() procedure does). */
	while (queue.size() != 0) {
		DWORD res = WaitForSingleObject(sem, INFINITE);
		ASSERT(res == WAIT_OBJECT_0);
		queue.pop_front();
	}
	LeaveCriticalSection(&cs); /* void */
#endif
}

#endif // QUEUE_H
