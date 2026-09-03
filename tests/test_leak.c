/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_leak.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 11:50:00 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 12:12:17 by cpoulain         ###   ########.fr       */
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

#define N 500

int	main(void)
{
	void	*pointers[N];
	int		i;

	fake_port_reset();
	ftm_heap_reset();
	i = 0;
	while (i < N)
	{
		pointers[i] = ftm_alloc(1 + (size_t)(i * 7) % 4096);
		CHECK(pointers[i] != NULL);
		i++;
	}
	i = 0;
	while (i < N)
		ftm_release(pointers[i++]);
	CHECK(ftm_check_heap());
	ftm_heap_reset();
	CHECK(fake_port_map_count() == fake_port_unmap_count());
	printf("leak: OK\n");
	return (0);
}