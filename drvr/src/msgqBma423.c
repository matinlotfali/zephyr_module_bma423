/**
 * @file msgqBma423.c
 * @date 20250526
 * @author flynnty
 * @copyright flynnty 2025
 *
 * @brief
 * BMA423 Message Queue Module
 *
 */

/**** Includes ********************************************************************************************************/
#include "bma4.h"
#include "bma423.h"
#include "bma4_defs.h"
#include "error.h"
#include "msgqBma423.h"
#include "zbusCommon.h"
#include "zephyrBma423.h"
#include <zephyr/logging/log.h>

/**** Defines *********************************************************************************************************/

// Full uint8_t range (256) used for correcting Bosch API sign extension bug in temperature reading
#define BMA4_TEMP_UINT8_RANGE ( UINT8_MAX + 1 )

/**** Types ***********************************************************************************************************/
/**** Variables *******************************************************************************************************/

static const struct bma4_int_pin_config pinConfig = {
    .edge_ctrl = BMA4_LEVEL_TRIGGER,
    .lvl = BMA4_ACTIVE_HIGH,
    .od = BMA4_PUSH_PULL,
    .output_en = BMA4_OUTPUT_ENABLE,
    .input_en = BMA4_INPUT_DISABLE,
};

static const struct bma423_axes_remap remapConfig = {
    .x_axis = 1,
    .x_axis_sign = 0xFF,
    .y_axis = 0,
    .y_axis_sign = 0xFF,
    .z_axis = 2,
    .z_axis_sign = 0xFF,
};

static const struct bma4_accel_config accelConfig = {
    .odr = BMA4_OUTPUT_DATA_RATE_100HZ,
    .range = BMA4_ACCEL_RANGE_2G,
    .bandwidth = BMA4_ACCEL_NORMAL_AVG4,
    .perf_mode = BMA4_CONTINUOUS_MODE,
};

/**** Macros **********************************************************************************************************/

LOG_MODULE_DECLARE( bma423_main, CONFIG_BMA423_LOG_LEVEL );

K_MSGQ_DEFINE( MsgqBma423, sizeof( MsgqBma423MsgContainer ), CONFIG_BMA423_MSGQ_SIZE, CONFIG_BMA423_MSGQ_ALIGN );

/**** Prototypes ******************************************************************************************************/

static int bma423FullInit( void );
static int bma423ResetSteps( void );
static int bma423SamplePublish( ZbusMsgTypeBma423 sampleMask );

/**** Definitions *****************************************************************************************************/

struct k_msgq *msgqBma423( void )
{
    return &MsgqBma423;
}

int msgqBma423Proc( void )
{
    int ret = ERR_OK;
    MsgqBma423MsgContainer msgq = { 0 };
    const k_timeout_t msgQueWaitMs = K_MSEC( MSEC_PER_SEC * CONFIG_BMA423_MSGQ_TIMEOUT_S );

    // we timeout every second to feed the task wdt
    ret = k_msgq_get( msgqBma423(), &msgq, msgQueWaitMs );
    if( ret == -EAGAIN ) {
        return ERR_OK;
    }

    __ASSERT( msgq.type == MSGQ_TYPE_BMA423_REQ, "msgq.type must be MSGQ_TYPE_BMA423_REQ" );
    __ASSERT( msgq.msg.zbus.type == ZBUS_TYPE_BMA423, "msgq.msg.zbus.type must be ZBUS_TYPE_BMA423" );

    // Here we have a valid message to process
    switch( msgq.msg.zbus.msg.bma423.mask ) {
    case ZBUS_MSG_TYPE_BMA423_RESET_STEPS:
        RETURN_ON_ERROR( bma423ResetSteps() );
        break;
    case ZBUS_MSG_TYPE_BMA423_INIT:
        RETURN_ON_ERROR( bma423FullInit() );
        break;
    default:
        if( msgq.msg.zbus.msg.bma423.mask < ZBUS_MSG_TYPE_BMA423_INVALID ) {
            RETURN_ON_ERROR( bma423SamplePublish( msgq.msg.zbus.msg.bma423.mask ) );
        } else {
            LOG_ERR( "Unknown message type: %d", msgq.type );
        }
        break;
    }

    return ERR_OK;
}

/**** Static Functions ************************************************************************************************/

static int bma423SamplePublish( ZbusMsgTypeBma423 sampleMask )
{
    struct bma4_dev *dev = bmaDevGet();
    ZbusMsgBma423 zbusMsg = { .mask = sampleMask };

    LOG_DBG( "BMA423 sample requested: 0x%x", sampleMask );

    // Read accelerometer XYZ
    if( sampleMask & ZBUS_MSG_TYPE_BMA423_ACCEL ) {
        struct bma4_accel accel = { 0 };
        LOG_ON_ERROR( bma4_read_accel_xyz( &accel, dev ) );
        zbusMsg.accelX = accel.x;
        zbusMsg.accelY = accel.y;
        zbusMsg.accelZ = accel.z;
    }

    // Read step counter
    if( sampleMask & ZBUS_MSG_TYPE_BMA423_STEPS ) {
        uint32_t stepCount = 0;
        LOG_ON_ERROR( bma423_step_counter_output( &stepCount, dev ) );
        zbusMsg.steps = stepCount;
    }

    // Read activity
    if( sampleMask & ZBUS_MSG_TYPE_BMA423_ACTIVITY ) {
        uint8_t activity = 0;
        LOG_ON_ERROR( bma423_activity_output( &activity, dev ) );
        zbusMsg.activity = activity;
    }

    // Read temperature in degrees Celsius (Bosch API returns milli-Celsius with BMA4_DEG)
    if( sampleMask & ZBUS_MSG_TYPE_BMA423_TEMP ) {
        int32_t tempMilli = 0;
        LOG_ON_ERROR( bma4_get_temperature( &tempMilli, BMA4_DEG, dev ) );

        // Workaround: Bosch API reads temp register as uint8_t but it's signed.
        // Values below 23°C get a 256°C offset due to missing sign extension.
        if( tempMilli > ( INT8_MAX + BMA4_OFFSET_TEMP ) * BMA4_SCALE_TEMP ) {
            tempMilli -= BMA4_TEMP_UINT8_RANGE * BMA4_SCALE_TEMP;
        }
        zbusMsg.tempMilli = tempMilli;
    }

    // Publish results to SUB channel
    RETURN_ON_ERROR( zbus_chan_pub( &ZBUS_CHAN_BMA423_SUB, &zbusMsg, K_NO_WAIT ) );

    LOG_DBG( "BMA423 sample published: X=%d Y=%d Z=%d steps=%u act=%u temp=%d", zbusMsg.accelX, zbusMsg.accelY, zbusMsg.accelZ, zbusMsg.steps,
             zbusMsg.activity, zbusMsg.tempMilli );

    return ERR_OK;
}

static int bma423ResetSteps( void )
{
    struct bma4_dev *dev = bmaDevGet();
    LOG_ON_ERROR( bma423_reset_step_counter( dev ) );
    LOG_DBG( "BMA423 step counter reset" );
    return ERR_OK;
}

static int bma423FullInit( void )
{
    struct bma4_dev *dev = bmaDevGet();

    // Soft reset
    LOG_DBG( "softReset" );
    uint8_t reg = BMA4_RESET_ADDR;
    RETURN_ON_ERROR( dev->bus_write( BMA4_I2C_ADDR_PRIMARY, BMA4_RESET_SET_MASK, &reg, 1 ) );
    k_msleep( 20 );

    // Chip init and config file
    LOG_DBG( "bma423_init" );
    RETURN_ON_ERROR( bma423_init( dev ) );

    LOG_DBG( "BMA423 writing config file..." );
    RETURN_ON_ERROR( bma423_write_config_file( dev ) );
    LOG_DBG( "BMA423 config file written." );

    // Hardware configuration
    RETURN_ON_ERROR( bma4_set_int_pin_config( &pinConfig, BMA4_INTR1_MAP, dev ) );
    RETURN_ON_ERROR( bma4_set_accel_config( &accelConfig, dev ) );
    RETURN_ON_ERROR( bma4_set_accel_enable( BMA4_ENABLE, dev ) );
    RETURN_ON_ERROR( bma423_set_remap_axes( &remapConfig, dev ) );

    // Enable features (accel must be configured and enabled BEFORE features)
    RETURN_ON_ERROR( bma423_feature_enable( BMA423_STEP_CNTR, true, dev ) );
    RETURN_ON_ERROR( bma423_feature_enable( BMA423_ACTIVITY, true, dev ) );
    RETURN_ON_ERROR( bma423_feature_enable( BMA423_TILT, true, dev ) );
    RETURN_ON_ERROR( bma423_feature_enable( BMA423_WAKEUP, true, dev ) );

    RETURN_ON_ERROR( bma423_select_platform( BMA423_WRIST_CONFIG, dev ) );
    RETURN_ON_ERROR( bma423_reset_step_counter( dev ) );

#ifdef CONFIG_BMA423_INTERRUPTS
    RETURN_ON_ERROR( bma423_map_interrupt( BMA4_INTR1_MAP, BMA423_STEP_CNTR_INT, BMA4_ENABLE, dev ) );
    RETURN_ON_ERROR( bma423_map_interrupt( BMA4_INTR1_MAP, BMA423_ACTIVITY_INT, BMA4_ENABLE, dev ) );
    RETURN_ON_ERROR( bma423_map_interrupt( BMA4_INTR1_MAP, BMA423_TILT_INT, BMA4_ENABLE, dev ) );
    RETURN_ON_ERROR( bma423_map_interrupt( BMA4_INTR1_MAP, BMA423_WAKEUP_INT, BMA4_ENABLE, dev ) );
#endif // CONFIG_BMA423_INTERRUPTS

    LOG_INF( "BMA423 Initialized." );
    return ERR_OK;
}
