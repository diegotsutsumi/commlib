#include "Exception.h"
#include <iostream>
#include "P2PMessage.h"
#include "Socket.h"
#include <unistd.h>
#include <fstream>

using namespace std;

int main ( int argc, char* argv[] )
{
	//unsigned char p2pId[17] = {0x4e,0x49,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xdd,0x87,0x42,0x42,0x43,0x42,0x45};
	//unsigned char p2pId[17] = {0x4e,0x49,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xb0,0x5b,0x44,0x42,0x44,0x44,0x44};
	unsigned char p2pId[17] = {0x4e,0x49,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xde,0x28,0x42,0x41,0x42,0x43,0x43}; //Ricardo
	//unsigned char p2pId[17] = {0x4e,0x49,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xc9,0x75,0x42,0x45,0x44,0x44,0x41}; //Sala
	//unsigned char p2pId[17] = {0x4e,0x49,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xdd,0x48,0x44,0x46,0x42,0x44,0x43}; //Casarotto
	
	//unsigned char usr[8] = {0x61,0x64,0x6d,0x69,0x6e,0x31,0x32,0x33}; //admin123
	unsigned char usr[5] = {0x61,0x64,0x6d,0x69,0x6e}; //admin
	//unsigned char login[8] = {0x74,0x72,0x6f,0x6c,0x6c,0x65,0x72,0x73}; //trollers
	unsigned char login[5] = {0x6e,0x76,0x72,0x64,0x63}; //nvrdc

	int snapIndex=0;
	unsigned char answer[80000];
	long int index=0;
	long int size=0;
	int errno_;
	std::ofstream outputFile; //test
	P2PMessage *ans=0;
	unsigned char packet_type=0;
	std::string snapFile="";
	std::string snapFilePre="snapshot2P2P";
	std::string snapFileSuf=".jpg";
	unsigned long long totalSize=0;
	
	ClientSocket cl_sock;

	if(!cl_sock.connectTo("127.0.0.1",30123))
	{
		std::cout << "Couldn't connect to server" << std::endl;
		return 0;
	}
	else
	{
		std::cout << "Connected to server" << std::endl;
	}

	P2PMessage *req = new ConnectRequest(3,p2pId,17,usr,5,login,0);

	unsigned int reqSize=0;
	unsigned char * reqBytes = req->serialize(&reqSize);
	bool timeover;
	cl_sock.sendData(reqBytes, reqSize, 0, &timeover);

	delete[] reqBytes;
	delete req;
	req=0;
	reqBytes=0;

	size = cl_sock.receiveData(answer, 80000, 0, &errno_);
	for(unsigned int i=0;i<size;i++)
	{
		std::cout << (unsigned int)answer[i] << " ";
	}
	std::cout << std::endl;


	req = new GetSnapshotRequest(3);
	reqBytes = req->serialize(&reqSize);
	snapIndex=0;
	ans = new SnapshotAnswer();
	while(1)
	{
		sleep(1);
		cl_sock.sendData(reqBytes, reqSize, 0, &timeover);

		size = cl_sock.receiveData(answer, 80000, 0, &errno_);

		packet_type = P2PMessage::getTypeFromRawInfo(answer,size);

		totalSize=size;
		if(packet_type==P2P_SNAPSHOT_CONTENT)
		{
			ans->unserialize(answer, size);
			
			unsigned long long snap_size=0;
			unsigned char *snapshot = ((SnapshotAnswer*)ans)->getSnapshot(&snap_size);

			while(totalSize < (snap_size+17))
			{
				size = cl_sock.receiveData(answer+totalSize, (80000-totalSize), 0, &errno_);
				totalSize += size;
				std::cout << "Split Image " << totalSize << " " << snap_size << std::endl;
			}

			snapFile = snapFilePre + std::to_string(snapIndex) + snapFileSuf;
			outputFile.open(snapFile,std::ios_base::app);
			for(unsigned int i=0;i<snap_size;i++)
			{
				outputFile << snapshot[i];
			}
			outputFile.close();
			snapIndex++;
			std::cout << "Snapshot " << snapIndex << " saved" << std::endl;
		}
		else
		{
			std::cout << "Not a snapshot " << (unsigned int)packet_type << std::endl;
		}
	}

	return 0;
}