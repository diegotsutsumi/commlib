#include "Exception.h"
#include <iostream>
#include "ClientSCM.h"
#include "AppType.h"
#include <string>
#include <stdexcept>
#include <arpa/inet.h>

using namespace std;

int main ( int argc, char* argv[] )
{
	AppType app_type=none_type;
	unsigned short c_port=0;
	string c_ip = "";
	unsigned int i=0;

	if(argc<2)
	{
		cout << endl << "One of the above list of parameters is needed to run SCM Server:" << endl;
		cout << "https" << endl;
		cout << "ssh" << endl;
		cout << "rtsp" << endl;
		cout << "Command Example: scm_client https" << endl << endl;
		return 0;
	}
	string arg1(argv[1]);
	if(arg1.compare("https")==0)
	{
		app_type=https_type;
	}
	else if(arg1.compare("ssh")==0)
	{
		app_type=ssh_type;
	}
	else if(arg1.compare("rtsp")==0)
	{
		app_type=rtsp_type;
		if(argc==4)
		{
			string arg2(argv[2]);
			string arg3(argv[3]);
			unsigned int t_port=0;
			struct sockaddr_in sa;
			try
			{
				t_port = stoi(arg3);
			}
			catch (const invalid_argument& ia)
			{
				cerr << "Invalid port number: " << ia.what() << '\n';
				cout << "Invalid port number: " << ia.what() << '\n';
				return 0;
			}
			if(t_port > 65536)
			{
				cout << "Invalid port number: " << t_port << endl;
				return 0;
			}
			else
			{
				c_port = t_port;
			}
			
			if(!inet_pton(AF_INET, arg2.c_str(), &(sa.sin_addr)))
			{
				cout << "Invalid Ip Address " << arg2 << endl;
				return 0;
			}
			else
			{
				c_ip = arg2;
			}
		}
		else
		{
			cout << endl << "RTSP Application type requires ip and port as parameters" << endl;
			cout << "Command Example: scm_snitch rtsp 172.125.89.23 554" << endl << endl;
			return 0;
		}
	
	}
	else
	{
		cout << endl << "Given parameter is not in the above list:" << endl;
		cout << "https" << endl;
		cout << "ssh" << endl;
		cout << "rtsp" << endl;
		cout << "Command Example: scm_server https" << endl << endl;
		return 0;
	}
	
	i=0;
	while(1)
	{
		{
			try
			{
				ClientSCM app(app_type, c_ip, c_port);
				switch(app_type)
				{
					case https_type:
						cout << "Client Routing https" << endl;
						break;

					case ssh_type:
						cout << "Client Routing ssh" << endl;
						break;

					case rtsp_type:
						cout << "Client Routing rtsp" << endl;
						break;

					default:
						cout << "Client Routing unknown app" << endl;
						break;
				}

				if(!app.runClientSCM())
				{
					cout << "SCM returned false." << endl;
				}
			}
			catch(SocketException& e)
			{
				cout << "Exception caught:" << e.description() << "\n";
			}
		}

		i++;
		if(i<150)
		{
			sleep(15);
		}
		else
		{
			break;
		}
	}

	return 0;
}
