/**
 * @file bma423.c
 * @date 20250526
 * @author flynnty
 * @copyright flynnty 2025
 *
 * @brief
 * Module Common
 *
 */

/**** Includes ********************************************************************************************************/
#include "bma423.h"
#include "zephyrBma423.h"
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

/**** Defines *********************************************************************************************************/

#define BMA423_NODE         DT_NODELABEL( bma423 )
#define BMA423_I2C_BUS      DEVICE_DT_GET( DT_BUS( BMA423_NODE ) )
#define BMA423_NODE_ADDRESS DT_REG_ADDR( BMA423_NODE )

/**** Types ***********************************************************************************************************/
/**** Prototypes ******************************************************************************************************/

static uint16_t i2cRead( uint8_t dev_addr, uint8_t reg_addr, uint8_t *read_data, uint16_t len );
static uint16_t i2cWrite( uint8_t dev_addr, uint8_t reg_addr, uint8_t *read_data, uint16_t len );
static void zephyrDelay( uint32_t delayMs );

/**** Variables *******************************************************************************************************/

static const struct device *i2cDev = BMA423_I2C_BUS;

static struct bma4_dev bma4Dev = {
    .dev_addr = (uint8_t)BMA423_NODE_ADDRESS,
    .interface = BMA4_I2C_INTERFACE,
    .bus_read = i2cRead,
    .bus_write = i2cWrite,
    .delay = zephyrDelay,
    .read_write_len = 8,
    .resolution = 12,
    .feature_len = BMA423_FEATURE_SIZE,
};

/**** Macros **********************************************************************************************************/
LOG_MODULE_DECLARE( bma423_main, CONFIG_BMA423_LOG_LEVEL );

/**** Definitions *****************************************************************************************************/

struct bma4_dev *bmaDevGet()
{
    return &bma4Dev;
}

/**** Static Functions ************************************************************************************************/

static uint16_t i2cRead( uint8_t dev_addr, uint8_t reg_addr, uint8_t *read_data, uint16_t len )
{
    return i2c_burst_read( i2cDev, dev_addr, reg_addr, read_data, len );
}

static uint16_t i2cWrite( uint8_t dev_addr, uint8_t reg_addr, uint8_t *read_data, uint16_t len )
{
    return i2c_burst_write( i2cDev, dev_addr, reg_addr, read_data, len );
}

static void zephyrDelay( uint32_t delayMs )
{
    k_msleep( (int32_t)delayMs );
}
