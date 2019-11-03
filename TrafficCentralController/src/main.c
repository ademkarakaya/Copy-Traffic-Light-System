#include <stdio.h>
#include <stdlib.h>

#include "Input.h"
#include "TCC_MessagePassing.h"
#include "TrafficCentralController.h"

void initData(traffic_state_data_t *i1_state_data,
			  traffic_state_data_t *i2_state_data,
			  rail_state_data_t *x1_state_data,
			  tcc_state_data_t *tcc_state_data,
			  input_thread_data_t * input_thread_data,
			  critical_server_thread_data_t *critical_server_thread_data,
			  traffic_mode_data_t * traffic_mode_data,
			  msg_data_t *msg_send,
			  send_message_thread_data_t *i1_send,
			  get_state_thread_data_t *request_i_state_data) {

	initTrafficStateData(i1_state_data);
	initTrafficStateData(i1_state_data);
	initRailStateData(x1_state_data);
	initModeData(traffic_mode_data);

	tcc_state_data->i1_state_data = i1_state_data;
	tcc_state_data->i2_state_data = i2_state_data;
	tcc_state_data->x1_state_data = x1_state_data;
	tcc_state_data->traffic_mode_data = traffic_mode_data;

	input_thread_data->tcc_state_data = tcc_state_data;

	critical_server_thread_data->x1_state_data = x1_state_data;

	msg_send->client_id = TCC_CLIENT_ID;
	msg_send->hdr.type = RESET_MSG;
	msg_send->data.reset = true;

	i1_send->attach_point = I1_CRITICAL_CHANNEL_ATTACH_POINT;
	i1_send->msg = msg_send;

	request_i_state_data->i1_state_data = i1_state_data;
	request_i_state_data->i2_state_data = i2_state_data;
}

int main(void) {

	traffic_state_data_t i1_state_data;
	traffic_state_data_t i2_state_data;
	rail_state_data_t x1_state_data;
	tcc_state_data_t tcc_state_data;
	input_thread_data_t input_thread_data;
	critical_server_thread_data_t critical_server_thread_data;
	traffic_mode_data_t traffic_mode_data;
	get_state_thread_data_t request_i_state_data;

	msg_data_t msg_send;
	send_message_thread_data_t i1_send;

	initData(&i1_state_data,
			 &i2_state_data,
			 &x1_state_data,
			 &tcc_state_data,
			 &input_thread_data,
			 &critical_server_thread_data,
			 &traffic_mode_data,
			 &msg_send,
			 &i1_send,
			 &request_i_state_data);
	
	// Thread definitions
	pthread_t th_input; // define
	pthread_t th_critical_server_channel;

	// Thread attributes
	pthread_attr_t th_input_attr; // define
	pthread_attr_t th_critical_server_channel_attr;
	pthread_attr_t th_send_attr;

	// Thread schedule parameters
	struct sched_param th_input_param; // define
	struct sched_param th_critical_server_channel_param;
	struct sched_param th_send_param;

	// Set thread attributes
	setThreadSettings(&th_critical_server_channel_attr, &th_critical_server_channel_param, TCC_CRIT_SERVER_CHANNEL_THREAD_PRIORITY, false);
	setThreadSettings(&th_send_attr, &th_send_param, TCC_SEND_THREAD_PRIORITY, false);

	// Spawn threads
	pthread_create(&th_input, NULL, &inputThread, &input_thread_data);
	pthread_create(&th_critical_server_channel, &th_critical_server_channel_attr, &critical_server_channel_thread, &critical_server_thread_data);

	pthread_join(th_input, NULL);
	pthread_join(th_critical_server_channel, NULL);
	
	printf("Main terminating\n");
	return EXIT_SUCCESS;
}
