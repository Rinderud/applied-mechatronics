/*
 * debug.c
 *
 * Author : Jacob Rinderud
 */

#define F_CPU 1000000
#define BAUD 2400
#define ubr (F_CPU / 16 / BAUD - 1)
#define AVGSIZE 8	  // For moving average
#define AVGSIZE_exp 3 // For moving average division
#define BITS 4		  // For fixed point

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>

// Global variables used in more than one place
int16_t count, speed, ref, duty, bias;
int16_t moving_register[AVGSIZE];

// Debug LEDs
int setup_dbug_LED(void)
{
	// PD2,3,4
	DDRD |= (0b00011100);
	return 1;
}
int LED_on(int i)
{
	switch (i)
	{
	case 0:
		PORTD |= (1 << PD4);
		return 1;
	case 1:
		PORTD |= (1 << PD3);
		return 1;
	case 2:
		PORTD |= (1 << PD2);
		return 1;
	}
	return 0;
}
int LED_off(int i)
{
	switch (i)
	{
	case 0:
		PORTD &= ~(1 << PD4);
		return 1;
	case 1:
		PORTD &= ~(1 << PD3);
		return 1;
	case 2:
		PORTD &= ~(1 << PD2);
		return 1;
	}
	return 0;
}

// Using the debug LEDs to illustrate the speed
int illustrate_speed(void)
{
	const int8_t acc = 3;
	const int8_t err = ref - speed; // Kan jag ha const h�r?
	//							den �ndras inte i funktionen men utanf�r?
	/* Turning off the LEDs to make sure only the ones that should be on is */
	LED_off(0);
	LED_off(1);
	LED_off(2);

	if (err > acc) // Low speed
	{
		if (err > (2 * acc)) // Very low
		{

			LED_on(0);
		}
		else
		{
			LED_on(0);
			LED_on(1);
		}
	}
	else if (err < -acc) // Negative err means high speed
	{
		if (err < (-2 * acc)) // Very high
		{
			LED_on(2);
		}
		else
		{
			LED_on(1);
			LED_on(2);
		}
	}
	else if ((-acc <= err) && (err <= acc))
	{
		LED_on(1);
	}
	return 1;
}

// Interrupts, both speed and dimmer
int setup_interrupts(void)
{
	PCICR = ((1 << PCIE1) | (1 << PCIE2) | (1 << PCINT13) | (1 << PCINT12));

	/* Enabling interrupts in the mask
	13 & 12 being the speed encoder
	9 & 8 for he dimmer encoder	*/
	PCMSK1 = (1 << PCINT9) | (1 << PCINT8) | (1 << PCINT13) | (1 << PCINT12);
	// PCMSK2 = (1 << PCINT22);
	//  (1<<PCINT13)|(1<<PCINT12)
	/* Turning on interrupts in general */
	sei();
	return 1;
}

// PWM
int setup_pwm(void)
{
	/* Set port */
	DDRD |= (1 << DDD5);

	/* Set the timer up
	Clear OC0A on compare match [10]
	Set OC0B on compare match	[11]
	------------				[--]
	Set Fast PWM				[11]
	*/
	TCCR0A |= 0b10110011;

	/* No prescaler */
	TCCR0B |= 0x01;
	return 1;
}
int update_pwm(void)
{
	/* Making sure the duty is within the range */
	if (duty < 0)
	{
		OCR0B = 0;
		return 1;
	}
	else if (duty > 255)
	{
		OCR0B = 255;
		return 1;
	}
	OCR0B = duty;
	return 1;
}

// Serial communications
int USART_init(unsigned int ubrr)
{
	/*Set baud rate */
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;

	/* Enable receiver and transmitter */
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	/* Set frame format: 8data, 1stop bit */
	UCSR0C = (3 << UCSZ00);
	return 1;
}
int USART_Transmit(unsigned char data)
{
	/* Wait for empty transmit buffer */
	while (!(UCSR0A & (1 << UDRE0)))
		;
	/* Put data into buffer, sends the data */
	UDR0 = data;
	return 1;
}
unsigned char USART_Receive(void)
{
	/* Wait for data to be received */
	while (!(UCSR0A & (1 << RXC0)))
		;

	/* Get and return received data from buffer */
	return UDR0;
}

// Speed count clock
int setup_speed_clock(void)
{
	/* Timer clock = I/O clock / prescaler 8 */
	TCCR1B = (1 << CS11);

	/* Clear ICF1. Clear pending interrupts */
	TIFR1 = 1 << ICF1;

	/* Enable Timer 1 Overflow Interrupt */
	TIMSK1 = 1 << TOIE1;
	return 1;
}

// Calculate average of the counts
int16_t average(void)
{
	uint16_t sum = 0;
	for (int i = 0; i < AVGSIZE; i++)
	{
		sum += moving_register[i];
	}
	return sum >> AVGSIZE_exp;
}
int16_t exp_average(void)
{
	uint32_t sum = 0;
	sum = (moving_register[0] >> (AVGSIZE - 1));
	for (int i = 1; i < AVGSIZE; i++)
	{
		sum += (moving_register[i] >> (AVGSIZE - i));
	}
	return sum;
}

// Save the new count by shifting the register
int count_saver(int16_t input)
{
	for (int i = 0; i < AVGSIZE - 1; i++)
	{
		moving_register[i] = moving_register[i + 1];
	}
	moving_register[AVGSIZE - 1] = input;
	return 1;
}

/* Fixed point functions */
int16_t to_fixed_point(int16_t x)
{
	int16_t X = (x << BITS);
	return X;
}
int16_t from_fixed_point(int16_t X)
{
	int16_t x = (X >> BITS);
	return x;
}

int16_t mul(int16_t X, int16_t Y)
{
	int32_t temp = (int32_t)X * Y;
	temp += (1 << (BITS - 1));
	temp = temp >> BITS;
	if (temp > INT16_MAX)
	{
		return INT16_MAX;
	}
	else if (temp < INT16_MIN)
	{
		return INT16_MIN;
	}
	else
	{
		return temp;
	}
}
int16_t div(int16_t X, int16_t Y)
{
	if (Y == 0)
	{
		return 0;
	}
	int32_t temp;
	temp = (int32_t)(X << BITS);
	temp += (Y >> 1);
	temp /= Y;
	return temp;
}

const int16_t UMAX = 255 << BITS;
const int16_t UMIN = 0;
int16_t sat(int16_t X)
{
	if (X > UMAX)
	{
		return UMAX;
	}
	else if (X < UMIN)
	{
		return UMIN;
	}
	else
	{
		return X;
	}
}

int16_t count_to_speed(int16_t average_count)
{
	int16_t rpm = from_fixed_point(mul(to_fixed_point(average_count), 19)); // 77?6

	/*
	uint16_t temp = average_count;
	uint16_t rpm = average_count;
	rpm += temp >> 2;
	rpm -= temp >> 5;
	rpm -= temp >> 6;
	*/
	return rpm + bias;
}

/* Regulation */
const int16_t AD = 10;
const int16_t BD = 1;
const int16_t BETA = 8; // 0<=beta<=1 = 0<=BETA<=16
const int16_t Ti = 4;	// Integration time
const int16_t K = 50;	// Higher K = faster & less stable
const int16_t H = 1;	// Lower = better?
						// h = 1.4/omega0
const int16_t Tr = 1;	// Anti-reset? LP-filter
						// Possible samples per rise time?
int16_t D = 0;
int16_t I = 0;
int16_t Yold = 0;
int pi(void)
{
	int16_t Y, Yref, E, V, U;
	Y = to_fixed_point(speed);
	Yref = to_fixed_point(ref);
	E = Yref - Y;
	D = mul(AD, D) - mul(BD, (Y - Yold));
	V = mul(K, (mul(BETA, Yref) - Y)) + I + D;
	U = sat(V);
	duty = from_fixed_point(U);
	I += mul(div(mul(K, H), Ti), E) + mul(div(H, Tr), (U - V));
	Yold = Y;
	return 1;
}

ISR(TIMER1_OVF_vect)
{
	// When the timer overflows
	cli();
	count_saver(count); // Save the count
	count = 0;			// Reset the count

	/* Calculate the speed */
	// speed = count_to_speed(average());
	speed = count_to_speed(exp_average());
	sei();
}

bool A, B;
ISR(PCINT1_vect)
{
	cli();
	count++;
	uint8_t diff = 1; // Arbitrary granularity in dimmer
	bool newA = 0;
	bool newB = 0;

	if (PINC & (1 << PINC1))
	{
		newA = 1;
	}
	if (PINC & (1 << PINC0))
	{
		newB = 1;
	}

	if (!newA && !newB)
	{
		int8_t direction = (B << 1) | A;
		direction += (newA << 1) | newB;
		if (direction == 0b01)
		{
			bias += diff;
		}
		else if (direction == 0b10)
		{
			bias -= diff;
		}
	}

	A = newA;
	B = newB;
	sei();
}

bool get_bit(int num, int position)
{
	bool bit = num & (1 << position);
	return bit;
}

int illustrate_number(int8_t num)
{
	for (int8_t i = 0; i < 3; i++)
	{
		if (get_bit(num, i))
		{
			LED_on(i);
		}
		else
		{
			LED_off(i);
		}
	}

	return 1;
}

int test_Dbug()
{
	LED_off(0);
	LED_off(1);
	LED_off(2);
	_delay_ms(200);
	LED_on(0);
	_delay_ms(200);
	LED_on(1);
	_delay_ms(200);
	LED_on(2);
	_delay_ms(400);

	LED_off(0);
	LED_off(1);
	LED_off(2);
	_delay_ms(200);
	LED_on(1);
	_delay_ms(200);
	LED_on(0);
	LED_on(2);
	_delay_ms(200);

	return 1:
}

int test_PWM()
{
	for (int8_t i = 0; i < 8; i++)
	{
		illustrate_number(i);
		duty = i * 32;
		update_pwm();
		_delay_ms(800);
	}

	return 1;
}

int test_dimmer()
{
	bias = 128;
	while (1)
	{
		duty = bias;
		_delay_ms(10);
	}
	return 1;
}

int test_loopback()
{
	unsigned char Rx;

	for (size_t i = 0; i < 42; i++)
	{
		Rx = USART_Receive();
		USART_Transmit(Rx);
	}

	return 1;
}

int main(void)
{
	setup_dbug_LED();
	test_Dbug();
	_delay_ms(500);

	setup_pwm();
	test_PWM();
	_delay_ms(500);

	setup_interrupts();
	test_dimmer(); // Infinity
	_delay_ms(500);

	USART_init(ubr);
	test_loopback();
	_delay_ms(500);

	setup_speed_clock();
	_delay_ms(500);

	speed = 0;
	count = 0;
	ref = 0;
	duty = 0;
	bias = 0;

	while (1)
	{
		ref = USART_Receive();
		USART_Transmit(speed);
		illustrate_speed();
		pi();
		update_pwm();

		_delay_ms(600);
	}
	return 1;
}
