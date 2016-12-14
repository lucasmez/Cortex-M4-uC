/*Inputs are 1, 2, 3, 4 and set. On PA2, PA3, PA4, PF0 and PF4 respectively
/ Outputs are Red(locked), Green(unlocked) and Blue(setting). On PF1, PF3 and PF2 respectively
*/

#include "TM4C123GH6PM.h"

#define MSTICKS 12000		//# of ticks in 1ms with 12MHz clock

/********DEFINITIONS**********************************/

//Input and output definitions
#define LEDS (*(volatile uint32_t*)(GPIOF_BASE + 0x0038))
#define PA234 (*(volatile uint32_t*)(GPIOA_BASE + 0x70))
#define PF0 ((*(volatile uint32_t*)(GPIOF_BASE + 0x04)) ^ 0x01)
#define PF4 ((*(volatile uint32_t*)(GPIOF_BASE + 0x40)) ^ 0x10)
#define PA2 (*(volatile uint32_t*)(GPIOA_BASE + 0x10))
#define PA3 (*(volatile uint32_t*)(GPIOA_BASE + 0x20))
#define PA4 (*(volatile uint32_t*)(GPIOA_BASE + 0x40))
#define SET (PF4 >> 4)
#define INPUTS ((PA234 >> 2) | (PF0 << 3) | PF4) //input format: 000 SET 4 3 2 1

#define RED 0x02
#define GREEN 0x08
#define BLUE 0x04
#define NONE 0x00

//States names definitions

#define LOCKED (StatesAr)
#define CHECK2 (StatesAr+1)
#define WRONG (StatesAr+2)
#define UNLOCKED (StatesAr+3)
#define SETTING (StatesAr + 4)
#define WAIT1 (StatesAr + 5)
#define IN2 (StatesAr + 6)
#define WAIT2 (StatesAr + 7)

/********TYPE DEFINITIONS****************************************/
//FSM Type
typedef struct fsm{
	const struct fsm* (*fncPt)(void);
}State;
typedef const State* StatePt;


/*******FUNCTION PROTOTYPES***************************************/
//FSM functions
static StatePt lockedF(void);
static StatePt check2F(void);
static StatePt wrongF(void);
static StatePt unlockedF(void);
static StatePt settingF(void);
static StatePt wait1F(void);
static StatePt in2F(void);
static StatePt wait2F(void);

//Helper and initialization functions
static void InitializePorts(void);
static void SysTickInit(void);
static void SysTickWait(unsigned delay);
static void SysTickWaitMs(unsigned delay);

/**************GLOBAL VARIABLES*************************************/

static uint8_t Pass[2]; //Current password
static const State StatesAr[8] = //Initialize State Machine
{
	{lockedF},
	{check2F},
	{wrongF},
	{unlockedF},
	{settingF},
	{wait1F},
	{in2F},
	{wait2F}
};



int __main()
{
	StatePt curS = SETTING; //Set the passcode first
	InitializePorts();
	SysTickInit();
	while(1)
		curS = curS->fncPt();
}




static StatePt lockedF(void)
{
	StatePt ret = LOCKED;
	uint32_t input = INPUTS; //Get inputs
	LEDS = RED; //Set output
	//Next State logic
	if((input&0x10) == 0x10) //SET
		ret = SETTING;
	else if((input == 0x08)  || (input == 0x04) || (input == 0x02) || (input ==  0x01)) //If input is 1 OR 2 OR 3 OR 4
	{
		if(input == Pass[0]) //Check if first pass code is correct
			ret = CHECK2;
		else
			ret = WRONG;
	}
	SysTickWaitMs(2000); //Wait so current input (which is not a tick) is not carried into next state
	return ret;
}

static StatePt check2F(void)
{
	StatePt ret = CHECK2;
	uint32_t input = INPUTS;
	LEDS = RED;
	if((input == 0x08)  || (input == 0x04) || (input == 0x02) || (input ==  0x01)) //If input is 1 OR 2 OR 3 OR 4
	{
		if(input == Pass[1]) //Check if second pass code is correct
			ret = UNLOCKED;
		else
		{
			SysTickWaitMs(2000);
			ret = LOCKED;
		}
	}
	return ret;
}

static StatePt wrongF(void)
{
	StatePt ret = WRONG;
	uint32_t input = INPUTS;
	LEDS = RED;
	if((input == 0x08)  || (input == 0x04) || (input == 0x02) || (input ==  0x01)) //If input is 1 OR 2 OR 3 OR 4
		ret = LOCKED;
	SysTickWaitMs(2000);
	return ret;
}
	
	
	
static StatePt unlockedF(void)
{
	LEDS = GREEN;
	SysTickWaitMs(50000); //Stay in unlocked position for 5 seconds before locking again
	return LOCKED;
}

static StatePt settingF(void)
{
	StatePt ret = SETTING;
	uint32_t input = INPUTS;
	LEDS = BLUE;
	if((input == 0x08)  || (input == 0x04) || (input == 0x02) || (input ==  0x01)) //If input is 1 OR 2 OR 3 OR 4
	{
		Pass[0] = input;
		ret = WAIT1;
	}
	return ret;
}

static StatePt wait1F(void)
{
	LEDS = NONE;
	SysTickWaitMs(10000); //Wait for 1 seconds before changing second passcode digit
	return IN2;
}

static StatePt in2F(void)
{
	StatePt ret = IN2;
	uint32_t input = INPUTS;
	LEDS = BLUE;
	if((input == 0x08)  || (input == 0x04) || (input == 0x02) || (input ==  0x01)) //If input is 1 OR 2 OR 3 OR 4
	{
		Pass[1] = input;
		ret = WAIT2;
	}
	return ret;
}

static StatePt wait2F(void)
{
	LEDS = NONE;
	SysTickWaitMs(10000); //Wait for 1 seconds before going back to locked position
	return LOCKED;
}

static void InitializePorts(void)
{
  volatile uint32_t* CR = (__IO uint32_t*) (&(GPIOF->CR));
	SYSCTL->RCGCGPIO |= 0x21;						//Enable clock on GPIO F and GPIO A
	while((SYSCTL->PRGPIO&0x20) == 0){}	//Wait until port is ready
	GPIOF->LOCK = 0x4C4F434B;						//Unlock GPIOF (unlocks GPIOCR register)
	*CR |= 0x1F;
	GPIOF->DIR = 0x0E;								//Clear bits 0 and 4 (make them input) 
	GPIOF->PUR |= 0x11;									//Enable pull-down resistor
	GPIOF->DEN |= 0x1F;	
		
	GPIOA->DIR &= ~0x1C;								//PA2,3,4 are inputs
	GPIOA->AFSEL = 0;
	GPIOA->AMSEL = 0;
	GPIOA->PCTL = 0;
	GPIOA->DEN = 0x1C;
}

static void SysTickInit(void)
{
	SysTick->CTRL = 0;				//Disable, clear count
	SysTick->LOAD = 0xFFFFFF;	//Load max value (23 bits)
	SysTick->CTRL = 5;				//Enable Systick, select core clock
}

static void SysTickWait(unsigned delay)
{
	SysTick->LOAD = delay;									
	SysTick->VAL = 0;												//Clear current val
	while((SysTick->CTRL&0x00010000)==0){}	//Wait for COUNT flag
}

static void SysTickWaitMs(unsigned delay)
{
	if(delay==0) return;
	if(delay <= 80)
		SysTickWait(delay*MSTICKS);
	else
	{
		int i;
		for(i=0; i<delay; i++)
			SysTickWait(MSTICKS);
	}
}
