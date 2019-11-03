/*
 * stateMachine.c
 *
 *  Created on: 24Sep.,2019
 *      Author: aprab
 */

#include "stateMachine.h"

void * timerThread (void * data){

	struct _pulse change_state_msg;
	int rcvid;

	timer_thread_data_t * thread_data = (timer_thread_data_t*)data;
	int chid = *thread_data->chid;
	change_state_flags_t * change_state_flags = thread_data->change_state_flags;

	while(1){
		rcvid = MsgReceivePulse(chid, &change_state_msg, sizeof(change_state_msg), NULL);

		if (change_state_msg.code == STATE_TIMER_EXPIRE_PULSE) {
			writeTrafficLogFile(LOG_FILE_NAME, "(I1) - STATE_TIMER_EXPIRE_PULSE\n");


			pthread_sleepon_lock();
				change_state_flags->state_timer_timeout = true;
				pthread_sleepon_signal(change_state_flags);
			pthread_sleepon_unlock();
		}
		else if (change_state_msg.code == PED_TIMER_EXPIRE_PULSE) {
			writeTrafficLogFile(LOG_FILE_NAME, "(I1) - PED_TIMER_EXPIRE_PULSE\n");
			pthread_sleepon_lock();
				change_state_flags->ped_timer_timeout = true;
				pthread_sleepon_signal(change_state_flags);
			pthread_sleepon_unlock();
		}
		else if (change_state_msg.code == PED_SENSE_TIMEOUT_EXPIRE_PULSE) {
			writeTrafficLogFile(LOG_FILE_NAME, "(I1) - PED_SENSE_TIMEOUT_EXPIRE_PULSE\n");
			pthread_sleepon_lock();
				change_state_flags->ped_sense_timer_timeout = true;
				pthread_sleepon_signal(change_state_flags);
			pthread_sleepon_unlock();
		}
	}

}

void * trafficStateMachine(void *data) {



	traffic_state_t next_state = INITIAL;
	traffic_state_t current_state = INITIAL;
	traffic_state_t previous_state = INITIAL;
	traffic_input_queue_t input_queue;
	traffic_mode_t mode;
	
	bool fault_flag = false;

	state_machine_thread_data_t *thread_data = (state_machine_thread_data_t *)data;

	traffic_input_data_t *input_data = thread_data->input_data;
	traffic_state_data_t *traffic_state_data = thread_data->traffic_state_data;
	rail_state_data_t *rail_state_data = thread_data->rail_state_data;
	traffic_mode_data_t * mode_data = thread_data->traffic_mode_data;
	fault_data_t * fault_data = thread_data->fault_data;
	change_state_flags_t * change_state_flags = thread_data->change_state_flags;

	change_state_flags_t state_flags;
	
	int change_state_channel_id = ChannelCreate(0);
	initiliseTimer(change_state_channel_id);

	timer_thread_data_t timer_thread_data = {
			&change_state_channel_id,
			change_state_flags
	};

	pthread_create(NULL, NULL, &timerThread, &timer_thread_data);

	bool state_machine_exit = false;
	while (!state_machine_exit) {

		//DELETE
		rail_state_t temp_rail;
		pthread_mutex_lock(&rail_state_data->mutex_state);
			temp_rail = rail_state_data->current_state;
	   pthread_mutex_unlock(&rail_state_data->mutex_state);
	   printf("rail: %d\n", temp_rail);

		pthread_rwlock_rdlock( &mode_data->rwlock );
			mode = mode_data->mode;
		pthread_rwlock_unlock( &mode_data->rwlock );


		pthread_rwlock_rdlock(&input_data->rwlock_input);	// get input flags
			memcpy(&input_queue, input_data->input_queue, sizeof(input_queue));
		pthread_rwlock_unlock(&input_data->rwlock_input);

		//printInputBuf(input_queue.input_buffer, input_queue.end_index);
		if (state_flags.fault) {
			next_state = handleFaultSig(current_state);
		}
		else if (mode == PEAK) {
			next_state = changeTrafficState_peak(current_state, previous_state, &input_queue);
		}
		else if (mode == OFF_PEAK) {
			next_state = changeTrafficState_offPeak(current_state, previous_state, &input_queue);
		}
		else {
			if (LOG) {
				writeTrafficLogFile("(I1) - Incorrect Mode, mode: %d\n", mode);
			}
		}

		// update previous state
		previous_state = current_state;
		current_state = next_state;

		// update current state
		pthread_mutex_lock(&traffic_state_data->mutex_state);
			traffic_state_data->current_state = current_state;
			traffic_state_data->data_ready = true;
			pthread_cond_broadcast(&traffic_state_data->cond_state_changed);
		pthread_mutex_unlock(&traffic_state_data->mutex_state);

		if (mode == PEAK) {
			updateTimersPeak(current_state);
		}
		else if (mode == OFF_PEAK) {
			updateTimersOffPeak(current_state);
		}
		else {
			if (LOG) {
				writeTrafficLogFile(LOG_FILE_NAME, "(I1) - Mode Error\n");
			}
		}

		stateToString(current_state, log_message);
		printf(log_message);
		printf("\n");
		printf("mode: %d\n", mode);
		if (LOG) {	// Print state info

			strcat(log_message, "\n");
			writeTrafficLogFile(LOG_FILE_NAME, log_message);
			sprintf(log_message, "(I1) - Timer:  %ld\n", itime_state_timer.it_value.tv_sec);
			writeTrafficLogFile(LOG_FILE_NAME, log_message);
		}

		// wait for timer
		bool change_state = false;
		while (!change_state) {
			pthread_sleepon_lock();
				pthread_sleepon_wait(change_state_flags);
				memcpy(&state_flags, change_state_flags, sizeof(state_flags));
			pthread_sleepon_unlock();

			pthread_rwlock_rdlock(&input_data->rwlock_input);
				memcpy(&input_queue, input_data->input_queue, sizeof(input_queue));
			pthread_rwlock_unlock(&input_data->rwlock_input);

			pthread_rwlock_rdlock( &mode_data->rwlock );
				mode = mode_data->mode;
			pthread_rwlock_unlock( &mode_data->rwlock );

			if (mode == PEAK) {
				printf("decide change peak\n");
				change_state = decideChangeStatePeak(&state_flags, current_state, &input_queue);
			}
			else if (mode == OFF_PEAK) {
				printf("decide change offpeak\n");
				change_state = decideChangeStateOffPeak(&state_flags, current_state, &input_queue);
			}
		}

		if (mode == PEAK) {
			clearInputPeak(input_data, current_state);
		}
		else if (mode == OFF_PEAK) {
			clearInputOffPeak(input_data, current_state);
		}

		clearFlags(change_state_flags, input_data);

		pthread_sleepon_lock();
			memcpy(&state_flags, change_state_flags, sizeof(state_flags));
		pthread_sleepon_unlock();




		if(state_flags.reset && (current_state == ALL_RED || current_state == FAULT || current_state == INITIAL)){
			state_machine_exit = true;
			printf("Exit\n");
		}
	};

	return EXIT_SUCCESS;
}

void initiliseTimer(int chid) {

	state_timer_event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0, chid, _NTO_SIDE_CHANNEL, 0);

	if (state_timer_event.sigev_coid != -1) {

		state_timer_event.sigev_notify = SIGEV_PULSE;

		struct sched_param th_param;
		pthread_getschedparam(pthread_self(), NULL, &th_param);		   // Get my own thread parameters
		state_timer_event.sigev_priority = th_param.sched_curpriority; // Set sig with same priority as myself

		state_timer_event.sigev_code = STATE_TIMER_EXPIRE_PULSE;

		// Create State Timer
		if (timer_create(CLOCK_REALTIME, &state_timer_event, &state_timer_id) == -1) { // create the timer, binding it to the event
			if (LOG) {
				sprintf(log_message, "(I1) - Couldn't create State Timer, errno: %d\n", errno);
				writeTrafficLogFile(LOG_FILE_NAME, log_message);
			}
			exit(EXIT_FAILURE);
		}

		// Timer for the state transition of the traffic intersection
		// one-shot timer (default = 0s)
		itime_state_timer.it_value.tv_sec = 0;		// 0 second
		itime_state_timer.it_value.tv_nsec = 0;		// 0 nano seconds
		itime_state_timer.it_interval.tv_sec = 0;	// 0 second 		(reload)
		itime_state_timer.it_interval.tv_nsec = 0;	// 0 nano seconds 	(reload)

	}
	else {
		if (LOG) {
			sprintf(log_message, "(I1) - Couldn't ConnectAttach to, chid: %d\n", chid);
			writeTrafficLogFile(LOG_FILE_NAME, log_message);
		}

		exit(EXIT_FAILURE);
	}

	ped_timer_event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0, chid, _NTO_SIDE_CHANNEL, 0);

	if (ped_timer_event.sigev_coid != -1) {

		ped_timer_event.sigev_notify = SIGEV_PULSE;

		struct sched_param th_param;
		pthread_getschedparam(pthread_self(), NULL, &th_param);			// Get my own thread parameters
		ped_timer_event.sigev_priority = th_param.sched_curpriority;	// Set sig with same priority as myself

		ped_timer_event.sigev_code = PED_TIMER_EXPIRE_PULSE;

		// Create Pedestrian Timer
		if (timer_create(CLOCK_REALTIME, &ped_timer_event, &ped_timer_id) == -1) { // create the timer, binding it to the event
			if (LOG) {
				sprintf(log_message, "(I1) - Couldn't create Pedestrian Timer, errno: %d\n", errno);
				writeTrafficLogFile(LOG_FILE_NAME, log_message);
			}
			exit(EXIT_FAILURE);
		}

		// Timer for the pedestrian crossing at the traffic intersection
		// one-shot timer (default = 0s)
		itime_ped_timer.it_value.tv_sec = 0;		// 0 second
		itime_ped_timer.it_value.tv_nsec = 0;		// 0 nano seconds
		itime_ped_timer.it_interval.tv_sec = 0;		// 0 second 		(reload)
		itime_ped_timer.it_interval.tv_nsec = 0;	// 0 nano seconds 	(reload)
	}
	else {
		if (LOG) {
			sprintf(log_message, "(I1) - Couldn't ConnectAttach to, chid: %d\n", chid);
			writeTrafficLogFile(LOG_FILE_NAME, log_message);
		}
		exit(EXIT_FAILURE);
	}

	ped_sense_timeout_timer_event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0, chid, _NTO_SIDE_CHANNEL, 0);

	if (ped_sense_timeout_timer_event.sigev_coid != -1) {

		printf("(I1) - set ped timeout timer\n");
		ped_sense_timeout_timer_event.sigev_notify = SIGEV_PULSE;

		struct sched_param th_param;
		pthread_getschedparam(pthread_self(), NULL, &th_param);			// Get my own thread parameters
		ped_sense_timeout_timer_event.sigev_priority = th_param.sched_curpriority;	// Set sig with same priority as myself

		ped_sense_timeout_timer_event.sigev_code = PED_SENSE_TIMEOUT_EXPIRE_PULSE;

		// Create Pedestrian Timer
		if (timer_create(CLOCK_REALTIME, &ped_sense_timeout_timer_event, &ped_sense_timeout_timer_id) == -1) { // create the timer, binding it to the event
			if (LOG) {
				sprintf(log_message, "(I1) - Couldn't create Pedestrian Timer, errno: %d\n", errno);
				writeTrafficLogFile(LOG_FILE_NAME, log_message);
			}
			exit(EXIT_FAILURE);
		}

		// Timer for the pedestrian crossing at the traffic intersection
		// one-shot timer (default = 0s)
		itime_ped_sense_timeout_timer.it_value.tv_sec = 0;		// 0 second
		itime_ped_sense_timeout_timer.it_value.tv_nsec = 0;		// 0 nano seconds
		itime_ped_sense_timeout_timer.it_interval.tv_sec = 0;		// 0 second 		(reload)
		itime_ped_sense_timeout_timer.it_interval.tv_nsec = 0;	// 0 nano seconds 	(reload)
	}
	else {
		if (LOG) {
			sprintf(log_message, "(I1) - Couldn't ConnectAttach to, chid: %d\n", chid);
			writeTrafficLogFile(LOG_FILE_NAME, log_message);
		}
		exit(EXIT_FAILURE);
	}

}

traffic_state_t handleFaultSig(traffic_state_t current_state) {

	printf("handleFaultSig()\n");
	traffic_state_t next_state = FAULT;

	switch (current_state) {
		case INITIAL:
			next_state = FAULT;
			break;

		case ALL_RED:
			next_state = FAULT;
			break;

		case NS_S_G_SEN:
			next_state = NS_S_Y;
			break;

		case NS_S_G_P_X:
			next_state = NS_S_Y;
			break;

		case NS_S_G_P_F:
			next_state = NS_S_Y;
			break;

		case NS_S_G:
			next_state = NS_S_Y;
			break;

		case NS_S_Y:
			next_state = FAULT;
			break;

		case EW_S_G_SEN:
			next_state = EW_S_Y;
			break;

		case EW_S_G_P_X:
			next_state = EW_S_Y;
			break;

		case EW_S_G_P_F:
			next_state = EW_S_Y;
			break;

		case EW_S_G:
			next_state = EW_S_Y;
			break;

		case EW_S_Y:
			next_state = FAULT;
			break;

		case NS_R_G:
			next_state = NS_R_Y;
			break;

		case NS_R_Y:
			next_state = FAULT;
			break;

		case EW_R_G:
			next_state = EW_R_Y;
			break;

		case EW_R_Y:
			next_state = FAULT;
			break;
		case FAULT:
			next_state = FAULT;
			break;

		default:
			next_state = FAULT;
			break;
	}

	return next_state;
}

traffic_state_t changeTrafficState_peak(traffic_state_t current_state, traffic_state_t previous_state, traffic_input_queue_t *input_queue) {

	traffic_state_t next_state = INITIAL;

	switch (current_state) {

		case INITIAL:
			next_state = ALL_RED;
			break;

		case ALL_RED:
			if ((previous_state == INITIAL) || (previous_state == NS_R_Y)) {
				next_state = NS_S_G_SEN;
			}
			else if (previous_state == NS_S_Y) {
				next_state = EW_R_G;
			}
			else if (previous_state == EW_R_Y) {
				next_state = EW_S_G_SEN;
			}
			else if (previous_state == EW_S_Y) {
				next_state = NS_R_G;
			}
			break;

		// North south straight transitions
		case NS_S_G_SEN:
			if (containsTrafficInputBuf(input_queue, NS_P)) {
				next_state = NS_S_G_P_X;
			}
			else {
				next_state = NS_S_G;
			}
			break;

		case NS_S_G_P_X:
			next_state = NS_S_G_P_F;
			break;

		case NS_S_G_P_F:
			next_state = NS_S_G;
			break;

		case NS_S_G:
			next_state = NS_S_Y;
			break;

		case NS_S_Y:
			next_state = ALL_RED;
			break;

		// East west straight transitions
		case EW_S_G_SEN:
			if (containsTrafficInputBuf(input_queue, EW_P)) {
				next_state = EW_S_G_P_X;
			}
			else {
				next_state = EW_S_G;
			}
			break;

		case EW_S_G_P_X:
			next_state = EW_S_G_P_F;
			break;

		case EW_S_G_P_F:
			next_state = EW_S_G;
			break;

		case EW_S_G:
			next_state = EW_S_Y;
			break;

		case EW_S_Y:
			next_state = ALL_RED;
			break;

		case NS_R_G:
			next_state = NS_R_Y;
			break;

		case NS_R_Y:
			next_state = ALL_RED;
			break;

		// East west right transitions
		case EW_R_G:
			next_state = EW_R_Y;
			break;

		case EW_R_Y:
			next_state = ALL_RED;
			break;
		case FAULT:

			break;
		default:
			next_state = FAULT;
			break;
	}

	return next_state;
}

traffic_state_t changeTrafficState_offPeak(traffic_state_t current_state, traffic_state_t previous_state, traffic_input_queue_t *input_queue) {

	traffic_state_t next_state = INITIAL;
	int input = readTrafficInputBuf(input_queue); // 0 == next input

	switch (current_state) {

		case INITIAL:
			next_state = ALL_RED;
			break;

		case ALL_RED:

			if (input == NS_STR || input == NS_P) {
				next_state = NS_S_G_P_X;
			}
			else if (input == NS_R) {
				next_state = NS_R_G;
			}
			else if (input == EW_R) {
				next_state = EW_R_G;
			}
			else if (input == EW_STR || input == EW_P) {
				next_state = EW_S_G_SEN;
			}
			else {
				next_state = EW_S_G_SEN;
			}
			break;

		// NS Straight state transitions
		case NS_S_G_P_X:
			next_state = NS_S_G_P_F;
			break;

		case NS_S_G_P_F:
			next_state = NS_S_Y;
			break;

		case NS_S_Y:
			next_state = ALL_RED;
			break;

		// EW Right turn state transitions
		case EW_R_G:
			next_state = EW_R_Y;
			break;

		case EW_R_Y:
			next_state = ALL_RED;
			break;

		// NS Right turn transitions
		case NS_R_G:
			next_state = NS_R_Y;
			break;

		case NS_R_Y:
			next_state = ALL_RED;
			break;

		// EW Straight state transitions
		case EW_S_G_SEN:

			if (input == EW_P) {
				next_state = EW_S_G_P_X;
			}
			else if (input == NS_STR || input == NS_R || input == NS_P || input == EW_R) {
				next_state = EW_S_Y;
			}
			else {
				next_state = EW_S_G_SEN; // Go back when no sensors and timer done
			}

			break;

		case EW_S_G_P_X:
			next_state = EW_S_G_P_F;
			break;

		case EW_S_G_P_F:
			next_state = EW_S_G_SEN;
			break;

		case EW_S_Y:
			next_state = ALL_RED;
			break;

		case FAULT:
			next_state = FAULT;
			break;

		// Get away from unused states in off-peak
		case NS_S_G_SEN:
			next_state = NS_S_Y;
			break;
		case NS_S_G:
			next_state = NS_S_Y;
			break;
		case EW_S_G:
			next_state = EW_S_Y;
			break;

		// Default state
		default:
			next_state = FAULT;
			break;
	}

	return next_state;
}

void updateTimersPeak(traffic_state_t current_state) {

	switch (current_state) {

		case INITIAL:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_INITIAL;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case ALL_RED:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_ALL_RED;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case NS_S_G_SEN:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_STRAIGHT;
			itime_ped_sense_timeout_timer.it_value.tv_sec = TIME_PED_TIME_OUT;

			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			timer_settime(ped_sense_timeout_timer_id, 0, &itime_ped_sense_timeout_timer, NULL);		// and start the timer!
			break;

		case NS_S_G_P_X:
			itime_ped_timer.it_value.tv_sec = TIME_PED_CROSS;
			timer_settime(ped_timer_id, 0, &itime_ped_timer, NULL); // and start the timer!
			break;

		case NS_S_G_P_F:
			itime_ped_timer.it_value.tv_sec = TIME_PED_FLASHING;
			timer_settime(ped_timer_id, 0, &itime_ped_timer, NULL); // and start the timer!
			break;

		case NS_S_G:
			// DO NOT SET TIMER
			break;

		case NS_S_Y:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_YELLOW;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case EW_S_G_SEN:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_STRAIGHT;
			itime_ped_sense_timeout_timer.it_value.tv_sec = TIME_PED_TIME_OUT;

			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			timer_settime(ped_sense_timeout_timer_id, 0, &itime_ped_sense_timeout_timer, NULL);		// and start the timer!
			break;

		case EW_S_G_P_X:
			itime_ped_timer.it_value.tv_sec = TIME_PED_CROSS;
			timer_settime(ped_timer_id, 0, &itime_ped_timer, NULL); // and start the timer!
			break;

		case EW_S_G_P_F:
			itime_ped_timer.it_value.tv_sec = TIME_PED_FLASHING;
			timer_settime(ped_timer_id, 0, &itime_ped_timer, NULL); // and start the timer!
			break;

		case EW_S_G:
			// DO NOT SET TIMER
			break;

		case EW_S_Y:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_YELLOW;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case NS_R_G:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_TURN;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case NS_R_Y:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_YELLOW;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case EW_R_G:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_TURN;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case EW_R_Y:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_YELLOW;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;
		case FAULT:
			// DO NOT SET TIMER
			break;
		default:
			// DO NOT SET TIMER
			break;
	}
}

void updateTimersOffPeak(traffic_state_t current_state) {

	switch (current_state) {

		case INITIAL:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_INITIAL;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case ALL_RED:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_ALL_RED;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case NS_S_G_SEN:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_STRAIGHT;

			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case NS_S_G_P_X:
			itime_ped_timer.it_value.tv_sec = TIME_PED_CROSS;
			timer_settime(ped_timer_id, 0, &itime_ped_timer, NULL); // and start the timer!
			break;

		case NS_S_G_P_F:
			itime_ped_timer.it_value.tv_sec = TIME_PED_FLASHING;
			timer_settime(ped_timer_id, 0, &itime_ped_timer, NULL); // and start the timer!
			break;

		case NS_S_G:
			// DO NOT SET TIMER
			break;

		case NS_S_Y:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_YELLOW;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case EW_S_G_SEN:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_STRAIGHT;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case EW_S_G_P_X:
			itime_ped_timer.it_value.tv_sec = TIME_PED_CROSS;
			timer_settime(ped_timer_id, 0, &itime_ped_timer, NULL); // and start the timer!
			break;

		case EW_S_G_P_F:
			itime_ped_timer.it_value.tv_sec = TIME_PED_FLASHING;
			timer_settime(ped_timer_id, 0, &itime_ped_timer, NULL); // and start the timer!
			break;

		case EW_S_G:
			// DO NOT SET TIMER
			break;

		case EW_S_Y:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_YELLOW;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case NS_R_G:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_TURN;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case NS_R_Y:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_YELLOW;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case EW_R_G:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_TURN;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;

		case EW_R_Y:
			itime_state_timer.it_value.tv_sec = TIME_TRAFFIC_YELLOW;
			timer_settime(state_timer_id, 0, &itime_state_timer, NULL); // and start the timer!
			break;
		case FAULT:
			// DO NOT SET TIMER
			break;
		default:
			// DO NOT SET TIMER
			break;
	}
}

bool decideChangeStatePeak(change_state_flags_t * change_state_flags, traffic_state_t current_state, traffic_input_queue_t * input_queue) {

	bool change_state = false;

	if(change_state_flags->fault){
		writeTrafficLogFile(LOG_FILE_NAME, "(I1) - FAULT_PULSE\n");
		change_state = true;
	}

	if(change_state_flags->reset){
		writeTrafficLogFile(LOG_FILE_NAME, "(I1) - RESET_PULSE\n");
		change_state = true;
	}

	if (change_state_flags->state_timer_timeout) {
		writeTrafficLogFile(LOG_FILE_NAME, "(I1) - STATE_TIMER_EXPIRE_PULSE\n");

		change_state = true;
	}

	if (change_state_flags->ped_timer_timeout) {
		writeTrafficLogFile(LOG_FILE_NAME, "(I1) - PED_TIMER_EXPIRE_PULSE\n");

		if(current_state == NS_S_G_P_X || current_state == NS_S_G_P_F ||
			current_state == EW_S_G_P_X || current_state == EW_S_G_P_F) {

			change_state = true;
		}
	}
	if (change_state_flags->ped_sense_timer_timeout) {
		writeTrafficLogFile(LOG_FILE_NAME, "(I1) - PED_TIMER_EXPIRE_PULSE\n");

		if(current_state == NS_S_G_SEN || current_state == EW_S_G_SEN ) {
			change_state = true;
		}
	}

	if(change_state_flags->input){

		if (containsTrafficInputBuf(input_queue, NS_P)) {
			writeTrafficLogFile(LOG_FILE_NAME, "(I1) - INPUT_PULSE\n");


			if (current_state == NS_S_G_SEN) {
				change_state = true;
			}
		}

		if (containsTrafficInputBuf(input_queue, EW_P)) {
			writeTrafficLogFile(LOG_FILE_NAME, "(I1) - INPUT_PULSE\n");

			if (current_state == EW_S_G_SEN) {
				change_state = true;
			}
		}
	}

	return change_state;
}

bool decideChangeStateOffPeak(change_state_flags_t * change_state_flags, traffic_state_t current_state, traffic_input_queue_t *input_queue) {
	bool change_state = false;

	if(change_state_flags->fault){
		writeTrafficLogFile(LOG_FILE_NAME, "(I1) - FAULT_PULSE\n");
		change_state = true;
	}

	if(change_state_flags->reset){
		writeTrafficLogFile(LOG_FILE_NAME, "(I1) - RESET_PULSE\n");
		change_state = true;
	}

	if (change_state_flags->state_timer_timeout) {
		writeTrafficLogFile(LOG_FILE_NAME, "(I1) - STATE_TIMER_EXPIRE_PULSE\n");

		if (!(current_state == EW_S_G_SEN && input_queue->length == 0)) {
			change_state = true;
		}
	}
	if (change_state_flags->ped_timer_timeout) {
		writeTrafficLogFile(LOG_FILE_NAME, "(I1) - PED_TIMER_EXPIRE_PULSE\n");

		change_state = true;
	}
	if ( change_state_flags->input ) {

		writeTrafficLogFile(LOG_FILE_NAME, "(I1) - INPUT_PULSE\n");

		if (current_state == EW_S_G_SEN && change_state_flags->state_timer_timeout) {
			change_state = true;
		}
	}
	return change_state;
}

void clearInputPeak(traffic_input_data_t *input_data, traffic_state_t state) {
	
	traffic_input_queue_t input_queue;		// clear input signal

	pthread_rwlock_rdlock(&input_data->rwlock_input);
		memcpy(&input_queue, input_data->input_queue, sizeof(input_queue));
	pthread_rwlock_unlock(&input_data->rwlock_input);

	if (state == NS_S_G_P_X) {
		deleteTrafficInputBufCell(&input_queue, NS_P);
	}
	else if (state == EW_S_G_P_X) {
		deleteTrafficInputBufCell(&input_queue, EW_P);
	}

	pthread_rwlock_wrlock(&input_data->rwlock_input);
		memcpy(input_data->input_queue, &input_queue, sizeof(input_queue));
	pthread_rwlock_unlock(&input_data->rwlock_input);
}

void clearInputOffPeak(traffic_input_data_t * input_data, traffic_state_t state) {

	traffic_input_queue_t input_queue;

	pthread_rwlock_rdlock(&input_data->rwlock_input);
		memcpy(&input_queue, input_data->input_queue, sizeof(input_queue));
	pthread_rwlock_unlock(&input_data->rwlock_input);

	if (state == NS_S_G_P_X) {
		int cell_count = 2;
		int deletCells[] = {NS_STR, NS_P};
		deleteTrafficInputBufCells(&input_queue, deletCells, cell_count);
	}
	else if (state == EW_S_G_P_X) {
		int cell_count = 2;
		int deletCells[] = {EW_STR, EW_P};
		deleteTrafficInputBufCells(&input_queue, deletCells, cell_count);
	}
	
	if (state == NS_S_G_SEN || state == NS_S_G) {
		deleteTrafficInputBufCell(&input_queue, NS_STR);
	}
	else if (state == EW_S_G_SEN || state == EW_S_G) {
		deleteTrafficInputBufCell(&input_queue, EW_STR);
	}
	else if (state == NS_R_G) {
		deleteTrafficInputBufCell(&input_queue, NS_R);
	}
	else if (state == EW_R_G) {
		deleteTrafficInputBufCell(&input_queue, EW_R);
	}

	pthread_rwlock_wrlock(&input_data->rwlock_input);
		memcpy(input_data->input_queue, &input_queue, sizeof(input_queue));
	pthread_rwlock_unlock(&input_data->rwlock_input);
}

void clearFlags(change_state_flags_t * change_state_flags, traffic_input_data_t * input_data) {

	pthread_rwlock_rdlock(&input_data->rwlock_input);
		int input_length = input_data->input_queue->length;
	pthread_rwlock_unlock(&input_data->rwlock_input);

	pthread_sleepon_lock();
		change_state_flags->state_timer_timeout = false;
		change_state_flags->ped_timer_timeout = false;
		change_state_flags->ped_sense_timer_timeout = false;

		if(input_length == 0){
			change_state_flags->input = false;
		}
	pthread_sleepon_unlock();
}



void stateToString(traffic_state_t state, char *state_str) {

	switch (state) {
		case INITIAL:
			sprintf(state_str, "INITIAL");
			break;

		case ALL_RED:
			sprintf(state_str, "ALL_RED");
			break;

		case FAULT:
			sprintf(state_str, "FAULT");
			break;

		case NS_S_G_SEN:
			sprintf(state_str, "NS_S_G_SEN");
			break;

		case NS_S_G_P_X:
			sprintf(state_str, "NS_S_G_P_X");
			break;

		case NS_S_G_P_F:
			sprintf(state_str, "NS_S_G_P_F");
			break;

		case NS_S_G:
			sprintf(state_str, "NS_S_G");
			break;

		case NS_S_Y:
			sprintf(state_str, "NS_S_Y");
			break;

		case EW_R_G:
			sprintf(state_str, "EW_R_G");
			break;

		case EW_R_Y:
			sprintf(state_str, "EW_R_Y");
			break;

		case EW_S_G_SEN:
			sprintf(state_str, "EW_S_G_SEN");
			break;

		case EW_S_G_P_X:
			sprintf(state_str, "EW_S_G_P_X");
			break;

		case EW_S_G_P_F:
			sprintf(state_str, "EW_S_G_P_F");
			break;

		case EW_S_G:
			sprintf(state_str, "EW_S_G");
			break;

		case EW_S_Y:
			sprintf(state_str, "EW_S_Y");
			break;

		case NS_R_G:
			sprintf(state_str, "NS_R_G");
			break;

		case NS_R_Y:
			sprintf(state_str, "NS_R_Y");
			break;

		default:
			break;
	}
}

