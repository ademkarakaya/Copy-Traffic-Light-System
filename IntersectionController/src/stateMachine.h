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

#include "IntersectionController.h"
#include <traffic_file_lib.h>
#include <traffic_types.h>

#define TIME_TRAFFIC_INITIAL 1
#define TIME_TRAFFIC_STRAIGHT 10 //NEVER LESS THAN 10
#define TIME_TRAFFIC_TURN 3
#define TIME_TRAFFIC_YELLOW 1
#define TIME_TRAFFIC_ALL_RED 1
#define TIME_TRAFFIC_NONE 0

#define TIME_PED_CROSS 3
#define TIME_PED_FLASHING 2
#define TIME_PED_TIME_OUT TIME_TRAFFIC_STRAIGHT-(TIME_PED_CROSS+TIME_PED_FLASHING+1)

#define TIME_PED_GREEN 30
#define TIME_PED_YELLOW 30

struct sigevent state_timer_event;
struct sigevent ped_timer_event;
struct sigevent ped_sense_timeout_timer_event;


timer_t state_timer_id;
timer_t ped_timer_id;
timer_t ped_sense_timeout_timer_id;

struct itimerspec itime_state_timer;
struct itimerspec itime_ped_timer; // used for ped/ped sense
struct itimerspec itime_ped_sense_timeout_timer; // used for ped/ped sense

void *trafficStateMachine(void *data);

void initiliseTimer();
traffic_state_t handleFaultSig(traffic_state_t current_state);
traffic_state_t changeTrafficState_peak(traffic_state_t current_state, traffic_state_t previous_state, traffic_input_queue_t *input_data);
traffic_state_t changeTrafficState_offPeak(traffic_state_t current_state, traffic_state_t previous_state, traffic_input_queue_t *input_data);
void updateTimersPeak(traffic_state_t current_state);
void updateTimersOffPeak(traffic_state_t current_state);

bool decideChangeStatePeak(change_state_flags_t * change_state_flags, traffic_state_t current_state, traffic_input_queue_t *input_queue);
bool decideChangeStateOffPeak(change_state_flags_t * change_state_flags, traffic_state_t current_state, traffic_input_queue_t *input_queue);
void clearInputPeak(traffic_input_data_t *input_data, traffic_state_t state);
void clearInputOffPeak(traffic_input_data_t *input_data, traffic_state_t state);
void clearFlags(change_state_flags_t * state_flags, traffic_input_data_t * input_data);

void stateToString(traffic_state_t state, char *state_str);
//void printInputBuf(int * buf, int length);
