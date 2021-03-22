#ifndef __PROCESS_MUTEX_H__
#define __PROCESS_MUTEX_H__

#ifdef _WIN32
#include "windows.h"
#include <Windows.h>
#include <process.h>
typedef unsigned long DWORD;
typedef const WCHAR *LPCWSTR;
typedef void *HANDLE;
#else

#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <memory.h>
#include <sys/stat.h> 
#include <stdlib.h>
#include <cstddef>
#endif


class CProcessMutex;

class CProcessMutex
{
public:
	void* m_pMutex;
	#ifdef _WIN32
		void* m_pTmp;
    #else
		sem_t* m_pSem;
	#endif
	//#ifdef linux
	//	sem_t* m_pSem;
	//#endif
	char m_cMutexName[50];
public:
    /* 默认创建匿名的互斥 */
	void ThreadCreateMutex(const char* name = NULL);
	bool ThreadWaitMutex();
	bool ThreadReleaseMutex();
	void ThreadCloseMutex();
    CProcessMutex()
	{
		m_pMutex=NULL;
		#ifdef _WIN32
			m_pMutex=NULL;
		#else
			m_pSem=NULL;
		#endif
		//#ifdef linux
		//	m_pSem=NULL;
		//#endif
		m_cMutexName[0] = 0;
	};
    ~CProcessMutex();

};
#endif
