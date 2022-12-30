#include<iostream>
#include<WinSock2.h>
#include<time.h>
#include<thread>
#include<stdio.h>
#include<string>

#pragma warning(disable:4996)
#pragma comment(lib,"ws2_32.lib")
using namespace std;


SOCKET sockClient;
const int maxClientNum = 5;	//�����������
int OnConnectNum = 0;			//��ǰ��������
const int MaxBufSize = 1024;	//���Ĵ�������ֽ���
SOCKADDR_IN addrC[maxClientNum];

//���ĸ�ʽ
typedef struct MESSAGE {
	char name[10];
	char recname[10];
	char time[64];
	char message[MaxBufSize-10-10-64];

}myMSG;

//�洢�û�����socket�ļ�ֵ�Խṹ
typedef struct clientMap {
	char name[10];
	SOCKET* sockClient;

}CM;

CM ClientMap[maxClientNum];


WSADATA wsadata;
WORD version;

SOCKET serverSocket;
SOCKADDR_IN addr;

//�����߳�
DWORD WINAPI SeverThread(LPVOID lpParameter)
{
	//�½�һ��SOCKET����ͨ��
	SOCKET* ClientSocket = (SOCKET*)lpParameter;
	bool init = 1;

	//������
	char RecvBuf[MaxBufSize];
	char SendBuf[MaxBufSize];
	
	//��ʼ������ת��
	while (1)
	{
		memset(RecvBuf, 0, 1024);

		if (recv(*ClientSocket, RecvBuf, sizeof(RecvBuf), 0) > 0)
		{ 

			myMSG* receMSG=(myMSG*)RecvBuf;

			//��ͻ��˳�ʼ����Ϣ
			if (strcmp(receMSG->message, "init") == 0&&init==1) {
				memcpy(ClientMap[OnConnectNum - 1].name, receMSG->name, 10);
				ClientMap[OnConnectNum - 1].sockClient= ClientSocket;

				cout << endl;
				cout << "/*********************************************/" << endl;
				cout << endl;
				cout << "�û��� " << receMSG->name << " ��ɳ�ʼ��";
				cout << endl;
				cout << "from:            " << receMSG->name << endl
					<< "to:              " << receMSG->recname << endl
					<< "time:            " << receMSG->time << endl
					<< "receive message: " << receMSG->message << endl;
				cout << endl;
				cout << "/*********************************************/" << endl;
				cout << endl;
				init = 0;
				continue;

			}




			//�ǳ�ʼ������ת����Ϣ
			if (init != 1) {

				SOCKET* ToSOCKET=NULL;
				bool find = 0;

				//��map��Ѱ���������
				for (int i = 0; i < maxClientNum; i++) {
					if (strcmp(receMSG->recname, ClientMap[i].name) == 0) {
						ToSOCKET = ClientMap[i].sockClient;
						find = 1;
						break;
					}
				}

				//�����û�����
				if (find == 1) {


					//�ͻ����˳�
					if (strcmp(receMSG->message, "exit") == 0) {

						cout << endl;
						cout << "/*********************************************/" << endl;
						cout << endl;
						cout << "from:            " << receMSG->name << endl
							<< "time:            " << receMSG->time << endl
							<< "�ͻ����˳�" << endl;
						//<< "receive message: " << receMSG->message << endl;
						cout << endl;
						cout << "/*********************************************/" << endl;
						cout << endl;

						//����������ͶԷ��˳�����Ϣ
						memset(SendBuf, 0, 1024);
						memcpy(SendBuf, RecvBuf, 1024);
						memcpy(SendBuf+84, "�Է����˳�", sizeof("�Է����˳�"));
						int k = 0;
						k = send(*ToSOCKET, SendBuf, sizeof(SendBuf), 0);
						//�ر��뵱ǰ�û���socket
						OnConnectNum--;
						closesocket(*ClientSocket);
						return 0;

					}

					
					//����Ҫת������Ϣ
					memset(SendBuf, 0, 1024);
					memcpy(SendBuf, RecvBuf, 1024);
					int k = 0;
					k = send(*ToSOCKET, SendBuf, sizeof(SendBuf), 0);
					cout << endl;
					cout << "/*********************************************/" << endl;
					cout << endl;
					cout << "from:            " << receMSG->name << endl
						<< "to:              " << receMSG->recname << endl
						<< "time:            " << receMSG->time << endl
						<< "receive message: " << receMSG->message << endl;
					cout << endl;
					cout << "/*********************************************/" << endl;
					cout << endl;
				}
				//�����û�������
				else {

					memset(SendBuf, 0, 1024);

					char name[10];
					char recname[10];
					memcpy(name, RecvBuf, 10);
					memcpy(recname, RecvBuf+10, 10);
					memcpy(SendBuf, "server", sizeof("server"));
					memcpy(SendBuf+10, name, 10);
					time_t t = time(0);
					strftime(SendBuf+20, 64, "%Y/%m/%d %X %A ", localtime(&t));
					memcpy(SendBuf+84, "�Է�δ���߻򲻴��ڸ��û�",sizeof("�Է�δ���߻򲻴��ڸ��û�") );

					int k = 0;
					k = send(*ClientSocket, SendBuf, sizeof(SendBuf), 0);

					cout << "/*********************************************/" << endl;
					cout << endl;
					cout << "������Ϣ����" << name << endl;
					cout <<"������Ϣ����"<< recname << "  �������û���δ����" << endl;
					cout << endl;
					cout << "/*********************************************/" << endl;
					cout << endl;
				}






			
			
			}



		}

	}


}



void main() {



	//Startup
	version = MAKEWORD(2, 2);
    int result_start;
	result_start= WSAStartup(version, &wsadata);

	if ( result_start!= 0) {
		cout << "Startup ʧ��" << endl;
		return ;//����
	}

	//�����׽���
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		cout << "�׽��ִ���ʧ��" << endl;
		return;
	}


	//�󶨵�ַ
	
	/*char* serverip = new char[50];
	cout << "����������ip�� ";
	cin >> serverip;
	int serverport;
	cout << "���������˶˿ںţ� " << endl;
	cin >> serverport;
	*/

	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); //ip��ַ
	addr.sin_port = htons(888);							//�˿�
	//addr.sin_addr.S_un.S_addr = inet_addr(serverip); //ip��ַ
	//addr.sin_port = htons(serverport);							//�˿�
	addr.sin_family = AF_INET;							//Э���
	
	int result_bind;
	result_bind = bind(serverSocket, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN));
	if (result_bind == SOCKET_ERROR) {
		cout << "bindʧ��" << endl;
		return;
	}

	//����
	int result_listen;
	result_listen=listen(serverSocket,10);
	if (result_listen == SOCKET_ERROR) {
		cout << "��������" << endl;
		return;
	}

	cout << "��������ʼ����" <<endl<<"������ip: "<< "127.0.0.1" <<"  "<<"�������˿ں�: "<<888 << endl;


	int len = sizeof(SOCKADDR_IN);


	
	while (1) {
	
		if (OnConnectNum < maxClientNum) {
			SOCKET* clientSocket = new SOCKET;
			*clientSocket = accept(serverSocket, (SOCKADDR*)&addrC[OnConnectNum], &len);


			cout << "/*********************************************/" << endl;
			cout << endl;
			cout << "accept Client  " << inet_ntoa(addrC[OnConnectNum].sin_addr) << "   " << ntohs(addrC[OnConnectNum].sin_port) << endl;
			cout << endl;
			cout << "/*********************************************/" << endl;
			cout << endl;


			OnConnectNum++;
			if (OnConnectNum == maxClientNum)
			{
				cout << "�ﵽ�����������" << endl;
			}
			HANDLE hThread;
			hThread = CreateThread(NULL, NULL, &SeverThread, (LPVOID)clientSocket, 0, NULL);
			CloseHandle(hThread);
			
		}

	
	}



	closesocket(serverSocket);
	WSACleanup();


}