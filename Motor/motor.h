#ifndef __MOTOR_H
#define __MOTOR_H
#include "main.h"

#define PWMXA TIM2->CCR1
#define PWMXB TIM2->CCR2
#define PWMYA TIM2->CCR3
#define PWMYB TIM2->CCR4

typedef struct
{
	float KP;			   // 比例系数
	float KI;			   // 积分系数
	float KD;			   // 微分系数
	float Encoder;		   // Read the real time speed of the motor by encoder //编码器数值，读取电机实时速度
	float Motor;		   // Motor PWM value, control the real-time speed of the motor //电机PWM数值，控制电机实时速度
	float Target;		   // Control the target speed of the motor //电机目标速度值，控制电机目标速度
	float prev_error;	   // 上一次的误差值
	float prev_prev_error; // 上上一次的误差值
	float prev_output;	   // 上一次的输出值
	float integral;		   // 积分项
} motor;

void Set_Pwm(motor *MotorX, motor *Motor_Y);
int Read_Encoder(int TIMX);
void MotorInit(void);
void Incremental_PID(motor *Motor);
float my_abs(float x);

extern float Velocity_KP, Velocity_KI;
extern float Motor_X, Motor_Y, Encoder_X, Encoder_Y, Target_X, Target_Y;
extern motor MotorX, MotorY;

#endif
