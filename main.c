#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_hid.h" 

#define USBHID_POWER			0x01
#define USBHID_NEXT			0x02
#define USBHID_PREVIOUS			0x04
#define USBHID_STOP			0x08
#define USBHID_PLAYPAUSE		0x10
#define USBHID_MUTE			0x20
#define USBHID_VOLUMEUP			0x40
#define USBHID_VOLUMEDOWN		0x80

CAN_HandleTypeDef CanHandle;
CanTxMsgTypeDef CanTxMessage;
CanRxMsgTypeDef CanRxMessage;

typedef struct
{
	uint32_t Id;
	uint32_t DataLen;
	uint32_t Data[8];
} CAN_Msg_t;

#define CAN_MSG_MAX	64

uint16_t CAN_Msg_Count=0;
CAN_Msg_t CAN_Msg_List[CAN_MSG_MAX]; //List of messages to accumulate

USBD_HandleTypeDef USBD_Device;

void SystemClock_Config(void);
void LED_Init(void);
void LED_GreenOn(void);
void LED_GreenOff(void);
void LED_GreenToggle(void);
void LED_RedOn(void);
void LED_RedOff(void);
void LED_RedToggle(void);
void Button_Init(void);
uint32_t Button_GetState(void);

volatile uint32_t *DWT_CYCCNT=(volatile uint32_t *)0xE0001004;
volatile uint32_t *DWT_CONTROL=(volatile uint32_t *)0xE0001000;
volatile uint32_t *SCB_DEMCR=(volatile uint32_t *)0xE000EDFC;

uint32_t TimingGetMs(void)
{
	return (*DWT_CYCCNT)/9600000;
}

void USBHID_Key(uint8_t key)
{
	uint8_t press[2]={ 0x02, key };
	uint8_t release[2]={ 0x02, 0x00 };

	USBD_HID_SendReport(&USBD_Device, press, 2);
	HAL_Delay(10);
	USBD_HID_SendReport(&USBD_Device, release, 2);
	HAL_Delay(10);
}

void CANSend(uint32_t id, uint32_t len, uint32_t b0, uint32_t b1, uint32_t b2, uint32_t b3, uint32_t b4, uint32_t b5, uint32_t b6, uint32_t b7)
{
	// Only send it when the module is ready
	if(HAL_CAN_GetState(&CanHandle)==HAL_CAN_STATE_READY)
	{
		CanHandle.pTxMsg->IDE=CAN_ID_STD;
		CanHandle.pTxMsg->StdId=id;
		CanHandle.pTxMsg->ExtId=0;
		CanHandle.pTxMsg->RTR=CAN_RTR_DATA;
		CanHandle.pTxMsg->DLC=len;
		CanHandle.pTxMsg->Data[0]=b0;
		CanHandle.pTxMsg->Data[1]=b1;
		CanHandle.pTxMsg->Data[2]=b2;
		CanHandle.pTxMsg->Data[3]=b3;
		CanHandle.pTxMsg->Data[4]=b4;
		CanHandle.pTxMsg->Data[5]=b5;
		CanHandle.pTxMsg->Data[6]=b6;
		CanHandle.pTxMsg->Data[7]=b7;
		HAL_CAN_Transmit_IT(&CanHandle);
		HAL_Delay(10); // wait at least 10ms for the message to transmit
	}
}

void DISText(int8_t *Line1, int8_t *Line2)
{
	// Continously transmit these messages for DIS text display

	// Radio status, apparently byte 1 = 0x01 is enough to tell the cluster it's "on"
	// other values mean other things, maybe Navigation? I donno.
	CANSend(0x661, 8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

	// Cluster DIS text line 1
	// 0x261 8 ASCII text (alphanumeric, uppercase only)
	CANSend(0x261, 8, Line1[0], Line1[1], Line1[2], Line1[3], Line1[4], Line1[5], Line1[6], Line1[7]);

	// Cluster DIS text line 2
	// 0x263 8 ASCII text (alphanumeric, uppercase only)
	CANSend(0x263, 8, Line2[0], Line2[1], Line2[2], Line2[3], Line2[4], Line2[5], Line2[6], Line2[7]);
}

// Used for searching the CAN message list for an ID
// If the ID isn't found, return -1
int32_t find_id(uint32_t Id)
{
	int32_t i;

	for(i=0;i<CAN_Msg_Count;i++)
	{
		if(CAN_Msg_List[i].Id==Id)
			return i;
	}

	return -1;
}

void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef *CanHandle)
{
	int32_t index=0, i;

	LED_GreenToggle(); //Toggle green LED on message

	index=find_id(CanHandle->pRxMsg->StdId);

	// If ID isn't found, add it to the end of the list
	if(index==-1)
	{
		CAN_Msg_List[CAN_Msg_Count].Id=CanHandle->pRxMsg->StdId; // Copy ID
		CAN_Msg_List[CAN_Msg_Count].DataLen=CanHandle->pRxMsg->DLC; // Copy data length

		for(i=0;i<CanHandle->pRxMsg->DLC;i++)
			CAN_Msg_List[CAN_Msg_Count].Data[i]=CanHandle->pRxMsg->Data[i];

		CAN_Msg_Count++;
	}
	else // else just update data
	{
		for(i=0;i<CanHandle->pRxMsg->DLC;i++)
			CAN_Msg_List[index].Data[i]=CanHandle->pRxMsg->Data[i];
	}

	if(HAL_CAN_Receive_IT(CanHandle, CAN_FIFO0)!=HAL_OK)
		LED_RedOn(); //Red LED = CAN controller crashed
}

HAL_StatusTypeDef CAN_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	CAN_FilterConfTypeDef sFilterConfig;

	HAL_CAN_DeInit(&CanHandle);

	__HAL_RCC_CAN1_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin=GPIO_PIN_12|GPIO_PIN_11; //TX = PA12 RX = PA11
	GPIO_InitStruct.Mode=GPIO_MODE_AF_PP;
	GPIO_InitStruct.Speed=GPIO_SPEED_FAST;
	GPIO_InitStruct.Pull=GPIO_PULLUP;
	GPIO_InitStruct.Alternate=GPIO_AF9_CAN1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	CanHandle.Instance=CAN1;
	CanHandle.pTxMsg=&CanTxMessage;
	CanHandle.pRxMsg=&CanRxMessage;
	CanHandle.Init.TTCM=DISABLE;
	CanHandle.Init.ABOM=DISABLE;
	CanHandle.Init.AWUM=DISABLE;
	CanHandle.Init.NART=DISABLE;
	CanHandle.Init.RFLM=DISABLE;
	CanHandle.Init.TXFP=DISABLE;
	CanHandle.Init.Mode=CAN_MODE_NORMAL;
	CanHandle.Init.SJW=CAN_SJW_1TQ;
	CanHandle.Init.BS1=CAN_BS1_15TQ;
	CanHandle.Init.BS2=CAN_BS2_4TQ;
	//prescale=bus/(canbus_baud*(sjw+bs1+bs2))
//	CanHandle.Init.Prescaler=20;	//100kbps @ 42MHz 1SJW/16BS1/4BS2
	CanHandle.Init.Prescaler=12;	//100kbps @ 24MHz 1SJW/15BS1/4BS2

	if(HAL_CAN_Init(&CanHandle)!=HAL_OK)
		return HAL_ERROR;

	sFilterConfig.FilterNumber=0;
	sFilterConfig.FilterMode=CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale=CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh=0x0000;
	sFilterConfig.FilterIdLow=0x0000;
	sFilterConfig.FilterMaskIdHigh=0x0000;
	sFilterConfig.FilterMaskIdLow=0x0000;
	sFilterConfig.FilterFIFOAssignment=0;
	sFilterConfig.FilterActivation=ENABLE;
	sFilterConfig.BankNumber=0;

	if(HAL_CAN_ConfigFilter(&CanHandle, &sFilterConfig)!=HAL_OK)
		return HAL_ERROR;

	HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);

	return HAL_OK;
}

int main(void)
{
	int32_t i, j, idx=0, ButtonPressed=0;
	uint32_t time=0;

	HAL_Init();
	SystemClock_Config();

	// Set up cycle counter for timing
	*SCB_DEMCR|=0x01000000;
	*DWT_CYCCNT=0;
	*DWT_CONTROL|=1;

	USBD_Init(&USBD_Device, &HID_Desc, 0);
	USBD_RegisterClass(&USBD_Device, USBD_HID_CLASS);
	USBD_Start(&USBD_Device);

	// Wait a bit for USB to settle
	HAL_Delay(1000);

	// LEDs are used for error status
	LED_Init();

	// Initalize CAN after LCD due to GPIO pin conflict
	if(CAN_Init()!=HAL_OK)
		LED_RedOn();

	// Start CANbus receiving interrupt
	if(HAL_CAN_Receive_IT(&CanHandle, CAN_FIFO0)!=HAL_OK)
		LED_RedOn();

	// Make sure CAN message list is cleared
	memset(&CAN_Msg_List, 0, sizeof(CAN_Msg_t)*CAN_MSG_MAX);

	time=TimingGetMs();

	// Main loop
	while(1)
	{
		int8_t Line1[10]="          ";
		int8_t Line2[10]="          ";

		// Pull Speed and convert to MPH/KPH
//		idx=find_id(0x351);
//		if(idx!=-1)
//			sprintf(Line1, " %0.3dMPH ", ((CAN_Msg_List[idx].Data[2]*256)+CAN_Msg_List[idx].Data[1])/322);

		// Pull RPM
//		idx=find_id(0x353);
//		if(idx!=-1)
//			sprintf(Line2, "%0.4dRPM ", (CAN_Msg_List[idx].Data[2]*256+CAN_Msg_List[idx].Data[1])/4);

		// Update DIS text only if ignition key is in position "3" (on)
		idx=find_id(0x271);
		if(idx!=-1)
		{
			if(CAN_Msg_List[idx].Data[0]==0x03)
			{
				// Only update every 100ms
				if((time-TimingGetMs())>100)
				{
					time=TimingGetMs();
					LED_RedToggle();
					DISText(Line1, Line2);
				}
			}
		}

		// Pull SWC status with one-shot keypress.
		// SWC only allows one button at a time, but long
		// presses would cause multiple USB keyboard presses.

		// Find the index in the CANbus message list
		idx=find_id(0x5c3);
		if(idx!=-1)
		{
			// Audi C5 A6 Tiptronic MFSW (non phone) is byte 1 = 0x39
			if(CAN_Msg_List[idx].Data[0]==0x39)
			{
				// Previous (Top left)
				if(CAN_Msg_List[idx].Data[1]==0x02&&ButtonPressed==0)
				{
					ButtonPressed=1;
					USBHID_Key(USBHID_POWER); // Assign as "Power"
				}

				// Next (Middle left)
				if(CAN_Msg_List[idx].Data[1]==0x03&&ButtonPressed==0)
				{
					ButtonPressed=1;
					USBHID_Key(USBHID_PLAYPAUSE); // Assign as "Play/Pause"
				}

				// Scan up (Bottom left
				if(CAN_Msg_List[idx].Data[1]==0x04&&ButtonPressed==0)
				{
					ButtonPressed=1;
					USBHID_Key(USBHID_NEXT); // Assign as "Next"
				}

				// Scan down (Bottom right)
				if(CAN_Msg_List[idx].Data[1]==0x05&&ButtonPressed==0)
				{
					ButtonPressed=1;
					USBHID_Key(USBHID_PREVIOUS); // Assign as "Previous"
				}

				// Volume up (Top right)
				if(CAN_Msg_List[idx].Data[1]==0x06&&ButtonPressed==0)
				{
					ButtonPressed=1;
					USBHID_Key(USBHID_VOLUMEUP); // Assign as "Volume up"
				}

				// Volume down (Middle right)
				if(CAN_Msg_List[idx].Data[1]==0x07&&ButtonPressed==0)
				{
					ButtonPressed=1;
					USBHID_Key(USBHID_VOLUMEDOWN); // Assign as "Volume down"
				}

				//Reset one-shot when "SWC = none"
				if(CAN_Msg_List[idx].Data[1]==0x00&&ButtonPressed==1)
					ButtonPressed=0;
			}
		}
	}
}

void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	RCC_OscInitStruct.OscillatorType=RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState=RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState=RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource=RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM=8;
	RCC_OscInitStruct.PLL.PLLN=192;	//96MHz for low power
//	RCC_OscInitStruct.PLL.PLLN=336;	//168MHz for testing
	RCC_OscInitStruct.PLL.PLLP=RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ=4; // divider for 96MHz
//	RCC_OscInitStruct.PLL.PLLQ=7; // divider for 168MHz
	HAL_RCC_OscConfig(&RCC_OscInitStruct);
	HAL_PWREx_EnableOverDrive();
  
	RCC_ClkInitStruct.ClockType=RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider=RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider=RCC_HCLK_DIV4;  
	RCC_ClkInitStruct.APB2CLKDivider=RCC_HCLK_DIV2;  
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

	PeriphClkInitStruct.PeriphClockSelection=RCC_PERIPHCLK_LTDC;
	PeriphClkInitStruct.PLLSAI.PLLSAIN=192;
	PeriphClkInitStruct.PLLSAI.PLLSAIR=4;
	PeriphClkInitStruct.PLLSAIDivR=RCC_PLLSAIDIVR_8;
	HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct); 
}

void LED_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;

	__GPIOG_CLK_ENABLE();
	GPIO_InitStruct.Pin=GPIO_PIN_13|GPIO_PIN_14;
	GPIO_InitStruct.Mode=GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull=GPIO_PULLUP;
	GPIO_InitStruct.Speed=GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);
}

void LED_GreenOn(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
}

void LED_GreenOff(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET);
}

void LED_GreenToggle(void)
{
	HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13);
}

void LED_RedOn(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_SET);
}

void LED_RedOff(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);
}

void LED_RedToggle(void)
{
	HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_14);
}

void Button_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	__GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin=GPIO_PIN_0;
	GPIO_InitStruct.Mode=GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull=GPIO_PULLDOWN;
	GPIO_InitStruct.Speed=GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

uint32_t Button_GetState(void)
{
	return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
}
