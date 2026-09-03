/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_env.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 14:57:57 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 14:59:44 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_port.h"
#include "ftm_internal.h"
#include "libft.h"

#include <stdlib.h>

void	ftm_debug_load(void)
{
	t_debug	*config;
	char	*value;

	config = ftm_debug();
	if (getenv("FT_MALLOC_SCRIBBLE") != NULL)
		config->scribble = true;
	value = getenv("FT_MALLOC_PERTURB");
	if (value != NULL)
	{
		config->perturb_on = true;
		config->perturb_byte = (unsigned char)ft_atoi(value);
	}
	if (getenv("FT_MALLOC_GUARD") != NULL)
		config->guard = true;
	if (getenv("FT_MALLOC_ABORT") != NULL)
		config->abort_on_error = true;
}