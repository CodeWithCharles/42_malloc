/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_malloc.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 17:12:03 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/01 17:33:27 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_MALLOC_H
# define FT_MALLOC_H

# include <stddef.h>

# define FTM_PUBLIC __attribute__((visibility("default")))

FTM_PUBLIC void *malloc(size_t size);
FTM_PUBLIC void free(void *ptr);
FTM_PUBLIC void *realloc(void *ptr, size_t size);
FTM_PUBLIC void *calloc(size_t nmemb, size_t size);

FTM_PUBLIC void show_alloc_mem(void);
FTM_PUBLIC void show_alloc_mem_ex(void);

#endif