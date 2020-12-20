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

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "gui_console_connector.h"
#include <in.h>
#include "dswnifi_lib.h"
#include "TGDSLogoLZSSCompressed.h"
#include "biosTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "dldi.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"

//C++ part
using namespace std;
#include <string>
#include <vector>

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
	
	int socket_desc;
    const char * message;
    char server_reply[10000];
    int total_len = 0;
    int len; 

    FILE *file = NULL;
    struct sockaddr_in server;
	
	// Find the IP address of the server, with gethostbyname
    struct hostent * myhost = gethostbyname(ServerDNS.c_str());
	
	struct in_addr **address_list = (struct in_addr **)myhost->h_addr_list;
    for(int i = 0; address_list[i] != NULL; i++){
        //printf("Server WAN IP Address! %s", inet_ntoa(*address_list[i]));
    }
	
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        return false;	//Could not create socket
    }
	
	memset(&server, 0, sizeof(server)); 
	int i=1;
	i=ioctl(socket_desc, FIONBIO,&i);	//set non-blocking
	
    server.sin_family = AF_INET;
    server.sin_port = htons(ServerPort);
    server.sin_addr.s_addr= *( (unsigned long *)(myhost->h_addr_list[0]) );
    
	connect(socket_desc,(struct sockaddr *)&server, sizeof(server));
    //printf("Connected to server! ");
	
    //Send request
	message =  string("GET " + strPath + " HTTP/1.1\r\nHost: " + ServerDNS +" \r\n\r\n Connection: keep-alive\r\n\r\n Keep-Alive: 300\r\n").c_str();
    if( send(socket_desc , message , strlen(message) , 0) < 0)
    {
        return false;
    }
	
    remove( string(string(outputPath) + strFilename).c_str() );
    file = fopen(string(string(outputPath) + strFilename).c_str(), "w+");

    if(file == NULL){
        return false;
	}
	
	int received_len = 0;
	while( ( received_len = recv( socket_desc, server_reply, sizeof(server_reply), 0 ) ) != 0 ) { // if recv returns 0, the socket has been closed.
		if(received_len>0) { // data was received!
			total_len += received_len;
			fwrite(server_reply , received_len , 1, file);
			printfCoords(0, 5, "---------------------------------------- ");
			printfCoords(0, 5, "Received byte size = %d Total length = %d >%d", received_len, total_len, TGDSPrintfColor_Yellow);
		}
	}

	shutdown(socket_desc,0); // good practice to shutdown the socket.
	closesocket(socket_desc); // remove the socket.
    fclose(file);
	return true;
}

void menuShow(){
	clrscr();
	printf(" ------------------------ ");
	
	printf("ToolchainGenericDS-filedownload: >%d", TGDSPrintfColor_Green);
	printf("A: Download PDF File from server");
	printf("X: Download MP3 File from server");
	
	printf("Available heap memory: %d", getMaxRam());
	printf("                              ");
	char IP[16];
	printf("DS IP address: %s ", print_ip((uint32)Wifi_GetIP(), IP));
	printarm7DebugBuffer();
}

int main(int argc, char argv[argvItems][MAX_TGDSFILENAME_LENGTH]) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	bool isCustomTGDSMalloc = false;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	printf("     ");
	printf("     ");
	
	#ifdef ARM7_DLDI
	setDLDIARM7Address((u32 *)TGDSDLDI_ARM7_ADDRESS);	//Required by ARM7DLDI!
	#endif
	
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
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	//Remove logo and restore Main Engine registers
	//restoreFBModeMainEngine();
	
	printf("Connecting to Access Point...");
	connectDSWIFIAP(DSWNIFI_ENTER_WIFIMODE);	
    menuShow();
	
	while (1){
		scanKeys();
		if (keysPressed() & KEY_A){
			int ServerPort = 80;
			char * fileDownloadURL = "www.axmag.com/download/pdfurl-guide.pdf";
			char * fileDownloadDir = "0:/";
			
			clrscr();
			printf(" ---- ");
			printf(" ---- ");
			printf("Downloading...");
			printf("%s", fileDownloadURL);
			
			if(DownloadFileFromServer(fileDownloadURL, ServerPort, fileDownloadDir) == true){
				printf("Download OK @ SD path: %s ", fileDownloadDir);
			}
			else{
				printf("Download FAIL. Check: URL / DLDI Driver / Internet ");
			}
			printf("Press (A) to continue. ");
			scanKeys();
			while(!keysPressed() & KEY_A){
				scanKeys();
			}
			scanKeys();
			while(keysPressed() & KEY_A){
				scanKeys();
			}
			menuShow();
		}
		
		if (keysPressed() & KEY_X){
			int ServerPort = 80;
			char * fileDownloadURL = "www.largesound.com/ashborytour/sound/brobob.mp3";
			char * fileDownloadDir = "0:/";
			
			clrscr();
			printf(" ---- ");
			printf(" ---- ");
			printf("Downloading...");
			printf("%s", fileDownloadURL);
			
			if(DownloadFileFromServer(fileDownloadURL, ServerPort, fileDownloadDir) == true){
				printf("Download OK @ SD path: %s ", fileDownloadDir);
			}
			else{
				printf("Download FAIL. Check URL / DLDI Driver");
			}
			printf("Press (A) to continue. ");
			scanKeys();
			while(!keysPressed() & KEY_A){
				scanKeys();
			}
			scanKeys();
			while(keysPressed() & KEY_A){
				scanKeys();
			}
			menuShow();
		}
		
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}
}
