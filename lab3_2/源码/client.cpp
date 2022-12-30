#include"Message.h"
#include<fstream>
#include<math.h>
#include <thread>
#pragma warning(disable:4996)
#pragma comment(lib,"ws2_32.lib")
using namespace std;




SOCKET sockClient;
SOCKADDR_IN addr_client;
SOCKADDR_IN addr_server;

char  ip_client[]="127.0.0.1";
WORD  port_client = 3999;
char ip_server[] = "127.0.0.1";
WORD  port_server = 4000;

char ip_route[] = "127.0.0.1";
WORD  port_route = 4001;

pseudoHead ph;		//α�ײ�

//windows

int N = 10;

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


	addr_client.sin_port = htons(port_client);					//port
	addr_client.sin_addr.S_un.S_addr = inet_addr(ip_client); //ip addr
	addr_client.sin_family = AF_INET;


	addr_server.sin_port = htons(port_route);
	addr_server.sin_addr.S_un.S_addr = inet_addr(ip_route);
	addr_server.sin_family = AF_INET;



	//creat socket
	sockClient = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockClient == INVALID_SOCKET) {
		cout << "socket creat failed" << endl;
		return;
	}

	u_long imode = 1;
	ioctlsocket(sockClient, FIONBIO, &imode);//������

	//bind
	int result_bind;
	result_bind = bind(sockClient, (SOCKADDR*)&addr_client, sizeof(SOCKADDR_IN));
	if (result_bind == SOCKET_ERROR) {
		cout << "bind failed" << endl;
		return;
	}

	//��ʼ��αͷ��
	memset(&ph, 0, sizeof(pseudoHead));
	ph.source_ip = inet_addr(ip_client);
	ph.dest_ip = inet_addr(ip_server);

	cout << "��ʼ���ɹ����ͻ��˿�ʼ����"<<endl;

};



bool establishConnect() {

	int len = sizeof(SOCKADDR_IN);

	char recBuffer[sizeof(msg)];	//���ջ�����
	char sendBuffer[sizeof(msg)];	//���ͻ�����

	memset(recBuffer, 0, sizeof(msg));
	memset(sendBuffer, 0, sizeof(msg));


	msg* sed = (msg*)sendBuffer;		
	msg* rec = (msg*)recBuffer;
	


	setSyn(sed);		//����SYN
	sed->seq_num = 0;	//����seq=0
	sed->ack_num = 0;	//����ack=0
	sed->source_port = port_client;
	sed->dest_port = port_server;
	setChecksum(sed, &ph);	//����У���


	//����
	sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
	

	clock_t start = clock(); //��ʼ��ʱ

	while (recvfrom(sockClient, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len) <= 0) {
		
		
		
		// over time
		if (clock() - start >= MAX_TIME) {
			
			//��ʱ�ش�
			sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
			start = clock();
		}
	}

	cout << "�ͻ���:���յ����ģ�SYN��ACK��" << endl;
	



		//�жϽ���
	if (isAck(rec) && isSyn(rec) && verfiyChecksum(rec, &ph)) {

			cout << "�ͻ���:���ձ��ģ�SYN��ACK����֤��ȷ" << endl;

			memset(sendBuffer, 0, sizeof(msg));
			setAck(sed);	//����ack

			sed->seq_num = 1;
			sed->ack_num = 1;
			sed->source_port = port_client;
			sed->dest_port = port_server;

			setChecksum(sed, &ph);
			sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
			
		}
	else {

			cout << "�ͻ���:���ձ��ģ�SYN��ACK����֤����" << endl;
			
			return 0;
			
		}

	
	cout << "�ͻ��ˣ� �������ӳɹ�" << endl;
		return true;





};


msg make_pkt( int seq, char * data , unsigned short len  ) {

	msg message;
	memset(&message, 0, sizeof(msg));
	message.source_port = port_client;
	message.dest_port = port_server;
	message.length = len;
	message.seq_num = seq;
	memcpy(message.msg, data, len);
	setChecksum(&message, &ph);	//����У���

	return message;
}

int base = 1;
int nextseqnum = 1;
bool start_t = 0;
bool rec_stage = 1;
clock_t start_timer;

DWORD WINAPI RecHandle(LPVOID param) {

	int len = sizeof(SOCKADDR_IN);
	char* recpktBuffer = new char[sizeof(msg)];
	msg* rec = (msg*)recpktBuffer;
	u_long imode = 0;
	ioctlsocket(sockClient, FIONBIO, &imode);//����

	//rec&&notcorrupt(recpkt)
	while (rec_stage) {
	recvfrom(sockClient, recpktBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len);

			if (isAck(rec) && verfiyChecksum(rec, &ph) && (rec->ack_num >= base)) {

				base = rec->ack_num + 1;
				printf("����\tack:\t%d\tACK:\t%d\tlength:\t%d\tchecksum:\t%d\tbase: \t%d\tnextseqnum: \t%d\n", rec->ack_num, isAck(rec), rec->length, rec->checksum, base, nextseqnum);
				//cout << "����\tack:\t" << rec->ack_num << "\tACK:\t" << isAck(rec) << "\tlength:\t" << rec->length << "\tchecksum:\t" << rec->checksum << "\tbase: \t" << base << "\tnextseqnum: \t" << nextseqnum << endl;
				if (base == nextseqnum) {
					start_t = 0;
				}
				else {
					start_t = 1;
					start_timer = clock();

				}
			}
		}
	
	return 1;
}




void GBN_send_FSM(unsigned long length_file ,char * file,char * filename) {

	int packetNUM = int(length_file / Max_Size) + (length_file % Max_Size ? 1 : 0);
	cout << "packetNUM: " << packetNUM << endl;

	int index = 0;

	int len = sizeof(SOCKADDR_IN);
	int	packetDataLen = min(Max_Size, length_file - index * Max_Size);
	char *dataBuffer=new char[Max_Size];
	char *pktBuffer = new char[sizeof(msg)];
	char* recpktBuffer = new char[sizeof(msg)];
	msg* rec = (msg*)recpktBuffer;
	msg sndpkt;
	base=1;
	nextseqnum=1;
	start_t = 0;
	

	bool* first_send_pkt = new bool[packetNUM+1];
	memset(first_send_pkt, 1, packetNUM + 1);
	cout << "�����ļ����ݳ���Ϊ    " << length_file << "Bytes,    ��Ҫ����" << packetNUM << "�����ݰ�" << endl;
	rec_stage = 1;
	HANDLE rechandler = CreateThread(nullptr, 0, RecHandle, nullptr ,0, nullptr);

	while(1){



		if (base == packetNUM+1) {
			CloseHandle(rechandler);
			rec_stage = 0;
			u_long imode = 1;
			ioctlsocket(sockClient, FIONBIO, &imode);//������


			char* sendBuffer = new char[sizeof(msg)];
			memset(sendBuffer, 0, sizeof(msg));
			msg* sed = (msg*)sendBuffer;
			setEnd(sed);
			sed->source_port = port_client;
			sed->dest_port = port_server;
			string fn = filename;
			int filename_len = sizeof(fn);

			memcpy(sed->msg, filename, filename_len);
			sed->length = filename_len;
			setChecksum(sed, &ph);	//����У���

			//����
			sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
			cout << "�ͻ��ˣ� ���ͱ��ģ�END��" << endl;

			clock_t start_timer = clock(); //��ʼ��ʱ

			while (recvfrom(sockClient, recpktBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len) <= 0 || !(isEnd(rec) && isAck(rec))) {
				// over time
				if (clock() - start_timer >= MAX_TIME) {

					//��ʱ�ش�
					sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
					cout << "�ͻ��ˣ� ���ͱ��ģ�END��,�ش�" << endl;
					start_timer = clock();
				}
			}


			if (isEnd(rec) && isAck(rec) && verfiyChecksum(rec, &ph)) {
				cout << "�ͻ��ˣ����շ��������ģ�END��ACK�����ļ��������" << endl;
				return;
			}
			else
				continue;


		}


		//send(data)
		if((nextseqnum<base+N)&&(nextseqnum<=packetNUM)){

			for (int i = nextseqnum; (i < base + N)&&(i<=packetNUM)&&first_send_pkt[i] ; i++) {
				index = i - 1;
				packetDataLen = min(Max_Size, length_file - index * Max_Size);
				memcpy(dataBuffer, file + index * Max_Size, packetDataLen);
				sndpkt = make_pkt(i, dataBuffer, packetDataLen);
				memcpy(pktBuffer, &sndpkt, sizeof(msg));
				sendto(sockClient, pktBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
				printf("����\tseq:\t%d\tindex:\t%d\tlength:\t%d\tchecksum:\t%d\tbase: \t%d\tnextseqnum: \t%d\n", i, index, packetDataLen, sndpkt.checksum, base, nextseqnum);
				//cout << "����\tseq:\t" << i << "\tindex:\t" << index << "\tlength:\t" << packetDataLen << "\tchecksum:\t" << sndpkt.checksum << "\tbase: \t" << base << "\tnextseqnum: \t" << nextseqnum << endl;
				first_send_pkt[i] = 0;
				
				if (base == nextseqnum  ) {
					start_t = 1;
					start_timer = clock();
					
				}
				nextseqnum++;
			}
		}





		//timeout
		if((clock() - start_timer >= MAX_TIME)&&start_t==1){
			start_timer = clock();
			for(int i=base;i <nextseqnum;i++){

			index = i - 1;
			packetDataLen = min(Max_Size, length_file - index * Max_Size);
			memcpy(dataBuffer, file+index* Max_Size, packetDataLen);
			sndpkt = make_pkt(i, dataBuffer, packetDataLen);
			memcpy(pktBuffer, &sndpkt, sizeof(msg));

			sendto(sockClient, pktBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
			printf("����\tseq:\t%d\tindex:\t%d\tlength:\t%d\tchecksum:\t%d\tbase: \t%d\tnextseqnum: \t%d\t(�ش�)\n", i, (i-1), packetDataLen, sndpkt.checksum, base, nextseqnum);
			//cout << "����\tseq:\t" << i << "\tindex:\t" << i-1 << "\tlength:\t" << packetDataLen << "\tchecksum:\t" << sndpkt.checksum << "\tbase: \t" << base << "\tnextseqnum: \t" << nextseqnum <<"\t(�ش�)" << endl;

			}

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



	//��1���ͻ��˷���FIN����
	setFin(sed);		//����Fin
	sed->seq_num = 0;	//����seq=0
	sed->ack_num = 0;	//����ack=0
	sed->source_port = port_client;
	sed->dest_port = port_server;

	setChecksum(sed, &ph);	//����У���

	//����
	sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
	cout << "�ͻ��ˣ����ͱ��ģ�FIN��" << endl;

	clock_t start = clock(); //��ʼ��ʱ


	//��2������ȷ���������˵�FIN��ACK����
	while (recvfrom(sockClient, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len) <= 0) {
		

		
		// over time
		if (clock() - start >= MAX_TIME) {

			//��ʱ�ش�
			sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
			cout << "�ͻ��ˣ����ͱ��ģ�FIN�����ش�" << endl;
			start = clock();
		}
	}

	
	if (isAck(rec) && isFin(rec) && verfiyChecksum(rec, &ph)) {

		cout << "�ͻ��ˣ����ձ��ģ�FIN��ACK����֤��ȷ" << endl;
		
	}
	else {
	
		return false;
	}


	u_long imode = 0;
	ioctlsocket(sockClient, FIONBIO, &imode);//����

	//��3������ȷ���������˵�FIN����

	while (1) {
		recvfrom(sockClient, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len);

		if (isFin(rec) && verfiyChecksum(rec, &ph)) {

			cout << "�ͻ��ˣ����յ����������ģ�FIN������֤��ȷ" << endl;
			break;
		}


	}

	imode = 1;
	ioctlsocket(sockClient, FIONBIO, &imode);//������

	//��4������FIN��ACK����
	cleanflag(sed);
	setFin(sed);
	setAck(sed);
	setChecksum(sed, &ph);	//����У���
	sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
	cout << "�ͻ��ˣ����ͱ��ģ�FIN��ACK��" << endl;

	//�ȴ�2MSL
	
	start = clock(); //��ʼ��ʱ

	while (clock() - start <= 2 * MAX_TIME) {
		//if(clock() - start>= MAX_TIME)
		if (recvfrom(sockClient, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len) > 0&& isFin(rec) && verfiyChecksum(rec, &ph)) {
			sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
			cout << "�ͻ���: ���ͱ��ģ�FIN��ACK��,�ش�" << endl;
		}
	}




	cout << "�ͻ��ˣ����ӹر�" << endl;
	closesocket(sockClient);
	return true;


	

};





int main() {

	
	//��ʼ��
	init();

	//��������
	if (!establishConnect()) {

		cout << "��������ʧ��" << endl;
	}

	cout << endl;
	cout << "�����봰�ڴ�С�� ";
	cin >> N;
	cout << endl;

	while (1) {

		char* filename = new char[100];
		memset(filename, 0, 100);
		string filedir;


		cout << endl;
		cout << "/*****************************************************/" << endl;

		cout << "Ĭ��: 1.jpg" << endl;
		cout << "1: 1.jpg" << endl;
		cout << "2: 2.jpg" << endl;
		cout << "3: 3.jpg" << endl;
		cout << "4: helloworld.txt" << endl;
		cout << "5: �˳�" << endl;
		cout << "������Ҫ������ļ��� " << endl;



		int i;
		cin >> i;
		
		cout << endl;
		cout << "/*****************************************************/" << endl;


		if (i == 5) {
			cout << "������� " << endl;
			break;
		}

		switch (i) {

		case 1:
			//filedir = "C:/Users/nan/Desktop/lab3/lab3_1/test file/1.jpg";
			filedir = "test file/1.jpg";
			memcpy(filename, "1.jpg", sizeof("1.jpg"));
			break;

		case 2:
			//filedir = "C:/Users/nan/Desktop/lab3/lab3_1/test file/2.jpg";
			filedir = "test file/2.jpg";
			memcpy(filename, "2.jpg", sizeof("2.jpg"));
			
			break;
		case 3:

			//filedir = "C:/Users/nan/Desktop/lab3/lab3_1/test file/3.jpg";
			filedir = "test file/3.jpg";
			memcpy(filename, "3.jpg", sizeof("3.jpg"));
			
			break;
		case 4:

			//filedir = "C:/Users/nan/Desktop/lab3/lab3_1/test file/helloworld.txt";
			filedir = "test file/helloworld.txt";
			memcpy(filename, "helloworld.txt", sizeof("helloworld.txt"));
			

			break;
		default:
			//filedir = "C:/Users/nan/Desktop/lab3/lab3_1/test file/1.jpg";
			filedir = "test file/1.jpg";
			memcpy(filename, "1.jpg", sizeof("1.jpg"));
			
			break;
		}


		
		ifstream infile(filedir, ifstream::binary);

		if (!infile.is_open()) {
			cout << "�޷����ļ�" << endl;
			return 0;
		}

		infile.seekg(0, infile.end);
		DWORD fileLen = infile.tellg();
		infile.seekg(0, infile.beg);

		cout<<"�����ļ�����: " << fileLen << endl;

		char* fileBuffer = new char[fileLen];
		infile.read(fileBuffer, fileLen);
		infile.close();

		
		

		cout << "��ʼ�����ļ�: "<< i << endl;

		clock_t start_timer = clock();

		GBN_send_FSM(fileLen, fileBuffer,filename);

		clock_t end_timer = clock();
		double endtime = (double)(end_timer - start_timer) / CLOCKS_PER_SEC;
		cout << "Total time:" << endtime <<" s" << endl;		
		cout << "�����ʣ�" << fileLen * 8 / endtime / 1000 / 1000 << "Mbps" << endl;
		

	}



	closeConnect();

	
	WSACleanup();

	system("pause");

}
