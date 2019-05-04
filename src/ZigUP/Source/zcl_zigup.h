#ifndef ZCL_ZIGUP_H
#define ZCL_ZIGUP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_lighting.h"
#include "zcl_closures.h"
  
#define ZIGUP_ENDPOINT            8

#define LIGHT_OFF                       0x00
#define LIGHT_ON                        0x01

// Application Events
#define ZIGUP_IDENTIFY_TIMEOUT_EVT     0x0001
#define ZIGUP_REPORTING_EVT            0x0002

// Custom attribute IDs
#define ATTRID_CPU_TEMP         41361
#define ATTRID_EXT_TEMP         41362
#define ATTRID_EXT_HUMI         41363
#define ATTRID_S0_COUNTS        41364
#define ATTRID_ADC_VOLT         41365
#define ATTRID_DIG_INPUT        41366

extern SimpleDescriptionFormat_t zclZigUP_SimpleDesc[];

extern CONST zclCommandRec_t zclZigUP_Cmds[];

extern CONST uint8 zclCmdsArraySize;

// attribute list
extern CONST zclAttrRec_t zclZigUP_Attrs[];
extern CONST uint8 zclZigUP_NumAttributes;

// Identify attributes
extern uint16 zclZigUP_IdentifyTime;
extern uint8  zclZigUP_IdentifyCommissionState;

// OnOff attributes
extern uint8  zclZigUP_OnOff;

void FactoryReset(void);

void Relais(uint8 state);
void LED(uint8 state);
void zclZigUP_Reporting(void);
void Measure(void);


static void zclZigUP_BasicResetCB( void );
static void zclZigUP_IdentifyCB( zclIdentify_t *pCmd );
static void zclZigUP_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void zclZigUP_OnOffCB( uint8 cmd );
ZStatus_t zclZigUP_MoveToColorCB( zclCCMoveToColor_t *pCmd );
static ZStatus_t zclZigUP_DoorLockCB ( zclIncoming_t *pInMsg, zclDoorLock_t *pInCmd );
static ZStatus_t zclZigUP_DoorLockRspCB ( zclIncoming_t *pInMsg, uint8 status );

static void zclZigUP_ProcessIdentifyTimeChange( void );


// Functions to process ZCL Foundation incoming Command/Response messages
static void zclZigUP_ProcessIncomingMsg( zclIncomingMsg_t *msg );
#ifdef ZCL_READ
static uint8 zclZigUP_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_WRITE
static uint8 zclZigUP_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif
static uint8 zclZigUP_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#ifdef ZCL_DISCOVER
static uint8 zclZigUP_ProcessInDiscCmdsRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 zclZigUP_ProcessInDiscAttrsRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 zclZigUP_ProcessInDiscAttrsExtRspCmd( zclIncomingMsg_t *pInMsg );
#endif

 /*
  * Initialization for the task
  */
extern void zclZigUP_Init( byte task_id );

/*
 *  Event Process for the task
 */
extern UINT16 zclZigUP_event_loop( byte task_id, UINT16 events );


#ifdef __cplusplus
}
#endif

#endif /* ZCL_ZIGUP_H */
