#include "database.h"

// Ket noi database
MYSQL *initConnection()
{
    MYSQL *conn;
    conn = mysql_init(NULL);

    if (!mysql_real_connect(conn, SERVER, USER, PASSWORD, DATABASE, 0, NULL, 0))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return NULL;
    }
    fprintf(stderr, "%s\n", "=> Connect to database successfully");
    return conn;
}

//Dong ket noi database
void closeConnection(MYSQL *conn)
{
    fprintf(stderr, "%s\n", "=> Close connect database successfully");
    mysql_close(conn);
}

