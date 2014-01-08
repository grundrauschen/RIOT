/*
 * port.h
 *
 *  Created on: Jan 8, 2014
 *      Author: tobias
 */

#ifndef PORT_H_
#define PORT_H_

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void WWDG_IRQHandler(void);

#endif /* PORT_H_ */
