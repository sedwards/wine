#!/bin/bash
# wine_session_manager.sh - Launches isolated Wine sessions with unique shared buffers

# Example: ./wine_session_manager.sh user1 notepad.exe

SESSION_NAME="$1"
shift
CMDLINE="$@"

if [[ -z "$SESSION_NAME" || -z "$CMDLINE" ]]; then
  echo "Usage: $0 <session_name> <wine_program>"
  exit 1
fi

export WINERDS_SESSION="$SESSION_NAME"
export WINEPREFIX="$HOME/.wine_sessions/$SESSION_NAME"
mkdir -p "$WINEPREFIX"

wine explorer /desktop=$SESSION_NAME,800x600 $CMDLINE

