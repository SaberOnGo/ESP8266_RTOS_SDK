#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtthread.h>

//#define SHOW_DEBUG_INFO
//#define SHOW_QUE_DEBUG_INFO
volatile portSTACK_TYPE *pxCurrentTCB = 0;
static rt_thread_t cur_old = 0;
extern rt_thread_t rt_current_thread;
static unsigned short mq_index = 0;
static unsigned short ms_index = 0;

void rt_hw_context_switch(rt_uint32_t from, rt_uint32_t to)
{
#ifdef SHOW_DEBUG_INFO
    ets_printf("Switch1 cur:%s %d\n",rt_current_thread->name,WDEV_NOW());
#endif
    PendSV(1);
#ifdef SHOW_DEBUG_INFO
    ets_printf("Switch2 cur:%s %d\n",rt_current_thread->name,WDEV_NOW());
#endif
} 
void rt_hw_context_switch_interrupt(rt_uint32_t from, rt_uint32_t to)
{
    pxCurrentTCB = (portSTACK_TYPE *)to;
#ifdef SHOW_DEBUG_INFO
    if (cur_old != rt_current_thread)
    {
        if (cur_old)
            ets_printf("TaskSwitch %s -> %s\n",cur_old->name,rt_current_thread->name);
        else
            ets_printf("TaskSwitchTo %s\n",rt_current_thread->name);
    }
#endif
    cur_old = rt_current_thread;
} 
rt_base_t rt_hw_interrupt_disable(void)
{
    vPortEnterCritical();
    return cpu_sr;
}
void rt_hw_interrupt_enable(rt_base_t level)
{
    vPortExitCritical();
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit)
{
    #define SET_STKREG(r,v) sp[(r) >> 2] = (rt_uint32_t)(v)
    rt_uint32_t *sp, *tp;

    /* Create interrupt stack frame aligned to 16 byte boundary */
    sp = (rt_uint32_t*) (((rt_uint32_t)(stack_addr) - XT_CP_SIZE - XT_STK_FRMSZ) & ~0xf);
    /* Clear the entire frame (do not use memset() because we don't depend on C library) */
    for (tp = sp; tp <= (rt_uint32_t*) stack_addr; ++tp)
        *tp = 0;

    /* Explicitly initialize certain saved registers */
    SET_STKREG( XT_STK_PC,  tentry                        );  /* task entrypoint                  */
    SET_STKREG( XT_STK_A0,  0                             );  /* to terminate GDB backtrace       */
    SET_STKREG( XT_STK_A1,  (rt_uint32_t)sp + XT_STK_FRMSZ);  /* physical top of stack frame      */
    SET_STKREG( XT_STK_A2,  (rt_uint32_t)parameter        );  /* parameters      */
    SET_STKREG( XT_STK_EXIT, _xt_user_exit                );  /* exit */

    /* Set initial PS to int level 0, EXCM disabled ('rfe' will enable), user mode. */
    #ifdef __XTENSA_CALL0_ABI__
    SET_STKREG( XT_STK_PS,      PS_UM | PS_EXCM     );
    #else
    /* + for windowed ABI also set WOE and CALLINC (pretend task was 'call4'd). */
    SET_STKREG( XT_STK_PS,      PS_UM | PS_EXCM | PS_WOE | PS_CALLINC(1) );
    #endif

    return (rt_uint8_t *)sp;
}

void ICACHE_FLASH_ATTR rtthread_startup(void)
{
    /* init tick */
    rt_system_tick_init();

    /* init kernel object */
    rt_system_object_init();

    /* init timer system */
    rt_system_timer_init();

    /* init scheduler system */
    rt_system_scheduler_init();

    /* init timer thread */
    rt_system_timer_thread_init();

    /* init idle thread */
    rt_thread_idle_init();
}
void rt_hw_console_output(const char *str)
{
    ets_printf(str);
}

#if 0
void ICACHE_FLASH_ATTR ff(void *pp)
{
    static int thread_i = 0;
    if (thread_i == 0)
    {
        vPortExitCritical();
        thread_i = 1;
    }
    while (1)
    {
        ets_printf("%d run %d\n",pp,rt_tick_get());
        rt_thread_delay(100);
    }
}
#endif

signed portBASE_TYPE ICACHE_FLASH_ATTR xTaskGenericCreate( pdTASK_CODE pxTaskCode, const signed char * const pcName, unsigned short usStackDepth, void *pvParameters, unsigned portBASE_TYPE uxPriority, xTaskHandle *pxCreatedTask, portSTACK_TYPE *puxStackBuffer, const xMemoryRegion * const xRegions )
{
    signed portBASE_TYPE xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    rt_thread_t thread = *pxCreatedTask = rt_thread_create(pcName,pxTaskCode,pvParameters,usStackDepth*sizeof(portSTACK_TYPE),20-uxPriority,10);
    if (*pxCreatedTask != 0)
    {
#ifdef SHOW_DEBUG_INFO
        ets_printf("TaskCreate name:%s pri:%d size:%d\n",pcName,(20-uxPriority),usStackDepth*sizeof(portSTACK_TYPE));
#endif
        rt_thread_startup(*pxCreatedTask);
        xReturn = pdPASS;
    }
    return xReturn;
}
void ICACHE_FLASH_ATTR vTaskDelete(xTaskHandle xTaskToDelete)
{
    rt_thread_t thread = xTaskToDelete;
    if (xTaskToDelete == 0)
        thread = rt_current_thread;
#ifdef SHOW_DEBUG_INFO
    ets_printf("TaskDelete name:%s\n",thread->name);
#endif
    rt_thread_delete(thread);
    rt_schedule();
}
unsigned portBASE_TYPE ICACHE_FLASH_ATTR uxTaskGetStackHighWaterMark( xTaskHandle xTask )
{
    register unsigned short usCount = 0U;
    rt_thread_t thread = xTask;
    if (xTask == 0)
        thread = rt_current_thread;
    rt_uint8_t *ptr = (rt_uint8_t *)thread->stack_addr;
    while (*ptr == '#')
    {
        usCount ++;
        ptr ++;
    }
    usCount /= sizeof( portSTACK_TYPE );
    return usCount;
}

void ICACHE_FLASH_ATTR vTaskStartScheduler(void) { xPortStartScheduler(); }
void ICACHE_FLASH_ATTR vTaskDelay(portTickType xTicksToDelay) { rt_thread_delay(xTicksToDelay); }
void ICACHE_FLASH_ATTR vTaskSuspendAll(void) { rt_enter_critical(); }
signed portBASE_TYPE ICACHE_FLASH_ATTR xTaskResumeAll( void ) { rt_exit_critical();return pdTRUE; }
portTickType ICACHE_FLASH_ATTR xTaskGetTickCount(void) { return rt_tick_get(); }
void ICACHE_FLASH_ATTR vTaskSwitchContext(void) { rt_interrupt_enter();rt_schedule();rt_interrupt_leave(); }
void ICACHE_FLASH_ATTR xPortSysTickHandle(void) { rt_interrupt_enter();rt_tick_increase();rt_interrupt_leave(); }

extern rt_mailbox_t rt_fmq_create(const char *name, rt_size_t item, rt_size_t size, rt_uint8_t flag);
extern rt_err_t rt_fmq_delete(rt_mailbox_t mb);
extern rt_err_t rt_fmq_send(rt_mailbox_t mb, void* value, rt_int32_t pos, rt_int32_t timeout);
extern rt_err_t rt_fmq_recv(rt_mailbox_t mb, void *value, rt_int32_t peek, rt_int32_t timeout);
xQueueHandle ICACHE_FLASH_ATTR xQueueGenericCreate( unsigned portBASE_TYPE uxQueueLength, unsigned portBASE_TYPE uxItemSize, unsigned char ucQueueType )
{
    char name[10] = {0};
    rt_object_t obj = 0;
    if (uxItemSize <= 0 || uxQueueLength <= 0)
    {
        sprintf(name,"s%02d",((++ms_index)%100));
        obj = (rt_object_t)rt_sem_create(name,0,RT_IPC_FLAG_PRIO);
    }
    else
    {
        sprintf(name,"q%02d",((++mq_index)%100));
        obj = (rt_object_t)rt_fmq_create(name,uxItemSize,uxQueueLength,RT_IPC_FLAG_PRIO);
    }
#ifdef SHOW_QUE_DEBUG_INFO
    ets_printf("QueueCreate name:%s count:%d size:%d\n",name,uxQueueLength,uxItemSize);
#endif
    return obj;
}
void ICACHE_FLASH_ATTR vQueueDelete( xQueueHandle xQueue )
{
    rt_object_t obj = xQueue;
    if (obj == 0)
        return;
#ifdef SHOW_QUE_DEBUG_INFO
    ets_printf("QueueDelete name:%s\n",obj->name);
#endif
    if (obj->type == RT_Object_Class_Semaphore)
        rt_sem_delete((rt_sem_t)obj);
    else
        rt_fmq_delete((rt_mailbox_t)obj);
}
signed portBASE_TYPE ICACHE_FLASH_ATTR xQueueGenericSend( xQueueHandle xQueue, const void * const pvItemToQueue, portTickType xTicksToWait, portBASE_TYPE xCopyPosition )
{
    rt_object_t obj = xQueue;
    if (obj == 0)
        return errQUEUE_FULL;
#ifdef SHOW_QUE_DEBUG_INFO
    ets_printf("QueueSend cur:%s name:%s wait:%d pos:%d\n",rt_current_thread->name,obj->name,xTicksToWait,xCopyPosition);
#endif
    rt_err_t err = RT_EOK;
    if (obj->type == RT_Object_Class_Semaphore)
        err = rt_sem_release((rt_sem_t)obj);
    else
        err = rt_fmq_send((rt_mailbox_t)obj,(void *)pvItemToQueue,xCopyPosition,xTicksToWait);
#ifdef SHOW_QUE_DEBUG_INFO
    ets_printf("QueueSendOver cur:%s name:%s ret:%d\n",rt_current_thread->name,obj->name,err);
#endif
    return (err==RT_EOK)?pdPASS:errQUEUE_FULL;
}
signed portBASE_TYPE ICACHE_FLASH_ATTR xQueueGenericSendFromISR( xQueueHandle xQueue, const void * const pvItemToQueue, signed portBASE_TYPE *pxHigherPriorityTaskWoken, portBASE_TYPE xCopyPosition )
{
    rt_object_t obj = xQueue;
    if (obj == 0)
        return errQUEUE_FULL;
    rt_interrupt_enter();
#ifdef SHOW_QUE_DEBUG_INFO
    ets_printf("QueueSendISR cur:%s name:%s pos:%d\n",rt_current_thread->name,obj->name,xCopyPosition);
#endif
    rt_err_t err = RT_EOK;
    if (obj->type == RT_Object_Class_Semaphore)
        err = rt_sem_release((rt_sem_t)obj);
    else
        err = rt_fmq_send((rt_mailbox_t)obj,(void *)pvItemToQueue,xCopyPosition,0);
#ifdef SHOW_QUE_DEBUG_INFO
    ets_printf("QueueSendISROver cur:%s name:%s ret:%d\n",rt_current_thread->name,obj->name,err);
#endif
    if (pxHigherPriorityTaskWoken) *pxHigherPriorityTaskWoken = pdFAIL;
    rt_interrupt_leave();
    return (err==RT_EOK)?pdPASS:errQUEUE_FULL;
}
signed portBASE_TYPE ICACHE_FLASH_ATTR xQueueGenericReceive( xQueueHandle xQueue, const void * const pvBuffer, portTickType xTicksToWait, portBASE_TYPE xJustPeeking )
{
    rt_object_t obj = xQueue;
    if (obj == 0)
        return errQUEUE_EMPTY;
#ifdef SHOW_QUE_DEBUG_INFO
    ets_printf("QueueRecv cur:%s name:%s wait:%d peek:%d\n",rt_current_thread->name,obj->name,xTicksToWait,xJustPeeking);
#endif
    rt_err_t err = RT_EOK;
    if (obj->type == RT_Object_Class_Semaphore)
        err = rt_sem_take((rt_sem_t)obj,xTicksToWait);
    else
        err = rt_fmq_recv((rt_mailbox_t)obj,(void *)pvBuffer,xJustPeeking,xTicksToWait);
#ifdef SHOW_QUE_DEBUG_INFO
    ets_printf("QueueRecvOver cur:%s name:%s ret:%d\n",rt_current_thread->name,obj->name,err);
#endif
    return (err==RT_EOK)?pdPASS:errQUEUE_EMPTY;
}
unsigned portBASE_TYPE ICACHE_FLASH_ATTR uxQueueMessagesWaiting(const xQueueHandle xQueue)
{ 
    rt_object_t obj = xQueue;
    if (obj == 0)
        return;
    unsigned portBASE_TYPE count = 0;
    vPortEnterCritical();
    if (obj->type == RT_Object_Class_Semaphore)
        count = ((rt_sem_t)obj)->value;
    else
        count = ((rt_mailbox_t)obj)->entry;
    vPortExitCritical();
    return count;
}
unsigned portBASE_TYPE ICACHE_FLASH_ATTR uxQueueMessagesWaitingFromISR(const xQueueHandle xQueue)
{ 
    rt_object_t obj = xQueue;
    if (obj == 0)
        return;
    unsigned portBASE_TYPE count = 0;
    if (obj->type == RT_Object_Class_Semaphore)
        count = ((rt_sem_t)obj)->value;
    else
        count = ((rt_mailbox_t)obj)->entry;
    return count;
}

xTimerHandle ICACHE_FLASH_ATTR xTimerCreate( const signed char * const pcTimerName, portTickType xTimerPeriodInTicks, unsigned portBASE_TYPE uxAutoReload, void *pvTimerID, tmrTIMER_CALLBACK pxCallbackFunction )
{
    ets_printf("xTimerCreate Failed!\n");
    return 0;
}
portBASE_TYPE ICACHE_FLASH_ATTR xTimerGenericCommand( xTimerHandle xTimer, portBASE_TYPE xCommandID, portTickType xOptionalValue, signed portBASE_TYPE *pxHigherPriorityTaskWoken, portTickType xBlockTime )
{
    ets_printf("xTimerGenericCommand Failed!\n");
    return pdFAIL;
}

void ICACHE_FLASH_ATTR vTaskStepTick(portTickType xTicksToJump)
{
    ets_printf("vTaskStepTick Failed!\n");
}
portTickType ICACHE_FLASH_ATTR prvGetExpectedIdleTime(void)
{
    ets_printf("prvGetExpectedIdleTime Failed!\n");
    return 0;
}
