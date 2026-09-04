/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_free_coalesce.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 16:13:10 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 13:17:07 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"
#include "fake_port.h"

#include <stdio.h>

#define CHECK(cond) do { \
	if (!(cond)) { \
		printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
		return (1); \
	} \
} while (0)

static size_t	count_blocks(t_zone *zone)
{
	t_block	*block;
	size_t	total;

	total = 0;
	block = ftm_zone_first_block(zone);
	while (block != NULL)
	{
		total++;
		block = block->next;
	}
	return (total);
}

static int	test_triple_coalesce(void)
{
	void	*a;
	void	*b;
	void	*c;
	t_zone	*zone;

	a = ftm_alloc(64);
	b = ftm_alloc(64);
	c = ftm_alloc(64);
	zone = ftm_heap_instance()->zones[FTM_TINY];
	CHECK(count_blocks(zone) == 4);
	CHECK(ftm_check_heap());

	ftm_release(b);
	CHECK(count_blocks(zone) == 4);
	CHECK(ftm_check_heap());

	ftm_release(a);
	CHECK(count_blocks(zone) == 3);
	CHECK(ftm_check_heap());

	ftm_release(c);
	CHECK(count_blocks(zone) == 1);
	CHECK(ftm_check_heap());
	return (0);
}

static int	test_large_is_unmapped(void)
{
	void	*large;

	large = ftm_alloc(5000);
	CHECK(ftm_heap_instance()->zone_count[FTM_LARGE] == 1);
	ftm_release(large);
	CHECK(ftm_heap_instance()->zone_count[FTM_LARGE] == 0);
	CHECK(ftm_check_heap());
	return (0);
}

static int	test_keep_one_tiny(void)
{
	void	*pointers[200];
	int		i;

	i = 0;
	while (i < 200)
		pointers[i++] = ftm_alloc(64);
	CHECK(ftm_heap_instance()->zone_count[FTM_TINY] == 2);
	i = 0;
	while (i < 200)
		ftm_release(pointers[i++]);
	CHECK(ftm_heap_instance()->zone_count[FTM_TINY] == 1);
	CHECK(ftm_check_heap());
	return (0);
}

int	main(void)
{
	fake_port_reset();
	ftm_heap_reset();
	if (test_triple_coalesce())
		return (1);
	fake_port_reset();
	ftm_heap_reset();
	if (test_large_is_unmapped())
		return (1);
	fake_port_reset();
	ftm_heap_reset();
	if (test_keep_one_tiny())
		return (1);
	ftm_release(NULL);
	printf("free_coalesce: OK\n");
	return (0);
}