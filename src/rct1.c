/*****************************************************************************
 * Copyright (c) 2014 Ted John
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

#include "addresses.h"
#include "interface/viewport.h"
#include "interface/window.h"
#include "localisation/localisation.h"
#include "management/finance.h"
#include "object.h"
#include "rct1.h"
#include "scenario.h"
#include "util/sawyercoding.h"
#include "util/util.h"
#include "world/climate.h"
#include "world/map.h"
#include "world/scenery.h"

typedef struct {
	const rct_object_entry* entries;
	int count;
} RCT1DefaultObjectsGroup;

const uint8 RCT1TerrainConvertTable[16];
const uint8 RCT1TerrainEdgeConvertTable[16];
static const RCT1DefaultObjectsGroup RCT1DefaultObjects[10];

static void rct1_remove_rides();
static void rct1_load_default_objects();
static void rct1_fix_terrain();
static void rct1_fix_scenery();
static void rct1_fix_entrance_positions();
static void rct1_reset_research();

static void sub_69F06A();
static void sub_666DFD();

static void read(void *dst, void *src, int length)
{
	memcpy(dst, src, length);
}

bool rct1_read_sc4(const char *path, rct1_s4 *s4)
{
	char *buffer, *decodedBuffer;
	long length, decodedLength;
	bool success;

	if (!readentirefile(path, (void**)&buffer, &length)) {
		RCT2_GLOBAL(RCT2_ADDRESS_ERROR_TYPE, uint8) = 255;
		RCT2_GLOBAL(RCT2_ADDRESS_ERROR_STRING_ID, uint16) = 3011;
		return 0;
	}

	int fileType = sawyercoding_detect_file_type(buffer, length);

	decodedBuffer = malloc(sizeof(rct1_s4));
	decodedLength = (fileType & FILE_VERSION_MASK) == FILE_VERSION_RCT1 ?
		sawyercoding_decode_sv4(buffer, decodedBuffer, length) :
		sawyercoding_decode_sc4(buffer, decodedBuffer, length);
	if (decodedLength == sizeof(rct1_s4)) {
		memcpy(s4, decodedBuffer, sizeof(rct1_s4));
		success = true;
	} else {
		success = false;
	}

	free(buffer);
	free(decodedBuffer);
	return success;
}

bool rct1_read_sv4(const char *path, rct1_s4 *s4)
{
	char *buffer, *decodedBuffer;
	long length, decodedLength;
	bool success;

	if (!readentirefile(path, (void**)&buffer, &length)) {
		RCT2_GLOBAL(RCT2_ADDRESS_ERROR_TYPE, uint8) = 255;
		RCT2_GLOBAL(RCT2_ADDRESS_ERROR_STRING_ID, uint16) = 3011;
		return 0;
	}

	decodedBuffer = malloc(sizeof(rct1_s4));
	decodedLength = sawyercoding_decode_sv4(buffer, decodedBuffer, length);
	if (decodedLength == sizeof(rct1_s4)) {
		memcpy(s4, decodedBuffer, sizeof(rct1_s4));
		success = true;
	} else {
		success = false;
	}

	free(buffer);
	free(decodedBuffer);
	return success;
}

/**
 *
 *  rct2: 0x0069EEA0
 */
void rct1_import_s4(rct1_s4 *s4)
{
	int i;
	rct_banner *banner;

	read((void*)RCT2_ADDRESS_CURRENT_MONTH_YEAR, &s4->month, 16);
	memset((void*)RCT2_ADDRESS_MAP_ELEMENTS, 0, 0x30000 * sizeof(rct_map_element));
	read((void*)RCT2_ADDRESS_MAP_ELEMENTS, s4->map_elements, sizeof(s4->map_elements));
	read((void*)0x010E63B8, &s4->unk_counter, 4 + sizeof(s4->sprites));

	for (i = 0; i < MAX_BANNERS; i++)
		gBanners[i].type = 255;

	read((void*)0x013573BC, &s4->next_sprite_index, 12424);

	for (i = 0; i < MAX_BANNERS; i++) {
		banner = &gBanners[i];
		if (banner->type != 255 && banner->string_idx != 3458)
			banner->string_idx = 778;
	}

	read((void*)0x0135A8F4, &s4->string_table, 0x2F51C);
	memset((void*)0x013CA672, 0, 204);
	read((void*)0x0138B580, &s4->map_animations, 0x258F2);
	read((void*)0x013C6A72, &s4->patrol_areas, sizeof(s4->patrol_areas));

	char *esi = (char*)0x13C6A72;
	char *edi = (char*)0x13B0E72;
	int ebx, edx = 116;
	do {
		ebx = 32;
		do {
			memcpy(edi, esi, 4); esi += 4; edi += 4;
			memset(edi, 0, 4); edi += 4;
		} while (--ebx > 0);
		memset(edi, 0, 64); edi += 64;
	} while (--edx > 0);
	edi += 0xA800;
	
	edx = 4;
	do {
		ebx = 32;
		do {
			memcpy(edi, esi, 4); esi += 4; edi += 4;
			memset(edi, 0, 4); edi += 4;
		} while (--ebx);
		memset(edi, 0, 64); edi += 64;
	} while (--edx);

	read((void*)0x013CA672, &s4->unk_1F42AA, 116);
	read((void*)0x013CA73A, &s4->unk_1F431E, 4);
	read((void*)0x013CA73E, &s4->unk_1F4322, 0x41EA);
}

/**
 *
 *  rct2: 0x006A2B62
 */
void rct1_fix_landscape()
{
	int i;
	rct_sprite *sprite;
	rct_ride *ride;

	RCT2_CALLPROC_EBPSAFE(0x0069F007);

	// Free sprite user strings
	for (i = 0; i < MAX_SPRITES; i++) {
		sprite = &g_sprite_list[i];
		if (sprite->unknown.sprite_identifier != 255)
			user_string_free(sprite->unknown.name_string_idx);
	}

	reset_sprite_list();

	// Free ride user strings
	FOR_ALL_RIDES(i, ride)
		user_string_free(ride->name);

	ride_init_all();
	RCT2_GLOBAL(RCT2_ADDRESS_GUESTS_IN_PARK, uint16) = 0;
	RCT2_GLOBAL(RCT2_ADDRESS_GUESTS_HEADING_FOR_PARK, uint16) = 0;
	RCT2_GLOBAL(0x01357BC8, uint16) = 0;
	RCT2_GLOBAL(0x013573FE, uint8) = 0;
	RCT2_CALLPROC_EBPSAFE(0x0069F44B);
	sub_69F06A();
	RCT2_CALLPROC_EBPSAFE(0x0069F143);
	RCT2_CALLPROC_EBPSAFE(0x0069F2D0);
	RCT2_CALLPROC_EBPSAFE(0x0069F3AB);
	rct1_remove_rides();
	object_unload_all();
	rct1_load_default_objects();
	reset_loaded_objects();
	RCT2_CALLPROC_EBPSAFE(0x006A2730);
	rct1_fix_scenery();
	rct1_fix_terrain();
	rct1_fix_entrance_positions();
	rct1_reset_research();
	research_populate_list_random();
	research_remove_non_separate_vehicle_types();

	climate_reset(RCT2_GLOBAL(RCT2_ADDRESS_CLIMATE, uint8));
	RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) = SCREEN_FLAGS_SCENARIO_EDITOR;
	viewport_init_all();
	news_item_init_queue();
	window_editor_main_open();

	rct_s6_header *s6Header = (rct_s6_header*)0x009E34E4;
	rct_s6_info *s6Info = (rct_s6_info*)0x0141F570;

	s6Info->var_000 = 1;
	s6Info->category = 4;
	format_string(s6Info->details, STR_NO_DETAILS_YET, NULL);
	s6Info->name[0] = 0;
	if (RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_NO_MONEY) {
		RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) |= PARK_FLAGS_NO_MONEY_SCENARIO;
	} else {
		RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) &= ~PARK_FLAGS_NO_MONEY_SCENARIO;
	}
	if (RCT2_GLOBAL(RCT2_ADDRESS_PARK_ENTRANCE_FEE, money16) == MONEY_FREE) {
		RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) |= PARK_FLAGS_PARK_FREE_ENTRY;
	} else {
		RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) &= ~PARK_FLAGS_PARK_FREE_ENTRY;
	}
	RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) &= ~PARK_FLAGS_18;
	RCT2_GLOBAL(RCT2_ADDRESS_GUEST_INITIAL_CASH, money16) = clamp(
		MONEY(10,00),
		RCT2_GLOBAL(RCT2_ADDRESS_GUEST_INITIAL_CASH, money16),
		MONEY(100,00)
	);
	RCT2_GLOBAL(RCT2_ADDRESS_INITIAL_CASH, money32) = min(
		MONEY(10000,00),
		RCT2_GLOBAL(RCT2_ADDRESS_INITIAL_CASH, money32)
	);
	RCT2_CALLPROC_EBPSAFE(0x0069E89B);
	sub_69E869();

	RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_LOAN, money32) = clamp(
		MONEY(0,00),
		RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_LOAN, money32),
		MONEY(5000000,00)
	);

	RCT2_GLOBAL(RCT2_ADDRESS_MAXIMUM_LOAN, money32) = clamp(
		MONEY(0,00),
		RCT2_GLOBAL(RCT2_ADDRESS_MAXIMUM_LOAN, money32),
		MONEY(5000000,00)
	);

	RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_INTEREST_RATE, uint8) = clamp(
		5,
		RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_INTEREST_RATE, uint8),
		80
	);

	if (
		RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_TYPE, uint8) == OBJECTIVE_NONE ||
		RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_TYPE, uint8) == OBJECTIVE_HAVE_FUN ||
		RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_TYPE, uint8) == OBJECTIVE_BUILD_THE_BEST
	) {
		RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_TYPE, uint8) = OBJECTIVE_GUESTS_BY;
		RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_YEAR, uint8) = 4;
		RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_NUM_GUESTS, uint16) = 1000;
	}

	RCT2_GLOBAL(0x01358774, uint16) = 0;
}

static void rct1_remove_rides()
{
	map_element_iterator it;

	map_element_iterator_begin(&it);
	do {
		switch (map_element_get_type(it.element)) {
		case MAP_ELEMENT_TYPE_PATH:
			if (it.element->type & 1) {
				it.element->properties.path.type &= 0xF7;
				it.element->properties.path.addition_status = 255;
			}
			break;

		case MAP_ELEMENT_TYPE_TRACK:
			RCT2_CALLPROC_EBPSAFE(0x006A7594);
			sub_6A6AA7(it.x * 32, it.y * 32, it.element);
			map_element_remove(it.element);
			map_element_iterator_restart_for_tile(&it);
			break;

		case MAP_ELEMENT_TYPE_ENTRANCE:
			if (it.element->properties.entrance.type != ENTRANCE_TYPE_PARK_ENTRANCE) {
				RCT2_CALLPROC_EBPSAFE(0x006A7594);
				sub_6A6AA7(it.x * 32, it.y * 32, it.element);
				map_element_remove(it.element);
				map_element_iterator_restart_for_tile(&it);
			}
			break;
		}
	} while (map_element_iterator_next(&it));
}

/**
 *
 *  rct2: 0x0069F53D
 */
static void rct1_load_default_objects()
{
	for (int i = 0; i < 9; i++) {
		rct_object_entry *entries = (rct_object_entry*)RCT1DefaultObjects[i].entries;
		for (int j = 0; j < RCT1DefaultObjects[i].count; j++) {
			if (!object_load(j, &entries[j], NULL)) {
				error_string_quit(0x99990000 + (i * 0x100) + j, -1);
				return;
			}
		}
	}

	// Water is a special case
	rct_object_entry *waterEntries = (rct_object_entry*)RCT1DefaultObjects[9].entries;
	rct_object_entry *waterEntry = &waterEntries[RCT2_GLOBAL(0x01358841, uint8) == 0 ? 0 : 1];
	if (!object_load(0, waterEntry, NULL)) {
		error_string_quit(0x99990900, -1);
		return;
	}
}

/**
 * 
 *  rct2: 0x006A29B9
 */
static void rct1_fix_terrain()
{
	rct_map_element *element;
	map_element_iterator it;

	map_element_iterator_begin(&it);
	while (map_element_iterator_next(&it)) {
		element = it.element;

		if (map_element_get_type(element) != MAP_ELEMENT_TYPE_SURFACE)
			continue;
		
		// Convert terrain
		map_element_set_terrain(element, RCT1TerrainConvertTable[map_element_get_terrain(element)]);
		map_element_set_terrain_edge(element, RCT1TerrainEdgeConvertTable[map_element_get_terrain_edge(element)]);
	}
}

/**
 *
 *  rct2: 0x006A2956
 */
static void rct1_fix_scenery()
{
	rct_map_element *element;
	map_element_iterator it;

	map_element_iterator_begin(&it);
	while (map_element_iterator_next(&it)) {
		element = it.element;

		if (map_element_get_type(element) != MAP_ELEMENT_TYPE_SCENERY)
			continue;

		switch (element->properties.scenery.type) {
		case 157:	// TGE1	(Geometric Sculpture)
		case 162:	// TGE2	(Geometric Sculpture)
		case 168:	// TGE3	(Geometric Sculpture)
		case 170:	// TGE4	(Geometric Sculpture)
		case 171:	// TGE5	(Geometric Sculpture)
			element->properties.scenery.colour_2 = 2;
			break;
		}
	}
}

/**
 * This isn't really RCT1 specific anymore.
 *  rct2: 0x006A2A68
 */
static void rct1_fix_entrance_positions()
{
	rct_map_element *element;
	map_element_iterator it;
	
	for (int i = 0; i < 4; i++)
		RCT2_ADDRESS(RCT2_ADDRESS_PARK_ENTRANCE_X, uint16)[i] = 0x8000;

	int entranceIndex = 0;

	map_element_iterator_begin(&it);
	while (map_element_iterator_next(&it)) {
		element = it.element;

		if (map_element_get_type(element) != MAP_ELEMENT_TYPE_ENTRANCE)
			continue;
		if (element->properties.entrance.type != ENTRANCE_TYPE_PARK_ENTRANCE)
			continue;
		if ((element->properties.entrance.index & 0x0F) != 0)
			continue;

		RCT2_ADDRESS(RCT2_ADDRESS_PARK_ENTRANCE_X, uint16)[entranceIndex] = it.x * 32;
		RCT2_ADDRESS(RCT2_ADDRESS_PARK_ENTRANCE_Y, uint16)[entranceIndex] = it.y * 32;
		RCT2_ADDRESS(RCT2_ADDRESS_PARK_ENTRANCE_Z, uint16)[entranceIndex] = element->base_height * 8;
		RCT2_ADDRESS(RCT2_ADDRESS_PARK_ENTRANCE_DIRECTION, uint8)[entranceIndex] = element->type & 3;
		entranceIndex++;

		// Prevent overflow
		if (entranceIndex == 4)
			return;
	}
}

/**
 *
 *  rct2: 0x0069F509
 */
static void rct1_reset_research()
{
	rct_research_item *researchItem;

	researchItem = gResearchItems;
	researchItem->entryIndex = RESEARCHED_ITEMS_SEPERATOR;
	researchItem++;
	researchItem->entryIndex = RESEARCHED_ITEMS_END;
	researchItem++;
	researchItem->entryIndex = RESEARCHED_ITEMS_END_2;
	RCT2_GLOBAL(RCT2_ADDRESS_RESEARH_PROGRESS_STAGE, uint8) = 0;
	RCT2_GLOBAL(0x01357CF4, sint32) = -1;
	news_item_init_queue();
}

/**
 *
 *  rct2: 0x0069F06A
 */
static void sub_69F06A()
{
	RCT2_CALLPROC_EBPSAFE(0x0069F06A); return;

	// TODO, bug with the following code
	RCT2_GLOBAL(0x013CE770, uint32) |= (1 << 0) | (1 << 1) | (1 << 14) | (1 << 2) | (1 << 3);
	if (!(RCT2_GLOBAL(0x013CE770, uint32) & (1 << 4))) {
		RCT2_GLOBAL(0x013CE770, uint32) |= (1 << 4);
		banner_init(); // 6B9CB0
	}
	if (!(RCT2_GLOBAL(0x013CE770, uint32) & (1 << 6))) {
		RCT2_GLOBAL(0x013CE770, uint32) |= (1 << 6);
		RCT2_CALLPROC_EBPSAFE(0x0069E891);
	}
	RCT2_GLOBAL(0x013CE770, uint32) |= (1 << 7);
	if (!(RCT2_GLOBAL(0x013CE770, uint32) & (1 << 8))) {
		RCT2_GLOBAL(0x013CE770, uint32) |= (1 << 8);
		sub_666DFD();
	}
	if (!(RCT2_GLOBAL(0x013CE770, uint32) & (1 << 9))) {
		RCT2_GLOBAL(0x013CE770, uint32) |= (1 << 9);
		RCT2_CALLPROC_EBPSAFE(0x0069E89B);
	}
	if (!(RCT2_GLOBAL(0x013CE770, uint32) & (1 << 13))) {
		RCT2_GLOBAL(0x013CE770, uint32) |= (1 << 13);
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_SIZE_UNITS, uint16) = 127 * 32;
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_MAXIMUM_X_Y, uint16) = 4350;
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_SIZE_UNITS, uint16) = 128;
		RCT2_GLOBAL(0x01358836, uint16) = 4095;
	}
	if (!(RCT2_GLOBAL(0x013CE770, uint32) & (1 << 15))) {
		RCT2_GLOBAL(0x013CE770, uint32) |= (1 << 15);
		RCT2_GLOBAL(0x01358838, uint32) = 0;
	}
	RCT2_GLOBAL(0x013CE770, uint32) |= (1 << 16) | (1 << 18) | (1 << 19);
}

/**
 *
 *  rct2: 0x00666DFD
 */
static void sub_666DFD()
{
	int x, y;
	rct_map_element *mapElement;

	x = RCT2_GLOBAL(0x013573EA, uint16);
	y = RCT2_GLOBAL(0x013573EC, uint16);
	if (x == (sint16)0x8000)
		return;

	x /= 32;
	y /= 32;
	mapElement = map_get_first_element_at(x, y);
	do {
		if (map_element_get_type(mapElement) == MAP_ELEMENT_TYPE_ENTRANCE) {
			if (mapElement->properties.entrance.type == ENTRANCE_TYPE_PARK_ENTRANCE) {
				mapElement->properties.entrance.path_type = 0;
				break;
			}
		}
	} while (!map_element_is_last_for_tile(mapElement++));
}

#pragma region Tables

const uint8 RCT1TerrainConvertTable[16] = {
	TERRAIN_GRASS,
	TERRAIN_SAND,
	TERRAIN_DIRT,
	TERRAIN_ROCK,
	TERRAIN_MARTIAN,
	TERRAIN_CHECKERBOARD,
	TERRAIN_GRASS_CLUMPS,
	TERRAIN_DIRT,				// Originally TERRAIN_ROOF_BROWN
	TERRAIN_ICE,
	TERRAIN_DIRT,				// Originally TERRAIN_ROOF_LOG
	TERRAIN_DIRT,				// Originally TERRAIN_ROOF_IRON
	TERRAIN_DIRT,				// Originally TERRAIN_ROOF_GREY
	TERRAIN_GRID_RED,
	TERRAIN_GRID_YELLOW,
	TERRAIN_GRID_BLUE,
	TERRAIN_GRID_GREEN
};

const uint8 RCT1TerrainEdgeConvertTable[16] = {
	TERRAIN_EDGE_ROCK,
	TERRAIN_EDGE_ROCK,			// Originally TERRAIN_EDGE_BRICK
	TERRAIN_EDGE_ROCK,			// Originally TERRAIN_EDGE_IRON
	TERRAIN_EDGE_WOOD_RED,
	TERRAIN_EDGE_ROCK,			// Originally TERRAIN_EDGE_GREY
	TERRAIN_EDGE_ROCK,			// Originally TERRAIN_EDGE_YELLOW
	TERRAIN_EDGE_WOOD_BLACK,
	TERRAIN_EDGE_ROCK,			// Originally TERRAIN_EDGE_RED
	TERRAIN_EDGE_ICE,
	TERRAIN_EDGE_ROCK,			// Originally TERRAIN_EDGE_PURPLE
	TERRAIN_EDGE_ROCK,			// Originally TERRAIN_EDGE_GREEN
	TERRAIN_EDGE_ROCK,			// Originally TERRAIN_EDGE_STONE_BROWN
	TERRAIN_EDGE_ROCK,			// Originally TERRAIN_EDGE_STONE_GREY
	TERRAIN_EDGE_ROCK,			// Originally TERRAIN_EDGE_SKYSCRAPER_A
	TERRAIN_EDGE_ROCK,			// Originally TERRAIN_EDGE_SKYSCRAPER_B
	TERRAIN_EDGE_ROCK			// Unused
};

#pragma endregion

#pragma region RCT1 Default Objects

static const rct_object_entry RCT1DefaultObjectsRides[] = {
	{ 0x00008000, { "PTCT1   " }, 0 },
	{ 0x00008000, { "TOGST   " }, 0 },
	{ 0x00008000, { "ARRSW1  " }, 0 },
	{ 0x00008000, { "NEMT    " }, 0 },
	{ 0x00008000, { "ZLDB    " }, 0 },
	{ 0x00008000, { "NRL     " }, 0 },
	{ 0x00008000, { "MONO2   " }, 0 },
	{ 0x00008000, { "BATFL   " }, 0 },
	{ 0x00008000, { "RBOAT   " }, 0 },
	{ 0x00008000, { "WMOUSE  " }, 0 },
	{ 0x00008000, { "STEEP1  " }, 0 },
	{ 0x00008000, { "SPCAR   " }, 0 },
	{ 0x00008000, { "SSC1    " }, 0 },
	{ 0x00008000, { "BOB1    " }, 0 },
	{ 0x00008000, { "OBS1    " }, 0 },
	{ 0x00008000, { "SCHT1   " }, 0 },
	{ 0x00008000, { "DING1   " }, 0 },
	{ 0x00008000, { "AMT1    " }, 0 },
	{ 0x00008000, { "CLIFT1  " }, 0 },
	{ 0x00008000, { "ARRT1   " }, 0 },
	{ 0x00008000, { "HMAZE   " }, 0 },
	{ 0x00008000, { "HSKELT  " }, 0 },
	{ 0x00008000, { "KART1   " }, 0 },
	{ 0x00008000, { "LFB1    " }, 0 },
	{ 0x00008000, { "RAPBOAT " }, 0 },
	{ 0x00008000, { "DODG1   " }, 0 },
	{ 0x00008000, { "SWSH1   " }, 0 },
	{ 0x00008000, { "SWSH2   " }, 0 },
	{ 0x00008000, { "ICECR1  " }, 0 },
	{ 0x00008000, { "CHPSH2  " }, 0 },
	{ 0x00008000, { "DRNKS   " }, 0 },
	{ 0x00008000, { "CNDYF   " }, 0 },
	{ 0x00008000, { "BURGB   " }, 0 },
	{ 0x00008000, { "MGR1    " }, 0 },
	{ 0x00008000, { "BALLN   " }, 0 },
	{ 0x00008000, { "INFOK   " }, 0 },
	{ 0x00008000, { "TLT1    " }, 0 },
	{ 0x00008000, { "FWH1    " }, 0 },
	{ 0x00008000, { "SIMPOD  " }, 0 },
	{ 0x00008000, { "C3D     " }, 0 },
	{ 0x00008000, { "TOPSP1  " }, 0 },
	{ 0x00008000, { "SRINGS  " }, 0 },
	{ 0x00008000, { "REVF1   " }, 0 },
	{ 0x00008000, { "SOUVS   " }, 0 },
	{ 0x00008000, { "BMVD    " }, 0 },
	{ 0x00008000, { "PIZZS   " }, 0 },
	{ 0x00008000, { "TWIST1  " }, 0 },
	{ 0x00008000, { "HHBUILD " }, 0 },
	{ 0x00008000, { "POPCS   " }, 0 },
	{ 0x00008000, { "CIRCUS1 " }, 0 },
	{ 0x00008000, { "GTC     " }, 0 },
	{ 0x00008000, { "BMSD    " }, 0 },
	{ 0x00008000, { "PTCT1   " }, 0 },
	{ 0x00008000, { "SFRIC1  " }, 0 },
	{ 0x00008000, { "SMC1    " }, 0 },
	{ 0x00008000, { "HOTDS   " }, 0 },
	{ 0x00008000, { "SQDST   " }, 0 },
	{ 0x00008000, { "HATST   " }, 0 },
	{ 0x00008000, { "TOFFS   " }, 0 },
	{ 0x00008000, { "VREEL   " }, 0 },
	{ 0x00008000, { "SPBOAT  " }, 0 },
	{ 0x00008000, { "MONBK   " }, 0 },
	{ 0x00008000, { "BMAIR   " }, 0 },
	{ 0x00008000, { "SMONO   " }, 0 },
	{ 0x00000000, { "        " }, 0 },
	{ 0x00008000, { "REVCAR  " }, 0 },
	{ 0x00008000, { "UTCAR   " }, 0 },
	{ 0x00008000, { "GOLF1   " }, 0 },
	{ 0x00000000, { "        " }, 0 },
	{ 0x00008000, { "GDROP1  " }, 0 },
	{ 0x00008000, { "FSAUC   " }, 0 },
	{ 0x00008000, { "CHBUILD " }, 0 },
	{ 0x00008000, { "HELICAR " }, 0 },
	{ 0x00008000, { "SLCT    " }, 0 },
	{ 0x00008000, { "CSTBOAT " }, 0 },
	{ 0x00008000, { "THCAR   " }, 0 },
	{ 0x00008000, { "IVMC1   " }, 0 },
	{ 0x00008000, { "JSKI    " }, 0 },
	{ 0x00008000, { "TSHRT   " }, 0 },
	{ 0x00008000, { "RFTBOAT " }, 0 },
	{ 0x00008000, { "DOUGH   " }, 0 },
	{ 0x00008000, { "ENTERP  " }, 0 },
	{ 0x00008000, { "COFFS   " }, 0 },
	{ 0x00008000, { "CHCKS   " }, 0 },
	{ 0x00008000, { "LEMST   " }, 0 }
};

// rct2: 0x0098BD0E
static const rct_object_entry RCT1DefaultObjectsSmallScenery[] = {
	{ 0x00000081, { "TL0     " }, 0 },
	{ 0x00000081, { "TL1     " }, 0 },
	{ 0x00000081, { "TL2     " }, 0 },
	{ 0x00000081, { "TL3     " }, 0 },
	{ 0x00000081, { "TM0     " }, 0 },
	{ 0x00000081, { "TM1     " }, 0 },
	{ 0x00000081, { "TM2     " }, 0 },
	{ 0x00000081, { "TM3     " }, 0 },
	{ 0x00000081, { "TS0     " }, 0 },
	{ 0x00000081, { "TS1     " }, 0 },
	{ 0x00000081, { "TS2     " }, 0 },
	{ 0x00000081, { "TS3     " }, 0 },
	{ 0x00000081, { "TS4     " }, 0 },
	{ 0x00000081, { "TS5     " }, 0 },
	{ 0x00000081, { "TS6     " }, 0 },
	{ 0x00000081, { "TIC     " }, 0 },
	{ 0x00000081, { "TLC     " }, 0 },
	{ 0x00000081, { "TMC     " }, 0 },
	{ 0x00000081, { "TMP     " }, 0 },
	{ 0x00000081, { "TITC    " }, 0 },
	{ 0x00000081, { "TGHC    " }, 0 },
	{ 0x00000081, { "TAC     " }, 0 },
	{ 0x00000081, { "TGHC2   " }, 0 },
	{ 0x00000081, { "TCJ     " }, 0 },
	{ 0x00000081, { "TMBJ    " }, 0 },
	{ 0x00000081, { "TCF     " }, 0 },
	{ 0x00000081, { "TCL     " }, 0 },
	{ 0x00000081, { "TRF     " }, 0 },
	{ 0x00000081, { "TRF2    " }, 0 },
	{ 0x00000081, { "TEL     " }, 0 },
	{ 0x00000081, { "TAP     " }, 0 },
	{ 0x00000081, { "TSP     " }, 0 },
	{ 0x00000081, { "TMZP    " }, 0 },
	{ 0x00000081, { "TCRP    " }, 0 },
	{ 0x00000081, { "TBP     " }, 0 },
	{ 0x00000081, { "TLP     " }, 0 },
	{ 0x00000081, { "TWP     " }, 0 },
	{ 0x00000081, { "TAS     " }, 0 },
	{ 0x00000081, { "TMG     " }, 0 },
	{ 0x00000081, { "TWW     " }, 0 },
	{ 0x00000081, { "TSB     " }, 0 },
	{ 0x00000081, { "TVL     " }, 0 },
	{ 0x00000081, { "TCT     " }, 0 },
	{ 0x00000081, { "TEF     " }, 0 },
	{ 0x00000081, { "TAL     " }, 0 },
	{ 0x00000081, { "TSQ     " }, 0 },
	{ 0x00000081, { "THT     " }, 0 },
	{ 0x00000081, { "TCB     " }, 0 },
	{ 0x00000081, { "TDM     " }, 0 },
	{ 0x00000081, { "TSD     " }, 0 },
	{ 0x00000081, { "TGS     " }, 0 },
	{ 0x00000081, { "TUS     " }, 0 },
	{ 0x00000081, { "TH1     " }, 0 },
	{ 0x00000081, { "TBC     " }, 0 },
	{ 0x00000081, { "TH2     " }, 0 },
	{ 0x00000081, { "TPM     " }, 0 },
	{ 0x00000081, { "TSC     " }, 0 },
	{ 0x00000081, { "TG1     " }, 0 },
	{ 0x00000081, { "TWF     " }, 0 },
	{ 0x00000081, { "TSH0    " }, 0 },
	{ 0x00000081, { "TSH1    " }, 0 },
	{ 0x00000081, { "TSH2    " }, 0 },
	{ 0x00000081, { "TSH3    " }, 0 },
	{ 0x00000081, { "TSH4    " }, 0 },
	{ 0x00000081, { "TSH5    " }, 0 },
	{ 0x00000081, { "TG2     " }, 0 },
	{ 0x00000081, { "TG3     " }, 0 },
	{ 0x00000081, { "TG4     " }, 0 },
	{ 0x00000081, { "TG5     " }, 0 },
	{ 0x00000081, { "TG6     " }, 0 },
	{ 0x00000081, { "TG7     " }, 0 },
	{ 0x00000081, { "TG8     " }, 0 },
	{ 0x00000081, { "TG9     " }, 0 },
	{ 0x00000081, { "TG10    " }, 0 },
	{ 0x00000081, { "TG11    " }, 0 },
	{ 0x00000081, { "TG12    " }, 0 },
	{ 0x00000081, { "TG13    " }, 0 },
	{ 0x00000081, { "TG14    " }, 0 },
	{ 0x00000081, { "TT1     " }, 0 },
	{ 0x00000081, { "TDF     " }, 0 },
	{ 0x00000081, { "TSH     " }, 0 },
	{ 0x00000081, { "THRS    " }, 0 },
	{ 0x00000081, { "TSTD    " }, 0 },
	{ 0x00000081, { "TRMS    " }, 0 },
	{ 0x00000081, { "TRWS    " }, 0 },
	{ 0x00000081, { "TRC     " }, 0 },
	{ 0x00000081, { "TQF     " }, 0 },
	{ 0x00000081, { "TES1    " }, 0 },
	{ 0x00000081, { "TEN     " }, 0 },
	{ 0x00000081, { "TERS    " }, 0 },
	{ 0x00000081, { "TERB    " }, 0 },
	{ 0x00000081, { "TEP     " }, 0 },
	{ 0x00000081, { "TST1    " }, 0 },
	{ 0x00000081, { "TST2    " }, 0 },
	{ 0x00000081, { "TMS1    " }, 0 },
	{ 0x00000081, { "TAS1    " }, 0 },
	{ 0x00000081, { "TAS2    " }, 0 },
	{ 0x00000081, { "TAS3    " }, 0 },
	{ 0x00000081, { "TST3    " }, 0 },
	{ 0x00000081, { "TST4    " }, 0 },
	{ 0x00000081, { "TST5    " }, 0 },
	{ 0x00000081, { "TAS4    " }, 0 },
	{ 0x00000081, { "TCY     " }, 0 },
	{ 0x00000081, { "TBW     " }, 0 },
	{ 0x00000081, { "TBR1    " }, 0 },
	{ 0x00000081, { "TBR2    " }, 0 },
	{ 0x00000081, { "TML     " }, 0 },
	{ 0x00000081, { "TMW     " }, 0 },
	{ 0x00000081, { "TBR3    " }, 0 },
	{ 0x00000081, { "TBR4    " }, 0 },
	{ 0x00000081, { "TMJ     " }, 0 },
	{ 0x00000081, { "TBR     " }, 0 },
	{ 0x00000081, { "TMO1    " }, 0 },
	{ 0x00000081, { "TMO2    " }, 0 },
	{ 0x00000081, { "TMO3    " }, 0 },
	{ 0x00000081, { "TMO4    " }, 0 },
	{ 0x00000081, { "TMO5    " }, 0 },
	{ 0x00000081, { "TWH1    " }, 0 },
	{ 0x00000081, { "TWH2    " }, 0 },
	{ 0x00000081, { "TNS     " }, 0 },
	{ 0x00000081, { "TP1     " }, 0 },
	{ 0x00000081, { "TP2     " }, 0 },
	{ 0x00000081, { "TK1     " }, 0 },
	{ 0x00000081, { "TK2     " }, 0 },
	{ 0x00000081, { "TR1     " }, 0 },
	{ 0x00000081, { "TR2     " }, 0 },
	{ 0x00000081, { "TQ1     " }, 0 },
	{ 0x00000081, { "TQ2     " }, 0 },
	{ 0x00000081, { "TWN     " }, 0 },
	{ 0x00000081, { "TCE     " }, 0 },
	{ 0x00000081, { "TCO     " }, 0 },
	{ 0x00000081, { "THL     " }, 0 },
	{ 0x00000081, { "TCC     " }, 0 },
	{ 0x00000081, { "TB1     " }, 0 },
	{ 0x00000081, { "TB2     " }, 0 },
	{ 0x00000081, { "TK3     " }, 0 },
	{ 0x00000081, { "TK4     " }, 0 },
	{ 0x00000081, { "TBN     " }, 0 },
	{ 0x00000081, { "TBN1    " }, 0 },
	{ 0x00000081, { "TDT1    " }, 0 },
	{ 0x00000081, { "TDT2    " }, 0 },
	{ 0x00000081, { "TDT3    " }, 0 },
	{ 0x00000081, { "TMM1    " }, 0 },
	{ 0x00000081, { "TMM2    " }, 0 },
	{ 0x00000081, { "TMM3    " }, 0 },
	{ 0x00000081, { "TGS1    " }, 0 },
	{ 0x00000081, { "TGS2    " }, 0 },
	{ 0x00000081, { "TGS3    " }, 0 },
	{ 0x00000081, { "TGS4    " }, 0 },
	{ 0x00000081, { "TDN4    " }, 0 },
	{ 0x00000081, { "TDN5    " }, 0 },
	{ 0x00000081, { "TJT1    " }, 0 },
	{ 0x00000081, { "TJT2    " }, 0 },
	{ 0x00000081, { "TJB1    " }, 0 },
	{ 0x00000081, { "TTF     " }, 0 },
	{ 0x00000081, { "TF1     " }, 0 },
	{ 0x00000081, { "TF2     " }, 0 },
	{ 0x00000081, { "TGE1    " }, 0 },
	{ 0x00000081, { "TJT3    " }, 0 },
	{ 0x00000081, { "TJT4    " }, 0 },
	{ 0x00000081, { "TJP1    " }, 0 },
	{ 0x00000081, { "TJB2    " }, 0 },
	{ 0x00000081, { "TGE2    " }, 0 },
	{ 0x00000081, { "TJT5    " }, 0 },
	{ 0x00000081, { "TJB3    " }, 0 },
	{ 0x00000081, { "TJB4    " }, 0 },
	{ 0x00000081, { "TJT6    " }, 0 },
	{ 0x00000081, { "TJP2    " }, 0 },
	{ 0x00000081, { "TGE3    " }, 0 },
	{ 0x00000081, { "TCK     " }, 0 },
	{ 0x00000081, { "TGE4    " }, 0 },
	{ 0x00000081, { "TGE5    " }, 0 },
	{ 0x00000081, { "TG15    " }, 0 },
	{ 0x00000081, { "TG16    " }, 0 },
	{ 0x00000081, { "TG17    " }, 0 },
	{ 0x00000081, { "TG18    " }, 0 },
	{ 0x00000081, { "TG19    " }, 0 },
	{ 0x00000081, { "TG20    " }, 0 },
	{ 0x00000081, { "TG21    " }, 0 },
	{ 0x00000081, { "TSM     " }, 0 },
	{ 0x00000081, { "TIG     " }, 0 },
	{ 0x00000081, { "TCFS    " }, 0 },
	{ 0x00000081, { "TRFS    " }, 0 },
	{ 0x00000081, { "TRF3    " }, 0 },
	{ 0x00000081, { "TNSS    " }, 0 },
	{ 0x00000081, { "TCT1    " }, 0 },
	{ 0x00000081, { "TCT2    " }, 0 },
	{ 0x00000081, { "TSF1    " }, 0 },
	{ 0x00000081, { "TSF2    " }, 0 },
	{ 0x00000081, { "TSF3    " }, 0 },
	{ 0x00000081, { "TCN     " }, 0 },
	{ 0x00000081, { "TTG     " }, 0 },
	{ 0x00000081, { "TSNC    " }, 0 },
	{ 0x00000081, { "TSNB    " }, 0 },
	{ 0x00000081, { "TSCP    " }, 0 },
	{ 0x00000081, { "TCD     " }, 0 },
	{ 0x00000081, { "TSG     " }, 0 },
	{ 0x00000081, { "TSK     " }, 0 },
	{ 0x00000081, { "TGH1    " }, 0 },
	{ 0x00000081, { "TGH2    " }, 0 },
	{ 0x00000081, { "TSMP    " }, 0 },
	{ 0x00000081, { "TJF     " }, 0 },
	{ 0x00000081, { "TLY     " }, 0 },
	{ 0x00000081, { "TGC1    " }, 0 },
	{ 0x00000081, { "TGC2    " }, 0 },
	{ 0x00000081, { "TGG     " }, 0 },
	{ 0x00000081, { "TSPH    " }, 0 },
	{ 0x00000081, { "TOH1    " }, 0 },
	{ 0x00000081, { "TOH2    " }, 0 },
	{ 0x00000081, { "TOT1    " }, 0 },
	{ 0x00000081, { "TOT2    " }, 0 },
	{ 0x00000081, { "TOS     " }, 0 },
	{ 0x00000081, { "TOT3    " }, 0 },
	{ 0x00000081, { "TOT4    " }, 0 },
	{ 0x00000081, { "TSC2    " }, 0 },
	{ 0x00000081, { "TSP1    " }, 0 },
	{ 0x00000081, { "TOH3    " }, 0 },
	{ 0x00000081, { "TSP2    " }, 0 },
	{ 0x00000081, { "ROMROOF1" }, 0 },
	{ 0x00000081, { "GEOROOF1" }, 0 },
	{ 0x00000081, { "TNTROOF1" }, 0 },
	{ 0x00000081, { "JNGROOF1" }, 0 },
	{ 0x00000081, { "MINROOF1" }, 0 },
	{ 0x00000081, { "ROMROOF2" }, 0 },
	{ 0x00000081, { "GEOROOF2" }, 0 },
	{ 0x00000081, { "PAGROOF1" }, 0 },
	{ 0x00000081, { "SPCROOF1" }, 0 },
	{ 0x00000081, { "ROOF1   " }, 0 },
	{ 0x00000081, { "ROOF2   " }, 0 },
	{ 0x00000081, { "ROOF3   " }, 0 },
	{ 0x00000081, { "ROOF4   " }, 0 },
	{ 0x00000081, { "ROOF5   " }, 0 },
	{ 0x00000081, { "ROOF6   " }, 0 },
	{ 0x00000081, { "ROOF7   " }, 0 },
	{ 0x00000081, { "ROOF8   " }, 0 },
	{ 0x00000081, { "ROOF9   " }, 0 },
	{ 0x00000081, { "ROOF10  " }, 0 },
	{ 0x00000081, { "ROOF11  " }, 0 },
	{ 0x00000081, { "ROOF12  " }, 0 },
	{ 0x00000081, { "ROOF13  " }, 0 },
	{ 0x00000081, { "ROOF14  " }, 0 },
	{ 0x00000081, { "IGROOF  " }, 0 },
	{ 0x00000081, { "CORROOF " }, 0 },
	{ 0x00000081, { "CORROOF2" }, 0 }
};

static const rct_object_entry RCT1DefaultObjectsLargeScenery[] = {
	{ 0x00000082, { "SCOL    " }, 0 },
	{ 0x00000082, { "SHS1    " }, 0 },
	{ 0x00000082, { "SSPX    " }, 0 },
	{ 0x00000082, { "SHS2    " }, 0 },
	{ 0x00000082, { "SCLN    " }, 0 },
	{ 0x00000082, { "SMH1    " }, 0 },
	{ 0x00000082, { "SMH2    " }, 0 },
	{ 0x00000082, { "SVLC    " }, 0 },
	{ 0x00000082, { "SPYR    " }, 0 },
	{ 0x00000082, { "SMN1    " }, 0 },
	{ 0x00000082, { "SMB     " }, 0 },
	{ 0x00000082, { "SSK1    " }, 0 },
	{ 0x00000082, { "SDN1    " }, 0 },
	{ 0x00000082, { "SDN2    " }, 0 },
	{ 0x00000082, { "SDN3    " }, 0 },
	{ 0x00000082, { "SIP     " }, 0 },
	{ 0x00000082, { "STB1    " }, 0 },
	{ 0x00000082, { "STB2    " }, 0 },
	{ 0x00000082, { "STG1    " }, 0 },
	{ 0x00000082, { "STG2    " }, 0 },
	{ 0x00000082, { "SCT     " }, 0 },
	{ 0x00000082, { "SOH1    " }, 0 },
	{ 0x00000082, { "SOH2    " }, 0 },
	{ 0x00000082, { "SOH3    " }, 0 },
	{ 0x00000082, { "SGP     " }, 0 },
	{ 0x00000082, { "SSR     " }, 0 },
	{ 0x00000082, { "STH     " }, 0 },
	{ 0x00000082, { "SAH     " }, 0 },
	{ 0x00000082, { "SPS     " }, 0 },
	{ 0x00000082, { "SPG     " }, 0 },
	{ 0x00000082, { "SOB     " }, 0 },
	{ 0x00000082, { "SAH2    " }, 0 },
	{ 0x00000082, { "SST     " }, 0 },
	{ 0x00000082, { "SSH     " }, 0 },
	{ 0x00000082, { "SAH3    " }, 0 },
	{ 0x00000082, { "SSIG1   " }, 0 },
	{ 0x00000082, { "SSIG2   " }, 0 },
	{ 0x00000082, { "SSIG3   " }, 0 },
	{ 0x00000082, { "SSIG4   " }, 0 }
};

static const rct_object_entry RCT1DefaultObjectsWall[] = {
	{ 0x00000083, { "WMF     " }, 0 },
	{ 0x00000083, { "WMFG    " }, 0 },
	{ 0x00000083, { "WRW     " }, 0 },
	{ 0x00000083, { "WEW     " }, 0 },
	{ 0x00000083, { "WHG     " }, 0 },
	{ 0x00000083, { "WHGG    " }, 0 },
	{ 0x00000083, { "WCW1    " }, 0 },
	{ 0x00000083, { "WCW2    " }, 0 },
	{ 0x00000083, { "WSW     " }, 0 },
	{ 0x00000083, { "WSWG    " }, 0 },
	{ 0x00000083, { "WMW     " }, 0 },
	{ 0x00000083, { "WFW1    " }, 0 },
	{ 0x00000083, { "WFWG    " }, 0 },
	{ 0x00000083, { "WPW1    " }, 0 },
	{ 0x00000083, { "WPW2    " }, 0 },
	{ 0x00000083, { "WPF     " }, 0 },
	{ 0x00000083, { "WPFG    " }, 0 },
	{ 0x00000083, { "WWTW    " }, 0 },
	{ 0x00000083, { "WMWW    " }, 0 },
	{ 0x00000083, { "WSW1    " }, 0 },
	{ 0x00000083, { "WSW2    " }, 0 },
	{ 0x00000083, { "WGW2    " }, 0 },
	{ 0x00000083, { "WBW     " }, 0 },
	{ 0x00000083, { "WBR1    " }, 0 },
	{ 0x00000083, { "WBRG    " }, 0 },
	{ 0x00000083, { "WBR2    " }, 0 },
	{ 0x00000083, { "WBR3    " }, 0 },
	{ 0x00000083, { "WPW3    " }, 0 },
	{ 0x00000083, { "WJF     " }, 0 },
	{ 0x00000083, { "WCH     " }, 0 },
	{ 0x00000083, { "WCHG    " }, 0 },
	{ 0x00000083, { "WC1     " }, 0 },
	{ 0x00000083, { "WC2     " }, 0 },
	{ 0x00000083, { "WC3     " }, 0 },
	{ 0x00000083, { "WC4     " }, 0 },
	{ 0x00000083, { "WC5     " }, 0 },
	{ 0x00000083, { "WC6     " }, 0 },
	{ 0x00000083, { "WC7     " }, 0 },
	{ 0x00000083, { "WC8     " }, 0 },
	{ 0x00000083, { "WC9     " }, 0 },
	{ 0x00000083, { "WC10    " }, 0 },
	{ 0x00000083, { "WC11    " }, 0 },
	{ 0x00000083, { "WC12    " }, 0 },
	{ 0x00000083, { "WC13    " }, 0 },
	{ 0x00000083, { "WC14    " }, 0 },
	{ 0x00000083, { "WC15    " }, 0 },
	{ 0x00000083, { "WC16    " }, 0 },
	{ 0x00000083, { "WC17    " }, 0 },
	{ 0x00000083, { "WC18    " }, 0 },
	{ 0x00000083, { "WALLBRDR" }, 0 },
	{ 0x00000083, { "WALLBR32" }, 0 },
	{ 0x00000083, { "WALLBR16" }, 0 },
	{ 0x00000083, { "WALLBR8 " }, 0 },
	{ 0x00000083, { "WALLCF8 " }, 0 },
	{ 0x00000083, { "WALLCF16" }, 0 },
	{ 0x00000083, { "WALLCF32" }, 0 },
	{ 0x00000083, { "WALLBB8 " }, 0 },
	{ 0x00000083, { "WALLBB16" }, 0 },
	{ 0x00000083, { "WALLBB32" }, 0 },
	{ 0x00000083, { "WALLRS8 " }, 0 },
	{ 0x00000083, { "WALLRS16" }, 0 },
	{ 0x00000083, { "WALLRS32" }, 0 },
	{ 0x00000083, { "WALLCB8 " }, 0 },
	{ 0x00000083, { "WALLCB16" }, 0 },
	{ 0x00000083, { "WALLCB32" }, 0 },
	{ 0x00000083, { "WALLGL8 " }, 0 },
	{ 0x00000083, { "WALLGL16" }, 0 },
	{ 0x00000083, { "WALLGL32" }, 0 },
	{ 0x00000083, { "WALLWD8 " }, 0 },
	{ 0x00000083, { "WALLWD16" }, 0 },
	{ 0x00000083, { "WALLWD32" }, 0 },
	{ 0x00000083, { "WALLTN32" }, 0 },
	{ 0x00000083, { "WALLJN32" }, 0 },
	{ 0x00000083, { "WALLMN32" }, 0 },
	{ 0x00000083, { "WALLSP32" }, 0 },
	{ 0x00000083, { "WALLPG32" }, 0 },
	{ 0x00000083, { "WALLU132" }, 0 },
	{ 0x00000083, { "WALLU232" }, 0 },
	{ 0x00000083, { "WALLCZ32" }, 0 },
	{ 0x00000083, { "WALLCW32" }, 0 },
	{ 0x00000083, { "WALLCY32" }, 0 },
	{ 0x00000083, { "WALLCX32" }, 0 },
	{ 0x00000083, { "WBR1A   " }, 0 },
	{ 0x00000083, { "WBR2A   " }, 0 },
	{ 0x00000083, { "WRWA    " }, 0 },
	{ 0x00000083, { "WWTWA   " }, 0 },
	{ 0x00000083, { "WALLIG16" }, 0 },
	{ 0x00000083, { "WALLIG24" }, 0 },
	{ 0x00000083, { "WALLCO16" }, 0 },
	{ 0x00000083, { "WALLCFDR" }, 0 },
	{ 0x00000083, { "WALLCBDR" }, 0 },
	{ 0x00000083, { "WALLBRWN" }, 0 },
	{ 0x00000083, { "WALLCFWN" }, 0 },
	{ 0x00000083, { "WALLCBWN" }, 0 }
};

static const rct_object_entry RCT1DefaultObjectsBanner[] = {
	{ 0x00000084, { "BN1     " }, 0 },
	{ 0x00000084, { "BN2     " }, 0 },
	{ 0x00000084, { "BN3     " }, 0 },
	{ 0x00000084, { "BN4     " }, 0 },
	{ 0x00000084, { "BN5     " }, 0 },
	{ 0x00000084, { "BN6     " }, 0 },
	{ 0x00000084, { "BN7     " }, 0 },
	{ 0x00000084, { "BN8     " }, 0 },
	{ 0x00000084, { "BN9     " }, 0 }
};

static const rct_object_entry RCT1DefaultObjectsPath[] = {
	{ 0x00000085, { "TARMAC  " }, 0 },
	{ 0x00000085, { "TARMACB " }, 0 },
	{ 0x00000085, { "PATHSPCE" }, 0 },
	{ 0x00000085, { "PATHDIRT" }, 0 },
	{ 0x00000085, { "ROAD    " }, 0 },
	{ 0x00000085, { "PATHCRZY" }, 0 },
	{ 0x00000085, { "PATHASH " }, 0 }
};

static const rct_object_entry RCT1DefaultObjectsPathBits[] = {
	{ 0x00000086, { "LAMP1   " }, 0 },
	{ 0x00000086, { "LAMP2   " }, 0 },
	{ 0x00000086, { "LITTER1 " }, 0 },
	{ 0x00000086, { "BENCH1  " }, 0 },
	{ 0x00000086, { "JUMPFNT1" }, 0 },
	{ 0x00000086, { "LAMP3   " }, 0 },
	{ 0x00000086, { "LAMP4   " }, 0 },
	{ 0x00000086, { "JUMPSNW1" }, 0 }
};

static const rct_object_entry RCT1DefaultObjectsSceneryGroup[] = {
	{ 0x00000087, { "SCGTREES" }, 0 },
	{ 0x00000087, { "SCGSHRUB" }, 0 },
	{ 0x00000087, { "SCGGARDN" }, 0 },
	{ 0x00000087, { "SCGPATHX" }, 0 },
	{ 0x00000087, { "SCGFENCE" }, 0 },
	{ 0x00000087, { "SCGMART " }, 0 },
	{ 0x00000087, { "SCGWOND " }, 0 },
	{ 0x00000087, { "SCGSNOW " }, 0 },
	{ 0x00000087, { "SCGWALLS" }, 0 }
};

static const rct_object_entry RCT1DefaultObjectsParkEntrance[] = {
	{ 0x00000088, { "PKENT1  " }, 0 }
};

static const rct_object_entry RCT1DefaultObjectsWater[] = {
	{ 0x00000089, { "WTRCYAN " }, 0 },
	{ 0x00000089, { "WTRORNG " }, 0 }
};

static const RCT1DefaultObjectsGroup RCT1DefaultObjects[10] = {
	{ NULL,								0											},
	{ RCT1DefaultObjectsSmallScenery,	countof(RCT1DefaultObjectsSmallScenery)		},
	{ RCT1DefaultObjectsLargeScenery,	countof(RCT1DefaultObjectsLargeScenery)		},
	{ RCT1DefaultObjectsWall,			countof(RCT1DefaultObjectsWall)				},
	{ RCT1DefaultObjectsBanner,			countof(RCT1DefaultObjectsBanner)			},
	{ RCT1DefaultObjectsPath,			countof(RCT1DefaultObjectsPath)				},
	{ RCT1DefaultObjectsPathBits,		countof(RCT1DefaultObjectsPathBits)			},
	{ RCT1DefaultObjectsSceneryGroup,	countof(RCT1DefaultObjectsSceneryGroup)		},
	{ RCT1DefaultObjectsParkEntrance,	countof(RCT1DefaultObjectsParkEntrance)		},
	{ RCT1DefaultObjectsWater,			countof(RCT1DefaultObjectsWater)			}
};

#pragma endregion