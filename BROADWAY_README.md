# Wine RDS Broadway Integration

## Overview

The Broadway integration adds web-based remote desktop capabilities to Wine RDS, allowing Wine applications to be accessed through a web browser in real-time. This implementation provides a modern, platform-independent way to interact with Wine applications remotely.

## Features

- **Web-based Client**: Access Wine applications through any modern web browser
- **Real-time Framebuffer**: Live display updates at 10 FPS
- **Shared Memory**: Efficient framebuffer sharing using Windows file mapping
- **Mouse & Keyboard Support**: WebSocket-based input handling (basic implementation)
- **No Additional Dependencies**: Built into the termsrv executable

## Architecture

```
Wine Application
    ↓ GDI calls
winerds.drv → Named Pipe → termsrv
    ↓                        ↓
Screenshots              Broadway Server
                              ↓
                         HTTP Server (port 8080)
                         WebSocket Server (port 8765)
                              ↓
                         Web Browser Client
```

## Usage

### Starting Broadway-enabled Terminal Service

```bash
# Enable Broadway web interface
./programs/termsrv/termsrv --broadway

# Use custom port (default: 8080)
./programs/termsrv/termsrv --broadway --broadway-port=9000

# Show help
./programs/termsrv/termsrv --help
```

### Accessing the Web Interface

1. Start termsrv with Broadway enabled
2. Open your web browser to: `http://localhost:8080`
3. The Wine RDS framebuffer will be displayed in the browser
4. Mouse clicks and keyboard input are forwarded to Wine applications

### Testing the Integration

```bash
# Run comprehensive test
./test_broadway_rds.sh

# Manual testing
./programs/termsrv/termsrv --broadway &
./programs/rds_test/ping_test
# Then open http://localhost:8080 in browser
```

## Implementation Details

### Components

1. **broadway_server.h/c**: HTTP/WebSocket server implementation
2. **Shared Framebuffer**: Windows file mapping for efficient data sharing
3. **HTML5 Client**: Embedded web client with Canvas rendering
4. **Input Bridge**: WebSocket-based mouse/keyboard event forwarding

### Network Ports

- **8080**: HTTP server for web client and framebuffer data
- **8765**: WebSocket server for input events (planned)

### Framebuffer Format

- **Size**: 800x600 pixels
- **Format**: RGBA32 (4 bytes per pixel)
- **Update Rate**: 10 FPS (100ms intervals)
- **Shared Memory**: `Global\winerds_framebuffer`

## Configuration

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--broadway` | Enable Broadway web interface | Disabled |
| `--broadway-port=PORT` | Set HTTP server port | 8080 |
| `--help` | Show help message | - |

### Environment Variables

None currently. Configuration is handled via command line arguments.

## Development Status

### ✅ Completed Features

- HTTP server with embedded HTML client
- Shared memory framebuffer
- Real-time display updates
- Integration with RDS GDI handlers
- Basic WebSocket server framework
- Command line configuration

### 🔧 In Progress

- WebSocket input handling implementation
- Mouse and keyboard event injection
- Connection state management

### 📋 Planned Features

- Secure WebSocket (WSS) support
- Multi-session support
- User authentication
- Session management web interface
- Mobile device optimization
- Performance monitoring

## Browser Compatibility

### Tested Browsers

- Chrome/Chromium 90+
- Firefox 88+
- Safari 14+
- Edge 90+

### Required Browser Features

- HTML5 Canvas
- WebSocket API
- Fetch API
- ES6 JavaScript

## Performance Considerations

### Framebuffer Updates

- **Update Rate**: 10 FPS (adjustable in JavaScript)
- **Bandwidth**: ~19MB/s at 10 FPS (uncompressed)
- **Memory Usage**: ~2MB for 800x600 RGBA framebuffer

### Optimization Opportunities

- Delta compression for framebuffer updates
- WebSocket binary frames for input events
- Configurable update rates
- Dirty region tracking

## Troubleshooting

### Common Issues

1. **Web interface not accessible**:
   - Check if termsrv started with `--broadway` flag
   - Verify port 8080 is not in use
   - Check firewall settings

2. **Blank or black screen**:
   - Ensure Wine applications are creating display contexts
   - Check if screenshots are being generated
   - Verify framebuffer is being updated

3. **No input response**:
   - WebSocket input handling is basic/incomplete
   - Check browser developer console for errors
   - Verify WebSocket connection on port 8765

### Debug Information

Enable debug output:
```bash
export WINEDEBUG=+termsrv,+broadway
./programs/termsrv/termsrv --broadway
```

Check generated files:
```bash
ls -la rds_screenshot_*.{bmp,png}
```

### Network Testing

Test HTTP server:
```bash
curl -I http://localhost:8080
```

Test WebSocket port:
```bash
nc -z localhost 8765 && echo "WebSocket port open"
```

## Security Considerations

### Current Implementation

- No authentication required
- HTTP only (no HTTPS)
- Local access only (binds to localhost)
- No input validation on WebSocket messages

### Production Recommendations

- Implement user authentication
- Use HTTPS/WSS for secure connections
- Add input validation and rate limiting
- Consider firewall rules for network access
- Implement session timeouts

## Integration with Existing RDS

Broadway integration is designed to be:

- **Non-intrusive**: Disabled by default, no impact when not used
- **Complementary**: Works alongside existing named pipe communication
- **Optional**: termsrv functions normally without Broadway
- **Backwards Compatible**: All existing RDS functionality preserved

## File Structure

```
programs/termsrv/
├── broadway_server.h          # Broadway server interface
├── broadway_server.c          # HTTP/WebSocket implementation
├── rds.c                     # Modified with Broadway integration
├── rds_gdi_handlers.c        # Modified with framebuffer updates
└── Makefile.in               # Updated with Broadway dependencies

test_broadway_rds.sh          # Integration test script
BROADWAY_README.md            # This documentation
```

## Contributing

When working on Broadway features:

1. Test with `./test_broadway_rds.sh`
2. Verify browser compatibility
3. Check for memory leaks in long-running sessions
4. Update documentation for new features
5. Consider security implications

---

**Last Updated**: 2025-06-15  
**Status**: Alpha - Basic functionality working, input handling in development  
**Platform**: Wine on Unix/Linux/macOS