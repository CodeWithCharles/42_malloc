/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_internal.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 14:36:57 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 15:40:43 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FTM_INTERNAL_H
# define FTM_INTERNAL_H

# include "ftm_types.h"

/* ---------------------------------- Debug --------------------------------- */

t_debug			*ftm_debug(void);
void			ftm_on_alloc(t_block *block);
void			ftm_on_free(t_block *block);
void			ftm_report_error(const char *message);

/* ---------------------------------- Utils --------------------------------- */

size_t      	ftm_round_up_to_alignment(size_t size);
t_zone_kind 	ftm_size_class(size_t size);
void			*ftm_alloc_aligned(
	size_t alignment,
	size_t size);

void			*ftm_aligned_base(
	void *ptr,
	t_zone *zone);

size_t			ftm_usable_size(void *ptr);

bool			ftm_pointer_is_allocated(
	void *ptr,
	t_zone *zone);

/* --------------------------------- Blocks --------------------------------- */

void        	*ftm_block_payload(t_block *block);
t_block			*ftm_payload_to_block(void *payload);
unsigned char	*ftm_block_end(t_block *block);

bool			ftm_block_is_free(const t_block *block);
void			ftm_block_mark_free(t_block *block);
void			ftm_block_mark_used(t_block *block);
t_block			*ftm_block_split(
	t_zone *zone,
	t_block *block,
	size_t payload_size);

void			ftm_block_coalesce_next(
	t_zone *zone,
	t_block *block);

bool			ftm_block_is_valid(const t_block *block);

/* ---------------------------------- Zones --------------------------------- */

t_zone			*ftm_zone_create(
	t_zone_kind kind,
	size_t payload_size);

void			ftm_zone_destroy(t_zone *zone);

t_block			*ftm_zone_first_block(t_zone *zone);
t_block			*ftm_zone_find_free(
	t_zone *zone,
	size_t payload_size);

size_t			ftm_zone_total_size(
	t_zone_kind kind,
	size_t payload_size);

/* ---------------------------------- Heap ---------------------------------- */

t_heap			*ftm_heap_instance(void);
void			ftm_heap_reset(void);
t_zone			*ftm_heap_find_zone(void *ptr);
bool			ftm_check_heap(void);

/* -------------------------------- Printing -------------------------------- */

size_t			ftm_fmt_hex(
	char *dst,
	uintptr_t value);

size_t			ftm_fmt_udec(
	char *dst,
	size_t value);

void			ftm_show(void);
void			ftm_show_ex(void);

/* ---------------------------------- Debug --------------------------------- */

void			ftm_history_record(
	char operation,
	void *ptr,
	size_t size);

void			ftm_history_dump(void);

/* ---------------------------------- Main ---------------------------------- */

void			ftm_release(void *ptr);
void			*ftm_alloc(size_t size);
void			*ftm_resize(
	void *ptr,
	size_t size);

/* --------------------------------- Caching -------------------------------- */

t_zone			*ftm_large_cache_take(size_t payload_size);
t_zone			*ftm_large_cache_put(t_zone *zone);
void			ftm_large_cache_flush(void);

/* --------------------------- Explicit free list --------------------------- */

void			ftm_free_list_push(
	t_zone *zone,
	t_block *block);

void			ftm_free_list_unlink(
	t_zone *zone,
	t_block *block);

t_block			*ftm_free_list_find(
	t_zone *zone,
	size_t payload_size);

t_block			*ftm_free_list_next(t_block *block);
t_block			*ftm_free_list_prev(t_block *block);

/* --------------------------------- Mapping -------------------------------- */

void			ftm_zone_map_insert(t_zone *zone);
void			ftm_zone_map_remove(t_zone *zone);
t_zone			*ftm_zone_map_lookup(void *ptr);
bool			ftm_zone_map_is_active(void);
void			ftm_zone_map_reset(void);

#endif