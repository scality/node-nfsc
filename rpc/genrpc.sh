#!/bin/bash

FILE="$1"
DIR="$(dirname "$FILE")"
cd "$DIR"

rpcgen -h -M "$(basename "$FILE")" |
    sed 's,rpc/rpc.h,gssrpc/rpc.h,g' > "$(basename "$FILE" .x).h"
rpcgen -l -M "$(basename "$FILE")" > "$(basename "$FILE" .x)_clnt.c"

echo '#include <xdr_u_quad.h>' > "$(basename "$FILE" .x)_xdr.c"
rpcgen -c -M "$(basename "$FILE")" >> "$(basename "$FILE" .x)_xdr.c"

echo "$DIR/$(basename "$FILE" .x)_clnt.c"
echo "$DIR/$(basename "$FILE" .x)_xdr.c"
