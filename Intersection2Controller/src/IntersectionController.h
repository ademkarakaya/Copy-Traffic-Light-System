#pragma once

#include <traffic_file_lib.h>
#include <traffic_library.h>
#include <traffic_types.h>

#define NULL_PULSE 0
#define STATE_TIMER_EXPIRE_PULSE 1
#define PED_TIMER_EXPIRE_PULSE 2
#define PED_SENSE_TIMEOUT_EXPIRE_PULSE 3

#define TRAFFIC_INPUT_COUNT 6

// KEYS
#define NS_STR 1
#define NS_R 2
#define NS_P 3
#define EW_STR 5
#define EW_R 6
#define EW_P 7

// Input Signal Buffer
typedef struct {
	int length;

	int input_buffer[TRAFFIC_INPUT_COUNT];
} traffic_input_queue_t;

typedef struct {
	traffic_input_queue_t *input_queue;
	pthread_rwlock_t rwlock_input;
} traffic_input_data_t;

typedef struct {
	bool state_timer_timeout;
	bool ped_timer_timeout;
	bool ped_sense_timer_timeout;
	bool input;
	bool fault;
	bool reset;
}change_state_flags_t;

// thread data

// state machine thread
typedef struct {
	traffic_input_data_t * input_data;
	traffic_state_data_t * traffic_state_data;
	rail_state_data_t * rail_state_data;
	traffic_mode_data_t * traffic_mode_data;
	change_state_flags_t * change_state_flags;
	fault_data_t * fault_data;
	reset_data_t * reset_data;
} state_machine_thread_data_t;

// input thread
typedef struct {
	traffic_input_data_t * input_data;
	change_state_flags_t * change_state_flags;
} input_thread_data_t;

// output thread
typedef struct {
	traffic_state_data_t *traffic_state_data;
	rail_state_data_t *rail_state_data;
} output_thread_data_t;

// critical server thread
typedef struct {
	traffic_state_data_t * traffic_state_data;
	rail_state_data_t * rail_state_data;
	traffic_mode_data_t * traffic_mode_data;
	change_state_flags_t * change_state_flags;
	fault_data_t * fault_data;
	reset_data_t * reset_data;
	name_attach_t * attach;
} critical_server_data_t;

typedef struct {
	int * chid;
	change_state_flags_t * change_state_flags;
} timer_thread_data_t;
