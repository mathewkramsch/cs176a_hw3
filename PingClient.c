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

char* getRTT(const char *buffer, int offset, int time_sec, int time_ms) {
// PRECONDITION: time_sec and time_ms are the current times
// POSTCONDITION: parses the sequence number from the buffer, returns as c-string
	char *RTT_arr = calloc(256, sizeof(char));
	char *parsedSentTime = getTimeStamp(buffer,offset);
	int sentTime_sec;
	int sentTime_ms;
	int RTT; // current time - time packet was sent
	strcat(RTT_arr, parsedSentTime);
	return RTT_arr;
}

char* gotResponseMssg(const char *buffer, const char *hostname, int time_sec, int time_ms) {
// PRECONDITION: buffer contains the Ping message: PING X Y\n, where X=seq_num, Y=RTT
// 		hostname is the name of the host passed through argv[1]
// POSTCONDITION: returns the client output message when ping received
// 		"PING received from machine_name: seq#=X time=Y ms\n"
	int offset = 0;  // if the sequence number is 10 (2 digits)
	if (buffer[6]=='0') offset++;
	char *mssg = calloc(256, sizeof(char));
	strcat(mssg, "PING received from ");
	strcat(mssg, hostname);
	strcat(mssg, ": seq#=");
	strncat(mssg, getSeqNum(buffer,offset), 1+offset);
	strcat(mssg, " time=");
	strncat(mssg, getRTT(buffer,offset,time_sec,time_ms), strlen(getRTT(buffer,offset,time_sec,time_ms)));
	strcat(mssg, " ms\n");
	return mssg;
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
	
	for (int seq_num=1; seq_num<=10; seq_num++) {
		char *newBuffer = calloc(256, sizeof(char));
		struct timeval current_time;
		struct timeval end;
		gettimeofday(&current_time, NULL);
		newBuffer = getPingMssg(seq_num,current_time.tv_sec,current_time.tv_usec);
		printf("%s",newBuffer);
		sendto(sock,newBuffer,strlen(newBuffer),0,(const struct sockaddr *)&server,length);
		recvfrom(sock,buffer,256,0,(struct sockaddr *)&from, &length);  // read from socket
		gettimeofday(&end, NULL);
		printf("%s",gotResponseMssg(buffer, argv[1], end.tv_sec, end.tv_usec));
	}
	
	// CLOSE THE SOCKET
	close(sock);
	return 0;
}

