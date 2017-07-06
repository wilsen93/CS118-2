#include "helper.h"
packet::packet(uint16_t s, uint16_t a, uint16_t w, uint16_t f, vector<char> d){
  	seq = s;
  	ack = a;
  	window = w;
  	flags = f;
  	data = d;
}

packet::packet(){

}

uint16_t packet::getS(){
  	return seq;
}

uint16_t packet::getA(){
  	return ack;
}

uint16_t packet::getW(){
  	return window;
}

bool packet::getF(uint16_t want){
	return flags & want;
}

string packet::getData(){
	string ret;
	vector<char>::iterator i;

	for(i = data.begin(); i != data.end(); i++){
		ret += *i;
	}
	return ret;
}

unsigned int packet::getDSize(){
	return data.size();
}

vector<char> packet::build(){
  	vector<char> ret;
  	vector<char>::iterator i;
  	
  	uint16_t s = htons(seq);
  	uint16_t a = htons(ack);
  	uint16_t w = htons(window);
	uint16_t f = htons(flags);

  	ret.push_back((char)(s >> 8));
  	ret.push_back((char) s); 
  	ret.push_back((char)(a >> 8));
  	ret.push_back((char) a);
  	ret.push_back((char)(w >> 8));
  	ret.push_back((char) w);
	ret.push_back((char)(f >> 8));
  	ret.push_back((char) f);

  	for(i = data.begin(); i != data.end(); i++){
     		ret.push_back(*i);
    	}
  	return ret;

}

void packet::init(char incpacket[]){
	seq = ntohs(incpacket[0] << 8 | incpacket[1]);
	ack = ntohs(incpacket[2] << 8 | incpacket[3]);
	window = ntohs(incpacket[4] << 8 | incpacket[5]);
	flags = ntohs(incpacket[6] << 8 | incpacket[7]);

	data.clear();
	unsigned int i = 8;
	while(incpacket[i] != '\0'){
		data.push_back(incpacket[i]);
		i++;
	}

	
}

