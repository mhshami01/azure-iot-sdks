#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "platform.h"
#include "socketio.h"
//#include "tlsio_schannel.h"

#include "liblwm2m.h"
#include "iotdm_client.h"

#define MAX_PACKET_SIZE 1024


#ifdef WITH_LOGS
#define PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define PRINT(...)
#endif


typedef enum IOTDM_CLIENT_STATE_TAG
{
    BLOCKED,
    LENGTH,
    RECEIVING
} IOTDM_CLIENT_STATE;


typedef struct IO_BUFFER_TAG
{
    uint16_t  length;
    uint16_t  available;
    uint8_t   buffer[MAX_PACKET_SIZE];
} IO_BUFFER;


typedef struct CLIENT_DATA_TAG
{
	lwm2m_context_t   *session;
	lwm2m_object_t    *securityObject;
	lwm2m_object_t    *serverObject;
	lwm2m_object_t    **allObjects;
	size_t              nrObjects;
    uint32_t            step;

    XIO_HANDLE          ioHandle;
    IOTDM_CLIENT_STATE  state;
    IO_BUFFER           input;
} CLIENT_DATA;


static size_t currentObject = 2;


/**
 *  Must be implemented by the security object. please see object_security.c provided with the samples.
 */
char *get_server_uri(lwm2m_object_t * objectP, uint16_t secObjInstID);


uint32_t parse_int(uint8_t *bytes, size_t length)
{
    uint32_t retVal = 0;
    size_t   i = 0;

    while (i < length)
    {
        retVal <<= 8;
        retVal |= bytes[i++];
    }

    return retVal;
}


uint16_t prv_min(uint16_t x, uint16_t y)
{
    if (x < y) return x;
	return y;
}


ON_BYTES_RECEIVED on_bytes_received(void *context, const char *buffer, size_t size)
{
    CLIENT_DATA *client = (CLIENT_DATA *)context;

    switch (client->state)
    {
    case BLOCKED:
        memcpy(&(client->input.buffer[client->input.available]), buffer, size);
        client->input.available += size;
        client->state = LENGTH;

        break;

    case LENGTH:
        memcpy(&(client->input.buffer[client->input.available]), buffer, size);
        client->input.length = parse_int(client->input.buffer, 2);
        client->input.available += size;
        client->state = RECEIVING;

        break;

    case RECEIVING:
    {
        size_t toCopy = prv_min(size, client->input.length + 2 - client->input.available);
        memcpy(&(client->input.buffer[client->input.available]), buffer, toCopy);
        client->input.available += toCopy;

        if ((client->input.available - 2) == client->input.length)
        {
            PRINT(" on_bytes_reived - msgLength: %d\n", client->input.length);
            lwm2m_handle_packet(client->session, &(client->input.buffer[2]), client->input.length, client->ioHandle);

            /* reset input buffer */
            memset(client->input.buffer, 0, MAX_PACKET_SIZE);

            /* compute the size of data left */
            size -= toCopy;
            if (size > 0)
            {
                memcpy(client->input.buffer, (buffer[toCopy]), size);
                client->input.available = size;
            }

            else
            {
                client->input.available = 0;
            }

            client->state = BLOCKED;
        }

        break;
    }

    default:
        PRINT("  Illegal client state.");

        break;
    }
}


ON_IO_STATE_CHANGED on_io_state_changed(void *context, IO_STATE new_io_state, IO_STATE old_io_state)
{
}


static void* connect_server(uint16_t objectID, void *userData)
{
	CLIENT_DATA *cd = (CLIENT_DATA *)userData;
	char        *uri = get_server_uri(cd->securityObject, objectID);
	if (NULL == uri)
	{
		return uri;
	}

	PRINT("    Connect to:'%s'\n", uri);

	// parse uri in the form "coaps://[host]:[port]"
	char *host;
	if (0 == strncmp(uri, "coaps://", strlen("coaps://")))
	{
		host = uri + strlen("coaps://");
	}

	else if (0 == strncmp(uri, "coap://", strlen("coap://")))
	{
		host = uri + strlen("coap://");
	}

	else if (0 == strncmp(uri, "http://", strlen("http://")))
	{
		host = uri + strlen("http://");
	}

	else
	{
		lwm2m_free(uri);
		return NULL;
	}

	char *port = strrchr(host, ':');
	if (port == NULL)
	{
		lwm2m_free(uri);
		return NULL;
	}

	// remove brackets
	if (host[0] == '[')
	{
		host++;
		if (*(port - 1) == ']')
		{
			*(port - 1) = 0;
		}

		else
		{
			lwm2m_free(uri);
			return NULL;
		}
	}

	// split strings
	*port = 0;
	port++;

//  TLSIO_SCHANNEL_CONFIG sCFG;
    SOCKETIO_CONFIG sCFG;
    sCFG.hostname = host;
    sCFG.port = atoi(port);

#if defined(WIN32)
    /*
    cd->ioHandle = tlsio_schannel_create(&sCFG, NULL);
    if (0 != tlsio_schannel_open(cd->ioHandle, on_bytes_received, on_io_state_changed, cd))
    {
        cd->ioHandle = NULL;
    }
    */
#endif

    cd->ioHandle = socketio_create(&sCFG, NULL);
    if (0 != socketio_open(cd->ioHandle, on_bytes_received, on_io_state_changed, cd))
    {
        cd->ioHandle = NULL;
    }

	lwm2m_free(uri);
	return (void *)cd->ioHandle;
}


static void print_indent(FILE *stream, int num)
{
	int i;

	for (i = 0; i < num; i++)
		fprintf(stream, "    ");
}


static void output_buffer(FILE *stream, uint8_t *buffer, int length, int indent)
{
	int i;

	if (length == 0) fprintf(stream, "\n");

	i = 0;
	while (i < length)
	{
		uint8_t array[16];
		int j;

		print_indent(stream, indent);
		memcpy(array, buffer + i, 16);
		for (j = 0; j < 16 && i + j < length; j++)
		{
			fprintf(stream, "%02X ", array[j]);
			if (j % 4 == 3) fprintf(stream, " ");
		}

		if (length > 16)
		{
			while (j < 16)
			{
				fprintf(stream, "   ");
				if (j % 4 == 3) fprintf(stream, " ");
				j++;
			}
		}

		fprintf(stream, " ");
		for (j = 0; j < 16 && i + j < length; j++)
		{
			if (isprint(array[j]))
				fprintf(stream, "%c", array[j]);
			else
				fprintf(stream, ".");
		}

		fprintf(stream, "\n");
		i += 16;
	}
}


static uint8_t send_to_server(void *context, uint8_t *buffer, size_t length, void *userData)
{
    XIO_HANDLE io_handle = (XIO_HANDLE)context;

	PRINT("    Sending %zd bytes\n", length);
#ifdef WITH_LOGS
	output_buffer(stderr, buffer, length, 2);
#endif

	/** first, send the length of the message */
    uint8_t data[2];

    data[0] = (uint8_t)(length >> 8);
    data[1] = (uint8_t)(length >> 0);

//  int nbSent = tlsio_schannel_send(io_handle, data, 2, NULL, NULL);
    int nbSent = socketio_send(io_handle, data, 2, NULL, NULL);
	if (0 != nbSent) return COAP_500_INTERNAL_SERVER_ERROR;

    /** now the message */
//  nbSent = tlsio_schannel_send(io_handle, buffer, length, NULL, NULL);
    nbSent = socketio_send(io_handle, buffer, length, NULL, NULL);
    if (0 != nbSent) return COAP_500_INTERNAL_SERVER_ERROR;

	return COAP_NO_ERROR;
}

/**
 *
 */
IOTDM_CLIENT_HANDLE IoTDMClient_Create()
{
	CLIENT_DATA *returnValue = (CLIENT_DATA *) lwm2m_malloc(sizeof(CLIENT_DATA));

	if (NULL != returnValue)
	{
		memset(returnValue, 0, sizeof(CLIENT_DATA));
	}

	return (IOTDM_CLIENT_HANDLE) returnValue;
}


IOTDM_CLIENT_RESULT IoTDMClient_Initialize(IOTDM_CLIENT_HANDLE h)
{
	if (NULL == h)
	{
		PRINT("Null client handle\r\n");
		return IOTMD_CLIENT_INVALID_ARG;
	}

#if defined(WIN32) // is temporarily needed untill a fix is provided in the IoT Shared library.
    platform_init();
#endif

	CLIENT_DATA *client = (CLIENT_DATA *)h;

	client->session = lwm2m_init(&connect_server, &send_to_server, client);
	if (NULL == client->session)
	{
		PRINT("    failed to initiate a client lwm2m session.\n");
		return IOTDM_CLIENT_ERROR;
	}

	int result = lwm2m_configure(client->session, "JEdison", NULL, NULL, client->nrObjects, client->allObjects);
	if (0 != result)
	{
		PRINT("    failed to configure lwm2m session\n");
		return IOTDM_CLIENT_ERROR;
	}

	result = lwm2m_start(client->session);
	if (0 != result)
	{
		PRINT("    failed to start lwm2m session\n");
		return IOTDM_CLIENT_ERROR;
	}

    time_t temp = 0;
    result = lwm2m_step(client->session, &temp);
    if (0 != result)
    {
        PRINT("    failed to register lwm2m server\n");
        return IOTDM_CLIENT_ERROR;
    }

    return IOTDM_CLIENT_OK;
}


static const char *NR_OBJECTS = "Number Of Objects";
IOTDM_CLIENT_RESULT IoTDMClient_SetOption(IOTDM_CLIENT_HANDLE h, const char *optionName, const void *value)
{
	if ((NULL == h) || (NULL == value))
	{
		return IOTMD_CLIENT_INVALID_ARG;
	}

	CLIENT_DATA *client = (CLIENT_DATA *) h;
	if (0 == strncmp(NR_OBJECTS, optionName, strlen(NR_OBJECTS)))
	{
		client->nrObjects = *((int *) value);
		client->allObjects = (lwm2m_object_t **)lwm2m_malloc(client->nrObjects * sizeof(lwm2m_object_t *));
		if (NULL == client->allObjects)
		{
			return IOTDM_CLIENT_ERROR;
		}
	}

    else
    {
        // unknown option
        return IOTDM_CLIENT_ERROR;
    }

	return IOTDM_CLIENT_OK;
}


static const char *SECURITY_OBJECT_NAME = "Security Object";
static const char *SERVER_OBJECT_NAME = "Server Object";
IOTDM_CLIENT_RESULT IoTDMClient_AddNewObject(IOTDM_CLIENT_HANDLE h, const char *optionName, const void *value)
{
	if ((NULL == h) || (NULL == value))
	{
		return IOTMD_CLIENT_INVALID_ARG;
	}

	CLIENT_DATA *client = (CLIENT_DATA *)h;
	if (NULL == client->allObjects)
	{
		return IOTDM_CLIENT_ERROR;
	}

	if (0 == strncmp(SECURITY_OBJECT_NAME, optionName, strlen(SECURITY_OBJECT_NAME)))
	{
		client->allObjects[0] = client->securityObject = (lwm2m_object_t *) value;
	}

	else if (0 == strncmp(SERVER_OBJECT_NAME, optionName, strlen(SERVER_OBJECT_NAME)))
	{
		client->allObjects[1] = client->serverObject = (lwm2m_object_t *) value;
	}

	else
	{
		if (currentObject == client->nrObjects)
		{
			return IOTDM_CLIENT_ERROR;
		}

		client->allObjects[currentObject++] = (lwm2m_object_t *) value;
	}

	return IOTDM_CLIENT_OK;
}


void IoTDMClient_Destroy(IOTDM_CLIENT_HANDLE h)
{
	if (NULL != h)
	{
		CLIENT_DATA *client = (CLIENT_DATA *)h;

		lwm2m_close(client->session);
//      tlsio_schannel_destroy(client->ioHandle);
        socketio_destroy(client->ioHandle);
        lwm2m_free(client);
	}

#if defined(WIN32) // is temporarily needed untill a fix is provided in the IoT Shared library.
    platform_deinit();
#endif
}


/***------------------------------------------------------------------- */
void IoTDMClient_DoWork(IOTDM_CLIENT_HANDLE h)
{
	if (NULL == h)
	{
		PRINT("Null client handle\r\n");
		return;
	}

	CLIENT_DATA *client = (CLIENT_DATA *)h;

    /* check for pending requests. */
//  tlsio_schannel_dowork(client->ioHandle);
    socketio_dowork(client->ioHandle);

    /* process any updates that are due. */
//  process_observed(client);
}
