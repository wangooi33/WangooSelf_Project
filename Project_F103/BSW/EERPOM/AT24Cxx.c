#include "AT24Cxx.h"

void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CYCCNT = 0;

    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
void Delay_us(uint32_t us)
{
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    uint32_t start = DWT->CYCCNT;

    while ((DWT->CYCCNT - start) < ticks);
}
void Delay_ms(uint32_t ms)
{
    while (ms--)
    {
        Delay_us(1000);
    }
}

void AT24Cxx_Init(void)
{
	IIC_Init();
}

uint8_t AT24Cxx_ReadByte(uint16_t Addr)
{
	uint8_t xReturn = 0;
	IIC_Start();

	/* 根据不同的24CXX型号, 发送高位地址
	 * 1, 24C16以上的型号, 分2个字节发送地址
	 * 2, 24C16及以下的型号, 分1个低字节地址 + 占用器件地址的bit1~bit3位 用于表示高位地址, 最多11位地址
	 *    对于24C01/02, 其器件地址格式(8bit)为: 1  0  1  0  A2  A1  A0  R/W
	 *    对于24C04,    其器件地址格式(8bit)为: 1  0  1  0  A2  A1  a8  R/W
	 *    对于24C08,    其器件地址格式(8bit)为: 1  0  1  0  A2  a9  a8  R/W
	 *    对于24C16,    其器件地址格式(8bit)为: 1  0  1  0  a10 a9  a8  R/W
	 *    R/W      : 读/写控制位 0,表示写; 1,表示读;
	 *    A0/A1/A2 : 对应器件的1,2,3引脚(只有24C01/02/04/8有这些脚)
	 *    a8/a9/a10: 对应存储整列的高位地址, 11bit地址最多可以表示2048个位置,可以寻址24C16及以内的型号
	 */    
	if (EE_TYPE > AT24C16)      /* 24C16以上的型号, 分2个字节发送地址 */
	{
		IIC_SendByte(0XA0);
		IIC_WaitAck();
		IIC_SendByte(Addr >> 8);/* 发送高字节地址 */
	}
	else 
	{
		IIC_SendByte(0XA0 + ((Addr >> 8) << 1));
	}

	IIC_WaitAck();
	IIC_SendByte(Addr % 256);
	IIC_WaitAck();

	IIC_Start();
	IIC_SendByte(0XA1);
	IIC_WaitAck();
	xReturn = IIC_ReadByte(0);
	IIC_Stop();

	return xReturn;
}

void AT24Cxx_WriteByte(uint16_t Addr, uint8_t Data)
{
	IIC_Start();

	if (EE_TYPE > AT24C16)
	{
		IIC_SendByte(0XA0);
		IIC_WaitAck();
		IIC_SendByte(Addr >> 8);
	}
	else 
	{
		IIC_SendByte(0XA0 + ((Addr >> 8) << 1));
	}
    
    IIC_WaitAck();
    IIC_SendByte(Addr % 256);
    IIC_WaitAck();
    
    IIC_SendByte(Data);
    IIC_WaitAck();
    IIC_Stop();
    Delay_ms(10);
}

void AT24Cxx_Read(uint16_t Addr, uint8_t *pBuf, uint16_t Datalen)
{
	while (Datalen--)
	{
	*pBuf++ = AT24Cxx_ReadByte(Addr++);
	}
}
/* 一次 I2C write 最多只能写 1 page，跨页会回卷覆盖 */
void AT24Cxx_Write(uint16_t Addr, uint8_t *pBuf, uint16_t Datalen)
{
	while (Datalen--)
	{
		AT24Cxx_WriteByte(Addr, *pBuf);
		Addr++;
		pBuf++;
	}
}

/**
 * @brief       检查AT24CXX是否正常
 *@note         检测原理: 在器件的末地址写如0X55, 然后再读取, 如果读取值为0X55
 *              则表示检测正常. 否则,则表示检测失败.
 * @retval      检测结果
 *              1: 检测成功
 *              0: 检测失败
 */
uint8_t AT24Cxx_Check(void)
{
    uint8_t xReturn;
    uint16_t Addr = EE_TYPE;
    xReturn = AT24Cxx_ReadByte(Addr);

	if (xReturn == 0X55)
	{
		return 1;
	}
	else
	{
		AT24Cxx_WriteByte(Addr,0X55);
		xReturn = AT24Cxx_ReadByte(255);
		if (xReturn == 0X55)
		{
			return 1;
		}
	}

	return 0;
}

