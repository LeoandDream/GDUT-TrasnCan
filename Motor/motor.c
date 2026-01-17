#include "motor.h"

motor MotorX, MotorY;


/**
 * @brief Initialize the motor control peripherals and parameters
 *
 */
void MotorInit()
{
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
	HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);

	MotorX.Vel_KP = 800.0;
	MotorX.Vel_KI = 10.0;
	MotorX.Vel_KD = 0.0;

	MotorY.Vel_KP = 800.0;
	MotorY.Vel_KI = 10.0;
	MotorY.Vel_KD = 0.0;
	// 新增成员初始化
	MotorX.prev_error = 0;
	MotorX.prev_prev_error = 0;
	MotorX.prev_output = 0;

	MotorY.prev_error = 0;
	MotorY.prev_prev_error = 0;
	MotorY.prev_output = 0;

	MotorX.Position = 218; // 摄像头x初始位置设为71 120
	MotorX.Pos_KP = 180.0f;
	MotorX.Pos_KI = 0.8f;
	MotorX.Pos_KD = 0.0f;
	MotorX.pos_prev_error = 0;
	MotorX.pos_integral = 0;
	MotorX.MaxSpeed = 6000.0f; // 单位置环时需要调的值为output_limit

	MotorY.Position = 120; //145
	MotorY.Pos_KP = 750.0f;
	MotorY.Pos_KI = 5.0f;
	MotorY.Pos_KD = 0.0f;
	MotorY.pos_prev_error = 0;
	MotorY.pos_integral = 0;
	MotorY.MaxSpeed = 6000.0f;
}

/**
 * @brief absolute value function
 *
 * @param x
 * @return float
 */
float my_abs(float x)
{
	return x > 0 ? x : (-1 * x);
}

/**
 * @brief Set the Pwm object
 *
 * @param Motor_X
 * @param Motor_Y
 */
void Set_Pwm(motor *Motor_X, motor *Motor_Y)
{
	if (Motor_X->Motor > 0) // 反向旋转，迎合坐标系方向
	{
		PWMXA = (uint32_t)(16800 - my_abs(Motor_X->Motor));
		PWMXB = 16800;
	}
	else
	{
		PWMXA = 16800;
		PWMXB = (uint32_t)(16800 - my_abs(Motor_X->Motor));
	}

	if (Motor_Y->Motor > 0)
	{
		PWMYB = 16800;
		PWMYA = (uint32_t)(16800 - my_abs(Motor_Y->Motor));
	}
	else
	{
		PWMYB = (uint32_t)(16800 - my_abs(Motor_Y->Motor));
		PWMYA = 16800;
	}
}

/**
 * @brief Incremental PID control function
 *		单一 PID 函数：依据 mode 做位置外环（若双环），再执行速度增量 PID
 * @param _Motor Pointer to the motor structure
 * @param mode Control mode
   - MODE_POSITION_SINGLE: 位置 PID 直接输出到 PWM
   - MODE_POSITION_DOUBLE: 先位置 PID 生成 Target_V，再走速度增量 PID
   - MODE_SPEED / MODE_SPEED_SINGLE: 仅速度增量 PID
 */
void Incremental_PID(motor *_Motor)
{
	const float output_limit = 8000.0f;
	/* 位置单环：直接用位置 PID 输出 PWM */
	if (_Motor->mode == MODE_POSITION_SINGLE)
	{
		float encoder_position_target = _Motor->Target_P * 13 * 30 / 360;
		_Motor->pos_error = _Motor->Target_P - _Motor->Position;
		_Motor->pos_integral += _Motor->pos_error;
		float pos_deriv = _Motor->pos_error - _Motor->pos_prev_error;
		float out = _Motor->Pos_KP * _Motor->pos_error + _Motor->Pos_KI * _Motor->pos_integral + _Motor->Pos_KD * pos_deriv;
		if (out > output_limit)
		{
			out = output_limit;
			_Motor->pos_integral -= _Motor->pos_error; /* anti-windup */
		}
		if (out < -output_limit)
		{
			out = -output_limit;
			_Motor->pos_integral -= _Motor->pos_error;
		}
		_Motor->pos_prev_error = _Motor->pos_error;
		_Motor->Motor = out;
		return;
	}

	/* 若为双环位置控制，先计算位置环输出速度命令并限幅 */
	if (_Motor->mode == MODE_POSITION_DOUBLE)
	{
		_Motor->pos_error = _Motor->Target_P - _Motor->Position;
		_Motor->pos_integral += _Motor->pos_error;
		float pos_deriv = _Motor->pos_error - _Motor->pos_prev_error;
		float v_cmd = _Motor->Pos_KP * _Motor->pos_error + _Motor->Pos_KI * _Motor->pos_integral + _Motor->Pos_KD * pos_deriv;
		if (v_cmd > _Motor->MaxSpeed)
		{
			v_cmd = _Motor->MaxSpeed;
			_Motor->pos_integral -= _Motor->pos_error; /* anti-windup */
		}
		if (v_cmd < -_Motor->MaxSpeed)
		{
			v_cmd = -_Motor->MaxSpeed;
			_Motor->pos_integral -= _Motor->pos_error;
		}
		_Motor->pos_prev_error = _Motor->pos_error;
		_Motor->Target_V = v_cmd; /* 外环把速度指令下发给内环 */
	}

	/* 速度增量 PID（适用于 MODE_SPEED 与 MODE_POSITION_DOUBLE） */
	float current_error = _Motor->Target_V - _Motor->Encoder;
	float delta_output =
		_Motor->Vel_KP * (current_error - _Motor->prev_error) + _Motor->Vel_KI * current_error + _Motor->Vel_KD * (current_error - 2 * _Motor->prev_error + _Motor->prev_prev_error);
	float new_output = _Motor->prev_output + delta_output;
	if (new_output > output_limit)
		new_output = output_limit;
	if (new_output < -output_limit)
		new_output = -output_limit;
	_Motor->prev_prev_error = _Motor->prev_error;
	_Motor->prev_error = current_error;
	_Motor->prev_output = new_output;
	_Motor->Motor = new_output;
}

/**
 * @brief Read the encoder value
 *
 * @param TIMX Timer number
 * @return int Encoder value
 */
int Read_Encoder(int TIMX)
{
	int Encoder_TIM;
	switch (TIMX)
	{
	case 3:
		Encoder_TIM = (short)TIM3->CNT;
		Encoder_TIM *= -1; // 根据接线方向调整正负
		TIM3->CNT = 0;
		break;
	case 4:
		Encoder_TIM = (short)TIM4->CNT;
		Encoder_TIM *= -1; // 根据接线方向调整正负
		TIM4->CNT = 0;
		break;
	default:
		Encoder_TIM = 0;
	}
	return Encoder_TIM;
}
