#!/bin/sh

mydir=`dirname "$0"`

if [ -z "$1" ]; then
	echo "Usage: $0 <outputdir>" >&2
	exit 1
fi
outputdir="$1"

cat > "$outputdir"/config.mk <<EOF
# config.mk generated on `date`
# CXX=$CXX
# CXXFLAGS=$CXXFLAGS
EOF

cat > "$outputdir"/config.log <<EOF
# config.log generated on `date`
# CXX=$CXX
# CXXFLAGS=$CXXFLAGS
EOF

for script in "$mydir"/configure.d/check-*.sh; do
	"$script" "$outputdir"/config.mk "$outputdir"/config.log
done
