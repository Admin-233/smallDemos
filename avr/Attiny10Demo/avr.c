#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

static unsigned char mask = 0b00000001;

/*
void delay(void) {
    for (uint32_t i = 0; i < 40000; i++) {
        __asm__ __volatile__ ("nop");
    }
}
*/

ISR(__vector_4)
{
	//TCNT0 = 65070;//timer count, precisely 1kHz
	//TCNT0 = 64910;//about 750Hz
	TCNT0 = 64579;//precisely 500Hz
	//TCNT0 = 65461;//about 4.9969 kHz
	//TCNT0 = 65462;//about 5.0505 kHz

	if(PINB & (1 << PINB2))//PINB2 high
	{
		PORTB = 0b00000000;

		set_sleep_mode(SLEEP_MODE_PWR_DOWN);

		EIMSK = 0b00000001;//enable int0		
	}
	else//PINB2 low active low
	{
		mask ^= 0b00000011;

		PORTB = mask;

		set_sleep_mode(SLEEP_MODE_IDLE);
	}

}

ISR(INT0_vect)
{
	TCNT0 = 0xFFFE;//timer count
	EIMSK = 0b00000000;//disable int0
	set_sleep_mode(SLEEP_MODE_IDLE);
}

int main()
{
	DDRB = 0b00000011;
	PORTB = 0b00000000;
	PUEB = 0b00000100;

	PRR = 0b00000010;

	EICRA = 0b00000000;//int0 active-low

	ACSR = 0b10000000;//shutdown analog comparator

    SMCR = 0b00000001;//enable sleepmode, idle mode

	TCCR0A = 0b00000000;//disable compare mode, set normal mode(WGM)
	TCCR0B = 0b00000001;//disable input capture functions, set normal mode(WGM), set clock(No prescaling)
	TCCR0C = 0b00000000;//disable Force Output Compare

	TCNT0 = 0xFFFE;//timer count
    
	TIMSK0 = 0b00000001;//only enable overflow int

	GTCCR = 0b00000000;//start timer

    sei();//open int

	while(1)
	{
		sleep_cpu();
	}
}

