#pragma once

#include "traffic_file_lib.h"
#include "traffic_types.h"



typedef struct {

	const char * attach_point;
	msg_data_t * msg;
	msg_reply_t * reply;
	pthread_rwlock_t rwlock;

} send_message_thread_data_t;


void * sendMessage(void *data);


void setThreadSettings(pthread_attr_t * th_attr, struct sched_param * th_sched_param, const int priority, const bool detached);

void initTrafficStateData(traffic_state_data_t *traffic_state_data);

void initRailStateData(rail_state_data_t *rail_state_data);

void initFaultData(fault_data_t *fault_data);

void initResetData(reset_data_t *reset_data);



name_attach_t *createChannel(const char *attach_point);

int connectChannel(const char *attach_point);

void handlePulse(msg_data_t *msg);

void handleIOConnect(msg_data_t *msg, int rcvid);
