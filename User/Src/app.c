/*
  ******************************************************************************
  * @file           : app.c
  * @brief          : 顶层逻辑实现
  ******************************************************************************
  * @attention
  *
  * 
  *
  ******************************************************************************
  */
#include "app.h"
#include "lwip/tcp.h"
#include "lwip/ip.h"

/* -------------------------------静态全局变量---------------------------- */
/* ----------------------------启动任务StartTask-------------------------- */
static OS_TCB   App_StartTaskTCB;
static CPU_STK  App_StartTaskStk[APP_CFG_TASK_START_STK_SIZE];

/* ----------------------------以太网任务EthTask-------------------------- */
static OS_TCB   App_EthTaskTCB;
static CPU_STK  App_EthTaskStk[APP_CFG_TASK_ETH_STK_SIZE];

/* --软定时器TMRDBG-- */
#if (APP_CFG_DBG_TMR > 0u)
static OS_TMR   App_TMRDBG;
#endif

/* ---------------------------------函数声明----------------------------- */
static void   App_StartTask(void *p_arg);
static void   App_EthTask(void *p_arg);
static void   App_ObjectInit(OS_ERR *err);

#if (APP_CFG_DBG_TMR > 0u)
static void App_TMRDBGCallback(void *p_tmr, void *p_arg);
#endif


/*
*********************************************************************************************************
*	函    数: App_Start()
*	说    明: 启动任务
*	形    参: *p_err,错误类型
*	返    回: 无
*********************************************************************************************************
*/
void App_Start(void)
{
  OS_ERR            os_err;

  /* 基本硬件操作 */
  BSP_OS_TickInit();      //配置Systick，定时间隔，中断优先级等。由于Systick被操作系统占用了，用于HAL库中用于更新uwTick的定时器由TIM7代替

  /* 初始化uCOS内核 */
  OSInit(&os_err);           //初始化uCOS内核

  /* 创建一个启动任务（也就是主任务）。启动任务会创建所有的应用程序任务 */
  OSTaskCreate((OS_TCB        *)&App_StartTaskTCB,                    //任务控制块
               (CPU_CHAR      *)"App Task Start",                     //任务名
               (OS_TASK_PTR    )App_StartTask,                        //任务函数地址
               (void          *)0,                                    //传递给任务的参数
               (OS_PRIO        )APP_CFG_TASK_START_PRIO,              //任务优先级
               (CPU_STK       *)App_StartTaskStk,                     //任务栈基地址
               (CPU_STK_SIZE   )APP_CFG_TASK_START_STK_SIZE / 10,     //任务栈的深度标记，当未使用的栈空间仅剩10%时就达到栈的极限深度
               (CPU_STK_SIZE   )APP_CFG_TASK_START_STK_SIZE,          //任务栈大小
               (OS_MSG_QTY     )0,                                    //任务内部消息队列能接受的消息的最大数目
               (OS_TICK        )0,                                    //使能时间片调度时，这个参数指定时间片长度，如果为0，则为默认时间片长度（时钟节拍频率除以10）
               (void          *)0,                                    //任务补充储存器地址，用来扩展任务的TCB，比如在任务切换时，可以用补充储存区的空间来存放浮点运算寄存器的内容
               (OS_OPT         )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                /*  定义如下：
                OS_OPT_TASK_NONE         没有选择任何项
					      OS_OPT_TASK_STK_CHK      使能检测任务栈，统计任务栈已用的和未用的
					      OS_OPT_TASK_STK_CLR      在创建任务时，清零任务栈
					      OS_OPT_TASK_SAVE_FP      如果CPU有浮点寄存器，则在任务切换时保存浮点寄存器的内容
				        */
               (OS_ERR        *)&os_err);

  /* 启动任务调度 */
  OSStart(&os_err);

  /* 程序永远不应该运行到这个位置 */
  if(os_err != OS_ERR_NONE)
  {

  }
}

static void App_StartTask(void *p_arg)
{
  OS_ERR    os_err = OS_ERR_NONE;
  BSP_ErrorTypedef  bsp_err = BSP_ERROR_NONE;

  (void)p_arg;

  BSP_OS_TickEnable();
  CPU_Init();     /* 这个函数必须调用，关系到OS_StatTask()统计CPU使用率，关系到中断时间测量 */
  
#if OS_CFG_STAT_TASK_EN > 0u
  OSStatTaskCPUUsageInit(&os_err);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
  CPU_IntDisMeasMaxCurReset();
#endif

  /* 液晶屏初始化 */
#if APP_CFG_DBG_LCD > 0
  bsp_err = BSP_LCDInit();
  if(bsp_err != BSP_ERROR_NONE)
  {
    //TODO
  }
#endif  /* APP_CFG_DBG_LCD */

  /* 创建内核对象 */ 
  App_ObjectInit(&os_err);     //app顶层代码需要的资源
  if(os_err != OS_ERR_NONE)
  {
    //TODO
  }

  os_err = App_LCDObjectInit();    //lcd液晶屏操作需要的资源
  if(os_err != OS_ERR_NONE)
  {
    //TODO
  }

  /* 创建任务 */
  LWIP_TCPIPInit();     //这里创建了tcpip_thread和ethrx_thread两个任务
  OSTaskCreate((OS_TCB        *)&App_EthTaskTCB,                    //任务控制块
               (CPU_CHAR      *)"App Eth task",                     //任务名
               (OS_TASK_PTR    )App_EthTask,                        //任务函数地址
               (void          *)0,                                    //传递给任务的参数
               (OS_PRIO        )APP_CFG_TASK_ETH_PRIO,              //任务优先级
               (CPU_STK       *)App_EthTaskStk,                     //任务栈基地址
               (CPU_STK_SIZE   )APP_CFG_TASK_START_STK_SIZE / 10,     //任务栈的深度标记，当未使用的栈空间仅剩10%时就达到栈的极限深度
               (CPU_STK_SIZE   )APP_CFG_TASK_START_STK_SIZE,          //任务栈大小
               (OS_MSG_QTY     )0,                                    //任务内部消息队列能接受的消息的最大数目
               (OS_TICK        )0,                                    //使能时间片调度时，这个参数指定时间片长度，如果为0，则为默认时间片长度（时钟节拍频率除以10）
               (void          *)0,                                    //任务补充储存器地址，用来扩展任务的TCB，比如在任务切换时，可以用补充储存区的空间来存放浮点运算寄存器的内容
               (OS_OPT         )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                /*  定义如下：
                OS_OPT_TASK_NONE         没有选择任何项
					      OS_OPT_TASK_STK_CHK      使能检测任务栈，统计任务栈已用的和未用的
					      OS_OPT_TASK_STK_CLR      在创建任务时，清零任务栈
					      OS_OPT_TASK_SAVE_FP      如果CPU有浮点寄存器，则在任务切换时保存浮点寄存器的内容
				        */
               (OS_ERR        *)&os_err);
  
  /* LCD调试界面初始化 */
#if APP_CFG_DBG_LCD > 0
  /* 完成调试界面的初始化 */
  os_err = APP_DBGLCDInit(OSTaskDbgListPtr);
  if(os_err != OS_ERR_NONE)
  {
    //TODO
  }
#endif

  /* 其他事务 */
  while(DEF_ON)
  {
#if APP_CFG_DBG_LCD > 0
    APP_DBGLCDFresh(OSTaskDbgListPtr);
#endif
    OSTimeDly(500, OS_OPT_TIME_PERIODIC, &os_err);
    
#if (APP_CFG_DBG_TMR == 0u)
    HAL_GPIO_TogglePin(STATUS_LED0_GPIO_Port, STATUS_LED0_Pin);
#endif
  }
}

void App_EthTask(void *p_arg)
{
  OS_ERR os_err;
  err_t lwip_err;

  ip_addr_t server_ip;
  uint16_t data_len = 0;
  void *data = NULL;
  struct netbuf *buf = NULL;
  struct netconn *conn = NULL;

  IP4_ADDR(&server_ip, IP_SERVER_ADDR0, IP_SERVER_ADDR1, IP_SERVER_ADDR2, IP_SERVER_ADDR3);
  LWIP_NETIFInit();

  while(1)
  {
    if(conn == NULL)      //conn实体不存在，申请conn并尝试连接服务器
    {
      conn = netconn_new(NETCONN_TCP);
      lwip_err = netconn_connect(conn, &server_ip, 65000);
      if(lwip_err != ERR_OK)       //连接不上，删除conn实体并重新申请连接
      {
        if(netconn_delete(conn) == ERR_OK)
        {
          conn = NULL;      //netconn_delete会删除conn实体但是并不会把conn置位NULL
        }
        //检查物理层状态
        //LWIP_NETIFCheck();
        OSTimeDly(500, OS_OPT_TIME_PERIODIC, &os_err);
        continue;
      }
      else    //连接申请成功，打开保活功能
      {
        conn->pcb.tcp->so_options |= SOF_KEEPALIVE;
      }
    }
    else        //conn实体存在，判断连接状态
    {
      if(conn->state == NETCONN_CLOSE)      //连接已经被关闭，删除conn实体重新申请连接
      {
        if(netconn_delete(conn) == ERR_OK)
        {
          conn = NULL;      //netconn_delete会删除conn实体但是并不会把conn置位NULL
        }
        continue;
      }   //剩下的NETCONN_NONE、正在发送数据、保持连接、listen状态，先不做任何处理
    }
    lwip_err = netconn_recv(conn, &buf);
    if(lwip_err == ERR_OK)
    {
      lwip_err = netbuf_data(buf, &data, &data_len);
      if(lwip_err == ERR_OK)    //数据处理
      {
        netbuf_delete(buf);     //释放动态申请的netbuf实例
        netconn_write(conn, data, data_len, NETCONN_COPY);
      }
    }
    else     //其他错误
    {
      if(netconn_delete(conn) == ERR_OK)    //删除连接
      {
        conn = NULL;      //netconn_delete会删除conn实体但是并不会把conn置位NULL
      }
      continue;
    }
  }
}
/*
*********************************************************************************************************
*	函    数: App_ObjectInit()
*	说    明: 顶层app应用代码需要用到的资源初始化
*	形    参: *p_err,错误类型
*	返    回: 无
*********************************************************************************************************
*/
static void App_ObjectInit(OS_ERR *err)
{
  /* 软定器对象 */
#if (APP_CFG_DBG_TMR > 0u)
  OSTmrCreate((OS_TMR           *)&App_TMRDBG,          //软定时器对象
             (CPU_CHAR          *)"soft timer1",      //软定时器名
             (OS_TICK            )0,                 //软定时周期，时基为10HZ，定时1s
             (OS_TICK            )1000,                 //软定时器周期重载值
             (OS_OPT             )OS_OPT_TMR_PERIODIC,//软定时器选项，周期性定时
             (OS_TMR_CALLBACK_PTR)App_TMRDBGCallback,   //软定时器回调函数
             (void              *)0,                  //传递给回调的参数
             (OS_ERR            *)err);
  OSTmrStart(&App_TMRDBG, err);
#endif
}

#if (APP_CFG_DBG_TMR > 0u)
static void App_TMRDBGCallback(void *p_tmr, void *p_arg)
{
  HAL_GPIO_TogglePin(STATUS_LED0_GPIO_Port, STATUS_LED0_Pin);
}
#endif

