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
const int maxClientNum = 5;	//最大连接数量
int OnConnectNum = 0;			//当前连接数量
const int MaxBufSize = 1024;	//报文传输最大字节数
SOCKADDR_IN addrC[maxClientNum];

//报文格式
typedef struct MESSAGE {
	char name[10];
	char recname[10];
	char time[64];
	char message[MaxBufSize-10-10-64];

}myMSG;

//存储用户名与socket的键值对结构
typedef struct clientMap {
	char name[10];
	SOCKET* sockClient;

}CM;

CM ClientMap[maxClientNum];


WSADATA wsadata;
WORD version;

SOCKET serverSocket;
SOCKADDR_IN addr;

//服务线程
DWORD WINAPI SeverThread(LPVOID lpParameter)
{
	//新建一个SOCKET用于通信
	SOCKET* ClientSocket = (SOCKET*)lpParameter;
	bool init = 1;

	//缓冲区
	char RecvBuf[MaxBufSize];
	char SendBuf[MaxBufSize];
	
	//开始接收与转发
	while (1)
	{
		memset(RecvBuf, 0, 1024);

		if (recv(*ClientSocket, RecvBuf, sizeof(RecvBuf), 0) > 0)
		{ 

			myMSG* receMSG=(myMSG*)RecvBuf;

			//与客户端初始化信息
			if (strcmp(receMSG->message, "init") == 0&&init==1) {
				memcpy(ClientMap[OnConnectNum - 1].name, receMSG->name, 10);
				ClientMap[OnConnectNum - 1].sockClient= ClientSocket;

				cout << endl;
				cout << "/*********************************************/" << endl;
				cout << endl;
				cout << "用户： " << receMSG->name << " 完成初始化";
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




			//非初始化，即转发消息
			if (init != 1) {

				SOCKET* ToSOCKET=NULL;
				bool find = 0;

				//在map中寻找聊天对象
				for (int i = 0; i < maxClientNum; i++) {
					if (strcmp(receMSG->recname, ClientMap[i].name) == 0) {
						ToSOCKET = ClientMap[i].sockClient;
						find = 1;
						break;
					}
				}

				//聊天用户在线
				if (find == 1) {


					//客户端退出
					if (strcmp(receMSG->message, "exit") == 0) {

						cout << endl;
						cout << "/*********************************************/" << endl;
						cout << endl;
						cout << "from:            " << receMSG->name << endl
							<< "time:            " << receMSG->time << endl
							<< "客户端退出" << endl;
						//<< "receive message: " << receMSG->message << endl;
						cout << endl;
						cout << "/*********************************************/" << endl;
						cout << endl;

						//向聊天对象发送对方退出的信息
						memset(SendBuf, 0, 1024);
						memcpy(SendBuf, RecvBuf, 1024);
						memcpy(SendBuf+84, "对方已退出", sizeof("对方已退出"));
						int k = 0;
						k = send(*ToSOCKET, SendBuf, sizeof(SendBuf), 0);
						//关闭与当前用户的socket
						OnConnectNum--;
						closesocket(*ClientSocket);
						return 0;

					}

					
					//复制要转发的消息
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
				//聊天用户不在线
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
					memcpy(SendBuf+84, "对方未上线或不存在该用户",sizeof("对方未上线或不存在该用户") );

					int k = 0;
					k = send(*ClientSocket, SendBuf, sizeof(SendBuf), 0);

					cout << "/*********************************************/" << endl;
					cout << endl;
					cout << "发送信息方：" << name << endl;
					cout <<"接收信息方："<< recname << "  不存在用户或未上线" << endl;
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
		cout << "Startup 失败" << endl;
		return ;//结束
	}

	//建立套接字
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		cout << "套接字创建失败" << endl;
		return;
	}


	//绑定地址
	
	/*char* serverip = new char[50];
	cout << "请输入服务端ip： ";
	cin >> serverip;
	int serverport;
	cout << "请输入服务端端口号： " << endl;
	cin >> serverport;
	*/

	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); //ip地址
	addr.sin_port = htons(888);							//端口
	//addr.sin_addr.S_un.S_addr = inet_addr(serverip); //ip地址
	//addr.sin_port = htons(serverport);							//端口
	addr.sin_family = AF_INET;							//协议簇
	
	int result_bind;
	result_bind = bind(serverSocket, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN));
	if (result_bind == SOCKET_ERROR) {
		cout << "bind失败" << endl;
		return;
	}

	//监听
	int result_listen;
	result_listen=listen(serverSocket,10);
	if (result_listen == SOCKET_ERROR) {
		cout << "监听错误" << endl;
		return;
	}

	cout << "服务器开始监听" <<endl<<"服务器ip: "<< "127.0.0.1" <<"  "<<"服务器端口号: "<<888 << endl;


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
				cout << "达到最多连接数量" << endl;
			}
			HANDLE hThread;
			hThread = CreateThread(NULL, NULL, &SeverThread, (LPVOID)clientSocket, 0, NULL);
			CloseHandle(hThread);
			
		}

	
	}



	closesocket(serverSocket);
	WSACleanup();


}