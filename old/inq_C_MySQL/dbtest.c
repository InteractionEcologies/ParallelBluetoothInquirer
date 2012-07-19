#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{	
	MYSQL *conn;
	
	conn = mysql_init(NULL);
	
	if( conn == NULL ){
		printf("Error\n");
	}

	if ( mysql_real_connect(conn, "localhost", "zetcode", "passwd", NULL, 0, NULL, 0) == NULL) {
		printf("Error\n");
	}

	if (mysql_query(conn, "create database testdb")) {
		printf("Error\n");
	}

	mysql_close(conn);
	return 0;
}
