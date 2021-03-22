#include "processmutex.h"


#ifdef _WIN32
#include <Windows.h>
#include <process.h>

void CProcessMutex::ThreadCreateMutex(const char* name)
{
	m_pMutex = CreateMutex(NULL,false,NULL);
}

bool CProcessMutex::ThreadWaitMutex()
{
    if (NULL == (m_pMutex))
    {
        return false;
    }
    DWORD nRet = WaitForSingleObject(m_pMutex, INFINITE);
    if (nRet != WAIT_OBJECT_0)
    {
        return false;
    }
    return true;
}

bool CProcessMutex::ThreadReleaseMutex()
{
    return ReleaseMutex(m_pMutex);
}

void CProcessMutex::ThreadCloseMutex()
{
    CloseHandle(m_pMutex);
}

#else
void CProcessMutex::ThreadCreateMutex(const char* name)
{
    memset(m_cMutexName, 0 ,sizeof(m_cMutexName));
    int min = strlen(name)>(sizeof(m_cMutexName)-1)?(sizeof(m_cMutexName)-1):strlen(name);
    strncpy(m_cMutexName, name, min);
    m_pSem = sem_open(name, O_RDWR | O_CREAT, 0644, 1);
}

bool CProcessMutex::ThreadWaitMutex()
{
	//互斥锁创建失败
    int ret = sem_wait(m_pSem);
    if (ret != 0)
    {
        return false;
    }
    return true;
}

bool CProcessMutex::ThreadReleaseMutex()
{
    int ret = sem_post(m_pSem);
    if (ret != 0)
    {
        return false;
    }
    return true;
}

void CProcessMutex::ThreadCloseMutex()
{
    int ret = sem_close(m_pSem);
    if (0 != ret)
    {
        printf("sem_close error %d\n", ret);
    }
    sem_unlink(m_cMutexName);
}
#endif
