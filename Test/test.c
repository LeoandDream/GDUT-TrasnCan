#include "main.h"

void main_test()
{
    // MotorX.Target = 10;
    // MotorY.Target = 10;
    PWMXB = 16800;
    PWMXA = 5000;
    HAL_Delay(700);
    PWMXB = 16800;
    PWMXA = 16800;

    HAL_Delay(300);

    PWMYB = 16800;
    PWMYA = 5000;
    HAL_Delay(800);
    PWMYB = 16800;
    PWMYA = 16800;

    // MotorX.Target = 0;
    // MotorY.Target = 0;

    Servo1(60);
    Servo2(180);
    HAL_Delay(1000);

    Servo1(60);
    Servo2(150);

    HAL_Delay(2000);

    Servo1(0);
    HAL_Delay(1000);
    Servo2(180);
    HAL_Delay(1000);

    PWMYA = 16800;
    PWMYB = 5000;
    HAL_Delay(500);
    PWMYA = 16800;
    PWMYB = 16800;

    HAL_Delay(300);

    PWMXA = 16800;
    PWMXB = 5000;
    HAL_Delay(600);
    PWMXA = 16800;
    PWMXB = 16800;

    Servo1(60);
    printf("Servo1 to 60 degrees\n");
    // MotorX.Target = -10;
    // MotorY.Target = -10;
    // HAL_Delay(1000);
    // MotorX.Target = 0;
    // MotorY.Target = 0;
}

void usart1_test()
{
    int i0 = 0;
    uint8_t init_msg[] = "UART Init OK\r\n";
    HAL_UART_Transmit(&huart1, init_msg, sizeof(init_msg) - 1, 100);
    printf("this is printf \r\n");
    if (i0++ == 1000)
        i0 = 0;
    HAL_Delay(1000);
}

void motor_speed_loop_test()
{
    MotorX.Target = 10;
    MotorY.Target = 10;
    HAL_Delay(1000);
    MotorX.Target = 0;
    MotorY.Target = 0;

    MotorX.Target = -10;
    MotorY.Target = -10;
    HAL_Delay(1000);
    MotorX.Target = 0;
    MotorY.Target = 0;
}

void motor_openloop_test()
{
    PWMXA = 5000;
    PWMXB = 16800;
    PWMYA = 5000;
    PWMYB = 16800;
    HAL_Delay(2000);
    PWMXA = 16800;
    PWMXB = 16800;
    PWMYA = 16800;
    PWMYB = 16800;

    HAL_Delay(1000);

    PWMXB = 5000;
    PWMXA = 16800;
    PWMYB = 5000;
    PWMYA = 16800;
    HAL_Delay(2000);
    PWMXB = 16800;
    PWMXA = 16800;
    PWMYB = 16800;
    PWMYA = 16800;
}
