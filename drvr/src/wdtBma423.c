/**
 * @file wdtBma423.c
 * @date 20250526
 * @author flynnty
 * @copyright flynnty 2025
 *
 * @brief
 * Task Watch Dog Timer Module
 *
 */

/**** Includes ********************************************************************************************************/
#include "wdtBma423.h"
#include "zbusCommon.h"
#include <zephyr/logging/log.h>
#include <zephyr/task_wdt/task_wdt.h>

/**** Defines *********************************************************************************************************/
/**** Types ***********************************************************************************************************/
/**** Variables *******************************************************************************************************/
static int wdtTaskId = -1;
static const uint32_t wdtTimeoutMs = ( CONFIG_BMA423_WDT_TIMEOUT_S * MSEC_PER_SEC );

/**** Macros **********************************************************************************************************/
LOG_MODULE_DECLARE( bma423_main, CONFIG_BMA423_LOG_LEVEL );

BUILD_ASSERT( CONFIG_BMA423_WDT_TIMEOUT_S > CONFIG_BMA423_MSGQ_TIMEOUT_S, "Watchdog timeout must be greater than trigger timeout" );

/**** Prototypes ******************************************************************************************************/

/**
 * @brief Time WDT timeout callback
 *
 * @param channel_id
 * @param user_data
 */
static void wdtTimeoutCb( int chanId, void *userData );

/**** Definitions *****************************************************************************************************/

void wdtTimeoutCb( int chanId, void *userData )
{
    LOG_ERR( "Watchdog expired, Channel: %d, Thread: %s", chanId, k_thread_name_get( (k_tid_t)userData ) );

    // Delete the watchdog task to prevent repeating the timeout callback.
    task_wdt_delete( wdtTaskId );

    // Inform other threads
    ZbusMsgWdt msg = { .sourceThreadId = (k_tid_t)userData };
    zbus_chan_pub( &ZBUS_CHAN_WDT_FAULT_SUB, &msg, K_NO_WAIT );
}

int wdtBma423Init()
{
    int ret = ERR_OK;

    ret = task_wdt_add( wdtTimeoutMs, wdtTimeoutCb, (void *)k_current_get() );
    if( ret < ERR_OK ) {
        LOG_ERR( "Failed to add task to watchdog: %d", wdtTaskId );
        return ret;
    }
    wdtTaskId = ret;

    return ERR_OK;
}

int wdtBma423Feed()
{
    int ret = ERR_OK;

    ret = task_wdt_feed( wdtTaskId );
    if( ret < ERR_OK ) {
        LOG_ERR( "Failed to feed the watchdog: %d", ret );
        return ret;
    }
    return ret;
}
