#pragma once

typedef void *IOTDM_CLIENT_HANDLE;
typedef void *IOTDM_CLIENT_OBJECT_HANDLE;
typedef enum  IOTDM_CLIENT_RESULT_TAG
{
	IOTDM_CLIENT_OK = 0,
	IOTMD_CLIENT_INVALID_ARG,
	IOTDM_CLIENT_ERROR
} IOTDM_CLIENT_RESULT;

/** The LWM2M defines several Standard Device Management Objects - some of which are identified below:
* ID: 0  - the Security Object (required)
* ID: 1  - the LWM2M Server Object (required)
* ID: 2  - the Access Control Object
* ID: 3  - the Device Object (required)
* ID: 4  - the Connectivity Monitoring Object
* ID: 5  - the Firmware Object
* ID: 6  - the Location Object
* ID: 9  - the Software Update Object
*/
typedef enum IOTDM_CLIENT_OBJECT_NAME_TAG
{
	IOTDM_CLIENT_OBJECT_Security = 0,
	IOTDM_CLIENT_OBJECT_Server,
	IOTDM_CLIENT_OBJECT_Device = 3,
	IOTDM_CLIENT_OBJECT_Other
} IOTDM_CLIENT_OBJECT_TYPE;


IOTDM_CLIENT_HANDLE IoTDMClient_Create();
IOTDM_CLIENT_RESULT IoTDMClient_Initialize(IOTDM_CLIENT_HANDLE h);

IOTDM_CLIENT_RESULT IoTDMClient_SetOption(IOTDM_CLIENT_HANDLE h, const char *optionName, const void *value);
IOTDM_CLIENT_RESULT IoTDMClient_AddNewObject(IOTDM_CLIENT_HANDLE h, const void *value);

void IoTDMClient_DoWork(IOTDM_CLIENT_HANDLE client);
void IoTDMClient_Destroy(IOTDM_CLIENT_HANDLE client);