#!/bin/sh

export CC="${CC:-cc}"
export CFLAGS="${CFLAGS:--O3 -Wall -g}"
export LDFLAGS="${LDFLAGS:-}"

die()
{
  echo "$@"
  exit 1
}

libobjs=""

for a in *.l; do
  base="`echo "$a"|sed 's/.l$//g'`"
  flex --outfile="$base.lex.c" --header-file="$base.lex.h" "$a" || die "flex"
done
for a in *.y; do
  base="`echo "$a"|sed 's/.y$//g'`"
  byacc -d -p "$base" -b "$base" -o "$base.tab.c" "$a" || die "byacc"
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
