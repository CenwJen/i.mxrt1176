/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "fsl_flexcan.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_gpt.h"

#include "data.h"
#include "Master.h"
#include "canfestival.h"

#include "FreeRTOS.h"
#include "task.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define GPT_IRQ_ID             GPT2_IRQn
#define EXAMPLE_GPT            GPT2
#define EXAMPLE_GPT_IRQHandler GPT2_IRQHandler

/* Get source clock for GPT driver (GPT prescaler = 0) */
#define EXAMPLE_GPT_CLK_FREQ CLOCK_GetFreq(kCLOCK_OscRc48MDiv2)


#define EXAMPLE_CAN           CAN1
#define RX_MESSAGE_BUFFER_NUM (9)
#define TX_MESSAGE_BUFFER_NUM (8)

/* Select OSC24Mhz as master flexcan clock source */
#define FLEXCAN_CLOCK_SOURCE_SELECT (1U)
/* Clock divider for master flexcan clock source */
#define FLEXCAN_CLOCK_SOURCE_DIVIDER (1U)
/* Get frequency of flexcan clock */
#define EXAMPLE_CAN_CLK_FREQ ((CLOCK_GetRootClockFreq(kCLOCK_Root_Can1) / 100000U) * 100000U)
/* Set USE_IMPROVED_TIMING_CONFIG macro to use api to calculates the improved CAN / CAN FD timing values. */
#define USE_IMPROVED_TIMING_CONFIG (1U)
/* Fix MISRA_C-2012 Rule 17.7. */
#define LOG_INFO (void)PRINTF

#define DLC (8)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
flexcan_handle_t flexcanHandle;
volatile bool txComplete = false;
volatile bool rxComplete = false;
volatile bool wakenUp    = false;

flexcan_mb_transfer_t txXfer, rxXfer;
flexcan_config_t flexcanConfig;
flexcan_rx_mb_config_t mbConfig;

//#if (defined(USE_CANFD) && USE_CANFD)
//flexcan_fd_frame_t frame;
//#else
//flexcan_frame_t frame;
//#endif

//uint32_t txIdentifier;
//uint32_t rxIdentifier;
uint32_t gptFreq;
gpt_config_t gptConfig;
	
uint8_t node_type;
uint8_t t = 0;
volatile bool gptIsrFlag = false;
int i = 0;

TIMEVAL last_counter_val = 0;
TIMEVAL elapsed_time = 0;

flexcan_frame_t rxFrame;
flexcan_frame_t txFrame;
Message Rx_Message;  
Message rxMessage; 

unsigned char nodeID;
//static CO_Data *co_data = NULL;

static TaskHandle_t AppTaskCreate_Handle = NULL;
static TaskHandle_t CANopenRx_Task_Handle = NULL;
/*******************************************************************************
 * Code
 ******************************************************************************/
 unsigned char canChangeBaudRate_driver( CAN_HANDLE fd, char* baud)
{ 
	return 0;
}
 
 
 //Set the timer for the next alarm.
void setTimer(TIMEVAL value)
{
		EXAMPLE_GPT->OCR[0] = EXAMPLE_GPT->CNT + value;
}

//Return the elapsed time to tell the Stack how much time is spent since last call.
TIMEVAL getElapsedTime(void)
{
		return EXAMPLE_GPT->CNT;
}

// The driver send a CAN message passed from the CANopen stack

unsigned char canSend(CAN_PORT port, Message *message) 
{
	
    flexcan_frame_t txFrame;
    status_t status;

		txFrame.format	= (message->cob_id > 0x7FF) ? kFLEXCAN_FrameFormatExtend : kFLEXCAN_FrameFormatStandard;
		txFrame.type  	= (message->rtr) ? kFLEXCAN_FrameTypeRemote : kFLEXCAN_FrameTypeData;
		txFrame.id 			= (txFrame.format == kFLEXCAN_FrameFormatStandard) ? FLEXCAN_ID_STD(message->cob_id) : FLEXCAN_ID_EXT(message->cob_id);
		txFrame.length = message->len;
	
		txFrame.dataByte0 = message->data[0];
		txFrame.dataByte1 = message->data[1];
		txFrame.dataByte2 = message->data[2];
		txFrame.dataByte3 = message->data[3];
		txFrame.dataByte4 = message->data[4];
		txFrame.dataByte5 = message->data[5];
		txFrame.dataByte6 = message->data[6];
		txFrame.dataByte7 = message->data[7];
	
    txXfer.frame = &txFrame;
    status = FLEXCAN_TransferSendNonBlocking(EXAMPLE_CAN, &flexcanHandle, &txXfer);
//		while (!txComplete)
//    {
//			LOG_INFO("%d\r\n", txComplete);
//    };
						
    txComplete = false;
//    FLEXCAN_ClearMbStatusFlags(EXAMPLE_CAN, 0x00000200);
		
    return (status == kStatus_Success) ? 0 : 1;
}
 
void EXAMPLE_GPT_IRQHandler(void)
{
    /* Clear interrupt flag.*/
    GPT_ClearStatusFlags(EXAMPLE_GPT, kGPT_OutputCompare1Flag);		
	
		TimeDispatch();
		//LOG_INFO("\r\n GPT interrupt is occurred %d!!", i);

    SDK_ISR_EXIT_BARRIER;

}

/*!
 * @brief FlexCAN Call Back function
 */
static FLEXCAN_CALLBACK(flexcan_callback)
{
    switch (status)
    {
        case kStatus_FLEXCAN_RxIdle:
            if (RX_MESSAGE_BUFFER_NUM == result)
            {															
                rxComplete = true;
            }
            break;

        case kStatus_FLEXCAN_TxIdle:
            if (TX_MESSAGE_BUFFER_NUM == result)
            {
								FLEXCAN_ClearMbStatusFlags(EXAMPLE_CAN, 0x00000200);
							
                txComplete = true;
            }
            break;

        case kStatus_FLEXCAN_WakeUp:
            wakenUp = true;
            break;

        default:
            break;
    }
}


static void BSP_Init(void)
{
    /* Initialize board hardware. */
    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
		
		GPT_GetDefaultConfig(&gptConfig);
    /* Initialize GPT module */
    GPT_Init(EXAMPLE_GPT, &gptConfig);
	
    /* Divide GPT clock source frequency by 3 inside GPT module */
    GPT_SetClockDivider(EXAMPLE_GPT, 23);
	
    /* Get GPT clock frequency */
    gptFreq = EXAMPLE_GPT_CLK_FREQ;
		
    /* GPT frequency is divided by 3 inside module */
		gptFreq = 1;
		
    /* Set both GPT modules to 1 second duration */
    GPT_SetOutputCompareValue(EXAMPLE_GPT, kGPT_OutputCompare_Channel1, gptFreq);
		
    /* Enable GPT Output Compare1 interrupt */
    GPT_EnableInterrupts(EXAMPLE_GPT, kGPT_OutputCompare1InterruptEnable);
		
    /* Enable at the Interrupt */
    EnableIRQ(GPT_IRQ_ID);
		
    /* Start Timer */
    PRINTF("\r\nStarting GPT timer ...\r\n");
    GPT_StartTimer(EXAMPLE_GPT);
		
    /*Clock setting for FLEXCAN*/
    clock_root_config_t rootCfg = {0};
    rootCfg.mux                 = FLEXCAN_CLOCK_SOURCE_SELECT;
    rootCfg.div                 = FLEXCAN_CLOCK_SOURCE_DIVIDER;
    CLOCK_SetRootClock(kCLOCK_Root_Can1, &rootCfg);

    LOG_INFO("********* FLEXCAN1 Interrupt EXAMPLE *********\r\n");
    LOG_INFO("    Message format: Standard (11 bit id)\r\n");
    LOG_INFO("    Message buffer %d used for Rx.\r\n", RX_MESSAGE_BUFFER_NUM);
    LOG_INFO("    Message buffer %d used for Tx.\r\n", TX_MESSAGE_BUFFER_NUM);
    LOG_INFO("    Interrupt Mode: Enabled\r\n");
    LOG_INFO("    Operation Mode: TX and RX --> Normal\r\n");
    LOG_INFO("*********************************************\r\n\r\n");

//		txIdentifier = 0x123;
//    rxIdentifier = 0x123;
//    nodeID = 0x02;

    /* Get FlexCAN module default Configuration. */
    /*
     * flexcanConfig.clkSrc                 = kFLEXCAN_ClkSrc0;
     * flexcanConfig.bitRate               = 1000000U;
     * flexcanConfig.bitRateFD             = 2000000U;
     * flexcanConfig.maxMbNum               = 16;
     * flexcanConfig.enableLoopBack         = false;
     * flexcanConfig.enableSelfWakeup       = false;
     * flexcanConfig.enableIndividMask      = false;
     * flexcanConfig.disableSelfReception   = false;
     * flexcanConfig.enableListenOnlyMode   = false;
     * flexcanConfig.enableDoze             = false;
     */
    FLEXCAN_GetDefaultConfig(&flexcanConfig);


#if (defined(USE_IMPROVED_TIMING_CONFIG) && USE_IMPROVED_TIMING_CONFIG)
    flexcan_timing_config_t timing_config;
    memset(&timing_config, 0, sizeof(flexcan_timing_config_t));

    if (FLEXCAN_CalculateImprovedTimingValues(EXAMPLE_CAN, flexcanConfig.bitRate, EXAMPLE_CAN_CLK_FREQ, &timing_config))
    {
        /* Update the improved timing configuration*/
        memcpy(&(flexcanConfig.timingConfig), &timing_config, sizeof(flexcan_timing_config_t));
    }
    else
    {
        LOG_INFO("No found Improved Timing Configuration. Just used default configuration\r\n\r\n");
    }

#endif


    FLEXCAN_Init(EXAMPLE_CAN, &flexcanConfig, EXAMPLE_CAN_CLK_FREQ);
		
		//setNodeId(&Master_Data, nodeID);
		setState(&Master_Data, Initialisation);		
		setState(&Master_Data, Operational);

    /* Create FlexCAN handle structure and set call back function. */
    FLEXCAN_TransferCreateHandle(EXAMPLE_CAN, &flexcanHandle, flexcan_callback, NULL);

    /* Set Rx Masking mechanism. */
    FLEXCAN_SetRxMbGlobalMask(EXAMPLE_CAN, 0);

    /* Setup Rx Message Buffer. */
    mbConfig.format = kFLEXCAN_FrameFormatStandard;
    mbConfig.type   = kFLEXCAN_FrameTypeData;
    mbConfig.id     = FLEXCAN_ID_STD(0);

    FLEXCAN_SetRxMbConfig(EXAMPLE_CAN, RX_MESSAGE_BUFFER_NUM, &mbConfig, true);


		/* Setup Tx Message Buffer. */

    FLEXCAN_SetTxMbConfig(EXAMPLE_CAN, TX_MESSAGE_BUFFER_NUM, true);
}
		
		
static void CANopenRx_Task(void* parameter)
{
    while (1) {
        /* Start receive data through Rx Message Buffer. */
        rxXfer.mbIdx = (uint8_t)RX_MESSAGE_BUFFER_NUM;

        rxXfer.frame = &rxFrame;
				LOG_INFO("Node 1\r\n");
								 
        (void)FLEXCAN_TransferReceiveNonBlocking(EXAMPLE_CAN, &flexcanHandle, &rxXfer);

				LOG_INFO("Node 2\r\n");
				rxMessage.cob_id = (rxXfer.frame->id >> CAN_ID_STD_SHIFT);
				rxMessage.len = rxXfer.frame->length;
				for (int i = 0; i < rxMessage.len; i++) 
				{
					switch (i) {
						case 3: rxMessage.data[i] = rxXfer.frame->dataByte3; break;
						case 2: rxMessage.data[i] = rxXfer.frame->dataByte2; break;
						case 1: rxMessage.data[i] = rxXfer.frame->dataByte1; break;
						case 0: rxMessage.data[i] = rxXfer.frame->dataByte0; break;
						case 7: rxMessage.data[i] = rxXfer.frame->dataByte7; break;
						case 6: rxMessage.data[i] = rxXfer.frame->dataByte6; break;
						case 5: rxMessage.data[i] = rxXfer.frame->dataByte5; break;
						case 4: rxMessage.data[i] = rxXfer.frame->dataByte4; break;
						default: break;
						}
				}
				LOG_INFO("Node 3\r\n");
				canDispatch(&Master_Data, &rxMessage);
				LOG_INFO("Node 4\r\n");
				
				for (int i = 0; i < rxMessage.len; i++) 
				{
					LOG_INFO("Rx MB ID: 0x%3x, Rx MB data: 0x%x, Time stamp: %d\r\n", rxFrame.id >> CAN_ID_STD_SHIFT,
                 rxMessage.data[i], rxFrame.timestamp);
				}
        }
}


static void AppTaskCreate(void)
{
      BaseType_t xReturn = pdPASS;
      taskENTER_CRITICAL(); 

      xReturn = xTaskCreate((TaskFunction_t )CANopenRx_Task, 
                            (const char*    )"CANopenRx_Task",
                            (uint16_t       )512,  
                            (void*          )NULL, 
                            (UBaseType_t    )2,
                            (TaskHandle_t*  )&CANopenRx_Task_Handle);
			if (pdPASS == xReturn)
            PRINTF("CANopenRx_Task success!\r\n");

      vTaskDelete(AppTaskCreate_Handle);

      taskEXIT_CRITICAL();
}

/*!
 * @brief Main function
 */
int main(void)
{	
		BaseType_t xReturn = pdPASS;
    BSP_Init();

    xReturn = xTaskCreate((TaskFunction_t )AppTaskCreate,
                          (const char*    )"AppTaskCreate",
                          (uint16_t       )512,  
                          (void*          )NULL,
                          (UBaseType_t    )1,
													(TaskHandle_t*  )&AppTaskCreate_Handle);
													
    if (pdPASS == xReturn)
        vTaskStartScheduler();
    else
    return -1;

    while (1);
}
