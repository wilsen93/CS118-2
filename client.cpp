#include <string>
#include <string.h>
#include <thread>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <fstream>
#include <sys/fcntl.h>
#include <stdint.h>
#include "helper.h"
using namespace std;

int main(int argc, char* argv[]){ 
  	string host = "10.0.0.1";
  	string port = "4000";
	vector<uint16_t> seq_received;

  	if(argc != 3 && argc != 1){
    		cout << "Invalid input" << "\n";
    		return -1;
  	}

  	if(argc == 3){
    		host = argv[1];
    		port = argv[2];
  	}

  	int socketfd;
  	struct addrinfo hints, *servinfo, *p;

	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if(getaddrinfo(host.c_str(),port.c_str(),&hints,&servinfo) != 0){
		cout << strerror(errno) << "\n";
		return -1;	
	}
  	
  	sockaddr_in *ip;
  	struct sockaddr serverAddress;
	for(p = servinfo; p != NULL; p = p->ai_next){
		ip = (sockaddr_in *)p->ai_addr;
		if((socketfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
			continue;
		} 
		break;
	}

	if(p == NULL){
		cout << "Failed to connect: " << host << "\n";
		return -1;
	}
	
	memcpy(&serverAddress,p->ai_addr,p->ai_addrlen);
	socklen_t servAddrSize = sizeof(serverAddress);
	//Set the RTO to 500 ms
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;

	if(setsockopt(socketfd, SOL_SOCKET,SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) == -1){
		cout << strerror(errno) << "\n";
		return -1;
	}
	/*string message = "hi";
	int val = 0;
	val = sendto(socketfd,message.c_str(),message.length(),0,&serverAddress, sizeof(serverAddress));
	cout << strerror(errno) << "\n";
	cout << inet_ntoa((in_addr)ip->sin_addr) << " hi" << "\n";*/

	uint16_t CUR_SEQ_NUM = 0;
	uint16_t CUR_ACK_NUM = 0;
	int BWND = 15360;
	//int BWND = 1000;
	bool shake3x = true;
	bool recSYNACK = false;
	char buffer[1032];
	packet read;
	packet ack;
	ofstream file;
	file.open("received.data" , ios::out|ios::binary);
	//int testCounter = 0;
	vector<uint16_t> receivedSeq();
	//Construct first shake packet and send it out
	uint16_t flags = 0x02;
	packet initial(CUR_SEQ_NUM,CUR_ACK_NUM,0,flags,vector<char>());
	vector<char> initialPacket = initial.build();
				
	if(sendto(socketfd,&initialPacket[0],initialPacket.size(),0,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
		cout << strerror(errno) << "\n";
		return -1;
	}
	cout << "Sending packet " << initial.getS() << " SYN" << "\n";

	while(1){
		memset(buffer,0,sizeof(buffer));
		//THREE WAY HANDSHAKE HERE
		if(shake3x == true){
			int recev = 0;
  			recev = recvfrom(socketfd,buffer,sizeof(buffer),0,(struct sockaddr *)&serverAddress,&servAddrSize);
			if(recev == -1){
				if(EWOULDBLOCK){
					if(recSYNACK == false){
						//Resend first shake packet
						uint16_t flags = 0x02;
						packet initial(CUR_SEQ_NUM,CUR_ACK_NUM,0,flags,vector<char>());
						vector<char> initialPacket = initial.build();
				
						if(sendto(socketfd,&initialPacket[0],initialPacket.size(),0,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
							cout << strerror(errno) << "\n";
							return -1;
						}
						cout << "Sending packet " << initial.getS() << " SYN" << "\n";
					}else if(recSYNACK == true){
						//Resend ACK
						uint16_t flags = 0x04;
						packet synACK(CUR_SEQ_NUM,read.getS() + 1,BWND,flags,vector<char>());
						vector<char> synACKP = synACK.build();

						if(sendto(socketfd,&synACKP[0],synACKP.size(),0,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
							cout << strerror(errno) << "\n";
							return -1;
						}

						cout << "Sending packet " << synACK.getS() << "\n";
					}
					continue;
				}else{
					cout << strerror(errno) << "\n";
					return -1;
				}
			}
			read.init(buffer);
			cout << "Receiving Packet " << read.getS() << "\n";
			seq_received.push_back(read.getS());
			for(vector<uint16_t>::iterator i = seq_received.begin(); i != seq_received.end(); i++)
			  cout << ' ' << *i;
			cout << '\n';

			//Received First Legitimate Packet
			if(read.getF(0x02) && read.getData().empty() == false && recSYNACK == true){
				cout << "END THREEWAY HANDSHAKE" << "\n\n";
				shake3x = false;
				file.write(read.getData().c_str(),read.getData().length());
				file.flush();
				//cout << read.getData() << "\n";

				uint16_t flags = 0x02;
				CUR_ACK_NUM += read.getData().length();
				packet ACK(CUR_SEQ_NUM,1 + CUR_ACK_NUM,BWND,flags,vector<char>());
				ack = ACK;
				vector<char> ACKP = ACK.build();

				if(sendto(socketfd,&ACKP[0],ACKP.size(),0,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
					cout << strerror(errno) << "\n";
					return -1;
				}

				cout << "Sending packet " << ACK.getA() << "\n";
				continue;
			}

			//Received SYNACK
			if(read.getF(0x06) && recSYNACK == false){
				CUR_SEQ_NUM++;
				recSYNACK = true;

				//Sending ACK
				uint16_t flags = 0x04;
				packet synACK(CUR_SEQ_NUM,read.getS() + 1,BWND,flags,vector<char>());
				vector<char> synACKP = synACK.build();

				if(sendto(socketfd,&synACKP[0],synACKP.size(),0,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
					cout << strerror(errno) << "\n";
					return -1;
				}

				cout << "Sending packet " << synACK.getS() << "\n";
				continue;
			}
			//incase some if statement fails we can catch it during debug
			cout << "Something went wrong..." << "\n";
			continue;
		}//END THREEWAY HANDSHAKE HERE
		//TEST THE GOODIES
		/*TODO: REMEMBER WHICH PACKETS CAME AND TO SEND LAST ACCUMALATIVE ACK ON FAIL*/
		int recev = 0;
  		recev = recvfrom(socketfd,buffer,sizeof(buffer),0,(struct sockaddr *)&serverAddress,&servAddrSize);
		if(recev == -1){
			if(EWOULDBLOCK){
				vector<char> ACKP = ack.build();
				if(sendto(socketfd,&ACKP[0],ACKP.size(),0,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
					cout << strerror(errno) << "\n";
					return -1;
				}
				cout << "Sending packet " << ack.getA() << "\n";
				
			}
			continue;
		}
		read.init(buffer);
		cout << "Receiving Packet " << read.getS() << "\n";
		seq_received.push_back(read.getS());
		//To print received packets
		for(vector<uint16_t>::iterator i = seq_received.begin(); i != seq_received.end(); i++)
		  cout << ' ' << *i;
		cout << '\n';
		
		file.write(read.getData().c_str(),read.getData().length());
		file.flush();

		uint16_t flags = 0x02;
		CUR_ACK_NUM += read.getData().length();
		packet ACK(CUR_SEQ_NUM,1 + CUR_ACK_NUM,BWND,flags,vector<char>());
		ack = ACK;
		vector<char> ACKP = ACK.build();

		if(sendto(socketfd,&ACKP[0],ACKP.size(),0,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
			cout << strerror(errno) << "\n";
			return -1;
		}

		cout << "Sending packet " << ACK.getA() << "\n";
		//testCounter++;
	}
	file.close();
  	close(socketfd);
  	return 0;
}
