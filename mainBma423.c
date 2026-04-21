/**
 * @file mainBma423.c
 * @date 20250526
 * @author flynnty
 * @copyright flynnty 2025
 *
 * @brief
 * Main entry point
 *
 */

/**** Includes ********************************************************************************************************/
#include "bma423.h"
#include "error.h"
#include "msgqBma423.h"
#include "wdtBma423.h"
#include "zephyrBma423.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/**** Defines *********************************************************************************************************/
/**** Types ***********************************************************************************************************/
/**** Variables *******************************************************************************************************/
/**** Macros **********************************************************************************************************/

LOG_MODULE_REGISTER( bma423_main, CONFIG_BMA423_LOG_LEVEL );

/**** Prototypes ******************************************************************************************************/

/**
 * @brief Module Thread
 *
 */
static int bma423Main( void );

/**** Definitions ****************************************************************************************************/

static int bma423Main( void )
{
    LOG_DBG( "BMA423 Thread started" );

    RETURN_ON_ERROR( wdtBma423Init() );

    // Populate bma4_dev runtime fields (chip_id, resolution, variant).
    // Required on every boot because the struct is in regular RAM, not RTC memory.
    // This is just one I2C read — no soft reset, config write, or step counter reset.
    struct bma4_dev *dev = bmaDevGet();
    RETURN_ON_ERROR( bma423_init( dev ) );

    // Re-cache the feature config base address from the BMA423 hardware.
    // The BMA423 retains this in registers 0x5B/0x5C across ESP32 deep sleep,
    // but the dev->asic_data cache in RAM is lost on every reboot.
    // Without this, feature config operations (e.g. step counter reset) corrupt
    // the address pointer by writing {0,0} to 0x5B/0x5C.
    uint8_t asicLsb = 0;
    uint8_t asicMsb = 0;
    RETURN_ON_ERROR( bma4_read_regs( BMA4_RESERVED_REG_5B_ADDR, &asicLsb, 1, dev ) );
    RETURN_ON_ERROR( bma4_read_regs( BMA4_RESERVED_REG_5C_ADDR, &asicMsb, 1, dev ) );
    dev->asic_data.asic_lsb = asicLsb & 0x0F;
    dev->asic_data.asic_msb = asicMsb;

    while( true ) {
        RETURN_ON_ERROR( wdtBma423Feed() );
        LOG_ON_ERROR( msgqBma423Proc() );
    }

    return ERR_OK;
}

/***************************************************************************************/

K_THREAD_DEFINE( bma423_main_id, CONFIG_BMA423_STACK_SIZE, bma423Main, NULL, NULL, NULL, CONFIG_BMA423_PRIORITY, 0, CONFIG_BMA423_DELAY_START_MS );
