// simple_client.c : Defines the entry point for the console application.
#include "simple_client.h"

static const int   serverID = 124;
static const int   keepAlive = 300;
static const char *serverURI = "coap://chiklee-m2m-t.cloudapp.net:5683";

int main(int argc, char *argv[])
{
    char *sURI;

    if (argc == 2) sURI = argv[1];
    else sURI = serverURI;

	IOTDM_CLIENT_HANDLE iotDMClient = IoTDMClient_Create();

	if (NULL == iotDMClient)
	{
		(void)printf("ERROR: iotDMClientHandle is NULL!\r\n");
		return -1;
	}

	(void)printf("Info: iotDMClientHandle: %p\r\n", iotDMClient);

	/** The Set Number of Objects operation must be performed prior to adding the objects */
	(void)printf("Info: prepare the client for 4 objects.\r\n");
	unsigned int nrObjects = 6;
	if (IOTDM_CLIENT_OK != IoTDMClient_SetOption(iotDMClient, "Number Of Objects", &nrObjects))
	{
		(void)printf("ERROR: failure to specify the number of objects for client: %p\r\n", iotDMClient);
		return -1;
	}

	(void)printf("Info: prepare the LWM2M Security Object.\r\n");
	IOTDM_CLIENT_OBJECT_HANDLE iotObject = make_security_object(serverID, sURI, false);
	if (IOTDM_CLIENT_OK != IoTDMClient_AddNewObject(iotDMClient, "Security Object", iotObject))
	{
		(void)printf("ERROR: failure to add the security object for client: %p\r\n", iotDMClient);
		return -1;
	}

	(void)printf("Info: prepare the LWM2M Device Object\r\n");
	iotObject = make_device_object();
	if (IOTDM_CLIENT_OK != IoTDMClient_AddNewObject(iotDMClient, "Device Object", iotObject))
	{
		(void)printf("ERROR: failure to add the device object for client: %p\r\n", iotDMClient);
		return -1;
	}

	(void)printf("Info: prepare the LWM2M Server Object\r\n");
	iotObject = make_server_object(serverID, keepAlive, false);
	if (IOTDM_CLIENT_OK != IoTDMClient_AddNewObject(iotDMClient, "Server Object", iotObject))
	{
		(void)printf("ERROR: failure to add the server object for client: %p\r\n", iotDMClient);
		return -1;
	}

    (void)printf("Info: prepare the LWM2M Smart Object\r\n");
    iotObject = make_temperature_sensor_object();
    if (IOTDM_CLIENT_OK != IoTDMClient_AddNewObject(iotDMClient, "Smart Object", iotObject))
    {
        (void)printf("ERROR: failure to add the smart object for client: %p\r\n", iotDMClient);
        return -1;
    }

    (void)printf("Info: prepare the LWM2M Smart Object\r\n");
    iotObject = make_humidity_sensor_object();
    if (IOTDM_CLIENT_OK != IoTDMClient_AddNewObject(iotDMClient, "Smart Object", iotObject))
    {
        (void)printf("ERROR: failure to add the smart object for client: %p\r\n", iotDMClient);
        return -1;
    }

    (void)printf("Info: prepare the LWM2M Led Object\r\n");
    iotObject = make_led_object();
    if (IOTDM_CLIENT_OK != IoTDMClient_AddNewObject(iotDMClient, "Light Object", iotObject))
    {
        (void)printf("ERROR: failure to add the smart object for client: %p\r\n", iotDMClient);
        return -1;
    }

    (void)printf("Info: connect to associated server and initialize an LWM2M session\r\n");
	if (IOTDM_CLIENT_OK != IoTDMClient_Initialize(iotDMClient))
	{
		(void)printf("ERROR: failure to start the client: %p\r\n", iotDMClient);
		return -1;
	}

    long stepInterval = 60;
    while (true)
    {
        /* Process any pending Send / Receive. */
        IoTDMClient_DoWork(iotDMClient);

        /* Take a break :-) */
        ThreadAPI_Sleep(stepInterval);
    }

	/* Disconnect and cleanup */
	IoTDMClient_Destroy(iotDMClient);
}

