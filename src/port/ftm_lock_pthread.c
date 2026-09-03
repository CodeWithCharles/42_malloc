/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_lock_pthread.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 14:38:42 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 14:43:05 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_port.h"

#include <pthread.h>

static pthread_mutex_t	g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t	g_atfork_once = PTHREAD_ONCE_INIT;

static void	lock_before_fork(void)
{
	pthread_mutex_lock(&g_mutex);
}

static void	unlock_after_fork(void)
{
	pthread_mutex_unlock(&g_mutex);
}

static void	register_atfork(void)
{
	pthread_atfork(lock_before_fork, unlock_after_fork, unlock_after_fork);
}

void	ftm_lock(void)
{
	pthread_once(&g_atfork_once, register_atfork);
	pthread_mutex_lock(&g_mutex);
}

void	ftm_unlock(void)
{
	pthread_mutex_unlock(&g_mutex);
}