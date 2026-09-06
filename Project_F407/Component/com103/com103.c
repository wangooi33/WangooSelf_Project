#include "com103.h"
#include "usart.h"
#include "bldc_control.h"
#include "foc.h"
#include "pid.h"
#include "../EC11/ec11.h"
#include "../../BSW/task/task.h"
#include <math.h>
#include <string.h>

/* local constants ----------------------------------------------------------*/
#define C103_REQ_HEAD_0             0x3A
#define C103_REQ_HEAD_1             0x3A
#define C103_REQ_TAIL_0             0x4C
#define C103_REQ_TAIL_1             0x5E
#define C103_RSP_HEAD_0             0x4A
#define C103_RSP_HEAD_1             0x4A
#define C103_RSP_TAIL_0             0x4D
#define C103_RSP_TAIL_1             0x5E

#define C103_PARSER_BUFFER_SIZE     (U1_TXRX_BUFMAX + C103_FRAME_LENGTH)
#define C103_PARSER_KEEP_SIZE       (C103_FRAME_LENGTH - 1U)

/* local variable -----------------------------------------------------------*/
static uint8_t Com103_RxParserBuf[C103_PARSER_BUFFER_SIZE];
static uint16_t Com103_RxParserLen = 0U;

/* local helpers ------------------------------------------------------------*/
static uint16_t Com103_GetFunid(const uint8_t *pBuf)
{
    return ((uint16_t)pBuf[C103_FUNID_HIGH_INDEX] << 8) |
            pBuf[C103_FUNID_LOW_INDEX];
}

static int32_t Com103_GetS32(const uint8_t *pBuf)
{
    uint32_t value = ((uint32_t)pBuf[C103_DATA_STARTINDEX] << 24)
                   | ((uint32_t)pBuf[C103_DATA_STARTINDEX + 1U] << 16)
                   | ((uint32_t)pBuf[C103_DATA_STARTINDEX + 2U] << 8)
                   | ((uint32_t)pBuf[C103_DATA_STARTINDEX + 3U]);

    return (int32_t)value;
}

static void Com103_PutS32(uint8_t *pBuf, int32_t value)
{
    uint32_t raw = (uint32_t)value;

    pBuf[0] = (uint8_t)(raw >> 24);
    pBuf[1] = (uint8_t)(raw >> 16);
    pBuf[2] = (uint8_t)(raw >> 8);
    pBuf[3] = (uint8_t)raw;
}

static int32_t Com103_RoundToS32(float value)
{
    if (value >= 0.0f)
    {
        return (int32_t)(value + 0.5f);
    }

    return (int32_t)(value - 0.5f);
}

static int32_t Com103_GetBLDCCurrentPeak_mA(void)
{
    float iu = fabsf(BLDC_Info.PhaseCurrent[0]);
    float iv = fabsf(BLDC_Info.PhaseCurrent[1]);
    float iw = fabsf(BLDC_Info.PhaseCurrent[2]);
    float peak = iu;

    if (iv > peak)
    {
        peak = iv;
    }
    if (iw > peak)
    {
        peak = iw;
    }

    return Com103_RoundToS32(peak * 1000.0f);
}

static void Com103_FillHandshakeData(uint8_t *pBuf)
{
    memset(pBuf, 0, 4U);
    memcpy(pBuf, SoftWareID, 4U);
}

static int32_t Com103_ReadValue(C103Funid_t Funid)
{
    switch (Funid)
    {
        case CMid_ReadBLDC_RPM:
            return Com103_RoundToS32(BLDC_Info.RPM);

        case CMid_ReadBLDC_Pos:
            return Com103_RoundToS32(BLDC_GetCurrentTurns() * 360.0f);

        case CMid_ReadBLDC_Cur:
            return Com103_GetBLDCCurrentPeak_mA();

        default:
            return 0;
    }
}

static void Com103_ApplyCommand(C103Funid_t Funid, int32_t Value)
{
    float targetDeg;
    float currentDeg;

    switch (Funid)
    {
        case CMid_WriteBLDC_RPM:
            Task_SetControlMode(TASK_CONTROL_SPEED);
            currentDeg = BLDC_GetCurrentTurns() * 360.0f;
            EC11_AbsoluteAngleDeg = currentDeg;
            EC11_TargetAngleDeg = currentDeg;
            BLDC_SetSpeedRef((float)Value);
            break;

        case CMid_WriteBLDC_Pos:
            Task_SetControlMode(TASK_CONTROL_POSITION);
            /*
             * Host position commands are absolute motor angles. Position_Ref
             * is a relative move, so convert from current Hall position.
             */
            targetDeg = (float)Value;
            currentDeg = BLDC_GetCurrentTurns() * 360.0f;
            BLDC_SetPositionRef((targetDeg * C103_DEG_TO_TURNS) -
                                (currentDeg * C103_DEG_TO_TURNS));
            EC11_AbsoluteAngleDeg = targetDeg;
            EC11_TargetAngleDeg = targetDeg;
            break;

        case CMid_WriteBLDC_Cur:
            Task_SetControlMode(TASK_CONTROL_CURRENT);
            currentDeg = BLDC_GetCurrentTurns() * 360.0f;
            EC11_AbsoluteAngleDeg = currentDeg;
            EC11_TargetAngleDeg = currentDeg;
            FOC_Info.Iq_Ref = Clampf((float)Value * C103_MA_TO_A,
                                     -C103_CURRENT_LIMIT_A,
                                      C103_CURRENT_LIMIT_A);
            break;

        default:
            break;
    }
}

static uint8_t Com103_IsSupportedFunid(C103Funid_t Funid)
{
    switch (Funid)
    {
        case CMid_Handshake:

        case CMid_ReadBLDC_RPM:
        case CMid_ReadBLDC_Pos:
        case CMid_ReadBLDC_Cur:
        case CMid_WriteBLDC_RPM:
        case CMid_WriteBLDC_Pos:
        case CMid_WriteBLDC_Cur:
            return 1U;

        default:
            return 0U;
    }
}

static void Com103_RemoveBytes(uint16_t Count)
{
    if (Count >= Com103_RxParserLen)
    {
        Com103_RxParserLen = 0U;
        return;
    }

    memmove(&Com103_RxParserBuf[0],
            &Com103_RxParserBuf[Count],
            (uint32_t)(Com103_RxParserLen - Count));
    Com103_RxParserLen -= Count;
}

static void Com103_KeepTailForNextChunk(void)
{
    uint16_t keep = Com103_RxParserLen;

    if (keep > C103_PARSER_KEEP_SIZE)
    {
        keep = C103_PARSER_KEEP_SIZE;
    }

    if (keep == 0U)
    {
        Com103_RxParserLen = 0U;
        return;
    }

    memmove(&Com103_RxParserBuf[0],
            &Com103_RxParserBuf[Com103_RxParserLen - keep],
            keep);
    Com103_RxParserLen = keep;
}

static void Com103_AppendRxData(const uint8_t *pData, uint16_t Size)
{
    uint16_t freeSpace;

    if (pData == NULL || Size == 0U)
    {
        return;
    }

    freeSpace = (uint16_t)(sizeof(Com103_RxParserBuf) - Com103_RxParserLen);
    if (Size > freeSpace)
    {
        Size = freeSpace;
    }

    memcpy(&Com103_RxParserBuf[Com103_RxParserLen], pData, Size);
    Com103_RxParserLen += Size;
}

static void Com103_ParsePendingFrames(void)
{
    uint16_t frameIndex;

    while (Com103_RxParserLen >= C103_FRAME_LENGTH)
    {
        uint8_t found = 0U;

        frameIndex = 0U;
        while ((uint32_t)frameIndex + C103_FRAME_LENGTH <=
               (uint32_t)Com103_RxParserLen)
        {
            if (Com103_RxParserBuf[frameIndex] == C103_REQ_HEAD_0 &&
                Com103_RxParserBuf[frameIndex + 1U] == C103_REQ_HEAD_1)
            {
                found = 1U;
                break;
            }
            frameIndex++;
        }

        if (found == 0U)
        {
            Com103_KeepTailForNextChunk();
            return;
        }

        if (frameIndex > 0U)
        {
            Com103_RemoveBytes(frameIndex);
        }

        if (Com103_CheckRequestFrame(Com103_RxParserBuf,
                                     C103_FRAME_LENGTH) != 0U)
        {
            Com103_RxEventHandler(Com103_RxParserBuf, C103_FRAME_LENGTH);
            Com103_RemoveBytes(C103_FRAME_LENGTH);
        }
        else
        {
            /* Drop one byte and search again. */
            Com103_RemoveBytes(1U);
        }
    }
}

static void Com103_RearmRx(void)
{
    (void)HAL_UARTEx_ReceiveToIdle_DMA(&huart1,
                                       gU1TxRxBuf,
                                       U1_TXRX_BUFMAX);
}

/* public functions ---------------------------------------------------------*/
void Com103_Init(void)
{
    Com103_RxParserLen = 0U;
    memset(gU1TxRxBuf, 0, U1_TXRX_BUFMAX);
    memset(Com103_RxParserBuf, 0, sizeof(Com103_RxParserBuf));

    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1,
                                     gU1TxRxBuf,
                                     U1_TXRX_BUFMAX) != HAL_OK)
    {
        return;
    }

    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
}

void Com103_Cyclic(void)
{
    uint32_t rxEventType;
    uint16_t received;

    if (huart1.RxState != HAL_UART_STATE_READY)
    {
        return;
    }

    rxEventType = HAL_UARTEx_GetRxEventType(&huart1);
    received = (uint16_t)(huart1.RxXferSize - huart1.RxXferCount);

    if ((rxEventType == HAL_UART_RXEVENT_IDLE ||
         rxEventType == HAL_UART_RXEVENT_TC) &&
        received > 0U &&
        received <= huart1.RxXferSize)
    {
        Com103_AppendRxData(gU1TxRxBuf, received);
        /* Rearm before parsing so a response never leaves the receiver blind. */
        Com103_RearmRx();
        Com103_ParsePendingFrames();
        return;
    }

    Com103_RearmRx();
}

void Com103_TxProcess(const uint8_t *pBuf, C103Funid_t Funid)
{
    uint8_t TxBuf[C103_FRAME_LENGTH];
    uint8_t index = 0U;
    uint16_t crc16;

    TxBuf[index++] = C103_RSP_HEAD_0;
    TxBuf[index++] = C103_RSP_HEAD_1;
    TxBuf[index++] = (uint8_t)((uint16_t)Funid >> 8);
    TxBuf[index++] = (uint8_t)((uint16_t)Funid & 0xFFU);

    if (pBuf != NULL)
    {
        memcpy(&TxBuf[index], pBuf, 4U);
    }
    else
    {
        memset(&TxBuf[index], 0, 4U);
    }
    index += 4U;

    crc16 = CheckCRC16(TxBuf, index);
    TxBuf[index++] = (uint8_t)(crc16 >> 8);
    TxBuf[index++] = (uint8_t)(crc16 & 0xFFU);
    TxBuf[index++] = C103_RSP_TAIL_0;
    TxBuf[index++] = C103_RSP_TAIL_1;

    HAL_UART_Transmit(&huart1, TxBuf, index, 50);
}

uint8_t Com103_CheckRequestFrame(const uint8_t *pBuf, uint16_t Size)
{
    uint16_t crc16;
    uint16_t crc16_origin;

    if (pBuf == NULL || Size != C103_FRAME_LENGTH)
    {
        return 0U;
    }

    if (pBuf[0] != C103_REQ_HEAD_0 || pBuf[1] != C103_REQ_HEAD_1)
    {
        return 0U;
    }

    if (pBuf[C103_FRAME_LENGTH - 2U] != C103_REQ_TAIL_0 ||
        pBuf[C103_FRAME_LENGTH - 1U] != C103_REQ_TAIL_1)
    {
        return 0U;
    }

    crc16 = CheckCRC16((uint8_t *)pBuf, C103_CRC_STARTINDEX);
    crc16_origin = ((uint16_t)pBuf[C103_CRC_STARTINDEX] << 8) |
                    pBuf[C103_CRC_STARTINDEX + 1U];

    return (crc16 == crc16_origin) ? 1U : 0U;
}

void Com103_RxEventHandler(uint8_t *pBuf, uint16_t Size)
{
    uint8_t txData[4];
    C103Funid_t funid;
    int32_t data;

    if (Com103_CheckRequestFrame(pBuf, Size) == 0U)
    {
        return;
    }

    funid = (C103Funid_t)Com103_GetFunid(pBuf);
    if (Com103_IsSupportedFunid(funid) == 0U)
    {
        return;
    }

    data = Com103_GetS32(pBuf);

    switch (funid)
    {
        case CMid_Handshake:
            Com103_FillHandshakeData(txData);
            Com103_TxProcess(txData, funid);
            break;

        case CMid_ReadBLDC_RPM:
        case CMid_ReadBLDC_Pos:
        case CMid_ReadBLDC_Cur:
            Com103_PutS32(txData, Com103_ReadValue(funid));
            Com103_TxProcess(txData, funid);
            break;

        case CMid_WriteBLDC_RPM:
        case CMid_WriteBLDC_Pos:
        case CMid_WriteBLDC_Cur:
            Com103_ApplyCommand(funid, data);
            Com103_PutS32(txData, data);
            Com103_TxProcess(txData, funid);
            break;

        default:
            break;
    }
}
