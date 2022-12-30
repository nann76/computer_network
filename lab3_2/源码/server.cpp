#include"Message.h"
#include<fstream>
#pragma warning(disable:4996)
#pragma comment(lib,"ws2_32.lib")
using namespace std;

SOCKET sockServer;
SOCKADDR_IN addr_client;
SOCKADDR_IN addr_server;

char  ip_client[] = "127.0.0.1";
WORD  port_client = 3999;
char ip_server[] = "127.0.0.1";
WORD  port_server = 4000;

char ip_route[] = "127.0.0.1";
WORD  port_route = 4001;


pseudoHead ph;		//α�ײ�

void init() {


	//Startup
	WSADATA wsadata;
	WORD version;
	version = MAKEWORD(2, 2);
	int result_start;
	result_start = WSAStartup(version, &wsadata);

	if (result_start != 0) {
		cout << "Startup failed" << endl;
		return;
	}

	//client and server  addr


	addr_client.sin_port = htons(port_route);					//port
	addr_client.sin_addr.S_un.S_addr = inet_addr(ip_route); //ip addr
	addr_client.sin_family = AF_INET;


	addr_server.sin_port = htons(port_server);
	addr_server.sin_addr.S_un.S_addr = inet_addr(ip_server);
	addr_server.sin_family = AF_INET;



	//creat socket
	sockServer = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockServer == INVALID_SOCKET) {
		cout << "socket creat failed" << endl;
		return;
	}

	

	//bind
	int result_bind;
	result_bind = bind(sockServer, (SOCKADDR*)&addr_server, sizeof(SOCKADDR_IN));
	if (result_bind == SOCKET_ERROR) {
		cout << "bind failed" << endl;
		return;
	}

	//��ʼ��α�ײ�
	memset(&ph, 0, sizeof(pseudoHead));
	ph.source_ip = inet_addr(ip_client);
	ph.dest_ip = inet_addr(ip_server);

	cout << "��ʼ���ɹ����������˿�ʼ����" << endl;

}




bool establishConnect() {

	int len = sizeof(SOCKADDR_IN);

	char recBuffer[sizeof(msg)];	//���ջ�����
	char sendBuffer[sizeof(msg)];	//���ͻ�����

	msg* sed = (msg*)sendBuffer;
	msg* rec = (msg*)recBuffer;

	memset(recBuffer, 0, sizeof(msg));
	memset(sendBuffer, 0, sizeof(msg));



	
	while (1) {


		//����������SYN
		recvfrom(sockServer, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, &len);

		if (isSyn(rec) && verfiyChecksum(rec, &ph) && rec->seq_num==0) {
		
			
			cout << "�������ˣ� ���յ��ͻ���SYN���ģ���֤�ɹ�" << endl;

			//����SYN��ACK����
			setAck(sed);
			setSyn(sed);
			sed->seq_num = 0;
			sed->ack_num = 1;
			sed->source_port = port_server;
			sed->dest_port = port_client;
			setChecksum(sed, &ph);

			//����SYN��ACK
			sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
			break;
		}
		else {
			
			cout << "�������ˣ� ���յ��ͻ���SYN���ģ���֤ʧ��" << endl;
			continue;
		
		}

	}

	//��Ϊ������
	u_long imode = 1;
	ioctlsocket(sockServer, FIONBIO, &imode);//������


	clock_t start = clock(); //��ʼ��ʱ
	while (recvfrom(sockServer, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, &len) <= 0) {
		


		
		// over time
		if (clock() - start >= MAX_TIME) {

			//��ʱ�ش�
			sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
			cout << "�������ˣ� �ش����ģ�SYN��ACK��" << endl;
			start = clock();
		}
	}


	if (isAck(rec) && verfiyChecksum(rec, &ph)) {

		cout << "�������ˣ����յ��ͻ��˱��ģ�SYN��ACK����֤��ȷ" << endl;
		

	}
	else {
	
		return false;
	}



	imode = 0;
	ioctlsocket(sockServer, FIONBIO, &imode);//����
	return true;



}




msg make_pkt(int ack) {

	msg message;
	memset(&message, 0, sizeof(msg));
	message.source_port = port_client;
	message.dest_port = port_server;
	setAck(&message);
	message.ack_num = ack;
	setChecksum(&message, &ph);	//����У���

	return message;
}



DWORD GBN_receive_FSM(char* file,char *filename) {



	int len = sizeof(SOCKADDR_IN);

	char* pktBuffer = new char[sizeof(msg)];
	char* recpktBuffer = new char[sizeof(msg)];
	char* sendBuffer = new char[sizeof(msg)];

	DWORD rec_data_len = 0;
	msg* rec = (msg*)recpktBuffer;

	

	u_long imode = 0;
	if(ioctlsocket(sockServer, FIONBIO, &imode)==SOCKET_ERROR)
	cout << "error" << endl;


	int expectedseqnum=1;


	
	while(1){


			recvfrom(sockServer, recpktBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, &len);

			if (isEnd(rec)) {

				memcpy(filename, rec->msg, rec->length);
				cout << "�������" << endl;
				memset(sendBuffer, 0, sizeof(msg));
				msg sed = make_pkt(0);
				setEnd(&sed);
				sed.checksum = 0;
				setChecksum(&sed, &ph);	

				memcpy(sendBuffer, &sed, sizeof(msg));
				
				sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
				cout << "������: ���ͱ��ģ�END��ACK��" << endl;
				return rec_data_len;
			}

			if (rec->seq_num != expectedseqnum) {
				msg sedpkt = make_pkt(expectedseqnum-1);
				memcpy(sendBuffer, &sedpkt, sizeof(msg));
				sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
				cout << "����\tack:\t" << expectedseqnum-1 << "\tACK:\t " << isAck(&sedpkt) << "\tlength:\t" << sedpkt.length << "\tchecksum:\t" << sedpkt.checksum << "\texpectedseqnum: \t" << expectedseqnum <<"ʧ��" << endl;
			}

			else if (rec->seq_num ==expectedseqnum  && (verfiyChecksum(rec, &ph))) {

				msg sedpkt = make_pkt(expectedseqnum);
				memcpy(sendBuffer, &sedpkt, sizeof(msg));
				sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
				expectedseqnum++;
				cout << "����\tack:\t"<< rec->seq_num <<"\tACK:\t "<< isAck(&sedpkt) << "\tlength:\t" << sedpkt.length << "\tchecksum:\t" << sedpkt.checksum << "\texpectedseqnum: \t" << expectedseqnum << endl;
				memcpy(file + rec_data_len, rec->msg, rec->length);
				rec_data_len += rec->length;
			   
				

			}


			

	}


	
}



bool closeConnect() {

	int len = sizeof(SOCKADDR_IN);
	char recBuffer[sizeof(msg)];	//���ջ�����
	char sendBuffer[sizeof(msg)];	//���ͻ�����
	memset(recBuffer, 0, sizeof(msg));
	memset(sendBuffer, 0, sizeof(msg));

	msg* sed = (msg*)sendBuffer;
	msg* rec = (msg*)recBuffer;



	u_long imode = 0;
	ioctlsocket(sockServer, FIONBIO, &imode);//����


	//��1��������֤�ͻ��˵�FIN���ġ�
	while (1) {
		recvfrom(sockServer, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len);

		if (isFin(rec) && verfiyChecksum(rec, &ph)) {

			cout << "�������� �յ��ͻ���Fin������֤��ȷ" << endl;
			break;
		}

	}


	//��2�������������÷���ACK��FIN����

	setFin(sed);		//����Fin
	setAck(sed);		//����Ack
	sed->seq_num = 0;	//����seq=0
	sed->ack_num = 0;	//����ack=0
	sed->source_port = port_client;
	sed->dest_port = port_server;
	setChecksum(sed, &ph);	//����У���

	//����
	sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);




	//ʣ������

	//����FIN
	cleanflag(sed);
	setFin(sed);		//����Fin
	setChecksum(sed, &ph);
	sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);


	imode = 1;
	ioctlsocket(sockServer, FIONBIO, &imode);//������

	//����
	clock_t  start = clock(); //��ʼ��ʱ
	while (recvfrom(sockServer, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len) <= 0 ||!(isAck(rec) && isFin(rec) && verfiyChecksum(rec, &ph)) ) {

		// over time
		if (clock() - start >= MAX_TIME) {

			//��ʱ�ش�
			sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
			cout << "�������� ���ͱ��ģ�FIN�����ش�" << endl;
			start = clock();
		}
	}

	if (isAck(rec) && isFin(rec) && verfiyChecksum(rec, &ph)) {

		cout << "�������� ���յ����ģ�FIN��ACK������֤��ȷ" << endl;

	}
	else {
	
		return false;
	}


	cout << "���ӹر�" << endl;
	closesocket(sockServer);
	return true;




};











int main() {

	init();
	if (establishConnect()) {

		cout << "����ˣ� �������ӳɹ�" << endl;
	}


	cout << endl;
	cout << "/*****************************************************/" << endl;

	//�����ļ��Ļ�����
	
	bool tran = 1;

	while (tran) {
		char* fileBuffer = new char[90000000];

		DWORD fileLength = 0;
		char* filename = new char[100];
		memset(filename, 0, 100);

	
		fileLength = GBN_receive_FSM(fileBuffer, filename);



		
		cout << "/*****************************************************/" << endl;
		cout << endl;
		//string dir = "C:/Users/nan/Desktop/lab3/lab3_1/rec file/";
		string dir = "rec file/";
		string  fn = filename;
		string filenm = dir + fn;


		//д�븴���ļ�
		ofstream outfile(filenm, ios::binary);
		//if (!outfile.is_open()) {
		//	cout << "���ļ�����" << endl;
		//	return 0;
		//}
		//cout << filenm << endl;
		outfile.write(fileBuffer, fileLength);
		outfile.close();


		cout << "�Ƿ�������ܴ��䣨Y/N���� ";
		char i;
		cin >> i;

		cout << "/*****************************************************/" << endl;
		cout << endl;
		switch (i)
		{
		case 'y':
			tran = 1;
			break;
		case 'n':
			tran = 0;
			break;

		default:
			break;
		}

	}
		closeConnect();
		WSACleanup();
		system("pause");

}