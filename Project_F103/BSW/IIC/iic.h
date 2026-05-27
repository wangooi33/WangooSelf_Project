#ifndef __IIC_H
#define __IIC_H

#ifdef __cplusplus
extern "C" {
#endif

/* includes -----------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include <stdint.h>

/* macro --------------------------------------------------------------------*/

/* SCL */
#define IIC_SCL_GPIO_PORT         GPIOB
#define IIC_SCL_GPIO_PIN          GPIO_PIN_6
#define IIC_SCL_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

/* SDA */
#define IIC_SDA_GPIO_PORT         GPIOB
#define IIC_SDA_GPIO_PIN          GPIO_PIN_7
#define IIC_SDA_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

#define IIC_SCL(x)        do{ x ? \
                              HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)       /* SCL */

#define IIC_SDA(x)        do{ x ? \
                              HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)       /* SDA */

#define IIC_READ_SDA()     HAL_GPIO_ReadPin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN)


/* functions prototypes -----------------------------------------------------*/
void IIC_Init(void);
void IIC_Start(void);
void IIC_Stop(void);
void IIC_Ack(void);
void IIC_NAck(void);
uint8_t IIC_WaitAck(void); 
void IIC_SendByte(uint8_t txd);
uint8_t IIC_ReadByte(unsigned char ack);


#ifdef __cplusplus
}
#endif

#endif /* __IIC_H */

