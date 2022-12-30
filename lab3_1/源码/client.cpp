#include"Message.h"
#include<fstream>
#include<math.h>
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
	ioctlsocket(sockClient, FIONBIO, &imode);//非阻塞

	//bind
	int result_bind;
	result_bind = bind(sockClient, (SOCKADDR*)&addr_client, sizeof(SOCKADDR_IN));
	if (result_bind == SOCKET_ERROR) {
		cout << "bind failed" << endl;
		return;
	}

	//初始化伪头部
	memset(&ph, 0, sizeof(pseudoHead));
	ph.source_ip = inet_addr(ip_client);
	ph.dest_ip = inet_addr(ip_server);

	cout << "初始化成功，客户端开始建接"<<endl;

};



bool establishConnect() {

	int len = sizeof(SOCKADDR_IN);

	char recBuffer[sizeof(msg)];	//接收缓冲区
	char sendBuffer[sizeof(msg)];	//发送缓冲区

	memset(recBuffer, 0, sizeof(msg));
	memset(sendBuffer, 0, sizeof(msg));


	msg* sed = (msg*)sendBuffer;		
	msg* rec = (msg*)recBuffer;
	


	setSyn(sed);		//设置SYN
	sed->seq_num = 0;	//设置seq=0
	sed->ack_num = 0;	//设置ack=0
	sed->source_port = port_client;
	sed->dest_port = port_server;
	setChecksum(sed, &ph);	//设置校验和


	//发送
	sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
	

	clock_t start = clock(); //开始计时

	while (recvfrom(sockClient, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len) <= 0) {
		
		
		
		// over time
		if (clock() - start >= MAX_TIME) {
			
			//超时重传
			sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
			start = clock();
		}
	}

	cout << "客户端:接收到报文（SYN，ACK）" << endl;
	



		//判断接收
	if (isAck(rec) && isSyn(rec) && verfiyChecksum(rec, &ph)) {

			cout << "客户端:接收报文（SYN，ACK）验证正确" << endl;

			memset(sendBuffer, 0, sizeof(msg));
			setAck(sed);	//设置ack

			sed->seq_num = 1;
			sed->ack_num = 1;
			sed->source_port = port_client;
			sed->dest_port = port_server;

			setChecksum(sed, &ph);
			sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
			
		}
	else {

			cout << "客户端:接收报文（SYN，ACK）验证错误" << endl;
			
			return 0;
			
		}

	
	cout << "客户端： 建立连接成功" << endl;
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
	pseudoHead ph;		//伪首部
	memset(&ph, 0, sizeof(pseudoHead));
	ph.source_ip = inet_addr(ip_client);
	ph.dest_ip = inet_addr(ip_server);
	setChecksum(&message, &ph);	//设置校验和

	return message;
}


void rdt3_send_FSM(unsigned long length_file ,char * file,char * filename) {

	int packetNUM = int(length_file / Max_Size) + (length_file % Max_Size ? 1 : 0);
	cout << "packetNUM: " << packetNUM << endl;

	int index = 0;
	int stage = 0;

	int len = sizeof(SOCKADDR_IN);
	int	packetDataLen = min(Max_Size, length_file - index * Max_Size);
	char *dataBuffer=new char[Max_Size];
	char *pktBuffer = new char[sizeof(msg)];
	char* recpktBuffer = new char[sizeof(msg)];
	msg sndpkt;
	clock_t start_timer;



	msg* rec = (msg*)recpktBuffer;

	cout << "本次文件数据长度为    " << length_file << "Bytes,    需要传输" << packetNUM << "个数据包" << endl;

	while (1) {
		

		//发送传输结束报文
		if (index == packetNUM) {

			u_long imode = 1;
			ioctlsocket(sockClient, FIONBIO, &imode);//非阻塞
			
		
			char* sendBuffer = new char[sizeof(msg)];
			memset(sendBuffer, 0, sizeof(msg));
			msg *sed = (msg*)sendBuffer;
			setEnd(sed);		
			sed->source_port = port_client;
			sed->dest_port = port_server;
			string fn = filename;
			int filename_len = sizeof(fn);

			memcpy(sed->msg, filename, filename_len);
			sed->length = filename_len;
			setChecksum(sed, &ph);	//设置校验和

			//发送
			sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
			cout << "客户端： 发送报文（END）" << endl;

			clock_t start_timer = clock(); //开始计时

			while (recvfrom(sockClient, recpktBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len) <= 0||!(isEnd(rec)&&isAck(rec))) {
				// over time
				if (clock() - start_timer >= MAX_TIME) {

					//超时重传
					sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
					cout << "客户端： 发送报文（END）,重传" << endl;
					start_timer = clock();
				}
			}

			
			if (isEnd(rec)&&isAck(rec) && verfiyChecksum(rec, &ph) ) {
				cout << "客户端：接收服务器报文（END，ACK），文件传输完成" << endl;
				return ;
			}
			else
				continue;
			

		}




		//传输数据长度
		packetDataLen=min(Max_Size, length_file - index * Max_Size);

		switch (stage) {
			case 0:
			



				memcpy(dataBuffer, file+index* Max_Size, packetDataLen);
				sndpkt = make_pkt(2, dataBuffer, packetDataLen);
				memcpy(pktBuffer, &sndpkt, sizeof(msg));
				sendto(sockClient, pktBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
				start_timer = clock();
				stage = 1;
				cout << "状态0\t发送\tseq:\t2\tindex:\t" << index <<"\tlength:\t"<< packetDataLen << "\tchecksum:\t"<<sndpkt.checksum << endl;
				break;

			case 1:

				//超时重传
				if (clock() - start_timer >= MAX_TIME) {
					sendto(sockClient, pktBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
					cout << "状态1\t发送\tseq:\t2\tindex:\t" << index << "\tlength:\t" << packetDataLen << "\tchecksum:\t" << sndpkt.checksum << "  (重传)" << endl;
					start_timer = clock();
				}
		
				
				if (recvfrom(sockClient, recpktBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len)) {
					if (isAck(rec) && verfiyChecksum(rec, &ph) && rec->ack_num == 2) {
						cout << "状态1\t接收\tack:\t2\tAck:\t" << isAck(rec) << "\tlength:\t" << rec->length << "\tchecksum:\t" << rec->checksum  << endl;
						stage = 2;
						index++;
						break;
					}
				}
				break;

			case 2:

	
				memcpy(dataBuffer, file + index * Max_Size, packetDataLen);
				sndpkt = make_pkt(3, dataBuffer, packetDataLen);
				memcpy(pktBuffer, &sndpkt, sizeof(msg));
				sendto(sockClient, pktBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
				cout << "状态2\t发送\tseq:\t3\tindex:\t" << index << " \tlength:\t" << packetDataLen << " \tchecksum:\t" << sndpkt.checksum << endl;
				start_timer = clock();
				stage = 3;
				break;


			case 3:

				//超时重传
				if (clock() - start_timer >= MAX_TIME) {
					sendto(sockClient, pktBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
					cout << "状态3\t发送\tseq:\t3\tindex:\t" << index << "\tlength:\t" << packetDataLen << " \tchecksum:\t" << sndpkt.checksum << "  (重传)" << endl;

					start_timer = clock();
				}


				if (recvfrom(sockClient, recpktBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len)) {
					if (isAck(rec) && verfiyChecksum(rec, &ph) && rec->ack_num == 3) {
						cout << "状态3\t接收\tack:\t3\tAck:\t" << isAck(rec) << "\tlength:\t" << rec->length << "\tchecksum:\t" << rec->checksum << endl;
						stage = 0;
						index++;
						break;
					}
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



	//（1）客户端发送FIN报文
	setFin(sed);		//设置Fin
	sed->seq_num = 0;	//设置seq=0
	sed->ack_num = 0;	//设置ack=0
	sed->source_port = port_client;
	sed->dest_port = port_server;

	setChecksum(sed, &ph);	//设置校验和

	//发送
	sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
	cout << "客户端：发送报文（FIN）" << endl;

	clock_t start = clock(); //开始计时


	//（2）接收确定服务器端的FIN，ACK报文
	while (recvfrom(sockClient, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len) <= 0) {
		

		
		// over time
		if (clock() - start >= MAX_TIME) {

			//超时重传
			sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
			cout << "客户端：发送报文（FIN），重传" << endl;
			start = clock();
		}
	}

	
	if (isAck(rec) && isFin(rec) && verfiyChecksum(rec, &ph)) {

		cout << "客户端：接收报文（FIN，ACK）验证正确" << endl;
		
	}
	else {
	
		return false;
	}


	u_long imode = 0;
	ioctlsocket(sockClient, FIONBIO, &imode);//阻塞

	//（3）接收确定服务器端的FIN报文

	while (1) {
		recvfrom(sockClient, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len);

		if (isFin(rec) && verfiyChecksum(rec, &ph)) {

			cout << "客户端：接收到服务器报文（FIN），验证正确" << endl;
			break;
		}


	}

	imode = 1;
	ioctlsocket(sockClient, FIONBIO, &imode);//非阻塞

	//（4）发送FIN，ACK报文
	cleanflag(sed);
	setFin(sed);
	setAck(sed);
	setChecksum(sed, &ph);	//设置校验和
	sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
	cout << "客户端：发送报文（FIN，ACK）" << endl;

	//等待2MSL
	
	start = clock(); //开始计时

	while (clock() - start <= 2 * MAX_TIME) {
		//if(clock() - start>= MAX_TIME)
		if (recvfrom(sockClient, recBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, &len) > 0&& isFin(rec) && verfiyChecksum(rec, &ph)) {
			sendto(sockClient, sendBuffer, sizeof(msg), 0, (sockaddr*)&addr_server, len);
			cout << "客户端: 发送报文（FIN，ACK）,重传" << endl;
		}
	}




	cout << "客户端：连接关闭" << endl;
	closesocket(sockClient);
	return true;


	

};





int main() {

	
	//初始化
	init();

	//建立连接
	if (!establishConnect()) {

		cout << "建立连接失败" << endl;
	}


	while (1) {

		char* filename = new char[100];
		memset(filename, 0, 100);
		string filedir;


		cout << endl;
		cout << "/*****************************************************/" << endl;

		cout << "默认: 1.jpg" << endl;
		cout << "1: 1.jpg" << endl;
		cout << "2: 2.jpg" << endl;
		cout << "3: 3.jpg" << endl;
		cout << "4: helloworld.txt" << endl;
		cout << "5: 退出" << endl;
		cout << "请输入要传输的文件： " << endl;



		int i;
		cin >> i;
		
		cout << endl;
		cout << "/*****************************************************/" << endl;


		if (i == 5) {
			cout << "传输结束 " << endl;
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
			cout << "无法打开文件" << endl;
			return 0;
		}

		infile.seekg(0, infile.end);
		DWORD fileLen = infile.tellg();
		infile.seekg(0, infile.beg);

		cout<<"传输文件长度: " << fileLen << endl;

		char* fileBuffer = new char[fileLen];
		infile.read(fileBuffer, fileLen);
		infile.close();

		
		

		cout << "开始传输文件: "<< i << endl;

		clock_t start_timer = clock();

		rdt3_send_FSM(fileLen, fileBuffer,filename);

		clock_t end_timer = clock();
		double endtime = (double)(end_timer - start_timer) / CLOCKS_PER_SEC;
		cout << "Total time:" << endtime <<" s" << endl;		
		cout << "吞吐率：" << fileLen * 8 / endtime / 1024 / 1024 << "Mbps" << endl;
		

	}



	closeConnect();

	
	WSACleanup();

	system("pause");

}
