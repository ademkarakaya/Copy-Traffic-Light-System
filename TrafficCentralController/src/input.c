/*
 * input.c
 *
 *  Created on: 16Oct.,2019
 *      Author: NickM_e75acww
 */
#include <traffic_library.h>
#include <traffic_types.h>
#include "TrafficCentralController.h"

void * inputThread(void * data) {

	name_attach_t * attach = createChannel(TCC_INPUT_CHANNEL);

	input_thread_data_t * thread_data = (input_thread_data_t*)data;
	traffic_state_data_t *i1_state_data = thread_data->tcc_state_data->i1_state_data;
	traffic_state_data_t *i2_state_data = thread_data->tcc_state_data->i2_state_data;
	rail_state_data_t *x1_state_data = thread_data->tcc_state_data->x1_state_data;
	traffic_mode_data_t * traffic_mode_data = thread_data->tcc_state_data->traffic_mode_data;

	tool_msg_t msg;
	int rcvid = 0;
	int Stay_alive = 1, living = 0; // server stays running (ignores _PULSE_CODE_DISCONNECT request)

	tool_reply_t replymsg; // replymsg structure for sending back to client
	replymsg.hdr.type = REPLY_MSG;
	replymsg.hdr.subtype = 0x00;

	msg_data_t msg_send;
	msg_send.client_id = TCC_CLIENT_ID;

	living = 1;
	while (living) {
		printf("Waiting Input\n");
		// Do your MsgReceive's here now with the chid
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);
		printf("\nMsgReceive\n");
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
					//sprintf(log_message, "Server received data packet with value of '%d' from client (ID:%d), ", msg.data.rail_state, msg.client_id);
					//writeTrafficLogFile(LOG_FILE_NAME, log_message);
				}

				fflush(stdout);
				//sleep(1); // Delay the reply by a second (just for demonstration purposes)

				if(msg.func == GET_FUNC){
					printf("GET_FUNC\n");
					get_state_thread_data_t get_state_thread_data = {
							i1_state_data,
							i2_state_data
					};
					pthread_t th_get_state;
					printf("spawn thread\n");
					pthread_create(&th_get_state, NULL, &getStateData, &get_state_thread_data);
					pthread_join(th_get_state, NULL);
					printf("joined\n");

					pthread_mutex_lock(&i1_state_data->mutex_state);
						replymsg.data.state.i1 = i1_state_data->current_state;
					pthread_mutex_unlock(&i1_state_data->mutex_state);

					pthread_mutex_lock(&i2_state_data->mutex_state);
						replymsg.data.state.i2 = i2_state_data->current_state;
					pthread_mutex_unlock(&i2_state_data->mutex_state);

					pthread_mutex_lock(&i2_state_data->mutex_state);
						replymsg.data.state.x1 = x1_state_data->current_state;
					pthread_mutex_unlock(&i2_state_data->mutex_state);

					pthread_rwlock_rdlock(&traffic_mode_data->rwlock);
						replymsg.data.state.mode = traffic_mode_data->mode;
						printf("mode:  %d\n", traffic_mode_data->mode);
					pthread_rwlock_unlock(&traffic_mode_data->rwlock);
					printf("reply mode:  %d\n", replymsg.data.state.mode);
				}
				else if(msg.func == SET_FUNC){
					printf("SET_FUNC\n");
					if(msg.option == MODE_OPTION){
						printf("MODE_OPTION\n");

						pthread_rwlock_rdlock(&traffic_mode_data->rwlock);
							traffic_mode_data->mode = msg.mode;
						pthread_rwlock_unlock(&traffic_mode_data->rwlock);
						printf("\treceive mode: %d\n", msg.mode);

						msg_send.hdr.type = MODE_MSG;
						msg_send.data.mode = msg.mode;

						printf("\tMODE MSG\n");
						send_message_thread_data_t i1_send = {
								I1_CRITICAL_CHANNEL_ATTACH_POINT,
								&msg_send,
								NULL,
								PTHREAD_RWLOCK_INITIALIZER
						};
						send_message_thread_data_t i2_send = {
								I2_CRITICAL_CHANNEL_ATTACH_POINT,
								&msg_send,
								NULL,
								PTHREAD_RWLOCK_INITIALIZER
						};

						// Thread definitions
						pthread_t i1_tx_rst_data;
						pthread_t i2_tx_rst_data;

						// Thread settings
						pthread_attr_t i_tx_rst_attr;
						struct sched_param i_tx_rst_param;
						setThreadSettings(&i_tx_rst_attr, &i_tx_rst_param, 63, false);

						pthread_create(&i1_tx_rst_data, &i_tx_rst_attr, &sendMessage, &i1_send);
						pthread_create(&i2_tx_rst_data, &i_tx_rst_attr, &sendMessage, &i2_send);

						// Set the time limit
						struct timespec send_time_limit;
						clock_gettime( CLOCK_REALTIME , &send_time_limit );
						send_time_limit.tv_sec = send_time_limit.tv_sec + MSG_SEND_TIMEOUT;

						if (pthread_timedjoin(i1_tx_rst_data, NULL, &send_time_limit) == ETIMEDOUT) {

							// Didn't get a reply in time, cancel the thread
							printf("Didn't send or reply to Intersection within time\n");

							if (pthread_cancel(i1_tx_rst_data) == EOK) {
								printf("Thread cancelled\n");
							}
						}

						if (pthread_timedjoin(i2_tx_rst_data, NULL, &send_time_limit) == ETIMEDOUT) {

							// Didn't get a reply in time, cancel the thread
							printf("Didn't send or reply to Intersection within time\n");

							if (pthread_cancel(i2_tx_rst_data) == EOK) {
								printf("Thread cancelled\n");
							}
						}


						sprintf(replymsg.data.buf, "OK");
					}
					else if(msg.option == RAIL_MSG_OPTION){
						printf("RAIL_MSG_OPTION\n");

						msg_send.hdr.type = RAIL_STATE_MSG;
						msg_send.data.rail_state = msg.rail_state;

						printf("\tMODE MSG\n");
						send_message_thread_data_t i1_send = {
								I1_CRITICAL_CHANNEL_ATTACH_POINT,
								&msg_send,
								NULL,
								PTHREAD_RWLOCK_INITIALIZER
						};
						send_message_thread_data_t i2_send = {
								I2_CRITICAL_CHANNEL_ATTACH_POINT,
								&msg_send,
								NULL,
								PTHREAD_RWLOCK_INITIALIZER
						};

						// Thread definitions
						pthread_t i1_tx_rst_data;
						pthread_t i2_tx_rst_data;

						// Thread settings
						pthread_attr_t i_tx_rst_attr;
						struct sched_param i_tx_rst_param;
						setThreadSettings(&i_tx_rst_attr, &i_tx_rst_param, 63, false);

						pthread_create(&i1_tx_rst_data, &i_tx_rst_attr, &sendMessage, &i1_send);
						pthread_create(&i2_tx_rst_data, &i_tx_rst_attr, &sendMessage, &i2_send);

						// Set the time limit
						struct timespec send_time_limit;
						clock_gettime( CLOCK_REALTIME , &send_time_limit );
						send_time_limit.tv_sec = send_time_limit.tv_sec + MSG_SEND_TIMEOUT;

						if (pthread_timedjoin(i1_tx_rst_data, NULL, &send_time_limit) == ETIMEDOUT) {

							// Didn't get a reply in time, cancel the thread
							printf("Didn't send or reply to Intersection within time\n");

							if (pthread_cancel(i1_tx_rst_data) == EOK) {
								printf("Thread cancelled\n");
							}
						}

						if (pthread_timedjoin(i2_tx_rst_data, NULL, &send_time_limit) == ETIMEDOUT) {

							// Didn't get a reply in time, cancel the thread
							printf("Didn't send or reply to Intersection within time\n");

							if (pthread_cancel(i2_tx_rst_data) == EOK) {
								printf("Thread cancelled\n");
							}
						}

						sprintf(replymsg.data.buf, "OK");
					}
					else if(msg.option == FAULT_OPTION){
						printf("FAULT_OPTION\n");
						msg_send.hdr.type = FAULT_MSG;
						msg_send.data.fault = true;


						if(msg.node == ALL_NODES) {
							printf("\tALL_NODES\n");
							send_message_thread_data_t i1_send = {
									I1_CRITICAL_CHANNEL_ATTACH_POINT,
									&msg_send,
									NULL,
									PTHREAD_RWLOCK_INITIALIZER
							};
							send_message_thread_data_t i2_send = {
									I2_CRITICAL_CHANNEL_ATTACH_POINT,
									&msg_send,
									NULL,
									PTHREAD_RWLOCK_INITIALIZER
							};

							// Thread definitions
							pthread_t i1_tx_fault_data;
							pthread_t i2_tx_fault_data;

							// Thread settings
							pthread_attr_t i_tx_fault_attr;
							struct sched_param i_tx_fault_param;
							setThreadSettings(&i_tx_fault_attr, &i_tx_fault_param, 63, false);

							pthread_create(&i1_tx_fault_data, &i_tx_fault_attr, &sendMessage, &i1_send);
							pthread_create(&i2_tx_fault_data, &i_tx_fault_attr, &sendMessage, &i2_send);

							// Set the time limit
							struct timespec send_time_limit;
							clock_gettime( CLOCK_REALTIME , &send_time_limit );
							send_time_limit.tv_sec = send_time_limit.tv_sec + MSG_SEND_TIMEOUT;

							if (pthread_timedjoin(i1_tx_fault_data, NULL, &send_time_limit) == ETIMEDOUT) {

								// Didn't get a reply in time, cancel the thread
								printf("Didn't send or reply to Intersection within time\n");

								if (pthread_cancel(i1_tx_fault_data) == EOK) {
									printf("Thread cancelled\n");
								}
							}

							if (pthread_timedjoin(i2_tx_fault_data, NULL, &send_time_limit) == ETIMEDOUT) {

								// Didn't get a reply in time, cancel the thread
								printf("Didn't send or reply to Intersection within time\n");

								if (pthread_cancel(i2_tx_fault_data) == EOK) {
									printf("Thread cancelled\n");
								}
							}

						}
						else if(msg.node == I1_NODE) {
							printf("\tI1_NODE\n");
							send_message_thread_data_t i1_send = {
									I1_CRITICAL_CHANNEL_ATTACH_POINT,
									&msg_send,
									NULL,
									PTHREAD_RWLOCK_INITIALIZER
							};
							// Thread definitions
							pthread_t i1_tx_fault_data;

							// Thread settings
							pthread_attr_t i_tx_fault_attr;
							struct sched_param i_tx_fault_param;
							setThreadSettings(&i_tx_fault_attr, &i_tx_fault_param, 63, false);

							pthread_create(&i1_tx_fault_data, &i_tx_fault_attr, &sendMessage, &i1_send);

							// Set the time limit
							struct timespec send_time_limit;
							clock_gettime( CLOCK_REALTIME , &send_time_limit );
							send_time_limit.tv_sec = send_time_limit.tv_sec + MSG_SEND_TIMEOUT;

							if (pthread_timedjoin(i1_tx_fault_data, NULL, &send_time_limit) == ETIMEDOUT) {

								// Didn't get a reply in time, cancel the thread
								printf("Didn't send or reply to Intersection within time\n");

								if (pthread_cancel(i1_tx_fault_data) == EOK) {
									printf("Thread cancelled\n");
								}
							}

						}
						else if(msg.node == I2_NODE) {
							printf("\tI2_NODE\n");
							send_message_thread_data_t i2_send = {
									I2_CRITICAL_CHANNEL_ATTACH_POINT,
									&msg_send,
									NULL,
									PTHREAD_RWLOCK_INITIALIZER
							};

							pthread_t i2_tx_fault_data;

							// Thread settings
							pthread_attr_t i_tx_fault_attr;
							struct sched_param i_tx_fault_param;
							setThreadSettings(&i_tx_fault_attr, &i_tx_fault_param, 63, false);

							pthread_create(&i2_tx_fault_data, &i_tx_fault_attr, &sendMessage, &i2_send);

							// Set the time limit
							struct timespec send_time_limit;
							clock_gettime( CLOCK_REALTIME , &send_time_limit );
							send_time_limit.tv_sec = send_time_limit.tv_sec + MSG_SEND_TIMEOUT;

							if (pthread_timedjoin(i2_tx_fault_data, NULL, &send_time_limit) == ETIMEDOUT) {

								// Didn't get a reply in time, cancel the thread
								printf("Didn't send or reply to Intersection within time\n");

								if (pthread_cancel(i2_tx_fault_data) == EOK) {
									printf("Thread cancelled\n");
								}
							}
						}
					}

					else if (msg.option == RESET_OPTION){
						printf("RESET_OPTION\n");
						send_message_thread_data_t i1_send;
						send_message_thread_data_t i2_send;
						msg_send.hdr.type = RESET_MSG;
						msg_send.data.fault = true;

						if(msg.node == ALL_NODES){
							printf("\tALL_NODES\n");
							send_message_thread_data_t i1_send = {
									I1_CRITICAL_CHANNEL_ATTACH_POINT,
									&msg_send,
									NULL,
									PTHREAD_RWLOCK_INITIALIZER
							};
							send_message_thread_data_t i2_send = {
									I2_CRITICAL_CHANNEL_ATTACH_POINT,
									&msg_send,
									NULL,
									PTHREAD_RWLOCK_INITIALIZER
							};

							// Thread definitions
							pthread_t i1_tx_rst_data;
							pthread_t i2_tx_rst_data;

							// Thread settings
							pthread_attr_t i_tx_rst_attr;
							struct sched_param i_tx_rst_param;
							setThreadSettings(&i_tx_rst_attr, &i_tx_rst_param, 63, false);

							pthread_create(&i1_tx_rst_data, &i_tx_rst_attr, &sendMessage, &i1_send);
							pthread_create(&i2_tx_rst_data, &i_tx_rst_attr, &sendMessage, &i2_send);

							// Set the time limit
							struct timespec send_time_limit;
							clock_gettime( CLOCK_REALTIME , &send_time_limit );
							send_time_limit.tv_sec = send_time_limit.tv_sec + MSG_SEND_TIMEOUT;

							if (pthread_timedjoin(i1_tx_rst_data, NULL, &send_time_limit) == ETIMEDOUT) {

								// Didn't get a reply in time, cancel the thread
								printf("Didn't send or reply to Intersection within time\n");

								if (pthread_cancel(i1_tx_rst_data) == EOK) {
									printf("Thread cancelled\n");
								}
							}

							if (pthread_timedjoin(i2_tx_rst_data, NULL, &send_time_limit) == ETIMEDOUT) {

								// Didn't get a reply in time, cancel the thread
								printf("Didn't send or reply to Intersection within time\n");

								if (pthread_cancel(i2_tx_rst_data) == EOK) {
									printf("Thread cancelled\n");
								}
							}

							
						}
						else if(msg.node == I1_NODE) {
							printf("\tI1_NODE\n");
							send_message_thread_data_t i1_send = {
									I1_CRITICAL_CHANNEL_ATTACH_POINT,
									&msg_send,
									NULL,
									PTHREAD_RWLOCK_INITIALIZER
							};


							// Thread definitions
							pthread_t i1_tx_rst_data;

							// Thread settings
							pthread_attr_t i_tx_rst_attr;
							struct sched_param i_tx_rst_param;
							setThreadSettings(&i_tx_rst_attr, &i_tx_rst_param, 63, false);

							pthread_create(&i1_tx_rst_data, &i_tx_rst_attr, &sendMessage, &i1_send);

							// Set the time limit
							struct timespec send_time_limit;
							clock_gettime( CLOCK_REALTIME , &send_time_limit );
							send_time_limit.tv_sec = send_time_limit.tv_sec + MSG_SEND_TIMEOUT;

							if (pthread_timedjoin(i1_tx_rst_data, NULL, &send_time_limit) == ETIMEDOUT) {

								// Didn't get a reply in time, cancel the thread
								printf("Didn't send or reply to Intersection within time\n");

								if (pthread_cancel(i1_tx_rst_data) == EOK) {
									printf("Thread cancelled\n");
								}
							}




						}
						else if(msg.node == I2_NODE) {
							printf("\tI2_NODE\n");
							send_message_thread_data_t i2_send = {
									I2_CRITICAL_CHANNEL_ATTACH_POINT,
									&msg_send,
									NULL,
									PTHREAD_RWLOCK_INITIALIZER
							};

							// Thread definitions

							pthread_t i2_tx_rst_data;

							// Thread settings
							pthread_attr_t i_tx_rst_attr;
							struct sched_param i_tx_rst_param;
							setThreadSettings(&i_tx_rst_attr, &i_tx_rst_param, 63, false);

							pthread_create(&i2_tx_rst_data, &i_tx_rst_attr, &sendMessage, &i2_send);

							// Set the time limit
							struct timespec send_time_limit;
							clock_gettime( CLOCK_REALTIME , &send_time_limit );
							send_time_limit.tv_sec = send_time_limit.tv_sec + MSG_SEND_TIMEOUT;

							if (pthread_timedjoin(i2_tx_rst_data, NULL, &send_time_limit) == ETIMEDOUT) {

								// Didn't get a reply in time, cancel the thread
								printf("Didn't send or reply to Intersection within time\n");

								if (pthread_cancel(i2_tx_rst_data) == EOK) {
									printf("Thread cancelled\n");
								}
							}
						}
					}

					sprintf(replymsg.data.buf, "OK");
				}


				if (LOG) {
					sprintf(log_message, "\n    -----> replying with: '%s'\n", replymsg.data.buf);
					writeTrafficLogFile(LOG_FILE_NAME, log_message);
				}
				MsgReply(rcvid, EOK, &replymsg, sizeof(replymsg));
			}
		}
		else {
			printf("\nERROR: Server received something, but could not handle it correctly\n");
		}
	}
}

