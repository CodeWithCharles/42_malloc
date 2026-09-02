/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_internal.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 14:36:57 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 17:29:20 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FTM_INTERNAL_H
# define FTM_INTERNAL_H

# include "ftm_types.h"

size_t      	ftm_round_up_to_alignment(size_t size);
t_zone_kind 	ftm_size_class(size_t size);

void        	*ftm_block_payload(t_block *block);
t_block			*ftm_payload_to_block(void *payload);
unsigned char	*ftm_block_end(t_block *block);

bool			ftm_block_is_free(const t_block *block);
void			ftm_block_mark_free(t_block *block);
void			ftm_block_mark_used(t_block *block);
t_block			*ftm_block_split(t_block *block, size_t payload_size);
void			ftm_block_coalesce_next(t_block *block);

t_zone			*ftm_zone_create(t_zone_kind kind, size_t payload_size);
void			ftm_zone_destroy(t_zone *zone);

t_block			*ftm_zone_first_block(t_zone *zone);
t_block			*ftm_zone_find_free(t_zone *zone, size_t payload_size);
size_t			ftm_zone_total_size(t_zone_kind kind, size_t payload_size);

t_heap			*ftm_heap_instance(void);
void			ftm_heap_reset(void);
t_zone			*ftm_heap_find_zone(void *ptr);
bool			ftm_check_heap(void);

void			ftm_release(void *ptr);
void			*ftm_alloc(size_t size);
void			*ftm_resize(void *ptr, size_t size);

#endif