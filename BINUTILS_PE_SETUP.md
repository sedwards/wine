# Binutils PE Tools for Wine

## Overview

Custom-built GNU Binutils 2.44 with PE/PE+ support for Wine development on ARM64 macOS.

## Installation Location

**Path**: `~/wine-binutils` (`/Users/sedwards/wine-binutils`)

## Required PATH Setup

Add this to your shell configuration (`~/.zshrc` or `~/.bash_profile`):

```bash
export PATH="$HOME/wine-binutils/bin:$PATH"
```

Then reload your shell or run:
```bash
source ~/.zshrc
```

## Available Tools

- `aarch64-w64-mingw32-dlltool` - Create import libraries from .def files
- `aarch64-w64-mingw32-windres` - Compile Windows resource files (.rc)
- `aarch64-w64-mingw32-windmc` - Compile Windows message files
- `aarch64-w64-mingw32-as` - ARM64 assembler with PE/COFF support
- `aarch64-w64-mingw32-ld` - Linker with PE+ support
- `aarch64-w64-mingw32-ar` - Archive manager for .a libraries
- Plus: nm, objcopy, objdump, ranlib, strip, etc.

**Note**: Wine-compatible symlinks (e.g., `aarch64-windows-dlltool`) are also created.

## Verifying Installation

Check if tools are accessible:
```bash
which aarch64-w64-mingw32-dlltool
# Should output: /Users/sedwards/wine-binutils/bin/aarch64-w64-mingw32-dlltool

aarch64-w64-mingw32-dlltool --version
# Should output: GNU aarch64-w64-mingw32-dlltool (GNU Binutils) 2.44
```

## Using with Wine

Once PATH is set, rebuild Wine:
```bash
cd /Users/sedwards/source/wine
./configure --disable-win16 --enable-win64
make
```

Wine's build system will automatically detect and use these tools for:
- Building import libraries from .spec files
- Compiling Windows resources
- Linking PE executables and DLLs
- All PE binary operations

## Troubleshooting

If Wine can't find the tools:
1. Verify PATH is set: `echo $PATH | grep wine-binutils`
2. Check symlinks exist: `ls -la ~/wine-binutils/bin/aarch64-windows-*`
3. Ensure you've reloaded your shell after adding to PATH

---
*Built: 2025-06-15 | Target: aarch64-w64-mingw32 | Format: PE/PE+ ARM64*