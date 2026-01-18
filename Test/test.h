#ifndef __TEST_H__
#define __TEST_H__

#include "main.h"

void usart1_Transmit_test(void);
void usart1_Receive_test(void);
void usart2_Transmit_test(void);
void usart2_Receive_test(void);
void main_test(void);
void motor_openloop_test(void);
void motor_speed_loop_test(void);
void motor_position_loop_test(void);

extern int i, i0, i1;
#endif
