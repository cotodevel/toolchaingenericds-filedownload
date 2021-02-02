// Includes
#include "WoopsiTemplate.h"
#include "woopsiheaders.h"
#include "bitmapwrapper.h"
#include "bitmap.h"
#include "graphics.h"
#include "rect.h"
#include "gadgetstyle.h"
#include "fonts/newtopaz.h"
#include "woopsistring.h"
#include "colourpicker.h"
#include "filerequester.h"
#include "soundTGDS.h"
#include "main.h"
#include "posixHandleTGDS.h"
#include "keypadTGDS.h"
#include <in.h>
#include <netdb.h>
#include "wifi_arm9.h"
#include "dswnifi_lib.h"
#include "textbox.h"

//C++ part
using namespace std;
#include <string>
#include <vector>

__attribute__((section(".itcm")))
WoopsiTemplate * WoopsiTemplateProc = NULL;

void WoopsiTemplate::startup(int argc, char **argv) {
	
	Rect rect;

	/** SuperBitmap preparation **/
	// Create bitmap for superbitmap
	Bitmap* superBitmapBitmap = new Bitmap(164, 191);

	// Get a graphics object from the bitmap so that we can modify it
	Graphics* gfx = superBitmapBitmap->newGraphics();

	// Clean up
	delete gfx;

	// Create screens
	newScreen = new AmigaScreen(TGDSPROJECTNAME, Gadget::GADGET_DRAGGABLE, AmigaScreen::AMIGA_SCREEN_SHOW_DEPTH | AmigaScreen::AMIGA_SCREEN_SHOW_FLIP);
	woopsiApplication->addGadget(newScreen);
	newScreen->setPermeable(true);

	// Add child windows
	controlWindow = new AmigaWindow(0, 13, 256, 33, "Controls", Gadget::GADGET_DRAGGABLE, AmigaWindow::AMIGA_WINDOW_SHOW_DEPTH);
	newScreen->addGadget(controlWindow);

	// Controls
	controlWindow->getClientRect(rect);
	
	_Index = new Button(rect.x, rect.y, 41, 16, "Index");	//_Index->disable();
	_Index->setRefcon(2);
	controlWindow->addGadget(_Index);
	_Index->addGadgetEventHandler(this);
	
	_lastFile = new Button(rect.x + 41, rect.y, 17, 16, "<");
	_lastFile->setRefcon(3);
	controlWindow->addGadget(_lastFile);
	_lastFile->addGadgetEventHandler(this);
	
	_nextFile = new Button(rect.x + 41 + 17, rect.y, 17, 16, ">");
	_nextFile->setRefcon(4);
	controlWindow->addGadget(_nextFile);
	_nextFile->addGadgetEventHandler(this);
	
	_play = new Button(rect.x + 41 + 17 + 17, rect.y, 40, 16, "Play");
	_play->setRefcon(5);
	controlWindow->addGadget(_play);
	_play->addGadgetEventHandler(this);
	
	_stop = new Button(rect.x + 41 + 17 + 17 + 40, rect.y, 40, 16, "Stop");
	_stop->setRefcon(6);
	controlWindow->addGadget(_stop);
	_stop->addGadgetEventHandler(this);
	
	// Add File listing screen
	_fileScreen = new AmigaScreen("File List", Gadget::GADGET_DRAGGABLE, AmigaScreen::AMIGA_SCREEN_SHOW_DEPTH | AmigaScreen::AMIGA_SCREEN_SHOW_FLIP);
	woopsiApplication->addGadget(_fileScreen);
	_fileScreen->setPermeable(true);
	_fileScreen->flipToTopScreen();
	// Add screen background
	_fileScreen->insertGadget(new Gradient(0, SCREEN_TITLE_HEIGHT, 256, 192 - SCREEN_TITLE_HEIGHT, woopsiRGB(0, 31, 0), woopsiRGB(0, 0, 31)));
	
	// Create FileRequester
	_fileReq = new FileRequester(10, 10, 150, 150, "Files", "/", GADGET_DRAGGABLE | GADGET_DOUBLE_CLICKABLE);
	_fileReq->setRefcon(1);
	_fileScreen->addGadget(_fileReq);
	_fileReq->addGadgetEventHandler(this);
	currentFileRequesterIndex = 0;
	
	//Create Download Widget. Get Button Handle
	_downloadFile = new Alert(2, 2, 240, 160, "Download File:", "www.largesound.com/ashborytour/sound/brobob.mp3");	//preset URL
	newScreen->addGadget(_downloadFile);
	Button * AlertOKButtonHandle = _downloadFile->getButtonHandle();
	AlertOKButtonHandle->setRefcon(7);
	AlertOKButtonHandle->addGadgetEventHandler(this);
	
	_MultiLineTextBoxLogger = NULL;	//destroyable TextBox
	
	//Destroyable Keyboard UI implementation
	AsynchronousProgramState = HandleWoopsiUIEvents;	//Connect to AP inmediately
	
	enableDrawing();	// Ensure Woopsi can now draw itself
	redraw();			// Draw initial state
	
	//Destroyable Textbox implementation init
	newScreen->getClientRect(rect);
	_MultiLineTextBoxLogger = new MultiLineTextBox(rect.x, rect.y, 262, 170, "Loading\n...", Gadget::GADGET_DRAGGABLE, 5);
	newScreen->addGadget(_MultiLineTextBoxLogger);
	_MultiLineTextBoxLogger->removeText(0);
	_MultiLineTextBoxLogger->moveCursorToPosition(0);
	_MultiLineTextBoxLogger->appendText("Connecting to Access Point...\n");
	if(connectDSWIFIAP(DSWNIFI_ENTER_WIFIMODE) == true){
		char outputConsole[256+1];
		memset(outputConsole, 0, sizeof(outputConsole));
		char IP[16];
		memset(IP, 0, sizeof(IP));
		sprintf(outputConsole, "%s[%s] \n", "Connect OK.  \nIP: ", print_ip((uint32)Wifi_GetIP(), IP));
		_MultiLineTextBoxLogger->appendText(WoopsiString(outputConsole));
	}
	else{
		_MultiLineTextBoxLogger->appendText("Connect failed. Check AP settings. \n");
	}
	waitForAOrTouchScreenButtonMessage(_MultiLineTextBoxLogger, "Press (A) or tap touchscreen to continue. \n");
	
	_MultiLineTextBoxLogger->invalidateVisibleRectCache();
	newScreen->eraseGadget(_MultiLineTextBoxLogger);
	_MultiLineTextBoxLogger->destroy();	//same as delete _MultiLineTextBoxLogger;
	//Destroyable Textbox implementation end
}

void WoopsiTemplate::shutdown() {
	Woopsi::shutdown();
}

void WoopsiTemplate::waitForAOrTouchScreenButtonMessage(MultiLineTextBox* thisLineTextBox, const WoopsiString& thisText) __attribute__ ((optnone)) {
	thisLineTextBox->appendText(thisText);
	scanKeys();
	while((!(keysDown() & KEY_A)) && (!(keysDown() & KEY_TOUCH))){
		scanKeys();
	}
	scanKeys();
	while((keysDown() & KEY_A) && (keysDown() & KEY_TOUCH)){
		scanKeys();
	}
}

void WoopsiTemplate::handleValueChangeEvent(const GadgetEventArgs& e) __attribute__ ((optnone)) {

	// Did a gadget fire this event?
	if (e.getSource() != NULL) {
		// Is the gadget the file requester?
		if (e.getSource()->getRefcon() == 1) {
			/*
			//Play WAV/ADPCM if selected from the FileRequester
			WoopsiString strObj = ((FileRequester*)e.getSource())->getSelectedOption()->getText();
			memset(currentFileChosen, 0, sizeof(currentFileChosen));
			strObj.copyToCharArray(currentFileChosen);
			pendPlay = 1;
			*/
		}
	}
}

void WoopsiTemplate::handleLidClosed() {
	// Lid has just been closed
	_lidClosed = true;

	// Run lid closed on all gadgets
	s32 i = 0;
	while (i < _gadgets.size()) {
		_gadgets[i]->lidClose();
		i++;
	}
}

void WoopsiTemplate::handleLidOpen() {
	// Lid has just been opened
	_lidClosed = false;

	// Run lid opened on all gadgets
	s32 i = 0;
	while (i < _gadgets.size()) {
		_gadgets[i]->lidOpen();
		i++;
	}
}

void WoopsiTemplate::handleClickEvent(const GadgetEventArgs& e) __attribute__ ((optnone)) {
	switch (e.getSource()->getRefcon()) {
		//_Index Event
		case 2:{
			//Get fileRequester size, if > 0, set the first element selected
			FileRequester * freqInst = _fileReq;
			FileListBox* freqListBox = freqInst->getInternalListBoxObject();
			if(freqListBox->getOptionCount() > 0){
				freqListBox->setSelectedIndex(0);
			}
			currentFileRequesterIndex = 0;
		}	
		break;
		
		//_lastFile Event
		case 3:{
			FileRequester * freqInst = _fileReq;
			FileListBox* freqListBox = freqInst->getInternalListBoxObject();
			if(currentFileRequesterIndex > 0){
				currentFileRequesterIndex--;
			}
			if(freqListBox->getOptionCount() > 0){
				freqListBox->setSelectedIndex(currentFileRequesterIndex);
			}
		}	
		break;
		
		//_nextFile Event
		case 4:{
			FileRequester * freqInst = _fileReq;
			FileListBox* freqListBox = freqInst->getInternalListBoxObject();
			if(currentFileRequesterIndex < (freqListBox->getOptionCount() - 1) ){
				currentFileRequesterIndex++;
				freqListBox->setSelectedIndex(currentFileRequesterIndex);
			}
		}	
		break;
		
		//_play Event
		case 5:{
			//Play WAV/ADPCM if selected from the FileRequester
			WoopsiString strObj = _fileReq->getSelectedOption()->getText();
			memset(currentFileChosen, 0, sizeof(currentFileChosen));
			strObj.copyToCharArray(currentFileChosen);
			pendPlay = 1;
		}	
		break;
		
		//_stop Event
		case 6:{
			pendPlay = 2;
		}	
		break;
		
		case 7:{
			Button * thisButton = _downloadFile->getButtonHandle();
			MultiLineTextBox* thisTextBox = _downloadFile->getTextBoxHandle();
			
			//Get URL text from textbox
			//Destroyable Textbox implementation init
			Rect rect;
			newScreen->getClientRect(rect);
			_MultiLineTextBoxLogger = new MultiLineTextBox(rect.x, rect.y, 262, 170, "Loading\n...", Gadget::GADGET_DRAGGABLE, 5);
			newScreen->addGadget(_MultiLineTextBoxLogger);
			
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			
			char textBoxValue[256+1];
			memset(textBoxValue, 0, sizeof(textBoxValue));
			thisTextBox->getText()->copyToCharArray(textBoxValue);
			char outputConsole[256+1];
			memset(outputConsole, 0, sizeof(outputConsole));
			sprintf(outputConsole, "%s\n[%s]\n", "Downloading: ", textBoxValue);
			_MultiLineTextBoxLogger->appendText(WoopsiString(outputConsole));
			
			//Download file
			char * fileDownloadDir = "0:/";
			int ServerPort = 80;
			
			if(DownloadFileFromServer(textBoxValue, ServerPort, fileDownloadDir, _MultiLineTextBoxLogger) == true){
				memset(outputConsole, 0, sizeof(outputConsole));
				sprintf(outputConsole, "Download OK @ SD path: %s ", fileDownloadDir);
				_MultiLineTextBoxLogger->appendText(WoopsiString(outputConsole));
			}
			else{
				_MultiLineTextBoxLogger->appendText(WoopsiString("Download FAIL. Check: URL / DLDI Driver / Internet \n"));
			}
			
			waitForAOrTouchScreenButtonMessage(_MultiLineTextBoxLogger, "Press (A) or tap touchscreen to continue. \n");
			
			_MultiLineTextBoxLogger->invalidateVisibleRectCache();
			newScreen->eraseGadget(_MultiLineTextBoxLogger);
			_MultiLineTextBoxLogger->destroy();	//same as delete _MultiLineTextBoxLogger;
			//Destroyable Textbox implementation end
		}
		break;
	}
}

vector<string> splitByVector(string str, string token) __attribute__ ((optnone)) {
    vector<string>result;
    while(str.size()){
        int index = str.find(token);
        if(index != (int)string::npos){
            result.push_back(str.substr(0,index));
            str = str.substr(index+token.size());
            if(str.size()==0)result.push_back(str);
        }else{
            result.push_back(str);
            str = "";
        }
    }
    return result;
}

bool DownloadFileFromServer(char * downloadAddr, int ServerPort, char * outputPath, MultiLineTextBox* logger) __attribute__ ((optnone)) {
	logger->removeText(0);
	logger->moveCursorToPosition(0);
	logger->appendText(WoopsiString(downloadAddr));
	logger->appendText("\n");
	
	std::string strURL = string(downloadAddr);
    vector <string> vecOut = splitByVector(strURL, string("/"));
    std::string ServerDNS = vecOut.at(0);
    
    size_t start = strlen(ServerDNS.c_str());
    size_t end = strlen(downloadAddr) - strlen(ServerDNS.c_str()); // Assume this is an exclusive bound.
    std::string strPath = strURL.substr(start, end); 
	std::string strFilename = vecOut.at(vecOut.size() - 1);
	
	// Find the IP address of the server, with gethostbyname
    struct hostent * myhost = gethostbyname(ServerDNS.c_str());
    logger->appendText("Found IP Address!\n");
 
    // Create a TCP socket
    int my_socket = socket( AF_INET, SOCK_STREAM, 0 );
    logger->appendText("Created Socket!\n");

    // Tell the socket to connect to the IP address we found, on port 80 (HTTP)
    struct sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_port = htons(80);
    sain.sin_addr.s_addr= *( (unsigned long *)(myhost->h_addr_list[0]) );
    connect( my_socket,(struct sockaddr *)&sain, sizeof(sain) );
    logger->appendText("Connected to server!\n");
	
    //Send request
	FILE *file = NULL;
	char logConsole[256+1];
	char * server_reply = (char *)TGDSARM9Malloc(100000);
    int total_len = 0;
	const char * message =  string("GET " + strPath + " HTTP/1.1\r\nHost: " + ServerDNS +" \r\n\r\n Connection: keep-alive\r\n\r\n Keep-Alive: 300\r\n").c_str();
    if( send(my_socket, message , strlen(message) , 0) < 0)
    {
        return false;
    }
    
	remove( string(string(outputPath) + strFilename).c_str() );
    file = fopen(string(string(outputPath) + strFilename).c_str(), "w+");
    if(file == NULL){
        return false;
	}
	
	logger->appendText("Download start.\n");
	
	int received_len = 0;
	while( ( received_len = recv(my_socket, server_reply, 100000, 0 ) ) != 0 ) { // if recv returns 0, the socket has been closed.
		if(received_len>0) { // data was received!
			total_len += received_len;
			fwrite(server_reply , received_len , 1, file);
			
			memset(logConsole, 0, sizeof(logConsole));
			sprintf(logConsole, "Received byte size = %d Total length = %d ", received_len, total_len);
			logger->setText(WoopsiString(logConsole));
		}
	}
	
	TGDSARM9Free(server_reply);
	
	shutdown(my_socket,0); // good practice to shutdown the socket.
	closesocket(my_socket); // remove the socket.
    fclose(file);
	logger->appendText("Download OK.\n");
	return true;
}

__attribute__((section(".dtcm")))
u32 pendPlay = 0;

char currentFileChosen[256+1];

//Called once Woopsi events are ended: TGDS Main Loop
__attribute__((section(".itcm")))
void Woopsi::ApplicationMainLoop(){
	//Earlier.. main from Woopsi SDK.
	
	if(WoopsiTemplateProc != NULL){
		//Handle WoopsiUI asynchronous events
		switch(WoopsiTemplateProc->AsynchronousProgramState){
			case(HandleWoopsiUIEvents):{
				
				
			}
			break;
			
			case(WaitForKeyboardReleaseEvent):{
				//AsynchronousProgramState = WaitForKeyboardReleaseEvent;	<-- Widget raiser event.
				
				
				WoopsiTemplateProc->AsynchronousProgramState = HandleWoopsiUIEvents;
			}
			break;
			
			case(DestroyKeyboardEvent):{
				//AsynchronousProgramState = DestroyKeyboardEvent;	<-- Event raises from the target release event.
				
				WoopsiTemplateProc->AsynchronousProgramState = HandleWoopsiUIEvents;
			}
			break;
			
		}
	}
	
	//Handle TGDS stuff...
	
	switch(pendPlay){
		case(1):{
			internalCodecType = playSoundStream(currentFileChosen, _FileHandleVideo, _FileHandleAudio);
			if(internalCodecType == SRC_NONE){
				//stop right now
				pendPlay = 2;
			}
			else{
				pendPlay = 0;
			}
		}
		break;
		case(2):{
			stopSoundStreamUser();
			pendPlay = 0;
		}
		break;
	}
}
