#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "spi.h"
#include "gpio.h"
#include "lsm6ds3_read_data_polling.h"

#define DEV_SPI "/dev/spidev0.0"

#define SPI_SPEED 5000000
#define SPI_BITS_PER_WORD 8

int fd;
int ret;
unsigned int mode, speed;

int main(int argc, char *argv[])
{

    fd=spi_open( DEV_SPI, 0, SPI_BITS_PER_WORD, SPI_SPEED);
    
 	if (fd < 0) 
	{
 		printf("ERROR open %s ret=%d\n", DEV_SPI, fd);
 		return -1;
 	}

    ret = example_main_lsm6ds3(fd);
    
    if ( ret != 0)
      printf("Error IMU ID\n");
    
 	// close device node
 	close(fd);
    
 	return 0;
}

