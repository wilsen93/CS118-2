#include <string>
#include <string.h>
#include <errno.h>
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
#include <math.h>
#include "helper.h"
using namespace std;

int main(int argc, char* argv[]){

  	int port = 4000;
  	string fileDirectory = "genericname.txt";
	vector<uint16_t> ack_received;

  	if(argc != 3 && argc != 1){
  	  	cout << "Invalid input" << "\n";
  	  	return -1;
 	}

 	if(argc == 3){
  	  	port = atoi(argv[1]);
  	  	fileDirectory = argv[2];
 	}
	
	ifstream testOpen(fileDirectory, ios::in|ios::binary);
	if(testOpen.good() == false){
		cout << "Invalid file." << "\n";
		return -1;
	}
	testOpen.close();


  	// create a socket using UDP
  	int socketfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
  	if(socketfd == -1){
   	 	cout << "Bad socket" << "\n";
   	 	close(socketfd);
   	 	return -1;
  	}

  	// allow others to reuse the address
  	int yes = 1;
  	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
  	  	cout << "Error setting socket option" << "\n";
  	  	return -1;
  	}

  	// bind address to socket
  	//struct hostent *he = gethostbyname("127.0.0.1");
  	struct sockaddr_in addr;
  	addr.sin_family = AF_INET;
  	addr.sin_port = htons(port);     // short, network byte order
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	//memcpy(&addr.sin_addr,he->h_addr_list[0],he->h_length);
	memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  	if (::bind(socketfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
   	 	cout << "Could not bind to socket" << "\n";
   	 	cout << strerror(errno) << "\n";
  	 	return -2;
  	}
  	
	//Set the RTO to 500 ms
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;

	if(setsockopt(socketfd, SOL_SOCKET,SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) == -1){
		cout << strerror(errno) << "\n";
		return -1;
	}
	
  	struct sockaddr_in clientAddress;
	socklen_t clientAddrSize = sizeof(clientAddress);
	uint16_t CUR_SEQ_NUM = 0;
	uint16_t CUR_ACK_NUM = 0;
	int CWND = 1024;
	int SST = 15360;
	int count = 0;
	int dupCount = 0;
	bool shake3x = true;
	bool recSYN = false;
	char buffer[1032];
	bool SS = true;
	bool sentAll = false;
	packet read;
	packet send;
	vector<packet> sentPackets;

	ifstream file(fileDirectory, ios::in|ios::binary);
	file.seekg(0, file.end);
	int size = file.tellg();
	char filebuffer[1024] = {'\0'};
	file.seekg(0, file.beg);

	while(1){
		memset(buffer,0,sizeof(buffer));
		//THREE WAY HANDSHAKE HERE
		if(shake3x == true){
			int recev = 0;
  			recev = recvfrom(socketfd,buffer,sizeof(buffer),0,(struct sockaddr *)&clientAddress,&clientAddrSize);
			if(recev == -1){
				if(EWOULDBLOCK){
					if(recSYN == true){
						//Sending SYNACK
						uint16_t flags = 0x06;
						packet synACK(CUR_SEQ_NUM,read.getS()+1,CWND,flags,vector<char>());
						vector<char> synACKP = synACK.build();

						if(sendto(socketfd,&synACKP[0],synACKP.size(),0,(struct sockaddr *)&clientAddress, clientAddrSize) == -1){
							cout << strerror(errno) << "\n";
							return -1;
						}
						cout << "Sending packet " << synACK.getS() << " " << CWND << " " << SST << " SYN" << "\n";
					}
					continue;
				}else{
					cout << strerror(errno) << "\n";
					return -1;
				}
			}
			read.init(buffer);
			cout << "Receiving Packet " << read.getA() << "\n";
			ack_received.push_back(read.getA());
			//To print ack received
			for(vector<uint16_t>::iterator i = ack_received.begin(); i != ack_received.end(); i++)
			  cout << ' ' << *i;
			cout << '\n';
			if(true /*read.getA() is present 3 times in ack_received*/)
			  {
			    //Retransmission
			  }

			//Received ACK end threeway handshake
			if(recSYN == true && read.getF(0x04)){
				cout << "END THREEWAY HANDSHAKE" << "\n\n";
				shake3x = false;
				CUR_SEQ_NUM++;

				uint16_t flags = 0x02;
				vector<char> payload;
				unsigned int i = 0;
				unsigned int window = 0;
				//Condition check: if receiver window is < initial cwnd
				if(read.getW() < CWND){
					window = read.getW();
				}else{
					window = CWND;
				}

				//Read File
				int readBytes = 0;
				memset(filebuffer,0,sizeof(filebuffer));

				if(read.getW() > 1024){
					file.read(filebuffer, sizeof(filebuffer));
				}else{
					file.read(filebuffer, read.getW());
				}
				readBytes = file.gcount();
				count += readBytes;

				//Nothing to send begin FIN
				if(readBytes == 0){
					sentAll = true;
					continue;
				}

				while((int)i < readBytes && i < window){
					payload.push_back(filebuffer[i]);
					i++;
				}

				packet payloadPacket(CUR_SEQ_NUM,1,CWND,flags,payload);
				sentPackets.push_back(payloadPacket);
				vector<char> payloadP = payloadPacket.build();
				CUR_SEQ_NUM += payload.size();
				cout << "Sending packet " << payloadPacket.getS() << " " << CWND << " " << SST << "\n";

				if(sendto(socketfd,&payloadP[0],payloadP.size(),0,(struct sockaddr *)&clientAddress, clientAddrSize) == -1){
					cout << strerror(errno) << "\n";
					return -1;
				}
				continue;
			}
			
			//Received SYN
			if(read.getF(0x02) && recSYN == false){
				recSYN = true;
				//Sending SYNACK
				uint16_t flags = 0x06;
				packet synACK(CUR_SEQ_NUM,read.getS() + 1,CWND,flags,vector<char>());
				vector<char> synACKP = synACK.build();

				if(sendto(socketfd,&synACKP[0],synACKP.size(),0,(struct sockaddr *)&clientAddress, clientAddrSize) == -1){
					cout << strerror(errno) << "\n";
					return -1;
				}

				cout << "Sending packet " << synACK.getS() << " " << CWND << " " << SST << " SYN" << "\n";
				continue;
			}
			//incase some if statement fails and we can catch it during debug
			cout << "Something went wrong..." << "\n";
			continue;
		}//THREE WAY HANDSHAKE END HERE
		//SEND THE GOOD STUFF OVER
		//BEGIN SLOWSTART
		if(SS == true && sentAll == false && shake3x == false){

			//IF FIRST DATA PACKET IS LOST
			/*TODO: CONVERT TO TIMEOUT AFTER FIRST DATA PACKET AND IF COMING FROM CONGESTION AVOIDANCE*/
			int recev = 0;
  			recev = recvfrom(socketfd,buffer,sizeof(buffer),0,(struct sockaddr *)&clientAddress,&clientAddrSize);
			if(recev == -1){
				if(EWOULDBLOCK){
					vector<char> payloadP = sentPackets.at(0).build();
					if(sendto(socketfd,&payloadP[0],payloadP.size(),0,(struct sockaddr *)&clientAddress, clientAddrSize) == -1){
						cout << strerror(errno) << "\n";
						return -1;
					}

					cout << "Sending packet " << sentPackets.at(0).getS() << " " << CWND << " " << SST << "\n";
					continue;
				}else{
					cout << strerror(errno) << "\n";
					return -1;
				}
				
			}

			read.init(buffer);
			cout << "Receiving Packet " << read.getA() << "\n";
			ack_received.push_back(read.getA());
			//To print ack received
			for(vector<uint16_t>::iterator i = ack_received.begin(); i != ack_received.end(); i++)
			  cout << ' ' << *i;
			cout << '\n';
		       	if(true /*read.getA() is present 3 times in ack_received*/)
			  {
			    //Retransmission
			  }
			
			bool receivedDuplicate = true;
			//REMOVE DATA PACKET SUCCESSFULLY SENT
			/*TODO: NEED TO REMOVE PACKET FROM DUPLICATE ACK*/
			for(unsigned int i = 0; sentPackets.size(); i++){
				if(read.getA() - sentPackets.at(i).getDSize() == sentPackets.at(i).getS()){
					cout << "Removed packet: " << sentPackets.at(i).getS() << "\n";
					sentPackets.erase(sentPackets.begin() + i);
					receivedDuplicate = false;
					break;
				}
			}

			/*TODO: dupCount == 3 do fast retransmit and return to slow start*/
			if(receivedDuplicate == true){
				dupCount++;
				if(dupCount == 3){
					cout << "Begin fast retransmit" << "\n";
					SS = false;
					continue;
				}
			}else{
				dupCount = 0;
			}
			cout << "Packet Buffer: " << sentPackets.size() << "\n";
			//IF FIRST DATA PACKET IS NOT LOST
			/*TODO: CONVERT FOR PACKETS AFTER FIRST DATA PACKET AND COMING FROM CONGESTION AVOIDANCE*/
			if(CWND < SST){
				CWND += 1024;
				unsigned int window = 0;
				//Condition check: if receiver window is < cwnd
				if(read.getW() < CWND){
					window = read.getW();
				}else{
					window = CWND;
				}
				cout << ceil(double(window)/(double)(1024)) << " Allowed to send this many packets: " << ceil(double(window)/(double)(1024)) - sentPackets.size() << "\n";
				int temp =  ceil(double(window)/(double)(1024)) - sentPackets.size();
				//Send out the packets
				for(int i = 0; i < temp; i++){
					//Read File
					int readBytes = 0;
					memset(filebuffer,0,sizeof(filebuffer));
					if(read.getW() > 1024){
						file.read(filebuffer, sizeof(filebuffer));
					}else{
						file.read(filebuffer, read.getW());
					}
					readBytes = file.gcount();
					count += readBytes;

					//Nothing left to send begin FIN
					if(readBytes == 0){
						sentAll = true;
						continue;
					}
					uint16_t flags = 0x02;
					vector<char> payload;
					unsigned int j = 0;
					while(j < window && (int)j < readBytes){
						payload.push_back(filebuffer[j]);
						j++;	
					}
				
					packet payloadPacket(CUR_SEQ_NUM,1,1024,flags,payload);
					sentPackets.push_back(payloadPacket);
					vector<char> payloadP = payloadPacket.build();
					if(sendto(socketfd,&payloadP[0],payloadP.size(),0,(struct sockaddr *)&clientAddress, clientAddrSize) == -1){
						cout << strerror(errno) << "\n";
						return -1;
					}

					CUR_SEQ_NUM += payload.size();
					cout << "Sending packet " << payloadPacket.getS() << " " << CWND << " " << SST << "\n";	
				}
				continue;
				//Wait for response from the recvfrom within the slow start function
			}else{
				//END CWND < SSTs
				SS = false;
				cout << "SST BROKEN" << "\n";
				continue;
			}
			
		}
		
		//BEGIN CONGESTION AVOIDANCE
		if(SS == false && sentAll == false && shake3x == false){
			
		}
		//DRAIN OUT THE REMAINING PACKETS SUCCESSFULLY SENT --- DEBUGGING PURPOSE ONLY
		if(sentPackets.size() != 0){
			int recev = 0;
  			recev = recvfrom(socketfd,buffer,sizeof(buffer),0,(struct sockaddr *)&clientAddress,&clientAddrSize);
			if(recev == -1){
				if(EWOULDBLOCK){
					vector<char> payloadP = sentPackets.at(0).build();
					if(sendto(socketfd,&payloadP[0],payloadP.size(),0,(struct sockaddr *)&clientAddress, clientAddrSize) == -1){
						cout << strerror(errno) << "\n";
						return -1;
					}

					cout << "Sending packet " << sentPackets.at(0).getS() << " " << CWND << " " << SST << "\n";
					continue;
				}else{
					cout << strerror(errno) << "\n";
					return -1;
				}
				
			}

			read.init(buffer);
			cout << "Receiving Packet " << read.getA() << "\n";
			ack_received.push_back(read.getA());
			//Print ack received
			for(vector<uint16_t>::iterator i = ack_received.begin(); i != ack_received.end(); i++)
			  cout << ' ' << *i;
			cout << '\n';
			if(true /*read.getA() is present 3 times in ack_received*/)
			  {
			    //Retransmission
			  }
			
			//REMOVE DATA PACKET SUCCESSFULLY SENT
			for(unsigned int i = 0; sentPackets.size(); i++){
				if(read.getA() - sentPackets.at(i).getDSize() == sentPackets.at(i).getS()){
					cout << "Removed packet: " << sentPackets.at(i).getS() << "\n";
					sentPackets.erase(sentPackets.begin() + i);
					break;
				}
			}	
			cout << "Packet Buffer: " << sentPackets.size() << "\n";
		}//END DRAINING HERE

		//BEGIN FIN PROCEDURES HERE
		if(sentAll == true && sentPackets.size() == 0){
			cout << "Close here" << "\n";
			break;
		}
	}
	/*char buffer[512];

  	while(1){
  		memset(buffer,0,sizeof(buffer));
  		int recev = 0;
  		if((recev = recvfrom(socketfd,buffer,sizeof(buffer),0,(struct sockaddr *)&clientAddr,&clientAddrSize)) == -1){
			cout << "error" << "\n";
			return -1;
		}
		cout << buffer << "\n";
  	}*/

  	////////////////////////////////

	return 0;
}
