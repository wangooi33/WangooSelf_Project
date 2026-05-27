#include "iic.h"

void IIC_Init(void)
{
    GPIO_InitTypeDef GPIOInit_St;

    IIC_SCL_GPIO_CLK_ENABLE();
    IIC_SDA_GPIO_CLK_ENABLE();

    GPIOInit_St.Pin = IIC_SCL_GPIO_PIN;
    GPIOInit_St.Mode = GPIO_MODE_OUTPUT_OD;
    GPIOInit_St.Pull = GPIO_PULLUP;
    GPIOInit_St.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(IIC_SCL_GPIO_PORT, &GPIOInit_St);

    GPIOInit_St.Pin = IIC_SDA_GPIO_PIN;
    HAL_GPIO_Init(IIC_SDA_GPIO_PORT, &GPIOInit_St);
}

static void IIC_Delay(void)
{
	volatile uint16_t i;

    for (i = 0; i < 30; i++);
}

void IIC_Start(void)
{
    IIC_SDA(1);
    IIC_SCL(1);
    IIC_Delay();
    IIC_SDA(0);
    IIC_Delay();
    IIC_SCL(0);
    IIC_Delay();
}

void IIC_Stop(void)
{
    IIC_SDA(0);
    IIC_Delay();
    IIC_SCL(1);
    IIC_Delay();
    IIC_SDA(1);
    IIC_Delay();
}

uint8_t IIC_WaitAck(void)
{
    uint8_t waittime = 0;
    uint8_t rack = 0;

    IIC_SDA(1);
    IIC_Delay();
    IIC_SCL(1);
    IIC_Delay();

    while (IIC_READ_SDA())
    {
        if (waittime++ > 250)
        {
            IIC_Stop();
            rack = 1;
            break;
        }
    }

    IIC_SCL(0);
    IIC_Delay();
    return rack;
}


void IIC_Ack(void)
{
    IIC_SDA(0);
    IIC_Delay();
    IIC_SCL(1);
    IIC_Delay();
    IIC_SCL(0);
    IIC_Delay();
    IIC_SDA(1);
    IIC_Delay();
}

void IIC_NAck(void)
{
    IIC_SDA(1);
    IIC_Delay();
    IIC_SCL(1);
    IIC_Delay();
    IIC_SCL(0);
    IIC_Delay();
}


void IIC_SendByte(uint8_t data)
{
    uint8_t t;
    
    for (t = 0; t < 8; t++)
    {
        IIC_SDA((data & 0x80) >> 7);
        IIC_Delay();
        IIC_SCL(1);
        IIC_Delay();
        IIC_SCL(0);
        data <<= 1;
    }
    IIC_SDA(1);
}

uint8_t IIC_ReadByte(uint8_t ack)
{
    uint8_t i, receive = 0;

    for (i = 0; i < 8; i++ )
    {
        receive <<= 1;
        IIC_SCL(1);
        IIC_Delay();

        if (IIC_READ_SDA())
        {
            receive++;
        }
        
        IIC_SCL(0);
        IIC_Delay();
    }

    if (!ack)
    {
        IIC_NAck();
    }
    else
    {
        IIC_Ack();
    }

    return receive;
}


