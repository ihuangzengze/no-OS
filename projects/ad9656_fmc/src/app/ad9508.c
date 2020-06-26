/***************************************************************************//**
 * @file ad9508.c
 * @brief Implementation of AD9508 Driver.
 * @author DHotolea (dan.hotoleanu@analog.com)
 ********************************************************************************
 * Copyright 2020(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * - Neither the name of Analog Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * - The use of this software may or may not infringe the patent rights
 * of one or more patent holders. This license does not release you
 * from the requirement that you obtain separate licenses from these
 * patent holders to use this software.
 * - Use of the software either in source or binary form, must be run
 * on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "error.h"
#include "ad9508.h"
#include "delay.h"

/**
 * @brief Reads from the ad9508 that is contected to the SPI
 * @param dev - The ad9508 device handler
 * @param reg_addr - The address of the internal register of the ad9508 chip
 * @param reg_data - The value read from the internal register
 * @return SUCCESS if the value was successfully read, FAILURE otherwise
 */
int32_t ad9508_spi_read(struct ad9508_dev *dev,
			uint16_t reg_addr,
			uint8_t *reg_data)
{
	uint8_t buf[3];

	int32_t ret;

	// the MSB of byte 0 indicates a r/w operation, following 7 bits are the
	// bits 14-8 of the address of the register that is accessed. Byte 1
	// contains the bits 7-0 of the address of the register.
	buf[0] = 0x80 | (reg_addr >> 8);
	buf[1] = reg_addr & 0xFF;
	buf[2] = 0x00;

	ret = spi_write_and_read(dev->spi_desc, buf, 3);

	if (ret < 0)
		return ret;

	*reg_data = buf[2];

	return ret;
}

/**
 * @brief Write to the ad9508 that is conected to the SPI
 * @param dev - The device handler for the ad9508 chip
 * @param reg_addr - Address of the internal register of the ad9508 chip
 * @param reg_data - Value to be written to the register
 * @return SUCCESS if the value was written successfully, FAILURE otherwise
 */
int32_t ad9508_spi_write(struct ad9508_dev *dev,
			 uint16_t reg_addr,
			 uint8_t reg_data)
{
	uint8_t buf[3];

	// the MSB of byte 0 indicates a r/w operation, following 7 bits are the
	// bits 14-8 of the address of the register that is accessed. Byte 1
	// contains the bits 7-0 of the address of the register. Byte 2 contains
	// the data to be written.
	buf[0] = reg_addr >> 8;
	buf[1] = reg_addr & 0xFF;
	buf[2] = reg_data;

	return spi_write_and_read(dev->spi_desc, buf, 3);
}

/**
 * @brief Setup the working parameters of the ad9508 chip
 * @param device - The device handler of the ad9508 chip
 * @param init_param - Values for the working parameters of ad9508
 * @return SUCCESS if device is ready for use, FAILURE otherwise
 */
int ad9508_setup(struct ad9508_dev **device,
		 const struct ad9508_init_param *init_param)
{
	int32_t ret;

	struct ad9508_dev *dev;
	uint8_t reg_data;

	dev = (struct ad9508_dev *)malloc(sizeof(*dev));
	if (!dev)
		return FAILURE;

	/* SPI */
	ret = spi_init(&dev->spi_desc, &init_param->spi_init);
	if (ret != SUCCESS)
		return ret;

	// reset
	ad9508_spi_write(dev, AD9508_SPI_CONFIG, 0x24);
	mdelay(250);

	// read family part id: 0x0C contains the least significant byte,
	//                      0x0D contains the most significant byte
	ad9508_spi_read(dev, AD9508_PART_ID_LOW, &reg_data);
	if (reg_data != (AD9508_PART_ID_VALUE & 0xFF))
		return FAILURE;
	ad9508_spi_read(dev, AD9508_PART_ID_HIGH, &reg_data);
	if (reg_data != (AD9508_PART_ID_VALUE >> 8))
		return FAILURE;

	// configure 9508 to pass the 125MHz input clock unmodified, so divider = 1
	// no phase offset
	ad9508_spi_write(dev, AD9508_OUT1_DIV_RATIO_LOW,
			 AD9508_DIVIDE_RATIO_ONE); // divide ratio[7:0]
	ad9508_spi_write(dev, AD9508_OUT1_DIV_RATIO_HIGH,
			 AD9508_DIVIDE_RATIO_ONE); // divide ratio[9:8]
	ad9508_spi_write(dev, AD9508_OUT1_PHASE_LOW,
			 AD9508_DIVIDE_RATIO_ONE); // phase offset[7:0]
	ad9508_spi_write(dev, AD9508_OUT1_PHASE_HIGH,
			 AD9508_DIVIDE_RATIO_ONE); // phase offset[10:8]

	return SUCCESS;
}

/**
 * @brief Free the resources allocated by ad9508_setup().
 * @param dev - The device structure.
 * @return SUCCESS in case of success, negative error code otherwise.
 */
int32_t ad9508_remove(struct ad9508_dev *dev)
{
	int32_t ret;

	ret = spi_remove(dev->spi_desc);

	free(dev);

	return ret;
}