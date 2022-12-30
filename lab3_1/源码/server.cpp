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


pseudoHead ph;		//伪首部

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

	//初始化伪首部
	memset(&ph, 0, sizeof(pseudoHead));
	ph.source_ip = inet_addr(ip_client);
	ph.dest_ip = inet_addr(ip_server);

	cout << "初始化成功，服务器端开始建接" << endl;

}




bool establishConnect() {

	int len = sizeof(SOCKADDR_IN);

	char recBuffer[sizeof(msg)];	//接收缓冲区
	char sendBuffer[sizeof(msg)];	//发送缓冲区

	msg* sed = (msg*)sendBuffer;
	msg* rec = (msg*)recBuffer;

	memset(recBuffer, 0, sizeof(msg));
	memset(sendBuffer, 0, sizeof(msg));



	
	while (1) {


		//阻塞，接收SYN
		recvfrom(sockServer, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, &len);

		if (isSyn(rec) && verfiyChecksum(rec, &ph) && rec->seq_num==0) {
		
			
			cout << "服务器端： 接收到客户端SYN报文，验证成功" << endl;

			//设置SYN，ACK报文
			setAck(sed);
			setSyn(sed);
			sed->seq_num = 0;
			sed->ack_num = 1;
			sed->source_port = port_server;
			sed->dest_port = port_client;
			setChecksum(sed, &ph);

			//发送SYN，ACK
			sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
			break;
		}
		else {
			
			cout << "服务器端： 接收到客户端SYN报文，验证失败" << endl;
			continue;
		
		}

	}

	//设为非阻塞
	u_long imode = 1;
	ioctlsocket(sockServer, FIONBIO, &imode);//非阻塞


	clock_t start = clock(); //开始计时
	while (recvfrom(sockServer, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, &len) <= 0) {
		


		
		// over time
		if (clock() - start >= MAX_TIME) {

			//超时重传
			sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
			cout << "服务器端： 重传报文（SYN，ACK）" << endl;
			start = clock();
		}
	}


	if (isAck(rec) && verfiyChecksum(rec, &ph)) {

		cout << "服务器端：接收到客户端报文（SYN，ACK）验证正确" << endl;
		

	}
	else {
	
		return false;
	}



	imode = 0;
	ioctlsocket(sockServer, FIONBIO, &imode);//阻塞
	return true;



}




msg make_pkt(int ack) {

	msg message;
	memset(&message, 0, sizeof(msg));
	message.source_port = port_client;
	message.dest_port = port_server;
	setAck(&message);
	message.ack_num = ack;
	pseudoHead ph;		//伪首部
	memset(&ph, 0, sizeof(pseudoHead));
	ph.source_ip = inet_addr(ip_client);
	ph.dest_ip = inet_addr(ip_server);
	setChecksum(&message, &ph);	//设置校验和

	return message;
}



DWORD rdt3_receive_FSM(char* file,char *filename) {



	int index = 0;
	int stage = 0;

	int len = sizeof(SOCKADDR_IN);

	char* pktBuffer = new char[sizeof(msg)];
	char* recpktBuffer = new char[sizeof(msg)];
	char* sendBuffer = new char[sizeof(msg)];

	DWORD rec_data_len = 0;
	msg* rec = (msg*)recpktBuffer;


	

	u_long imode = 0;
	if(ioctlsocket(sockServer, FIONBIO, &imode)==SOCKET_ERROR)
	cout << "error" << endl;

	bool start_tran = 0;

	while (1) {

		
		memset(recpktBuffer, 0, sizeof(msg));

		switch (stage) {
		case 0:
			
			if (recvfrom(sockServer, recpktBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, &len) > 0 &&  rec->length!=0) {
			
			}
			else {
				
				break;
			}

			

			if (isEnd(rec)&& start_tran) {

				memcpy(filename, rec->msg, rec->length);
				cout << "传输完毕" << endl;
				memset(sendBuffer, 0, sizeof(msg));
				msg sed = make_pkt(0);
				setEnd(&sed);
				sed.checksum = 0;
				setChecksum(&sed, &ph);	

				memcpy(sendBuffer, &sed, sizeof(msg));
				
				sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
				cout << "服务器: 发送报文（END，ACK）" << endl;
				return rec_data_len;
			}


			if (rec->seq_num == 3 || !(verfiyChecksum(rec, &ph))) {

				msg sedpkt = make_pkt(3);
				memcpy(sendBuffer, &sedpkt, sizeof(msg));
				sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
				cout << "状态0\t接收\tseq:\t3\t" << endl;
				cout << "状态0\t发送\tack:\t3\tACK:\t" << isAck(&sedpkt) << "\tlength:\t" << sedpkt.length << "\tchecksum:\t" << sedpkt.checksum << endl;

				stage = 0;
				break;

			}

			//正确接收
			if (rec->seq_num == 2 && (verfiyChecksum(rec, &ph))) {

				msg sedpkt = make_pkt(2);
				memcpy(sendBuffer, &sedpkt, sizeof(msg));
				sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
				cout << "状态0\t接收\tseq:\t2\tindex:\t" << index << "\tlength:\t" << rec->length << "\tchecksum:\t" << rec->checksum << endl;
				cout << "状态0\t发送\tack:\t2\tACK:\t" << isAck(&sedpkt) << "\tlength:\t" << sedpkt.length << "\tchecksum:\t" << sedpkt.checksum << endl;				
				memcpy(file + rec_data_len, rec->msg, rec->length);
				rec_data_len += rec->length;
				stage = 1;
				start_tran = 1;
				index++;
				break;

			}
			break;


		case 1:

			recvfrom(sockServer, recpktBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, &len);
		

			if (isEnd(rec)&& start_tran) {

				memcpy(filename, rec->msg, rec->length);
				cout << "传输完毕" << endl;
				memset(sendBuffer, 0, sizeof(msg));
				msg sed = make_pkt(0);
				setEnd(&sed);
				sed.checksum = 0;
				setChecksum(&sed, &ph);
				memcpy(sendBuffer, &sed, sizeof(msg));
				sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);

				return rec_data_len;
			}


			if (rec->seq_num == 2 || !(verfiyChecksum(rec, &ph))) {

				msg sedpkt = make_pkt(2);
				memcpy(sendBuffer, &sedpkt, sizeof(msg));
				sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
				cout << "状态1\t接收\tseq:\t2\t" << endl;
				cout << "状态1\t发送\tack:\t2\tACK:\t" << isAck(&sedpkt) << "\tlength:\t" << sedpkt.length << "\tchecksum:\t" << sedpkt.checksum << endl;

				stage = 1;

				break;

			}

			if (rec->seq_num == 3 && (verfiyChecksum(rec, &ph))) {

				msg sedpkt = make_pkt(3);
				memcpy(sendBuffer, &sedpkt, sizeof(msg));
				sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
				cout << "状态1\t接收\tseq:\t3\tindex:\t" << index << "\tlength:\t" << rec->length << "\tchecksum:\t" << rec->checksum << endl;
				cout << "状态1\t发送\tack:\t3\tACK:\t" << isAck(&sedpkt) << "\tlength:\t" << sedpkt.length << "\tchecksum:\t" << sedpkt.checksum << endl;
				memcpy(file + rec_data_len, rec->msg, rec->length);
				rec_data_len += rec->length;
				start_tran = 1;
				stage = 0;
				
			
				index++;
				break;

			}
			break;
		}


	}

	
}



bool closeConnect() {

	int len = sizeof(SOCKADDR_IN);
	char recBuffer[sizeof(msg)];	//接收缓冲区
	char sendBuffer[sizeof(msg)];	//发送缓冲区
	memset(recBuffer, 0, sizeof(msg));
	memset(sendBuffer, 0, sizeof(msg));

	msg* sed = (msg*)sendBuffer;
	msg* rec = (msg*)recBuffer;



	u_long imode = 0;
	ioctlsocket(sockServer, FIONBIO, &imode);//阻塞


	//（1）接收验证客户端的FIN报文。
	while (1) {
		recvfrom(sockServer, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len);

		if (isFin(rec) && verfiyChecksum(rec, &ph)) {

			cout << "服务器： 收到客户端Fin请求，验证正确" << endl;
			break;
		}

	}


	//（2）服务器：设置发送ACK，FIN报文

	setFin(sed);		//设置Fin
	setAck(sed);		//设置Ack
	sed->seq_num = 0;	//设置seq=0
	sed->ack_num = 0;	//设置ack=0
	sed->source_port = port_client;
	sed->dest_port = port_server;
	setChecksum(sed, &ph);	//设置校验和

	//发送
	sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);




	//剩余数据

	//发送FIN
	cleanflag(sed);
	setFin(sed);		//设置Fin
	setChecksum(sed, &ph);
	sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);


	imode = 1;
	ioctlsocket(sockServer, FIONBIO, &imode);//非阻塞

	//接收
	clock_t  start = clock(); //开始计时
	while (recvfrom(sockServer, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len) <= 0 ||!(isAck(rec) && isFin(rec) && verfiyChecksum(rec, &ph)) ) {

		// over time
		if (clock() - start >= MAX_TIME) {

			//超时重传
			sendto(sockServer, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_client, len);
			cout << "服务器： 发送报文（FIN），重传" << endl;
			start = clock();
		}
	}

	if (isAck(rec) && isFin(rec) && verfiyChecksum(rec, &ph)) {

		cout << "服务器： 接收到报文（FIN，ACK），验证正确" << endl;

	}
	else {
	
		return false;
	}


	cout << "连接关闭" << endl;
	closesocket(sockServer);
	return true;




};











int main() {

	init();
	if (establishConnect()) {

		cout << "服务端： 建立连接成功" << endl;
	}


	cout << endl;
	cout << "/*****************************************************/" << endl;

	//接收文件的缓冲区
	
	bool tran = 1;

	while (tran) {
		char* fileBuffer = new char[90000000];

		DWORD fileLength = 0;
		char* filename = new char[100];
		memset(filename, 0, 100);

	
		fileLength = rdt3_receive_FSM(fileBuffer, filename);



		
		cout << "/*****************************************************/" << endl;
		cout << endl;
		//string dir = "C:/Users/nan/Desktop/lab3/lab3_1/rec file/";
		string dir = "rec file/";
		string  fn = filename;
		string filenm = dir + fn;


		//写入复制文件
		ofstream outfile(filenm, ios::binary);
		//if (!outfile.is_open()) {
		//	cout << "打开文件出错" << endl;
		//	return 0;
		//}
		//cout << filenm << endl;
		outfile.write(fileBuffer, fileLength);
		outfile.close();


		cout << "是否继续接受传输（Y/N）： ";
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