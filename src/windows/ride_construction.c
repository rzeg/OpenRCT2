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

#include "../addresses.h"
#include "../drawing/drawing.h"
#include "../game.h"
#include "../input.h"
#include "../interface/viewport.h"
#include "../interface/widget.h"
#include "../interface/window.h"
#include "../localisation/localisation.h"
#include "../ride/track.h"

/* move to ride.c */
void sub_6B2FA9(rct_windownumber number)
{
	rct_window* w;

	w = window_find_by_number(WC_RIDE, number);
	if (w != NULL && w->page == 1)
		window_close(w);
}

enum {
	WIDX_BACKGROUND,
	WIDX_TITLE,
	WIDX_CLOSE,
	WIDX_DIRECTION_GROUPBOX,
	WIDX_SLOPE_GROUPBOX,
	WIDX_BANKING_GROUPBOX,
	WIDX_LEFT_CURVE_VERY_SMALL,
	WIDX_LEFT_CURVE_SMALL,
	WIDX_LEFT_CURVE,
	WIDX_STRAIGHT,
	WIDX_RIGHT_CURVE,
	WIDX_RIGHT_CURVE_SMALL,
	WIDX_RIGHT_CURVE_VERY_SMALL,
	WIDX_SPECIAL_TRACK_DROPDOWN,
	WIDX_SLOPE_DOWN_STEEP,
	WIDX_SLOPE_DOWN,
	WIDX_LEVEL,
	WIDX_SLOPE_UP,
	WIDX_SLOPE_UP_STEEP,
	WIDX_CHAIN_LIFT,
	WIDX_BANK_LEFT,
	WIDX_BANK_STRAIGHT,
	WIDX_BANK_RIGHT,
	WIDX_CONSTRUCT,
	WIDX_DEMOLISH,
	WIDX_LEFT_CURVE_LARGE,
	WIDX_PREVIOUS_SECTION,
	WIDX_NEXT_SECTION,
	WIDX_ENTRANCE_EXIT_GROUPBOX,
	WIDX_ENTRANCE,
	WIDX_EXIT,
	WIDX_RIGHT_CURVE_LARGE,
	WIDX_ROTATE,
	WIDX_U_TRACK,
	WIDX_O_TRACK,
	WIDX_SEAT_ROTATION_GROUPBOX,
	WIDX_SEAT_ROTATION_ANGLE_SPINNER,
	WIDX_SEAT_ROTATION_ANGLE_SPINNER_UP,
	WIDX_SEAT_ROTATION_ANGLE_SPINNER_DOWN,

	WIDX_MAZE_MODE_GROUPBOX = 3,
	WIDX_MAZE_BUILD_MODE = 6,
	WIDX_MAZE_MOVE_MODE,
	WIDX_MAZE_FILL_MODE,
	WIDX_MAZE_DIRECTION_GROUPBOX = 23,
	WIDX_MAZE_DIRECTION_NW,
	WIDX_MAZE_DIRECTION_NE,
	WIDX_MAZE_DIRECTION_SW,
	WIDX_MAZE_DIRECTION_SE,
};

#pragma region Widgets

static rct_widget window_ride_construction_widgets[] = {
	{ WWT_FRAME,			0,	0,		165,	0,		393,	0xFFFFFFFF,							STR_NONE												},
	{ WWT_CAPTION,			0,	1,		164,	1,		14,		896,								STR_WINDOW_TITLE_TIP									},
	{ WWT_CLOSEBOX,			0,	153,	163,	2,		13,		STR_CLOSE_X,						STR_CLOSE_WINDOW_TIP									},
	{ WWT_GROUPBOX,			0,	3,		162,	17,		73,		STR_RIDE_CONSTRUCTION_DIRECTION,	STR_NONE												},
	{ WWT_GROUPBOX,			0,	3,		162,	76,		116,	STR_RIDE_CONSTRUCTION_SLOPE,		STR_NONE												},
	{ WWT_GROUPBOX,			0,	3,		162,	120,	160,	STR_RIDE_CONSTRUCTION_ROLL_BANKING,	STR_NONE												},
	{ WWT_FLATBTN,			1,	6,		27,		29,		52,		0x1414,								STR_RIDE_CONSTRUCTION_LEFT_CURVE_VERY_SMALL_TIP			},
	{ WWT_FLATBTN,			1,	6,		27,		29,		52,		0x1414,								STR_RIDE_CONSTRUCTION_LEFT_CURVE_SMALL_TIP				},
	{ WWT_FLATBTN,			1,	28,		49,		29,		52,		0x1412,								STR_RIDE_CONSTRUCTION_LEFT_CURVE_TIP					},
	{ WWT_FLATBTN,			1,	72,		93,		29,		52,		0x1411,								STR_RIDE_CONSTRUCTION_STRAIGHT_TIP						},
	{ WWT_FLATBTN,			1,	116,	137,	29,		52,		0x1413,								STR_RIDE_CONSTRUCTION_RIGHT_CURVE_TIP					},
	{ WWT_FLATBTN,			1,	138,	159,	29,		52,		0x1415,								STR_RIDE_CONSTRUCTION_RIGHT_CURVE_SMALL_TIP				},
	{ WWT_FLATBTN,			1,	138,	159,	29,		52,		0x1415,								STR_RIDE_CONSTRUCTION_RIGHT_CURVE_VERY_SMALL_TIP		},
	{ WWT_DROPDOWN_BUTTON,	1,	17,		148,	55,		68,		1384,								STR_RIDE_CONSTRUCTION_OTHER_TRACK_CONFIGURATIONS_TIP	},
	{ WWT_FLATBTN,			1,	23,		46,		88,		111,	0x1418,								STR_RIDE_CONSTRUCTION_STEEP_SLOPE_DOWN_TIP				},
	{ WWT_FLATBTN,			1,	47,		70,		88,		111,	0x1419,								STR_RIDE_CONSTRUCTION_SLOPE_DOWN_TIP					},
	{ WWT_FLATBTN,			1,	71,		94,		88,		111,	0x141A,								STR_RIDE_CONSTRUCTION_LEVEL_TIP							},
	{ WWT_FLATBTN,			1,	95,		118,	88,		111,	0x141B,								STR_RIDE_CONSTRUCTION_SLOPE_UP_TIP						},
	{ WWT_FLATBTN,			1,	119,	142,	88,		111,	0x141C,								STR_RIDE_CONSTRUCTION_STEEP_SLOPE_UP_TIP				},
	{ WWT_FLATBTN,			1,	134,	157,	88,		111,	0x142B,								STR_RIDE_CONSTRUCTION_CHAIN_LIFT_TIP					},
	{ WWT_FLATBTN,			1,	47,		70,		132,	155,	0x1421,								STR_RIDE_CONSTRUCTION_ROLL_FOR_LEFT_CURVE_TIP			},
	{ WWT_FLATBTN,			1,	71,		94,		132,	155,	0x1422,								STR_RIDE_CONSTRUCTION_NO_ROLL_TIP						},
	{ WWT_FLATBTN,			1,	95,		118,	132,	155,	0x1423,								STR_RIDE_CONSTRUCTION_ROLL_FOR_RIGHT_CURVE_TIP			},
	{ WWT_IMGBTN,			1,	3,		162,	164,	333,	0xFFFFFFFF,							STR_RIDE_CONSTRUCTION_CONSTRUCT_SELECTED_SECTION_TIP	},
	{ WWT_FLATBTN,			1,	60,		105,	338,	361,	0x142A,								STR_RIDE_CONSTRUCTION_REMOVE_HIGHLIGHTED_SECTION_TIP	},
	{ WWT_FLATBTN,			1,	50,		71,		29,		52,		0x1416,								STR_RIDE_CONSTRUCTION_LEFT_CURVE_LARGE_TIP				},
	{ WWT_FLATBTN,			1,	20,		43,		338,	361,	0x1428,								STR_RIDE_CONSTRUCTION_MOVE_TO_PREVIOUS_SECTION_TIP		},
	{ WWT_FLATBTN,			1,	122,	145,	338,	361,	0x1429,								STR_RIDE_CONSTRUCTION_MOVE_TO_NEXT_SECTION_TIP			},
	{ WWT_GROUPBOX,			0,	3,		162,	362,	389,	0xFFFFFFFF,							STR_NONE												},
	{ WWT_DROPDOWN_BUTTON,	1,	9,		78,		372,	383,	STR_RIDE_CONSTRUCTION_ENTRANCE,		STR_RIDE_CONSTRUCTION_ENTRANCE_TIP						},
	{ WWT_DROPDOWN_BUTTON,	1,	87,		156,	372,	383,	STR_RIDE_CONSTRUCTION_EXIT,			STR_RIDE_CONSTRUCTION_EXIT_TIP							},
	{ WWT_FLATBTN,			1,	94,		115,	29,		52,		0x1417,								STR_RIDE_CONSTRUCTION_RIGHT_CURVE_LARGE_TIP				},
	{ WWT_FLATBTN,			1,	72,		95,		338,	361,	0x1431,								STR_ROTATE_90_TIP										},
	{ WWT_FLATBTN,			1,	19,		42,		132,	155,	0x1424,								STR_RIDE_CONSTRUCTION_U_SHAPED_OPEN_TRACK_TIP			},
	{ WWT_FLATBTN,			1,	123,	146,	132,	155,	0x1425,								STR_RIDE_CONSTRUCTION_O_SHAPED_ENCLOSED_TRACK_TIP		},
	{ WWT_GROUPBOX,			0,	96,		162,	120,	160,	STR_RIDE_CONSTRUCTION_SEAT_ROT,		STR_NONE												},
	{ WWT_SPINNER,			1,	101,	158,	138,	149,	0,									STR_RIDE_CONSTRUCTION_SELECT_SEAT_ROTATION_ANGLE_TIP	},
	{ WWT_DROPDOWN_BUTTON,	1,	147,	157,	139,	143,	STR_NUMERIC_UP,						STR_RIDE_CONSTRUCTION_SELECT_SEAT_ROTATION_ANGLE_TIP	},
	{ WWT_DROPDOWN_BUTTON,	1,	147,	157,	144,	148,	STR_NUMERIC_DOWN,					STR_RIDE_CONSTRUCTION_SELECT_SEAT_ROTATION_ANGLE_TIP	},
	{ WIDGETS_END }
};

static rct_widget window_ride_construction_maze_widgets[] = {
	{ WWT_FRAME,			0,	0,		165,	0,		199,	0xFFFFFFFF,							STR_NONE												},
	{ WWT_CAPTION,			0,	1,		164,	1,		14,		896,								STR_WINDOW_TITLE_TIP									},
	{ WWT_CLOSEBOX,			0,	153,	163,	2,		13,		STR_CLOSE_X,						STR_CLOSE_WINDOW_TIP									},
	{ WWT_GROUPBOX,			0,	3,		162,	17,		71,		STR_RIDE_CONSTRUCTION_MODE,			STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_FLATBTN,			1,	35,		66,		29,		60,		0x15C8,								STR_RIDE_CONSTRUCTION_BUILD_MODE						},
	{ WWT_FLATBTN,			1,	67,		98,		29,		60,		0x15C9,								STR_RIDE_CONSTRUCTION_MOVE_MODE							},
	{ WWT_FLATBTN,			1,	99,		130,	29,		60,		0x15CA,								STR_RIDE_CONSTRUCTION_FILL_IN_MODE						},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_GROUPBOX,			0,	3,		162,	80,		166,	STR_RIDE_CONSTRUCTION_BUILD,		STR_NONE												},
	{ WWT_FLATBTN,			1,	83,		127,	96,		124,	0x1603,								STR_RIDE_CONSTRUCTION_BUILD_MAZE_IN_THIS_DIRECTION_TIP	},
	{ WWT_FLATBTN,			1,	83,		127,	125,	153,	0x1604,								STR_RIDE_CONSTRUCTION_BUILD_MAZE_IN_THIS_DIRECTION_TIP	},
	{ WWT_FLATBTN,			1,	38,		82,		125,	153,	0x1605,								STR_RIDE_CONSTRUCTION_BUILD_MAZE_IN_THIS_DIRECTION_TIP	},
	{ WWT_FLATBTN,			1,	38,		82,		96,		124,	0x1606,								STR_RIDE_CONSTRUCTION_BUILD_MAZE_IN_THIS_DIRECTION_TIP	},
	{ WWT_GROUPBOX,			0,	3,		162,	168,	195,	0xFFFFFFFF,							STR_NONE												},
	{ WWT_DROPDOWN_BUTTON,	1,	9,		78,		178,	189,	STR_RIDE_CONSTRUCTION_ENTRANCE,		STR_RIDE_CONSTRUCTION_ENTRANCE_TIP						},
	{ WWT_DROPDOWN_BUTTON,	1,	87,		156,	178,	189,	STR_RIDE_CONSTRUCTION_EXIT,			STR_RIDE_CONSTRUCTION_EXIT_TIP							},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WWT_EMPTY,			0,	0,		0,		0,		0,		0xFFFFFFFF,							STR_NONE												},
	{ WIDGETS_END }
};

#pragma endregion

#pragma region Events

static void window_ride_construction_emptysub() {}

static void window_ride_construction_close();
static void window_ride_construction_mouseup();
static void window_ride_construction_update(rct_window *w);
static void window_ride_construction_invalidate();
static void window_ride_construction_paint();

static void window_ride_construction_maze_close();
static void window_ride_construction_maze_invalidate();
static void window_ride_construction_maze_paint();

// 0x993F6C
static void* window_ride_construction_maze_events[] = {
	window_ride_construction_maze_close,
	(void*)0x006CD461,
	(void*)0x006CD623,
	(void*)0x006CD48C,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	(void*)0x006CD767,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	(void*)0x006CD63E,
	(void*)0x006CD65D,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_maze_invalidate,
	window_ride_construction_maze_paint,
	window_ride_construction_emptysub
};

//0x993EEC
static void* window_ride_construction_events[] = {
	window_ride_construction_close,
	window_ride_construction_mouseup,
	(void*)0x006C7934,
	(void*)0x006C6E6A,
	(void*)0x006C78CD,
	window_ride_construction_emptysub,
	window_ride_construction_update,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	(void*)0x006C8229,
	(void*)0x006C8248,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_emptysub,
	window_ride_construction_invalidate,
	window_ride_construction_paint,
	window_ride_construction_emptysub
};

#pragma endregion

static void window_ride_construction_mouseup_demolish(rct_window* w);
static void window_ride_construction_draw_track_piece(
	rct_window *w, rct_drawpixelinfo *dpi,
	int rideIndex, int trackType, int trackRotation, int unknown
);
static void sub_6C6A77();
void sub_6C94D8();
static bool sub_6CA2DF(int *trackType, int *trackDirection, int *rideIndex, int *edxRS16);
static void sub_6CBCE2(
	int rideIndex, int trackType, int trackDirection, int edx,
	int originX, int originY, int originZ
);
static void window_ride_construction_update_possible_ride_configurations();

#define _enabledRidePieces							RCT2_GLOBAL(0x00F44048, uint64)

#define _currentTrackPrice							RCT2_GLOBAL(0x00F44070, money32)

#define _numCurrentPossibleRideConfigurations		RCT2_GLOBAL(0x00F44078, uint8)

#define _previousTrackPieceSlope					RCT2_GLOBAL(0x00F440A0, uint16)

#define _currentRideIndex							RCT2_GLOBAL(0x00F440A7, uint8)
#define _currentTrackPieceX							RCT2_GLOBAL(0x00F440A8, uint16)
#define _currentTrackPieceY							RCT2_GLOBAL(0x00F440AA, uint16)
#define _currentTrackPieceZ							RCT2_GLOBAL(0x00F440AC, uint16)
#define _currentTrackPieceDirection					RCT2_GLOBAL(0x00F440AE, uint8)
#define _currentTrackPieceType						RCT2_GLOBAL(0x00F440AF, uint8)

#define _previousTrackBankStart						RCT2_GLOBAL(0x00F440B3, uint8)

#define _previousTrackBankEnd						RCT2_GLOBAL(0x00F440B6, uint8)
#define _previousTrackSlopeEnd						RCT2_GLOBAL(0x00F440B7, uint8)

#define _currentSeatRotationAngle					RCT2_GLOBAL(0x00F440CF, uint8)

uint8 *_currentPossibleRideConfigurations = (uint8*)0x00F4407C;

/**
 *
 * rct2: 0x006CB481
 */
rct_window *window_ride_construction_open()
{
	int rideIndex = _currentRideIndex;
	sub_6B2FA9(rideIndex);

	rct_window *w;
	rct_ride* ride = GET_RIDE(rideIndex);
	if (ride->type == RIDE_TYPE_MAZE) {
		w = window_create(0, 29, 166, 200, (uint32*)window_ride_construction_maze_events, WC_RIDE_CONSTRUCTION, WF_9);

		w->widgets = window_ride_construction_maze_widgets;
		w->enabled_widgets = 0x6F0001C4;

		window_init_scroll_widgets(w);

		w->colours[0] = 24;
		w->colours[1] = 24;
		w->colours[2] = 24;

		w->number = rideIndex;

		window_push_others_right(w);
		show_gridlines();
		return w;
	}

	w = window_create(0, 29, 166, 394, (uint32*)window_ride_construction_events, WC_RIDE_CONSTRUCTION, WF_9);

	w->widgets = window_ride_construction_widgets;
	w->enabled_widgets = 0x67EFFFFFC4;

	window_init_scroll_widgets(w);

	w->colours[0] = 24;
	w->colours[1] = 24;
	w->colours[2] = 24;

	w->number = rideIndex;

	window_push_others_right(w);
	show_gridlines();

	_currentTrackPrice = MONEY32_UNDEFINED;
	RCT2_GLOBAL(0x00F440CD, uint8) = 8;
	RCT2_GLOBAL(0x00F440CE, uint8) = 18;
	RCT2_GLOBAL(0x00F440CF, uint8) = 4;

	if (ride->type == RIDE_TYPE_REVERSE_FREEFALL_COASTER)
		RCT2_GLOBAL(0x00F440CE, uint8) = 30;

	if (ride->type == RIDE_TYPE_AIR_POWERED_VERTICAL_COASTER)
		RCT2_GLOBAL(0x00F440CE, uint8) = 30;

	_previousTrackPieceSlope = RCT2_ADDRESS(0x0097CC68, uint8)[ride->type * 2] | 0x100;
	RCT2_GLOBAL(0x00F440B2, uint8) = 0;
	RCT2_GLOBAL(0x00F440B3, uint8) = 0;
	RCT2_GLOBAL(0x00F440B4, uint8) = 0;
	RCT2_GLOBAL(0x00F440B5, uint8) = 0;

	if (RCT2_ADDRESS(0x0097D4F2, uint16)[ride->type * 4] & 0x8000)
		RCT2_GLOBAL(0x00F440B5, uint8) |= 2;

	RCT2_GLOBAL(0x00F440B6, uint8) = 0;
	RCT2_GLOBAL(0x00F440B7, uint8) = 0;

	RCT2_GLOBAL(RCT2_ADDRESS_TRACK_PREVIEW_ROTATION, uint8) = 0;
	RCT2_GLOBAL(0x00F440A6, uint8) = 4;
	RCT2_GLOBAL(0x00F440B0, uint8) = 0;
	RCT2_GLOBAL(0x00F440B1, uint8) = 0;
	RCT2_GLOBAL(0x00F44159, uint8) = 0;
	RCT2_GLOBAL(0x00F4415C, uint8) = 0;
	return w;
}

/**
 *
 * rct2: 0x006C845D
 */
static void window_ride_construction_close()
{
	rct_window *w;
	rct_xy_element mapElement;

	window_get_register(w);

	sub_6C9627();
	viewport_set_visibility(0);

	map_invalidate_map_selection_tiles();
	RCT2_GLOBAL(0x009DE58A, uint16) &= 0xFFFD;

	hide_gridlines();

	uint8 rideIndex = _currentRideIndex;
	if (sub_6CAF80(rideIndex, &mapElement)) {
		window_ride_main_open(rideIndex);
	} else {
		int eax = RCT2_GLOBAL(RCT2_ADDRESS_GAME_PAUSED, uint8);

		RCT2_GLOBAL(RCT2_ADDRESS_GAME_PAUSED, uint8) = 0;
		game_do_command(0, 9, 0, rideIndex, GAME_COMMAND_7, 0, 0);

		RCT2_GLOBAL(RCT2_ADDRESS_GAME_PAUSED, uint8) = eax;
	}
}

/**
 *
 * rct2: 0x006CD811
 */
static void window_ride_construction_maze_close()
{
	rct_window *w;

	window_get_register(w);

	sub_6C9627();
	viewport_set_visibility(0);

	map_invalidate_map_selection_tiles();
	RCT2_GLOBAL(0x009DE58A, uint16) &= 0xFFFD;

	hide_gridlines();

	uint8 rideIndex = _currentRideIndex;
	rct_ride* ride = GET_RIDE(rideIndex);
	if (ride->overall_view == 0xFFFF) {
		int savedPausedState = RCT2_GLOBAL(RCT2_ADDRESS_GAME_PAUSED, uint8);
		RCT2_GLOBAL(RCT2_ADDRESS_GAME_PAUSED, uint8) = 0;
		game_do_command(0, 9, 0, rideIndex, GAME_COMMAND_7, 0, 0);
		RCT2_GLOBAL(RCT2_ADDRESS_GAME_PAUSED, uint8) = savedPausedState;
	} else {
		window_ride_main_open(rideIndex);
	}
}

/**
 *
 * rct2: 0x006C6E14
 */
static void window_ride_construction_mouseup()
{
	short widgetIndex;
	rct_window *w;

	window_widget_get_registers(w, widgetIndex);

	sub_6C6A77();
	switch (widgetIndex){
	case WIDX_CLOSE:
		window_close(w);
		break;
	case 27:
		RCT2_CALLPROC_X(0x006C9296, 0, 0, 0, widgetIndex, (int)w, 0, 0);
		break;
	case 26:
		RCT2_CALLPROC_X(0x006C93B8, 0, 0, 0, widgetIndex, (int)w, 0, 0);
		break;
	case 23:
		RCT2_CALLPROC_X(0x006C9F72, 0, 0, 0, widgetIndex, (int)w, 0, 0);
		break;
	case WIDX_DEMOLISH:
		window_ride_construction_mouseup_demolish(w);
		break;
	case 32:
		RCT2_CALLPROC_X(0x006C78AA, 0, 0, 0, widgetIndex, (int)w, 0, 0);
		break;
	case 29:
		RCT2_CALLPROC_X(0x006C7802, 0, 0, 0, widgetIndex, (int)w, 0, 0);
		break;
	case 30:
		RCT2_CALLPROC_X(0x006C7866, 0, 0, 0, widgetIndex, (int)w, 0, 0);
		break;
	}
}

/**
 *
 * rct2: 0x006C9BA5
 */
static void window_ride_construction_mouseup_demolish(rct_window* w)
{
	RCT2_CALLPROC_X(0x006C9BA5, 0, 0, 0, 0, (int)w, 0, 0);
	return;

	_currentTrackPrice = MONEY32_UNDEFINED;
	sub_6C9627();

	RCT2_GLOBAL(0x00F440B8, uint8) = 3;
	if (RCT2_GLOBAL(0x00F440A6, uint8) == 1) {
		//6C9C4F
	}

	if (RCT2_GLOBAL(0x00F440A6, uint8) != 2) {
		//6c9cc4
		int eax = _currentTrackPieceX,
			ebx = _currentTrackPieceType || (_currentTrackPieceDirection << 8),
			ecx = _currentTrackPieceY,
			edx = _currentTrackPieceZ;

		sub_6C683D(&eax, &ecx, edx, RCT2_GLOBAL(RCT2_ADDRESS_TRACK_PREVIEW_ROTATION, uint8), RCT2_GLOBAL(0xF440AF, uint8) & 0x3FF, 0, 0, 0);
	}

	RCT2_GLOBAL(0x00F441D2, uint8) = _currentRideIndex;
	//6c9BFE
}

/**
 *
 * rct2: 0x006C8374
 */
static void window_ride_construction_update(rct_window *w)
{
	switch (_previousTrackPieceSlope) {
	case 429:
	case 376:
	case 369:
	case 368:
		widget_invalidate(w, WIDX_CONSTRUCT);
		break;
	}

	if (RCT2_GLOBAL(0x00F440A6, uint8) == 4) {
		if (!widget_is_active_tool(w, WIDX_ENTRANCE) && !widget_is_active_tool(w, WIDX_EXIT)) {
			RCT2_GLOBAL(0x00F440A6, uint8) = RCT2_GLOBAL(0x00F440CC, uint8);
			sub_6C84CE();
		}
	}
	switch (RCT2_GLOBAL(0x00F440A6, uint8)) {
	case 1:
	case 2:
	case 3:
		if (
			(RCT2_GLOBAL(RCT2_ADDRESS_INPUT_FLAGS, uint32) & INPUT_FLAG_TOOL_ACTIVE) &&
			RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WINDOWCLASS, rct_windowclass) == WC_RIDE_CONSTRUCTION
		) {
			tool_cancel();
		}
		break;
	}

	sub_6C94D8();
}

/**
 *
 * rct2: 0x006CD435
 */
static void window_ride_construction_maze_invalidate()
{
	rct_ride *ride = GET_RIDE(_currentRideIndex);

	// Set window title arguments
	RCT2_GLOBAL(0x013CE956, uint16_t) = ride->name;
	RCT2_GLOBAL(0x013CE958, uint32_t) = ride->name_arguments;
}

/**
 *
 * rct2: 0x006C6AD5
 */
static void window_ride_construction_invalidate()
{
	rct_window *w;
	rct_ride *ride;
	rct_string_id stringId;

	window_get_register(w);

	ride = GET_RIDE(_currentRideIndex);

	stringId = STR_RIDE_CONSTRUCTION_SPECIAL;
	if (_previousTrackPieceSlope >= 256) {
		stringId = RCT2_ADDRESS(0x00999492, rct_string_id)[_previousTrackPieceSlope - 256];
		if (stringId == STR_RAPIDS && ride->type == RIDE_TYPE_CAR_RIDE)
			stringId = STR_LOG_BUMPS;
	}
	RCT2_GLOBAL(0x013CE952, uint16) = stringId;

	if (RCT2_GLOBAL(0x00F440D3, uint8) == 1)
		RCT2_GLOBAL(0x013CE952 + 2, uint16) = ((RCT2_GLOBAL(0x00F440CD, uint8) * 9) >> 2) & 0xFFFF;

	window_ride_construction_widgets[WIDX_SEAT_ROTATION_ANGLE_SPINNER].image =
		STR_RIDE_CONSTRUCTION_SEAT_ROTATION_ANGLE_NEG_180 + RCT2_GLOBAL(0x00F440CF, uint8);

	if (RCT2_GLOBAL(0x00F440D3, uint8) == 2)
		RCT2_GLOBAL(0x013CE952 + 2, uint16) = ((RCT2_GLOBAL(0x00F440CE, uint8) * 9) >> 2) & 0xFFFF;

	// Set window title arguments
	RCT2_GLOBAL(0x013CE952 + 4, uint16) = ride->name;
	RCT2_GLOBAL(0x013CE952 + 6, uint32) = ride->name_arguments;
}

/**
 *
 * rct2: 0x006C6B86
 */
static void window_ride_construction_paint()
{
	rct_window *w;
	rct_drawpixelinfo *dpi, *clipdpi;
	rct_widget *widget;
	int x, y;

	window_paint_get_registers(w, dpi);

	window_draw_widgets(w, dpi);
	
	widget = &window_ride_construction_widgets[WIDX_CONSTRUCT];
	if (widget->type == WWT_EMPTY)
		return;

	int trackType, trackDirection, rideIndex, edxRS16;
	if (sub_6CA2DF(&trackType, &trackDirection, &rideIndex, &edxRS16))
		return;

	// Draw track piece
	clipdpi = clip_drawpixelinfo(
		dpi,
		w->x + widget->left + 1,
		widget->right - widget->left - 1,
		w->y + widget->top + 1,
		widget->bottom - widget->top - 1
	);
	if (clipdpi != NULL) {
		window_ride_construction_draw_track_piece(w, clipdpi, rideIndex, trackType, trackDirection, edxRS16);
		rct2_free(clipdpi);
	}

	// Draw cost
	x = w->x + (widget->left + widget->right) / 2;
	y = w->y + widget->bottom - 23;
	if (RCT2_GLOBAL(0x00F440A6, uint8) != 4)
		gfx_draw_string_centred(dpi, 1407, x, y, 0, w);

	y += 11;
	if (
		_currentTrackPrice != MONEY32_UNDEFINED &&
		!(RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_NO_MONEY)
	) {
		gfx_draw_string_centred(dpi, 1408, x, y, 0, (void*)&_currentTrackPrice);
	}
}

/**
 *
 * rct2: 0x006CD45B
 */
static void window_ride_construction_maze_paint()
{
	rct_window *w;
	rct_drawpixelinfo *dpi;

	window_paint_get_registers(w, dpi);
	
	window_draw_widgets(w, dpi);
}

/**
 *
 * rct2: 0x0068B2B7
 */
static void sub_68B2B7(int x, int y)
{
	RCT2_CALLPROC_X(0x0068B2B7, x, 0, y, 0, 0, 0, 0);
}

static void window_ride_construction_draw_track_piece(
	rct_window *w, rct_drawpixelinfo *dpi,
	int rideIndex, int trackType, int trackDirection, int unknown
) {
	rct_preview_track *trackBlock;
	rct_ride *ride;

	ride = GET_RIDE(rideIndex);

	trackBlock = ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_SELLS_FOOD) ?
		RCT2_ADDRESS(0x00994A38, rct_preview_track*)[trackType] :
		RCT2_ADDRESS(0x00994638, rct_preview_track*)[trackType];

	while ((trackBlock + 1)->var_00 != 0xFF)
		trackBlock++;

	short x = trackBlock->x;
	short z = trackBlock->z;
	short y = trackBlock->y;
	if (trackBlock->var_09 & 2) {
		x = 0;
		y = 0;
	}

	short tmp;
	switch (trackDirection & 3) {
	case 1:
		tmp = x;
		x = y;
		y = -tmp;
		break;
	case 2:
		x = -x;
		y = -y;
		break;
	case 3:
		tmp = x;
		x = -y;
		y = tmp;
		break;
	case 0:
		break;
	}
	//this is actually case 0, but the other cases all jump to it
	x = 4112 + (x / 2);
	y = 4112 + (y / 2);
	z = 1024 + z;

	short bx = ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_SELLS_FOOD) ?
		RCT2_GLOBAL(0x009984A2 + trackType * 8, uint8) :
		RCT2_GLOBAL(0x00997CA2 + trackType * 8, uint8);

	z -= bx;
	int start_x = x;
	switch (RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_ROTATION, uint32_t)) {
	case 0:
		x = y - x;
		y = (y + start_x) / 2 - z;
		break;
	case 1:
		x = -x - y;
		y = (y - start_x) / 2 - z;
		break;
	case 2:
		x -= y;
		y = (-y - start_x) / 2 - z;
		break;
	case 3:
		x += y;
		y = (-y + start_x) / 2 - z;
		break;
	}
	dpi->x += x - dpi->width / 2;
	dpi->y += y - dpi->height / 2 - 16;
	RCT2_GLOBAL(0x0140E9A8, rct_drawpixelinfo*) = dpi;
	uint32_t d = unknown << 16;
	d |= rideIndex;
	d |= trackType << 8;

	sub_6CBCE2(rideIndex, trackType, trackDirection, d, 4096, 4096, 1024);

	/*
	RCT2_CALLPROC_X(
		0x006CBCE2,
		4096,
		(((uint16_t)bx) & 0xFF) | (RCT2_GLOBAL(0xF44134, uint8_t) << 8),
		4096,
		d,
		dpi->width,
		1024,
		dpi->height
	);
	*/
}

static rct_map_element _tempTrackMapElement;
static rct_map_element _tempSideTrackMapElement = { 0x80, 0x8F, 128, 128, 0, 0, 0, 0 };
static rct_map_element *_backupMapElementArrays[5];

/**
 *
 * rct2: 0x006CBCE2
 * bh: trackDirection
 * dl: rideIndex
 * dh: trackType
 */
static void sub_6CBCE2(
	int rideIndex, int trackType, int trackDirection, int edx,
	int originX, int originY, int originZ
) {
	// RCT2_CALLPROC_X(
	// 	0x006CBCE2,
	// 	originX,
	// 	(trackDirection & 0xFF) | (RCT2_GLOBAL(0x00F44134, uint8) << 8),
	// 	originY,
	// 	(edx & 0xFFFF0000) | (trackType << 8) | rideIndex,
	// 	0,
	// 	originZ,
	// 	0
	// ); return;

	rct_ride *ride;
	rct_preview_track *trackBlock;
	int preserve_word_141E9E4;
	int x, y, baseZ, clearanceZ, offsetX, offsetY;
	uint64 preserve_map_size_vars;

	preserve_word_141E9E4 = RCT2_GLOBAL(0x0141E9E4, uint16);
	RCT2_GLOBAL(0x0141E9E4, uint16) = 0;
	trackDirection &= 3;

	RCT2_GLOBAL(0x00EE7880, uint32) = 0x00F1A4CC;
	sub_0x68615B(0x00EE788C);

	ride = GET_RIDE(rideIndex);

	preserve_map_size_vars = RCT2_GLOBAL(RCT2_ADDRESS_MAP_SIZE_UNITS, uint64);

	RCT2_GLOBAL(RCT2_ADDRESS_MAP_SIZE_UNITS, uint16) = 255 * 32;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_MAXIMUM_X_Y, uint16) = (255 * 32) + 286;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_SIZE, uint16) = 256;
	RCT2_GLOBAL(0x01358836, uint16) = (256 * 32) - 1;

	trackBlock = ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_SELLS_FOOD) ?
		RCT2_ADDRESS(0x00994A38, rct_preview_track*)[trackType] :
		RCT2_ADDRESS(0x00994638, rct_preview_track*)[trackType];

	while (trackBlock->var_00 != 255) {
		int bl = trackBlock->var_08;
		int bh;
		switch (trackDirection) {
		case 0:
			offsetX =  trackBlock->x;
			offsetY =  trackBlock->y;
			break;
		case 1:
			offsetX =  trackBlock->y;
			offsetY = -trackBlock->x;
			bl = rol8(bl, 1);
			bh = bl;
			bh = ror8(bh, 4);
			bl &= 0xEE;
			bh &= 0x11;
			bl |= bh;
			break;
		case 2:
			offsetX = -trackBlock->x;
			offsetY = -trackBlock->y;
			bl = rol8(bl, 2);
			bh = bl;
			bh = ror8(bh, 4);
			bl &= 0xCC;
			bh &= 0x33;
			bl |= bh;
			break;
		case 3:
			offsetX = -trackBlock->y;
			offsetY =  trackBlock->x;
			bl = rol8(bl, 3);
			bh |= bl;
			bh = ror8(bh, 4);
			bl &= 0x88;
			bh &= 0x77;
			bl |= bh;
			break;
		}
		x = originX + offsetX;
		y = originY + offsetY;
		baseZ = (originZ + trackBlock->z) >> 3;
		clearanceZ = ((trackBlock->var_07 + RCT2_GLOBAL(0x0097D219 + (ride->type * 8), uint8)) >> 3) + baseZ + 4;

		rct_map_element *hmmm = map_get_first_element_at(x >> 5, y >> 5);

		int tileX = x >> 5;
		int tileY = y >> 5;

		// Replace map elements with temporary ones containing track
		_backupMapElementArrays[0] = map_get_first_element_at(tileX + 0, tileY + 0);
		_backupMapElementArrays[1] = map_get_first_element_at(tileX + 1, tileY + 0);
		_backupMapElementArrays[2] = map_get_first_element_at(tileX - 1, tileY + 0);
		_backupMapElementArrays[3] = map_get_first_element_at(tileX + 0, tileY + 1);
		_backupMapElementArrays[4] = map_get_first_element_at(tileX + 0, tileY - 1);
		map_set_tile_elements(tileX + 0, tileY + 0, &_tempTrackMapElement);
		map_set_tile_elements(tileX + 1, tileY + 0, &_tempSideTrackMapElement);
		map_set_tile_elements(tileX - 1, tileY + 0, &_tempSideTrackMapElement);
		map_set_tile_elements(tileX + 0, tileY + 1, &_tempSideTrackMapElement);
		map_set_tile_elements(tileX + 0, tileY - 1, &_tempSideTrackMapElement);

		// Set the temporary track element
		_tempTrackMapElement.type = trackDirection | MAP_ELEMENT_TYPE_TRACK | (edx & 0x10000 ? 0x80 : 0);
		_tempTrackMapElement.flags = (bl & 0x0F) | MAP_ELEMENT_FLAG_LAST_TILE;
		_tempTrackMapElement.base_height = baseZ;
		_tempTrackMapElement.clearance_height = clearanceZ;
		_tempTrackMapElement.properties.track.type = trackType;
		_tempTrackMapElement.properties.track.sequence = trackBlock->var_00;
		_tempTrackMapElement.properties.track.colour = (edx & 0x20000 ? 4 : 0);
		_tempTrackMapElement.properties.track.ride_index = rideIndex;

		// Draw this map tile
		sub_68B2B7(x, y);

		// Restore map elements
		map_set_tile_elements(tileX + 0, tileY + 0, _backupMapElementArrays[0]);
		map_set_tile_elements(tileX + 1, tileY + 0, _backupMapElementArrays[1]);
		map_set_tile_elements(tileX - 1, tileY + 0, _backupMapElementArrays[2]);
		map_set_tile_elements(tileX + 0, tileY + 1, _backupMapElementArrays[3]);
		map_set_tile_elements(tileX + 0, tileY - 1, _backupMapElementArrays[4]);

		trackBlock++;
	}

	RCT2_GLOBAL(RCT2_ADDRESS_MAP_SIZE_UNITS, uint64) = preserve_map_size_vars;

	RCT2_CALLPROC_X(0x00688217, 0, 0, 0, 0, 0, 0, 0);
	sub_688485();

	RCT2_GLOBAL(0x0141E9E4, uint16) = preserve_word_141E9E4;
}

/**
 * 
 * rct2: 0x006C84CE
 */
void sub_6C84CE()
{
	// RCT2_CALLPROC_X(0x006C84CE, 0, 0, 0, 0, 0, 0, 0); return;

	rct_window *w;
	rct_preview_track *trackBlock;
	int trackType, trackDirection, originX, originY, offsetX, offsetY, x, y, z;

	sub_6C6A77();
	w = window_find_by_class(WC_RIDE_CONSTRUCTION);
	if (w == NULL)
		return;

	map_invalidate_map_selection_tiles();
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_FLAGS, uint16) |= 10;

	switch (RCT2_GLOBAL(0x00F440A6, uint8)) {
	case 0:
		trackDirection = _currentTrackPieceDirection;
		trackType = 0;
		originX = _currentTrackPieceX;
		originY = _currentTrackPieceY;
		break;
	case 3:
		trackDirection = _currentTrackPieceDirection;
		trackType = _currentTrackPieceType;
		originX = _currentTrackPieceX;
		originY = _currentTrackPieceY;
		break;
	default:
		if (sub_6CA2DF(&trackType, &trackDirection, NULL, NULL)) {
			trackDirection = _currentTrackPieceDirection;
			trackType = 0;
			originX = _currentTrackPieceX;
			originY = _currentTrackPieceY;
		}
		break;
	}

	uint8 rideIndex = _currentRideIndex;
	rct_ride *ride = GET_RIDE(rideIndex);

	trackBlock = ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_SELLS_FOOD) ?
		RCT2_ADDRESS(0x00994638, rct_preview_track*)[trackType] :
		RCT2_ADDRESS(0x00994A38, rct_preview_track*)[trackType];

	trackDirection &= 3;
	int selectionTileIndex = 0;
	while (trackBlock->var_00 != 255) {
		switch (trackDirection) {
		case 0:
			offsetX = trackBlock->x;
			offsetY = trackBlock->y;
			break;
		case 1:
			offsetX = trackBlock->y;
			offsetY = -trackBlock->x;
			break;
		case 2:
			offsetX = -trackBlock->x;
			offsetY = -trackBlock->y;
			break;
		case 3:
			offsetX = -trackBlock->y;
			offsetY = trackBlock->x;
			break;
		}
		gMapSelectionTiles[selectionTileIndex].x = originX + offsetX;
		gMapSelectionTiles[selectionTileIndex].y = originY + offsetY;
		selectionTileIndex++;
		trackBlock++;
	}
	gMapSelectionTiles[selectionTileIndex].x = 255;
	gMapSelectionTiles[selectionTileIndex].y = 255;
	map_invalidate_map_selection_tiles();

	w->hold_down_widgets = 0;
	RCT2_GLOBAL(0x00F440D0, uint8) = 255;
	if (RCT2_GLOBAL(0x00F440A6, uint8) == 3) {
		x = _currentTrackPieceX;
		y = _currentTrackPieceY;
		z = _currentTrackPieceZ;
		if (!sub_6C683D(&x, &y, z, _currentTrackPieceDirection & 3, _currentTrackPieceType, 0, 0, 0)) {
			int edi = RCT2_GLOBAL(0x00F441AC, uint32);
			int dl = RCT2_GLOBAL(edi + 4, uint8);
			RCT2_GLOBAL(0x00F440D0, uint8) = dl;
			if (dl == 99)
				RCT2_GLOBAL(0x00F440CD, uint8) = (RCT2_GLOBAL(edi + 5, uint8) >> 4) << 1;
			_currentSeatRotationAngle = RCT2_GLOBAL(edi + 6, uint8) >> 4;
		}
	}

	window_ride_construction_update_possible_ride_configurations();

	int edi;
	if (RCT2_GLOBAL(0x00F440B5, uint8) & 2)
		edi = RCT2_GLOBAL(0x0097D4F5 + (ride->type * 8), uint8);
	else
		edi = ride->type;

	if (ride_type_has_flag(edi, RIDE_TYPE_FLAG_IS_SHOP)) {
		window_ride_construction_widgets[WIDX_ENTRANCE_EXIT_GROUPBOX].type = WWT_EMPTY;
		window_ride_construction_widgets[WIDX_ENTRANCE].type = WWT_EMPTY;
		window_ride_construction_widgets[WIDX_EXIT].type = WWT_EMPTY;
	} else {
		window_ride_construction_widgets[WIDX_ENTRANCE_EXIT_GROUPBOX].type = WWT_GROUPBOX;
		window_ride_construction_widgets[WIDX_ENTRANCE].type = WWT_DROPDOWN_BUTTON;
		window_ride_construction_widgets[WIDX_EXIT].type = WWT_DROPDOWN_BUTTON;
	}
	
	if (RCT2_GLOBAL(0x00F4407A, uint16) == 0) {
		window_ride_construction_widgets[WIDX_SPECIAL_TRACK_DROPDOWN].type = WWT_EMPTY;
	} else {
		window_ride_construction_widgets[WIDX_SPECIAL_TRACK_DROPDOWN].type = WWT_DROPDOWN_BUTTON;
	}

	if (RCT2_GLOBAL(0x00F44048, uint32) & (1 << 1)) {
		window_ride_construction_widgets[WIDX_STRAIGHT].type = WWT_FLATBTN;
	} else {
		window_ride_construction_widgets[WIDX_STRAIGHT].type = WWT_EMPTY;
	}

	if (ride_type_has_flag(edi, RIDE_TYPE_FLAG_30)) {
		window_ride_construction_widgets[WIDX_LEFT_CURVE_LARGE].type = WWT_FLATBTN;
		window_ride_construction_widgets[WIDX_RIGHT_CURVE_LARGE].type = WWT_FLATBTN;
	} else {
		window_ride_construction_widgets[WIDX_LEFT_CURVE_LARGE].type = WWT_EMPTY;
		window_ride_construction_widgets[WIDX_RIGHT_CURVE_LARGE].type = WWT_EMPTY;
	}

	window_ride_construction_widgets[WIDX_LEFT_CURVE].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_RIGHT_CURVE].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_LEFT_CURVE_SMALL].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_RIGHT_CURVE_SMALL].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_LEFT_CURVE_VERY_SMALL].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_RIGHT_CURVE_VERY_SMALL].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_LEFT_CURVE_SMALL].left = 28;
	window_ride_construction_widgets[WIDX_LEFT_CURVE_SMALL].right = 49;
	window_ride_construction_widgets[WIDX_RIGHT_CURVE_SMALL].left = 116;
	window_ride_construction_widgets[WIDX_RIGHT_CURVE_SMALL].right = 137;
	window_ride_construction_widgets[WIDX_LEFT_CURVE_SMALL].image = 5138;
	window_ride_construction_widgets[WIDX_RIGHT_CURVE_SMALL].image = 5139;
	if (_enabledRidePieces & (1ULL << 40)) {
		window_ride_construction_widgets[WIDX_LEFT_CURVE_SMALL].type = WWT_FLATBTN;
		window_ride_construction_widgets[WIDX_LEFT_CURVE_SMALL].left = 6;
		window_ride_construction_widgets[WIDX_LEFT_CURVE_SMALL].right = 27;
		window_ride_construction_widgets[WIDX_LEFT_CURVE_SMALL].image = 5140;
		window_ride_construction_widgets[WIDX_RIGHT_CURVE_SMALL].type = WWT_FLATBTN;
		window_ride_construction_widgets[WIDX_RIGHT_CURVE_SMALL].left = 138;
		window_ride_construction_widgets[WIDX_RIGHT_CURVE_SMALL].right = 159;
		window_ride_construction_widgets[WIDX_RIGHT_CURVE_SMALL].image = 5141;
	}
	if (_enabledRidePieces & (1 << 16)) {
		window_ride_construction_widgets[WIDX_LEFT_CURVE].type = WWT_FLATBTN;
		window_ride_construction_widgets[WIDX_RIGHT_CURVE].type = WWT_FLATBTN;
		window_ride_construction_widgets[WIDX_LEFT_CURVE_SMALL].left = 6;
		window_ride_construction_widgets[WIDX_LEFT_CURVE_SMALL].right = 27;
		window_ride_construction_widgets[WIDX_LEFT_CURVE_SMALL].image = 5140;
		window_ride_construction_widgets[WIDX_RIGHT_CURVE_SMALL].left = 138;
		window_ride_construction_widgets[WIDX_RIGHT_CURVE_SMALL].right = 159;
		window_ride_construction_widgets[WIDX_RIGHT_CURVE_SMALL].image = 5141;
	}
	if (_enabledRidePieces & (1 << 15)) {
		window_ride_construction_widgets[WIDX_LEFT_CURVE_SMALL].type = WWT_FLATBTN;
		window_ride_construction_widgets[WIDX_RIGHT_CURVE_SMALL].type = WWT_FLATBTN;
	}
	if (_enabledRidePieces & (1 << 14)) {
		window_ride_construction_widgets[WIDX_LEFT_CURVE_VERY_SMALL].type = WWT_FLATBTN;
		window_ride_construction_widgets[WIDX_RIGHT_CURVE_VERY_SMALL].type = WWT_FLATBTN;
	}

	window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_SLOPE_DOWN].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_LEVEL].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_SLOPE_UP].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].image = 5144;
	window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].tooltip = STR_RIDE_CONSTRUCTION_STEEP_SLOPE_DOWN_TIP;
	window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].image = 5148;
	window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].tooltip = STR_RIDE_CONSTRUCTION_STEEP_SLOPE_UP_TIP;
	if (edi == RIDE_TYPE_REVERSE_FREEFALL_COASTER || edi == RIDE_TYPE_AIR_POWERED_VERTICAL_COASTER) {
		window_ride_construction_widgets[WIDX_LEVEL].type = WWT_FLATBTN;
		window_ride_construction_widgets[WIDX_SLOPE_UP].type = WWT_FLATBTN;
	}
	if (_enabledRidePieces & ((1 << 8) | (1 << 9))) {
		window_ride_construction_widgets[WIDX_LEVEL].type = WWT_FLATBTN;
	}
	if (_enabledRidePieces & (1 << 8)) {
		window_ride_construction_widgets[WIDX_SLOPE_DOWN].type = WWT_FLATBTN;
		window_ride_construction_widgets[WIDX_SLOPE_UP].type = WWT_FLATBTN;
	}
	if (
		(_enabledRidePieces & (1 << 21)) &&
		_previousTrackBankStart != TRACK_BANK_NONE &&
		RCT2_GLOBAL(0x00F440B2, uint8) != 0
	) {
		if (_previousTrackPieceSlope >= 1 && _previousTrackPieceSlope <= 4)
			goto loc_6C8AA3;
	}

	if (_enabledRidePieces & (1 << 9)) {
	loc_6C8AA3:
		window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].type = WWT_FLATBTN;
		if (edi != RIDE_TYPE_SPLASH_BOATS)
			window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].type = WWT_FLATBTN;
	}

	if ((_enabledRidePieces & (1 << 3)) && _previousTrackPieceSlope < 256) {
		window_ride_construction_widgets[WIDX_CHAIN_LIFT].type = WWT_FLATBTN;
		x = 9;
	} else {
		window_ride_construction_widgets[WIDX_CHAIN_LIFT].type = WWT_EMPTY;
		x = 23;
	}

	for (int i = WIDX_SLOPE_DOWN_STEEP; i <= WIDX_SLOPE_UP_STEEP; i++) {
		window_ride_construction_widgets[i].left = x;
		window_ride_construction_widgets[i].right = x + 23;
		x += 24;
	}

	window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].image = 5148;
	window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].tooltip = STR_RIDE_CONSTRUCTION_STEEP_SLOPE_UP_TIP;
	window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].image = 5144;
	window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].tooltip = STR_RIDE_CONSTRUCTION_STEEP_SLOPE_DOWN_TIP;
	if (_enabledRidePieces & (1 << 28)) {
		if (_previousTrackSlopeEnd == TRACK_UP_60 || _previousTrackSlopeEnd == TRACK_UP_90) {
			for (int i = WIDX_SLOPE_UP_STEEP; i > WIDX_SLOPE_DOWN_STEEP; i--) {
				window_ride_construction_widgets[i].left = window_ride_construction_widgets[i - 1].left;
				window_ride_construction_widgets[i].right = window_ride_construction_widgets[i - 1].right;
			}
			window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].left = 0;
			window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].right = 0;
			window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].image = 5149;
			window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].tooltip = STR_RIDE_CONSTRUCTION_VERTICAL_RISE_TIP;
		} else if (_previousTrackSlopeEnd == TRACK_DOWN_60 || _previousTrackSlopeEnd == TRACK_DOWN_90) {
			// TODO
		}
	}

	if (
		(_enabledRidePieces & (1 << 23)) &&
		RCT2_GLOBAL(0x00F440B2, uint8) == 0 &&
		_previousTrackBankStart == TRACK_BANK_NONE &&
		(_previousTrackPieceSlope == 1 || _previousTrackPieceSlope == 2)
	) {
		window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].image = 5151;
		window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].tooltip = STR_RIDE_CONSTRUCTION_HELIX_DOWN_TIP;
		window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].image = 5152;
		window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].tooltip = STR_RIDE_CONSTRUCTION_HELIX_UP_TIP;

		int tmp = window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].left;
		window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].left = window_ride_construction_widgets[WIDX_SLOPE_DOWN].left;
		window_ride_construction_widgets[WIDX_SLOPE_DOWN].left = tmp;

		tmp = window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].right;
		window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].right = window_ride_construction_widgets[WIDX_SLOPE_DOWN].right;
		window_ride_construction_widgets[WIDX_SLOPE_DOWN].right = tmp;

		tmp = window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].right;
		window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].right = window_ride_construction_widgets[WIDX_SLOPE_UP].right;
		window_ride_construction_widgets[WIDX_SLOPE_UP].right = tmp;
	}

	if (
		_enabledRidePieces & ((1 << 22) | (1 << 21)) &&
		(_previousTrackPieceSlope >= 1 && _previousTrackPieceSlope <= 4) &&
		RCT2_GLOBAL(0x00F440B2, uint8) == 0 &&
		_previousTrackBankStart != TRACK_BANK_NONE
	) {
		window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].image = 5151;
		window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].tooltip = STR_RIDE_CONSTRUCTION_HELIX_DOWN_TIP;
		window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].image = 5152;
		window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].tooltip = STR_RIDE_CONSTRUCTION_HELIX_UP_TIP;

		int tmp = window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].left;
		window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].left = window_ride_construction_widgets[WIDX_SLOPE_DOWN].left;
		window_ride_construction_widgets[WIDX_SLOPE_DOWN].left = tmp;

		tmp = window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].right;
		window_ride_construction_widgets[WIDX_SLOPE_DOWN_STEEP].right = window_ride_construction_widgets[WIDX_SLOPE_DOWN].right;
		window_ride_construction_widgets[WIDX_SLOPE_DOWN].right = tmp;

		tmp = window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].right;
		window_ride_construction_widgets[WIDX_SLOPE_UP_STEEP].right = window_ride_construction_widgets[WIDX_SLOPE_UP].right;
		window_ride_construction_widgets[WIDX_SLOPE_UP].right = tmp;
	}

	window_ride_construction_widgets[WIDX_BANKING_GROUPBOX].image = STR_RIDE_CONSTRUCTION_ROLL_BANKING;
	window_ride_construction_widgets[WIDX_BANK_LEFT].image = 5153;
	window_ride_construction_widgets[WIDX_BANK_LEFT].tooltip = STR_RIDE_CONSTRUCTION_ROLL_FOR_LEFT_CURVE_TIP;
	window_ride_construction_widgets[WIDX_BANK_LEFT].left = 47;
	window_ride_construction_widgets[WIDX_BANK_LEFT].right = 70;
	window_ride_construction_widgets[WIDX_BANK_LEFT].top = 132;
	window_ride_construction_widgets[WIDX_BANK_LEFT].bottom = 155;
	window_ride_construction_widgets[WIDX_BANK_STRAIGHT].image = 5154;
	window_ride_construction_widgets[WIDX_BANK_STRAIGHT].tooltip = STR_RIDE_CONSTRUCTION_NO_ROLL_TIP;
	window_ride_construction_widgets[WIDX_BANK_STRAIGHT].left = 71;
	window_ride_construction_widgets[WIDX_BANK_STRAIGHT].right = 94;
	window_ride_construction_widgets[WIDX_BANK_STRAIGHT].top = 132;
	window_ride_construction_widgets[WIDX_BANK_STRAIGHT].bottom = 155;
	window_ride_construction_widgets[WIDX_BANK_RIGHT].image = 5155;
	window_ride_construction_widgets[WIDX_BANK_RIGHT].tooltip = STR_RIDE_CONSTRUCTION_ROLL_FOR_RIGHT_CURVE_TIP;
	window_ride_construction_widgets[WIDX_BANK_RIGHT].left = 95;
	window_ride_construction_widgets[WIDX_BANK_RIGHT].right = 118;
	window_ride_construction_widgets[WIDX_BANK_RIGHT].top = 132;
	window_ride_construction_widgets[WIDX_BANK_RIGHT].bottom = 155;
	window_ride_construction_widgets[WIDX_BANK_LEFT].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_BANK_STRAIGHT].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_BANK_RIGHT].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_U_TRACK].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_O_TRACK].type = WWT_EMPTY;
	if (RCT2_GLOBAL(0x00F440D0, uint8) != 99 && _previousTrackPieceSlope != 355) {
		if (_enabledRidePieces & (1 << 6)) {
			window_ride_construction_widgets[WIDX_BANK_LEFT].type = WWT_FLATBTN;
			window_ride_construction_widgets[WIDX_BANK_STRAIGHT].type = WWT_FLATBTN;
			window_ride_construction_widgets[WIDX_BANK_RIGHT].type = WWT_FLATBTN;
		}
		if (ride_type_has_flag(edi, RIDE_TYPE_FLAG_12)) {
			if (edi == RIDE_TYPE_WATER_COASTER) {
				window_ride_construction_widgets[WIDX_U_TRACK].image = 5158;
				window_ride_construction_widgets[WIDX_O_TRACK].image = 5159;
				window_ride_construction_widgets[WIDX_U_TRACK].tooltip = STR_RIDE_CONSTRUCTION_STANDARD_RC_TRACK_TIP;
				window_ride_construction_widgets[WIDX_O_TRACK].tooltip = STR_RIDE_CONSTRUCTION_WATER_CHANNEL_TIP;
				if (_previousTrackPieceSlope < 3 && RCT2_GLOBAL(0x00F440B2, uint8) == 0 && _previousTrackBankStart == 0) {
					window_ride_construction_widgets[WIDX_BANKING_GROUPBOX].image = STR_RIDE_CONSTRUCTION_TRACK_STYLE;
					window_ride_construction_widgets[WIDX_U_TRACK].type = WWT_FLATBTN;
					window_ride_construction_widgets[WIDX_O_TRACK].type = WWT_FLATBTN;
				}
			} else {
				window_ride_construction_widgets[WIDX_U_TRACK].image = 5156;
				window_ride_construction_widgets[WIDX_O_TRACK].image = 5157;
				window_ride_construction_widgets[WIDX_U_TRACK].tooltip = STR_RIDE_CONSTRUCTION_U_SHAPED_OPEN_TRACK_TIP;
				window_ride_construction_widgets[WIDX_O_TRACK].tooltip = STR_RIDE_CONSTRUCTION_O_SHAPED_ENCLOSED_TRACK_TIP;
				window_ride_construction_widgets[WIDX_BANKING_GROUPBOX].image = STR_RIDE_CONSTRUCTION_TRACK_STYLE;
				window_ride_construction_widgets[WIDX_U_TRACK].type = WWT_FLATBTN;
				window_ride_construction_widgets[WIDX_O_TRACK].type = WWT_FLATBTN;
			}
		}
	} else {
		window_ride_construction_widgets[WIDX_BANKING_GROUPBOX].image = STR_RIDE_CONSTRUCTION_BRAKE_SPEED;
		RCT2_GLOBAL(0x00F440D3, uint8) = 1;
		window_ride_construction_widgets[WIDX_BANK_LEFT].image = 1675;
		window_ride_construction_widgets[WIDX_BANK_LEFT].tooltip = STR_RIDE_CONSTRUCTION_BRAKE_SPEED_LIMIT_TIP;
		window_ride_construction_widgets[WIDX_BANK_STRAIGHT].tooltip = STR_RIDE_CONSTRUCTION_BRAKE_SPEED_LIMIT_TIP;
		window_ride_construction_widgets[WIDX_BANK_RIGHT].tooltip = STR_RIDE_CONSTRUCTION_BRAKE_SPEED_LIMIT_TIP;
		window_ride_construction_widgets[WIDX_BANK_LEFT].type = WWT_SPINNER;
		window_ride_construction_widgets[WIDX_BANK_LEFT].left = 12;
		window_ride_construction_widgets[WIDX_BANK_LEFT].right = 83;
		window_ride_construction_widgets[WIDX_BANK_LEFT].top = 138;
		window_ride_construction_widgets[WIDX_BANK_LEFT].bottom = 149;
		window_ride_construction_widgets[WIDX_BANK_STRAIGHT].type = WWT_DROPDOWN_BUTTON;
		window_ride_construction_widgets[WIDX_BANK_STRAIGHT].image = STR_NUMERIC_UP;
		window_ride_construction_widgets[WIDX_BANK_STRAIGHT].left = 72;
		window_ride_construction_widgets[WIDX_BANK_STRAIGHT].right = 82;
		window_ride_construction_widgets[WIDX_BANK_STRAIGHT].top = 139;
		window_ride_construction_widgets[WIDX_BANK_STRAIGHT].bottom = 143;
		window_ride_construction_widgets[WIDX_BANK_RIGHT].type = WWT_DROPDOWN_BUTTON;
		window_ride_construction_widgets[WIDX_BANK_RIGHT].image = STR_NUMERIC_DOWN;
		window_ride_construction_widgets[WIDX_BANK_RIGHT].left = 72;
		window_ride_construction_widgets[WIDX_BANK_RIGHT].right = 82;
		window_ride_construction_widgets[WIDX_BANK_RIGHT].top = 144;
		window_ride_construction_widgets[WIDX_BANK_RIGHT].bottom = 148;
		w->hold_down_widgets |= (1 << WIDX_BANK_STRAIGHT) | (1 << WIDX_BANK_RIGHT);
	}

	window_ride_construction_widgets[WIDX_BANKING_GROUPBOX].right = 162;
	window_ride_construction_widgets[WIDX_SEAT_ROTATION_GROUPBOX].type = 0;
	window_ride_construction_widgets[WIDX_SEAT_ROTATION_ANGLE_SPINNER].type = 0;
	window_ride_construction_widgets[WIDX_SEAT_ROTATION_ANGLE_SPINNER_UP].type = 0;
	window_ride_construction_widgets[WIDX_SEAT_ROTATION_ANGLE_SPINNER_DOWN].type = 0;
	if (
		(edi == RIDE_TYPE_MULTI_DIMENSION_ROLLER_COASTER || edi == RIDE_TYPE_38) &&
		RCT2_GLOBAL(0x00F440D0, uint8) != 99 &&
		_previousTrackPieceSlope != 355
	) {
		window_ride_construction_widgets[WIDX_SEAT_ROTATION_GROUPBOX].type = WWT_GROUPBOX;
		window_ride_construction_widgets[WIDX_SEAT_ROTATION_ANGLE_SPINNER].type = WWT_SPINNER;
		window_ride_construction_widgets[WIDX_SEAT_ROTATION_ANGLE_SPINNER_UP].type = WWT_DROPDOWN_BUTTON;
		window_ride_construction_widgets[WIDX_SEAT_ROTATION_ANGLE_SPINNER_DOWN].type = WWT_DROPDOWN_BUTTON;
		window_ride_construction_widgets[WIDX_BANKING_GROUPBOX].right = 92;
		if (window_ride_construction_widgets[WIDX_BANK_LEFT].type == WWT_SPINNER) {
			for (int i = WIDX_BANK_LEFT; i <= WIDX_BANK_RIGHT; i++) {
				window_ride_construction_widgets[i].left -= 36;
				window_ride_construction_widgets[i].right -= 36;
			}
		}
	}

	uint64 pressedWidgets = w->pressed_widgets & 0x7D80003F;
	window_ride_construction_widgets[WIDX_CONSTRUCT].type = WWT_EMPTY;
	window_ride_construction_widgets[WIDX_DEMOLISH].type = WWT_FLATBTN;
	window_ride_construction_widgets[WIDX_ROTATE].type = WWT_EMPTY;
	if (ride_type_has_flag(edi, RIDE_TYPE_FLAG_8)) {
		window_ride_construction_widgets[WIDX_PREVIOUS_SECTION].type = WWT_EMPTY;
		window_ride_construction_widgets[WIDX_NEXT_SECTION].type = WWT_EMPTY;
	} else {
		window_ride_construction_widgets[WIDX_PREVIOUS_SECTION].type = WWT_FLATBTN;
		window_ride_construction_widgets[WIDX_NEXT_SECTION].type = WWT_FLATBTN;
	}

	switch (RCT2_GLOBAL(0x00F440A6, uint8)) {
	case 1:
		window_ride_construction_widgets[WIDX_CONSTRUCT].type = WWT_IMGBTN;
		window_ride_construction_widgets[WIDX_NEXT_SECTION].type = WWT_EMPTY;
		break;
	case 2:
		window_ride_construction_widgets[WIDX_CONSTRUCT].type = WWT_IMGBTN;
		window_ride_construction_widgets[WIDX_PREVIOUS_SECTION].type = WWT_EMPTY;
		break;
	case 4:
		window_ride_construction_widgets[WIDX_CONSTRUCT].type = WWT_IMGBTN;
		window_ride_construction_widgets[WIDX_DEMOLISH].type = WWT_EMPTY;
		window_ride_construction_widgets[WIDX_NEXT_SECTION].type = WWT_EMPTY;
		window_ride_construction_widgets[WIDX_PREVIOUS_SECTION].type = WWT_EMPTY;
		window_ride_construction_widgets[WIDX_ROTATE].type = WWT_FLATBTN;
		break;
	case 5:
		window_ride_construction_widgets[WIDX_DEMOLISH].type = WWT_EMPTY;
		window_ride_construction_widgets[WIDX_NEXT_SECTION].type = WWT_EMPTY;
		window_ride_construction_widgets[WIDX_PREVIOUS_SECTION].type = WWT_EMPTY;
		break;
	default:
		w->pressed_widgets = pressedWidgets;
		window_invalidate(w);
		return;
	}

	int widgetIndex;
	switch (_previousTrackPieceSlope) {
	case 0:
		widgetIndex = WIDX_STRAIGHT;
		break;
	case 1:
		widgetIndex = WIDX_LEFT_CURVE;
		break;
	case 2:
		widgetIndex = WIDX_RIGHT_CURVE;
		break;
	case 3:
		widgetIndex = WIDX_LEFT_CURVE_SMALL;
		break;
	case 4:
		widgetIndex = WIDX_RIGHT_CURVE_SMALL;
		break;
	case 5:
		widgetIndex = WIDX_LEFT_CURVE_VERY_SMALL;
		break;
	case 6:
		widgetIndex = WIDX_RIGHT_CURVE_VERY_SMALL;
		break;
	case 7:
		widgetIndex = WIDX_LEFT_CURVE_LARGE;
		break;
	case 8:
		widgetIndex = WIDX_RIGHT_CURVE_LARGE;
		break;
	default:
		widgetIndex = WIDX_SPECIAL_TRACK_DROPDOWN;
		break;
	}
	pressedWidgets |= (1ULL << widgetIndex);

	switch (RCT2_GLOBAL(0x00F440B2, uint8)) {
	case 8:
	case 10:
		widgetIndex = WIDX_SLOPE_DOWN_STEEP;
		break;
	case 6:
		widgetIndex = WIDX_SLOPE_DOWN;
		break;
	case 2:
		widgetIndex = WIDX_SLOPE_UP;
		break;
	case 4:
	case 18:
		widgetIndex = WIDX_SLOPE_UP_STEEP;
		break;
	default:
		widgetIndex = WIDX_LEVEL;
		break;
	}
	pressedWidgets |= (1ULL << widgetIndex);

	if (RCT2_GLOBAL(0x00F440D3, uint8) == 0) {
		if (ride_type_has_flag(edi, RIDE_TYPE_FLAG_12)) {
			if (RCT2_GLOBAL(0x00F440B5, uint8) & 1) {
				w->pressed_widgets |= (1ULL << WIDX_O_TRACK);
			} else {
				w->pressed_widgets |= (1ULL << WIDX_U_TRACK);
			}
		}
		switch (_previousTrackBankStart) {
		case TRACK_BANK_LEFT:
			widgetIndex = WIDX_BANK_LEFT;
			break;
		case TRACK_BANK_NONE:
			widgetIndex = WIDX_BANK_STRAIGHT;
			break;
		default:
			widgetIndex = WIDX_BANK_RIGHT;
			break;
		}
		pressedWidgets |= (1ULL << widgetIndex);
	}

	if (RCT2_GLOBAL(0x00F440B4, uint8) & 1)
		pressedWidgets |= (1 << WIDX_CHAIN_LIFT);

	w->pressed_widgets = pressedWidgets;
	window_invalidate(w);
}

/**
 * 
 * rct2: 0x006CA2DF
 * bh: trackRotation (out)
 * dl: ??? (out)
 * dh: trackType (out)
 * edx >> 16: ??? (out)
 */
static bool sub_6CA2DF(int *trackType, int *trackDirection, int *rideIndex, int *edxRS16)
{
	int eax, ebx, ecx, edx, esi, edi, ebp;
	if (RCT2_CALLFUNC_X(0x006CA2DF, &eax, &ebx, &ecx, &edx, &esi, &edi, &ebp) & 0x100)
		return true;

	if (trackType != NULL) *trackType = (edx >> 8) & 0xFF;
	if (trackDirection != NULL) *trackDirection = (ebx >> 8) & 0xFF;
	if (rideIndex != NULL) *rideIndex = edx & 0xFF;
	if (edxRS16 != NULL) *edxRS16 = (edx >> 16) & 0xFFFF;
	return false;
}

/**
 * 
 * rct2: 0x006C6A77
 */
static void sub_6C6A77()
{
	rct_ride *ride = GET_RIDE(_currentRideIndex);
	rct_ride_type *rideEntry = ride_get_entry(ride);

	int rideType = RCT2_GLOBAL(0x00F440B5, uint8) & 2 ? RCT2_ADDRESS(0x0097D4F5, uint8)[ride->type * 8] : ride->type;
	RCT2_GLOBAL(0x00F44048, uint32) = rideEntry->enabledTrackPieces & RCT2_ADDRESS(0x01357444, uint32)[rideType];
	RCT2_GLOBAL(0x00F4404C, uint32) = rideEntry->enabledTrackPiecesAdditional & RCT2_ADDRESS(0x01357644, uint32)[rideType];
}

/**
 * 
 * rct2: 0x006CA162
 */
money32 sub_6CA162(int rideIndex)
{
	int eax, ebx, ecx, edx, esi, edi, ebp;
	RCT2_CALLFUNC_X(0x006CA162, &eax, &ebx, &ecx, &edx, &esi, &edi, &ebp);
	return ebx;
}

/**
 * 
 * rct2: 0x006C94D8
 */
void sub_6C94D8()
{
	int x, y, z, direction, type, ebp, rideIndex, edxRS16;

	switch (RCT2_GLOBAL(0x00F440A6, uint8)) {
	case 1:
	case 2:
		if (!(RCT2_GLOBAL(0x00F440B0, uint8) & 2)) {
			if (sub_6CA2DF(&type, &direction, &rideIndex, &edxRS16)) {
				sub_6C96C0();
			} else {
				_currentTrackPrice = sub_6CA162(rideIndex);
				sub_6C84CE();
			}
		}
		RCT2_GLOBAL(0x00F440B1, sint8)--;
		if (RCT2_GLOBAL(0x00F440B1, sint8) >= 0)
			break;

		RCT2_GLOBAL(0x00F440B1, sint8) = 5;
		RCT2_GLOBAL(0x00F440B0, uint8) ^= 1;
		x = _currentTrackPieceX;
		y = _currentTrackPieceY;
		z = _currentTrackPieceZ;
		direction = _currentTrackPieceDirection & 3;
		type = _currentTrackPieceType;
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_ARROW_X, uint16) = x;
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_ARROW_Y, uint16) = y;
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_ARROW_Z, uint16) = z;
		if (direction < 4)
			direction += 4;
		if (RCT2_GLOBAL(0x00F440A6, uint8) == 2)
			direction = 2;
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_ARROW_DIRECTION, uint8) = direction;
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_FLAGS, uint16) &= ~4;
		if (RCT2_GLOBAL(0x00F440B0, uint8) & 1)
			RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_FLAGS, uint16) |= 4;
		map_invalidate_tile_full(x, y);
		break;
	case 3:
		RCT2_GLOBAL(0x00F440B1, sint8)--;
		if (RCT2_GLOBAL(0x00F440B1, sint8) >= 0)
			break;

		RCT2_GLOBAL(0x00F440B1, sint8) = 5;
		RCT2_GLOBAL(0x00F440B0, uint8) ^= 1;
		x = _currentTrackPieceX;
		y = _currentTrackPieceY;
		z = _currentTrackPieceZ;
		direction = _currentTrackPieceDirection & 3;
		type = _currentTrackPieceType;
		ebp = RCT2_GLOBAL(0x00F440B0, uint8) & 1 ? 2 : 1;
		if (sub_6C683D(&x, &y, z, direction, type, 0, 0, ebp)) {
			sub_6C96C0();
			RCT2_GLOBAL(0x00F440A6, uint8) = 0;
		}
		break;
	case 6:
	case 7:
	case 8:
		RCT2_GLOBAL(0x00F440B1, sint8)--;
		if (RCT2_GLOBAL(0x00F440B1, sint8) >= 0)
			break;

		RCT2_GLOBAL(0x00F440B1, sint8) = 5;
		RCT2_GLOBAL(0x00F440B0, uint8) ^= 1;
		x = _currentTrackPieceX & 0xFFE0;
		y = _currentTrackPieceY & 0xFFE0;
		z = _currentTrackPieceZ + 15;
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_ARROW_X, uint16) = x;
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_ARROW_Y, uint16) = y;
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_ARROW_Z, uint16) = z;
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_ARROW_DIRECTION, uint8) = 4;
		if (((_currentTrackPieceX & 0x1F) | (_currentTrackPieceY & 0x1F)) != 0) {
			RCT2_GLOBAL(RCT2_ADDRESS_MAP_ARROW_DIRECTION, uint8) = 6;
			if (((_currentTrackPieceX & 0x1F) & (_currentTrackPieceY & 0x1F)) == 0) {
				RCT2_GLOBAL(RCT2_ADDRESS_MAP_ARROW_DIRECTION, uint8) = 5;
				if ((_currentTrackPieceY & 0x1F) == 0)
					RCT2_GLOBAL(RCT2_ADDRESS_MAP_ARROW_DIRECTION, uint8) = 7;
			}
		}
		RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_FLAGS, uint16) &= ~4;
		if (RCT2_GLOBAL(0x00F440B0, uint8) & 1)
			RCT2_GLOBAL(RCT2_ADDRESS_MAP_SELECTION_FLAGS, uint16) |= 4;
		map_invalidate_tile_full(x, y);
		break;
	}
}

/**
 * 
 * rct2: 0x006C8648
 */
static void window_ride_construction_update_possible_ride_configurations()
{
	rct_ride *ride;
	int trackType;
	int edx, edi;

	ride = GET_RIDE(_currentRideIndex);

	RCT2_GLOBAL(0x00F440D3, uint8) = 0;
	if (RCT2_GLOBAL(0x00F440B5, uint8) & 2)
		edi = RCT2_GLOBAL(0x0097D4F5 + (ride->type * 8), uint8);
	else
		edi = ride->type;

	int currentPossibleRideConfigurationIndex = 0;
	RCT2_GLOBAL(0x00F4407A, uint16) = 0;
	for (trackType = 0; trackType < 256; trackType++) {
		edx = ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_SELLS_FOOD) ?
			RCT2_GLOBAL(0x00997C9D + (trackType * 8), uint8) :
			RCT2_GLOBAL(0x0099849D + (trackType * 8), uint8);

		if (edx == 0)
			continue;

		if (edx & 0x80) {
			edx &= 0x7F;
			if (edx != edi)
				continue;
		} else {
			if (edx <= 32) {
				if (!(RCT2_GLOBAL(0x00F44048, uint32) & (1 << edx)))
					continue;
			} else {
				edx -= 32;
				if (!RCT2_GLOBAL(0x00F4404C, uint32) & edx)
					continue;
			}
		}

		int slope, bank;
		if (RCT2_GLOBAL(0x00F440A6, uint8) != 1 && RCT2_GLOBAL(0x00F440A6, uint8) != 4) {
			if (RCT2_GLOBAL(0x00F440A6, uint8) != 2)
				continue;

			if (ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_SELLS_FOOD)) {
				slope = RCT2_GLOBAL(0x0099849E + (trackType * 8), uint8);
				bank = RCT2_GLOBAL(0x009984A0 + (trackType * 8), uint8);
			} else {
				slope = RCT2_GLOBAL(0x00997C9E + (trackType * 8), uint8);
				bank = RCT2_GLOBAL(0x00997CA0 + (trackType * 8), uint8);
			}
		} else {
			if (ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_SELLS_FOOD)) {
				slope = RCT2_GLOBAL(0x0099849F + (trackType * 8), uint8);
				bank = RCT2_GLOBAL(0x009984A1 + (trackType * 8), uint8);
			} else {
				slope = RCT2_GLOBAL(0x00997C9F + (trackType * 8), uint8);
				bank = RCT2_GLOBAL(0x00997CA1 + (trackType * 8), uint8);
			}
		}

		if (!ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_SELLS_FOOD)) {
			if (
				RCT2_GLOBAL(0x00997C9D + (trackType * 8), uint8) == 21 ||
				RCT2_GLOBAL(0x00997C9D + (trackType * 8), uint8) != 22
			) {
				if (bank != _previousTrackBankEnd) {
					if (_previousTrackBankEnd != 0)
						continue;

					if (bank != TRACK_BANK_LEFT)
						continue;
				}
			}
		}

		if (bank == TRACK_BANK_UPSIDE_DOWN && bank != _previousTrackBankEnd)
			continue;
		
		_currentPossibleRideConfigurations[currentPossibleRideConfigurationIndex] = trackType;
		RCT2_GLOBAL(0x00F4409C, uint32) |= (1 << currentPossibleRideConfigurationIndex);
		if (
			_currentTrackPieceDirection < 4 &&
			slope == _previousTrackSlopeEnd &&
			bank == _previousTrackBankEnd &&
			(trackType != 66 || ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_29))
		) {
			RCT2_GLOBAL(0x00F4409C, uint32) &= ~(1 << currentPossibleRideConfigurationIndex);
			RCT2_GLOBAL(0x00F4407A, uint16)++;
		}
		currentPossibleRideConfigurationIndex++;
	}
	_numCurrentPossibleRideConfigurations = currentPossibleRideConfigurationIndex;
}