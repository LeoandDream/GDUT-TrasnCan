#include "servo.h"

/**
 * @brief Convert angle in degrees to PWM value for 270 degree servo
 * @param angle Angle in degrees (0 to 270)
 * @return int PWM value
 */
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

/**
 * @brief Convert angle in degrees to PWM value for 180 degree servo
 * @param angle Angle in degrees (0 to 180)
 * @return int PWM value
 */
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

/**
 * @brief Set servo 1 to specified angle
 * @param degree Angle in degrees (0 to 180)
 */
void Servo1(int degree)
{
	TIM1->CCR1 = angle_to_pwm_180(degree);
}

/**
 * @brief Set servo 2 to specified angle
 * @param degree Angle in degrees (0 to 180)
 */
void Servo2(int degree)
{
	TIM1->CCR2 = angle_to_pwm_180(degree);
}

/**
 * @brief Set servo 3 to specified angle
 * @param degree Angle in degrees (0 to 180)
 */
void Servo3(int degree)
{
	TIM1->CCR3 = angle_to_pwm_180(degree);
}

/**
 * @brief Initialize servos by starting PWM on TIM1 channels
 */
void ServoInit()
{
	// 电机PWM开启
	TIM1->CCR1 = angle_to_pwm_180(60);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
}
