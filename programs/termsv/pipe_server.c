#include <windows.h>
#include <stdio.h>
#include "../../dlls/winerds.drv/rds_message.h" // Attempting relative path
#include "pipe_server.h"

static HANDLE hPipeThread = NULL;
static volatile BOOL bServerRunning = FALSE;

DWORD WINAPI RDSPipeServerThread(LPVOID lpParam)
{
    char buffer[sizeof(RDS_MESSAGE) + 4096]; // Buffer for RDS_MESSAGE + potential variable data
    DWORD cbRead;
    BOOL bConnected;
    HANDLE hCurrentPipe = INVALID_HANDLE_VALUE;

    printf("RDS Pipe Server Thread started.\n");

    while (bServerRunning)
    {
        hCurrentPipe = CreateNamedPipeW(
            L"\\\\.\\pipe\\wine_rds_gdi_commands", // Pipe name
            PIPE_ACCESS_DUPLEX,         // Read/write access
            PIPE_TYPE_MESSAGE |         // Message type pipe
            PIPE_READMODE_MESSAGE |     // Message-read mode
            PIPE_WAIT,                 // Blocking mode
            1,                         // Max. instances
            sizeof(buffer),            // Output buffer size
            sizeof(buffer),            // Input buffer size
            NMPWAIT_USE_DEFAULT_WAIT,  // Default timeout
            NULL);                     // Default security attributes

        if (hCurrentPipe == INVALID_HANDLE_VALUE)
        {
            printf("CreateNamedPipe failed, GLE=%d.\n", GetLastError());
            if (!bServerRunning) break;
            Sleep(1000); // Wait a bit before retrying
            continue;
        }

        printf("Pipe server instance created (%p). Waiting for client connection...\n", hCurrentPipe);

        bConnected = ConnectNamedPipe(hCurrentPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (bConnected && bServerRunning)
        {
            printf("Client connected to pipe instance (%p).\n", hCurrentPipe);
            // Loop to receive messages
            while (bServerRunning)
            {
                RDS_MESSAGE *rdsMsg = (RDS_MESSAGE*)buffer;
                // Zero out the RDS_MESSAGE part of the buffer before reading
                memset(rdsMsg, 0, sizeof(RDS_MESSAGE));

                if (ReadFile(hCurrentPipe, buffer, sizeof(buffer), &cbRead, NULL)) // Read up to buffer size
                {
                    if (cbRead >= sizeof(RDS_MESSAGE_TYPE)) // At least msgType should be there
                    {
                        printf("Received message type: %d (%lu bytes read)\n", rdsMsg->msgType, cbRead);

                        // Example for TextOut:
                        if (rdsMsg->msgType == RDS_MSG_TEXT_OUT && cbRead >= sizeof(RDS_MESSAGE) && rdsMsg->params.textOut.data_size > 0)
                        {
                            printf("Received TextOut with data_size: %lu\n", rdsMsg->params.textOut.data_size);
                        }
                    }
                    else if (cbRead == 0) 
                    {
                        printf("ReadFile returned 0 bytes, but TRUE. Client likely disconnected gracefully.\n");
                        break; 
                    }
                }
                else // ReadFile failed
                {
                    DWORD dwError = GetLastError();
                    if (dwError == ERROR_BROKEN_PIPE)
                    {
                        printf("Client disconnected (ReadFile - ERROR_BROKEN_PIPE) from pipe instance (%p).\n", hCurrentPipe);
                    }
                    else
                    {
                        printf("ReadFile failed, GLE=%d, on pipe instance (%p).\n", dwError, hCurrentPipe);
                    }
                    break; // Error or client disconnected
                }
            }
        }
        else if (!bServerRunning && hCurrentPipe != INVALID_HANDLE_VALUE)
        {
            printf("Server shutting down while waiting for client on pipe instance (%p).\n", hCurrentPipe);
        }
        
        printf("Closing pipe instance %p\n", hCurrentPipe);
        DisconnectNamedPipe(hCurrentPipe);
        CloseHandle(hCurrentPipe);
        hCurrentPipe = INVALID_HANDLE_VALUE;
    }
    printf("Pipe server thread exiting.\n");
    return 0;
}

BOOL StartRDSPipeServer(void)
{
    if (bServerRunning) return TRUE;

    bServerRunning = TRUE;
    hPipeThread = CreateThread(NULL, 0, RDSPipeServerThread, NULL, 0, NULL);
    if (hPipeThread == NULL)
    {
        printf("Failed to create pipe server thread, GLE=%d.\n", GetLastError());
        bServerRunning = FALSE;
        return FALSE;
    }
    printf("Pipe server thread successfully started.\n");
    return TRUE;
}

void StopRDSPipeServer(void)
{
    if (!bServerRunning) return;

    printf("Stopping pipe server thread...\n");
    bServerRunning = FALSE;

    // Create a dummy client to connect to the pipe and unblock the server thread
    // if it's waiting in ConnectNamedPipe.
    HANDLE hDummyClient = CreateFileW(
        L"\\\\.\\pipe\\wine_rds_gdi_commands",
        GENERIC_WRITE, 
        0,             
        NULL,          
        OPEN_EXISTING, 
        0,             
        NULL);         

    if (hDummyClient != INVALID_HANDLE_VALUE)
    {
        printf("Dummy client connected to unblock server thread.\n");
        CloseHandle(hDummyClient);
    } else {
        // This might happen if the server was not actually blocked on ConnectNamedPipe,
        // or if the pipe was already being closed.
        printf("Dummy client connection failed GLE=%d. Server thread might take longer to stop if blocked on ConnectNamedPipe.\n", GetLastError());
    }

    if (hPipeThread)
    {
        printf("Waiting for pipe server thread to exit...\n");
        WaitForSingleObject(hPipeThread, INFINITE); // Wait for the thread to finish
        CloseHandle(hPipeThread);
        hPipeThread = NULL;
    }
    printf("Pipe server thread stopped.\n");
}

