/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_free_list.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 13:27:08 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 15:40:28 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"

static t_free_node	*node_of(t_block *block)
{
	return ((t_free_node *)ftm_block_payload(block));
}

t_block	*ftm_free_list_next(t_block *block)
{
	return (node_of(block)->next_free);
}

t_block	*ftm_free_list_prev(t_block *block)
{
	return (node_of(block)->prev_free);
}

void	ftm_free_list_push(t_zone *zone, t_block *block)
{
	t_free_node	*node;

	node = node_of(block);
	node->prev_free = NULL;
	node->next_free = zone->free_list;
	if (zone->free_list != NULL)
		node_of(zone->free_list)->prev_free = block;
	zone->free_list = block;
}

void	ftm_free_list_unlink(t_zone *zone, t_block *block)
{
	t_free_node	*node;

	node = node_of(block);
	if (node->prev_free != NULL)
		node_of(node->prev_free)->next_free = node->next_free;
	else
		zone->free_list = node->next_free;
	if (node->next_free != NULL)
		node_of(node->next_free)->prev_free = node->prev_free;
	node->next_free = NULL;
	node->prev_free = NULL;
}

t_block	*ftm_free_list_find(t_zone *zone, size_t payload_size)
{
	t_block	*block;

	block = zone->free_list;
	while (block != NULL)
	{
		if (block->payload_size >= payload_size)
			return (block);
		block = node_of(block)->next_free;
	}
	return (NULL);
}