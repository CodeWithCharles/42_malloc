/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_aligned.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 11:59:17 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 12:07:09 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"

void	*ftm_alloc_aligned(size_t alignment, size_t size)
{
	void		*base;
	uintptr_t	aligned;
	t_align_tag	*tag;
	size_t		over;

	if (alignment <= FTM_ALIGNMENT)
		return (ftm_alloc(size));
	if (size > SIZE_MAX - alignment - sizeof(t_align_tag))
		return (NULL);
	over = size + alignment + sizeof(t_align_tag);
	base = ftm_alloc(over);
	if (base == NULL)
		return (NULL);
	aligned = ((uintptr_t)base + sizeof(t_align_tag) + alignment - 1)
		& ~(alignment - 1);
	tag = (t_align_tag *)(aligned - sizeof(t_align_tag));
	tag->magic = FTM_ALIGN_MAGIC;
	tag->base = base;
	return ((void *)aligned);
}

void	*ftm_aligned_base(void *ptr, t_zone *zone)
{
	t_align_tag		*tag;
	unsigned char	*lowest;

	lowest = (unsigned char *)zone + FTM_ZONE_HEADER_SIZE + sizeof(t_align_tag);
	if ((unsigned char *)ptr < lowest)
		return (NULL);
	tag = (t_align_tag *)((unsigned char *)ptr - sizeof(t_align_tag));
	if (tag->magic != FTM_ALIGN_MAGIC)
		return (NULL);
	return (tag->base);
}

size_t	ftm_usable_size(void *ptr)
{
	t_zone			*zone;
	void			*base;
	t_block			*block;
	unsigned char	*end;

	if (ptr == NULL)
		return (0);
	zone = ftm_heap_find_zone(ptr);
	if (zone == NULL)
		return (0);
	if (ftm_pointer_is_allocated(ptr, zone))
		return (ftm_payload_to_block(ptr)->payload_size);
	base = ftm_aligned_base(ptr, zone);
	if (base == NULL)
		return (0);
	block = ftm_payload_to_block(base);
	end = (unsigned char *)ftm_block_payload(block) + block->payload_size;
	if ((unsigned char *)ptr <= end)
		return ((size_t)(end - (unsigned char *)ptr));
	return (0);
}