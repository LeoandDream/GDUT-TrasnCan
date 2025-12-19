#include "motor.h"

motor MotorX, MotorY;
int i, x1, y1, x2, y2;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM6) // 200hz定时中断
	{
				/* 读取编码器（增量） */
		MotorX.Encoder = -Read_Encoder(3)*360.0*0.4375/(13*30); // 读取编码器增量 参数为特调
		MotorY.Encoder = Read_Encoder(4)*360.0*0.3987/(13*30);
		/* 累加位置（使用编码器增量累加） */
		MotorX.Position += MotorX.Encoder;
		MotorY.Position += MotorY.Encoder;
		//printf("%f,%f,%f\r\n",MotorX.Position,MotorX.Target_P,MotorX.Motor);

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

	MotorX.KP = 100.0; //800.0;
	MotorX.KI = 1.0; //10.0; // 增量式PI的KI应比位置式小
	MotorX.KD = 0.0;  // 初始微分系数设为0

	MotorY.KP = 100.0;
	MotorY.KI = 1.0;
	MotorY.KD = 0.0;

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

	MotorX.Position = 71;  // 摄像头x初始位置设为71
	MotorX.Pos_KP = 100.0f;
	MotorX.Pos_KI = 0.1f;
	MotorX.Pos_KD = 0.0f;
	MotorX.pos_prev_error = 0;
	MotorX.pos_integral = 0;
	MotorX.MaxSpeed = 3000.0f; //单位置环时需要调的值为output_limit

	MotorY.Position = 0;
	MotorY.Pos_KP = 100.0f;
	MotorY.Pos_KI = 0.1f;
	MotorY.Pos_KD = 0.0f;
	MotorY.pos_prev_error = 0;
	MotorY.pos_integral = 0;
	MotorY.MaxSpeed = 3000.0f;
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
	if (Motor_X->Motor > 0)//反向旋转，迎合坐标系方向
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
void Incremental_PID(motor *_Motor)
{
	const float output_limit = 15000.0f;
	/* 位置单环：直接用位置 PID 输出 PWM */
	if (_Motor->mode == MODE_POSITION_SINGLE)
	{
		float encoder_position_target = _Motor->Target_P * 13 * 30 / 360;
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
	if (_Motor->mode == MODE_POSITION_DOUBLE)
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

/**************************************************************************
功能：计算转数对应编码器脉冲数  （保证pid输入参数为脉冲，应用不同电机时不用再调pid参数）
输入：num：转数；ppr：码盘数；ratio:减速比
返回值：电机脉冲数
**************************************************************************/
long Num_Encoder_Cnt(float num,uint16_t ppr,float ratio)
{
    return (num*ratio*ppr);                               /*单倍频 */       
}

/**************************************************************************
功能：计算转速对应编码器脉冲数
输入：encoder_cnt：脉冲数；ppr：码盘数；ratio:减速比；cnt_time：计数时间（ms）
返回值：电机脉冲数
**************************************************************************/
long Rpm_Encoder_Cnt(float rpm,uint16_t ppr,uint16_t ratio,uint16_t cnt_time)
{
	return (rpm*ratio*ppr)/(60*1000/cnt_time);            /*单倍频 */     
}