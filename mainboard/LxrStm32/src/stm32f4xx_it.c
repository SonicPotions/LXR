
#include "stm32f4xx_it.h"
#include "globals.h"
#include "sequencer.h"

#include "usb_core.h"
#include "usbd_core.h"
#include "usb_midi_core.h"

//asm(".extern hard_fault_handler_c");


extern USB_OTG_CORE_HANDLE           USB_OTG_dev;
extern uint32_t USBD_OTG_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);


// time since system start
volatile uint32_t  systick_ticks=0;

//exception handlers


void NMI_Handler(void)
{
}

void MemManage_Handler(void)
{

  while (1)
  {
  }
}


void BusFault_Handler(void)
{

  while (1)
  {
  }
}

void UsageFault_Handler(void)
{

  while (1)
  {
  }
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
	  systick_ticks++;

}



#ifdef USE_USB_OTG_FS
void OTG_FS_WKUP_IRQHandler(void)
{
  if(USB_OTG_dev.cfg.low_power)
  {
    *(uint32_t *)(0xE000ED10) &= 0xFFFFFFF9 ;
    SystemInit();
    USB_OTG_UngateClock(&USB_OTG_dev);
  }
  EXTI_ClearITPendingBit(EXTI_Line18);
}
#endif

#ifdef USE_USB_OTG_HS
void OTG_HS_IRQHandler(void)
#else
void OTG_FS_IRQHandler(void)
#endif /* USE_USB_OTG_HS */
{
  USBD_OTG_ISR_Handler (&USB_OTG_dev);
}

#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED

void OTG_HS_EP1_IN_IRQHandler(void)
{
  USBD_OTG_EP1IN_ISR_Handler (&USB_OTG_dev);
}


void OTG_HS_EP1_OUT_IRQHandler(void)
{
  USBD_OTG_EP1OUT_ISR_Handler (&USB_OTG_dev);
}
#endif /* USB_OTG_HS_DEDICATED_EP1_ENABLED */



