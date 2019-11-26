CC= gcc
RM= rm
SQL = `mysql_config --cflags --libs`
server client : server.o protocol.o database.o accountlist.o client.o
	$(CC) -o server server.o database.o protocol.o accountlist.o -lm -pthread $(SQL) 
	$(CC) -o client client.o protocol.o -pthread -pipe -rdynamic -lm `pkg-config --cflags --libs gtk+-3.0`
	$(RM) -rf  *.o                                 
       
protocol.o: protocol.c
	$(CC) -c -Wall protocol.c                         
server.o: server.c
	$(CC) -c -Wall server.c $(SQL)
accountlist.o: accountlist.c 
	$(CC) -c -Wall accountlist.c $(SQL)                    
database.o: database.c
	$(CC) -c -Wall database.c $(SQL)
client.o: client.c
	$(CC) -c -Wall client.c -pthread -pipe -rdynamic `pkg-config --cflags --libs gtk+-3.0`                
  
clean:
	$(RM) -rf  *.o server client
