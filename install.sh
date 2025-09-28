#!/bin/sh

if [ '!' -x "amyplan" ]; then
  echo "amyplan not made"
  exit 1
fi

RELOGIN=0
ISLOCAL=0
PREFIX="$1"

if [ "a$PREFIX" = "a" ]; then
  PREFIX=~/.local
  ISLOCAL=1
  if [ '!' -d "$PREFIX" ]; then
    RELOGIN=1
  fi
  mkdir -p "$PREFIX"
fi

P="$PREFIX"
H="`hostname`"

if [ '!' -w "$P" ]; then
  echo "No write permissions to $P"
  exit 1
fi
if [ '!' -d "$P" ]; then
  echo "Not a valid directory: $P"
  exit 1
fi

instbin()
{
  if [ -e "$P/bin/$1" ]; then
    ln "$P/bin/$1" "$P/bin/.$1.abceinstold.$$.$H" || exit 1
  fi
  cp "$1" "$P/bin/.$1.abceinstnew.$$.$H" || exit 1
  mv "$P/bin/.$1.abceinstnew.$$.$H" "$P/bin/$1" || exit 1
  if [ -e "$P/bin/.$1.abceinstold.$$.$H" ]; then
    # If you mount binaries across NFS, and run this command on the NFS server,
    # you might want to comment out this rm command.
    rm "$P/bin/.$1.abceinstold.$$.$H" || exit 1
  fi
}

# Ensure bin directory is there
if [ "$ISLOCAL" = "1" ]; then
  if [ '!' -d "$P"/bin ]; then
    RELOGIN=1
  fi
fi
mkdir -p "$P/bin" || exit 1

# Install binary
instbin amyplan

echo "All done, abce has been installed to $P"
if [ "$RELOGIN" = "1" ]; then
  echo "As ~/.local/bin did not exist, you may need to re-log-in"
fi
