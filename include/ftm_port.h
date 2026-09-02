/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_port.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 14:12:19 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 15:37:13 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FTM_PORT_H
# define FTM_PORT_H

# include "ftm_stdint.h"

size_t  ftm_page_size(void);
void    *ftm_map_pages(size_t length);
void    ftm_unmap_pages(void *addr, size_t length);

void    ftm_lock(void);
void    ftm_unlock(void);

void    ftm_write(const char *buffer, size_t length);

void    *ftm_memcpy(void *dst, const void *src, size_t length);
void    *ftm_memset(void *dst, int value, size_t length);

void    ftm_fatal(const char *message);

#endif