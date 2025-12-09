#ifndef __SERVO_H
#define __SERVO_H

#include "main.h"

#define laser_H  HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_SET)
#define laser_L  HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_RESET)

void ServoInit(void);
int angle_to_pwm_270(float angle);
int angle_to_pwm_180(float angle);
void test(void);
void Servo1(int degree);
void Servo2(int degree);
void Servo3(int degree);

#endif
