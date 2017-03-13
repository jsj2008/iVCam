#include <time.h>
#include <sys/stat.h> 
#include "log.h"
#include "macro.h" 

CMyLog::CMyLog()
	: m_fp(NULL)
	, m_ulFileSize(0)
{	
	m_ulFileSize = 1024 * 1024 * 5;
	m_ucLevel = LOG_LEVEL_INFO;
	
	pthread_mutex_init(&m_mutex, NULL);

	char* home = getenv("HOME");
	if (!home)
	{
		printf("cann't get home dir\n");
		return;
	}

	m_path = std::string(home);
	m_path = m_path + "/Library/Application Support";
	if (access(m_path.c_str(), 0))
	{
		printf("HOME/Library/Application Support dir not exist\n");
		return;
	}

	m_path = m_path + "/insta360";
	if (access(m_path.c_str(), 0))
	{
		mkdir(m_path.c_str(), 0766);
	}

	m_path = m_path + "/Air";
	if (access(m_path.c_str(), 0))
	{
		mkdir(m_path.c_str(), 0766);
	}

	std::string logfile = m_path + "/decoder.log";
	m_fp = fopen(logfile.c_str(), "a+");
	
	if (!m_fp)
	{
		printf("Fail to open log file.\n");
		return;
	}
}

CMyLog::~CMyLog()
{	
	if (m_fp)
	{
		fclose(m_fp);
	}

	pthread_mutex_destroy(&m_mutex);
}

CMyLog& CMyLog::GetInstance()
{
	static CMyLog* pLog = NULL;

	if (!pLog)
	{
		pLog = new CMyLog;
	}

	return *pLog;
}

void CMyLog::Log(unsigned char level, const char* file, int line, const char* fmt, ...)
{	
	static char LogLevelString[][10] = {"ERROR", "INFO ", "DEBUG"};

	if (level > m_ucLevel)
	{
		return;
	}

	pthread_mutex_lock(&m_mutex);
	time_t timer = time(NULL);
	struct tm *tmt = localtime(&timer);
	va_list arg;

	/* to log file */
	if (m_fp)
	{
		fprintf(m_fp, "%04d-%02d-%02d %02d:%02d:%02d %s ",
			tmt->tm_year + 1900,
			tmt->tm_mon + 1,
			tmt->tm_mday,
			tmt->tm_hour,
			tmt->tm_min,
			tmt->tm_sec,
			LogLevelString[level]);

		/* log content */
		va_start(arg, fmt);
		vfprintf(m_fp, fmt, arg);
		va_end(arg);

		/* log file */
		fprintf(m_fp, " %s:%d\r\n", file, line);
		fflush(m_fp);
		ChangeLogFile();
	}

	pthread_mutex_unlock(&m_mutex);
}

void CMyLog::Log(unsigned char level, const char* str)
{	
	static char LogLevelString[][10] = { "ERROR", "INFO ", "DEBUG" };
	if (level > m_ucLevel)
	{
		return;
	}

	pthread_mutex_lock(&m_mutex);
	time_t timer = time(NULL);
	struct tm *tmt = localtime(&timer);
	/* to log file */
	if (m_fp)
	{
		fprintf(m_fp, "%04d-%02d-%02d %02d:%02d:%02d %s %s",
			tmt->tm_year + 1900,
			tmt->tm_mon + 1,
			tmt->tm_mday,
			tmt->tm_hour,
			tmt->tm_min,
			tmt->tm_sec,
			LogLevelString[level],
			str);

		fflush(m_fp);
		ChangeLogFile();
	}

	pthread_mutex_unlock(&m_mutex);
}

void CMyLog::ChangeLogFile()
{
	std::string origfile = m_path + "/decoder.log";
 
	struct stat statbuff;  
	if (stat(origfile.c_str(), &statbuff) < 0)
	{  
		return;  
	}

	if (statbuff.st_size  < (int)m_ulFileSize)
	{
		return;
	}

	fclose(m_fp);
	m_fp = NULL;

	char filename[256] = {0};
	time_t timer = time(NULL);
	struct tm *tmt = localtime(&timer);

	CC_SPRINTF(filename, 256 - 1, "/decoder_%04d_%02d_%02d_%02d_%02d_%02d.log", 
		tmt->tm_year + 1900,
		tmt->tm_mon + 1,
		tmt->tm_mday,
		tmt->tm_hour,
		tmt->tm_min,
		tmt->tm_sec);

	std::string newfile = m_path + filename;
	rename(origfile.c_str(), newfile.c_str());
	m_fp = fopen(origfile.c_str(), "a+");
	if (!m_fp)
	{
		printf("Failed to open log file.\r\n");
	}
}

