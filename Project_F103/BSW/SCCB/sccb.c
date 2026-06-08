#include "sccb.h"
#include "delay.h"

void SCCB_Init(void)
{
    GPIO_InitTypeDef GPIOInit_St;
    GPIOInit_St.Mode = GPIO_MODE_OUTPUT_OD;
    GPIOInit_St.Pull  =GPIO_PULLUP;
    GPIOInit_St.Speed = GPIO_SPEED_FREQ_HIGH;

    GPIOInit_St.Pin = SCCB_SCL_GPIO_PIN;
    HAL_GPIO_Init(SCCB_SCL_GPIO_PORT, &GPIOInit_St);
    GPIOInit_St.Pin = SCCB_SDA_GPIO_PIN;
    HAL_GPIO_Init(SCCB_SDA_GPIO_PORT, &GPIOInit_St);
}
void SCCB_Delay(void)
{
    Delay_us(10);
}
void SCCB_Start(void)
{
    SCCB_SDA(1);
    SCCB_SCL(1);
    SCCB_Delay();
    SCCB_SDA(0);
    SCCB_Delay();
    SCCB_SCL(0);
}
void SCCB_Stop(void)
{
    SCCB_SCL(0);
    SCCB_Delay();
    SCCB_SDA(1);
    SCCB_Delay();
    SCCB_SCL(1);
	SCCB_Delay();
}
void SCCB_SendByte(uint8_t Byte)
{
    int8_t DataIndex;
    uint8_t DataBit;
    
    for (DataIndex = 7; DataIndex >= 0; DataIndex--)
    {
        DataBit = (Byte >> DataIndex) & 0x01;
        SCCB_SDA(DataBit);
        SCCB_Delay();
        SCCB_SCL(1);
        SCCB_Delay();
        SCCB_SCL(0);
    }
    
    SCCB_SDA(1);
    SCCB_Delay();
    SCCB_SCL(1);
    SCCB_Delay();
    SCCB_SCL(0);
}
void SCCB_ReceiveByte(uint8_t *Byte)
{
    int8_t DataIndex;
    uint8_t DataBit;
    
    for (DataIndex = 7; DataIndex >= 0; DataIndex--)
    {
        SCCB_Delay();
        SCCB_SCL(1);
        DataBit = SCCB_READ_SDA();
        *Byte |= (DataBit << DataIndex);
        SCCB_Delay();
        SCCB_SCL(0);
    }
    
    SCCB_Delay();
    SCCB_SCL(1);
    SCCB_Delay();
    SCCB_SCL(0);
    SCCB_Delay();
    SCCB_SDA(0);
    SCCB_Delay();
}
void SCCB_3PhaseWrite(uint8_t ID_Addr, uint8_t Sub_Addr, uint8_t Data)
{
    SCCB_Start();
    SCCB_SendByte((ID_Addr << 1) | SCCB_WRITE);
    SCCB_SendByte(Sub_Addr);
    SCCB_SendByte(Data);
    SCCB_Stop();
}
void SCCB_2PhaseWrite(uint8_t ID_Addr, uint8_t Sub_Addr)
{
    SCCB_Start();
    SCCB_SendByte((ID_Addr << 1) | SCCB_WRITE);
    SCCB_SendByte(Sub_Addr);
    SCCB_Stop();
}
void SCCB_2PhaseRead(uint8_t ID_Addr, uint8_t *Data)
{
    SCCB_Start();
    SCCB_SendByte((ID_Addr << 1) | SCCB_READ);
    SCCB_ReceiveByte(Data);
    SCCB_Stop();
}
