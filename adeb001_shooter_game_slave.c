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

void set_PWM(double frequency) {
	
	
	// Keeps track of the currently set frequency
	// Will only update the registers when the frequency
	// changes, plays music uninterrupted.
	//static double current_frequency;
	//if (frequency != current_frequency) {

	if (!frequency) TCCR3B &= 0x08; //stops timer/counter
	else TCCR3B |= 0x03; // resumes/continues timer/counter
	
	// prevents OCR3A from overflowing, using prescaler 64
	// 0.954 is smallest frequency that will not result in overflow
	if (frequency < 0.954) OCR3A = 0xFFFF;
	
	// prevents OCR3A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
	else if (frequency > 31250) OCR3A = 0x0000;
	
	// set OCR3A based on desired frequency
	else OCR3A = (short)(8000000 / (128 * frequency)) - 1;

	TCNT3 = 0; // resets counter
	//current_frequency = frequency;
	//}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB6 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM32: When counter (TCNT3) matches OCR3A, reset counter
	// CS31 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

//LCD SM var
unsigned char data1 = 0;
unsigned char data2 = 0;
unsigned char tens = 0;
unsigned char text_code = 0;
unsigned char hits_val = 0;
unsigned char sound_code = 0;
unsigned char ammo_val = 0;
char* string;

//music var
unsigned char tmpA;
unsigned char tmpB;
unsigned char l;
unsigned char curr;
double freq;
unsigned char melodyPosition = 0;
double gunNotes[] = {5000,5000,0,0,10,10};
double reloadNotes[] = {50,50,0,0,50};
double melodyNotes0[] = {440,440,493.88,659.26,0,392,440,493.88,0,0,440,392,0,0,0,0,349.23,349.23,349.23,392,349.23,0,392,329.63,0,0,0,0,0,0,0,0};//cat relaxing //150ms
double melodyNotes1[] = {698.46, 0, 293.66, 493.88, 392, 698.46, 293.66, 493.88, 587.33, 0, 493.88, 0, 587.33, 0, 0, 523.25,493.88,493.88,493.88,440,493.88,0,0,440,493.88,493.88,698.46,783.99,880,0,0,0}; //chocobo //150ms
double melodyNotes2[] = {1396.91,1318.51,1396.91,1760,1318.51,0,0,0,1174.66,1046.50,1174.66,1396.91,1046.50,0,0,0,987.77,880,987.77,1396.91,1318.51,0,1046.50,0,1174.66,1318.51,1396.91,1760,1567.98,0,0,0,0};//confession // 150ms

//SM
enum LCD_States {Init};
enum MUSIC_States{M_Init, M_Start, M_Fire, M_Reload, M_End};
int LCD_Tick();
int Music_Tick();

void getString()
{
	switch(text_code)
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
	tmpA = hits_val / 10;
	switch (tmpA)
	{
		case 1: {tens = 1; break;};
		case 2: {tens = 2; break;};
		case 3: {tens = 3; break;};
		default: break;
	}
}

void getAmmo()
{
	switch (ammo_val)
	{
		case 0: {break;};
		default: break;
	}
}

void USART_code()
{
	switch (data1)
	{
		case 0: {text_code = data2; getString(); break;};
		case 1: {hits_val = data2; getTens(); break;};
		case 2: {sound_code = data2; break;};
		case 3: {ammo_val = data2; getAmmo(); break;};
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
			USART_code();
			LCD_DisplayString(1,string);
			LCD_Cursor(0);
			if (text_code == 1)
			{
				LCD_Cursor(13);
				if (hits_val < 10)
				{
					LCD_WriteData(hits_val + '0');
				}
				else if (hits_val >= 10)
				{
					LCD_WriteData(tens + '0');
					LCD_Cursor(14);
					hits_val = hits_val % 10;
					LCD_WriteData(hits_val + '0');
				}
				LCD_Cursor(0);
			}
			else if (text_code == 2)
			{
				LCD_Cursor(29);
				if (hits_val < 10)
				{
					LCD_WriteData(hits_val + '0');
				}
				else if (hits_val >= 10)
				{
					LCD_WriteData(tens + '0');
					LCD_Cursor(30);
					hits_val = hits_val % 10;
					LCD_WriteData(hits_val + '0');
				}
				LCD_Cursor(0);
			}
			break;
		}
		default:
		break;
	}
	return state;
}

int Music_Tick(int state)
{
	//Transition
	switch(state)
	{
		case(M_Init):
		{
			if (melodyPosition == (sizeof(melodyNotes0)/sizeof(double)) && sound_code == 0)
			{
				melodyPosition = 0;
				state = M_Init;
				break;
			}
			if (sound_code == 1)
			{
				melodyPosition = 0;
				state = M_Start;
				break;
			}
			else state = M_Init; break;
		}
		case(M_Start):
		{
			if (melodyPosition == (sizeof(melodyNotes1)/sizeof(double)) && sound_code == 1)
			{
				melodyPosition = 0;
				state = M_Start;
				break;
			}
			if (sound_code == 2)
			{
				melodyPosition = 0;
				state = M_End;
				break;
			}
			else if(sound_code == 3)
			{
				melodyPosition = 0;
				state = M_Fire;
				break;
			}
			else if(sound_code == 4)
			{
				melodyPosition = 0;
				state = M_Reload;
				break;
			}
			else state = M_Start; break;
		}
		case(M_Fire):
		{
			state = M_Start; break;	
		}
		case(M_Reload):
		{
			state = M_Start; break;
		}
		case(M_End):
		{
			if (melodyPosition == (sizeof(melodyNotes2)/sizeof(double)) && sound_code == 2)
			{
				melodyPosition = 0;
				state = M_End;
				break;
			}
			if (sound_code == 0)
			{
				melodyPosition = 0;
				state = M_Start;
				break;
			}
			else state = M_End; break;
		}
		default: state = M_Init; break;
	}
	//Actions
	switch(state)
	{
		case(M_Init):
		{
			if (melodyPosition == sizeof(melodyNotes0))
			{
				melodyPosition = 0;
			}
			freq = melodyNotes0[melodyPosition];
			set_PWM(freq);
			if(curr == 1)
			{
				melodyPosition++;
				curr = 0;
			}
			else
			{
				curr++;
			}
			break;
		}
		case(M_Start):
		{
			if (melodyPosition == sizeof(melodyNotes1))
			{
				melodyPosition = 0;
			}
			freq = melodyNotes1[melodyPosition];
			set_PWM(freq);
			if(curr == 1)
			{
				melodyPosition++;
				curr = 0;
			}
			else
			{
				curr++;
			}
			break;
		}
		case(M_Fire):
		{
			freq = gunNotes[melodyPosition];
			set_PWM(freq);
			if(curr == 1)
			{
				melodyPosition++;
				curr = 0;
			}
			else
			{
				curr++;
			}
			break;
		}
		case(M_Reload):
		{
			freq = reloadNotes[melodyPosition];
			set_PWM(freq);
			if(curr == 1)
			{
				melodyPosition++;
				curr = 0;
			}
			else
			{
				curr++;
			}
			break;
		}
		case(M_End):
		{
			if (melodyPosition == sizeof(melodyNotes2))
			{
				melodyPosition = 0;
			}
			freq = melodyNotes2[melodyPosition];
			set_PWM(freq);
			if(curr == 1)
			{
				melodyPosition++;
				curr = 0;
			}
			else
			{
				curr++;
			}
			break;
		}
		default: break;
	}
	return state;
}


int main(void)
{
	DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	initUSART(0);
	initUSART(1);
	LCD_init();
	PWM_on();

	// Period for the tasks
	unsigned long int LCD_calc = 100;
	unsigned long int Music_calc = 150;


	//Calculating GCD
	unsigned long int tmpGCD = LCD_calc;
	tmpGCD = findGCD(tmpGCD, Music_calc);

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int LCD_period = LCD_calc/GCD;
	unsigned long int Music_period = Music_calc/GCD;

	//Declare an array of tasks
	static task task1, task2;
	task *tasks[] = { &task1, &task2 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = LCD_period;//Task Period.
	task1.elapsedTime = LCD_period;//Task current elapsed time.
	task1.TickFct = &LCD_Tick;//Function pointer for the tick.
	
	// Task 2
	task2.state = -1;//Task initial state.
	task2.period = Music_period;//Task Period.
	task2.elapsedTime = Music_period;//Task current elapsed time.
	task2.TickFct = &Music_Tick;//Function pointer for the tick.

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

