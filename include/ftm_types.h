/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_types.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 12:37:40 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 14:49:27 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FTM_TYPES_H
# define FTM_TYPES_H

# include "ftm_stdint.h"
# include "ftm_config.h"

/* ---------------------------------- Debug --------------------------------- */

typedef struct s_debug
{
	bool			scribble;
	bool			perturb_on;
	bool			guard;
	bool			abort_on_error;
	unsigned char	perturb_byte;
	size_t			error_count;
} t_debug;

/* ---------------------------------- Enum ---------------------------------- */

typedef enum e_zone_kind
{
	FTM_TINY = 0,
	FTM_SMALL = 1,
	FTM_LARGE = 2,
	FTM_ZONE_KIND_COUNT = 3
} t_zone_kind;

/* ---------------------------------- Flags --------------------------------- */

# define FTM_BLOCK_FREE		((uintptr_t)0x01)
# define FTM_BLOCK_CANARY	((uintptr_t)0x02)
# define FTM_STATE_MASK		((uintptr_t)0xFF)
# define FTM_BLOCK_MAGIC	((uintptr_t)0x4D464C00)

/* --------------------------------- Structs -------------------------------- */

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

# define FTM_ALIGN_MAGIC		((uintptr_t)0x414C4D42)

typedef struct	s_align_tag
{
	uintptr_t	magic;
	void		*base;
} t_align_tag;

# define FTM_BLOCK_HEADER_SIZE  FTM_ALIGN_UP(sizeof(t_block), FTM_ALIGNMENT)
# define FTM_ZONE_HEADER_SIZE   FTM_ALIGN_UP(sizeof(t_zone), FTM_ALIGNMENT)

/* ------------------------------ Static checks ----------------------------- */

_Static_assert(FTM_ALIGNMENT >= sizeof(void *),
	"alignment too small");
_Static_assert((FTM_ALIGNMENT & (FTM_ALIGNMENT - 1)) == 0,
	"alignment must be power of two");
_Static_assert(FTM_BLOCK_HEADER_SIZE % FTM_ALIGNMENT == 0,
	"block header size must be a multiple of alignment");
_Static_assert(FTM_ZONE_HEADER_SIZE % FTM_ALIGNMENT == 0,
	"zone header size must be a multiple of alignment");

#endif