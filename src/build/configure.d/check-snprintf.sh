#!/bin/sh
config_mk="$1"
config_log="$2"

echo -n "Checking if snprintf is available..." | tee -a "$config_log"

if [ -z "$CXX" ]; then
	echo "CXX not defined" >&2
	exit 2
elif ! $CXX --version >/dev/null 2>&1; then
	echo "CXX not working" >&2
	exit 2
fi

tmpdir=`mktemp -d` || exit 2

cat > "$tmpdir"/test.cc <<EOF
#include <stdio.h>

int main(int argc, char ** argv)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "foo%d", 1);
	va_list args;
	snprintf(buf, sizeof(buf), "foo%d", args);
	return 0;
}
EOF

$CXX $CXXFLAGS -o "$tmpdir"/test "$tmpdir"/test.cc >> "$config_log" 2>&1
ret=$?

rm -rf "$tmpdir"

if [ $ret -eq 0 ]; then
	echo "yes" | tee -a "$config_log"
	echo "HAVE_SNPRINTF = 1" >> "$config_mk"
	exit 0
else
	echo "no" | tee -a "$config_log"
	echo "HAVE_SNPRINTF = 0" >> "$config_mk"
	exit 0
fi
