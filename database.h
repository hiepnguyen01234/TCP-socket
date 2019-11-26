#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <mysql.h>
#include <stdio.h>
#include <string.h>

#define SERVER "localhost"
#define USER "root"
#define PASSWORD "tomtom"
#define DATABASE "filesharing"

#define MAX_LENGTH_QUERY 1024
#define MAX_LENGTH_RESULT 8196

MYSQL *initConnection();
void closeConnection(MYSQL *conn);

#endif
