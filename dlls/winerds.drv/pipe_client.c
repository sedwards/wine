/* pipe_client.c - Connect to WineRDS named pipe from driver side */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wine/debug.h>

#include "rds_message.h"

WINE_DEFAULT_DEBUG_CHANNEL(rds);

#define RDS_PIPE_NAME L"\\\\.\\pipe\\WineRDS"
#define RDS_PIPE_TIMEOUT_MS 5000
#define RDS_PIPE_RETRY_INTERVAL_MS 250

static HANDLE rds_pipe = INVALID_HANDLE_VALUE;

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
                return TRUE;
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

    
