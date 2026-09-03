/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_aligned.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 12:20:41 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 12:27:18 by cpoulain         ###   ########.fr       */
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

static int	test_aligned_alloc_and_free(void)
{
	void	*p;
	int		i;

	i = 3;
	while (i < 16)
	{
		p = ftm_alloc_aligned((size_t)1 << i, 100);
		CHECK(p != NULL);
		CHECK((uintptr_t)p % ((size_t)1 << i) == 0);
		ftm_release(p);
		CHECK(ftm_check_heap());
		i++;
	}
	return (0);
}

static int	test_usable_size(void)
{
	void	*p;
	void	*a;

	p = ftm_alloc(100);
	CHECK(ftm_usable_size(p) >= 100);
	a = ftm_alloc_aligned(256, 100);
	CHECK(ftm_usable_size(a) >= 100);
	ftm_release(p);
	ftm_release(a);
	return (0);
}

static int	test_no_leak(void)
{
	void	*p[100];
	int		i;

	i = 0;
	while (i < 100)
		p[i++] = ftm_alloc_aligned(512, 200);
	i = 0;
	while (i < 100)
		ftm_release(p[i++]);
	CHECK(ftm_check_heap());
	ftm_heap_reset();
	CHECK(fake_port_map_count() == fake_port_unmap_count());
	return (0);
}

int	main(void)
{
	ftm_heap_reset();
	fake_port_reset();
	if (test_aligned_alloc_and_free())
		return (1);
	ftm_heap_reset();
	fake_port_reset();
	if (test_usable_size())
		return (1);
	ftm_heap_reset();
	fake_port_reset();
	if (test_no_leak())
		return (1);
	printf("aligned: OK\n");
	return (0);
}