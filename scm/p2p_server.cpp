#include "Exception.h"
#include <iostream>
#include "P2PRequestServer.h"
#include <unistd.h>

using namespace std;

int main ( int argc, char* argv[] )
{
	while(1)
	{
		{
			try
			{
				P2PRequestServer *app = new P2PRequestServer(30123);

				cout << "P2PRequestServer starting." << endl;

				if(!app->start()) //It blocks
				{
					cout << "P2PRequestServer returned false." << endl;
				}

				app->stop();
			}
			catch(Exception &e)
			{
				cout << "Exception caught: " << e.description() << endl;
			}
		}
	}

	return 0;
}