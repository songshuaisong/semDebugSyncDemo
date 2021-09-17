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
/* ����һ�����ӵľ�� */
HHOOK hKeyboardHook = NULL;
/* ���ڿ����Ƿ�����������еı�־��EnumReturnTrue Ϊ ���У� EnumReturnFalse Ϊ�˳����� */
EnumReturn is_Process_Running_Enable = EnumReturnTrue;
int KeyPress_SHIFT_Cnt = 0; /* shift�������µĴ�����¼ */
int KeyPress_F5_Cnt = 0;    /* F5�������µļ�¼ */
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


	/* �����µ��߳����ڰ����¼����� */
	g_handle = (HANDLE)_beginthreadex(NULL, 0, keyBoard_Listen_Event, NULL, 0, &ThreadId1);
	//g_handle = (HANDLE)_beginthreadex(NULL, 0, test_Thread_1, NULL, 0, &ThreadId1);

	while (1)
	{
		printf("Cycle Num : %d, WaitForSingleObject(%s)!\n", sys_Cycle_Cnt, App1_App2_Sync_Sem_Name);
		/* �ȴ� ATP ���� ATO ���ź���֪ͨ */
		WaitForSingleObject(App1_App2_Sync_Sem, INFINITE);
		printf("Cycle Num : %d, WaitForSingleObject(%s)!\n", sys_Cycle_Cnt, App3_App2_Sync_Sem_Name);
		/* �ȴ� VVS ���� ATO ���ź���֪ͨ */
		WaitForSingleObject(App3_App2_Sync_Sem, INFINITE);


		sys_Cycle_Cnt++;
		usSleep(100000);


		/* �ͷ� ATO ���ڿ��� VVS ���ź��� */
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

		Sleep(100);/* VC ʹ��Sleep*/
	}
}

/******************************************************************************************************************************************
*    @fn               LowLevelKeyboardProc
*    @brief            �����¼�����������
*    @detail           �����¼�������������Ϊ�ص�����
*
*    @return           ��
*
*    @REV    1.0.0         2021.08.09 Create
*******************************************************************************************************************************************/
LRESULT WINAPI LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	static DWORD lastHitTime = 0;

	if ((nCode == HC_ACTION) && ((wParam == WM_SYSKEYDOWN) || (wParam == WM_KEYDOWN)))
	{
		KBDLLHOOKSTRUCT hooked_key = *((KBDLLHOOKSTRUCT*)lParam);

		if (hooked_key.vkCode == 0xA0) /* SHIFT �������� */
		{
			if (hooked_key.time - lastHitTime > 1000) /* ������ΰ��µ�ʱ�����һ�ΰ��µ�ʱ�䳬���� 1�� */
			{
				KeyPress_SHIFT_Cnt = 0;
			}
			KeyPress_SHIFT_Cnt++;
		}
		else if (hooked_key.vkCode == 0x74) /* F5 ���ܰ��� */
		{
			if (hooked_key.time - lastHitTime > 1000) /* ������ΰ��µ�ʱ�����һ�ΰ��µ�ʱ�䳬���� 1�� */
			{
				KeyPress_F5_Cnt = 0;
			}
			KeyPress_F5_Cnt++;
		}

		if (KeyPress_SHIFT_Cnt > 0 && KeyPress_F5_Cnt > 0)
		{
			if (hooked_key.time - lastHitTime < 1000) /* �����������µ�����Ƿ���Ч */
			{
				TerminateProcess(slave_pi1.hProcess, 0);
				TerminateProcess(slave_pi2.hProcess, 0);

				/* ���ý����������еı�־Ϊ EnumReturnFalse����Ҫ�˳����� */
				is_Process_Running_Enable = EnumReturnFalse;
				///* �˳��������߳� */
				//_endthread();
				/* �˳��Լ� */
				exit(0);
			}
			else
			{
				/*- do nothing*/
			}
		}

		lastHitTime = hooked_key.time; /* ��¼����shift������ʱ�� */
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

/******************************************************************************************************************************************
*    @fn               LowLevelKeyboardProc
*    @brief            ִ�м��̰��������¼�
*    @detail           ִ�м��̰��������¼���Ϊ�̷߳�����
*
*    @return           ��
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
		/* ȡ��һ����Ϣ���������ָ���Ľṹ, ���ڼ������̰����¼������� */
		GetMessage(NULL, NULL, 0, 0);
	}
}


void system_Init()
{


	/* �������ߴ�һ������ATO���е��ź���,���ҳ�ʼ������Ϊ3�������ֵΪ3������Ӧ��3��ATO����Ϊһ��ATP���ڣ�*/
	App1_App2_Sync_Sem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, (LPCTSTR)App1_App2_Sync_Sem_Name);
	if (App1_App2_Sync_Sem == NULL) /* ��ʧ�ܣ�˵����������Ҫ�򿪵��ź��� */
	{
		/* ����һ����Ӧ���Ե��ź��� */
		App1_App2_Sync_Sem = CreateSemaphore(NULL, 3, 3, (LPCTSTR)App1_App2_Sync_Sem_Name);
		if (App1_App2_Sync_Sem == NULL) /* �����ź���ʧ�� */
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

	/* �������ߴ�һ������ATO���е��ź���,���ҳ�ʼ������Ϊ3�������ֵΪ3������Ӧ��3��ATO����Ϊһ��ATP���ڣ�*/
	App2_App3_Sync_Sem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, (LPCTSTR)App2_App3_Sync_Sem_Name);
	if (App2_App3_Sync_Sem == NULL) /* ��ʧ�ܣ�˵����������Ҫ�򿪵��ź��� */
	{
		/* ����һ����Ӧ���Ե��ź��� */
		App2_App3_Sync_Sem = CreateSemaphore(NULL, 0, 1, (LPCTSTR)App2_App3_Sync_Sem_Name);
		if (App2_App3_Sync_Sem == NULL) /* �����ź���ʧ�� */
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
		/* ������ VVS ���� ATO ���е��ź��� */
		App3_App2_Sync_Sem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, (LPCTSTR)App3_App2_Sync_Sem_Name);
	} while (App3_App2_Sync_Sem == NULL);

	printf("Cycle Num : %d, Open semaphore named '%s' successed!\n", sys_Cycle_Cnt, App3_App2_Sync_Sem_Name);
}

/******************************************************************************************************************************************
*    @fn               usSleep
*    @brief            ΢�뼶����ʱ����
*    @detail           ΢�뼶��ʱ��������ȷ��Ϊ 10΢�뼶������ʱ�䲹��
*
*    @return           ��
*
*    @REV    1.0.0    z.n    2021.03.15 Create
*******************************************************************************************************************************************/
static void usSleep(double usec)
{
	static int flag = 0;
	static __int64 nStart = 0; //��ʼ����

	__int64 nFreq = 0; //Ƶ��
	__int64 nEnd = 0; // ��ֹ����
	static double offset = 0;

	if (flag == 0)
	{
		flag = 1;
		QueryPerformanceCounter((LARGE_INTEGER*)&nStart);
		QueryPerformanceFrequency((LARGE_INTEGER*)&nFreq);
		// ��ǰ�����£�nFreq Ϊ 1000 0000
		offset = 1000000.0 / (double)nFreq; // ������ת��Ϊ΢��
	}
	usec += 200; // ��ʱ����м򵥵�ʱ�䲹��
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
/*���ƣ�       DebugData_To_LogFile_Writer                                                                                                 */
/*���ܣ�       ��������Ϣд�뵽�ļ���                                                                                                          */
/*������       ���ú��������ļ����ƣ�Ҫд���ļ���buff��buff�Ĵ�С�������Զ�Ӧ��ʮ�������ַ�������ʽд�뵽�ļ���                                           */
/*���룺       file_name                �ļ����ƣ���Я��·��                                                                                   */
/*            buf					   Ҫд������ݵ��׵�ַ                                                                                   */
/*            buf_len                  Ҫд������ݵĳ��ȣ��ֽ�����                                                                            */
/*����ֵ��     ��                                                                                                                           */
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