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
** ��   ��   ��: stm32_drv_gpio.h
**
** ��   ��   ��: Jiao.jinxing
**
** �ļ���������: 2020 �� 04 �� 07 ��
**
** ��        ��: STM32 оƬ GPIO ����ͷ�ļ�
*********************************************************************************************************/

#ifndef STM32_DRV_GPIO_H
#define STM32_DRV_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

ms_err_t stm32_gpio_drv_register(void);
ms_err_t stm32_gpio_dev_create(const char *path, ms_addr_t base, ms_uint16_t pin);

void stm32_gpio_isr_install(ms_uint16_t pin, void (*isr)(ms_ptr_t), ms_ptr_t arg);
void stm32_gpio_int_enable(ms_uint16_t pin, ms_bool_t enable);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  STM32_DRV_TOUCH_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
