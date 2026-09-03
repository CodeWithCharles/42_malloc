/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 17:30:28 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 12:20:21 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"
#include "ftm_internal.h"
#include "ftm_port.h"
#include <errno.h>

void *malloc(size_t size)
{
	void	*ptr;

	ftm_lock();
	ptr = ftm_alloc(size);
	ftm_unlock();
	if (ptr == NULL)
		errno = ENOMEM;
	return (ptr);
}

void free(void *ptr)
{
	ftm_lock();
	ftm_release(ptr);
	ftm_unlock();
}

void *realloc(void *ptr, size_t size)
{
	void	*result;

	ftm_lock();
	result = ftm_resize(ptr, size);
	ftm_unlock();
	if (result == NULL && size != 0)
		errno = ENOMEM;
	return (result);
}

void	*calloc(size_t nmemb, size_t size)
{
	size_t	total;
	void	*ptr;

	if (size != 0 && nmemb > SIZE_MAX / size)
	{
		errno = ENOMEM;
		return (NULL);
	}
	total = nmemb * size;
	ftm_lock();
	ptr = ftm_alloc(total);
	ftm_unlock();
	if (ptr == NULL)
		errno = ENOMEM;
	else
		ftm_memset(ptr, 0, total);
	return (ptr);
}

int	posix_memalign(void **memptr, size_t alignment, size_t size)
{
	void	*ptr;

	if (memptr == NULL)
		return (EINVAL);
	if (alignment < sizeof(void *) || (alignment & (alignment - 1)) != 0)
		return (EINVAL);
	ftm_lock();
	ptr = ftm_alloc_aligned(alignment, size);
	ftm_unlock();
	if (ptr == NULL)
		return (ENOMEM);
	*memptr = ptr;
	return (0);
}

static void	*aligned_common(size_t alignment, size_t size)
{
	void	*ptr;
	
	if (alignment == 0 || (alignment & (alignment - 1)) != 0)
	{
		errno = EINVAL;
		return (NULL);
	}
	ftm_lock();
	ptr = ftm_alloc_aligned(alignment, size);
	ftm_unlock();
	if (ptr == NULL)
		errno = ENOMEM;
	return (ptr);
}

void	*aligned_alloc(size_t alignment, size_t size)
{
	return (aligned_common(alignment, size));
}

void	*memalign(size_t alignment, size_t size)
{
	return (aligned_common(alignment, size));
}

void	*valloc(size_t size)
{
	return (aligned_common(ftm_page_size(), size));
}

void	*pvalloc(size_t size)
{
	size_t	page;
	size_t	rounded;

	page = ftm_page_size();
	if (size > SIZE_MAX - page)
	{
		errno = ENOMEM;
		return (NULL);
	}
	rounded = (size + page - 1) & ~(page - 1);
	if (rounded == 0)
		rounded = page;
	return (aligned_common(page, rounded));
}

size_t	malloc_usable_size(void *ptr)
{
	size_t	result;

	ftm_lock();
	result = ftm_usable_size(ptr);
	ftm_unlock();
	return (result);
}

void	*reallocarray(void *ptr, size_t nmemb, size_t size)
{
	void	*result;

	if (size != 0 && nmemb > SIZE_MAX / size)
	{
		errno = ENOMEM;
		return (NULL);
	}
	ftm_lock();
	result = ftm_resize(ptr, nmemb * size);
	ftm_unlock();
	if (result == NULL && nmemb * size != 0)
		errno = ENOMEM;
	return (result);
}

void show_alloc_mem(void)
{
	ftm_lock();
	ftm_show();
	ftm_unlock();
}

void show_alloc_mem_ex(void) {}