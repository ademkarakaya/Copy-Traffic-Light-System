/*
 * RailCentralController.h
 *
 *  Created on: 14Oct.,2019
 *      Author: NickM_e75acww
 */

#ifndef RAILCENTRALCONTROLLER_H_
#define RAILCENTRALCONTROLLER_H_

#include <traffic_file_lib.h>
#include <traffic_library.h>
#include <traffic_types.h>

typedef struct {
	rail_state_data_t * rail_state_data;
} critical_client_thread_data_t;

typedef struct {
	rail_state_data_t * rail_state_data;
} critical_server_thread_data_t;

#endif /* RAILCENTRALCONTROLLER_H_ */
