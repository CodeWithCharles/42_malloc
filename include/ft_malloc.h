/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_malloc.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 17:12:03 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 11:56:33 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_MALLOC_H
# define FT_MALLOC_H

# include <stddef.h>

# define FTM_PUBLIC __attribute__((visibility("default")))

FTM_PUBLIC void 	*malloc(size_t size);

FTM_PUBLIC void 	free(void *ptr);
FTM_PUBLIC void 	*realloc(
	void *ptr,
	size_t size);

FTM_PUBLIC void 	*calloc(
	size_t nmemb,
	size_t size);

FTM_PUBLIC void 	show_alloc_mem(void);
FTM_PUBLIC void 	show_alloc_mem_ex(void);

FTM_PUBLIC int		posix_memalign(
	void **memptr,
	size_t alignment,
	size_t size);

FTM_PUBLIC void		*aligned_alloc(
	size_t alignment,
	size_t size);

FTM_PUBLIC void		*memalign(
	size_t alignment,
	size_t size);

FTM_PUBLIC void		*valloc(size_t size);
FTM_PUBLIC void		*pvalloc(size_t size);
FTM_PUBLIC size_t	malloc_usable_size(void *ptr);
FTM_PUBLIC void		*reallocarray(
	void *ptr,
	size_t nmemb,
	size_t size);

#endif