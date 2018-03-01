#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstdlib>
#include "strings.h"
#include "unistd.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <queue>
#include <signal.h>
#include <sys/time.h>
#include "md5sum.h"
#include <time.h>
#include <stdlib.h>
using namespace std;

const int SERVER_PORT = 5432;
const int MAX_LINE = 256;
const int HEADER_SIZE = 11;
    
void catch_timeout(int);
string addPacketId(string, int);
void setPacketId(char *, int );
bool incrementPosition(int); 

unsigned int timeout_interval;
int s;
char **buf;
int packet_size;
int packet_id = 0;
int cur_pos_window = 0;

int window_size;
int seq_range;
ifstream file;
char buf1[MAX_LINE];

int ack_id;
int server_ack_id;

queue<int> dropped_packets;
queue<int> damaged_packets;


int main(int argc, char * argv[]) { 
    cout << "Packet Size = ";
    cin >> packet_size;
    cout << "Timeout Interval = ";
    cin >> timeout_interval;
    cout << "Sliding Window Size = ";
    cin >> window_size;
    cout << "Range of Sequence Numbers = ";
    cin >> seq_range;

    cout << "Packet Size with header is: " << packet_size << endl;
    
    srand(time(NULL));
    int s_errs = 9;
    while (s_errs != 0 && s_errs != 1 && s_errs != 2) {
        cout << "For No Errors Enter 0" << endl << "For User Specified Enter 1" << endl;
        cin >> s_errs;
    }

    int temp = 9;
    if (s_errs == 1){
        cout << "Please Type in Order Packets that should be dropped = (Enter -99 to exit and -98 to generate random value)" << endl;
        while(temp != -99) {
            cin >> temp;
            if (temp >= 0)
                dropped_packets.push(temp);
            if (temp == -98) {
                temp = rand() % seq_range;
                dropped_packets.push(temp);
                cout << temp << endl;
            }
        }
        temp = 9;
        cout << "Please Type in Order Packets that should be damaged = (Enter -99 to exit and -98 to generate random value)" << endl;
        while(temp != -99) {
            cin >> temp;
            if (temp >= 0)
                damaged_packets.push(temp);
            if (temp == -98){
                temp = rand() % seq_range;
                damaged_packets.push(temp);
                cout << temp << endl;
            }
        }
    }
    
    //FILE *fp;
	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
    buf = (char **)malloc(window_size * sizeof(char *));
    if(buf == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
    for(int i = 0; i < window_size; i++) {
        buf[i] = (char *)malloc((HEADER_SIZE+packet_size) * sizeof(char));
        if(buf[i] == NULL) {
            fprintf(stderr, "out of memory\n");
            return 1;
        }
    }
    socklen_t len;
	if (argc==2) {
		host = argv[1];
	}
	else {
		fprintf(stderr, "usage: simplex-talk host\n");
		exit(1);
                cout << "ERROR RECV RETURNED < 0" << endl;
	}
	/* translate host name into peer's IP address */
	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
		exit(1);
	}
	/* build address data structure */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	//sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);
	/* setup passive open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("simplex-talk: socket");
		exit(1);
	}
	if ((connect(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
		perror("simplex-talk: bind");
        close(s);
		exit(1);
	}


    string filename;
    cout << "Please Type The Filename to Send" << endl;
    cin  >> filename;

    string outFilename;     
    cout << "Please Type The Filename to Save" << endl;
    cin >> outFilename;
    //outFilename = outFilename.substr(0, outFilename.length()-1);

    //Send File Name
	send(s, outFilename.c_str(), outFilename.length()+1, 0);
    //cout << "Message Sent" << endl;
    len = recv(s, buf1, sizeof(buf1), 0);
    //fputs(buf1, stdout);
    //cout << endl;

    //Open file
    file.open(filename); // ios_base::in | ios_base::binary); 
    file.seekg(0,file.end);
    int size_of_file = file.tellg();
    file.seekg(0,file.beg);

    //Send file size
    string sz_file = to_string(size_of_file);
	send(s, sz_file.c_str(), sz_file.length()+1, 0);
    //cout << "Message Sent" << endl;
    len = recv(s, buf1, sizeof(buf1), 0);
    //fputs(buf1, stdout);
    //cout << endl;


    //Get MD5 checksum
    unsigned char checksum[MD5_DIGEST_LENGTH];
    md5sum(filename.c_str(),checksum);

    //Send checksum
	send(s, checksum, MD5_DIGEST_LENGTH, 0);
    //cout << "Message Sent" << endl;
    len = recv(s, buf1, sizeof(buf1), 0);
    //fputs(buf1, stdout);
    //cout << endl;

    //Send Packet Size
    string s_packet_size = to_string(packet_size);
	send(s, s_packet_size.c_str(), s_packet_size.length()+1, 0);
    //cout << "Message Sent" << endl;
    len = recv(s, buf1, sizeof(buf1), 0);
    //fputs(buf1, stdout);
    //cout << endl;

    //Send Seq Range
    string s_seq_range = to_string(seq_range);
	send(s, s_seq_range.c_str(), s_seq_range.length()+1, 0);
    //cout << "Message Sent" << endl;
    len = recv(s, buf1, sizeof(buf1), 0);
    //fputs(buf1, stdout);
    //cout << endl;

    //Set up timeout handler
    signal(SIGALRM, catch_timeout);


    cout << "Setting Up Initial Window" << endl;
    for (int i = 0; i < window_size; i++) {
        file.read(&buf[i][HEADER_SIZE], packet_size);
        setPacketId(buf[i],seq_range);
        packet_id += 1;
        packet_id %= seq_range;

        //print out buf
        //cout << "Printing Header" << endl;
        //fputs(buf[i], stdout);
        //cout << endl;
        //cout << "Printing Message" << endl;
        //fputs(&buf[i][HEADER_SIZE], stdout);
        //cout << endl;
    }

    ack_id = 0;
    bool cont = true;
    int increment_num;

    while (cont){ //file.read(&buf[cur_pos_window][HEADER_SIZE], packet_size)) {
        //cout << "Resending Window Timeout Will Be Re-Enabled" << endl;
        catch_timeout(0);
        
        //cout << "Waiting for ACK" << ack_id << endl;
        len = recv(s, buf1, sizeof(buf1), 0);
        alarm(0);
        server_ack_id = atoi(buf1);
        //cout << "Got ACK" << server_ack_id << endl;
        //cout << "Timeout Canceled" << endl;
        
        //cout << "Printing Header" << endl;
        //fputs(buf1, stdout);
        //cout << endl;
        //cout << "Printing Message" << endl;
        //fputs(&buf1[HEADER_SIZE], stdout);
        //cout << endl;
        
        //cout << "Ack ID is " << ack_id << endl;
        ///cout << "Server Ack ID is " << server_ack_id << endl;
        increment_num = server_ack_id - ack_id + 1;
        if (increment_num < 0) 
            increment_num += seq_range;
        //cout << "Incrementing " << increment_num << " times" << endl;
        for (int k = 0; k < increment_num; k++)
            cont = incrementPosition(s);
        ack_id = server_ack_id + 1;
        ack_id %= seq_range;

	}
 
    alarm(0);
    sleep(timeout_interval);

    cout << "Freeing Memory used by Transmit Window" << endl;
    for (int l=0; l < window_size; l++)
        free(buf[l]);
    free(buf);

    file.close();
    cout << "Closing File" << endl;
    
    close(s);
    cout << "Closing Connection" << endl;
    return 0;
}


void catch_timeout(int sig) {
    int cur_packet = (dropped_packets.empty()) ? -99 : dropped_packets.front();

    for (int i = cur_pos_window; i < window_size; i++) {
        if (atoi(buf[i]) == cur_packet) {
            dropped_packets.pop();
            cur_packet = (dropped_packets.empty()) ? -99 : dropped_packets.front();
        }
        else {
            send(s, buf[i], HEADER_SIZE+packet_size, 0);
            ///cout << endl << "Header="; fputs(buf[i], stdout); cout << endl;
            //cout << "Message="; fputs(&buf[i][HEADER_SIZE], stdout); cout << endl;
            //cout << "Waiting for " << timeout_interval << " seconds" << endl;
        }
    }
    for (int i = 0; i < cur_pos_window; i++) {
        if (atoi(buf[i]) == cur_packet) {
            dropped_packets.pop();
            cur_packet = (dropped_packets.empty()) ? -99 : dropped_packets.front();
        }
        else {
            send(s, buf[i], HEADER_SIZE+packet_size, 0);
            //cout << endl << "Header="; fputs(buf[i], stdout); cout << endl;
            //cout << "Message="; fputs(&buf[i][HEADER_SIZE], stdout); cout << endl;
            ///cout << "Waiting for " << timeout_interval << " seconds" << endl;
        }
    }
    alarm(timeout_interval);
    //cout << endl << "Messages Sent: Waiting for " << timeout_interval << " seconds" << endl;
}

void setPacketId(char *buf, int seq_range) {
    string s_packet_id = to_string(packet_id); 
    int num_zeros = HEADER_SIZE - s_packet_id.length()-1;
    for (int i =0; i < num_zeros; i++) {
        buf[i] = '0';
    }
    for (int i =0; i < s_packet_id.length(); i++) {
        buf[num_zeros+i] = s_packet_id.at(i);
    }
    buf[HEADER_SIZE-1] = '\0';

    int cur_packet = (damaged_packets.empty()) ? -99 : damaged_packets.front();
    //cout << "Looking to Damage Packet" << cur_packet << endl;
    if (packet_id == cur_packet) {
        cout << "Packet Id " << cur_packet << " will be damaged" << endl;
        buf[HEADER_SIZE] += 1;
        damaged_packets.pop();
    }
}



bool incrementPosition(int s) {
    if (file.read(&buf[cur_pos_window][HEADER_SIZE], packet_size)) {
        //cout << "Just Read In From File" << endl;
        
        //cout << "Setting Up Packet for Currrent Position" << endl;
        //cout << "Current Pos=" << cur_pos_window << endl;
        
        //cout << "Set Packet Id then Increment It" << endl;
        setPacketId(buf[cur_pos_window],seq_range);
        packet_id += 1;
        packet_id %= seq_range;
        
        //cout << "Packet ID Incremented and is now =" << packet_id << endl;
        //cout << "Printing Header" << endl;
        //fputs(buf[cur_pos_window], stdout); cout << endl;
        //cout << "Printing Message" << endl;
        //fputs(&buf[cur_pos_window][HEADER_SIZE], stdout); cout << endl;
        
        cur_pos_window += 1;
        cur_pos_window %= window_size;
        //cout << "Current Pos Now=" << cur_pos_window << endl;

        return true;
	}
 
    //Send Final Packet
    else if (file.gcount() != 0) {
        int gcount = file.gcount();
        cout << "Size of Final Packet: " << gcount << endl;

        file.read(&buf[cur_pos_window][HEADER_SIZE], gcount);

        cout << "Final Packet Id is " << packet_id << endl;
        setPacketId(buf[cur_pos_window],seq_range);
        //fputs(buf[cur_pos_window], stdout); cout << endl;
        //fputs(&buf[cur_pos_window][HEADER_SIZE], stdout); cout << endl;    

        cur_pos_window += 1;
        cur_pos_window %= window_size;
        //cout << "Current Pos Now=" << cur_pos_window << endl;
    
        catch_timeout(0);

        //int error1 = 0;
        //socklen_t len1 = sizeof (error1);
        socklen_t len;
        while (packet_id != server_ack_id) {//getsockopt (s, SOL_SOCKET, SO_ERROR, &error1, &len1 )) {
            len = recv(s, buf1, sizeof(buf1), 0);
            //print message to std out
            //cout << "Printing Header" << endl;
            //fputs(buf1, stdout);
            //cout << endl;
            //cout << "Printing Message" << endl;
            //fputs(&buf1[HEADER_SIZE], stdout);
            //cout << endl;

            server_ack_id = atoi(buf1);
            cout << "Final Packet ID is " << packet_id << endl;
            cout << "Server Ack ID is " << server_ack_id << endl;
        }
        cout << endl << "Final ACK Recieved and Printed to STDOUT" << endl;
        return false;
    }
    else {
        cout << "No Final Packet" << endl;
        return false;
    }
}
