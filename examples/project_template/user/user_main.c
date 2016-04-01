/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "user_config.h"
#include "esp_common.h"

extern long list_thread(void);
ALIGN(RT_ALIGN_SIZE)
static char thread_main_stack[2048];
struct rt_thread thread_main;
static void rt_thread_entry_main(void* parameter)
{
    /* init finsh */
#ifdef RT_USING_FINSH
    finsh_system_init();
    finsh_set_device(FINSH_DEVICE_NAME);
#endif

    while (1)
    {
        rt_thread_delay(1000);
    }
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    rt_hw_board_init();
    rt_show_version();
    printf("\nSDK version:%s\n\n", system_get_sdk_version());
    //
    rt_thread_init(&thread_main,
                   "main",
                   rt_thread_entry_main,
                   RT_NULL,
                   &thread_main_stack[0],
                   sizeof(thread_main_stack),13,10);
    rt_thread_startup(&thread_main);
}

