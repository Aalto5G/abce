#!/bin/sh

if [ '!' -x "amyplan" ]; then
  echo "amyplan not made"
  exit 1
fi

PREFIX="$1"

if [ "a$PREFIX" = "a" ]; then
  PREFIX=~/.local
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
mkdir -p "$P/bin" || exit 1

# Install binary
instbin amyplan

echo "All done, abce has been installed to $P"
