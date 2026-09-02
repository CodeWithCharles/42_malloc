/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 17:30:28 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 16:12:40 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"
#include "ftm_internal.h"
#include "ftm_port.h"

void *malloc(size_t size)
{
	void	*ptr;

	ftm_lock();
	ptr = ftm_alloc(size);
	ftm_unlock();
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
	(void)ptr;
	(void)size;
	return (NULL);
}

void *calloc(size_t nmemb, size_t size)
{
	(void)nmemb;
	(void)size;
	return (NULL);
}

void show_alloc_mem(void) {}

void show_alloc_mem_ex(void) {}