#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <traffic_types.h>


void printOptions(){

	printf("Options:\n");
	printf("  -g state\tget state of node (default=all)\n");
	printf("  -g mode\t\tget system mode\n");
	printf("  -s rail_msg value\t\tset system mode\n");
	printf("  -s mode value\t\tset system mode\n");
	printf("  -s fault <node>\tset fault node to fault(default=all)\n");
	printf("  -s reset <node>\treset flag of node (default=all)\n");

	printf("\nNode:\n");
	printf("  i1: Intersection 1\n");
	printf("  i2: Intersection 2\n");

	printf("\rail_msg Value:\n");
	printf("  train: \tPeak\n");
	printf("  no_train: \tOff Peak\n");

	printf("\nMode Value:\n");
	printf("  peak: \tPeak\n");
	printf("  offpeak: \tOff Peak\n");
}

int main(int argc,char *argv[]) {

	enum FUNC func = INVALID_FUNC;
	enum OPTION option = INVALID_OPTION;
	enum NODE node = INVALID_NODE;
	traffic_mode_t mode;
	rail_state_t rail_state;

	int valid_input = 0;

	if(argc == 1){
		printOptions();
	}

	else if(argc >= 3 && argc <= 4){
		// argv 1
		if(0 == strcmp(argv[1], "-g")) {
			func = GET_FUNC;
		}
		else if(0 == strcmp(argv[1], "-s")){
			func = SET_FUNC;
		}
		else {
			printf("\nIncorrect Function: '%s'\n", argv[1]);
		}

		// argv 2
		if(0 == strcmp(argv[2], "state") && func == GET_FUNC) {
			option = STATE_OPTION;
		}
		else if(0 == strcmp(argv[2], "mode")){
			option = MODE_OPTION;
		}
		else if(0 == strcmp(argv[2], "rail_msg") && func == SET_FUNC){
			option = RAIL_MSG_OPTION;
		}
		else if(0 == strcmp(argv[2], "fault") && func == SET_FUNC){
			option = FAULT_OPTION;
		}
		else if(0 == strcmp(argv[2], "reset") && func == SET_FUNC){
			option = RESET_OPTION;
		}
		else {
			printf("\nIncorrect Option: '%s'\n", argv[2]);
		}

		// argv 3
		if(option == MODE_OPTION){
			if(argc == 3 && func == GET_FUNC){
				valid_input = 1;
			}
			else if(argc == 4 && func == SET_FUNC){
				if(0 == strcmp(argv[3], "peak")) {
					mode = PEAK;
					printf("peak mode: %d\n", mode);
					valid_input = 1;
				}
				else if(0 == strcmp(argv[3], "offpeak")){
					mode = OFF_PEAK;
					printf("offpeak mode: %d\n", mode);
					valid_input = 1;
				}
				else {
					printf("\nIncorrect Value: '%s'\n", argv[3]);
				}
			}
			else {
				printf("\nToo Many Inputs\n");

			}
		}
		else if(option == RAIL_MSG_OPTION){
			if(argc == 4){
				node = ALL_NODES;
				valid_input = 1;
				if(0 == strcmp(argv[3], "train")) {
					rail_state = TRAIN;
					valid_input = 1;
				}
				else if(0 == strcmp(argv[3], "no_train")) {
					rail_state = NO_TRAIN;
					valid_input = 1;
				}
				else {
					printf("\nIncorrect Msg: '%s'\n", argv[3]);
				}
			}
			else {
				printf("\nIncorrect Msg\n");
			}


		}
		else {
			if(argc == 3){
				node = ALL_NODES;
				valid_input = 1;
			}
			else {
				if(0 == strcmp(argv[3], "all")) {
					node = ALL_NODES;
					valid_input = 1;
				}
				else if(0 == strcmp(argv[3], "i1")) {
					node = I1_NODE;
					valid_input = 1;
				}
				else if(0 == strcmp(argv[3], "i2")){
					node = I2_NODE;
					valid_input = 1;
				}
				else {
					printf("\nIncorrect Node: '%s'\n", argv[3]);
				}
			}
		}
	}
	else if(argc < 3){
		printf("\nToo Few Inputs\n");
	}
	else if(argc > 4){
		printf("\nToo Many Inputs\n");
	}

	if(valid_input){
		tool_msg_t msg;
		msg.func = func;
		msg.option = option;
		msg.node = node;
		msg.mode = mode;
		msg.rail_state = rail_state;

		tool_reply_t replymsg; // replymsg structure for sending back to client

		int server_coid;
		printf("  ---> Trying to connect to server named: %s\n", TCC_INPUT_CHANNEL_ATTACH_POINT);
		if ((server_coid = name_open(TCC_INPUT_CHANNEL_ATTACH_POINT, 0)) == -1) {
			printf("\n    ERROR, could not connect to server!\n\n");
			return EXIT_SUCCESS;
		}

		printf("Connection established to: %s\n", TCC_INPUT_CHANNEL_ATTACH_POINT);
		fflush(stdout);

		if (MsgSend(server_coid, &msg, sizeof(msg), &replymsg, sizeof(replymsg)) == -1) {
			printf(" Error data '%d' NOT sent to server\n", msg.func);
			// maybe we did not get a reply from the server
		}
		if(option == STATE_OPTION && func == GET_FUNC) { // now process the reply
			printf("   -->Reply \n");
			printf("   \ti1  : '%d'\n", replymsg.data.state.i1);
			printf("   \ti2  : '%d'\n", replymsg.data.state.i2);
			printf("   \tx1  : '%d'\n", replymsg.data.state.x1);
		}
		if (option == MODE_OPTION && func == GET_FUNC){
			printf("   \tmode: '%d'\n", replymsg.data.state.mode);
		}

		ConnectDetach(server_coid);
	}


	return EXIT_SUCCESS;
}
