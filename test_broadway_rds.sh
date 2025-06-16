#!/bin/bash

# Wine RDS Broadway Integration Test Script

echo "=== Wine RDS Broadway Integration Test ==="
echo "This script tests the integrated Broadway web interface for Wine RDS"
echo

# Check if binaries exist
echo "1. Checking required binaries..."
if [ ! -f "programs/termsrv/termsrv" ]; then
    echo "❌ termsrv binary not found. Run 'make programs/termsrv' first."
    exit 1
fi
echo "✅ termsrv binary found"

if [ ! -f "programs/rds_test/ping_test" ]; then
    echo "❌ ping_test binary not found. Run 'make programs/rds_test' first."
    exit 1
fi
echo "✅ ping_test binary found"

# Check if Broadway server files are compiled
if [ ! -f "programs/termsrv/broadway_server.o" ] && [ ! -f "programs/termsrv/aarch64-windows/broadway_server.o" ]; then
    echo "❌ broadway_server.o not found. Broadway integration may not be compiled."
    echo "   Run 'make programs/termsrv' to compile with Broadway support."
    exit 1
fi
echo "✅ Broadway server object file found"

echo
echo "2. Starting Broadway-enabled termsrv..."
echo "   Command: ./programs/termsrv/termsrv --broadway"
echo "   This will start:"
echo "   - Named pipe server on \\\\pipe\\WineRDS"
echo "   - HTTP server on port 8080 (web interface)"
echo "   - WebSocket server on port 8765 (input handling)"
echo

# Start termsrv with Broadway enabled in background
./programs/termsrv/termsrv --broadway &
TERMSRV_PID=$!

# Give it time to start
echo "Waiting for termsrv to initialize..."
sleep 3

# Check if termsrv is still running
if ! kill -0 $TERMSRV_PID 2>/dev/null; then
    echo "❌ termsrv failed to start or crashed"
    exit 1
fi

echo "✅ termsrv started successfully (PID: $TERMSRV_PID)"
echo

echo "3. Testing HTTP server availability..."
HTTP_TEST=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080 --connect-timeout 5 || echo "000")
if [ "$HTTP_TEST" = "200" ]; then
    echo "✅ HTTP server responding on port 8080"
    echo "   Web interface: http://localhost:8080"
else
    echo "❌ HTTP server not responding (status: $HTTP_TEST)"
fi

echo
echo "4. Testing WebSocket server availability..."
# Simple WebSocket test using netcat if available
if command -v nc >/dev/null 2>&1; then
    WS_TEST=$(timeout 2s nc -z localhost 8765 && echo "OK" || echo "FAIL")
    if [ "$WS_TEST" = "OK" ]; then
        echo "✅ WebSocket server listening on port 8765"
    else
        echo "❌ WebSocket server not responding on port 8765"
    fi
else
    echo "⚠️  netcat not available, skipping WebSocket test"
fi

echo
echo "5. Testing RDS pipe connectivity..."
timeout 10s ./programs/rds_test/ping_test &
PING_PID=$!

sleep 2

if kill -0 $PING_PID 2>/dev/null; then
    echo "✅ ping_test connected to RDS pipe"
    wait $PING_PID
    PING_EXIT=$?
    if [ $PING_EXIT -eq 0 ]; then
        echo "✅ ping_test completed successfully"
    else
        echo "⚠️  ping_test exited with code $PING_EXIT"
    fi
else
    echo "❌ ping_test failed to connect"
fi

echo
echo "6. Checking for screenshot generation..."
sleep 2
SCREENSHOTS=$(ls rds_screenshot_*.{bmp,png} 2>/dev/null | wc -l)
if [ $SCREENSHOTS -gt 0 ]; then
    echo "✅ Found $SCREENSHOTS screenshot(s) generated"
    ls -la rds_screenshot_*.{bmp,png} 2>/dev/null | head -3
else
    echo "⚠️  No screenshots found yet (may take up to 60 seconds)"
fi

echo
echo "7. Broadway Integration Summary:"
echo "================================"
echo "📊 Service Status:"
if kill -0 $TERMSRV_PID 2>/dev/null; then
    echo "   ✅ termsrv running (PID: $TERMSRV_PID)"
else
    echo "   ❌ termsrv not running"
fi

echo "📊 Network Services:"
echo "   🌐 Web Interface: http://localhost:8080"
echo "   🔌 WebSocket: ws://localhost:8765"
echo "   📡 Named Pipe: \\\\pipe\\WineRDS"

echo
echo "🎯 To test Broadway functionality:"
echo "   1. Open http://localhost:8080 in your web browser"
echo "   2. You should see the Wine RDS framebuffer (800x600)"
echo "   3. Canvas should update with drawing operations"
echo "   4. Mouse clicks and keyboard input should work (when WebSocket is fully implemented)"

echo
echo "⏹️  To stop the test:"
echo "   Press Ctrl+C or run: kill $TERMSRV_PID"

# Wait for user interrupt or termsrv to exit
echo
echo "Press Ctrl+C to stop the test and termsrv..."
wait $TERMSRV_PID

echo
echo "=== Test completed ==="