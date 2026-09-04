/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_env.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 14:57:57 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 13:14:32 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_port.h"
#include "ftm_internal.h"
#include "libft.h"

#include <stdlib.h>

static bool	env_flag(const char *name)
{
	char	*value;

	value = getenv(name);
	if (value == NULL || value[0] == '\0')
		return (false);
	return (!(value[0] == '0' && value[1] == '\0'));
}

void	ftm_debug_load(void)
{
	t_debug	*config;
	char	*value;

	config = ftm_debug();
	config->scribble = env_flag("FT_MALLOC_SCRIBBLE");
	config->guard = env_flag("FT_MALLOC_GUARD");
	config->abort_on_error = env_flag("FTM_MALLOC_ABORT");
	config->history = env_flag("FT_MALLOC_HISTORY");
	value = getenv("FT_MALLOC_PERTURB");
	if (value != NULL)
	{
		config->perturb_on = true;
		config->perturb_byte = (unsigned char)ft_atoi(value);
	}
}