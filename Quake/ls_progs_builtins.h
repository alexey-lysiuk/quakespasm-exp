/*

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#pragma once

#ifdef USE_LUA_SCRIPTING

extern "C"
{
#include "q_stdinc.h"
#include "pr_comp.h"
}

constexpr etype_t BUILTIN_RETURN_TYPES[] =
{
	ev_bad,
	ev_void,     // void(entity e) makevectors = #1
	ev_void,     // void(entity e, vector o) setorigin = #2
	ev_void,     // void(entity e, string m) setmodel = #3
	ev_void,     // void(entity e, vector min, vector max) setsize = #4
	ev_void,     // void(entity e, vector min, vector max) setabssize = #5
	ev_void,     // void() break = #6
	ev_float,    // float() random = #7
	ev_void,     // void(entity e, float chan, string samp) sound = #8
	ev_vector,   // vector(vector v) normalize = #9
	ev_void,     // void(string e) error = #10
	ev_void,     // void(string e) objerror = #11
	ev_float,    // float(vector v) vlen = #12
	ev_float,    // float(vector v) vectoyaw = #13
	ev_entity,   // entity() spawn = #14
	ev_void,     // void(entity e) remove = #15
	ev_float,    // float(vector v1, vector v2, float tryents) traceline = #16
	ev_entity,   // entity() clientlist = #17
	ev_entity,   // entity(entity start, .string fld, string match) find = #18
	ev_void,     // void(string s) precache_sound = #19
	ev_void,     // void(string s) precache_model = #20
	ev_void,     // void(entity client, string s)stuffcmd = #21
	ev_entity,   // entity(vector org, float rad) findradius = #22
	ev_void,     // void(string s) bprint = #23
	ev_void,     // void(entity client, string s) sprint = #24
	ev_void,     // void(string s) dprint = #25
	ev_void,     // void(string s) ftos = #26
	ev_void,     // void(string s) vtos = #27
	ev_void,     // void() coredump = #28
	ev_void,     // void() traceon = #29
	ev_void,     // void() traceoff = #30
	ev_void,     // void(entity e) eprint = #31
	ev_float,    // float(float yaw, float dist) walkmove = #32
	ev_bad,      // #33 was removed
	ev_float,    // float(float yaw, float dist) droptofloor = #34
	ev_void,     // void(float style, string value) lightstyle = #35
	ev_float,    // float(float v) rint = #36
	ev_float,    // float(float v) floor = #37
	ev_float,    // float(float v) ceil = #38
	ev_bad,      // #39 was removed
	ev_float,    // float(entity e) checkbottom = #40
	ev_float,    // float(vector v) pointcontents = #41
	ev_bad,      // #42 was removed
	ev_float,    // float(float f) fabs = #43
	ev_vector,   // vector(entity e, float speed) aim = #44
	ev_float,    // float(string s) cvar = #45
	ev_void,     // void(string s) localcmd = #46
	ev_entity,   // entity(entity e) nextent = #47
	ev_void,     // void(vector o, vector d, float color, float count) particle = #48
	ev_void,     // void() ChangeYaw = #49
	ev_bad,      // #50 was removed
	ev_vector,   // vector(vector v) vectoangles = #51
	ev_void,     // void(float to, float f) WriteByte = #52
	ev_void,     // void(float to, float f) WriteChar = #53
	ev_void,     // void(float to, float f) WriteShort = #54
	ev_void,     // void(float to, float f) WriteLong = #55
	ev_void,     // void(float to, float f) WriteCoord = #56
	ev_void,     // void(float to, float f) WriteAngle = #57
	ev_void,     // void(float to, string s) WriteString = #58
	ev_void,     // void(float to, entity s) WriteEntity = #59
	ev_bad,      // #60 was removed
	ev_bad,      // #61 was removed
	ev_bad,      // #62 was removed
	ev_bad,      // #63 was removed
	ev_bad,      // #64 was removed
	ev_bad,      // #65 was removed
	ev_bad,      // #66 was removed
	ev_void,     // void(float step) movetogoal = #67
	ev_string,   // string(string s) precache_file = #68
	ev_void,     // void(entity e) makestatic = #69
	ev_void,     // void(string s) changelevel = #70
	ev_bad,      // #71 was removed
	ev_void,     // void(string var, string val) cvar_set = #72
	ev_void,     // void(entity client, string s) centerprint = #73
	ev_void,     // void(vector pos, string samp, float vol, float atten) ambientsound = #74
	ev_string,   // string(string s) precache_model2 = #75
	ev_string,   // string(string s) precache_sound2 = #76
	ev_string,   // string(string s) precache_file2 = #77
	ev_void,     // void(entity e) setspawnparms = #78

	// 2021 re-release, see PR_PatchRereleaseBuiltins()
	ev_float,    // float() finaleFinished = #79
	ev_void,     // void localsound (entity client, string sample) = #80
	ev_void,     // void draw_point (vector point, float colormap, float lifetime, float depthtest) = #81
	ev_void,     // void draw_line (vector start, vector end, float colormap, float lifetime, float depthtest) = #82
	ev_void,     // void draw_arrow (vector start, vector end, float colormap, float size, float lifetime, float depthtest) = #83
	ev_void,     // void draw_ray (vector start, vector direction, float length, float colormap, float size, float lifetime, float depthtest) = #84
	ev_void,     // void draw_circle (vector origin, float radius, float colormap, float lifetime, float depthtest) = #85
	ev_void,     // void draw_bounds (vector min, vector max, float colormap, float lifetime, float depthtest) = #86
	ev_void,     // void draw_worldtext (string s, vector origin, float size, float lifetime, float depthtest) = #87
	ev_void,     // void draw_sphere (vector origin, float radius, float colormap, float lifetime, float depthtest) = #88
	ev_void,     // void draw_cylinder (vector origin, float halfHeight, float radius, float colormap, float lifetime, float depthtest) = #89
	ev_float,    // float CheckPlayerEXFlags( entity playerEnt ) = #90
	ev_float,    // float walkpathtogoal( float movedist, vector goal ) = #91
	ev_float,    // float bot_movetopoint( entity bot, vector point ) = #92
	// same # as above, float bot_followentity( entity bot, entity goal ) = #92
};

#endif // USE_LUA_SCRIPTING
