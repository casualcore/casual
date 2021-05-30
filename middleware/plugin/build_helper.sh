set -o nounset
set -o errexit

pushd "$(dirname $(readlink -f ${0}))" > /dev/null

mkdir bin 2>/dev/null || :

NAME=helper       # library name
VERSION=0.1.0     # library version suffix
SONAME=0unstable  # library link reference

DEFINES="-DEXPORT=1" # sets __attribue__ ((visibility="default")) in .h-file
WARN_FLAGS="-Wall -Werror -pedantic"
COMPILER_FLAGS="${DEFINES} ${WARN_FLAGS} -pthread \
  -fPIC -fvisibility=hidden -O0"

# compile
g++ -c ${COMPILER_FLAGS} -o helper.o helper.cpp

# link
g++ -shared -fPIC \
  -o bin/lib${NAME}.so.${VERSION} \
  -Wl,-soname,lib${NAME}.so.${SONAME} \
  helper.o \
  -I"${CASUAL_BUILD_HOME}/middleware/xatmi/include" \
  -I"${CASUAL_BUILD_HOME}/middleware/http/include" \
  -L"${CASUAL_BUILD_HOME}/middleware/xatmi/bin" \
  -L"${CASUAL_BUILD_HOME}/middleware/http/bin" \
  -Wl,--fatal-warnings,--no-undefined,--no-allow-shlib-undefined,-rpath-link=lib \
  -Wl,-z,origin -Wl,-rpath,'$ORIGIN' \
  -Wl,-rpath=$CASUAL_BUILD_HOME/middleware/xatmi/bin \
  -Wl,-rpath=$CASUAL_BUILD_HOME/middleware/http/bin \
  -Wl,-rpath=$CASUAL_BUILD_HOME/middleware/common/bin \
  -pthread \
  -lcasual-xatmi \
  -lcasual-http-inbound-common

# strip --strip-all bin/lib${NAME}.so.${VERSION}

# create symbolic links
pushd bin > /dev/null
ln -sf lib${NAME}.so.${VERSION} lib${NAME}.so.${SONAME}   # SONAME-link to file
ln -sf lib${NAME}.so.${SONAME} lib${NAME}.so              # NAME-link to SONAME
