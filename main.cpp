#ifndef F_CPU
#define  F_CPU 16000000UL
#endif
#include <avr/io.h>
#include <stdlib.h>						// include the random nr gen library
#include <util/delay.h>
#include <avr/eeprom.h>					// include eeprom library to save measurements to eeprom memory
#include <time.h>
#include <avr/interrupt.h>


void init()
{
	// Pin setup
	DDRC |= (1 << 5);					// sets bit 5 to be output (LED)
	DDRB |= (1 << 1);					// set PB1 (OC1A) as output (Servo)
	
	// Timer 1 init
	TCCR1A = (1 << COM1A1);				// Set OC1A & OC1B and WMG11
	TCCR1B |= (1 << WGM13)|(1 << CS01);	// Set WGM13
	ICR1 = 20000;						// Count of 40000 for a 20 ms period or 50 Hz cycle
	OCR1A = 2900;

	// Buttons
	DDRB &= ~(1 << DDB0);				// Clear the PB0 pin and make it an input (Button1)
	DDRD &= ~(1 << DDD0);				// Clear the PD0 pin and make it and an input (Button2)
	PORTB |= (1 << PORTB0);				// Make button pin 1 high
	PORTD |= (1 << PORTD0);				// Make button pin high 
	
	// External interrupt
	PCICR |= (1 << PCIE0);				// PCIE0 to enable PCMSK0 scan
	PCMSK0 |= (1 << PCINT0);			// Set PCINT0 to trigger an interrupt on a state change
	
	// Random
	srand (time(NULL));
	sei();
}

void GetRandom()						// Function to generate a random number for LED 
{
	int wait = rand() % 4000 + 1000;	// Number will anything between 1 and 4 
	while (wait > 1)
	{    _delay_ms(1);
		wait -= 1;
	}
	PORTC |= (1 << 5);					// Turns on LED and whenever the random time runs out, LED goes on 
	TCNT1 = 0;							// Turns on timer count to get the amount of ticks it took to react 
	
}

unsigned char button_state()			// This little function says that the button is pressed when Button1 pin is low
{										// Because with pull up button, the state is high as long as the button is not pressed
	if (!(PIND & (1 << PORTD0)))
	{
		if (!(PIND & (1 << PORTD0))) return 1;
	}
	return 0;
}


ISR (PCINT0_vect)
{
	GetRandom();								// Whenever button is pressed 
}												// External interrupt happens and random number for LED gets generated 


void score_converter()							// Converts the amount of ticks to pwm signal for servo
{
	int score = TCNT1;
	if (score < 4000)							// If you're really quicky 
	{
		OCR1A = 2400;
	}
	if ((4000 <= score) & (score < 6000))		// If you're not so quick 
	{
		OCR1A = 2000;
	}
	if ((6000 <= score) & (score < 8000))		// If you're pretty slow
	{
		OCR1A = 1700;
	}
	if (score > 8000)							// If you are as slow as a snail 
	{
		OCR1A = 900;
	}	
}


void eeprom()
{
	uint8_t captured_time = TCNT1;
	uint8_t eeprom_time = ceil(captured_time / 10);
	
	if (eeprom_time != 0)
	{
		eeprom_busy_wait();										// on the second position in eeprom, there is a row count
		uint8_t j = eeprom_read_byte((uint8_t*)1);				// it reads the row count and adds 1 to it
		uint8_t i = j + 1;										// and writes the score to that line
		eeprom_update_byte((uint8_t*)1, i);						// rewrite the row count on position 1
		eeprom_write_byte(&i, eeprom_time);						// write score to the right position

	}
	
	if (eeprom_time < eeprom_read_byte((uint8_t*)0))			// if measured time is lower than high score
	{
		eeprom_busy_wait();
		eeprom_update_byte((uint8_t*)0, eeprom_time);			// rewrite 0 position (high score position) with new score
	}
}



int main(void)
{
	init();
	while (1)
	{
		if (button_state())					// If Button1 is pressed
		{
			PORTC &= ~(1 << 5);				// Turn off the LED
			score_converter();				// Convert the score to pwm signal for Servo 
			eeprom();						// Insert score to eeprom + update high score 
			_delay_ms(1000);
			OCR1A = 2900;					// Go back to 0 position on Servo 
			
		}
	}
	
}



