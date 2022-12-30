#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <iostream>
#include<WinSock2.h>
#include<time.h>
#include<stdio.h>
#include<string>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#define  Max_Size 8192
#define MAX_TIME 500




typedef struct MyStruct
{

	WORD source_port = 0, dest_port = 0;	//port
	DWORD seq_num = 0, ack_num = 0;			//seq,ack
	WORD  length = 0;					//length
	BYTE   flag = 0;				     //flag
	WORD checksum = 0;					//checksum
	char msg[Max_Size] = { 0 };



}msg;


typedef struct MyStruct2 {

	DWORD source_ip = 0, dest_ip = 0;		//ip
	char zero = 0;
	char protocol = 0;
	WORD length = sizeof(msg);

}pseudoHead;

void setlength(msg* msg, short int len) {

	msg->length = len & 0x0004;

}

short int getlength(msg& msg) {

	return msg.length;

}





//ACK=0x01, SYN=0x02, FIN=0x04, END=0x08

void cleanflag(msg* msg) {
	msg->flag |= 0x0;

}

void setAck(msg* msg) {

	msg->flag |= 0x01;
}

bool isAck(msg* msg) {


	return msg->flag & 0x01;
}


void setSyn(msg* msg) {

	msg->flag |= 0x02;
}

bool isSyn(msg* msg) {


	return msg->flag & 0x02;
}

void setFin(msg* msg) {

	msg->flag |= 0x04;
}

bool isFin(msg* msg) {


	return msg->flag & 0x04;
}

void setEnd(msg* msg) {

	msg->flag |= 0x08;
}

bool isEnd(msg* msg) {


	return msg->flag & 0x08;
}



void setMsg(msg* msg, char* data) {

	memcpy(msg->msg, data, Max_Size);

};



void setChecksum(msg* message, pseudoHead* ph) {

	//��Ϊ0
	message->checksum = 0;

	int sum = 0;

	int len_pseudo = sizeof(pseudoHead);
	int len_msg = sizeof(msg);

	for (int i = 0; i < len_pseudo / 2; i++) {
		sum += ((WORD*)ph)[i];
	}

	for (int i = 0; i < len_msg / 2; i++) {
		sum += ((WORD*)message)[i];
	}

	while (sum >> 16) {

		sum = (sum & 0xffff) + (sum >> 16);

	}
	message->checksum = ~sum;

};



bool verfiyChecksum(msg* message, pseudoHead* ph) {

	int sum = 0;

	int len_pseudo = sizeof(pseudoHead);
	int len_msg = sizeof(msg);

	for (int i = 0; i < len_pseudo / 2; i++) {
		sum += ((WORD*)ph)[i];
	}

	for (int i = 0; i < len_msg / 2; i++) {
		sum += ((WORD*)message)[i];
	}

	while (sum >> 16) {

		sum = (sum & 0xffff) + (sum >> 16);

	}

	return sum == 0xffff;

};


#endif // !__MESSAGE_H__

