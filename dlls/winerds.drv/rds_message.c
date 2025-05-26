// Driver message processing loop
DWORD WINAPI RDS_DriverMessageThread(LPVOID lpParam)
{
    HANDLE hPipe = (HANDLE)lpParam;
    BOOL connected = TRUE;
    RDS_MESSAGE msg;
    DWORD bytesRead;
    
    while (connected)
    {
        // Wait for a message from termsrv
        if (!ReadFile(hPipe, &msg, sizeof(RDS_MESSAGE), &bytesRead, NULL))
        {
            DWORD error = GetLastError();
            if (error == ERROR_BROKEN_PIPE)
            {
                WINE_TRACE("Pipe connection closed\n");
                connected = FALSE;
            }
            else
            {
                WINE_ERR("ReadFile failed with error %u\n", error);
                // Maybe try to reconnect here
                Sleep(1000);
            }
            continue;
        }
        
        // Process the message based on its type
        switch (msg.msgType)
        {
            case RDS_MSG_CREATE_SURFACE:
                {
                    PRDS_SURFACE surface = RDS_CreateSurface(
                        msg.params.createSurface.width,
                        msg.params.createSurface.height,
                        msg.params.createSurface.bpp);
                    
                    // Send response with surface ID
                    RDS_MESSAGE response;
                    response.msgType = RDS_MSG_SURFACE_CREATED;
                    response.params.surfaceCreated.surfaceId = (DWORD_PTR)surface;
                    WriteFile(hPipe, &response, sizeof(RDS_MESSAGE), NULL, NULL);
                }
                break;
                
            case RDS_MSG_PAINT_SURFACE:
                {
                    PRDS_SURFACE surface = (PRDS_SURFACE)msg.params.paintSurface.surfaceId;
                    // Paint operations using GDI on the surface's DC
                    // ...
                    
                    // Mark surface as dirty
                    RDS_MarkDirty(surface, &msg.params.paintSurface.updateRect);
                    
                    // Signal that painting is complete
                    RDS_MESSAGE response;
                    response.msgType = RDS_MSG_PAINT_COMPLETE;
                    WriteFile(hPipe, &response, sizeof(RDS_MESSAGE), NULL, NULL);
                }
                break;
                
            // Handle other message types
        }
    }
    
    return 0;
}

