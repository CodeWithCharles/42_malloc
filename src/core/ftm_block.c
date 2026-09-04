/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_block.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 15:31:28 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 17:51:02 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"

t_block *ftm_block_split(t_zone *zone, t_block *block, size_t payload_size)
{
	t_block *remainder;
	size_t leftover;

	leftover = block->payload_size - payload_size;
	if (leftover < FTM_BLOCK_HEADER_SIZE + FTM_ALIGNMENT)
		return (block);
	remainder = (t_block *)((unsigned char *)ftm_block_payload(block)
		+ payload_size);
	remainder->payload_size = leftover - FTM_BLOCK_HEADER_SIZE;
	remainder->flags = FTM_BLOCK_MAGIC;
	ftm_block_mark_free(remainder);
	remainder->prev = block;
	remainder->next = block->next;
	if (block->next != NULL)
		block->next->prev = remainder;
	block->next = remainder;
	block->payload_size = payload_size;
	ftm_free_list_push(zone, remainder);
	return (block);
}

void	ftm_block_coalesce_next(t_zone *zone, t_block *block)
{
	t_block	*next;

	next = block->next;
	if (next == NULL || !ftm_block_is_free(block) || !ftm_block_is_free(next))
		return ;
	ftm_free_list_unlink(zone, next);
	block->payload_size += FTM_BLOCK_HEADER_SIZE + next->payload_size;
	block->next = next->next;
	if (next->next != NULL)
		next->next->prev = block;
}

bool	ftm_block_is_valid(const t_block *block)
{
	return ((block->flags & FTM_MAGIC_MASK) == FTM_BLOCK_MAGIC);
}