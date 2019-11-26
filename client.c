#include <gtk/gtk.h>
#include "protocol.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <signal.h>

int mkdir(const char *pathname, mode_t mode);

gpointer filterMessage(gpointer data);
gpointer uploadFile(gpointer data);
gpointer handleMessage(gpointer data);

int client_sock, SERV_PORT;
struct sockaddr_in server_addr;
struct in_addr SERV_ADDR;

GMutex mutex_interface;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char fileName[1024];
char namefileUp[1024];
char pathfileUp[1024];

char namefileDown[1024];
char pathdirDownload[1024];
char pathfileDownload[1024];

char currentGroup[255];

int have_new_notification,
warning_disconnect,
logout_success,
update_current_path,
refresh_w_mygroup,
refresh_w_joingroup,
refresh_w_directtory,
refresh_w_member,
refresh_w_notification,
refresh_w_joininfomation,
phaseAuth,
phaseUpload,
phaseDownload;

int timesleep=0;

unsigned int sizeFile;

GtkBuilder      *builder;

typedef struct {
	GtkWidget *window_main, *window_auth,*window_conect,*window_warning, *labellogin, *w_directory, *w_joingroup, *w_mygroup, *w_member, *w_joininfo, *w_notification;
	GtkTreeView *treeview;
	GtkListStore *liststore1, *liststore2, *liststore3, *liststore4, *liststore5, *liststore6;
	GtkEntry *username, *Password, *addr, *port;
	GtkTreeModel *mtree1, *mtree2, *mtree3, *mtree4, *mtree5, *mtree6;
	GtkLabel *l_notice, *l_upload_path, *l_download_path,*l_current_path;
	GtkWidget *s_upload, *s_download, *im_notice_login;
} app_widgets;

app_widgets *newdata;

//////////////////////////////////////////////////////////////////POPUP///////////////////////////////////////////////////////////
void view_popup_menu_do_download_file(GtkWidget *menuitem1, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *name, *type;
		gtk_tree_model_get(model, &iter, 0, &name, 1, &type, 2, &sizeFile, -1);
		printf("SELECT : %s-%s\n", name, type);
		sprintf(fileName, "%s", name);
		if (phaseDownload == 0)
		{
			if (strcmp(pathdirDownload, "") != 0)
			{
				printf("SIZE: %u\n", sizeFile);
				memset(namefileDown, '\0', sizeof(namefileDown));
				sprintf(namefileDown, "%s", name);
				clientMsgSend("DFILE", strlen(name), name, client_sock);
			}
			else
			{
				gtk_label_set_text(newdata->l_notice, "You have not selected the download folder");
			}
		}
	}
	else
	{
		gtk_label_set_text(newdata->l_notice,"Please wait. You are downloading another file");
	}
}

void view_popup_menu_do_delete_file(GtkWidget *menuitem2, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *a, *b;
		gtk_tree_model_get(model, &iter, 0, &a, 1, &b, -1);
		printf("DELETE : %s-%s\n", a, b);
		clientMsgSend("DELE", strlen(a), a, client_sock);
		gtk_list_store_clear(newdata->liststore2);
		clientMsgSend("LIST", 0, "", client_sock);
	}
}

void view_popup_menu_do_remove_folder(GtkWidget *menuitem3, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *a, *b;
		gtk_tree_model_get(model, &iter, 0, &a, 1, &b, -1);
		printf("REMOVE : %s-%s\n", a, b);
		clientMsgSend("RMD", strlen(a), a, client_sock);
		gtk_list_store_clear(newdata->liststore2);
		clientMsgSend("LIST", 0, "", client_sock);
	}
}

void view_popup_menu_do_f_remove_folder(GtkWidget *menuitem3, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *a, *b;
		gtk_tree_model_get(model, &iter, 0, &a, 1, &b, -1);
		printf("FORCE REMOVE : %s-%s\n", a, b);
		clientMsgSend("FORCERMD", strlen(a), a, client_sock);
		gtk_list_store_clear(newdata->liststore2);
		clientMsgSend("LIST", 0, "", client_sock);
	}
}

void view_popup_menu_do_joingroup(GtkWidget *menuitem3, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *a, *b;
		gtk_tree_model_get(model, &iter, 0, &a, 1, &b, -1);
		printf("JOIN TO: %s-%s\n", a, b);
		clientMsgSend("JOIN", strlen(a), a, client_sock);
	}
}

void view_popup_menu_do_leavegroup(GtkWidget *menuitem3, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *a, *b;
		gtk_tree_model_get(model, &iter, 0, &a, 1, &b, -1);
		printf("LEAVE GROUP : %s-%s\n", a, b);
		clientMsgSend("GLEAVE", strlen(a), a, client_sock);
		gtk_list_store_clear(newdata->liststore1);
		clientMsgSend("GLIST", 0, "", client_sock);
		gtk_list_store_clear(newdata->liststore2);
		clientMsgSend("LIST", 0, "", client_sock);
		gtk_list_store_clear(newdata->liststore4);
		clientMsgSend("MYGR", 0, "", client_sock);
	}
}

void view_popup_menu_do_delegroup(GtkWidget *menuitem3, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *a, *b;
		gtk_tree_model_get(model, &iter, 0, &a, 1, &b, -1);
		printf("DELETE GROUP : %s-%s\n", a, b);
		clientMsgSend("DELEGROUP", strlen(a), a, client_sock);
		gtk_list_store_clear(newdata->liststore1);
		clientMsgSend("GLIST", 0, "", client_sock);
		gtk_list_store_clear(newdata->liststore2);
		clientMsgSend("LIST", 0, "", client_sock);
		gtk_list_store_clear(newdata->liststore4);
		clientMsgSend("MYGR", 0, "", client_sock);
	}
}

void view_popup_menu_do_kickout(GtkWidget *menuitem3, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		char *temp = (char*)calloc(1024, sizeof(char));
		gchar *username, *role, *groupname;
		gtk_tree_model_get(model, &iter, 0, &username, 1, &role, 2, &groupname, -1);
		printf("KICK MEMBER : %s-%s-%s\n", username, role, groupname);
		sprintf(temp, "%s:%s", groupname, username);
		clientMsgSend("KICK", strlen(temp), temp, client_sock);
		gtk_list_store_clear(newdata->liststore3);
		clientMsgSend("GMEM", strlen(currentGroup), currentGroup, client_sock);
		SAFE_DEL(temp);
	}
}

void view_popup_menu_do_accept(GtkWidget *menuitem3, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *a, *b;
		gtk_tree_model_get(model, &iter, 0, &a, 1, &b, -1);
		printf("ACEPT MEMBER : %s-%s\n", a, b);
		clientMsgSend("ACEPT", strlen(a), a, client_sock);
		gtk_list_store_clear(newdata->liststore6);
		clientMsgSend("JOININFO", 0, "", client_sock);
	}
}

void view_popup_menu_do_reject(GtkWidget *menuitem3, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *a, *b;
		gtk_tree_model_get(model, &iter, 0, &a, 1, &b, -1);
		printf("REJECT MEMBER : %s-%s\n", a, b);
		clientMsgSend("RJECT", strlen(a), a, client_sock);
		gtk_list_store_clear(newdata->liststore6);
		clientMsgSend("JOININFO", 0, "", client_sock);
	}
}

void view_popup_menu_do_delete_notification(GtkWidget *menuitem3, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreeView *treeview = GTK_TREE_VIEW(userdata);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *a, *b;
		gtk_tree_model_get(model, &iter, 0, &a, 1, &b, -1);
		printf("DELETE NOTIFY : %s-%s\n", a, b);
		clientMsgSend("DELENOTIF", strlen(a), a, client_sock);
		gtk_list_store_clear(newdata->liststore5);
		clientMsgSend("MYNOTIF", 0, "", client_sock);
	}
}
////////////////////////////////////////////////////////////POPUP MENU/////////////////////////////////////////////////////////////////////

void view_popup_menu_file(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	GtkWidget *menu, *menu_download, *menu_delete_file;
	menu = gtk_menu_new();
	menu_download = gtk_menu_item_new_with_label("Download file");
	g_signal_connect(menu_download, "activate", (GCallback)view_popup_menu_do_download_file, treeview);
	menu_delete_file = gtk_menu_item_new_with_label("Delete file");
	g_signal_connect(menu_delete_file, "activate", (GCallback)view_popup_menu_do_delete_file, treeview);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_download);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_delete_file);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));
}

void view_popup_menu_folder(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	GtkWidget *menu, *menu_remove_folder, *menu_f_remove_folder;
	menu = gtk_menu_new();
	menu_remove_folder = gtk_menu_item_new_with_label("Remove");
	menu_f_remove_folder = gtk_menu_item_new_with_label("Force remove");
	g_signal_connect(menu_remove_folder, "activate", (GCallback)view_popup_menu_do_remove_folder, treeview);
	g_signal_connect(menu_f_remove_folder, "activate", (GCallback)view_popup_menu_do_f_remove_folder, treeview);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_remove_folder);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_f_remove_folder);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));

}

void view_popup_menu_joingroup(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	GtkWidget *menu, *menu_joingroup;
	menu = gtk_menu_new();
	menu_joingroup = gtk_menu_item_new_with_label("Join group");
	g_signal_connect(menu_joingroup, "activate", (GCallback)view_popup_menu_do_joingroup, treeview);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_joingroup);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));

}

void view_popup_menu_mygroup(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	GtkWidget *menu, *menu_leavegroup;
	menu = gtk_menu_new();
	menu_leavegroup = gtk_menu_item_new_with_label("Leave group");
	g_signal_connect(menu_leavegroup, "activate", (GCallback)view_popup_menu_do_leavegroup, treeview);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_leavegroup);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));
}

void view_popup_menu_mygroup2(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	GtkWidget *menu, *menu_delegroup;
	menu = gtk_menu_new();
	menu_delegroup = gtk_menu_item_new_with_label("Delete group");
	g_signal_connect(menu_delegroup, "activate", (GCallback)view_popup_menu_do_delegroup, treeview);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_delegroup);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));
}

void view_popup_menu_member(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	GtkWidget *menu, *menu_kickout;
	menu = gtk_menu_new();
	menu_kickout = gtk_menu_item_new_with_label("Kick out");
	g_signal_connect(menu_kickout, "activate", (GCallback)view_popup_menu_do_kickout, treeview);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_kickout);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));
}

void view_popup_menu_joininfo(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	GtkWidget *menu, *menu_accept, *menu_reject;
	menu = gtk_menu_new();
	menu_accept = gtk_menu_item_new_with_label("Accept");
	g_signal_connect(menu_accept, "activate", (GCallback)view_popup_menu_do_accept, treeview);
	menu_reject = gtk_menu_item_new_with_label("Reject");
	g_signal_connect(menu_reject, "activate", (GCallback)view_popup_menu_do_reject, treeview);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_accept);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_reject);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));
}

void view_popup_menu_notification(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{

	GtkWidget *menu, *menu_delete_notification;
	menu = gtk_menu_new();
	menu_delete_notification = gtk_menu_item_new_with_label("Delete notification");
	g_signal_connect(menu_delete_notification, "activate", (GCallback)view_popup_menu_do_delete_notification, treeview);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_delete_notification);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));
}

gboolean view_onButtonPressed_directory(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
	{
		GtkTreeSelection *selection;
		GtkTreeModel *model;
		GtkTreeIter   iter;
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
			gchar *a, *b;
			gtk_tree_model_get(model, &iter, 0, &a, 1, &b, -1);
			if (strcmp(b, "folder") == 0 && strcmp(a, ".") != 0 && strcmp(a, "..") != 0) {
				view_popup_menu_folder(treeview, event, userdata);
			}
			else if (strcmp(b, "file") == 0) {
				view_popup_menu_file(treeview, event, userdata);
			}
			return TRUE; /* we handled this */
		}
	}
	return FALSE; /* we did not handle this */
}

gboolean view_onButtonPressed_joingroup(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
	{
		/* optional: select row if no row is selected or only one other row is selected (will only do something if you set a tree selection mode as described later in the tutorial) */
		GtkTreeSelection *selection;
		GtkTreeModel *model;
		GtkTreeIter   iter;
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		if (gtk_tree_selection_get_selected(selection, &model, &iter))
		{
			view_popup_menu_joingroup(treeview, event, userdata);
			return TRUE; /* we handled this */
		}
	}
	return FALSE; /* we did not handle this */
}

gboolean view_onButtonPressed_mygroup(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
	{
		GtkTreeSelection *selection;
		GtkTreeModel *model;
		GtkTreeIter   iter;
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		if (gtk_tree_selection_get_selected(selection, &model, &iter))
		{
			gchar *role_group;
			gtk_tree_model_get(model, &iter, 1, &role_group, -1);
			if (strcmp(role_group, "Member") == 0)
			{
				view_popup_menu_mygroup(treeview, event, userdata);
				return TRUE; /* we handled this */
			}
			else
			{
				view_popup_menu_mygroup2(treeview, event, userdata);
				return TRUE; /* we handled this */
			}
		}
	}
	return FALSE; /* we did not handle this */
}

gboolean view_onButtonPressed_member(GtkWidget *treeview, GdkEventButton *event, gpointer userdata, app_widgets *allwidgets)
{
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
	{
		GtkTreeSelection *selection;
		GtkTreeModel *model;
		GtkTreeIter   iter;
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		if (gtk_tree_selection_get_selected(selection, &model, &iter))
		{
			gchar *role_group;
			gtk_tree_model_get(model, &iter, 1, &role_group, -1);
			if (strcmp(role_group, "Member") == 0)
			{
				view_popup_menu_member(treeview, event, userdata);
				return TRUE; /* we handled this */
			}
		}
	}
	return FALSE; /* we did not handle this */
}

gboolean view_onButtonPressed_joininfo(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
	{
		GtkTreeSelection *selection;
		GtkTreeModel *model;
		GtkTreeIter   iter;
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		if (gtk_tree_selection_get_selected(selection, &model, &iter))
		{
			view_popup_menu_joininfo(treeview, event, userdata);
			return TRUE; /* we handled this */
		}
	}
	return FALSE; /* we did not handle this */
}

gboolean view_onButtonPressed_notification(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
	{
		GtkTreeSelection *selection;
		GtkTreeModel *model;
		GtkTreeIter   iter;
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		if (gtk_tree_selection_get_selected(selection, &model, &iter))
		{
			view_popup_menu_notification(treeview, event, userdata);
			return TRUE; /* we handled this */
		}
	}
	return FALSE; /* we did not handle this */
}

int fileset(GtkFileChooserButton *chooser)
{
	int i, offset;
	char *filepath;
	memset(namefileUp, '\0', sizeof(namefileUp));
	memset(pathfileUp, '\0', sizeof(pathfileUp));
	GtkFileChooser *filechooser = GTK_FILE_CHOOSER(chooser);
	filepath = gtk_file_chooser_get_filename(filechooser);
	sprintf(pathfileUp, "%s", filepath);
	offset = 0;
	for (i = strlen(pathfileUp); i > 0; i--)
	{
		if (pathfileUp[i] == '/')break;
		offset++;
	}
	memcpy(namefileUp, pathfileUp + strlen(pathfileUp) - offset + 1, offset - 1);
	printf("CHOOSER FILE : %s\n", pathfileUp);
	return 0;
}

int folderset(GtkFileChooserButton *chooser)
{
	char *folderpath;
	memset(namefileUp, '\0', sizeof(namefileUp));
	memset(pathfileUp, '\0', sizeof(pathfileUp));
	GtkFileChooser *folderchooser = GTK_FILE_CHOOSER(chooser);
	folderpath = gtk_file_chooser_get_filename(folderchooser);
	sprintf(pathdirDownload, "%s", folderpath);
	printf("CHOOSER FOLDER : %s\n", pathdirDownload);
	return 0;
}

//================================================================================================= UPLOAD FILE ==================
int upload_clicked(GtkButton *button, app_widgets *allwidgets)
{
	printf("UPLOAD CLICKED\n");
	FILE *upfile = fopen(pathfileUp, "rb");
	if (phaseUpload == 0)
	{
		if (upfile != NULL)
		{
			clientMsgSend("UFILE", strlen(namefileUp), namefileUp, client_sock);
			fclose(upfile);
			return 0;
		}
	}
	else
	{
		gtk_label_set_text(allwidgets->l_notice,"You can only upload files one by one");
		return -1;
	}
	return -1;
}

int b_clear_notification_clicked_cb(GtkButton *button)
{
	clientMsgSend("CLRNOTIF", 0, "", client_sock);
	gtk_list_store_clear(newdata->liststore5);
	clientMsgSend("MYNOTIF", 0, "", client_sock);
	return 0;
}

int b_newfolder_clicked_cb(GtkButton *button, GtkEntry *input_newfolder)
{
	printf("NEW FOLDER CLICKED\n");
	const gchar *folder_name = gtk_entry_get_text(input_newfolder);
	clientMsgSend("MKD", strlen((char*)folder_name), (char*)folder_name, client_sock);
	gtk_list_store_clear(newdata->liststore2);
	clientMsgSend("LIST", 0, "", client_sock);
	return 0;
}

int b_newgroup_clicked_cb(GtkButton *button, GtkEntry *input_newgroup)
{
	printf("NEW GROUP CLICKED\n");
	const gchar *groupname = gtk_entry_get_text(input_newgroup);
	clientMsgSend("GCREAT", strlen((char*)groupname), (char*)groupname, client_sock);
	gtk_list_store_clear(newdata->liststore1);
	clientMsgSend("MYGR", 0, "", client_sock);
	return 0;
}

int b_conect_clicked_cb(GtkButton *button, app_widgets* allwidgets)
{
	const gchar *addr = gtk_entry_get_text(allwidgets->addr);
	const gchar *port = gtk_entry_get_text(allwidgets->port);
	inet_aton((char *)addr, &SERV_ADDR);
	SERV_PORT = StringToNumber((char*)port);
	client_sock = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERV_PORT);
	server_addr.sin_addr = SERV_ADDR;
	if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) < 0) {
		printf("Error!Can not connect to sever! Client exit imediately!\n");
		return -1;
	}
	else
	{
		gtk_widget_show(allwidgets->window_auth);
		gtk_widget_hide(allwidgets->window_conect);
		return 0;
	}
}

int b_login_clicked_cb(GtkButton *button, app_widgets* allwidgets)
{
	message *recvMsg;
	printf("LOGIN CLICKED\n");
	const gchar *text1 = gtk_entry_get_text(allwidgets->username);
	clientMsgSend("USER", strlen((char*)text1), (char*)text1, client_sock);

	prefixReceive(client_sock);
	recvMsg = messageReceive(client_sock);
	if (recvMsg != NULL)
	{
		if (strcmp(recvMsg->Opcode, "C3001") != 0)
		{
			gtk_widget_show(allwidgets->im_notice_login);
			gtk_label_set_text(GTK_LABEL(allwidgets->labellogin), recvMsg->Payload);
			return -1;
		}
	}
	const gchar *text2 = gtk_entry_get_text(allwidgets->Password);
	clientMsgSend("PASS", strlen((char*)text2), (char*)text2, client_sock);

	prefixReceive(client_sock);
	recvMsg = messageReceive(client_sock);
	if (recvMsg != NULL)
	{
		if (strcmp(recvMsg->Opcode, "C2100") != 0)
		{
			gtk_widget_show(allwidgets->im_notice_login);
			gtk_label_set_text(GTK_LABEL(allwidgets->labellogin), recvMsg->Payload);
			return -1;
		}
	}

	phaseAuth = 1;

	clientMsgSend("MYGR", 0, "", client_sock);
	clientMsgSend("GLIST", 0, "", client_sock);
	clientMsgSend("JOININFO", 0, "", client_sock);
	clientMsgSend("MYNOTIF", 0, "", client_sock);

	gtk_entry_set_text(allwidgets->username, "");
	gtk_entry_set_text(allwidgets->Password, "");
	gtk_label_set_text(GTK_LABEL(allwidgets->labellogin), "");
	gtk_widget_hide(allwidgets->im_notice_login);

	gtk_widget_show(allwidgets->window_main);
	gtk_widget_hide(allwidgets->window_auth);
	SAFE_DEL(recvMsg);
	return 0;
}

int b_log_out_clicked_cb(GtkButton *button, app_widgets *allwidgets)
{
	printf("LOGOUT CLICKED\n");
	clientMsgSend("LOUT", 0, "", client_sock);
	return 0;
}

int b_exit_program_clicked_cb(GtkButton *button, app_widgets *allwidgets)
{
	printf("EXIT CLICKED\n");
	gtk_main_quit();
	exit(EXIT_SUCCESS);
}

void window_main_destroy_cb()
{
	gtk_main_quit();
	exit(EXIT_SUCCESS);

}
void window_conect_destroy_cb()
{
	gtk_main_quit();
	exit(EXIT_SUCCESS);

}

int login_destroy()
{
	gtk_main_quit();
	exit(EXIT_SUCCESS);
}

int b_signup_clicked_cb(GtkButton *button, app_widgets *allwidgets)
{
	printf("SIGN UP CLICKED\n");
	gtk_widget_show(allwidgets->im_notice_login);
	message *recvMsg = (message*)malloc(sizeof(message));
	printf("LOGIN CLICKED\n");
	const gchar *text1 = gtk_entry_get_text(allwidgets->username);
	clientMsgSend("CUSER", strlen((char*)text1), (char*)text1, client_sock);

	prefixReceive(client_sock);
	recvMsg = messageReceive(client_sock);
	if (recvMsg != NULL)
	{
		if (strcmp(recvMsg->Opcode, "C3000") != 0)
		{
			gtk_label_set_text(GTK_LABEL(allwidgets->labellogin), recvMsg->Payload);
			return -1;
		}
	}
	const gchar *text2 = gtk_entry_get_text(allwidgets->Password);
	clientMsgSend("CPASS", strlen((char*)text2), (char*)text2, client_sock);

	prefixReceive(client_sock);
	recvMsg = messageReceive(client_sock);
	if (recvMsg != NULL)
	{
		gtk_label_set_text(GTK_LABEL(allwidgets->labellogin), recvMsg->Payload);

	}
	SAFE_DEL(recvMsg);
	return 0;
}

int b_exitprogram_clicked_cb(GtkButton *button, app_widgets *allwidgets)
{
	printf("EXIT CLICKED\n");
	close(client_sock);
	gtk_main_quit();
	exit(EXIT_SUCCESS);
	return 0;
}
int b_reconect_clicked_cb(GtkButton *button, app_widgets *allwidgets)
{
	printf("RECONNECT CLICKED\n");
	gtk_widget_show(allwidgets->window_conect);
	gtk_widget_hide(allwidgets->window_main);
	gtk_widget_hide(allwidgets->window_warning);
	return 0;
}

int b_refresh_clicked_cb(GtkButton *button, app_widgets *allwidgets)
{
	printf("REFRESH CLICKED\n");
	gtk_list_store_clear(newdata->liststore3);
	gtk_list_store_clear(newdata->liststore4);
	clientMsgSend("GMEM", strlen(currentGroup), currentGroup, client_sock);
	clientMsgSend("GLIST", 0, "", client_sock);
	return 0;
}

void selectgroup(GtkTreeSelection *treeselection)
{
	// gchar *a, *b;
	// GtkTreeModel *model;
	// GtkTreeIter iter;
	// gtk_tree_selection_set_mode (treeselection,GTK_SELECTION_SINGLE);
	// if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
	// {
	// 	gtk_tree_model_get(model, &iter, 0, &a, 1, &b, -1);
	// 	printf("SELECT: %s-%s\n", a, b);
	// 	clientMsgSend("CHG", strlen(a), a, client_sock);
	// 	gtk_list_store_clear(newdata->liststore2);
	// 	gtk_list_store_clear(newdata->liststore3);
	// 	clientMsgSend("LIST", strlen(a), a, client_sock);
	// 	clientMsgSend("GMEM", strlen(a), a, client_sock);
	// 	memset(currentGroup,'\0',255);
	// 	sprintf(currentGroup,"%s",a);
	// 	g_free(a);
	// 	g_free(b);
	// }
}

void view_onRowDirectoryActivated(GtkTreeView *treeview)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	selection = gtk_tree_view_get_selection(treeview);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *name, *type;
		gtk_tree_model_get(model, &iter, 0, &name, 1, &type, -1);
		g_print("CLICKED ROW : %s\n", name);
		if (strcmp(type, "folder") == 0)
		{
			clientMsgSend("CWD", strlen(name), name, client_sock);
			gtk_list_store_clear(newdata->liststore2);
			clientMsgSend("LIST", 0, "", client_sock);
			clientMsgSend("PWD", 0, "", client_sock);
		}
		g_free(name);
		g_free(type);
	}
}

void view_onRowDirectoryActivated1(GtkTreeView *treeview)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	selection = gtk_tree_view_get_selection(treeview);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *name, *role;
		gtk_tree_model_get(model, &iter, 0, &name, 1, &role, -1);
		g_print("CLICKED ROW : %s\n", name);
		clientMsgSend("CHG", strlen(name), name, client_sock);
		gtk_list_store_clear(newdata->liststore2);
		gtk_list_store_clear(newdata->liststore3);
		clientMsgSend("LIST", 0, "", client_sock);
		clientMsgSend("PWD", 0, "", client_sock);
		clientMsgSend("GMEM", strlen(name), name, client_sock);
		memset(currentGroup, '\0', 255);
		sprintf(currentGroup, "%s", name);
		g_free(name);
		g_free(role);
	}
}

void newnotification() 
{
	printf("ALARM\n");
	if(have_new_notification==1)
	{
		have_new_notification=0;
		gtk_list_store_clear(newdata->liststore1);
		gtk_list_store_clear(newdata->liststore2);
		gtk_list_store_clear(newdata->liststore4);
		gtk_list_store_clear(newdata->liststore5);
		gtk_list_store_clear(newdata->liststore6);
		clientMsgSend("GLIST", 0, "", client_sock);
		clientMsgSend("LIST", 0, "", client_sock);
		clientMsgSend("MYGR", 0, "", client_sock);
		clientMsgSend("MYNOTIF", 0, "", client_sock);
		clientMsgSend("JOININFO", 0, "", client_sock);		
	}
	if(warning_disconnect==1)
	{
		warning_disconnect=0;
		gtk_widget_show(newdata->window_warning);
	}
	if(refresh_w_directtory==1)
	{
		refresh_w_directtory=0;
		gtk_list_store_clear(newdata->liststore2);
		clientMsgSend("LIST", 0, "", client_sock);
	}
	if(logout_success==1)
	{
		logout_success=0;
		gtk_list_store_clear(newdata->liststore1);
		gtk_list_store_clear(newdata->liststore2);
		gtk_list_store_clear(newdata->liststore3);
		gtk_list_store_clear(newdata->liststore4);
		gtk_list_store_clear(newdata->liststore5);
		gtk_list_store_clear(newdata->liststore6);
		gtk_label_set_text(GTK_LABEL(newdata->l_notice), "");
		gtk_widget_show(newdata->window_auth);
		gtk_widget_hide_on_delete(newdata->window_main);
	}
	alarm(0);
}

int main(int argc, char *argv[])
{
	app_widgets *allwidgets = g_slice_new(app_widgets);
	newdata = allwidgets;
	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "GLADE/filesharing.glade", NULL);
	gtk_builder_connect_signals(builder, allwidgets);

	allwidgets->window_auth = GTK_WIDGET(gtk_builder_get_object(builder, "window_login"));
	allwidgets->window_main = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
	allwidgets->window_conect = GTK_WIDGET(gtk_builder_get_object(builder, "window_conect"));
	allwidgets->window_warning = GTK_WIDGET(gtk_builder_get_object(builder, "window_warning"));
	gtk_window_set_title(GTK_WINDOW(allwidgets->window_main), "FILES SHARING CLIENT PROGRAM");
	gtk_window_set_title(GTK_WINDOW(allwidgets->window_auth), "SIGNUP LOGIN YOUR ACCT");
	gtk_window_set_title(GTK_WINDOW(allwidgets->window_conect), "CONNECT TO SERVER");

	allwidgets->username = GTK_ENTRY(gtk_builder_get_object(builder, "input_username"));
	allwidgets->Password = GTK_ENTRY(gtk_builder_get_object(builder, "input_password"));
	allwidgets->addr = GTK_ENTRY(gtk_builder_get_object(builder, "input_addr"));
	allwidgets->port = GTK_ENTRY(gtk_builder_get_object(builder, "input_port"));
	allwidgets->labellogin = GTK_WIDGET(gtk_builder_get_object(builder, "login_label"));

	allwidgets->l_notice = GTK_LABEL(gtk_builder_get_object(builder, "l_notice"));
	allwidgets->l_upload_path = GTK_LABEL(gtk_builder_get_object(builder, "l_upload_path"));
	allwidgets->l_download_path = GTK_LABEL(gtk_builder_get_object(builder, "l_download_path"));
	allwidgets->l_current_path = GTK_LABEL(gtk_builder_get_object(builder, "l_current_path"));

	allwidgets->s_upload = GTK_WIDGET(gtk_builder_get_object(builder, "s_upload"));
	allwidgets->s_download = GTK_WIDGET(gtk_builder_get_object(builder, "s_download"));
	allwidgets->im_notice_login = GTK_WIDGET(gtk_builder_get_object(builder, "im_notice_login"));

	allwidgets->liststore1 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore1"));		//my group
	allwidgets->liststore4 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore4"));		//list group
	allwidgets->liststore2 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore2"));    	//directory
	allwidgets->liststore3 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore3"));		//member
	allwidgets->liststore5 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore5"));		//notification
	allwidgets->liststore6 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore6"));		//join infomation

	allwidgets->mtree1 = GTK_TREE_MODEL(gtk_builder_get_object(builder, "liststore1"));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(allwidgets->mtree1), 1, GTK_SORT_ASCENDING);
	allwidgets->mtree2 = GTK_TREE_MODEL(gtk_builder_get_object(builder, "liststore2"));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(allwidgets->mtree2), 0, GTK_SORT_ASCENDING);
	allwidgets->mtree3 = GTK_TREE_MODEL(gtk_builder_get_object(builder, "liststore3"));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(allwidgets->mtree3), 1, GTK_SORT_ASCENDING);
	allwidgets->mtree4 = GTK_TREE_MODEL(gtk_builder_get_object(builder, "liststore4"));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(allwidgets->mtree4), 1, GTK_SORT_ASCENDING);

	allwidgets->w_mygroup = GTK_WIDGET(gtk_builder_get_object(builder, "treeview1"));
	allwidgets->w_directory = GTK_WIDGET(gtk_builder_get_object(builder, "treeview2"));
	allwidgets->w_member = GTK_WIDGET(gtk_builder_get_object(builder, "treeview3"));
	allwidgets->w_joingroup = GTK_WIDGET(gtk_builder_get_object(builder, "treeview4"));
	allwidgets->w_notification = GTK_WIDGET(gtk_builder_get_object(builder, "treeview5"));
	allwidgets->w_joininfo = GTK_WIDGET(gtk_builder_get_object(builder, "treeview7"));

	g_signal_connect(allwidgets->w_directory, "row-activated", (GCallback)view_onRowDirectoryActivated, NULL);
	g_signal_connect(allwidgets->w_mygroup, "row-activated", (GCallback)view_onRowDirectoryActivated1, NULL);

	g_signal_connect(allwidgets->w_directory, "button-press-event", (GCallback)view_onButtonPressed_directory, NULL);
	g_signal_connect(allwidgets->w_joingroup, "button-press-event", (GCallback)view_onButtonPressed_joingroup, NULL);
	g_signal_connect(allwidgets->w_mygroup, "button-press-event", (GCallback)view_onButtonPressed_mygroup, NULL);
	g_signal_connect(allwidgets->w_member, "button-press-event", (GCallback)view_onButtonPressed_member, NULL);
	g_signal_connect(allwidgets->w_joininfo, "button-press-event", (GCallback)view_onButtonPressed_joininfo, NULL);
	g_signal_connect(allwidgets->w_notification, "button-press-event", (GCallback)view_onButtonPressed_notification, NULL);

	refresh_w_mygroup = 0;
	refresh_w_joingroup = 0;
	refresh_w_directtory = 0;
	refresh_w_member = 0;
	refresh_w_notification = 0;
	refresh_w_joininfomation = 0;

	phaseAuth = 0;
	phaseUpload = 0;
	phaseDownload = 0;

	have_new_notification=0;
	warning_disconnect=0;
	logout_success=0;

	g_thread_new("thread_recv_message_update UI", filterMessage, allwidgets);

	if (signal(SIGALRM,newnotification)==SIG_ERR)
	{
		perror("\nSIGALRM");
		exit(EXIT_SUCCESS);
	}

	gtk_widget_show(allwidgets->window_conect);
	gtk_widget_hide(allwidgets->s_upload);
	gtk_widget_hide(allwidgets->s_download);
	gtk_widget_hide(allwidgets->im_notice_login);

	g_object_unref(builder);
	gtk_main();

	return 0;
}

gpointer filterMessage(gpointer data)
{
	app_widgets *allwidgets = (app_widgets *)data;
	printf("+ THREAD RECV RETURN DATA\n");
	int prefix;
	GtkTreeIter	iter;
	while (1)
	{
		if (phaseAuth == 1)
		{
			prefix = prefixReceive(client_sock);
			printf("PREFIX : %d\n", prefix);
			if (prefix == PF_MESSAGE)
			{
				handleMessage(data);
			}
			else if (prefix == PF_MEMBER)
			{
				member *member_data = memberReceive(client_sock);
				gtk_list_store_prepend(allwidgets->liststore3, &iter);
				if (member_data->role == R_ADMIN)
				{
					gtk_list_store_set(allwidgets->liststore3, &iter, 0, member_data->username, 1, "Admin", 2, member_data->groupname, -1);
				}
				else
				{
					gtk_list_store_set(allwidgets->liststore3, &iter, 0, member_data->username, 1, "Member", 2, member_data->groupname, -1);
				}
				SAFE_DEL(member_data);
			}
			else if (prefix == PF_DIRECTORY)
			{
				direct *direct_data = directReceive(client_sock);
				gtk_list_store_insert_before(allwidgets->liststore2, &iter, NULL);
				if (direct_data->type == 4)
				{
					gtk_list_store_set(allwidgets->liststore2, &iter, 0, direct_data->name, 1, "folder", 2, NULL, -1);
				}
				if (direct_data->type == 8)
				{
					gtk_list_store_set(allwidgets->liststore2, &iter, 0, direct_data->name, 1, "file", 2, direct_data->size, -1);
				}
				SAFE_DEL(direct_data);
			}
			else if (prefix == PF_JOIN_INFOMATION)
			{
				join_notification *join_data = joinNotiReceive(client_sock);
				char * temp = (char*)calloc(1024, sizeof(char));
				gtk_list_store_append(allwidgets->liststore6, &iter);
				sprintf(temp, "%s wants to join %s", join_data->joiner, join_data->name_group);
				gtk_list_store_set(allwidgets->liststore6, &iter, 0, join_data->id_notification, 1, join_data->time_create, 2, temp, -1);
				SAFE_DEL(temp);
				SAFE_DEL(join_data);
			}
			else if (prefix == PF_NOTIFICATION)
			{
				notification *notifi_data = notificationReceive(client_sock);
				gtk_list_store_append(allwidgets->liststore5, &iter);
				gtk_list_store_set(allwidgets->liststore5, &iter, 0, notifi_data->id_notification, 1, notifi_data->time_create, 2, notifi_data->content, -1);
				SAFE_DEL(notifi_data);
			}
			else if (prefix == PF_MY_GROUP)
			{
				group *group_data = groupReceive(client_sock);
				gtk_list_store_append(allwidgets->liststore1, &iter);
				if (group_data->role == R_ADMIN)
				{
					gtk_list_store_set(allwidgets->liststore1, &iter, 0, group_data->group_name, 1, "Admin", -1);
				}
				else
				{
					gtk_list_store_set(allwidgets->liststore1, &iter, 0, group_data->group_name, 1, "Member", -1);
				}
				SAFE_DEL(group_data);
			}
			else if (prefix == PF_PUBLIC_GROUP)
			{
				publicgroup *public_group_data = publicgroupReceive(client_sock);
				gtk_list_store_append(allwidgets->liststore4, &iter);
				if (public_group_data->group_status == ST_CAN_JOIN)
				{
					gtk_list_store_set(allwidgets->liststore4, &iter, 0, public_group_data->group_name, 1, "Can join", -1);
				}
				else if (public_group_data->group_status == ST_CANNOT_JOIN)
				{
					gtk_list_store_set(allwidgets->liststore4, &iter, 0, public_group_data->group_name, 1, "Can not join", -1);
				}
				SAFE_DEL(public_group_data);
			}
			else if (prefix == -1)
			{
				pthread_mutex_lock (&mutex);
				printf("=> Server error.No Connection \n");
				phaseAuth=0;
				warning_disconnect=1;
				alarm(1);
				pthread_mutex_unlock (&mutex);
			}
		}
		else
		{
			usleep(100000);
		}
	}
}


gpointer handleMessage(gpointer data)
{
	app_widgets *allwidgets = (app_widgets *)data;
	message *recvMsg1;
	recvMsg1 = messageReceive(client_sock);
	if (recvMsg1 != NULL)
	{
		printf("RECV: %s\n", recvMsg1->Opcode);
		if (recvMsg1->Opcode[1] != '1'&& strcmp(recvMsg1->Opcode, "C2312") != 0 && strcmp(recvMsg1->Opcode, "TRANF") != 0)
		{
			gtk_label_set_text(allwidgets->l_notice, recvMsg1->Payload);
		}
		//=======================================================================DOWNLOAD FILE=======================================
		if (strcmp(recvMsg1->Opcode, "TRANF") == 0)
		{
			if (phaseDownload == 1)
			{
				if (recvMsg1->Lenght == 0)
				{
					phaseDownload = 0;
					gtk_label_set_text(allwidgets->l_download_path, "");
					gtk_widget_hide(allwidgets->s_download);
				}
				else
				{
					FILE* my_file = fopen(pathfileDownload, "ab+");
					fwrite(recvMsg1->Payload, 1, recvMsg1->Lenght, my_file);
					fclose(my_file);
				}
			}
		}
		else if (strcmp(recvMsg1->Opcode, "C1309") == 0)
		{
			gtk_label_set_text(allwidgets->l_current_path,recvMsg1->Payload);
		}
		else if (strcmp(recvMsg1->Opcode, "C2308") == 0)
		{
			phaseUpload = 1;
			g_thread_new("THREAD_UPLOAD_FILE", uploadFile, data);
		}
		//=======================================================================DOWNLOAD FILE=========================================
		else if (strcmp(recvMsg1->Opcode, "C2309") == 0)
		{
			memset(pathfileDownload, '\0', sizeof(pathfileDownload));
			sprintf(pathfileDownload, "%s/%s", pathdirDownload, namefileDown);
			phaseDownload = 1;
			gtk_label_set_text(allwidgets->l_download_path, namefileDown);
			gtk_widget_show(allwidgets->s_download);
		}
		//=======================================================================UPLOAD SUCCSESS==================================
		else if (strcmp(recvMsg1->Opcode, "C2313") == 0)
		{
			refresh_w_directtory=1;
			ualarm(1, 100000);
		}
		//=======================================================================LOGOUT==================================
		else if (strcmp(recvMsg1->Opcode, "C2311") == 0)
		{
			logout_success=1;
			ualarm(1, 100000);
			phaseAuth = 0;
		}
		//=======================================================================NEW NOTIFICATION====================================
		else if (strcmp(recvMsg1->Opcode, "M0000") == 0)
		{
			have_new_notification=1;
			alarm(1);
		}
		SAFE_DEL(recvMsg1);
	}
	return 0;
}

gpointer uploadFile(gpointer data)
{
	printf("+ THREAD UPLOAD FILE TO SERVER\n");
	gtk_label_set_text(newdata->l_upload_path, pathfileUp);
	gtk_widget_show(newdata->s_upload);
	int nLeft, ret;
	FILE *my_file;
	char buff[BUFF_SIZE];
	unsigned int sizeSendFile = SizeFile(pathfileUp);
	my_file = fopen(pathfileUp, "rb");
	nLeft = sizeSendFile;
	while (nLeft > 0)
	{
		usleep(0);
		memset(buff, '\0', BUFF_SIZE);
		ret = fread(buff, 1, BUFF_SIZE, my_file);
		clientMsgSend("TRANF", ret, buff, client_sock);
		nLeft -= ret;
	}
	clientMsgSend("TRANF", 0, "", client_sock);
	phaseUpload = 0;
	gtk_label_set_text(newdata->l_upload_path, "");
	gtk_widget_hide(newdata->s_upload);
	return 0;
}
