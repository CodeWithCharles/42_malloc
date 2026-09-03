/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_errors.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 11:25:44 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 11:29:02 by cpoulain         ###   ########.fr       */
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

static int	test_double_free(void)
{
	void	*p;

	p = ftm_alloc(64);
	ftm_release(p);
	ftm_release(p);
	CHECK(ftm_check_heap());
	return (0);
}

static int	test_foreign_pointer(void)
{
	int	stack_value;

	ftm_release(&stack_value);
	CHECK(ftm_check_heap());
	return (0);
}

static int	test_mid_block_pointer(void)
{
	void	*p;

	p = ftm_alloc(64);
	ftm_release((unsigned char *)p + 8);
	ftm_release(p);
	CHECK(ftm_check_heap());
	return (0);
}

static int	test_overflow(void)
{
	CHECK(ftm_alloc(SIZE_MAX) == NULL);
	return (0);
}

static int	test_map_failure(void)
{
	ftm_alloc(64);
	fake_port_fail_after(0);
	CHECK(ftm_alloc(5000) == NULL);
	CHECK(ftm_check_heap());
	return (0);
}

int	main(void)
{
		fake_port_reset();
	ftm_heap_reset();
	if (test_double_free())
		return (1);
	fake_port_reset();
	ftm_heap_reset();
	if (test_foreign_pointer())
		return (1);
	fake_port_reset();
	ftm_heap_reset();
	if (test_mid_block_pointer())
		return (1);
	fake_port_reset();
	ftm_heap_reset();
	if (test_overflow())
		return (1);
	fake_port_reset();
	ftm_heap_reset();
	if (test_map_failure())
		return (1);
	printf("errors: OK\n");
	return (0);
}