#include "motor.h"

motor MotorX, MotorY;
int i, x1, y1, x2, y2;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM6) // 200hz定时中断
	{
		/* 读取编码器（增量） */
		MotorX.Encoder = Read_Encoder(3); // 读取编码器增量
		MotorY.Encoder = Read_Encoder(4);
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == 0)
			x1++;
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == 0)
			y1++;
		if (x1 > 1000)
			x1 = 0;
		if (y1 > 1000)
			y1 = 0;

		if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_12) == 0)
			x2++;
		if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_13) == 0)
			y2++;
		if (x2 > 1000)
			x2 = 0;
		if (y2 > 1000)
			y2 = 0;
		/* 累加位置（使用编码器增量累加） */
		MotorX.Position += MotorX.Encoder;
		MotorY.Position += MotorY.Encoder;

		/* 默认保留速度单环调用：如果需要位置-速度双环，可先在此处计算外环
		   然后将 mode 改为 MODE_POSITION_DOUBLE 或调用 Incremental_PID 时传入不同模式 */
		// Incremental_PID(&MotorX, MODE_SPEED);
		// Incremental_PID(&MotorY, MODE_SPEED);
		Incremental_PID(&MotorX, MODE_POSITION_SINGLE);
		Incremental_PID(&MotorY, MODE_POSITION_SINGLE);
		// Incremental_PID(&MotorX, MODE_POSITION_DOUBLE);
		// Incremental_PID(&MotorY, MODE_POSITION_DOUBLE);
		/* 更新 PWM 输出 */
		Set_Pwm(&MotorX, &MotorY);
		if (i++ == 10000)
		{
			i = 0;
		}
	}
}

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

	MotorX.Position = 0;
	MotorX.Pos_KP = 0.0f;
	MotorX.Pos_KI = 0.0f;
	MotorX.Pos_KD = 0.0f;
	MotorX.pos_prev_error = 0;
	MotorX.pos_integral = 0;
	MotorX.MaxSpeed = 1000.0f;

	MotorY.Position = 0;
	MotorY.Pos_KP = 0.0f;
	MotorY.Pos_KI = 0.0f;
	MotorY.Pos_KD = 0.0f;
	MotorY.pos_prev_error = 0;
	MotorY.pos_integral = 0;
	MotorY.MaxSpeed = 1000.0f;
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
	if (Motor_X->Motor > 0)
	{
		PWMXA = 15000;
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

/**
 * @brief Incremental PID control function
 *		单一 PID 函数：依据 mode 做位置外环（若双环），再执行速度增量 PID
 * @param _Motor Pointer to the motor structure
 * @param mode Control mode
   - MODE_POSITION_SINGLE: 位置 PID 直接输出到 PWM
   - MODE_POSITION_DOUBLE: 先位置 PID 生成 Target_V，再走速度增量 PID
   - MODE_SPEED / MODE_SPEED_SINGLE: 仅速度增量 PID
 */
void Incremental_PID(motor *_Motor, ControlMode mode)
{
	const float output_limit = 16800.0f;

	/* 位置单环：直接用位置 PID 输出 PWM */
	if (mode == MODE_POSITION_SINGLE)
	{
		float pos_err = _Motor->Target_P - _Motor->Position;
		_Motor->pos_integral += pos_err;
		float pos_deriv = pos_err - _Motor->pos_prev_error;
		float out = _Motor->Pos_KP * pos_err + _Motor->Pos_KI * _Motor->pos_integral + _Motor->Pos_KD * pos_deriv;
		if (out > output_limit)
		{
			out = output_limit;
			_Motor->pos_integral -= pos_err; /* anti-windup */
		}
		if (out < -output_limit)
		{
			out = -output_limit;
			_Motor->pos_integral -= pos_err;
		}
		_Motor->pos_prev_error = pos_err;
		_Motor->Motor = out;
		return;
	}

	/* 若为双环位置控制，先计算位置环输出速度命令并限幅 */
	if (mode == MODE_POSITION_DOUBLE)
	{
		float pos_err = _Motor->Target_P - _Motor->Position;
		_Motor->pos_integral += pos_err;
		float pos_deriv = pos_err - _Motor->pos_prev_error;
		float v_cmd = _Motor->Pos_KP * pos_err + _Motor->Pos_KI * _Motor->pos_integral + _Motor->Pos_KD * pos_deriv;
		if (v_cmd > _Motor->MaxSpeed)
		{
			v_cmd = _Motor->MaxSpeed;
			_Motor->pos_integral -= pos_err; /* anti-windup */
		}
		if (v_cmd < -_Motor->MaxSpeed)
		{
			v_cmd = -_Motor->MaxSpeed;
			_Motor->pos_integral -= pos_err;
		}
		_Motor->pos_prev_error = pos_err;
		_Motor->Target_V = v_cmd; /* 外环把速度指令下发给内环 */
	}

	/* 速度增量 PID（适用于 MODE_SPEED 与 MODE_POSITION_DOUBLE） */
	float current_error = _Motor->Target_V - _Motor->Encoder;
	float delta_output =
		_Motor->KP * (current_error - _Motor->prev_error) + _Motor->KI * current_error + _Motor->KD * (current_error - 2 * _Motor->prev_error + _Motor->prev_prev_error);
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
		TIM3->CNT = 0;
		break;
	case 4:
		Encoder_TIM = (short)TIM4->CNT;
		TIM4->CNT = 0;
		break;
		// default:
		// 	Encoder_TIM = 0;
	}
	return Encoder_TIM;
}
