/*
 * Mac clipboard driver
 *
 * Copyright 1994 Martin Ayotte
 *           1996 Alex Korobka
 *           1999 Noel Borthwick
 *           2003 Ulrich Czekalla for CodeWeavers
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <dlfcn.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "basetsd.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "wine/list.h"

#include "broadwaydrv.h"

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "broadwaydrv.h"
#include "winuser.h"
#include "shellapi.h"
#include "shlobj.h"


WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

/* window */
struct broadwaydrv_window_features {
    unsigned int    wf;
};

/**************************************************************************
 *              Types
 **************************************************************************/

typedef void *(*DRVIMPORTFUNC)(void data, size_t *ret_size);
typedef void (*DRVEXPORTFUNC)(void *data, size_t size);

typedef struct _WINE_CLIPFORMAT
{
    struct list             entry;
    UINT                    format_id;
    LPSTR                    type;
    DRVIMPORTFUNC           import_func;
    DRVEXPORTFUNC           export_func;
    BOOL                    synthesized;
    struct _WINE_CLIPFORMAT *natural_format;
} WINE_CLIPFORMAT;


/**************************************************************************
 *              Constants
 **************************************************************************/

#define CLIPBOARD_UPDATE_DELAY 2000   /* delay between checks of the Mac pasteboard */


/**************************************************************************
 *              Forward Function Declarations
 **************************************************************************/

/**************************************************************************
 *              Static Variables
 **************************************************************************/

static const WCHAR clipboard_classname[] =
    {'_','_','w','i','n','e','_','c','l','i','p','b','o','a','r','d','_','m','a','n','a','g','e','r',0};

/* Clipboard formats */
static struct list format_list = LIST_INIT(format_list);

/*  There are two naming schemes involved and we want to have a mapping between
    them.  There are Win32 clipboard format names and there are Mac pasteboard
    types.

    The Win32 standard clipboard formats don't have names, but they are associated
    with Mac pasteboard types through the following tables, which are used to
    initialize the format_list.  Where possible, the standard clipboard formats
    are mapped to predefined pasteboard type UTIs.  Otherwise, we create Wine-
    specific types of the form "org.winehq.builtin.<format>", where <format> is
    the name of the symbolic constant for the format minus "CF_" and lowercased.
    E.g. CF_BITMAP -> org.winehq.builtin.bitmap.

    Win32 clipboard formats which originate in a Windows program may be registered
    with an arbitrary name.  We construct a Mac pasteboard type from these by
    prepending "org.winehq.registered." to the registered name.

    Likewise, Mac pasteboard types which originate in other apps may have
    arbitrary type strings.  We ignore these.

    Summary:
    Win32 clipboard format names:
        <none>                              standard clipboard format; maps via
                                            format_list to either a predefined Mac UTI
                                            or org.winehq.builtin.<format>.
        <other>                             name registered within Win32 land; maps to
                                            org.winehq.registered.<other>
    Mac pasteboard type names:
        org.winehq.builtin.<format ID>      representation of Win32 standard clipboard
                                            format for which there was no corresponding
                                            predefined Mac UTI; maps via format_list
        org.winehq.registered.<format name> representation of Win32 registered
                                            clipboard format name; maps to <format name>
        <other>                             Mac pasteboard type originating with system
                                            or other apps; either maps via format_list
                                            to a standard clipboard format or ignored
*/

static const struct
{
    UINT          id;
    LPSTR          type;
    DRVIMPORTFUNC import;
    DRVEXPORTFUNC export;
    BOOL          synthesized;
} builtin_format_ids[] =
{
};

static const WCHAR wszRichTextFormat[] = {'R','i','c','h',' ','T','e','x','t',' ','F','o','r','m','a','t',0};
static const WCHAR wszGIF[] = {'G','I','F',0};
static const WCHAR wszJFIF[] = {'J','F','I','F',0};
static const WCHAR wszPNG[] = {'P','N','G',0};
static const WCHAR wszHTMLFormat[] = {'H','T','M','L',' ','F','o','r','m','a','t',0};
static const struct
{
    LPCWSTR       name;
    LPSTR         type;
    DRVIMPORTFUNC import;
    DRVEXPORTFUNC export;
    BOOL          synthesized;
} builtin_format_names[] =
{
};

/* The prefix prepended to a Win32 clipboard format name to make a Mac pasteboard type. */
static const LPSTR registered_name_type_prefix = "org.winehq.registered.";

static unsigned int clipboard_thread_id;
static HWND clipboard_hwnd;
static BOOL is_clipboard_owner;
static unsigned int last_clipboard_update;
static unsigned int last_get_seqno;
static WINE_CLIPFORMAT **current_mac_formats;
static unsigned int nb_current_mac_formats;


/**************************************************************************
 *              Internal Clipboard implementation methods
 **************************************************************************/

/*
 * format_list functions
 */

/**************************************************************************
 *              debugstr_format
 */
static const char *debugstr_format(UINT id)
{
    WCHAR buffer[256];

    if (NtUserGetClipboardFormatName(id, buffer, 256))
        return wine_dbg_sprintf("0x%04x %s", id, debugstr_w(buffer));

    switch (id)
    {
#define BUILTIN(id) case id: return #id;
    BUILTIN(CF_TEXT)
    BUILTIN(CF_BITMAP)
    BUILTIN(CF_METAFILEPICT)
    BUILTIN(CF_SYLK)
    BUILTIN(CF_DIF)
    BUILTIN(CF_TIFF)
    BUILTIN(CF_OEMTEXT)
    BUILTIN(CF_DIB)
    BUILTIN(CF_PALETTE)
    BUILTIN(CF_PENDATA)
    BUILTIN(CF_RIFF)
    BUILTIN(CF_WAVE)
    BUILTIN(CF_UNICODETEXT)
    BUILTIN(CF_ENHMETAFILE)
    BUILTIN(CF_HDROP)
    BUILTIN(CF_LOCALE)
    BUILTIN(CF_DIBV5)
    BUILTIN(CF_OWNERDISPLAY)
    BUILTIN(CF_DSPTEXT)
    BUILTIN(CF_DSPBITMAP)
    BUILTIN(CF_DSPMETAFILEPICT)
    BUILTIN(CF_DSPENHMETAFILE)
#undef BUILTIN
    default: return wine_dbg_sprintf("0x%04x", id);
    }
}


/**************************************************************************
 *              insert_clipboard_format
 */
static WINE_CLIPFORMAT *insert_clipboard_format(UINT id, LPSTR type)
{
    WINE_CLIPFORMAT *format;

    format = malloc(sizeof(*format));

    if (format == NULL)
    {
        WARN("No more memory for a new format!\n");
        return NULL;
    }
    format->format_id = id;
    //format->import_func = import_clipboard_data;
    //format->export_func = export_clipboard_data;
    format->synthesized = FALSE;
    format->natural_format = NULL;

    if (type)
        format->type = CFStringCreateCopy(NULL, type);
    else
    {
        WCHAR buffer[256];

        if (!NtUserGetClipboardFormatName(format->format_id, buffer, ARRAY_SIZE(buffer)))
        {
            WARN("failed to get name for format %s; error 0x%08x\n", debugstr_format(format->format_id),
                 (unsigned int)RtlGetLastWin32Error());
            free(format);
            return NULL;
        }

        format->type = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@%S"),
                                                registered_name_type_prefix, (const WCHAR*)buffer);
    }

    list_add_tail(&format_list, &format->entry);

    TRACE("Registering format %s type %s\n", debugstr_format(format->format_id),
          debugstr_cf(format->type));

    return format;
}


/**************************************************************************
 *              register_format
 *
 * Register a custom Mac clipboard format.
 */
static WINE_CLIPFORMAT* register_format(UINT id, LPSTR type)
{
    WINE_CLIPFORMAT *format;

    /* walk format chain to see if it's already registered */
    LIST_FOR_EACH_ENTRY(format, &format_list, WINE_CLIPFORMAT, entry)
        if (format->format_id == id) return format;

    return insert_clipboard_format(id, type);
}


/**************************************************************************
 *              natural_format_for_format
 *
 * Find the "natural" format for this format_id (the one which isn't
 * synthesized from another type).
 */
static WINE_CLIPFORMAT* natural_format_for_format(UINT format_id)
{
    WINE_CLIPFORMAT *format;
        format = NULL;
    return format;
}


static ATOM register_clipboard_format(const WCHAR *name)
{
    ATOM atom;
    if (NtAddAtom(name, lstrlenW(name) * sizeof(WCHAR), &atom)) return 0;
    return atom;
}


/**************************************************************************
 *              register_builtin_formats
 */
static void register_builtin_formats(void)
{
    UINT i;
    WINE_CLIPFORMAT *format;

    /* Register built-in formats */
    for (i = 0; i < ARRAY_SIZE(builtin_format_ids); i++)
    {
        if (!(format = malloc(sizeof(*format)))) break;
        format->format_id       = builtin_format_ids[i].id;
        format->type            = CFRetain(builtin_format_ids[i].type);
        format->import_func     = builtin_format_ids[i].import;
        format->export_func     = builtin_format_ids[i].export;
        format->synthesized     = builtin_format_ids[i].synthesized;
        format->natural_format  = NULL;
        list_add_tail(&format_list, &format->entry);
    }

    /* Register known mappings between Windows formats and Mac types */
    for (i = 0; i < ARRAY_SIZE(builtin_format_names); i++)
    {
        if (!(format = malloc(sizeof(*format)))) break;
        format->format_id       = register_clipboard_format(builtin_format_names[i].name);
        format->import_func     = builtin_format_names[i].import;
        format->export_func     = builtin_format_names[i].export;
        format->synthesized     = builtin_format_names[i].synthesized;
        format->natural_format  = NULL;

        if (builtin_format_names[i].type)
            format->type = CFRetain(builtin_format_names[i].type);
        else
        {
            format->type = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@%S"),
                                                    registered_name_type_prefix, builtin_format_names[i].name);
        }

        list_add_tail(&format_list, &format->entry);
    }

    LIST_FOR_EACH_ENTRY(format, &format_list, WINE_CLIPFORMAT, entry)
    {
        if (format->synthesized)
            format->natural_format = natural_format_for_format(format->format_id);
    }
}


/**************************************************************************
 *              format_for_type
 */
static WINE_CLIPFORMAT* format_for_type(LPSTR type)
{
    WINE_CLIPFORMAT *format;

    TRACE("type %s\n", debugstr_cf(type));

    if (list_empty(&format_list)) 
	register_builtin_formats();

    TRACE(" -> %p/%s\n", format, debugstr_format(format ? format->format_id : 0));
    return format;
}


/***********************************************************************
 *              bitmap_info_size
 *
 * Return the size of the bitmap info structure including color table.
 */
static int bitmap_info_size(const BITMAPINFO *info, WORD coloruse)
{
    unsigned int colors, size, masks = 0;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER*)info;
        colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors *
             ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        colors = min(info->bmiHeader.biClrUsed, 256);
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max(info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD));
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}


/**************************************************************************
 *		get_html_description_field
 *
 *  Find the value of a field in an HTML Format description.
 */
static const char* get_html_description_field(const char* data, const char* keyword)
{
    const char* pos = data;

    while (pos && *pos && *pos != '<')
    {
        if (memcmp(pos, keyword, strlen(keyword)) == 0)
            return pos + strlen(keyword);

        pos = strchr(pos, '\n');
        if (pos) pos++;
    }

    return NULL;
}


/**************************************************************************
 *              import_clipboard_data
 *
 *  Generic import clipboard data routine.
 */
static void *import_clipboard_data(long data, size_t *ret_size)
{
    void *ret = NULL;
#if 0
    size_t len = CFDataGetLength(data);
    if (len && (ret = malloc(len)))
    {
        memcpy(ret, CFDataGetBytePtr(data), len);
        *ret_size = len;
    }
#endif
    return ret;
}


static CPTABLEINFO *get_ansi_cp(void)
{
    USHORT utf8_hdr[2] = { 0, CP_UTF8 };
    static CPTABLEINFO cp;
    if (!cp.CodePage)
    {
        if (NtCurrentTeb()->Peb->AnsiCodePageData)
            RtlInitCodePageTable(NtCurrentTeb()->Peb->AnsiCodePageData, &cp);
        else
            RtlInitCodePageTable(utf8_hdr, &cp);
    }
    return &cp;
}


/* based on wine_get_dos_file_name */
static WCHAR *get_dos_file_name(const char *path)
{
    ULONG len = strlen(path) + 9; /* \??\unix prefix */
    WCHAR *ret;

    if (!(ret = malloc(len * sizeof(WCHAR)))) return NULL;
    if (wine_unix_to_nt_file_name(path, ret, &len))
    {
        free(ret);
        return NULL;
    }

    if (ret[5] == ':')
    {
        /* get rid of the \??\ prefix */
        memmove(ret, ret + 4, (len - 4) * sizeof(WCHAR));
    }
    else ret[1] = '\\';
    return ret;
}


/***********************************************************************
 *           get_nt_pathname
 *
 * Simplified version of RtlDosPathNameToNtPathName_U.
 */
static BOOL get_nt_pathname(const WCHAR *name, UNICODE_STRING *nt_name)
{
    static const WCHAR ntprefixW[] = {'\\','?','?','\\'};
    static const WCHAR uncprefixW[] = {'U','N','C','\\'};
    size_t len = lstrlenW(name);
    WCHAR *ptr;

    nt_name->MaximumLength = (len + 8) * sizeof(WCHAR);
    if (!(ptr = malloc(nt_name->MaximumLength))) return FALSE;
    nt_name->Buffer = ptr;

    memcpy(ptr, ntprefixW, sizeof(ntprefixW));
    ptr += ARRAYSIZE(ntprefixW);
    if (name[0] == '\\' && name[1] == '\\')
    {
        if ((name[2] == '.' || name[2] == '?') && name[3] == '\\')
        {
            name += 4;
            len -= 4;
        }
        else
        {
            memcpy(ptr, uncprefixW, sizeof(uncprefixW));
            ptr += ARRAYSIZE(uncprefixW);
            name += 2;
            len -= 2;
        }
    }
    memcpy(ptr, name, (len + 1) * sizeof(WCHAR));
    ptr += len;
    nt_name->Length = (ptr - nt_name->Buffer) * sizeof(WCHAR);
    return TRUE;
}


/* based on wine_get_unix_file_name */
static char *get_unix_file_name(const WCHAR *dosW)
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    ULONG size = 256;
    char *buffer;

    if (!get_nt_pathname(dosW, &nt_name)) return NULL;
    InitializeObjectAttributes(&attr, &nt_name, 0, 0, NULL);
    for (;;)
    {
        if (!(buffer = malloc(size)))
        {
            free(nt_name.Buffer);
            return NULL;
        }
        status = wine_nt_to_unix_file_name(&attr, buffer, &size, FILE_OPEN_IF);
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        free(buffer);
    }
    free(nt_name.Buffer);
    if (status)
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

/**************************************************************************
 *              register_win32_formats
 *
 * Register Win32 clipboard formats the first time we encounter them.
 */
static void register_win32_formats(const UINT *ids, UINT size)
{
    unsigned int i;

    if (list_empty(&format_list)) register_builtin_formats();

    for (i = 0; i < size; i++)
        register_format(ids[i], NULL);
}


/***********************************************************************
 *              get_clipboard_formats
 *
 * Return a list of all formats currently available on the Win32 clipboard.
 * Helper for set_mac_pasteboard_types_from_win32_clipboard.
 */
static UINT *get_clipboard_formats(UINT *size)
{
    UINT *ids;

    *size = 256;
    for (;;)
    {
        if (!(ids = malloc(*size * sizeof(*ids)))) return NULL;
        if (NtUserGetUpdatedClipboardFormats(ids, *size, size)) break;
        free(ids);
        if (RtlGetLastWin32Error() != ERROR_INSUFFICIENT_BUFFER) return NULL;
    }
    register_win32_formats(ids, *size);
    return ids;
}


/**************************************************************************
 *              set_mac_pasteboard_types_from_win32_clipboard
 */
static void set_mac_pasteboard_types_from_win32_clipboard(void)
{
    WINE_CLIPFORMAT *format;
    UINT count, i, *formats;

    if (!(formats = get_clipboard_formats(&count))) return;

    //macdrv_clear_pasteboard(clipboard_cocoa_window);

    for (i = 0; i < count; i++)
    {
        LIST_FOR_EACH_ENTRY(format, &format_list, WINE_CLIPFORMAT, entry)
        {
	/*
            if (format->format_id != formats[i]) continue;
            TRACE("%s -> %s\n", debugstr_format(format->format_id), debugstr_cf(format->type));
            macdrv_set_pasteboard_data(format->type, NULL, clipboard_cocoa_window);
	  */
        }
    }

    free(formats);
    return;
}


/**************************************************************************
 *              grab_win32_clipboard
 *
 * Grab the Win32 clipboard when a Mac app has taken ownership of the
 * pasteboard, and fill it with the pasteboard data types.
 */
static void grab_win32_clipboard(void)
{
}


/**************************************************************************
 *              update_clipboard
 *
 * Periodically update the clipboard while the clipboard is owned by a
 * Mac app.
 */
static void update_clipboard(void)
{
    static BOOL updating;

    TRACE("is_clipboard_owner %d last_clipboard_update %u now %u\n",
          is_clipboard_owner, last_clipboard_update, (unsigned int)NtGetTickCount());

    if (updating) return;
    updating = TRUE;

    if (is_clipboard_owner)
    {
        if (NtGetTickCount() - last_clipboard_update > CLIPBOARD_UPDATE_DELAY)
            grab_win32_clipboard();
    }
    grab_win32_clipboard();

    updating = FALSE;
}


static BOOL init_clipboard(HWND hwnd)
{
    struct broadwaydrv_window_features wf;

    memset(&wf, 0, sizeof(wf));

    clipboard_hwnd = hwnd;
    clipboard_thread_id = GetCurrentThreadId();
    NtUserAddClipboardFormatListener(clipboard_hwnd);
    register_builtin_formats();
    grab_win32_clipboard();

    TRACE("clipboard thread %04x running\n", clipboard_thread_id);
    return TRUE;
}


/**************************************************************************
 *              macdrv_ClipboardWindowProc
 *
 * Window procedure for the clipboard manager.
 */
LRESULT BROADWAY_ClipboardWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_NCCREATE:
            return init_clipboard(hwnd);
        case WM_CLIPBOARDUPDATE:
            if (is_clipboard_owner) break;  /* ignore our own changes */
            if ((LONG)(NtUserGetClipboardSequenceNumber() - last_get_seqno) <= 0) break;
            //set_mac_pasteboard_types_from_win32_clipboard();
            break;
        case WM_RENDERFORMAT:
            break;
        case WM_TIMER:
            if (!is_clipboard_owner) break;
            grab_win32_clipboard();
            break;
        case WM_DESTROYCLIPBOARD:
            TRACE("WM_DESTROYCLIPBOARD: lost ownership\n");
            is_clipboard_owner = FALSE;
            NtUserKillTimer(hwnd, 1);
            break;
        case WM_USER:
            update_clipboard();
            break;
    }
    return NtUserMessageCall(hwnd, msg, wp, lp, NULL, NtUserDefWindowProc, FALSE);
}


/**************************************************************************
 *              Mac User Driver Clipboard Exports
 **************************************************************************/


/**************************************************************************
 *              macdrv_UpdateClipboard
 */
void BROADWAYDRV_UpdateClipboard(void)
{
    static ULONG last_update;
    static HWND clipboard_manager;
    ULONG now;
    DWORD_PTR ret;

    if (GetCurrentThreadId() == clipboard_thread_id) return;

    TRACE("\n");

    now = NtGetTickCount();
    if (last_update && (int)(now - last_update) <= CLIPBOARD_UPDATE_DELAY) return;

    if (!NtUserIsWindow(clipboard_manager))
    {
        UNICODE_STRING str;
        RtlInitUnicodeString(&str, clipboard_classname);
        clipboard_manager = NtUserFindWindowEx(NULL, NULL, &str, NULL, 0);
        if (!clipboard_manager)
        {
            ERR("clipboard manager not found\n");
            return;
        }
    }

    last_update = now;
}


/**************************************************************************
 *              MACDRV Private Clipboard Exports
 **************************************************************************/


/**************************************************************************
 *              query_pasteboard_data
 */
BOOL query_pasteboard_data(HWND hwnd, LPSTR type)
{
#if 0
    struct get_clipboard_params params = { .data_only = TRUE, .size = 1024 };
    WINE_CLIPFORMAT *format;
    BOOL ret = FALSE;

    TRACE("win %p/%p type %s\n", hwnd, clipboard_cocoa_window, debugstr_cf(type));

    format = format_for_type(type);
    if (!format) return FALSE;

    if (!NtUserOpenClipboard(clipboard_hwnd, 0))
    {
        ERR("failed to open clipboard for %s\n", debugstr_cf(type));
        return FALSE;
    }

    for (;;)
    {
        if (!(params.data = malloc(params.size))) break;
        if (NtUserGetClipboardData(format->format_id, &params))
        {
            CFDataRef data;

            TRACE("exporting %s\n", debugstr_format(format->format_id));

            if ((data = format->export_func(params.data, params.size)))
            {
                ret = macdrv_set_pasteboard_data(format->type, data, clipboard_cocoa_window);
                CFRelease(data);
            }
            free(params.data);
            break;
        }
        free(params.data);
        if (!params.data_size) break;
        params.size = params.data_size;
        params.data_size = 0;
    }

    last_get_seqno = NtUserGetClipboardSequenceNumber();

    NtUserCloseClipboard();

    return ret;
#endif
}


/**************************************************************************
 *              macdrv_lost_pasteboard_ownership
 *
 * Handler for the LOST_PASTEBOARD_OWNERSHIP event.
 */
void macdrv_lost_pasteboard_ownership(HWND hwnd)
{
 //   TRACE("win %p\n", hwnd);
   // if (!macdrv_is_pasteboard_owner(clipboard_cocoa_window))
     //   grab_win32_clipboard();
}


/**************************************************************************
 *              macdrv_dnd_release
 */
NTSTATUS macdrv_dnd_release(void *arg)
{
    UINT64 handle = *(UINT64 *)arg;
    //CFRelease(pasteboard_from_handle(handle));
    return 0;
}


/**************************************************************************
 *              macdrv_dnd_retain
 */
NTSTATUS macdrv_dnd_retain(void *arg)
{
    UINT64 handle = *(UINT64 *)arg;
    //CFRetain(pasteboard_from_handle(handle));
    return 0;
}
