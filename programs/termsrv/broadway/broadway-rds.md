# Wine RDS Terminal Service Design Overview

This document summarizes the implementation and integration plan for a Wine-based Remote Desktop Service (WineRDS), allowing multiple isolated Wine sessions to be accessed and interacted with over the network via WebSockets and a browser frontend.

---

## 📦 Components Overview

### 1. WineRDS Display Driver

A Wine display driver (`winerds.drv`) that:

* Creates a framebuffer using `CreateDIBSection`
* Shares the buffer in a named shared memory segment: `Global\winerds_framebuffer_<session>`
* Captures GDI operations using `BitBlt` and `PatBlt`
* Supports multi-session via the `WINERDS_SESSION` environment variable

### 2. Terminal Service Executable (C)

Embedded WebSocket server using Mongoose:

* Parses JSON input from clients
* Injects input via `PostMessage` to Wine’s virtual desktop window
* Sends acknowledgments back to clients for cursor and input state
* Optional feedback can be toggled with `--no-feedback`

### 3. HTML5 Client (Browser)

A lightweight browser client:

* HTML5 `<canvas>` to draw and interact
* WebSocket-based input handling
* Red cursor overlay that follows ACKs from server
* Sends mouse and keyboard events in JSON format

### 4. Wine Session Launcher (Bash Script)

```bash
./wine_session_manager.sh user1 notepad.exe
```

* Launches Wine using unique `WINEPREFIX` and `/desktop=User1,...`
* Sets `WINERDS_SESSION=user1` for shared memory isolation

### 5. Systemd Template

A `wine-session@.service` file to run each session under its own user

---

## ✅ JSON Protocol Format

### Mouse Click:

```json
{ "type": "mouse", "action": "click", "x": 300, "y": 200 }
```

### Key Press:

```json
{ "type": "key", "char": "A" }
```

### ACK from Server:

```json
{ "ack": "mouse", "x": 300, "y": 200 }
{ "ack": "key", "char": "A" }
```

---

## 🔧 Terminal Service Executable

Implemented in `terminal_service_ws.c`:

* Mongoose-based WebSocket server
* Uses `FindWindowA` + `PostMessage` for input injection
* ACK logic toggleable

---

## 📜 Additional Tools

### Python: validate\_shared\_framebuffer.py

Cross-platform reader that opens Wine’s shared memory buffer and optionally displays it with Pillow.

### Python: websocket\_input\_bridge.py

Simple Python version of the WebSocket listener using `pywin32`, useful for diagnostics or integration.

---

## 🧩 To Do / Optional Enhancements

* Canvas framebuffer auto-refresh from Wine's shared buffer
* Support `wss://` secure WebSocket
* Switchable sessions via browser query or dropdown
* Custom login/authentication model for session access

---

Let me know when you're ready to bundle this into a portable service package, or if you'd like a CI pipeline setup (e.g., GitHub Actions) for building WineRDS drivers and deploying the WebSocket bridge.



