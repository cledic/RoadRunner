#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>

#include "spi.h"

int spi_open(char *name, uint32_t mode, uint8_t bits, uint32_t speed)
{
 int fd;
 int err;

 fd=open(name, O_RDWR | O_NONBLOCK);
 if(fd < 0)
     return -1;

 err=ioctl(fd, SPI_IOC_WR_MODE32, &mode);
 if(!err)
     err=ioctl(fd, SPI_IOC_RD_MODE32, &mode);
 if(!err)
     err=ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
 if(!err)
     err=ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
 if(!err)
     err=ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
 if(!err)
     err=ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);

 if(!err)
     return fd;
 close(fd);
 return -1;
}

ssize_t spi_transfer(int fd, void *out, void *in, size_t len)
{
 struct spi_ioc_transfer buff;

 memset(&buff, 0, sizeof(buff));
 buff.tx_buf= (unsigned long)out;
 buff.rx_buf= (unsigned long)in;
 buff.len=len;
 buff.delay_usecs=0;
 if(ioctl(fd, SPI_IOC_MESSAGE(1), &buff) < 0)
     return -1;
 return len;
}

int spi_close(int fd)
{
 return close(fd);
}
