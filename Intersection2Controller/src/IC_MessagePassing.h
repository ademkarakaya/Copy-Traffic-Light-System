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

#include "IntersectionController.h"
#include <traffic_types.h>

void *critical_server_channel_thread(void *data);
