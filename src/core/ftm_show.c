/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_show.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 17:55:49 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 15:22:19 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"

static bool	g_extended;

static bool	g_extended;

static void	print_hex_preview(t_block *block)
{
	static const char	digits[] = "0123456789ABCDEF";
	unsigned char		*p;
	char				line[80];
	size_t				i;
	size_t				j;
	size_t				pos;
	size_t				n;

	p = ftm_block_payload(block);
	n = block->request_size;
	if (n > 64)
		n = 64;
	i = 0;
	while (i < n)
	{
		pos = 0;
		line[pos++] = ' ';
		line[pos++] = ' ';
		j = 0;
		while (j < 16 && i + j < n)
		{
			line[pos++] = digits[p[i + j] >> 4];
			line[pos++] = digits[p[i + j] & 0xF];
			line[pos++] = ' ';
			j++;
		}
		line[pos++] = ' ';
		j = 0;
		while (j < 16 && i + j < n)
		{
			if (p[i + j] >= 32 && p[i + j] < 127)
				line[pos++] = (char)p[i + j];
			else
				line[pos++] = '.';
			j++;
		}
		line[pos++] = '\n';
		ftm_write(line, pos);
		i += 16;
	}
}

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
	if (g_extended)
		print_hex_preview(block);
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

static size_t	emit_listing(void)
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
	return (total);
}

static void	emit_stat(const char *label, size_t value)
{
	char	line[64];
	size_t	pos;

	pos = append(line, 0, label);
	pos += ftm_fmt_udec(line + pos, value);
	line[pos++] = '\n';
	ftm_write(line, pos);
}

static void	emit_stats(void)
{
	t_heap	*heap;
	t_zone	*zone;
	t_block	*block;
	size_t	stats[4];
	int		kind;

	heap = ftm_heap_instance();
	stats[0] = 0;
	stats[1] = 0;
	stats[2] = 0;
	stats[3] = 0;
	kind = 0;
	while (kind < FTM_ZONE_KIND_COUNT)
	{
		zone = heap->zones[kind];
		while (zone != NULL)
		{
			stats[0]++;
			block = ftm_zone_first_block(zone);
			while (block != NULL)
			{
				stats[1]++;
				if (ftm_block_is_free(block))
				{
					stats[2] += block->payload_size;
					if (block->payload_size > stats[3])
						stats[3] = block->payload_size;
				}
				block = block->next;
			}
			zone = zone->next;
		}
		kind++;
	}
	ftm_write("--- stats ---\n", 14);
	emit_stat("zones        : ", stats[0]);
	emit_stat("blocks       : ", stats[1]);
	emit_stat("free bytes   : ", stats[2]);
	emit_stat("largest free : ", stats[3]);
	emit_stat("mmap calls   : ", heap->map_calls);
	emit_stat("munmap calls : ", heap->unmap_calls);
}

void	ftm_show(void)
{
	g_extended = false;
	emit_listing();
}

void	ftm_show_ex(void)
{
	g_extended = true;
	emit_listing();
	g_extended = false;
	emit_stats();
	ftm_history_dump();
}