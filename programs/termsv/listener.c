/*
 * Copyright 2008 Hans Leidekker for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#if 0
#include <stdio.h>

#include <basetsd.h>
#include <windef.h>
#include <winnt.h>
#include <winternl.h>
#include <winsvc.h>
#include <wtsapi32.h>
#include <sspi.h>

#define WINPR_H
#define WINPR_TIMEZONE_H
#define WINPR_WTYPES_H
#define WINPR_FILE_H
#define WINPR_SPEC_H
#define WINPR_CRT_STRING_H
#define WINPR_CRT_H
#define WINPR_COLLECTIONS_H
#define WINPR_NT_H
#define WINPR_WTSAPI_H
#define WINPR_SYNCH_H
#define WINPR_UTILS_STREAM_H
#define WINPR_API WINAPI
#define WINPR_SSPI_H
#define WINPR_SECURITY_H
#define WINPR_TIMEZONE_H
#define WINPR_THREAD_H
#define WINPR_WINDOWS_H
#define WINPR_INPUT_H
#define WINPR_WND_H
#define WINPR_LOG_H

#define INFINITE 0xFFFFFFFF

typedef struct rdp_rdp rdpRdp;
typedef struct rdp_gdi rdpGdi;
typedef struct rdp_rail rdpRail;
typedef struct rdp_cache rdpCache;
typedef struct rdp_channels rdpChannels;
typedef struct rdp_graphics rdpGraphics;
typedef struct rdp_metrics rdpMetrics;
typedef struct rdp_codecs rdpCodecs;

typedef struct rdp_freerdp freerdp;
typedef struct rdp_context rdpContext;
typedef struct rdp_freerdp_peer freerdp_peer;

/// Move this in to rdp.h ////
        typedef void* (*OBJECT_NEW_FN)(void* val);
        typedef void (*OBJECT_INIT_FN)(void* obj);
        typedef void (*OBJECT_UNINIT_FN)(void* obj);
        typedef void (*OBJECT_FREE_FN)(void* obj);
        typedef BOOL (*OBJECT_EQUALS_FN)(const void* objA, const void* objB);

        struct _wObject
        {
                OBJECT_NEW_FN fnObjectNew;
                OBJECT_INIT_FN fnObjectInit;
                OBJECT_UNINIT_FN fnObjectUninit;
                OBJECT_FREE_FN fnObjectFree;
                OBJECT_EQUALS_FN fnObjectEquals;
        };
        typedef struct _wObject wObject;

        struct _wLogMessage
        {
                DWORD Type;

                DWORD Level;

                LPSTR PrefixString;

                LPCSTR FormatString;
                LPSTR TextString;

                DWORD LineNumber;    /* __LINE__ */
                LPCSTR FileName;     /* __FILE__ */
                LPCSTR FunctionName; /* __FUNCTION__ */

                /* Data Message */

                void* Data;
                int Length;

                /* Image Message */

                void* ImageData;
                int ImageWidth;
                int ImageHeight;
                int ImageBpp;

                /* Packet Message */

                void* PacketData;
                int PacketLength;
                DWORD PacketFlags;
        };
        typedef struct _wLogMessage wLogMessage;
        typedef struct _wLogLayout wLogLayout;
        typedef struct _wLogAppender wLogAppender;
        typedef struct _wLog wLog;

        /* Message Queue */

        typedef struct _wMessage wMessage;

        typedef void (*MESSAGE_FREE_FN)(wMessage* message);

        struct _wMessage
        {
                UINT32 id;
                void* context;
                void* wParam;
                void* lParam;
                UINT64 time;
                MESSAGE_FREE_FN Free;
        };

/* winbase.h */
#define WAIT_OBJECT_0           0

/* winnt.h */
typedef RTL_CRITICAL_SECTION CRITICAL_SECTION;

#include "rdp.h"
#include "wtsapi.h"

typedef VIRTUALCHANNELENTRYEX* PVIRTUALCHANNELENTRYEX;


        typedef struct _wStreamPool wStreamPool;

        struct _wStream
        {
                BYTE* buffer;
                BYTE* pointer;
                size_t length;
                size_t capacity;

                DWORD count;
                wStreamPool* pool;
                BOOL isAllocatedStream;
                BOOL isOwner;
        };
        typedef struct _wStream wStream;


#define INLINE inline
#define FREERDP_EXPORTS
#define WINPR_DEPRECATED(obj) obj
//#define FREERDP_API __attribute__((visibility("default")))
#undef _WIN32
//#define __TERMSRV_LISTNER_C
//static FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN freerdp_load_static_channel_addin_entry = NULL;
//typedef FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN void;

	typedef struct _SYSTEMTIME
{
        WORD wYear;
        WORD wMonth;
        WORD wDayOfWeek;
        WORD wDay;
        WORD wHour;
        WORD wMinute;
        WORD wSecond;
        WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;


        typedef struct _TIME_ZONE_INFORMATION
        {
                LONG Bias;
                WCHAR StandardName[32];
                SYSTEMTIME StandardDate;
                LONG StandardBias;
                WCHAR DaylightName[32];
                SYSTEMTIME DaylightDate;
                LONG DaylightBias;
        } TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

#if 0
#define FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN WINAPI
//static FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN freerdp_load_static_channel_addin_entry = NULL;

#typedef 
FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN provider;

int freerdp_register_addin_provider(FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN provider, DWORD dwFlags)
{
//	freerdp_load_static_channel_addin_entry = provider;
	return 0;
}

FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN freerdp_get_current_addin_provider(void)
{
	return freerdp_load_static_channel_addin_entry;
}
#endif

#define WLog_ERR sprintf
#include "rdp.h"
#include </usr/include/freerdp2/freerdp/settings.h>
#define FREERDP_EVENT_H
#include </usr/include/freerdp2/freerdp/svc.h>
#include </usr/include/freerdp2/freerdp/client/rdpgfx.h>
#include </usr/include/freerdp2/freerdp/server/rdpgfx.h>
#include </usr/include/freerdp2/freerdp/listener.h>
//#include <freerdp/listener.h>

typedef long rdpServer;
//typedef long rdpContext;
rdpServer *server = NULL;

typedef struct
{
    rdpServer* server;
    HANDLE stopEvent;
} ServerContext;

static void handle_client(rdpContext* context)
{
    // Handle the RDP client connection
    // Implement your desired logic here
//     Draw a window 
}

static DWORD WINAPI ServerThread(LPVOID arg)
{
    ServerContext *serverContext;
#if 0
    *serverContext = (ServerContext*)arg;

    while (WaitForSingleObject(serverContext->stopEvent, 0) != WAIT_OBJECT_0)
    {
        if (!freerdp_server_accept(serverContext->server, handle_client))
        {
            //WLog_Print(g_Log, WLOG_ERROR, "Failed to accept client connection.");
        }
    }
#endif
    return 0;
}

#if 0
    // Initialize FreeRDP
    if (!freerdp_server_global_init())
    {
        printf("Failed to initialize FreeRDP server.\n");
        return -1;
    }

    /////  freerdp_settings_new
	info.test_dump_rfx_realtime = TRUE;

	errno = 0;

	for (int i = 1; i < argc; i++)
	{
		char* arg = argv[i];

		if (strncmp(arg, options.sfast, sizeof(options.sfast)) == 0)
			info.test_dump_rfx_realtime = FALSE;
		else if (strncmp(arg, options.sport, sizeof(options.sport)) == 0)
		{
			const char* sport = &arg[sizeof(options.sport)];
			port = strtol(sport, NULL, 10);

			if ((port < 1) || (port > UINT16_MAX) || (errno != 0))
				usage(app, arg);
		}
		else if (strncmp(arg, options.slocal_only, sizeof(options.slocal_only)) == 0)
			localOnly = TRUE;
		else if (strncmp(arg, options.spcap, sizeof(options.spcap)) == 0)
		{
			info.test_pcap_file = &arg[sizeof(options.spcap)];
			if (!winpr_PathFileExists(info.test_pcap_file))
				usage(app, arg);
		}
		else if (strncmp(arg, options.scert, sizeof(options.scert)) == 0)
		{
			info.cert = &arg[sizeof(options.scert)];
			if (!winpr_PathFileExists(info.cert))
				usage(app, arg);
		}
		else if (strncmp(arg, options.skey, sizeof(options.skey)) == 0)
		{
			info.key = &arg[sizeof(options.skey)];
			if (!winpr_PathFileExists(info.key))
				usage(app, arg);
		}
		else
			usage(app, arg);
	}

	WTSRegisterWtsApiFunctionTable(FreeRDP_InitWtsApi());
	winpr_InitializeSSL(WINPR_SSL_INIT_DEFAULT);
	instance = freerdp_listener_new();

	if (!instance)
		return -1;

	if (!info.cert)
		info.cert = "server.crt";
	if (!info.key)
		info.key = "server.key";

	instance->info = (void*)&info;
	instance->PeerAccepted = test_peer_accepted;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		goto fail;

	/* Open the server socket and start listening. */
	sprintf_s(name, sizeof(name), "tfreerdp-server.%ld", port);
	file = GetKnownSubPath(KNOWN_PATH_TEMP, name);

	if (!file)
		goto fail;

	if (localOnly)
	{
		WINPR_ASSERT(instance->OpenLocal);
		started = instance->OpenLocal(instance, file);
	}
	else
	{
		WINPR_ASSERT(instance->Open);
		started = instance->Open(instance, NULL, (UINT16)port);
	}

	if (started)
	{
		/* Entering the server main loop. In a real server the listener can be run in its own
		 * thread. */
		test_server_mainloop(instance);
	}

#if 0
    // Create an RDP server instance
    server = freerdp_server_new();
    // Set server settings
    server->Port = 3389;
    server->NlaSecurity = FALSE; // Disable NLA authentication

    // Start the RDP server
    status = freerdp_server_start(server);
    if (status != 0)
    {
        printf("Failed to start RDP server. Error: %d\n", status);
        freerdp_server_free(server);
        freerdp_server_global_uninit();
        return -1;
    }

   // Create an RDP server instance
   // server = freerdp_server_new();

    // Set server settings
    //server->Port = 3389;
    //server->NlaSecurity = FALSE; // Disable NLA authentication

    /* Disable Auth */
    // Create and configure your RDP server here
    //freerdp_settings* settings = freerdp_server_settings_new();
    //settings->SecurityMode = SECURITY_PROTOCOL_NEGO;
    //settings->NlaSecurity = FALSE;

    // Configure other server settings as needed

    // Set the server settings
    //freerdp_server_set_settings(server, settings);

    // Start the server
    //freerdp_server_start(server);

    /*
    status = freerdp_server_start(server);
    if (status != 0)
    {
        printf("Failed to start RDP server. Error: %d\n", status);
        freerdp_server_free(server);
        freerdp_server_global_uninit();
        return -1;
    }
*/
//////////////////////////////////////////////////////////////////////////////////////////////////
    // Start the RDP server thread
    serverContext.server = server;
    serverContext.stopEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    serverThread = CreateThread(NULL, 0, ServerThread, &serverContext, 0, NULL);
    if (serverThread == NULL)
    {
        //WLog_Print(g_Log, WLOG_ERROR, "Failed to create server thread.");
        freerdp_server_free(server);
        freerdp_server_global_uninit();
        return -1;
    }
//////////////////////////////////////////////////////////////////////////////////////////////////

    SERVICE_STATUS status;
    // Accept incoming RDP connections
    while (serviceStatus.dwCurrentState == SERVICE_RUNNING)
    {
/*
    while (freerdp_server_is_running(server))
    {
        HANDLE clientEvent;
        DWORD dwWaitResult;

        clientEvent = freerdp_server_get_event_handle(server);
        dwWaitResult = WaitForSingleObject(clientEvent, INFINITE);

        if (dwWaitResult == WAIT_OBJECT_0)
        {
            // Process the incoming client connection
            if (!freerdp_server_accept(server, handle_client))
            {
                printf("Failed to accept client connection.\n");
            }
        }
*/

        // Wait for a client connection
        if (freerdp_listener_get_fds(listener, NULL, NULL, NULL, 0) != TRUE)
        {
            printf("Failed to get file descriptor for listener.\n");
            break;
        }

        // Check for incoming connections
        if (freerdp_listener_check_fds(listener) != TRUE)
        {
            printf("Failed to check file descriptor for listener.\n");
            break;
        }

        // Accept incoming connection
        if (freerdp_listener_accept(listener, &client) < 0)
        {
            printf("Failed to accept incoming connection.\n");
            break;
        }

        printf("Client connected.\n");

        // Main server loop
        while (freerdp_peer_get_event_handle(client))
        {
            if (freerdp_peer_check_fds(client) != TRUE)
            {
                printf("Failed to check file descriptor for client.\n");
                break;
            }
        }
        printf("Client disconnected.\n");
    }

    ////////////////////////////////////////////////////
    // Wait until the service is stopped
    WaitForSingleObject(GetCurrentThread(), INFINITE);

    // Stop and clean up the listener
    freerdp_listener_stop(listener);
    freerdp_listener_free(listener);

    // Clean up the client
    freerdp_peer_free(client);
    client = NULL;

    return 1;
//}

//int connect()
//{

    /* Set the callback function to handle client connections */
    //listener->PeerAccepted = handle_client;
    //freerdp_listener* listener;

    /* Initialize FreeRDP */
    //freerdp_listener_create(&listener);

    listener = freerdp_listener_new();
    if (!listener)
    {
        printf("Failed to create FreeRDP listener.\n");
        return -1;
    }

    // Set the listener options
    listener->PeerAccepted = NULL; // No authentication callback

    // Disable authentication
    //freerdp_listener_set_authenticate_peer(listener, FALSE);

    // Create an RDP server instance
    server = freerdp_server_new();

    // Set server settings
    server->Port = 3389;
    server->NlaSecurity = FALSE; // Disable NLA authentication



    // Wait for incoming client connections
    while (freerdp_server_is_running(server))
    {
        HANDLE clientEvent;
        DWORD dwWaitResult;

        clientEvent = freerdp_server_get_event_handle(server);
        dwWaitResult = WaitForSingleObject(clientEvent, INFINITE);

        if (dwWaitResult == WAIT_OBJECT_0)
        {
            // Process the incoming client connection
            if (!freerdp_server_accept(server, handle_client))
            {
                printf("Failed to accept client connection.\n");
            }
        }
    }
fail:
	free(file);
	freerdp_listener_free(instance);
	WSACleanup();
	return status;
}
#endif

#include <pthread.h>

void* handle_client(void* arg)
{
    freerdp_peer* client = (freerdp_peer*)arg;

    // Handle the client connection
    // ...

    // Cleanup and disconnect the client
    freerdp_peer_free(client);

    return NULL;
}
#endif

#if 0
#include <stdio.h>
#include <winsock2.h>

int main()
{
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    char buffer[4096];

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("Failed to initialize Winsock\n");
        return 1;
    }

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET)
    {
        printf("Failed to create socket\n");
        WSACleanup();
        return 1;
    }

    // Bind socket
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(3389);
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        printf("Failed to bind socket\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("Failed to listen for connections\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("RDP server is listening on port 3389...\n");

    while (1)
    {
        // Accept client connection
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET)
        {
            printf("Failed to accept client connection\n");
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        // Send "Please Wait" message
        const char* message = "Please Wait";
        send(clientSocket, message, strlen(message), 0);

        // Perform other tasks as needed

        // Close client connection
        closesocket(clientSocket);
        printf("Client disconnected\n");
    }

    // Close server socket
    closesocket(serverSocket);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}

#endif




//#include <freerdp/freerdp.h>
//#include <freerdp/listener.h>
#include <pthread.h>
#if 0
void* handle_client(void* arg)
{
    freerdp_peer* client = (freerdp_peer*)arg;

            //if (freerdp_peer_get_event_handle(client))
            //{
             //   if (freerdp_peer_check_fds(client) != TRUE)
             //   {
             //       printf("Failed to check file descriptor for client.\n");
             //       break;
             //   }
             // }


    // Handle the client connection
    // ...

    // Cleanup and disconnect the client
    freerdp_peer_free(client);

    return NULL;
}
#endif


int rdpserve_main(int argc, char* argv[])
{
    //FreeRDP_Init();
    freerdp_listener* listener = freerdp_listener_new();
        fprintf(stdout, " listener created\n");
    if (!listener)
    {
        fprintf(stdout, "Failed to create listener.\n");
        freerdp_listener_free(listener);
        return -1;
    }

    //freerdp_listener_set_security_mode(listener, ENCRYPTION_NONE);
    //freerdp_listener_set_security_level(listener, RDP_SECURITY_LEVEL_LOW);

    while (1)
    {
        fprintf(stdout, "check_fds\n");
        // Handle incoming connections
        freerdp_check_fds(listener);
        fprintf(stdout, "check_fds_passed\n");

	//0000000000077030 T freerdp_channels_get_event_handle
        //00000000000771d0 T freerdp_check_event_handles
        //0000000000077050 T freerdp_get_event_handles

        // Check if there are new clients waiting to be processed
        while (freerdp_channels_get_event_handle(listener))
        {
            printf ("trynig to spawn a thread\n");
            // Accept the new client connection
            //freerdp_peer* client;
            freerdp_peer* client = freerdp_peer_new(listener);

            //if (freerdp_peer_get_event_handle(client))
            //{
             //   if (freerdp_peer_check_fds(client) != TRUE)
             //   {
             //       printf("Failed to check file descriptor for client.\n");
             //       break;
             //   }
             // }

            // Create a new thread to handle the client connection
          //  pthread_t thread;
          //  if (pthread_create(&thread, NULL, handle_client, client) != 0)
          //  {
          //      fprintf(stdout, "Failed to create thread for client connection.\n");
          //      freerdp_peer_free(client);
          //  }
        }

        // Perform other tasks as needed
        // ...
    }

    // Cleanup and shutdown
    freerdp_listener_free(listener);
    FreeRDP_Shutdown();

    return 0;
}
#endif
