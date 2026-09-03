/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_guard.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 11:19:20 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 11:29:46 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"

bool	ftm_pointer_is_allocated(void *ptr, t_zone *zone)
{
	unsigned char	*first_payload;
	t_block			*block;

	first_payload = ftm_block_payload(ftm_zone_first_block(zone));
	if ((unsigned char *)ptr < first_payload)
		return (false);
	if ((unsigned char *)ptr >= (unsigned char *)zone + zone->total_size)
		return (false);
	block = ftm_payload_to_block(ptr);
	if (!ftm_block_is_valid(block))
		return (false);
	return (!ftm_block_is_free(block));
}