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
#define KEEPALIVE_INTERVAL_MS 2000  // Reduced from 5000 for better diagnostics

static HANDLE rds_pipe = INVALID_HANDLE_VALUE;
static HANDLE hKeepAliveThread = NULL;
static volatile BOOL bKeepAliveRun = FALSE;
static BOOL connection_attempted = FALSE;  // Track if we've tried to connect

static unsigned __stdcall KeepAliveThreadProc(void *arg)
{
    RDS_MESSAGE msg;
    RDS_MESSAGE response;
    DWORD cbRead;
    DWORD ping_count = 0;
    DWORD failed_pings = 0;
    
    TRACE("KeepAliveThreadProc started.\n");

    while (bKeepAliveRun && rds_pipe != INVALID_HANDLE_VALUE)
    {
        // Construct and send PING message
        ZeroMemory(&msg, sizeof(RDS_MESSAGE));
        msg.msgType = RDS_MSG_PING;

        ping_count++;
        TRACE("[PING #%lu] Sending RDS_MSG_PING at %lu ms\n", ping_count, GetTickCount());
        
        if (!SendRDSMessage(&msg, NULL, 0))
        {
            failed_pings++;
            ERR("[PING #%lu] SendRDSMessage failed for PING. Failed pings: %lu. Terminating keep-alive.\n", 
                ping_count, failed_pings);
            bKeepAliveRun = FALSE; 
            break; 
        }
        
        // Try to read PONG response with timeout
        DWORD start_wait = GetTickCount();
        BOOL got_pong = FALSE;
        
        // Set pipe to non-blocking mode temporarily
        DWORD mode = PIPE_READMODE_MESSAGE | PIPE_NOWAIT;
        SetNamedPipeHandleState(rds_pipe, &mode, NULL, NULL);
        
        // Wait up to 500ms for PONG
        while (GetTickCount() - start_wait < 500)
        {
            if (ReadFile(rds_pipe, &response, sizeof(RDS_MESSAGE), &cbRead, NULL))
            {
                if (cbRead >= sizeof(RDS_MESSAGE_TYPE) && response.msgType == RDS_MSG_PONG)
                {
                    TRACE("[PING #%lu] Received PONG after %lu ms\n", 
                          ping_count, GetTickCount() - start_wait);
                    got_pong = TRUE;
                    break;
                }
                else
                {
                    WARN("[PING #%lu] Received unexpected message type %d while waiting for PONG\n",
                         ping_count, response.msgType);
                }
            }
            else
            {
                DWORD err = GetLastError();
                if (err != ERROR_NO_DATA) // ERROR_NO_DATA is expected when no data available
                {
                    if (err == ERROR_BROKEN_PIPE)
                    {
                        ERR("[PING #%lu] Pipe broken while waiting for PONG\n", ping_count);
                        bKeepAliveRun = FALSE;
                        break;
                    }
                }
            }
            Sleep(50);
        }
        
        // Restore blocking mode
        mode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
        SetNamedPipeHandleState(rds_pipe, &mode, NULL, NULL);
        
        if (!got_pong)
        {
            failed_pings++;
            WARN("[PING #%lu] No PONG received within timeout. Failed pings: %lu\n", 
                 ping_count, failed_pings);
            
            // If we fail too many pings, assume connection is dead
            if (failed_pings >= 3)
            {
                ERR("Too many failed pings (%lu). Assuming connection dead.\n", failed_pings);
                bKeepAliveRun = FALSE;
                break;
            }
        }
        else
        {
            failed_pings = 0; // Reset on successful ping/pong
        }

        // Sleep for the defined interval
        for (DWORD i = 0; i < KEEPALIVE_INTERVAL_MS / 100; ++i) {
            if (!bKeepAliveRun) break;
            Sleep(100);
        }
    }

    TRACE("KeepAliveThreadProc exiting. Total pings sent: %lu, Failed: %lu\n", 
          ping_count, failed_pings);
    _endthreadex(0);
    return 0;
}

// Internal function to attempt connection without timeout
static BOOL TryConnect(void)
{
    TRACE("TryConnect: Attempting to connect to named pipe: %S at %lu ms\n", RDS_PIPE_NAME, GetTickCount());
    
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

    TRACE("Successfully connected to WineRDS pipe at %lu ms\n", GetTickCount());
    
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
    DWORD retry_count = 0;

    connection_attempted = TRUE;
    
    TRACE("StartRDSClientPipe: Beginning connection attempts to %S at %lu ms\n", RDS_PIPE_NAME, start_time);

    while (1)
    {
        retry_count++;
        TRACE("Connection attempt #%lu at %lu ms (elapsed: %lu ms)\n", 
              retry_count, GetTickCount(), GetTickCount() - start_time);
              
        if (TryConnect())
        {
            TRACE("Connection successful after %lu attempts, %lu ms total\n", 
                  retry_count, GetTickCount() - start_time);
            return TRUE;
        }

        elapsed = GetTickCount() - start_time;
        if (elapsed >= RDS_PIPE_TIMEOUT_MS)
        {
            WARN("Timed out after %lu attempts over %lu ms. Pipe not available.\n", 
                 retry_count, elapsed);
            break;
        }

        TRACE("Retry in %lu ms...\n", (DWORD)RDS_PIPE_RETRY_INTERVAL_MS);
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
                        TRACE("[MSG] Sent type %d (%s) with %lu bytes data at %lu ms\n", 
                              msg->msgType,
                              msg->msgType == RDS_MSG_PING ? "PING" :
                              msg->msgType == RDS_MSG_LINE_TO ? "LINE_TO" :
                              msg->msgType == RDS_MSG_RECTANGLE ? "RECTANGLE" :
                              msg->msgType == RDS_MSG_TEXT_OUT ? "TEXT_OUT" : "OTHER",
                              variable_data_size, GetTickCount());
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
                TRACE("[MSG] Sent type %d (%s) at %lu ms\n", 
                      msg->msgType,
                      msg->msgType == RDS_MSG_PING ? "PING" :
                      msg->msgType == RDS_MSG_LINE_TO ? "LINE_TO" :
                      msg->msgType == RDS_MSG_RECTANGLE ? "RECTANGLE" :
                      msg->msgType == RDS_MSG_TEXT_OUT ? "TEXT_OUT" : "OTHER",
                      GetTickCount());
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

