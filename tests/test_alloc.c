/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_alloc.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 15:46:29 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 15:51:45 by cpoulain         ###   ########.fr       */
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

#define TINY_COUNT	200

static size_t	count_zones(t_zone_kind kind)
{
	t_zone	*zone;
	size_t	total;

	total = 0;
	zone = ftm_heap_instance()->zones[kind];
	while (zone != NULL)
	{
		total++;
		zone = zone->next;
	}
	return (total);
}

static int	test_many_tiny(void)
{
	unsigned char	*pointers[TINY_COUNT];
	int				i;
	int				j;

	i = 0;
	while (i < TINY_COUNT)
	{
		pointers[i] = ftm_alloc(64);
		CHECK(pointers[i] != NULL);
		CHECK((uintptr_t)pointers[i] % FTM_ALIGNMENT == 0);
		pointers[i][0] = (unsigned char)i;
		i++;
	}
	i = 0;
	while (i < TINY_COUNT)
	{
		CHECK(pointers[i][0] == (unsigned char)i);
		j = i + 1;
		while (j < TINY_COUNT)
		{
			CHECK(pointers[i] != pointers[j]);
			j++;
		}
		i++;
	}
	CHECK(count_zones(FTM_TINY) == 2);
	return (0);
}

static int	test_routing(void)
{
	void	*small;
	void	*large;

	small = ftm_alloc(512);
	large = ftm_alloc(5000);
	CHECK(small != NULL && large != NULL);
	CHECK(count_zones(FTM_LARGE) == 1);
	return (0);
}

static int test_edge_sizes(void)
{
	void	*zero;
	
	zero = ftm_alloc(0);
	CHECK(zero != NULL);
	CHECK(ftm_alloc(SIZE_MAX) == NULL);
	return (0);
}

int	main(void)
{
	fake_port_reset();
	ftm_heap_reset();
	if (test_many_tiny() || test_routing() || test_edge_sizes())
		return (1);
	printf("alloc: OK\n");
	return (0);
}