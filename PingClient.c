// PingClient.c

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
#include <poll.h>

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
// POSTCONDITION: returns the client output message when ping received, adds the RTT to RTTarr
// 		"PING received from machine_name: seq#=X time=Y ms\n"
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

	strcat(mssg, "PING received from ");
	strcat(mssg, hostname);
	strcat(mssg, ": seq#=");
	strncat(mssg, seq_num, 1+offset);
	strcat(mssg, " time=");
	strncat(mssg, RTT, strlen(RTT));
	strcat(mssg, " ms\n");
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
	double avg = 0;
	for(int i=0; i<len; i++) avg += arr[i];
	return avg/len;
}

void printStats(double *RTTarr, int RTTarr_length, int numTrnsmtd, int numRcvd, const char *hostname) {
// POSTCONDITION: prints the ping statistics of the RTTs
	double min = getMin(RTTarr, RTTarr_length);
	double max = getMax(RTTarr, RTTarr_length);
	double avg = getAvg(RTTarr, RTTarr_length);
	printf("--- %s ping statistics ---\n", hostname);
	printf("%i packets transmitted, ",numTrnsmtd);
	printf("%i received, ",numRcvd);
	float percentLoss = numTrnsmtd-numRcvd;
	percentLoss /= numTrnsmtd;
	percentLoss *= 100;
	printf("%.0f%% packet loss", percentLoss);
	if (percentLoss < 100) {
		printf(" rtt min/avg/max = ");
		printf("%.3f ", min);
		printf("%.3f ", avg);
		printf("%.3f ", max);
		printf("ms\n");
	} else printf("\n");
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

	// utility variables
	double RTTarr[10];  // int array to hold RTTs
	int RTTarr_length = 0;  // length of RTRarr
	int numTrnsmtd=0;  // counts number of packets transmitted
	int	numRcvd=0;  // counts number of packets received
	struct pollfd fd;  // timer
	int res;

	for (int seq_num=1; seq_num<=10; seq_num++) {
		char *newBuffer = calloc(256, sizeof(char));
		struct timeval current_time;
		struct timeval end;
		gettimeofday(&current_time, NULL);
		newBuffer = getPingMssg(seq_num,current_time.tv_sec,current_time.tv_usec);
		sendto(sock,newBuffer,strlen(newBuffer),0,(const struct sockaddr *)&server,length);
		numTrnsmtd++;
		free(newBuffer);

		// Timeout: got the timeout code from stackoverflow post: 
		// https://stackoverflow.com/questions/16163260/setting-timeout-for-recv-fcn-of-a-udp-socket
		fd.fd = sock;
		fd.events = POLLIN;
		res = poll(&fd,1,1000);  // 1000 ms timeout
		if (res == 0) printf("Request timeout for seq#=%i\n", seq_num);
		else if (res == -1) printf("error\n");	
		else {
			recvfrom(sock,buffer,256,0,(struct sockaddr *)&from, &length);
			numRcvd++;
			gettimeofday(&end, NULL);
			printf("%s",responseMssg(buffer, argv[1], end.tv_sec, end.tv_usec, RTTarr+RTTarr_length++));
			res = poll(&fd,1,1000);  // 1000 ms timeout
		}
	}
	printStats(RTTarr, RTTarr_length, numTrnsmtd, numRcvd, argv[1]);
	
	// CLOSE THE SOCKET
	close(sock);
	return 0;
}

