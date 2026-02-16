/**
 * @file msgqBma423.h
 * @date 20250526
 * @author flynnty
 * @copyright flynnty 2025
 *
 * @brief
 * Task Message Queue Module
 *
 */
#pragma once

/**** Includes ********************************************************************************************************/
#include "zbusCommon.h"

/**** Defines *********************************************************************************************************/
/**** Types ***********************************************************************************************************/

typedef enum
{
    MSGQ_TYPE_BMA423_REQ = 0,
    MSGQ_TYPE_COUNT
} MsgqBma423MsgType;

typedef struct
{
    MsgqBma423MsgType type;
    union
    {
        ZbusMsgContainer zbus;
    } msg;
} MsgqBma423MsgContainer;

/**** Variables *******************************************************************************************************/
/**** Macros **********************************************************************************************************/
/**** Prototypes ******************************************************************************************************/

/**
 * @brief Get event msgq handle
 *
 * @return struct k_msgq*
 */
struct k_msgq *msgqBma423( void );

/**
 * @brief Process Module Work Q
 *
 * @return int
 */
int msgqBma423Proc( void );
