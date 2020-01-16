/**
 * Copyright (c) 2020 Paul-Louis Ageneau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "addr.h"
#include "log.h"

socklen_t addr_get_len(const struct sockaddr *sa) {
	switch (sa->sa_family) {
	case AF_INET:
		return sizeof(struct sockaddr_in);
	case AF_INET6:
		return sizeof(struct sockaddr_in6);
	default:
		JLOG_WARN("Unknown address family %hu", sa->sa_family);
		return 0;
	}
}

uint16_t addr_get_port(const struct sockaddr *sa) {
	switch (sa->sa_family) {
	case AF_INET:
		return ntohs(((struct sockaddr_in *)sa)->sin_port);
	case AF_INET6:
		return ntohs(((struct sockaddr_in6 *)sa)->sin6_port);
	default:
		JLOG_WARN("Unknown address family %hu", sa->sa_family);
		return 0;
	}
}

int addr_set_port(struct sockaddr *sa, uint16_t port) {
	switch (sa->sa_family) {
	case AF_INET:
		((struct sockaddr_in *)sa)->sin_port = htons(port);
		return 0;
	case AF_INET6:
		((struct sockaddr_in6 *)sa)->sin6_port = htons(port);
		return 0;
	default:
		JLOG_WARN("Unknown address family %hu", sa->sa_family);
		return -1;
	}
}

bool addr_is_local(struct sockaddr *sa) {
	switch (sa->sa_family) {
	case AF_INET: {
		const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
		const uint8_t *b = (const uint8_t *)&sin->sin_addr;
		if (b[0] == 127) // loopback
			return true;
		if (b[0] == 169 && b[1] == 254) // link-local
			return true;
		return false;
	}
	case AF_INET6: {
		const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
		if (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr)) {
			return true;
		}
		if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
			return true;
		}
		if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
			const uint8_t *b = (const uint8_t *)&sin6->sin6_addr + 12;
			if (b[0] == 127) // loopback
				return true;
			if (b[0] == 169 && b[1] == 254) // link-local
				return true;
			return false;
		}
		return false;
	}
	default:
		return false;
	}
}

bool addr_is_temp_inet6(struct sockaddr *sa) {
	if (sa->sa_family != AF_INET6)
		return false;
	if (addr_is_local(sa))
		return false;
	const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
	const uint8_t *b = (const uint8_t *)&sin6->sin6_addr;
	return (b[8] & 0x02) ? false : true;
}

bool addr_unmap_inet6_v4mapped(struct sockaddr *sa, socklen_t *len) {
	if (sa->sa_family != AF_INET6)
		return false;

	const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
	if (!IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr))
		return false;

	struct sockaddr_in6 copy = *sin6;
	sin6 = &copy;

	struct sockaddr_in *sin = (struct sockaddr_in *)sa;
	sin->sin_family = AF_INET;
	sin->sin_port = sin6->sin6_port;
	memcpy(&sin->sin_addr, ((const uint8_t *)&sin6->sin6_addr) + 12, 4);
	*len = sizeof(*sin);
	return true;
}

int addr_resolve(const char *hostname, const char *service,
                 addr_record_t *records, size_t count) {
	addr_record_t *end = records + count;

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_ADDRCONFIG;
	struct addrinfo *ai_list = NULL;
	if (getaddrinfo(hostname, service, &hints, &ai_list))
		return -1;

	int ret = 0;
	for (struct addrinfo *ai = ai_list; ai; ai = ai->ai_next) {
		if (ai->ai_family == AF_INET || ai->ai_family == AF_INET6) {
			++ret;
			if (records != end) {
				memcpy(&records->addr, ai->ai_addr, ai->ai_addrlen);
				records->len = ai->ai_addrlen;
				++records;
			}
		}
	}

	freeaddrinfo(ai_list);
	return ret;
}