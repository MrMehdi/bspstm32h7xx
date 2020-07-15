/*********************************************************************************************************
**
**                                ���������Ϣ�������޹�˾
**
**                                  ΢�Ͱ�ȫʵʱ����ϵͳ
**
**                                      MS-RTOS(TM)
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: stm32_drv_uart.h
**
** ��   ��   ��: Jiao.jinxing
**
** �ļ���������: 2020 �� 04 �� 07 ��
**
** ��        ��: STM32 оƬ UART ����ͷ�ļ�
*********************************************************************************************************/

#ifndef STM32_DRV_UART_H
#define STM32_DRV_UART_H

#ifdef __cplusplus
extern "C" {
#endif

ms_err_t stm32_uart_drv_register(void);
ms_err_t stm32_uart_dev_create(const char *path, ms_uint8_t channel,
                               ms_uint32_t rx_buf_size, ms_uint32_t tx_buf_size);

ms_err_t stm32_uart_get_hw_info(ms_uint8_t channel, IRQn_Type *irq, UART_HandleTypeDef **handler);
void     stm32_uart_irq_handler(ms_uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  STM32_DRV_UART_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
