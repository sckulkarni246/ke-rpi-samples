#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "oled_functions.h"

int main(void) {
	int looper;
	printf("Welcome to OLED Demo...\r\n");
	
	gpio_configure_for_display();
	spi_configure_for_display();
	
	display_do_init();
	
	for(looper=0; looper<10; ++looper) {
		display_write_string("<==   KickstartEmbedded   ==>");
		usleep(500000);
		display_clear();

		display_write_string("<==          Subscribe          ==>");
		usleep(500000);
		display_clear();

	}
	
	return 0;
}
