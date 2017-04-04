#include "Exception.h"
#include <iostream>
#include "SnitchNotifier.h"

using namespace std;

int main ( int argc, char* argv[] )
{
	while(1)
	{
		{
			try
			{
				SnitchNotifier app(40232);

				if(!app.run()) //It blocks
				{
					cout << "Notifier returned false." << endl;
				}

				app.stop();
			}
			catch(Exception &e)
			{
				cout << "Exception caught: " << e.description() << endl;
			}
		}
		sleep(5);
	}

	return 0;
}