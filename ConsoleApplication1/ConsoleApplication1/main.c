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

		/* �ȴ� VVS ���Ƶ��ź���֪ͨ */
		WaitForSingleObject(App3_App1_Sync_Sem, INFINITE);


		sys_Cycle_Cnt++;
		// usSleep(300000);


		
		
		/* �ͷ�3���ź�������ʾATP�Ѿ��ɹ�����һ������ */
		ReleaseSemaphore(App1_App2_Sync_Sem, 3, NULL);
		printf("Cycle Num : %d, ReleaseSemaphore(%s)!\n", sys_Cycle_Cnt, App1_App2_Sync_Sem_Name);


	}

	return 0;
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

	/* �������ߴ�����VVS����ATP���ڵ��ź��� */
	App3_App1_Sync_Sem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, (LPCTSTR)App3_App1_Sync_Sem_Name);
	if (App3_App1_Sync_Sem == NULL) /* ��ʧ�ܣ�˵����������Ҫ�򿪵��ź��� */
	{
		/* ����һ����Ӧ���Ե��ź��� */
		App3_App1_Sync_Sem = CreateSemaphore(NULL, 0, 1, (LPCTSTR)App3_App1_Sync_Sem_Name);
		if (App3_App1_Sync_Sem == NULL) /* �����ź���ʧ�� */
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

/*

error C4996: 'fopen': This function or variable may be unsafe. Consider using fopen_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.













*/