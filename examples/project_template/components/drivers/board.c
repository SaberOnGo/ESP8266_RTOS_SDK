#include <rtthread.h>
#include <rthw.h>
#include <rtm.h>
#include <rtdevice.h>
#include "user_config.h"
#include "uart.h"

struct esp8266_uart
{
    int num;
    int irq;
};

static rt_err_t esp8266_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    struct esp8266_uart* uart = (struct esp8266_uart *)serial->parent.user_data;

    UART_ConfigTypeDef uart_config;
    uart_config.baud_rate   = cfg->baud_rate;
    uart_config.data_bits   = UART_WordLength_8b;
    uart_config.parity      = USART_Parity_None;
    uart_config.stop_bits   = USART_StopBits_1;
    uart_config.flow_ctrl   = USART_HardwareFlowControl_None;
    uart_config.UART_RxFlowThresh = 120;
    uart_config.UART_InverseMask  = UART_None_Inverse;
    UART_ParamConfig(UART0, &uart_config);
   
    if (uart->num > 0)
    {
        UART_IntrConfTypeDef uart_intr;
        uart_intr.UART_IntrEnMask = UART_RXFIFO_TOUT_INT_ENA | UART_FRM_ERR_INT_ENA | UART_RXFIFO_FULL_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA;
        uart_intr.UART_RX_FifoFullIntrThresh = 10;
        uart_intr.UART_RX_TimeOutIntrThresh = 2;
        uart_intr.UART_TX_FifoEmptyIntrThresh = 20;
        UART_IntrConfig(uart->num, &uart_intr);
        ETS_UART_INTR_ENABLE();
    }
    
    return RT_EOK;
}

static rt_err_t esp8266_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct esp8266_uart* uart = (struct esp8266_uart *)serial->parent.user_data;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        /* disable rx irq */
        if (uart->irq > 0)
            ETS_UART_INTR_DISABLE();
        break;
    case RT_DEVICE_CTRL_SET_INT:
        /* enable rx irq */
        if (uart->irq > 0)
            ETS_UART_INTR_ENABLE();
        break;
    }

    return RT_EOK;
}

static int esp8266_putc(struct rt_serial_device *serial, char c)
{
    struct esp8266_uart* uart = (struct esp8266_uart *)serial->parent.user_data;

    while (true) {
        uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(uart->num)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);
        if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126)
            break;
    }
    WRITE_PERI_REG(UART_FIFO(uart->num), c);
    return 1;
}

static int esp8266_getc(struct rt_serial_device *serial)
{
    struct esp8266_uart* uart = (struct esp8266_uart *)serial->parent.user_data;

    int ch = -1;
    uint8 fifo_cnt = (READ_PERI_REG(UART_STATUS(uart->num)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
    if (fifo_cnt > 0)
        ch = READ_PERI_REG(UART_FIFO(uart->num)) & 0xFF;
    return ch;
}

static void uart0_rx_intr_handler(void *para)
{
    /* uart0 and uart1 intr combine togther, when interrupt occur, see reg 0x3ff20020, bit2, bit0 represents
    * uart1 and uart0 respectively
    */
    struct rt_serial_device *serial = (struct rt_serial_device *)para;
    struct esp8266_uart* uart = (struct esp8266_uart *)serial->parent.user_data;
    uint32 uart_intr_status = READ_PERI_REG(UART_INT_ST(uart->num)) ;

    while (uart_intr_status != 0x0) {
        if (UART_FRM_ERR_INT_ST == (uart_intr_status & UART_FRM_ERR_INT_ST)) {
            WRITE_PERI_REG(UART_INT_CLR(uart->num), UART_FRM_ERR_INT_CLR);
        } else if (UART_RXFIFO_FULL_INT_ST == (uart_intr_status & UART_RXFIFO_FULL_INT_ST)) {
            rt_interrupt_enter();
            rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
            rt_interrupt_leave();
            WRITE_PERI_REG(UART_INT_CLR(uart->num), UART_RXFIFO_FULL_INT_CLR);
        } else if (UART_RXFIFO_TOUT_INT_ST == (uart_intr_status & UART_RXFIFO_TOUT_INT_ST)) {
            rt_interrupt_enter();
            rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
            rt_interrupt_leave();
            WRITE_PERI_REG(UART_INT_CLR(uart->num), UART_RXFIFO_TOUT_INT_CLR);
        } else if (UART_TXFIFO_EMPTY_INT_ST == (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST)) {
            WRITE_PERI_REG(UART_INT_CLR(uart->num), UART_TXFIFO_EMPTY_INT_CLR);
            CLEAR_PERI_REG_MASK(UART_INT_ENA(uart->num), UART_TXFIFO_EMPTY_INT_ENA);
        } else {
            //skip
        }
        uart_intr_status = READ_PERI_REG(UART_INT_ST(uart->num)) ;
    }
}

static const struct rt_uart_ops esp8266_uart_ops =
{
    esp8266_configure,
    esp8266_control,
    esp8266_putc,
    esp8266_getc,
};
static struct rt_serial_device serial0;
static struct rt_serial_device serial1;
struct esp8266_uart uart0 = { 0, ETS_UART_INUM };
struct esp8266_uart uart1 = { 0, -1};

int rt_hw_usart_init() 
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    UART_WaitTxFifoEmpty(UART0);
    UART_WaitTxFifoEmpty(UART1);
    UART_intr_handler_register(uart0_rx_intr_handler, &serial0);
    ETS_UART_INTR_DISABLE();

    serial0.ops = &esp8266_uart_ops;
    serial0.config = config;
    rt_hw_serial_register(&serial0, "uart0", 
        RT_DEVICE_FLAG_RDWR|RT_DEVICE_FLAG_INT_RX, &uart0);

    serial1.ops = &esp8266_uart_ops;
    serial1.config = config;
    rt_hw_serial_register(&serial1, "uart1", 
        RT_DEVICE_FLAG_RDWR, &uart1);
}

void rt_hw_board_init(void)
{
    /* initialize uart */
    rt_hw_usart_init();
#ifdef RT_USING_CONSOLE
    rt_console_set_device(CONSOLE_DEVICE);
#endif
}
