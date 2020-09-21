/*
 * Copyright(c) 2018 - 2020 Yaguan Technology Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FreeRTOS.h"
#include "ya_common.h"
#include "ya_hal_net.h"

uint32_t ya_hal_htonl(uint32_t hostlong)
{
	return htonl(hostlong);
}

uint16_t ya_hal_htons(uint16_t n)
{
	return htons(n);
}

uint16_t ya_hal_ntohs(uint16_t n)
{
	return ntohs(n);
}

uint32_t ya_hal_inet_addr(const char *cp)
{
	return inet_addr(cp);
}

int32_t ya_hal_get_ip_by_name(const char *host_name, uint32_t *dest_ip)
{
	struct hostent *s_hostent = lwip_gethostbyname(host_name);

	(*dest_ip) = (s_hostent->h_addr_list[0][0] << 24) |
					((0xff & s_hostent->h_addr_list[0][1]) << 16) | 
					((0xff & s_hostent->h_addr_list[0][2]) << 8) |
					(0xff & s_hostent->h_addr_list[0][3]);

	if(*dest_ip != 0)
		return 0;

	return -1;
}

int32_t ya_hal_socket_create(int32_t domain, int32_t socket_type, int32_t protocol)
{
	int32_t ret;
	lwip_socket_init();
	ret = lwip_socket(domain, socket_type, protocol);
	return ret;
}

int32_t ya_hal_socket_close(int32_t fd)
{
	return lwip_close(fd);
}

int32_t ya_hal_socket_bind(int32_t fd, const struct sockaddr *addr, socklen_t addrlen)
{
	return lwip_bind(fd, addr, addrlen);
}

int32_t ya_hal_socket_connect(int32_t fd, const struct sockaddr *addr, socklen_t addrlen)
{
	int ret, non_blocked = 1;
	fd_set fdset;
	struct timeval time_out;

	ret = lwip_ioctl(fd, FIONBIO, &non_blocked);
	if(ret < 0)
		return -1;

	ret = lwip_connect(fd, addr, addrlen);
	if(ret < 0)
	{
		time_out.tv_sec = 6;
		time_out.tv_usec = 0;
		
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);

		ret = select(fd+1, NULL, &fdset, NULL, &time_out);
		if(ret <= 0)
			return -1;
	}
	return ret;
}

int32_t ya_hal_socket_connect_block(int32_t fd, const struct sockaddr *addr, socklen_t addrlen)
{
	int ret, non_blocked = 1;
	fd_set fdset;
	struct timeval time_out;

	ret = lwip_ioctl(fd, FIONBIO, &non_blocked);
	if(ret < 0)
		return -1;

	ret = lwip_connect(fd, addr, addrlen);
	if(ret < 0)
	{
		time_out.tv_sec = 6;
		time_out.tv_usec = 0;
		
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);

		ret = select(fd+1, NULL, &fdset, NULL, &time_out);
		if(ret <= 0)
			return -1;
	}

	non_blocked = 0;
	ret = lwip_ioctl(fd, FIONBIO, &non_blocked);
	
	return ret;
}

int32_t ya_hal_socket_listen(int32_t fd, int32_t backlog)
{
	return lwip_listen(fd, backlog);
}

int32_t ya_hal_socket_recv(int32_t fd, void *buf, size_t nbyte, int32_t flags)
{
	return lwip_recv(fd, buf, nbyte, flags);
}

int32_t ya_hal_socket_send(int32_t fd, const void *buf, size_t nbyte, int32_t flags)
{
	int32_t ret = -1;
	if( fd < 0)
		return -1;

	ret = lwip_send(fd, buf, nbyte, flags);

	if(ret > 0)
		return nbyte;

	return -1;
}

int32_t ya_hal_socket_sendto(int32_t fd, const void *buf, size_t len, int32_t flags, const struct sockaddr *target_addr, socklen_t target_len)
{
	int32_t ret = lwip_sendto(fd, buf, len, flags, target_addr, target_len);
	if(ret > 0)
		return len;

	return -1;
}

int32_t ya_hal_socket_recvform(int32_t fd, const void *buf, size_t len, int32_t flags, const struct sockaddr *source_addr, socklen_t *source_len)
{
	return lwip_recvfrom(fd, (void *)buf, len, flags, (struct sockaddr *)source_addr, source_len);
}

uint16_t ya_get_tcp_new_port()
{
	uint8_t random_num[2] = {0};
	ya_os_get_random(random_num, 2);

	return (10000 + (random_num[0] + ((random_num[1] & 0x00FF) << 8))%10000);
}

int32_t ya_tcp_client_connect(int32_t *sock, uint32_t dest_ip, uint16_t dest_port, uint32_t rece_timeout)
{
	int32_t timeout = rece_timeout;
	struct sockaddr_in dest_addr;
	struct sockaddr_in source_addr;
	uint16_t ya_source_port;
	
	if(((*sock) = ya_hal_socket_create(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	ya_source_port = ya_get_tcp_new_port();

	memset(&source_addr, 0, sizeof(struct sockaddr_in));
	source_addr.sin_family = AF_INET;
	source_addr.sin_addr.s_addr = INADDR_ANY;
	source_addr.sin_port = ya_hal_htons(ya_source_port);

	if(ya_hal_socket_bind((*sock), (struct sockaddr *)&source_addr, sizeof(struct sockaddr_in)) < 0)
	{
		ya_hal_socket_close(*sock);
		(*sock) = -1;
		return -1;
	}

	if(timeout > 0)
	{
		if(setsockopt((*sock), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
		{
			ya_hal_socket_close(*sock);
			(*sock) = -1;
			return -1;
		}
	}

	memset(&dest_addr, 0, sizeof(struct sockaddr_in));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = ya_hal_htonl(dest_ip);
	dest_addr.sin_port = ya_hal_htons(ya_source_port);

	if(ya_hal_socket_connect((*sock), (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in)) < 0)
	{
		ya_hal_socket_close(*sock);
		(*sock) = -1;
		return -1;
	}

	return 0;
}

int32_t ya_tcp_client_connect_block(int32_t *sock, uint32_t dest_ip, uint16_t dest_port, uint32_t rece_timeout)
{
	int32_t timeout = rece_timeout;
	struct sockaddr_in dest_addr;
	struct sockaddr_in source_addr;
	uint16_t ya_source_port;
	
	if(((*sock) = ya_hal_socket_create(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	ya_source_port = ya_get_tcp_new_port();

	memset(&source_addr, 0, sizeof(struct sockaddr_in));
	source_addr.sin_family = AF_INET;
	source_addr.sin_addr.s_addr = INADDR_ANY;
	source_addr.sin_port = ya_hal_htons(ya_source_port);

	if(ya_hal_socket_bind((*sock), (struct sockaddr *)&source_addr, sizeof(struct sockaddr_in)) < 0)
	{
		ya_hal_socket_close(*sock);
		(*sock) = -1;
		return -1;
	}

	if(timeout > 0)
	{
		if(setsockopt((*sock), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
		{
			ya_hal_socket_close(*sock);
			(*sock) = -1;
			return -1;
		}
	}

	memset(&dest_addr, 0, sizeof(struct sockaddr_in));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = ya_hal_htonl(dest_ip);
	dest_addr.sin_port = ya_hal_htons(dest_port);

	if(ya_hal_socket_connect_block((*sock), (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in)) < 0)
	{
		ya_hal_socket_close(*sock);
		(*sock) = -1;
		return -1;
	}

	return 0;
}


int32_t ya_server_set_up(int32_t *sock, uint16_t server_port, int32_t backlog)
{
	struct sockaddr_in source_addr;

	if(((*sock) = ya_hal_socket_create(PF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	memset(&source_addr, 0, sizeof(struct sockaddr_in));
	source_addr.sin_family = AF_INET;
	source_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	source_addr.sin_port = ya_hal_htons(server_port);

	if(ya_hal_socket_bind((*sock), (struct sockaddr *)&source_addr, sizeof(struct sockaddr_in)) < 0)
	{
		ya_hal_socket_close(*sock);
		(*sock) = -1;
		return -1;
	}

	if(ya_hal_socket_listen((*sock), backlog) < 0)
	{
		ya_hal_socket_close(*sock);
		(*sock) = -1;
		return -1;
	}

	return 0;

}

int32_t ya_udp_broadcast_set_up(int32_t *sock, uint16_t port)
{
	int32_t on = -1;

	if(((*sock) = ya_hal_socket_create(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;

	#if 1
	struct sockaddr_in source_addr;
	source_addr.sin_family = AF_INET;
	source_addr.sin_addr.s_addr = INADDR_ANY;
	source_addr.sin_port = ya_hal_htons(port);

	if(ya_hal_socket_bind((*sock), (struct sockaddr *)&source_addr, sizeof(struct sockaddr_in)) < 0)
	{
		ya_hal_socket_close(*sock);
		(*sock) = -1;
		return -1;
	}

	#endif

	if(setsockopt((*sock), SOL_SOCKET, SO_BROADCAST, (void *)&on, sizeof(int32_t)) < 0)
	{
		ya_hal_socket_close(*sock);
		(*sock) = -1;
		return -1;
	}

	return 0;
}

int32_t ya_udp_server(uint16_t port, int timeout)
{
	int32_t timeout_set ;
	int fd = -1;
	int ret = -1;
	struct sockaddr_in sock_addr;
	socklen_t sock_addr_len;

	timeout_set = timeout;
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0) 
	{
		printf("Creat udp socket failed...");
		return -1;
	}

	memset(&sock_addr, 0, sizeof(sock_addr));	
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = INADDR_ANY;
	sock_addr.sin_port = htons(port);
	sock_addr_len = sizeof(sock_addr);

	if(timeout_set)
	{
		if(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout_set, sizeof(int)) < 0)
		{
			ya_hal_socket_close(fd);
			printf("setsockopt is failed");
			return -1;
		}
	}

	ret = bind(fd, (struct sockaddr *) &sock_addr, sock_addr_len);
	if (ret < 0)
	{
		ya_hal_socket_close(fd);
		printf("bind is failed...");
		return -1;
	}

	return fd;
}

