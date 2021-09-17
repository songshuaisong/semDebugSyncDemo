#include <stdio.h>
#include <windows.h>


HANDLE App1_App2_Sync_Sem = NULL;
HANDLE App3_App1_Sync_Sem = NULL;

static void usSleep(double usec);
int DebugStringTo_LogFile_Writer(const char *file_name, unsigned char *buf, size_t buf_len);
void system_Init();


int sys_Cycle_Cnt = 0;
TCHAR App1_App2_Sync_Sem_Name[128] = { "App1_App2_Sync_Sem" };
TCHAR App3_App1_Sync_Sem_Name[128] = { "App3_App1_Sync_Sem" };


int main(int argc, const void **argv)
{

	system_Init();

	while (1)
	{
		printf("Cycle Num : %d, WaitForSingleObject(%s)!\n", sys_Cycle_Cnt, App3_App1_Sync_Sem_Name);

		/* 等待 VVS 控制的信号量通知 */
		WaitForSingleObject(App3_App1_Sync_Sem, INFINITE);


		sys_Cycle_Cnt++;
		// usSleep(300000);


		
		
		/* 释放3个信号量，表示ATP已经成功运行一个周期 */
		ReleaseSemaphore(App1_App2_Sync_Sem, 3, NULL);
		printf("Cycle Num : %d, ReleaseSemaphore(%s)!\n", sys_Cycle_Cnt, App1_App2_Sync_Sem_Name);


	}

	return 0;
}

void system_Init()
{

	/* 创建或者打开一个控制ATO运行的信号量,并且初始化计数为3个，最大值为3个（对应于3个ATO周期为一个ATP周期）*/
	App1_App2_Sync_Sem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, (LPCTSTR)App1_App2_Sync_Sem_Name);
	if (App1_App2_Sync_Sem == NULL) /* 打开失败，说明不存在想要打开的信号量 */
	{
		/* 创建一个对应属性的信号量 */
		App1_App2_Sync_Sem = CreateSemaphore(NULL, 3, 3, (LPCTSTR)App1_App2_Sync_Sem_Name);
		if (App1_App2_Sync_Sem == NULL) /* 创建信号量失败 */
		{
			DebugStringTo_LogFile_Writer("App1.log", "Cycle Num : %d, Create semaphore named 'App1_App2_Sync_Sem' failed!", 50);
			return;
		}
		printf("Cycle Num : %d, Create semaphore named '%s' successed!\n", sys_Cycle_Cnt, App1_App2_Sync_Sem_Name);
	}
	else
	{
		printf("Cycle Num : %d, Open semaphore named '%s' successed!\n", sys_Cycle_Cnt, App1_App2_Sync_Sem_Name);
	}

	/* 创建或者打开用于VVS控制ATP周期的信号量 */
	App3_App1_Sync_Sem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, (LPCTSTR)App3_App1_Sync_Sem_Name);
	if (App3_App1_Sync_Sem == NULL) /* 打开失败，说明不存在想要打开的信号量 */
	{
		/* 创建一个对应属性的信号量 */
		App3_App1_Sync_Sem = CreateSemaphore(NULL, 0, 1, (LPCTSTR)App3_App1_Sync_Sem_Name);
		if (App3_App1_Sync_Sem == NULL) /* 创建信号量失败 */
		{
			DebugStringTo_LogFile_Writer("App1.log", "Cycle Num : %d, Create semaphore named 'App3_App1_Sync_Sem' failed!", 50);
			return;
		}
		printf("Cycle Num : %d, Create semaphore named '%s' successed!\n", sys_Cycle_Cnt, App3_App1_Sync_Sem_Name);
	}
	else
	{
		printf("Cycle Num : %d, Open semaphore named '%s' successed!\n", sys_Cycle_Cnt, App3_App1_Sync_Sem_Name);
	}
}


/******************************************************************************************************************************************
*    @fn               usSleep
*    @brief            微秒级的延时函数
*    @detail           微秒级延时函数，精确度为 10微秒级，内置时间补偿
*
*    @return           无
*
*    @REV    1.0.0    z.n    2021.03.15 Create
*******************************************************************************************************************************************/
static void usSleep(double usec)
{
	static int flag = 0;
	static __int64 nStart = 0; //起始计数

	__int64 nFreq = 0; //频率
	__int64 nEnd = 0; // 终止计数
	static double offset = 0;

	if (flag == 0)
	{
		flag = 1;
		QueryPerformanceCounter((LARGE_INTEGER*)&nStart);
		QueryPerformanceFrequency((LARGE_INTEGER*)&nFreq);
		// 当前环境下，nFreq 为 1000 0000
		offset = 1000000.0 / (double)nFreq; // 将计数转换为微秒
	}
	usec += 200; // 给时间进行简单的时间补偿
	for (;;)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&nEnd);
		if (usec <= (double)(nEnd - nStart) * offset)
		{
			break;
		}
	}
	QueryPerformanceCounter((LARGE_INTEGER*)&nStart);
}


/******************************************************************************************************************************************/
/*名称：       DebugData_To_LogFile_Writer                                                                                                 */
/*功能：       将调试信息写入到文件中                                                                                                          */
/*方法：       调用函数输入文件名称，要写入文件的buff，buff的大小，将会以对应的十六进制字符串的形式写入到文件中                                           */
/*输入：       file_name                文件名称，可携带路径                                                                                   */
/*            buf					   要写入的数据的首地址                                                                                   */
/*            buf_len                  要写入的数据的长度（字节数）                                                                            */
/*返回值：     无                                                                                                                           */
/******************************************************************************************************************************************/
int DebugStringTo_LogFile_Writer(const char *file_name, unsigned char *buf, size_t buf_len)
{
	int ret = 0;
	if (buf_len > 0)
	{
		FILE* pfile = fopen(file_name, "a+");
		unsigned char *pbuf = (unsigned char *)buf;

		for (size_t idx = 0; idx < buf_len; idx++)
			ret += fprintf(pfile, "%c", pbuf[idx]);

		fprintf(pfile, "\n");
		fclose(pfile);
	}
	return ret;
}

/*

error C4996: 'fopen': This function or variable may be unsafe. Consider using fopen_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.













*/