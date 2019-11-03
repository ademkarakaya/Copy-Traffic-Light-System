#include "traffic_file_lib.h"


FILE *  openTrafficLogFile(const char * filename) {
	return fopen(filename, "a") ;
}

int writeTrafficLogFile(const char * filename, char * msg) {
	FILE * fp = openTrafficLogFile(filename);

	int retval = EOF;

	if(fp != NULL){
		retval = fputs(msg, fp);
		fclose(fp);
	}

	return retval;
}

int closeTrafficLogFile(FILE * fp) {
	return fclose(fp);
}
