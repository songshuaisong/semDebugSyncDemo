using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WindowsFormsApplication
{
    public partial class Form1 : Form
    {
        /// <summary>
        /// 获取当前系统性能计数
        /// </summary>
        /// <param name="lpPerformanceCount"></param>
        /// <returns></returns>
        [DllImport("Kernel32.dll")]
        private static extern bool QueryPerformanceCounter(out long lpPerformanceCount);
        /// <summary>
        /// 获取当前系统性能频率
        /// </summary>
        /// <param name="lpFrequency"></param>
        /// <returns></returns>
        [DllImport("Kernel32.dll")]
        private static extern bool QueryPerformanceFrequency(out long lpFrequency);



        Semaphore App2_App3_Sync_Semaphore = null;
        Semaphore APP3_App2_Sync_Semaphore = null;
        Semaphore APP3_App1_Sync_Semaphore = null;


        Thread thread_Main;
        Thread thread_T1;
        Thread thread_T2;
        Thread thread_T3;
        Thread thread_T4;

        bool SysFalg_MainThread_MainProsee_Enable = true;
        bool SysFalg_Thread_T1_Enable = true;
        bool SysFalg_Thread_T2_Enable = true;
        bool SysFalg_Thread_T3_Enable = true;
        bool SysFalg_Thread_T4_Enable = true;




        long sys_Cycle_Cnt = 0;
        public Form1()
        {
            InitializeComponent();     
            
            /* 取消跨线程的访问 */
            Control.CheckForIllegalCrossThreadCalls = false;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            /* 创建或者打开信号量 */
            App2_App3_Sync_Semaphore = new Semaphore(0, 1, "App2_App3_Sync_Sem");
            APP3_App2_Sync_Semaphore = new Semaphore(1, 1, "App3_App2_Sync_Sem");
            APP3_App1_Sync_Semaphore = new Semaphore(0, 1, "App3_App1_Sync_Sem");

            /* 主要业务的逻辑循环处理进程 */
            thread_Main = new Thread(Main_Process_Cycle);
            thread_Main.SetApartmentState(ApartmentState.STA);
            thread_Main.Start();/* 启动接受数据的线程 */

            /* 接收 线路仿真 数据*/
            thread_T1 = new Thread(thread_T1_Proc);
            thread_T1.SetApartmentState(ApartmentState.STA);
            thread_T1.Start();//启动接受数据的线程

            /* 接收 线路仿真 数据*/
            thread_T2 = new Thread(thread_T2_Proc);
            thread_T2.SetApartmentState(ApartmentState.STA);
            thread_T2.Start();//启动接受数据的线程

            /* 接收 线路仿真 数据*/
            thread_T3 = new Thread(thread_T3_Proc);
            thread_T3.SetApartmentState(ApartmentState.STA);
            thread_T3.Start();//启动接受数据的线程

            /* 接收 线路仿真 数据*/
            thread_T4 = new Thread(thread_T4_Proc);
            thread_T4.SetApartmentState(ApartmentState.STA);
            thread_T4.Start();//启动接受数据的线程
        }

        void thread_T1_Proc()
        {
            while (SysFalg_Thread_T1_Enable)
            {
                Thread.Sleep(1000);
            }
        }
        void thread_T2_Proc()
        {
            while (SysFalg_Thread_T2_Enable)
            {
                Thread.Sleep(1000);
            }
        }
        void thread_T3_Proc()
        {
            while (SysFalg_Thread_T3_Enable)
            {
                Thread.Sleep(1000);
            }
        }
        void thread_T4_Proc()
        {
            while (SysFalg_Thread_T4_Enable)
            {
                Thread.Sleep(1000);
                textBox1.Text = str;
            }
        }

        String str = "";
        String str_Last = "";
        void Main_Process_Cycle()
        {
            while (SysFalg_MainThread_MainProsee_Enable)
            {
                //Console.WriteLine("Cycle Num: " + sys_Cycle_Cnt + ", WaitForSingleObject(" + "App2_App3_Sync_Sem" + ")!" + "");
                str_Last  = "\r\nCycle Num: " + sys_Cycle_Cnt + ", WaitForSingleObject(" + "App2_App3_Sync_Sem" + ")!" + "";
                str += str_Last;
                App2_App3_Sync_Semaphore.WaitOne();


                sys_Cycle_Cnt++;


                try
                {

                    APP3_App2_Sync_Semaphore.Release();
                    //Console.WriteLine("Cycle Num: " + sys_Cycle_Cnt + ", Release(" + "APP3_App2_Sync_Sem" + ")!" + "");
                    str_Last = "\r\nCycle Num: " + sys_Cycle_Cnt + ", Release(" + "APP3_App2_Sync_Sem" + ")!" + "";
                    str += str_Last;
                    
                    str += "\r\n";
                }
                catch
                {
                }


                if (sys_Cycle_Cnt % 3 == 0)
                {
                    try
                    {
                        APP3_App1_Sync_Semaphore.Release();
                        //Console.WriteLine("Cycle Num: " + sys_Cycle_Cnt + ", Release(" + "APP3_App1_Sync_Sem" + ")!" + "");
                        str_Last = "\r\nCycle Num: " + sys_Cycle_Cnt + ", Release(" + "APP3_App1_Sync_Sem" + ")!" + "";
                        str += str_Last;
                    }
                    catch
                    {

                    }

                }

                usSleep(100000);
            }
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {
            //文本框选中的起始点在最后
            textBox1.SelectionStart = textBox1.TextLength;
            //将控件内容滚动到当前插入符号位置
            textBox1.ScrollToCaret();
        }
    



        private void frmMain_FormClosing(object sender, FormClosingEventArgs e)
        {

            if (null != thread_Main) /* 关闭取消周期处理的线程 */
            {
                /* 设置周期处理的线程的运行的标志为false */
                SysFalg_MainThread_MainProsee_Enable = false;
                thread_Main.Abort(); /* 取消线程 */

                SysFalg_Thread_T1_Enable = false;
                SysFalg_Thread_T2_Enable = false;
                SysFalg_Thread_T3_Enable = false;
                SysFalg_Thread_T4_Enable = false;
            }
        }


        void usSleep(double usec)
        {
             int flag = 0;
            long nStart = 0; //起始计数

            long nFreq; //频率
            long nEnd = 0; // 终止计数
            double offset = 0;

            if (flag == 0)
            {
                flag = 1;
                QueryPerformanceCounter(out  nStart);
                QueryPerformanceFrequency(out nFreq);
                // 当前环境下，nFreq 为 1000 0000
                offset = 1000000.0 / (double)nFreq; // 将计数转换为微秒
            }
            usec += 200; // 给时间进行简单的时间补偿
            for (;;)
            {
                QueryPerformanceCounter(out nEnd);
                if (usec <= (double)(nEnd - nStart) * offset)
                {
                    break;
                }
            }
            QueryPerformanceCounter(out nStart);
        }

        private void textBox1_DoubleClick(object sender, EventArgs e)
        {
            str = "";
            str = str_Last;
        }
    }
}
