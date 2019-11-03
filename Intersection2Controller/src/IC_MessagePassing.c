#include "IC_MessagePassing.h"


void * critical_server_channel_thread(void * data) {



	critical_server_data_t * thread_data = (critical_server_data_t *) data;
	traffic_state_data_t * traffic_state_data = thread_data->traffic_state_data;
	rail_state_data_t * rail_state_data = thread_data->rail_state_data;
	traffic_mode_data_t * traffic_mode_data = thread_data->traffic_mode_data;
	fault_data_t * fault_data = thread_data->fault_data;
	reset_data_t * reset_data = thread_data->reset_data;
	change_state_flags_t * change_state_flags = thread_data->change_state_flags;
	name_attach_t * attach;

	// Create a local name (/dev/name/...)
	if ((attach = createChannel(I2_CRITICAL_CHANNEL)) == NULL) {
		printf("(I2) - Couldn't create channel\n");
		pthread_exit(NULL);
	}
	else {
		thread_data->attach = attach;
		printf("(I2) - channel: %s\n ", I2_CRITICAL_CHANNEL);
	}



	msg_data_t msg;
	int rcvid = 0;
	int Stay_alive = 1, living = 0; // server stays running (ignores _PULSE_CODE_DISCONNECT request)

	msg_reply_t replymsg; // replymsg structure for sending back to client
	replymsg.hdr.type = REPLY_MSG;
	replymsg.hdr.subtype = 0x00;

	living = 1;

	bool reset = false;
	while (!reset) {
	// Do your MsgReceive's here now with the chid
	  rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);

	  if (rcvid == -1)  // Error condition, exit
	  {
		  if(LOG){
			  writeTrafficLogFile(LOG_FILE_NAME,"\n(I2) - Failed to MsgReceive \n");
		  }
		  break;
	  }

	  // did we receive a Pulse or message?
	  // for Pulses:
	  if (rcvid == 0)  //  Pulse received, work out what type
	  {
		  handlePulse(&msg);
		  continue;// go back to top of while loop
	  }

	  // for messages:
	  if(rcvid > 0) // if true then A message was received
	  {

		  if(msg.hdr.type >= _IO_BASE && msg.hdr.type <= _IO_MAX ){
			  handleIOConnect(&msg, rcvid);
		  }
		  else {
			   // A message (presumably ours) received

			   // put your message handling code here and assemble a reply message

			  if(LOG){
				  sprintf(log_message, "(I2) - Server received data packet with value of '%d' from client (ID:%d), ", msg.data.rail_state, msg.client_id);
				  writeTrafficLogFile(LOG_FILE_NAME,log_message);
			  }
			   fflush(stdout);
				   //sleep(1); // Delay the reply by a second (just for demonstration purposes)


			   if(msg.hdr.type == STATE_REQUEST_MSG){
				   printf("STATE_REQUEST_MSG\n");
				   pthread_mutex_lock(&traffic_state_data->mutex_state);
					   replymsg.data.traffic_state =  traffic_state_data->current_state;
				   pthread_mutex_unlock(&traffic_state_data->mutex_state);
			   }
			   else if(msg.hdr.type == RAIL_STATE_MSG){
				   printf("RAIL_STATE_MSG\n");
				   pthread_mutex_lock(&rail_state_data->mutex_state);
					   rail_state_data->current_state = msg.data.rail_state;
				   pthread_mutex_unlock(&rail_state_data->mutex_state);

				   sprintf(replymsg.data.buf,"OK");
			   }
			   else if(msg.hdr.type == MODE_MSG){
				   printf("MODE_MSG\n");
				   printf("mode msg: %d\n", msg.data.mode);
				   pthread_rwlock_wrlock(&traffic_mode_data->rwlock);
					   traffic_mode_data->mode = msg.data.mode;
				   pthread_rwlock_unlock(&traffic_mode_data->rwlock);

				   pthread_sleepon_lock();
						change_state_flags->state_timer_timeout = true;
						pthread_sleepon_signal(change_state_flags);
					pthread_sleepon_unlock();

				   sprintf(replymsg.data.buf,"OK");
			   }
			   else if(msg.hdr.type == FAULT_MSG){
				   printf("FAULT_MSG\n");
				   pthread_mutex_lock(&fault_data->mutex);
					   fault_data->fault = msg.data.fault;
				   pthread_mutex_unlock(&fault_data->mutex);

				   pthread_sleepon_lock();
						change_state_flags->fault = true;
						pthread_sleepon_signal(change_state_flags);
					pthread_sleepon_unlock();

				   sprintf(replymsg.data.buf,"OK");
			   }
			   else if(msg.hdr.type == RESET_MSG){
				   printf("RESET_MSG\n");
				   if(msg.data.reset){
					   reset = msg.data.reset;
					   printf("reset: %d\n", reset);
					   pthread_sleepon_lock();
							change_state_flags->reset = true;
							pthread_sleepon_signal(change_state_flags);
						pthread_sleepon_unlock();
				   }
				   sprintf(replymsg.data.buf,"OK");
			   }
			   else {
				   printf("msg.type: %d\n", msg.hdr.type);
				   printf("message type not defined\n");
			   }

			   if(LOG){
				   sprintf(log_message, "\n(I2)\t-----> replying with: '%d'\n",replymsg.data.traffic_state);
				   writeTrafficLogFile(LOG_FILE_NAME,log_message);
			   }
			   MsgReply(rcvid, EOK, &replymsg, sizeof(replymsg));
		  }

	  }
	  else
	  {
		  writeTrafficLogFile(LOG_FILE_NAME, "\nERROR: Server received something, but could not handle it correctly\n");
	  }

	}

	printf("name detach\n");

	return EXIT_SUCCESS;
}
