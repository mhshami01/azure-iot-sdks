#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(WIN32)
#include <windows.h>
#include <tchar.h>
#endif

#include "iotdm_client.h"
#include "liblwm2m.h"


#ifdef WITH_LOGS
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif

#include "threadapi.h"

// object_security.c
lwm2m_object_t *make_security_object(int serverId, const char* serverUri, bool isBootstrap);
char           *get_server_uri(lwm2m_object_t * objectP, uint16_t secObjInstID);

// object_server.c
lwm2m_object_t *make_server_object(int serverId, int lifetime, bool storing);

// object_device.c
lwm2m_object_t *make_device_object();

// object_temp_sensor.c
lwm2m_object_t *make_temperature_sensor_object();

// object_humidity_sensor.c
lwm2m_object_t *make_humidity_sensor_object();

// object_led.c
lwm2m_object_t *make_led_object();