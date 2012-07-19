#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
	bson b;
	mongo conn;

	if( mongo_connect( &conn, "127.0.0.1", 27017 ) != MONGO_OK){
		switch( conn.err){
			case MONGO_CONN_NO_SOCKET:
				printf("FAIL: Could not create a socket!\n");
				break;
			case MONGO_CONN_FAIL:
				printf("FAIL: Could not connect to mongod. Make sure it's listening at 127.0.0.1:27017");
				break;
		}
	
		exit( 1 );
	}

	mongo_destroy( &conn);
	return 0;
}
