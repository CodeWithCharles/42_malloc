/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fake_port.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 14:28:06 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 18:10:21 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FAKE_PORT_H
# define FAKE_PORT_H

# include "ftm_stdint.h"

void    	fake_port_reset(void);
void    	fake_port_fail_after(int successful_maps);
size_t  	fake_port_map_count(void);
size_t  	fake_port_unmap_count(void);

void		fake_capture_reset(void);
const char	*fake_capture_buffer(void);

#endif