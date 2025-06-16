#!/bin/bash

# Script to register the WineRDS driver with Wine
# This tells Wine to load our driver when creating graphics devices

echo "=== Registering WineRDS Driver with Wine ==="

# Register our driver in the Wine registry
echo "Adding registry key to enable winerds driver..."
./wine reg add "HKEY_CURRENT_USER\\Software\\Wine\\Drivers" /v Graphics /t REG_SZ /d "winerds" /f

echo "Registry key added. Wine will now try to load winerds.drv"

# Check if the key was added successfully
echo "Verifying registry entry..."
./wine reg query "HKEY_CURRENT_USER\\Software\\Wine\\Drivers" /v Graphics

echo
echo "=== Testing with GDI Test Program ==="

# Test with gditest
if [ -f "programs/gditest/x86_64-windows/gditest.exe" ]; then
    echo "Running gditest.exe with RDS driver..."
    WINEDLLOVERRIDES="winerds=n,b" ./wine programs/gditest/x86_64-windows/gditest.exe
else
    echo "gditest.exe not found, testing with winecfg..."
    timeout 5s ./wine winecfg
fi

echo
echo "=== Checking Wine Debug Output ==="
echo "Look for messages like:"
echo "  - 'RDS driver initializing'"
echo "  - 'wine_get_user_driver called'"
echo "  - 'Updating RDS display devices'"
echo
echo "If you see 'Failed to read display config' errors, the driver may not be loading properly."

# Alternative: Set as primary driver (risky!)
echo
echo "=== Alternative: Set as Primary Driver (Advanced) ==="
echo "If you want to make RDS the primary/only driver, run:"
echo "  wine reg add \"HKEY_CURRENT_USER\\\\Software\\\\Wine\\\\Drivers\" /v Graphics /t REG_SZ /d \"winerds\" /f"
echo
echo "To revert back to X11:"
echo "  wine reg add \"HKEY_CURRENT_USER\\\\Software\\\\Wine\\\\Drivers\" /v Graphics /t REG_SZ /d \"x11\" /f"

