/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_types.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 12:37:40 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 18:12:46 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FTM_TYPES_H
# define FTM_TYPES_H

# include "ftm_stdint.h"
# include "ftm_config.h"

typedef enum e_zone_kind
{
	FTM_TINY = 0,
	FTM_SMALL = 1,
	FTM_LARGE = 2,
	FTM_ZONE_KIND_COUNT = 3
} t_zone_kind;

# define FTM_BLOCK_FREE     (1U << 0)
# define FTM_BLOCK_CANARY   (1U << 1)

typedef struct s_block
{
	size_t          payload_size;
	size_t			request_size;
	struct s_block  *next;
	struct s_block  *prev;
	uintptr_t       flags;
} t_block;

typedef struct s_zone
{
	struct s_zone   *next;
	struct s_zone   *prev;
	size_t          total_size;
	t_zone_kind     kind;
} t_zone;

typedef struct s_heap
{
	t_zone  *zones[FTM_ZONE_KIND_COUNT];
	size_t  map_calls;
	size_t  unmap_calls;
	bool    is_initialized;
} t_heap;

# define FTM_BLOCK_HEADER_SIZE  FTM_ALIGN_UP(sizeof(t_block), FTM_ALIGNMENT)
# define FTM_ZONE_HEADER_SIZE   FTM_ALIGN_UP(sizeof(t_zone), FTM_ALIGNMENT)

_Static_assert(FTM_ALIGNMENT >= sizeof(void *),
	"alignment too small");
_Static_assert((FTM_ALIGNMENT & (FTM_ALIGNMENT - 1)) == 0,
	"alignment must be power of two");
_Static_assert(FTM_BLOCK_HEADER_SIZE % FTM_ALIGNMENT == 0,
	"block header size must be a multiple of alignment");
_Static_assert(FTM_ZONE_HEADER_SIZE % FTM_ALIGNMENT == 0,
	"zone header size must be a multiple of alignment");

#endif