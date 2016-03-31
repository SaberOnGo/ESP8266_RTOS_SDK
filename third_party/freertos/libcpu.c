#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtthread.h>

volatile portSTACK_TYPE *pxCurrentTCB = 0;
void rt_hw_context_switch(rt_uint32_t from, rt_uint32_t to)
{
    PendSV(1);
} 
void rt_hw_context_switch_interrupt(rt_uint32_t from, rt_uint32_t to)
{
    pxCurrentTCB = (portSTACK_TYPE *)to;
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

static signed rtt_init = 0;
void rtthread_startup(void)
{
    /* show version */
    rt_show_version();

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

void ICACHE_FLASH_ATTR ff(void *pp)
{
    int cc = 0;
    while (1)
    {
        ets_printf("%d run1 %d\n",pp,rt_tick_get());
        rt_thread_delay(100);
        cc++;
        cc++;
        cc++;
        cc++;
        cc++;
        ets_printf("%d run2 %d\n",pp,cc);
    }
}

signed portBASE_TYPE ICACHE_FLASH_ATTR xTaskGenericCreate( pdTASK_CODE pxTaskCode, const signed char * const pcName, unsigned short usStackDepth, void *pvParameters, unsigned portBASE_TYPE uxPriority, xTaskHandle *pxCreatedTask, portSTACK_TYPE *puxStackBuffer, const xMemoryRegion * const xRegions )
{
    signed portBASE_TYPE xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    rt_thread_t thread = *pxCreatedTask = rt_thread_create(pcName,ff,(void *)(20-uxPriority),10000,20-uxPriority,10);
    if (*pxCreatedTask != 0)
    {
        rt_thread_startup(*pxCreatedTask);
        xReturn = pdPASS;
    }
    return xReturn;
}

void ICACHE_FLASH_ATTR vTaskStartScheduler(void) { xPortStartScheduler(); }
void ICACHE_FLASH_ATTR vTaskDelete(xTaskHandle xTaskToDelete) { rt_thread_delete(xTaskToDelete); }
void ICACHE_FLASH_ATTR vTaskDelay(portTickType xTicksToDelay) { rt_thread_delay(xTicksToDelay); }
void ICACHE_FLASH_ATTR vTaskSuspendAll(void) { rt_enter_critical(); }
signed portBASE_TYPE ICACHE_FLASH_ATTR xTaskResumeAll( void ) { rt_exit_critical();return pdTRUE; }
void ICACHE_FLASH_ATTR vTaskSwitchContext(void) { rt_interrupt_enter();rt_schedule();rt_interrupt_leave(); }
portTickType ICACHE_FLASH_ATTR xTaskGetTickCount(void) { return rt_tick_get(); }
void ICACHE_FLASH_ATTR xPortSysTickHandle(void) { rt_interrupt_enter();rt_tick_increase();rt_interrupt_leave(); }

unsigned portBASE_TYPE ICACHE_FLASH_ATTR uxTaskGetStackHighWaterMark( xTaskHandle xTask )
{
    ets_printf("uxTaskGetStackHighWaterMark\n");
    return 0;
}
void ICACHE_FLASH_ATTR vTaskStepTick(portTickType xTicksToJump)
{
    ets_printf("vTaskStepTick\n");
}
portTickType ICACHE_FLASH_ATTR prvGetExpectedIdleTime(void)
{
    ets_printf("prvGetExpectedIdleTime\n");
    return 0;
}

xTimerHandle ICACHE_FLASH_ATTR xTimerCreate( const signed char * const pcTimerName, portTickType xTimerPeriodInTicks, unsigned portBASE_TYPE uxAutoReload, void *pvTimerID, tmrTIMER_CALLBACK pxCallbackFunction )
{
    ets_printf("xTimerCreate\n");
    return 0;
}
portBASE_TYPE ICACHE_FLASH_ATTR xTimerGenericCommand( xTimerHandle xTimer, portBASE_TYPE xCommandID, portTickType xOptionalValue, signed portBASE_TYPE *pxHigherPriorityTaskWoken, portTickType xBlockTime )
{
    ets_printf("xTimerGenericCommand\n");
    return 0;
}

xQueueHandle ICACHE_FLASH_ATTR xQueueGenericCreate( unsigned portBASE_TYPE uxQueueLength, unsigned portBASE_TYPE uxItemSize, unsigned char ucQueueType )
{
    ets_printf("xQueueGenericCreate\n");
    return malloc(100);
}
void ICACHE_FLASH_ATTR vQueueDelete( xQueueHandle xQueue )
{
    ets_printf("vQueueDelete\n");
}
signed portBASE_TYPE ICACHE_FLASH_ATTR xQueueGenericSend( xQueueHandle xQueue, const void * const pvItemToQueue, portTickType xTicksToWait, portBASE_TYPE xCopyPosition )
{
    ets_printf("xQueueGenericSend\n");
    return pdPASS;
}
signed portBASE_TYPE ICACHE_FLASH_ATTR xQueueGenericReceive( xQueueHandle xQueue, const void * const pvBuffer, portTickType xTicksToWait, portBASE_TYPE xJustPeeking )
{
    ets_printf("xQueueGenericReceive\n");
    return pdPASS;
}
signed portBASE_TYPE ICACHE_FLASH_ATTR xQueueGenericSendFromISR( xQueueHandle xQueue, const void * const pvItemToQueue, signed portBASE_TYPE *pxHigherPriorityTaskWoken, portBASE_TYPE xCopyPosition )
{
    return pdPASS;
}
signed portBASE_TYPE ICACHE_FLASH_ATTR xQueueReceiveFromISR( xQueueHandle xQueue, const void * const pvBuffer, signed portBASE_TYPE *pxHigherPriorityTaskWoken )
{
    ets_printf("xQueueReceiveFromISR\n");
    return pdPASS;
}
unsigned portBASE_TYPE ICACHE_FLASH_ATTR uxQueueMessagesWaitingFromISR( const xQueueHandle xQueue )
{
    ets_printf("uxQueueMessagesWaitingFromISR\n");
    return pdPASS;
}
unsigned portBASE_TYPE ICACHE_FLASH_ATTR uxQueueMessagesWaiting( const xQueueHandle xQueue )
{
    ets_printf("uxQueueMessagesWaiting\n");
    return pdPASS;
}
xQueueHandle ICACHE_FLASH_ATTR xQueueCreateMutex( unsigned char ucQueueType )
{
    ets_printf("xQueueCreateMutex Failed!\n");
    return 0;
}
