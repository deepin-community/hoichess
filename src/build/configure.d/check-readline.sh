#!/bin/sh
config_mk="$1"
config_log="$2"

echo -n "Checking if readline is available..." | tee -a "$config_log"

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
#include <readline/readline.h>
#include <readline/history.h>

int main()
{
	char * line = readline(">");
	add_history(line);
	fputs(line, stdout);
	return 0;
}
EOF

$CXX $CXXFLAGS -lreadline -o "$tmpdir"/test "$tmpdir"/test.cc >/dev/null >> "$config_log" 2>&1
ret=$?

rm -rf "$tmpdir"

if [ $ret -eq 0 ]; then
	echo "yes" | tee -a "$config_log"
	echo "HAVE_READLINE = 1" >> "$config_mk"
	exit 0
else
	echo "no" | tee -a "$config_log"
	echo "HAVE_READLINE = 0" >> "$config_mk"
	exit 0
fi
