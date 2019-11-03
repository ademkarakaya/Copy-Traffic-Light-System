#include "RCC_MessagePassing.h"

void *critical_client_channel_thread(void *data) {

		critical_client_thread_data_t * thread_data = (critical_client_thread_data_t*) data;
		rail_state_data_t * state_data = thread_data->rail_state_data;

	    msg_data_t msg;
	    msg_reply_t reply;

	    msg.client_id = RCC_CLIENT_ID; // unique number for this client (optional)

		int server_coid = -1;

		while (server_coid == -1) {
			printf("  ---> Trying to connect to server named: %s\n", TCC_CRITICAL_CHANNEL_ATTACH_POINT);
			if ((server_coid = name_open(TCC_CRITICAL_CHANNEL_ATTACH_POINT, 0)) == -1)
			{
				printf("\n    ERROR, could not connect to server!\n\n");
			}
			sleep(3);
		}

		printf("Connection established to: %s\n", TCC_CRITICAL_CHANNEL_ATTACH_POINT);

	    // We would have pre-defined data to stuff here
		msg.hdr.type = 0x00;
		msg.hdr.subtype = 0x00;

		while(1){

			pthread_mutex_lock(&state_data->mutex_state);

			while (!state_data->data_ready){
				pthread_cond_wait(&state_data->cond_state_changed, &state_data->mutex_state);
			}

				msg.data.rail_state = state_data->current_state;

				state_data->data_ready = false;
			pthread_mutex_unlock(&state_data->mutex_state);

			printf("data consumed\n");

			fflush(stdout);
			if (MsgSend(server_coid, &msg, sizeof(msg), &reply, sizeof(reply)) == -1) {
				printf(" Error data '%d' NOT sent to server\n", msg.data.rail_state);
					// maybe we did not get a reply from the server
				break;
			}
			else { // now process the reply
				printf("   -->Reply is: '%s'\n", reply.data.buf);
			}

			sleep(5);

		}

		ConnectDetach(server_coid);
}

void *critical_server_channel_thread(void *data) {

	critical_server_thread_data_t *thread_data = (critical_server_thread_data_t *)data;
	rail_state_data_t *state_data = thread_data->rail_state_data;

	printf("critical_server_channel()\n");
	name_attach_t *attach = createChannel(RCC_CRITICAL_CHANNEL);

	msg_data_t msg;
	int rcvid = 0;
	int Stay_alive = 1, living = 0; // server stays running (ignores _PULSE_CODE_DISCONNECT request)

	msg_reply_t replymsg; // replymsg structure for sending back to client
	replymsg.hdr.type = REPLY_MSG;
	replymsg.hdr.subtype = 0x00;

	living = 1;
	while (living) {
		// Do your MsgReceive's here now with the chid
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);

		if (rcvid == -1) { // Error condition, exit
			printf("\nFailed to MsgReceive\n");
			break;
		}

		// did we receive a Pulse or message?
		// for Pulses:
		if (rcvid == 0) { //  Pulse received, work out what type

			handlePulse(&msg);
			continue; // go back to top of while loop
		}

		// for messages:
		if (rcvid > 0) { // if true then A message was received

			if (msg.hdr.type >= _IO_BASE && msg.hdr.type <= _IO_MAX) {
				handleIOConnect(&msg, rcvid);
			}
			else { // A message (presumably ours) received

				// put your message handling code here and assemble a reply message
				if (LOG) {
					sprintf(log_message, "Server received data packet with value of '%d' from client (ID:%d), ", msg.data.rail_state, msg.client_id);
					writeTrafficLogFile(LOG_FILE_NAME, log_message);
				}

				if (msg.client_id == X1_CLIENT_ID) {

					fflush(stdout);
					//sleep(1); // Delay the reply by a second (just for demonstration purposes)

					pthread_mutex_lock(&state_data->mutex_state);
						state_data->current_state = msg.data.rail_state;

						state_data->data_ready = true;
						pthread_cond_signal(&state_data->cond_state_changed);
					pthread_mutex_unlock(&state_data->mutex_state);

					sprintf(replymsg.data.buf, "OK");
					if (LOG) {
						sprintf(log_message, "\n    -----> replying with: '%s'\n", replymsg.data.buf);
						writeTrafficLogFile(LOG_FILE_NAME, log_message);
					}
					MsgReply(rcvid, EOK, &replymsg, sizeof(replymsg));
				}
				else {
					printf("\nERROR: Server received something, but could not handle it correctly\n");
				}
			}
		}
	}

	// Remove the attach point name from the file system (i.e. /dev/name/local/<myname>)
	ChannelDestroy(attach->chid);
}
