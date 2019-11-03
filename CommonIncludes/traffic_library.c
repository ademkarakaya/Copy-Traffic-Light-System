#include "traffic_library.h"

void * sendMessage(void *data) {
	printf("sendmsg\n");
	send_message_thread_data_t *thread_data = (send_message_thread_data_t *)data;

	msg_data_t msg;
	pthread_rwlock_rdlock(&thread_data->rwlock);
		memcpy(&msg, thread_data->msg, sizeof(msg));
	pthread_rwlock_unlock(&thread_data->rwlock);

	printf("msg.type: %d\n", msg.hdr.type);

	msg_reply_t reply;

	int server_coid = -1;

	printf("  ---> Trying to connect to server named: %s\n", thread_data->attach_point);
	if ((server_coid = name_open(thread_data->attach_point, 0)) == -1) {
		printf("\n    ERROR, could not connect to server!\n\n");
	}
	else {
		printf("Connection established to: %s\n", thread_data->attach_point);
	}
	fflush(stdout);

	if (MsgSend(server_coid, &msg, sizeof(msg), &reply, sizeof(reply)) == -1) {
		printf(" Error data '%d' NOT sent to server\n", msg.data.traffic_state);
		// maybe we did not get a reply from the server
	} else { // now process the reply
		printf("   -->Reply is: '%d'\n", reply.data.traffic_state);
	}

	if(thread_data->reply != NULL){
		pthread_rwlock_wrlock(&thread_data->rwlock);
			memcpy(thread_data->reply, &reply, sizeof(reply));
		pthread_rwlock_unlock(&thread_data->rwlock);
	}
	ConnectDetach(server_coid);
}



void setThreadSettings(pthread_attr_t * th_attr, struct sched_param * th_sched_param, const int priority, const bool detached) {

	pthread_attr_init(th_attr);											// Init thread attrs to default vals
	pthread_attr_setschedpolicy(th_attr, SCHED_RR);						// Set scheduling to round robin

	if (detached) {
		pthread_attr_setdetachstate(th_attr, PTHREAD_CREATE_DETACHED );	// Set thread to deteached mode
	}

	th_sched_param->sched_priority = priority;							// Set priority of thread
	pthread_attr_setschedparam(th_attr, th_sched_param);
	pthread_attr_setinheritsched(th_attr, PTHREAD_EXPLICIT_SCHED);		// Set attr to use explicit scheduling settings
	// pthread_attr_setstacksize(th_attr, 8000);							// Increase thread stack size
}

void initTrafficStateData(traffic_state_data_t *traffic_state_data) {
	traffic_state_data->current_state = INITIAL;
	traffic_state_data->data_ready = false;
	pthread_mutex_init(&traffic_state_data->mutex_state, NULL);
	pthread_cond_init(&traffic_state_data->cond_state_changed, NULL);
}

void initRailStateData(rail_state_data_t *rail_state_data) {
	rail_state_data->current_state = NO_TRAIN;
	rail_state_data->data_ready = false;
	pthread_mutex_init(&rail_state_data->mutex_state, NULL);
	pthread_cond_init(&rail_state_data->cond_state_changed, NULL);
}

void initModeData(traffic_mode_data_t * traffic_mode_data){
	traffic_mode_data->mode = PEAK;
	pthread_rwlock_init(&traffic_mode_data->rwlock, NULL);
}

void initFaultData(fault_data_t *fault_data) {
	fault_data->fault = false;
	pthread_mutex_init(&fault_data->mutex, NULL);
}

void initResetData(reset_data_t *reset_data) {
	reset_data->reset = false;
	reset_data->data_ready = false;
	pthread_mutex_init(&reset_data->mutex, NULL);
	pthread_cond_init(&reset_data->cond, NULL);
}



name_attach_t *createChannel(const char *attach_point) {

	name_attach_t *attach = NULL;

	// Create a local name (/dev/name/...)
	if ((attach = name_attach(NULL, attach_point, 0)) == NULL) {
		printf("failed %s", attach_point);
		sprintf(log_message, "Failed to name_attach on ATTACH_POINT: %s \n", attach_point);
		writeTrafficLogFile(LOG_FILE_NAME, log_message);
	} else {
		sprintf(log_message, "Server Listening for Clients on ATTACH_POINT: %s \n", attach_point);
		writeTrafficLogFile(LOG_FILE_NAME, log_message);
	}
	return attach;
}

void handlePulse(msg_data_t *msg) {
	switch (msg->hdr.code) {

		case _PULSE_CODE_DISCONNECT:
			if (LOG) {
				sprintf(log_message, "\nServer received Detach pulse from ClientID:%d but rejected it ...\n", msg->client_id);
				writeTrafficLogFile(LOG_FILE_NAME, log_message);
			}
			break;

		case _PULSE_CODE_UNBLOCK:
			// REPLY blocked client wants to unblock (was hit by a signal
			// or timed out).  It's up to you if you reply now or later.
			if (LOG) {
				writeTrafficLogFile(LOG_FILE_NAME, "\nServer got _PULSE_CODE_UNBLOCK\n");
			}
			break;

		case _PULSE_CODE_COIDDEATH: // from the kernel
			if (LOG) {
				writeTrafficLogFile(LOG_FILE_NAME, "\nServer got _PULSE_CODE_COIDDEATH\n");
			}
			break;

		case _PULSE_CODE_THREADDEATH: // from the kernel
			if (LOG) {
				writeTrafficLogFile(LOG_FILE_NAME, "\nServer got _PULSE_CODE_THREADDEATH\n");
			}
			break;

		default:

			if (LOG) { // Some other pulse sent by one of your processes or the kernel
				sprintf(log_message, "\nServer got some other pulse: %d\n", msg->hdr.code);
				writeTrafficLogFile(LOG_FILE_NAME, log_message);
			}
			break;
	}
}

void handleIOConnect(msg_data_t *msg, int rcvid) {

	// If the Global Name Service (gns) is running, name_open() sends a connect message. The server must EOK it.
	if (msg->hdr.type == _IO_CONNECT) {
		MsgReply(rcvid, EOK, NULL, 0);
		if (LOG) {
			printf("\n gns service is running....");
		}
	}

	// Some other I/O message was received; reject it
	if (msg->hdr.type > _IO_BASE && msg->hdr.type <= _IO_MAX) {
		MsgError(rcvid, ENOSYS);

		if (LOG) {
			printf("\n Server received and IO message and rejected it....");
		}
	}
}
