#include "TM4C123GH6PM.h"

#define MSTICKS 12000

//Input and output ports definitions
#define PA7 (*(volatile uint32_t*) (GPIOA_BASE+0x0200))
#define PF0 (*(volatile uint32_t*)(GPIOF_BASE+0x0004))
#define PF4 (*(volatile uint32_t*)(GPIOF_BASE+0x0040))
#define inputs ((~(PF0 | (PF4 >> 3))) & 0x03)
#define outputs (*(volatile uint32_t*)(GPIOF_BASE+0x38)) //PF123

//State type-definitions
typedef struct state{
	uint32_t output[2];
	uint32_t wait; //Wait time in ms
	const struct state* next[4];
} State;

#define RED 1
#define BLUE 2
#define GREEN 4
#define RB 3
#define BG 5
#define RG 6

#define Happy  &(StatesAr[0]) //red
#define Hungry &(StatesAr[1]) //blue
#define Sleepy &(StatesAr[2]) //green

static const State StatesAr[3] = 
{
  {{RED,BLUE}, 750, {Hungry, Happy, Hungry, Sleepy}},
  {{GREEN, RB}, 600, {Hungry, Hungry, Happy, Sleepy}},
	{{RG,BG}, 3000, {Happy, Sleepy, Happy, Hungry}}
};

static void InitializePorts(void); //PF1,2,3 outputs. PF0,4 inputs
static void SysTickInit(void);
static void SysTickWait(unsigned delay); //delay is in 83.3 ns units
static void SysTickWaitMs(unsigned delay); //delay is in 1 ms units

int __main()
{
	const State* curS = Happy;
	InitializePorts();
	SysTickInit();
	
	while(1)
	{
		uint32_t debuginputs = inputs;
		outputs = (curS->output[inputs&0x01]) << 1; //Output
		SysTickWaitMs(curS->wait); //Systick wait
		curS = curS->next[inputs]; //Change state
	}
}


static void InitializePorts(void)
{
  volatile uint32_t* CR = (__IO uint32_t*) (&(GPIOF->CR));
	SYSCTL->RCGCGPIO |= 0x20;						//Enable clock on GPIO F
	while((SYSCTL->PRGPIO&0x20) == 0){}	//Wait until port is ready
	GPIOF->LOCK = 0x4C4F434B;						//Unlock GPIOF (unlocks GPIOCR register)
	*CR |= 0x1F;
	GPIOF->DIR = 0x0E;								//Clear bits 0 and 4 (make them input) 
	GPIOF->PUR |= 0x11;									//Enable pull-down resistor
	GPIOF->DEN |= 0x1F;
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
	while((SysTick->CTRL&0x00010000)==0){	//Wait for COUNT flag
		if(PA7) return;											//If PA7==1 before counter reaches 0, return
	}
}

static void SysTickWaitMs(unsigned delay)
{
	if(delay <= 80)
		SysTickWait(delay*MSTICKS);
	else
	{
		int i;
		for(i=0; i<delay; i++)
			SysTickWait(MSTICKS);
	}
}

