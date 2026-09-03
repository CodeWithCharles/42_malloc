/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_debug.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 15:00:07 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 15:04:16 by cpoulain         ###   ########.fr       */
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

static void	setup(void)
{
	t_debug	*d;

	ftm_heap_reset();
	fake_port_reset();
	d = ftm_debug();
	d->scribble = false;
	d->perturb_on = false;
	d->guard = false;
	d->abort_on_error = false;
	d->perturb_byte = 0;
	d->error_count = 0;
}

static int	test_scribble(void)
{
	unsigned char	*p;

	setup();
	ftm_debug()->scribble = true;
	p = ftm_alloc(64);
	ftm_release(p);
	CHECK(p[0] == 0xDE && p[63] == 0xDE);
	return (0);
}

static int	test_perturb(void)
{
	unsigned char	*p;

	setup();
	ftm_debug()->perturb_on = true;
	ftm_debug()->perturb_byte = 0xAB;
	p = ftm_alloc(64);
	CHECK(p[0] == 0xAB && p[63] == 0xAB);
	ftm_release(p);
	return (0);
}

static int	test_guard_ok(void)
{
	void	*p;
	
	setup();
	ftm_debug()->guard = true;
	p = ftm_alloc(64);
	ftm_release(p);
	CHECK(ftm_debug()->error_count == 0);
	return (0);
}

static int	test_guard_overflow(void)
{
	unsigned char	*p;

	setup();
	ftm_debug()->guard = true;
	p = ftm_alloc(64);
	p[64] = 0x00;
	ftm_release(p);
	CHECK(ftm_debug()->error_count == 1);
	return (0);
}

int	main(void)
{
	if (test_scribble() || test_perturb()
		|| test_guard_ok() || test_guard_overflow())
		return (1);
	printf("debug: OK\n");
	return (0);
}