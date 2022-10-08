#!/bin/sh
config_mk="$1"
config_log="$2"

echo -n "Checking if strtok_r is available..." | tee -a "$config_log"

if [ -z "$CXX" ]; then
	echo "CXX not defined" >&2
	exit 2
elif ! $CXX --version >/dev/null 2>&1; then
	echo "CXX not working" >&2
	exit 2
fi

tmpdir=`mktemp -d` || exit 2

cat > "$tmpdir"/test.cc <<EOF
#include <string.h>

int main(int argc, char ** argv)
{
	char * p;
	strtok_r(argv[0], " ", &p);
	return 0;
}
EOF

$CXX $CXXFLAGS -o "$tmpdir"/test "$tmpdir"/test.cc >> "$config_log" 2>&1
ret=$?

rm -rf "$tmpdir"

if [ $ret -eq 0 ]; then
	echo "yes" | tee -a "$config_log"
	echo "HAVE_STRTOK_R = 1" >> "$config_mk"
	exit 0
else
	echo "no" | tee -a "$config_log"
	echo "HAVE_STRTOK_R = 0" >> "$config_mk"
	exit 0
fi
