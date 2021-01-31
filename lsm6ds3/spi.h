#ifndef SPI_H
#include <stdint.h>
#include <unistd.h>

int spi_open(char *, uint32_t, uint8_t, uint32_t);
ssize_t spi_transfer(int , void *, void *, size_t);
int spi_close(int);

#endif
