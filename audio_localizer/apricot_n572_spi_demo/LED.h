#ifndef __LED_H
#define __LED_H

#include <stdio.h>
#include <string.h>
#include "Platform.h"
#include "Driver/DrvGPIO.h"
#include "Driver/DrvSYS.h"
#include "Driver/DrvCLK.h"
#include "Driver/DrvSPI.h"
#include "Driver/DrvTimer.h"

void LED_turn_off(void);
void LED_set_low(void);
void LED_turn_on(void);
void LED_loop(void);
void LED_blink_for_half_second(void);

#endif // __LED_H
