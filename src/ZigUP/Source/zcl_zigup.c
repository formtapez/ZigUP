#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "zcl_lighting.h"
#include "zcl_closures.h"
#include "zcl_zigup.h"
#include "zcl.h"
#include "zcl_ha.h"
#include "zcl_diagnostic.h"

#include "onboard.h"
#include "ZDSecMgr.h"
#include "bitmasks.h"
#include "delay.h"
#include "ds18b20.h"
#include "uart.h"
#include "global.h"
#include "adc.h"
#include "random.h"
#include "ws2812.h"
#include "interrupts.h"
#include "led.h"
#include "dht22.h"
#include "utils.h"

#define ZIGUP_REPORTING_INTERVAL 30000

afAddrType_t zclZigUP_DstAddr;

// Test Endpoint to allow SYS_APP_MSGs
static endPointDesc_t ZigUP_TestEp =
{
  ZIGUP_ENDPOINT,
  &zclZigUP_TaskID,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};

devStates_t zclZigUP_NwkState = DEV_INIT;

void zclZigUP_Reporting(uint16 REPORT_REASON)
{
  const uint8 NUM_ATTRIBUTES = 9;
  
  // send report
  zclReportCmd_t *pReportCmd;
  
  pReportCmd = osal_mem_alloc( sizeof(zclReportCmd_t) + ( NUM_ATTRIBUTES * sizeof(zclReport_t) ) );
  if ( pReportCmd != NULL )
  {
    pReportCmd->numAttr = NUM_ATTRIBUTES;
    
    pReportCmd->attrList[0].attrID = ATTRID_ON_OFF;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_BOOLEAN;
    pReportCmd->attrList[0].attrData = (void *)(&STATE_LIGHT);
    
    pReportCmd->attrList[1].attrID = ATTRID_CPU_TEMP;
    pReportCmd->attrList[1].dataType = ZCL_DATATYPE_SINGLE_PREC;
    pReportCmd->attrList[1].attrData = (void *)(&CPU_Temperature);
    
    pReportCmd->attrList[2].attrID = ATTRID_EXT_TEMP;
    pReportCmd->attrList[2].dataType = ZCL_DATATYPE_SINGLE_PREC;
    pReportCmd->attrList[2].attrData = (void *)(&EXT_Temperature);
    
    pReportCmd->attrList[3].attrID = ATTRID_EXT_HUMI;
    pReportCmd->attrList[3].dataType = ZCL_DATATYPE_SINGLE_PREC;
    pReportCmd->attrList[3].attrData = (void *)(&EXT_Humidity);
    
    pReportCmd->attrList[4].attrID = ATTRID_S0_COUNTS;
    pReportCmd->attrList[4].dataType = ZCL_DATATYPE_UINT32;
    pReportCmd->attrList[4].attrData = (void *)(&S0);
    
    pReportCmd->attrList[5].attrID = ATTRID_ADC_VOLT;
    pReportCmd->attrList[5].dataType = ZCL_DATATYPE_SINGLE_PREC;
    pReportCmd->attrList[5].attrData = (void *)(&ADC_Voltage);
    
    pReportCmd->attrList[6].attrID = ATTRID_DIG_INPUT;
    pReportCmd->attrList[6].dataType = ZCL_DATATYPE_UINT16; // boolean or uint8 causes every second report to hang...
    pReportCmd->attrList[6].attrData = (void *)(&DIG_IN);

    pReportCmd->attrList[7].attrID = ATTRID_REPORT_REASON;
    pReportCmd->attrList[7].dataType = ZCL_DATATYPE_UINT16;
    pReportCmd->attrList[7].attrData = (void *)(&REPORT_REASON);
 
    pReportCmd->attrList[8].attrID = ATTRID_EXT_TEMPS;
    pReportCmd->attrList[8].dataType = ZCL_DATATYPE_CHAR_STR;
    pReportCmd->attrList[8].attrData = (void *)(&EXT_Temperature_string);
   
      
    zclZigUP_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
    zclZigUP_DstAddr.addr.shortAddr = 0;
    zclZigUP_DstAddr.endPoint=1;
    
    zcl_SendReportCmd( ZIGUP_ENDPOINT, &zclZigUP_DstAddr, ZCL_CLUSTER_ID_GEN_ON_OFF, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, zclZigUPSeqNum++ );
  }
  
  osal_mem_free( pReportCmd );
}


/*********************************************************************
* ZCL General Profile Callback table
*/
static zclGeneral_AppCallbacks_t zclZigUP_CmdCallbacks =
{
  zclZigUP_BasicResetCB,                  // Basic Cluster Reset command
  zclZigUP_IdentifyCB,                    // Identify command
  NULL,                                   // Identify Trigger Effect command
  zclZigUP_IdentifyQueryRspCB,            // Identify Query Response command
  zclZigUP_OnOffCB,                       // On/Off cluster commands
  NULL,                                   // On/Off cluster enhanced command Off with Effect
  NULL,                                   // On/Off cluster enhanced command On with Recall Global Scene
  NULL,                                   // On/Off cluster enhanced command On with Timed Off
#ifdef ZCL_GROUPS
  NULL,                                   // Group Response commands
#endif
#ifdef ZCL_SCENES
  NULL,                                   // Scene Store Request command
  NULL,                                   // Scene Recall Request command
  NULL,                                   // Scene Response command
#endif
#ifdef ZCL_ALARMS
  NULL,                                   // Alarm (Response) commands
#endif
#ifdef SE_UK_EXT
  NULL,                                   // Get Event Log command
  NULL,                                   // Publish Event Log command
#endif
  NULL,                                   // RSSI Location command
  NULL                                    // RSSI Location Response command
};

static zclLighting_AppCallbacks_t zclZigUP_LightingCmdCBs =
{
  NULL,                         // Move To Hue Command
  NULL,                         // Move Hue Command
  NULL,                         // Step Hue Command
  NULL,                         // Move To Saturation Command
  NULL,                         // Move Saturation Command
  NULL,                         // Step Saturation Command
  NULL,                         // Move To Hue And Saturation  Command
  zclZigUP_MoveToColorCB,       // Move To Color Command
  NULL,                         // Move Color Command
  NULL,                         // STEP To Color Command
  NULL,                         // Move To Color Temperature Command
  NULL,                         // Enhanced Move To Hue
  NULL,                         // Enhanced Move Hue;
  NULL,                         // Enhanced Step Hue;
  NULL,                         // Enhanced Move To Hue And Saturation;
  NULL,                         // Color Loop Set Command
  NULL,                         // Stop Move Step;
};

static zclClosures_DoorLockAppCallbacks_t zclZigUP_DoorLockCmdCallbacks =
{
  zclZigUP_DoorLockCB,                           // DoorLock cluster command
  zclZigUP_DoorLockRspCB,                        // DoorLock Response
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};


/*********************************************************************
* @fn          zclZigUP_Init
*
* @brief       Initialization function for the zclGeneral layer.
*
* @param       none
*
* @return      none
*/
void zclZigUP_Init( byte task_id )
{
  //  BindRestoreFromNV();
  zclZigUP_TaskID = task_id;
  
  // Set destination address to indirect
  zclZigUP_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  zclZigUP_DstAddr.endPoint = 0;
  zclZigUP_DstAddr.addr.shortAddr = 0;
  
  // This app is part of the Home Automation Profile
  zclHA_Init( &zclZigUP_SimpleDesc[0] );
  
  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( ZIGUP_ENDPOINT, &zclZigUP_CmdCallbacks );

  // Register the ZCL Lighting Cluster Library callback functions
  zclLighting_RegisterCmdCallbacks( ZIGUP_ENDPOINT, &zclZigUP_LightingCmdCBs );

  // Register the ZCL DoorLock Cluster Library callback function
  zclClosures_RegisterDoorLockCmdCallbacks( ZIGUP_ENDPOINT, &zclZigUP_DoorLockCmdCallbacks );
  
  // Register the application's attribute list
  zcl_registerAttrList( ZIGUP_ENDPOINT, zclZigUP_NumAttributes, zclZigUP_Attrs );
  
  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( zclZigUP_TaskID );
  
#ifdef ZCL_DISCOVER
  // Register the application's command list
  zcl_registerCmdList( ZIGUP_ENDPOINT, zclCmdsArraySize, zclZigUP_Cmds );
#endif
  
  // Register for all key events - This app will handle all key events
  RegisterForKeys( zclZigUP_TaskID );
  
  // Register for a test endpoint
  afRegister( &ZigUP_TestEp );
  
#ifdef ZCL_DIAGNOSTIC
  // Register the application's callback function to read/write attribute data.
  // This is only required when the attribute data format is unknown to ZCL.
  zcl_registerReadWriteCB( ZIGUP_ENDPOINT, zclDiagnostic_ReadWriteAttrCB, NULL );
  
  if ( zclDiagnostic_InitStats() == ZSuccess )
  {
    // Here the user could start the timer to save Diagnostics to NV
  }
#endif
  
  
  
  
  
  P0SEL = b00001101;                    // 0=GPIO 1=Peripheral (ADC, UART)
  P0DIR = b00001000;                    // 1=output
  P0INP = b00000001;                    // 1=no pulling
  
  P1SEL = b01000000;                    // 0=GPIO 1=Peripheral (SPI)
  P1DIR = b01000110;                    // 1=output
  P1INP = b00000000;                    // 1=no pulling
  
  P2SEL = b00000000;                    // 0=GPIO 1=Peripheral, Priorities
  P2DIR = b00000000;                    // 1=output, Priorities
  P2INP = b00000000;                    // 1=no pulling, Ports P0-P2 all Pull-UP
  
  PICTL |= BV(0) | BV(1) | BV(3);       // Falling Edge INT P0 (S0) + P1.0-P1.3 (KEY) + P2.0 (Dig-In)
  IEN0 |= BV(7);                        // EA - Global interrupt enable
  IEN1 |= BV(5);                        // Port 0 interrupt enable
  IEN2 |= BV(1) | BV(4);                // Port 1+2 interrupt enable
  
  P0IEN |= BV(6);                       // S0 (P0.6) interrupt enable
  P1IEN |= BV(3);                       // KEY (P1.3) interrupt enable
  P2IEN |= BV(0);                       // Dig-In (P2.0) interrupt enable
  
  
  PERCFG |= BV(1);                      // UART1 SPI Alternative #2 Pins
  U1CSR = b00000000;                    // UART1 SPI Master
  U1GCR = b00010000;                    // UART1 Baud_E
  U1BAUD = b01000000;                   // UART1 Baud_M
  
  UART_Init();
  
  _delay_ms(GetRandomNumber()); // Random delay
  
  // Turn all lights off
  Relais(0);
  WS2812_SendLED(0, 0, 0);
  LED(0);

  // invalidate float values by "assigning" NaN
  // they will be filled later if sensors are present
  EXT_Temperature = *(float*)&float_NaN;
  EXT_Humidity = *(float*)&float_NaN;
  ADC_Voltage = *(float*)&float_NaN;
  CPU_Temperature = *(float*)&float_NaN;
  
  char buffer[100];
  
  // autodetecting sensor type
  if (DHT22_Measure())
  {
    TEMP_SENSOR = -1;
    UART_String("Sensor type DHT22 detected.");
  }
  else if(TEMP_SENSOR = ds18b20_find_devices()) 
  {
    sprintf(buffer, "Found %d ds18b20 sensors", TEMP_SENSOR);
    UART_String(buffer);
  }
  else 
  {  
    sprintf(buffer, "No sensor detected. code: %d", TEMP_SENSOR);
    UART_String(buffer);
  }

// start measurement task for reporting of values
  osal_start_reload_timer( zclZigUP_TaskID, ZIGUP_REPORTING_EVT, ZIGUP_REPORTING_INTERVAL );
  
  UART_String("Init done.");
}

/*********************************************************************
* @fn          zclSample_event_loop
*
* @brief       Event Loop Processor for zclGeneral.
*
* @param       none
*
* @return      none
*/
uint16 zclZigUP_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  
  (void)task_id;  // Intentionally unreferenced parameter
  
  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( zclZigUP_TaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
      case ZCL_INCOMING_MSG:
        // Incoming ZCL Foundation command/response messages
        zclZigUP_ProcessIncomingMsg( (zclIncomingMsg_t *)MSGpkt );
        break;
        
      case ZDO_STATE_CHANGE:
        zclZigUP_NwkState = (devStates_t)(MSGpkt->hdr.status);
        
        // now on the network
        if ( (zclZigUP_NwkState == DEV_ZB_COORD) || (zclZigUP_NwkState == DEV_ROUTER) || (zclZigUP_NwkState == DEV_END_DEVICE) )
        {
          // now on network
        }
        break;
        
      default:
        break;
      }
      
      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
    }
    
    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  
  if ( events & ZIGUP_IDENTIFY_TIMEOUT_EVT )
  {
    if ( zclZigUP_IdentifyTime > 0 ) zclZigUP_IdentifyTime--;
    zclZigUP_ProcessIdentifyTimeChange();
    
    return ( events ^ ZIGUP_IDENTIFY_TIMEOUT_EVT );
  }
  
  if ( events & ZIGUP_REPORTING_EVT )
  {
    // update measurements
    Measure_QuickStuff();
    Measure_Sensor();
    
    // report states
    zclZigUP_Reporting(REPORT_REASON_TIMER);
    
    return ( events ^ ZIGUP_REPORTING_EVT );
  }    
  
  // Discard unknown events
  return 0;
}

/*********************************************************************
* @fn      zclZigUP_ProcessIdentifyTimeChange
*
* @brief   Called to process any change to the IdentifyTime attribute.
*
* @param   none
*
* @return  none
*/
static void zclZigUP_ProcessIdentifyTimeChange( void )
{
  if ( zclZigUP_IdentifyTime > 0 )
  {
    osal_start_timerEx( zclZigUP_TaskID, ZIGUP_IDENTIFY_TIMEOUT_EVT, 1000 );
//    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    osal_stop_timerEx( zclZigUP_TaskID, ZIGUP_IDENTIFY_TIMEOUT_EVT );
  }
}

/*********************************************************************
* @fn      zclZigUP_BasicResetCB
*
* @brief   Callback from the ZCL General Cluster Library
*          to set all the Basic Cluster attributes to default values.
*
* @param   none
*
* @return  none
*/
static void zclZigUP_BasicResetCB( void )
{
  NLME_LeaveReq_t leaveReq;
  // Set every field to 0
  osal_memset( &leaveReq, 0, sizeof( NLME_LeaveReq_t ) );
  
  // This will enable the device to rejoin the network after reset.
  leaveReq.rejoin = TRUE;
  
  // Set the NV startup option to force a "new" join.
  zgWriteStartupOptions( ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );
  
  // Leave the network, and reset afterwards
  if ( NLME_LeaveReq( &leaveReq ) != ZSuccess )
  {
    // Couldn't send out leave; prepare to reset anyway
    ZDApp_LeaveReset( FALSE );
  }
}

/*********************************************************************
* @fn      zclZigUP_IdentifyCB
*
* @brief   Callback from the ZCL General Cluster Library when
*          it received an Identity Command for this application.
*
* @param   srcAddr - source address and endpoint of the response message
* @param   identifyTime - the number of seconds to identify yourself
*
* @return  none
*/
static void zclZigUP_IdentifyCB( zclIdentify_t *pCmd )
{
  zclZigUP_IdentifyTime = pCmd->identifyTime;
  zclZigUP_ProcessIdentifyTimeChange();
}

/*********************************************************************
* @fn      zclZigUP_IdentifyQueryRspCB
*
* @brief   Callback from the ZCL General Cluster Library when
*          it received an Identity Query Response Command for this application.
*
* @param   srcAddr - requestor's address
* @param   timeout - number of seconds to identify yourself (valid for query response)
*
* @return  none
*/
static void zclZigUP_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp )
{
  (void)pRsp;
}

/*********************************************************************
* @fn      zclZigUP_OnOffCB
*
* @brief   Callback from the ZCL General Cluster Library when
*          it received an On/Off Command for this application.
*
* @param   cmd - COMMAND_ON, COMMAND_OFF or COMMAND_TOGGLE
*
* @return  none
*/
static void zclZigUP_OnOffCB( uint8 cmd )
{
  afIncomingMSGPacket_t *pPtr = zcl_getRawAFMsg();
  zclZigUP_DstAddr.addr.shortAddr = pPtr->srcAddr.addr.shortAddr;

  
  char buffer[100];
  sprintf(buffer, "CMD: %u\n", cmd);
  UART_String(buffer); 
  
  
  // Turn on the light
  if ( cmd == COMMAND_ON )
  {
    Relais(LIGHT_ON);
  }
  // Turn off the light
  else if ( cmd == COMMAND_OFF )
  {
    Relais(LIGHT_OFF);
  }
  // Toggle the light
  else if ( cmd == COMMAND_TOGGLE )
  {
    Relais(!STATE_LIGHT);
  }
}

ZStatus_t zclZigUP_MoveToColorCB( zclCCMoveToColor_t *pCmd )
{
  // Converts CIE color space to RGB color space
  // from https://github.com/usolved/cie-rgb-converter/blob/master/cie_rgb_converter.js
  
  char buffer[100];
  sprintf(buffer, "Light-CMD: x: %u // y: %u\n", pCmd->colorX, pCmd->colorY);
  UART_String(buffer); 

  float x = pCmd->colorX/ 65536.0; // the given x value
  float y = pCmd->colorY/ 65536.0; // the given y value
  
  float z = 1.0 - x - y;
  float Y = 1.0;
  float X = (Y / y) * x;
  float Z = (Y / y) * z;
  
  //Convert to RGB using Wide RGB D65 conversion
  float red 	=  X * 1.656492 - Y * 0.354851 - Z * 0.255038;
  float green 	= -X * 0.707196 + Y * 1.655397 + Z * 0.036152;
  float blue 	=  X * 0.051713 - Y * 0.121364 + Z * 1.011530;
  
  //If red, green or blue is larger than 1.0 set it back to the maximum of 1.0
  if (red > blue && red > green && red > 1.0)
  {
    green = green / red;
    blue = blue / red;
    red = 1.0;
  }
  else if (green > blue && green > red && green > 1.0)
  {
    red = red / green;
    blue = blue / green;
    green = 1.0;
  }
  else if (blue > red && blue > green && blue > 1.0)
  {
    red = red / blue;
    green = green / blue;
    blue = 1.0;
  }
  
  //Reverse gamma correction
  red 	= red <= 0.0031308 ? 12.92 * red : (1.0 + 0.055) * pow(red, (1.0 / 2.4)) - 0.055;
  green 	= green <= 0.0031308 ? 12.92 * green : (1.0 + 0.055) * pow(green, (1.0 / 2.4)) - 0.055;
  blue 	= blue <= 0.0031308 ? 12.92 * blue : (1.0 + 0.055) * pow(blue, (1.0 / 2.4)) - 0.055;
  
  
  //Convert normalized decimal to decimal
  red *= 255;
  green *= 255;
  blue *= 255;

  uint8 r = (uint8)red;
  uint8 g = (uint8)green;
  uint8 b = (uint8)blue;

  if (red > 254.5) r = 255;
  else if (red < 0.5) r = 0;

  if (green > 254.5) g = 255;
  else if (green < 0.5) g = 0;

  if (blue > 254.5) b = 255;
  else if (blue < 0.5) b = 0;
  
  sprintf(buffer, "Light-CMD: r: %u // g: %u // b: %u\n", r, g, b);
  UART_String(buffer); 

  WS2812_SendLED(r, g, b);
  
  return ( ZSuccess );
}

/*********************************************************************
 * @fn      zclZigUP_DoorLockCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Door Lock cluster Command for this application.
 *
 * @param   pInMsg - process incoming message
 * @param   pInCmd - PIN/RFID code of command
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclZigUP_DoorLockCB ( zclIncoming_t *pInMsg, zclDoorLock_t *pInCmd )
{
  // LED OFF - Lock the door
  if ( pInMsg->hdr.commandID == COMMAND_CLOSURES_LOCK_DOOR )
  {
    LED(0);
    zclClosures_SendDoorLockStatusResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr, COMMAND_CLOSURES_LOCK_DOOR, ZCL_STATUS_SUCCESS, TRUE, pInMsg->hdr.transSeqNum );
  }
  
  // LED ON - Unlock the door
  else if ( pInMsg->hdr.commandID == COMMAND_CLOSURES_UNLOCK_DOOR )
  {
    LED(1);
    zclClosures_SendDoorLockStatusResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr, COMMAND_CLOSURES_UNLOCK_DOOR, ZCL_STATUS_SUCCESS, TRUE, pInMsg->hdr.transSeqNum );
  }
  
  // Toggle the door
  else if ( pInMsg->hdr.commandID == COMMAND_CLOSURES_TOGGLE_DOOR )
  {
    LED(!STATE_LED);
    zclClosures_SendDoorLockStatusResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr, COMMAND_CLOSURES_TOGGLE_DOOR, ZCL_STATUS_SUCCESS, TRUE, pInMsg->hdr.transSeqNum );
  }
  
  else
  {
    return ( ZCL_STATUS_FAILURE );  // invalid command
  }
  
  return ( ZCL_STATUS_CMD_HAS_RSP );
}

/*********************************************************************
 * @fn      zclZigUP_DoorLockRspCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Door Lock response for this application.
 *
 * @param   cmd - Command ID
 * @param   srcAddr - Requestor's address
 * @param   transSeqNum - Transaction sequence number
 * @param   status - status response from server's door lock cmd
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclZigUP_DoorLockRspCB ( zclIncoming_t *pInMsg, uint8 status )
{
  return ( ZCL_STATUS_SUCCESS );
}


/******************************************************************************
*
*  Functions for processing ZCL Foundation incoming Command/Response messages
*
*****************************************************************************/

/*********************************************************************
* @fn      zclZigUP_ProcessIncomingMsg
*
* @brief   Process ZCL Foundation incoming message
*
* @param   pInMsg - pointer to the received message
*
* @return  none
*/
static void zclZigUP_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#ifdef ZCL_READ
  case ZCL_CMD_READ_RSP:
    zclZigUP_ProcessInReadRspCmd( pInMsg );
    break;
#endif
#ifdef ZCL_WRITE
  case ZCL_CMD_WRITE_RSP:
    zclZigUP_ProcessInWriteRspCmd( pInMsg );
    break;
#endif
#ifdef ZCL_REPORT
    // Attribute Reporting implementation should be added here
  case ZCL_CMD_CONFIG_REPORT:
    // zclZigUP_ProcessInConfigReportCmd( pInMsg );
    break;
    
  case ZCL_CMD_CONFIG_REPORT_RSP:
    // zclZigUP_ProcessInConfigReportRspCmd( pInMsg );
    break;
    
  case ZCL_CMD_READ_REPORT_CFG:
    // zclZigUP_ProcessInReadReportCfgCmd( pInMsg );
    break;
    
  case ZCL_CMD_READ_REPORT_CFG_RSP:
    // zclZigUP_ProcessInReadReportCfgRspCmd( pInMsg );
    break;
    
  case ZCL_CMD_REPORT:
    // zclZigUP_ProcessInReportCmd( pInMsg );
    break;
#endif
  case ZCL_CMD_DEFAULT_RSP:
    zclZigUP_ProcessInDefaultRspCmd( pInMsg );
    break;
#ifdef ZCL_DISCOVER
  case ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP:
    zclZigUP_ProcessInDiscCmdsRspCmd( pInMsg );
    break;
    
  case ZCL_CMD_DISCOVER_CMDS_GEN_RSP:
    zclZigUP_ProcessInDiscCmdsRspCmd( pInMsg );
    break;
    
  case ZCL_CMD_DISCOVER_ATTRS_RSP:
    zclZigUP_ProcessInDiscAttrsRspCmd( pInMsg );
    break;
    
  case ZCL_CMD_DISCOVER_ATTRS_EXT_RSP:
    zclZigUP_ProcessInDiscAttrsExtRspCmd( pInMsg );
    break;
#endif
    
  default:
    break;
  }
  
  if ( pInMsg->attrCmd ) osal_mem_free( pInMsg->attrCmd );
}

#ifdef ZCL_READ
/*********************************************************************
* @fn      zclZigUP_ProcessInReadRspCmd
*
* @brief   Process the "Profile" Read Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclZigUP_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;
  
  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < readRspCmd->numAttr; i++)
  {
    // Notify the originator of the results of the original read attributes
    // attempt and, for each successfull request, the value of the requested
    // attribute
  }
  
  return ( TRUE );
}
#endif // ZCL_READ

#ifdef ZCL_WRITE
/*********************************************************************
* @fn      zclZigUP_ProcessInWriteRspCmd
*
* @brief   Process the "Profile" Write Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclZigUP_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;
  
  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < writeRspCmd->numAttr; i++ )
  {
    // Notify the device of the results of the its original write attributes
    // command.
  }
  
  return ( TRUE );
}
#endif // ZCL_WRITE

/*********************************************************************
* @fn      zclZigUP_ProcessInDefaultRspCmd
*
* @brief   Process the "Profile" Default Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclZigUP_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;
  
  // Device is notified of the Default Response command.
  (void)pInMsg;
  
  return ( TRUE );
}

#ifdef ZCL_DISCOVER
/*********************************************************************
* @fn      zclZigUP_ProcessInDiscCmdsRspCmd
*
* @brief   Process the Discover Commands Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclZigUP_ProcessInDiscCmdsRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverCmdsCmdRsp_t *discoverRspCmd;
  uint8 i;
  
  discoverRspCmd = (zclDiscoverCmdsCmdRsp_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numCmd; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }
  
  return ( TRUE );
}

/*********************************************************************
* @fn      zclZigUP_ProcessInDiscAttrsRspCmd
*
* @brief   Process the "Profile" Discover Attributes Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclZigUP_ProcessInDiscAttrsRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverAttrsRspCmd_t *discoverRspCmd;
  uint8 i;
  
  discoverRspCmd = (zclDiscoverAttrsRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }
  
  return ( TRUE );
}

/*********************************************************************
* @fn      zclZigUP_ProcessInDiscAttrsExtRspCmd
*
* @brief   Process the "Profile" Discover Attributes Extended Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclZigUP_ProcessInDiscAttrsExtRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverAttrsExtRsp_t *discoverRspCmd;
  uint8 i;
  
  discoverRspCmd = (zclDiscoverAttrsExtRsp_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }
  
  return ( TRUE );
}
#endif // ZCL_DISCOVER
