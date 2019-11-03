#pragma once
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/dispatch.h>
#include <sys/netmgr.h>

#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 100

#define INTERSECTION_COUNT 2

#define I1_HOSTNAME "VM_x86_Target01"
#define I2_HOSTNAME "VM_x86_Target02"
#define TCC_HOSTNAME "VM_x86_Target01"
#define RCC_HOSTNAME "cycloneV_user"
#define RAIL_HOSTNAME "RMIT_BBB_v5_03"

#define TCC_CRITICAL_CHANNEL "grp11_state_recieve"
#define TCC_INPUT_CHANNEL "grp11_input_recieve"
#define I1_CRITICAL_CHANNEL "grp11_rail_state_recieve"
#define I2_CRITICAL_CHANNEL "grp11_rail_state_recieve"
#define RC_CRITICAL_CHANNEL "grp11_sig_recieve"
#define RCC_CRITICAL_CHANNEL "grp11_rail_state_recieve"



/* QNET Channels */
#define TCC_CRITICAL_CHANNEL_ATTACH_POINT "/net/" TCC_HOSTNAME "/dev/name/local/" TCC_CRITICAL_CHANNEL // hostname using full path, change myname to the name used for server
#define TCC_INPUT_CHANNEL_ATTACH_POINT  "/net/" TCC_HOSTNAME "/dev/name/local/" TCC_INPUT_CHANNEL  // hostname using full path, change myname to the name used for server
#define I1_CRITICAL_CHANNEL_ATTACH_POINT "/net/" I1_HOSTNAME "/dev/name/local/" I1_CRITICAL_CHANNEL	// hostname using full path, change myname to the name used for server
#define I2_CRITICAL_CHANNEL_ATTACH_POINT "/net/" I2_HOSTNAME "/dev/name/local/" I2_CRITICAL_CHANNEL	// hostname using full path, change myname to the name used for server
#define RCC_CRITICAL_CHANNEL_ATTACH_POINT "/net/" RCC_HOSTNAME "/dev/name/local/" RCC_CRITICAL_CHANNEL
#define RC_CRITICAL_CHANNEL_ATTACH_POINT "/net/" RAIL_HOSTNAME "/dev/name/local/" RC_CRITICAL_CHANNEL


/* Client IDs */
#define I1_CLIENT_ID 1
#define I2_CLIENT_ID 2
#define TCC_CLIENT_ID 3
#define X1_CLIENT_ID 4
#define RCC_CLIENT_ID 5

/* Message Types */
#define STATE_REQUEST_MSG _IO_MAX + 1
#define RAIL_STATE_MSG _IO_MAX + 2
#define MODE_MSG _IO_MAX + 3
#define FAULT_MSG _IO_MAX + 4
#define RESET_MSG _IO_MAX + 5
#define REPLY_MSG _IO_MAX + 6

#define MSG_SEND_TIMEOUT 5

/* Thread priorities */
#define I_CRIT_SEVER_CHANNEL_THREAD_PRIORITY 61
#define I_TRAFFIF_STATE_MACHINE_THREAD_PRIORITY 63
#define I_INPUT_THREAD_PRIORITY 60
#define I_OUTPUT_THREAD_PRIORITY 62

#define RCC_CRIT_CLIENT_CHANNEL_THREAD_PRIORITY 63
#define RCC_CRIT_SERVER_CHANNEL_THREAD_PRIORITY 62
#define RCC_INPUT_THREAD_PRIORITY 61

#define RC_INPUT_THREAD_PRIORITY 60
#define RC_RAIL_STATE_MACHINE_THREAD_PRIORITY 63
#define RC_OUTPUT_THREAD_PRIORITY 62
#define RC_CRIT_CLIENT_CHANNEL_THREAD_PRIORITY 61
#define RC_CRIT_SERVER_CHANNEL_THREAD_PRIORITY 63

#define TCC_OUTPUT_THREAD_PRIORITY 60
#define TCC_CRIT_SERVER_CHANNEL_THREAD_PRIORITY 60
#define TCC_SEND_THREAD_PRIORITY 60
#define TCC_INPUT_THREAD_PRIORITY 61

/* Pulse priorities */
#define PULSE_PRIORITY 10

/* Boolean typedef */
typedef enum {
	false,
	true
} bool;

/* Traffic light states */
typedef enum {

	INITIAL,
	ALL_RED,
	FAULT,

	NS_S_G_SEN,
	NS_S_G_P_X,
	NS_S_G_P_F,
	NS_S_G,
	NS_S_Y,

	EW_S_G_SEN,
	EW_S_G_P_X,
	EW_S_G_P_F,
	EW_S_G,
	EW_S_Y,

	EW_R_G,
	EW_R_Y,

	NS_R_G,
	NS_R_Y

} traffic_state_t;

/* Train states*/
typedef enum {
	TRAIN,
	NO_TRAIN,
	FAULT_TRAIN
} rail_state_t;

typedef enum {
	PEAK,
	OFF_PEAK
} traffic_mode_t;

/* Struct for r/w locking of traffic states */
typedef struct {
	traffic_state_t current_state;

	bool data_ready;

	pthread_mutex_t mutex_state;	   // needs to be set to PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cond_state_changed; // needs to be set to PTHREAD_COND_INITIALIZER;

} traffic_state_data_t;

/* Struct for r/w locking of rail states*/
typedef struct {

	rail_state_t current_state;

	bool data_ready;
	pthread_mutex_t mutex_state;	   // needs to be set to PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cond_state_changed; // needs to be set to PTHREAD_COND_INITIALIZER;

} rail_state_data_t;

typedef struct {
	traffic_mode_t mode;
	pthread_rwlock_t rwlock;
} traffic_mode_data_t;

/* Struct for r/w locking of rail states*/
typedef struct {
	bool fault;
	pthread_mutex_t mutex; // needs to be set to PTHREAD_MUTEX_INITIALIZER;
} fault_data_t;

/* Struct for r/w locking of rail states*/
typedef struct {
	bool reset;

	bool data_ready;
	pthread_mutex_t mutex; // needs to be set to PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cond;   // needs to be set to PTHREAD_COND_INITIALIZER;

} reset_data_t;

typedef struct {
	struct _pulse hdr; // Our real data comes after this header
	int client_id;
	union {
		traffic_state_t traffic_state;
		rail_state_t rail_state;
		traffic_mode_t mode;
		bool fault;
		bool reset;
	} data;
} msg_data_t;

typedef struct {
	struct _pulse hdr; // Our real data comes after this header
	union {
		char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
		traffic_state_t traffic_state;
		rail_state_t rail_state;
		traffic_mode_t mode;
		bool fault;
		bool reset;
	} data;
} msg_reply_t;

enum FUNC {INVALID_FUNC, SET_FUNC, GET_FUNC};
enum OPTION {INVALID_OPTION, STATE_OPTION, MODE_OPTION, RAIL_MSG_OPTION, FAULT_OPTION, RESET_OPTION};
enum NODE {INVALID_NODE, ALL_NODES, I1_NODE, I2_NODE, X1_NODE};

typedef struct {
	struct _pulse hdr;
	enum FUNC func;
	enum OPTION option;
	enum NODE node;
	traffic_mode_t mode;
	rail_state_t rail_state;
} tool_msg_t;

typedef struct {
	struct _pulse hdr;
	union{
		char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
		struct {
			traffic_state_t i1;
			traffic_state_t i2;
			rail_state_t x1;
			traffic_mode_t mode;
		} state;
	}data;

} tool_reply_t;
