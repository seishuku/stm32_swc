#include "stm32f4xx_hal.h"
#include "stm32f4xx_it.h"

extern CAN_HandleTypeDef CanHandle;
extern PCD_HandleTypeDef hpcd;
extern TIM_HandleTypeDef TimHandle;

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
//	while(1)
//	{
//	}
}

void MemManage_Handler(void)
{
//	while(1)
//	{
//	}
}

void BusFault_Handler(void)
{
//	while(1)
//	{
//	}
}

void UsageFault_Handler(void)
{
//	while(1)
//	{
//	}
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
	HAL_IncTick();
}

void CAN1_RX0_IRQHandler(void)
{
	HAL_CAN_IRQHandler(&CanHandle);
}

void CAN1_RX1_IRQHandler(void)
{
	HAL_CAN_IRQHandler(&CanHandle);
}

void CAN1_TX_IRQHandler(void)
{
	HAL_CAN_IRQHandler(&CanHandle);
}

void OTG_HS_IRQHandler(void)
{
	HAL_PCD_IRQHandler(&hpcd);
}
