/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 17:30:28 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/01 17:31:07 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_malloc.h"

void *malloc(size_t size)
{
    (void)size;
    return (NULL);
}

void free(void *ptr) { (void)ptr; }

void *realloc(void *ptr, size_t size)
{
    (void)ptr;
    (void)size;
    return (NULL);
}

void *calloc(size_t n, size_t size)
{
    (void)n;
    (void)size;
    return (NULL);
}

void show_alloc_mem(void) {}

void show_alloc_mem_ex(void) {}