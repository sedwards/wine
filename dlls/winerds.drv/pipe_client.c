/* pipe_client.c - Connect to WineRDS named pipe from driver side */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <wine/debug.h>

#include "rds_message.h"

WINE_DEFAULT_DEBUG_CHANNEL(rds);

#define RDS_PIPE_NAME L"\\\\.\\pipe\\WineRDS"
#define RDS_PIPE_TIMEOUT_MS 5000
#define RDS_PIPE_RETRY_INTERVAL_MS 250
#define KEEPALIVE_INTERVAL_MS 5000

static HANDLE rds_pipe = INVALID_HANDLE_VALUE;
static HANDLE hKeepAliveThread = NULL;
static volatile BOOL bKeepAliveRun = FALSE;

static unsigned __stdcall KeepAliveThreadProc(void *arg)
{
    RDS_MESSAGE msg;
    TRACE("KeepAliveThreadProc started.\n");

    while (bKeepAliveRun && rds_pipe != INVALID_HANDLE_VALUE)
    {
        // Construct and send PING message
        ZeroMemory(&msg, sizeof(RDS_MESSAGE)); // Ensure all params are zeroed out
        msg.msgType = RDS_MSG_PING;

        TRACE("Sending RDS_MSG_PING.\n");
        if (!SendRDSMessage(&msg, NULL, 0))
        {
            ERR("KeepAliveThreadProc: SendRDSMessage failed for PING. Terminating keep-alive.\n");
            // SendRDSMessage itself handles closing rds_pipe on critical write failures.
            // We set bKeepAliveRun to FALSE to signal that the keep-alive mechanism is no longer active.
            bKeepAliveRun = FALSE; 
            break; 
        }

        // Sleep for the defined interval
        // Check bKeepAliveRun again after sleep, in case it was changed during the sleep period
        for (DWORD i = 0; i < KEEPALIVE_INTERVAL_MS / 100; ++i) {
            if (!bKeepAliveRun) break;
            Sleep(100);
        }
    }

    TRACE("KeepAliveThreadProc exiting.\n");
    _endthreadex(0); // Explicitly end the thread
    return 0; // Return value for _beginthreadex
}

BOOL StartRDSClientPipe(void)
{
    DWORD start_time = GetTickCount();
    DWORD elapsed;

    TRACE("Attempting to connect to named pipe: %S\n", RDS_PIPE_NAME);

    while (1)
    {
        if (WaitNamedPipeW(RDS_PIPE_NAME, RDS_PIPE_RETRY_INTERVAL_MS))
        {
            rds_pipe = CreateFileW(RDS_PIPE_NAME,
                                    GENERIC_READ | GENERIC_WRITE,
                                    0, NULL, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, NULL);

            if (rds_pipe != INVALID_HANDLE_VALUE)
            {
                TRACE("Successfully connected to WineRDS pipe.\n");
                
                bKeepAliveRun = TRUE;
                hKeepAliveThread = (HANDLE)_beginthreadex(NULL, 0, KeepAliveThreadProc, NULL, 0, NULL);

                if (hKeepAliveThread == NULL) // _beginthreadex returns NULL on failure
                {
                    ERR("Failed to create KeepAliveThread, error %lu\n", GetLastError());
                    bKeepAliveRun = FALSE;
                    CloseHandle(rds_pipe);
                    rds_pipe = INVALID_HANDLE_VALUE;
                    // No need to break here, the main loop's timeout or other conditions will eventually lead to exit.
                    // Or, we can return FALSE directly if this is considered a fatal error for pipe startup.
                    // For now, let the existing loop structure handle eventual timeout if pipe retries.
                    // However, given the context, it's better to return FALSE immediately.
                    return FALSE; // Explicitly return FALSE as keep-alive is essential.
                }
                else
                {
                    TRACE("KeepAliveThread started successfully.\n");
                    return TRUE; // Successfully connected and started keep-alive
                }
            }
            else
            {
                WARN("CreateFileW failed: %lu\n", GetLastError());
            }
        }
        else
        {
            DWORD err = GetLastError();
            if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PIPE_BUSY)
            {
                ERR("WaitNamedPipeW failed: %lu\n", err);
                break;
            }
        }

        elapsed = GetTickCount() - start_time;
        if (elapsed >= RDS_PIPE_TIMEOUT_MS)
        {
            ERR("Timed out waiting for RDS pipe to become available.\n");
            break;
        }

        Sleep(RDS_PIPE_RETRY_INTERVAL_MS);
    }

    return FALSE;
}

void StopRDSClientPipe(void)
{
    // Stop and clean up the keep-alive thread first
    if (hKeepAliveThread != NULL)
    {
        TRACE("Stopping KeepAliveThread...\n");
        bKeepAliveRun = FALSE; // Signal the thread to terminate

        // Wait for the thread to exit
        // It's important that KeepAliveThreadProc itself handles pipe errors
        // and doesn't try to use rds_pipe if it becomes invalid
        // due to external reasons before StopRDSClientPipe is called.
        // The check for rds_pipe != INVALID_HANDLE_VALUE in its loop helps.
        if (WaitForSingleObject(hKeepAliveThread, KEEPALIVE_INTERVAL_MS + 1000) == WAIT_TIMEOUT)
        {
            ERR("KeepAliveThread did not terminate in time!\n");
            // Consider TerminateThread as a last resort if needed, but it's generally unsafe.
            // For now, just log and continue with closing handles.
        } else {
            TRACE("KeepAliveThread stopped.\n");
        }
        
        CloseHandle(hKeepAliveThread);
        hKeepAliveThread = NULL;
    }

    if (rds_pipe != INVALID_HANDLE_VALUE)
    {
        TRACE("Disconnecting from RDS pipe.\n");
        CloseHandle(rds_pipe);
        rds_pipe = INVALID_HANDLE_VALUE;
    }
}

#if 0
BOOL SendRDSMessage(const RDS_MESSAGE *msg, const void *variable_data, DWORD variable_data_size)
{
    DWORD written;
    BOOL result;

    if (rds_pipe == INVALID_HANDLE_VALUE)
    {
        ERR("Pipe not connected.\n");
        return FALSE;
    }

    // Send header
    result = WriteFile(rds_pipe, msg, sizeof(*msg), &written, NULL);
    if (!result || written != sizeof(*msg)) {
        ERR("Failed to write message header (%lu bytes).\n", written);
        return FALSE;
    }

    // Send variable data if present
    if (variable_data && variable_data_size > 0) {
        result = WriteFile(rds_pipe, variable_data, variable_data_size, &written, NULL);
        if (!result || written != variable_data_size) {
            ERR("Failed to write message data (%lu bytes).\n", written);
            return FALSE;
        }
    }

    TRACE("Sent RDS message: type=%lu size=%lu\n", msg->type, msg->size);
    return TRUE;
}
#endif

BOOL SendRDSMessage(const RDS_MESSAGE *msg, const void *variable_data, DWORD variable_data_size)
{
    DWORD cbWritten;
    BOOL bSuccess = FALSE;

    if (!msg) return FALSE;

    if (rds_pipe == INVALID_HANDLE_VALUE) // Double check after acquiring lock
    {
        return FALSE;
    }

    // Send the fixed part of the message
    if (WriteFile(rds_pipe, msg, sizeof(RDS_MESSAGE), &cbWritten, NULL))
    {
        if (cbWritten == sizeof(RDS_MESSAGE))
        {
            bSuccess = TRUE;
            // Send variable data if present
            if (variable_data && variable_data_size > 0)
            {
                if (WriteFile(rds_pipe, variable_data, variable_data_size, &cbWritten, NULL))
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
        CloseHandle(rds_pipe);
        rds_pipe = INVALID_HANDLE_VALUE;
        FIXME("Pipe handle closed due to WriteFile failure.\n");
    }

    return bSuccess;
}

