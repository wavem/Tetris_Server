//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "cxClasses"
#pragma link "cxControls"
#pragma link "cxGraphics"
#pragma link "cxLookAndFeelPainters"
#pragma link "cxLookAndFeels"
#pragma link "dxBar"
#pragma link "dxRibbon"
#pragma link "dxRibbonCustomizationForm"
#pragma link "dxRibbonSkins"
#pragma link "dxSkinBlack"
#pragma link "dxSkinBlue"
#pragma link "dxSkinBlueprint"
#pragma link "dxSkinCaramel"
#pragma link "dxSkinCoffee"
#pragma link "dxSkinDarkRoom"
#pragma link "dxSkinDarkSide"
#pragma link "dxSkinDevExpressDarkStyle"
#pragma link "dxSkinDevExpressStyle"
#pragma link "dxSkinFoggy"
#pragma link "dxSkinGlassOceans"
#pragma link "dxSkinHighContrast"
#pragma link "dxSkiniMaginary"
#pragma link "dxSkinLilian"
#pragma link "dxSkinLiquidSky"
#pragma link "dxSkinLondonLiquidSky"
#pragma link "dxSkinMcSkin"
#pragma link "dxSkinMetropolis"
#pragma link "dxSkinMetropolisDark"
#pragma link "dxSkinMoneyTwins"
#pragma link "dxSkinOffice2007Black"
#pragma link "dxSkinOffice2007Blue"
#pragma link "dxSkinOffice2007Green"
#pragma link "dxSkinOffice2007Pink"
#pragma link "dxSkinOffice2007Silver"
#pragma link "dxSkinOffice2010Black"
#pragma link "dxSkinOffice2010Blue"
#pragma link "dxSkinOffice2010Silver"
#pragma link "dxSkinOffice2013DarkGray"
#pragma link "dxSkinOffice2013LightGray"
#pragma link "dxSkinOffice2013White"
#pragma link "dxSkinPumpkin"
#pragma link "dxSkinsCore"
#pragma link "dxSkinsDefaultPainters"
#pragma link "dxSkinsdxBarPainter"
#pragma link "dxSkinsdxRibbonPainter"
#pragma link "dxSkinSeven"
#pragma link "dxSkinSevenClassic"
#pragma link "dxSkinSharp"
#pragma link "dxSkinSharpPlus"
#pragma link "dxSkinSilver"
#pragma link "dxSkinSpringTime"
#pragma link "dxSkinStardust"
#pragma link "dxSkinSummer2008"
#pragma link "dxSkinTheAsphaltWorld"
#pragma link "dxSkinValentine"
#pragma link "dxSkinVisualStudio2013Blue"
#pragma link "dxSkinVisualStudio2013Dark"
#pragma link "dxSkinVisualStudio2013Light"
#pragma link "dxSkinVS2010"
#pragma link "dxSkinWhiteprint"
#pragma link "dxSkinXmas2008Blue"
#pragma link "AdvSmoothButton"
#pragma link "AdvMemo"
#pragma link "AdvGrid"
#pragma link "AdvObj"
#pragma link "BaseGrid"
#pragma link "AdvGrid"
#pragma link "AdvObj"
#pragma link "BaseGrid"
#pragma resource "*.dfm"
TFormMain *FormMain;
//---------------------------------------------------------------------------
__fastcall TFormMain::TFormMain(TComponent* Owner)
	: TForm(Owner)
{
	InitProgram();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::FormClose(TObject *Sender, TCloseAction &Action)
{
	ExitProgram();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::InitProgram() {

	// Common
	UnicodeString t_Str = L"";

	// Set First Notebook Page
	Notebook_Main->PageIndex = 0; // Status

	// Set TrayIcon
	TrayIcon->ShowBalloonHint();

	// Init
	m_ClientCnt = 0;
	m_TCPListenThread = NULL;
	for(int i = 0 ; i < MAX_TCP_CLIENT_USER_COUNT ; i++) {
		m_ClientSocket[i] = INVALID_SOCKET;
		m_Client[i] = NULL;
	}

	// Init Sender Thread
	for(int i = 0 ; i < MAX_SENDER_THREAD_COUNT ; i++) {
		m_SenderThread[i] = NULL;
		m_SenderThreadWorkCount[i] = 0;
	}

	// Init Grid
	InitGrid();

	// Create Mutex
	m_Mutex = CreateMutex(NULL, true, NULL);
	ReleaseMutex(m_Mutex);

	// Socket
	WSADATA data;
	WORD version;
	int ret = 0;

	version = MAKEWORD(2, 2);
	ret = WSAStartup(version, &data);
	if(ret != 0) {
		ret = WSAGetLastError();
		if(ret == WSANOTINITIALISED) {
			t_Str.sprintf(L"Socket not initialised (error code : %d)", ret);
			PrintMsg(t_Str);
		} else {
			t_Str.sprintf(L"Socket error (error code : %d)", ret);
			PrintMsg(t_Str);
		}
	} else {
		PrintMsg(L"Socket init success");
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ExitProgram() {

	// Delete TCP Listen Socket
	if(m_TCPListenSocket != INVALID_SOCKET) {
		closesocket(m_TCPListenSocket);
		m_TCPListenSocket = INVALID_SOCKET;
	}

	// Turn Off TCP Listen Thread Routine
	if(m_TCPListenThread) {
		m_TCPListenThread->DoTerminate();
		m_TCPListenThread->Terminate();
		delete m_TCPListenThread;
		m_TCPListenThread = NULL;
	}

	// Delete Client Socket
	for(int i = 0 ; i < MAX_TCP_CLIENT_USER_COUNT ; i++) {
		if(m_ClientSocket[i] != INVALID_SOCKET) {
			closesocket(m_ClientSocket[i]);
			m_ClientSocket[i] = INVALID_SOCKET;
		}
	}

	// Delete Client Thread
	for(int i = 0 ; i < MAX_TCP_CLIENT_USER_COUNT ; i++) {
		if(m_Client[i]) {
			m_Client[i]->Terminate();
			delete m_Client[i];
		}
	}

	// Delete Sender Thread
	for(int i = 0 ; i < MAX_SENDER_THREAD_COUNT ; i++) {
		if(m_SenderThread[i]) {
			m_SenderThread[i]->DoTerminate();
			//m_cv_ClientMsgQ.notify_all();
			m_SenderThread[i]->Terminate();
			delete m_SenderThread[i];
			m_SenderThread[i] = NULL;
		}
	}

	// Socket Clean Up
	WSACleanup();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::InitGrid() {
	// Common
	UnicodeString tempStr = L"";

	// Load Images
	TBitmap* t_bmp = new TBitmap;
	t_bmp->LoadFromFile(L".\\Images\\green.bmp");
	ImgList->Add(t_bmp, t_bmp);
	t_bmp->FreeImage();

	t_bmp->LoadFromFile(L".\\Images\\gray.bmp");
	ImgList->Add(t_bmp, t_bmp);
	t_bmp->FreeImage();

	delete t_bmp;

	// Setting
	grid->ControlLook->NoDisabledButtonLook = true;

	// Init Grid
	int t_RowCnt = grid->RowCount;
	for(int i = 1 ; i < t_RowCnt ; i++) {
		grid->Cells[0][i] = i; // Idx
		grid->AddImageIdx(1, i, 1, haCenter, Advgrid::vaCenter); // State
		//grid->Cells[2][i] = L"basslover7022"; // ID
		//grid->Cells[3][i] = L"192.168.220.201"; // IP
		//grid->Cells[4][i] = L"65535"; // Port
		//grid->Cells[5][i] = L"Lobby"; // Status
		grid->AddButton(6, i, 70, 24, L"View", haCenter, Advgrid::vaCenter); // View
		//grid->Cells[7][i] = L"07"; // Last Message
		//grid->Cells[8][i] = L"2020-08-04 14:22:33"; // Connection Time
		//grid->Cells[9][i] = L"2020-08-04 14:22:33"; // Disconnection Time
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::btn_TestClick(TObject *Sender)
{
	// Common
	UnicodeString tempStr = L"";
	PrintMsg(L"TEST BUTTON CLICKED");


// Below is Message Queue Test Code
#if 0
	DWORD ret = 0;
	CLIENTMSG t_ClientMsg;
	memset(&t_ClientMsg, 0, sizeof(t_ClientMsg));

	// Create Mutex
	for(int i = 0 ; i < 1000; i++) {
		ret = WaitForSingleObject(m_Mutex, 2000);
		if(ret == WAIT_FAILED) {
			tempStr = L"wait failed";
		} else if(ret == WAIT_ABANDONED) {
			tempStr = L"wait abandoned";
		} else if(ret == WAIT_TIMEOUT) {
			tempStr = L"Wait Time out!!";
		} else if(ret == WAIT_OBJECT_0) {
			tempStr = L"Pushed into Queue";
			for(int i = 0 ; i < 100 ; i++) {
				m_ClientMsgQ.push(t_ClientMsg);
			}
		} else {
			tempStr = L"ETC";
		}
	//	PrintMsg(tempStr);

		ReleaseMutex(m_Mutex);
	}
	tempStr.sprintf(L"Queue Size : %d", m_ClientMsgQ.size());
	PrintMsg(tempStr);
#endif
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::btn_ListenClick(TObject *Sender)
{
	if(m_TCPListenThread) {
		PrintMsg(L"Server is already listening...");
		return;
	}

	if(CreateTCPListenSocket() == false) return;
	PrintMsg(L"Success to create Listening Socket");

	// Create TCP Listen Thread
	m_TCPListenThread = new CTCPListenThread(&m_TCPListenSocket, m_ClientSocket, &m_ClientCnt);
	PrintMsg(L"TCP Listen Thread Start...");

	// Create Sender Thread
	for(int i = 0 ; i < MAX_SENDER_THREAD_COUNT ; i++) {
		//m_SenderThread[i] = new DataSenderThread(i, &m_Mutex_ClientMsgQ, &m_cv_ClientMsgQ);
		m_SenderThread[i] = new DataSenderThread(i, m_Mutex);
	}
	PrintMsg(L"Sender Thread 1~10 Start...");
}
//---------------------------------------------------------------------------

bool __fastcall TFormMain::CreateTCPListenSocket() {

	// Common
	UnicodeString tempStr = L"";

	// Create Listen Socket
	m_TCPListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(m_TCPListenSocket == INVALID_SOCKET) {
		tempStr = L"Socket create fail";
		PrintMsg(tempStr);
		return false;
	}

	struct sockaddr_in	t_sockaddr_in;
	memset(&t_sockaddr_in, 0, sizeof(t_sockaddr_in));
	t_sockaddr_in.sin_family = AF_INET;
	t_sockaddr_in.sin_addr.s_addr = htonl(INADDR_ANY);
	t_sockaddr_in.sin_port = htons(TCP_SERVER_PORT);

	// Set Socket Option : REUSE
	int t_SockOpt = 1;
	setsockopt(m_TCPListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&t_SockOpt, sizeof(t_SockOpt));

	// Bind Socket
	if(bind(m_TCPListenSocket, (struct sockaddr*)&t_sockaddr_in, sizeof(t_sockaddr_in)) < 0) {
		tempStr = L"Socket bind fail";
		PrintMsg(tempStr);
		return false;
	}

	// Listen Socket
	if(listen(m_TCPListenSocket, MAX_TCP_CLIENT_LISTENING_COUNT) < 0) {
		tempStr = L"Socket Listen Fail";
		PrintMsg(tempStr);
		return false;
	}

	tempStr = L"Socket is ready";
	PrintMsg(tempStr);
	return true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::btn_StopClick(TObject *Sender)
{
	if(!m_TCPListenThread) {
		PrintMsg(L"There is no TCP Listen Thread");
		return;
	}
	m_TCPListenThread->Stop();
	PrintMsg(L"TCP Listen Thread Stoped");
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::btn_ResumeClick(TObject *Sender)
{
	if(!m_TCPListenThread) {
		PrintMsg(L"There is no TCP Listen Thread");
		return;
	}
	m_TCPListenThread->Resume();
	PrintMsg(L"TCP Listen Thread Resume");
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::btn_TerminateClick(TObject *Sender)
{
	if(!m_TCPListenThread) {
		PrintMsg(L"There is no TCP Listen Thread");
		return;
	}

	m_TCPListenThread->DoTerminate();
	if(m_TCPListenThread->GetThreadStatus() == THREAD_TERMINATED) {
		m_TCPListenThread->Terminate();
		delete m_TCPListenThread;
		m_TCPListenThread = NULL;
		PrintMsg(L"TCP Listen Thread Terminated");
	} else {
		PrintMsg(L"TCP Listen Thread is not Terminated");
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::btn_GetRunningTimeClick(TObject *Sender)
{
	if(!m_TCPListenThread) {
		PrintMsg(L"There is no TCP Listen Thread");
		return;
	}
	TTime t_StartTime, t_CurrentTime;
	//t_StartTime = m_TCPListenThread->GetStartTime();
	//t_CurrentTime = m_TCPListenThread->GetCurrentTime();
	PrintMsg(L"Start : " + t_StartTime.TimeString() + L" ");
	PrintMsg(L"Current : " + t_CurrentTime.TimeString() + L" ");
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PrintMsg(UnicodeString _str) {
	int t_Idx = memo->Lines->Add(_str);
	memo->SetCursor(0, t_Idx);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PrintLog(UnicodeString _str) {
	int t_Idx = memo_log->Lines->Add(_str);
	memo_log->SetCursor(0, t_Idx);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::Exit1Click(TObject *Sender)
{
	//FormMain->Close();
	// Terminate 를 하면 Form Close() 로 접근하는 일은 없다.
	// 하지만, Application 자체를 Terminate() 해도 문제 되는건 없는 것 같다.
	// 따라서 이 내용을 걍 메모만 해 두겠음.
	Application->Terminate();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::TrayIconDblClick(TObject *Sender)
{
	UnicodeString t_strPW = L"";
	TFormPassword *dlg = new TFormPassword(NULL, &t_strPW);
	dlg->ShowModal();
	delete dlg;

	if(t_strPW == L"1212") {
		FormMain->Show();
	} else if(t_strPW == L"") {
		// Do Nothing
	} else {
		ShowMessage(L"PASSWORD INCORRECT");
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MenuBtn_StatusClick(TObject *Sender)
{
	Notebook_Main->PageIndex = 0; // Status
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MenuBtn_SettingClick(TObject *Sender)
{
	Notebook_Main->PageIndex = 1; // Setting
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::btn_HideClick(TObject *Sender)
{
	FormMain->Hide();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PrintThreadMessage(TMessage &_msg) {
	unsigned int t_wParam = _msg.WParam;
	int t_lParam = _msg.LParam;

	UnicodeString tempStr = L"";
	UnicodeString *p = NULL;
	p = (UnicodeString*)t_wParam;
	tempStr = *p;
	//tempStr.sprintf(L"W : %08x, L : %08x", t_wParam, t_lParam);
	int t_Idx = memo->Lines->Add(tempStr);
	memo->SetCursor(0, t_Idx);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::tm_FindClientTimer(TObject *Sender)
{
	// Common
	UnicodeString tempStr = L"";
	BYTE t_Buffer[5] = {0, };
	t_Buffer[0] = 0x59;
	t_Buffer[1] = 0x02;
	t_Buffer[2] = 0x03;
	t_Buffer[3] = 0x04;
	t_Buffer[4] = 0x05;
	int t_rst = 0;

	for(int i = 0 ; i < MAX_TCP_CLIENT_USER_COUNT ; i++) {
		if(m_ClientSocket[i] != INVALID_SOCKET) {
			//t_rst = send(m_ClientSocket[i], (char*)t_Buffer, 5, 0);
			//tempStr.sprintf(L"Socket[%d] -> Send Result : %d", i, t_rst);
			//PrintLog(tempStr);
			t_rst = 0;
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::AddClient(TMessage &_msg) {

	// Common
	UnicodeString tempStr = L"";
	UnicodeString t_IPStr = L"";
	UnicodeString t_DateTimeStr = L"";
	unsigned int t_wParam = _msg.WParam;
	CLIENTINFO* p_ClientInfo;
	CLIENTINFO t_ClientInfo;
	memset(&t_ClientInfo, 0, sizeof(t_ClientInfo));
	int t_Port = 0;
	int t_ClientIdx = 0;
	TDateTime t_DateTime;

	// Receive Client Info
	p_ClientInfo = (CLIENTINFO*)t_wParam;
	t_ClientInfo = *p_ClientInfo;

	// Extract Client Index
	t_ClientIdx = t_ClientInfo.ClientIndex;

	// Create Client Thread
	m_Client[t_ClientIdx] = new ClientThread(&(m_ClientSocket[t_ClientIdx]) ,t_ClientInfo);

	// Refresh Client Info Grid
	RefreshClientInfoGrid();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::RefreshClientInfoGrid() {

	// Common
	UnicodeString tempStr = L"";
	int t_FixedIdx = 0;
	UnicodeString t_IPStr = L"";
	UnicodeString t_DateTimeStr = L"";
	int t_Port = 0;
	int t_ClientIdx = 0;
	TDateTime t_DateTime;

	// Refresh Routine
	for(int i = 0 ; i < MAX_TCP_CLIENT_USER_COUNT ; i++) {
		// Fix Index Number for Grid
		t_FixedIdx = i + 1;

		// Check if Thread is NULL, Reset Grid Row
		if(m_Client[i] == NULL) {
			grid->RemoveImageIdx(1, t_FixedIdx);
			grid->Cells[2][t_FixedIdx] = L""; // Client ID
			grid->Cells[3][t_FixedIdx] = L""; // Client IP
			grid->Cells[4][t_FixedIdx] = L""; // Client Port
			grid->Cells[5][t_FixedIdx] = L""; // Status
			grid->Cells[8][t_FixedIdx] = L""; // Connected Date Time
			continue;
		}

		// Parsing from Client Thread (For Test)
		t_IPStr = inet_ntoa(m_Client[i]->info.ClientSockAddrIn.sin_addr);
		t_Port = ntohs(m_Client[i]->info.ClientSockAddrIn.sin_port);
		t_DateTime = m_Client[i]->info.ConnectionDateTime;
		t_ClientIdx = m_Client[i]->info.ClientIndex; // Re-Parsing(Re-Extracting)

		// Set Date Format String
		t_DateTimeStr = t_DateTime.FormatString(L"yyyy-mm-dd, hh:mm:ss");

		// Re Draw Grid
		grid->RemoveImageIdx(1, t_FixedIdx);
		grid->AddImageIdx(1, t_FixedIdx, 0, haCenter, Advgrid::vaCenter); // Set Green Icon
		tempStr.sprintf(L"Client[%02X]", t_ClientIdx); // Temporary Creating Client ID
		grid->Cells[2][t_FixedIdx] = tempStr; // Client ID
		grid->Cells[3][t_FixedIdx] = t_IPStr; // Client IP
		grid->Cells[4][t_FixedIdx] = t_Port; // Client Port
		grid->Cells[5][t_FixedIdx] = L"Connected"; // Status
		grid->Cells[8][t_FixedIdx] = t_DateTimeStr; // Connected Date Time
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::tm_DeleteClientTimer(TObject *Sender)
{
	for(int i = 0 ; i < MAX_TCP_CLIENT_USER_COUNT ; i++) {
		if(m_Client[i] == NULL) continue;
		if(m_Client[i]->GetThreadStatus() == THREAD_TERMINATED) {
        	// Close Socket
			if(m_ClientSocket[i]) {
				closesocket(m_ClientSocket[i]);
				m_ClientSocket[i] = INVALID_SOCKET;
			}

			// Delete Client Thread
			if(m_Client[i]) {
				m_Client[i]->DoTerminate();
				m_Client[i]->Terminate();
				delete m_Client[i];
				m_Client[i] = NULL;
			}

			// Refresh Client Info Grid
			RefreshClientInfoGrid();
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MenuBtn_VersionClick(TObject *Sender)
{
	TFormVersion *dlg = new TFormVersion(NULL);
	dlg->ShowModal();
	delete dlg;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ReceiveClientMessage(TMessage &_msg) {

	// Common
	UnicodeString tempStr = L"";
	unsigned int t_wParam = _msg.WParam;
	CLIENTMSG* p_ClientMsg;
	CLIENTMSG t_ClientMsg;
	memset(&t_ClientMsg, 0, sizeof(t_ClientMsg));
	unsigned short t_RecvSize = 0;
	BYTE t_DataType = 0;
	int t_ClientIdx = 0;

	// Receive Client Message
	p_ClientMsg = (CLIENTMSG*)t_wParam;
	t_ClientMsg = *p_ClientMsg;

	// Logging Received Information
	t_ClientIdx = t_ClientMsg.ClientInfo.ClientIndex;
	memcpy(&t_RecvSize, &t_ClientMsg.Data[1], 2);
	tempStr.sprintf(L"Received %04d byte from Client[%02d]", t_RecvSize, t_ClientIdx);
	PrintLog(tempStr);

	// Extract Data Type
	t_DataType = t_ClientMsg.Data[3];

	// Distribute Message by Data Type
	switch(t_DataType) {
	case DATA_TYPE_SIGN_UP:
		ClientMsg_SIGN_UP(&t_ClientMsg);
		break;

	case DATA_TYPE_SIGN_IN:
		ClientMsg_SIGN_IN(&t_ClientMsg);
		break;

	case DATA_TYPE_SIGN_OUT:
		break;

	case DATA_TYPE_LOBBY_CHATTING:
		ClientMsg_LOBBY_CHATTING(t_ClientMsg);
		break;

	case DATA_TYPE_INGAME_CHATTING:
		break;

	case DATA_TYPE_CHANGE_USER_INFO:
		break;

	case DATA_TYPE_INGAME_CMD:
		break;

	case DATA_TYPE_ENTER_GAME_ROOM:
		break;

	case DATA_TYPE_ESCAPE_GAME_ROOM:
		break;

	case DATA_TYPE_HEART_BEAT:
		break;

	case DATA_TYPE_INGAME_DATA:
		break;

	default:
		break;
	}



	// Test Message
	//tempStr.sprintf(L"Queue Size(Before) : [%d]", m_ClientMsgQ.size());
	//PrintLog(tempStr);

	// Push into Client Message Queue
	int ret = WaitForSingleObject(m_Mutex, 2000);
	if(ret == WAIT_FAILED) {
		tempStr = L"Wait Failed";
	} else if(ret == WAIT_ABANDONED) {
		tempStr = L"Wait Abandoned";
	} else if(ret == WAIT_TIMEOUT) {
		tempStr = L"Wait Time Out";
	} else if(ret == WAIT_OBJECT_0) {
		tempStr = L"Success to Push Packet into Message Queue";
		m_ClientMsgQ.push(t_ClientMsg);
	} else {
		tempStr = L"ETC";
	}
	PrintMsg(tempStr);
	ReleaseMutex(m_Mutex);

	// Test Message
	//tempStr.sprintf(L"Queue Size(After) : [%d]", m_ClientMsgQ.size());
	//PrintLog(tempStr);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::btn_UserInfoClick(TObject *Sender)
{
	Notebook_Main->PageIndex = 2; // DB USER INFO
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::btn_CountClick(TObject *Sender)
{
	// Common
	UnicodeString tempStr = L"";

	for(int i = 0 ; i < MAX_SENDER_THREAD_COUNT ; i++) {
		tempStr.sprintf(L"[%d] count : %d", i, m_SenderThreadWorkCount[i]);
		PrintLog(tempStr);
	}
	PrintLog(L"-----------------------");
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ClientMsg_LOBBY_CHATTING(CLIENTMSG _ClientMsg) {

	// Common
	UnicodeString tempStr = L"";
	UnicodeString str_Chat = L"";
	unsigned short t_RecvSize = 0;
	int t_ClientIdx = 0;
	CLIENTMSG t_ClientMsg;

	// Extract Information
	t_ClientMsg = _ClientMsg;
	t_ClientIdx = t_ClientMsg.ClientInfo.ClientIndex;
	memcpy(&t_RecvSize, &t_ClientMsg.Data[1], 2);
	str_Chat.sprintf(L"Client[%02d] : ", t_ClientIdx);

	// Receive Chatting Text and Print Out
	wchar_t* temp = new wchar_t[t_RecvSize - 4];
	memcpy(temp, &t_ClientMsg.Data[4], t_RecvSize - 4);
	tempStr = temp;
	tempStr += L"    ";
	str_Chat += tempStr; // Merge Text Message
	PrintLog(str_Chat);
	delete[] temp;
}
//---------------------------------------------------------------------------

bool __fastcall TFormMain::FindUserID(UnicodeString _ID) {

	// Common
	UnicodeString tempStr = L"";
	UnicodeString t_sql = L"";

	// Making Query
	t_sql = L"Select * from DB\\DB.USER where UserID = '";
	t_sql += _ID;
	t_sql += L"'";

	// Find User Routine
	Query_USER->SQL->Clear();
	Query_USER->SQL->Add(t_sql);
	Query_USER->Open();
	tempStr = Query_USER->FieldByName(L"UserID")->AsString;
	if(tempStr == _ID) {
		return true;
	} else {
		return false;
	}
}
//---------------------------------------------------------------------------

bool __fastcall TFormMain::AddUserID(UnicodeString _ID, UnicodeString _PW, UnicodeString _USERNAME) {

	// Common
	UnicodeString tempStr = L"";

	if(FindUserID(_ID)) {
		PrintMsg(L"The ID Already Exists...");
		return false;
	}

	Table_User->Insert();
	Table_User->FieldByName(L"UserName")->AsString = _USERNAME;
	Table_User->FieldByName(L"UserID")->AsString = _ID;
	Table_User->FieldByName(L"Password")->AsString = _PW;
	Table_User->Post();

	return true;
}
//---------------------------------------------------------------------------

bool __fastcall TFormMain::DeleteUserID(UnicodeString _ID) {

	// Common
	UnicodeString tempStr = L"";
	UnicodeString t_sql = L"";

	// Check Exists
	if(FindUserID(_ID) == false) {
		PrintMsg(L"There is no ID");
		return false;
	}

	// Making Query
	t_sql = L"Delete from DB\\DB.USER where UserID = '";
	t_sql += _ID;
	t_sql += L"'";

	// Delete User Routine
	Query_USER->SQL->Clear();
	Query_USER->SQL->Add(t_sql);
	Query_USER->ExecSQL();

	// Refresh DB Grid
	t_sql = L"Select * from DB\\DB.USER";
	Query_USER->SQL->Clear();
	Query_USER->SQL->Add(t_sql);
	Query_USER->Open();
	Table_User->Active = false;
	Table_User->Active = true;

	// Check Success to Delete
	if(FindUserID(_ID)) {
		return false;
	} else {
		return true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::btn_AddDBClick(TObject *Sender)
{
	if(AddUserID(L"MJW", L"mjw", L"WAVE")) {
		PrintMsg(L"Complete");
	} else {
		PrintMsg(L"Fail");
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::btn_DelDBClick(TObject *Sender)
{
	if(DeleteUserID(L"MJW")) {
		PrintMsg(L"Delete Complete");
	} else {
		PrintMsg(L"Delete Fail...");
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ClientMsg_SIGN_UP(CLIENTMSG* _ClientMsg) {

	// Common
	UnicodeString tempStr = L"";
	UnicodeString t_UserNameStr = L"";
	UnicodeString t_UserIDStr = L"";
	UnicodeString t_UserPWStr = L"";
	unsigned short t_RecvSize = 0;
	int t_ClientIdx = 0;
	CLIENTMSG t_ClientMsg;
	int t_Size = 0;
	BYTE t_rst = 0;
	unsigned short t_SendSize = 5; // Fixed.. in Protocol

	// Extract Information
	t_ClientMsg = *_ClientMsg;
	t_ClientIdx = t_ClientMsg.ClientInfo.ClientIndex;

	// Extract User Name
	t_Size = t_ClientMsg.Data[126]; // 126 is User Name Size
	wchar_t* t_UserName = new wchar_t[t_Size];
	memcpy(t_UserName, &t_ClientMsg.Data[4], t_Size);
	t_UserNameStr = t_UserName;
	PrintLog(t_UserNameStr);
	delete[] t_UserName;

	// Extract User ID
	t_Size = t_ClientMsg.Data[127]; // 127 is User ID Size
	wchar_t* t_UserID = new wchar_t[t_Size];
	memcpy(t_UserID, &t_ClientMsg.Data[46], t_Size);
	t_UserIDStr = t_UserID;
	PrintLog(t_UserIDStr);
	delete[] t_UserID;

	// Extract User PW
	t_Size = t_ClientMsg.Data[128]; // 128 is User PW Size
	wchar_t* t_UserPW = new wchar_t[t_Size];
	memcpy(t_UserPW, &t_ClientMsg.Data[86], t_Size);
	t_UserPWStr = t_UserPW;
	PrintLog(t_UserPWStr);
	delete[] t_UserPW;

	// Try to add User ID
	memcpy(&_ClientMsg->Data[1], &t_SendSize, sizeof(t_SendSize));
	memset(&_ClientMsg->Data[4], 0, MAX_RECV_PACKET_SIZE - 4);
	if(AddUserID(t_UserIDStr, t_UserPWStr, t_UserNameStr)) {
		t_rst = 0;
		memcpy(&_ClientMsg->Data[4], &t_rst, sizeof(t_rst));
		PrintLog(L"Welcome into DB");
	} else {
		t_rst = 1;
		memcpy(&_ClientMsg->Data[4], &t_rst, sizeof(t_rst));
		PrintLog(L"Not Welcome into DB");
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ClientMsg_SIGN_IN(CLIENTMSG* _ClientMsg) {

	// Common
	UnicodeString tempStr = L"";
	UnicodeString t_UserIDStr = L"";
	UnicodeString t_UserPWStr = L"";
	unsigned short t_RecvSize = 0;
	int t_ClientIdx = 0;
	CLIENTMSG t_ClientMsg;
	int t_Size = 0;
	BYTE t_rst = 0;
	unsigned short t_SendSize = 5; // Fixed.. in Protocol

	// Extract Information
	t_ClientMsg = *_ClientMsg;
	t_ClientIdx = t_ClientMsg.ClientInfo.ClientIndex;

	// Extract User ID
	t_Size = t_ClientMsg.Data[127]; // 127 is User ID Size
	wchar_t* t_UserID = new wchar_t[t_Size];
	memcpy(t_UserID, &t_ClientMsg.Data[46], t_Size);
	t_UserIDStr = t_UserID;
	PrintLog(t_UserIDStr);
	delete[] t_UserID;

	// Extract User PW
	t_Size = t_ClientMsg.Data[128]; // 128 is User PW Size
	wchar_t* t_UserPW = new wchar_t[t_Size];
	memcpy(t_UserPW, &t_ClientMsg.Data[86], t_Size);
	t_UserPWStr = t_UserPW;
	PrintLog(t_UserPWStr);
	delete[] t_UserPW;

	// Try to add User ID
	memcpy(&_ClientMsg->Data[1], &t_SendSize, sizeof(t_SendSize));
	memset(&_ClientMsg->Data[4], 0, MAX_RECV_PACKET_SIZE - 4);
	t_rst = Login(t_UserIDStr, t_UserPWStr);
	memcpy(&_ClientMsg->Data[4], &t_rst, sizeof(t_rst));
	if(t_rst == ERR_LOGIN_OK) {
		PrintLog(L"Welcome into DB");
	} else {
		PrintLog(L"Not Welcome into DB");
	}
}
//---------------------------------------------------------------------------

BYTE __fastcall TFormMain::Login(UnicodeString _ID, UnicodeString _PW) {

	// Common
	UnicodeString tempStr = L"";
	UnicodeString t_sql = L"";

	// ID Existence Check
	if(FindUserID(_ID) == false) {
		PrintMsg(L"There is no ID");
		return ERR_LOGIN_ID;
	}

	// Login Routine
	t_sql = L"Select * from DB\\DB.USER where UserID = '";
	t_sql += _ID;
	t_sql += L"'";

	// Find User Routine
	Query_USER->SQL->Clear();
	Query_USER->SQL->Add(t_sql);
	Query_USER->Open();
	tempStr = Query_USER->FieldByName(L"Password")->AsString;
	if(tempStr == _PW) {
		// Login Routine Here

		return ERR_LOGIN_OK;
	} else {
		PrintMsg(L"Password is incorrect");
		return ERR_LOGIN_PW;
	}
}
//---------------------------------------------------------------------------
