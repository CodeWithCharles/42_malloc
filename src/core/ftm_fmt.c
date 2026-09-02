/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_fmt.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 17:52:23 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 17:55:33 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"

size_t	ftm_fmt_hex(char *dst, uintptr_t value)
{
	static const char	digits[] = "0123456789ABCDEF";
	char				tmp[16];
	size_t				length;
	size_t				i;

	if (value == 0)
	{
		dst[0] = '0';
		return (1);
	}
	length = 0;
	while (value != 0)
	{
		tmp[length++] = digits[value & 0xF];
		value >>= 4;
	}
	i = 0;
	while (i < length)
	{
		dst[i] = tmp[length - 1 - i];
		i++;
	}
	return (length);
}

size_t	ftm_fmt_udec(char *dst, size_t value)
{
	char	tmp[20];
	size_t	length;
	size_t	i;

	if (value == 0)
	{
		dst[0] = '0';
		return (1);
	}
	length = 0;
	while (value != 0)
	{
		tmp[length++] = (char)('0' + value % 10);
		value /= 10;
	}
	i = 0;
	while (i < length)
	{
		dst[i] = tmp[length - 1 - i];
		i++;
	}
	return (length);
}