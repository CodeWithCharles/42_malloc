/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_show.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 17:55:49 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 18:18:58 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"

static size_t	append(char *dst, size_t pos, const char *text)
{
	size_t	i;

	i = 0;
	while (text[i] != '\0')
		dst[pos++] = text[i++];
	return (pos);
}

static void	print_block(t_block *block, size_t *total)
{
	char		line[128];
	size_t		pos;
	uintptr_t	start;

	start = (uintptr_t)ftm_block_payload(block);
	pos = append(line, 0, "0x");
	pos += ftm_fmt_hex(line + pos, start);
	pos = append(line, pos, " - 0x");
	pos += ftm_fmt_hex(line + pos, start + block->request_size);
	pos = append(line, pos, " : ");
	pos += ftm_fmt_udec(line + pos, block->request_size);
	pos = append(line, pos, " bytes\n");
	ftm_write(line, pos);
	*total += block->request_size;
}

static void	print_zone_header(const char *label, t_zone *zone)
{
	char	line[64];
	size_t	pos;

	pos = append(line, 0, label);
	pos = append(line, pos, " : 0x");
	pos += ftm_fmt_hex(line + pos, (uintptr_t)zone);
	pos = append(line, pos, "\n");
	ftm_write(line, pos);
}

static void	print_zone(const char *label, t_zone *zone, size_t *total)
{
	t_block	*block;
	bool	header_done;

	header_done = false;
	block = ftm_zone_first_block(zone);
	while (block != NULL)
	{
		if (!ftm_block_is_free(block))
		{
			if (!header_done)
			{
				print_zone_header(label, zone);
				header_done = true;
			}
			print_block(block, total);
		}
		block = block->next;
	}
}

static t_zone	*next_zone_by_address(t_zone *head, uintptr_t after)
{
	t_zone	*best;

	best = NULL;
	while (head != NULL)
	{
		if ((uintptr_t)head > after
			&& (best == NULL || (uintptr_t)head < (uintptr_t)best))
			best = head;
		head = head->next;
	}
	return (best);
}

static void	show_kind(const char *label, t_zone *head, size_t *total)
{
	t_zone		*zone;
	uintptr_t	after;

	after = 0;
	zone = next_zone_by_address(head, after);
	while (zone != NULL)
	{
		print_zone(label, zone, total);
		after = (uintptr_t)zone;
		zone = next_zone_by_address(head, after);
	}
}

void	ftm_show(void)
{
	t_heap	*heap;
	size_t	total;
	char	line[64];
	size_t	pos;

	heap = ftm_heap_instance();
	total = 0;
	show_kind("TINY", heap->zones[FTM_TINY], &total);
	show_kind("SMALL", heap->zones[FTM_SMALL], &total);
	show_kind("LARGE", heap->zones[FTM_LARGE], &total);
	pos = append(line, 0, "Total : ");
	pos += ftm_fmt_udec(line + pos, total);
	pos = append(line, pos, " bytes\n");
	ftm_write(line, pos);
}