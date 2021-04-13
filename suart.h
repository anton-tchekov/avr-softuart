#ifndef __SUART_H__
#define __SUART_H__

#include <avr/io.h>

#define SUART_BAUD_RATE    9600

#define SUART_RX_DIR           DDRB
#define SUART_RX_IN            PINB
#define SUART_RX_PIN          3

#define SUART_TX_DIR           DDRB
#define SUART_TX_OUT           PORTB
#define SUART_TX_PIN          4

void suart_init(void);
uint8_t suart_available(void);
void suart_tx(const char c);
int16_t suart_rx(void);
void suart_tx_str(const char *s);
void suart_tx_str_P(const char *s);

#endif

