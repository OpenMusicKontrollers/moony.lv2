/*
 * Copyright (c) 2015-2016 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#ifndef LV2_OSC_STREAM_H
#define LV2_OSC_STREAM_H

#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <osc.lv2/osc.h>

#if !defined(LV2_OSC_STREAM_SNDBUF)
#	define LV2_OSC_STREAM_SNDBUF 0x100000 // 1 M
#endif

#if !defined(LV2_OSC_STREAM_RCVBUF)
#	define LV2_OSC_STREAM_RCVBUF 0x100000 // 1 M
#endif

#if !defined(LV2_OSC_STREAM_REQBUF)
#	define LV2_OSC_STREAM_REQBUF 1024
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void *
(*LV2_OSC_Stream_Write_Request)(void *data, size_t minimum, size_t *maximum);

typedef void
(*LV2_OSC_Stream_Write_Advance)(void *data, size_t written);

typedef const void *
(*LV2_OSC_Stream_Read_Request)(void *data, size_t *toread);

typedef void
(*LV2_OSC_Stream_Read_Advance)(void *data);

typedef struct _LV2_OSC_Address LV2_OSC_Address;
typedef struct _LV2_OSC_Driver LV2_OSC_Driver;
typedef struct _LV2_OSC_Stream LV2_OSC_Stream;

struct _LV2_OSC_Address {
	socklen_t len;
	union {
		struct sockaddr in;
		struct sockaddr_in in4;
		struct sockaddr_in6 in6;
	};
};

struct _LV2_OSC_Driver {
	LV2_OSC_Stream_Write_Request write_req;
	LV2_OSC_Stream_Write_Advance write_adv;
	LV2_OSC_Stream_Read_Request read_req;
	LV2_OSC_Stream_Read_Advance read_adv;
};

struct _LV2_OSC_Stream {
	int socket_family;
	int socket_type;
	int protocol;
	bool server;
	bool slip;
	int sock;
	int fd;
	LV2_OSC_Address self;
	LV2_OSC_Address peer;
	const LV2_OSC_Driver *driv;
	void *data;
	uint8_t tx_buf [8092];
	uint8_t rx_buf [8092];
	size_t rx_off;
};

typedef enum _LV2_OSC_Enum {
	LV2_OSC_NONE = (0 << 0),
	LV2_OSC_SEND = (1 << 0),
	LV2_OSC_RECV = (1 << 1)
} LV2_OSC_Enum;

static const char *udp_prefix = "osc.udp://";
static const char *tcp_prefix = "osc.tcp://";
static const char *tcp_slip_prefix = "osc.slip.tcp://";
static const char *tcp_prefix_prefix = "osc.prefix.tcp://";
//FIXME serial

static int
lv2_osc_stream_init(LV2_OSC_Stream *stream, const char *url,
	const LV2_OSC_Driver *driv, void *data)
{
	memset(stream, 0x0, sizeof(LV2_OSC_Stream));

	char *dup = strdup(url);
	if(!dup)
	{
		fprintf(stderr, "%s: out-of-memory\n", __func__);
		goto fail;
	}

	char *ptr = dup;
	char *tmp;

	if(strncmp(ptr, udp_prefix, strlen(udp_prefix)) == 0)
	{
		stream->slip = false;
		stream->socket_family = AF_INET;
		stream->socket_type = SOCK_DGRAM;
		stream->protocol = IPPROTO_UDP;
		ptr += strlen(udp_prefix);
	}
	else if(strncmp(ptr, tcp_prefix, strlen(tcp_prefix)) == 0)
	{
		stream->slip = true;
		stream->socket_family = AF_INET;
		stream->socket_type = SOCK_STREAM;
		stream->protocol = IPPROTO_TCP;
		ptr += strlen(tcp_prefix);
	}
	else if(strncmp(ptr, tcp_slip_prefix, strlen(tcp_slip_prefix)) == 0)
	{
		stream->slip = true;
		stream->socket_family = AF_INET;
		stream->socket_type = SOCK_STREAM;
		stream->protocol = IPPROTO_TCP;
		ptr += strlen(tcp_slip_prefix);
	}
	else if(strncmp(ptr, tcp_prefix_prefix, strlen(tcp_prefix_prefix)) == 0)
	{
		stream->slip = false;
		stream->socket_family = AF_INET;
		stream->socket_type = SOCK_STREAM;
		stream->protocol = IPPROTO_TCP;
		ptr += strlen(tcp_prefix_prefix);
	}
	else
	{
		fprintf(stderr, "%s: invalid protocol\n", __func__);
		goto fail;
	}

	if(ptr[0] == '\0')
	{
		fprintf(stderr, "%s: URI has no colon\n", __func__);
		goto fail;
	}

	const char *node = NULL;
	const char *iface = NULL;
	const char *service = NULL;

	char *colon = strrchr(ptr, ':');

	// optional IPv6
	if(ptr[0] == '[')
	{
		stream->socket_family = AF_INET6;
		++ptr;
	}

	node = ptr;

	// optional IPv6
	if( (tmp = strchr(ptr, '%')) )
	{
		if(stream->socket_family != AF_INET6)
		{
			fprintf(stderr, "%s: no IPv6 interface delimiter expected here\n", __func__);
			goto fail;
		}

		ptr = tmp;
		ptr[0] = '\0';
		iface = ++ptr;
	}

	// optional IPv6
	if( (tmp = strchr(ptr, ']')) )
	if(ptr)
	{
		if(stream->socket_family != AF_INET6)
		{
			fprintf(stderr, "%s: no closing IPv6 bracket expected here\n", __func__);
			goto fail;
		}

		ptr = tmp;
		ptr[0] = '\0';
		++ptr;
	}

	// mandatory IPv4/6
	ptr = strchr(ptr, ':');
	if(!ptr)
	{
		fprintf(stderr, "%s: pre-service colon expected\n", __func__);
		goto fail;
	}

	ptr[0] = '\0';

	service = ++ptr;

	if(strlen(node) == 0)
	{
		node = NULL;
		stream->server = true;
	}

	stream->sock = socket(stream->socket_family, stream->socket_type,
		stream->protocol);

	if(stream->sock < 0)
	{
		fprintf(stderr, "%s: socket failed\n", __func__);
		goto fail;
	}

	if(fcntl(stream->sock, F_SETFL, O_NONBLOCK) == -1)
	{
		fprintf(stderr, "%s: fcntl failed\n", __func__);
		goto fail;
	}

	const int sendbuff = LV2_OSC_STREAM_SNDBUF;
	const int recvbuff = LV2_OSC_STREAM_RCVBUF;

	if(setsockopt(stream->sock, SOL_SOCKET,
		SO_SNDBUF, &sendbuff, sizeof(int))== -1)
	{
		fprintf(stderr, "%s: setsockopt failed\n", __func__);
		goto fail;
	}

	if(setsockopt(stream->sock, SOL_SOCKET,
		SO_RCVBUF, &recvbuff, sizeof(int))== -1)
	{
		fprintf(stderr, "%s: setsockopt failed\n", __func__);
		goto fail;
	}

	stream->driv = driv;
	stream->data = data;

	if(stream->socket_family == AF_INET) // IPv4
	{
		if(stream->server)
		{
			// resolve self address
			struct addrinfo hints;
			memset(&hints, 0x0, sizeof(struct addrinfo));
			hints.ai_family = stream->socket_family;
			hints.ai_socktype = stream->socket_type;
			hints.ai_protocol = stream->protocol;

			struct addrinfo *res;
			if(getaddrinfo(node, service, &hints, &res) != 0)
			{
				fprintf(stderr, "%s: getaddrinfo failed\n", __func__);
				goto fail;
			}
			if(res->ai_addrlen != sizeof(stream->peer.in4))
			{
				fprintf(stderr, "%s: IPv4 address expected\n", __func__);
				goto fail;
			}

			stream->self.len = res->ai_addrlen;
			stream->self.in = *res->ai_addr;
			stream->self.in4.sin_addr.s_addr = htonl(INADDR_ANY);

			freeaddrinfo(res);

			if(bind(stream->sock, &stream->self.in, stream->self.len) != 0)
			{
				fprintf(stderr, "%s: bind failed\n", __func__);
				goto fail;
			}
		}
		else // client
		{
			stream->self.len = sizeof(stream->self.in4);
			stream->self.in4.sin_family = stream->socket_family;
			stream->self.in4.sin_port = htons(0);
			stream->self.in4.sin_addr.s_addr = htonl(INADDR_ANY);

			if(bind(stream->sock, &stream->self.in, stream->self.len) != 0)
			{
				fprintf(stderr, "%s: bind failed\n", __func__);
				goto fail;
			}

			// resolve peer address
			struct addrinfo hints;
			memset(&hints, 0x0, sizeof(struct addrinfo));
			hints.ai_family = stream->socket_family;
			hints.ai_socktype = stream->socket_type;
			hints.ai_protocol = stream->protocol;

			struct addrinfo *res;
			if(getaddrinfo(node, service, &hints, &res) != 0)
			{
				fprintf(stderr, "%s: getaddrinfo failed\n", __func__);
				goto fail;
			}
			if(res->ai_addrlen != sizeof(stream->peer.in4))
			{
				fprintf(stderr, "%s: IPv4 address failed\n", __func__);
				goto fail;
			}

			stream->peer.len = res->ai_addrlen;
			stream->peer.in = *res->ai_addr;

			freeaddrinfo(res);
		}

		if(stream->socket_type == SOCK_DGRAM)
		{
			const int broadcast = 1;

			if(setsockopt(stream->sock, SOL_SOCKET, SO_BROADCAST,
				&broadcast, sizeof(broadcast)) != 0)
			{
				fprintf(stderr, "%s: setsockopt failed\n", __func__);
				goto fail;
			}

			//FIXME handle multicast
		}
		else if(stream->socket_type == SOCK_STREAM)
		{
			const int flag = 1;

			if(setsockopt(stream->sock, stream->protocol,
				TCP_NODELAY, &flag, sizeof(int)) != 0)
			{
				fprintf(stderr, "%s: setsockopt failed\n", __func__);
				goto fail;
			}

			if(setsockopt(stream->sock, SOL_SOCKET,
				SO_KEEPALIVE, &flag, sizeof(int)) != 0)
			{
				fprintf(stderr, "%s: setsockopt failed\n", __func__);
				goto fail;
			}

			if(stream->server)
			{
				if(listen(stream->sock, 1) != 0)
				{
					fprintf(stderr, "%s: listen failed\n", __func__);
					goto fail;
				}
			}
			else // client
			{
				connect(stream->sock, &stream->peer.in, stream->peer.len);
			}
		}
		else
		{
			fprintf(stderr, "%s: invalid socket type\n", __func__);
			goto fail;
		}
	}
	else if(stream->socket_family == AF_INET6) // IPv6
	{
		if(stream->server)
		{
			// resolve self address
			struct addrinfo hints;
			memset(&hints, 0x0, sizeof(struct addrinfo));
			hints.ai_family = stream->socket_family;
			hints.ai_socktype = stream->socket_type;
			hints.ai_protocol = stream->protocol;

			struct addrinfo *res;
			if(getaddrinfo(node, service, &hints, &res) != 0)
			{
				fprintf(stderr, "%s: getaddrinfo failed\n", __func__);
				goto fail;
			}
			if(res->ai_addrlen != sizeof(stream->peer.in6))
			{
				fprintf(stderr, "%s: IPv6 address expected\n", __func__);
				goto fail;
			}

			stream->self.len = res->ai_addrlen;
			stream->self.in = *res->ai_addr;
			stream->self.in6.sin6_addr = in6addr_any;
			if(iface)
			{
				stream->self.in6.sin6_scope_id = if_nametoindex(iface);
			}

			freeaddrinfo(res);

			if(bind(stream->sock, &stream->self.in, stream->self.len) != 0)
			{
				fprintf(stderr, "%s: bind failed\n", __func__);
				goto fail;
			}
		}
		else // client
		{
			stream->self.len = sizeof(stream->self.in6);
			stream->self.in6.sin6_family = stream->socket_family;
			stream->self.in6.sin6_port = htons(0);
			stream->self.in6.sin6_addr = in6addr_any;
			if(iface)
			{
				stream->self.in6.sin6_scope_id = if_nametoindex(iface);
			}

			if(bind(stream->sock, &stream->self.in, stream->self.len) != 0)
			{
				fprintf(stderr, "%s: bind failed\n", __func__);
				goto fail;
			}

			// resolve peer address
			struct addrinfo hints;
			memset(&hints, 0x0, sizeof(struct addrinfo));
			hints.ai_family = stream->socket_family;
			hints.ai_socktype = stream->socket_type;
			hints.ai_protocol = stream->protocol;

			struct addrinfo *res;
			if(getaddrinfo(node, service, &hints, &res) != 0)
			{
				fprintf(stderr, "%s: getaddrinfo failed\n", __func__);
				goto fail;
			}
			if(res->ai_addrlen != sizeof(stream->peer.in6))
			{
				fprintf(stderr, "%s: IPv6 address expected\n", __func__);
				goto fail;
			}
			stream->peer.len = res->ai_addrlen;
			stream->peer.in = *res->ai_addr;
			if(iface)
			{
				stream->peer.in6.sin6_scope_id = if_nametoindex(iface);
			}

			freeaddrinfo(res);
		}

		if(stream->socket_type == SOCK_DGRAM)
		{
			// nothing to do
		}
		else if(stream->socket_type == SOCK_STREAM)
		{
			const int flag = 1;

			if(setsockopt(stream->sock, stream->protocol,
				TCP_NODELAY, &flag, sizeof(int)) != 0)
			{
				fprintf(stderr, "%s: setsockopt failed\n", __func__);
				goto fail;
			}

			if(setsockopt(stream->sock, SOL_SOCKET,
				SO_KEEPALIVE, &flag, sizeof(int)) != 0)
			{
				fprintf(stderr, "%s: setsockopt failed\n", __func__);
				goto fail;
			}

			if(stream->server)
			{
				if(listen(stream->sock, 1) != 0)
				{
					fprintf(stderr, "%s: listen failed\n", __func__);
					goto fail;
				}
			}
			else // client
			{
				connect(stream->sock, &stream->peer.in, stream->peer.len);
			}
		}
		else
		{
			fprintf(stderr, "%s: invalid socket type\n", __func__);
			goto fail;
		}
	}
	else
	{
		fprintf(stderr, "%s: invalid socket family\n", __func__);
		goto fail;
	}

	free(dup);

	return 0;

fail:
	if(dup)
	{
		free(dup);
	}

	if(stream->sock >= 0)
	{
		close(stream->sock);
		stream->sock = 0;
	}

	return -1;
}

#define SLIP_END					0300	// 0xC0, 192, indicates end of packet
#define SLIP_ESC					0333	// 0xDB, 219, indicates byte stuffing
#define SLIP_END_REPLACE	0334	// 0xDC, 220, ESC ESC_END means END data byte
#define SLIP_ESC_REPLACE	0335	// 0xDD, 221, ESC ESC_ESC means ESC data byte

// SLIP encoding
static size_t
lv2_osc_slip_encode_inline(uint8_t *dst, size_t len)
{
	if(len == 0)
		return 0;

	const uint8_t *end = dst + len;

	// estimate new size
	size_t size = 2; // double ended SLIP
	for(const uint8_t *from=dst; from<end; from++, size++)
	{
		if( (*from == SLIP_END) || (*from == SLIP_ESC))
			size ++;
	}

	// fast track if no escaping needed
	if(size == len + 2)
	{
		memmove(dst+1, dst, len);
		dst[0] = SLIP_END;
		dst[size-1] = SLIP_END;

		return size;
	}

	// slow track if some escaping needed
	uint8_t *to = dst + size - 1;
	*to-- = SLIP_END;
	for(const uint8_t *from=end-1; from>=dst; from--)
	{
		if(*from == SLIP_END)
		{
			*to-- = SLIP_END_REPLACE;
			*to-- = SLIP_ESC;
		}
		else if(*from == SLIP_ESC)
		{
			*to-- = SLIP_ESC_REPLACE;
			*to-- = SLIP_ESC;
		}
		else
			*to-- = *from;
	}
	*to-- = SLIP_END;

	return size;
}

// SLIP decoding
static size_t 
lv2_osc_slip_decode_inline(uint8_t *dst, size_t len, size_t *size)
{
	const uint8_t *src = dst;
	const uint8_t *end = dst + len;
	uint8_t *ptr = dst;

	bool whole = false;

	if( (src < end) && (*src == SLIP_END) )
	{
		 whole = true;
		 src++;
	}

	while(src < end)
	{
		if(*src == SLIP_ESC)
		{
			if(src == end-1)
				break;

			src++;
			if(*src == SLIP_END_REPLACE)
				*ptr++ = SLIP_END;
			else if(*src == SLIP_ESC_REPLACE)
				*ptr++ = SLIP_ESC;
			src++;
		}
		else if(*src == SLIP_END)
		{
			src++;

			*size = whole ? ptr - dst : 0;
			return src - dst;
		}
		else
		{
			*ptr++ = *src++;
		}
	}

	*size = 0;
	return 0;
}

static LV2_OSC_Enum
lv2_osc_stream_run(LV2_OSC_Stream *stream)
{
	LV2_OSC_Enum ev = LV2_OSC_NONE;

	// handle connections
	if( (stream->socket_type == SOCK_STREAM)
		&& (stream->server)
		&& (stream->fd <= 0)) // no peer
	{
		stream->peer.len = sizeof(stream->peer.in);
		stream->fd = accept(stream->sock, &stream->peer.in, &stream->peer.len);

		if(stream->fd > 0)
		{
			const int flag = 1;
			const int sendbuff = LV2_OSC_STREAM_SNDBUF;
			const int recvbuff = LV2_OSC_STREAM_RCVBUF;

			if(fcntl(stream->fd, F_SETFL, O_NONBLOCK) == -1)
			{
				fprintf(stderr, "%s: fcntl failed\n", __func__);
			}

			if(setsockopt(stream->fd, stream->protocol,
				TCP_NODELAY, &flag, sizeof(int)) != 0)
			{
				fprintf(stderr, "%s: setsockopt failed\n", __func__);
			}

			if(setsockopt(stream->sock, SOL_SOCKET,
				SO_KEEPALIVE, &flag, sizeof(int)) != 0)
			{
				fprintf(stderr, "%s: setsockopt failed\n", __func__);
			}

			if(setsockopt(stream->fd, SOL_SOCKET,
				SO_SNDBUF, &sendbuff, sizeof(int))== -1)
			{
				fprintf(stderr, "%s: setsockopt failed\n", __func__);
			}

			if(setsockopt(stream->fd, SOL_SOCKET,
				SO_RCVBUF, &recvbuff, sizeof(int))== -1)
			{
				fprintf(stderr, "%s: setsockopt failed\n", __func__);
			}

			//FIXME ev |=
		}
	}

	// send everything
	if(stream->socket_type == SOCK_DGRAM)
	{
		if(stream->peer.len) // has a peer
		{
			const uint8_t *buf;
			size_t tosend;

			while( (buf = stream->driv->read_req(stream->data, &tosend)) )
			{
				const ssize_t sent = sendto(stream->sock, buf, tosend, 0,
					&stream->peer.in, stream->peer.len);

				if(sent == -1)
				{
					if( (errno = EAGAIN) || (errno == EWOULDBLOCK) )
					{
						// full queue
						break;
					}

					fprintf(stderr, "%s: sendto: %s\n", __func__, strerror(errno));
					break;
				}
				else if(sent != (ssize_t)tosend)
				{
					fprintf(stderr, "%s: only sent %zi of %zu bytes", __func__, sent, tosend);
					break;
				}

				stream->driv->read_adv(stream->data);
				ev |= LV2_OSC_SEND;
			}
		}
	}
	else if(stream->socket_type == SOCK_STREAM)
	{
		const int fd = stream->server
			? stream->fd
			: stream->sock;

		if(fd > 0)
		{
			const uint8_t *buf;
			size_t tosend;

			while( (buf = stream->driv->read_req(stream->data, &tosend)) )
			{
				if(stream->slip) // SLIP framed
				{
					if(tosend <= sizeof(stream->tx_buf)) // check if there is enough memory
					{
						memcpy(stream->tx_buf, buf, tosend);
						tosend = lv2_osc_slip_encode_inline(stream->tx_buf, tosend);
					}
					else
					{
						tosend = 0;
					}
				}
				else // uint32_t prefix frames
				{
					const size_t nsize = tosend + sizeof(uint32_t);

					if(nsize <= sizeof(stream->tx_buf)) // check if there is enough memory
					{
						const uint32_t prefix = htonl(tosend);

						memcpy(stream->tx_buf, &prefix, sizeof(uint32_t));
						memcpy(stream->tx_buf + sizeof(uint32_t), buf, tosend);
						tosend = nsize;
					}
					else
					{
						tosend = 0;
					}
				}

				const ssize_t sent = tosend
					? send(fd, stream->tx_buf, tosend, 0)
					: 0;

				if(sent == -1)
				{
					if( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
					{
						break;
					}
					else if(errno == ECONNRESET)
					{
						if(stream->server)
						{
							// peer has shut down
							close(stream->fd);
							stream->fd = 0;
							break;
						}
						else
						{
							assert(false); //FIXME reconnect
						}
					}

					fprintf(stderr, "%s: send: %s\n", __func__, strerror(errno));
					break;
				}
				else if(sent != (ssize_t)tosend)
				{
					fprintf(stderr, "%s: only sent %zi of %zu bytes", __func__, sent, tosend);
					break;
				}

				stream->driv->read_adv(stream->data);
				ev |= LV2_OSC_SEND;
			}
		}
	}

	// recv everything
	if(stream->socket_type == SOCK_DGRAM)
	{
		uint8_t *buf;
		size_t max_len;

		while( (buf = stream->driv->write_req(stream->data,
			LV2_OSC_STREAM_REQBUF, &max_len)) )
		{
			struct sockaddr in;
			socklen_t in_len = sizeof(in);

			const ssize_t recvd = recvfrom(stream->sock, buf, max_len, 0,
				&in, &in_len);

			if(recvd == -1)
			{
				if( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
				{
					// empty queue
					break;
				}

				fprintf(stderr, "%s: recv: %s\n", __func__, strerror(errno));
				break;
			}
			else if(recvd == 0)
			{
				// peer has shut down
				break;
			}

			stream->peer.len = in_len;
			stream->peer.in = in;

			stream->driv->write_adv(stream->data, recvd);
			ev |= LV2_OSC_RECV;
		}
	}
	else if(stream->socket_type == SOCK_STREAM)
	{
		const int fd = stream->server
			? stream->fd
			: stream->sock;

		if(fd > 0)
		{
			if(stream->slip) // SLIP framed
			{
				while(true)
				{
					ssize_t recvd = recv(fd, stream->rx_buf + stream->rx_off,
						sizeof(stream->rx_buf) - stream->rx_off, 0);

					if(recvd == -1)
					{
						if( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
						{
							// empty queue
							break;
						}
						else if(errno == ECONNRESET)
						{
							if(stream->server)
							{
								// peer has shut down
								close(stream->fd);
								stream->fd = 0;
								break;
							}
							else
							{
								assert(false); //FIXME reconnect
							}
						}

						fprintf(stderr, "%s: recv: %s\n", __func__, strerror(errno));
						break;
					}
					else if( (recvd == 0) && stream->server)
					{
						// peer has shut down
						close(stream->fd);
						stream->fd = 0;
						break;
					}

					uint8_t *ptr = stream->rx_buf;
					recvd += stream->rx_off;

					while(recvd > 0)
					{
						size_t size;
						const size_t parsed = lv2_osc_slip_decode_inline(ptr, recvd, &size);

						if(size) // dispatch
						{
							uint8_t *buf ;

							if( (buf = stream->driv->write_req(stream->data, size, NULL)) )
							{
								memcpy(buf, ptr, size);

								stream->driv->write_adv(stream->data, size);
								ev |= LV2_OSC_RECV;
							}
							else
							{
								fprintf(stderr, "%s: write buffer overflow", __func__);
							}
						}

						if(parsed)
						{
							ptr += parsed;
							recvd -= parsed;
						}
						else
						{
							break;
						}
					}

					if(recvd > 0) // is there remaining chunk for next call?
					{
						memmove(stream->rx_buf, ptr, recvd);
						stream->rx_off = recvd;
					}
					else
					{
						stream->rx_off = 0;
					}

					break;
				}
			}
			else // uint32_t prefix frames
			{
				uint8_t *buf;
				
				while( (buf = stream->driv->write_req(stream->data,
					LV2_OSC_STREAM_REQBUF, NULL)) )
				{
					uint32_t prefix;

					ssize_t recvd = recv(fd, &prefix, sizeof(uint32_t), 0);
					if(recvd == sizeof(uint32_t))
					{
						prefix = ntohl(prefix); //FIXME check prefix <= max_len
						recvd = recv(fd, buf, prefix, 0);
					}

					if(recvd == -1)
					{
						if( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
						{
							// empty queue
							break;
						}
						else if(errno == ECONNRESET)
						{
							if(stream->server)
							{
								// peer has shut down
								close(stream->fd);
								stream->fd = 0;
								break;
							}
							else
							{
								assert(false); //FIXME reconnect
							}
						}

						fprintf(stderr, "%s: recv: %s\n", __func__, strerror(errno));
						break;
					}
					else if( (recvd == 0) && stream->server)
					{
						// peer has shut down
						close(stream->fd);
						stream->fd = 0;
						break;
					}

					stream->driv->write_adv(stream->data, recvd);
					ev |= LV2_OSC_RECV;
				}
			}
		}
	}

	return ev;
}

static int
lv2_osc_stream_deinit(LV2_OSC_Stream *stream)
{
	if(stream->fd >= 0)
	{
		close(stream->fd);
		stream->fd = 0;
	}

	if(stream->sock >= 0)
	{
		close(stream->sock);
		stream->sock = 0;
	}

	return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LV2_OSC_STREAM_H
