/******************************************************************************
 * @file    _read_data_polling.c
 * @author  Sensors Software Solution Team
 * @brief   This file show the simplest way to get data from sensor.
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/*
 * This example was developed using the following STMicroelectronics
 * evaluation boards:
 *
 * - STEVAL_MKI109V3
 * - NUCLEO_F411RE
 * - DISCOVERY_SPC584B
 *
 * Used interfaces:
 *
 * STEVAL_MKI109V3    - Host side:   USB (Virtual COM)
 *                    - Sensor side: SPI(Default) / I2C(supported)
 *
 * NUCLEO_STM32F411RE - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I2C(Default) / SPI(supported)
 *
 * DISCOVERY_SPC584B  - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I2C(Default) / SPI(supported)
 *
 * If you need to run this example on a different hardware platform a
 * modification of the functions: `platform_write`, `platform_read`,
 * `tx_com` and 'platform_init' is required.
 *
 */

/* STMicroelectronics evaluation boards definition
 *
 * Please uncomment ONLY the evaluation boards in use.
 * If a different hardware is used please comment all
 * following target board and redefine yours.
 */

//#define STEVAL_MKI109V3  /* little endian */
//#define NUCLEO_F411RE    /* little endian */
//#define SPC584B_DIS      /* big endian */
#define ROAD_RUNNER

/* ATTENTION: By default the driver is little endian. If you need switch
 *            to big endian please see "Endianness definitions" in the
 *            header file of the driver (_reg.h).
 */

#if defined(STEVAL_MKI109V3)
/* MKI109V3: Define communication interface */
#define SENSOR_BUS hspi2
/* MKI109V3: Vdd and Vddio power supply values */
#define PWM_3V3 915

#elif defined(NUCLEO_F411RE)
/* NUCLEO_F411RE: Define communication interface */
#define SENSOR_BUS hi2c1

#elif defined(SPC584B_DIS)
/* DISCOVERY_SPC584B: Define communication interface */
#define SENSOR_BUS I2CD1

#endif

/* Includes ------------------------------------------------------------------*/
#include "lsm6ds3_reg.h"
#include <string.h>
#include <stdio.h>

#if defined(NUCLEO_F411RE)
#include "stm32f4xx_hal.h"
#include "usart.h"
#include "gpio.h"
#include "i2c.h"

#elif defined(STEVAL_MKI109V3)
#include "stm32f4xx_hal.h"
#include "usbd_cdc_if.h"
#include "gpio.h"
#include "spi.h"
#include "tim.h"

#elif defined(SPC584B_DIS)
#include "components.h"

#elif defined(ROAD_RUNNER)
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdlib.h>
#include <errno.h>
#include <libexplain/ioctl.h>
#include "lsm6ds3_read_data_polling.h"
#endif

/* Private macro -------------------------------------------------------------*/
#define    BOOT_TIME   20 //ms

/* Private variables ---------------------------------------------------------*/
static int16_t data_raw_acceleration[3];
static int16_t data_raw_angular_rate[3];
static int16_t data_raw_temperature;
static float acceleration_mg[3];
static float angular_rate_mdps[3];
static float temperature_degC;
static uint8_t whoamI, rst;
static uint8_t tx_buffer[1000];

/* Extern variables ----------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/*   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static int32_t platform_write(void *handle, uint8_t reg,
                              uint8_t *bufp,
                              uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void tx_com( uint8_t *tx_buffer, uint16_t len );
static void platform_delay(uint32_t ms);
static void platform_init(void);

/* Main Example --------------------------------------------------------------*/
int32_t example_main_lsm6ds3(int handle)
{

  /* Initialize mems driver interface */
  stmdev_ctx_t dev_ctx;
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.handle = handle;
  /* Init test platform */
  platform_init();
  /* Wait sensor boot time */
  platform_delay(BOOT_TIME);
  /* Check device ID */
  uint32_t i=0;
  do
  {
    platform_delay(50);
    lsm6ds3_device_id_get(&dev_ctx, &whoamI);
    i++;
  } while( (whoamI != LSM6DS3_ID) && i<3);
  
  printf("ID: %02X [%d]\n", whoamI, i);

  if (whoamI != LSM6DS3_ID)
  {
     return -1;
  }
  
  /* Restore default configuration */
  lsm6ds3_reset_set(&dev_ctx, PROPERTY_ENABLE);
  platform_delay(1);
  
  lsm6ds3_reset_set(&dev_ctx, PROPERTY_ENABLE);
  platform_delay(1);
  
  lsm6ds3_reset_get(&dev_ctx, &rst);
  
  /*  Enable Block Data Update */
  lsm6ds3_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
  /* Set full scale */
  lsm6ds3_xl_full_scale_set(&dev_ctx, LSM6DS3_2g); 
  lsm6ds3_gy_full_scale_set(&dev_ctx, LSM6DS3_2000dps); 
  /* Set Output Data Rate for Acc and Gyro */
  lsm6ds3_xl_data_rate_set(&dev_ctx, LSM6DS3_XL_ODR_12Hz5);
  lsm6ds3_gy_data_rate_set(&dev_ctx, LSM6DS3_GY_ODR_12Hz5);
  
  /* Read samples in polling mode (no int) */
  while (1) 
  {
    uint8_t reg;
    /* Read output only if new value is available */
    lsm6ds3_xl_flag_data_ready_get(&dev_ctx, &reg);

    if (reg) 
    {
      /* Read acceleration field data */
      memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
      lsm6ds3_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
      acceleration_mg[0] =
        lsm6ds3_from_fs2g_to_mg(data_raw_acceleration[0]);
      acceleration_mg[1] =
        lsm6ds3_from_fs2g_to_mg(data_raw_acceleration[1]);
      acceleration_mg[2] =
        lsm6ds3_from_fs2g_to_mg(data_raw_acceleration[2]);
      sprintf((char *)tx_buffer,
              "Acceleration [mg]:%4.2f\t%4.2f\t%4.2f\r\n",
              acceleration_mg[0], acceleration_mg[1], acceleration_mg[2]);
      tx_com(tx_buffer, strlen((char const *)tx_buffer));
    }

    lsm6ds3_gy_flag_data_ready_get(&dev_ctx, &reg);

    if (reg) 
    {
      /* Read angular rate field data */
      memset(data_raw_angular_rate, 0x00, 3 * sizeof(int16_t));
      lsm6ds3_angular_rate_raw_get(&dev_ctx, data_raw_angular_rate);
      angular_rate_mdps[0] =
        lsm6ds3_from_fs2000dps_to_mdps(data_raw_angular_rate[0]);
      angular_rate_mdps[1] =
        lsm6ds3_from_fs2000dps_to_mdps(data_raw_angular_rate[1]);
      angular_rate_mdps[2] =
        lsm6ds3_from_fs2000dps_to_mdps(data_raw_angular_rate[2]);
      sprintf((char *)tx_buffer,
              "Angular rate [mdps]:%4.2f\t%4.2f\t%4.2f\r\n",
              angular_rate_mdps[0],
              angular_rate_mdps[1],
              angular_rate_mdps[2]);
      tx_com(tx_buffer, strlen((char const *)tx_buffer));
    }

    lsm6ds3_temp_flag_data_ready_get(&dev_ctx, &reg);

    if (reg) 
    {
      /* Read temperature data */
      memset(&data_raw_temperature, 0x00, sizeof(int16_t));
      lsm6ds3_temperature_raw_get(&dev_ctx, &data_raw_temperature);
      temperature_degC =
        lsm6ds3_from_lsb_to_celsius(data_raw_temperature);
      sprintf((char *)tx_buffer,
              "Temperature [degC]:%6.2f\r\n",
              temperature_degC);
      tx_com(tx_buffer, strlen((char const *)tx_buffer));
    }
  }
}

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg,
                              uint8_t *bufp,
                              uint16_t len)
{
#if defined(NUCLEO_F411RE)
  HAL_I2C_Mem_Write(handle, LSM6DS3_I2C_ADD_L, reg,
                    I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Transmit(handle, bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_write(handle,  LSM6DS3_I2C_ADD_L & 0xFE, reg, bufp, len);
  
#elif defined(ROAD_RUNNER)
  int ret;
  struct spi_ioc_transfer xfer;
  uint8_t*tx_buf;
  
  memset(&xfer, 0, sizeof(xfer));
  
  /* Alloco la memoria per il buffer + 1 byte del registro */
  /* che dovrò inviare.                                    */
  tx_buf=malloc(len+1);
  if ( tx_buf == (uint8_t*)NULL) return -1;
  /* Carico il registro ed a seguire copio il buffer da inviare */
  tx_buf[0] = reg;
  memcpy(&tx_buf[1], bufp, len);
  
  xfer.tx_buf=(unsigned long)tx_buf;
  xfer.len=len+1;
  xfer.speed_hz = 5000000;
  xfer.bits_per_word = 8;
  xfer.cs_change = 0;
  
  if(ioctl(handle, SPI_IOC_MESSAGE(1), &xfer) < 0)
  {
   free(tx_buf);
   return -1;  
  }
  
  free(tx_buf);
#endif
  return 0;
}

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len)
{
#if defined(NUCLEO_F411RE)
  HAL_I2C_Mem_Read(handle, LSM6DS3_I2C_ADD_L, reg,
                   I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  reg |= 0x80;
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Receive(handle, bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_read(handle, LSM6DS3_I2C_ADD_L & 0xFE, reg, bufp, len);
  
#elif defined(ROAD_RUNNER)
  int ret;
  uint8_t tx_buf[1];
  struct spi_ioc_transfer xfer[1] = {0};
  
  reg |= 0x80;

  memset(&xfer, 0, sizeof(xfer));
  
  tx_buf[0]=reg;
  xfer[0].tx_buf=(unsigned long)tx_buf;
  xfer[0].len=1;
  xfer[0].speed_hz = 5000000;
  xfer[0].bits_per_word = 8;
  xfer[0].cs_change = 1;		// In questa maniera il CS rimane attivo e 
					            // l'invio dei byte seguenti rimangono nella 
                                // stessa transazione.

  if((ret=ioctl(handle, SPI_IOC_MESSAGE(1), xfer))<0)
  {
     int errsv = errno;
     fprintf(stderr, "%s\n", explain_ioctl(handle, SPI_IOC_MESSAGE(1), xfer));
     printf("ERRORE ioctl [%d]\n", strerror(errsv));
     return -1;  
  }
  
  memset(&xfer, 0, sizeof(xfer));

  xfer[0].rx_buf=(unsigned long)bufp;
  xfer[0].len=len;
  xfer[0].speed_hz = 5000000;
  xfer[0].bits_per_word = 8;
  xfer[0].cs_change = 0;		// Il CS a fine invio torna inattivo.

  if((ret=ioctl(handle, SPI_IOC_MESSAGE(1), xfer))<0)
  {
     int errsv = errno;
     fprintf(stderr, "%s\n", explain_ioctl(handle, SPI_IOC_MESSAGE(1), xfer));
     printf("ERRORE ioctl [%d]\n", strerror(errsv));
     return -1;  
  }

  return 0;  
#endif
  return 0;
}

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  tx_buffer     buffer to transmit
 * @param  len           number of byte to send
 *
 */
static void tx_com(uint8_t *tx_buffer, uint16_t len)
{
#if defined(NUCLEO_F411RE)
  HAL_UART_Transmit(&huart2, tx_buffer, len, 1000);
#elif defined(STEVAL_MKI109V3)
  CDC_Transmit_FS(tx_buffer, len);
#elif defined(SPC584B_DIS)
  sd_lld_write(&SD2, tx_buffer, len);
  
#elif defined(ROAD_RUNNER)
  printf( "%s", tx_buffer);
#endif
}

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 *
 */
static void platform_delay(uint32_t ms)
{
#if defined(NUCLEO_F411RE) | defined(STEVAL_MKI109V3)
  HAL_Delay(ms);
#elif defined(SPC584B_DIS)
  osalThreadDelayMilliseconds(ms);
  
#elif defined(ROAD_RUNNER)
  usleep(ms*1000);
#endif
}

/*
 * @brief  platform specific initialization (platform dependent)
 */
static void platform_init(void)
{
#if defined(STEVAL_MKI109V3)
  TIM3->CCR1 = PWM_3V3;
  TIM3->CCR2 = PWM_3V3;
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_Delay(1000);
#endif
}

