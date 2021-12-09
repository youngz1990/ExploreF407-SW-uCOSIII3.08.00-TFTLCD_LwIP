/*
 * Copyright (c) 2017 Simon Goldschmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "lwip/apps/snmp_opts.h"
#include "string.h"

#if defined(LWIP_PROVIDE_ERRNO)
int errno;
#endif

/* ---------------------------lwIP用到的系统资源------------------------ */
sys_mutex_t LwIP_Mutex_Arch;

/* --------------------------------任务资源---------------------------- */
OS_TCB    TCPIP_TaskTCB;
CPU_STK   TCPIP_TaskSTK[TCPIP_THREAD_STACKSIZE];

/* Initialize this module (see description in sys.h) */
void
sys_init(void)
{
  OS_ERR err;
  /* initialize sys_arch_protect global mutex */
  OSMutexCreate(&LwIP_Mutex_Arch,
                "LwIP Mutex Arch",
                &err);

  LWIP_ASSERT("failed to create LwIP_Mutex_Arch mutex",
              err != OS_ERR_NONE);
}

u32_t
sys_now(void)
{
  return OSTickCtr;
}

u32_t
sys_jiffies(void)
{
  return OSTickCtr;
}

#if SYS_LIGHTWEIGHT_PROT

sys_prot_t
sys_arch_protect(void)
{
  OS_ERR err;

  OSMutexPend(&LwIP_Mutex_Arch,
              0,
              OS_OPT_PEND_BLOCKING,
              0,
              &err);

  LWIP_ASSERT("LwIP_Mutex_Arch failed to take the mutex", err == OS_ERR_NONE);

  return 1;
}

void
sys_arch_unprotect(sys_prot_t pval)
{
  OS_ERR err;

  (void)pval;
  OSMutexPost(&LwIP_Mutex_Arch,
              OS_OPT_POST_NONE,
              &err);

  LWIP_ASSERT("LwIP_Mutex_Arch failed to give the mutex", err == OS_ERR_NONE);
}

#endif /* SYS_LIGHTWEIGHT_PROT */

#if LWIP_COMPAT_MUTEX == 0        //LWIP_COMPAT_MUTEX == 1代表使用兼容的mutex，既使用sem代替mutex

/* Create a new mutex*/
err_t
sys_mutex_new(sys_mutex_t *mutex)
{
  OS_ERR err;

  OSMutexCreate(mutex,
                "LwIP Mutex",
                &err);

  LWIP_ASSERT("mutex create failed", err == OS_ERR_NONE);

  if(err != OS_ERR_NONE)
  {
    SYS_STATS_INC(mutex.err);
    return ERR_MEM;
  }
  SYS_STATS_INC_USED(mutex);
  return ERR_OK;
}

void
sys_mutex_lock(sys_mutex_t *mutex)
{
  OS_ERR err;

  OSMutexPend(mutex,
              0,
              OS_OPT_PEND_BLOCKING,
              0,
              &err);

  LWIP_ASSERT("failed to take the mutex", err == OS_ERR_NONE);
}

void
sys_mutex_unlock(sys_mutex_t *mutex)
{
  OS_ERR err;

  OSMutexPost(mutex,
              OS_OPT_POST_NONE,
              &err);

  LWIP_ASSERT("failed to give the mutex", err == OS_ERR_NONE);
}

void
sys_mutex_free(sys_mutex_t *mutex)
{
  OS_ERR err;

  SYS_STATS_DEC(mutex.used);

  OSMutexDel(mutex,
             OS_OPT_DEL_NO_PEND,
             &err);

  LWIP_ASSERT("failed to free the mutex", err == OS_ERR_NONE);
}

#endif /* !LWIP_COMPAT_MUTEX */

err_t
sys_sem_new(sys_sem_t *sem, u8_t initial_count)
{
  OS_ERR err;

  LWIP_ASSERT("initial_count invalid (not 0 or 1)",
    (initial_count == 0) || (initial_count == 1));

  /* uCOS并没有严格区分二值信号量和计数信号量，需要使用者在代码中控制pend和post来实现想要的功能，一般情况下
     OSSemCreate函数的第三个参数为0时为二值信号量，一个任务post一个任务pend实现IPC
     OSSemCreate函数的第三个参数大于0时为计数信号量*/
  OSSemCreate(sem,                  
              "LwIP SEM",
              initial_count,
              &err);

  if(err != OS_ERR_NONE) {
    SYS_STATS_INC(sem.err);
    LWIP_ASSERT("creat sem failed", 0);
    return ERR_MEM;
  }
  SYS_STATS_INC_USED(sem);

  return ERR_OK;
}

void
sys_sem_signal(sys_sem_t *sem)
{
  OS_ERR err;

  OSSemPost(sem,
            OS_OPT_POST_1,
            &err);
  
  /* queue full is OK, this is a signal only... */
  LWIP_ASSERT("sys_sem_signal: sane return value",
    err == OS_ERR_NONE);
}

u32_t
sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout_ms)
{
  OS_ERR err;
  OS_TICK starttick = OSTickCtr;

  OSSemPend(sem,
            timeout_ms,
            OS_OPT_PEND_BLOCKING,
            0,
            &err);
  
  if(err != OS_ERR_NONE)
  {
    LWIP_ASSERT("taking semaphore failed", 0);
    return SYS_ARCH_TIMEOUT;
  }

  /* Old versions of lwIP required us to return the time waited.
     This is not the case any more. Just returning != SYS_ARCH_TIMEOUT
     here is enough. */
  return OSTickCtr - starttick;       //in case of warning
}

void
sys_sem_free(sys_sem_t *sem)
{
  OS_ERR err;

  OSSemDel(sem,
           OS_OPT_DEL_NO_PEND,
           &err);
  
  if(err == OS_ERR_NONE)
  {
    SYS_STATS_DEC(sem.used);
  }
  else
  {
    LWIP_ASSERT("fail to delete the sem", 0);
  }
}

int sys_sem_valid(sys_sem_t *sem)
{
  if (sem == SYS_SEM_NULL)
    return 0;
  else
    return 1;
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
  sem = SYS_SEM_NULL;
}

err_t
sys_mbox_new(sys_mbox_t *mbox, int size)
{
  OS_ERR err0, err1;

  OSQCreate(&(mbox->msg_queen),
            "LwIP MSQ",
            size,
            &err0);

  OSSemCreate(&(mbox->msg_sem),
              "LwIP MSQ_SEM",
              size,
              &err1);

  if((err0 != OS_ERR_NONE) ||(err1 != OS_ERR_NONE)) {
    SYS_STATS_INC(mbox.err);
    LWIP_ASSERT("fail to creat the mbox", 0);
    return ERR_MEM;
  }
  SYS_STATS_INC_USED(mbox);
  return ERR_OK;
}

/* 阻塞调用，
 * 如果消息队列满了也要一直阻塞当前任务直到消息被填充进队列
 * 因为阻塞，所以不能在中断中调用 */
void
sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
  OS_ERR err;

  OSSemPend(&(mbox->msg_sem),
            0,
            OS_OPT_PEND_BLOCKING,
            0,
            &err);
  if(err != OS_ERR_NONE)
  {
    LWIP_ASSERT("mbox post failed", 0);
    return;
  }

  OSQPost(&(mbox->msg_queen),
          msg,
          sizeof(void *),
          OS_OPT_POST_FIFO,
          &err);
  LWIP_ASSERT("mbox post failed", err == OS_ERR_NONE);
}

/* 非阻塞调用
 * 如果消息队列满了也直接返回，可以在中断中调用
 * uCos-III支持中断调用OSQPost() */
err_t
sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
  OS_ERR err;

  OSQPost(&(mbox->msg_queen),
          msg,
          sizeof(void *),
          OS_OPT_POST_FIFO,
          &err);
  if(err != OS_ERR_NONE)
  {
    LWIP_ASSERT("mbox trypost failed", 0);
    SYS_STATS_INC(mbox.err);
    return ERR_MEM; 
  }

  // 在中断中调用该函数不知道会不会有问题
  // 会检测是不是在中断中被调用，如果第三个参数是OS_OPT_PEND_NON_BLOCKING则不会返回错误
  OSSemPend(&(mbox->msg_sem),
            0,
            OS_OPT_PEND_NON_BLOCKING,
            0,
            &err);
  if(err != OS_ERR_NONE)
  {
    LWIP_ASSERT("mbox trypost failed", 0);
    return ERR_MEM;
  }

  return ERR_OK; 
}

/* 在引用该函数的地方提到
 *Same as @ref tcpip_callbackmsg_trycallback but calls sys_mbox_trypost_fromisr(),
 * mainly to help FreeRTOS, where calls differ between task level and ISR level. 
 * 因此他的实现保持和sys_mbox_trypost()一致 */
err_t
sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
  OS_ERR err;

  OSQPost(&(mbox->msg_queen),
          msg,
          sizeof(void *),
          OS_OPT_POST_FIFO,
          &err);
  if(err != OS_ERR_NONE)
  {
    LWIP_ASSERT("mbox trypost failed", 0);
    SYS_STATS_INC(mbox.err);
    return ERR_MEM; 
  }

  // 在中断中调用该函数不知道会不会有问题
  // 会检测是不是在中断中被调用，如果第三个参数是OS_OPT_PEND_NON_BLOCKING则不会返回错误
  OSSemPend(&(mbox->msg_sem),
            0,
            OS_OPT_PEND_NON_BLOCKING,
            0,
            &err);
  if(err != OS_ERR_NONE)
  {
    LWIP_ASSERT("mbox trypost failed", 0);
    return ERR_MEM;
  }

  return ERR_OK; 
}

/* 阻塞调用，在timeout_ms超时时间内一直阻塞 */
u32_t
sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout_ms)
{
  OS_ERR err;
  OS_MSG_SIZE size = 0;
  OS_TICK starttick = OSTickCtr;
  OS_TICK elapsetick = 0;
  
  *msg = OSQPend(&(mbox->msg_queen),
                 timeout_ms,
                 OS_OPT_PEND_BLOCKING,
                 &size,
                 0,
                 &err);
  elapsetick = OSTickCtr - starttick;
  if(err != OS_ERR_NONE)    //从队列中取消息失败
  {
    LWIP_ASSERT("mbox fetch failed", 0);
    *msg = NULL;
    return SYS_ARCH_TIMEOUT;
  }
  else      //从队列里取消息成功
  {
    OSSemPost(&(mbox->msg_sem),
              OS_OPT_POST_1,
              &err);
    if(err != OS_ERR_NONE)
    {
      LWIP_ASSERT("mbox sem operation error", 0);
      return SYS_ARCH_TIMEOUT;
    }
  }

  return elapsetick;
}

/* 非阻塞调用 */
u32_t
sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
  OS_ERR err;
  OS_MSG_SIZE size = 0;

  *msg = OSQPend(&(mbox->msg_queen),
                 0,
                 OS_OPT_PEND_NON_BLOCKING,
                 &size,
                 0,
                 &err);
  if(err != OS_ERR_NONE)
  {
    msg = NULL;
    LWIP_ASSERT("mbox tryfetch failed", 0);
    return SYS_MBOX_EMPTY;
  }

  OSSemPost(&(mbox->msg_sem),
            OS_OPT_POST_1,
            &err);
  if(err != OS_ERR_NONE)
  {
    msg = NULL;
    LWIP_ASSERT("mbox tryfetch failed", 0);
    return SYS_MBOX_EMPTY;
  }

  /* Old versions of lwIP required us to return the time waited.
     This is not the case any more. Just returning != SYS_ARCH_TIMEOUT
     here is enough. */
  return 0;
}

void
sys_mbox_free(sys_mbox_t *mbox)
{
  OS_ERR err;

  OSSemDel(&(mbox->msg_sem),
           OS_OPT_DEL_ALWAYS,
           &err);
  LWIP_ASSERT("sys_mbox_free failed", err == OS_ERR_NONE);

  OSQDel(&(mbox->msg_queen),
         OS_OPT_DEL_ALWAYS,
         &err);
  LWIP_ASSERT("sys_mbox_free failed", err == OS_ERR_NONE);   

  SYS_STATS_DEC(mbox.used);
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
  if (mbox == SYS_MBOX_NULL)
    return 0;
  else
    return 1;
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
  mbox = SYS_MBOX_NULL;
}

sys_thread_t
sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
  OS_ERR err;

  const char *taskname_tcpip = TCPIP_THREAD_NAME;
  const char *taskname_snmp = SNMP_THREAD_NAME;

  if(memcmp((void *)name, (void *)taskname_tcpip, 5) == 0)
  {
    OSTaskCreate((OS_TCB        *)&TCPIP_TaskTCB,          //任务控制块
                 (CPU_CHAR      *)name,                     //任务名
                 (OS_TASK_PTR    )thread,                   //任务函数地址
                 (void          *)arg,                      //传递给任务的参数
                 (OS_PRIO        )prio,                     //任务优先级
                 (CPU_STK       *)TCPIP_TaskSTK,           //任务栈基地址
                 (CPU_STK_SIZE   )stacksize / 10,           //任务栈的深度标记，当未使用的栈空间仅剩10%时就达到栈的极限深度
                 (CPU_STK_SIZE   )stacksize,                //任务栈大小
                 (OS_MSG_QTY     )0,                        //任务内部消息队列能接受的消息的最大数目
                 (OS_TICK        )0,                        //使能时间片调度时，这个参数指定时间片长度，如果为0，则为默认时间片长度（时钟节拍频率除以10）
                 (void          *)0,                        //任务补充储存器地址，用来扩展任务的TCB，比如在任务切换时，可以用补充储存区的空间来存放浮点运算寄存器的内容
                 (OS_OPT         )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                  /*  定义如下：
                  OS_OPT_TASK_NONE         没有选择任何项
					        OS_OPT_TASK_STK_CHK      使能检测任务栈，统计任务栈已用的和未用的
					        OS_OPT_TASK_STK_CLR      在创建任务时，清零任务栈
					        OS_OPT_TASK_SAVE_FP      如果CPU有浮点寄存器，则在任务切换时保存浮点寄存器的内容
				          */
                 (OS_ERR        *)&err);
  }

  LWIP_ASSERT("task creation failed", err == OS_ERR_NONE);
}

#if LWIP_NETCONN_SEM_PER_THREAD
#if configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0

sys_sem_t *
sys_arch_netconn_sem_get(void)
{
  void* ret;
  TaskHandle_t task = xTaskGetCurrentTaskHandle();
  LWIP_ASSERT("task != NULL", task != NULL);

  ret = pvTaskGetThreadLocalStoragePointer(task, 0);
  return ret;
}

void
sys_arch_netconn_sem_alloc(void)
{
  void *ret;
  TaskHandle_t task = xTaskGetCurrentTaskHandle();
  LWIP_ASSERT("task != NULL", task != NULL);

  ret = pvTaskGetThreadLocalStoragePointer(task, 0);
  if(ret == NULL) {
    sys_sem_t *sem;
    err_t err;
    /* need to allocate the memory for this semaphore */
    sem = mem_malloc(sizeof(sys_sem_t));
    LWIP_ASSERT("sem != NULL", sem != NULL);
    err = sys_sem_new(sem, 0);
    LWIP_ASSERT("err == ERR_OK", err == ERR_OK);
    LWIP_ASSERT("sem invalid", sys_sem_valid(sem));
    vTaskSetThreadLocalStoragePointer(task, 0, sem);
  }
}

void sys_arch_netconn_sem_free(void)
{
  void* ret;
  TaskHandle_t task = xTaskGetCurrentTaskHandle();
  LWIP_ASSERT("task != NULL", task != NULL);

  ret = pvTaskGetThreadLocalStoragePointer(task, 0);
  if(ret != NULL) {
    sys_sem_t *sem = ret;
    sys_sem_free(sem);
    mem_free(sem);
    vTaskSetThreadLocalStoragePointer(task, 0, NULL);
  }
}

#else /* configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 */
#error LWIP_NETCONN_SEM_PER_THREAD needs configNUM_THREAD_LOCAL_STORAGE_POINTERS
#endif /* configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 */

#endif /* LWIP_NETCONN_SEM_PER_THREAD */

#if 0/* LWIP_FREERTOS_CHECK_CORE_LOCKING */
#if LWIP_TCPIP_CORE_LOCKING

/** Flag the core lock held. A counter for recursive locks. */
static u8_t lwip_core_lock_count;
static TaskHandle_t lwip_core_lock_holder_thread;

void
sys_lock_tcpip_core(void)
{
   sys_mutex_lock(&lock_tcpip_core);
   if (lwip_core_lock_count == 0) {
     lwip_core_lock_holder_thread = xTaskGetCurrentTaskHandle();
   }
   lwip_core_lock_count++;
}

void
sys_unlock_tcpip_core(void)
{
   lwip_core_lock_count--;
   if (lwip_core_lock_count == 0) {
       lwip_core_lock_holder_thread = 0;
   }
   sys_mutex_unlock(&lock_tcpip_core);
}

#endif /* LWIP_TCPIP_CORE_LOCKING */

#if !NO_SYS
static TaskHandle_t lwip_tcpip_thread;
#endif

void
sys_mark_tcpip_thread(void)
{
#if !NO_SYS
  lwip_tcpip_thread = xTaskGetCurrentTaskHandle();
#endif
}

void
sys_check_core_locking(void)
{
  /* Embedded systems should check we are NOT in an interrupt context here */
  /* E.g. core Cortex-M3/M4 ports:
         configASSERT( ( portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK ) == 0 );

     Instead, we use more generic FreeRTOS functions here, which should fail from ISR: */
  taskENTER_CRITICAL();
  taskEXIT_CRITICAL();

#if !NO_SYS
  if (lwip_tcpip_thread != 0) {
    TaskHandle_t current_thread = xTaskGetCurrentTaskHandle();

#if LWIP_TCPIP_CORE_LOCKING
    LWIP_ASSERT("Function called without core lock",
                current_thread == lwip_core_lock_holder_thread && lwip_core_lock_count > 0);
#else /* LWIP_TCPIP_CORE_LOCKING */
    LWIP_ASSERT("Function called from wrong thread", current_thread == lwip_tcpip_thread);
#endif /* LWIP_TCPIP_CORE_LOCKING */
  }
#endif /* !NO_SYS */
}

#endif /* LWIP_FREERTOS_CHECK_CORE_LOCKING*/
