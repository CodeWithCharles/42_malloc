/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 17:30:28 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 11:25:01 by cpoulain         ###   ########.fr       */
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

void show_alloc_mem(void)
{
	ftm_lock();
	ftm_show();
	ftm_unlock();
}

void show_alloc_mem_ex(void) {}