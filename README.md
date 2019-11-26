
/*====================================================================*/
                           LIST COMMAND
/*====================================================================*/
                                
-CUSER 		len Username 		: Tao tai khoan moi 
-CPASS 		len Password 		: Tao pass tai khoan moi

-USER  		len Username 		: Xac thuc ten tai khoan
-PASS  		len Password 		: Xac thuc pass
-LOUT 		0 	""				: Dang xuat 

-GCREAT 	len Groupname		: Tao nhom moi
-GLIST 		0 	"" 				: Cac nhom co the join
-MYGR		0	""				: Cac nhom da tham gia
-GMEM		len Groupname		: Get thanh vien cua nhom
-KICK		len Gname:Mname		: Xoa thanh vien ra khoi nhom
-GLEAVE 	len Groupname 		: Roi khoi nhom

-JOIN  		len Groupname 		: Xin tham gia vao nhom
-ACEPT 		len ID_Join 		: Cho phep tham gia nhom  (Lay ID_Join qua lenh JOININFO)
-RJECT 		len ID_Join 		: Tu choi yeu cau tham gia	(Lay ID_Join qua lenh JOININFO)
-MYNOTIF	0	""				: Get thong bao gui den tai khoan
-JOININFO	0	""				: Thong tin cac yeu cau xin tham gia nhom (Kem ID_Join -> ACEPT or RJECT)
-CLRNOTIF	0	""				: Clear tat ca thong bao 

-LIST 		0 	"" 				: Get danh sach file,thu muc cua nhom
-MKD		len Foldename		: Tao thu muc
-RMD    	len Foldename 		: Xoa thu muc
-DELE   	len Filename 		: Xoa file
-UFILE  	len Filename 		: Gui yeu cau upload
-SIZE   	len Filename 		: Get kich thuoc file trong nhom
-DFILE  	len Filename 		: Gui yeu cau download file trong nhom
-TRANF  	len Data            : Truyen du lieu (Upload,Download file)
-TRANF  	0 	""				: Ket thuc truyen du lieu

-CWD    	len Foldename 		: Truy cap thu muc trong nhom
-CHG		len Groupname		: Truy cap vao nhom

-EXIT 		0	""				: Client thoat chuong trinh, ngat ket noi

/*=====================================================================*/
                          LIST RETURN CODES

1xyz 	=> 1: "yeu cau du lieu thanh cong,co du lieu tra ve"					
2xyz	=> 2: "yeu cau thuc hien thanh cong"							
3xyz	=> 3: "yeu cau thanh cong, can tiep tuc de hoan thanh"		
5xyz	=> 5: "yeu cau duoc thuc hien, ket qua tra ve khong mong muon"			
6xyz 	=> 6: "yeu cau khong duoc thuc hien"							
7xyz	=> 7: "khong xac dinh duoc yeu cau"							

/*=====================================================================*/

#define M0000 "You have new notifications"

#define C1300 "Get account information successfully"
#define C1301 "Get group list successful"
#define C1302 "Get the directory listing successful"
#define C1303 "Get file size successful"
#define C1304 "You have requested to join your group"
#define C1305 "Get your group join request information successful"
#define C1306 "Get the group members information successful"
#define C1307 "Get the notification successful"

#define C2100 "Logged in successfully"
#define C2200 "Successful account registration"
#define C2300 "Create new group successfully"	
#define C2301 "Join the group"				
#define C2302 "Added user to group"
#define C2303 "Deny to join the group"
#define C2304 "Leave the group successful"
#define C2305 "Create directory success"
#define C2306 "Delete directory success"
#define C2307 "Delete file success"
#define C2308 "Accept the file upload request"
#define C2309 "Accept the file download request"
#define C2310 "Update current directory"
#define C2311 "Successful logout"
#define C2312 "Recv data ok"
#define C2313 "Upload file success"
#define C2314 "Move to group success"
#define C2315 "Has sent a request to join the group"
#define C2316 "Leave the group successful"
#define C2317 "Remove members from the group successful"
#define C2318 "Clear all notification successful"

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
#define C6305 "Error. You need access to a group"
#define C6306 "Logged in, the request is not accepted."
#define C6307 "Your request already exists. Wait for group admin to approve"
#define C6308 "Accep request are not accepted"
#define C6309 "Deny request not accepted"
#define C6310 "Sorry, only members can get information"
#define C6311 "Member removal requests are not accepted"

#define C7000 "The syntax is unknown"
#define C7100 "The syntax is unknown"
#define C7200 "The syntax is unknown"
#define C7300 "The syntax is unknown"


=======================================================================

#define M0000 "Ban co thong bao moi"

#define C1300 "Lay thong tin tai khoan thanh cong"
#define C1301 "Lay danh sach nhom thanh cong"
#define C1302 "Lay danh sach file, thu muc thanh cong"
#define C1303 "Lay kich thuoc file thanh cong"
#define C1304 "Co nguoi muon xin vao nhom cua ban"
#define C1305 "Lay danh sach thong bao thanh cong"
#define C1306 "Lay danh sach thanh vien cua nhom thanh cong"
#define C1307 "Lay danh sach thong bao thanh cong"

#define C2100 "Dang nhap thanh cong"
#define C2200 "Tao tai khoan moi thanh cong"
#define C2300 "Tao nhom moi thanh cong"	
#define C2301 "Xin vao nhom thanh cong"				
#define C2302 "Da chap nhan yeu cau tham gia nhom"
#define C2303 "Da tu choi yeu cau tham gia nhom"
#define C2304 "Da roi khoi nhom"
#define C2305 "Tao thu muc thanh cong"
#define C2306 "Xoa thu muc thanh cong"
#define C2307 "Xoa file thanh cong"
#define C2308 "Da chap nhan yeu cau upload file"
#define C2309 "Da chap nhan yeu cau download file"
#define C2310 "Da cap nhat duong dan hien tai"
#define C2311 "Dang xuat thanh cong"
#define C2312 "Tranfer OK"
#define C2313 "Upload file thanh cong"
#define C2314 "Truy cap vao nhom thanh cong"
#define C2315 "Da gui yeu cau tham gia nhom den admin"
#define C2316 "Roi nhom thanh cong"
#define C2317 "Da xoa thanh vien ra khoi nhom cua ban"
#define C2318 "Da don sach toan bo thong bao"

#define C3000 "Ten tai khoan hop le, can tao mat khau de hoan thanh viec dang ky"
#define C3001 "Ten dang nhap dung, can mat khau"

#define C5000 "Ten tai khoan nay da ton tai"
#define C5001 "Tai khoan nay khong ton tai" 
#define C5002 "Tai khoan da bi khoa"
#define C5003 "Tai khoan nay hien dang dang nhap o mot noi khac"
#define C5100 "Sai mat khau"
#define C5101 "Sai mat khau, tai khoan cua ban da bi khoa"
#define C5102 "Xin loi, ban khong the tiep tuc dang nhap vi tai khoan da bi khoa"
#define C5300 "Ten nhom da ton tai"
#define C5301 "Thu muc da ton tai"
#define C5302 "Nhom khong ton tai, khong the truy cap"
#define C5303 "Thu muc khong ton tai"
#define C5304 "File da ton tai "
#define C5305 "File khong ton tai, khong the download"
#define C5306 "Error"
#define C5307 "Xin loi, chi thanh vien moi co the dang nhap"
#define C5308 "Yeu cau tham gia vao nhom khong duoc thuc hien"
#define C5309 "Yeu cau roi nhom khong duoc chap nhan"

#define C6000 "Ban can xac nhan ten dang nhap truoc khi gui mat khau"
#define C6001 "Ban khong the dang xuat vi ban chua dang nhap"
#define C6002 "Ban chua dang nhap, yeu cau khong duoc xu ly"
#define C6100 "Dang nhap chua hoan tat, yeu cau khong duoc xu ly"
#define C6101 "Ban dang xac thuc mot tai khoan khac"
#define C6102 "Ban khong the dang xuat vi ban chua dang nhap"
#define C6200 "Ban dang tao tai khoan, thao tac khong duoc xu ly"
#define C6201 "Mat khau khong duoc de trong"
#define C6202 "Ten tai khoan khong duoc de trong"
#define C6300 "Da dang nhap, ban khong the dang nhap mot tai khoan khac"
#define C6301 "Ban khong co quyen thuc hien hanh dong nay"
#define C6302 "Ten file khong hop le"
#define C6303 "Ban khong the gui du lieu len bay gio"
#define C6304 "Day dang la thu muc goc"
#define C6305 "Ban can truy cap vao nhom"
#define C6306 "Ban da dang nhap, yeu cau khong duoc xu ly"
#define C6307 "Yeu cau tham gia da duoc gui, ban can cho admin xac nhan"
#define C6308 "Yeu cau them thanh vien khong duoc chap nhan"
#define C6309 "Yeu cau tu choi khong duoc chap nhan"
#define C6310 "Xin loi, chi thanh vien moi co the lay thong tin"
#define C6311 "Yeu cau xoa thanh vien khong duoc chap nhan"

#define C7000 "Loi cu phap, khong xac dinh duoc command"
#define C7100 "Loi cu phap, khong xac dinh duoc command"
#define C7200 "Loi cu phap, khong xac dinh duoc command"
#define C7300 "Loi cu phap, khong xac dinh duoc command"