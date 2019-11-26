#include "accountlist.h"

/*=====================================================================================================/
    ListAccount* createNewList()

    TODO   : > Create new and return an empty account list.
    ---------------------------------------------------------------------------
    OUTPUT : + return m_list [Pointers to the empty account list]

/=====================================================================================================*/

ListAccount* createNewList() {
	ListAccount *m_list = (ListAccount*)malloc(sizeof(ListAccount));
	m_list->head = NULL;
	m_list->current = NULL;
	return m_list;
}

/*=====================================================================================================/
    ListAccount* loadAccount()

    TODO   : > Get account information from data file and add to account list.
    Get account information, initialize the initial value of the structure 
    under NodeAccount structure and add to the account list.
    ---------------------------------------------------------------------------
    OUTPUT : + return m_list [Pointers to the account list after adding the account information]
             + return NULL   [Can not read the account data file.]

/=====================================================================================================*/

ListAccount* loadAccount() {
	char temp_path[255];
	ListAccount *m_list = createNewList();
	MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    conn = initConnection();
    char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
    sprintf(query, "select* from users");
    if (mysql_query(conn, query))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return NULL;
    }
    res = mysql_use_result(conn);
	while ((row = mysql_fetch_row(res)) != NULL)
    {
    	//printf("LIST :%s-%s-%s\n",row[1],row[2],row[3]);
    	NodeAccount *account = (NodeAccount*)malloc(sizeof(NodeAccount));
    	sprintf(account->info.userName,"%s",row[1]);
    	sprintf(account->info.userName,"%s",row[1]);
    	sprintf(account->info.password,"%s",row[2]);
    	account->info.statusAccount = stringToNumber(row[3]);
        account->statusLogin = 0;
        sprintf(temp_path,"%s/%s",SERVER_ROOT,account->info.userName);
        mkdir(temp_path, 0777);
        account->next = m_list->head;       	// add account into first
		m_list->head = account;             	// update head of list account

    }
    mysql_free_result(res);
    closeConnection(conn);
	return m_list;
}

int addAccount (ListAccount *m_list, char *username, char*password)
{
	char temp_path[255];
	MYSQL *conn;
    conn = initConnection();
    char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
    sprintf(query, "insert into users (username, password, status) values ('%s', '%s', %d)",username,password,ST_ACTIVATED);
    if (mysql_query(conn, query))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return -1;
    }
    NodeAccount *account = (NodeAccount*)malloc(sizeof(NodeAccount));
    memcpy(account->info.userName,username,strlen(username));
    memcpy(account->info.password,password,strlen(password));
    sprintf(account->info.userName,"%s",username);
    sprintf(account->info.password,"%s",password);
	account->info.statusAccount = ST_ACTIVATED;
	account->statusNewNotification=0;
	account->statusLogin = 0;           // status login default = 0 ( not logged in )
	sprintf(temp_path,"%s/%s",SERVER_ROOT,account->info.userName);
    mkdir(temp_path, 0777);
	account->next = m_list->head;       // add account into first
	m_list->head = account;             // update head of list account
    closeConnection(conn);
    return 0;
}

/*=====================================================================================================/
    NodeAccount* checkUsername(char username[MAX_STRING], ListAccount *m_list)

    TODO   : > Check an existing account ? return node account in list if 
    existed, else return node account = NULL
    ---------------------------------------------------------------------------
    INPUT  : - char username[MAX_STRING] [account name to search]
    		 - ListAccount *m_list       [search account list]
    OUTPUT : + return NodeAccount*       [Returns the pointer to the location of the account if found]
             + return NULL               [in the list no user information "username"]

/=====================================================================================================*/  

NodeAccount* checkUsername(char *username, ListAccount *m_list) {
	m_list->current = m_list->head;
	while (m_list->current != NULL)
	{
		if (strcmp(username, m_list->current->info.userName) == 0) {
			return m_list->current;
		}
		m_list->current = m_list->current->next;
	}
	return NULL;
}

/*=====================================================================================================/
    int checkPassword(char password[MAX_STRING], NodeAccount *node)

    TODO   : > Check the password correctly or wrong.
    ------------------------------------------------------------------------------------------
    INPUT  : - char password[MAX_STRING] [external password input]
    		 - NodeAccount *node         [Pointers to account data need to check the password]
    OUTPUT : + return 0       [correct password]
             + return -1      [wrong password]

/=====================================================================================================*/

int checkPassword(char *password, NodeAccount *node) {
	//correct password
	if (strcmp(password, node->info.password) == 0) 
	{
		node->statusLogin = 1;
		return 0;
	}
	//wrong password
	else 
	{
		return -1;
	}
}

/*=====================================================================================================/
    void blockAccount(NodeAccount *node)

    TODO   : > Edit account information is blocked in the data file.
    ----------------------------------------------------------------------
    INPUT  : - NodeAccount *node  [Pointers to account data need to block]

/=====================================================================================================*/

int blockAccount(char *username)
{
	MYSQL *conn;
    conn = initConnection();
    char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
    sprintf(query, "update users set status = %d where username = '%s'",ST_BLOCKED,username);
    if (mysql_query(conn, query))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return -1;
    }
    return 0;
}

int createGroup(char *username,char *groupname)
{
	char temp_path[255];
	MYSQL_ROW row,row1;
	MYSQL *conn;
	MYSQL_RES *res,*res1;
    conn = initConnection();
    char *query = (char *)malloc(MAX_LENGTH_QUERY * sizeof(char));
    sprintf(query, "select* from users where username = '%s'",username);
    if (mysql_query(conn, query))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return -1;
    }
    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);
    if(row!=NULL)
    {
    	memset(query,'\0',MAX_LENGTH_QUERY);
    	sprintf(query, "select* from groups where group_name = '%s'",groupname);
    	if (mysql_query(conn, query))
    	{
        	fprintf(stderr, "%s\n", mysql_error(conn));
        	return -1;
    	}
    	res1 = mysql_store_result(conn);
    	row1 = mysql_fetch_row(res1);
    	if(row1==NULL)
    	{
    		memset(query,'\0',MAX_LENGTH_QUERY);
    		sprintf(temp_path,"%s/%s/%s",SERVER_ROOT,username,groupname);
    		sprintf(query, "insert into groups (owner_id, group_name, root_folder_name) values (%s, '%s', '%s')",row[0],groupname,temp_path);
    		if (mysql_query(conn, query))
    		{
        		fprintf(stderr, "%s\n", mysql_error(conn));
        		return -1;
    		}
    		mkdir(temp_path, 0777);
    		memset(query,'\0',MAX_LENGTH_QUERY);
    		sprintf(query, "select* from groups where group_name = '%s'",groupname);
    		if (mysql_query(conn, query))
    		{
        		fprintf(stderr, "%s\n", mysql_error(conn));
        		return -1;
    		}
    		res1 = mysql_store_result(conn);
    		row1 = mysql_fetch_row(res1);
    		printf("%s-%s\n",row1[0],row[0]);
    		memset(query,'\0',MAX_LENGTH_QUERY);
    		sprintf(query, "insert into group_users (group_id,user_id) values (%s,%s)",row1[0],row[0]);
    		if (mysql_query(conn, query))
    		{
        		fprintf(stderr, "%s\n", mysql_error(conn));
        		return -1;
    		}
    	}
    	else {
    		    free(query);
    			mysql_free_result(res);
    			closeConnection(conn);
    			return -3;
    		}
    }
    else 
    	{
    		free(query);
    		mysql_free_result(res);
    		closeConnection(conn);
    		return -2;
    	}
    free(query);
    mysql_free_result(res);
    closeConnection(conn);
    return 0;
}
/*=====================================================================================================/
    void freeListAccount(ListAccount *m_list)

    TODO   : > Release the account list, recover the memory.
    ---------------------------------------------------------------------------
    INPUT  : - ListAccount *m_list [list needs to be released]

/=====================================================================================================*/

void freeListAccount(ListAccount *m_list)
{
	m_list->current = m_list->head;
	while (m_list->current != NULL) {
		m_list->head = m_list->head->next;
		SAFE_DEL(m_list->current);
		m_list->current = m_list->head;
	}
}

void printList(ListAccount *m_list)
{
	m_list->current = m_list->head;
	while (m_list->current != NULL) 
	{
		printf("\n%s|%s|%d",m_list->current->info.userName,m_list->current->info.password,m_list->current->info.statusAccount);
		m_list->current = m_list->current->next;
	}
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

int stringToNumber(char *s_Input)
{
	    int i,value =0;
		for(i=0;i<strlen(s_Input);i++)
			{
				if(s_Input[i]<'0' || s_Input[i]>'9') return -1;
				// ex. 123 = 1*10^2 + 2*10^1 + 3*10^0
				value+=(s_Input[i]-48)*(pow(10,(strlen(s_Input)- 1 -i)));  
			}
		return value;
}
