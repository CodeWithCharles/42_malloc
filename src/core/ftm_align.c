/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_align.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 14:39:34 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 17:56:49 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"

size_t  	ftm_round_up_to_alignment(size_t size)
{
	if (size > SIZE_MAX - (FTM_ALIGNMENT - 1))
		return (0);
	return (FTM_ALIGN_UP(size, FTM_ALIGNMENT));
}

t_zone_kind ftm_size_class(size_t size)
{
	if (size <= FTM_TINY_MAX)
		return (FTM_TINY);
	if (size <= FTM_SMALL_MAX)
		return (FTM_SMALL);
	return (FTM_LARGE);
}

void        *ftm_block_payload(t_block *block)
{
	return ((unsigned char *)block + FTM_BLOCK_HEADER_SIZE);
}

t_block		*ftm_payload_to_block(void *payload)
{
	return ((t_block *)((unsigned char *)payload - FTM_BLOCK_HEADER_SIZE));
}

unsigned char	*ftm_block_end(t_block *block)
{
	return ((unsigned char *)ftm_block_payload(block) + block->payload_size);
}

bool	ftm_block_is_free(const t_block *block)
{
	return ((block->flags & FTM_BLOCK_FREE) != 0);
}

void	ftm_block_mark_free(t_block *block)
{
	block->flags |= FTM_BLOCK_FREE;
}

void	ftm_block_mark_used(t_block *block)
{
	block->flags &= ~(uintptr_t)FTM_BLOCK_FREE;
}