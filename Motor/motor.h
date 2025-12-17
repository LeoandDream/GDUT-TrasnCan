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
	float Target_V;		   // Control the target speed of the motor //电机目标速度值，控制电机目标速度
	float Target_P;		   // 目标位置
	float Position;		   // 累计位置（编码器计数累加）
	float Pos_KP;		   // 位置环比例
	float Pos_KI;		   // 位置环积分
	float Pos_KD;		   // 位置环微分
	float pos_prev_error;  // 位置环上一次误差
	float pos_integral;	   // 位置环积分值
	float MaxSpeed;		   // 位置环输出到速度环的最大速度限幅
	float prev_error;	   // 上一次的误差值
	float prev_prev_error; // 上上一次的误差值
	float prev_output;	   // 上一次的输出值
	float integral;		   // 积分项
} motor;

typedef enum
{
	MODE_SPEED = 0,
	MODE_SPEED_SINGLE,
	MODE_POSITION_SINGLE, // 位置单环（位置直接输出到PWM）——保留
	MODE_POSITION_DOUBLE  // 位置-速度双环（推荐）
} ControlMode;

void Set_Pwm(motor *MotorX, motor *Motor_Y);
int Read_Encoder(int TIMX);
void MotorInit(void);
void Incremental_PID(motor *Motor, ControlMode mode);
float my_abs(float x);
long Num_Encoder_Cnt(float num,uint16_t ppr,float ratio);
long Rpm_Encoder_Cnt(float rpm,uint16_t ppr,uint16_t ratio,uint16_t cnt_time);

extern float Velocity_KP, Velocity_KI;
extern float Motor_X, Motor_Y, Encoder_X, Encoder_Y, Target_X, Target_Y;
extern motor MotorX, MotorY;

#endif
