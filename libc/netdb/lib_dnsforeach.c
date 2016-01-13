/****************************************************************************
 * libc/netdb/lib_dnsforeach.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/net/dns.h>

#include "netdb/lib_dns.h"

#ifdef CONFIG_NETDB_DNSCLIENT

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR char *skip_spaces(FAR char *ptr)
{
  while (isspace(*ptr)) ptr++;
  return ptr;
}

static FAR char *find_spaces(FAR char *ptr)
{
  while (isspace(*ptr)) ptr++;
  return ptr;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dns_foreach_nameserver
 *
 * Description:
 *   Traverse each nameserver entry in the resolv.conf file and perform the
 *   the provided callback.
 *
 ****************************************************************************/

#ifdef CONFIG_NETDB_RESOLVCONF

int dns_foreach_nameserver(dns_callback_t callback, FAR void *arg)
{
  union dns_server_u u;
  FAR FILE *stream;
  char line[DNS_MAX_LINE];
  FAR char *ptr;
  int keylen;
  int ret;

  /* Open the resolver configuration file */

  stream = fopen(CONFIG_NETDB_RESOLVCONF, "rb");
  if (stream == NULL)
    {
      int errcode = errno;
      ndbg("ERROR: Failed to open %s: %d\n",
        CONFIG_NETDB_RESOLVCONF, errcode);
      DEBUGASSERT(errcode > 0);
      return -errcode;
    }

  keylen = strlen(NETDB_DNS_KEYWORD);
  while (fgets(line, DNS_MAX_LINE, stream) != NULL)
    {
      ptr = skip_spaces(line);
      if (strncmp(ptr, NETDB_DNS_KEYWORD, keylen) == 0)
        {
          socklen_t copylen;

          /* REVISIT:  We really need a customizable port number. The
           * OpenBSD version supports a [host]:port syntax.  When a
           * non-standard port is specified the host address must be
           * enclosed in square brackets.  For example:
           *
           *   nameserver [10.0.0.1]:5353
           *   nameserver [::1]:5353
           */

#ifdef CONFIG_NET_IPv4
          /* Try to convert the IPv4 address */

          ret = inet_pton(AF_INET, ptr, &u.ipv4.sin_addr);

          /* The inet_pton() function returns 1 if the conversion succeeds */

          if (ret == 1)
            {
              /* REVISIT:  We really need a customizable port number */

              u.ipv4.sin_family = AF_INET;
              u.ipv4.sin_port   = DNS_DEFAULT_PORT;
              ret = callback(arg, AF_INET, &u.ipv4, sizeof(struct sockaddr_in));
            }
          else
#endif
#ifdef CONFIG_NET_IPv6
            {
              /* Try to convert the IPv6 address */

              ret = inet_pton(AF_INET6, ptr, &u.ipv6.sin6_addr);

              /* The inet_pton() function returns 1 if the conversion
               * succeeds.
               */

              if (ret == 1)
                {
                  /* REVISIT:  We really need a customizable port number */

                  u.ipv6.sin6_family = AF_INET6;
                  u.ipv6.sin6_port   = DNS_DEFAULT_PORT;
                  ret = callback(arg, &u.ipv6, sizeof(struct sockaddr_in6));
                }
              else
#endif
                {
                  ndbg("ERROR: Unrecognized address: %s\n", ptr)
                  ret = OK;
                }
#ifdef CONFIG_NET_IPv6
            }
#endif
          if (ret != OK)
            {
              fclose(stream);
              return ret;
            }
        }
    }

  fclose(stream);
  return OK;
}

#else /* CONFIG_NETDB_RESOLVCONF */

int dns_foreach_nameserver(dns_callback_t callback, FAR void *arg)
{
  int ret = OK;

  if (g_dns_address)
    {
#ifdef CONFIG_NET_IPv4
      /* Check for an IPv4 address */

      if (g_dns_server.addr.sa_family == AF_INET)
        {
          /* Perform the callback */

          ret = callback(arg, &g_dns_server.ipv4, sizeof(struct sockaddr_in);
        }
      else
#endif

#ifdef CONFIG_NET_IPv6
      /* Check for an IPv6 address */

      if (g_dns_server.addr.sa_family == AF_INET6)
        {
          /* Perform the callback */

          ret = callback(arg, &g_dns_server.ipv6, sizeof(struct sockaddr_in6);
        }
      else
#endif
        {
          nvdbg("ERROR: Unsupported family: %d\n",
                g_dns_server.addr.sa_family);
          ret = -ENOSYS;
        }
    }

  return ret;
}

#endif /* CONFIG_NETDB_RESOLVCONF */
#endif /* CONFIG_NETDB_DNSCLIENT */
