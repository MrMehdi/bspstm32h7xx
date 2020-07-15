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
** ��   ��   ��: stm32_drv_wdg.c
**
** ��   ��   ��: Jiao.jinxing
**
** �ļ���������: 2020 �� 04 �� 07 ��
**
** ��        ��: STM32 оƬ���Ź�����
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include "includes.h"

#if BSP_CFG_WDG_EN > 0

#if BSP_CFG_WDG_DEADLINE <= 100
#error BSP_CFG_WDG_DEADLINE must > 100
#endif

static void __stm32_wdg_start(void)
{
    IWDG->KR  = IWDG_KEY_WRITE_ACCESS_ENABLE;                       /* ʹ�ܶ� IWDG->PR �� IWDG->RLR ��д */
    IWDG->PR  = IWDG_PRESCALER_64;                                  /* ���÷�Ƶϵ�� */
    IWDG->RLR = 500 * BSP_CFG_WDG_DEADLINE / MS_CFG_KERN_TICK_HZ;   /* ���ؼĴ���, ��� 0xFFF */
    IWDG->KR  = IWDG_KEY_RELOAD;                                    /* ���� */
    IWDG->KR  = IWDG_KEY_ENABLE;                                    /* ʹ�ܿ��Ź� */
}

static void __stm32_wdg_feed(void)
{
    IWDG->KR = IWDG_KEY_RELOAD;                                     /* ���� */
}

static ms_wdg_drv_t __stm32_wdg_drv = {
    .start     = __stm32_wdg_start,
    .feed      = __stm32_wdg_feed,
    .dead_line = BSP_CFG_WDG_DEADLINE / 2,
};

ms_wdg_drv_t *stm32_wdg_drv(void)
{
    return &__stm32_wdg_drv;
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
