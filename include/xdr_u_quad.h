/*
 * Copyright 2017 Scality
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @authors:
 *    Vianney Rancurel <vr@scality.com>
 */
#pragma once
/*
 *  missing XDR with gssrpc
 *
 *  WARNING: this function is already defined in the glibc but not exposed in xdr header.
 *  Puting this code in its own translation unit doesn't prevent the module to use the function
 *  from the glibc which expect xdrs->x_private to be a FILE* whereas gssrpc uses a RECSTREAM
 */
#include <gssrpc/rpc.h>

static inline bool_t
xdr_u_quad_t (XDR *xdrs, u_quad_t *ullp)
{
  long int t1, t2;

  if (xdrs->x_op == XDR_ENCODE)
    {
      t1 = (unsigned long) ((*ullp) >> 32);
      t2 = (unsigned long) (*ullp);
      return (XDR_PUTLONG(xdrs, &t1) && XDR_PUTLONG(xdrs, &t2));
    }

  if (xdrs->x_op == XDR_DECODE)
    {
      if (!XDR_GETLONG(xdrs, &t1) || !XDR_GETLONG(xdrs, &t2))
        return FALSE;
      *ullp = ((u_quad_t) t1) << 32;
      *ullp |= (uint32_t) t2;
      return TRUE;
    }

  if (xdrs->x_op == XDR_FREE)
    return TRUE;

  return FALSE;
}
