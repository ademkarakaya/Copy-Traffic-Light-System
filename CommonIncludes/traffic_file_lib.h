#pragma once

#include "traffic_types.h"

#define LOG 1
#define TEST 1



#define LOG_FILE_NAME "traffic.log"
#define TEST_FILE_NAME "traffic.test"

#define MAX_MESSAGE_LENGTH 100
char log_message[MAX_MESSAGE_LENGTH];

FILE * openTrafficFile(const char * filename);
int writeTrafficFile(const char * filename, char * msg);
int closeTrafficFile(FILE * fp);
