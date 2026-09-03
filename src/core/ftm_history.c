/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_history.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 15:14:09 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 15:19:55 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"

#define FTM_HISTORY_SIZE	256

typedef struct s_history_entry
{
	char	operation;
	void	*ptr;
	size_t	size;
} t_history_entry;

static t_history_entry	g_history[FTM_HISTORY_SIZE];
static size_t			g_history_count;
static size_t			g_history_next;

void	ftm_history_record(char operation, void *ptr, size_t size)
{
	g_history[g_history_next].operation = operation;
	g_history[g_history_next].ptr = ptr;
	g_history[g_history_next].size = size;
	g_history_next = (g_history_next + 1) % FTM_HISTORY_SIZE;
	if (g_history_count < FTM_HISTORY_SIZE)
		g_history_count++;
}

static void	dump_entry(t_history_entry *entry)
{
	char	line[80];
	size_t	pos;

	pos = 0;
	line[pos++] = ' ';
	line[pos++] = ' ';
	line[pos++] = entry->operation;
	line[pos++] = ' ';
	line[pos++] = '0';
	line[pos++] = 'x';
	pos += ftm_fmt_hex(line + pos, (uintptr_t)entry->ptr);
	line[pos++] = ' ';
	pos += ftm_fmt_udec(line + pos, entry->size);
	line[pos++] = '\n';
	ftm_write(line, pos);
}

void	ftm_history_dump(void)
{
	size_t	shown;
	size_t	index;

	ftm_write("--- history ---\n", 16);
	shown = 0;
	while (shown < g_history_count)
	{
		index = (g_history_next + FTM_HISTORY_SIZE - g_history_count + shown)
			% FTM_HISTORY_SIZE;
		dump_entry(&g_history[index]);
		shown++;
	}
}