#ifndef __LED_H
#define __LED_H

#include "PublType.h"
#include "kd32f3xx.h"
#include "kd32f328xb.h"

/* Definimos el LED Rx (Verde) en GPIOA Pin 3 */
#define LED_RX_PORT GPIOA
#define LED_RX_PIN  GPIO_Pin_3

void LED_Init(void);
void LED_Toggle(void);

#endif /* __LED_H */
