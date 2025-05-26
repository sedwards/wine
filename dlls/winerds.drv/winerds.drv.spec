# winerds.drv.spec - Minimum exports for a Wine display driver

# Driver core functions
;@ stdcall ActivateDriver(ptr long)
;@ stdcall GetDriverName()
;@ stdcall DisableDriver()

# GDI core functions
;@ stdcall CreateDC(wstr wstr wstr ptr)
;@ stdcall CreateCompatibleDC(long)
;@ stdcall DeleteDC(long)
;@ stdcall GetDeviceCaps(long long)
;@ stdcall SelectObject(long long)
;@ stdcall DeleteObject(long)

# Surface/bitmap functions
;@ stdcall CreateCompatibleBitmap(long long long)
;@ stdcall GetPixel(long long long)
;@ stdcall SetPixel(long long long long)
;@ stdcall BitBlt(long long long long long long long long long)
;@ stdcall StretchBlt(long long long long long long long long long long long)

# Drawing primitives
;@ stdcall Rectangle(long long long long long)
;@ stdcall LineTo(long long long)
;@ stdcall MoveToEx(long long long ptr)
;@ stdcall Ellipse(long long long long long)
;@ stdcall Polygon(long ptr long long)
;@ stdcall SetDIBits(long long long long ptr ptr long)
;@ stdcall GetDIBits(long long long long ptr ptr long)

# Text functions
;@ stdcall TextOutA(long long long str long)
;@ stdcall TextOutW(long long long wstr long)
;@ stdcall ExtTextOutA(long long long long ptr str long ptr)
;@ stdcall ExtTextOutW(long long long long ptr wstr long ptr)

# Desktop management
;@ stdcall GetSystemMetrics(long)
;@ stdcall EnumDisplayMonitors(long ptr ptr long)
;@ stdcall EnumDisplaySettingsExW(wstr long ptr long)
;@ stdcall ChangeDisplaySettingsExW(wstr ptr long long ptr)

# Window management helpers 
;@ stdcall UpdateWindow(long)
;@ stdcall RedrawWindow(long ptr long long)
;@ stdcall InvalidateRect(long ptr long)

# Init Driver
@ stdcall DrvEnableDriver(long long ptr) winerdp.drv.DrvEnableDriver
@ stdcall DrvEnablePDEV(ptr ptr long ptr long ptr long ptr long ptr long) winerdp.drv.DrvEnablePDEV
@ stdcall DrvCompletePDEV(ptr ptr) winerdp.drv.DrvCompletePDEV
@ stdcall DrvDisablePDEV(ptr) winerdp.drv.DrvDisablePDEV
@ stdcall DrvEnableSurface(ptr) winerdp.drv.DrvEnableSurface
@ stdcall DrvDisableSurface(ptr) winerdp.drv.DrvDisableSurface
@ stdcall DrvCreateDC(wstr wstr wstr ptr) winerdp.drv.DrvCreateDC
@ stdcall DrvCreateDeviceBitmap(long long long long) winerdp.drv.DrvCreateDeviceBitmap
@ stdcall DrvDeleteDeviceBitmap(long) winerdp.drv.DrvDeleteDeviceBitmap

# Basic DC Functions
@ stdcall CreateDCA(str str str ptr) User32.CreateDCA
@ stdcall CreateDCW(wstr wstr wstr ptr) User32.CreateDCW
@ stdcall CreateCompatibleDC(long) User32.CreateCompatibleDC
@ stdcall DeleteDC(long) User32.DeleteDC
@ stdcall GetDeviceCaps(long long) User32.GetDeviceCaps
@ stdcall SaveDC(long) User32.SaveDC
@ stdcall RestoreDC(long long) User32.RestoreDC

# Viewport/Coordinate Functions
@ stdcall SetViewportOrgEx(long long long ptr) gdi32.RdsSetViewportOrgEx
@ stdcall SetViewportExtEx(long long long ptr) gdi32.RdsSetViewportExtEx
@ stdcall GetViewportOrgEx(long ptr) gdi32.GetViewportOrgEx
@ stdcall GetViewportExtEx(long ptr) gdi32.GetViewportExtEx

# DIB Functions
@ stdcall CreateDIBSection(long ptr long ptr long long) gdi32.RdsCreateDIBSection
@ stdcall GetDIBits(long long long long ptr ptr long) gdi32.GetDIBits
@ stdcall SetDIBits(long long long long ptr ptr long) gdi32.SetDIBits

# Drawing Functions - Forwarded to gdi32
@ stdcall BitBlt(long long long long long long long long long) gdi32.BitBlt
@ stdcall StretchBlt(long long long long long long long long long long long) gdi32.StretchBlt
@ stdcall PatBlt(long long long long long long) gdi32.PatBlt
@ stdcall LineTo(long long long) gdi32.LineTo
@ stdcall Rectangle(long long long long long) gdi32.Rectangle
@ stdcall Ellipse(long long long long long) gdi32.Ellipse
@ stdcall TextOutA(long long long str long) gdi32.TextOutA
@ stdcall TextOutW(long long long wstr long) gdi32.TextOutW
@ stdcall ExtTextOutA(long long long long ptr str long ptr) gdi32.ExtTextOutA
@ stdcall ExtTextOutW(long long long long ptr wstr long ptr) gdi32.ExtTextOutW

# Brush/Pen Functions - Forwarded to gdi32
@ stdcall CreateSolidBrush(long) gdi32.CreateSolidBrush
@ stdcall CreatePen(long long long) gdi32.CreatePen
@ stdcall SelectObject(long long) gdi32.SelectObject
@ stdcall DeleteObject(long) gdi32.DeleteObject

# Palette Functions - Forwarded to gdi32
@ stdcall CreatePalette(ptr) gdi32.CreatePalette
@ stdcall RealizePalette(long) gdi32.RealizePalette
@ stdcall GetSystemPaletteEntries(long long long ptr) gdi32.GetSystemPaletteEntries

# RDS-Specific Functions
;@ stdcall RdsCaptureBitmapBits(long ptr ptr ptr ptr ptr)
;@ stdcall RdsEnumSessionDevices(long ptr ptr)
;@ stdcall RdsSetDeviceResolution(long long long long)

# Window Functions - Forwarded to user32
@ stdcall GetDC(long) user32.GetDC
@ stdcall ReleaseDC(long long) user32.ReleaseDC
@ stdcall GetWindowDC(long) user32.GetWindowDC

# Region Functions - Forwarded to gdi32
@ stdcall CreateRectRgn(long long long long) gdi32.CreateRectRgn
@ stdcall SelectClipRgn(long long) gdi32.SelectClipRgn
