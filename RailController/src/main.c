#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "RC_MessagePassing.h"
#include "RailController.h"
#include "input.h"
#include "output.h"
#include "stateMachine.h"
#include <traffic_library.h>

void initRailInputData(rail_input_data_t *input_data) {
	input_data->e_train = OFF;
	input_data->w_train = OFF;
	pthread_rwlock_init(&input_data->rwlock_input, NULL);
}

void initChangeStateFlags(change_state_flags_t *change_state_flags) {
	change_state_flags->input = false;
	change_state_flags->fault = false;
	change_state_flags->reset = false;
}

void initData(rail_state_data_t *rail_state_data,
			  rail_input_data_t *input_data,
			  state_machine_thread_data_t *state_machine_thread_data,
			  input_thread_data_t *input_thread_data,
			  ouput_thread_data_t *ouput_thread_data,
			  critial_client_thread_data_t *critial_client_thread_data,
			  fault_data_t *fault_data,
			  reset_data_t *reset_data,
			  change_state_flags_t *change_state_flags) {

	initRailStateData(rail_state_data);
	initRailInputData(input_data);
	initFaultData(fault_data);
	initResetData(reset_data);
	initChangeStateFlags(change_state_flags);

	state_machine_thread_data->input_data = input_data;
	state_machine_thread_data->rail_state_data = rail_state_data;
	state_machine_thread_data->change_state_flags = change_state_flags;

	input_thread_data->input_data = input_data;
	input_thread_data->change_state_flags = change_state_flags;

	ouput_thread_data->rail_state_data = rail_state_data;

	critial_client_thread_data->rail_state_data = rail_state_data;

}

int main(void) {

	while (1) {


		rail_state_data_t rail_state_data;
		rail_input_data_t input_data;
		state_machine_thread_data_t state_machine_thread_data;
		input_thread_data_t input_thread_data;
		ouput_thread_data_t ouput_thread_data;
		critial_client_thread_data_t critial_client_thread_data;
		fault_data_t fault_data;
		reset_data_t reset_data;
		change_state_flags_t change_state_flags;

		initData(
			&rail_state_data,
			&input_data,
			&state_machine_thread_data,
			&input_thread_data,
			&ouput_thread_data,
			&critial_client_thread_data,
			&fault_data,
			&reset_data,
			&change_state_flags);

		// Thread definitions
		pthread_t th_input;
		pthread_t th_rail_state_machine;
		pthread_t th_output;
		pthread_t th_critical_client_channel;

		// Thread attributes
		pthread_attr_t th_input_attr;
		pthread_attr_t th_rail_state_machine_attr;
		pthread_attr_t th_output_attr;
		pthread_attr_t th_critical_client_channel_attr;

		// Thread schedule parameters
		struct sched_param th_input_param;
		struct sched_param th_rail_state_machine_param;
		struct sched_param th_output_param;
		struct sched_param th_critical_client_channel_param;

		// Set thread attributes
		setThreadSettings(&th_input_attr, &th_input_param, RC_INPUT_THREAD_PRIORITY, true);
		setThreadSettings(&th_rail_state_machine_attr, &th_rail_state_machine_param, RC_RAIL_STATE_MACHINE_THREAD_PRIORITY, false);
		setThreadSettings(&th_output_attr, &th_output_param, RC_OUTPUT_THREAD_PRIORITY, true);
		setThreadSettings(&th_critical_client_channel_attr, &th_critical_client_channel_param, RC_CRIT_CLIENT_CHANNEL_THREAD_PRIORITY, true);

		// Spawn threads
		pthread_create(&th_input, &th_input_attr, &inputThread, &input_thread_data);
		pthread_create(&th_rail_state_machine, &th_rail_state_machine_attr, &railStateMachine, &state_machine_thread_data);
		pthread_create(&th_output, &th_output_attr, LCD_Control, &ouput_thread_data);
		pthread_create(&th_critical_client_channel, &th_critical_client_channel_attr, &critical_client_channel_thread, &critial_client_thread_data);

		pthread_join(th_rail_state_machine, NULL);

		printf("Reseting\nKilling threads\n");

		pthread_cancel(th_input);
		pthread_cancel(th_output);
		pthread_cancel(th_critical_client_channel);

		printf("Threads destroyed\n");

	}

	printf("Main terminating - how did we end up here?");

	return EXIT_SUCCESS;
}
