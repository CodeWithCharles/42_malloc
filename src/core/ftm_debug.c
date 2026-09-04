/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_debug.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 14:51:58 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 13:08:40 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"

#define FTM_SCRIBBLE_BYTE	0xDE
#define FTM_CANARY_BYTE		0x55

static t_debug	g_debug;

t_debug	*ftm_debug(void)
{
	return (&g_debug);
}

void	ftm_report_error(const char *message)
{
	g_debug.error_count++;
	if (g_debug.abort_on_error)
		ftm_fatal(message);
}

static void	write_canary(t_block *block)
{
	unsigned char	*payload;
	size_t			i;

	payload = ftm_block_payload(block);
	i = block->request_size;
	while (i < block->payload_size)
		payload[i++] = FTM_CANARY_BYTE;
}

static bool	canary_intact(t_block *block)
{
	unsigned char	*payload;
	size_t			i;

	payload = ftm_block_payload(block);
	i = block->request_size;
	while (i < block->payload_size)
		if (payload[i++] != FTM_CANARY_BYTE)
			return(false);
	return (true);
}

void	ftm_on_alloc(t_block *block)
{
	if (g_debug.history)
		ftm_history_record('A', ftm_block_payload(block),
			block->request_size);
	if (g_debug.perturb_on)
		ftm_memset(ftm_block_payload(block), g_debug.perturb_byte,
			block->request_size);
	if (g_debug.guard)
		write_canary(block);
}

void	ftm_on_free(t_block *block)
{
	if (g_debug.history)
		ftm_history_record('F', ftm_block_payload(block),
			block->payload_size);
	if (g_debug.guard && !canary_intact(block))
		ftm_report_error("ft_malloc: buffer overflow detected on free");
	if (g_debug.scribble)
		ftm_memset(ftm_block_payload(block), FTM_SCRIBBLE_BYTE,
			block->payload_size);
}