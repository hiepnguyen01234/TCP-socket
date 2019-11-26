#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>

int system(const char *command);
int usleep(useconds_t usec);

#define BUFF_SIZE 8196

//define a function that safely releases memory
#define SAFE_DEL(x) { if (x) { free(x); x = NULL; } }

//defines message structure
typedef struct _message
{
	char Opcode[20];
	int  Lenght;
	char Payload[BUFF_SIZE];
} message;
//--------------------------
typedef struct _join_notification
{
	char id_notification[10];
	char admin_group[255];
	char name_group[255];
	char joiner[255];
	char time_create[512];
} join_notification;

typedef struct _notification
{
	char id_notification[10];
	char content[1024];
	char time_create[512];
} notification;

//--------------------------
typedef struct _member
{
	int  role;
	char username[255];
	char groupname[255];
} member;
//--------------------------
typedef struct _direct
{
	int  type;
	char name[255];
	unsigned int size;
} direct;
//--------------------------
typedef struct _group
{
	int role;
	char group_name[255];
} group;
//--------------------------
typedef struct _publicgroup
{
	int role;
	char group_name[255];
	int  group_status;
} publicgroup;
//--------------------------
typedef struct _infofile
{
	char file_name[255];
	unsigned int size;
} infofile;

//Phase authenticate
#define NOT_AUTHENTICATED 	100
#define AUTHENTICATING 	    101
#define SIGN_UP         	102
#define AUTHENTICATED    	103

//Phase data tranfer
#define WAIT_REQUEST		200
#define RECEIVE_FILE		201
#define SEND_FILE			202

//Define type notification
#define NOTIFI_JOIN			300
#define NOTIFI_NORMAL		301

//Define role
#define R_ADMIN   			400
#define R_MEMBER  			401
#define R_UNKNOWN 			402

//Define prefix message
#define PF_MESSAGE 			500
#define PF_JOIN_INFOMATION 	501
#define PF_NOTIFICATION 	502
#define PF_MEMBER		 	503
#define PF_DIRECTORY 		504
#define PF_MY_GROUP 		505
#define PF_PUBLIC_GROUP 	506

#define ST_ACTIVATED		600
#define ST_BLOCKED			601
#define ST_CAN_JOIN			602
#define ST_CANNOT_JOIN		603

#define M0000 "You have new notifications"

#define C1300 "Get account information successfully"
#define C1301 "Get group list successful"
#define C1302 "Get the directory listing successful"
#define C1303 "Get file size successful"
#define C1304 "You have requested to join your group"
#define C1305 "Get your group join request information successful"
#define C1306 "Get the group members information successful"
#define C1307 "Get the notification successful"
#define C1308 "Get group list successful"
#define C1309 "Get working directory successful"

#define C2100 "Logged in successfully"
#define C2200 "Successful account registration"
#define C2300 "Create new group successfully"	
#define C2301 "Join the group"				
#define C2302 "Added user to group"
#define C2303 "Deny to join the group"
#define C2304 "Leave the group successful"
#define C2305 "Create directory successful"
#define C2306 "Delete directory successful"
#define C2307 "Delete file success"
#define C2308 "Accept the file upload request"
#define C2309 "Accept the file download request"
#define C2310 "Update working directory"
#define C2311 "Logout Successful "
#define C2312 "Data ok"
#define C2313 "Upload file successful"
#define C2314 "Move to group successful"
#define C2315 "Has sent a request to join the group"
#define C2316 "Leave the group successful"
#define C2317 "Remove members from the group successful"
#define C2318 "Clear all notification successful"
#define C2319 "Download file successful"
#define C2320 "Delete notification successful"
#define C2321 "Delete group successful"

#define C3000 "Username okay, need password to complete the registration"
#define C3001 "Username okay, need password"

#define C5000 "This username already exists"
#define C5001 "This account does not exist" 
#define C5002 "This account has been blocked"
#define C5003 "Account is logged on another machine"
#define C5100 "Wrong password"
#define C5101 "Wrong password. Your account has been blocked"
#define C5102 "Sorry, this account can't continue to log in because it has been blocked"
#define C5300 "This group name already exists"
#define C5301 "This folder already exists"
#define C5302 "Group does not exist, can not access"
#define C5303 "Folder does not exist"
#define C5304 "File already exists"
#define C5305 "Can't download, file does not exist"
#define C5306 "Error"
#define C5307 "Sorry, only members can access"
#define C5308 "Request to join the group failed"
#define C5309 "Group leave request is not accepted"

#define C6000 "You need to send us your username before sending your password"
#define C6001 "You are not allowed to sign out because you are not logged in"
#define C6002 "You are not logged in, your request could not be accepted"
#define C6100 "You are validating the account, other requests are not accepted"
#define C6101 "You are authenticating another account"
#define C6102 "You are not allowed to sign out because you are not logged in"
#define C6200 "You are registering an account, other request is not accepted"
#define C6201 "Error. Password can not be blank"
#define C6202 "Error. Username can not be blank"
#define C6300 "Logged in, you are not allowed to login to another account"
#define C6301 "You do not have permission to execute this action"
#define C6302 "Error. The file name is not valid"
#define C6303 "Error. You are not allowed to send data now"
#define C6304 "Error. Is at the root of the group"
#define C6305 "Sorry. You need access to a group"
#define C6306 "Logged in, the request is not accepted."
#define C6307 "Wait for group admin to approve"
#define C6308 "Accept request are not accepted"
#define C6309 "Deny request not accepted"
#define C6310 "Sorry, only members can get information"
#define C6311 "Member removal requests are not accepted"
#define C6312 "Error. Group name can not be blank"
#define C6313 "Error. You are uploading another file"
#define C6314 "Error. Folder name can not be blank"
#define C6315 "Request delete notification is not accepted"
#define C6316 "File is uploading incomplete. Please wait..."
#define C6317 "You do not have permission to delete this group!"

#define C7000 "The syntax is unknown"
#define C7100 "The syntax is unknown"
#define C7200 "The syntax is unknown"
#define C7300 "The syntax is unknown"

int prefixReceive(int conn_sock);

int clientMsgSend(char *opcode,int lenght,char *payload,int socket);

int serverMsgSend(int prefix,char *opcode,int lenght,char *payload,int socket);
message* messageReceive(int conn_sock);

int directSend(int prefix,int type,char *name,unsigned int size,int socket);
direct* directReceive(int conn_sock);

int memberSend(int prefix,int role,char *username,char *groupname,int socket);
member* memberReceive(int conn_sock);

int joinNotiSend(int prefix,char * id_notification,char *admin_group,char *name_group,char *joiner,char *time_create ,int socket);
join_notification* joinNotiReceive(int conn_sock);

int notificationSend(int prefix,char *id_notification,char *content,char *time_create,int socket);
notification* notificationReceive(int conn_sock);

int groupSend(int prefix, int role, char *group_name, int socket);
group* groupReceive(int conn_sock);

int publicgroupSend(int prefix, int role, char *group_name,int status, int socket);
publicgroup* publicgroupReceive(int conn_sock);

unsigned int SizeFile(char *fileDirectory);

char *currentTime();

int StringToNumber(char *s_Input);

#endif