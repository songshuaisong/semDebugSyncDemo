#include <stdio.h>
#include <windows.h>


HANDLE App1_App2_Sync_Sem = NULL;
HANDLE App2_App3_Sync_Sem = NULL;
HANDLE App3_App2_Sync_Sem = NULL;

static void usSleep(double usec);
int DebugStringTo_LogFile_Writer(const char *file_name, unsigned char *buf, size_t buf_len);
void system_Init();


#ifdef WIN32
static PROCESS_INFORMATION slave_pi1;
static PROCESS_INFORMATION slave_pi2;
static HANDLE hSemaphore;
static STARTUPINFO slave_si1;
static STARTUPINFO slave_si2;
#pragma comment  (lib, "User32.lib")
#endif
typedef enum 
{
	EnumReturnTrue = 0x55,
	EnumReturnFalse = 0xAA
}EnumReturn;
/* 定义一个钩子的句柄 */
HHOOK hKeyboardHook = NULL;
/* 用于控制是否允许进程运行的标志，EnumReturnTrue 为 运行， EnumReturnFalse 为退出运行 */
EnumReturn is_Process_Running_Enable = EnumReturnTrue;
int KeyPress_SHIFT_Cnt = 0; /* shift按键按下的次数记录 */
int KeyPress_F5_Cnt = 0;    /* F5按键按下的记录 */
void keyBoard_Listen_Event(void *arg);
HANDLE g_handle = NULL;
void test_Thread_1(void *arg);


TCHAR App1_App2_Sync_Sem_Name[128] = { "App1_App2_Sync_Sem" };
TCHAR App2_App3_Sync_Sem_Name[128] = { "App2_App3_Sync_Sem" };
TCHAR App3_App2_Sync_Sem_Name[128] = { "App3_App2_Sync_Sem" };
int sys_Cycle_Cnt = 0;



int main(int argc, const void **argv)
{
	unsigned int ThreadId1 = 0;
	unsigned int ThreadId2 = 0;

	system_Init();


	/* 启动新的线程用于按键事件监听 */
	g_handle = (HANDLE)_beginthreadex(NULL, 0, keyBoard_Listen_Event, NULL, 0, &ThreadId1);
	//g_handle = (HANDLE)_beginthreadex(NULL, 0, test_Thread_1, NULL, 0, &ThreadId1);

	while (1)
	{
		printf("Cycle Num : %d, WaitForSingleObject(%s)!\n", sys_Cycle_Cnt, App1_App2_Sync_Sem_Name);
		/* 等待 ATP 控制 ATO 的信号量通知 */
		WaitForSingleObject(App1_App2_Sync_Sem, INFINITE);
		printf("Cycle Num : %d, WaitForSingleObject(%s)!\n", sys_Cycle_Cnt, App3_App2_Sync_Sem_Name);
		/* 等待 VVS 控制 ATO 的信号量通知 */
		WaitForSingleObject(App3_App2_Sync_Sem, INFINITE);


		sys_Cycle_Cnt++;
		usSleep(100000);


		/* 释放 ATO 用于控制 VVS 的信号量 */
		ReleaseSemaphore(App2_App3_Sync_Sem, 1, NULL);
		printf("Cycle Num : %d, ReleaseSemaphore(%s)!\n", sys_Cycle_Cnt, App2_App3_Sync_Sem_Name);

	}

	return 0;
}

void test_Thread_1(void *arg)
{
	HINSTANCE hins;
	int sys_Thread_Cnt = 0;
	char bugggg[128] = { 0 };
	while (1)
	{
		sys_Thread_Cnt++;
		sprintf(bugggg, "sys_Thread_Cnt = %d", sys_Thread_Cnt);
		DebugStringTo_LogFile_Writer("logFile", bugggg, strlen(bugggg));

		Sleep(100);/* VC 使用Sleep*/
	}
}

/******************************************************************************************************************************************
*    @fn               LowLevelKeyboardProc
*    @brief            按键事件监听服务函数
*    @detail           按键事件监听服务函数，为回调函数
*
*    @return           无
*
*    @REV    1.0.0         2021.08.09 Create
*******************************************************************************************************************************************/
LRESULT WINAPI LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	static DWORD lastHitTime = 0;

	if ((nCode == HC_ACTION) && ((wParam == WM_SYSKEYDOWN) || (wParam == WM_KEYDOWN)))
	{
		KBDLLHOOKSTRUCT hooked_key = *((KBDLLHOOKSTRUCT*)lParam);

		if (hooked_key.vkCode == 0xA0) /* SHIFT 按键按下 */
		{
			if (hooked_key.time - lastHitTime > 1000) /* 如果本次按下的时间较上一次按下的时间超过了 1秒 */
			{
				KeyPress_SHIFT_Cnt = 0;
			}
			KeyPress_SHIFT_Cnt++;
		}
		else if (hooked_key.vkCode == 0x74) /* F5 功能按键 */
		{
			if (hooked_key.time - lastHitTime > 1000) /* 如果本次按下的时间较上一次按下的时间超过了 1秒 */
			{
				KeyPress_F5_Cnt = 0;
			}
			KeyPress_F5_Cnt++;
		}

		if (KeyPress_SHIFT_Cnt > 0 && KeyPress_F5_Cnt > 0)
		{
			if (hooked_key.time - lastHitTime < 1000) /* 两个按键按下的组合是否有效 */
			{
				TerminateProcess(slave_pi1.hProcess, 0);
				TerminateProcess(slave_pi2.hProcess, 0);

				/* 设置进程允许运行的标志为 EnumReturnFalse，将要退出进程 */
				is_Process_Running_Enable = EnumReturnFalse;
				///* 退出监听的线程 */
				//_endthread();
				/* 退出自己 */
				exit(0);
			}
			else
			{
				/*- do nothing*/
			}
		}

		lastHitTime = hooked_key.time; /* 记录本次shift触发的时间 */
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

/******************************************************************************************************************************************
*    @fn               LowLevelKeyboardProc
*    @brief            执行键盘按键监听事件
*    @detail           执行键盘按键监听事件，为线程服务函数
*
*    @return           无
*
*    @REV    1.0.0         2021.08.09 Create
*******************************************************************************************************************************************/
void keyBoard_Listen_Event(void *arg)
{
	HINSTANCE hins;

	hins = GetModuleHandle(NULL);
	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)LowLevelKeyboardProc, hins, 0);


	while (EnumReturnTrue == is_Process_Running_Enable)
	{
		/* 取得一个消息并将其放于指定的结构, 用于监听键盘按下事件，阻塞 */
		GetMessage(NULL, NULL, 0, 0);
	}
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

	/* 创建或者打开一个控制ATO运行的信号量,并且初始化计数为3个，最大值为3个（对应于3个ATO周期为一个ATP周期）*/
	App2_App3_Sync_Sem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, (LPCTSTR)App2_App3_Sync_Sem_Name);
	if (App2_App3_Sync_Sem == NULL) /* 打开失败，说明不存在想要打开的信号量 */
	{
		/* 创建一个对应属性的信号量 */
		App2_App3_Sync_Sem = CreateSemaphore(NULL, 0, 1, (LPCTSTR)App2_App3_Sync_Sem_Name);
		if (App2_App3_Sync_Sem == NULL) /* 创建信号量失败 */
		{
			DebugStringTo_LogFile_Writer("App1.log", "Cycle Num : %d, Create semaphore named 'App1_App2_Sync_Sem' failed!\n", 50);
			return;
		}
		printf("Cycle Num : %d, Create semaphore named '%s' successed!\n", sys_Cycle_Cnt, App2_App3_Sync_Sem_Name);
	}
	else
	{
		printf("Cycle Num : %d, Open semaphore named '%s' successed!\n", sys_Cycle_Cnt, App2_App3_Sync_Sem_Name);
	}

	printf("Cycle Num : %d, Openning semaphore named '%s'!\n", sys_Cycle_Cnt, App3_App2_Sync_Sem_Name);
	do
	{
		/* 打开用于 VVS 控制 ATO 运行的信号量 */
		App3_App2_Sync_Sem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, (LPCTSTR)App3_App2_Sync_Sem_Name);
	} while (App3_App2_Sync_Sem == NULL);

	printf("Cycle Num : %d, Open semaphore named '%s' successed!\n", sys_Cycle_Cnt, App3_App2_Sync_Sem_Name);
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