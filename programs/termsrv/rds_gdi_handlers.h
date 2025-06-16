#ifndef RDS_GDI_HANDLERS_H
#define RDS_GDI_HANDLERS_H

#include "../../dlls/winerds.drv/rds_message.h" // Adjust path if placeholder was used by worker previously

// Handler function prototypes
void Handle_RDS_MSG_MOVE_TO(const RDS_MESSAGE *msg);
void Handle_RDS_MSG_LINE_TO(const RDS_MESSAGE *msg);
void Handle_RDS_MSG_RECTANGLE(const RDS_MESSAGE *msg);
void Handle_RDS_MSG_TEXT_OUT(const RDS_MESSAGE *msg, const WCHAR *text_data); 
void Handle_RDS_MSG_PING(const RDS_MESSAGE *msg, HANDLE hPipe);

#endif // RDS_GDI_HANDLERS_H
