# validate_shared_framebuffer.py
# Cross-platform script to validate and display the shared framebuffer

import mmap
import struct
import sys
from pathlib import Path
from time import sleep

WIDTH = 800
HEIGHT = 600
SHM_NAME = "/dev/shm/winerds_framebuffer" if Path("/dev/shm").exists() else "\\\\.\\Global\\winerds_framebuffer"
FB_SIZE = WIDTH * HEIGHT * 4

try:
    if sys.platform == "win32":
        import ctypes
        from ctypes import wintypes

        kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)

        INVALID_HANDLE_VALUE = wintypes.HANDLE(-1).value

        kernel32.OpenFileMappingW.restype = wintypes.HANDLE
        kernel32.OpenFileMappingW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]

        kernel32.MapViewOfFile.restype = wintypes.LPVOID
        kernel32.MapViewOfFile.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, ctypes.c_size_t]

        FILE_MAP_READ = 0x0004

        hMap = kernel32.OpenFileMappingW(FILE_MAP_READ, False, SHM_NAME)
        if not hMap:
            raise RuntimeError("Could not open shared memory")

        ptr = kernel32.MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, FB_SIZE)
        if not ptr:
            raise RuntimeError("Could not map shared memory")

        buf = (ctypes.c_ubyte * FB_SIZE).from_address(ptr)
        framebuffer = bytes(buf)
    else:
        with open(SHM_NAME, "rb") as f:
            mm = mmap.mmap(f.fileno(), FB_SIZE, access=mmap.ACCESS_READ)
            framebuffer = mm.read(FB_SIZE)

    print("✅ Framebuffer read successfully ({} bytes)".format(len(framebuffer)))

except Exception as e:
    print(f"❌ Failed to read framebuffer: {e}")
    sys.exit(1)

# Optional: Display with Pillow
try:
    from PIL import Image
    import io

    img = Image.frombytes("RGBA", (WIDTH, HEIGHT), framebuffer)
    img.show()
except ImportError:
    print("🛈 Pillow not installed. Install with: pip install pillow")


