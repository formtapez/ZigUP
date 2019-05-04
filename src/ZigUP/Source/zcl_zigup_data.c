#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_ezmode.h"
#include "zcl_poll_control.h"
#include "zcl_electrical_measurement.h"
#include "zcl_diagnostic.h"
#include "zcl_meter_identification.h"
#include "zcl_appliance_identification.h"
#include "zcl_appliance_events_alerts.h"
#include "zcl_power_profile.h"
#include "zcl_appliance_control.h"
#include "zcl_appliance_statistics.h"
#include "zcl_hvac.h"

#include "zcl_zigup.h"

#include "global.h"

#define ZIGUP_DEVICE_VERSION     0
#define ZIGUP_FLAGS              0

#define ZIGUP_HWVERSION          1
#define ZIGUP_ZCLVERSION         1

// Basic Cluster
const uint8 zclZigUP_HWRevision = ZIGUP_HWVERSION;
const uint8 zclZigUP_ZCLVersion = ZIGUP_ZCLVERSION;
const uint8 zclZigUP_ManufacturerName[] = { 9, 'f','o','r','m','t','a','p','e','z' };
const uint8 zclZigUP_ModelId[] = { 9, 'Z','i','g','B','e','e',' ','U','P' };
const uint8 zclZigUP_DateCode[] = { 16, '2','0','1','9','0','4','0','6',' ',' ',' ',' ',' ',' ',' ',' ' };
const uint8 zclZigUP_PowerSource = POWER_SOURCE_MAINS_1_PHASE;

uint8 zclZigUP_LocationDescription[17] = { 16, ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
uint8 zclZigUP_PhysicalEnvironment = 0;
uint8 zclZigUP_DeviceEnable = DEVICE_ENABLED;

// Identify Cluster
uint16 zclZigUP_IdentifyTime = 0;

// On/Off Cluster
uint8 zclZigUP_OnOff = LIGHT_OFF;

#if ZCL_DISCOVER
CONST zclCommandRec_t zclZigUP_Cmds[] =
{
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    COMMAND_BASIC_RESET_FACT_DEFAULT,
    CMD_DIR_SERVER_RECEIVED
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    COMMAND_OFF,
    CMD_DIR_SERVER_RECEIVED
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    COMMAND_ON,
    CMD_DIR_SERVER_RECEIVED
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    COMMAND_TOGGLE,
    CMD_DIR_SERVER_RECEIVED
  }
};

CONST uint8 zclCmdsArraySize = ( sizeof(zclZigUP_Cmds) / sizeof(zclZigUP_Cmds[0]) );
#endif // ZCL_DISCOVER

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
CONST zclAttrRec_t zclZigUP_Attrs[] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  // Attribute record
      ATTRID_BASIC_HW_VERSION,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)&zclZigUP_HWRevision  // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_ZCL_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclZigUP_ZCLVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclZigUP_ManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclZigUP_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclZigUP_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclZigUP_PowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)zclZigUP_LocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclZigUP_PhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclZigUP_DeviceEnable
    }
  },

#ifdef ZCL_IDENTIFY
  // *** Identify Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // Attribute record
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclZigUP_IdentifyTime
    }
  },
#endif

  // *** On/Off Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF,
      ZCL_DATATYPE_BOOLEAN,
      ACCESS_CONTROL_READ,
      (void *)&zclZigUP_OnOff   // TODO
    }
  },
  
  
  // *** Color control Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_X,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclZigUP_OnOff   // TODO
    }
  }
  
  
};

uint8 CONST zclZigUP_NumAttributes = ( sizeof(zclZigUP_Attrs) / sizeof(zclZigUP_Attrs[0]) );

/*********************************************************************
 * SIMPLE DESCRIPTOR
 */
// This is the Cluster ID List and should be filled with Application specific cluster IDs.
const cId_t zclZigUP_InClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_GEN_GROUPS,
  ZCL_CLUSTER_ID_GEN_SCENES,
  ZCL_CLUSTER_ID_GEN_ON_OFF,
  ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL    
};
#define ZCLZIGUP_MAX_INCLUSTERS (sizeof(zclZigUP_InClusterList) / sizeof(zclZigUP_InClusterList[0]))

const cId_t zclZigUP_OutClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC
};
#define ZCLZIGUP_MAX_OUTCLUSTERS (sizeof(zclZigUP_OutClusterList) / sizeof(zclZigUP_OutClusterList[0]))

SimpleDescriptionFormat_t zclZigUP_SimpleDesc[1] = {
{
  ZIGUP_ENDPOINT,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,               //  uint16 AppProfId;
  ZCL_HA_DEVICEID_ON_OFF_LIGHT,    //  uint16 AppDeviceId;
  ZIGUP_DEVICE_VERSION,            //  int   AppDevVer:4;
  ZIGUP_FLAGS,                     //  int   AppFlags:4;
  ZCLZIGUP_MAX_INCLUSTERS,         //  byte  AppNumInClusters;
  (cId_t *)zclZigUP_InClusterList, //  byte *pAppInClusterList;
  ZCLZIGUP_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
  (cId_t *)zclZigUP_OutClusterList //  byte *pAppInClusterList;
}
};
