#ifndef __SCCB_H
#define __SCCB_H
/* includes -----------------------------------------------------------------*/
#include "main.h"

/* macro --------------------------------------------------------------------*/
#define SCCB_WRITE  0x00
#define SCCB_READ   0x01

#define SCCB_SCL_GPIO_PORT               GPIOD
#define SCCB_SCL_GPIO_PIN                GPIO_PIN_3
#define SCCB_SCL_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)

#define SCCB_SDA_GPIO_PORT               GPIOG
#define SCCB_SDA_GPIO_PIN                GPIO_PIN_13
#define SCCB_SDA_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)

#define SCCB_SCL(x)                     do{ x ? \
                                              HAL_GPIO_WritePin(SCCB_SCL_GPIO_PORT, SCCB_SCL_GPIO_PIN, GPIO_PIN_SET) : \
                                              HAL_GPIO_WritePin(SCCB_SCL_GPIO_PORT, SCCB_SCL_GPIO_PIN, GPIO_PIN_RESET); \
                                          }while(0)

#define SCCB_SDA(x)                     do{ x ? \
                                              HAL_GPIO_WritePin(SCCB_SDA_GPIO_PORT, SCCB_SDA_GPIO_PIN, GPIO_PIN_SET) : \
                                              HAL_GPIO_WritePin(SCCB_SDA_GPIO_PORT, SCCB_SDA_GPIO_PIN, GPIO_PIN_RESET); \
                                        }while(0)

#define SCCB_READ_SDA()                 HAL_GPIO_ReadPin(SCCB_SDA_GPIO_PORT, SCCB_SDA_GPIO_PIN) 

/* enum ---------------------------------------------------------------------*/

    
/* types --------------------------------------------------------------------*/

/* functions prototypes -----------------------------------------------------*/
void SCCB_Init(void);
void SCCB_Delay(void);
void SCCB_Start(void);
void SCCB_Stop(void);
void SCCB_NAck(void);
void SCCB_NoCare(void);
void SCCB_SendByte(uint8_t Byte);
void SCCB_ReceiveByte(uint8_t *Byte);


void SCCB_3PhaseWrite(uint8_t ID_Addr, uint8_t Sub_Addr, uint8_t Data);
void SCCB_2PhaseWrite(uint8_t ID_Addr, uint8_t Sub_Addr);
void SCCB_2PhaseRead(uint8_t ID_Addr, uint8_t *Data);


#endif /* __SCCB_H */
