/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_thread.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 14:44:04 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 14:44:43 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"
#include "fake_port.h"

#include <pthread.h>
#include <stdio.h>

#define THREADS 8
#define OPS     20000
#define LIVE    32

static volatile int	g_failed = 0;

static void	*safe_alloc(size_t size)
{
	void	*ptr;

	ftm_lock();
	ptr = ftm_alloc(size);
	ftm_unlock();
	return (ptr);
}

static void	safe_release(void *ptr)
{
	ftm_lock();
	ftm_release(ptr);
	ftm_unlock();
}

static void	*worker(void *arg)
{
	unsigned char	*live[LIVE];
	size_t			sizes[LIVE];
	unsigned char	seeds[LIVE];
	unsigned long	rng;
	int				step;
	int				idx;
	size_t			k;

	rng = (unsigned long)(uintptr_t)arg * 2654435761UL + 12345UL;
	step = 0;
	while (step < LIVE)
		live[step++] = NULL;
	step = 0;
	while (step < OPS && !g_failed)
	{
		rng ^= rng << 13;
		rng ^= rng >> 7;
		rng ^= rng << 17;
		idx = (int)(rng % LIVE);
		if (live[idx] == NULL)
		{
			sizes[idx] = 1 + (rng >> 8) % 512;
			seeds[idx] = (unsigned char)(rng >> 3);
			live[idx] = safe_alloc(sizes[idx]);
			if (live[idx] == NULL)
			{
				g_failed = 1;
				return (NULL);
			}
			k = 0;
			while (k < sizes[idx])
			{
				live[idx][k] = (unsigned char)(seeds[idx] + k);
				k++;
			}
		}
		else
		{
			k = 0;
			while (k < sizes[idx])
			{
				if (live[idx][k] != (unsigned char)(seeds[idx] + k))
				{
					g_failed = 1;
					return (NULL);
				}
				k++;
			}
			safe_release(live[idx]);
			live[idx] = NULL;
		}
		step++;
	}
	idx = 0;
	while (idx < LIVE)
	{
		if (live[idx] != NULL)
			safe_release(live[idx]);
		idx++;
	}
	return (NULL);
}

int	main(void)
{
	pthread_t	threads[THREADS];
	int			i;

	fake_port_reset();
	ftm_heap_reset();
	i = 0;
	while (i < THREADS)
	{
		if (pthread_create(&threads[i], NULL, worker,
				(void *)(uintptr_t)i) != 0)
		{
			printf("FAIL pthread_create\n");
			return (1);
		}
		i++;
	}
	i = 0;
	while (i < THREADS)
		pthread_join(threads[i++], NULL);
	if (g_failed)
	{
		printf("FAIL data corruption under threads\n");
		return (1);
	}
	if (!ftm_check_heap())
	{
		printf("FAIL invariants after threads\n");
		return (1);
	}
	printf("thread: OK\n");
	return (0);
}