# websocket_input_bridge.py
# Receives WebSocket mouse/keyboard events and injects them into the Wine session

import asyncio
import json
import win32gui
import win32con
import win32api
import websockets

WINE_WINDOW_NAME = "RemoteDesktop"

hwnd = win32gui.FindWindow(None, WINE_WINDOW_NAME)
if not hwnd:
    raise SystemExit(f"Window '{WINE_WINDOW_NAME}' not found")

print(f"✅ WebSocket input bridge targeting HWND={hwnd}")

# Handle incoming JSON messages
async def handle_client(websocket):
    async for message in websocket:
        try:
            evt = json.loads(message)
            if evt["type"] == "mouse":
                x, y = evt["x"], evt["y"]
                lparam = win32api.MAKELONG(x, y)
                if evt["action"] == "click":
                    win32gui.PostMessage(hwnd, win32con.WM_MOUSEMOVE, 0, lparam)
                    win32gui.PostMessage(hwnd, win32con.WM_LBUTTONDOWN, win32con.MK_LBUTTON, lparam)
                    win32gui.PostMessage(hwnd, win32con.WM_LBUTTONUP, 0, lparam)
            elif evt["type"] == "key":
                char = evt["char"]
                vk = win32api.VkKeyScan(char)
                win32gui.PostMessage(hwnd, win32con.WM_KEYDOWN, vk, 0)
                win32gui.PostMessage(hwnd, win32con.WM_CHAR, ord(char), 0)
                win32gui.PostMessage(hwnd, win32con.WM_KEYUP, vk, 0)
        except Exception as e:
            print(f"⚠️  Error: {e}")

async def main():
    print("🔌 Listening on ws://localhost:8765")
    async with websockets.serve(handle_client, "0.0.0.0", 8765):
        await asyncio.Future()

asyncio.run(main())


