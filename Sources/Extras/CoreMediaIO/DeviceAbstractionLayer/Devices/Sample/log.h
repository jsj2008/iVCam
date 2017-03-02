#ifndef _TT_LOG_H_
#define _TT_LOG_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#if (defined _WIN32 || defined _WIN64)
#include <windows.h>
#include <assert.h>
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#define TEXT 

class  CMyLog 
{
public:
	CMyLog();
	~CMyLog();

	static CMyLog& GetInstance();
	void Log(unsigned char level, const char* file, int line, const char* fmt, ...);
	void Log(unsigned char level, const char* str);

private:
	void ChangeLogFile(); 
	FILE* m_fp;
	unsigned int m_ulFileSize;
	unsigned char m_ucLevel;

	std::string m_path;

#if (defined _WIN32 || defined _WIN64)
	HANDLE m_hMutex;
#else
	pthread_mutex_t m_mutex;
#endif

};

#define LOG_LEVEL_ERR  0 
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_DBG  2

#define LOGERR(fmt, ...)  CMyLog::GetInstance().Log(LOG_LEVEL_ERR,  __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define LOGINFO(fmt, ...) CMyLog::GetInstance().Log(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define LOGDBG(fmt, ...)  CMyLog::GetInstance().Log(LOG_LEVEL_DBG,  __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#endif


