/*
 * adeb001_shoote_game.c
 * 
 * I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 * 
 * Author : Anthony De Belen
 */ 
 #define F_CPU 8000000UL

 #define SEC 1000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <time.h>
#include <stdlib.h>
#include "usart_ATmega1284.h"

//ADC variables
unsigned short adcval;
unsigned short MAX = 96;

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	//        in Free Running Mode, a new conversion will trigger whenever
	//        the previous conversion completes.
}

void Set_ADC_Pin(unsigned char pinNum) {
	ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
	// Allow channel to stabilize
	static unsigned char i = 0;
	for ( i=0; i<15; i++ ) { asm("nop"); }
}

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


enum Motor_States{Init,Press_CW,Press_CCW};
enum Gesture_States{Initialize,Input};
enum Photoresistor_LED_States{P_L};
enum Game_States{G_Init,Get_Target,Show_Target,Laser_Hit,Game_End};
//motor sm
unsigned char input_v = 0;
unsigned char tmpA = 0;
unsigned char tmpB = 0;
unsigned char tmpC = 0;
unsigned char tmpD = 0;
int phase = 0;
unsigned long init_cnt = 0;
//photoresistor sm
unsigned short newMax = 242;
unsigned int maxCount = 0;
unsigned int count = 0;
//game sm
int gameStart = 0;
unsigned char photo_code = 0;
unsigned char led_code = 0;
unsigned int l_hit = 0;
unsigned long c_timer = 0;
unsigned long max_timer = 2400;								//Time Limit
unsigned char total_hits = 0x00;
unsigned char c_target;
unsigned char counter1 = 0;
unsigned char pattern[20] = {2,5,8,1,4,6,5,2,7,3,6,1,5,8,2,3,6,1,4,8};
unsigned char text_code = 0;
unsigned short init_light = 0x01;
unsigned char boot_cnt = 0;
unsigned long end_cnt = 0;
//SMs
int Motor_Tick();
int Gesture_Tick();
int Photoresistor_LED_Tick();
int Game_Tick();

int Motor_Tick(int state){
	switch (state)
	{
		case Init:
		{
			//PORTC = input_v;
			if (input_v == 8)
			{
				switch(input_v)
				{
					case 8: {maxCount = 512; break;};
				}
				state = Press_CW;
				break;
			}
			else if (input_v == 1)
			{
				switch(input_v)
				{
					case 1: {maxCount = 512; break;};
				}
				state = Press_CCW;
				break;
			}
			else{
				state = Init;
				break;
			}
		}
		case Press_CW:
		{
			//PORTC = input_v;
			if (input_v == 8 || count <= maxCount)
			{
				state = Press_CW;
				break;
			}
			else if (input_v == 1 || count <= maxCount)
			{
				state = Press_CCW;
				break;
			}
			else if (count >= maxCount)
			{
				state = Init;
				break;
			}
		}
		case Press_CCW:
		{
			//PORTC = input_v;
			if (input_v == 1 || count <= maxCount)
			{
				state = Press_CCW;
				break;
			}
			else if (input_v == 8 || count <= maxCount)
			{
				state = Press_CW;
				break;
			} 
			if (count >= maxCount)
			{
				state = Init;
				break;
			}
		}
		default: state = Init;
	}
	switch (state)
	{
		case Init:
		{
			count = 0;
			break;
		}
		case Press_CW:
		{
			switch (phase)
			{
				case 0:{tmpB = 0x01; phase++; break;};
				case 1:{tmpB = 0x03; phase++; break;};
				case 2:{tmpB = 0x02; phase++; break;};
				case 3:{tmpB = 0x06; phase++; break;};
				case 4:{tmpB = 0x04; phase++; break;};
				case 5:{tmpB = 0x0C; phase++; break;};
				case 6:{tmpB = 0x08; phase++; break;};
				case 7:{tmpB = 0x09; phase++; break;};
				default: break;
			}
			if (phase > 7)
			{
				phase = 0;
			}
			count++;
			PORTB = tmpB;
			break;
		}
		case Press_CCW:
		{
			switch (phase)
			{
				case 0:{tmpB = 0x01; phase--; break;};
				case 1:{tmpB = 0x03; phase--; break;};
				case 2:{tmpB = 0x02; phase--; break;};
				case 3:{tmpB = 0x06; phase--; break;};
				case 4:{tmpB = 0x04; phase--; break;};
				case 5:{tmpB = 0x0C; phase--; break;};
				case 6:{tmpB = 0x08; phase--; break;};
				case 7:{tmpB = 0x09; phase--; break;};
				default: break;
			}
			if (phase < 0)
			{
				phase = 7;
			}
			count++;
			PORTB = tmpB;
			break;
		}
		//case DEAD:
		//	break;
		default: break;
	}
	return state;
}

int Gesture_Tick(int state) {
	// Transitions
	switch (state) {
		case Initialize:
		if (init_cnt <= 6000)
		{
			//PORTC = tmpD | 0x80;
			init_cnt++;
			input_v = 4;
			state = Initialize;
			break;
		}
		else
		{
			init_cnt = 0;
			gameStart = 1;
			text_code = 1;
			state = Input;
			break;
		}
		case Input:
		state = Input;
		break;
		default:
		state = Initialize;
		break;
	}
	// Actions
	switch (state) {
		case Initialize:
		{
			if (USART_IsSendReady(0))
			{
				USART_Send(text_code,0);
				if (USART_HasTransmitted(0))
				{
					USART_Flush(0);
				}
			}
			if (boot_cnt < 100)
			{
				PORTC = init_light;
				boot_cnt++;
			}
			else
			{
				init_light = init_light << 1;
				if (init_light > 128)
				{
					init_light = 0x01;
				}
				boot_cnt = 0;
			}
			break;
		}
		case Input:
		tmpD = PIND & 0x30;
		//PORTC = tmpD;
		if (tmpD == 16)
		{
			input_v =  1;
			//_delay_us(2);
		}
		else if (tmpD == 32)
		{
			input_v =  8;
			//_delay_us(2);
		}
		else
		{
			input_v = 0;
		}
		//_delay_ms(SEC);
		break;
		default:
		break;
	}
	return state;
}

int Photoresistor_LED_Tick(int state) {
	// Transitions
	switch (state) {
		case P_L:
		{
			state = P_L;
			break;
		}
		default:
		state = P_L;
		break;
	}
	// Actions
	switch (state) {
		case P_L:
		{
			if (gameStart == 1)
			{
			PORTC = led_code;
			//photo_code = 0x04;
			Set_ADC_Pin(photo_code);
			adcval = ADC;
			if(adcval <= newMax)
			{
				l_hit = 0;
				//PORTC = led_code | 0x00;
			}
			else
			{
				l_hit = 1;
				//PORTC = led_code | 0x40;
			}
			}
		}
		break;
		default:
		break;
	}
	return state;
}

int Game_Tick(int state) {
	// Transitions
	switch (state) {
		case G_Init:
		{
			if (gameStart == 1)
			{
				state = Get_Target;
				break;
			}
			else
			{
				state = G_Init;
				break;
			}
		}
		case Get_Target:
		{
			if (c_timer >= max_timer)
			{
				state = Game_End;
				break;
			}
			else
			{
				state = Show_Target;
				break;
			}
		}
		case Show_Target:
		{
			if (c_timer >= max_timer)
			{
				state = Game_End;
				break;
			}
			else if (l_hit == 1)
			{
				state = Laser_Hit;
				break;
			}
			else
			{
				state = Show_Target;
				break;
			}
		}
		case Laser_Hit:
		{
			if (c_timer >= max_timer)
			{
				state = Game_End;
				break;
			}
			else
			{
				state = Get_Target;
				break;
			}
		}
		case Game_End:
		{
			if (end_cnt >= 100)
			{
				state = G_Init;
				end_cnt = 0;
				break;
			}
			else
			{
				state = Game_End;
				break;
			}
		}
		default:
		state = G_Init;
		break;
	}
	// Actions
	switch (state) {
		case G_Init:
		{
			break;
		}
		case Get_Target:
		{
			//srand(time(NULL));
			//c_target = rand() % 8 + 1;
			text_code = 1;
			c_target = pattern[counter1];
			counter1++;
			if (counter1 == 20)
			{
				counter1 = 0;
			}
			//PORTC = c_target;
			switch (c_target)
			{
				case 1: {photo_code = 0x00; led_code = 1; break;};
				case 2: {photo_code = 0x01; led_code = 2; break;};
				case 3: {photo_code = 0x02; led_code = 4; break;};
				case 4: {photo_code = 0x03; led_code = 8; break;};
				case 5: {photo_code = 0x04; led_code = 16; break;};
				case 6: {photo_code = 0x05; led_code = 32; break;};
				case 7: {photo_code = 0x06; led_code = 64; break;};
				case 8: {photo_code = 0x07; led_code = 128; break;};
				default: break;
			}
			c_timer++;
			break;
		}
		case Show_Target:
		{
			if (USART_IsSendReady(0))
			{
				USART_Send(text_code,0);
				if (USART_HasTransmitted(0))
				{
					USART_Flush(0);
				}
			}
			if (USART_IsSendReady(1))
			{
				USART_Send(total_hits,1);
				if (USART_HasTransmitted(1))
				{
					USART_Flush(1);
				}
			}
			c_timer++;
			break;
		}
		case Laser_Hit:
		{
			c_timer++;
			total_hits++;
			l_hit = 0;
			if (USART_IsSendReady(0))
			{
				USART_Send(text_code,0);
				if (USART_HasTransmitted(0))
				{
					USART_Flush(0);
				}
			}
			if (USART_IsSendReady(1))
			{
				USART_Send(total_hits,1);
				if (USART_HasTransmitted(1))
				{
					USART_Flush(1);
				}
			}
			break;
		}
		case Game_End:
		{
			//gameStart = 0;
			c_timer = 0;
			//total_hits = 0;
			photo_code = 0;
			counter1 = 0;
			led_code = 0;
			text_code = 2;
			if (USART_IsSendReady(0))
			{
				USART_Send(text_code,0);
				if (USART_HasTransmitted(0))
				{
					USART_Flush(0);
				}
			}
			if (USART_IsSendReady(1))
			{
				USART_Send(total_hits,1);
				if (USART_HasTransmitted(1))
				{
					USART_Flush(1);
				}
			}
			end_cnt++;
			break;
		}
		default:
		break;
	}
	return state;
}

// Implement scheduler code from PES.
int main()
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0x00; PORTD = 0xFF;

	// Period for the tasks
	unsigned long int Motor_calc = 3;
	unsigned long int Gesture_calc = 10;
	unsigned long int Photoresistor_LED_calc = 10;
	unsigned long int Game_calc = 50;


	//Calculating GCD
	unsigned long int tmpGCD = Motor_calc;
	tmpGCD = findGCD(Motor_calc, Gesture_calc);
	tmpGCD = findGCD(tmpGCD, Photoresistor_LED_calc);
	tmpGCD = findGCD(tmpGCD, Game_calc);

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int Motor_period = Motor_calc/GCD;
	unsigned long int Gesture_period = Gesture_calc/GCD;
	unsigned long int Photoresistor_LED_period = Photoresistor_LED_calc/GCD;
	unsigned long int Game_period = Game_calc/GCD;

	//Declare an array of tasks
	static task task1, task2, task3, task4;
	task *tasks[] = { &task1, &task2, &task3, &task4 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = Motor_period;//Task Period.
	task1.elapsedTime = Motor_period;//Task current elapsed time.
	task1.TickFct = &Motor_Tick;//Function pointer for the tick.

	// Task 2
	task2.state = -1;//Task initial state.
	task2.period = Gesture_period;//Task Period.
	task2.elapsedTime = Gesture_period;//Task current elapsed time.
	task2.TickFct = &Gesture_Tick;//Function pointer for the tick.

	// Task 3
	task3.state = -1;//Task initial state.
	task3.period = Photoresistor_LED_period;//Task Period.
	task3.elapsedTime = Photoresistor_LED_period;//Task current elapsed time.
	task3.TickFct = &Photoresistor_LED_Tick;//Function pointer for the tick.

	// Task 4
	task4.state = -1;//Task initial state.
	task4.period = Game_period;//Task Period.
	task4.elapsedTime = Game_period;//Task current elapsed time.
	task4.TickFct = &Game_Tick;//Function pointer for the tick.

	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();
	ADC_init();
	initUSART(0);
	initUSART(1);

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