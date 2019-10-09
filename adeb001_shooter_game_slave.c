/*
 * adeb001_shooter_game_slave.c
 * 
 * I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 * 
 * Author : Anthony De Belen
 */ 

#include <avr/io.h>
#include "usart_ATmega1284.h"
#include <avr/interrupt.h>
#include "io2.c"


//--------Find GCD function --------------------------------------------------
unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
}
//--------End find GCD function ----------------------------------------------

//--------Task scheduler data structure---------------------------------------
// Struct for Tasks represent a running process in our simple real-time operating system.
typedef struct _task {
	/*Tasks should have members that include: state, period,
		a measurement of elapsed time, and a function pointer.*/
	signed char state; //Task's current state
	unsigned long int period; //Task period
	unsigned long int elapsedTime; //Time elapsed since last task tick
	int (*TickFct)(int); //Task tick function
} task;

//--------End Task scheduler data structure-----------------------------------

volatile unsigned char TimerFlag = 0;
unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;


void TimerOn()
{
	TCCR1B = 0x0B;
	OCR1A = 125;
	TIMSK1 = 0x02;
	TCNT1 = 0;
	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80;
}

void TimerOff()
{
	TCCR1B = 0x00;
}

void TimerISR()
{
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect)
{
	_avr_timer_cntcurr--;
	if(_avr_timer_cntcurr == 0)
	{
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M)
{
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

unsigned char data1 = 0;
unsigned char data2 = 0;
unsigned char tens = 0;
enum States {Init} state;
char* string;
int LCD_Tick();

void getString()
{
	switch(data1)
	{
		case 0: {string = "Shooter Game... Booting Sensors"; break;};
		case 1: {string = "Total Hits: "; break;};
		case 2: {string = "Out of Time!    Total Hits: "; break;};
		default: break;
	}
}

void getTens()
{
	unsigned int tmpA = 0;
	tmpA = data2 / 10;
	switch (tmpA)
	{
		case 1: {tens = 1; break;};
		case 2: {tens = 2; break;};
		case 3: {tens = 3; break;};
		default: break;
	}
}

int LCD_Tick(int state)
{
	switch(state)		//state transitions
	{
		case(Init):
		{
			break;
		}
		default:
		state = Init;
	}//end of transitions

	switch(state)			//state actions
	{
		case(Init):
		{
			if(USART_HasReceived(0))
			{
				data1 = USART_Receive(0);
			}
			if(USART_HasReceived(1))
			{
				data2 = USART_Receive(1);
			}
			getString();
			LCD_DisplayString(1,string);
			LCD_Cursor(0);
			if (data1 != 0)
			{
				if (data1 == 1)
				{
					LCD_Cursor(13);
					if (data2 < 10)
					{
						LCD_WriteData(data2 + '0');
					}
					else if (data2 >= 10)
					{
						getTens();
						LCD_WriteData(tens + '0');
						LCD_Cursor(14);
						data2 = data2 % 10;
						LCD_WriteData(data2 + '0');
					}
					LCD_Cursor(0);
				}
				else if (data1 == 2)
				{
					LCD_Cursor(29);
					if (data2 < 10)
					{
						LCD_WriteData(data2 + '0');
					}
					else if (data2 >= 10)
					{
						getTens();
						LCD_WriteData(tens + '0');
						LCD_Cursor(30);
						data2 = data2 % 10;
						LCD_WriteData(data2 + '0');
					}
					LCD_Cursor(0);
				}
			}
			break;
		}
		default:
		break;
	}
	return state;
}


int main(void)
{
	DDRC = 0xFF;
	PORTC = 0x00;
	DDRD = 0xFF;
	PORTD = 0x00;
	initUSART(0);
	initUSART(1);
	LCD_init();

	// Period for the tasks
	unsigned long int LCD_calc = 500;


	//Calculating GCD
	unsigned long int tmpGCD = LCD_calc;

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int LCD_period = LCD_calc/GCD;

	//Declare an array of tasks
	static task task1;
	task *tasks[] = { &task1 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = LCD_period;//Task Period.
	task1.elapsedTime = LCD_period;//Task current elapsed time.
	task1.TickFct = &LCD_Tick;//Function pointer for the tick.

	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();

	unsigned short i; // Scheduler for-loop iterator
	while(1) {
		// Scheduler code
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				// Reset the elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}

	// Error: Program should not exit!
	return 0;
}

