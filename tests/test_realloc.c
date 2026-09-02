/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_realloc.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 17:20:41 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 17:34:31 by cpoulain         ###   ########.fr       */
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

static void	fill(unsigned char *ptr, size_t n, unsigned char seed)
{
	size_t	i;

	i = 0;
	while (i < n)
	{
		ptr[i] = (unsigned char)(seed + i);
		i++;
	}
}

static int	verify(unsigned char *ptr, size_t n, unsigned char seed)
{
	size_t	i;

	i = 0;
	while (i < n)
	{
		if (ptr[i] != (unsigned char)(seed + i))
			return (0);
		i++;
	}
	return (1);
}

static int	test_null_and_zero(void)
{
	void	*p;

	CHECK(ftm_resize(NULL, 64) != NULL);
	p = ftm_alloc(64);
	CHECK(ftm_resize(p, 0) != NULL);
	return (0);
}

static int	test_grow_and_shrink_preserve(void)
{
	unsigned char	*p;

	p = ftm_alloc(64);
	fill(p, 64, 7);
	p = ftm_resize(p, 512);
	CHECK(p != NULL);
	CHECK(verify(p, 64, 7));
	p = ftm_resize(p, 32);
	CHECK(p != NULL);
	CHECK(verify(p, 32, 7));
	CHECK(ftm_check_heap());
	return (0);
}

static int	test_grow_in_place(void)
{
	void	*a;
	void	*b;
	void	*grown;

	a = ftm_alloc(64);
	b = ftm_alloc(64);
	ftm_release(b);
	grown = ftm_resize(a, 120);
	CHECK(grown == a);
	CHECK(ftm_check_heap());
	return (0);
}

static int	test_class_change(void)
{
	unsigned char	*p;

	p = ftm_alloc(64);
	fill(p, 64, 42);
	p = ftm_resize(p, 5000);
	CHECK(p != NULL);
	CHECK(ftm_heap_find_zone(p)->kind == FTM_LARGE);
	CHECK(verify(p, 64, 42));
	CHECK(ftm_check_heap());
	return (0);
}

int	main(void)
{
	fake_port_reset();
	ftm_heap_reset();
	if (test_null_and_zero())
		return (1);
	fake_port_reset();
	ftm_heap_reset();
	if (test_grow_and_shrink_preserve() || test_grow_in_place()
		|| test_class_change())
		return (1);
	printf("realloc: OK\n");
	return (0);
}