/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_block.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 15:31:28 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 15:37:09 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"

t_block *ftm_block_split(t_block *block, size_t payload_size)
{
	t_block *remainder;
	size_t leftover;

	leftover = block->payload_size - payload_size;
	if (leftover < FTM_BLOCK_HEADER_SIZE + FTM_ALIGNMENT)
		return (block);
	remainder = (t_block *)((unsigned char *)ftm_block_payload(block) + payload_size);
	remainder->payload_size = leftover - FTM_BLOCK_HEADER_SIZE;
	remainder->flags = 0;
	ftm_block_mark_free(remainder);
	remainder->prev = block;
	remainder->next = block->next;
	if (block->next != NULL)
		block->next->prev = remainder;
	block->next = remainder;
	block->payload_size = payload_size;
	return (block);
}
