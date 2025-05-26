#ifndef PIPE_CLIENT_H
#define PIPE_CLIENT_H

#include <windows.h>
#include "rds_message.h" // Assuming rds_message.h is in the same directory or accessible

BOOL StartRDSClientPipe(void);
void StopRDSClientPipe(void);
BOOL SendRDSMessage(const RDS_MESSAGE *msg, const void *variable_data, DWORD variable_data_size);

#endif // PIPE_CLIENT_H
