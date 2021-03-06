/**
  ******************************************************************************
  * File Name          : FunctionTask.c
  * Description        : 用于记录机器人独有的功能
  ******************************************************************************
  *
  * Copyright (c) 2019 Team Jiao Long-Shanghai Jiao Tong University
  * All rights reserved.
  *
  ******************************************************************************
  */
#include "includes.h"
int16_t hookmode = 0; //store the state of the hook
int16_t hookmode_now = 2;
float rotate_speed = 0;
KeyboardMode_e KeyboardMode = NO_CHANGE;
RampGen_t LRSpeedRamp = RAMP_GEN_DAFAULT;
RampGen_t FBSpeedRamp = RAMP_GEN_DAFAULT;
ChassisSpeed_Ref_t ChassisSpeedRef;

int32_t auto_counter = 0; //自动倒数变量，这个变量每毫秒自减1，可用于依赖时间的自动化任务
						  //参赛队伍也可以在ControlTask.c的158行处找到自减的原理，并由此配置其他的时间变量
int16_t channelrrow = 0;  //这四个参数用于存储遥控器传回的拨动量
int16_t channelrcol = 0;  //向上向右为正，最大值（即拨到底）在600左右
int16_t channellrow = 0;
int16_t channellcol = 0;

//初始化
void FunctionTaskInit()
{
	LRSpeedRamp.SetScale(&LRSpeedRamp, MOUSE_LR_RAMP_TICK_COUNT);
	FBSpeedRamp.SetScale(&FBSpeedRamp, MOUSR_FB_RAMP_TICK_COUNT);
	LRSpeedRamp.ResetCounter(&LRSpeedRamp);
	FBSpeedRamp.ResetCounter(&FBSpeedRamp);

	ChassisSpeedRef.forward_back_ref = 0.0f;
	ChassisSpeedRef.left_right_ref = 0.0f;
	ChassisSpeedRef.rotate_ref = 0.0f;

	KeyboardMode = NO_CHANGE;
}

void Limit_and_Synchronization() //这个函数用来硬性限制电机角度，防止电机转到不合适的位置
{
	//demo
	//	MINMAX(AnyMotor.TargetAngle,-900,270);//limit
	//demo end
}

//******************
//遥控器模式功能编写
//******************
void RemoteControlProcess(Remote *rc)
{
	if (WorkState <= 0)
		return;
	//max=297
	channelrrow = (rc->ch0 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET);
	channelrcol = (rc->ch1 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET);
	channellrow = (rc->ch2 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET);
	channellcol = (rc->ch3 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET);
	////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////主要功能在这里编写

	if (WorkState == NORMAL_STATE||WorkState == ADDITIONAL_STATE_ONE) //钩子为正，可以将左边上下写成刷子（可调速和方向）
	{
		ChassisSpeedRef.forward_back_ref = channelrcol * RC_CHASSIS_SPEED_REF*1.5f;   //-           //这里已经默认写好了底盘的控制函数
		ChassisSpeedRef.left_right_ref = -channelrrow * RC_CHASSIS_SPEED_REF *3/4; //-           //右边摇杆控制前后左右的平移 左边摇杆控制旋转
		rotate_speed = -channellrow * RC_ROTATE_SPEED_REF*1.5f;						  //RC_CHASSIS_SPEED_REF是一个默认的数值，用来让行进速度达到合理值
		
		
		//左边上下为刷子
		//angle正为刷球入库
		M2006.TargetAngle += channellcol * 0.05;
		if (channellcol==0) M2006.RealAngle=  M2006.TargetAngle;//锁止2006
		
		
		//左边上下为钩子微调
		HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);								  //这个函数用于生成PWM波
		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1500);	//ChassisSpeedRef.forward_back_ref是一个封装好的变量，通过改变它
		
		
		//one push for 钩子
		/*
		if (rc->dial > 1100) //向下
		{
			
			__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 2500);
		}
		if (rc->dial < 900) //向上
		{
			__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1800 );
		}
		*/
		//可以直接控制车的前进后退速度，其余两个同理
		//如果想要提高车速，只需要在等号右侧乘上一个系数即可
		//当然速度的提高是有上限的，还请根据需要自行调节

		//demo
		//	M2006.TargetAngle += channellcol * 0.05;                                             //对于2006，经验上这样的转速是适中的，可以自行在这个基础上调节

		//demo end
	}
	if (WorkState == ADDITIONAL_STATE_TWO) //扔球模式，拨轮控制电机，方法同１                                                   //下档
	{
		M2006.RealAngle=M2006.TargetAngle;//锁止2006
		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1500+channellcol);
		/*
		HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3); //这个函数用于生成PWM波
		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 1500);
		HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4); //这个函数用于生成PWM波
		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 1500);
		*/
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	Limit_and_Synchronization();
}

uint16_t KM_FORWORD_BACK_SPEED = NORMAL_FORWARD_BACK_SPEED;
uint16_t KM_LEFT_RIGHT_SPEED = NORMAL_LEFT_RIGHT_SPEED;
void KeyboardModeFSM(Key *key);

//****************
//键鼠模式功能编写  校内赛中不会用到键鼠
//****************
void MouseKeyControlProcess(Mouse *mouse, Key *key)
{
	if (WorkState <= 0)
		return;

	MINMAX(mouse->x, -150, 150);
	MINMAX(mouse->y, -150, 150);

	KeyboardModeFSM(key);

	switch (KeyboardMode)
	{
	case SHIFT_CTRL: //State control
	{

		break;
	}
	case CTRL: //slow
	{

	}			//DO NOT NEED TO BREAK
	case SHIFT: //quick
	{

	}						//DO NOT NEED TO BREAK
	case NO_CHANGE:			//normal
	{						//CM Movement Process
		if (key->v & KEY_W) //key: w
			ChassisSpeedRef.forward_back_ref = KM_FORWORD_BACK_SPEED * FBSpeedRamp.Calc(&FBSpeedRamp);
		else if (key->v & KEY_S) //key: s
			ChassisSpeedRef.forward_back_ref = -KM_FORWORD_BACK_SPEED * FBSpeedRamp.Calc(&FBSpeedRamp);
		else
		{
			ChassisSpeedRef.forward_back_ref = 0;
			FBSpeedRamp.ResetCounter(&FBSpeedRamp);
		}
		if (key->v & KEY_D) //key: d
			ChassisSpeedRef.left_right_ref = KM_LEFT_RIGHT_SPEED * LRSpeedRamp.Calc(&LRSpeedRamp);
		else if (key->v & KEY_A) //key: a
			ChassisSpeedRef.left_right_ref = -KM_LEFT_RIGHT_SPEED * LRSpeedRamp.Calc(&LRSpeedRamp);
		else
		{
			ChassisSpeedRef.left_right_ref = 0;
			LRSpeedRamp.ResetCounter(&LRSpeedRamp);
		}
	}
	}
	Limit_and_Synchronization();
}

void KeyboardModeFSM(Key *key)
{
	if ((key->v & 0x30) == 0x30) //Shift_Ctrl
	{
		KM_FORWORD_BACK_SPEED = LOW_FORWARD_BACK_SPEED;
		KM_LEFT_RIGHT_SPEED = LOW_LEFT_RIGHT_SPEED;
		KeyboardMode = SHIFT_CTRL;
	}
	else if (key->v & KEY_SHIFT) //Shift
	{
		KM_FORWORD_BACK_SPEED = HIGH_FORWARD_BACK_SPEED;
		KM_LEFT_RIGHT_SPEED = HIGH_LEFT_RIGHT_SPEED;
		KeyboardMode = SHIFT;
	}
	else if (key->v & KEY_CTRL) //Ctrl
	{
		KM_FORWORD_BACK_SPEED = LOW_FORWARD_BACK_SPEED;
		KM_LEFT_RIGHT_SPEED = LOW_LEFT_RIGHT_SPEED;
		KeyboardMode = CTRL;
	}
	else
	{
		KM_FORWORD_BACK_SPEED = NORMAL_FORWARD_BACK_SPEED;
		KM_LEFT_RIGHT_SPEED = NORMAL_LEFT_RIGHT_SPEED;
		KeyboardMode = NO_CHANGE;
	}
}
