
/*
 *  Resources:
 *
 *          Name           | ID   | Operations | Instances | Mandatory |  Type   |  Range  | Units |
 *    Sensor Value         | 5700 |     R      |     1     |    yes    |  Float  |         |       |
 *    Units                | 5701 |     R      |     1     |    No     |  String |         |       |
 *    Min Measured Value   | 5601 |     R      |     1     |    No     |  Float  |         |       |
 *    Max Measured Value   | 5602 |     R      |     1     |    No     |  Float  |         |       |
 *    Min Range Value      | 5603 |     R      |     1     |    No     |  Float  |         |       |
 *    Max Range Value      | 5604 |     R      |     1     |    No     |  Float  |         |       |
 *    Reset Measured Values| 5605 |     E      |     1     |    No     |  N/A    |         |       |
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simple_client.h"

typedef struct _humidity_sensor_instance_
{
    struct _humidity_sensor_instance_ *next;    // matches lwm2m_list_t::next
    uint16_t    instanceId;            // matches lwm2m_list_t::id

    float     value;
    float     minValue;
    float     maxValue;
    float     minRange;
    float     maxRange;
} humidity_sensor_instance_t;

float prv_get_current_humidity()
{
    float retValue = 0.0;

    return retValue;
}

static uint8_t prv_get_value(lwm2m_data_t *tlvP, humidity_sensor_instance_t *targetP)
{
    // There are no multiple instance resources
    tlvP->type = LWM2M_TYPE_RESOURCE;
    switch (tlvP->id)
    {
        case 5700:
            lwm2m_data_encode_float(targetP->value = prv_get_current_temperature(), tlvP);
            break;

        case 5601:
            lwm2m_data_encode_float(targetP->minValue, tlvP);
            break;

        case 5602:
            lwm2m_data_encode_float(targetP->maxValue, tlvP);
            break;

        case 5603:
            lwm2m_data_encode_float(targetP->minRange, tlvP);
            break;

        case 5604:
            lwm2m_data_encode_float(targetP->maxRange, tlvP);
            break;

        default:
            return COAP_404_NOT_FOUND;
    }

    if (0 == tlvP->length) return COAP_500_INTERNAL_SERVER_ERROR;
    else return COAP_205_CONTENT;
}

static uint8_t prv_humidity_sensor_read(uint16_t instanceId,
                               int *numDataP,
                               lwm2m_data_t **dataArrayP,
                               lwm2m_object_t *objectP)
{
    humidity_sensor_instance_t *targetP;
    uint8_t result;
    int i;

    targetP = (humidity_sensor_instance_t *) lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    // is the server asking for the full instance ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = { 5700, 5701, 5601, 5602, 5603, 5604 };
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

static uint8_t prv_humidity_sensor_write(uint16_t instanceId,
                                int numData,
                                lwm2m_data_t *dataArray,
                                lwm2m_object_t *objectP)
{
    humidity_sensor_instance_t *targetP;
    int i;
    uint8_t result;
#ifdef LWM2M_BOOTSTRAP
    bool bootstrapPending;

    bootstrapPending = (dataArray->flags & LWM2M_TLV_FLAG_BOOTSTRAPPING) != 0;
#endif
    targetP = (humidity_sensor_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP)
    {
        return COAP_404_NOT_FOUND;
    }

    i = 0;
    do
    {
        switch (dataArray[i].id)
        {
        default:
            return COAP_404_NOT_FOUND;
        }
        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_humidity_sensor_execute(uint16_t instanceId,
                                  uint16_t resourceId,
                                  uint8_t *buffer,
                                  int length,
                                  lwm2m_object_t *objectP)

{
    humidity_sensor_instance_t *targetP;

    targetP = (humidity_sensor_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;
    
    switch (resourceId)
    {
        case 5605:
            targetP->minValue = targetP->maxValue = 0.0;
            return COAP_204_CHANGED;

        default:
            return COAP_405_METHOD_NOT_ALLOWED;
    }
}

static uint8_t prv_humidity_sensor_delete(uint16_t id, lwm2m_object_t *objectP)
{
    humidity_sensor_instance_t *oneInstance;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&oneInstance);
    if (NULL == oneInstance) return COAP_404_NOT_FOUND;

    lwm2m_free(oneInstance);

    return COAP_202_DELETED;
}

static uint8_t prv_humidity_sensor_create(uint16_t instanceId,
                                 int numData,
                                 lwm2m_data_t *dataArray,
                                 lwm2m_object_t *objectP)
{
    humidity_sensor_instance_t *oneInstance;
    uint8_t result;

    oneInstance = (humidity_sensor_instance_t *)lwm2m_malloc(sizeof(humidity_sensor_instance_t));
    if (NULL == oneInstance) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(oneInstance, 0, sizeof(humidity_sensor_instance_t));

    oneInstance->instanceId = instanceId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, oneInstance);

    result = prv_humidity_sensor_write(instanceId, numData, dataArray, objectP);
    if (result != COAP_204_CHANGED)
    {
        (void)prv_humidity_sensor_delete(instanceId, objectP);
    }

    else
    {
        result = COAP_201_CREATED;
    }

    return result;
}

static void prv_humidity_sensor_close(lwm2m_object_t *object)
{
    while (object->instanceList != NULL)
    {
        humidity_sensor_instance_t *oneInstance = (humidity_sensor_instance_t *)object->instanceList;
        object->instanceList = object->instanceList->next;
        lwm2m_free(oneInstance);
    }
}

void display_sensor_object(lwm2m_object_t * object)
{
}

lwm2m_object_t *make_humidity_sensor_object()
{
    lwm2m_object_t *oneObj;

    oneObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));
    if (NULL != oneObj)
    {
        humidity_sensor_instance_t *oneInstance;
        memset(oneObj, 0, sizeof(lwm2m_object_t));

        oneObj->objID = 3304;

        // Manually create an hardcoded one
        oneInstance = (humidity_sensor_instance_t *) lwm2m_malloc(sizeof(humidity_sensor_instance_t));
        if (NULL == oneInstance)
        {
            lwm2m_free(oneObj);
            return NULL;
        }

        memset(oneInstance, 0, sizeof(humidity_sensor_instance_t));

        oneInstance->instanceId = 0;

        oneObj->instanceList = LWM2M_LIST_ADD(oneObj->instanceList, oneInstance);

        oneObj->readFunc = prv_humidity_sensor_read;
        oneObj->writeFunc = prv_humidity_sensor_write;
        oneObj->createFunc = prv_humidity_sensor_create;
        oneObj->deleteFunc = prv_humidity_sensor_delete;
        oneObj->executeFunc = prv_humidity_sensor_execute;
        oneObj->closeFunc = prv_humidity_sensor_close;
    }

    return oneObj;
}
