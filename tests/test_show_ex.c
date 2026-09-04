/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_show_ex.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 15:23:20 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 13:11:46 by cpoulain         ###   ########.fr       */
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

int	main(void)
{
	char	*p;

	ftm_heap_reset();
	fake_port_reset();
	fake_capture_reset();
	ftm_debug()->history = true;

	p = ftm_alloc(20);
	strcpy(p, "hello");
	ftm_show_ex();

	CHECK(strstr(fake_capture_buffer(), "Total : ") != NULL);
	CHECK(strstr(fake_capture_buffer(), "--- stats ---") != NULL);
	CHECK(strstr(fake_capture_buffer(), "zones        : ") != NULL);
	CHECK(strstr(fake_capture_buffer(), "--- history ---") != NULL);
	CHECK(strstr(fake_capture_buffer(), "68 65 6C 6C 6F") != NULL);
	CHECK(strstr(fake_capture_buffer(), "hello") != NULL);

	ftm_release(p);
	printf("show_ex: OK\n");
	return (0);
}