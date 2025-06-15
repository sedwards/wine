# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## **🚀 PREPARATION PHASE - Every Task Starts Here**

### 1. **Knowledge Base Review - MANDATORY FIRST STEP**
**ALWAYS start by reviewing these project documents:**
- `WINE_RDS_COMPREHENSIVE_GUIDE.md` - Complete project documentation
- `BROADWAY_README.md` - Broadway web interface specifics  
- This `CLAUDE.md` file for current project context
- Check for `// TODO:` markers in files - these get priority
- **Never assume** any file, class, or implementation exists - always verify

### 2. **Discovery Conversation Required**
- **No coding without discovery**: Clarify requirements, edge cases, integration points
- Establish success criteria and identify who implements what
- Understand how changes fit into existing Wine RDS architecture
- Verify impact on Broadway web interface if relevant

### 3. **Wine RDS Context Check**
- Understand whether work affects: winerds.drv, termsrv, Broadway, or testing
- Check current implementation status in comprehensive guide
- Verify integration points with named pipe protocol
- Consider impact on headless Wine operation

---

## **⚙️ DEVELOPMENT PHASE - Incremental Excellence**

### **Core Development Approach**
- **Small iterations**: 20-50 lines maximum per step
- **Production ready**: Every line must be deployment quality
- **Developer approval**: Get explicit approval before next increment
- **Integration focus**: Seamless with existing Wine RDS patterns

### **Wine RDS Code Quality Standards**
- **Comments**: Explain WHY, especially for Wine driver integration
- **Error handling**: Comprehensive for GDI calls, pipe communication, Broadway
- **Wine compatibility**: Follow Wine coding standards and debug channels
- **Testing approach**: Include validation method for each component

---

## **📋 PROJECT OVERVIEW**

### **Wine RDS (Remote Desktop Services)**
Custom implementation enabling Wine to run Windows applications headless with remote rendering through:

1. **winerds.drv** - Custom Wine graphics driver intercepting GDI calls
2. **termsrv** - Terminal services server processing drawing commands  
3. **Broadway** - Web interface for browser-based access
4. **Named pipe protocol** - Communication using `\\.\pipe\WineRDS`

### **Current Implementation Status** 
- ✅ **Core RDS**: GDI interception, named pipe communication, surface rendering
- ✅ **Broadway Web Interface**: HTTP server, shared memory framebuffer, HTML5 client
- 🔧 **In Progress**: WebSocket input handling for Broadway
- ⚠️ **Known Issues**: Driver registration, timing dependencies

---

## **🔧 ESSENTIAL COMMANDS**

### **Build Commands**
```bash
# Configure Wine with required options  
./configure --disable-win16 --enable-win64

# Build RDS components (includes Broadway)
make dlls/winerds.drv
make programs/termsrv  # Now includes Broadway server
make programs/rds_test

# Full rebuild
make clean && make
```

### **Testing Commands**
```bash
# Broadway-enabled testing (NEW)
./programs/termsrv/termsrv --broadway &
./programs/rds_test/ping_test
# Browser: http://localhost:8080

# Comprehensive Broadway test
./test_broadway_rds.sh

# Traditional RDS testing
export WINEDEBUG=+winerds,+termsrv,+termsrv_gdi,+broadway
./programs/termsrv/termsrv &
./programs/rds_test/ping_test
```

### **Development Workflow**
1. **Start termsrv** (creates named pipe server, optionally Broadway)
2. **Run test applications** that create display contexts
3. **Monitor outputs**: Screenshots (`rds_screenshot_*.bmp/png`), `termsrv.log`
4. **Test Broadway**: Web interface at `http://localhost:8080` if enabled

---

## **🏗️ ARCHITECTURE OVERVIEW**

```
Windows Application
    ↓ GDI calls (CreateDC, LineTo, etc.)
Wine GDI Subsystem  
    ↓ routes to registered driver
winerds.drv Graphics Driver
    ↓ serializes GDI operations
Named Pipe (\\.\pipe\WineRDS)
    ↓ receives and processes messages
termsrv Terminal Server
    ↓ renders to virtual surface + updates shared framebuffer
    ├── Screenshot Output (BMP/PNG)
    └── Broadway Web Server (HTTP:8080 + WebSocket:8765)
            ↓ serves framebuffer + handles input
        Web Browser Client (Canvas + WebSocket)
```

### **Key Architecture Notes**
- **Headless operation**: No X11/Wayland dependencies
- **800x600x32bpp**: Standard surface resolution  
- **RDS_MESSAGE protocol**: Custom message format over named pipes
- **Shared memory framebuffer**: Windows file mapping for Broadway (`Global\winerds_framebuffer`)
- **Real-time updates**: Broadway framebuffer updates on every GDI operation

---

## **📁 KEY FILES & COMPONENTS**

### **Driver Layer** (`dlls/winerds.drv/`)
- `gdi_funcs.c` - GDI function implementations and interception
- `pipe_client.c` - Named pipe client communication
- `rdsgdi_driver.h` - Driver definitions and structures
- `winerds.drv.spec` - Wine DLL specification

### **Terminal Services** (`programs/termsrv/`)
- `rds.c` - Main service logic (Broadway integration added)
- `broadway_server.h/c` - **NEW**: HTTP/WebSocket server implementation
- `pipe_server.c` - Named pipe server
- `rds_gdi_handlers.c` - GDI message processing (Broadway framebuffer updates added)
- `rds_surface_drawing.c` - Surface drawing operations

### **Testing & Documentation**
- `programs/rds_test/ping_test.c` - Basic connectivity testing
- `test_broadway_rds.sh` - **NEW**: Broadway integration testing
- `WINE_RDS_COMPREHENSIVE_GUIDE.md` - **UPDATED**: Complete documentation
- `BROADWAY_README.md` - **NEW**: Broadway-specific documentation

---

## **🎯 CURRENT PRIORITIES & TODOS**

### **High Priority**
1. **Complete WebSocket input handling** in Broadway server
2. **Fix driver registration issues** for automatic Wine integration
3. **Improve error handling** across all components

### **Medium Priority** 
1. **Broadway security enhancements** (HTTPS, authentication)
2. **Performance optimization** (framebuffer compression, delta updates)
3. **Extended GDI support** (additional drawing operations)

### **Check for TODOs**
- Search files for `// TODO:` markers before starting work
- Prioritize existing TODOs over new features
- Ask for clarification on TODO requirements

---

## **📞 COMMUNICATION STANDARDS**

### **Thread Summaries Required**
- **Every 8-12 messages** or at major milestones
- **Format**: Progress update, current status, next steps, Wine RDS impact
- **Purpose**: Maintain context for handoffs and integration

### **Decision Documentation**
- Record technical decisions affecting Wine RDS architecture
- Note Broadway integration points and dependencies  
- Track TODO completions and their outcomes
- Document Wine compatibility considerations

### **Response Guidelines**
1. **Knowledge base first**: Always check comprehensive guide
2. **No code without discovery**: Complete discovery phase before implementation
3. **Small increments**: 20-50 lines with explicit approval gates
4. **Focus on WHY**: Explain rationale, especially for Wine driver integration

---

## **🔍 DEBUGGING & VALIDATION**

### **Debug Channels**
```bash
export WINEDEBUG=+winerds,+termsrv,+termsrv_gdi,+broadway
```

### **Validation Methods**
- **Screenshots**: Verify rendering in generated BMP/PNG files
- **Broadway**: Test web interface at `http://localhost:8080`
- **Pipe communication**: Monitor `termsrv.log` for message flow
- **Browser console**: Check for WebSocket/Canvas errors

### **Integration Testing**
- Use `./test_broadway_rds.sh` for comprehensive validation
- Test both traditional RDS and Broadway functionality
- Verify Wine compatibility and headless operation

---

## **🎯 KEY PRINCIPLES FOR WINE RDS WORK**

1. **Knowledge First** - Review comprehensive guide before suggesting solutions
2. **Never Assume** - Always verify through codebase exploration  
3. **TODO Priority** - Address existing TODOs before new features
4. **Incremental Excellence** - Small, perfect steps over large implementations
5. **Production Ready** - Every line must be Wine deployment quality
6. **Broadway Integration** - Consider web interface impact for all changes
7. **Wine Compatibility** - Follow Wine coding standards and patterns

---

**Last Updated**: 2025-06-15  
**Configuration Version**: 2.0 (with new Claude rules + Broadway integration)  
**Branch**: winerds-gdi-pipe-prototype  
**Status**: Broadway web interface integrated, WebSocket input pending