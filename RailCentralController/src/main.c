#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "RCC_MessagePassing.h"
#include "RailCentralController.h"
#include <traffic_library.h>
#include <traffic_types.h>

void initData(rail_state_data_t *rail_state_data,
			  critical_client_thread_data_t *critical_client_channel_thread_data,
			  critical_server_thread_data_t *critical_server_channel_thread_data) {

	initRailStateData(rail_state_data);

	critical_client_channel_thread_data->rail_state_data = rail_state_data;
	critical_server_channel_thread_data->rail_state_data = rail_state_data;
}

int main(void) {

	rail_state_data_t rail_state_data;
	critical_client_thread_data_t critical_client_channel_thread_data;
	critical_server_thread_data_t critical_server_channel_thread_data;

	initData(&rail_state_data,
			 &critical_client_channel_thread_data,
			 &critical_server_channel_thread_data);

	// Thread definitions
	pthread_t th_critical_client_channel;
	pthread_t th_critical_server_channel;

	// Thread attributes
	pthread_attr_t th_critical_client_channel_attr;
	pthread_attr_t th_critical_server_channel_attr;

	// Thread schedule parameters
	struct sched_param th_critical_client_channel_param;
	struct sched_param th_critical_server_channel_param;
	
	// Set thread attributes
	setThreadSettings(&th_critical_client_channel_attr, &th_critical_client_channel_param, RCC_CRIT_CLIENT_CHANNEL_THREAD_PRIORITY, true);
	setThreadSettings(&th_critical_server_channel_attr, &th_critical_server_channel_param, RCC_CRIT_SERVER_CHANNEL_THREAD_PRIORITY, false);
	
	// Spawn threads
	pthread_create(&th_critical_client_channel, &th_critical_client_channel_attr, &critical_client_channel_thread, &critical_client_channel_thread_data);
	pthread_create(&th_critical_server_channel, &th_critical_server_channel_attr, &critical_server_channel_thread, &critical_server_channel_thread_data);

	pthread_join(th_critical_server_channel, NULL);

	printf("main terminating\n");
	return EXIT_SUCCESS;
}
