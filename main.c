#include <avr/pgmspace.h>
#include "suart.c"

int main(void)
{
	int16_t c;
	suart_init();
	sei();
	suart_tx_str_P(PSTR("HELLO WORLD.\n"));
	for(;;)
	{
		c = suart_rx();
		if(c > 0)
		{
			suart_tx(c);
		}
	}

	return 0;
}
