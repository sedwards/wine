#include <windows.h>
#include <stdio.h>
#include "../../dlls/winerds.drv/rds_message.h" // Attempting relative path
#include "pipe_server.h"
#include "rds_gdi_handlers.h" // For GDI message handlers

static HANDLE hPipeThread = NULL;
static volatile BOOL bServerRunning = FALSE;
#define RDS_PIPE_NAME L"\\\\.\\pipe\\WineRDS"



DWORD WINAPI RDSPipeServerThread(LPVOID lpParam)
{
    char buffer[sizeof(RDS_MESSAGE) + 4096]; // Buffer for RDS_MESSAGE + potential variable data
    DWORD cbRead;
    BOOL bConnected;
    HANDLE hCurrentPipe = INVALID_HANDLE_VALUE;
    DWORD connection_count = 0;
    DWORD thread_start_time = GetTickCount();

    printf("[PIPE] Server thread started at %lu ms\n", (unsigned long)thread_start_time);

    while (bServerRunning)
    {
        hCurrentPipe = CreateNamedPipeW(
            RDS_PIPE_NAME, // Pipe name
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
            printf("CreateNamedPipe failed, GLE=%ld.\n", GetLastError());
            if (!bServerRunning) break;
            Sleep(1000); // Wait a bit before retrying
            continue;
        }

        printf("[PIPE] Instance created (%p) at %lu ms. Waiting for client...\n", 
               hCurrentPipe, (unsigned long)GetTickCount());

        bConnected = ConnectNamedPipe(hCurrentPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (bConnected && bServerRunning)
        {
            connection_count++;
            DWORD connection_time = GetTickCount();
            printf("[PIPE] Client #%lu connected to instance (%p) at %lu ms\n", 
                   (unsigned long)connection_count, hCurrentPipe, (unsigned long)connection_time);
            // Loop to receive messages
            while (bServerRunning)
            {
                // RDS_MESSAGE *rdsMsg = (RDS_MESSAGE*)buffer; // Moved down
                // Zero out the RDS_MESSAGE part of the buffer before reading
                // memset(rdsMsg, 0, sizeof(RDS_MESSAGE)); // This should be done carefully, or let ReadFile overwrite

                if (ReadFile(hCurrentPipe, buffer, sizeof(buffer), &cbRead, NULL))
                {
                    if (cbRead == 0) // Client disconnected gracefully after ConnectNamedPipe before sending anything
                    {
                       printf("[PIPE] Client disconnected gracefully (0 bytes read) at %lu ms\n", 
                              (unsigned long)GetTickCount());
                       break; // Break from inner message reading loop
                    }

                    if (cbRead >= sizeof(RDS_MESSAGE_TYPE)) // Basic check: enough data for at least the msgType
                    {
                        RDS_MESSAGE *rdsMsg = (RDS_MESSAGE*)buffer;
                        printf("[MSG] Received type %d (%s), size=%lu bytes at %lu ms\n", 
                               rdsMsg->msgType,
                               rdsMsg->msgType == RDS_MSG_PING ? "PING" :
                               rdsMsg->msgType == RDS_MSG_PONG ? "PONG" :
                               rdsMsg->msgType == RDS_MSG_LINE_TO ? "LINE_TO" :
                               rdsMsg->msgType == RDS_MSG_RECTANGLE ? "RECTANGLE" :
                               rdsMsg->msgType == RDS_MSG_TEXT_OUT ? "TEXT_OUT" :
                               rdsMsg->msgType == RDS_MSG_MOVE_TO ? "MOVE_TO" : "OTHER",
                               cbRead, (unsigned long)GetTickCount());

                        // Ensure we have the full fixed part of the message
                        if (cbRead < sizeof(RDS_MESSAGE)) {
                            // WINE_WARN("Received partial message: %lu bytes, expected at least %u for fixed part.\n", cbRead, (unsigned int)sizeof(RDS_MESSAGE));
                            printf("WARN: Received partial message: %lu bytes, expected at least %u for fixed part.\n", cbRead, (unsigned int)sizeof(RDS_MESSAGE));
                            // Decide how to handle: continue to try to process, or break, or request more data (complex)
                            // For now, we'll try to process if msgType is known and params are simple.
                        }

                        switch (rdsMsg->msgType)
                        {
                            case RDS_MSG_PING:
				 if (cbRead >= sizeof(RDS_MESSAGE))
				     Handle_RDS_MSG_PING(rdsMsg, hCurrentPipe);
				 else  
                                    printf("WARN: Partial RDS_MSG_PING message received.\n");
                                break;
                            case RDS_MSG_MOVE_TO:
                                // Check size for this specific message part.
                                if (cbRead >= sizeof(rdsMsg->params.moveTo) + sizeof(RDS_MESSAGE_TYPE)) 
                                    Handle_RDS_MSG_MOVE_TO(rdsMsg);
                                else
                                    printf("WARN: Partial RDS_MSG_MOVE_TO message received.\n");
                                break;
                            case RDS_MSG_LINE_TO:
                                if (cbRead >= sizeof(rdsMsg->params.lineTo) + sizeof(RDS_MESSAGE_TYPE))
                                    Handle_RDS_MSG_LINE_TO(rdsMsg);
                                else
                                    printf("WARN: Partial RDS_MSG_LINE_TO message received.\n");
                                break;
                            case RDS_MSG_RECTANGLE:
                                // Ensure enough data for rectangle params (msgType + params.rectangle)
                                // This check is simplified for the fixed part of the message.
                                // A more precise check would be sizeof(rdsMsg->msgType) + sizeof(rdsMsg->params.rectangle)
                                // but sizeof(RDS_MESSAGE) covers the largest possible fixed part.
                                if (cbRead >= sizeof(RDS_MESSAGE)) 
                                    Handle_RDS_MSG_RECTANGLE(rdsMsg);
                                else
                                    printf("WARN: Partial RDS_MSG_RECTANGLE message received. Expected %u, got %lu.\n", (unsigned int)sizeof(RDS_MESSAGE), cbRead);
                                break;
                            case RDS_MSG_TEXT_OUT:
                                // Check if the fixed part of RDS_MESSAGE (up to data_size field) is received
                                // The struct definition ensures params.textOut is within RDS_MESSAGE.
                                if (cbRead >= sizeof(RDS_MESSAGE))
                                {
                                    // Fixed size of an RDS_MESSAGE structure:
                                    size_t fixed_rds_message_size = sizeof(RDS_MESSAGE); 

                                    if (rdsMsg->params.textOut.data_size > 0)
                                    {
                                        // PIPE_TYPE_MESSAGE should deliver the entire message (fixed + variable) in one ReadFile,
                                        // if the buffer is large enough.
                                        if (cbRead == fixed_rds_message_size + rdsMsg->params.textOut.data_size) {
                                            WCHAR *text_data = (WCHAR *)(buffer + fixed_rds_message_size);
                                            Handle_RDS_MSG_TEXT_OUT(rdsMsg, text_data);
                                        } else {
                                            printf("WARN: Size mismatch for RDS_MSG_TEXT_OUT with data. cbRead=%lu, expected_fixed=%u, data_size_field=%lu, expected_total=%lu\n",
                                                   cbRead, (unsigned int)fixed_rds_message_size, (unsigned long)rdsMsg->params.textOut.data_size, (unsigned long)(fixed_rds_message_size + rdsMsg->params.textOut.data_size));
                                        }
                                    }
                                    else // No variable data (data_size is 0, count might also be 0)
                                    {
                                         // Ensure the full fixed struct was read if data_size is 0
                                         if (cbRead == fixed_rds_message_size) {
                                            Handle_RDS_MSG_TEXT_OUT(rdsMsg, NULL); // No text data to pass
                                         } else {
                                            printf("WARN: Partial RDS_MSG_TEXT_OUT (no data) message received. cbRead=%lu, expected_fixed=%u\n", cbRead, (unsigned int)fixed_rds_message_size);
                                         }
                                    }
                                }
                                else
                                {
                                    printf("WARN: Partial RDS_MSG_TEXT_OUT message received (too small for RDS_MESSAGE fixed part). Expected %u, got %lu\n", (unsigned int)sizeof(RDS_MESSAGE), cbRead);
                                }
                                break;
                            default:
                                printf("WARN: Received unhandled message type: %d\n", rdsMsg->msgType);
                                break;
                        }
                    } else {
                        // WINE_ERR("ReadFile returned TRUE but cbRead (%lu) is too small for a message type.\n", cbRead);
                        printf("ERR: ReadFile returned TRUE but cbRead (%lu) is too small for a message type.\n", cbRead);
                        // This case should ideally not be hit if client sends valid messages.
                    }
                }
                else // ReadFile failed
                {
                    DWORD dwError = GetLastError();
                    if (dwError == ERROR_BROKEN_PIPE)
                    {
                        printf("[PIPE] Client disconnected (ERROR_BROKEN_PIPE) at %lu ms\n", 
                               (unsigned long)GetTickCount());
                    }
                    else
                    {
                        printf("ReadFile failed, GLE=%ld, on pipe instance (%p).\n", dwError, hCurrentPipe);
                    }
                    break; // Break from inner message reading loop (client disconnected or error)
                }
            }
        }
        else if (!bServerRunning && hCurrentPipe != INVALID_HANDLE_VALUE)
        {
            printf("Server shutting down while waiting for client on pipe instance (%p).\n", hCurrentPipe);
        }
        
        printf("[PIPE] Closing instance %p at %lu ms\n", hCurrentPipe, (unsigned long)GetTickCount());
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
        printf("Failed to create pipe server thread, GLE=%ld.\n", GetLastError());
        bServerRunning = FALSE;
        return FALSE;
    }
    printf("Pipe server thread successfully started.\n");
    return TRUE;
}

void StopRDSPipeServer(void)
{
    if (!bServerRunning) return;

    HANDLE hDummyClient;
    
    printf("Stopping pipe server thread...\n");
    bServerRunning = FALSE;

    // Create a dummy client to connect to the pipe and unblock the server thread
    // if it's waiting in ConnectNamedPipe.
    hDummyClient = CreateFileW(
        RDS_PIPE_NAME,
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
        printf("Dummy client connection failed GLE=%ld. Server thread might take longer to stop if blocked on ConnectNamedPipe.\n", GetLastError());
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
