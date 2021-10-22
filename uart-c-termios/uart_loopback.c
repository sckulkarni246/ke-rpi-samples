#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#define SERIAL_PORT_PATH        "/dev/ttyS0"

struct termios g_tty;
int g_fd;

// FILE OPERATION
static int file_open_and_get_descriptor(const char *fname) {
    int fd;

    fd = open(fname, O_RDWR | O_NONBLOCK);
    if(fd < 0) {
        printf("Could not open file %s...%d\r\n",fname,fd);
    }
    return fd;
}

static int file_write_data(int fd, uint8_t *buff, uint32_t len_buff) {
    return write(fd,buff,len_buff);
}

static int file_read_data(int fd, uint8_t *buff, uint32_t len_buff) {
    return read(fd,buff,len_buff);
}

static int file_close(int fd) {
    return close(fd);
}


static void open_serial_port(void) {
    g_fd = file_open_and_get_descriptor(SERIAL_PORT_PATH);
    if(g_fd < 0) {
        printf("Something went wrong while opening the port...\r\n");
        exit(EXIT_FAILURE);
    }
}

static void configure_serial_port(void) {
    if(tcgetattr(g_fd, &g_tty)) {
        printf("Something went wrong while getting port attributes...\r\n");
        exit(EXIT_FAILURE);
    }

    cfsetispeed(&g_tty,B115200);
    cfsetospeed(&g_tty,B115200);

    cfmakeraw(&g_tty);

    if(tcsetattr(g_fd,TCSANOW,&g_tty)) {
        printf("Something went wrong while setting port attributes...\r\n");
        exit(EXIT_FAILURE);
    }
}

static void perform_demo(void) {
    uint8_t l_buff[256];
    uint32_t l_len_buff = 256;
    uint32_t l_looper;
    
    for(l_looper=0; l_looper<l_len_buff; ++l_looper)
        l_buff[l_looper] = l_looper;

    file_write_data(g_fd,l_buff,l_len_buff);
    sleep(1);
    memset(l_buff,0,l_len_buff);
    file_read_data(g_fd,l_buff,l_len_buff);

    for(l_looper=0; l_looper<l_len_buff; ++l_looper) {
        printf("%02x",l_buff[l_looper]);
        if(l_buff[l_looper] != l_looper) {
        printf("\r\nSomething went wrong in the loopback data check...%d and %d\r\n",l_buff[l_looper],l_looper) ;
            exit(EXIT_FAILURE);
        }
    }
    printf("\r\nThe data loopback was successful!\r\n");
}

static void close_serial_port(void) {
    file_close(g_fd);
}

int main(void) {
    printf("Starting the loopback application...\r\n");

    open_serial_port();

    configure_serial_port();

    perform_demo();

    close_serial_port();

    return 0;
}

