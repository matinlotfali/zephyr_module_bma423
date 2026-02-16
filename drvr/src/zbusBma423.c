/**
 * @file zbusBma423.c
 * @date 20250526
 * @author flynnty
 * @copyright flynnty 2025
 *
 * @brief
 * BMA423 Module Zbus Listener
 *
 */

/**** Includes ********************************************************************************************************/
#include "zbusBma423.h"

#include "error.h"
#include "msgqBma423.h"
#include "zbusCommon.h"
#include <zephyr/logging/log.h>

/**** Defines *********************************************************************************************************/
/**** Types ***********************************************************************************************************/
/**** Variables *******************************************************************************************************/
/**** Prototypes ******************************************************************************************************/
static void zbusListenerCb( const struct zbus_channel *chan );

/**** Macros **********************************************************************************************************/
LOG_MODULE_DECLARE( bma423_main, CONFIG_BMA423_LOG_LEVEL );

ZBUS_LISTENER_DEFINE( bma423_thread_lis, zbusListenerCb );
ZBUS_CHAN_ADD_OBS( ZBUS_CHAN_BMA423_REQ, bma423_thread_lis, 0 );

/**** Definitions *****************************************************************************************************/

void zbusListenerCb( const struct zbus_channel *chan )
{
    MsgqBma423MsgContainer msgq = { 0 };
    int ret = ERR_OK;

    if( chan == &ZBUS_CHAN_BMA423_REQ ) {
        msgq.type = MSGQ_TYPE_BMA423_REQ;
        msgq.msg.zbus.type = ZBUS_TYPE_BMA423;
        msgq.msg.zbus.msg.bma423 = *(ZbusMsgBma423 *)zbus_chan_const_msg( chan );
        ret = k_msgq_put( msgqBma423(), &msgq, K_NO_WAIT );
    }

    if( ret != ERR_OK ) {
        LOG_ERR( "ERR: Event queue full, dropping Zbus message" );
    }
}
