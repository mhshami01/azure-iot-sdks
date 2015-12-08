#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.0)
#this is CMakeLists for iothub_client

#compileAsC99()
#set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")

set (CMAKE_C_FLAGS "-DLWM2M_CLIENT_MODE -DWITH_LOGS -DLWM2M_LITTLE_ENDIAN ${CMAKE_C_FLAGS}")
set (CMAKE_C_FLAGS "-Wno-unused-result -DCOAP_TCP ${CMAKE_C_FLAGS}")

set(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../wakaama)
set(wakaama_c_files
	${LIBLWM2M_DIR}/core/bootstrap.c
	${LIBLWM2M_DIR}/core/json.c
	${LIBLWM2M_DIR}/core/liblwm2m.c
	${LIBLWM2M_DIR}/core/list.c
	${LIBLWM2M_DIR}/core/management.c
	${LIBLWM2M_DIR}/core/memtrace.c
	${LIBLWM2M_DIR}/core/objects.c
	${LIBLWM2M_DIR}/core/observe.c
	${LIBLWM2M_DIR}/core/packet.c
	${LIBLWM2M_DIR}/core/registration.c
	${LIBLWM2M_DIR}/core/tlv.c
	${LIBLWM2M_DIR}/core/transaction.c
	${LIBLWM2M_DIR}/core/uri.c
	${LIBLWM2M_DIR}/core/utils.c
	${LIBLWM2M_DIR}/core/er-coap-13/er-coap-13.c
)

set(wakaama_h_files
	${LIBLWM2M_DIR}/core/internals.h
	${LIBLWM2M_DIR}/core/liblwm2m.h
	${LIBLWM2M_DIR}/core/memtrace.h
	${LIBLWM2M_DIR}/core/er-coap-13/er-coap-13.h
)

set(LIBCOMMON_DIR ${PROJECT_SOURCE_DIR}/../common)

set(iotdm_client_c_files
	./src/iotdm_client.c
)

set(iotdm_client_h_files
	./inc/iotdm_client.h
)

#these are the include folders
set(IOTDM_CLIENT_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using iothub_client lib" FORCE)

include_directories(${LIBLWM2M_DIR}/core)
include_directories(${LIBLWM2m_DIR}/core/er-coap-13)
include_directories(${IOTDM_CLIENT_INC_FOLDER})
include_directories(${LIBCOMMON_DIR}/inc)

IF(WIN32)
	#windows needs this define
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

add_library(iotdm_client
	${iotdm_client_c_files}
	${wakaama_c_files}
)

add_subdirectory(samples)
