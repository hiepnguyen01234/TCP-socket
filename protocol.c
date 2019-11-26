#include "protocol.h"


/*=====================================================================================================/
int serverMsgSend(char opcode,int lenght,char *payload,int socket)

TODO   : > Send the message struct of the protocol and solve the Byte stream problem.
---------------------------------------------------------------------------
INPUT  : - char opcode     	[Opcode message]
- int lenght    	[lenght of payload]
- char *payload   	[pointer to string payload]
- int socket      	[socket number]
OUTPUT : + return 0        	[The process of sending the message is complete]
+ return -1       	[The process of sending messages is interrupted]

/=====================================================================================================*/

int prefixReceive(int conn_sock)
{
	int prefix, ret;
	ret = recv(conn_sock, &prefix, sizeof(int), MSG_WAITALL);
	if (ret <= 0)
	{
		return -1;
	}
	return prefix;
}

int serverMsgSend(int prefix, char *opcode, int lenght, char *payload, int socket)
{
	//create a message
	message *msgSend = (message*)calloc(1, sizeof(message));
	//Assign the corresponding data to the message structure.
	msgSend->Lenght = lenght;
	memcpy(msgSend->Opcode, opcode, sizeof(msgSend->Opcode));
	memcpy(msgSend->Payload, payload, lenght);
	// Assume s is a valid, connected stream socket
	char *temp = (char*)msgSend;
	int ret, msg_len, idx = 0;
	msg_len = sizeof(message);
	ret = send(socket, &prefix, sizeof(int), 0);
	if (ret == -1)
	{
		free(msgSend);
		return -1;
	}
	while (msg_len > 0) {
		// Assume s is a valid, connected stream socket
		ret = send(socket, temp + idx, msg_len, 0);
		if (ret == -1)
		{
			free(msgSend);
			return -1;
		}
		msg_len -= ret;
		idx += ret;
	}
	printf("   Send: %s \n", msgSend->Opcode);
	//free memory space for messages.
	free(msgSend);
	return 0;
}

int directSend(int prefix, int type, char *name, unsigned int size, int socket)
{
	//create a message
	direct *msgSend = (direct*)calloc(1, sizeof(direct));
	//Assign the corresponding data to the message structure.
	msgSend->type = type;
	sprintf(msgSend->name, "%s", name);
	msgSend->size = size;
	// Assume s is a valid, connected stream socket
	char *temp = (char*)msgSend;
	int ret, msg_len, idx = 0;
	msg_len = sizeof(direct);
	ret = send(socket, &prefix, sizeof(int), 0);
	if (ret == -1)
	{
		free(msgSend);
		return -1;
	}
	while (msg_len > 0) {
		// Assume s is a valid, connected stream socket
		ret = send(socket, temp + idx, msg_len, 0);
		if (ret == -1)
		{
			free(msgSend);
			return -1;
		}
		msg_len -= ret;
		idx += ret;
	}
	printf("   Send: %s \n", msgSend->name);
	//free memory space for messages.
	free(msgSend);
	return 0;
}

direct* directReceive(int conn_sock)
{
	int ret, nLeft, idx = 0;
	char recvBuff[BUFF_SIZE];
	//Control of receiving data by "char" pointer,solve the Byte stream problem.	
	char *temp = (char*)calloc(1, sizeof(direct));
	nLeft = sizeof(direct);
	while (nLeft >0) {
		if (nLeft >= BUFF_SIZE)
			ret = recv(conn_sock, &recvBuff, BUFF_SIZE, MSG_WAITALL);
		else
			ret = recv(conn_sock, &recvBuff, nLeft, MSG_WAITALL);
		//Pointers to NULL, data is interrupted
		if (ret <= 0) return NULL;
		memcpy(temp + idx, recvBuff, ret);
		idx += ret;
		nLeft -= ret;
	}
	//When receiving full data of the message, 
	//converting to the message pointer can read the data 
	//according to the message structure.
	direct *msgReceive = (direct*)temp;
	return msgReceive;
}

int memberSend(int prefix, int role, char *username, char *groupname, int socket)
{
	//create a message
	member *msgSend = (member*)calloc(1, sizeof(member));
	//Assign the corresponding data to the message structure.
	msgSend->role = role;
	sprintf(msgSend->username, "%s", username);
	sprintf(msgSend->groupname, "%s", groupname);
	// Assume s is a valid, connected stream socket
	char *temp = (char*)msgSend;
	int ret, msg_len, idx = 0;
	msg_len = sizeof(member);
	ret = send(socket, &prefix, sizeof(int), 0);
	if (ret == -1)
	{
		free(msgSend);
		return -1;
	}
	while (msg_len > 0) {
		// Assume s is a valid, connected stream socket
		ret = send(socket, temp + idx, msg_len, 0);
		if (ret == -1)
		{
			free(msgSend);
			return -1;
		}
		msg_len -= ret;
		idx += ret;
	}
	printf("   Send: %s \n", msgSend->username);
	//free memory space for messages.
	free(msgSend);
	return 0;
}

member* memberReceive(int conn_sock)
{
	int ret, nLeft, idx = 0;
	char recvBuff[BUFF_SIZE];
	//Control of receiving data by "char" pointer,solve the Byte stream problem.	
	char *temp = (char*)calloc(1, sizeof(member));
	nLeft = sizeof(member);
	while (nLeft >0) {
		if (nLeft >= BUFF_SIZE)
			ret = recv(conn_sock, &recvBuff, BUFF_SIZE, MSG_WAITALL);
		else
			ret = recv(conn_sock, &recvBuff, nLeft, MSG_WAITALL);
		//Pointers to NULL, data is interrupted
		if (ret <= 0) return NULL;
		memcpy(temp + idx, recvBuff, ret);
		idx += ret;
		nLeft -= ret;
	}
	//When receiving full data of the message, 
	//converting to the message pointer can read the data 
	//according to the message structure.
	member *msgReceive = (member*)temp;
	return msgReceive;
}

int notificationSend(int prefix, char *id_notification, char *content, char *time_create, int socket)
{
	//create a message
	notification *msgSend = (notification*)calloc(1, sizeof(notification));
	//Assign the corresponding data to the message structure.
	sprintf(msgSend->id_notification, "%s", id_notification);
	sprintf(msgSend->content, "%s", content);
	sprintf(msgSend->time_create, "%s", time_create);
	// Assume s is a valid, connected stream socket
	char *temp = (char*)msgSend;
	int ret, msg_len, idx = 0;
	msg_len = sizeof(notification);
	ret = send(socket, &prefix, sizeof(int), 0);
	if (ret == -1)
	{
		free(msgSend);
		return -1;
	}
	while (msg_len > 0) {
		// Assume s is a valid, connected stream socket
		ret = send(socket, temp + idx, msg_len, 0);
		if (ret == -1)
		{
			free(msgSend);
			return -1;
		}
		msg_len -= ret;
		idx += ret;
	}
	printf("   Send: %s \n", msgSend->content);
	//free memory space for messages.
	free(msgSend);
	return 0;
}

notification* notificationReceive(int conn_sock)
{
	int ret, nLeft, idx = 0;
	char recvBuff[BUFF_SIZE];
	//Control of receiving data by "char" pointer,solve the Byte stream problem.	
	char *temp = (char*)calloc(1, sizeof(notification));
	nLeft = sizeof(notification);
	while (nLeft >0) {
		if (nLeft >= BUFF_SIZE)
			ret = recv(conn_sock, &recvBuff, BUFF_SIZE, MSG_WAITALL);
		else
			ret = recv(conn_sock, &recvBuff, nLeft, MSG_WAITALL);
		//Pointers to NULL, data is interrupted
		if (ret <= 0) return NULL;
		memcpy(temp + idx, recvBuff, ret);
		idx += ret;
		nLeft -= ret;
	}
	//When receiving full data of the message, 
	//converting to the message pointer can read the data 
	//according to the message structure.
	notification *msgReceive = (notification*)temp;
	return msgReceive;
}

int joinNotiSend(int prefix, char * id_notification, char *admin_group, char *name_group, char *joiner, char *time_create, int socket)
{
	//create a message
	join_notification *msgSend = (join_notification*)calloc(1, sizeof(join_notification));
	//Assign the corresponding data to the message structure.
	sprintf(msgSend->id_notification, "%s", id_notification);
	sprintf(msgSend->admin_group, "%s", admin_group);
	sprintf(msgSend->name_group, "%s", name_group);
	sprintf(msgSend->joiner, "%s", joiner);
	sprintf(msgSend->time_create, "%s", time_create);
	// Assume s is a valid, connected stream socket
	char *temp = (char*)msgSend;
	int ret, msg_len, idx = 0;
	msg_len = sizeof(join_notification);
	ret = send(socket, &prefix, sizeof(int), 0);
	if (ret == -1)
	{
		free(msgSend);
		return -1;
	}
	while (msg_len > 0) {
		// Assume s is a valid, connected stream socket
		ret = send(socket, temp + idx, msg_len, 0);
		if (ret == -1)
		{
			free(msgSend);
			return -1;
		}
		msg_len -= ret;
		idx += ret;
	}
	printf("   Send: %s \n", msgSend->id_notification);
	//free memory space for messages.
	free(msgSend);
	return 0;
}

join_notification* joinNotiReceive(int conn_sock)
{
	int ret, nLeft, idx = 0;
	char recvBuff[BUFF_SIZE];
	//Control of receiving data by "char" pointer,solve the Byte stream problem.	
	char *temp = (char*)calloc(1, sizeof(join_notification));
	nLeft = sizeof(join_notification);
	while (nLeft >0) {
		if (nLeft >= BUFF_SIZE)
			ret = recv(conn_sock, &recvBuff, BUFF_SIZE, MSG_WAITALL);
		else
			ret = recv(conn_sock, &recvBuff, nLeft, MSG_WAITALL);
		//Pointers to NULL, data is interrupted
		if (ret <= 0) return NULL;
		memcpy(temp + idx, recvBuff, ret);
		idx += ret;
		nLeft -= ret;
	}
	//When receiving full data of the message, 
	//converting to the message pointer can read the data 
	//according to the message structure.
	join_notification *msgReceive = (join_notification*)temp;
	return msgReceive;
}


int groupSend(int prefix, int role, char *group_name, int socket)
{
	//create a message
	group *msgSend = (group*)calloc(1, sizeof(group));
	//Assign the corresponding data to the message structure.
	msgSend->role = role;
	sprintf(msgSend->group_name, "%s", group_name);
	// Assume s is a valid, connected stream socket
	char *temp = (char*)msgSend;
	int ret, msg_len, idx = 0;
	msg_len = sizeof(group);
	ret = send(socket, &prefix, sizeof(int), 0);
	if (ret == -1)
	{
		free(msgSend);
		return -1;
	}
	while (msg_len > 0) {
		// Assume s is a valid, connected stream socket
		ret = send(socket, temp + idx, msg_len, 0);
		if (ret == -1)
		{
			free(msgSend);
			return -1;
		}
		msg_len -= ret;
		idx += ret;
	}
	printf("   Send: %s \n", msgSend->group_name);
	//free memory space for messages.
	free(msgSend);
	return 0;
}

group* groupReceive(int conn_sock)
{
	int ret, nLeft, idx = 0;
	char recvBuff[BUFF_SIZE];
	//Control of receiving data by "char" pointer,solve the Byte stream problem.	
	char *temp = (char*)calloc(1, sizeof(group));
	nLeft = sizeof(group);
	while (nLeft >0) {
		if (nLeft >= BUFF_SIZE)
			ret = recv(conn_sock, &recvBuff, BUFF_SIZE, MSG_WAITALL);
		else
			ret = recv(conn_sock, &recvBuff, nLeft, MSG_WAITALL);
		//Pointers to NULL, data is interrupted
		if (ret <= 0) return NULL;
		memcpy(temp + idx, recvBuff, ret);
		idx += ret;
		nLeft -= ret;
	}
	//When receiving full data of the message, 
	//converting to the message pointer can read the data 
	//according to the message structure.
	group *msgReceive = (group*)temp;
	return msgReceive;
}

int publicgroupSend(int prefix, int role, char *group_name,int status, int socket)
{
	//create a message
	publicgroup *msgSend = (publicgroup*)calloc(1, sizeof(publicgroup));
	//Assign the corresponding data to the message structure.
	msgSend->role = role;
	msgSend->group_status = status;
	sprintf(msgSend->group_name, "%s", group_name);
	// Assume s is a valid, connected stream socket
	char *temp = (char*)msgSend;
	int ret, msg_len, idx = 0;
	msg_len = sizeof(publicgroup);
	ret = send(socket, &prefix, sizeof(int), 0);
	if (ret == -1)
	{
		free(msgSend);
		return -1;
	}
	while (msg_len > 0) {
		// Assume s is a valid, connected stream socket
		ret = send(socket, temp + idx, msg_len, 0);
		if (ret == -1)
		{
			free(msgSend);
			return -1;
		}
		msg_len -= ret;
		idx += ret;
	}
	printf("   Send: %s \n", msgSend->group_name);
	//free memory space for messages.
	free(msgSend);
	return 0;
}

publicgroup* publicgroupReceive(int conn_sock)
{
	int ret, nLeft, idx = 0;
	char recvBuff[BUFF_SIZE];
	//Control of receiving data by "char" pointer,solve the Byte stream problem.	
	char *temp = (char*)calloc(1, sizeof(publicgroup));
	nLeft = sizeof(publicgroup);
	while (nLeft >0) {
		if (nLeft >= BUFF_SIZE)
			ret = recv(conn_sock, &recvBuff, BUFF_SIZE, MSG_WAITALL);
		else
			ret = recv(conn_sock, &recvBuff, nLeft, MSG_WAITALL);
		//Pointers to NULL, data is interrupted
		if (ret <= 0) return NULL;
		memcpy(temp + idx, recvBuff, ret);
		idx += ret;
		nLeft -= ret;
	}
	//When receiving full data of the message, 
	//converting to the message pointer can read the data 
	//according to the message structure.
	publicgroup *msgReceive = (publicgroup*)temp;
	return msgReceive;
}

/*=====================================================================================================/
int serverMsgSend(char opcode,int lenght,char *payload,int socket)

TODO   : > Send the message struct of the protocol and solve the Byte stream problem.
---------------------------------------------------------------------------
INPUT  : - char opcode     [Opcode message]
- int lenght    [lenght of payload]
- char *payload   [pointer to string payload]
- int socket      [socket number]
OUTPUT : + return 0        [The process of sending the message is complete]
+ return -1       [The process of sending messages is interrupted]

/=====================================================================================================*/

int clientMsgSend(char *opcode, int lenght, char *payload, int socket)
{ 
	//create a message
	message *msgSend = (message*)calloc(1, sizeof(message));
	//Assign the corresponding data to the message structure.
	msgSend->Lenght = lenght;
	memcpy(msgSend->Opcode, opcode, sizeof(msgSend->Opcode));
	memcpy(msgSend->Payload, payload, lenght);
	// Assume s is a valid, connected stream socket
	char *temp = (char*)msgSend;
	int ret, msg_len, idx = 0;
	msg_len = sizeof(message);
	while (msg_len > 0) {
		// Assume s is a valid, connected stream socket
		ret = send(socket, temp + idx, msg_len, 0);
		if (ret == -1)
		{
			free(msgSend);
			return -1;
		}
		msg_len -= ret;
		idx += ret;
	}
	//free memory space for messages.
	free(msgSend);
	return 0;
}

/*=====================================================================================================//
message* messageReceive(int conn_sock)

TODO   : > Receive the message struct of the protocol and solve the Byte stream problem.
Receive the message and return the pointer to the data if successful.
Returns the pointer NULL if the process of receiving data is interrupted.
---------------------------------------------------------------------------
INPUT  : - int conn_sock     [socket number]
OUTPUT : + return msgReceive [pointer type struct message,point to the data receiving area.]
+ return NULL       [Pointers to NULL, data is interrupted]

//=====================================================================================================*/

message* messageReceive(int conn_sock)
{
	int ret, nLeft, idx = 0;
	char recvBuff[BUFF_SIZE];
	//Control of receiving data by "char" pointer,solve the Byte stream problem.	
	char *temp = (char*)calloc(1, sizeof(message));
	nLeft = sizeof(message);
	while (nLeft >0) {
		if (nLeft >= BUFF_SIZE)
			ret = recv(conn_sock, &recvBuff, BUFF_SIZE, MSG_WAITALL);
		else
			ret = recv(conn_sock, &recvBuff, nLeft, MSG_WAITALL);
		//Pointers to NULL, data is interrupted
		if (ret <= 0) return NULL;
		memcpy(temp + idx, recvBuff, ret);
		idx += ret;
		nLeft -= ret;
	}
	//When receiving full data of the message, 
	//converting to the message pointer can read the data 
	//according to the message structure.
	message *msgReceive = (message*)temp;
	return msgReceive;
}

/*=====================================================================================================/
int StringToNumber(char *s_Input)

TODO   : > Convert string s_Input to number.Get into a string of numbers, convert to numeric.
Returns the numeric value if the conversion succeeds and returns -1 if there are errors.
---------------------------------------------------------------------------
INPUT  : - s_Input                         [Pointer to string need to convert]
OUTPUT : + return value if convert success [s_Input are numeric strings]
+ return -1                       [s_Input are not numeric strings]

/=====================================================================================================*/

int StringToNumber(char *s_Input)
{
	int i, value = 0;
	for (i = 0; i<strlen(s_Input); i++)
	{
		if (s_Input[i]<'0' || s_Input[i]>'9') return -1;
		// ex. 123 = 1*10^2 + 2*10^1 + 3*10^0
		value += (s_Input[i] - 48)*(pow(10, (strlen(s_Input) - 1 - i)));
	}
	return value;
}

/*=====================================================================================================/
unsigned int SizeFile(char *fileDirectory)
---------------------------------------------------------------------------
TODO   : > Get size of file with parameter path file
---------------------------------------------------------------------------
INPUT  : - char *fileDirectory              [File path need to get size]
OUTPUT : + return unsigned int sizeOfFile   [size of file affter get]

/=====================================================================================================*/

unsigned int SizeFile(char *fileDirectory)
{
	unsigned int sizeOfFile;
	FILE *my_file = fopen(fileDirectory, "rb");
	fseek(my_file, 0, SEEK_END);
	sizeOfFile = ftell(my_file);
	fclose(my_file);
	return sizeOfFile;
}

char *currentTime() {
	char *result;
	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	result = asctime(timeinfo);
	result[strlen(result) - 1] = '\0';
	return result;
}