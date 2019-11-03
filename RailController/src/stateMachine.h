#pragma once

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/netmgr.h>
#include <sys/neutrino.h>

#include "RailController.h"
#include <traffic_types.h>

void *railStateMachine(void *data);
void printState(rail_state_t state);
