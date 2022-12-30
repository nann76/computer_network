#include<iostream>
#include<WinSock2.h>
#include<time.h>
#include<thread>
#include<stdio.h>
#include<string>
#pragma warning(disable:4996)
#pragma comment(lib,"ws2_32.lib")
using namespace std;

char name[10];
char recname[10];
const int MaxBufSize = 1024;
SOCKET sockClient;

//���ĸ�ʽ
typedef struct MESSAGE {
	char name[10];
	char recname[10];
	char time[64];
	char message[MaxBufSize - 10-10 - 64];

}myMSG;


bool sendflag = 1;
bool recflag = 1;
void sendMsg() {

	//������
	char SendBuf[MaxBufSize];
	memset(SendBuf, 0, MaxBufSize);
	while (sendflag)
	{

		memset(SendBuf, 0, MaxBufSize);
		cout << "�����뷢����Ϣ:" << endl;
		myMSG* sendMSG = (myMSG*)SendBuf;

		memcpy(sendMSG->name, name, 10);
		memcpy(sendMSG->recname, recname, 10);

		time_t t = time(0);
		strftime(sendMSG->time, 64, "%Y/%m/%d %X %A ", localtime(&t));


		scanf_s("%s", sendMSG->message, MaxBufSize - 10-10 - 64);

		//��������
		send(sockClient, SendBuf, MaxBufSize, 0);
		if (strcmp(sendMSG->message, "exit") == 0) {
			sendflag = 0;
			recflag = 0;
			break;
		}
		
	}


}


void recMsg() {

	//������
	char RecvBuf[MaxBufSize];
	memset(RecvBuf, 0, MaxBufSize);
	while (recflag)
	{
		int n;
		memset(RecvBuf, 0, MaxBufSize);
		if (recv(sockClient, RecvBuf, MaxBufSize, 0) > 0) {


			myMSG* receMSG = (myMSG*)RecvBuf;
			cout << endl;
			cout << "/*****************************************/" << endl;
			cout << endl;
			cout << "name: " << receMSG->name << endl
				<< "time :" << receMSG->time << endl
				<< "receive message :" << receMSG->message << endl;
			cout << endl;
			cout << "/*****************************************/" << endl;
			cout << endl;



			cout << "�����뷢����Ϣ:" << endl;
		}
		else
			break;
		

	}

}


WSADATA wsadata;
WORD version;
SOCKADDR_IN addr;
SOCKADDR_IN addr_server;


void main() {

	//Startup
	version = MAKEWORD(2, 2);
	int result_start;
	result_start = WSAStartup(version, &wsadata);

	if (result_start != 0) {
		cout << "Startup failed" << endl;
		return;//����
	}

	//�����׽���
	sockClient = socket(AF_INET, SOCK_STREAM, 0);
	if (sockClient == INVALID_SOCKET) {
		cout << "�׽��ִ���ʧ��" << endl;
		return;
	}


	//�󶨵�ַ

	int cport;
	cout << "������ͻ��˶˿ںţ� " ;
	cin >> cport;		
	addr.sin_port = htons(cport);					//�˿�

	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); //ip��ַ
	addr.sin_family = AF_INET;							//Э���

	int result_bind;
	result_bind = bind(sockClient, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN));
	if (result_bind == SOCKET_ERROR) {
		cout << "bindʧ��" << endl;
		return;
	}

	//connect
	//������ip���˿ں�����
	addr_server.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	addr_server.sin_port = htons(888);
	addr_server.sin_family = AF_INET;

	int result_connect=1;
	result_connect=  connect(sockClient, (SOCKADDR*)&addr_server, sizeof(SOCKADDR_IN));

	if (result_connect == 0) {
		cout << "���ӷ������ɹ�" << endl;
	}
	
	cout << "��ǰ�ͻ�����Ϣ��" << endl << "ip: " << "127.0.0.1" << endl << "�˿ں�: " << cport << endl;


	//�û���Ϣ����
	cout << "�����������ƣ� " ;
	cout << endl;
	scanf_s("%s", name, 10);
	cout << "������շ����ƣ� " ;
	cout << endl;
	scanf_s("%s", recname, 10);
	


	//��server���ͳ�ʼ����Ϣ
	char SendBuf[MaxBufSize];
	memset(SendBuf, 0, MaxBufSize);
	memset(SendBuf, 0, 1024);
	myMSG* sendMSG = (myMSG*)SendBuf;
	time_t t = time(0);
	strftime(sendMSG->time, 64, "%Y/%m/%d %X %A ", localtime(&t));
	memcpy(sendMSG->name, name, 10);
	memcpy(sendMSG->recname, recname, 10);
	memcpy(sendMSG->message, "init", 5);
	send(sockClient, SendBuf, MaxBufSize, 0);


	thread first(sendMsg);     // �����̣߳����ã�sends()
	thread second(recMsg);  // �����̣߳����ã�recvs()
	first.detach();
	second.detach();

	while (1) {}

	closesocket(sockClient);
	WSACleanup();


}