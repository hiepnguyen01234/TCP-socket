#ifndef _ACCOUNT_MANAGER_H_
#define _ACCOUNT_MANAGER_H_

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "database.h"
#include "protocol.h"

//Directory storage file on server
#define SERVER_ROOT "SERVER"    	

//Struct Account: structure of account information
typedef struct _Account
{
	char userName[255];
	char password[255];
	int  statusAccount;
} Account;

//Struct NodeAccount: Structure of account information and activity information.
typedef struct _Node
{
	Account info;
	int statusLogin;
	int statusNewNotification;
	struct _Node *next;
} NodeAccount;

//Struct ListAccount: Account list structure
typedef struct _List
{
	NodeAccount *head;
	NodeAccount *current;

} ListAccount;

int mkdir(const char *pathname, mode_t mode);

ListAccount* createNewList();
ListAccount* loadAccount();
NodeAccount* checkUsername(char *username, ListAccount *m_list);
int checkPassword(char *password, NodeAccount *node);
int addAccount(ListAccount *m_list, char *username, char*password);
int blockAccount(char *username);
int createGroup(char *username,char *groupname);
void freeListAccount(ListAccount *m_list);
void printList(ListAccount *m_list);
int stringToNumber(char *s_Input);

#endif
