// program.c : Defines the entry point for the console application.
#include "program.h"

static const int   serverID = 124;
static const int   lifeTime = 300;
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

    (void)printf("Info: prepare the LWM2M Security Object.\r\n");
    IOTDM_CLIENT_OBJECT_HANDLE iotObject = make_security_object(serverID, sURI, false);
    if (IOTDM_CLIENT_OK != IoTDMClient_AddNewObject(iotDMClient, iotObject))
    {
        (void)printf("ERROR: failure to add the security object for client: %p\r\n", iotDMClient);
        return -1;
    }

    (void)printf("Info: prepare the LWM2M Device Object\r\n");
    iotObject = make_device_object();
    if (IOTDM_CLIENT_OK != IoTDMClient_AddNewObject(iotDMClient, iotObject))
    {
        (void)printf("ERROR: failure to add the device object for client: %p\r\n", iotDMClient);
        return -1;
    }

    (void)printf("Info: prepare the LWM2M Server Object\r\n");
    iotObject = make_server_object(serverID, lifeTime, false);
    if (IOTDM_CLIENT_OK != IoTDMClient_AddNewObject(iotDMClient, iotObject))
    {
        (void)printf("ERROR: failure to add the server object for client: %p\r\n", iotDMClient);
        return -1;
    }

    (void)printf("Info: prepare the LWM2M Smart Object\r\n");
    iotObject = make_temperature_sensor_object();
    if (IOTDM_CLIENT_OK != IoTDMClient_AddNewObject(iotDMClient, iotObject))
    {
        (void)printf("ERROR: failure to add the smart object for client: %p\r\n", iotDMClient);
        return -1;
    }

    (void)printf("Info: prepare the LWM2M Smart Object\r\n");
    iotObject = make_humidity_sensor_object();
    if (IOTDM_CLIENT_OK != IoTDMClient_AddNewObject(iotDMClient, iotObject))
    {
        (void)printf("ERROR: failure to add the smart object for client: %p\r\n", iotDMClient);
        return -1;
    }

    (void)printf("Info: prepare the LWM2M Led Object\r\n");
    iotObject = make_led_object();
    if (IOTDM_CLIENT_OK != IoTDMClient_AddNewObject(iotDMClient, iotObject))
    {
        (void)printf("ERROR: failure to add the smart object for client: %p\r\n", iotDMClient);
        return -1;
    }

    (void)printf("Info: connect to associated server and initialize an LWM2M session\r\n");
    if (IOTDM_CLIENT_OK != IoTDMClient_Connect(iotDMClient))
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


