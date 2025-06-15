#!/bin/bash
# Quick Wine RDS Test - Basic functionality check

echo "=== Quick Wine RDS Test ==="

# Check if we're in the right directory
if [ ! -f "configure.ac" ]; then
    echo "❌ Not in Wine source directory"
    exit 1
fi

echo "✅ In Wine source directory"

# Check if binaries exist
echo "Checking binaries..."
if [ -f "programs/termsrv/termsrv" ]; then
    echo "✅ termsrv found"
else
    echo "❌ termsrv not found - need to build"
    exit 1
fi

if [ -f "programs/rds_test/ping_test" ]; then
    echo "✅ ping_test found"
else
    echo "❌ ping_test not found - need to build"
    exit 1
fi

# Quick functionality test
echo "Starting quick test..."

# Start termsrv in background
echo "Starting termsrv..."
timeout 15s ./programs/termsrv/termsrv > quick_test.log 2>&1 &
TERMSRV_PID=$!

# Wait a moment for startup
sleep 3

# Check if termsrv is running
if ! kill -0 $TERMSRV_PID 2>/dev/null; then
    echo "❌ termsrv failed to start"
    cat quick_test.log
    exit 1
fi

echo "✅ termsrv started"

# Run ping test
echo "Running ping test..."
timeout 10s ./programs/rds_test/ping_test >> quick_test.log 2>&1
PING_RESULT=$?

# Cleanup
kill $TERMSRV_PID 2>/dev/null || true
wait $TERMSRV_PID 2>/dev/null || true

# Check results
if [ $PING_RESULT -eq 0 ]; then
    echo "✅ Ping test completed"
else
    echo "❌ Ping test failed"
fi

# Check for key messages in log
if grep -q "RDS service initialized successfully" quick_test.log; then
    echo "✅ Service initialized"
else
    echo "❌ Service initialization unclear"
fi

if grep -q "CreateDC.*succeeded" quick_test.log; then
    echo "✅ CreateDC successful"
else
    echo "⚠️  CreateDC status unclear"
fi

if grep -q "PING.*PONG" quick_test.log; then
    echo "✅ Ping/Pong communication working"
else
    echo "⚠️  Ping/Pong communication unclear"
fi

echo ""
echo "=== Quick Test Complete ==="
echo "See quick_test.log for details"
echo ""
echo "Last 10 lines of log:"
tail -10 quick_test.log