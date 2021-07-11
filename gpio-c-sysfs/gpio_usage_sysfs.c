#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#define SYSFS_GPIO_PATH             "/sys/class/gpio"
#define SYSFS_GPIO_EXPORT_FN        "/export"
#define SYSFS_GPIO_UNEXPORT_FN      "/unexport"
#define SYSFS_GPIO_VALUE            "/value"
#define SYSFS_GPIO_DIRECTION        "/direction"
#define SYSFS_GPIO_EDGE             "/edge"

#define DIR_IN                      "in"
#define DIR_OUT                     "out"

#define VALUE_HIGH                  "1"
#define VALUE_LOW                   "0"

#define EDGE_RISING                 "rising"
#define EDGE_FALLING                "falling"

#define POLL_TIMEOUT        10*1000

/*      HELPER FUNCTIONS       */

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

// GPIO SET EDGE
int gpio_set_edge(int gpio_num, const char* edge) {
    char path_str[40];
    sprintf(path_str,"%s/gpio%d%s",SYSFS_GPIO_PATH,gpio_num,SYSFS_GPIO_EDGE);
    return file_open_and_write_value(path_str,edge);
}

int gpio_get_fd_to_value(int gpio_num) {
    int fd;
    char fname[40];
    sprintf(fname,"%s/gpio%d%s",SYSFS_GPIO_PATH,gpio_num,SYSFS_GPIO_VALUE);
    fd = open(fname, O_RDONLY | O_NONBLOCK);
    if(fd < 0) {
        printf("Could not open file %s...%d\r\n",fname,fd);
    }
    return fd;
}

int main(int argc, char **argv) {
    unsigned int gpio_out;
    struct pollfd fdpoll;
    int num_fdpoll = 1;
    int gpio_in, gpio_in_fd;
    int res;
    int looper = 0;
    char *buf[64];

    if(argc<3) {
        printf("Usage: gpio_usage_sysfs <gpio-out> <gpio-in>\r\n");
        exit(-1);
    }

    // We reached here so params are OK - not checking for a legal number!
    gpio_out = atoi(argv[1]);
    gpio_in = atoi(argv[2]);

    gpio_unexport(gpio_out);
    gpio_unexport(gpio_in);

    gpio_export(gpio_out);
    gpio_export(gpio_in);

    gpio_set_direction(gpio_out,DIR_OUT);
    gpio_set_direction(gpio_in,DIR_IN);

    gpio_set_value(gpio_out,VALUE_HIGH);
    gpio_set_edge(gpio_in,EDGE_FALLING);

    gpio_in_fd = gpio_get_fd_to_value(gpio_in);

    // We will wait for button press here for 10s or exit anyway
    while(looper<10) {
        memset((void *)&fdpoll,0,sizeof(fdpoll));
        fdpoll.fd = gpio_in_fd;
        fdpoll.events = POLLPRI;

        res = poll(&fdpoll,num_fdpoll,POLL_TIMEOUT);

        if(res < 0) {
            printf("Poll failed...%d\r\n",res);            
        }
        if(res == 0) {
            printf("Poll success...timed out or received button press...\r\n");
        }
        if(fdpoll.revents & POLLPRI) {
			lseek(fdpoll.fd, 0, SEEK_SET);
			read(fdpoll.fd, buf, 64);
            printf("Received a button press...%d\r\n",looper);
        }
        ++looper;
        fflush(stdout);
    }

    close(gpio_in_fd);
    gpio_set_value(gpio_out,VALUE_LOW);
    gpio_unexport(gpio_out);
    gpio_unexport(gpio_in);

    return 0;
}