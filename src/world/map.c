/*****************************************************************************
 * Copyright (c) 2014 Ted John, Peter Hill
 * OpenRCT2, an open source clone of Roller Coaster Tycoon 2.
 *
 * This file is part of OpenRCT2.
 *
 * OpenRCT2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#include "../addresses.h"
#include "../audio/audio.h"
#include "../game.h"
#include "../interface/window.h"
#include "../localisation/date.h"
#include "../localisation/localisation.h"
#include "../management/finance.h"
#include "../scenario.h"
#include "banner.h"
#include "climate.h"
#include "footpath.h"
#include "map.h"
#include "map_animation.h"
#include "park.h"
#include "scenery.h"

const rct_xy16 TileDirectionDelta[] = {
	{ -32,   0 },
	{   0, +32 },
	{ +32,   0 },
	{   0, -32 },
	{ -32, +32 },
	{ +32, +32 },
	{ +32, -32 },
	{ -32, -32 }
};

rct_xy16 *gMapSelectionTiles = (rct_xy16*)0x009DE596;

bool LandPaintMode;
bool LandRightsMode;
bool gClearSmallScenery;
bool gClearLargeScenery;
bool gClearFootpath;

int _sub_6A876D_save_x;
int _sub_6A876D_save_y;

static void tiles_init();
static void sub_6A87BB(int x, int y);
static void map_update_grass_length(int x, int y, rct_map_element *mapElement);
static void map_set_grass_length(int x, int y, rct_map_element *mapElement, int length);

void rotate_map_coordinates(sint16* x, sint16* y, uint8 rotation){
	int temp;
	switch (rotation){
	case MAP_ELEMENT_DIRECTION_WEST:
		break;
	case MAP_ELEMENT_DIRECTION_NORTH:
		temp = *x;
		*x = *y;
		*y = -temp;
		break;
	case MAP_ELEMENT_DIRECTION_EAST:
		*x = -*x;
		*y = -*y;
		break;
	case MAP_ELEMENT_DIRECTION_SOUTH:
		temp = *y;
		*y = *x;
		*x = -temp;
		break;
	}
}

void map_element_iterator_begin(map_element_iterator *it)
{
	it->x = 0;
	it->y = 0;
	it->element = map_get_first_element_at(0, 0);
}

int map_element_iterator_next(map_element_iterator *it)
{
	if (it->element == NULL) {
		it->element = map_get_first_element_at(it->x, it->y);
		return 1;
	}

	if (!map_element_is_last_for_tile(it->element)) {
		it->element++;
		return 1;
	}

	if (it->x < 255) {
		it->x++;
		it->element = map_get_first_element_at(it->x, it->y);
		return 1;
	}

	if (it->y < 255) {
		it->x = 0;
		it->y++;
		it->element = map_get_first_element_at(it->x, it->y);
		return 1;
	}

	return 0;
}

void map_element_iterator_restart_for_tile(map_element_iterator *it)
{
	it->element = NULL;
}

rct_map_element *map_get_first_element_at(int x, int y)
{
	if (x < 0 || y < 0 || x > 255 || y > 255)
	{ 
		log_error("Trying to access element outside of range"); 
		return NULL;
	}
	return TILE_MAP_ELEMENT_POINTER(x + y * 256);
}

int map_element_is_last_for_tile(rct_map_element *element)
{
	return element->flags & MAP_ELEMENT_FLAG_LAST_TILE;
}

int map_element_get_type(rct_map_element *element)
{
	return element->type & MAP_ELEMENT_TYPE_MASK;
}

int map_element_get_terrain(rct_map_element *element)
{
	int terrain = (element->properties.surface.terrain >> 5) & 7;
	if (element->type & 1)
		terrain |= (1 << 3);
	return terrain;
}

int map_element_get_terrain_edge(rct_map_element *element)
{
	int terrain_edge = (element->properties.surface.slope >> 5) & 7;
	if (element->type & 128)
		terrain_edge |= (1 << 3);
	return terrain_edge;
}

void map_element_set_terrain(rct_map_element *element, int terrain)
{
	// Bit 3 for terrain is stored in element.type bit 0
	if (terrain & 8)
		element->type |= 1;
	else
		element->type &= ~1;

	// Bits 0, 1, 2 for terrain are stored in element.terrain bit 5, 6, 7
	element->properties.surface.terrain &= ~0xE0;
	element->properties.surface.terrain |= (terrain & 7) << 5;
}

void map_element_set_terrain_edge(rct_map_element *element, int terrain)
{
	// Bit 3 for terrain is stored in element.type bit 7
	if (terrain & 8)
		element->type |= 128;
	else
		element->type &= ~128;

	// Bits 0, 1, 2 for terrain are stored in element.slope bit 5, 6, 7
	element->properties.surface.slope &= ~0xE0;
	element->properties.surface.slope |= (terrain & 7) << 5;
}

rct_map_element *map_get_surface_element_at(int x, int y)
{
	rct_map_element *mapElement = map_get_first_element_at(x, y);

	if (mapElement == NULL)
		return NULL;

	// Find the first surface element
	while (map_element_get_type(mapElement) != MAP_ELEMENT_TYPE_SURFACE) {
		if (map_element_is_last_for_tile(mapElement))
			return NULL;

		mapElement++;
	}

	return mapElement;
}

rct_map_element* map_get_path_element_at(int x, int y, int z){
	rct_map_element *mapElement = map_get_first_element_at(x, y);

	if (mapElement == NULL)
		return NULL;

	uint8 mapFound = 0;
	// Find the path element at known z
	do {
		if (map_element_get_type(mapElement) != MAP_ELEMENT_TYPE_PATH)
			continue;
		if (mapElement->base_height != z)
			continue;

		return mapElement;
	} while (!map_element_is_last_for_tile(mapElement++));

	return NULL;
}
/**
 * 
 *  rct2: 0x0068AB4C
 */
void map_init(int size)
{
	int i;
	rct_map_element *map_element;

	date_reset();
	RCT2_GLOBAL(0x0138B580, sint16) = 0;
	RCT2_GLOBAL(0x010E63B8, sint32) = 0;

	for (i = 0; i < MAX_TILE_MAP_ELEMENT_POINTERS; i++) {
		map_element = GET_MAP_ELEMENT(i);
		map_element->type = (MAP_ELEMENT_TYPE_SURFACE << 2);
		map_element->flags = MAP_ELEMENT_FLAG_LAST_TILE;
		map_element->base_height = 14;
		map_element->clearance_height = 14;
		map_element->properties.surface.slope = 0;
		map_element->properties.surface.grass_length = 1;
		map_element->properties.surface.ownership = 0;
		map_element->properties.surface.terrain = 0;

		map_element_set_terrain(map_element, TERRAIN_GRASS);
		map_element_set_terrain_edge(map_element, TERRAIN_EDGE_ROCK);
	}

	RCT2_GLOBAL(RCT2_ADDRESS_GRASS_SCENERY_TILEPOS, sint16) = 0;
	_sub_6A876D_save_x = 0;
	_sub_6A876D_save_y = 0;
	RCT2_GLOBAL(0x01358830, sint16) = size * 32 - 32;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_MAXIMUM_X_Y, sint16) = size * 32 - 2;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_SIZE, sint16) = size;
	RCT2_GLOBAL(0x01358836, sint16) = size * 32 - 33;
	RCT2_GLOBAL(0x01359208, sint16) = 7;
	map_update_tile_pointers();
	RCT2_CALLPROC_EBPSAFE(0x0068ADBC);

	climate_reset(CLIMATE_WARM);
}

/**
 * 
 *  rct2: 0x0068AFFD
 */
void map_update_tile_pointers()
{
	int i, x, y;

	for (i = 0; i < MAX_TILE_MAP_ELEMENT_POINTERS; i++)
		TILE_MAP_ELEMENT_POINTER(i) = TILE_UNDEFINED_MAP_ELEMENT;

	rct_map_element *mapElement = RCT2_ADDRESS(RCT2_ADDRESS_MAP_ELEMENTS, rct_map_element);
	rct_map_element **tile = RCT2_ADDRESS(RCT2_ADDRESS_TILE_MAP_ELEMENT_POINTERS, rct_map_element*);
	for (y = 0; y < 256; y++) {
		for (x = 0; x < 256; x++) {
			*tile++ = mapElement;
			do { } while (!map_element_is_last_for_tile(mapElement++));
		}
	}

	// Possible next free map element
	RCT2_GLOBAL(0x0140E9A4, rct_map_element*) = mapElement;
}

/**
 * Return the absolute height of an element, given its (x,y) coordinates
 *
 *  ax: x
 *  cx: y
 *  dx: return remember to & with 0xFFFF if you don't want water affecting results
 *  rct2: 0x00662783
 */
int map_element_height(int x, int y)
{
	rct_map_element *mapElement;

	// Off the map
	if (x >= 8192 || y >= 8192)
		return 16;

	// Truncate subtile coordinates
	int x_tile = x & 0xFFFFFFE0;
	int y_tile = y & 0xFFFFFFE0;

	// Get the surface element for the tile
	mapElement = map_get_surface_element_at(x_tile / 32, y_tile / 32);

	uint32 height =
		((mapElement->properties.surface.terrain & MAP_ELEMENT_WATER_HEIGHT_MASK) << 20) |
		(mapElement->base_height << 3);

	uint32 slope = (mapElement->properties.surface.slope & MAP_ELEMENT_SLOPE_MASK);
	uint8 extra_height = (slope & 0x10) >> 4; // 0x10 is the 5th bit - sets slope to double height
	// Remove the extra height bit
	slope &= 0xF;

	sint8 quad, quad_extra; // which quadrant the element is in?
	                        // quad_extra is for extra height tiles

	uint8 xl, yl;	    // coordinates across this tile

	uint8 TILE_SIZE = 31;

	xl = x & 0x1f;
	yl = y & 0x1f;

	// Slope logic:
	// Each of the four bits in slope represents that corner being raised
	// slope == 15 (all four bits) is not used and slope == 0 is flat
	// If the extra_height bit is set, then the slope goes up two z-levels

	// We arbitrarily take the SW corner to be closest to the viewer

	// One corner up
	if ((slope == 1) || (slope == 2) || (slope == 4) || (slope == 8)) {
		switch (slope) {
		case 1:    // NE corner up
			quad = xl + yl - TILE_SIZE;
			break;
		case 2:    // SE corner up
			quad = xl - yl;
			break;
		case 4:   // SW corner up
			quad = TILE_SIZE - yl - xl;
			break;
		case 8:   // NW corner up
			quad = yl - xl;
			break;
		}
		// If the element is in the quadrant with the slope, raise its height
		if (quad > 0) {
			height += quad / 2;
		}
	}

	// One side up
	switch (slope) {
	case 3:   // E side up
		height += xl / 2 + 1;
		break;
	case 6:   // S side up
		height += (TILE_SIZE - yl) / 2;
		break;
	case 9:    // N side up
		height += yl / 2;
		height++;
		break;
	case 12:  // W side up
		height += (TILE_SIZE - xl) / 2;
		break;
	}

	// One corner down
	if ((slope == 7) || (slope == 11) || (slope == 13) || (slope == 14)) {
		switch (slope) {
		case 7:   // NW corner down
			quad_extra = xl + TILE_SIZE - yl;
			quad = xl - yl;
			break;
		case 11:  // SW corner down
			quad_extra = xl + yl;
			quad = xl + yl - TILE_SIZE - 1;
			break;
		case 13:  // SE corner down
			quad_extra = TILE_SIZE - xl + yl;
			quad = yl - xl;
			break;
		case 14:  // NE corner down
			quad_extra = (TILE_SIZE - xl) + (TILE_SIZE - yl);
			quad = TILE_SIZE - yl - xl - 1;
			break;
		}

		if (extra_height) {
			height += quad_extra / 2;
			height++;
			return height;
		}
		// This tile is essentially at the next height level
		height += 0x10;
		// so we move *down* the slope
		if (quad < 0) {
			height += quad / 2;
		}
	}

	// Valleys
	if ((slope == 5) || (slope == 10)) {
		switch (slope) {
		case 5:  // NW-SE valley
			if (xl + yl <= TILE_SIZE + 1) {
				return height;
			}
			quad = TILE_SIZE - xl - yl;
			break;
		case 10: // NE-SW valley
			quad = xl - yl;
			break;
		}
		if (quad > 0) {
			height += quad / 2;
		}
	}

	return height;
}

/**
 *
 *  rct2: 0x0068B089
 */
void sub_68B089()
{
	int i;
	rct_map_element *mapElementFirst, *mapElement;

	if (RCT2_GLOBAL(0x009DEA6F, uint8) & 1)
		return;

	i = RCT2_GLOBAL(0x0010E63B8, uint32);
	do {
		i++;
		if (i >= MAX_TILE_MAP_ELEMENT_POINTERS)
			i = 0;
	} while (TILE_MAP_ELEMENT_POINTER(i) == TILE_UNDEFINED_MAP_ELEMENT);
	RCT2_GLOBAL(0x0010E63B8, uint32) = i;

	mapElementFirst = mapElement = TILE_MAP_ELEMENT_POINTER(i);
	do {
		mapElement--;
		if (mapElement < (rct_map_element*)RCT2_ADDRESS_MAP_ELEMENTS)
			break;
	} while (mapElement->base_height == 255);
	mapElement++;

	if (mapElement == mapElementFirst)
		return;

	// 
	TILE_MAP_ELEMENT_POINTER(i) = mapElement;
	do {
		*mapElement = *mapElementFirst;
		mapElementFirst->base_height = 255;

		mapElement++;
		mapElementFirst++;
	} while (!map_element_is_last_for_tile(mapElement - 1));

	// Update next free element?
	mapElement = RCT2_GLOBAL(0x0140E9A4, rct_map_element*);
	do {
		mapElement--;
	} while (mapElement->base_height == 255);
	mapElement++;
	RCT2_GLOBAL(0x0140E9A4, rct_map_element*) = mapElement;
}


/**
 * Checks if the tile at coordinate at height counts as connected.
 * @return 1 if connected, 0 otherwise
 */
int map_coord_is_connected(int x, int y, int z, uint8 faceDirection)
{
	rct_map_element *mapElement = map_get_first_element_at(x, y);

	do {
		if (map_element_get_type(mapElement) != MAP_ELEMENT_TYPE_PATH)
			continue;

		rct_map_element_path_properties props = mapElement->properties.path;
		uint8 pathType = props.type >> 2;
		uint8 pathDirection = props.type & 3;

		if (pathType & 1) {
			if (pathDirection == faceDirection) {
				if (z == mapElement->base_height + 2)
					return 1;
			} else if ((pathDirection ^ 2) == faceDirection && z == mapElement->base_height) {
				return 1;
			}
		} else {
			if (z == mapElement->base_height)
				return 1;
		}
	} while (!map_element_is_last_for_tile(mapElement++));

	return 0;
}

/**
 *
 *  rct2: 0x006A876D
 */
void sub_6A876D()
{
	int i, x, y;

	if (RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & (SCREEN_FLAGS_TRACK_DESIGNER | SCREEN_FLAGS_TRACK_MANAGER))
		return;

	// Presumebly sub_6A87BB is too computationally expensive to call for every
	// tile every update, so word_13CE774 and word_13CE776 store the x and y
	// progress. A maximum of 128 calls is done per update.
	x = _sub_6A876D_save_x;
	y = _sub_6A876D_save_y;
	for (i = 0; i < 128; i++) {
		sub_6A87BB(x, y);

		// Next x, y tile
		x += 32;
		if (x >= 8192) {
			x = 0;
			y += 32;
			if (y >= 8192)
				y = 0;
		}
	}
	_sub_6A876D_save_x = x;
	_sub_6A876D_save_y = y;
}

/**
 *
 *  rct2: 0x006A87BB
 */
static void sub_6A87BB(int x, int y)
{
	RCT2_CALLPROC_X(0x006A87BB, x, 0, y, 0, 0, 0, 0);
}

/**
 *
 *  rct2: 0x006A7B84
 */
int map_height_from_slope(int x, int y, int slope)
{
	if (!(slope & 4))
		return 0;

	switch (slope & 3) {
	case 0:
		return (31 - (x & 31)) / 2;
	case 1:
		return (y & 31) / 2;
	case 2:
		return (x & 31) / 2;
	case 3:
		return (31 - (y & 31)) / 2;
	}
	return 0;
}

/**
 *
 *  rct2: 0x00664F72
 */
int map_is_location_owned(int x, int y, int z)
{
	rct_map_element *mapElement;

	if (x < (256 * 32) && y < (256 * 32)) {
		mapElement = map_get_surface_element_at(x / 32, y / 32);
		if (mapElement->properties.surface.ownership & OWNERSHIP_OWNED)
			return 1;

		if (mapElement->properties.surface.ownership & OWNERSHIP_CONSTRUCTION_RIGHTS_OWNED) {
			z /= 8;
			if (z < mapElement->base_height || z - 2 > mapElement->base_height)
				return 1;
		}
	}

	RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = 1729;
	return 0;
}

/**
 *
 *  rct2: 0x00664F2C
 */
int map_is_location_in_park(int x, int y)
{
	rct_map_element *mapElement;

	if (x < (256 * 32) && y < (256 * 32)) {
		mapElement = map_get_surface_element_at(x / 32, y / 32);
		if (mapElement->properties.surface.ownership & OWNERSHIP_OWNED)
			return 1;
	}

	RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = 1729;
	return 0;
}

/**
 *
 *  rct2: 0x006ECB60
 * NOTE: x, y and z are in pixels, not tile units
 */
void map_invalidate_tile(int x, int y, int zLow, int zHigh)
{
	RCT2_CALLPROC_X(0x006ECB60, x, 0, y, 0, zHigh, zLow, 0);
}

/**
 *
 *  rct2: 0x006E0E01
 */
void game_command_remove_scenery(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	int x = *eax;
	int y = *ecx;
	uint8 base_height = *edx;
	uint8 scenery_type = *edx >> 8;
	uint8 map_element_type = *ebx >> 8;
	money32 cost;
	
	rct_scenery_entry *entry = g_smallSceneryEntries[scenery_type];
	cost = entry->small_scenery.removal_price * 10;

	RCT2_GLOBAL(RCT2_ADDRESS_NEXT_EXPENDITURE_TYPE, uint8) = RCT_EXPENDITURE_TYPE_LANDSCAPING * 4;
	RCT2_GLOBAL(0x009DEA5E, uint32) = x + 16;
	RCT2_GLOBAL(0x009DEA60, uint32) = y + 16;
	RCT2_GLOBAL(0x009DEA62, uint32) = base_height * 8;

	if (!(*ebx & 0x40) && RCT2_GLOBAL(RCT2_ADDRESS_GAME_PAUSED, uint8) != 0) {
		RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_CONSTRUCTION_NOT_POSSIBLE_WHILE_GAME_IS_PAUSED;
		*ebx = MONEY32_UNDEFINED;
		return;
	}

	if (!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR) && !(*ebx & 0x40)) {
		// Check if allowed to remove item
		if (RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_FORBID_TREE_REMOVAL) {
			if (entry->small_scenery.height > 64) {
				RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_FORBIDDEN_BY_THE_LOCAL_AUTHORITY;
				*ebx = MONEY32_UNDEFINED;
				return;
			}
		}

		// Check if the land is owned
		if (!map_is_location_owned(x, y, RCT2_GLOBAL(0x009DEA62, uint32))){
			*ebx = MONEY32_UNDEFINED;
			return;
		}
	}

	rct_map_element* map_element = map_get_first_element_at(x / 32, y / 32);
	while(map_element->type != map_element_type ||
		map_element->base_height != base_height ||
		map_element->properties.scenery.type != scenery_type ||
		(*ebx & 0x40) && !(map_element->flags & MAP_ELEMENT_FLAG_5)){
		map_element++;
		if((map_element - 1)->flags & MAP_ELEMENT_FLAG_LAST_TILE){
			*ebx = 0;
			return;
		}
	}

	// Remove element
	if (*ebx & GAME_COMMAND_FLAG_APPLY) {
		map_invalidate_tile_full(x, y);
		map_element_remove(map_element);
	}
	*ebx = (RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_NO_MONEY ? 0 : cost);
}

/**
 *
 *  rct2: 0x006B8E1B
 */
void game_command_remove_large_scenery(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	uint8 base_height = *edx;
	uint8 scenerymultiple_index = *edx >> 8;
	uint8 map_element_direction = *ebx >> 8;
	int x = *eax;
	int y = *ecx;
	int z = map_element_height(x, y);
	RCT2_GLOBAL(0x009DEA5E, uint16) = x + 16;
	RCT2_GLOBAL(0x009DEA60, uint16) = y + 16;
	RCT2_GLOBAL(0x009DEA62, uint16) = z;
	RCT2_GLOBAL(RCT2_ADDRESS_NEXT_EXPENDITURE_TYPE, uint8) = 12;
	
	if (!(*ebx & 0x40) && RCT2_GLOBAL(RCT2_ADDRESS_GAME_PAUSED, uint8) != 0) {
		RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_CONSTRUCTION_NOT_POSSIBLE_WHILE_GAME_IS_PAUSED;
		*ebx = MONEY32_UNDEFINED;
		return;
	}

	uint8 element_found = 0;
	rct_map_element* map_element = map_get_first_element_at(x / 32, y / 32);
	do {
		if (map_element_get_type(map_element) != MAP_ELEMENT_TYPE_SCENERY_MULTIPLE)
			continue;

		if (map_element->base_height != base_height)
			continue;

		if ((map_element->properties.scenerymultiple.type >> 10) != scenerymultiple_index)
			continue;

		if ((map_element->type & MAP_ELEMENT_DIRECTION_MASK) != map_element_direction)
			continue;

		element_found = 1;
		break;
	} while (!map_element_is_last_for_tile(map_element++));

	if (!element_found){
		*ebx = 0;
		return;
	}

	if((*ebx & 0x40) && !(map_element->flags & MAP_ELEMENT_FLAG_5)){
		*ebx = 0;
		return;
	}
	int ecx2 = map_element->properties.scenerymultiple.type >> 10;
	rct_scenery_entry* scenery_entry = RCT2_ADDRESS(RCT2_ADDRESS_LARGE_SCENERY_ENTRIES, rct_scenery_entry*)[map_element->properties.scenerymultiple.type & 0x3FF];
	if(scenery_entry->large_scenery.var_11 != 0xFF){
		uint8 banner_num = map_element->type & MAP_ELEMENT_QUADRANT_MASK;
		banner_num |= (map_element->properties.scenerymultiple.colour[0] & 0xE0) >> 2;
		banner_num |= (map_element->properties.scenerymultiple.colour[1] & 0xE0) >> 5;
		if(gBanners[banner_num].type != BANNER_NULL){
			window_close_by_number(WC_BANNER, banner_num);
			gBanners[banner_num].type = BANNER_NULL;
			user_string_free(gBanners[banner_num].string_idx);
		}
	}

	rct_xyz16 firstTile = { 
		.x = scenery_entry->large_scenery.tiles[ecx2].x_offset, 
		.y = scenery_entry->large_scenery.tiles[ecx2].y_offset,
		.z = (base_height * 8) - scenery_entry->large_scenery.tiles[ecx2].z_offset
	};

	rotate_map_coordinates(&firstTile.x, &firstTile.y, map_element_direction);

	firstTile.x = x - firstTile.x;
	firstTile.y = y - firstTile.y;

	for (int i = 0; scenery_entry->large_scenery.tiles[i].x_offset != -1; i++){

		rct_xyz16 currentTile = {
			.x = scenery_entry->large_scenery.tiles[i].x_offset,
			.y = scenery_entry->large_scenery.tiles[i].y_offset,
			.z = scenery_entry->large_scenery.tiles[i].z_offset
		};

		rotate_map_coordinates(&currentTile.x, &currentTile.y, map_element_direction);

		currentTile.x += firstTile.x;
		currentTile.y += firstTile.y;
		currentTile.z += firstTile.z;

		if (!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR)){
			if (!map_is_location_owned(currentTile.x, currentTile.y, currentTile.z)){
				*ebx = MONEY32_UNDEFINED;
				return;
			}
		}

		// If not applying then no need to delete the actual element
		if (!(*ebx & GAME_COMMAND_FLAG_APPLY))
			continue;
		
		rct_map_element* sceneryElement = map_get_first_element_at(currentTile.x / 32, currentTile.y / 32);
		uint8 tile_not_found = 1;
		do
		{
			if (map_element_get_type(sceneryElement) != MAP_ELEMENT_TYPE_SCENERY_MULTIPLE)
				continue;

			if ((sceneryElement->type & MAP_ELEMENT_DIRECTION_MASK) != map_element_direction)
				continue;

			if ((sceneryElement->properties.scenerymultiple.type >> 10) != i)
				continue;

			if (sceneryElement->base_height != currentTile.z / 8)
				continue;

			map_invalidate_tile_full(currentTile.x, currentTile.y);
			map_element_remove(sceneryElement);
			tile_not_found = 0;
			break;
		} while (!map_element_is_last_for_tile(sceneryElement++));

		if (tile_not_found){
			log_error("Tile not found when trying to remove element!");
		}
	}

	*ebx = scenery_entry->large_scenery.removal_price * 10;
	if (RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_NO_MONEY){
		*ebx = 0;
	}
	return;
}

/**
 *
 *  rct2: 0x006BA058
 */
void game_command_remove_banner(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	int x = *eax;
	int y = *ecx;
	uint8 base_height = *edx;
	uint8 banner_position = *edx >> 8;
	int z = base_height * 8;
	RCT2_GLOBAL(RCT2_ADDRESS_NEXT_EXPENDITURE_TYPE, uint8) = 12;
 	RCT2_GLOBAL(0x009DEA5E, uint16) = x + 16;
	RCT2_GLOBAL(0x009DEA60, uint16) = y + 16;
	RCT2_GLOBAL(0x009DEA62, uint16) = z;
	if(!(*ebx & 0x40) && RCT2_GLOBAL(0x009DEA6E, uint8) != 0){
		RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_CONSTRUCTION_NOT_POSSIBLE_WHILE_GAME_IS_PAUSED;
		*ebx = MONEY32_UNDEFINED;
		return;
	}
	if(!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR) && !map_is_location_owned(x, y, z - 16)){
		*ebx = MONEY32_UNDEFINED;
		return;
	}
	rct_map_element* map_element = map_get_first_element_at(x / 32, y / 32);
	while(map_element->type != MAP_ELEMENT_TYPE_BANNER ||
		map_element->properties.banner.position != banner_position){
		map_element++;
		if((map_element - 1)->flags & MAP_ELEMENT_FLAG_LAST_TILE){
			*ebx = MONEY32_UNDEFINED;
			return;
		}
	}
	rct_banner *banner = &gBanners[map_element->properties.banner.index];
	uint8 banner_type = banner->type;
	if(*ebx & GAME_COMMAND_FLAG_APPLY){
		window_close_by_number(WC_BANNER, map_element->properties.banner.index);
		user_string_free(banner->string_idx);
		banner->type = BANNER_NULL;
		map_invalidate_tile(x, y, z, z + 32);
		map_element_remove(map_element);
	}
	rct_scenery_entry *scenery_entry = (rct_scenery_entry*)object_entry_groups[OBJECT_TYPE_BANNERS].chunks[banner_type];
	*ebx = (scenery_entry->banner.price * -3) / 4;
	if(RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_NO_MONEY){
		*ebx = 0;
	}
}

/**
 *
 *  rct2: 0x006E0F26
 */
void game_command_set_scenery_colour(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	RCT2_GLOBAL(RCT2_ADDRESS_NEXT_EXPENDITURE_TYPE, uint8) = 12;
	int x = *eax;
	int y = *ecx;
	uint8 base_height = *edx;
	uint8 scenery_type = *edx >> 8;
	uint8 map_element_type = *ebx >> 8;
	uint8 color1 = *ebp;
	uint8 color2 = *ebp >> 8;
	int z = base_height * 8;
	RCT2_GLOBAL(0x009DEA5E, uint16) = x + 16;
	RCT2_GLOBAL(0x009DEA60, uint16) = y + 16;
	RCT2_GLOBAL(0x009DEA62, uint16) = z;
	if (!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR)){
		if (!map_is_location_owned(x, y, z)){
			*ebx = MONEY32_UNDEFINED;
			return;
		}
	}

	rct_map_element* map_element = map_get_first_element_at(x / 32, y / 32);
	while(map_element->type != map_element_type ||
		map_element->base_height != base_height ||
		map_element->properties.scenery.type != scenery_type){
		map_element++;
		if((map_element - 1)->flags & MAP_ELEMENT_FLAG_LAST_TILE){
			*ebx = 0;
			return;
		}
	}
	if((*ebx & 0x40) && !(map_element->flags & MAP_ELEMENT_FLAG_5)){
		*ebx = 0;
		return;
	}
	if(*ebx & GAME_COMMAND_FLAG_APPLY){
		map_element->properties.scenery.colour_1 &= 0xE0;
		map_element->properties.scenery.colour_1 |= color1;
		map_element->properties.scenery.colour_2 &= 0xE0;
		map_element->properties.scenery.colour_2 |= color2;
		map_invalidate_tile_full(x, y);
	}
	
	*ebx = 0;
}

/**
 *
 *  rct2: 0x006E56B5
 */
void game_command_set_fence_colour(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	RCT2_GLOBAL(RCT2_ADDRESS_NEXT_EXPENDITURE_TYPE, uint8) = 12;
	int x = *eax;
	int y = *ecx;
	uint8 map_element_direction = *edx;
	uint8 base_height = *edx >> 8;
	uint8 color1 = *ebx >> 8;
	uint8 color2 = *ebp;
	uint8 color3 = *ebp >> 8;
	int z = base_height * 8;
	RCT2_GLOBAL(0x009DEA5E, uint16) = x + 16;
	RCT2_GLOBAL(0x009DEA60, uint16) = y + 16;
	RCT2_GLOBAL(0x009DEA62, uint16) = z;
	if(!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR)){
		if(!map_is_location_in_park(x, y)){
			*ebx = MONEY32_UNDEFINED;
			return;
		}
		rct_map_element* map_element = map_get_first_element_at(x / 32, y / 32);
		while(map_element_get_type(map_element) != MAP_ELEMENT_TYPE_FENCE ||
			map_element->base_height != base_height ||
			(map_element->type & MAP_ELEMENT_DIRECTION_MASK) != map_element_direction||
			((*ebx & 0x40) && !(map_element->flags & MAP_ELEMENT_FLAG_5))){
			map_element++;
			if((map_element - 1)->flags & MAP_ELEMENT_FLAG_LAST_TILE){
				*ebx = 0;
				return;
			}
		}
		if((*ebx & 0x40) && !(map_element->flags & MAP_ELEMENT_FLAG_5)){
			*ebx = 0;
			return;
		}
		if(*ebx & GAME_COMMAND_FLAG_APPLY){
			rct_scenery_entry* scenery_entry = RCT2_ADDRESS(RCT2_ADDRESS_WALL_SCENERY_ENTRIES, rct_scenery_entry*)[map_element->properties.fence.type];
			map_element->properties.fence.item[1] &= 0xE0;
			map_element->properties.fence.item[1] |= color1;
			map_element->flags &= 0x9F;
			map_element->properties.fence.item[1] &= 0x1F;
			map_element->properties.fence.item[1] |= (color2 & 0x7) * 32;
			map_element->flags |= (color2 & 0x18) * 4;
			if(scenery_entry->wall.flags & 0x80){
				map_element->properties.fence.item[0] = color3;
			}
			map_invalidate_tile(x, y, z, z + 0x48);
		}
	}
	*ebx = 0;
}

/**
 *
 *  rct2: 0x006B909A
 */
void game_command_set_large_scenery_colour(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	RCT2_GLOBAL(RCT2_ADDRESS_NEXT_EXPENDITURE_TYPE, uint8) = 12;
	int x = *eax;
	int y = *ecx;
	uint8 map_element_direction = *ebx >> 8;
	uint8 base_height = *edx;
	uint8 scenerymultiple_index = *edx >> 8;
	uint8 color1 = *ebp;
	uint8 color2 = *ebp >> 8;
	int z = map_element_height(x, y);
	RCT2_GLOBAL(0x009DEA5E, uint16) = x + 16;
	RCT2_GLOBAL(0x009DEA60, uint16) = y + 16;
	RCT2_GLOBAL(0x009DEA62, uint16) = z;


	rct_map_element* map_element = map_get_first_element_at(x / 32, y / 32);
	while(map_element_get_type(map_element) != MAP_ELEMENT_TYPE_SCENERY_MULTIPLE ||
		map_element->base_height != base_height ||
		map_element->properties.scenerymultiple.type >> 10 != scenerymultiple_index ||
		(map_element->type & MAP_ELEMENT_DIRECTION_MASK) != map_element_direction){
		map_element++;
		if((map_element - 1)->flags & MAP_ELEMENT_FLAG_LAST_TILE){
			*ebx = 0;
			return;
		}
	}
	if((*ebx & 0x40) && !(map_element->flags & MAP_ELEMENT_FLAG_5)){
		*ebx = 0;
		return;
	}
	int ecx2 = map_element->properties.scenerymultiple.type >> 10;
	rct_scenery_entry* scenery_entry = RCT2_ADDRESS(RCT2_ADDRESS_LARGE_SCENERY_ENTRIES, rct_scenery_entry*)[map_element->properties.scenerymultiple.type & 0x3FF];
	int x2 = scenery_entry->large_scenery.tiles[ecx2].x_offset;
	int y2 = scenery_entry->large_scenery.tiles[ecx2].y_offset;
	int z2 = (base_height * 8) - scenery_entry->large_scenery.tiles[ecx2].z_offset;
	switch(map_element->type & MAP_ELEMENT_DIRECTION_MASK){
		case MAP_ELEMENT_DIRECTION_WEST:
			break;
		case MAP_ELEMENT_DIRECTION_NORTH:{
			int temp = x2;
			x2 = y2;
			y2 = -temp;
			}break;
		case MAP_ELEMENT_DIRECTION_EAST:
			x2 = -x2;
			y2 = -y2;
			break;
		case MAP_ELEMENT_DIRECTION_SOUTH:{
			int temp = y2;
			y2 = x2;
			x2 = -temp;
			}break;
	}
	x2 = -x2 + x;
	y2 = -y2 + y;
	int i = 0;
	while(1){
		if(scenery_entry->large_scenery.tiles[i].x_offset == -1){
			*ebx = 0;
			return;
		}
		int x3 = scenery_entry->large_scenery.tiles[i].x_offset;
		int y3 = scenery_entry->large_scenery.tiles[i].y_offset;
		int z3 = scenery_entry->large_scenery.tiles[i].z_offset;
		switch(map_element->type & MAP_ELEMENT_DIRECTION_MASK){
			case MAP_ELEMENT_DIRECTION_WEST:
				break;
			case MAP_ELEMENT_DIRECTION_NORTH:{
				int temp = x3;
				x3 = y3;
				y3 = -temp;
				}break;
			case MAP_ELEMENT_DIRECTION_EAST:
				x3 = -x3;
				y3 = -y3;
				break;
			case MAP_ELEMENT_DIRECTION_SOUTH:{
				int temp = y3;
				y3 = x3;
				x3 = -temp;
				}break;
		}
		x3 += x2;
		y3 += y2;
		z3 += z2;
		if (!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR)){
			if (!map_is_location_owned(x3, y3, z3)){
				*ebx = MONEY32_UNDEFINED;
				return;
			}
		}

		if(*ebx & GAME_COMMAND_FLAG_APPLY){
			rct_map_element* map_element = map_get_first_element_at(x3 / 32, y3 / 32);
			while(map_element_get_type(map_element) != MAP_ELEMENT_TYPE_SCENERY_MULTIPLE ||
				(map_element->type & MAP_ELEMENT_DIRECTION_MASK) != map_element_direction ||
				map_element->properties.scenerymultiple.type >> 10 != i ||
				map_element->base_height != base_height){
				map_element++;
			}
			map_element->properties.scenerymultiple.colour[0] &= 0xE0;
			map_element->properties.scenerymultiple.colour[0] |= color1;
			map_element->properties.scenerymultiple.colour[1] &= 0xE0;
			map_element->properties.scenerymultiple.colour[1] |= color2;
			map_invalidate_tile_full(x3, y3);
		}
		
		i++;
	}
	*ebx = 0;
}

/**
 *
 *  rct2: 0x006BA16A
 */
void game_command_set_banner_colour(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	RCT2_GLOBAL(RCT2_ADDRESS_NEXT_EXPENDITURE_TYPE, uint8) = 12;
	int x = *eax;
	int y = *ecx;
	uint8 base_height = *edx;
	uint8 banner_position = *edx >> 8;
	uint8 color = *ebp;
	int z = (base_height * 8);
	RCT2_GLOBAL(0x009DEA5E, uint16) = x + 16;
	RCT2_GLOBAL(0x009DEA60, uint16) = y + 16;
	RCT2_GLOBAL(0x009DEA62, uint16) = z;

	if (!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR)){
		if (!map_is_location_owned(x, y, z - 16)){
			*ebx = MONEY32_UNDEFINED;
			return;
		}
	}

	if(*ebx & GAME_COMMAND_FLAG_APPLY){
		rct_map_element* map_element = map_get_first_element_at(x / 32, y / 32);
		while(map_element->type != MAP_ELEMENT_TYPE_BANNER ||
			map_element->properties.banner.position != banner_position){
			map_element++;
			if((map_element - 1)->flags & MAP_ELEMENT_FLAG_LAST_TILE){
				*ebx = MONEY32_UNDEFINED;
				return;
			}
		}
		rct_window* window = window_find_by_number(WC_BANNER, map_element->properties.banner.index);
		if(window){
			window_invalidate(window);
		}
		gBanners[map_element->properties.banner.index].colour = color;
		map_invalidate_tile(x, y, z, z + 32);
	}
	
	*ebx = 0;
}

money32 sub_6A67C0(int x, int y, int z, int flags)
{
	int eax, ebx, ecx, edx, esi, edi, ebp;
	eax = x * 32;
	ecx = y * 32;
	ebx = flags & 0xFF;
	edx = z & 0xFF;
	RCT2_CALLFUNC_X(0x006A67C0, &eax, &ebx, &ecx, &edx, &esi, &edi, &ebp);
	return ebx;
}

// This will cause clear scenery to remove paths
// This should be a flag for the game command which can be set via a checkbox on the clear scenery window.
// #define CLEAR_SCENERY_REMOVES_PATHS

/**
 *
 *  rct2: 0x0068DFE4
 */
money32 map_clear_scenery_from_tile(int x, int y, int flags)
{
	int type;
	money32 cost, totalCost;
	rct_map_element *mapElement;

	totalCost = 0;

restart_from_beginning:
	mapElement = map_get_first_element_at(x, y);
	do {
		type = map_element_get_type(mapElement);
		switch (type) {
		case MAP_ELEMENT_TYPE_PATH:
			if (gClearFootpath) {
				cost = sub_6A67C0(x, y, mapElement->base_height, flags);
				if (cost == MONEY32_UNDEFINED)
					return MONEY32_UNDEFINED;

				totalCost += cost;
				if (flags & 1)
					goto restart_from_beginning;
			} break;
		case MAP_ELEMENT_TYPE_SCENERY:
			if (gClearSmallScenery) {
				int eax = x * 32;
				int ebx = (mapElement->type << 8) | flags;
				int ecx = y * 32;
				int edx = (mapElement->properties.scenery.type << 8) | (mapElement->base_height);
				int esi, edi, ebp; 
				game_command_remove_scenery(&eax, &ebx, &ecx, &edx, &esi, &edi, &ebp);
				cost = ebx;

				if (cost == MONEY32_UNDEFINED)
					return MONEY32_UNDEFINED;

				totalCost += cost;
				if (flags & 1)
					goto restart_from_beginning;

			} break;
		case MAP_ELEMENT_TYPE_FENCE:
			if (gClearSmallScenery) {
				int eax = x * 32;
				int ebx = flags;
				int ecx = y * 32;
				int edx = (mapElement->base_height << 8) | (mapElement->type & MAP_ELEMENT_DIRECTION_MASK);
				int esi, edi, ebp; 
				game_command_remove_fence(&eax, &ebx, &ecx, &edx, &esi, &edi, &ebp);
				cost = ebx;

				if (cost == MONEY32_UNDEFINED)
					return MONEY32_UNDEFINED;

				totalCost += cost;
				if (flags & 1)
					goto restart_from_beginning;

			} break;
		case MAP_ELEMENT_TYPE_SCENERY_MULTIPLE:
			if (gClearLargeScenery) {
				int eax = x * 32;
				int ebx = flags | ((mapElement->type & MAP_ELEMENT_DIRECTION_MASK) << 8);
				int ecx = y * 32;
				int edx = mapElement->base_height | ((mapElement->properties.scenerymultiple.type >> 10) << 8);
				int esi, edi, ebp;
				game_command_remove_large_scenery(&eax, &ebx, &ecx, &edx, &esi, &edi, &ebp);
				cost = ebx;

				if (cost == MONEY32_UNDEFINED)
					return MONEY32_UNDEFINED;

				totalCost += cost;
				if (flags & 1)
					goto restart_from_beginning;

			} break;
			break;
		}
	} while (!map_element_is_last_for_tile(mapElement++));

	return totalCost;
}

money32 map_clear_scenery(int x0, int y0, int x1, int y1, int flags)
{
	int x, y, z;
	money32 totalCost, cost;

	RCT2_GLOBAL(RCT2_ADDRESS_NEXT_EXPENDITURE_TYPE, uint8) = RCT_EXPENDITURE_TYPE_LANDSCAPING * 4;

	x = (x0 + x1) / 2 + 16;
	y = (y0 + y1) / 2 + 16;
	z = map_element_height(x, y);
	RCT2_GLOBAL(0x009DEA5E, uint16) = x;
	RCT2_GLOBAL(0x009DEA60, uint16) = y;
	RCT2_GLOBAL(0x009DEA62, uint16) = z;

	x0 = clamp(0, x0, 255);
	y0 = clamp(0, y0, 255);
	x1 = clamp(0, x1, 255);
	y1 = clamp(0, y1, 255);

	totalCost = 0;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++) {
			cost = map_clear_scenery_from_tile(x, y, flags);
			if (cost == MONEY32_UNDEFINED)
				return MONEY32_UNDEFINED;

			totalCost += cost;
		}
	}

	return totalCost;
}

/**
 *
 *  rct2: 0x0068DF91
 */
void game_command_clear_scenery(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	*ebx = map_clear_scenery(
		(*eax & 0xFFFF) / 32,
		(*ecx & 0xFFFF) / 32,
		(*edi & 0xFFFF) / 32,
		(*ebp & 0xFFFF) / 32,
		*ebx & 0xFF
	);
}

/* rct2: 0x00663CCD */
money32 map_change_surface_style(int x0, int y0, int x1, int y1, uint8 surface_style, uint8 edge_style, uint8 flags)
{
	RCT2_GLOBAL(0x141F56C, uint8) = 12;

	int x_mid, y_mid;

	x_mid = (x0 + x1) / 2 + 16;
	y_mid = (y0 + y1) / 2 + 16;

	int height_mid = map_element_height(x_mid, y_mid);

	RCT2_GLOBAL(0x9DEA5E, uint16) = x_mid;
	RCT2_GLOBAL(0x9DEA60, uint16) = y_mid;
	RCT2_GLOBAL(0x9DEA62, uint16) = height_mid;
	RCT2_GLOBAL(0x9E32B4, uint32) = 0;

	money32 cur_cost = 0;

	if (RCT2_GLOBAL(RCT2_ADDRESS_GAME_PAUSED, uint8) != 0){
		cur_cost += RCT2_GLOBAL(0x9E32B4, uint32);

		if (RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_NO_MONEY){
			return 0;
		}
		return cur_cost;
	}

	if (!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR) && RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_FORBID_LANDSCAPE_CHANGES){
		cur_cost += RCT2_GLOBAL(0x9E32B4, uint32);

		if (RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_NO_MONEY){
			return 0;
		}
		return cur_cost;
	}

	for (int x = x0; x <= x1; x += 32){
		for (int y = y0; y <= y1; y += 32){
			if (x > 0x1FFF)continue;
			if (y > 0x1FFF)continue;

			if (!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR)){
				if (!map_is_location_in_park(x, y))continue;
			}
			
			rct_map_element* map_element = map_get_surface_element_at(x / 32, y / 32);

			if (surface_style != 0xFF){
				uint8 cur_terrain = (
					(map_element->type&MAP_ELEMENT_DIRECTION_MASK) << 3) 
					| (map_element->properties.surface.terrain >> 5);

				if (surface_style != cur_terrain){
					RCT2_GLOBAL(0x9E32B4, uint32) += RCT2_ADDRESS(0x97B8B8, uint32)[surface_style & 0x1F];

					if (flags & 1){
						map_element->properties.surface.terrain &= MAP_ELEMENT_WATER_HEIGHT_MASK;
						map_element->type &= MAP_ELEMENT_QUADRANT_MASK | MAP_ELEMENT_TYPE_MASK;

						//Save the new terrain
						map_element->properties.surface.terrain |= surface_style << 5;
						//Save the new direction mask
						map_element->type |= (surface_style >> 3) & MAP_ELEMENT_DIRECTION_MASK;

						map_invalidate_tile_full(x, y);
						RCT2_CALLPROC_X(0x673883, x, 0, y, map_element_height(x, y), 0, 0, 0);
					}
				}
			}

			if (edge_style != 0xFF){
				uint8 cur_edge = 
					((map_element->type & 0x80) >> 4) 
					| (map_element->properties.surface.slope >> 5);

				if (edge_style != cur_edge){
					cur_cost++;

					if (flags & 1){
						map_element->properties.surface.slope &= MAP_ELEMENT_SLOPE_MASK;
						map_element->type &= 0x7F;

						//Save edge style
						map_element->properties.surface.slope |= edge_style << 5;
						//Save ???
						map_element->type |= (edge_style << 4) & 0x80;
						map_invalidate_tile_full(x, y);
					}
				}
			}
			
			if (flags & 1){
				if (!(map_element->properties.surface.terrain & MAP_ELEMENT_SURFACE_TERRAIN_MASK)){
					if (!(map_element->type & MAP_ELEMENT_DIRECTION_MASK)){
						if ((map_element->properties.surface.grass_length & 7) != GRASS_LENGTH_CLEAR_0){
							map_element->properties.surface.grass_length = GRASS_LENGTH_CLEAR_0;
							map_invalidate_tile_full(x, y);
						}
					}
				}
			}
		}
	}

	cur_cost *= 100;

	cur_cost += RCT2_GLOBAL(0x9E32B4, uint32);

	if (RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_NO_MONEY){
		return 0;
	}
	return cur_cost;
}

/* rct2: 0x00663CCD */
void game_command_change_surface_style(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp){
	//RCT2_CALLFUNC_X(0x663CCD, eax, ebx, ecx, edx, esi, edi, ebp);
	//return;

	*ebx = map_change_surface_style(
		(*eax & 0xFFFF),
		(*ecx & 0xFFFF),
		(*edi & 0xFFFF),
		(*ebp & 0xFFFF),
		*edx & 0xFF,
		(*edx & 0xFF00) >> 8,
		*ebx & 0xFF
		);
}

//0x00981A1E
const uint8 map_element_raise_styles[5][32] = {
	{0x01, 0x1B, 0x03, 0x1B, 0x05, 0x21, 0x07, 0x21, 0x09, 0x1B, 0x0B, 0x1B, 0x0D, 0x21, 0x20, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x23, 0x18, 0x19, 0x1A, 0x3B, 0x1C, 0x29, 0x24, 0x1F},
	{0x02, 0x03, 0x17, 0x17, 0x06, 0x07, 0x17, 0x17, 0x0A, 0x0B, 0x22, 0x22, 0x0E, 0x20, 0x22, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x37, 0x18, 0x19, 0x1A, 0x23, 0x1C, 0x28, 0x26, 0x1F},
	{0x04, 0x05, 0x06, 0x07, 0x1E, 0x24, 0x1E, 0x24, 0x0C, 0x0D, 0x0E, 0x20, 0x1E, 0x24, 0x1E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x26, 0x18, 0x19, 0x1A, 0x21, 0x1C, 0x2C, 0x3E, 0x1F},
	{0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x20, 0x1D, 0x1D, 0x28, 0x28, 0x1D, 0x1D, 0x28, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x22, 0x18, 0x19, 0x1A, 0x29, 0x1C, 0x3D, 0x2C, 0x1F},
	{0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x22, 0x20, 0x20, 0x20, 0x21, 0x20, 0x28, 0x24, 0x20},
};

/**
 *
 *  rct2: 0x0068C542
 */
void game_command_raise_land(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	int x = *eax;
	int y = *ecx;
	int z = map_element_height(*eax, *ecx);
	int ax = (uint16)*edx;
	int ay = (uint16)*ebp;
	int bx = (uint16)(*edx >> 16);
	int by = (uint16)(*ebp >> 16);
	uint16 selection_type = *edi;

	int cost = 0;

	if(*ebx & GAME_COMMAND_FLAG_APPLY && RCT2_GLOBAL(0x009A8C28, uint8) == 1){
		sound_play_panned(SOUND_PLACE_ITEM, 0x8001, x, y, z);
	}

	uint8 min_height = 0xFF;

	// find lowest map element in selection
	for(int yi = ay; yi <= by; yi += 32){
		for(int xi = ax; xi <= bx; xi += 32){
			rct_map_element* map_element = map_get_surface_element_at(xi / 32, yi / 32);
			if(min_height > map_element->base_height){
				min_height = map_element->base_height;
			}
		}
	}

	for(int yi = ay; yi <= by; yi += 32){
		for(int xi = ax; xi <= bx; xi += 32){
			rct_map_element* map_element = map_get_surface_element_at(xi / 32, yi / 32);
			uint8 height = map_element->base_height;
			if(height <= min_height){
				uint8 new_style = map_element_raise_styles[selection_type][map_element->properties.surface.slope & MAP_ELEMENT_SLOPE_MASK];
				if(new_style & 0x20){ // needs to be raised
					height += 2;
					new_style &= ~0x20;
				}
				int ebx2 = *ebx;
				int edx2 = (new_style << 8) | height;
				int edi2 = selection_type << 5;
				RCT2_CALLFUNC_X(0x0066397F, &xi, &ebx2, &yi, &edx2, (int*)&map_element, &edi2, ebp); // actually apply the change
				if(ebx2 == MONEY32_UNDEFINED){
					*ebx = MONEY32_UNDEFINED;
					return;
				}else{
					cost += ebx2;
				}
			}
		}
	}
	RCT2_GLOBAL(0x141F56C, uint8) = 12;
	RCT2_GLOBAL(0x009DEA5E, uint32) = x;
	RCT2_GLOBAL(0x009DEA60, uint32) = y;
	RCT2_GLOBAL(0x009DEA62, uint32) = z;
	*ebx = cost;
}

//0x00981ABE
const uint8 map_element_lower_styles[5][32] = {
	{0x2E, 0x00, 0x2E, 0x02, 0x3E, 0x04, 0x3E, 0x06, 0x2E, 0x08, 0x2E, 0x0A, 0x3E, 0x0C, 0x3E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x06, 0x18, 0x19, 0x1A, 0x0B, 0x1C, 0x0C, 0x3E, 0x1F},
	{0x2D, 0x2D, 0x00, 0x01, 0x2D, 0x2D, 0x04, 0x05, 0x3D, 0x3D, 0x08, 0x09, 0x3D, 0x3D, 0x0C, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x07, 0x18, 0x19, 0x1A, 0x09, 0x1C, 0x3D, 0x0C, 0x1F},
	{0x2B, 0x3B, 0x2B, 0x3B, 0x00, 0x01, 0x02, 0x03, 0x2B, 0x3B, 0x2B, 0x3B, 0x08, 0x09, 0x0A, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x03, 0x18, 0x19, 0x1A, 0x3B, 0x1C, 0x09, 0x0E, 0x1F},
	{0x27, 0x27, 0x37, 0x37, 0x27, 0x27, 0x37, 0x37, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x37, 0x18, 0x19, 0x1A, 0x03, 0x1C, 0x0D, 0x06, 0x1F},
	{0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x0D, 0x0E, 0x00},
};

/**
 *
 *  rct2: 0x0068C6D1
 */
void game_command_lower_land(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	int x = *eax;
	int y = *ecx;
	int z = map_element_height(*eax, *ecx);
	int ax = (uint16)*edx;
	int ay = (uint16)*ebp;
	int bx = (uint16)(*edx >> 16);
	int by = (uint16)(*ebp >> 16);
	uint16 selection_type = *edi;

	int cost = 0;

	if(*ebx & GAME_COMMAND_FLAG_APPLY && RCT2_GLOBAL(0x009A8C28, uint8) == 1){
		sound_play_panned(SOUND_PLACE_ITEM, 0x8001, x, y, z);
	}

	uint8 max_height = 0;

	// find highest map element in selection
	for(int yi = ay; yi <= by; yi += 32){
		for(int xi = ax; xi <= bx; xi += 32){
			rct_map_element* map_element = map_get_surface_element_at(xi / 32, yi / 32);
			uint8 base_height = map_element->base_height;
			if(map_element->properties.surface.slope & 0xF){
				base_height += 2;
			}
			if(map_element->properties.surface.slope & 0x10){
				base_height += 2;
			}
			if(max_height < base_height){
				max_height = base_height;
			}
		}
	}

	for(int yi = ay; yi <= by; yi += 32){
		for(int xi = ax; xi <= bx; xi += 32){
			rct_map_element* map_element = map_get_surface_element_at(xi / 32, yi / 32);
			uint8 height = map_element->base_height;
			if(map_element->properties.surface.slope & 0xF){
				height += 2;
			}
			if(map_element->properties.surface.slope & 0x10){
				height += 2;
			}
			if(height >= max_height){
				height =  map_element->base_height;
				uint8 new_style = map_element_lower_styles[selection_type][map_element->properties.surface.slope & MAP_ELEMENT_SLOPE_MASK];
				if(new_style & 0x20){ // needs to be lowered
					height -= 2;
					new_style &= ~0x20;
				}
				int ebx2 = *ebx;
				int edx2 = (new_style << 8) | height;
				int edi2 = selection_type << 5;
				RCT2_CALLFUNC_X(0x0066397F, &xi, &ebx2, &yi, &edx2, (int*)&map_element, &edi2, ebp); // actually apply the change
				if(ebx2 == MONEY32_UNDEFINED){
					*ebx = MONEY32_UNDEFINED;
					return;
				}else{
					cost += ebx2;
				}
			}
		}
	}
	RCT2_GLOBAL(0x141F56C, uint8) = 12;
	RCT2_GLOBAL(0x009DEA5E, uint32) = x;
	RCT2_GLOBAL(0x009DEA60, uint32) = y;
	RCT2_GLOBAL(0x009DEA62, uint32) = z;
	*ebx = cost;
}

money32 raise_water(sint16 x0, sint16 y0, sint16 x1, sint16 y1, uint8 flags){
	money32 cost = 0;

	uint8 max_height = 0xFF;

	for (int yi = y0; yi <= y1; yi += 32){
		for (int xi = x0; xi <= x1; xi += 32){
			rct_map_element* map_element = map_get_surface_element_at(xi / 32, yi / 32);
			uint8 height = map_element->base_height;
			if (map_element->properties.surface.terrain & 0x1F){
				height = (map_element->properties.surface.terrain & 0x1F) * 2;
			}
			if (max_height > height){
				max_height = height;
			}
		}
	}

	for (int yi = y0; yi <= y1; yi += 32){
		for (int xi = x0; xi <= x1; xi += 32){
			rct_map_element* map_element = map_get_surface_element_at(xi / 32, yi / 32);

			if (map_element->base_height <= max_height){
				uint8 height = (map_element->properties.surface.terrain & 0x1F);
				if (height){
					height *= 2;
					if (height > max_height){
						continue;
					}
					height += 2;
				}
				else{
					height = map_element->base_height + 2;
				}

				money32 cost2 = game_do_command(xi, flags, yi, (max_height << 8) + height, GAME_COMMAND_16, 0, 0);
				if (cost2 == MONEY32_UNDEFINED){
					return MONEY32_UNDEFINED;
				}
				else{
					cost += cost2;
				}
			}
		}
	}
	if (flags & GAME_COMMAND_FLAG_APPLY){
		int x = ((x0 + x1) / 2) + 16;
		int y = ((y0 + y1) / 2) + 16;
		int z = map_element_height(x, y);
		sint16 water_height_z = z >> 16;
		sint16 base_height_z = z;
		z = water_height_z;
		if (!z){
			z = base_height_z;
		}
		RCT2_GLOBAL(0x009DEA5E, uint32) = x;
		RCT2_GLOBAL(0x009DEA60, uint32) = y;
		RCT2_GLOBAL(0x009DEA62, uint32) = z;
		sound_play_panned(SOUND_LAYING_OUT_WATER, 0x8001, x, y, z);
	}
	return cost;
}

/**
 *
 *  rct2: 0x006E66A0
 */
void game_command_raise_water(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	*ebx = raise_water(
		(sint16)*eax,
		(sint16)*ecx,
		(sint16)*edi,
		(sint16)*ebp,
		(uint8)*ebx);
}

money32 lower_water(sint16 x0, sint16 y0, sint16 x1, sint16 y1, uint8 flags){
	money32 cost = 0;

	uint8 min_height = 0;

	for (int yi = y0; yi <= y1; yi += 32){
		for (int xi = x0; xi <= x1; xi += 32){
			rct_map_element* map_element = map_get_surface_element_at(xi / 32, yi / 32);

			uint8 height = map_element->properties.surface.terrain & 0x1F;
			if (height){
				height *= 2;
				if (height > min_height){
					min_height = height;
				}
			}
		}
	}

	for (int yi = y0; yi <= y1; yi += 32){
		for (int xi = x0; xi <= x1; xi += 32){
			rct_map_element* map_element = map_get_surface_element_at(xi / 32, yi / 32);

			uint8 height = (map_element->properties.surface.terrain & 0x1F);
			if (height){
				height *= 2;
				if (height < min_height){
					continue;
				}
				height -= 2;
				int cost2 = game_do_command(xi, flags, yi, (min_height << 8) + height, GAME_COMMAND_16, 0, 0);
				if (cost2 == MONEY32_UNDEFINED){
					return MONEY32_UNDEFINED;
				}
				else{
					cost += cost2;
				}
			}
		}
	}
	if (flags & GAME_COMMAND_FLAG_APPLY){
		int x = ((x0 + x1) / 2) + 16;
		int y = ((y0 + y1) / 2) + 16;
		int z = map_element_height(x, y);
		sint16 water_height_z = z >> 16;
		sint16 base_height_z = z;
		z = water_height_z;
		if (!z){
			z = base_height_z;
		}
		RCT2_GLOBAL(0x009DEA5E, uint32) = x;
		RCT2_GLOBAL(0x009DEA60, uint32) = y;
		RCT2_GLOBAL(0x009DEA62, uint32) = z;
		sound_play_panned(SOUND_LAYING_OUT_WATER, 0x8001, x, y, z);
	}
	return cost;
}

/**
 *
 *  rct2: 0x006E6878
 */
void game_command_lower_water(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	*ebx = lower_water(
		(sint16)*eax, 
		(sint16)*ecx, 
		(sint16)*edi, 
		(sint16)*ebp, 
		(uint8)*ebx);
}

/**
 *
 *  rct2: 0x006E5597
 */
void game_command_remove_fence(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	int x = *eax;
	int y = *ecx;
	uint8 base_height = (*edx >> 8);
	uint8 direction = *edx;

	RCT2_GLOBAL(0x141F56C, uint8) = 12;
	if(!(*ebx & 0x40) && RCT2_GLOBAL(RCT2_ADDRESS_GAME_PAUSED, uint8) != 0){
		RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_CONSTRUCTION_NOT_POSSIBLE_WHILE_GAME_IS_PAUSED;
		*ebx = MONEY32_UNDEFINED;
		return;
	}
	if(!(*ebx & 0x40) && !(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR) && !map_is_location_owned(x, y, base_height * 8)){
		*ebx = MONEY32_UNDEFINED;
		return;
	}
	rct_map_element* map_element = map_get_first_element_at(x / 32, y / 32);
	while(map_element_get_type(map_element) != MAP_ELEMENT_TYPE_FENCE ||
	map_element->base_height != base_height ||
	(map_element->type & MAP_ELEMENT_DIRECTION_MASK) != direction ||
	((*ebx & 0x40) && !(map_element->flags & MAP_ELEMENT_FLAG_5))){
		map_element++;
		if((map_element - 1)->flags & MAP_ELEMENT_FLAG_LAST_TILE){
			*ebx = 0;
			return;
		}
	}

	if (!(*ebx & GAME_COMMAND_FLAG_APPLY)){
		*ebx = 0;
		return;
	}

	rct_scenery_entry* scenery_entry = RCT2_ADDRESS(RCT2_ADDRESS_WALL_SCENERY_ENTRIES, rct_scenery_entry*)[map_element->properties.fence.type];
	if(scenery_entry->wall.var_0D != 0xFF){
		rct_banner* banner = &gBanners[map_element->properties.fence.item[0]];
		if(banner->type != BANNER_NULL){
			window_close_by_number(WC_BANNER, map_element->properties.fence.item[0]);
			banner->type = BANNER_NULL;
			user_string_free(banner->string_idx);
		}
	}
	map_invalidate_tile(x, y, map_element->base_height * 8, (map_element->base_height * 8) + 72);
	map_element_remove(map_element);
	*ebx = 0;
}

/**
 *
 *  rct2: 0x006B9E6D
 */
void game_command_place_banner(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	int x = (uint16)*eax;
	int y = (uint16)*ecx;
	uint8 base_height = *edx;
	uint8 edge = *edx >> 8;
	uint8 colour = *edi;
	uint8 type = *ebx >> 8;
	RCT2_GLOBAL(0x009DEA5E, uint32) = x + 16;
	RCT2_GLOBAL(0x009DEA60, uint32) = y + 16;
	RCT2_GLOBAL(0x009DEA62, uint32) = base_height * 16;
	RCT2_GLOBAL(0x141F56C, uint8) = 12;
	if(RCT2_GLOBAL(RCT2_ADDRESS_GAME_PAUSED, uint8) == 0){
		if(sub_68B044() && x < 8192 && y < 8192){
			rct_map_element* map_element = map_get_first_element_at(x / 32, y / 32);
			int dl = base_height * 2;
			int ch = (base_height - 1) * 2;
			while(map_element_get_type(map_element) != MAP_ELEMENT_TYPE_PATH ||
			(map_element->base_height != dl && map_element->base_height != ch) ||
			!(map_element->properties.path.edges & (1 << edge))){
				map_element++;
				if((map_element - 1)->flags & MAP_ELEMENT_FLAG_LAST_TILE){
					RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_CAN_ONLY_BE_BUILT_ACROSS_PATHS;
					*ebx = MONEY32_UNDEFINED;
					return;
				}
			}
			if(!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR) && !map_is_location_owned(x, y, base_height * 16)){
				*ebx = MONEY32_UNDEFINED;
				return;
			}
			map_element = map_get_first_element_at(x / 32, y / 32);
			dl = (base_height + 1) * 2;
			while(map_element->type != MAP_ELEMENT_TYPE_BANNER ||
			map_element->base_height != dl ||
			(map_element->properties.banner.position & 0x3) != edge){
				map_element++;
				if((map_element - 1)->flags & MAP_ELEMENT_FLAG_LAST_TILE){
					int banner_index = create_new_banner(*ebx);
					if(banner_index == BANNER_NULL){
						*ebx = MONEY32_UNDEFINED;
						return;
					}
					*edi = banner_index;
					if(*ebx & GAME_COMMAND_FLAG_APPLY){
						rct_map_element* new_map_element = map_element_insert(x / 32, y / 32, (base_height + 1) * 2, 0);
						gBanners[banner_index].type = type;
						gBanners[banner_index].colour = colour;
						gBanners[banner_index].x = x / 32;
						gBanners[banner_index].y = y / 32;
						new_map_element->type = MAP_ELEMENT_TYPE_BANNER;
						new_map_element->clearance_height = new_map_element->base_height + 2;
						new_map_element->properties.banner.position = edge;
						new_map_element->properties.banner.flags = 0xFF;
						new_map_element->properties.banner.unused = 0;
						new_map_element->properties.banner.index = banner_index;
						if(*ebx & 0x40){
							new_map_element->flags |= 0x10;
						}
						map_invalidate_tile_full(x, y);
						map_animation_create(0x0A, x, y, new_map_element->base_height);
					}
					rct_scenery_entry *scenery_entry = (rct_scenery_entry*)object_entry_groups[OBJECT_TYPE_BANNERS].chunks[type];
					*ebx = scenery_entry->banner.price;
					if(RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_NO_MONEY){
						*ebx = 0;
					}
					return;
				}
			}
			RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_BANNER_SIGN_IN_THE_WAY;
			*ebx = MONEY32_UNDEFINED;
			return;
		}
	}else{
		RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_CONSTRUCTION_NOT_POSSIBLE_WHILE_GAME_IS_PAUSED;
	}
	*ebx = MONEY32_UNDEFINED;
}

/**
 *
 *  rct2: 0x006E08F4
 */
void game_command_place_scenery(int* eax, int* ebx, int* ecx, int* edx, int* esi, int* edi, int* ebp)
{
	RCT2_GLOBAL(0x141F56C, uint8) = 12;
	int x = (uint16)*eax;
	int y = (uint16)*ecx;
	uint8 color2 = *edi >> 16;
	uint8 rotation = *edi;
	int z = *ebp;
	uint8 scenery_type = *ebx >> 8;
	uint8 quadrant = *edx;
	uint8 color1 = *edx >> 8;
	int F64F1D = 0;
	int F64EC8 = z;
	int base_height = map_element_height(x, y);
	if(base_height & 0xFFFF0000){
		base_height >>= 16;
	}
	RCT2_GLOBAL(0x009DEA5E, uint16) = x;
	RCT2_GLOBAL(0x009DEA60, uint16) = y;
	RCT2_GLOBAL(0x009DEA62, uint16) = base_height;
	if(F64EC8){
		base_height = F64EC8;
		RCT2_GLOBAL(0x009DEA62, uint16) = base_height;
	}
	RCT2_GLOBAL(0x009DEA5E, uint16) += 16;
	RCT2_GLOBAL(0x009DEA60, uint16) += 16;
	if(RCT2_GLOBAL(RCT2_ADDRESS_GAME_PAUSED, uint8) == 0){
		if(sub_68B044()){
			if(RCT2_GLOBAL(0x009D8150, uint8) & 1 || (x <= RCT2_GLOBAL(0x01358836, uint16) && y <= RCT2_GLOBAL(0x01358836, uint16))){
				rct_scenery_entry* scenery_entry = (rct_scenery_entry*)object_entry_groups[OBJECT_TYPE_SMALL_SCENERY].chunks[scenery_type];
				if((scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG_FULL_TILE && scenery_entry->small_scenery.flags & (SMALL_SCENERY_FLAG9 | SMALL_SCENERY_FLAG24 | SMALL_SCENERY_FLAG25)) || scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG9){
					quadrant = 0;
				}
				int x2 = x;
				int y2 = y;
				if(scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG_FULL_TILE){
					x2 += 16;
					y2 += 16;
				}else{
					x2 += RCT2_ADDRESS(0x009A3E74, uint8)[(quadrant & 3) * 2] - 1;
					y2 += RCT2_ADDRESS(0x009A3E75, uint8)[(quadrant & 3) * 2] - 1;
				}
				int base_height2 = map_element_height(x2, y2);
				if(base_height2 & 0xFFFF0000){
					base_height2 >>= 16;
					if(F64EC8 == 0){
						F64F1D = 1;
					}
				}
				if(F64EC8 == 0){
					F64EC8 = base_height2;
				}
				if(!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR) && !map_is_location_owned(x, y, F64EC8)){
					*ebx = MONEY32_UNDEFINED;
					return;
				}
				if(*ebx & GAME_COMMAND_FLAG_APPLY && !(*ebx & 0x40)){
					sub_673883(x, y, F64EC8);
					if(scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG19){
						RCT2_CALLPROC_X(0x006E588E, x, scenery_entry->small_scenery.height, y, F64EC8, 0, 0, 0);
					}
				}
				rct_map_element* map_element = map_get_first_element_at(x / 32, y / 32);
				while(map_element_get_type(map_element) != MAP_ELEMENT_TYPE_SURFACE){
					map_element++;
				}
				if(map_element->properties.surface.terrain & 0x1F){
					int water_height = ((map_element->properties.surface.terrain & 0x1F) * 16) - 1;
					if(water_height > F64EC8){
						RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_CANT_BUILD_THIS_UNDERWATER;
						*ebx = MONEY32_UNDEFINED;
						return;
					}
				}
				if(!(scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG18)){
					if(F64F1D != 0){
						RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_CAN_ONLY_BUILD_THIS_ON_LAND;
						*ebx = MONEY32_UNDEFINED;
						return;
					}
					if(map_element->properties.surface.terrain & 0x1F){
						if(((map_element->properties.surface.terrain & 0x1F) * 16) > F64EC8){
							RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_CAN_ONLY_BUILD_THIS_ON_LAND;
							*ebx = MONEY32_UNDEFINED;
							return;
						}
					}
				}
				if(!(scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG_REQUIRE_FLAT_SURFACE) || z != 0 || F64F1D != 0 || !(map_element->properties.surface.slope & 0x1F)){
					if(scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG18 || z == 0){
					l_6E0B78: ;
						int bp = quadrant;
						int zLow = F64EC8 / 8;
						int zHigh = zLow + ((scenery_entry->small_scenery.height + 7) / 8);
						int bl = 0xF;
						if(!(scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG_FULL_TILE)){
							bp ^= 2;
							bl = 1;
							bl <<= bp;
						}
						if(!(scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG24)){
							if(scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG9 && scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG_FULL_TILE){
								if(scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG25){
									bp ^= 2;
									bp += rotation;
									bp &= 3;
									bl = 0xBB;
									bl = rol8(bl, bp);
									bl &= 0xF;
								}else{
									bp += rotation;
									bp &= 1;
									bl = 0xA;
									bl >>= bp;
								}
							}
						}else{
							bp ^= 2;
							bp += rotation;
							bp &= 3;
							bl = 0x33;
							bl = rol8(bl, bp);
							bl &= 0xF;
						}
						if(z == 0){
							bl |= 0xF0;
						}
						RCT2_GLOBAL(0x00F64F22, uint16) = x;
						RCT2_GLOBAL(0x00F64F24, uint16) = y;
						RCT2_GLOBAL(0x00F64F1E, uint32) = (uint32)(ebx - 1); //0x006E0D6E uses [F64F1E+4] to read ebx value
						if(map_can_construct_with_clear_at(x, y, zLow, zHigh, (void*)0x006E0D6E, bl)){
							RCT2_GLOBAL(0x00F64F14, uint8) = RCT2_GLOBAL(0x00F1AD60, uint8) & 0x3;
							if(*ebx & GAME_COMMAND_FLAG_APPLY){
								int flags = (bl & 0xf);
								rct_map_element* new_map_element = map_element_insert(x / 32, y / 32, zLow, flags);
								RCT2_GLOBAL(0x00F64EBC, rct_map_element*) = new_map_element;
								uint8 type = quadrant << 6;
								type |= MAP_ELEMENT_TYPE_SCENERY;
								type |= rotation;
								new_map_element->type = type;
								new_map_element->properties.scenery.type = scenery_type;
								new_map_element->properties.scenery.age = 0;
								new_map_element->properties.scenery.colour_1 = color1;
								new_map_element->properties.scenery.colour_2 = color2;
								new_map_element->clearance_height = new_map_element->base_height + ((scenery_entry->small_scenery.height + 7) / 8);
								if(z != 0){
									new_map_element->properties.scenery.colour_1 |= 0x20;
								}
								if(*ebx & 0x40){
									new_map_element->flags |= 0x10;
								}
								map_invalidate_tile_full(x, y);
								if(scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG_ANIMATED){
									map_animation_create(2, x, y, new_map_element->base_height);
								}
							}
							*ebx = (scenery_entry->small_scenery.price * 10);
							if(RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_NO_MONEY){
								*ebx = 0;
							}
							return;
						}
					}else{
						if(F64F1D == 0){
							if((map_element->properties.surface.terrain & 0x1F) || (map_element->base_height * 8) != F64EC8){
								RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_LEVEL_LAND_REQUIRED;
							}else{
								goto l_6E0B78;
							}
						}else{
							RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_CAN_ONLY_BUILD_THIS_ON_LAND;
						}
					}
				}else{
					RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_LEVEL_LAND_REQUIRED;
				}
			}
		}
	}else{
		RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TEXT, uint16) = STR_CONSTRUCTION_NOT_POSSIBLE_WHILE_GAME_IS_PAUSED;
	}
	*ebx = MONEY32_UNDEFINED;
}

/**
 *
 *  rct2: 0x006EC6D7
 */
void map_invalidate_tile_full(int x, int y)
{
	RCT2_CALLPROC_X(0x006EC6D7, x, 0, y, 0, 0, 0, 0);
}

int map_get_station(rct_map_element *mapElement)
{
	return (mapElement->properties.track.sequence & 0x70) >> 4;
}

/**
 *
 *  rct2: 0x0068B280
 */
void map_element_remove(rct_map_element *mapElement)
{
	RCT2_CALLPROC_X(0x0068B280, 0, 0, 0, 0, (int)mapElement, 0, 0);
}

/**
 *
 *  rct2: 0x006A6AA7
 * @param x x-coordinate in units (not tiles)
 * @param y y-coordinate in units (not tiles)
 */
void sub_6A6AA7(int x, int y, rct_map_element *mapElement)
{
	RCT2_CALLPROC_X(0x006A6AA7, x, 0, y, 0, (int)mapElement, 0, 0);
}

/**
 *
 *  rct2: 0x00675A8E
 */
void map_remove_all_rides()
{
	map_element_iterator it;

	map_element_iterator_begin(&it);
	do {
		switch (map_element_get_type(it.element)) {
		case MAP_ELEMENT_TYPE_PATH:
			if (it.element->type & 1) {
				it.element->properties.path.type &= ~8;
				it.element->properties.path.addition_status = 255;
			}
			break;
		case MAP_ELEMENT_TYPE_ENTRANCE:
			if (it.element->properties.entrance.type == ENTRANCE_TYPE_PARK_ENTRANCE)
				break;

			// fall-through
		case MAP_ELEMENT_TYPE_TRACK:
			RCT2_CALLPROC_EBPSAFE(0x006A7594);
			sub_6A6AA7(it.x * 32, it.y * 32, it.element);
			map_element_remove(it.element);
			map_element_iterator_restart_for_tile(&it);
			break;
		}
	} while (map_element_iterator_next(&it));
}

/**
 *
 *  rct2: 0x0068AB1B
 */
void map_invalidate_map_selection_tiles()
{
	rct_xy16 *position;

	if (!(RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_FLAGS, uint16) & (1 << 1)))
		return;

	for (position = gMapSelectionTiles; position->x != -1; position++)
		map_invalidate_tile_full(position->x, position->y);
}

/**
 *
 *  rct2: 0x0068AAE1
 */
void map_invalidate_selection_rect()
{
	int x, y, x0, y0, x1, y1;

	if (!(RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_FLAGS, uint16) & (1 << 0)))
		return;

	x0 = RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_A_X, uint16);
	y0 = RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_A_Y, uint16);
	x1 = RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_B_X, uint16);
	y1 = RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_B_Y, uint16);

	for (x = x0; x <= x1; x++)
		for (y = y0; y <= y1; y++)
			map_invalidate_tile_full(x, y);
}

/**
 *
 *  rct2: 0x0068B111
 */
void map_reorganise_elements()
{
	RCT2_CALLPROC_EBPSAFE(0x0068B111);
}

/**
 *
 *  rct2: 0x0068B044
 */
int sub_68B044()
{
	return (RCT2_CALLPROC_X(0x0068B044, 0, 0, 0, 0, 0, 0, 0) & 0x100) == 0;
}

/**
 *
 *  rct2: 0x0068B1F6
 */
rct_map_element *map_element_insert(int x, int y, int z, int flags)
{
	rct_map_element *originalMapElement, *newMapElement, *insertedElement;

	sub_68B044();

	newMapElement = RCT2_GLOBAL(0x00140E9A4, rct_map_element*);
	originalMapElement = TILE_MAP_ELEMENT_POINTER(y * 256 + x);

	// Set tile index pointer to point to new element block
	TILE_MAP_ELEMENT_POINTER(y * 256 + x) = newMapElement;

	// Copy all elements that are below the insert height
	while (z >= originalMapElement->base_height) {
		// Copy over map element
		*newMapElement = *originalMapElement;
		originalMapElement->base_height = 255;
		originalMapElement++;
		newMapElement++;

		if ((newMapElement - 1)->flags & MAP_ELEMENT_FLAG_LAST_TILE) {
			// No more elements above the insert element
			(newMapElement - 1)->flags &= ~MAP_ELEMENT_FLAG_LAST_TILE;
			flags |= MAP_ELEMENT_FLAG_LAST_TILE;
			break;
		}
	}

	// Insert new map element
	insertedElement = newMapElement;
	newMapElement->base_height = z;
	newMapElement->flags = flags;
	newMapElement->clearance_height = z;
	*((uint32*)&newMapElement->properties) = 0;
	newMapElement++;

	// Insert rest of map elements above insert height
	if (!(flags & MAP_ELEMENT_FLAG_LAST_TILE)) {
		do {
			// Copy over map element
			*newMapElement = *originalMapElement;
			originalMapElement->base_height = 255;
			originalMapElement++;
			newMapElement++;
		} while (!((newMapElement - 1)->flags & MAP_ELEMENT_FLAG_LAST_TILE));
	}

	RCT2_GLOBAL(0x00140E9A4, rct_map_element*) = newMapElement;
	return insertedElement;
}

/**
 *
 *  rct2: 0x0068B932
 */
int map_can_construct_with_clear_at(int x, int y, int zLow, int zHigh, void *clearFunc, uint8 bl)
{
	return (RCT2_CALLPROC_X(0x0068B932, x, bl, y, (zHigh << 8) | zLow, 0, 0, (int)clearFunc) & 0x100) == 0;
}

/**
 *
 *  rct2: 0x0068B93A
 */
int map_can_construct_at(int x, int y, int zLow, int zHigh, uint8 bl)
{
	return map_can_construct_with_clear_at(x, y, zLow, zHigh, (void*)0xFFFFFFFF, bl);
}

/**
 *
 *  rct2: 0x006BA278
 */
int sub_6BA278(int ebx)
{
	int eax, ecx, edx, esi, edi, ebp;
	RCT2_CALLFUNC_X(0x006BA278, &eax, &ebx, &ecx, &edx, &esi, &edi, &ebp);
	return eax;
}

/**
 * 
 *  rct2: 0x006E5935
 */
void map_remove_intersecting_walls(int x, int y, int z0, int z1, int direction)
{
	int bannerIndex;
	rct_banner *banner;
	rct_map_element *mapElement;
	rct_scenery_entry *sceneryEntry;

	mapElement = map_get_first_element_at(x >> 5, y >> 5);
	do {
		if (map_element_get_type(mapElement) != MAP_ELEMENT_TYPE_FENCE)
			continue;

		if (mapElement->clearance_height <= z0 || mapElement->base_height >= z1)
			continue;

		if (direction != (mapElement->type & 3))
			continue;

		sceneryEntry = g_wallSceneryEntries[mapElement->properties.fence.type];
		if (sceneryEntry->wall.var_0D != 255) {
			bannerIndex = mapElement->properties.fence.item[0];
			banner = &gBanners[bannerIndex];
			if (banner->type != BANNER_NULL) {
				window_close_by_number(WC_BANNER, bannerIndex);
				banner->type = BANNER_NULL;
				user_string_free(banner->string_idx);
			}
		}
		
		map_invalidate_tile(x, y, mapElement->base_height * 8, mapElement->base_height * 8 + 72);
		map_element_remove(mapElement);
		mapElement -= 1;
	} while (!map_element_is_last_for_tile(mapElement++));
}

/**
 * Updates grass length, scenery age and jumping fountains.
 *
 *  rct2: 0x006646E1
 */
void map_update_tiles()
{
	int ignoreScreenFlags = SCREEN_FLAGS_SCENARIO_EDITOR | SCREEN_FLAGS_TRACK_DESIGNER | SCREEN_FLAGS_TRACK_MANAGER;
	if (RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & ignoreScreenFlags)
		return;

	// Update 43 more tiles
	for (int j = 0; j < 43; j++) {
		int x = 0;
		int y = 0;

		uint16 interleaved_xy = RCT2_GLOBAL(RCT2_ADDRESS_GRASS_SCENERY_TILEPOS, sint16);
		for (int i = 0; i < 8; i++) {
			x = (x << 1) | (interleaved_xy & 1);
			interleaved_xy >>= 1;
			y = (y << 1) | (interleaved_xy & 1);
			interleaved_xy >>= 1;
		}

		rct_map_element *mapElement = map_get_surface_element_at(x, y);
		if (mapElement != NULL) {
			map_update_grass_length(x * 32, y * 32, mapElement);
			scenery_update_tile(x * 32, y * 32);
		}

		RCT2_GLOBAL(RCT2_ADDRESS_GRASS_SCENERY_TILEPOS, sint16)++;
		RCT2_GLOBAL(RCT2_ADDRESS_GRASS_SCENERY_TILEPOS, sint16) &= 0xFFFF;
	}
}

/**
 *
 *  rct2: 0x006647A1
 */
static void map_update_grass_length(int x, int y, rct_map_element *mapElement)
{
	// Check if tile is grass
	if ((mapElement->properties.surface.terrain & 0xE0) && !(mapElement->type & 3))
		return;

	int grassLength = mapElement->properties.surface.grass_length & 7;

	// Check if grass is underwater or outside park
	int waterHeight = (mapElement->properties.surface.terrain & 0x1F) * 2;
	if (waterHeight > mapElement->base_height || !map_is_location_in_park(x, y)) {
		if (grassLength != GRASS_LENGTH_CLEAR_0)
			map_set_grass_length(x, y, mapElement, GRASS_LENGTH_CLEAR_0);

		return;
	}

	// Grass can't grow any further
	if (grassLength == GRASS_LENGTH_CLUMPS_2)
		return;

	int z0 = mapElement->base_height;
	int z1 = mapElement->base_height + 2;
	if (mapElement->properties.surface.slope & 0x10)
		z1 += 2;

	// Check objects above grass
	rct_map_element *mapElementAbove = mapElement;
	for (;;) {
		if (mapElementAbove->flags & MAP_ELEMENT_FLAG_LAST_TILE) {
			// Grow grass
			if (mapElement->properties.surface.grass_length < 0xF0) {
				mapElement->properties.surface.grass_length += 0x10;
			} else {
				mapElement->properties.surface.grass_length += 0x10;
				mapElement->properties.surface.grass_length ^= 8;
				if (mapElement->properties.surface.grass_length & 8) {
					// Random growth rate
					mapElement->properties.surface.grass_length |= scenario_rand() & 0x70;
				} else {
					// Increase length
					map_set_grass_length(x, y, mapElement, grassLength + 1);
				}
			}
		} else {
			mapElementAbove++;
			if (map_element_get_type(mapElementAbove) == MAP_ELEMENT_TYPE_FENCE)
				continue;
			if (z0 >= mapElementAbove->clearance_height)
				continue;
			if (z1 < mapElementAbove->base_height)
				continue;
		}
		break;
	}
}

static void map_set_grass_length(int x, int y, rct_map_element *mapElement, int length)
{
	int z0, z1;

	mapElement->properties.surface.grass_length = length;
	z0 = mapElement->base_height * 8;
	z1 = z0 + 16;
	gfx_invalidate_viewport_tile(x, y, z0, z1);
}