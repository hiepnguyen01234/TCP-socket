//https://stackoverflow.com/questions/30607429/gtk3-and-multithreading-replacing-deprecated-functions
#include <sys/poll.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include "accountlist.h"
#include "protocol.h"
#include "database.h"

#define BACKLOG 			100
#define NUM_COMMAND 		31
#define MAX_LENGTH_NAME 	255
#define MAX_LENGTH_TEMP 	1024
#define MAX_LENGTH_PATH		2048

unsigned int timesleep=0;

ListAccount *m_list;

char* arrayCommand[NUM_COMMAND] = {
	"CUSER",	"CPASS",	"USER",		"PASS",		"LOUT",		"GCREAT",
	"GLIST",	"MYGR",		"JOIN",		"ACEPT",	"RJECT",	"JOININFO",
	"GMEM",		"KICK",		"GLEAVE",	"LIST",		"MKD",		"RMD",		
	"UFILE",	"DFILE",	"TRANF",	"CHG",		"CWD",		"EXIT",		
	"MYNOTIF",	"DELE",		"CLRNOTIF",	"DELENOTIF","FORCERMD",	"PWD",		
	"DELEGROUP"
};

//Struct AccClient: structure of account login information from client
typedef struct _AccClient
{
	char nameAcc[MAX_LENGTH_NAME];
	int countPassError;
	int statusLogin;
	struct _AccClient *next;

} AccClient;

//Struct ListAccClient: structure list accounts requires login of client 
typedef struct _ListAccClient
{
	AccClient *head;
	AccClient *current;
} ListAccClient;

//The current storage structure of the client is connecting to the server
struct clientInfo
{
	int fd;
	int phase;
	int phase_data_up;
	int phase_data_down;
	int statusGroupAccess, statusUpload;
	NodeAccount *accountInfo;
	ListAccClient *list_acc_client;
	char rootPath[MAX_LENGTH_PATH];
	char currentGroup[MAX_LENGTH_NAME];
	char currentPath[MAX_LENGTH_PATH];
	char fileUpPath[MAX_LENGTH_QUERY];
	char fileDownPath[MAX_LENGTH_PATH];
	char usernameTemp[MAX_LENGTH_NAME];
	char fileName[MAX_LENGTH_NAME];
};

ListAccClient* newListAccClient();
AccClient* findNodeAccClient(char *inputUsername, ListAccClient *list_A_C);
void freeListAccClient(ListAccClient *list_A_C);
int requestHandler(struct clientInfo *current_client, message *recvMsg, struct clientInfo *c_Info, int num_fd);
int checkCommand(char* arrayCommand[NUM_COMMAND], char *input);
void compressArray(struct pollfd *fd_array, struct clientInfo *c_Info, int *num_fd);
void *sendFile(void *arg);

//==========================================MAIN=======================================================//

int main(int argv, char * args[]) {

	int SERV_PORT, sin_size;
	int listen_sock, connfd;   // file descriptors 
	struct sockaddr_in server; // server's address information 
	struct sockaddr_in client; // client's address information 
							   // Pointer to the received message data from the client

	message *recvMsg;
	unsigned timeout;
	// poll fd array 
	struct pollfd fd_array[FD_SETSIZE];
	// client Info array  
	struct clientInfo *c_Info = (struct clientInfo*)calloc(FD_SETSIZE, sizeof(struct clientInfo));
	// number client current     
	int num_fd = 1, current_size = 0, rv, i, j;
	// variable condition to compress array 
	int statusCompress = 0;
	// Timeout poll is 15 minutes 
	timeout = 15 * 60 * 1000;
	// Set default array info                     
	for (i = 0; i < FD_SETSIZE; i++)
	{
		c_Info[i].fd = -1;
		c_Info[i].phase = NOT_AUTHENTICATED;
		c_Info[i].phase_data_up = WAIT_REQUEST;
		c_Info[i].phase_data_down = WAIT_REQUEST;
		c_Info[i].list_acc_client = newListAccClient();
		c_Info[i].accountInfo = NULL;
	}

	if (argv != 2) {
		printf("\nInvaild Parameters!\n");
		exit(1);
	}
	else {
		// Convert string port to number, port max = 65535
		if (StringToNumber(args[1]) == -1 || StringToNumber(args[1]) > 65535) {
			printf("\nInvaid Port Parameters!\n");
			exit(1);
		}
		SERV_PORT = StringToNumber(args[1]);
	}
	//Step 1: Construct a TCP socket to listen connection request
	// calls socket() 
	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("\nError: ");
		return 0;
	}
	//Step 2: Bind address to socket
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	// Remember htons() from "Conversions" section? =) 
	server.sin_port = htons(SERV_PORT);
	// INADDR_ANY puts your IP address automatically
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	// calls bind() 
	if (bind(listen_sock, (struct sockaddr*)&server, sizeof(server)) == -1) {
		perror("\nError: ");
		return 0;
	}
	//Step 3: Listen request from client
	// calls listen() 
	if (listen(listen_sock, BACKLOG) == -1) {
		perror("\nError: ");
		return 0;
	}
	printf("\n=> Server Started \n=> Server ROOT folder : %s\n", SERVER_ROOT);
	//Create server directory to save file 
	mkdir(SERVER_ROOT, 0777);

	m_list = loadAccount();
	//If read file error -> close server
	if (m_list == NULL)
	{
		printf("=> Server Closed\n");
		exit(1);
	}
	printf("=> All user data in database has been loaded onto the server\n");
	//Step 3: Communicate with client
	memset(fd_array, 0, sizeof(fd_array));
	fd_array[0].fd = listen_sock;
	fd_array[0].events = POLLIN;
	do {
		//Call poll function
		rv = poll(fd_array, num_fd, timeout);
		if (rv < 0) {
			perror("=> Poll error => end server");
			break;
		}
		if (rv == 0) {
			printf("=> Timed out => end server\n");
			break;
		}
		//Loop check revents
		current_size = num_fd;
		for (i = 0; i < current_size; i++)
		{
			if (fd_array[i].revents == 0)continue;
			if (fd_array[i].revents != POLLIN) {
				printf("=> Error! revents = %d\n", fd_array[i].revents);
				close(fd_array[i].fd);
				fd_array[i].fd = -1;
				c_Info[i].fd = -1;
				statusCompress = 1;
			}
			if (fd_array[i].fd == listen_sock) {
				// Accept new connection
				sin_size = sizeof(struct sockaddr_in);
				connfd = accept(listen_sock, (struct sockaddr *)&client, (socklen_t*)&sin_size);
				if (connfd < 0) {
					perror("=> Accept error");
					break;
				}
				printf("------------------------------------------------------------------\n");
				printf("=> Got a connection from %s\n", inet_ntoa(client.sin_addr));
				fd_array[num_fd].fd = connfd;
				fd_array[num_fd].events = POLLIN;
				c_Info[num_fd].fd = connfd;
				c_Info[num_fd].phase = NOT_AUTHENTICATED;
				c_Info[num_fd].phase_data_up = WAIT_REQUEST;
				c_Info[num_fd].phase_data_down = WAIT_REQUEST;
				c_Info[num_fd].statusGroupAccess = 0;
				c_Info[num_fd].statusUpload=0;
				c_Info[num_fd].accountInfo = NULL;
				c_Info[num_fd].list_acc_client = newListAccClient();
				memset(c_Info[num_fd].rootPath, '\0', sizeof(c_Info[num_fd].rootPath));
				memset(c_Info[num_fd].currentGroup,'\0',sizeof(c_Info[num_fd].currentGroup));
				memset(c_Info[num_fd].fileName, '\0', sizeof(c_Info[num_fd].fileName));
				memset(c_Info[num_fd].fileDownPath, '\0', sizeof(c_Info[num_fd].fileDownPath));
				memset(c_Info[num_fd].usernameTemp, '\0', sizeof(c_Info[num_fd].usernameTemp));
				num_fd++;
				if (num_fd == FD_SETSIZE) {
					printf("=> Too many clients\n");
					close(connfd);
				}
			}
			else {
				// Receive message from client
				recvMsg = messageReceive(fd_array[i].fd);
				// Message processing
				if (requestHandler(&c_Info[i], recvMsg, c_Info, current_size) == -1) {
					// Slose connection, reset client info
					close(fd_array[i].fd);
					fd_array[i].fd = -1;
					c_Info[i].fd = -1;
					if (c_Info[i].accountInfo != NULL)
					{
						c_Info[i].accountInfo->statusLogin = 0;
					}
					// Turn on compress
					statusCompress = 1;
				}
				SAFE_DEL(recvMsg);
			}
		}
		// Check variable condition
		if (statusCompress == 1) {
			statusCompress = 0;
			// Compress
			compressArray(fd_array, c_Info, &num_fd);
		}
		// Check tai khoan dang online va gui thong bao
		for (j = 0; j < current_size; j++)
		{
			if (c_Info[j].accountInfo != NULL)
			{
				if (c_Info[j].accountInfo->statusLogin == 1 && c_Info[j].accountInfo->statusNewNotification == 1)
				{
					serverMsgSend(PF_MESSAGE, "M0000", strlen(M0000), M0000, c_Info[j].fd);
					c_Info[j].accountInfo->statusNewNotification = 0;
				}
			}
		}
	} while (1);
	close(listen_sock);
	return 0;
}

/*=====================================================================================================/
int requestHandler(struct clientInfo *current_client, message *recvMsg)

TODO   :	 > Handles client messages sent to the server,
---------------------------------------------------------------------------------------
INPUT  : 	- struct clientInfo *current_client [pointer to a structure containing client information]
- message *recvMsg                  [A pointer to the message received from current client]
OUTPUT : 	+ return 0                          [process complete]
+ return -1                         [error connection disconnected]

/=====================================================================================================*/

int requestHandler(struct clientInfo *current_client, message *recvMsg, struct clientInfo *c_Info, int num_fd)
{
	if (recvMsg != NULL) {
		AccClient *accountSelect = NULL;
		if (checkCommand(arrayCommand, recvMsg->Opcode) == -1)
		{
			switch (current_client->phase)
			{
			case NOT_AUTHENTICATED:
			{
				if (serverMsgSend(PF_MESSAGE, "C7000", strlen(C7000), C7000, current_client->fd) == -1) return -1;
				break;
			}
			case AUTHENTICATING:
			{
				if (serverMsgSend(PF_MESSAGE, "C7100", strlen(C7100), C7100, current_client->fd) == -1) return -1;
				break;
			}
			case AUTHENTICATED:
			{
				if (serverMsgSend(PF_MESSAGE, "C7300", strlen(C7300), C7300, current_client->fd) == -1) return -1;
				break;
			}
			case SIGN_UP:
			{
				if (serverMsgSend(PF_MESSAGE, "C7200", strlen(C7200), C7200, current_client->fd) == -1) return -1;
				break;
			}
			}
		}
		else
		{
			switch (current_client->phase)
			{
			case NOT_AUTHENTICATED:
			{
				//-----------------------------------------------------------------------------------------------------------
				if (strcmp(recvMsg->Opcode, "CUSER") == 0) {
					if (strcmp(recvMsg->Payload, "") == 0)
					{
						if (serverMsgSend(PF_MESSAGE, "C6202", strlen(C6202), C6202, current_client->fd) == -1) return -1;
					}
					else
					{
						if (checkUsername(recvMsg->Payload, m_list) == NULL) {
							//"Username okay, need password to complete the registration"
							if (serverMsgSend(PF_MESSAGE, "C3000", strlen(C3000), C3000, current_client->fd) == -1) return -1;
							memcpy(current_client->usernameTemp, recvMsg->Payload, recvMsg->Lenght);
							current_client->phase = SIGN_UP;
						}
						else {
							//"This username already exists"
							if (serverMsgSend(PF_MESSAGE, "C5000", strlen(C5000), C5000, current_client->fd) == -1) return -1;
						}
					}
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "USER") == 0) {
					current_client->accountInfo = checkUsername(recvMsg->Payload, m_list);
					if (current_client->accountInfo == NULL) {
						//"This account does not exist"
						if (serverMsgSend(PF_MESSAGE, "C5001", strlen(C5001), C5001, current_client->fd) == -1) return -1;
					}
					else {
						if (current_client->accountInfo->info.statusAccount == ST_ACTIVATED) {
							if (current_client->accountInfo->statusLogin == 0)
							{
								//This client is the first login account "username"
								if (findNodeAccClient(current_client->accountInfo->info.userName, current_client->list_acc_client) == NULL) {
									//Create and add account infomation to list accounts requires login of client
									AccClient *account = (AccClient*)malloc(sizeof(AccClient));
									strcpy(account->nameAcc, current_client->accountInfo->info.userName);
									account->countPassError = 0;
									account->statusLogin = 0;
									account->next = current_client->list_acc_client->head;   // add account into first
									current_client->list_acc_client->head = account;  // update head of list account of client
								}
								//"User name okay, need password"
								if (serverMsgSend(PF_MESSAGE, "C3001", strlen(C3001), C3001, current_client->fd) == -1) return -1;
								current_client->phase = AUTHENTICATING;
							}
							// Tai khoan dang duoc dang nhap o mot noi khac
							else if (current_client->accountInfo->statusLogin == 1)
							{
								if (serverMsgSend(PF_MESSAGE, "C5003", strlen(C5003), C5003, current_client->fd) == -1) return -1;
							}
						}
						else {
							//"This account has been blocked"
							char temp[MAX_LENGTH_TEMP];
							sprintf(temp, "Sorry. Account %s has been blocked", current_client->accountInfo->info.userName);
							if (serverMsgSend(PF_MESSAGE, "C5002", sizeof(temp), temp, current_client->fd) == -1) return -1;
						}
					}
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "PASS") == 0) {
					//"You need to send us your username before sending your password"
					if (serverMsgSend(PF_MESSAGE, "C6000", strlen(C6000), C6000, current_client->fd) == -1) return -1;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "LOUT") == 0) {
					//"You are not allowed to sign out because you are not logged in"
					if (serverMsgSend(PF_MESSAGE, "C6001", strlen(C6001), C6001, current_client->fd) == -1) return -1;
				}
				//-----------------------------------------------------------------------------------------------------------
				else {
					//"You are not logged in, your request could not be accepted" 
					if (serverMsgSend(PF_MESSAGE, "C6002", strlen(C6002), C6002, current_client->fd) == -1) return -1;
				}
				//-----------------------------------------------------------------------------------------------------------
				break;
			}

			case AUTHENTICATING:
			{
				//-----------------------------------------------------------------------------------------------------------
				if (strcmp(recvMsg->Opcode, "PASS") == 0) {
					if (current_client->accountInfo->info.statusAccount == ST_ACTIVATED) { //check the password
						int checkPass = checkPassword(recvMsg->Payload, current_client->accountInfo);
						//Get the account information is authentic from list account client
						accountSelect = findNodeAccClient(current_client->accountInfo->info.userName, current_client->list_acc_client);
						if (checkPass == 0) { //"Logged in successfully"
							current_client->phase = AUTHENTICATED;
							accountSelect->countPassError = 0;
							accountSelect->statusLogin = 1;
							char temp[MAX_LENGTH_TEMP];
							sprintf(current_client->rootPath, "%s/%s", SERVER_ROOT, current_client->accountInfo->info.userName);
							sprintf(current_client->currentPath, "%s/%s", SERVER_ROOT, current_client->accountInfo->info.userName);
							current_client->statusGroupAccess = 0;
							sprintf(temp, "%s. Hello %s", C2100, current_client->accountInfo->info.userName);
							if (serverMsgSend(PF_MESSAGE, "C2100", sizeof(temp), temp, current_client->fd) == -1) return -1;
						}
						else if (checkPass == -1) { //"Wrong password"
							current_client->phase = NOT_AUTHENTICATED;
							char temp[MAX_LENGTH_TEMP];
							accountSelect->countPassError += 1;
							if (accountSelect->countPassError <= 2) { //wrong password less than 2 times. send a warning
								sprintf(temp, "%s. Wrong %d times the account will be blocked", C5100, 3 - accountSelect->countPassError);
								if (serverMsgSend(PF_MESSAGE, "C5100", sizeof(temp), temp, current_client->fd) == -1) return -1;
							}
							else { //"wrong password more than 3 times. Your account has been blocked"
								current_client->accountInfo->info.statusAccount = ST_BLOCKED;
								blockAccount(current_client->accountInfo->info.userName);
								current_client->phase = NOT_AUTHENTICATED;

								if (serverMsgSend(PF_MESSAGE, "C5101", strlen(C5101), C5101, current_client->fd) == -1) return -1;
							}
						}
					}
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "USER") == 0) {
					//"you are authenticating another account"
					if (serverMsgSend(PF_MESSAGE, "C6101", strlen(C6101), C6101, current_client->fd) == -1) return -1;
					//current_client->phase = NOT_AUTHENTICATED;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "LOUT") == 0) {
					//"Sign out error because you are not signed in"
					if (serverMsgSend(PF_MESSAGE, "C6102", strlen(C6102), C6102, current_client->fd) == -1) return -1;
				}
				//-----------------------------------------------------------------------------------------------------------
				else {
					//"You are not logged in, your request could not be accepted" 
					if (serverMsgSend(PF_MESSAGE, "C6100", strlen(C6100), C6100, current_client->fd) == -1) return -1;
				}
				break;
			}
			case SIGN_UP:
			{
				//-----------------------------------------------------------------------------------------------------------
				if (strcmp(recvMsg->Opcode, "CPASS") == 0) {
					if (strcmp(recvMsg->Payload, "") == 0)
					{
						if (serverMsgSend(PF_MESSAGE, "C6201", strlen(C6201), C6201, current_client->fd) == -1) return -1;
						current_client->phase = NOT_AUTHENTICATED;
					}
					else
					{
						addAccount(m_list, current_client->usernameTemp, recvMsg->Payload);
						memset(current_client->usernameTemp, '\0', sizeof(current_client->usernameTemp));
						current_client->phase = NOT_AUTHENTICATED;
						if (serverMsgSend(PF_MESSAGE, "C2200", strlen(C2200), C2200, current_client->fd) == -1) return -1;
					}
				}
				else {
					//"You are registering an account, other request is not accepted"
					if (serverMsgSend(PF_MESSAGE, "C6200", strlen(C6200), C6200, current_client->fd) == -1) return -1;
				}
				//-----------------------------------------------------------------------------------------------------------
				break;
			}
			case AUTHENTICATED:
			{
				//-----------------------------------------------------------------------------------------------------------
				if (strcmp(recvMsg->Opcode, "USER") == 0) {
					//"Logged in, you are not allowed to login to another account"
					if (serverMsgSend(PF_MESSAGE, "C6300", strlen(C6300), C6300, current_client->fd) == -1) return -1;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "LOUT") == 0) {
					//"Successful logout"
					current_client->phase = NOT_AUTHENTICATED;
					current_client->accountInfo->statusLogin = 0;
					current_client->list_acc_client->current = current_client->list_acc_client->head;
					while (current_client->list_acc_client->current != NULL) {
						if (current_client->list_acc_client->current->statusLogin == 1) {
							accountSelect = current_client->list_acc_client->current;
							break;
						}
						current_client->list_acc_client->current = current_client->list_acc_client->current->next;
					}
					accountSelect->statusLogin = 0;
					char temp[MAX_LENGTH_TEMP];
					sprintf(temp, "%s. Goodbye %s", C2311, accountSelect->nameAcc);

					if (serverMsgSend(PF_MESSAGE, "C2311", sizeof(temp), temp, current_client->fd) == -1) return -1;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "GCREAT") == 0) {
					timesleep = 10000;
					if (recvMsg->Lenght == 0)
					{
						if (serverMsgSend(PF_MESSAGE, "C6312", strlen(C6312), C6312, current_client->fd) == -1) return -1;
					}
					else
					{
						int result = createGroup(current_client->accountInfo->info.userName, recvMsg->Payload);
						if (result == 0)
						{
							if (serverMsgSend(PF_MESSAGE, "C2300", strlen(C2300), C2300, current_client->fd) == -1) return -1;
						}
						else if (result == -3)
						{
							if (serverMsgSend(PF_MESSAGE, "C5300", strlen(C5300), C5300, current_client->fd) == -1) return -1;
						}
						break;
					}
					timesleep = 0;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "GLIST") == 0) {
					MYSQL *conn;
					MYSQL_RES *resU, *res;
					MYSQL_ROW rowU, row;
					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "select* from users where username = '%s'", current_client->accountInfo->info.userName);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					resU = mysql_store_result(conn);
					rowU = mysql_fetch_row(resU);
					memset(query, '\0', MAX_LENGTH_QUERY);
					sprintf(query, "select group_name,users.status from groups, users WHERE users.id = groups.owner_id AND groups.id NOT IN (SELECT group_id from group_users where user_id = %s )", rowU[0]);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					res = mysql_use_result(conn);
					timesleep = 10000;
					if (serverMsgSend(PF_MESSAGE, "C1308", strlen(C1301), C1301, current_client->fd) == -1) return -1;
					while ((row = mysql_fetch_row(res)) != NULL) {
						if(StringToNumber(row[1])==ST_ACTIVATED)
						{
							if (publicgroupSend(PF_PUBLIC_GROUP, R_UNKNOWN, row[0],ST_CAN_JOIN, current_client->fd) == -1)return -1;
						}
						else if(StringToNumber(row[1])==ST_BLOCKED)
						{
							if (publicgroupSend(PF_PUBLIC_GROUP, R_UNKNOWN, row[0],ST_CANNOT_JOIN, current_client->fd) == -1)return -1;
						}
					}
					timesleep = 0;
					mysql_free_result(resU);
					mysql_free_result(res);
					closeConnection(conn);
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "JOININFO") == 0) {
					MYSQL *conn;
					MYSQL_RES *res;
					MYSQL_ROW row;
					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "SELECT user_requests.id,users1.username, groups.id,groups.group_name,users2.id,users2.username,user_requests.time_create from users as users1,users as users2,groups,user_requests WHERE users1.id = groups.owner_id AND user_requests.group_id = groups.id AND user_requests.joiner_id = users2.id AND users1.username ='%s' ORDER BY user_requests.id DESC", current_client->accountInfo->info.userName);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					res = mysql_use_result(conn);
					timesleep = 10000;
					if (serverMsgSend(PF_MESSAGE, "C1305", strlen(C1305), C1305, current_client->fd) == -1) return -1;
					while ((row = mysql_fetch_row(res)) != NULL)
					{
						if (joinNotiSend(PF_JOIN_INFOMATION, row[0], row[1], row[3], row[5], row[6], current_client->fd) == -1) return -1;
					}
					timesleep = 0;
					mysql_free_result(res);
					closeConnection(conn);
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "MYNOTIF") == 0) {
					MYSQL *conn;
					MYSQL_RES *res;
					MYSQL_ROW row;
					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "SELECT user_notification.id,user_notification.content,user_notification.time_create from users,user_notification WHERE users.id = user_notification.user_id AND users.username = '%s' ORDER BY user_notification.id DESC", current_client->accountInfo->info.userName);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					res = mysql_use_result(conn);
					timesleep = 10000;
					if (serverMsgSend(PF_MESSAGE, "C1307", strlen(C1307), C1307, current_client->fd) == -1) return -1;
					while ((row = mysql_fetch_row(res)) != NULL)
					{
						if (notificationSend(PF_NOTIFICATION, row[0], row[1], row[2], current_client->fd) == -1) return -1;
					}
					timesleep = 0;
					mysql_free_result(res);
					closeConnection(conn);
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "DELENOTIF") == 0) {
					MYSQL *conn;
					MYSQL_RES *res;
					MYSQL_ROW row;
					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "select * from user_notification,users where user_notification.user_id = users.id AND username = '%s' AND user_notification.id = %s", current_client->accountInfo->info.userName, recvMsg->Payload);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					res = mysql_store_result(conn);
					row = mysql_fetch_row(res);
					timesleep = 10000;
					if (row != NULL)
					{
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query, "delete from user_notification where user_notification.id = %s", recvMsg->Payload);
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						if (serverMsgSend(PF_MESSAGE, "C2320", strlen(C2320), C2320, current_client->fd) == -1) return -1;
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6315", strlen(C6315), C6315, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
					mysql_free_result(res);
					closeConnection(conn);
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "CLRNOTIF") == 0) {
					MYSQL *conn;
					MYSQL_RES *resU;
					MYSQL_ROW rowU;
					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "select id from users where username = '%s'", current_client->accountInfo->info.userName);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					resU = mysql_store_result(conn);
					rowU = mysql_fetch_row(resU);
					memset(query, '\0', MAX_LENGTH_QUERY);
					sprintf(query, "delete from user_notification WHERE user_id = %s", rowU[0]);
					timesleep = 10000;
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					if (serverMsgSend(PF_MESSAGE, "C2318", strlen(C2318), C2318, current_client->fd) == -1) return -1;
					timesleep = 0;
					mysql_free_result(resU);
					closeConnection(conn);
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "GMEM") == 0) {
					
					MYSQL *conn;
					MYSQL_RES *resG, *res;
					MYSQL_ROW rowG, row;
					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "SELECT users.username,groups.group_name from users,group_users,groups WHERE users.id = group_users.user_id AND group_users.group_id = groups.id AND users.username = '%s' AND groups.group_name = '%s'", current_client->accountInfo->info.userName, recvMsg->Payload);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					resG = mysql_store_result(conn);
					rowG = mysql_fetch_row(resG);
					if (rowG != NULL)
					{
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query, "SELECT u1.username,users.username FROM users as u1,groups,group_users,users WHERE u1.id = groups.owner_id AND groups.id = group_users.group_id AND group_users.user_id = users.id AND groups.group_name = '%s'", recvMsg->Payload);
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						res = mysql_use_result(conn);
						timesleep = 10000;
						if (serverMsgSend(PF_MESSAGE, "C1306", strlen(C1306), C1306, current_client->fd) == -1) return -1;
						while ((row = mysql_fetch_row(res)) != NULL)
						{
							if (strcmp(row[0], row[1]) == 0)
							{
								if (memberSend(PF_MEMBER, R_ADMIN, row[1], recvMsg->Payload, current_client->fd) == -1)return -1;
							}
							else
							{
								if (memberSend(PF_MEMBER, R_MEMBER, row[1], recvMsg->Payload, current_client->fd) == -1)return -1;
							}
						}
						timesleep = 0;
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6310", strlen(C6310), C6310, current_client->fd) == -1) return -1;
					}
					mysql_free_result(resG);
					mysql_free_result(res);
					closeConnection(conn);
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "DELEGROUP") == 0) {
					MYSQL *conn,*conn1;
					MYSQL_RES *res;
					MYSQL_ROW row;
					int check=0;
					char group_id[10];
					char *temp;
					conn = initConnection();
					conn1 = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "SELECT u1.id,group_users.group_id,group_users.user_id,u2.username FROM users as u1,group_users,groups, users as u2 WHERE u1.id = groups.owner_id AND group_users.group_id = groups.id AND group_users.user_id = u2.id AND u1.username ='%s' AND groups.group_name = '%s' ",current_client->accountInfo->info.userName,recvMsg->Payload);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					res = mysql_use_result(conn);
					while ((row = mysql_fetch_row(res)) != NULL) {
						check =1;
						sprintf(group_id,"%s",row[1]);
						if(strcmp(row[0],row[2])!=0)
						{
							memset(query, '\0', MAX_LENGTH_QUERY);
							sprintf(query, "INSERT INTO user_notification (user_id,type,content,time_create) VALUES (%s,%d,'The %s group has been deleted','%s')",row[2], NOTIFI_NORMAL, recvMsg->Payload, currentTime());
							if (mysql_query(conn1, query))
							{
								fprintf(stderr, "%s\n", mysql_error(conn1));
							}
							NodeAccount* node = checkUsername(row[3], m_list);
							if (node != NULL)
							{
								node->statusNewNotification = 1;
							}
						}
					}
					if(check==1)
					{
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query,"DELETE FROM group_users WHERE group_users.group_id = %s",group_id);
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query,"DELETE FROM user_requests WHERE group_id = %s",group_id);
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query,"DELETE FROM groups WHERE id = %s",group_id);
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						temp = (char*)calloc(1024,sizeof(char));
						sprintf(temp,"rm -rf \"%s/%s/%s\"",SERVER_ROOT,current_client->accountInfo->info.userName,recvMsg->Payload);
						system(temp);
						if (serverMsgSend(PF_MESSAGE, "C2321", strlen(C2321), C2321, current_client->fd) == -1) return -1;
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6317", strlen(C6317), C6317, current_client->fd) == -1) return -1;
					}
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "KICK") == 0) {

					MYSQL *conn;
					MYSQL_RES *res;
					MYSQL_ROW row;
					int i, count, error = 0;
					char *group_name;
					char *group_member;

					for (i = 0; i<strlen(recvMsg->Payload); i++)
					{
						if (recvMsg->Payload[i] == ':')break;
						if (i == strlen(recvMsg->Payload) - 1)error = 1;
						count++;
					}
					if (error == 1)
					{
					}
					else
					{
						group_name = (char*)calloc(MAX_LENGTH_NAME, sizeof(char));
						group_member = (char*)calloc(MAX_LENGTH_NAME, sizeof(char));
						memcpy(group_name, recvMsg->Payload, count);
						group_name[count] = '\0';
						memcpy(group_member, recvMsg->Payload + count + 1, strlen(recvMsg->Payload) - count - 1);
						group_member[strlen(recvMsg->Payload) - count - 1] = '\0';
					}

					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "SELECT u1.username,users.username,group_users.id,u1.id FROM users as u1,group_users,groups,users WHERE group_users.group_id = groups.id AND groups.owner_id = users.id AND users.username = '%s' AND groups.group_name ='%s' AND u1.id = group_users.user_id AND u1.username ='%s'", current_client->accountInfo->info.userName, group_name, group_member);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					res = mysql_store_result(conn);
					row = mysql_fetch_row(res);
					timesleep = 10000;
					if (row != NULL && strcmp(row[0], row[1]) != 0)
					{
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query, "delete from group_users where id = %s", row[2]);
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query, "INSERT INTO user_notification (user_id,type,content,time_create) VALUES (%s,%d,'Kicked out of %s','%s')", row[3], NOTIFI_NORMAL, group_name, currentTime());
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						NodeAccount* node = checkUsername(group_member, m_list);
						if (node != NULL)
						{
							node->statusNewNotification = 1;
						}
						if (serverMsgSend(PF_MESSAGE, "C2317", strlen(C2317), C2317, current_client->fd) == -1) return -1;
						
						int k;
						for(k=0;k<num_fd;k++)
						{
							if(c_Info[k].accountInfo!=NULL)
							{
								if(strcmp(c_Info[k].accountInfo->info.userName,group_member)==0 && strcmp(c_Info[k].currentGroup,group_name)==0)
								{
									c_Info[k].statusGroupAccess=0;
								}
							}
						}
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6311", strlen(C6311), C6311, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
					SAFE_DEL(group_name);
					SAFE_DEL(group_member);
					mysql_free_result(res);
					closeConnection(conn);
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "MYGR") == 0) {
					MYSQL *conn;
					MYSQL_RES *resU, *res;
					MYSQL_ROW rowU, row;
					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "select* from users where username = '%s'", current_client->accountInfo->info.userName);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					resU = mysql_store_result(conn);
					rowU = mysql_fetch_row(resU);
					memset(query, '\0', MAX_LENGTH_QUERY);
					sprintf(query, "select group_name from groups where owner_id = %s", rowU[0]);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					res = mysql_use_result(conn);
					timesleep = 10000;
					if (serverMsgSend(PF_MESSAGE, "C1301", strlen(C1301), C1301, current_client->fd) == -1) return -1;
					while ((row = mysql_fetch_row(res)) != NULL) {
						if (groupSend(PF_MY_GROUP, R_ADMIN, row[0], current_client->fd) == -1)return -1;
					}
					memset(query, '\0', MAX_LENGTH_QUERY);
					sprintf(query, "SELECT group_name FROM groups,group_users WHERE groups.id = group_users.group_id AND group_users.user_id = %s AND groups.id NOT IN (SELECT id FROM groups WHERE groups.owner_id = %s )", rowU[0], rowU[0]);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					res = mysql_use_result(conn);
					while ((row = mysql_fetch_row(res)) != NULL) {
						if (groupSend(PF_MY_GROUP, R_MEMBER, row[0], current_client->fd) == -1)return -1;
					}
					timesleep = 0;
					mysql_free_result(resU);
					mysql_free_result(res);
					closeConnection(conn);
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "JOIN") == 0) {
					int checkResult = 0;
					char id_group[10];
					char id_admin[10];
					char username_admin[MAX_LENGTH_NAME];
					MYSQL *conn;
					MYSQL_RES *resU, *resR, *res;
					MYSQL_ROW rowU, rowR, row;
					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "select id,username from users where username = '%s'", current_client->accountInfo->info.userName);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					resU = mysql_store_result(conn);
					rowU = mysql_fetch_row(resU);
					memset(query, '\0', MAX_LENGTH_QUERY);
					sprintf(query, "select groups.id,groups.owner_id,users.username,groups.group_name from users,groups WHERE users.id = groups.owner_id AND users.status =%d AND groups.id NOT IN (SELECT group_id from group_users where user_id = %s )",ST_ACTIVATED,rowU[0]);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					res = mysql_use_result(conn);
					while ((row = mysql_fetch_row(res)) != NULL) {
						if (strcmp(row[3], recvMsg->Payload) == 0)
						{
							sprintf(id_group, "%s", row[0]);
							sprintf(id_admin, "%s", row[1]);
							sprintf(username_admin, "%s", row[2]);
							checkResult = 1;
						}
					}
					timesleep = 10000;
					if (checkResult == 0)
					{
						if (serverMsgSend(PF_MESSAGE, "C5308", strlen(C5308), C5308, current_client->fd) == -1) return -1;
					}
					else if (checkResult == 1)
					{
						//======================================================================================
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query, "select id from user_requests where joiner_id = %s AND group_id = %s", rowU[0], id_group);
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						resR = mysql_store_result(conn);
						rowR = mysql_fetch_row(resR);
						if (rowR == NULL)
						{
							memset(query, '\0', MAX_LENGTH_QUERY);
							sprintf(query, "insert into user_requests (group_id, joiner_id, time_create) values (%s, %s, '%s')", id_group, rowU[0], currentTime());
							if (mysql_query(conn, query))
							{
								fprintf(stderr, "%s\n", mysql_error(conn));
							}
							memset(query, '\0', MAX_LENGTH_QUERY);
							sprintf(query, "INSERT INTO user_notification (user_id,type,content,time_create) VALUES (%s,%d,'%s wants to join your group','%s')", id_admin, NOTIFI_JOIN, rowU[1], currentTime());
							if (mysql_query(conn, query))
							{
								fprintf(stderr, "%s\n", mysql_error(conn));
							}
							NodeAccount* node = checkUsername(username_admin, m_list);
							if (node != NULL)
							{
								node->statusNewNotification = 1;
							}
							// Join success
							if (serverMsgSend(PF_MESSAGE, "C2315", strlen(C2315), C2315, current_client->fd) == -1) return -1;
						}
						else
						{
							if (serverMsgSend(PF_MESSAGE, "C6307", strlen(C6307), C6307, current_client->fd) == -1) return -1;
						}
					}
					timesleep = 0;
					mysql_free_result(resU);
					mysql_free_result(resR);
					mysql_free_result(res);
					closeConnection(conn);
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "ACEPT") == 0) {

					MYSQL *conn;
					MYSQL_RES *res;
					MYSQL_ROW row;
					int checkResult = 0;
					char id_group[10];
					char id_user[10];
					char *name_user;
					char *name_group;

					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "SELECT user_requests.id,users1.username, groups.id,groups.group_name,users2.id,users2.username from users as users1,users as users2,groups,user_requests WHERE users1.id = groups.owner_id AND user_requests.group_id = groups.id AND user_requests.joiner_id = users2.id ");
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					res = mysql_use_result(conn);
					while ((row = mysql_fetch_row(res)) != NULL)
					{
						if (strcmp(row[1], current_client->accountInfo->info.userName) == 0 && strcmp(recvMsg->Payload, row[0]) == 0)
						{
							name_user = (char*)calloc(MAX_LENGTH_NAME, sizeof(char));
							name_group = (char*)calloc(MAX_LENGTH_NAME, sizeof(char));
							sprintf(id_group, "%s", row[2]);
							sprintf(id_user, "%s", row[4]);
							sprintf(name_user, "%s", row[5]);
							sprintf(name_group, "%s", row[3]);
							checkResult = 1;
							break;
						}
					}
					closeConnection(conn);
					conn = initConnection();
					timesleep = 10000;
					if (checkResult == 1)
					{
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query, "delete from user_requests where id = %s", recvMsg->Payload);
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query, "insert into group_users (group_id,user_id) values (%s,%s)", id_group, id_user);
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query, "INSERT INTO user_notification (user_id,type,content,time_create) VALUES (%s,%d,'Accept you to join %s','%s')", id_user, NOTIFI_NORMAL, name_group, currentTime());
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						NodeAccount* node = checkUsername(name_user, m_list);
						if (node != NULL)
						{
							node->statusNewNotification = 1;
						}
						if (serverMsgSend(PF_MESSAGE, "C2302", strlen(C2302), C2302, current_client->fd) == -1) return -1;
					}
					else if (checkResult == 0)
					{
						if (serverMsgSend(PF_MESSAGE, "C6308", strlen(C6308), C6308, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
					mysql_free_result(res);
					closeConnection(conn);
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "RJECT") == 0) {

					MYSQL *conn;
					MYSQL_RES *res;
					MYSQL_ROW row;
					int checkResult = 0;
					char id_user[10];
					char *name_user;
					char *name_group;

					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "SELECT user_requests.id,users1.username, groups.id,groups.group_name,users2.id,users2.username from users as users1,users as users2,groups,user_requests WHERE users1.id = groups.owner_id AND user_requests.group_id = groups.id AND user_requests.joiner_id = users2.id ");
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					res = mysql_use_result(conn);
					while ((row = mysql_fetch_row(res)) != NULL)
					{
						if (strcmp(row[1], current_client->accountInfo->info.userName) == 0 && strcmp(recvMsg->Payload, row[0]) == 0)
						{
							name_user = (char*)calloc(MAX_LENGTH_NAME, sizeof(char));
							name_group = (char*)calloc(MAX_LENGTH_NAME, sizeof(char));
							sprintf(name_user, "%s", row[5]);
							sprintf(id_user, "%s", row[4]);
							sprintf(name_group, "%s", row[3]);
							checkResult = 1;
							break;
						}
					}
					closeConnection(conn);
					conn = initConnection();
					timesleep = 10000;
					if (checkResult == 1)
					{
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query, "delete from user_requests where id = %s", recvMsg->Payload);
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query, "INSERT INTO user_notification (user_id,type,content,time_create) VALUES (%s,%d,'Deny you to join %s','%s')", id_user, NOTIFI_NORMAL, name_group, currentTime());
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						NodeAccount* node = checkUsername(name_user, m_list);
						if (node != NULL)
						{
							node->statusNewNotification = 1;
						}
						if (serverMsgSend(PF_MESSAGE, "C2303", strlen(C2303), C2303, current_client->fd) == -1) return -1;
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6309", strlen(C6309), C6309, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
					mysql_free_result(res);
					closeConnection(conn);
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "GLEAVE") == 0) {
					int checkResult = 0;
					char id_group[10];
					MYSQL *conn;
					MYSQL_RES *resU, *res;
					MYSQL_ROW rowU, row;
					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "select id from users where username = '%s'", current_client->accountInfo->info.userName);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					resU = mysql_store_result(conn);
					rowU = mysql_fetch_row(resU);
					memset(query, '\0', MAX_LENGTH_QUERY);
					sprintf(query, "SELECT group_name,group_id FROM groups,group_users WHERE groups.id = group_users.group_id AND group_users.user_id = %s AND groups.id NOT IN (SELECT id FROM groups WHERE groups.owner_id = %s )", rowU[0], rowU[0]);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					res = mysql_use_result(conn);
					while ((row = mysql_fetch_row(res)) != NULL) {
						if (strcmp(row[0], recvMsg->Payload) == 0)
						{
							sprintf(id_group, "%s", row[1]);
							checkResult = 1;
						}
					}
					timesleep = 10000;
					if (checkResult == 1)
					{
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query, "delete from group_users where user_id = %s AND group_id = %s", rowU[0], id_group);
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						if(strcmp(recvMsg->Payload,current_client->currentGroup)==0)
						{
							current_client->statusGroupAccess=0;
						}
						if (serverMsgSend(PF_MESSAGE, "C2316", strlen(C2316), C2316, current_client->fd) == -1) return -1;
					}
					else if (checkResult == 0)
					{
						if (serverMsgSend(PF_MESSAGE, "C5309", strlen(C5309), C5309, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
					mysql_free_result(resU);
					mysql_free_result(res);
					closeConnection(conn);
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "LIST") == 0) {
					char *temp = (char*)calloc(MAX_LENGTH_TEMP, sizeof(char));
					char *temp_file_path = (char*)calloc(MAX_LENGTH_PATH, sizeof(char));
					struct dirent *de;
					DIR *dr;
					if (current_client->statusGroupAccess == 1)
					{
						dr = opendir(current_client->currentPath);
						if (dr == NULL)
						{
							sprintf(temp, "%s. %s", C5306, strerror(errno));
							if (serverMsgSend(PF_MESSAGE, "C5306", strlen(temp), temp, current_client->fd) == -1) return -1;
							SAFE_DEL(temp);
						}
						else
						{
							timesleep = 10000;
							if (serverMsgSend(PF_MESSAGE, "C1302", strlen(C1302), C1302, current_client->fd) == -1) return -1;
							while ((de = readdir(dr)) != NULL)
							{	
								if (de->d_type == 4) {
									directSend(PF_DIRECTORY, de->d_type, de->d_name, 0, current_client->fd);
								}
							}
							dr = opendir(current_client->currentPath);
							while ((de = readdir(dr)) != NULL)
							{
								memset(temp_file_path, '\0', MAX_LENGTH_PATH);
								sprintf(temp_file_path, "%s/%s", current_client->currentPath, de->d_name);
								if (de->d_type == 8) {
									directSend(PF_DIRECTORY, de->d_type, de->d_name, SizeFile(temp_file_path), current_client->fd);
								}
							}
							timesleep = 0;
						}
						closedir(dr);
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6305", strlen(C6305), C6305, current_client->fd) == -1) return -1;
					}
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "MKD") == 0) {
					timesleep = 10000;
					if (current_client->statusGroupAccess == 1)
					{
						if (recvMsg->Lenght == 0)
						{
							if (serverMsgSend(PF_MESSAGE, "C6314", strlen(C6314), C6314, current_client->fd) == -1) return -1;
						}
						else
						{
							char *temp = (char*)calloc(MAX_LENGTH_TEMP, sizeof(char));
							sprintf(temp, "%s/%s", current_client->currentPath, recvMsg->Payload);
							if (mkdir(temp, 0777) == -1)
							{
								memset(temp, '\0', MAX_LENGTH_TEMP);
								sprintf(temp, "%s. %s", C5306, strerror(errno));
								if (serverMsgSend(PF_MESSAGE, "C5306", strlen(temp), temp, current_client->fd) == -1) return -1;
								SAFE_DEL(temp);
							}
							else
							{
								if (serverMsgSend(PF_MESSAGE, "C2305", strlen(C2305), C2305, current_client->fd) == -1) return -1;
							}
						}
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6305", strlen(C6305), C6305, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "RMD") == 0) {
					timesleep = 10000;
					if (current_client->statusGroupAccess == 1)
					{
						int lenght = strlen(current_client->rootPath);
						char *temp = (char*)calloc(MAX_LENGTH_TEMP, sizeof(char));
						memcpy(temp, current_client->currentPath, lenght);
						if (strcmp(temp, current_client->rootPath) == 0)
						{
							memset(temp, '\0', MAX_LENGTH_TEMP);
							sprintf(temp, "%s/%s", current_client->currentPath, recvMsg->Payload);
							if (rmdir(temp) == -1)
							{
								memset(temp, '\0', MAX_LENGTH_TEMP);
								sprintf(temp, "%s. %s", C5306, strerror(errno));

								if (serverMsgSend(PF_MESSAGE, "C5306", strlen(temp), temp, current_client->fd) == -1) return -1;
							}
							else
							{
								if (serverMsgSend(PF_MESSAGE, "C2306", strlen(C2306), C2306, current_client->fd) == -1) return -1;
							}
						}
						else
						{
							if (serverMsgSend(PF_MESSAGE, "C6301", strlen(C6301), C6301, current_client->fd) == -1) return -1;
						}
						SAFE_DEL(temp);
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6305", strlen(C6305), C6305, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "FORCERMD") == 0) {
					timesleep = 10000;
					if (current_client->statusGroupAccess == 1)
					{
						int lenght = strlen(current_client->rootPath);
						char *temp = (char*)calloc(MAX_LENGTH_TEMP, sizeof(char));
						memcpy(temp, current_client->currentPath, lenght);
						if (strcmp(temp, current_client->rootPath) == 0)
						{
							memset(temp, '\0', MAX_LENGTH_TEMP);
							sprintf(temp, "rm -rf \"%s/%s\"", current_client->currentPath, recvMsg->Payload);
							printf("%s\n",temp);
							system(temp);
							if (serverMsgSend(PF_MESSAGE, "C2306", strlen(C2306), C2306, current_client->fd) == -1) return -1;
						}
						else
						{
							if (serverMsgSend(PF_MESSAGE, "C6301", strlen(C6301), C6301, current_client->fd) == -1) return -1;
						}
						SAFE_DEL(temp);
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6305", strlen(C6305), C6305, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "DELE") == 0) {
					timesleep = 10000;
					if (current_client->statusGroupAccess == 1)
					{
						int lenght = strlen(current_client->rootPath);
						char *temp = (char*)calloc(MAX_LENGTH_TEMP, sizeof(char));
						memcpy(temp, current_client->currentPath, lenght);
						if (strcmp(temp, current_client->rootPath) == 0)
						{
							memset(temp, '\0', MAX_LENGTH_TEMP);
							sprintf(temp, "%s/%s", current_client->currentPath, recvMsg->Payload);
							if (remove(temp) == -1)
							{
								memset(temp, '\0', MAX_LENGTH_TEMP);
								sprintf(temp, "%s. %s", C5306, strerror(errno));
								if (serverMsgSend(PF_MESSAGE, "C5306", strlen(temp), temp, current_client->fd) == -1) return -1;
							}
							else
							{
								if (serverMsgSend(PF_MESSAGE, "C2307", strlen(C2307), C2307, current_client->fd) == -1) return -1;
							}
						}
						else
						{
							if (serverMsgSend(PF_MESSAGE, "C6301", strlen(C6301), C6301, current_client->fd) == -1) return -1;
						}
						SAFE_DEL(temp);
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6305", strlen(C6305), C6305, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "UFILE") == 0) {
					timesleep = 10000;
					if (current_client->statusGroupAccess == 1)
					{
						if (recvMsg->Lenght == 0)
						{
							if (serverMsgSend(PF_MESSAGE, "C6302", strlen(C6302), C6302, current_client->fd) == -1) return -1;
						}
						else
						{
							if (current_client->phase_data_up == WAIT_REQUEST)
							{
								char *temp = (char*)calloc(MAX_LENGTH_TEMP, sizeof(char));
								sprintf(temp, "%s/%s", current_client->currentPath, recvMsg->Payload);
								FILE *my_file = fopen(temp, "rb");
								if (my_file != NULL)
								{
									if (serverMsgSend(PF_MESSAGE, "C5304", strlen(C5304), C5304, current_client->fd) == -1) return -1;
								}
								else
								{
									if (serverMsgSend(PF_MESSAGE, "C2308", strlen(C2308), C2308, current_client->fd) == -1) return -1;
									memset(current_client->fileName, '\0', sizeof(current_client->fileName));
									memset(current_client->fileUpPath, '\0', sizeof(current_client->fileUpPath));
									sprintf(current_client->fileName, "%s", recvMsg->Payload);
									sprintf(current_client->fileUpPath, "%s/%s", current_client->currentPath, recvMsg->Payload);
									printf("File up name : %s\n", current_client->fileName);
									printf("File up path : %s\n", current_client->fileUpPath);
									current_client->statusUpload=1;
									current_client->phase_data_up = RECEIVE_FILE;
								}
							}
							else
							{
								if (serverMsgSend(PF_MESSAGE, "C6313", strlen(C6313), C6313, current_client->fd) == -1) return -1;
							}
						}
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6305", strlen(C6305), C6305, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "DFILE") == 0) {
					timesleep = 10000;
					if (current_client->statusGroupAccess == 1)
					{
						FILE *my_file;
						char *temp = (char*)calloc(MAX_LENGTH_TEMP, sizeof(char));
						memset(current_client->fileDownPath, '\0', sizeof(current_client->fileDownPath));
						sprintf(current_client->fileDownPath, "%s/%s", current_client->currentPath, recvMsg->Payload);
						my_file = fopen(current_client->fileDownPath, "r");
						if (my_file == NULL)
						{
							memset(temp, '\0', MAX_LENGTH_TEMP);
							sprintf(temp, "%s. %s", C5306, strerror(errno));
							if (serverMsgSend(PF_MESSAGE, "C5306", strlen(temp), temp, current_client->fd) == -1) return -1;
							memset(current_client->fileDownPath, '\0', sizeof(current_client->fileDownPath));
							SAFE_DEL(temp);
						}
						else
						{
							fclose(my_file);
							if (current_client->phase_data_down == WAIT_REQUEST)
							{
								int k,check=0;
								for (k=0;k<num_fd;k++)
								{
									if(strcmp(current_client->fileDownPath,c_Info[k].fileUpPath)==0)
									{
										if (serverMsgSend(PF_MESSAGE, "C6316", strlen(C6316), C6316, current_client->fd) == -1) return -1;
										memset(current_client->fileDownPath, '\0', sizeof(current_client->fileDownPath));
										check=1;
										break;
									}
								}
								if(check==0)
								{
									if (serverMsgSend(PF_MESSAGE, "C2309", strlen(C2309), C2309, current_client->fd) == -1) return -1;
									pthread_t tid;
									current_client->phase_data_down = SEND_FILE;
									pthread_create(&tid, NULL, &sendFile, current_client);
								}
							}
						}
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6305", strlen(C6305), C6305, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "TRANF") == 0) {
					if (current_client->phase_data_up == RECEIVE_FILE) {
						//if length! = 0 continue writing data to temp file
						if (recvMsg->Lenght != 0) {
							FILE *my_file = fopen(current_client->fileUpPath, "ab");
							fwrite(recvMsg->Payload, 1, recvMsg->Lenght, my_file);
							fclose(my_file);
							//if (serverMsgSend(PF_MESSAGE, "C2312", strlen(C2312), C2312, current_client->fd) == -1) return -1;
						}
						else
						{	timesleep = 10000;
							if (serverMsgSend(PF_MESSAGE, "C2313", strlen(C2313), C2313, current_client->fd) == -1) return -1;
							memset(current_client->fileName, '\0', sizeof(current_client->fileName));
							memset(current_client->fileUpPath, '\0', sizeof(current_client->fileUpPath));
							current_client->statusUpload=0;
							current_client->phase_data_up = WAIT_REQUEST;
							timesleep = 0;
						}
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C6303", strlen(C6303), C6303, current_client->fd) == -1) return -1;
					}
					break;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "CHG") == 0) {
					MYSQL *conn;
					MYSQL_RES *resU, *resG, *res;
					MYSQL_ROW rowU, rowG, row;
					conn = initConnection();
					char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
					sprintf(query, "select* from users where username = '%s'", current_client->accountInfo->info.userName);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					resU = mysql_store_result(conn);
					rowU = mysql_fetch_row(resU);
					memset(query, '\0', MAX_LENGTH_QUERY);
					sprintf(query, "select* from groups where group_name = '%s'", recvMsg->Payload);
					if (mysql_query(conn, query))
					{
						fprintf(stderr, "%s\n", mysql_error(conn));
					}
					resG = mysql_store_result(conn);
					rowG = mysql_fetch_row(resG);
					timesleep = 10000;
					if (rowG != NULL)
					{
						int checkResult = 0;
						memset(query, '\0', MAX_LENGTH_QUERY);
						sprintf(query, "select group_id from group_users where user_id = %s", rowU[0]);
						if (mysql_query(conn, query))
						{
							fprintf(stderr, "%s\n", mysql_error(conn));
						}
						res = mysql_use_result(conn);
						while ((row = mysql_fetch_row(res)) != NULL) {
							if (strcmp(rowG[0], row[0]) == 0) {
								checkResult = 1;
								break;
							}
						}
						if (checkResult == 1)
						{
							memset(current_client->currentPath, '\0', sizeof(current_client->currentPath));
							sprintf(current_client->currentPath, "%s", rowG[3]);
							memset(current_client->currentGroup,'\0',sizeof(current_client->currentGroup));
							sprintf(current_client->currentGroup,"%s",recvMsg->Payload);
							if (serverMsgSend(PF_MESSAGE, "C2314", strlen(C2314), C2314, current_client->fd) == -1) return -1;
							current_client->statusGroupAccess = 1;
							mysql_free_result(resG);
						}
						else
						{
							if (serverMsgSend(PF_MESSAGE, "C5307", strlen(C5307), C5307, current_client->fd) == -1) return -1;
						}
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C5302", strlen(C5302), C5302, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
					mysql_free_result(resU);
					closeConnection(conn);
				}
				else if (strcmp(recvMsg->Opcode, "PWD") == 0) {
					if (serverMsgSend(PF_MESSAGE, "C1309", strlen(current_client->currentPath), current_client->currentPath, current_client->fd) == -1) return -1;
				}
				//-----------------------------------------------------------------------------------------------------------
				else if (strcmp(recvMsg->Opcode, "CWD") == 0) {
					timesleep = 10000;
					if (recvMsg->Lenght != 0)
					{
						int i, count;
						char *temp_path = (char *)calloc(8196, sizeof(char));
						for (i = 0; i < strlen(current_client->currentPath); i++)
						{
							if (current_client->currentPath[i] == '/')count++;
						}
						if (count <= 1)
						{
							if (serverMsgSend(PF_MESSAGE, "C6305", strlen(C6305), C6305, current_client->fd) == -1) return -1;
						}
						else
						{
							if (strcmp(recvMsg->Payload, ".") == 0)
							{
								printf("Update path : %s\n", current_client->currentPath);

								if (serverMsgSend(PF_MESSAGE, "C2310", strlen(C2310), C2310, current_client->fd) == -1) return -1;
							}
							else if (strcmp(recvMsg->Payload, "..") == 0)
							{
								int lenght = strlen(current_client->currentPath);
								int count = lenght - 1;
								while (count > 0)
								{
									if (current_client->currentPath[count] != '/')count--;
									else break;
								}
								memcpy(temp_path, current_client->currentPath, count);
								count = 0;
								for (i = 0; i < strlen(temp_path); i++)
								{
									if (temp_path[i] == '/')count++;
								}
								if (count < 2)
								{
									if (serverMsgSend(PF_MESSAGE, "C6304", strlen(C6304), C6304, current_client->fd) == -1) return -1;
								}
								else
								{
									memset(current_client->currentPath, '\0', sizeof(current_client->currentPath));
									sprintf(current_client->currentPath, "%s", temp_path);
									printf("Update path : %s\n", current_client->currentPath);

									if (serverMsgSend(PF_MESSAGE, "C2310", strlen(C2310), C2310, current_client->fd) == -1) return -1;
								}
							}
							else
							{
								int checkResult = 0;
								struct dirent *de;
								DIR *dr;
								dr = opendir(current_client->currentPath);
								while ((de = readdir(dr)) != NULL)
								{
									if (strcmp(de->d_name, recvMsg->Payload) == 0)
									{
										checkResult = 1;
										break;
									}
								}
								if (checkResult == 1)
								{
									sprintf(temp_path, "%s/%s", current_client->currentPath, recvMsg->Payload);
									memset(current_client->currentPath, '\0', sizeof(current_client->currentPath));
									sprintf(current_client->currentPath, "%s", temp_path);
									printf("Update path : %s\n", current_client->currentPath);

									if (serverMsgSend(PF_MESSAGE, "C2310", strlen(C2310), C2310, current_client->fd) == -1) return -1;
								}
								else if (checkResult == 0)
								{
									if (serverMsgSend(PF_MESSAGE, "C5303", strlen(C5303), C5303, current_client->fd) == -1) return -1;
								}
							}
						}
						SAFE_DEL(temp_path);
					}
					else
					{
						if (serverMsgSend(PF_MESSAGE, "C5306", strlen(C5306), C5306, current_client->fd) == -1) return -1;
					}
					timesleep = 0;
				}
				else if (strcmp(recvMsg->Opcode, "EXIT") == 0) {

					return -1;
				}
				//-----------------------------------------------------------------------------------------------------------
				else {
					timesleep = 10000;
					if (serverMsgSend(PF_MESSAGE, "C6306", strlen(C6306), C6306, current_client->fd) == -1) return -1;
					timesleep = 0;
				}
			}
			}
		}
	}
	//No conection
	else {
		printf("------------------------------------------------------------------\n");
		printf("=> Close sockfd %d\n", current_client->fd);
		return -1;
	}
	return 0;
}

int checkCommand(char *arrayCommand[NUM_COMMAND], char *input)
{
	int i;
	for (i = 0; i<NUM_COMMAND; i++)
	{
		if (strcmp(input, arrayCommand[i]) == 0) return 0;
	}
	return -1;
}

/*=====================================================================================================/
void compressArray(struct pollfd *fd_array,struct clientInfo *c_Info, int *num_fd)

TODO   : > Compression function pollfd array and clientinfo array every time a connection is closed
---------------------------------------------------------------------------------------------------
INPUT  : - struct pollfd *fd_array   [pointer to the pollfd array]
- struct clientInfo *c_Info [pointer to the clientInfo array]
- int *num_fd               [current connection number]

/=====================================================================================================*/

void compressArray(struct pollfd *fd_array, struct clientInfo *c_Info, int *num_fd)
{
	int i, j, temp;
	temp = *num_fd;
	for (i = 0; i < *num_fd; i++) {
		if (fd_array[i].fd == -1) {
			for (j = i; j < *num_fd; j++) {
				fd_array[j].fd = fd_array[j + 1].fd;
				c_Info[j] = c_Info[j + 1];
			}
			i--;
			*num_fd = *num_fd - 1;
		}
	}
	printf("=> Compress array : %d -> %d client\n", temp - 1, *num_fd - 1);
	printf("------------------------------------------------------------------\n");
}

/*=====================================================================================================/
ListAccClient* newListAccClient()

TODO   : > Create new and return an empty list accounts requires login of client
--------------------------------------------------------------------------------
OUTPUT : + return m_list [Pointers to the empty account list from client]
/=====================================================================================================*/

ListAccClient* newListAccClient() {
	ListAccClient *m_list = (ListAccClient*)malloc(sizeof(ListAccClient));
	m_list->head = NULL;
	m_list->current = NULL;
	return m_list;
}

/*=====================================================================================================/
AccClient* findNodeAccClient(char *inputUsername,ListAccClient *list_A_C)

TODO   : > Find the node in the list. return node account in list if
existed, else return node = NULL
---------------------------------------------------------------------------
INPUT  : - char *inputUsername       [account name to search]
- ListAccClient *list_A_C   [search account list]
OUTPUT : + return AccClient*         [Returns the pointer to the location of the account if found]
+ return NULL               [in the list no user information "username"]
/=====================================================================================================*/

AccClient* findNodeAccClient(char *inputUsername, ListAccClient *list_A_C)
{
	list_A_C->current = list_A_C->head;
	while (list_A_C->current != NULL)
	{
		if (strcmp(list_A_C->current->nameAcc, inputUsername) == 0)return list_A_C->current;
		list_A_C->current = list_A_C->current->next;
	}
	return NULL;
}

/*=====================================================================================================/
void freeListAccClient(ListAccClient *list_A_C)

TODO   : > Release the list, recover the memory.
---------------------------------------------------------------------------
INPUT  : - ListAccClient *list_A_C [list needs to be released]

/=====================================================================================================*/

void freeListAccClient(ListAccClient *list_A_C)
{
	list_A_C->current = list_A_C->head;
	while (list_A_C->current != NULL) {
		list_A_C->head = list_A_C->head->next;
		SAFE_DEL(list_A_C->current);
		list_A_C->current = list_A_C->head;
	}
}


void *sendFile(void *arg)
{
	pthread_detach(pthread_self());
	struct clientInfo *c_Info = (struct clientInfo*)arg;
	FILE *result;
	char buff[BUFF_SIZE];
	int nLeft, ret;
	//Return the resulting file to the client.
	unsigned int sizeSendFile = SizeFile(c_Info->fileDownPath);
	result = fopen(c_Info->fileDownPath, "rb");
	nLeft = sizeSendFile;
	printf("=> Send result file to sockfd %d\n", c_Info->fd);
	while (nLeft > 0) {
		usleep(timesleep);
		memset(buff, '\0', BUFF_SIZE);
		ret = fread(buff, 1, BUFF_SIZE, result);
		if (serverMsgSend(PF_MESSAGE, "TRANF", ret, buff, c_Info->fd) == -1)
		{
			printf("=> Error sockfd %d closed, can't continue data reply\n", c_Info->fd);
			return 0;
		}
		nLeft -= ret;
	}
	if (serverMsgSend(PF_MESSAGE, "TRANF", 0, "", c_Info->fd) == -1)
	{
		printf("=> Error sockfd %d closed, can't continue data reply\n", c_Info->fd);
		return 0;
	}
	if (serverMsgSend(PF_MESSAGE, "C2319", strlen(C2319), C2319, c_Info->fd) == -1)
	{
		printf("=> Error sockfd %d closed, can't continue data reply\n", c_Info->fd);
		return 0;
	}
	printf("=> Send finish\n");
	//After sended the result file, delete the temp file and the result file
	//reset to wait for a new request
	memset(c_Info->fileDownPath, '\0', sizeof(c_Info->fileDownPath));
	c_Info->phase_data_down = WAIT_REQUEST;
	return 0;
}

