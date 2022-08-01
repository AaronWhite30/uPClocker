#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 1000000ul

#include <util/delay.h>

#define portdPinChangeInterrupt PCIE2

#define durationTimerModeSelectPin PIND7
#define stepModePulsePin PIND4
#define durationTimerTogglePin PIND2

#define pulseTimerOutput PIND3
#define durationTimerOutput PINC5
#define durationTimer0Output PINB0
#define durationTimer1Output PINB1
#define durationTimer2Output PINB2
#define durationTimer3Output PINB3
#define durationTimerOffStepModeOutput PINB4

#define pulseTimerCycles 20

int  stepModePulseRequest = 0;
int  durationTimerModeRequest = 0;
int  durationTimerCompareMatched = 0;

int  durationTimerMode = 0;
int  durationTimerToggle = 0;

static const int durationTimerModes[5] = {977, 1465, 1953, 2441, 0};
	
ISR(PCINT2_vect){
	if(PIND & 1<<durationTimerTogglePin){
		if(durationTimerMode != 4){
			if(durationTimerToggle){
				durationTimerToggle = 0;
			}else if(~durationTimerToggle){
				durationTimerToggle = 1;
			}
		}
	}
	if(PIND & 1<<durationTimerModeSelectPin){
		if(durationTimerToggle == 0){
			durationTimerModeRequest = 1;
		}
	}
	if(PIND & 1<<stepModePulsePin){
		if(durationTimerToggle == 0 && durationTimerMode == 4){
			stepModePulseRequest = 1;
		}
	}
}

void setupDurationTimer(){	
	TCCR1B = (1<<WGM12);
	OCR1A = durationTimerModes[durationTimerMode];
	TIMSK1 = (1<<OCIE1A);
}

ISR(TIMER1_COMPA_vect){
	durationTimerCompareMatched = 1;
}

void turnOnDurationTimer(){
	OCR1A = durationTimerModes[durationTimerMode];
	TCCR1B |= (1<<CS12)|(1<<CS10);
}

void turnOffDurationTimer(){
	TCCR1B &= ~(1<<CS10);
	TCCR1B &= ~(1<<CS12);
	PORTC &= ~(1<<durationTimerOutput);
}

void sendPulse(){
	uint8_t m=0xff-(pulseTimerCycles-1); OCR2B=m; TCNT2=m-1;
}

void setupPulseTimer(){
	
	TCCR2B =  0;
	TCNT2 = 0x00;
	OCR2A = 0;
	TCCR2A = (1<<COM2B0) | (1<<COM2B1) | (1<<WGM20) | (1<<WGM21);
	TCCR2B = (1<<WGM22) | (1<<CS20);
}

void setup(){
	
	cli();
	
	DDRD |= (1<<pulseTimerOutput);
	DDRC |= (1<<durationTimerOutput);
	DDRD &= ~(1<<durationTimerTogglePin);
	DDRD &= ~(1<<durationTimerModeSelectPin);
	DDRD &= ~(1<<stepModePulsePin);
	
	PCICR |= (1<<portdPinChangeInterrupt);
	PCMSK2 |= (1<<PCINT23) | (1<<PCINT20) | (1<<PCINT18);

	DDRB |= (1<<durationTimer0Output) | (1<<durationTimer1Output) | 1<<(durationTimer2Output)
		| 1<<(durationTimer3Output) | (1<<durationTimerOffStepModeOutput);
	
	durationTimerMode = 0;
	PORTB |= (1<<durationTimer0Output);
	
	setupDurationTimer();
	setupPulseTimer();
	
	sei();
}

int main()
{
	setup();
	
	while(1)
	{
		
		if(durationTimerModeRequest){
			durationTimerModeRequest = 0;
			switch(durationTimerMode){
				case 0:
					durationTimerMode = 1;
					PORTB &= ~(1<<durationTimer0Output);
					PORTB |= (1<<durationTimer1Output);
					break;
				case 1:
					durationTimerMode = 2;
					PORTB &= ~(1<<durationTimer1Output);
					PORTB |= (1<<durationTimer2Output);
					break;
				case 2:
					durationTimerMode = 3;
					PORTB &= ~(1<<durationTimer2Output);
					PORTB |= (1<<durationTimer3Output);
					break;
				case 3:
					durationTimerMode = 4;
					PORTB &= ~(1<<durationTimer3Output);
					PORTB |= (1<<durationTimerOffStepModeOutput);
					break;
				case 4:
					durationTimerMode = 0;
					PORTB &= ~(1<<durationTimerOffStepModeOutput);
					PORTB |= (1<<durationTimer0Output);
					break;
			}
		}
		
		if(durationTimerToggle){
			turnOnDurationTimer();
		}else if(~durationTimerToggle){
			turnOffDurationTimer();
		}
		
		if(stepModePulseRequest){			
			stepModePulseRequest = 0;
			PORTC |= (1 << durationTimerOutput);
			sendPulse();
			_delay_ms(500);
			PORTC &= ~(1 << durationTimerOutput);
		}
		
		if(durationTimerCompareMatched){
			durationTimerCompareMatched = 0;
			PORTC ^= (1 << durationTimerOutput);
			sendPulse();
		}
	}
}