#include "motor.h"

motor MotorX, MotorY;
int i;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM6) // 200ms定时中断
	{

		MotorX.Encoder = Read_Encoder(3); //===读取编码器的值							 //为了保证M法测速的时间基准，首先读取编码器数据
		MotorY.Encoder = Read_Encoder(4); //===读取编码器的值	                                 //差速小车和履带小车运动学分析
										  //		Incremental_PID(&MotorX);		  //===速度闭环控制计算电机A最终PWM  采用位置式
										  //		Incremental_PID(&MotorY);		  //===PWM限幅
										  //		Set_Pwm(&MotorX, &MotorY);
	}
}

void MotorInit()
{
	//	HAL_TIM_Base_Start_IT(&htim6);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
	HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);

	MotorX.KP = 800.0;
	MotorX.KI = 10.0; // 增量式PI的KI应比位置式小
	MotorX.KD = 0.0;  // 初始微分系数设为0

	MotorY.KP = 800.0;
	MotorY.KI = 10.0;
	MotorY.KD = 0.0;

	// 新增成员初始化
	MotorX.prev_error = 0;
	MotorX.prev_prev_error = 0;
	MotorX.prev_output = 0;

	MotorY.prev_error = 0;
	MotorY.prev_prev_error = 0;
	MotorY.prev_output = 0;
}

float my_abs(float x)
{
	return x > 0 ? x : (-1 * x);
}

void Set_Pwm(motor *Motor_X, motor *Motor_Y)
{

	if (Motor_X->Motor > 0)
	{
		PWMXA = 16800;
		PWMXB = (uint32_t)(16800 - my_abs(Motor_X->Motor));
	}
	else
	{
		PWMXA = (uint32_t)(16800 - my_abs(Motor_X->Motor));
		PWMXB = 16800;
	}

	if (Motor_Y->Motor > 0)
	{
		PWMYA = 16800;
		PWMYB = (uint32_t)(16800 - my_abs(Motor_Y->Motor));
	}
	else
	{
		PWMYA = (uint32_t)(16800 - my_abs(Motor_Y->Motor));
		PWMYB = 16800;
	}
}

void Incremental_PID(motor *_Motor)
{
	const float output_limit = 16800.0f; // 输出限幅值

	float current_error = _Motor->Target - _Motor->Encoder;

	float delta_output =
		_Motor->KP * (current_error - _Motor->prev_error)								   // 比例项
		+ _Motor->KI * current_error													   // 积分项
		+ _Motor->KD * (current_error - 2 * _Motor->prev_error + _Motor->prev_prev_error); // 微分项

	float new_output = _Motor->prev_output + delta_output;

	if (new_output > output_limit)
		new_output = output_limit;
	if (new_output < -output_limit)
		new_output = -output_limit;

	_Motor->prev_prev_error = _Motor->prev_error; // 更新上上次误差
	_Motor->prev_error = current_error;			  // 更新上次误差
	_Motor->prev_output = new_output;			  // 更新上次输出
	_Motor->Motor = new_output;
}

int Read_Encoder(int TIMX)
{
	int Encoder_TIM;
	switch (TIMX)
	{
	case 3:
		Encoder_TIM = (short)TIM3->CNT;
		TIM3->CNT = 0;
		break;
	case 4:
		Encoder_TIM = (short)TIM4->CNT;
		TIM4->CNT = 0;
		break;
	default:
		Encoder_TIM = 0;
	}
	return Encoder_TIM;
}
