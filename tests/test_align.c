/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_align.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 14:48:35 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 15:36:58 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"

#include <stdio.h>

#define CHECK(cond) do { \
	if (!(cond)) { \
		printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
		return (1); \
	} \
} while (0)

static int	test_round_up(void)
{
	CHECK(ftm_round_up_to_alignment(0) == 0);
	CHECK(ftm_round_up_to_alignment(1) == FTM_ALIGNMENT);
	CHECK(ftm_round_up_to_alignment(FTM_ALIGNMENT) == FTM_ALIGNMENT);
	CHECK(ftm_round_up_to_alignment(FTM_ALIGNMENT + 1) == 2 * FTM_ALIGNMENT);
	CHECK(ftm_round_up_to_alignment(SIZE_MAX) == 0);
	return (0);
}

static int	test_size_class(void)
{
	CHECK(ftm_size_class(1) == FTM_TINY);
	CHECK(ftm_size_class(FTM_TINY_MAX) == FTM_TINY);
	CHECK(ftm_size_class(FTM_TINY_MAX + 1) == FTM_SMALL);
	CHECK(ftm_size_class(FTM_SMALL_MAX) == FTM_SMALL);
	CHECK(ftm_size_class(FTM_SMALL_MAX + 1) == FTM_LARGE);
	return (0);
}

static int	test_accessors(void)
{
	unsigned char	buffer[FTM_BLOCK_HEADER_SIZE + 64];
	t_block			*block;
	void			*payload;

	block = (t_block *)buffer;
	block->payload_size = 64;
	payload = ftm_block_payload(block);
	CHECK((unsigned char *)payload == buffer + FTM_BLOCK_HEADER_SIZE);
	CHECK(ftm_payload_to_block(payload) == block);
	CHECK(ftm_block_end(block) == (unsigned char *)payload + 64);
	return (0);
}

static int	test_flags(void)
{
	unsigned char	buffer[FTM_BLOCK_HEADER_SIZE];
	t_block			*block;

	block = (t_block *)buffer;
	block->flags = 0;
	ftm_block_mark_free(block);
	CHECK(ftm_block_is_free(block));
	ftm_block_mark_used(block);
	CHECK(!ftm_block_is_free(block));
	return (0);
}

int main(void)
{
	CHECK(FTM_BLOCK_HEADER_SIZE % FTM_ALIGNMENT == 0);
	if (test_round_up() || test_size_class()
		|| test_accessors() || test_flags())
		return (1);
	printf("align: OK\n");
	return (0);
}