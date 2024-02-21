#include <avr/io.h>
#include <stdio.h>
#define F_CPU 8000000UL

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>

#define DEVICE_ADDR 2
#define DEBOUNCE_MS 20

void USART_transmit(unsigned char data)
{
	while(!(UCSRA & (1 << UDRE)))
		;
	UDR = data;
}

void processCmd(uint8_t cmd, uint8_t var)
{
	if(cmd == 0)
		OCR0A = var;
	else if(cmd == 1)
		OCR1A = var;
	else if(cmd == 2)
		OCR0B = var;
	else if(cmd == 3)
		OCR1B = var;
}

void processUsartIrq(uint8_t byte)
{
	static bool isRxAddr = false;
	static bool isRxCmd = false;
	static bool isRxVar = false;

	static uint8_t cmd = 0;

	if(isRxVar)
	{
		processCmd(cmd, byte);

		isRxAddr = false;
		isRxCmd = false;
		isRxVar = false;
	}
	else if(isRxCmd)
	{
		cmd = byte;
		isRxVar = true;
	}
	else if(isRxAddr)
	{
		if(byte == DEVICE_ADDR)
		{
			isRxCmd = true;
		}
	}
	else
	{
		if(byte == 0xFF)
		{
			isRxAddr = true;
		}
	}
}

void transmitButton(uint8_t button, bool state)
{
	USART_transmit(0xFF);
	USART_transmit(DEVICE_ADDR);
	USART_transmit(button);
	USART_transmit(state);
}

ISR(USART_RX_vect)
{
	processUsartIrq(UDR);
}

int main(void)
{
	DDRB = (1 << 2) | (1 << 3) | (1 << 4);
	PORTB = (1 << 0) | (1 << 1) | (1 << 5);
	DDRD = (1 << 0) | (1 << 1) | (1 << 5);
	PORTD = (1 << 6);

	UBRRH = (unsigned char)(8 >> 8);
	UBRRL = (unsigned char)8;
	UCSRB = (1 << TXEN) | (1 << RXEN) | (1 << RXCIE);
	UCSRC = (1 << USBS) | (3 << UCSZ0);

	TCCR0A = (1 << COM0A1) | (1 << COM0B1) | /*(1 << WGM01) | */ (1 << WGM00);
	TCCR0B = (1 << CS01); // clock source = CLK/8, start PWM

	TCCR1A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM10);
	TCCR1B = (1 << CS01); // clock source = CLK/8, start PWM

	OCR0A = 0; // write new PWM value
	OCR0B = 0;

	OCR1A = 0; // write new PWM value
	OCR1B = 0;

	_delay_ms(200);

	sei();

#define CHECK_BTN(port, pin, num)                                                 \
	if((port & (1 << pin)) == false && state[num] == false && debounce[num] == 0) \
	{                                                                             \
		debounce[num] = 20;                                                       \
		state[num] = true;                                                        \
		transmitButton(num, true);                                                \
	}                                                                             \
	else if((port & (1 << pin)) && state[num] == true && debounce[num] == 0)      \
	{                                                                             \
		debounce[num] = 20;                                                       \
		state[num] = false;                                                       \
		transmitButton(num, false);                                               \
	}

	while(1)
	{
		static uint8_t debounce[4] = {0, 0, 0, 0};
		static bool state[4] = {0, 0, 0, 0};

		CHECK_BTN(PINB, 0, 0);
		CHECK_BTN(PINB, 1, 2);
		CHECK_BTN(PINB, 5, 3);
		CHECK_BTN(PIND, 6, 1);

		for(uint8_t i = 0; i < sizeof(debounce) / sizeof(debounce[0]); i++)
		{
			if(debounce[i] > 0) debounce[i]--;
		}

		_delay_ms(1);
	}

	return 0;
}
