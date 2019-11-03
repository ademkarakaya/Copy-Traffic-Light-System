#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "IC_MessagePassing.h"
#include "Input.h"
#include "IntersectionController.h"
#include "Output.h"
#include "stateMachine.h"

void initTrafficModeData(traffic_mode_data_t *traffic_mode_data) {
	traffic_mode_data->mode = PEAK;
	pthread_rwlock_init(&traffic_mode_data->rwlock, NULL);
}

void initInputQueue(traffic_input_queue_t *input_queue) {
	input_queue->length = 0;
	for (int i = 0; i < TRAFFIC_INPUT_COUNT; i++) {
		input_queue->input_buffer[i] = 0;
	}
}

void initInputData(traffic_input_data_t *input_data, traffic_input_queue_t *input_queue) {
	input_data->input_queue = input_queue;
	pthread_rwlock_init(&input_data->rwlock_input, NULL);
}

void initChangeStateFlags(change_state_flags_t *change_state_flags) {
	change_state_flags->state_timer_timeout = false;
	change_state_flags->ped_timer_timeout = false;
	change_state_flags->ped_sense_timer_timeout = false;
	change_state_flags->input = false;
	change_state_flags->fault = false;
	change_state_flags->reset = false;
}

void initData(traffic_mode_data_t *traffic_mode_data,
			  traffic_input_queue_t *input_queue,
			  traffic_input_data_t *input_data,
			  traffic_state_data_t *traffic_state_data,
			  rail_state_data_t *rail_state_data,
			  state_machine_thread_data_t *state_machine_thread_data,
			  input_thread_data_t *input_thread_data,
			  output_thread_data_t *output_thread_data,
			  critical_server_data_t *critical_server_data,
			  fault_data_t *fault_data,
			  reset_data_t *reset_data,
			  change_state_flags_t *change_state_flags) {

	initTrafficModeData(traffic_mode_data);
	initFaultData(fault_data);
	initResetData(reset_data);
	initInputQueue(input_queue);
	initInputData(input_data, input_queue);
	initTrafficStateData(traffic_state_data);
	initRailStateData(rail_state_data);
	initChangeStateFlags(change_state_flags);

	state_machine_thread_data->input_data = input_data;
	state_machine_thread_data->traffic_state_data = traffic_state_data;
	state_machine_thread_data->rail_state_data = rail_state_data;
	state_machine_thread_data->traffic_mode_data = traffic_mode_data;
	state_machine_thread_data->change_state_flags = change_state_flags;
	state_machine_thread_data->fault_data = fault_data;
	state_machine_thread_data->reset_data = reset_data;

	input_thread_data->input_data = input_data;
	input_thread_data->change_state_flags = change_state_flags;

	output_thread_data->traffic_state_data = traffic_state_data;
	output_thread_data->rail_state_data = rail_state_data;

	critical_server_data->traffic_state_data = traffic_state_data;
	critical_server_data->rail_state_data = rail_state_data;
	critical_server_data->traffic_mode_data = traffic_mode_data;
	critical_server_data->change_state_flags = change_state_flags;
	critical_server_data->fault_data = fault_data;
	critical_server_data->reset_data = reset_data;
	critical_server_data->attach = NULL;
}

int main(void) {

	FILE *fp;
	if ((fp = openTrafficLogFile(LOG_FILE_NAME)) == NULL) {
		printf("Failed to open log file: %s\n", LOG_FILE_NAME);
	}
	else {
		printf("Successful open log file: %s\n", LOG_FILE_NAME);
		fclose(fp);
	}

	while (1) {

		printf("At reset point\n");

		// Declare memory for all data
		traffic_mode_data_t traffic_mode_data;
		traffic_input_queue_t input_queue;
		traffic_input_data_t input_data;
		traffic_state_data_t traffic_state_data;
		rail_state_data_t rail_state_data; // TODO: to update this data type
		state_machine_thread_data_t state_machine_thread_data;
		input_thread_data_t input_thread_data;
		output_thread_data_t output_thread_data;
		critical_server_data_t critical_server_data;
		fault_data_t fault_data;
		reset_data_t reset_data;
		change_state_flags_t change_state_flags;

		// Initialise all data
		initData(&traffic_mode_data,
				 &input_queue,
				 &input_data,
				 &traffic_state_data,
				 &rail_state_data,
				 &state_machine_thread_data,
				 &input_thread_data,
				 &output_thread_data,
				 &critical_server_data,
				 &fault_data,
				 &reset_data,
				 &change_state_flags);

		// Thread definitions
		pthread_t th_critical_server_channel;
		pthread_t th_traffic_state_machine;
		pthread_t th_input;
		pthread_t th_output;

		// Thread attributes
		pthread_attr_t th_critical_server_channel_attr;
		pthread_attr_t th_traffic_state_machine_attr;
		pthread_attr_t th_input_attr;
		pthread_attr_t th_output_attr;

		// Thread schedule parameters
		struct sched_param th_critical_server_channel_param;
		struct sched_param th_traffic_state_machine_param;
		struct sched_param th_input_param;
		struct sched_param th_output_param;

		// Set thread attributes
		setThreadSettings(&th_critical_server_channel_attr, &th_critical_server_channel_param, I_CRIT_SEVER_CHANNEL_THREAD_PRIORITY, true);
		setThreadSettings(&th_traffic_state_machine_attr, &th_traffic_state_machine_param, I_TRAFFIF_STATE_MACHINE_THREAD_PRIORITY, false);
		setThreadSettings(&th_input_attr, &th_input_param, I_INPUT_THREAD_PRIORITY, true);
		setThreadSettings(&th_output_attr, &th_output_param, I_OUTPUT_THREAD_PRIORITY, true);

		// Spawn threads
		pthread_create(&th_critical_server_channel, &th_critical_server_channel_attr, &critical_server_channel_thread, &critical_server_data);
		//pthread_create(&th_input, &th_input_attr, &inputThread, &input_thread_data);
		pthread_create(&th_traffic_state_machine, &th_traffic_state_machine_attr, &trafficStateMachine, &state_machine_thread_data);
		//pthread_create(&th_output, &th_output_attr, LCD_Control, &output_thread_data);

		pthread_join(th_traffic_state_machine, NULL);		// Join state machine. This thread should exit on a reset signal

		// Reset: At this point the state machine has terminated. Kill all threads and start again

		printf("Reseting\nKilling threads\n");

		pthread_cancel(th_input);
		pthread_cancel(th_output);
		pthread_cancel(th_critical_server_channel);
		name_detach(critical_server_data.attach, NULL);

		printf("Threads destroyed\n");
	
	}

	printf("Main terminating - how did we end up here?");

	return EXIT_SUCCESS;
}
