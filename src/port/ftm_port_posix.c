/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_port_posix.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 14:20:39 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 14:41:25 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#define _GNU_SOURCE

#include "ftm_port.h"
#include "libft.h"

#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>

static size_t   cached_page_size(void)
{
	static size_t   page_size = 0;

	if (page_size == 0)
		page_size = (size_t)sysconf(_SC_PAGESIZE);
	return (page_size);
}

size_t  ftm_page_size(void)
{
	return (cached_page_size());
}

static bool request_fits_address_space(size_t length)
{
	struct rlimit   limit;

	if (getrlimit(RLIMIT_AS, &limit) != 0)
		return (true);
	if (limit.rlim_cur == RLIM_INFINITY)
		return (true);
	return (length <= (size_t)limit.rlim_cur);
}

void    *ftm_map_pages(size_t length)
{
	void    *pages;

	if (length == 0 || !request_fits_address_space(length))
		return (NULL);
	pages = mmap(NULL, length, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (pages == MAP_FAILED)
		return (NULL);
	return (pages);
}

void    ftm_unmap_pages(void *addr, size_t length)
{
	if (addr != NULL && length != 0)
		munmap(addr, length);
}

void    ftm_write(const char *buffer, size_t length)
{
	ssize_t written;
	size_t  total;

	total = 0;
	while (total < length)
	{
		written = write(STDOUT_FILENO, buffer + total, length - total);
		if (written <= 0)
			return;
		total += (size_t)written;
	}
}

void    *ftm_memcpy(void *dst, const void *src, size_t length)
{
	return (ft_memcpy(dst, src, length));
}

void    *ftm_memset(void *dst, int value, size_t length)
{
	return (ft_memset(dst, value, length));
}

static void	fatal_write(const char *buf, size_t length)
{
	ssize_t	unused;

	unused = write(STDERR_FILENO, buf, length);
	(void)unused;
}

void    ftm_fatal(const char *message)
{
	if (message != NULL)
	{
		while (*message)
			fatal_write(message++, 1);
		fatal_write("\n", 1);
	}
	abort();
}