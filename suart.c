#include "suart.h"
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>

#define SUART_PRESCALER       8
#define SUART_PRESCALER_MASK   (1 << CS01)

#define SUART_TIMER_TOP \
	(F_CPU / SUART_PRESCALER / SUART_BAUD_RATE / 3 - 1)

#if SUART_TIMER_TOP > 0xff

#warning "SUART_TIMER_TOP greater than 0xFF: " \
	"increase prescaler, lower F_CPU or use a 16 bit timer"

#endif

#define SUART_RX_BUF_SIZE    32

#define SUART_TX_HIGH()        (SUART_TX_OUT |= (1 << SUART_TX_PIN))
#define SUART_TX_LOW()         (SUART_TX_OUT &= ~(1 << SUART_TX_PIN))
#define SUART_RX_READ()        (SUART_RX_IN & (1 << SUART_RX_PIN))

static uint8_t _quot;
volatile static uint8_t
	_qin,
	_rx_ready,
	_tx_busy,
	_tx_cnt,
	_tx_bits;

volatile static uint16_t _tx_byte;
volatile static char _rx_buf[SUART_RX_BUF_SIZE];

ISR(TIMER0_COMPA_vect)
{
	static uint8_t _rx_mask, _rx_cnt, _rx_bits, _rx_byte, _rx_wait_stop_bit;
	uint8_t tmp;

	/* transmitter */
	if(_tx_busy)
	{
		tmp = _tx_cnt;
		if(--tmp == 0)
		{
			if(_tx_byte & 1)
			{
				SUART_TX_HIGH();
			}
			else
			{
				SUART_TX_LOW();
			}

			_tx_byte >>= 1;
			tmp = 3;
			if(--_tx_bits == 0)
			{
				_tx_busy = 0;
			}
		}

		_tx_cnt = tmp;
	}

	/* receiver */
	if(_rx_wait_stop_bit)
	{
		if(--_rx_cnt == 0)
		{
			_rx_wait_stop_bit = 0;
			_rx_ready = 0;
			_rx_buf[_qin] = _rx_byte;
			if(++_qin >= SUART_RX_BUF_SIZE)
			{
				_qin = 0;
			}
		}
	}
	else
	{
		if(_rx_ready)
		{
			tmp = _rx_cnt;
			if(--tmp == 0)
			{
				tmp = 3;
				if(SUART_RX_READ())
				{
					_rx_byte |= _rx_mask;
				}

				_rx_mask <<= 1;
				if(--_rx_bits == 0)
				{
					_rx_wait_stop_bit = 1;
				}
			}

			_rx_cnt = tmp;
		}
		else
		{
			if(!SUART_RX_READ())
			{
				_rx_ready = 1;
				_rx_byte = 0;
				_rx_cnt = 4;
				_rx_bits = 8;
				_rx_mask = 1;
			}
		}
	}
}

void suart_init(void)
{
	_tx_busy = 0;
	_rx_ready = 0;
	SUART_TX_HIGH();
	SUART_TX_DIR |= (1 << SUART_TX_PIN);
	SUART_RX_DIR &= ~(1 << SUART_RX_PIN);
	OCR0A = SUART_TIMER_TOP;
	TCCR0A = (1 << WGM01);
	TCCR0B = SUART_PRESCALER_MASK;
	TIMSK0 |= (1 << OCIE0A);
	TCNT0 = 0;
}

uint8_t suart_available(void)
{
	return _qin != _quot;
}

void suart_tx(const char c)
{
	while(_tx_busy) ;
	_tx_cnt = 3;
	_tx_bits = 10;
	_tx_byte = (c << 1) | 0x200;
	_tx_busy = 1;
}

int16_t suart_rx(void)
{
	int16_t c = -1;
	if(_quot != _qin)
	{
		c = _rx_buf[_quot];
		if(++_quot >= SUART_RX_BUF_SIZE)
		{
			_quot = 0;
		}
	}

	return c;
}

void suart_tx_str(const char *s)
{
	register char c;
	while((c = *s++))
	{
		suart_tx(c);
	}
}

void suart_tx_str_P(const char *s)
{
	char c;
	while((c = pgm_read_byte(s++)))
	{
		suart_tx(c);
	}
}

