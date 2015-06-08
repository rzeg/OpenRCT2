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
#include "config.h"
#include "game.h"
#include "interface/viewport.h"
#include "localisation/date.h"
#include "localisation/localisation.h"
#include "management/award.h"
#include "management/finance.h"
#include "management/marketing.h"
#include "management/research.h"
#include "management/news_item.h"
#include "object.h"
#include "peep/staff.h"
#include "platform/platform.h"
#include "ride/ride.h"
#include "scenario.h"
#include "title.h"
#include "util/sawyercoding.h"
#include "util/util.h"
#include "world/map.h"
#include "world/park.h"
#include "world/scenery.h"
#include "world/sprite.h"
#include "world/water.h"

static char _scenarioPath[MAX_PATH];
static const char *_scenarioFileName;

char gScenarioSaveName[MAX_PATH];

static int scenario_create_ducks();

/**
 * Loads only the basic information from a scenario.
 *  rct2: 0x006761D6
 */
int scenario_load_basic(const char *path, rct_s6_header *header, rct_s6_info *info)
{
	FILE *file;

	log_verbose("loading scenario details, %s", path);

	file = fopen(path, "rb");
	if (file != NULL) {
		// Read first chunk
		sawyercoding_read_chunk(file, (uint8*)header);
		if (header->type == S6_TYPE_SCENARIO) {
			// Read second chunk
			sawyercoding_read_chunk(file, (uint8*)info);
			fclose(file);
			RCT2_GLOBAL(0x009AA00C, uint8) = 0;

			// Checks for a scenario string object (possibly for localisation)
			if ((info->entry.flags & 0xFF) != 255) {
				if (object_get_scenario_text(&info->entry)) {
					rct_stex_entry* stex_entry = RCT2_GLOBAL(RCT2_ADDRESS_SCENARIO_TEXT_TEMP_CHUNK, rct_stex_entry*);
					format_string(info->name, stex_entry->scenario_name, NULL);
					format_string(info->details, stex_entry->details, NULL);
					RCT2_GLOBAL(0x009AA00C, uint8) = stex_entry->var_06;
					object_free_scenario_text();
				}
			}
			return 1;
		}
		fclose(file);
	}

	log_error("invalid scenario, %s", path);
	// RCT2_GLOBAL(RCT2_ADDRESS_ERROR_TYPE, sint8) = -1;
	// RCT2_GLOBAL(RCT2_ADDRESS_ERROR_STRING_ID, sint16) = 3011;
	return 0;
}

/**
 * 
 *  rct2: 0x00676053
 * scenario (ebx)
 */
int scenario_load(const char *path)
{
	log_verbose("loading scenario, %s", path);

	FILE *file;
	int i, j;
	rct_s6_header *s6Header = (rct_s6_header*)0x009E34E4;
	rct_s6_info *s6Info = (rct_s6_info*)0x0141F570;

	file = fopen(path, "rb");
	if (file != NULL) {
		if (!sawyercoding_validate_checksum(file)) {
			fclose(file);
			RCT2_GLOBAL(RCT2_ADDRESS_ERROR_TYPE, uint8) = 255;
			RCT2_GLOBAL(RCT2_ADDRESS_ERROR_STRING_ID, uint16) = STR_FILE_CONTAINS_INVALID_DATA;

			log_error("failed to load scenario, invalid checksum");
			return 0;
		}

		// Read first chunk
		sawyercoding_read_chunk(file, (uint8*)s6Header);
		if (s6Header->type == S6_TYPE_SCENARIO) {
			// Read second chunk
			sawyercoding_read_chunk(file, (uint8*)s6Info);

			// Read packed objects
			if (s6Header->num_packed_objects > 0) {
				j = 0;
				for (i = 0; i < s6Header->num_packed_objects; i++)
					j += object_load_packed(file);
				if (j > 0)
					object_list_load();
			}

			uint8 load_success = object_read_and_load_entries(file);

			// Read flags (16 bytes). Loads:
			//	RCT2_ADDRESS_CURRENT_MONTH_YEAR
			//	RCT2_ADDRESS_CURRENT_MONTH_TICKS
			//	RCT2_ADDRESS_SCENARIO_TICKS
			sawyercoding_read_chunk(file, (uint8*)RCT2_ADDRESS_CURRENT_MONTH_YEAR);

			// Read map elements
			memset((void*)RCT2_ADDRESS_MAP_ELEMENTS, 0, MAX_MAP_ELEMENTS * sizeof(rct_map_element));
			sawyercoding_read_chunk(file, (uint8*)RCT2_ADDRESS_MAP_ELEMENTS);

			// Read game data, including sprites
			sawyercoding_read_chunk(file, (uint8*)0x010E63B8);

			// Read number of guests in park and something else
			sawyercoding_read_chunk(file, (uint8*)RCT2_ADDRESS_GUESTS_IN_PARK);

			// Read ?
			sawyercoding_read_chunk(file, (uint8*)0x01357BC8);

			// Read park rating
			sawyercoding_read_chunk(file, (uint8*)RCT2_ADDRESS_CURRENT_PARK_RATING);

			// Read ?
			sawyercoding_read_chunk(file, (uint8*)RCT2_ADDRESS_ACTIVE_RESEARCH_TYPES);

			// Read ?
			sawyercoding_read_chunk(file, (uint8*)RCT2_ADDRESS_CURRENT_EXPENDITURE);

			// Read ?
			sawyercoding_read_chunk(file, (uint8*)RCT2_ADDRESS_CURRENT_PARK_VALUE);

			// Read more game data, including research items and rides
			sawyercoding_read_chunk(file, (uint8*)RCT2_ADDRESS_COMPLETED_COMPANY_VALUE);

			fclose(file);
			if (!load_success){
				log_error("failed to load all entries.");
				set_load_objects_fail_reason();
				return 0;
			}
			// Check expansion pack
			// RCT2_CALLPROC_EBPSAFE(0x006757E6);

			reset_loaded_objects();
			map_update_tile_pointers();
			reset_0x69EBE4();
			return 1;
		}

		fclose(file);
	}

	log_error("failed to find scenario file.");
	RCT2_GLOBAL(RCT2_ADDRESS_ERROR_TYPE, uint8) = 255;
	RCT2_GLOBAL(RCT2_ADDRESS_ERROR_STRING_ID, uint16) = STR_FILE_CONTAINS_INVALID_DATA;
	return 0;
}

/**
 * 
 *  rct2: 0x00678282
 * scenario (ebx)
 */
int scenario_load_and_play(const rct_scenario_basic *scenario)
{
	char path[MAX_PATH];

	subsitute_path(path, RCT2_ADDRESS(RCT2_ADDRESS_SCENARIOS_PATH, char), scenario->path);
	return scenario_load_and_play_from_path(path);
}

int scenario_load_and_play_from_path(const char *path)
{
	rct_window *mainWindow;
	rct_s6_info *s6Info = (rct_s6_info*)0x0141F570;

	// Create the scenario pseduo-random seeds using the current time
	uint32 srand0, srand1;
	srand0 = RCT2_GLOBAL(RCT2_ADDRESS_SCENARIO_SRAND_0, uint32) ^ platform_get_ticks();
	srand1 = RCT2_GLOBAL(RCT2_ADDRESS_SCENARIO_SRAND_1, uint32) ^ platform_get_ticks();

	window_close_construction_windows();

	if (!scenario_load(path))
		return 0;

	strcpy(_scenarioPath, path);
	_scenarioFileName = path_get_filename(_scenarioPath);

	log_verbose("starting scenario, %s", path);

	RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) = SCREEN_FLAGS_PLAYING;
	viewport_init_all();
	game_create_windows();
	mainWindow = window_get_main();

	mainWindow->viewport_target_sprite = -1;
	mainWindow->saved_view_x = RCT2_GLOBAL(RCT2_ADDRESS_SAVED_VIEW_X, sint16);
	mainWindow->saved_view_y = RCT2_GLOBAL(RCT2_ADDRESS_SAVED_VIEW_Y, sint16);

	uint8 _cl = (RCT2_GLOBAL(0x0138869E, sint16) & 0xFF) - mainWindow->viewport->zoom;
	mainWindow->viewport->zoom = RCT2_GLOBAL(0x0138869E, sint16) & 0xFF;
	*((char*)(&RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_ROTATION, sint32))) = RCT2_GLOBAL(0x0138869E, sint16) >> 8;
	if (_cl != 0) {
		if (_cl < 0) {
			_cl = -_cl;
			mainWindow->viewport->view_width >>= _cl;
			mainWindow->viewport->view_height >>= _cl;
		} else {
			mainWindow->viewport->view_width <<= _cl;
			mainWindow->viewport->view_height <<= _cl;
		}
	}
	mainWindow->saved_view_x -= mainWindow->viewport->view_width >> 1;
	mainWindow->saved_view_y -= mainWindow->viewport->view_height >> 1;
	window_invalidate(mainWindow);

	reset_all_sprite_quadrant_placements();
	window_new_ride_init_vars();

	// Set the scenario pseduo-random seeds
	RCT2_GLOBAL(RCT2_ADDRESS_SCENARIO_SRAND_0, sint32) = srand0;
	RCT2_GLOBAL(RCT2_ADDRESS_SCENARIO_SRAND_1, sint32) = srand1;

	RCT2_GLOBAL(0x009DEB7C, sint16) = 0;
	RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, sint32) &= 0xFFFFF7FF;
	if (RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, sint32) & PARK_FLAGS_NO_MONEY_SCENARIO)
		RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, sint32) |= PARK_FLAGS_NO_MONEY;
	sub_684AC3();
	scenery_set_default_placement_configuration();
	news_item_init_queue();
	if (RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_TYPE, uint8) != OBJECTIVE_NONE)
		window_park_objective_open();

	RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_PARK_RATING, sint16) = calculate_park_rating();
	RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_PARK_VALUE, money32) = calculate_park_value();
	RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_COMPANY_VALUE, money32) = calculate_company_value();
	RCT2_GLOBAL(0x013587D0, money32) = RCT2_GLOBAL(0x013573DC, money32) - RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_LOAN, money32);
	RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_MONEY_ENCRYPTED, sint32) = ENCRYPT_MONEY(RCT2_GLOBAL(0x013573DC, sint32));

	sub_69E869(); // (loan related)

	strcpy((char*)RCT2_ADDRESS_SCENARIO_DETAILS, s6Info->details);
	strcpy((char*)RCT2_ADDRESS_SCENARIO_NAME, s6Info->name);

	rct_stex_entry* stex = g_stexEntries[0];
	if ((int)stex != -1) {
		// 
		format_string((char*)RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER, stex->park_name, 0);
		
		// Set park name
		RCT2_GLOBAL(RCT2_ADDRESS_GAME_COMMAND_ERROR_TITLE, uint16) = STR_CANT_RENAME_PARK;
		game_do_command(1, 1, 0, *((int*)(RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER + 0)), GAME_COMMAND_SET_PARK_NAME,
			*((int*)(RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER + 8)), 
			*((int*)(RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER + 4)));
		game_do_command(2, 1, 0, *((int*)(RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER + 12)), GAME_COMMAND_SET_PARK_NAME, 
			*((int*)(RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER + 20)),
			*((int*)(RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER + 16)));
		game_do_command(0, 1, 0, *((int*)(RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER + 24)), GAME_COMMAND_SET_PARK_NAME,
			*((int*)(RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER + 32)),
			*((int*)(RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER + 28)));

		// 
		format_string((char*)RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER, stex->scenario_name, 0);
		strncpy((char*)RCT2_ADDRESS_SCENARIO_NAME, (char*)RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER, 31);
		((char*)RCT2_ADDRESS_SCENARIO_NAME)[31] = '\0';

		// Set scenario details
		format_string((char*)RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER, stex->details, 0);
		strncpy((char*)RCT2_ADDRESS_SCENARIO_DETAILS, (char*)RCT2_ADDRESS_COMMON_STRING_FORMAT_BUFFER, 255);
		((char*)RCT2_ADDRESS_SCENARIO_DETAILS)[255] = '\0';
	}

	// Set the last saved game path
	format_string(gScenarioSaveName, RCT2_GLOBAL(RCT2_ADDRESS_PARK_NAME, rct_string_id), (void*)RCT2_ADDRESS_PARK_NAME_ARGS);
	strcpy((char*)RCT2_ADDRESS_SAVED_GAMES_PATH_2, (char*)RCT2_ADDRESS_SAVED_GAMES_PATH);
	strcpy((char*)RCT2_ADDRESS_SAVED_GAMES_PATH_2 + strlen((char*)RCT2_ADDRESS_SAVED_GAMES_PATH_2), gScenarioSaveName);
	strcat((char*)RCT2_ADDRESS_SAVED_GAMES_PATH_2, ".SV6");

	memset((void*)0x001357848, 0, 56);
	RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_EXPENDITURE, uint32) = 0;
	RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_PROFIT, money32) = 0;
	RCT2_GLOBAL(0x01358334, money32) = 0;
	RCT2_GLOBAL(0x01358338, uint16) = 0;
	RCT2_GLOBAL(RCT2_ADDRESS_COMPLETED_COMPANY_VALUE, uint32) = MONEY32_UNDEFINED;
	RCT2_GLOBAL(RCT2_ADDRESS_TOTAL_ADMISSIONS, uint32) = 0;
	RCT2_GLOBAL(RCT2_ADDRESS_INCOME_FROM_ADMISSIONS, uint32) = 0;
	RCT2_GLOBAL(0x013587D8, uint16) = 63;
	sub_69E869(); // (loan related, called above already)
	park_reset_history();
	finance_reset_history();
	award_reset();
	reset_all_ride_build_dates();
	date_reset();
	duck_remove_all();
	park_calculate_size();
	staff_reset_stats();
	RCT2_GLOBAL(0x01358840, uint8) = 0;
	memset((void*)0x001358102, 0, 20);
	RCT2_GLOBAL(0x00135882E, uint16) = 0;

	// Open park with free entry when there is no money
	if (RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_NO_MONEY) {
		RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) |= PARK_FLAGS_PARK_OPEN;
		RCT2_GLOBAL(RCT2_ADDRESS_PARK_ENTRANCE_FEE, uint16) = 0;
	}

	RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) |= PARK_FLAGS_18;

	load_palette();

	gfx_invalidate_screen();
	RCT2_GLOBAL(0x009DEA66, uint16) = 0;
	RCT2_GLOBAL(0x009DEA5C, uint16) = 62000; // (doesn't appear to ever be read)
	return 1;
}


void scenario_end()
{
	rct_window* w;
	window_close_by_class(WC_DROPDOWN);
	
	for (w = g_window_list; w < RCT2_GLOBAL(RCT2_ADDRESS_NEW_WINDOW_PTR, rct_window*); w++){
		if (!(w->flags & (WF_STICK_TO_BACK | WF_STICK_TO_FRONT)))
			window_close(w);
	}
	window_park_objective_open();
}

void scenario_set_filename(const char *value)
{
	subsitute_path(_scenarioPath, RCT2_ADDRESS(RCT2_ADDRESS_SCENARIOS_PATH, char), value);
	_scenarioFileName = path_get_filename(_scenarioPath);
}

/**
 *
 *  rct2: 0x0066A752
 **/
void scenario_failure()
{
	RCT2_GLOBAL(RCT2_ADDRESS_COMPLETED_COMPANY_VALUE, uint32) = 0x80000001;
	scenario_end();
}

/**
 *
 *  rct2: 0x0066A75E
 */
void scenario_success()
{
	int i;
	rct_scenario_basic* scenario;
	uint32 current_val = RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_COMPANY_VALUE, uint32);
	
	RCT2_GLOBAL(RCT2_ADDRESS_COMPLETED_COMPANY_VALUE, uint32) = current_val;
	peep_applause();

	for (i = 0; i < gScenarioListCount; i++) {
		scenario = &gScenarioList[i];

		if (strequals(scenario->path, _scenarioFileName, 256, true)) {
			// Check if record company value has been broken
			if ((scenario->flags & SCENARIO_FLAGS_COMPLETED) && scenario->company_value >= current_val)
				break;

			// Allow name entry
			RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) |= PARK_FLAGS_SCENARIO_COMPLETE_NAME_INPUT;
			scenario->company_value = current_val;
			scenario->flags |= SCENARIO_FLAGS_COMPLETED;
			scenario->completed_by[0] = 0;
			RCT2_GLOBAL(0x013587C0, uint32) = current_val;
			scenario_scores_save();
			break;
		}
	}
	scenario_end();
}

/**
 *
 *  rct2: 0x006695E8
 */
void scenario_success_submit_name(const char *name)
{
	int i;
	rct_scenario_basic* scenario;
	uint32 scenarioWinCompanyValue;
	
	for (i = 0; i < gScenarioListCount; i++) {
		scenario = &gScenarioList[i];

		if (strequals(scenario->path, _scenarioFileName, 256, true)) {
			scenarioWinCompanyValue = RCT2_GLOBAL(0x013587C0, uint32);
			if (scenario->company_value == scenarioWinCompanyValue) {
				strncpy(scenario->completed_by, name, 64);
				strncpy((char*)0x013587D8, name, 32);
				scenario_scores_save();
			}
			break;
		}
	}
	
	RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) &= ~PARK_FLAGS_SCENARIO_COMPLETE_NAME_INPUT;
}

/**
* Checks if there are 10 rollercoasters of different subtype with
* excitement >= 600 .
* rct2:
**/
void scenario_objective5_check()
{
	int i, rcs = 0;
	uint8 type_already_counted[256];
	rct_ride* ride;

	memset(type_already_counted, 0, 256);

	FOR_ALL_RIDES(i, ride) {
		uint8 subtype_id = ride->subtype;
		rct_ride_type *rideType = gRideTypeList[subtype_id];

		if ((rideType->category[0] == RIDE_GROUP_ROLLERCOASTER || rideType->category[1] == RIDE_GROUP_ROLLERCOASTER) &&
			ride->status == RIDE_STATUS_OPEN &&
			ride->excitement >= RIDE_RATING(6,00) && type_already_counted[subtype_id] == 0){
			type_already_counted[subtype_id]++;
			rcs++;
		}
	}

	if (rcs >= 10)
		scenario_success();
}

/**
 * Checks if there are 10 rollercoasters of different subtype with
 * excitement > 700 and a minimum length;
 * rct2: 0x0066A6B5
 **/
void scenario_objective8_check()
{
	int i, rcs = 0;
	uint8 type_already_counted[256];
	rct_ride* ride;
	sint16 objective_length = RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_NUM_GUESTS, uint16);

	memset(type_already_counted, 0, 256);

	FOR_ALL_RIDES(i, ride) {
		uint8 subtype_id = ride->subtype;
		rct_ride_type *rideType = gRideTypeList[subtype_id];
		if ((rideType->category[0] == RIDE_GROUP_ROLLERCOASTER || rideType->category[1] == RIDE_GROUP_ROLLERCOASTER) &&
			ride->status == RIDE_STATUS_OPEN &&
			ride->excitement >= RIDE_RATING(7,00) && type_already_counted[subtype_id] == 0){

			if ((ride_get_total_length(ride) >> 16) > objective_length) {
				type_already_counted[subtype_id]++;
				rcs++;
			}
		}
	}

	if (rcs >= 10)
		scenario_success();
}



/*
 * Checks the win/lose conditions of the current objective.
 * rct2: 0x0066A4B2
 **/
void scenario_objectives_check()
{
	uint8 objective_type = RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_TYPE, uint8),
		objective_year = RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_YEAR, uint8);
	sint16 park_rating = RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_PARK_RATING, sint16),
		guests_in_park = RCT2_GLOBAL(RCT2_ADDRESS_GUESTS_IN_PARK, uint16),
		objective_guests = RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_NUM_GUESTS, uint16),
		cur_month_year = RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_MONTH_YEAR, sint16);
	uint32 scenario_completed_company_value = RCT2_GLOBAL(RCT2_ADDRESS_COMPLETED_COMPANY_VALUE, uint32);
	sint32 objective_currency = RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_CURRENCY, sint32),
		park_value = RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_PARK_VALUE, sint32);


	if ( scenario_completed_company_value != MONEY32_UNDEFINED)
		return;

	switch (objective_type) {
	case OBJECTIVE_GUESTS_BY://1

		if (cur_month_year == 8 * objective_year){
			if (park_rating >= 600 && guests_in_park >= objective_guests)
				scenario_success();
			else
				scenario_failure();
		}
		break;

	case OBJECTIVE_PARK_VALUE_BY://2

		if (cur_month_year == 8 * objective_year) {
			if (park_value >= objective_currency)
				scenario_success();
			else
				scenario_failure();
		}
		break;

	case OBJECTIVE_10_ROLLERCOASTERS://5

		scenario_objective5_check();
		break;

	case OBJECTIVE_GUESTS_AND_RATING://6

		if (park_rating >= 700 && guests_in_park >= objective_guests)
			scenario_success();
		break;

	case OBJECTIVE_MONTHLY_RIDE_INCOME://7
	{
		sint32 monthly_ride_income = RCT2_GLOBAL(RCT2_ADDRESS_MONTHLY_RIDE_INCOME, sint32);
		if (monthly_ride_income >= objective_currency)
			scenario_success();
		break;
	}
	case OBJECTIVE_10_ROLLERCOASTERS_LENGTH://8

		scenario_objective8_check();
		break;

	case OBJECTIVE_FINISH_5_ROLLERCOASTERS://9
	{
		int i;
		rct_ride* ride;

		// ORIGINAL BUG?:
		// This does not check if the rides are even rollercoasters nevermind the right rollercoasters to be finished.
		// It also did not exclude null rides.
		int rcs = 0;
		FOR_ALL_RIDES(i, ride)
			if (ride->status != RIDE_STATUS_CLOSED && ride->excitement >= objective_currency)
				rcs++;

		if (rcs >= 5)
			scenario_success();
		break;
	}
	case OBJECTIVE_REPLAY_LOAN_AND_PARK_VALUE://A
	{
		sint32 current_loan = RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_LOAN, sint32);
		if (current_loan <= 0 && park_value >= objective_currency)
			scenario_success();
		break;
	}
	case OBJECTIVE_MONTHLY_FOOD_INCOME://B
	{
		sint32 income_sum = RCT2_GLOBAL(0x013578A4, sint32) + RCT2_GLOBAL(0x013578A0, sint32) +
						   RCT2_GLOBAL(0x0135789C, sint32) + RCT2_GLOBAL(0x01357898, sint32);
		if (income_sum >= objective_currency)
			scenario_success();
		break;
	}
	default:
		return;
	}
}


/*
 * Send a warning when entrance price is too high.
 * rct2: 0x0066A80E
 **/
void scenario_entrance_fee_too_high_check()
{
	uint16 x, y;
	uint16 totalRideValue = RCT2_GLOBAL(RCT2_TOTAL_RIDE_VALUE, uint16);
	uint16 park_entrance_fee = RCT2_GLOBAL(RCT2_ADDRESS_PARK_ENTRANCE_FEE, uint16);
	int max_fee = totalRideValue + (totalRideValue / 2);
	uint32 game_flags = RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32), packed_xy;

	if ((game_flags & PARK_FLAGS_PARK_OPEN) && park_entrance_fee > max_fee) {
		for (int i = 0; RCT2_ADDRESS(RCT2_ADDRESS_PARK_ENTRANCE_X, uint16)[i] != SPRITE_LOCATION_NULL; ++i) {
			x = RCT2_ADDRESS(RCT2_ADDRESS_PARK_ENTRANCE_X, uint16)[i] + 16;
			y = RCT2_ADDRESS(RCT2_ADDRESS_PARK_ENTRANCE_Y, uint16)[i] + 16;
		}

		packed_xy = (y << 16) | x;
		news_item_add_to_queue(NEWS_ITEM_BLANK, STR_ENTRANCE_FEE_TOO_HI, packed_xy);
	}
}

static void scenario_autosave_check()
{
	uint32 next_month_tick = RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_MONTH_TICKS, uint16) + 4;
	uint16 month = RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_MONTH_YEAR, uint16);
	bool shouldSave = 0;

	switch (gConfigGeneral.autosave_frequency) {
	case AUTOSAVE_EVERY_WEEK:
		shouldSave = (next_month_tick % 0x4000 == 0);
		break;
	case AUTOSAVE_EVERY_2_WEEKS:
		shouldSave = (next_month_tick % 0x8000 == 0);
		break;
	case AUTOSAVE_EVERY_MONTH:
		shouldSave = (next_month_tick >= 0x10000);
		break;
	case AUTOSAVE_EVERY_4_MONTHS:
		if (next_month_tick >= 0x10000)
			shouldSave = (((month + 1) & 3) == 0);
		break;
	case AUTOSAVE_EVERY_YEAR:
		if (next_month_tick >= 0x10000)
			shouldSave = (((month + 1) & 7) == 0);
		break;
	}

	if (shouldSave)
		game_autosave();
}

/*
 * Scenario and finance related update iteration.
 * rct2: 0x006C44B1
 **/
void scenario_update()
{
	uint8 screen_flags = RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8);
	uint32 month_tick = RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_MONTH_TICKS, uint16),
		next_month_tick = month_tick + 4;
	uint8 month = RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_MONTH_YEAR, sint16) & 7,
		current_days_in_month = (uint8)days_in_month[month],
		objective_type = RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_TYPE, uint8);

	if (screen_flags & (~SCREEN_FLAGS_PLAYING)) // only in normal play mode
		return;

	scenario_autosave_check();

	if ((current_days_in_month * next_month_tick) >> 16 != (current_days_in_month * month_tick) >> 16) {
		// daily checks
		finance_update_daily_profit();
		peep_update_days_in_queue();
		get_local_time();
		RCT2_CALLPROC_EBPSAFE(0x0066A13C); // objective 6 dragging
		switch (objective_type) {
		case OBJECTIVE_REPLAY_LOAN_AND_PARK_VALUE:
		case OBJECTIVE_FINISH_5_ROLLERCOASTERS:
		case OBJECTIVE_10_ROLLERCOASTERS_LENGTH:
		case OBJECTIVE_GUESTS_AND_RATING:
		case OBJECTIVE_10_ROLLERCOASTERS:
			scenario_objectives_check();
			break;
		}

		window_invalidate_by_class(WC_BOTTOM_TOOLBAR);
	}

	if (next_month_tick % 0x4000 == 0) {
		// weekly checks
		finance_pay_wages();
		finance_pay_research();
		finance_pay_interest();
		marketing_update();
		peep_problem_warnings_update();
		ride_check_all_reachable();
		ride_update_favourited_stat();

		rct_water_type* water_type = (rct_water_type*)object_entry_groups[OBJECT_TYPE_WATER].chunks[0];

		if (month <= 1 && (sint32)water_type != -1 && water_type->var_0E & 1) {
			// 100 attempts at finding some water to create a few ducks at
			for (int i = 0; i < 100; i++) {
				if (scenario_create_ducks())
					break;
			}
		}
		park_update_histories();
		park_calculate_size();
	}

	if (next_month_tick % 0x8000 == 0) {
		// fortnightly 
		finance_pay_ride_upkeep();
	}

	RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_MONTH_TICKS, uint16) = (uint16)next_month_tick;
	if (next_month_tick >= 0x10000) {
		// month ends actions
		RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_MONTH_YEAR, sint16)++;
		RCT2_GLOBAL(RCT2_ADDRESS_BTM_TOOLBAR_DIRTY_FLAGS, uint32) |= BTM_TB_DIRTY_FLAG_DATE;
		finance_shift_expenditure_table();
		scenario_objectives_check();
		scenario_entrance_fee_too_high_check();
		award_update_all();
	}
}

/**
 *
 *  rct2: 0x006744A9
 */
static int scenario_create_ducks()
{
	int i, j, r, c, x, y, waterZ, centreWaterZ, x2, y2;

	r = scenario_rand();
	x = ((r >> 16) & 0xFFFF) & 0x7F;
	y = (r & 0xFFFF) & 0x7F;
	x = (x + 64) * 32;
	y = (y + 64) * 32;

	if (!map_is_location_in_park(x, y))
		return 0;

	centreWaterZ = (map_element_height(x, y) >> 16) & 0xFFFF;
	if (centreWaterZ == 0)
		return 0;

	// Check 7x7 area around centre tile
	x2 = x - (32 * 3);
	y2 = y - (32 * 3);
	c = 0;
	for (i = 0; i < 7; i++) {
		for (j = 0; j < 7; j++) {
			waterZ = (map_element_height(x2, y2) >> 16) & 0xFFFF;
			if (waterZ == centreWaterZ)
				c++;

			x2 += 32;
		}
		x2 -= 224;
		y2 += 32;
	}

	// Must be at least 25 water tiles of the same height in 7x7 area
	if (c < 25)
		return 0;

	// Set x, y to the centre of the tile
	x += 16;
	y += 16;
	c = (scenario_rand() & 3) + 2;
	for (i = 0; i < c; i++) {
		r = scenario_rand();
		x2 = (r >> 16) & 0x7F;
		y2 = (r & 0xFFFF) & 0x7F;
		create_duck(x + x2 - 64, y + y2 - 64);
	}

	return 1;
}

/**
 *
 *  rct2: 0x006E37D2
 */
unsigned int scenario_rand()
{
	int eax = RCT2_GLOBAL(RCT2_ADDRESS_SCENARIO_SRAND_0, uint32);
	RCT2_GLOBAL(RCT2_ADDRESS_SCENARIO_SRAND_0, uint32) += ror32(RCT2_GLOBAL(RCT2_ADDRESS_SCENARIO_SRAND_1, uint32) ^ 0x1234567F, 7);
	return RCT2_GLOBAL(RCT2_ADDRESS_SCENARIO_SRAND_1, uint32) = ror32(eax, 3);
}

/**
 * Prepare rides, for the finish five rollercoasters objective.
 *  rct2: 0x006788F7
 */
void scenario_prepare_rides_for_save()
{
	int i;
	rct_ride *ride;
	map_element_iterator it;

	int isFiveCoasterObjective = RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_TYPE, uint8) == OBJECTIVE_FINISH_5_ROLLERCOASTERS;

	// Set all existing track to be indestructible
	map_element_iterator_begin(&it);
	do {
		if (map_element_get_type(it.element) == MAP_ELEMENT_TYPE_TRACK) {
			if (isFiveCoasterObjective)
				it.element->flags |= 0x40;
			else
				it.element->flags &= ~0x40;
		}
	} while (map_element_iterator_next(&it));

	// Set all existing rides to have indestructible track
	FOR_ALL_RIDES(i, ride) {
		if (isFiveCoasterObjective)
			ride->lifecycle_flags |= RIDE_LIFECYCLE_INDESTRUCTIBLE_TRACK;
		else
			ride->lifecycle_flags &= ~RIDE_LIFECYCLE_INDESTRUCTIBLE_TRACK;
	}
}

/**
 *
 *  rct2: 0x006726C7
 */
int scenario_prepare_for_save()
{
	rct_s6_info *s6Info = (rct_s6_info*)0x0141F570;
	char buffer[256];

	s6Info->entry.flags = 255;

	rct_stex_entry* stex = g_stexEntries[0];
	if ((int)stex != 0xFFFFFFFF) {
		format_string(buffer, stex->scenario_name, NULL);
		strncpy(s6Info->name, buffer, sizeof(s6Info->name));
		
		memcpy(&s6Info->entry, &object_entry_groups[OBJECT_TYPE_SCENARIO_TEXT].entries[0], sizeof(rct_object_entry));
	}

	if (s6Info->name[0] == 0)
		format_string(s6Info->name, RCT2_GLOBAL(RCT2_ADDRESS_PARK_NAME, rct_string_id), (void*)RCT2_ADDRESS_PARK_NAME_ARGS);

	s6Info->objective_type = RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_TYPE, uint8);
	s6Info->objective_arg_1 = RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_YEAR, uint8);
	s6Info->objective_arg_2 = RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_CURRENCY, sint32);
	s6Info->objective_arg_3 = RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_NUM_GUESTS, uint16);

	scenario_prepare_rides_for_save();

	if (RCT2_GLOBAL(RCT2_ADDRESS_OBJECTIVE_TYPE, uint8) == OBJECTIVE_GUESTS_AND_RATING)
		RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) |= PARK_FLAGS_PARK_OPEN;

	return 1;
}

/**
 *
 *  rct2: 0x006AA244
 */
int scenario_get_num_packed_objects_to_write()
{
	int i, count = 0;
	rct_object_entry_extended *entry = (rct_object_entry_extended*)0x00F3F03C;

	for (i = 0; i < 721; i++, entry++) {
		if (RCT2_ADDRESS(0x009ACFA4, uint32)[i] == 0xFFFFFFFF || (entry->flags & 0xF0))
			continue;

		count++;
	}

	return count;
}

/**
 *
 *  rct2: 0x006AA26E
 */
int scenario_write_packed_objects(FILE *file)
{
	int i;
	rct_object_entry_extended *entry = (rct_object_entry_extended*)0x00F3F03C;
	for (i = 0; i < 721; i++, entry++) {
		if (RCT2_ADDRESS(0x009ACFA4, uint32)[i] == 0xFFFFFFFF || (entry->flags & 0xF0))
			continue;

		if (!write_object_file(file, (rct_object_entry*)entry))
			return 0;
	}

	return 1;
}

/**
 *
 *  rct2: 0x006AA039
 */
int scenario_write_available_objects(FILE *file)
{
	char *buffer, *dstBuffer;
	int i, encodedLength;
	sawyercoding_chunk_header chunkHeader;

	const int totalEntries = 721;
	const int bufferLength = totalEntries * sizeof(rct_object_entry);

	// Initialise buffers
	buffer = malloc(bufferLength);
	dstBuffer = malloc(bufferLength + sizeof(sawyercoding_chunk_header));
	if (buffer == NULL || dstBuffer == NULL)
		return 0;

	// Write entries
	rct_object_entry_extended *srcEntry = (rct_object_entry_extended*)0x00F3F03C;
	rct_object_entry *dstEntry = (rct_object_entry*)buffer;
	for (i = 0; i < 721; i++) {
		if (RCT2_ADDRESS(0x009ACFA4, uint32)[i] == 0xFFFFFFFF)
			memset(dstEntry, 0xFF, sizeof(rct_object_entry));
		else
			*dstEntry = *((rct_object_entry*)srcEntry);
		
		srcEntry++;
		dstEntry++;
	}

	// Write chunk
	chunkHeader.encoding = CHUNK_ENCODING_ROTATE;
	chunkHeader.length = bufferLength;
	encodedLength = sawyercoding_write_chunk_buffer(dstBuffer, buffer, chunkHeader);
	fwrite(dstBuffer, encodedLength, 1, file);

	// Free buffers
	free(dstBuffer);
	free(buffer);
	return 1;
}

/**
 *
 *  rct2: 0x006754F5
 * @param flags bit 0: pack objects, 1: save as scenario
 */
int scenario_save(char *path, int flags)
{
	rct_s6_header *s6Header = (rct_s6_header*)0x009E34E4;
	rct_s6_info *s6Info = (rct_s6_info*)0x0141F570;

	FILE *file;
	char *buffer;
	sawyercoding_chunk_header chunkHeader;
	int encodedLength;
	long fileSize;
	uint32 checksum;

	rct_window *w;
	rct_viewport *viewport;
	int viewX, viewY, viewZoom, viewRotation;

	strcpy(gScenarioSaveName, path_get_filename(path));
	path_remove_extension(gScenarioSaveName);

	if (flags & 2)
		log_verbose("saving scenario, %s", path);
	else
		log_verbose("saving game, %s", path);


	if (!(flags & 0x80000000))
		window_close_construction_windows();

	map_reorganise_elements();
	RCT2_CALLPROC_EBPSAFE(0x0069EBE4);
	RCT2_CALLPROC_EBPSAFE(0x0069EBA4);
	RCT2_CALLPROC_EBPSAFE(0x00677552);
	RCT2_CALLPROC_EBPSAFE(0x00674BCF);

	// Set saved view
	w = window_get_main();
	if (w != NULL) {
		viewport = w->viewport;

		viewX = viewport->view_width / 2 + viewport->view_x;
		viewY = viewport->view_height / 2 + viewport->view_y;
		viewZoom = viewport->zoom;
		viewRotation = RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_ROTATION, uint8);
	} else {
		viewX = 0;
		viewY = 0;
		viewZoom = 0;
		viewRotation = 0;
	}

	RCT2_GLOBAL(RCT2_ADDRESS_SAVED_VIEW_X, uint16) = viewX;
	RCT2_GLOBAL(RCT2_ADDRESS_SAVED_VIEW_Y, uint16) = viewY;
	RCT2_GLOBAL(RCT2_ADDRESS_SAVED_VIEW_ZOOM_AND_ROTATION, uint16) = viewZoom | (viewRotation << 8);

	// 
	memset(s6Header, 0, sizeof(rct_s6_header));
	s6Header->type = flags & 2 ? S6_TYPE_SCENARIO : S6_TYPE_SAVEDGAME;
	s6Header->num_packed_objects = flags & 1 ? scenario_get_num_packed_objects_to_write() : 0;
	s6Header->version = S6_RCT2_VERSION;
	s6Header->magic_number = S6_MAGIC_NUMBER;

	file = fopen(path, "wb+");
	if (file == NULL) {
		log_error("Unable to write to %s", path);
		return 0;
	}

	buffer = malloc(0x600000);
	if (buffer == NULL) {
		log_error("Unable to allocate enough space for a write buffer.");
		fclose(file);
		return 0;
	}

	// Write header chunk
	chunkHeader.encoding = CHUNK_ENCODING_ROTATE;
	chunkHeader.length = sizeof(rct_s6_header);
	encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)s6Header, chunkHeader);
	fwrite(buffer, encodedLength, 1, file);

	// Write scenario info chunk
	if (flags & 2) {
		chunkHeader.encoding = CHUNK_ENCODING_ROTATE;
		chunkHeader.length = sizeof(rct_s6_info);
		encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)s6Info, chunkHeader);
		fwrite(buffer, encodedLength, 1, file);
	}

	// Write packed objects
	if (s6Header->num_packed_objects > 0) {
		if (!scenario_write_packed_objects(file)) {
			free(buffer);
			fclose(file);
			return 0;
		}
	}

	// Write available objects chunk
	scenario_write_available_objects(file);

	// Write date etc. chunk
	chunkHeader.encoding = CHUNK_ENCODING_RLECOMPRESSED;
	chunkHeader.length = 16;
	encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)0x00F663A8, chunkHeader);
	fwrite(buffer, encodedLength, 1, file);

	// Write map elements
	chunkHeader.encoding = CHUNK_ENCODING_RLECOMPRESSED;
	chunkHeader.length = 0x4A85EC;
	encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)0x00F663B8, chunkHeader);
	fwrite(buffer, encodedLength, 1, file);

	if (flags & 2) {
		// Write chunk
		chunkHeader.encoding = CHUNK_ENCODING_RLECOMPRESSED;
		chunkHeader.length = 0x27104C;
		encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)0x010E63B8, chunkHeader);
		fwrite(buffer, encodedLength, 1, file);

		// Write chunk
		chunkHeader.encoding = CHUNK_ENCODING_RLECOMPRESSED;
		chunkHeader.length = 4;
		encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)0x01357844, chunkHeader);
		fwrite(buffer, encodedLength, 1, file);

		// Write chunk
		chunkHeader.encoding = CHUNK_ENCODING_RLECOMPRESSED;
		chunkHeader.length = 8;
		encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)0x01357BC8, chunkHeader);
		fwrite(buffer, encodedLength, 1, file);

		// Write chunk
		chunkHeader.encoding = CHUNK_ENCODING_RLECOMPRESSED;
		chunkHeader.length = 2;
		encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)0x01357CB0, chunkHeader);
		fwrite(buffer, encodedLength, 1, file);

		// Write chunk
		chunkHeader.encoding = CHUNK_ENCODING_RLECOMPRESSED;
		chunkHeader.length = 1082;
		encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)0x01357CF2, chunkHeader);
		fwrite(buffer, encodedLength, 1, file);

		// Write chunk
		chunkHeader.encoding = CHUNK_ENCODING_RLECOMPRESSED;
		chunkHeader.length = 16;
		encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)0x0135832C, chunkHeader);
		fwrite(buffer, encodedLength, 1, file);

		// Write chunk
		chunkHeader.encoding = CHUNK_ENCODING_RLECOMPRESSED;
		chunkHeader.length = 4;
		encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)0x0135853C, chunkHeader);
		fwrite(buffer, encodedLength, 1, file);

		// Write chunk
		chunkHeader.encoding = CHUNK_ENCODING_RLECOMPRESSED;
		chunkHeader.length = 0x761E8;
		encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)0x01358740, chunkHeader);
		fwrite(buffer, encodedLength, 1, file);
	} else {
		// Write chunk
		chunkHeader.encoding = CHUNK_ENCODING_RLECOMPRESSED;
		chunkHeader.length = 0x2E8570;
		encodedLength = sawyercoding_write_chunk_buffer(buffer, (uint8*)0x010E63B8, chunkHeader);
		fwrite(buffer, encodedLength, 1, file);
	}

	free(buffer);

	// Determine number of bytes written
	fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Read all written bytes back into a single buffer
	buffer = malloc(fileSize);
	fread(buffer, fileSize, 1, file);
	checksum = sawyercoding_calculate_checksum(buffer, fileSize);
	free(buffer);

	// Append the checksum
	fseek(file, fileSize, SEEK_SET);
	fwrite(&checksum, sizeof(uint32), 1, file);
	fclose(file);

	if (!(flags & 0x80000000))
		reset_loaded_objects();

	gfx_invalidate_screen();
	RCT2_GLOBAL(0x009DEA66, uint16) = 0;
	return 1;
}
