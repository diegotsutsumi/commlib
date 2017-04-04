#include <stdlib.h>
#include <string.h>

#include "scm_db.h"

MYSQL dbconn;

int scmDbConnected = false;

void scmDbConnect()
{
	if ( !mysql_init( &dbconn ) )
	{
		Error( "Can't initialise database connection: %s", mysql_error( &dbconn ) );
		exit( mysql_errno( &dbconn ) );
	}
    my_bool reconnect = 1;
    if ( mysql_options( &dbconn, MYSQL_OPT_RECONNECT, &reconnect ) )
        Fatal( "Can't set database auto reconnect option: %s", mysql_error( &dbconn ) );
    std::string::size_type colonIndex = staticConfig.DB_HOST.find( ":/" );
    if ( colonIndex != std::string::npos )
    {
        std::string dbHost = staticConfig.DB_HOST.substr( 0, colonIndex );
        std::string dbPort = staticConfig.DB_HOST.substr( colonIndex+1 );
	    if ( !mysql_real_connect( &dbconn, dbHost.c_str(), staticConfig.DB_USER.c_str(), staticConfig.DB_PASS.c_str(), 0, atoi(dbPort.c_str()), 0, 0 ) )
	    {
		    Error( "Can't connect to server: %s", mysql_error( &dbconn ) );
		    exit( mysql_errno( &dbconn ) );
	    }
    }
    else
    {
	    if ( !mysql_real_connect( &dbconn, staticConfig.DB_HOST.c_str(), staticConfig.DB_USER.c_str(), staticConfig.DB_PASS.c_str(), 0, 0, 0, 0 ) )
	    {
		    Error( "Can't connect to server: %s", mysql_error( &dbconn ) );
		    exit( mysql_errno( &dbconn ) );
	    }
    }
	if ( mysql_select_db( &dbconn, staticConfig.DB_NAME.c_str() ) )
	{
		Error( "Can't select database: %s", mysql_error( &dbconn ) );
		exit( mysql_errno( &dbconn ) );
	}
    zmDbConnected = true;
}

