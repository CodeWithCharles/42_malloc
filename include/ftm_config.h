/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_config.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 11:53:58 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 12:07:46 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FTM_CONFIG_H
# define FTM_CONFIG_H

# include "ftm_stdint.h"

# define FTM_ALIGNMENT      (2 * sizeof(void *))
# define FTM_ALIGN_UP(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))

/* ---------------------------------- Sizes --------------------------------- */

# define FTM_TINY_MAX       			128
# ifndef FTM_SMALL_MAX
#  define FTM_SMALL_MAX      			2048
# endif

/* --------------------------------- Caching -------------------------------- */

# ifndef FTM_LARGE_CACHE_MAX_ZONES
#  define FTM_LARGE_CACHE_MAX_ZONES		32
# endif
# ifndef FTM_LARGE_CACHE_FIT_FACTOR
#  define FTM_LARGE_CACHE_FIT_FACTOR	2
# endif

/* --------------------------------- Mapping -------------------------------- */

# ifndef FTM_ZONE_MAP_CAPACITY
#  define FTM_ZONE_MAP_CAPACITY			8192
# endif
# define FTM_ZONE_MAP_MAX_LIVE			(FTM_ZONE_MAP_CAPACITY / 4 * 3)
# define FTM_ZONE_MAP_TOMBSTONE			((uintptr_t) - 1)

/* ---------------------------------- Zone ---------------------------------- */

# define FTM_MIN_ALLOCS     			100

# define FTM_ZONE_SIZE(max_alloc, page_size, block_hdr, zone_hdr) \
	FTM_ALIGN_UP((zone_hdr) + FTM_MIN_ALLOCS * ((block_hdr) + (max_alloc)), (page_size))

#endif