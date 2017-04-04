#include "Exception.h"
#include <iostream>
#include <string>
#include "ServerSCM.h"
#include "AppType.h"

using namespace std;

int main ( int argc, char* argv[] )
{
	AppType type=none_type;
	if(argc!=2)
	{
		cout << endl << "Only one of the above list of parameters is needed to run SCM Server:" << endl;
		cout << "https" << endl;
		cout << "ssh" << endl;
		cout << "rtsp" << endl;
		cout << "Command Example: scm_server rtsp" << endl << endl;
		return 0;
	}
	string arg(argv[1]);
	if(arg.compare("https")==0)
	{
		type=https_type;
	}
	else if(arg.compare("ssh")==0)
	{
		type=ssh_type;
	}
	else if(arg.compare("rtsp")==0)
	{
		type=rtsp_type;
	}
	else
	{
		cout << endl << "Given parameter is not in the above list:" << endl;
		cout << "https" << endl;
		cout << "ssh" << endl;
		cout << "rtsp" << endl;
		cout << "Command Example: scm_server rtsp" << endl << endl;
		return 0;
	}

	while(1)
	{
		{
			try
			{
				ServerSCM app(type);
				switch(type)
				{
					case https_type:
						cout << "Server Routing https" << endl;
						break;

					case ssh_type:
						cout << "Server Routing ssh" << endl;
						break;

					case rtsp_type:
						cout << "Server Routing rtsp" << endl;
						break;

					default:
						cout << "Server Routing unknown app" << endl;
						break;
				}

				if(!app.startServerSCM())
				{
					cout << "SCM returned false." << endl;
				}

				app.stopServerSCM();
			}
			catch(Exception &e)
			{
				cout << "SnitchServer Exception caught: " << e.description() << endl;
			}
		}
		sleep(5);
	}

	return 0;
}