// PingClient.c

/* CLIENT CODE:
 * 10 times:
 * 		send ping request to server (PING seq_num time)
 * 		if (time waiting for reply > 1 second):
 * 			assume its packet || server's reply is lost
 * 			output got_response_mssg (just prints to terminal, not socket)
 * 		else:
 * 			output no_response_mssg
 * output ping_stats
 * 
 * CLIENT MESSAGE:
 * 	PING seq_num time
 *		seq_num: starts at 1, progresses to 10 for successive ping mssg
 *		time: when the client sends the message
 *
 * CLIENT OUTPUT MESSAGES:
 * got_response_mssg = "PING received from machine_name: seq#=X time=Y ms"
 * 			X is seq # of received packet
 * 			Y is RTT in ms
 * no_response_mssg = "Request timeout for seq#=X"
 * 
 */

/* TODO
 * implement min/max/avg calculations
 * implement wait 1 sec btwn each ping
 * implement drop packet error if RTT > 1sec 
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

char* getPingMssg(int i, int time_sec, int time_ms) {
// PRECONDITION: i is the sequence number of the ping segment
// POSTCONDITION: returns the message to ping: "PING seq_num time\n"
	char *mssg = calloc(256, sizeof(char));
	strcat(mssg, "PING ");
	char seq_num[3];
	char time_sec_arr[150];
	char time_ms_arr[100];
	sprintf(seq_num, "%d", i);
	sprintf(time_sec_arr, "%d", time_sec);
	sprintf(time_ms_arr, "%d", time_ms);
	strcat(mssg, seq_num);
	strcat(mssg, " ");
	strcat(mssg,time_sec_arr);
	strcat(mssg,".");
	strcat(mssg,time_ms_arr);
	strcat(mssg,"\n");
	return mssg;
}

char* getSeqNum(const char *buffer, int offset) {
// POSTCONDITION: parses the sequence number from the buffer, returns as c-string
	char *seq_num = calloc(256, sizeof(char));
	strcat(seq_num, &buffer[5]);  // buffer[5] = the sequence number
	if (offset != 0) strcat(seq_num, &buffer[6]); // case where seq_num=10
	return seq_num;
}

char* getTimeStamp(const char *buffer, int offset) {
// PRECONDTION: parses the timestamp from the buffer, returns in a char array
	char *parsedTime = calloc(100,sizeof(char));
	for (int i=7+offset; buffer[i]!='\n'; i++)
		strncat(parsedTime,&buffer[i],1);
	return parsedTime;
}

double timeIntsToFloat(int time_sec, int time_ms) {
// POSTCONDITION: returns time_sec.time_ms as a float
	char *pEnd;
	char *time_float_arr = calloc(256, sizeof(char));
	char time_sec_arr[100];
	char time_ms_arr[100];
	sprintf(time_sec_arr, "%d", time_sec);
	sprintf(time_ms_arr, "%d", time_ms);
	strcat(time_float_arr, time_sec_arr);
	strcat(time_float_arr, ".");
	strcat(time_float_arr, time_ms_arr);
	double time_float = strtod(time_float_arr, &pEnd);
	free(time_float_arr);
	return time_float;
}

char* getRTT(const char *buffer, int offset, int time_sec, int time_ms) {
// PRECONDITION: time_sec and time_ms are the current times
// POSTCONDITION: parses the sequence number from the buffer, returns as c-string
	char *pEnd;
	char *RTT_arr = calloc(256, sizeof(char));
	char *parsedSentTime = getTimeStamp(buffer,offset);
	double sentTime = strtod(parsedSentTime, &pEnd);
	double currentTime = timeIntsToFloat(time_sec, time_ms);
	double RTT = (currentTime-sentTime)*1000;  // converts seconds to ms
	sprintf(RTT_arr, "%.3f", RTT);
	return RTT_arr;
}

char* responseMssg(const char *buffer, const char *hostname, int time_sec, int time_ms, double *RTTarr_at_i) {
// PRECONDITION: buffer contains the Ping message: PING X Y\n, where X=seq_num, Y=RTT
// 		hostname is the name of the host passed through argv[1]
// POSTCONDITION: returns the client output message when ping received
// 		"PING received from machine_name: seq#=X time=Y ms\n"
// 		if RTT > 1 sec, prints timeout mssg
//		adds the RTT to RTTarr
	int offset = 0;  // if the sequence number is 10 (2 digits)
	if (buffer[6]=='0') offset++;
	char *mssg = calloc(256, sizeof(char));

	// get sequence number
	char *seq_num = calloc(256, sizeof(char));
	strcat(seq_num, getSeqNum(buffer,offset));
	
	// get RTT
	char *pEnd;
	char *RTT = calloc(256, sizeof(char));
	strcat(RTT, getRTT(buffer,offset,time_sec,time_ms));
	*RTTarr_at_i = strtod(RTT, &pEnd);

	// if RTT > 1 sec, getRTT returns -1, print error message
	if (!strcmp(RTT,"-1")) {  // strcmp() returns 0(false) if RTT=="-1"
		strcat(mssg, "Request timeout for seq#=");
		strncat(mssg, seq_num, 1+offset);
		strcat(mssg, "\n");
	} else {  // if !timeout, print PING received ...
		strcat(mssg, "PING received from ");
		strcat(mssg, hostname);
		strcat(mssg, ": seq#=");
		strncat(mssg, seq_num, 1+offset);
		strcat(mssg, " time=");
		strncat(mssg, RTT, strlen(RTT));
		strcat(mssg, " ms\n");
	}
	return mssg;
}

double getMin(double arr[], int len) {
	double min = arr[0];
	for(int i=0; i<len; i++)
		if (arr[i] < min) min = arr[i];
	return min;
} double getMax(double arr[], int len) {
	double max = arr[0];
	for(int i=0; i<len; i++)
		if (arr[i] > max) max = arr[i];
	return max;
} double getAvg(double arr[], int len) {
	double avg = arr[0];
	for(int i=0; i<len; i++) avg += arr[i];
	return avg/len;
}

void printStats(double *RTTarr, int RTTarr_length, int numTrnsmtd, int numRcvd, const char *hostname) {
	double min = getMin(RTTarr, RTTarr_length);
	double max = getMax(RTTarr, RTTarr_length);
	double avg = getAvg(RTTarr, RTTarr_length);
	printf("--- %s ping statistics ---\n", hostname);
	printf("%i packets transmitted, ",numTrnsmtd);
	printf("%i received, ",numRcvd);
	float percentLoss = ((1-numRcvd)/numTrnsmtd)*100;
	printf("%.0f%% packet loss ", percentLoss);
	printf("rtt min/avg/max = ");
	printf("%.3f ", min);
	printf("%.3f ", avg);
	printf("%.3f ", max);
	printf("ms\n");
}

int main(int argc, char *argv[]) {
	int sock;
	unsigned int length;
	struct sockaddr_in server, from;
	struct hostent *hp;
	char buffer[256];

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	server.sin_family = AF_INET;
	hp = gethostbyname(argv[1]);
	bcopy((char *)hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	server.sin_port = htons(atoi(argv[2]));
	length=sizeof(struct sockaddr_in);

	int RTTarr_length = 0;
	double RTTarr[10];  // int array to hold RTTs
	int numTrnsmtd=0;
	int	numRcvd=0;

	for (int seq_num=1; seq_num<=10; seq_num++) {
		char *newBuffer = calloc(256, sizeof(char));
		struct timeval current_time;
		struct timeval end;
		gettimeofday(&current_time, NULL);
		newBuffer = getPingMssg(seq_num,current_time.tv_sec,current_time.tv_usec);
		sendto(sock,newBuffer,strlen(newBuffer),0,(const struct sockaddr *)&server,length);
		numTrnsmtd++;
		recvfrom(sock,buffer,256,0,(struct sockaddr *)&from, &length);  // read from socket
		numRcvd++;
		gettimeofday(&end, NULL);
		printf("%s",responseMssg(buffer, argv[1], end.tv_sec, end.tv_usec, RTTarr+RTTarr_length++));
		free(newBuffer);
	}
	printStats(RTTarr, RTTarr_length, numTrnsmtd, numRcvd, argv[1]);
	
	// CLOSE THE SOCKET
	close(sock);
	return 0;
}

