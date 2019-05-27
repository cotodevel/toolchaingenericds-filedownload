/*

			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/

//C++ part
#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <iterator>

using namespace std;

#include "socket.h"
#include "in.h"
#include <netdb.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "main.h"
#include "InterruptsARMCores_h.h"
#include "ipcfifoTGDSUser.h"
#include "ff.h"
#include "memoryHandleTGDS.h"
#include "fileHandleTGDS.h"
#include "reent.h"
#include "sys/types.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "typedefsTGDS.h"
#include "consoleTGDS.h"
#include "utilsTGDS.h"

#include "devoptab_devices.h"
#include "fsfatlayerTGDS.h"
#include "usrsettingsTGDS.h"
#include "exceptionTGDS.h"

#include "videoTGDS.h"
#include "keypadTGDS.h"
#include "dldi.h"
#include "SpecialFunctions.h"
#include "dmaTGDS.h"
#include "biosTGDS.h"
#include "nds_cp15_misc.h"
#include "limitsTGDS.h"
#include "dswnifi_lib.h"


//customHandler 
void CustomDebugHandler(){
	clrscr();
	
	//Init default console here
	bool project_specific_console = false;	//set default console or custom console: default console
	GUI_init(project_specific_console);
	GUI_clear();
	
	printf("custom Handler!");
	uint32 * debugVector = (uint32 *)&exceptionArmRegs[0];
	uint32 pc_abort = (uint32)exceptionArmRegs[0xf];
	
	if((debugVector[0xe] & 0x1f) == 0x17){
		pc_abort = pc_abort - 8;
	}
	
	printf("R0[%x] R1[%X] R2[%X] ",debugVector[0],debugVector[1],debugVector[2]);
	printf("R3[%x] R4[%X] R5[%X] ",debugVector[3],debugVector[4],debugVector[5]);
	printf("R6[%x] R7[%X] R8[%X] ",debugVector[6],debugVector[7],debugVector[8]);
	printf("R9[%x] R10[%X] R11[%X] ",debugVector[9],debugVector[0xa],debugVector[0xb]);
	printf("R12[%x] R13[%X] R14[%X]  ",debugVector[0xc],debugVector[0xd],debugVector[0xe]);
	printf("R15[%x] SPSR[%x] CPSR[%X]  ",pc_abort,debugVector[17],debugVector[16]);
	while(1==1){}
}

vector<string> splitCustom(string str, string token){
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


bool DownloadFileFromServer(char * downloadAddr, int ServerPort, char * outputPath){
	std::string strURL = string(downloadAddr);
    vector <string> vecOut = splitCustom(strURL, string("/"));
    std::string ServerDNS = vecOut.at(0);
    
    size_t start = strlen(ServerDNS.c_str());
    size_t end = strlen(downloadAddr) - strlen(ServerDNS.c_str()); // Assume this is an exclusive bound.
    std::string strPath = strURL.substr(start, end); 
	std::string strFilename = vecOut.at(vecOut.size() - 1);
	
	const char * message;
    char server_reply[10000];
    int total_len = 0;
    int len; 

    FILE *file = NULL;
	struct sockaddr_in server;	//structure holding the server IP/Port DS connects to.
	int serverSocket = openAsyncConn((char*)ServerDNS.c_str(), 80, &server);
	bool connStatus = connectAsync(serverSocket, &server);
	
    if((serverSocket >= 0) && (connStatus ==true)){
		printf("Connected to server! ");
	}
	else{
		printf("Could not connect. Check DS Firmware AP settings and set a valid DNS Server.");
		return false;
	}
    //Send request
	message =  string("GET " + strPath + " HTTP/1.1\r\nHost: " + ServerDNS +" \r\n\r\n Connection: keep-alive\r\n\r\n Keep-Alive: 300\r\n").c_str();
    if( send(serverSocket , message , strlen(message) , 0) < 0)
    {
        return false;
    }
    remove( string(string(outputPath) + strFilename).c_str() );
    file = fopen(string(string(outputPath) + strFilename).c_str(), "w+");

    if(file == NULL){
		disconnectAsync(serverSocket);
        return false;
	}
	int received_len = 0;
	while( ( received_len = recv( serverSocket, server_reply, sizeof(server_reply), 0 ) ) != 0 ) { // if recv returns 0, the socket has been closed.
		if(received_len>0) { // data was received!
			total_len += received_len;
			fwrite(server_reply , received_len , 1, file);
			//printf("Received byte size = %d Total lenght = %d", received_len, total_len);
		}
	}
	disconnectAsync(serverSocket);
	fclose(file);
	return true;
}

void menuShow(){
	clrscr();
	printf("                              ");
	printf("ToolchainGenericDS-filedownload: ");
	printf("A: Download PDF File from server");
	printf("X: Download MP3 File from server");
	
	printf("B: clear messages");
	printf("                              ");
	printf("DS IP address: %s ", (char*)print_ip((uint32)Wifi_GetIP()));	
	
}

int main(int _argc, sint8 **_argv) {
	
	/*			TGDS 1.5 Standard ARM9 Init code start	*/
	bool project_specific_console = false;	//set default console or custom console: default console
	GUI_init(project_specific_console);
	GUI_clear();
	
	sint32 fwlanguage = (sint32)getLanguage();
	
	printf("     ");
	printf("     ");
	
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
	}
	switch_dswnifi_mode(dswifi_idlemode);
	/*			TGDS 1.5 Standard ARM9 Init code end	*/
	
	//custom Handler
	setupCustomExceptionHandler((uint32*)&CustomDebugHandler);
	
	connectDSWIFIAP(DSWNIFI_ENTER_WIFIMODE);	
    menuShow();
	
	while (1){
		scanKeys();
		
		if (keysPressed() & KEY_A){
			int ServerPort = 80;
			char * fileDownloadURL = "www.axmag.com/download/pdfurl-guide.pdf";
			char * fileDownloadDir = "0:/";
			
			printf("    ");
			printf("    ");
			printf("    ");
			printf("    ");
			
			printf("Downloading...");
			printf("%s", fileDownloadURL);
			
			if(DownloadFileFromServer(fileDownloadURL, ServerPort, fileDownloadDir) == true){
				printf("Download OK @ SD path: %s ", fileDownloadDir);
			}
			else{
				printf("Download FAIL. Check URL / DLDI Driver");
			}
			while(keysPressed() & KEY_A){
				scanKeys();
			}
		}
		
		if (keysPressed() & KEY_X){
			int ServerPort = 80;
			char * fileDownloadURL = "www.largesound.com/ashborytour/sound/brobob.mp3";
			char * fileDownloadDir = "0:/";
			
			printf("    ");
			printf("    ");
			printf("    ");
			printf("    ");
			
			printf("Downloading...");
			printf("%s", fileDownloadURL);
			
			if(DownloadFileFromServer(fileDownloadURL, ServerPort, fileDownloadDir) == true){
				printf("Download OK @ SD path: %s ", fileDownloadDir);
			}
			else{
				printf("Download FAIL. Check URL / DLDI Driver");
			}
			while(keysPressed() & KEY_X){
				scanKeys();
			}
		}
		
		if (keysPressed() & KEY_B){
			menuShow();
			while(keysPressed() & KEY_B){
				scanKeys();
			}
		}
		
		
		/*
		if (keysPressed() & KEY_B){
			//	//float printf support: todo
			// sqrtf() is a library function to calculate square root
			//float number = 5.0, squareRoot = 0.0;
			//squareRoot = sqrtf(number);
			//printf("float:Square root of %f=%f",number,squareRoot);
			
			//double number2 = 4.0, squareRoot2 = 0.0;
			//squareRoot = sqrt(number2);
			//printf("double:Square root of %lf=%lf",number,squareRoot);

			cl s;

			s.put_i(10);
			printf("Test1:res:%d",s.get_i());
			
			//test constructor
			Box BoxInst(20,20,80);
			printf("Test2:Box Test:%d",BoxInst.volume());
			
			//copy constructor
			printf("Test3: Constructor");
			myclass a(10);
			display(a);
			
			while(keysPressed() & KEY_B){}
		}
		
		if (keysPressed() & KEY_X){
			printf("Test4: Destructor");
			myclass2 a(10);
			display2(a);
			while(keysPressed() & KEY_X){}
		}
		*/
		
		IRQVBlankWait();
	}

}
