#!/bin/bash

# Wine RDS Driver Setup Script
# This script configures Wine to use the RDS driver

echo "Setting up Wine RDS Driver..."

# Create the registry file
cat > /tmp/rds_driver_setup.reg << 'EOF'
REGEDIT4

[HKEY_CURRENT_USER\Software\Wine\Drivers]
"Graphics"="winerds"

[HKEY_CURRENT_USER\Software\Wine\WineRDS]
"ScreenWidth"=dword:00000320
"ScreenHeight"=dword:00000258
"ScreenDepth"=dword:00000020

[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Video\{00000000-0000-0000-0000-000000000000}\0000]
"VideoID"="{00000000-0000-0000-0000-000000000000}"
"Service"="winerds"
"InstalledDisplayDrivers"="winerds"

[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Hardware Profiles\Current\System\CurrentControlSet\Control\VIDEO\{00000000-0000-0000-0000-000000000000}\0000]
"DefaultSettings.BitsPerPixel"=dword:00000020
"DefaultSettings.XResolution"=dword:00000320
"DefaultSettings.YResolution"=dword:00000258
"DefaultSettings.VRefresh"=dword:0000003c
"DefaultSettings.Flags"=dword:00000000

[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\winerds]
"Type"=dword:00000001
"Start"=dword:00000001
"Group"="Video"
"ErrorControl"=dword:00000000
"ImagePath"="winerds.drv"

[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\GraphicsDrivers]
"DrvList"="winerds.drv"

[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{4D36E968-E325-11CE-BFC1-08002BE10318}\0000]
"ProviderName"="Wine"
"DriverDesc"="Wine RDS Display"
"DriverVersion"="1.0.0.0"
"DriverDate"="01-01-2024"
"MatchingDeviceId"="wine_rds_display"
EOF

# Apply the registry settings
echo "Applying registry settings..."
wine regedit /tmp/rds_driver_setup.reg

# Verify the driver files exist
echo "Checking driver files..."
if [ -f "dlls/winerds.drv/x86_64-windows/winerds.drv" ]; then
    echo "✓ winerds.drv found"
else
    echo "✗ winerds.drv NOT found - check build"
fi

if [ -f "dlls/winerds.drv/winerds.so" ]; then
    echo "✓ winerds.so found"
else
    echo "✗ winerds.so NOT found - check build"
fi

# Set environment variables
echo "Setting environment variables..."
export WINEDLLOVERRIDES="winerds=n,b"
export WINEPREFIX=${WINEPREFIX:-$HOME/.wine}

echo "Wine RDS Driver setup complete!"
echo ""
echo "Usage:"
echo "1. Start terminal server: ./wine programs/termsrv/x86_64-windows/termsrv.exe"
echo "2. In another terminal, run: WINEDLLOVERRIDES=\"winerds=n,b\" ./wine your_app.exe"
echo "3. Check screenshots in current directory"

# Clean up
rm -f /tmp/rds_driver_setup.reg

