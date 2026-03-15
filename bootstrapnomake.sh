#!/bin/sh

export FLEX="${FLEX:-flex}"
export BYACC="${BYACC:-byacc}"
export CC="${CC:-cc}"
export CFLAGS="${CFLAGS:--O3 -Wall -Wextra -Wno-unused-parameter -g}"
export LDFLAGS="${LDFLAGS:-}"

die()
{
  echo "$@"
  exit 1
}
doflex()
{
  if [ -e "$1" -a -e "$2" ]; then
    if which $FLEX > /dev/null; then
      return 0
    else
      echo "No flex but targets exist"
      return 1
    fi
  fi
  return 0
}
dobyacc()
{
  if [ -e "$1" -a -e "$2" ]; then
    if which $BYACC > /dev/null; then
      return 0
    else
      echo "No byacc but targets exist"
      return 1
    fi
  fi
  return 0
}

libobjs=""

for a in *.l; do
  base="`echo "$a"|sed 's/.l$//g'`"
  if doflex "$base.lex.c" "$base.lex.h"; then
    $FLEX --outfile="$base.lex.c" --header-file="$base.lex.h" "$a" || die "flex"
  fi
done
for a in *.y; do
  base="`echo "$a"|sed 's/.y$//g'`"
  if dobyacc "$base.tab.c" "$base.tab.h"; then
    $BYACC -d -p "$base" -b "$base" -o "$base.tab.c" "$a" || die "byacc"
  fi
done
for a in *.c; do
  base="`echo "$a"|sed 's/.c$//g'`"
  if grep -q '^int main(int' "$a"; then
    true
  else
    libobjs="$libobjs $base.o"
  fi
  $CC $CFLAGS -Wno-sign-compare -Wno-missing-prototypes -c -o "$base.o" "$a" || die "cc"
done

rm -f libabce.a
ar rvs libabce.a $libobjs || die "ar"
