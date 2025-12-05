#include "servo.h"

int angle_to_pwm_270(float angle)
{
	// 限制角度在0到270度之间
	if (angle < 0.0f)
	{
		angle = 0.0f;
	}
	else if (angle > 270.0f)
	{
		angle = 270.0f;
	}

	// 线性转换公式：PWM = (angle * 1000 / 270) + 250
	float pwm = (angle * 1000.0f / 270.0f) + 250.0f;
	return (int)roundf(pwm); // 四舍五入到整数
}

int angle_to_pwm_180(float angle)
{

	if (angle < 0.0f)
	{
		angle = 0.0f;
	}
	else if (angle > 180.0f)
	{
		angle = 180.0f;
	}

	float pwm = (angle * 1000.0f / 180.0f) + 250.0f;
	return (int)roundf(pwm); // 四舍五入到整数
}

void test()
{
	// MotorX.Target = 10;
	// MotorY.Target = 10;
	PWMXB = 16800;
	PWMXA = 5000;
	HAL_Delay(700);
	PWMXB = 16800;
	PWMXA = 16800;

	HAL_Delay(500);
	
	PWMYB = 16800;
	PWMYA = 5000;
	HAL_Delay(700);
	PWMYB = 16800;
	PWMYA = 16800;

	// MotorX.Target = 0;
	// MotorY.Target = 0;

	TIM1->CCR2 = angle_to_pwm_180(180);
	TIM1->CCR1 = angle_to_pwm_180(60);
	HAL_Delay(1000);

	TIM1->CCR1 = angle_to_pwm_180(60);
	TIM1->CCR2 = angle_to_pwm_180(135);

	HAL_Delay(2000);

	TIM1->CCR1 = angle_to_pwm_180(0);
	HAL_Delay(1000);
	TIM1->CCR2 = angle_to_pwm_180(180);
	HAL_Delay(1000);
	
	PWMXA = 16800;
	PWMXB = 5000;
	HAL_Delay(1000);
	PWMXA = 16800;
	PWMXB = 16800;

	PWMYA = 16800;
	PWMYB = 5000;
	HAL_Delay(1000);
	PWMYA = 16800;
	PWMYB = 16800;
	
	TIM1->CCR1 = angle_to_pwm_180(60);

	// MotorX.Target = -10;
	// MotorY.Target = -10;
	// HAL_Delay(1000);
	// MotorX.Target = 0;
	// MotorY.Target = 0;
}

void ServoInit()
{
	// 电机PWM开启
	TIM1->CCR1 = angle_to_pwm_180(60);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
}
