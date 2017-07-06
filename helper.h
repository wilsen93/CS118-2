#include <string>
#include <vector>
#include <stdint.h>
#include <arpa/inet.h>
#include <iostream>
using namespace std;

class packet {
public:
  	packet(uint16_t s, uint16_t a, uint16_t w, uint16_t f, vector<char> d);
	packet(char incpacket[]);
	packet();
	
  	uint16_t getS();
  	uint16_t getA();
  	uint16_t getW();
  	bool getF(uint16_t want);

  	string getData();
  	unsigned int getDSize();
	vector<char> build();
	void init(char incpacket[]);
	
private:
  	uint16_t seq;
  	uint16_t ack;
  	uint16_t window;
  	uint16_t flags;
  	vector<char> data;
};
