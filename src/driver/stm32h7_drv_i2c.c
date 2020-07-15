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
** ��   ��   ��: stm32_drv_i2c.c
**
** ��   ��   ��: Jiao.jinxing
**
** �ļ���������: 2020 �� 04 �� 07 ��
**
** ��        ��: STM32 оƬ I2C ��������
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include <string.h>

#include "includes.h"

#if BSP_CFG_I2C_EN > 0

#define __STM32_I2C_MAX_CHANNEL         (3U)

/*
 * Private info
 */
typedef struct {
    I2C_HandleTypeDef  i2c_handler;
    IRQn_Type          event_irq;
    IRQn_Type          error_irq;
    ms_i2c_param_t     param;
    ms_handle_t        sembid;
} privinfo_t;

/*
 * I2C interrupt request setting
 */
static void __stm32_i2c_bus_int_set(privinfo_t *priv, ms_bool_t enable)
{
    if (enable) {
        HAL_NVIC_EnableIRQ(priv->event_irq);
        HAL_NVIC_EnableIRQ(priv->error_irq);
    } else {
        HAL_NVIC_DisableIRQ(priv->event_irq);
        HAL_NVIC_DisableIRQ(priv->error_irq);
    }
}

/*
 * Convert ms_uart_param_t to native structure uart_init_t
 */
static int __stm32_i2c_bus_param_convert(ms_i2c_param_t *param, I2C_InitTypeDef *config)
{
    config->OwnAddress1  = param->own_address1;
    config->OwnAddress2  = param->own_address2;

    config->Timing = BSP_I2C_GetTiming(param->clk_speed);

    if (param->addressing_mode == MS_I2C_ADDRESSING_MODE_7B) {
        config->AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    } else if (param->addressing_mode == MS_I2C_ADDRESSING_MODE_10B) {
        config->AddressingMode = I2C_ADDRESSINGMODE_10BIT;
    } else {
        return MS_ERR;
    }

    if (param->dual_address_mode == MS_I2C_DUAL_ADDRESS_DISABLE) {
        config->DualAddressMode = I2C_DUALADDRESS_DISABLED;
    } else if (param->dual_address_mode == MS_I2C_DUAL_ADDRESS_ENABLE) {
        config->DualAddressMode = I2C_DUALADDRESS_ENABLED;
    } else {
        return MS_ERR;
    }

    if (param->general_call_mode == MS_I2C_GENERAL_CALL_DISABLE) {
        config->GeneralCallMode  = I2C_GENERALCALL_DISABLED;
    } else if (param->general_call_mode == MS_I2C_GENERAL_CALL_ENABLE) {
        config->GeneralCallMode  = I2C_GENERALCALL_ENABLED;
    } else {
        return MS_ERR;
    }

    if (param->no_stretch_mode == MS_I2C_NO_STRETCH_DISABLE) {
        config->NoStretchMode    = I2C_NOSTRETCH_DISABLED;
    } else if (param->no_stretch_mode == MS_I2C_NO_STRETCH_ENABLE) {
        config->NoStretchMode    = I2C_NOSTRETCH_ENABLED;
    } else {
        return MS_ERR;
    }

    return MS_ERR_NONE;
}

static ms_ssize_t __stm32_i2c_bus_trans(ms_ptr_t bus_ctx, const ms_i2c_msg_t *msg, ms_size_t n_msg)
{
    privinfo_t *priv = bus_ctx;

    if (n_msg == 2) {
        ms_uint16_t addr_size;
        ms_uint16_t memaddr;

        if (msg[0].len > 1) {
            ms_uint8_t *pmemaddr = msg[0].buf;
            addr_size = I2C_MEMADD_SIZE_16BIT;
            memaddr = (pmemaddr[0] << 8) | (pmemaddr[1]);
        } else {
            ms_uint8_t *pmemaddr = msg[0].buf;
            addr_size = I2C_MEMADD_SIZE_8BIT;
            memaddr = *pmemaddr;
        }

        if (msg[1].flags & MS_I2C_M_READ) {
            if (HAL_I2C_Mem_Read(&priv->i2c_handler, msg[0].addr << 1U, memaddr, addr_size, msg[1].buf, msg[1].len, 1000) == HAL_OK) {
                return 2;
            }
        } else {
            if (HAL_I2C_Mem_Write(&priv->i2c_handler, msg[0].addr << 1U, memaddr, addr_size, msg[1].buf, msg[1].len, 1000) == HAL_OK) {
                return 2;
            }
        }
    } else {
        if (msg[0].flags & MS_I2C_M_READ) {
            if (HAL_I2C_Master_Receive(&priv->i2c_handler, msg[0].addr << 1U, msg[0].buf, msg[0].len, 1000) == HAL_OK) {
                return 1;
            }
        } else {
            if (HAL_I2C_Master_Transmit(&priv->i2c_handler, msg[0].addr << 1U, msg[0].buf, msg[0].len, 1000) == HAL_OK) {
                return 1;
            }
        }
    }

    return 0;
}

static int __stm32_i2c_bus_ioctl(ms_ptr_t bus_ctx, int cmd, ms_ptr_t arg)
{
    privinfo_t *priv = bus_ctx;
    int ret;

    switch (cmd) {
    case MS_I2C_CMD_GET_PARAM:
        if (ms_access_ok(arg, sizeof(ms_i2c_param_t), MS_ACCESS_W)) {
            *(ms_i2c_param_t *)arg = priv->param;
            ret = 0;
        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_I2C_CMD_SET_PARAM:
        if (ms_access_ok(arg, sizeof(ms_i2c_param_t), MS_ACCESS_R)) {
            priv->param = *(ms_i2c_param_t *)arg;

            if (__stm32_i2c_bus_param_convert(&priv->param, &(priv->i2c_handler.Init)) == MS_ERR_NONE) {
                __stm32_i2c_bus_int_set(priv, MS_FALSE);
                HAL_I2C_DeInit(&priv->i2c_handler);
                HAL_I2C_Init(&priv->i2c_handler);
                __stm32_i2c_bus_int_set(priv, MS_TRUE);
                ret = 0;

            } else {
                ms_thread_set_errno(EINVAL);
                ret = -1;
            }
        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    default:
        ms_thread_set_errno(EINVAL);
        ret = -1;
        break;
    }

    return ret;
}

static ms_i2c_bus_ops_t __stm32_i2c_bus_ops = {
        .trans = __stm32_i2c_bus_trans,
        .ioctl = __stm32_i2c_bus_ioctl,
};

static privinfo_t __stm32_i2c_bus_privinfo[__STM32_I2C_MAX_CHANNEL] = {
#ifdef I2C1_BASE
    {
            .i2c_handler = {
                    .Instance = (I2C_TypeDef *)I2C1_BASE,
            },
            .event_irq = I2C1_EV_IRQn,
            .error_irq = I2C1_ER_IRQn,
    },
#ifdef I2C2_BASE
    {
            .i2c_handler = {
                    .Instance = (I2C_TypeDef *)I2C2_BASE,
            },
            .event_irq = I2C2_EV_IRQn,
            .error_irq = I2C2_ER_IRQn,
    },
#ifdef I2C3_BASE
    {
            .i2c_handler = {
                    .Instance = (I2C_TypeDef *)I2C3_BASE,
            },
            .event_irq = I2C3_EV_IRQn,
            .error_irq = I2C3_ER_IRQn,
    },
#endif
#endif
#endif
};

static ms_i2c_bus_t __stm32_i2c_bus[__STM32_I2C_MAX_CHANNEL] = {
#ifdef I2C1_BASE
    {
        /* I2C1 device data structure defined here */
        .nnode = {
            .name = "i2c1",
        },
        .ops = &__stm32_i2c_bus_ops,
        .ctx = &__stm32_i2c_bus_privinfo[0],
    },
#ifdef I2C2_BASE
    {
        /* I2C2 device data structure defined here */
        .nnode = {
            .name = "i2c2",
        },
        .ops = &__stm32_i2c_bus_ops,
        .ctx = &__stm32_i2c_bus_privinfo[1],
    },
#ifdef I2C3_BASE
    {
        /* I2C3 device data structure defined here */
        .nnode = {
            .name = "i2c3",
        },
        .ops = &__stm32_i2c_bus_ops,
        .ctx = &__stm32_i2c_bus_privinfo[2],
    },
#endif
#endif
#endif
};

/**
  * @brief  Master Tx Transfer completed callback.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    privinfo_t *priv = MS_CONTAINER_OF(hi2c, privinfo_t, i2c_handler);

    (void)ms_semb_post(priv->sembid);
}

/**
  * @brief  Master Rx Transfer completed callback.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    privinfo_t *priv = MS_CONTAINER_OF(hi2c, privinfo_t, i2c_handler);

    (void)ms_semb_post(priv->sembid);
}

void stm32_i2c_ev_irqhandler(ms_uint8_t channel)
{
    HAL_I2C_EV_IRQHandler(&__stm32_i2c_bus_privinfo[channel - 1U].i2c_handler);
}

void stm32_i2c_er_irqhandler(ms_uint8_t channel)
{
    HAL_I2C_ER_IRQHandler(&__stm32_i2c_bus_privinfo[channel - 1U].i2c_handler);
}

/*
 * Create I2C device file
 */
ms_err_t stm32_i2c_bus_dev_create(const char *path, ms_uint8_t channel)
{
    ms_i2c_bus_t *i2c_bus;
    privinfo_t *priv;
    ms_err_t err;

    if ((path == MS_NULL) || (channel > __STM32_I2C_MAX_CHANNEL) || (channel == 0U)) {
        return MS_ERR;
    }

    i2c_bus = &__stm32_i2c_bus[channel - 1U];

    priv = (privinfo_t *)i2c_bus->ctx;

    err = ms_semb_create("i2c_semb", MS_FALSE, MS_WAIT_TYPE_PRIO, &priv->sembid);
    if (err == MS_ERR_NONE) {
        /*
         * Initial device
         */
        priv->param.clk_speed         = MS_I2C_CLK_SPEED_STANDARD;
        priv->param.duty_cycle        = MS_I2C_DUTY_CYCLE_2;
        priv->param.own_address1      = 0U;
        priv->param.addressing_mode   = MS_I2C_ADDRESSING_MODE_7B;
        priv->param.dual_address_mode = MS_I2C_DUAL_ADDRESS_DISABLE;
        priv->param.own_address2      = 0U;
        priv->param.general_call_mode = MS_I2C_GENERAL_CALL_DISABLE;
        priv->param.no_stretch_mode   = MS_I2C_NO_STRETCH_DISABLE;

        /*
         * Convert ms_i2c_param_t to native structure I2C_InitTypeDef
         */
        __stm32_i2c_bus_param_convert(&priv->param, &(priv->i2c_handler.Init));

        /*
         * Hardware initialization
         */
        if (HAL_I2C_Init(&priv->i2c_handler) == HAL_OK) {
            __stm32_i2c_bus_int_set(priv, MS_TRUE);

            err = ms_i2c_bus_register(i2c_bus);
            if (err == MS_ERR_NONE) {
                err = ms_i2c_bus_dev_create(path, i2c_bus);
            }
        } else {
            err = MS_ERR;
        }
    }

    return err;
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
