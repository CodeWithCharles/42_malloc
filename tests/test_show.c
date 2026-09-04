/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_show.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 18:11:59 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 17:53:30 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"
#include "fake_port.h"

#include <stdio.h>
#include <string.h>

#define CHECK(cond) do { \
	if (!(cond)) { \
		printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
		return (1); \
	} \
} while (0)

static void	block_line(char *dst, void *ptr)
{
	t_block		*block;
	uintptr_t	start;
	size_t		pos;

	block = ftm_payload_to_block(ptr);
	start = (uintptr_t)ptr;
	pos = 0;
	dst[pos++] = '0';
	dst[pos++] = 'x';
	pos += ftm_fmt_hex(dst + pos, start);
	dst[pos++] = ' ';
	dst[pos++] = '-';
	dst[pos++] = ' ';
	dst[pos++] = '0';
	dst[pos++] = 'x';
	pos += ftm_fmt_hex(dst + pos, start + ftm_block_request(block));
	dst[pos++] = ' ';
	dst[pos++] = ':';
	dst[pos++] = ' ';
	pos += ftm_fmt_udec(dst + pos, ftm_block_request(block));
	dst[pos] = '\0';
}

int	main(void)
{
	void	*p1;
	void	*p2;
	char	expected[128];

	fake_port_reset();
	ftm_heap_reset();
	fake_capture_reset();

	p1 = ftm_alloc(42);
	p2 = ftm_alloc(84);
	(void)p2;
	ftm_show();

	CHECK(strstr(fake_capture_buffer(), "TINY : 0x") != NULL);
	block_line(expected, p1);
	CHECK(strstr(fake_capture_buffer(), expected) != NULL);
	CHECK(strstr(fake_capture_buffer(), "Total : 126 bytes") != NULL);

	fake_capture_reset();
	ftm_release(p1);
	ftm_show();
	block_line(expected, p1);
	CHECK(strstr(fake_capture_buffer(), expected) == NULL);
	CHECK(strstr(fake_capture_buffer(), "Total : 84 bytes") != NULL);

	printf("show: OK\n");
	return (0);
}