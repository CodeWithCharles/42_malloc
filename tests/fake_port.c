/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fake_port.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 17:32:02 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 18:11:37 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_port.h"
#include "fake_port.h"

#include <unistd.h>
#include <stdlib.h>

#define FAKE_POOL_PAGE  4096
#define FAKE_POOL_SIZE  (16 * 1024 * 1024)
#define FAKE_CAPTURE_SIZE (64 * 1024)

static _Alignas(FAKE_POOL_PAGE) unsigned char	g_pool[FAKE_POOL_SIZE];
static size_t	g_offset = 0;
static int		g_fail_after = -1;
static size_t	g_map_count = 0;
static size_t	g_unmap_count = 0;
static char		g_capture[FAKE_CAPTURE_SIZE];
static size_t	g_capture_len = 0;
static int		g_capturing = 0;

void	fake_capture_reset(void)
{
	g_capture_len = 0;
	g_capturing = 1;
}

const char	*fake_capture_buffer(void)
{
	g_capture[g_capture_len] = '\0';
	return (g_capture);
}

void	fake_port_reset(void)
{
	g_offset = 0;
	g_fail_after = -1;
	g_map_count = 0;
	g_unmap_count = 0;
	fake_capture_reset();
	g_capturing = 0;
}

void	fake_port_fail_after(int successful_maps)
{
	g_fail_after = successful_maps;
}

size_t	fake_port_map_count(void)
{
	return (g_map_count);
}

size_t	fake_port_unmap_count(void)
{
	return (g_unmap_count);
}

size_t	ftm_page_size(void)
{
	return (FAKE_POOL_PAGE);
}

void	*ftm_map_pages(size_t length)
{
	void	*pages;

	if (length == 0)
		return (NULL);
	if (g_fail_after == 0)
		return (NULL);
	if (g_offset + length > FAKE_POOL_SIZE)
		return (NULL);
	pages = g_pool + g_offset;
	g_offset += length;
	g_map_count++;
	if (g_fail_after > 0)
		g_fail_after--;
	return (pages);
}

void	ftm_unmap_pages(void *addr, size_t length)
{
	(void)addr;
	(void)length;
	g_unmap_count++;
}

void	ftm_lock(void)
{
}

void	ftm_unlock(void)
{
}

void	ftm_write(const char *buffer, size_t length)
{
	size_t	i;

	if (g_capturing)
	{
		i = 0;
		while (i < length && g_capture_len + 1 < FAKE_CAPTURE_SIZE)
			g_capture[g_capture_len++] = buffer[i++];
		return ;
	}
	write(STDOUT_FILENO, buffer, length);
}

void	*ftm_memcpy(void *dst, const void *src, size_t length)
{
	unsigned char		*d;
	const unsigned char	*s;
	size_t				i;

	d = dst;
	s = src;
	i = 0;
	while (i < length)
	{
		d[i] = s[i];
		i++;
	}
	return (dst);
}

void	*ftm_memset(void *dst, int value, size_t length)
{
	unsigned char	*d;
	size_t			i;

	d = dst;
	i = 0;
	while (i < length)
		d[i++] = (unsigned char)value;
	return (dst);
}

void	ftm_fatal(const char *message)
{
	if (message != NULL)
	{
		while (*message)
			write(STDERR_FILENO, message++, 1);
		write(STDERR_FILENO, "\n", 1);
	}
	abort();
}