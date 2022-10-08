#!/bin/sh
config_mk="$1"
config_log="$2"

echo -n "Checking if pthread is available..." | tee -a "$config_log"

if [ -z "$CXX" ]; then
	echo "CXX not defined" >&2
	exit 2
elif ! $CXX --version >/dev/null 2>&1; then
	echo "CXX not working" >&2
	exit 2
fi

tmpdir=`mktemp -d` || exit 2

cat > "$tmpdir"/test.cc <<EOF
#ifdef __leonbare__
# define SOLARIS_NP
# include <fsu_pthread.h>
#else
# include <pthread.h>
#endif

void * tmain(void * arg)
{
	return arg;
}

int main()
{
	pthread_t t;
	pthread_create(&t, NULL, tmain, NULL);
	pthread_mutex_t m;
	pthread_mutex_init(&m, NULL);
	return 0;
}
EOF

$CXX $CXXFLAGS -o "$tmpdir"/test "$tmpdir"/test.cc -lpthread >> "$config_log" 2>&1
ret=$?

rm -rf "$tmpdir"

if [ $ret -eq 0 ]; then
	echo "yes" | tee -a "$config_log"
	echo "HAVE_PTHREAD = 1" >> "$config_mk"
	exit 0
else
	echo "no" | tee -a "$config_log"
	echo "HAVE_PTHREAD = 0" >> "$config_mk"
	exit 0
fi
