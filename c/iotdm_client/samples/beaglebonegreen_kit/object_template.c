
/*
 *  Resources:
 *
 *          Name         | ID | Operations | Instances | Mandatory |  Type   |  Range  | Units |
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simple_client.h"

typedef struct _OBJECT_instance_
{
    struct _OBJECT_instance_ * next;   // matches lwm2m_list_t::next
    uint16_t    instanceId;            // matches lwm2m_list_t::id
} OBJECT_instance_t;

static uint8_t prv_get_value(lwm2m_data_t * tlvP, OBJECT_instance_t * targetP)
{
    // There are no multiple instance resources
    tlvP->type = LWM2M_TYPE_RESOURCE;

    switch (tlvP->id)
    {
        default:
            return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_OBJECT_read(uint16_t instanceId,
                               int * numDataP,
                               lwm2m_data_t ** dataArrayP,
                               lwm2m_object_t * objectP)
{
    OBJECT_instance_t *targetP;
    uint8_t result;
    int i;

    targetP = (OBJECT_instance_t *) lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    // is the server asking for the full instance ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {
        };
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

static uint8_t prv_set_int_value(lwm2m_data_t * dataArray, uint32_t * data)
{
    uint8_t result;
    int64_t value;

    if (1 == lwm2m_data_decode_int(dataArray, &value))
    {
        if (value >= 0 && value <= 0xFFFFFFFF)
        {
            *data = value;
            result = COAP_204_CHANGED;
        }

        else
        {
            result = COAP_406_NOT_ACCEPTABLE;
        }
    }

    else
    {
        result = COAP_400_BAD_REQUEST;
    }

    return result;
}

static uint8_t prv_OBJECT_write(uint16_t instanceId,
                                int numData,
                                lwm2m_data_t * dataArray,
                                lwm2m_object_t * objectP)
{
    OBJECT_instance_t *targetP;
    int i;
    uint8_t result;

    targetP = (OBJECT_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
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

static uint8_t prv_OBJECT_execute(uint16_t instanceId,
                                  uint16_t resourceId,
                                  uint8_t * buffer,
                                  int length,
                                  lwm2m_object_t * objectP)

{
    OBJECT_instance_t * targetP;

    targetP = (OBJECT_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;
    
    switch (resourceId)
    {
        default:
            return COAP_405_METHOD_NOT_ALLOWED;
    }
}

static uint8_t prv_OBJECT_delete(uint16_t id,
                                 lwm2m_object_t * objectP)
{
    OBJECT_instance_t *oneInstance;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&oneInstance);
    if (NULL == oneInstance) return COAP_404_NOT_FOUND;

    lwm2m_free(oneInstance);

    return COAP_202_DELETED;
}

static uint8_t prv_OBJECT_create(uint16_t instanceId,
                                 int numData,
                                 lwm2m_data_t   *dataArray,
                                 lwm2m_object_t *objectP)
{
    OBJECT_instance_t *oneInstance;
    uint8_t result;

    oneInstance = (OBJECT_instance_t *)lwm2m_malloc(sizeof(OBJECT_instance_t));
    if (NULL == oneInstance) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(oneInstance, 0, sizeof(OBJECT_instance_t));

    oneInstance->instanceId = instanceId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, oneInstance);

    result = prv_OBJECT_write(instanceId, numData, dataArray, objectP);
    if (result != COAP_204_CHANGED)
    {
        (void)prv_OBJECT_delete(instanceId, objectP);
    }

    else
    {
        result = COAP_201_CREATED;
    }

    return result;
}

static void prv_OBJECT_close(lwm2m_object_t * object)
{
    while (object->instanceList != NULL)
    {
        OBJECT_instance_t *oneInstance = (OBJECT_instance_t *)object->instanceList;
        object->instanceList = object->instanceList->next;
        lwm2m_free(oneInstance);
    }
}

lwm2m_object_t *make_OBJECT_object()
{
    lwm2m_object_t *oneObj;

    oneObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != oneObj)
    {
        OBJECT_instance_t *oneInstance;
        memset(oneObj, 0, sizeof(lwm2m_object_t));

        oneObj->objID = -1;

        // Manually create an hardcoded one
        oneInstance = (OBJECT_instance_t *) lwm2m_malloc(sizeof(OBJECT_instance_t));
        if (NULL == oneInstance)
        {
            lwm2m_free(oneObj);
            return NULL;
        }

        memset(oneInstance, 0, sizeof(OBJECT_instance_t));

        oneInstance->instanceId = 0;

        oneObj->instanceList = LWM2M_LIST_ADD(oneObj->instanceList, oneInstance);

        oneObj->readFunc = prv_OBJECT_read;
        oneObj->writeFunc = prv_OBJECT_write;
        oneObj->createFunc = prv_OBJECT_create;
        oneObj->deleteFunc = prv_OBJECT_delete;
        oneObj->executeFunc = prv_OBJECT_execute;
        oneObj->closeFunc = prv_OBJECT_close;
    }

    return oneObj;
}
