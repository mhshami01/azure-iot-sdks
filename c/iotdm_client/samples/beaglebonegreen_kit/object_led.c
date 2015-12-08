
/*
 *  Resources:
 *
 *          Name         | ID   | Operations | Instances | Mandatory |  Type   |  Range  | Units |
 *      On / Off         | 5850 |   R/W      |  Multiple |   Yes     |   Bool  |   0-1   |       |
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "program.h"

typedef struct _led_instance_
{
    struct _led_instance_ *next;   // matches lwm2m_list_t::next
    uint16_t    instanceId;        // matches lwm2m_list_t::id

    bool        lever;
} led_instance_t;

void prv_init_device()
{
    system("echo 67 > /sys/class/gpio/export");
    system("echo out > /sys/devices/virtual/gpio/gpio67/direction");
}

int prv_read_lever(bool *state)
{
    FILE *fp = fopen("/sys/devices/virtual/gpio/gpio67/value", "r");
    if (NULL == fp) return -1;

    char data[1];
    if (NULL == fgets(data, sizeof(data), fp))
    {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    *state = data[0] == '1';
    return 0;
}

static uint8_t prv_get_value(lwm2m_data_t *tlvP, led_instance_t *targetP)
{
    // There are no multiple instance resources
    tlvP->type = LWM2M_TYPE_RESOURCE;

    switch (tlvP->id)
    {
        case 5850:
        {
            if (0 == prv_read_lever(&(targetP->lever)))
            {
                lwm2m_data_encode_bool(targetP->lever, tlvP);
                return COAP_205_CONTENT;
            }

            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        case 5851:
        case 5706:
        case 5701:
        case 5852:
        case 5805:
        case 5820:
            return COAP_501_NOT_IMPLEMENTED;

        default:
            return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_led_read(uint16_t instanceId,
                               int * numDataP,
                               lwm2m_data_t ** dataArrayP,
                               lwm2m_object_t * objectP)
{
    led_instance_t *targetP;
    uint8_t result;
    int i;

    targetP = (led_instance_t *) lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    // is the server asking for the full instance ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = { 5850 };
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL)
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do
    {
        result = prv_get_value((*dataArrayP) + i, targetP);
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

int prv_write_lever(bool value)
{
    FILE *fp = fopen("/sys/devices/virtual/gpio/gpio67/value", "w");
    if (NULL == fp) return -1;

    char data[2];
    data[0] = (value ? '1' : '0'); data[1] = '\0';
    if (EOF == fputs(data, fp))
    {
        fclose(fp);
        return -1;
    }

    fflush(fp);
    fclose(fp);

    return 1;
}

static uint8_t prv_led_write(uint16_t instanceId,
                                int numData,
                                lwm2m_data_t * dataArray,
                                lwm2m_object_t * objectP)
{
    led_instance_t *targetP;
    int i;
    uint8_t result;

    targetP = (led_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP)
    {
        return COAP_404_NOT_FOUND;
    }

    i = 0;
    do
    {
        switch (dataArray[i].id)
        {
            case 5850:
            {
                result = COAP_500_INTERNAL_SERVER_ERROR;
                bool value;
                if (1 == lwm2m_data_decode_bool(dataArray + i, &value))
                {
                    if (1 == prv_write_lever(value)) result = COAP_204_CHANGED;
                }

                break;
            }

            default:
                return COAP_404_NOT_FOUND;
        }
        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_led_execute(uint16_t instanceId,
                                  uint16_t resourceId,
                                  uint8_t * buffer,
                                  int length,
                                  lwm2m_object_t * objectP)

{
    led_instance_t *targetP;

    targetP = (led_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    switch (resourceId)
    {
        default:
            return COAP_405_METHOD_NOT_ALLOWED;
    }
}

static uint8_t prv_led_delete(uint16_t id, lwm2m_object_t *objectP)
{
    led_instance_t *oneInstance;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&oneInstance);
    if (NULL == oneInstance) return COAP_404_NOT_FOUND;

    lwm2m_free(oneInstance);

    return COAP_202_DELETED;
}

static uint8_t prv_led_create(uint16_t instanceId,
                                 int numData,
                                 lwm2m_data_t * dataArray,
                                 lwm2m_object_t * objectP)
{
    led_instance_t *oneInstance;
    uint8_t result;

    oneInstance = (led_instance_t *)lwm2m_malloc(sizeof(led_instance_t));
    if (NULL == oneInstance) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(oneInstance, 0, sizeof(led_instance_t));

    oneInstance->instanceId = instanceId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, oneInstance);

    result = prv_led_write(instanceId, numData, dataArray, objectP);
    if (result != COAP_204_CHANGED)
    {
        (void)prv_led_delete(instanceId, objectP);
    }

    else
    {
        result = COAP_201_CREATED;
    }

    return result;
}

static void prv_led_close(lwm2m_object_t *object)
{
    while (object->instanceList != NULL)
    {
        led_instance_t *oneInstance = (led_instance_t *)object->instanceList;
        object->instanceList = object->instanceList->next;
        lwm2m_free(oneInstance);
    }
}

lwm2m_object_t *make_led_object()
{
    lwm2m_object_t *oneObj;

    oneObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != oneObj)
    {
        led_instance_t *oneInstance;
        memset(oneObj, 0, sizeof(lwm2m_object_t));

        oneObj->objID = 3311;

        // Manually create an hardcoded one
        oneInstance = (led_instance_t *) lwm2m_malloc(sizeof(led_instance_t));
        if (NULL == oneInstance)
        {
            lwm2m_free(oneObj);
            return NULL;
        }

        memset(oneInstance, 0, sizeof(led_instance_t));
        prv_init_device();

        oneInstance->instanceId = 0;

        oneObj->instanceList = LWM2M_LIST_ADD(oneObj->instanceList, oneInstance);

        oneObj->readFunc = prv_led_read;
        oneObj->writeFunc = prv_led_write;
        oneObj->createFunc = prv_led_create;
        oneObj->deleteFunc = prv_led_delete;
        oneObj->executeFunc = prv_led_execute;
        oneObj->closeFunc = prv_led_close;
    }

    return oneObj;
}
