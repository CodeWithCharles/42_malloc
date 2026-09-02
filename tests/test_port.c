/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_port.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 14:29:35 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 15:36:52 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_port.h"
#include "fake_port.h"

#include <stdio.h>

#define CHECK(cond) do { \
	if (!(cond)) { \
		printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
		return (1); \
	} \
} while (0)

int	main(void)
{
	void	*pages;
	size_t	length;

	fake_port_reset();
	CHECK(ftm_page_size() == 4096);

	length = 4 * ftm_page_size();
	pages = ftm_map_pages(length);
	CHECK(pages != NULL);
	CHECK(((uintptr_t)pages % ftm_page_size()) == 0);

	ftm_memset(pages, 0xAB, length);
	CHECK(((unsigned char *)pages)[0] == 0xAB);
	CHECK(((unsigned char *)pages)[length - 1] == 0xAB);

	ftm_unmap_pages(pages, length);
	CHECK(fake_port_map_count() == 1);
	CHECK(fake_port_unmap_count() == 1);

	fake_port_fail_after(0);
	CHECK(ftm_map_pages(length) == NULL);

	printf("port: OK\n");
	return (0);
}