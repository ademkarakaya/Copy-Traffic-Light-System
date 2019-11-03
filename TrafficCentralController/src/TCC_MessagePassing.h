#pragma once

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/dispatch.h>
#include <time.h>

#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>

#include "TrafficCentralController.h"
#include <traffic_types.h>


void * getStateData(void *data);
void *critical_server_channel_thread(void *data);
