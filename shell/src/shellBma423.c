/**
 * @file shellBma423.c
 * @date 20250526
 * @author flynnty
 * @copyright flynnty 2025
 *
 * @brief
 * Module Shell for direct access to the BMA423 driver
 *
 */

/**** Includes ********************************************************************************************************/
#include "bma4.h"
#include "bma423.h"
#include "zephyrBma423.h"
#include <stdlib.h>
#include <zephyr/shell/shell.h>

/**** Defines *********************************************************************************************************/
/**** Types ***********************************************************************************************************/
/**** Variables *******************************************************************************************************/
/**** Prototypes ******************************************************************************************************/

static int shellBma423Accel( const struct shell *shell, size_t argc, char **argv );
static int shellBma423Steps( const struct shell *shell, size_t argc, char **argv );
static int shellBma423Activity( const struct shell *shell, size_t argc, char **argv );
static int shellBma423Temp( const struct shell *shell, size_t argc, char **argv );
static int shellBma423ResetSteps( const struct shell *shell, size_t argc, char **argv );

/**** Macros **********************************************************************************************************/
/**** Definitions *****************************************************************************************************/

static int shellBma423Accel( const struct shell *shell, size_t argc, char **argv )
{
    ARG_UNUSED( argc );
    ARG_UNUSED( argv );
    struct bma4_dev *dev = bmaDevGet();
    struct bma4_accel accel = { 0 };

    uint16_t ret = bma4_read_accel_xyz( &accel, dev );
    if( ret != 0 ) {
        shell_error( shell, "Failed to read accel: %d", ret );
        return -EIO;
    }

    shell_print( shell, "Accel X:%d Y:%d Z:%d", accel.x, accel.y, accel.z );
    return 0;
}

static int shellBma423Steps( const struct shell *shell, size_t argc, char **argv )
{
    ARG_UNUSED( argc );
    ARG_UNUSED( argv );
    struct bma4_dev *dev = bmaDevGet();
    uint32_t stepCount = 0;

    uint16_t ret = bma423_step_counter_output( &stepCount, dev );
    if( ret != 0 ) {
        shell_error( shell, "Failed to read steps: %d", ret );
        return -EIO;
    }

    shell_print( shell, "Steps: %u", stepCount );
    return 0;
}

static int shellBma423Activity( const struct shell *shell, size_t argc, char **argv )
{
    ARG_UNUSED( argc );
    ARG_UNUSED( argv );
    struct bma4_dev *dev = bmaDevGet();
    uint8_t activity = 0;

    uint16_t ret = bma423_activity_output( &activity, dev );
    if( ret != 0 ) {
        shell_error( shell, "Failed to read activity: %d", ret );
        return -EIO;
    }

    const char *actStr = "unknown";
    switch( activity ) {
    case 0:
        actStr = "still";
        break;
    case 1:
        actStr = "walking";
        break;
    case 2:
        actStr = "running";
        break;
    }

    shell_print( shell, "Activity: %s (%d)", actStr, activity );
    return 0;
}

static int shellBma423Temp( const struct shell *shell, size_t argc, char **argv )
{
    ARG_UNUSED( argc );
    ARG_UNUSED( argv );
    struct bma4_dev *dev = bmaDevGet();
    int32_t tempMilli = 0;

    uint16_t ret = bma4_get_temperature( &tempMilli, BMA4_DEG, dev );
    if( ret != 0 ) {
        shell_error( shell, "Failed to read temperature: %d", ret );
        return -EIO;
    }

    shell_print( shell, "Temp: %d.%03d C", tempMilli / 1000, abs( tempMilli % 1000 ) );
    return 0;
}

static int shellBma423ResetSteps( const struct shell *shell, size_t argc, char **argv )
{
    ARG_UNUSED( argc );
    ARG_UNUSED( argv );
    struct bma4_dev *dev = bmaDevGet();

    uint16_t ret = bma423_reset_step_counter( dev );
    if( ret != 0 ) {
        shell_error( shell, "Failed to reset steps: %d", ret );
        return -EIO;
    }

    shell_print( shell, "Step counter reset" );
    return 0;
}

// clang-format off
SHELL_STATIC_SUBCMD_SET_CREATE( sub_list,
    SHELL_CMD( accel,       NULL, "Read accelerometer XYZ",    shellBma423Accel ),
    SHELL_CMD( steps,       NULL, "Read step counter",         shellBma423Steps ),
    SHELL_CMD( activity,    NULL, "Read activity recognition", shellBma423Activity ),
    SHELL_CMD( temp,        NULL, "Read temperature",          shellBma423Temp ),
    SHELL_CMD( reset_steps, NULL, "Reset step counter",        shellBma423ResetSteps ),
    SHELL_SUBCMD_SET_END );
// clang-format on

SHELL_CMD_REGISTER( bma423, &sub_list, "BMA423 Accelerometer Commands", NULL );
