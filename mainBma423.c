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
    RETURN_ON_ERROR( bma423_init( bmaDevGet() ) );

    while( true ) {
        RETURN_ON_ERROR( wdtBma423Feed() );
        LOG_ON_ERROR( msgqBma423Proc() );
    }

    return ERR_OK;
}

/***************************************************************************************/

K_THREAD_DEFINE( bma423_main_id, CONFIG_BMA423_STACK_SIZE, bma423Main, NULL, NULL, NULL, CONFIG_BMA423_PRIORITY, 0, CONFIG_BMA423_DELAY_START_MS );
