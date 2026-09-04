/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_config.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 11:53:58 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 18:05:28 by cpoulain         ###   ########.fr       */
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
#  define FTM_LARGE_CACHE_MAX_ZONES		64
# endif
# ifndef FTM_LARGE_CACHE_FIT_NUM
#  define FTM_LARGE_CACHE_FIT_NUM 2
# endif
# ifndef FTM_LARGE_CACHE_FIT_DEN
#  define FTM_LARGE_CACHE_FIT_DEN 1
# endif

/* --------------------------------- Mapping -------------------------------- */

# ifndef FTM_ZONE_MAP_CAPACITY
#  define FTM_ZONE_MAP_CAPACITY			2048
# endif
# define FTM_ZONE_MAP_MAX_LIVE			(FTM_ZONE_MAP_CAPACITY / 4 * 3)
# define FTM_ZONE_MAP_TOMBSTONE			((uintptr_t) - 1)

/* ---------------------------------- Zone ---------------------------------- */

# define FTM_MIN_ALLOCS     			100

# define FTM_ZONE_SIZE(max_alloc, page_size, block_hdr, zone_hdr) \
	FTM_ALIGN_UP((zone_hdr) + FTM_MIN_ALLOCS * ((block_hdr) + (max_alloc)), (page_size))

/* ------------------------------ Ablation ---------------------------------- */

# ifndef FTM_ENABLE_LARGE_CACHE
#  define FTM_ENABLE_LARGE_CACHE		1
# endif
# ifndef FTM_ENABLE_ZONE_MAP
#  define FTM_ENABLE_ZONE_MAP			1
# endif
# ifndef FTM_ENABLE_LARGE_FASTPATH
#  define FTM_ENABLE_LARGE_FASTPATH		1
# endif

# if FTM_ENABLE_LARGE_CACHE
#  define FTM_CACHE_TAKE(size)			ftm_large_cache_take(size)
#  define FTM_CACHE_PUT(zone)			ftm_large_cache_put(zone)
# else
#  define FTM_CACHE_TAKE(size)			(NULL)
#  define FTM_CACHE_PUT(zone)			(zone)
# endif

# if FTM_ENABLE_ZONE_MAP
#  define FTM_MAP_INSERT(zone)			ftm_zone_map_insert(zone)
#  define FTM_MAP_REMOVE(zone)			ftm_zone_map_remove(zone)
#  define FTM_MAP_ACTIVE()			ftm_zone_map_is_active()
# else
#  define FTM_MAP_INSERT(zone)			((void)(zone))
#  define FTM_MAP_REMOVE(zone)			((void)(zone))
#  define FTM_MAP_ACTIVE()			(0)
# endif

# if FTM_ENABLE_LARGE_FASTPATH
#  define FTM_SCANS_ZONES(kind)			((kind) != FTM_LARGE)
# else
#  define FTM_SCANS_ZONES(kind)			(1)
# endif

#endif
