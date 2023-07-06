/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Sample Server (Advanced Input)
 *
 * Copyright 2022 Armin Novak <armin.novak@thincast.com>
 * Copyright 2022 Thincast Technologies GmbH
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

//#include <winpr/assert.h>

#include "sfreerdp.h"

#include "sf_ainput.h"

#include <freerdp/server/server-common.h>
#include <freerdp/server/ainput.h>

#include <freerdp/log.h>
#define TAG SERVER_TAG("sample.ainput")

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT sf_peer_ainput_mouse_event(ainput_server_context* context, UINT64 timestamp,
                                       UINT64 flags, INT32 x, INT32 y)
{
	/* TODO: Implement */
	WINPR_ASSERT(context);

	WLog_WARN(TAG, "not implemented: 0x%08" PRIx64 ", 0x%08" PRIx64 ", %" PRId32 "x%" PRId32,
	          timestamp, flags, x, y);
	return CHANNEL_RC_OK;
}

void sf_peer_ainput_init(testPeerContext* context)
{
#if 0
	WINPR_ASSERT(context);

	context->ainput = ainput_server_context_new(context->vcm);
	WINPR_ASSERT(context->ainput);

	context->ainput->rdpcontext = &context->_p;
	context->ainput->data = context;

	context->ainput->MouseEvent = sf_peer_ainput_mouse_event;
#endif
}

BOOL sf_peer_ainput_start(testPeerContext* context)
{
	//if (!context || !context->ainput || !context->ainput->Open)
		return FALSE;

	//return context->ainput->Open(context->ainput) == CHANNEL_RC_OK;
}

BOOL sf_peer_ainput_stop(testPeerContext* context)
{
//	if (!context || !context->ainput || !context->ainput->Close)
		return FALSE;

//	return context->ainput->Close(context->ainput) == CHANNEL_RC_OK;
}

BOOL sf_peer_ainput_running(testPeerContext* context)
{
	//if (!context || !context->ainput || !context->ainput->IsOpen)
		return FALSE;

	//return context->ainput->IsOpen(context->ainput);
}

void sf_peer_ainput_uninit(testPeerContext* context)
{
	//ainput_server_context_free(context->ainput);
}
/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Sample Server (Audio Input)
 *
 * Copyright 2012 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2015 Thincast Technologies GmbH
 * Copyright 2015 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
 * Copyright 2023 Pascal Nowack <Pascal.Nowack@gmx.de>
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

//#include <winpr/assert.h>

#include "sfreerdp.h"

#include "sf_audin.h"

#include <freerdp/server/server-common.h>
#include <freerdp/log.h>
#define TAG SERVER_TAG("sample")

#if defined(CHANNEL_AUDIN_SERVER)

static UINT sf_peer_audin_data(audin_server_context* audin, const SNDIN_DATA* data)
{
	/* TODO: Implement */
	WINPR_ASSERT(audin);
	WINPR_ASSERT(data);

	WLog_WARN(TAG, "not implemented");
	WLog_DBG(TAG, "receive %" PRIdz " bytes.", Stream_Length(data->Data));
	return CHANNEL_RC_OK;
}

#endif

BOOL sf_peer_audin_init(testPeerContext* context)
{
	WINPR_ASSERT(context);
#if defined(CHANNEL_AUDIN_SERVER)
	context->audin = audin_server_context_new(context->vcm);
	WINPR_ASSERT(context->audin);

	context->audin->rdpcontext = &context->_p;
	context->audin->userdata = context;

	context->audin->Data = sf_peer_audin_data;

	return audin_server_set_formats(context->audin, -1, NULL);
#else
	return TRUE;
#endif
}

BOOL sf_peer_audin_start(testPeerContext* context)
{
#if defined(CHANNEL_AUDIN_SERVER)
	if (!context || !context->audin || !context->audin->Open)
		return FALSE;

	return context->audin->Open(context->audin);
#else
	return FALSE;
#endif
}

BOOL sf_peer_audin_stop(testPeerContext* context)
{
#if defined(CHANNEL_AUDIN_SERVER)
	if (!context || !context->audin || !context->audin->Close)
		return FALSE;

	return context->audin->Close(context->audin);
#else
	return FALSE;
#endif
}

BOOL sf_peer_audin_running(testPeerContext* context)
{
#if defined(CHANNEL_AUDIN_SERVER)
	if (!context || !context->audin || !context->audin->IsOpen)
		return FALSE;

	return context->audin->IsOpen(context->audin);
#else
	return FALSE;
#endif
}

void sf_peer_audin_uninit(testPeerContext* context)
{
	WINPR_ASSERT(context);

#if defined(CHANNEL_AUDIN_SERVER)
	audin_server_context_free(context->audin);
	context->audin = NULL;
#endif
}
/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Sample Server (Lync Multiparty)
 *
 * Copyright 2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2015 Thincast Technologies GmbH
 * Copyright 2015 DI (FH) Martin Haimberger <martin.haimberger@thincast.com>
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


#include "sf_encomsp.h"

BOOL sf_peer_encomsp_init(testPeerContext* context)
{
	WINPR_ASSERT(context);

	context->encomsp = encomsp_server_context_new(context->vcm);
	if (!context->encomsp)
		return FALSE;

	context->encomsp->rdpcontext = &context->_p;

	WINPR_ASSERT(context->encomsp->Start);
	if (context->encomsp->Start(context->encomsp) != CHANNEL_RC_OK)
		return FALSE;

	return TRUE;
}
 /* FreeRDP Sample Server (Audio Output) */
 
#include "sf_rdpsnd.h"

#include <freerdp/server/server-common.h>
#include <freerdp/log.h>
#define TAG SERVER_TAG("sample")

static void sf_peer_rdpsnd_activated(RdpsndServerContext* context)
{
	WINPR_UNUSED(context);
	WINPR_ASSERT(context);
	WLog_DBG(TAG, "RDPSND Activated");
}

BOOL sf_peer_rdpsnd_init(testPeerContext* context)
{
	WINPR_ASSERT(context);

	context->rdpsnd = rdpsnd_server_context_new(context->vcm);
	WINPR_ASSERT(context->rdpsnd);
	context->rdpsnd->rdpcontext = &context->_p;
	context->rdpsnd->data = context;
	context->rdpsnd->num_server_formats =
	    server_rdpsnd_get_formats(&context->rdpsnd->server_formats);

	if (context->rdpsnd->num_server_formats > 0)
		context->rdpsnd->src_format = &context->rdpsnd->server_formats[0];

	context->rdpsnd->Activated = sf_peer_rdpsnd_activated;

	WINPR_ASSERT(context->rdpsnd->Initialize);
	if (context->rdpsnd->Initialize(context->rdpsnd, TRUE) != CHANNEL_RC_OK)
	{
		return FALSE;
	}

	return TRUE;
}
