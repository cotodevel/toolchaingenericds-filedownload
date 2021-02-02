#ifndef _DEMO_H_
#define _DEMO_H_

#ifdef __cplusplus
#include "alert.h"
#include "woopsi.h"
#include "woopsiheaders.h"
#include "filerequester.h"
#include "textbox.h"
#include "soundTGDS.h"
#include "button.h"
#include "woopsikeyboard.h"

//C++ part
#include <vector>
#include <string>
using namespace std;
using namespace WoopsiUI;

#define TGDSPROJECTNAME ((char*) "ToolchainGenericDS-filedownload")

//Helpers to be used with int AsynchronousProgramState;

//WoopsiUI asynchronous events
#define HandleWoopsiUIEvents 		(int)(0)	//Normal program flow
#define WaitForKeyboardReleaseEvent (int)(1)	//Normal program flow -> Desired widget wait for Release event
#define DestroyKeyboardEvent 		(int)(2)	//Desired widget wait for Release event -> Desired widget destroy event -> Normal program flow


class WoopsiTemplate : public Woopsi, public GadgetEventHandler {
public:
	void startup(int argc, char **argv);
	void shutdown();
	void handleValueChangeEvent(const GadgetEventArgs& e);	//Handles UI events if they change
	void handleClickEvent(const GadgetEventArgs& e);	//Handles UI events when they take click action
	void waitForAOrTouchScreenButtonMessage(MultiLineTextBox* thisLineTextBox, const WoopsiString& thisText);
	void handleLidClosed();
	void handleLidOpen();
	void ApplicationMainLoop();
	FileRequester* _fileReq;
	int currentFileRequesterIndex;
	AmigaScreen* newScreen;		//top screen
	AmigaScreen* _fileScreen;	//bottom screen
	MultiLineTextBox* _MultiLineTextBoxLogger;
	
	AmigaWindow* controlWindow;	//Multimedia controls widget
	
	Button* _Index;
	Button* _lastFile;
	Button* _nextFile;
	Button* _play;
	Button* _stop;
	
	Alert* _downloadFile;
	
	//Asynchronous Keyboard: todo
	int AsynchronousProgramState;	//enables asynchronous events to take place, while retaining UI events.
	
private:
	Alert* _alert;
};
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

extern WoopsiTemplate * WoopsiTemplateProc;
extern u32 pendPlay;
extern char currentFileChosen[256+1];

extern vector<string> splitByVector(string str, string token);
extern bool DownloadFileFromServer(char * downloadAddr, int ServerPort, char * outputPath, MultiLineTextBox* logger);
	
#ifdef __cplusplus
}
#endif
