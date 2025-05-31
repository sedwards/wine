#include "rdsdrv_dll.h"

#include "ntuser.h"
#include "winuser.h"

#include "rds_message.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winerds);

static HANDLE hPipe = INVALID_HANDLE_VALUE;
static HANDLE hClientThread = NULL;
static volatile BOOL bClientRunning = FALSE;
static CRITICAL_SECTION pipe_cs; // For thread-safe access to hPipe by SendRDSMessage

DWORD WINAPI RDSClientPipeThread(LPVOID lpParam)
{
    printf("RDS Pipe Client Thread started.\n");
    while (bClientRunning)
    {
        if (hPipe == INVALID_HANDLE_VALUE) // Attempt to connect only if not already connected
        {
            printf("Attempting to connect to named pipe %S...\n", L"\\\\.\\pipe\\wine_rds_gdi_commands");
            hPipe = CreateFileW(
                L"\\\\.\\pipe\\wine_rds_gdi_commands", // Pipe name
                GENERIC_READ | GENERIC_WRITE,
                0,                            // No sharing
                NULL,                         // Default security attributes
                OPEN_EXISTING,                // Opens existing pipe
                0,                            // Default attributes
                NULL);                        // No template file

            if (hPipe != INVALID_HANDLE_VALUE)
            {
                printf("Connected to named pipe.\n");
                // Optional: Could set to message mode if server is also message mode
                // DWORD dwMode = PIPE_READMODE_MESSAGE;
                // SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL);
            }
            else
            {
                DWORD dwError = GetLastError();
                // Don't spam logs if server is just not there yet
                if (dwError != ERROR_FILE_NOT_FOUND) {
                   printf("Error connecting to named pipe: %lu\n", dwError);
                }
            }
        }

        if (hPipe == INVALID_HANDLE_VALUE && bClientRunning)
        {
            Sleep(2000); // Wait before retrying connection
            continue;
        }
        
        // If connected, this thread could optionally handle server-initiated messages (responses)
        // For now, it just maintains the connection. SendRDSMessage will do the writing.
        // If the pipe breaks, hPipe will be closed by SendRDSMessage or here.
        if(bClientRunning) Sleep(100); // Keep thread alive, periodically check bClientRunning
    }

    // Cleanup if thread is exiting and pipe is still open
    EnterCriticalSection(&pipe_cs);
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
        printf("Pipe handle closed in thread exit.\n");
    }
    LeaveCriticalSection(&pipe_cs);
    printf("RDS Pipe Client Thread exiting.\n");
    return 0;
}

BOOL StartRDSClientPipe(void)
{
    if (bClientRunning) return TRUE;

    InitializeCriticalSection(&pipe_cs);
    bClientRunning = TRUE;
    hClientThread = CreateThread(NULL, 0, RDSClientPipeThread, NULL, 0, NULL);
    if (hClientThread == NULL)
    {
        printf("Failed to create pipe client thread, GLE=%ld.\n", GetLastError());
        DeleteCriticalSection(&pipe_cs);
        bClientRunning = FALSE;
        return FALSE;
    }
    printf("Pipe client thread successfully started.\n");
    return TRUE;
}

void StopRDSClientPipe(void)
{
    if (!bClientRunning) return;

    printf("Stopping pipe client thread...\n");
    bClientRunning = FALSE;

    EnterCriticalSection(&pipe_cs);
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipe); 
        hPipe = INVALID_HANDLE_VALUE;
        printf("Pipe handle closed in StopRDSClientPipe.\n");
    }
    LeaveCriticalSection(&pipe_cs);

    if (hClientThread)
    {
        WaitForSingleObject(hClientThread, INFINITE);
        CloseHandle(hClientThread);
        hClientThread = NULL;
    }
    DeleteCriticalSection(&pipe_cs);
    printf("Pipe client thread stopped.\n");
}

BOOL SendRDSMessage(const RDS_MESSAGE *msg, const void *variable_data, DWORD variable_data_size)
{
    DWORD cbWritten;
    BOOL bSuccess = FALSE;

    if (!msg) return FALSE;
    if (!bClientRunning || hPipe == INVALID_HANDLE_VALUE)
    {
        printf("Pipe not connected, cannot send message type %d\n", msg->msgType);
        return FALSE;
    }

    EnterCriticalSection(&pipe_cs);
    if (hPipe == INVALID_HANDLE_VALUE) // Double check after acquiring lock
    {
        LeaveCriticalSection(&pipe_cs);
        return FALSE;
    }

    // Send the fixed part of the message
    if (WriteFile(hPipe, msg, sizeof(RDS_MESSAGE), &cbWritten, NULL))
    {
        if (cbWritten == sizeof(RDS_MESSAGE))
        {
            bSuccess = TRUE;
            // Send variable data if present
            if (variable_data && variable_data_size > 0)
            {
                if (WriteFile(hPipe, variable_data, variable_data_size, &cbWritten, NULL))
                {
                    if (cbWritten == variable_data_size)
                    {
                        // printf("Successfully sent message type %d with %lu bytes of variable data.\n", msg->msgType, variable_data_size);
                    }
                    else 
                    {
                        printf("WriteFile for variable data wrote %lu of %lu bytes.\n", cbWritten, variable_data_size);
                        bSuccess = FALSE; 
                    }
                }
                else 
                {
                    printf("WriteFile for variable data failed, GLE=%ld.\n", GetLastError());
                    bSuccess = FALSE;
                }
            }
             else { // No variable data or zero size
                printf("Successfully sent message type %d.\n", msg->msgType);
             }
        }
        else 
        {
            printf("WriteFile for RDS_MESSAGE wrote %lu of %u bytes.\n", cbWritten, (unsigned int)sizeof(RDS_MESSAGE));
        }
    }
    else 
    {
        printf("WriteFile for RDS_MESSAGE failed, GLE=%ld.\n", GetLastError());
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
        printf("Pipe handle closed due to WriteFile failure.\n");
    }
    
    LeaveCriticalSection(&pipe_cs);
    return bSuccess;
}
