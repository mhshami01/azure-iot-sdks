#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for BeagleBone Green Client

cmake_minimum_required(VERSION 3.0)

project(beaglebonegreen_kit)

add_definitions(-Wno-unused-result)
add_definitions(-DLWM2M_CLIENT_MODE -DWITH_LOGS -DLWM2M_LITTLE_ENDIAN -DCOAP_TCP)

set(LIBIOTDM_DIR ${PROJECT_SOURCE_DIR}/../..)
set(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../../wakaama)
set(LIBCOMMON_DIR ${PROJECT_SOURCE_DIR}/../../../common)

set(common_c_files
	${LIBCOMMON_DIR}/adapters/agenttime.c
	${LIBCOMMON_DIR}/src/list.c
)

if(WIN32)
set(common_platform_c_files
	${LIBCOMMON_DIR}/adapters/httpapi_winhttp.c
	${LIBCOMMON_DIR}/adapters/lock_win32.c
	${LIBCOMMON_DIR}/adapters/platform_win32.c
	${LIBCOMMON_DIR}/adapters/threadapi_win32.c
)
else()
set(common_platform_c_files
	${LIBCOMMON_DIR}/adapters/lock_pthreads.c
	${LIBCOMMON_DIR}/adapters/socketio_berkeley.c
	${LIBCOMMON_DIR}/adapters/threadapi_pthreads.c
)
endif()

set(sample_c_files
	object_device.c
	object_humidity_sensor.c
	object_led.c
	object_security.c
	object_server.c
	object_template.c
	object_temp_sensor.c
	program.c
)

set(sample_c_files
	program.h
)

IF(WIN32)
	#windows needs this define
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(.)

#use env and macros in lieu of the ../..
include_directories(${LIBIOTDM_DIR}/inc)
include_directories(${LIBLWM2M_DIR}/core)
include_directories(${LIBCOMMON_DIR}/inc)

add_executable(beaglebonegreen_kit
	${sample_c_files}
	${common_c_files}
	${common_platform_c_files}
)

target_link_libraries(beaglebonegreen_kit iotdm_client)
	
if(WIN32)
else()
	target_link_libraries(beaglebonegreen_kit "-static-libgcc -static")
	target_link_libraries(beaglebonegreen_kit m pthread)
endif()

