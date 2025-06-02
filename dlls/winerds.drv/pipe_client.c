#include "rdsdrv_dll.h"

#include "ntuser.h"
#include "winuser.h"

#include "pipe_client.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winerds);

static HANDLE hPipe = INVALID_HANDLE_VALUE;
static HANDLE hClientThread = NULL;
static volatile BOOL bClientRunning = FALSE;
static CRITICAL_SECTION pipe_cs; // For thread-safe access to hPipe by SendRDSMessage

DWORD WINAPI RDSClientPipeThread(LPVOID lpParam)
{
    FIXME("RDS Pipe Client Thread started.\n");

    LPCWSTR pipe_name_log = (LPCWSTR)L"\\\\.\\pipe\\wine_rds_gdi_commands"; // For logging
    FIXME("winerds.drv: RDSClientPipeThread: Running (bClientRunning=%d).\n", bClientRunning); // Check if thread runs


    while (bClientRunning)
    {
        if (hPipe == INVALID_HANDLE_VALUE) // Attempt to connect only if not already connected
        {
            FIXME("winerds.drv: RDSClientPipeThread: Attempting to connect to pipe: %S\n", pipe_name_log);
            hPipe = CreateFileW(
                pipe_name_log, // Use the variable for clarity
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);

            if (hPipe != INVALID_HANDLE_VALUE)
            {
		FIXME("winerds.drv: RDSClientPipeThread: Successfully connected to pipe %S. Handle: %p\n", pipe_name_log, hPipe);

		// Optional: Set to message mode (though server sets this, client can too)
                // DWORD dwMode = PIPE_READMODE_MESSAGE;
                // if (!SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL)) {
                //    FIXME("winerds.drv: RDSClientPipeThread: SetNamedPipeHandleState failed. GLE=%u\n", (unsigned int)GetLastError());
                // }
            }
            else
            {
                FIXME("winerds.drv: RDSClientPipeThread: Failed to connect to pipe %S. GLE=%u\n", pipe_name_log, (unsigned int)GetLastError());
                DWORD dwError = GetLastError();
                // Don't spam logs if server is just not there yet
                if (dwError != ERROR_FILE_NOT_FOUND) {
                   FIXME("Error connecting to named pipe: %lu\n", dwError);
                }
            }
        }

        if (hPipe == INVALID_HANDLE_VALUE && bClientRunning)
        {
            Sleep(2000); // Wait before retrying connection
            continue;
        }

    	if (!bClientRunning) break; // Exit if server is stopping

    	// If connected, this thread could optionally handle server-initiated messages (responses)
    	// For now, it just maintains the connection. SendRDSMessage will do the writing.
    	// Add a check to see if the pipe is still valid, otherwise try to reconnect.
    	DWORD pipe_state = 0;
    	if (!GetNamedPipeHandleStateA(hPipe, &pipe_state, NULL, NULL, NULL, NULL, 0))
    	{
        	FIXME("winerds.drv: RDSClientPipeThread: Pipe %p no longer valid (e.g. server closed it). GLE=%u. Attempting to reconnect.\n", hPipe, (unsigned int)GetLastError());
        	EnterCriticalSection(&pipe_cs);
        	CloseHandle(hPipe);
        	hPipe = INVALID_HANDLE_VALUE;
        	LeaveCriticalSection(&pipe_cs);
        	// The loop will then attempt to reconnect via CreateFileW
    	} else {
        	Sleep(100); // Pipe still seems valid, sleep before checking bClientRunning again
    	}

    }

    // Cleanup if thread is exiting and pipe is still open
    EnterCriticalSection(&pipe_cs);
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
        FIXME("Pipe handle closed in thread exit.\n");
    }
    LeaveCriticalSection(&pipe_cs);
    FIXME("RDS Pipe Client Thread exiting.\n");
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
        FIXME("Failed to create pipe client thread, GLE=%ld.\n", GetLastError());
        DeleteCriticalSection(&pipe_cs);
        bClientRunning = FALSE;
        return FALSE;
    }
    FIXME("Pipe client thread successfully started.\n");
    return TRUE;
}

void StopRDSClientPipe(void)
{
    if (!bClientRunning) return;

    FIXME("Stopping pipe client thread...\n");
    bClientRunning = FALSE;

    EnterCriticalSection(&pipe_cs);
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipe); 
        hPipe = INVALID_HANDLE_VALUE;
        FIXME("Pipe handle closed in StopRDSClientPipe.\n");
    }
    LeaveCriticalSection(&pipe_cs);

    if (hClientThread)
    {
        WaitForSingleObject(hClientThread, INFINITE);
        CloseHandle(hClientThread);
        hClientThread = NULL;
    }
    DeleteCriticalSection(&pipe_cs);
    FIXME("Pipe client thread stopped.\n");
}

BOOL SendRDSMessage(const RDS_MESSAGE *msg, const void *variable_data, DWORD variable_data_size)
{
    DWORD cbWritten;
    BOOL bSuccess = FALSE;

    if (!msg) return FALSE;
    if (!bClientRunning || hPipe == INVALID_HANDLE_VALUE)
    {
        FIXME("Pipe not connected, cannot send message type %d\n", msg->msgType);
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
                        // FIXME("Successfully sent message type %d with %lu bytes of variable data.\n", msg->msgType, variable_data_size);
                    }
                    else 
                    {
                        FIXME("WriteFile for variable data wrote %lu of %lu bytes.\n", cbWritten, variable_data_size);
                        bSuccess = FALSE; 
                    }
                }
                else 
                {
                    FIXME("WriteFile for variable data failed, GLE=%ld.\n", GetLastError());
                    bSuccess = FALSE;
                }
            }
             else { // No variable data or zero size
                FIXME("Successfully sent message type %d.\n", msg->msgType);
             }
        }
        else 
        {
            FIXME("WriteFile for RDS_MESSAGE wrote %lu of %u bytes.\n", cbWritten, (unsigned int)sizeof(RDS_MESSAGE));
        }
    }
    else 
    {
        FIXME("WriteFile for RDS_MESSAGE failed, GLE=%ld.\n", GetLastError());
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
        FIXME("Pipe handle closed due to WriteFile failure.\n");
    }
    
    LeaveCriticalSection(&pipe_cs);
    return bSuccess;
}

