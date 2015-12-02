/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    domedambrosio - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>

*/

/*
 * This object is single instance only, and is mandatory to all LWM2M device as it describe the object such as its
 * manufacturer, model, etc...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <sys/stat.h>

#include "simple_client.h"


#define PRV_NOT_AVAILABLE     "N/A"
#define PRV_MANUFACTURER      "Manufacturer Name"
#define PRV_MODEL_NUMBER      "Model Number"
#define PRV_SERIAL_NUMBER     "Serial Number"
#define PRV_POWER_SOURCE_1    1
#define PRV_POWER_SOURCE_2    5
#define PRV_POWER_VOLTAGE_1   3800
#define PRV_POWER_VOLTAGE_2   5000
#define PRV_POWER_CURRENT_1   125
#define PRV_POWER_CURRENT_2   900
#define PRV_BATTERY_LEVEL     100
#define PRV_MEMORY_FREE       15
#define PRV_ERROR_CODE        0
#define PRV_BINDING_MODE      "T"

#define PRV_OFFSET_MAXLEN   7 //+HH:MM\0 at max
#define PRV_TLV_BUFFER_SIZE 128

// Resource Id's:
#define RES_O_MANUFACTURER          0
#define RES_O_MODEL_NUMBER          1
#define RES_O_SERIAL_NUMBER         2
#define RES_O_FIRMWARE_VERSION      3
#define RES_M_REBOOT                4
#define RES_O_FACTORY_RESET         5
#define RES_O_AVL_POWER_SOURCES     6
#define RES_O_POWER_SOURCE_VOLTAGE  7
#define RES_O_POWER_SOURCE_CURRENT  8
#define RES_O_BATTERY_LEVEL         9
#define RES_O_MEMORY_FREE           10
#define RES_M_ERROR_CODE            11
#define RES_O_RESET_ERROR_CODE      12
#define RES_O_CURRENT_TIME          13
#define RES_O_UTC_OFFSET            14
#define RES_O_TIMEZONE              15
#define RES_M_BINDING_MODES         16
// since TS 20141126-C:
#define RES_O_DEVICE_TYPE           17
#define RES_O_HARDWARE_VERSION      18
#define RES_O_SOFTWARE_VERSION      19
#define RES_O_BATTERY_STATUS        20
#define RES_O_MEMORY_TOTAL          21


typedef struct
{
    int64_t free_memory;
    int64_t error;
    int64_t time;
    uint8_t battery_level;
    char time_offset[PRV_OFFSET_MAXLEN];
} device_data_t;



// basic check that the time offset value is at ISO 8601 format
// bug: +12:30 is considered a valid value by this function
static int prv_check_time_offset(char * buffer,
                                 int length)
{
    int min_index;

    if (length != 3 && length != 5 && length != 6) return 0;
    if (buffer[0] != '-' && buffer[0] != '+') return 0;
    switch (buffer[1])
    {
        case '0':
            if (buffer[2] < '0' || buffer[2] > '9') return 0;
            break;

        case '1':
            if (buffer[2] < '0' || buffer[2] > '2') return 0;
            break;

        default:
            return 0;
    }

    switch (length)
    {
        case 3:
            return 1;

        case 5:
            min_index = 3;
            break;

        case 6:
            if (buffer[3] != ':') return 0;
            min_index = 4;
            break;

        default:
            // never happen
            return 0;
    }

    if (buffer[min_index] < '0' || buffer[min_index] > '5') return 0;
    if (buffer[min_index+1] < '0' || buffer[min_index+1] > '9') return 0;

    return 1;
}

static uint8_t prv_set_value(lwm2m_data_t  *tlvP,
                             device_data_t *devDataP)
{
    // a simple switch structure is used to respond at the specified resource asked
    switch (tlvP->id)
    {
        case RES_O_MANUFACTURER:
            tlvP->value  = (uint8_t*)PRV_MANUFACTURER;
            tlvP->length = strlen(PRV_MANUFACTURER);
            tlvP->flags  = LWM2M_TLV_FLAG_STATIC_DATA;
            tlvP->type     = LWM2M_TYPE_RESOURCE;
            tlvP->dataType = LWM2M_TYPE_STRING;
            return COAP_205_CONTENT;

        case RES_O_MODEL_NUMBER:
            tlvP->value  = (uint8_t*)PRV_MODEL_NUMBER;
            tlvP->length = strlen(PRV_MODEL_NUMBER);
            tlvP->flags  = LWM2M_TLV_FLAG_STATIC_DATA;
            tlvP->type     = LWM2M_TYPE_RESOURCE;
            tlvP->dataType = LWM2M_TYPE_STRING;
            return COAP_205_CONTENT;

        case RES_O_SERIAL_NUMBER:
        {
			FILE *fd = fopen("/factory/serial_number", "r");
            if (NULL == fd)
            {
                tlvP->value    = (uint8_t *) PRV_NOT_AVAILABLE;
                tlvP->length   = strlen(PRV_NOT_AVAILABLE);
                tlvP->flags    = LWM2M_TLV_FLAG_STATIC_DATA;
                tlvP->type     = LWM2M_TYPE_RESOURCE;
                tlvP->dataType = LWM2M_TYPE_STRING;
            }

            else
            {
                fseek(fd, 0, SEEK_END);
                size_t size = ftell(fd) + 5;
                char *sn = (char *) lwm2m_malloc(size * sizeof(char));
                if (NULL == sn)
                {
                    fclose(fd);
                    return COAP_500_INTERNAL_SERVER_ERROR;
                }

                fseek(fd, 0L, SEEK_SET);
                int count = fread(sn, sizeof(char), size, fd);
                fclose(fd);

                tlvP->value    = (uint8_t *) sn;
                tlvP->length   = count;
                tlvP->flags    = LWM2M_TLV_FLAG_TEXT_FORMAT;
                tlvP->type     = LWM2M_TYPE_RESOURCE;
                tlvP->dataType = LWM2M_TYPE_STRING;
            }

            return COAP_205_CONTENT;
        }

        case RES_O_FIRMWARE_VERSION:
        {
            FILE *fd = fopen("/etc/version", "r");
            if (NULL == fd)
            {
                /** wer are running on development box - send bogus data! */
                tlvP->value    = (uint8_t *) PRV_NOT_AVAILABLE;
                tlvP->length   = strlen(PRV_NOT_AVAILABLE);
                tlvP->flags    = LWM2M_TLV_FLAG_STATIC_DATA;
                tlvP->type     = LWM2M_TYPE_RESOURCE;
                tlvP->dataType = LWM2M_TYPE_STRING;

                return COAP_205_CONTENT;
            }

            else
            {
                fseek(fd, 0, SEEK_END);
                size_t size = ftell(fd) + 5;
                char *version = (char *) lwm2m_malloc(size * sizeof(char));
                if (NULL == version)
                {
                    fclose(fd);
                    return COAP_500_INTERNAL_SERVER_ERROR;
                }

                fseek(fd, 0L, SEEK_SET);
                int count = fread(version, sizeof(char), size, fd);
                fclose(fd);
                version[count] = '\0';

                tlvP->value    = (uint8_t *) version;
                tlvP->length   = count;
                tlvP->flags    = LWM2M_TLV_FLAG_TEXT_FORMAT;
                tlvP->type     = LWM2M_TYPE_RESOURCE;
                tlvP->dataType = LWM2M_TYPE_STRING;

                return COAP_205_CONTENT;
            }
        }

        case RES_M_REBOOT:
            return COAP_405_METHOD_NOT_ALLOWED;

        case RES_O_FACTORY_RESET:
            return COAP_405_METHOD_NOT_ALLOWED;

        case RES_O_AVL_POWER_SOURCES: 
        {
			lwm2m_data_t *subTlvP;

            subTlvP = lwm2m_data_new(2);

            subTlvP[0].flags = 0;
            subTlvP[0].id = 0;
            subTlvP[0].type = LWM2M_TYPE_RESOURCE_INSTANCE;
            lwm2m_data_encode_int(PRV_POWER_SOURCE_1, subTlvP);
            if (0 == subTlvP[0].length)
            {
                lwm2m_data_free(2, subTlvP);
                return COAP_500_INTERNAL_SERVER_ERROR;
            }

            subTlvP[1].flags = 0;
            subTlvP[1].id = 1;
            subTlvP[1].type = LWM2M_TYPE_RESOURCE_INSTANCE;
            lwm2m_data_encode_int(PRV_POWER_SOURCE_2, subTlvP + 1);
            if (0 == subTlvP[1].length)
            {
                lwm2m_data_free(2, subTlvP);
                return COAP_500_INTERNAL_SERVER_ERROR;
            }

            lwm2m_data_include(subTlvP, 2, tlvP);

            return COAP_205_CONTENT;
        }

        case RES_O_POWER_SOURCE_VOLTAGE:
        {
            lwm2m_data_t * subTlvP;

            subTlvP = lwm2m_data_new(2);

            subTlvP[0].flags = 0;
            subTlvP[0].id = 0;
            subTlvP[0].type = LWM2M_TYPE_RESOURCE_INSTANCE;
            lwm2m_data_encode_int(PRV_POWER_VOLTAGE_1, subTlvP);
            if (0 == subTlvP[0].length)
            {
                lwm2m_data_free(2, subTlvP);
                return COAP_500_INTERNAL_SERVER_ERROR;
            }

            subTlvP[1].flags = 0;
            subTlvP[1].id = 1;
            subTlvP[1].type = LWM2M_TYPE_RESOURCE_INSTANCE;
            lwm2m_data_encode_int(PRV_POWER_VOLTAGE_2, subTlvP + 1);
            if (0 == subTlvP[1].length)
            {
                lwm2m_data_free(2, subTlvP);
                return COAP_500_INTERNAL_SERVER_ERROR;
            }

            lwm2m_data_include(subTlvP, 2, tlvP);

            return COAP_205_CONTENT;
        }

        case RES_O_POWER_SOURCE_CURRENT:
        {
            lwm2m_data_t * subTlvP;

            subTlvP = lwm2m_data_new(2);

            subTlvP[0].flags = 0;
            subTlvP[0].id = 0;
            subTlvP[0].type = LWM2M_TYPE_RESOURCE_INSTANCE;
            lwm2m_data_encode_int(PRV_POWER_CURRENT_1, &subTlvP[0]);
            if (0 == subTlvP[0].length)
            {
                lwm2m_data_free(2, subTlvP);
                return COAP_500_INTERNAL_SERVER_ERROR;
            }

            subTlvP[1].flags = 0;
            subTlvP[1].id = 1;
            subTlvP[1].type = LWM2M_TYPE_RESOURCE_INSTANCE;
            lwm2m_data_encode_int(PRV_POWER_CURRENT_2, &subTlvP[1]);
            if (0 == subTlvP[1].length)
            {
                lwm2m_data_free(2, subTlvP);
                return COAP_500_INTERNAL_SERVER_ERROR;
            }

            lwm2m_data_include(subTlvP, 2, tlvP);

            return COAP_205_CONTENT;
        }

        case RES_O_BATTERY_LEVEL:
            lwm2m_data_encode_int(devDataP->battery_level, tlvP);
            tlvP->type = LWM2M_TYPE_RESOURCE;

            if (0 != tlvP->length) return COAP_205_CONTENT;
            else return COAP_500_INTERNAL_SERVER_ERROR;

        case RES_O_MEMORY_FREE:
            lwm2m_data_encode_int(devDataP->free_memory, tlvP);
            tlvP->type = LWM2M_TYPE_RESOURCE;

            if (0 != tlvP->length) return COAP_205_CONTENT;
            else return COAP_500_INTERNAL_SERVER_ERROR;

        case RES_M_ERROR_CODE:
        {
            lwm2m_data_t * subTlvP;

            subTlvP = lwm2m_data_new(1);

            subTlvP[0].flags = 0;
            subTlvP[0].id = 0;
            subTlvP[0].type = LWM2M_TYPE_RESOURCE_INSTANCE;
            lwm2m_data_encode_int(devDataP->error, subTlvP);
            if (0 == subTlvP[0].length)
            {
                lwm2m_data_free(2, subTlvP);
                return COAP_500_INTERNAL_SERVER_ERROR;
            }

            lwm2m_data_include(subTlvP, 1, tlvP);

            return COAP_205_CONTENT;
        }

        case RES_O_RESET_ERROR_CODE:
            return COAP_405_METHOD_NOT_ALLOWED;

        case RES_O_CURRENT_TIME:
            devDataP->time = time(NULL);
            lwm2m_data_encode_int(devDataP->time, tlvP);
            tlvP->type = LWM2M_TYPE_RESOURCE;
            tlvP->dataType = LWM2M_TYPE_TIME;

            if (0 != tlvP->length) return COAP_205_CONTENT;
            else return COAP_500_INTERNAL_SERVER_ERROR;

        case RES_O_UTC_OFFSET:
        {
            time_t raw; time(&raw);
            struct tm *t = localtime(&raw);

#if defined(WIN32)
			sprintf(devDataP->time_offset, "%d:00", 0 /* timezone() / (3600)*/);
#else
//            sprintf(devDataP->time_offset, "%d:00", t->tm_gmtoff / (3600));
#endif
            tlvP->value    = (uint8_t *) devDataP->time_offset;
            tlvP->length   = strlen(devDataP->time_offset);
            tlvP->flags    = LWM2M_TLV_FLAG_STATIC_DATA;
            tlvP->type     = LWM2M_TYPE_RESOURCE;
            tlvP->dataType = LWM2M_TYPE_STRING;

            return COAP_205_CONTENT;
        }

        case RES_O_TIMEZONE:
        {
            char *value = (char *) malloc(41 * sizeof(char));
            if (NULL == value)
            {
                return COAP_500_INTERNAL_SERVER_ERROR;
            }

            value[0] = '\0';
#if defined(WIN32)
			TIME_ZONE_INFORMATION tz;
			DWORD rv = GetTimeZoneInformation(&tz);
			if (TIME_ZONE_ID_INVALID == rv)
			{
				return COAP_500_INTERNAL_SERVER_ERROR;
			}

			WideCharToMultiByte(CP_UTF8, 0, tz.StandardName, lstrlenW(tz.StandardName), value, 40, NULL, NULL);
			if (TIME_ZONE_ID_DAYLIGHT == rv)
			{
				value[strlen(value)] = '/';
				WideCharToMultiByte(CP_UTF8, 0, tz.DaylightName, lstrlenW(tz.DaylightName), &value[strlen(value)], 40 - strlen(value), NULL, NULL);
			}
#else
//			tzset();
//			sprintf(value, "%s", tzname[0]);
//			if (daylight > 0) /* add the daylight saving portion as appropriate */
//			{
//				sprintf(&value[strlen(value)], "/%s", tzname[1]);
//			}
#endif

            tlvP->value  = (uint8_t *) value;
            tlvP->length = strlen(value);
            tlvP->flags    = LWM2M_TLV_FLAG_TEXT_FORMAT;
            tlvP->type     = LWM2M_TYPE_RESOURCE;
            tlvP->dataType = LWM2M_TYPE_STRING;
        }

        return COAP_205_CONTENT;
      
        case RES_M_BINDING_MODES:
            tlvP->value  = (uint8_t*)PRV_BINDING_MODE;
            tlvP->length = strlen(PRV_BINDING_MODE);
            tlvP->flags  = LWM2M_TLV_FLAG_STATIC_DATA;
            tlvP->type     = LWM2M_TYPE_RESOURCE;
            tlvP->dataType = LWM2M_TYPE_STRING;
            return COAP_205_CONTENT;

        default:
            return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_device_read(uint16_t instanceId,
                               int * numDataP,
                               lwm2m_data_t ** dataArrayP,
                               lwm2m_object_t * objectP)
{
    uint8_t result;
    int i;

    LOG("    device_read(%d)\n", instanceId);
    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        uint16_t resList[] =
        {
             RES_O_MANUFACTURER,
             RES_O_MODEL_NUMBER,
             RES_O_SERIAL_NUMBER,
             RES_O_FIRMWARE_VERSION,
             //E: RES_M_REBOOT,
             //E: RES_O_FACTORY_RESET,
             RES_O_AVL_POWER_SOURCES,
             RES_O_POWER_SOURCE_VOLTAGE,
             RES_O_POWER_SOURCE_CURRENT,
             RES_O_BATTERY_LEVEL,
             RES_O_MEMORY_FREE,
             RES_M_ERROR_CODE,
             //E: RES_O_RESET_ERROR_CODE,
             RES_O_CURRENT_TIME,
             RES_O_UTC_OFFSET,
             RES_O_TIMEZONE,
             RES_M_BINDING_MODES
        };
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do
    {
        result = prv_set_value((*dataArrayP) + i, (device_data_t*)(objectP->userData));
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

static uint8_t prv_device_write(uint16_t instanceId,
                                int numData,
                                lwm2m_data_t * dataArray,
                                lwm2m_object_t * objectP)
{
    int i;
    uint8_t result;

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    i = 0;

    do
    {
        switch (dataArray[i].id)
        {
        case RES_O_CURRENT_TIME:
            if (1 == lwm2m_data_decode_int(dataArray + i, &((device_data_t*)(objectP->userData))->time))
            {
            //  ((device_data_t*)(objectP->userData))->time -= time(NULL);
                result = COAP_204_CHANGED;
            }

            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        case RES_O_UTC_OFFSET:
            if (1 == prv_check_time_offset((char*)dataArray[i].value, dataArray[i].length))
            {
                strncpy(((device_data_t*)(objectP->userData))->time_offset, (char*)dataArray[i].value, dataArray[i].length);
                ((device_data_t*)(objectP->userData))->time_offset[dataArray[i].length] = 0;
                result = COAP_204_CHANGED;
            }

            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        case RES_O_TIMEZONE:
            //ToDo IANA TZ Format
            result = COAP_501_NOT_IMPLEMENTED;
            break;

        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
        }

        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_device_execute(uint16_t instanceId,
                                  uint16_t resourceId,
                                  uint8_t * buffer,
                                  int length,
                                  lwm2m_object_t * objectP)
{
    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    switch (resourceId)
    {
        case RES_M_REBOOT:
        {
            LOG("\n\t REBOOT\r\n\n");
#if !defined(WIN32)
            pid_t child = fork();
            if (child == 0)
            {
                setuid(0);
                sync();
                reboot(0x1234567 /* RB_AUTOBOOT */);
            }

            return COAP_204_CHANGED;
#else
			return COAP_405_METHOD_NOT_ALLOWED;
#endif
		}

        case RES_O_FACTORY_RESET:
        {
            LOG("\n\t FACTORY RESET\r\n\n");
#if defined(EDISON)

            struct stat sb;
            if (0 != stat("/home/root/factoryImage.zip", &sb)) return COAP_400_BAD_REQUEST;

            pid_t child = vfork();
            if (child == 0)
            {
                setuid(0);
                system("mkdir -p /update");
                system("systemctl stop clloader");
                system("echo on > /sys/devices/pci0000:00/0000:00:11.0/dwc3-device.1/power/control");
                system("rmmod g_multi");
                system("losetup -o 8192 /dev/loop0 /dev/disk/by-partlabel/update");
                system("mount /dev/loop0 /update");
                system("rm -rf /update/* /update/.[!.]* /update/..?*");
                system("unzip -o /home/root/factoryImage.zip -d /update");

                system("reboot ota");
            }

            else if (child < 0) return COAP_400_BAD_REQUEST;
            else return COAP_204_CHANGED;
#else
			return COAP_405_METHOD_NOT_ALLOWED;
#endif
		}

        case RES_O_RESET_ERROR_CODE:
            LOG("\n\t RESET ERROR CODE\r\n\n");
            ((device_data_t*)(objectP->userData))->error = 0;
            return COAP_204_CHANGED;

        default:
            return COAP_405_METHOD_NOT_ALLOWED;
    }
}

static void prv_device_close(lwm2m_object_t * objectP)
{
    if (NULL != objectP->userData)
    {
        lwm2m_free(objectP->userData);
        objectP->userData = NULL;
    }

    if (NULL != objectP->instanceList)
    {
        lwm2m_free(objectP->instanceList);
        objectP->instanceList = NULL;
    }
}

void display_device_object(lwm2m_object_t * object)
{
#ifdef WITH_LOGS
    device_data_t * data = (device_data_t *)object->userData;
    LOG("  /%u: Device object:\r\n", object->objID);
    if (NULL != data)
    {
        LOG("    time: %lld, time_offset: %s\r\n",
                (long long) data->time, data->time_offset);
    }
#endif
}

lwm2m_object_t *make_device_object()
{
    /*
     * The get_object_device function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t * deviceObj;

    deviceObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != deviceObj)
    {
        memset(deviceObj, 0, sizeof(lwm2m_object_t));

        /*
         * It assigns his unique ID
         * The 3 is the standard ID for the mandatory object "Object device".
         */
        deviceObj->objID = LWM2M_DEVICE_OBJECT_ID;

        /*
         * and its unique instance
         *
         */
        deviceObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != deviceObj->instanceList)
        {
            memset(deviceObj->instanceList, 0, sizeof(lwm2m_list_t));
        }
        else
        {
            lwm2m_free(deviceObj);
            return NULL;
        }
        
        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
        deviceObj->readFunc    = prv_device_read;
        deviceObj->writeFunc   = prv_device_write;
        deviceObj->executeFunc = prv_device_execute;
        deviceObj->closeFunc   = prv_device_close;
        deviceObj->userData    = lwm2m_malloc(sizeof(device_data_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables 
         */
        if (NULL != deviceObj->userData)
        {
            ((device_data_t*)deviceObj->userData)->battery_level = PRV_BATTERY_LEVEL;
            ((device_data_t*)deviceObj->userData)->free_memory   = PRV_MEMORY_FREE;
            ((device_data_t*)deviceObj->userData)->error = PRV_ERROR_CODE;

            time_t raw; time(&raw);
            ((device_data_t*)deviceObj->userData)->time = raw;
#if defined(EDISON)
            struct tm *t = localtime(&raw);
			sprintf(((device_data_t *) deviceObj->userData)->time_offset, "%d:00", t->tm_gmtoff / (3600));
#else
			sprintf(((device_data_t *) deviceObj->userData)->time_offset, "%d:00", 0 /* timezone() / (3600) */);
#endif
        }

        else
        {
            lwm2m_free(deviceObj);
            deviceObj = NULL;
        }
    }

    return deviceObj;
}

uint8_t device_change(lwm2m_data_t * dataArray,
                      lwm2m_object_t * objectP)
{
    uint8_t result;

    switch (dataArray->id)
    {
        case RES_O_BATTERY_LEVEL:
            {
                int64_t value;
                if (1 == lwm2m_data_decode_int(dataArray, &value))
                {
                    if ((0 <= value) && (100 >= value))
                    {
                        ((device_data_t*)(objectP->userData))->battery_level = value;
                        result = COAP_204_CHANGED;
                    }
                    else
                    {
                        result = COAP_400_BAD_REQUEST;
                    }
                }
                else
                {
                    result = COAP_400_BAD_REQUEST;
                }
            }
            break;

        case RES_M_ERROR_CODE:
            if (1 == lwm2m_data_decode_int(dataArray, &((device_data_t*)(objectP->userData))->error))
            {
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        case RES_O_MEMORY_FREE:
            if (1 == lwm2m_data_decode_int(dataArray, &((device_data_t*)(objectP->userData))->free_memory))
            {
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;
        }
    
    return result;
}
