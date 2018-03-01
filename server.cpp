#include <stdio.h>
#include <sys/types.h>
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
#include "md5sum.h"
#include <time.h>
#include <stdlib.h>
using namespace std;

const int SERVER_PORT = 5432;
const int MAX_PENDING = 5;
const int MAX_LINE = 256;
const int HEADER_SIZE = 11;

int packet_id = 0;
int seq_range;


void setPacketId(char *, int);

string addPacketId(string, int);

queue<int> dropped_ACKs;
int cur_ACK; 

int total_packets = 0;

int start_time;
int final_time;

int main() {

    struct sockaddr_in sin;
	char buf1[MAX_LINE];
	socklen_t len;
	int s, new_s;
	
    /* build address data structure */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);
	
    /* setup passive open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("simplex-talk: socket");
		exit(1);
	}
	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
		perror("simplex-talk: bind");
		exit(1);
	}
	listen(s, MAX_PENDING);

	/* wait for connection, then receive and print text */
	if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
        perror("simplex-talk: accept");
        exit(1);
    }

    len = recv(new_s, buf1, sizeof(buf1), 0);
    string outFileName(buf1); 
    cout << "Recieved Filename ";
    fputs(buf1, stdout);
    cout << endl;

    ofstream of(outFileName);
    cout << "Opened File " << outFileName << " For Writing" << endl;
    if (!of.is_open())
        cout << "ERR FILE NOT OPEN" << endl;

    //send ack to client
    send(new_s, "ACK\0", 4, 0);
    cout << "ACK Sent to " << new_s << endl;
    
    //wait for file size
    len = recv(new_s, buf1, sizeof(buf1), 0);
    int sz_file = atoi(buf1);
    int char_received = 0;
    cout << "Size of File is: " << sz_file << endl;
        
    //send ack to client
    send(new_s, "ACK\0", 4, 0);
    cout << "ACK Sent to " << new_s << endl;
    
    //wait for checksum
    unsigned char checksum[MD5_DIGEST_LENGTH+1];
    len = recv(new_s, checksum, MD5_DIGEST_LENGTH, 0);
    checksum[MD5_DIGEST_LENGTH] = '\0';
    cout << "MD5 checksum is: " << checksum << endl;
        
    //send ack to client
    send(new_s, "ACK\0", 4, 0);
    cout << "ACK Sent to " << new_s << endl;
    
    //wait for packet size
    len = recv(new_s, buf1, sizeof(buf1), 0);
    int packet_size = atoi(buf1);
	char buf[packet_size+HEADER_SIZE];
    cout << "Size of Packet is: " << packet_size << endl;
        
    //send ack to client
    send(new_s, "ACK\0", 4, 0);
    cout << "ACK Sent to " << new_s << endl;
    
    //wait for seq range
    len = recv(new_s, buf1, sizeof(buf1), 0);
    seq_range = atoi(buf1);
    cout << "Seq Range is: " << seq_range << endl;
        
    //send ack to client
    send(new_s, "ACK\0", 4, 0);
    cout << "ACK Sent to " << new_s << endl;
    
    cout << "Please Type in Order ACKs that should be dropped = (Enter -99 to exit and -98 to generate random value)" << endl;
    int temp = 9;
    while(temp != -99) {
        cin >> temp;
        if (temp > 0)
            dropped_ACKs.push(temp);
        if (temp == -98) {
            temp = rand() % seq_range;
            dropped_ACKs.push(temp);
            cout << temp << endl;
        }
    }
    
    cout << "Recieved Initial Configuration Information" << endl << "Now Entering Receive Loop" << endl;

    bool got_correct_packet = false;
    int client_packet_id;
    start_time = time(NULL);
    cout << "Start time = " << start_time << endl;
    while(packet_size+char_received < sz_file) { 

        while (!got_correct_packet) {
            //cout << "Waiting for Message" << endl;
            len = recv(new_s, buf, sizeof(buf), 0);

            //cout << "Printing Header" << endl;
            //fputs(buf, stdout);
            //cout << endl;
            //cout << "Printing Message" << endl;
            //fputs(&buf[HEADER_SIZE], stdout);
            //cout << endl;

            //Strip Packet Id and see if they match
            client_packet_id = atoi(buf);
            //cout << "Client Packet Id is " << client_packet_id << endl;
            //cout << "Packet Id is " << packet_id << endl;
            if (client_packet_id == packet_id)
                got_correct_packet = true;
            else
                got_correct_packet = false;
        }
	    total_packets++;
        //cout << "Got Correct Packet Now Printing To File" << endl;
        for (int i = HEADER_SIZE; i <  HEADER_SIZE+packet_size; i++)
            of << buf[i];

        cur_ACK = (dropped_ACKs.empty()) ? -99 : dropped_ACKs.front();
        if (atoi(buf) == cur_ACK) {
            dropped_ACKs.pop();
        }
        else {
            //cout << "send ack to client" << endl;
            //cout << endl << "ACK" << packet_id  << " Sent to " << new_s << endl;
            send(new_s, addPacketId("ACK\0", seq_range).c_str(), HEADER_SIZE+4, 0);
        }

        //cout << "Increament packet id" << endl;
        packet_id += 1;
        packet_id %= seq_range;
        //cout << "Packet ID is " << packet_id << endl;

        //cout << "Set loop variables" << endl;
        got_correct_packet = false;
        char_received += packet_size;
	} 

    //Calculate amount of file left
    int char_left = sz_file-char_received;
    cout << "Size of Final Packet: " << char_left << endl;

    cout << "Final Message" << endl;
    while (!got_correct_packet) {
        cout << "Wait for Message" << endl;
        len = recv(new_s, buf, sizeof(buf), 0);

        cout << "Printing Header" << endl;
        fputs(buf, stdout);
        cout << endl;
        
        cout << "Printing Message" << endl;
        fputs(&buf[HEADER_SIZE], stdout);
        cout << endl;

        cout << "Strip Packet Id and see if they match" << endl;
        client_packet_id = atoi(buf);
        cout << "Final Client Packet Id is " << client_packet_id << endl;
        cout << "Final Packet Id is " << packet_id << endl;
        if (client_packet_id == packet_id)
            got_correct_packet = true;
        else
            got_correct_packet = false;
    }
    
    cout << "Save to File" << endl;
    for (int i = HEADER_SIZE; i < char_left + HEADER_SIZE; i++)
        of << buf[i]; 
    
    cout << "Send Final Ack to client" << endl;
    send(new_s, addPacketId("ACK\0", seq_range).c_str(), HEADER_SIZE+4, 0);
    cout << endl << "ACK Sent to " << new_s << " For Final Message" << endl;

    final_time = time(NULL);
    cout << "Final Time = " << final_time << endl;
    cout << "Time To Complete = " << final_time - start_time << endl;
    cout << "Amount of Packets Sent = " << total_packets << endl;
    cout << "Throughput = " << total_packets/((final_time-start_time)*packet_size*sizeof(char)+0.00001) << " bytes per second" << endl;
 

    cout << "Closing Connections" << endl;
	close(new_s);
    cout << "Closing File" << endl;
    of.close();

    cout << "Checking md5sum" << endl;
    unsigned char checksum_final[MD5_DIGEST_LENGTH+1];
    md5sum(outFileName.c_str(), checksum_final);
    checksum_final[MD5_DIGEST_LENGTH] = '\0';
    bool checksum_was_good = true;
    for (int k =0; k <= MD5_DIGEST_LENGTH; k++)
        if (checksum[k] != checksum_final[k]){
            checksum_was_good = false;
            break;
        }
    if (checksum_was_good)
        cout << "File Transmitted Successfully" << endl;
    else
        cout << "File Transmission Failed MD5 Check" << endl << "Please Try Again, File May Have Been Corrupted" << endl;
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
    //packet_id += 1;
    //packet_id %= seq_range;
}

string addPacketId(string s, int seq_range) {
    string s_packet_id = to_string(packet_id); 
    int num_zeros = HEADER_SIZE - s_packet_id.length()-1;
    string zeros = "";
    for (int i =0; i < num_zeros; i++) {
        zeros += '0';
    }
    string retval = (zeros + s_packet_id + '\0' + s);
    //packet_id += 1;
    //packet_id %= seq_range;
    return retval;
}
