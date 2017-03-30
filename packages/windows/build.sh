#!/bin/sh

LANG=C

run()
{
  "$@"
  if test $? -ne 0; then
    echo "Failed $@"
    exit 1
  fi
}

cd "$(dirname $0)"

. tmp/env.sh

for architecture in ${ARCHITECTURES}; do
  run rake build                                        \
      TMP_DIR="/tmp"                                    \
      VERSION="${VERSION}"                              \
      SOURCE="${SOURCE}"                                \
      DEBUG_BUILD="${DEBUG_BUILD}"                      \
      MEMORY_DEBUG_BUILD="${MEMORY_DEBUG_BUILD}"        \
      ARCHITECTURE="${architecture}"
done
