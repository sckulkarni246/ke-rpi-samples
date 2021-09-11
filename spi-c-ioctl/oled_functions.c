#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "oled_functions.h"

extern uint8_t*	font_table[];
int fd;
struct spi_ioc_transfer trx;
uint32_t scratch32;


#define IO_DATA_CMD					(2)
#define IO_RESET					(3)
#define SPI_DEVICE          		"/dev/spidev0.0"

#define SYSFS_GPIO_PATH             "/sys/class/gpio"
#define SYSFS_GPIO_EXPORT_FN        "/export"
#define SYSFS_GPIO_UNEXPORT_FN      "/unexport"
#define SYSFS_GPIO_VALUE            "/value"
#define SYSFS_GPIO_DIRECTION        "/direction"

#define DIR_OUT                     "out"

#define VALUE_HIGH                  "1"
#define VALUE_LOW                   "0"

#define MODE_CMD					VALUE_LOW
#define MODE_DATA					VALUE_HIGH

/*      GPIO HELPER FUNCTIONS       */

// FILE OPERATION
static int file_open_and_write_value(const char *fname, const char *wdata) {
    int fd;

    fd = open(fname, O_WRONLY | O_NONBLOCK);
    if(fd < 0) {
        printf("Could not open file %s...%d\r\n",fname,fd);
    }
    write(fd,wdata,strlen(wdata));
    close(fd);

    return 0;
}

// GPIO EXPORT
int gpio_export(int gpio_num) {
    char gpio_str[4];
    sprintf(gpio_str,"%d",gpio_num);
    return file_open_and_write_value(SYSFS_GPIO_PATH SYSFS_GPIO_EXPORT_FN,gpio_str);
}

// GPIO UNEXPORT
int gpio_unexport(int gpio_num) {
    char gpio_str[4];
    sprintf(gpio_str,"%d",gpio_num);
    return file_open_and_write_value(SYSFS_GPIO_PATH SYSFS_GPIO_UNEXPORT_FN,gpio_str);
}

// GPIO DIRECTION
int gpio_set_direction(int gpio_num,const char* dir) {
    char path_str[40];
    sprintf(path_str,"%s/gpio%d%s",SYSFS_GPIO_PATH,gpio_num,SYSFS_GPIO_DIRECTION);
    return file_open_and_write_value(path_str,dir);
}

// GPIO SET VALUE
int gpio_set_value(int gpio_num, const char *value) {
    char path_str[40];
    sprintf(path_str,"%s/gpio%d%s",SYSFS_GPIO_PATH,gpio_num,SYSFS_GPIO_VALUE);
    return file_open_and_write_value(path_str,value);
}

// Configure IO lines for Display (RESET, D/C)
void gpio_configure_for_display(void) {
	gpio_unexport(IO_DATA_CMD);
    gpio_unexport(IO_RESET);

    gpio_export(IO_DATA_CMD);
    gpio_export(IO_RESET);

    gpio_set_direction(IO_DATA_CMD,DIR_OUT);	
    gpio_set_direction(IO_RESET,DIR_OUT);
    
    gpio_set_value(IO_DATA_CMD,VALUE_HIGH);	// Holding DC HIGH
    gpio_set_value(IO_RESET,VALUE_LOW);		// Holding RESET HIGH
}

// Conigure SPI for future usage
void spi_configure_for_display(void) {

	int ret;
	
    fd = open(SPI_DEVICE, O_RDWR);
    if(fd < 0) {
        printf("Could not open the SPI device...\r\n");
        exit(EXIT_FAILURE);
    }

    ret = ioctl(fd, SPI_IOC_RD_MODE32, &scratch32);
    if(ret != 0) {
        printf("Could not read SPI mode...\r\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    scratch32 |= SPI_MODE_0;

    ret = ioctl(fd, SPI_IOC_WR_MODE32, &scratch32);
    if(ret != 0) {
        printf("Could not write SPI mode...\r\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &scratch32);
    if(ret != 0) {
        printf("Could not read the SPI max speed...\r\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    scratch32 = 100000;

    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &scratch32);
    if(ret != 0) {
        printf("Could not write the SPI max speed...\r\n");
        close(fd);
        exit(EXIT_FAILURE);
    }
	
}

void spi_write_1_byte(uint8_t val) {
	uint8_t tx_buffer[4];
	int ret;
	
	tx_buffer[0] = val;
	
    trx.tx_buf = (unsigned long)tx_buffer;
    trx.rx_buf = (unsigned long)NULL;
    trx.bits_per_word = 0;
    trx.speed_hz = 100000;
    trx.delay_usecs = 0;
    trx.len = 1;

    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &trx);
    if(ret != 0) {
        printf("SPI transfer returned %d...\r\n", ret);
    }
}

// Write a command to the display
void display_write_command(uint8_t cmd) {
	gpio_set_value(IO_DATA_CMD,MODE_CMD);
	// DO SPI WRITE - TBD
	spi_write_1_byte(cmd);
}

// Write a data byte to the display
void display_write_data(uint8_t data) {
	gpio_set_value(IO_DATA_CMD,MODE_DATA);
	// DO SPI WRITE - TBD
	spi_write_1_byte(data);
}

// Hard Reset
void display_do_reset(void) {
	gpio_set_value(IO_RESET,VALUE_LOW);
	usleep(10000);
	gpio_set_value(IO_RESET,VALUE_HIGH);
}

// Turn Display ON
void display_turn_on(void) {
	display_write_command(SSD1306_CMD_SET_DISPLAY_ON);
}

// Turn Display OFF
void display_turn_off(void) {
	display_write_command(SSD1306_CMD_SET_DISPLAY_OFF);
}

// Set Display Contrast
void display_set_contrast(uint8_t contrast) {
    display_write_command(SSD1306_CMD_SET_CONTRAST_CONTROL_FOR_BANK0);
    display_write_command(contrast);
}

// Disable invert
void display_invert_disable(void) {
	display_write_command(SSD1306_CMD_SET_NORMAL_DISPLAY);
}

// Set page address
void display_set_page_address(uint8_t address) {
    // Make sure that the address is 4 bits (only 8 pages)
    address &= 0x0F;
    display_write_command(SSD1306_CMD_SET_PAGE_START_ADDRESS(address));
	
}

// Set column address
void display_set_column_address(uint8_t address) {
    // Make sure the address is 7 bits
    address &= 0x7F;
    display_write_command(SSD1306_CMD_SET_HIGH_COL(address >> 4));
    display_write_command(SSD1306_CMD_SET_LOW_COL(address & 0x0F));	
}

// Clear the display
void display_clear(void)
{
    uint8_t page = 0;
    uint8_t col  = 0;

	for (page = 0; page < 4; ++page) {
		display_set_page_address(page);
		display_set_column_address(0);
		for (col = 0; col < 128; ++col) {
			display_write_data(0x00);
		}
	}
}

// Initialize the display
void display_do_init(void) {

	// Do a hard reset of the OLED display controller
    display_do_reset();
    
    // 1/32 Duty (0x0F~0x3F)
    display_write_command(SSD1306_CMD_SET_MULTIPLEX_RATIO);
    display_write_command(0x1F);

    // Shift Mapping RAM Counter (0x00~0x3F)
    display_write_command(SSD1306_CMD_SET_DISPLAY_OFFSET);
    display_write_command(0x00);

    // Set Mapping RAM Display Start Line (0x00~0x3F)
    display_write_command(SSD1306_CMD_SET_START_LINE(0x00));

    // Set Column Address 0 Mapped to SEG0
    display_write_command(SSD1306_CMD_SET_SEGMENT_RE_MAP_COL127_SEG0);

    // Set COM/Row Scan Scan from COM63 to 0
    display_write_command(SSD1306_CMD_SET_COM_OUTPUT_SCAN_DOWN);

    // Set COM Pins hardware configuration
    display_write_command(SSD1306_CMD_SET_COM_PINS);
    display_write_command(0x02);

    display_set_contrast(0x8F);

    // Disable Entire display On
    display_write_command(SSD1306_CMD_ENTIRE_DISPLAY_AND_GDDRAM_ON);

    display_invert_disable();

    // Set Display Clock Divide Ratio / Oscillator Frequency (Default => 0x80)
    display_write_command(SSD1306_CMD_SET_DISPLAY_CLOCK_DIVIDE_RATIO);
    display_write_command(0x80);

    // Enable charge pump regulator
    display_write_command(SSD1306_CMD_SET_CHARGE_PUMP_SETTING);
    display_write_command(0x14);

    // Set VCOMH Deselect Level
    display_write_command(SSD1306_CMD_SET_VCOMH_DESELECT_LEVEL);
    display_write_command(0x40); // Default => 0x20 (0.77*VCC)

    // Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    display_write_command(SSD1306_CMD_SET_PRE_CHARGE_PERIOD);
    display_write_command(0xF1);

    display_turn_on();
    
    display_clear();
	
}

// Write a string to the display - we use a FONT TABLE for this
void display_write_string(const char* string) {
	const uint8_t *char_ptr;
    uint8_t            	i;

    while (*string != 0) {
		if (*string < 0x7F) {
			char_ptr = font_table[*string - 32];
			for (i = 1; i <= char_ptr[0]; i++) {
				display_write_data(char_ptr[i]);
			}
			display_write_data(0x00);
		}
		string++;
    }
	
}












