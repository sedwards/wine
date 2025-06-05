/* pipe_client.c - Connect to WineRDS named pipe from driver side - WITH RETRY LOGIC */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <wine/debug.h>

#include "rds_message.h"

WINE_DEFAULT_DEBUG_CHANNEL(winerds);

#define RDS_PIPE_NAME L"\\\\.\\pipe\\WineRDS"
#define RDS_PIPE_TIMEOUT_MS 5000
#define RDS_PIPE_RETRY_INTERVAL_MS 250
#define KEEPALIVE_INTERVAL_MS 5000

static HANDLE rds_pipe = INVALID_HANDLE_VALUE;
static HANDLE hKeepAliveThread = NULL;
static volatile BOOL bKeepAliveRun = FALSE;
static BOOL connection_attempted = FALSE;  // Track if we've tried to connect

static unsigned __stdcall KeepAliveThreadProc(void *arg)
{
    RDS_MESSAGE msg;
    TRACE("KeepAliveThreadProc started.\n");

    while (bKeepAliveRun && rds_pipe != INVALID_HANDLE_VALUE)
    {
        // Construct and send PING message
        ZeroMemory(&msg, sizeof(RDS_MESSAGE));
        msg.msgType = RDS_MSG_PING;

        TRACE("Sending RDS_MSG_PING.\n");
        if (!SendRDSMessage(&msg, NULL, 0))
        {
            ERR("KeepAliveThreadProc: SendRDSMessage failed for PING. Terminating keep-alive.\n");
            bKeepAliveRun = FALSE; 
            break; 
        }

        // Sleep for the defined interval
        for (DWORD i = 0; i < KEEPALIVE_INTERVAL_MS / 100; ++i) {
            if (!bKeepAliveRun) break;
            Sleep(100);
        }
    }

    TRACE("KeepAliveThreadProc exiting.\n");
    _endthreadex(0);
    return 0;
}

// Internal function to attempt connection without timeout
static BOOL TryConnect(void)
{
    TRACE("TryConnect: Attempting to connect to named pipe: %S\n", RDS_PIPE_NAME);
    
    if (!WaitNamedPipeW(RDS_PIPE_NAME, 100)) // Very short wait
    {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PIPE_BUSY)
        {
            TRACE("Pipe not available (error %lu)\n", err);
        }
        else
        {
            WARN("WaitNamedPipeW failed: %lu\n", err);
        }
        return FALSE;
    }

    rds_pipe = CreateFileW(RDS_PIPE_NAME,
                          GENERIC_READ | GENERIC_WRITE,
                          0, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);

    if (rds_pipe == INVALID_HANDLE_VALUE)
    {
        WARN("CreateFileW failed: %lu\n", GetLastError());
        return FALSE;
    }

    TRACE("Successfully connected to WineRDS pipe.\n");
    
    // Start keep-alive thread
    bKeepAliveRun = TRUE;
    hKeepAliveThread = (HANDLE)_beginthreadex(NULL, 0, KeepAliveThreadProc, NULL, 0, NULL);

    if (hKeepAliveThread == NULL)
    {
        ERR("Failed to create KeepAliveThread, error %lu\n", GetLastError());
        CloseHandle(rds_pipe);
        rds_pipe = INVALID_HANDLE_VALUE;
        return FALSE;
    }

    TRACE("KeepAliveThread started successfully.\n");
    return TRUE;
}

BOOL StartRDSClientPipe(void)
{
    DWORD start_time = GetTickCount();
    DWORD elapsed;

    connection_attempted = TRUE;
    
    TRACE("Attempting to connect to named pipe: %S\n", RDS_PIPE_NAME);

    while (1)
    {
        if (TryConnect())
        {
            return TRUE;
        }

        elapsed = GetTickCount() - start_time;
        if (elapsed >= RDS_PIPE_TIMEOUT_MS)
        {
            WARN("Timed out waiting for RDS pipe to become available.\n");
            break;
        }

        Sleep(RDS_PIPE_RETRY_INTERVAL_MS);
    }

    return FALSE;
}

// NEW FUNCTION: Try to establish connection if not already connected
static BOOL EnsureConnection(void)
{
    // If already connected, we're good
    if (rds_pipe != INVALID_HANDLE_VALUE)
        return TRUE;

    // If we've never tried to connect, try once
    if (!connection_attempted)
    {
        TRACE("First connection attempt...\n");
        return TryConnect();
    }

    // For subsequent attempts, try a quick connection (no long timeout)
    TRACE("Retrying connection...\n");
    return TryConnect();
}

void StopRDSClientPipe(void)
{
    // Stop and clean up the keep-alive thread first
    if (hKeepAliveThread != NULL)
    {
        TRACE("Stopping KeepAliveThread...\n");
        bKeepAliveRun = FALSE;

        if (WaitForSingleObject(hKeepAliveThread, KEEPALIVE_INTERVAL_MS + 1000) == WAIT_TIMEOUT)
        {
            ERR("KeepAliveThread did not terminate in time!\n");
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

BOOL SendRDSMessage(const RDS_MESSAGE *msg, const void *variable_data, DWORD variable_data_size)
{
    DWORD cbWritten;
    BOOL bSuccess = FALSE;

    if (!msg) return FALSE;

    // Try to ensure we have a connection
    if (!EnsureConnection())
    {
        TRACE("No pipe connection available\n");
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
                        TRACE("Successfully sent message type %d with %lu bytes of variable data.\n", msg->msgType, variable_data_size);
                    }
                    else
                    {
                        WARN("WriteFile for variable data wrote %lu of %lu bytes.\n", cbWritten, variable_data_size);
                        bSuccess = FALSE;
                    }
                }
                else
                {
                    WARN("WriteFile for variable data failed, GLE=%ld.\n", GetLastError());
                    bSuccess = FALSE;
                }
            }
             else {
                TRACE("Successfully sent message type %d.\n", msg->msgType);
             }
        }
        else
        {
            WARN("WriteFile for RDS_MESSAGE wrote %lu of %u bytes.\n", cbWritten, (unsigned int)sizeof(RDS_MESSAGE));
        }
    }
    else
    {
        WARN("WriteFile for RDS_MESSAGE failed, GLE=%ld.\n", GetLastError());
        // Connection failed - close and mark as invalid so we'll retry next time
        CloseHandle(rds_pipe);
        rds_pipe = INVALID_HANDLE_VALUE;
        bKeepAliveRun = FALSE; // Stop keep-alive thread
        TRACE("Pipe handle closed due to WriteFile failure - will retry on next message\n");
    }

    return bSuccess;
}

