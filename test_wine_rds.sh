#!/bin/bash
# Wine RDS Automated Test Script
# This script performs basic validation of the Wine RDS system

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
TERMSRV_TIMEOUT=30
PING_TEST_TIMEOUT=20
SCREENSHOT_WAIT=5

# Cleanup function
cleanup() {
    echo -e "${YELLOW}Cleaning up...${NC}"
    if [ ! -z "$TERMSRV_PID" ]; then
        kill $TERMSRV_PID 2>/dev/null || true
        wait $TERMSRV_PID 2>/dev/null || true
    fi
    
    # Kill any remaining test processes
    pkill -f "ping_test" 2>/dev/null || true
    pkill -f "termsrv" 2>/dev/null || true
    
    # Clean up test files
    rm -f test_*.log termsrv.log ping_test.log 2>/dev/null || true
}

# Set up cleanup trap
trap cleanup EXIT

# Helper functions
print_header() {
    echo -e "\n${BLUE}=== $1 ===${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

# Test functions
test_binaries_exist() {
    print_header "Testing Binary Existence"
    
    local all_good=true
    
    if [ -f "programs/termsrv/termsrv" ]; then
        print_success "termsrv binary found"
    else
        print_error "termsrv binary missing"
        all_good=false
    fi
    
    if [ -f "programs/rds_test/ping_test" ]; then
        print_success "ping_test binary found"
    else
        print_error "ping_test binary missing"
        all_good=false
    fi
    
    if [ -f "dlls/winerds.drv/winerds.drv.so" ]; then
        print_success "winerds.drv driver found"
    else
        print_error "winerds.drv driver missing"
        all_good=false
    fi
    
    if [ "$all_good" = false ]; then
        print_error "Missing required binaries. Run 'make' to build the project."
        exit 1
    fi
}

test_termsrv_startup() {
    print_header "Testing Terminal Server Startup"
    
    # Start termsrv in background
    timeout $TERMSRV_TIMEOUT ./programs/termsrv/termsrv > termsrv.log 2>&1 &
    TERMSRV_PID=$!
    
    # Wait for startup
    sleep 3
    
    # Check if process is still running
    if ! kill -0 $TERMSRV_PID 2>/dev/null; then
        print_error "Terminal server failed to start or crashed"
        cat termsrv.log
        exit 1
    fi
    
    # Check for key initialization messages
    if grep -q "RDS service initialized successfully" termsrv.log; then
        print_success "Terminal server started successfully"
    else
        print_error "Terminal server startup incomplete"
        print_info "Last few lines of termsrv.log:"
        tail -5 termsrv.log
        exit 1
    fi
    
    # Check for pipe server
    if grep -q "Ready to receive connections" termsrv.log; then
        print_success "Pipe server is ready"
    else
        print_warning "Pipe server status unclear"
    fi
}

test_ping_connectivity() {
    print_header "Testing Ping/Pong Connectivity"
    
    # Run ping test
    timeout $PING_TEST_TIMEOUT ./programs/rds_test/ping_test > ping_test.log 2>&1
    local ping_exit_code=$?
    
    if [ $ping_exit_code -eq 0 ]; then
        print_success "Ping test completed successfully"
    else
        print_error "Ping test failed with exit code $ping_exit_code"
        print_info "Last few lines of ping_test.log:"
        tail -10 ping_test.log
        return 1
    fi
    
    # Analyze termsrv log for ping/pong activity
    local ping_count=$(grep -c "Received RDS_MSG_PING" termsrv.log || echo "0")
    local pong_count=$(grep -c "Sent RDS_MSG_PONG successfully" termsrv.log || echo "0")
    
    if [ "$ping_count" -gt 0 ] && [ "$pong_count" -gt 0 ]; then
        print_success "Ping/Pong exchange detected ($ping_count pings, $pong_count pongs)"
    else
        print_error "No ping/pong activity detected in termsrv log"
        return 1
    fi
    
    # Check for connection establishment
    if grep -q "Client.*connected" termsrv.log; then
        print_success "Client connection established"
    else
        print_warning "Client connection not clearly established"
    fi
}

test_gdi_interception() {
    print_header "Testing GDI Message Interception"
    
    # Look for GDI messages in termsrv log
    local line_to_count=$(grep -c "LINE_TO" termsrv.log || echo "0")
    local rectangle_count=$(grep -c "RECTANGLE" termsrv.log || echo "0")
    
    if [ "$line_to_count" -gt 0 ]; then
        print_success "LINE_TO operations detected ($line_to_count)"
    else
        print_warning "No LINE_TO operations detected"
    fi
    
    if [ "$rectangle_count" -gt 0 ]; then
        print_success "RECTANGLE operations detected ($rectangle_count)"
    else
        print_warning "No RECTANGLE operations detected"
    fi
    
    # Check for any GDI message types
    local total_gdi=$(grep -c "MSG.*Received type" termsrv.log || echo "0")
    if [ "$total_gdi" -gt 0 ]; then
        print_success "Total GDI messages processed: $total_gdi"
    else
        print_error "No GDI messages detected - driver may not be intercepting calls"
        return 1
    fi
}

test_screenshot_generation() {
    print_header "Testing Screenshot Generation"
    
    # Wait a bit for potential screenshot generation
    print_info "Waiting ${SCREENSHOT_WAIT}s for screenshot generation..."
    sleep $SCREENSHOT_WAIT
    
    # Look for screenshot files
    local screenshot_files=$(find . -name "rds_screenshot_*.bmp" -o -name "rds_screenshot_*.png" 2>/dev/null | wc -l)
    
    if [ "$screenshot_files" -gt 0 ]; then
        print_success "Screenshot files generated ($screenshot_files found)"
        # Show file details
        find . -name "rds_screenshot_*" -exec ls -la {} \; 2>/dev/null | head -3
    else
        print_warning "No screenshot files found (may require longer runtime)"
    fi
}

test_performance_metrics() {
    print_header "Testing Performance Metrics"
    
    # Analyze ping response times from termsrv log
    local response_times=$(grep "response time:" termsrv.log | sed -n 's/.*response time: \([0-9]*\) ms.*/\1/p')
    
    if [ ! -z "$response_times" ]; then
        local total=0
        local count=0
        local max=0
        
        for time in $response_times; do
            total=$((total + time))
            count=$((count + 1))
            if [ "$time" -gt "$max" ]; then
                max=$time
            fi
        done
        
        if [ "$count" -gt 0 ]; then
            local avg=$((total / count))
            print_success "Ping response times - Avg: ${avg}ms, Max: ${max}ms, Count: $count"
            
            if [ "$avg" -lt 50 ]; then
                print_success "Average response time is excellent (< 50ms)"
            elif [ "$avg" -lt 100 ]; then
                print_info "Average response time is acceptable (< 100ms)"
            else
                print_warning "Average response time is high (>= 100ms)"
            fi
        fi
    else
        print_warning "No response time data found"
    fi
}

# Main test execution
main() {
    print_header "Wine RDS Automated Test Suite"
    print_info "Starting comprehensive test of Wine Remote Desktop Services"
    
    # Create test directory
    mkdir -p wine_rds_test_output
    cd wine_rds_test_output
    
    # Copy binaries to test directory for easier access
    cp ../programs/termsrv/termsrv . 2>/dev/null || true
    cp ../programs/rds_test/ping_test . 2>/dev/null || true
    
    # Run tests in sequence
    test_binaries_exist
    test_termsrv_startup
    
    # Give termsrv a moment to fully initialize
    sleep 2
    
    test_ping_connectivity
    test_gdi_interception
    test_screenshot_generation
    test_performance_metrics
    
    print_header "Test Summary"
    
    # Analyze overall results
    local total_errors=$(grep -c "❌" <<< "$(tail -50 termsrv.log ping_test.log 2>/dev/null || echo '')" || echo "0")
    local total_successes=$(grep -c "✅" <<< "$(tail -50 termsrv.log ping_test.log 2>/dev/null || echo '')" || echo "0")
    
    print_info "Log files generated:"
    print_info "  - termsrv.log (terminal server output)"
    print_info "  - ping_test.log (client test output)"
    
    if [ -f termsrv.log ]; then
        local termsrv_lines=$(wc -l < termsrv.log)
        print_info "  - termsrv.log has $termsrv_lines lines"
    fi
    
    print_success "Test suite completed successfully!"
    print_info "Review the logs above for any warnings or issues."
    
    # Return to original directory
    cd ..
}

# Run main function
main "$@"