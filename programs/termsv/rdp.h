/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Type Definitions
 *
 * Copyright 2009-2011 Jay Sorg
 * Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TERMSRV_TYPES_H
#define TERMSRV_TYPES_H

/* First, insure we ignore everything from WinPR that we don't need
 * and can cause conflicts with wine */

////////////////////////////////////
#define WINPR_H
#define WINPR_TIMEZONE_H
#define WINPR_WTYPES_H
#define WINPR_FILE_H
#define WINPR_SPEC_H
#define WINPR_CRT_STRING_H
#define WINPR_CRT_H
#define WINPR_COLLECTIONS_H
#define WINPR_NT_H
#define WINPR_WTSAPI_H
#define WINPR_SYNCH_H
#define WINPR_UTILS_STREAM_H
#define WINPR_API WINAPI
#define WINPR_SSPI_H
#define WINPR_SECURITY_H
#define WINPR_TIMEZONE_H
#define WINPR_THREAD_H
#define WINPR_WINDOWS_H
#define WINPR_INPUT_H
#define WINPR_WND_H
#define WINPR_LOG_H

#define INFINITE 0xFFFFFFFF

typedef struct rdp_rdp rdpRdp;
typedef struct rdp_gdi rdpGdi;
typedef struct rdp_rail rdpRail;
typedef struct rdp_cache rdpCache;
typedef struct rdp_channels rdpChannels;
typedef struct rdp_graphics rdpGraphics;
typedef struct rdp_metrics rdpMetrics;
typedef struct rdp_codecs rdpCodecs;

typedef struct rdp_freerdp freerdp;
typedef struct rdp_context rdpContext;
typedef struct rdp_freerdp_peer freerdp_peer;


        typedef void* (*OBJECT_NEW_FN)(void* val);
        typedef void (*OBJECT_INIT_FN)(void* obj);
        typedef void (*OBJECT_UNINIT_FN)(void* obj);
        typedef void (*OBJECT_FREE_FN)(void* obj);
        typedef BOOL (*OBJECT_EQUALS_FN)(const void* objA, const void* objB);

	//Collection.h

#if 0
        struct _wObject
        {
                OBJECT_NEW_FN fnObjectNew;
                OBJECT_INIT_FN fnObjectInit;
                OBJECT_UNINIT_FN fnObjectUninit;
                OBJECT_FREE_FN fnObjectFree;
                OBJECT_EQUALS_FN fnObjectEquals;
        };
        typedef struct _wObject wObject;
#endif
	struct _wLogMessage
        {
                DWORD Type;

                DWORD Level;

                LPSTR PrefixString;

                LPCSTR FormatString;
                LPSTR TextString;

                DWORD LineNumber;    /* __LINE__ */
                LPCSTR FileName;     /* __FILE__ */
                LPCSTR FunctionName; /* __FUNCTION__ */

                /* Data Message */

                void* Data;
                int Length;

                /* Image Message */

                void* ImageData;
                int ImageWidth;
                int ImageHeight;
                int ImageBpp;

                /* Packet Message */

                void* PacketData;
                int PacketLength;
                DWORD PacketFlags;
        };
        typedef struct _wLogMessage wLogMessage;
        typedef struct _wLogLayout wLogLayout;
        typedef struct _wLogAppender wLogAppender;
        typedef struct _wLog wLog;

#if 0
        /* Message Queue */

        typedef struct _wMessage wMessage;

        typedef void (*MESSAGE_FREE_FN)(wMessage* message);

        struct _wMessage
        {
                UINT32 id;
                void* context;
                void* wParam;
                void* lParam;
                UINT64 time;
                MESSAGE_FREE_FN Free;
        };
#endif

/* winbase.h */
#define WAIT_OBJECT_0           0
/* winnt.h */
typedef RTL_CRITICAL_SECTION CRITICAL_SECTION;


// wtsapi

#define CHANNEL_NAME_LEN 7

typedef struct tagCHANNEL_DEF
{
        char name[CHANNEL_NAME_LEN + 1];
        ULONG options;
} CHANNEL_DEF;
typedef CHANNEL_DEF* PCHANNEL_DEF;
typedef PCHANNEL_DEF* PPCHANNEL_DEF;



#include "wtsapi.h"

typedef VIRTUALCHANNELENTRYEX* PVIRTUALCHANNELENTRYEX;


        typedef struct _wStreamPool wStreamPool;

        struct _wStream
        {
                BYTE* buffer;
                BYTE* pointer;
                size_t length;
                size_t capacity;

                DWORD count;
                wStreamPool* pool;
                BOOL isAllocatedStream;
                BOOL isOwner;
        };
        typedef struct _wStream wStream;


#define INLINE inline
#define FREERDP_EXPORTS
#define WINPR_DEPRECATED(obj) obj
//#define FREERDP_API __attribute__((visibility("default")))
#undef _WIN32
//#define __TERMSRV_LISTNER_C
//static FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN freerdp_load_static_channel_addin_entry = NULL;
//typedef FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN void;
#if 0
	typedef struct _SYSTEMTIME
{
        WORD wYear;
        WORD wMonth;
        WORD wDayOfWeek;
        WORD wDay;
        WORD wHour;
        WORD wMinute;
        WORD wSecond;
        WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;


        typedef struct _TIME_ZONE_INFORMATION
        {
                LONG Bias;
                WCHAR StandardName[32];
                SYSTEMTIME StandardDate;
                LONG StandardBias;
                WCHAR DaylightName[32];
                SYSTEMTIME DaylightDate;
                LONG DaylightBias;
        } TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

#define FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN WINAPI
//static FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN freerdp_load_static_channel_addin_entry = NULL;

#typedef 
FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN provider;

int freerdp_register_addin_provider(FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN provider, DWORD dwFlags)
{
//	freerdp_load_static_channel_addin_entry = provider;
	return 0;
}

FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN freerdp_get_current_addin_provider(void)
{
	return freerdp_load_static_channel_addin_entry;
}
#endif

#define WLog_ERR sprintf
#if 0
#include </usr/include/freerdp2/freerdp/settings.h>
#define FREERDP_EVENT_H
#include </usr/include/freerdp2/freerdp/svc.h>
#include </usr/include/freerdp2/freerdp/client/rdpgfx.h>
#include </usr/include/freerdp2/freerdp/server/rdpgfx.h>
#endif

typedef struct _SEC_WINNT_AUTH_IDENTITY_W
{
    unsigned short* User;
    ULONG UserLength;
    unsigned short* Domain;
    ULONG DomainLength;
    unsigned short* Password;
    ULONG PasswordLength;
    ULONG Flags;
} SEC_WINNT_AUTH_IDENTITY, *PSEC_WINNT_AUTH_IDENTITY;

#if 0
#include </usr/include/freerdp2/freerdp/listener.h>
//#include <freerdp/listener.h>

typedef long rdpServer;
//typedef long rdpContext;
rdpServer *server = NULL;

typedef struct
{
    rdpServer* server;
    HANDLE stopEvent;
} ServerContext;
#endif 

#include "collections.h"

#include </usr/include/freerdp2/freerdp/listener.h>
//#include <freerdp/listener.h>

typedef long rdpServer;
//typedef long rdpContext;
rdpServer *server = NULL;

typedef struct
{
    rdpServer* server;
    HANDLE stopEvent;
} ServerContext;

///////////
#if 0
/* WinPR Collection.h */
        struct _wMessageQueue
        {
                int head;
                int tail;
                int size;
                int capacity;
                wMessage* array;
                CRITICAL_SECTION lock;
                HANDLE event;

                wObject object;
        };
        typedef struct _wMessageQueue wMessageQueue;

        /* System.Collections.Specialized.ListDictionary */

        typedef struct _wListDictionaryItem wListDictionaryItem;

        struct _wListDictionaryItem
        {
                void* key;
                void* value;

                wListDictionaryItem* next;
        };

        struct _wListDictionary
        {
                BOOL synchronized;
                CRITICAL_SECTION lock;

                wListDictionaryItem* head;
                wObject objectKey;
                wObject objectValue;
        };
        typedef struct _wListDictionary wListDictionary;

        /* Publisher/Subscriber Pattern */

        struct _wEventArgs
        {
                DWORD Size;
                const char* Sender;
        };
        typedef struct _wEventArgs wEventArgs;

        typedef void (*pEventHandler)(void* context, wEventArgs* e);

#define MAX_EVENT_HANDLERS 32

        struct _wEventType
        {
                const char* EventName;
                wEventArgs EventArgs;
                int EventHandlerCount;
                pEventHandler EventHandlers[MAX_EVENT_HANDLERS];
        };
        typedef struct _wEventType wEventType;


        struct _wPubSub
        {
                CRITICAL_SECTION lock;
                BOOL synchronized;

                int size;
                int count;
                wEventType* events;
        };
        typedef struct _wPubSub wPubSub;
#endif

/* Not Public */
// freerdp/include/types.h
// Should contain this...


        typedef enum
        {
                CONNECTION_STATE_INITIAL,
                CONNECTION_STATE_NEGO,
                CONNECTION_STATE_NLA,
                CONNECTION_STATE_AAD,
                CONNECTION_STATE_MCS_CREATE_REQUEST,
                CONNECTION_STATE_MCS_CREATE_RESPONSE,
                CONNECTION_STATE_MCS_ERECT_DOMAIN,
                CONNECTION_STATE_MCS_ATTACH_USER,
                CONNECTION_STATE_MCS_ATTACH_USER_CONFIRM,
                CONNECTION_STATE_MCS_CHANNEL_JOIN_REQUEST,
                CONNECTION_STATE_MCS_CHANNEL_JOIN_RESPONSE,
                CONNECTION_STATE_RDP_SECURITY_COMMENCEMENT,
                CONNECTION_STATE_SECURE_SETTINGS_EXCHANGE,
                CONNECTION_STATE_CONNECT_TIME_AUTO_DETECT_REQUEST,
                CONNECTION_STATE_CONNECT_TIME_AUTO_DETECT_RESPONSE,
                CONNECTION_STATE_LICENSING,
                CONNECTION_STATE_MULTITRANSPORT_BOOTSTRAPPING_REQUEST,
                CONNECTION_STATE_MULTITRANSPORT_BOOTSTRAPPING_RESPONSE,
                CONNECTION_STATE_CAPABILITIES_EXCHANGE_DEMAND_ACTIVE,
                CONNECTION_STATE_CAPABILITIES_EXCHANGE_MONITOR_LAYOUT,
                CONNECTION_STATE_CAPABILITIES_EXCHANGE_CONFIRM_ACTIVE,
                CONNECTION_STATE_FINALIZATION_SYNC,
                CONNECTION_STATE_FINALIZATION_COOPERATE,
                CONNECTION_STATE_FINALIZATION_REQUEST_CONTROL,
                CONNECTION_STATE_FINALIZATION_PERSISTENT_KEY_LIST,
                CONNECTION_STATE_FINALIZATION_FONT_LIST,
                CONNECTION_STATE_FINALIZATION_CLIENT_SYNC,
                CONNECTION_STATE_FINALIZATION_CLIENT_COOPERATE,
                CONNECTION_STATE_FINALIZATION_CLIENT_GRANTED_CONTROL,
                CONNECTION_STATE_FINALIZATION_CLIENT_FONT_MAP,
                CONNECTION_STATE_ACTIVE
        } CONNECTION_STATE;

        typedef struct rdp_channels rdpChannels;
        typedef struct rdp_freerdp freerdp;
        typedef struct rdp_context rdpContext;
        typedef struct rdp_freerdp_peer freerdp_peer;
        typedef struct rdp_transport rdpTransport; /* Opaque */

	// freerdp crypto
        typedef struct rdp_private_key rdpPrivateKey;

#include <transport_io.h>

// freerdp/include/types.h
// Should contain this...

        typedef struct stream_dump_context rdpStreamDumpContext;

        typedef enum
        {
                STREAM_MSG_SRV_RX = 1,
                STREAM_MSG_SRV_TX = 2
        } StreamDumpDirection;

#define KBDEXT 0x0100u
#define WINPR_SSL_INIT_DEFAULT 0x00
#define KNOWN_PATH_TEMP 2

#endif /* TERMSERV_TYPES_H */
