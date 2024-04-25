/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2014 QuakeSpasm developers

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
// world.c -- world query functions

#include "quakedef.h"

#include <time.h>

/*

entities never clip against themselves, or their owner

line of sight checks trace->crosscontent, but bullets don't

*/


typedef struct
{
	vec3_t		boxmins, boxmaxs;// enclose the test object along entire move
	float		*mins, *maxs;	// size of the moving object
	vec3_t		mins2, maxs2;	// size when clipping against mosnters
	float		*start, *end;
	trace_t		trace;
	int			type;
	edict_t		*passedict;
} moveclip_t;


int SV_HullPointContents (hull_t *hull, int num, vec3_t p);

/*
===============================================================================

HULL BOXES

===============================================================================
*/


static	hull_t		box_hull;
static	mclipnode_t	box_clipnodes[6]; //johnfitz -- was dclipnode_t
static	mplane_t	box_planes[6];

/*
===================
SV_InitBoxHull

Set up the planes and clipnodes so that the six floats of a bounding box
can just be stored out and get a proper hull_t structure.
===================
*/
void SV_InitBoxHull (void)
{
	int		i;
	int		side;

	box_hull.clipnodes = box_clipnodes;
	box_hull.planes = box_planes;
	box_hull.firstclipnode = 0;
	box_hull.lastclipnode = 5;

	for (i=0 ; i<6 ; i++)
	{
		box_clipnodes[i].planenum = i;

		side = i&1;

		box_clipnodes[i].children[side] = CONTENTS_EMPTY;
		if (i != 5)
			box_clipnodes[i].children[side^1] = i + 1;
		else
			box_clipnodes[i].children[side^1] = CONTENTS_SOLID;

		box_planes[i].type = i>>1;
		box_planes[i].normal[i>>1] = 1;
	}

}


/*
===================
SV_HullForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
hull_t	*SV_HullForBox (vec3_t mins, vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = mins[0];
	box_planes[2].dist = maxs[1];
	box_planes[3].dist = mins[1];
	box_planes[4].dist = maxs[2];
	box_planes[5].dist = mins[2];

	return &box_hull;
}



/*
================
SV_HullForEntity

Returns a hull that can be used for testing or clipping an object of mins/maxs
size.
Offset is filled in to contain the adjustment that must be added to the
testing object's origin to get a point to use with the returned hull.
================
*/
hull_t *SV_HullForEntity (edict_t *ent, vec3_t mins, vec3_t maxs, vec3_t offset)
{
	qmodel_t	*model;
	vec3_t		size;
	vec3_t		hullmins, hullmaxs;
	hull_t		*hull;

// decide which clipping hull to use, based on the size
	if (ent->v.solid == SOLID_BSP)
	{	// explicit hulls in the BSP model
		if (ent->v.movetype != MOVETYPE_PUSH)
			Host_Error ("SOLID_BSP without MOVETYPE_PUSH (%s at %f %f %f)",
				    PR_GetString(ent->v.classname), ent->v.origin[0], ent->v.origin[1], ent->v.origin[2]);

		model = sv.models[ (int)ent->v.modelindex ];

		if (!model || model->type != mod_brush)
			Host_Error ("SOLID_BSP with a non bsp model (%s at %f %f %f)",
				    PR_GetString(ent->v.classname), ent->v.origin[0], ent->v.origin[1], ent->v.origin[2]);

		VectorSubtract (maxs, mins, size);
		if (size[0] < 3)
			hull = &model->hulls[0];
		else if (size[0] <= 32)
			hull = &model->hulls[1];
		else
			hull = &model->hulls[2];

// calculate an offset value to center the origin
		VectorSubtract (hull->clip_mins, mins, offset);
		VectorAdd (offset, ent->v.origin, offset);
	}
	else
	{	// create a temp hull from bounding box sizes

		VectorSubtract (ent->v.mins, maxs, hullmins);
		VectorSubtract (ent->v.maxs, mins, hullmaxs);
		hull = SV_HullForBox (hullmins, hullmaxs);

		VectorCopy (ent->v.origin, offset);
	}


	return hull;
}

/*
===============================================================================

ENTITY AREA CHECKING

===============================================================================
*/

typedef struct areanode_s
{
	int		axis;		// -1 = leaf node
	float	dist;
	struct areanode_s	*children[2];
	link_t	trigger_edicts;
	link_t	solid_edicts;
} areanode_t;

#define	AREA_DEPTH	4
#define	AREA_NODES	32

static	areanode_t	sv_areanodes[AREA_NODES];
static	int			sv_numareanodes;

/*
===============
SV_CreateAreaNode

===============
*/
areanode_t *SV_CreateAreaNode (int depth, vec3_t mins, vec3_t maxs)
{
	areanode_t	*anode;
	vec3_t		size;
	vec3_t		mins1, maxs1, mins2, maxs2;

	anode = &sv_areanodes[sv_numareanodes];
	sv_numareanodes++;

	ClearLink (&anode->trigger_edicts);
	ClearLink (&anode->solid_edicts);

	if (depth == AREA_DEPTH)
	{
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;
		return anode;
	}

	VectorSubtract (maxs, mins, size);
	if (size[0] > size[1])
		anode->axis = 0;
	else
		anode->axis = 1;

	anode->dist = 0.5 * (maxs[anode->axis] + mins[anode->axis]);
	VectorCopy (mins, mins1);
	VectorCopy (mins, mins2);
	VectorCopy (maxs, maxs1);
	VectorCopy (maxs, maxs2);

	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

	anode->children[0] = SV_CreateAreaNode (depth+1, mins2, maxs2);
	anode->children[1] = SV_CreateAreaNode (depth+1, mins1, maxs1);

	return anode;
}

/*
===============
SV_ClearWorld

===============
*/
void SV_ClearWorld (void)
{
	SV_InitBoxHull ();

	memset (sv_areanodes, 0, sizeof(sv_areanodes));
	sv_numareanodes = 0;
	SV_CreateAreaNode (0, sv.worldmodel->mins, sv.worldmodel->maxs);
}


/*
===============
SV_UnlinkEdict

===============
*/
void SV_UnlinkEdict (edict_t *ent)
{
	if (!ent->area.prev)
		return;		// not linked in anywhere
	RemoveLink (&ent->area);
	ent->area.prev = ent->area.next = NULL;
}


/*
====================
SV_AreaTriggerEdicts

Spike -- just builds a list of entities within the area, rather than walking
them and risking the list getting corrupt.
====================
*/
static void
SV_AreaTriggerEdicts ( edict_t *ent, areanode_t *node, edict_t **list, int *listcount, const int listspace )
{
	link_t		*l, *next;
	edict_t		*touch;

// touch linked edicts
	for (l = node->trigger_edicts.next ; l != &node->trigger_edicts ; l = next)
	{
		next = l->next;
		touch = EDICT_FROM_AREA(l);
		if (touch == ent)
			continue;
		if (!touch->v.touch || touch->v.solid != SOLID_TRIGGER)
			continue;
		if (ent->v.absmin[0] > touch->v.absmax[0]
		|| ent->v.absmin[1] > touch->v.absmax[1]
		|| ent->v.absmin[2] > touch->v.absmax[2]
		|| ent->v.absmax[0] < touch->v.absmin[0]
		|| ent->v.absmax[1] < touch->v.absmin[1]
		|| ent->v.absmax[2] < touch->v.absmin[2] )
			continue;

		if (*listcount == listspace)
			return; // should never happen

		list[*listcount] = touch;
		(*listcount)++;
	}

// recurse down both sides
	if (node->axis == -1)
		return;

	if ( ent->v.absmax[node->axis] > node->dist )
		SV_AreaTriggerEdicts ( ent, node->children[0], list, listcount, listspace );
	if ( ent->v.absmin[node->axis] < node->dist )
		SV_AreaTriggerEdicts ( ent, node->children[1], list, listcount, listspace );
}

/*
====================
SV_TouchLinks

ericw -- copy the touching edicts to an array (on the hunk) so we can avoid
iteating the trigger_edicts linked list while calling PR_ExecuteProgram
which could potentially corrupt the list while it's being iterated.
Based on code from Spike.
====================
*/
void SV_TouchLinks (edict_t *ent)
{
	edict_t		**list;
	edict_t		*touch;
	int		old_self, old_other;
	int		i, listcount;
	int		mark;
	
	mark = Hunk_LowMark ();
	list = (edict_t **) Hunk_Alloc (sv.num_edicts*sizeof(edict_t *));
	
	listcount = 0;
	SV_AreaTriggerEdicts (ent, sv_areanodes, list, &listcount, sv.num_edicts);

	for (i = 0; i < listcount; i++)
	{
		touch = list[i];
	// re-validate in case of PR_ExecuteProgram having side effects that make
	// edicts later in the list no longer touch
		if (touch == ent)
			continue;
		if (!touch->v.touch || touch->v.solid != SOLID_TRIGGER)
			continue;
		if (ent->v.absmin[0] > touch->v.absmax[0]
		|| ent->v.absmin[1] > touch->v.absmax[1]
		|| ent->v.absmin[2] > touch->v.absmax[2]
		|| ent->v.absmax[0] < touch->v.absmin[0]
		|| ent->v.absmax[1] < touch->v.absmin[1]
		|| ent->v.absmax[2] < touch->v.absmin[2] )
			continue;
		old_self = pr_global_struct->self;
		old_other = pr_global_struct->other;

		pr_global_struct->self = EDICT_TO_PROG(touch);
		pr_global_struct->other = EDICT_TO_PROG(ent);
		pr_global_struct->time = sv.time;
		PR_ExecuteProgram (touch->v.touch);

		pr_global_struct->self = old_self;
		pr_global_struct->other = old_other;
	}

// free hunk-allocated edicts array
	Hunk_FreeToLowMark (mark);
}


/*
===============
SV_FindTouchedLeafs

===============
*/
void SV_FindTouchedLeafs (edict_t *ent, mnode_t *node)
{
	mplane_t	*splitplane;
	mleaf_t		*leaf;
	int			sides;
	int			leafnum;

	if (node->contents == CONTENTS_SOLID)
		return;

	if (ent->num_leafs == MAX_ENT_LEAFS)
		return;

// add an efrag if the node is a leaf

	if ( node->contents < 0)
	{
		leaf = (mleaf_t *)node;
		leafnum = leaf - sv.worldmodel->leafs - 1;

		ent->leafnums[ent->num_leafs] = leafnum;
		ent->num_leafs++;
		return;
	}

// NODE_MIXED

	splitplane = node->plane;
	sides = BOX_ON_PLANE_SIDE(ent->v.absmin, ent->v.absmax, splitplane);

// recurse down the contacted sides
	if (sides & 1)
		SV_FindTouchedLeafs (ent, node->children[0]);

	if (sides & 2)
		SV_FindTouchedLeafs (ent, node->children[1]);
}

/*
===============
SV_LinkEdict

===============
*/
void SV_LinkEdict (edict_t *ent, qboolean touch_triggers)
{
	areanode_t	*node;

	if (ent->area.prev)
		SV_UnlinkEdict (ent);	// unlink from old position

	if (ent == sv.edicts)
		return;		// don't add the world

	if (ent->free)
		return;

// set the abs box
	VectorAdd (ent->v.origin, ent->v.mins, ent->v.absmin);
	VectorAdd (ent->v.origin, ent->v.maxs, ent->v.absmax);

//
// to make items easier to pick up and allow them to be grabbed off
// of shelves, the abs sizes are expanded
//
	if ((int)ent->v.flags & FL_ITEM)
	{
		ent->v.absmin[0] -= 15;
		ent->v.absmin[1] -= 15;
		ent->v.absmax[0] += 15;
		ent->v.absmax[1] += 15;
	}
	else
	{	// because movement is clipped an epsilon away from an actual edge,
		// we must fully check even when bounding boxes don't quite touch
		ent->v.absmin[0] -= 1;
		ent->v.absmin[1] -= 1;
		ent->v.absmin[2] -= 1;
		ent->v.absmax[0] += 1;
		ent->v.absmax[1] += 1;
		ent->v.absmax[2] += 1;
	}

// link to PVS leafs
	ent->num_leafs = 0;
	if (ent->v.modelindex)
		SV_FindTouchedLeafs (ent, sv.worldmodel->nodes);

	if (ent->v.solid == SOLID_NOT)
		return;

// find the first node that the ent's box crosses
	node = sv_areanodes;
	while (1)
	{
		if (node->axis == -1)
			break;
		if (ent->v.absmin[node->axis] > node->dist)
			node = node->children[0];
		else if (ent->v.absmax[node->axis] < node->dist)
			node = node->children[1];
		else
			break;		// crosses the node
	}

// link it in

	if (ent->v.solid == SOLID_TRIGGER)
		InsertLinkBefore (&ent->area, &node->trigger_edicts);
	else
		InsertLinkBefore (&ent->area, &node->solid_edicts);

// if touch_triggers, touch all entities at this node and decend for more
	if (touch_triggers)
		SV_TouchLinks ( ent );
}



/*
===============================================================================

POINT TESTING IN HULLS

===============================================================================
*/

/*
==================
SV_HullPointContents

==================
*/
int SV_HullPointContents (hull_t *hull, int num, vec3_t p)
{
	float		d;
	mclipnode_t	*node; //johnfitz -- was dclipnode_t
	mplane_t	*plane;

	while (num >= 0)
	{
		if (num < hull->firstclipnode || num > hull->lastclipnode)
			Sys_Error ("SV_HullPointContents: bad node number");

		node = hull->clipnodes + num;
		plane = hull->planes + node->planenum;

		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DoublePrecisionDotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	return num;
}


/*
==================
SV_PointContents

==================
*/
int SV_PointContents (vec3_t p)
{
	int		cont;

	cont = SV_HullPointContents (&sv.worldmodel->hulls[0], 0, p);
	if (cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN)
		cont = CONTENTS_WATER;
	return cont;
}

int SV_TruePointContents (vec3_t p)
{
	return SV_HullPointContents (&sv.worldmodel->hulls[0], 0, p);
}

//===========================================================================

/*
============
SV_TestEntityPosition

This could be a lot more efficient...
============
*/
edict_t	*SV_TestEntityPosition (edict_t *ent)
{
	trace_t	trace;

	trace = SV_Move (ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, 0, ent);

	if (trace.startsolid)
		return sv.edicts;

	return NULL;
}


/*
===============================================================================

LINE TESTING IN HULLS

===============================================================================
*/

/*
==================
SV_RecursiveHullCheck

==================
*/
qboolean SV_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace)
{
	mclipnode_t	*node; //johnfitz -- was dclipnode_t
	mplane_t	*plane;
	float		t1, t2;
	float		frac;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

// check for empty
	if (num < 0)
	{
		if (num != CONTENTS_SOLID)
		{
			trace->allsolid = false;
			if (num == CONTENTS_EMPTY)
				trace->inopen = true;
			else
				trace->inwater = true;
		}
		else
			trace->startsolid = true;
		return true;		// empty
	}

	if (num < hull->firstclipnode || num > hull->lastclipnode)
		Sys_Error ("SV_RecursiveHullCheck: bad node number");

//
// find the point distances
//
	node = hull->clipnodes + num;
	plane = hull->planes + node->planenum;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = DoublePrecisionDotProduct (plane->normal, p1) - plane->dist;
		t2 = DoublePrecisionDotProduct (plane->normal, p2) - plane->dist;
	}

#if 1
	if (t1 >= 0 && t2 >= 0)
		return SV_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
	if (t1 < 0 && t2 < 0)
		return SV_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);
#else
	if ( (t1 >= DIST_EPSILON && t2 >= DIST_EPSILON) || (t2 > t1 && t1 >= 0) )
		return SV_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
	if ( (t1 <= -DIST_EPSILON && t2 <= -DIST_EPSILON) || (t2 < t1 && t1 <= 0) )
		return SV_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);
#endif

// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < 0)
		frac = (t1 + DIST_EPSILON)/(t1-t2);
	else
		frac = (t1 - DIST_EPSILON)/(t1-t2);
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;

	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	side = (t1 < 0);

// move up to the node
	if (!SV_RecursiveHullCheck (hull, node->children[side], p1f, midf, p1, mid, trace) )
		return false;

#ifdef PARANOID
	if (SV_HullPointContents (sv_hullmodel, mid, node->children[side])
	== CONTENTS_SOLID)
	{
		Con_Printf ("mid PointInHullSolid\n");
		return false;
	}
#endif

	if (SV_HullPointContents (hull, node->children[side^1], mid)
	!= CONTENTS_SOLID)
// go past the node
		return SV_RecursiveHullCheck (hull, node->children[side^1], midf, p2f, mid, p2, trace);

	if (trace->allsolid)
		return false;		// never got out of the solid area

//==================
// the other side of the node is solid, this is the impact point
//==================
	if (!side)
	{
		VectorCopy (plane->normal, trace->plane.normal);
		trace->plane.dist = plane->dist;
	}
	else
	{
		VectorSubtract (vec3_origin, plane->normal, trace->plane.normal);
		trace->plane.dist = -plane->dist;
	}

	while (SV_HullPointContents (hull, hull->firstclipnode, mid)
	== CONTENTS_SOLID)
	{ // shouldn't really happen, but does occasionally
		frac -= 0.1;
		if (frac < 0)
		{
			trace->fraction = midf;
			VectorCopy (mid, trace->endpos);
			Con_DPrintf ("backup past 0\n");
			return false;
		}
		midf = p1f + (p2f - p1f)*frac;
		for (i=0 ; i<3 ; i++)
			mid[i] = p1[i] + frac*(p2[i] - p1[i]);
	}

	trace->fraction = midf;
	VectorCopy (mid, trace->endpos);

	return false;
}


/*
==================
SV_ClipMoveToEntity

Handles selection or creation of a clipping hull, and offseting (and
eventually rotation) of the end points
==================
*/
trace_t SV_ClipMoveToEntity (edict_t *ent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	trace_t		trace;
	vec3_t		offset;
	vec3_t		start_l, end_l;
	hull_t		*hull;

// fill in a default trace
	memset (&trace, 0, sizeof(trace_t));
	trace.fraction = 1;
	trace.allsolid = true;
	VectorCopy (end, trace.endpos);

// get the clipping hull
	hull = SV_HullForEntity (ent, mins, maxs, offset);

	VectorSubtract (start, offset, start_l);
	VectorSubtract (end, offset, end_l);

// trace a line through the apropriate clipping hull
	SV_RecursiveHullCheck (hull, hull->firstclipnode, 0, 1, start_l, end_l, &trace);

// fix trace up by the offset
	if (trace.fraction != 1)
		VectorAdd (trace.endpos, offset, trace.endpos);

// did we clip the move?
	if (trace.fraction < 1 || trace.startsolid  )
		trace.ent = ent;

	return trace;
}

//===========================================================================

/*
====================
SV_ClipToLinks

Mins and maxs enclose the entire area swept by the move
====================
*/
void SV_ClipToLinks ( areanode_t *node, moveclip_t *clip )
{
	link_t		*l, *next;
	edict_t		*touch;
	trace_t		trace;

// touch linked edicts
	for (l = node->solid_edicts.next ; l != &node->solid_edicts ; l = next)
	{
		next = l->next;
		touch = EDICT_FROM_AREA(l);
		if (touch->v.solid == SOLID_NOT)
			continue;
		if (touch == clip->passedict)
			continue;
		if (touch->v.solid == SOLID_TRIGGER)
			Sys_Error ("Trigger in clipping list");

		if (clip->type == MOVE_NOMONSTERS && touch->v.solid != SOLID_BSP)
			continue;

		if (clip->boxmins[0] > touch->v.absmax[0]
		|| clip->boxmins[1] > touch->v.absmax[1]
		|| clip->boxmins[2] > touch->v.absmax[2]
		|| clip->boxmaxs[0] < touch->v.absmin[0]
		|| clip->boxmaxs[1] < touch->v.absmin[1]
		|| clip->boxmaxs[2] < touch->v.absmin[2] )
			continue;

		if (clip->passedict && clip->passedict->v.size[0] && !touch->v.size[0])
			continue;	// points never interact

	// might intersect, so do an exact clip
		if (clip->trace.allsolid)
			return;
		if (clip->passedict)
		{
		 	if (PROG_TO_EDICT(touch->v.owner) == clip->passedict)
				continue;	// don't clip against own missiles
			if (PROG_TO_EDICT(clip->passedict->v.owner) == touch)
				continue;	// don't clip against owner
		}

		if ((int)touch->v.flags & FL_MONSTER)
			trace = SV_ClipMoveToEntity (touch, clip->start, clip->mins2, clip->maxs2, clip->end);
		else
			trace = SV_ClipMoveToEntity (touch, clip->start, clip->mins, clip->maxs, clip->end);
		if (trace.allsolid || trace.startsolid ||
		trace.fraction < clip->trace.fraction)
		{
			trace.ent = touch;
		 	if (clip->trace.startsolid)
			{
				clip->trace = trace;
				clip->trace.startsolid = true;
			}
			else
				clip->trace = trace;
		}
		else if (trace.startsolid)
			clip->trace.startsolid = true;
	}

// recurse down both sides
	if (node->axis == -1)
		return;

	if ( clip->boxmaxs[node->axis] > node->dist )
		SV_ClipToLinks ( node->children[0], clip );
	if ( clip->boxmins[node->axis] < node->dist )
		SV_ClipToLinks ( node->children[1], clip );
}


/*
==================
SV_MoveBounds
==================
*/
void SV_MoveBounds (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, vec3_t boxmins, vec3_t boxmaxs)
{
#if 0
// debug to test against everything
boxmins[0] = boxmins[1] = boxmins[2] = -9999;
boxmaxs[0] = boxmaxs[1] = boxmaxs[2] = 9999;
#else
	int		i;

	for (i=0 ; i<3 ; i++)
	{
		if (end[i] > start[i])
		{
			boxmins[i] = start[i] + mins[i] - 1;
			boxmaxs[i] = end[i] + maxs[i] + 1;
		}
		else
		{
			boxmins[i] = end[i] + mins[i] - 1;
			boxmaxs[i] = start[i] + maxs[i] + 1;
		}
	}
#endif
}

/*
==================
SV_Move
==================
*/
trace_t SV_Move (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, edict_t *passedict)
{
	moveclip_t	clip;
	int			i;

	memset ( &clip, 0, sizeof ( moveclip_t ) );

// clip to world
	clip.trace = SV_ClipMoveToEntity ( sv.edicts, start, mins, maxs, end );

	clip.start = start;
	clip.end = end;
	clip.mins = mins;
	clip.maxs = maxs;
	clip.type = type;
	clip.passedict = passedict;

	if (type == MOVE_MISSILE)
	{
		for (i=0 ; i<3 ; i++)
		{
			clip.mins2[i] = -15;
			clip.maxs2[i] = 15;
		}
	}
	else
	{
		VectorCopy (mins, clip.mins2);
		VectorCopy (maxs, clip.maxs2);
	}

// create the bounding box of the entire move
	SV_MoveBounds ( start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs );

// clip to entities
	SV_ClipToLinks ( sv_areanodes, &clip );

	return clip.trace;
}


//
// Entity tracing
//

typedef struct
{
	moveclip_t clip;
	trace_t initialtrace;
	vec3_t mins;
	vec3_t maxs;
	vec3_t start;
	vec3_t end;
	vec3_t forward;
} et_state_t;

static void ET_InitEntityTrace(et_state_t* state)
{
	edict_t* player = state->clip.passedict;
	const vec_t* angles = player->v.v_angle;

	vec_t angle = angles[YAW] * (M_PI * 2 / 360);
	vec_t sy = sin(angle);
	vec_t cy = cos(angle);

	angle = angles[PITCH] * (M_PI*2 / 360);
	vec_t sp = sin(angle);
	vec_t cp = cos(angle);

	state->forward[0] = cp * cy;
	state->forward[1] = cp * sy;
	state->forward[2] = -sp;

	VectorCopy(vec3_origin, state->mins);
	VectorCopy(vec3_origin, state->maxs);

	VectorCopy(player->v.origin, state->start);
	state->start[2] += 20;
	VectorMA(state->start, 16 * 1024, state->forward, state->end);

	// clip to world
	moveclip_t* clip = &state->clip;
	clip->trace = SV_ClipMoveToEntity(sv.edicts, state->start, state->mins, state->maxs, state->end);
	clip->start = state->start;
	clip->end = state->end;
	clip->mins = state->mins;
	clip->maxs = state->maxs;
	clip->type = 0;

	VectorCopy(state->mins, clip->mins2);
	VectorCopy(state->maxs, clip->maxs2);

	// create the bounding box of the entire move
	SV_MoveBounds(state->start, clip->mins2, clip->maxs2, state->end, clip->boxmins, clip->boxmaxs);
}

// Mostly a copy of SV_ClipToLinks() but for trigger_edicts
static void ET_TraceTriger(areanode_t* node, moveclip_t* clip)
{
	for (link_t* next, *current = node->trigger_edicts.next; current != &node->trigger_edicts; current = next)
	{
		next = current->next;

		edict_t* touch = EDICT_FROM_AREA(current);

		if (touch->v.solid == SOLID_NOT)
			continue;

		if (touch == clip->passedict)
			continue;

		if (clip->boxmins[0] > touch->v.absmax[0]
			|| clip->boxmins[1] > touch->v.absmax[1]
			|| clip->boxmins[2] > touch->v.absmax[2]
			|| clip->boxmaxs[0] < touch->v.absmin[0]
			|| clip->boxmaxs[1] < touch->v.absmin[1]
			|| clip->boxmaxs[2] < touch->v.absmin[2])
			continue;

		// might intersect, so do an exact clip
		if (clip->trace.allsolid)
			return;

		if (clip->passedict)
		{
			if (PROG_TO_EDICT(touch->v.owner) == clip->passedict)
				continue;	// don't clip against own missiles

			if (PROG_TO_EDICT(clip->passedict->v.owner) == touch)
				continue;	// don't clip against owner
		}

		trace_t trace;

		{
			// Replacement of SV_ClipMoveToEntity()

			// fill in a default trace
			memset(&trace, 0, sizeof(trace_t));
			trace.fraction = 1;
			trace.allsolid = true;
			VectorCopy(clip->end, trace.endpos);

			// get the clipping hull
			hull_t* hull;
			vec3_t offset;

			{
				// Replacemet of SV_HullForEntity() with adjustment for zero-sized triggers
				// like lava balls (fireball quakec class)

				vec3_t hullmins, hullmaxs;
				VectorSubtract(touch->v.mins, clip->maxs, hullmins);
				VectorSubtract(touch->v.maxs, clip->mins, hullmaxs);

				if (VectorCompare(vec3_origin, hullmins) && VectorCompare(vec3_origin, hullmaxs))
				{
					// Set predifined size in order to make zero-sized trigger trace-able
					hullmins[0] = -16.f;
					hullmins[1] = -16.f;
					hullmins[2] = -16.f;
					hullmaxs[0] = 16.f;
					hullmaxs[1] = 16.f;
					hullmaxs[2] = 16.f;
				}

				hull = SV_HullForBox(hullmins, hullmaxs);

				VectorCopy(touch->v.origin, offset);
			}

			vec3_t start, end;
			VectorSubtract(clip->start, offset, start);
			VectorSubtract(clip->end, offset, end);

			// trace a line through the apropriate clipping hull
			SV_RecursiveHullCheck(hull, hull->firstclipnode, 0, 1, start, end, &trace);

			// fix trace up by the offset
			if (trace.fraction != 1)
				VectorAdd (trace.endpos, offset, trace.endpos);

			// did we clip the move?
			if (trace.fraction < 1 || trace.startsolid  )
				trace.ent = touch;
		}

		if (trace.allsolid || trace.startsolid || trace.fraction < clip->trace.fraction)
		{
			trace.ent = touch;

			if (clip->trace.startsolid)
			{
				clip->trace = trace;
				clip->trace.startsolid = true;
			}
			else
				clip->trace = trace;
		}
		else if (trace.startsolid)
			clip->trace.startsolid = true;
	}

	// recurse down both sides
	if (node->axis == -1)
		return;

	if (clip->boxmaxs[node->axis] > node->dist)
		ET_TraceTriger(node->children[0], clip);
	if (clip->boxmins[node->axis] < node->dist)
		ET_TraceTriger(node->children[1], clip);
}

static const char *EN_GetClassName(const edict_t *entity)
{
	return PR_GetString(entity->v.classname);
}

static const char *EN_GetNetName(const edict_t *entity)
{
	return LOC_GetString(PR_GetString(entity->v.netname));
}

static const char *EN_GetModel(const edict_t *entity)
{
	return PR_GetString(entity->v.model);
}

static const char *EN_GetFuncName(const edict_t *entity, func_t func)
{
	dfunction_t* function = pr_functions + func;
	return PR_GetString(function->s_name);
}

static const char *EN_GetThinkFunc(const edict_t *entity)
{
	return EN_GetFuncName(entity, entity->v.think);
}

static const char *EN_GetTouchFunc(const edict_t *entity)
{
	return EN_GetFuncName(entity, entity->v.touch);
}

static const char *EN_GetUseFunc(const edict_t *entity)
{
	return EN_GetFuncName(entity, entity->v.use);
}

typedef const char *(*entitynamefunc_t)(const edict_t *entity);

static const entitynamefunc_t en_functions[] =
{
	EN_GetClassName,
	EN_GetNetName,
	EN_GetModel,
	EN_GetThinkFunc,
	EN_GetTouchFunc,
	EN_GetUseFunc,
};

const char* SV_GetEntityName(edict_t* entity)
{
	if (entity->free)
		return "<free>";

	for (size_t i = 0; i < Q_COUNTOF(en_functions); ++i)
	{
		const char *name = en_functions[i](entity);
		if (name[0] != '\0')
			return name;
	}

	return "<unnamed>";
}

static vec_t ET_DistanceToEntity(et_state_t* state, const edict_t* entity, qboolean setend)
{
	vec3_t end;

	for (int i = 0; i < 3; ++i)
		end[i] = entity->v.origin[i] + 0.5 * (entity->v.mins[i] + entity->v.maxs[i]);

	if (setend)
		VectorCopy(end, state->end);

	vec3_t dir;
	VectorSubtract(end, state->start, dir);
	VectorNormalize(dir);

	return DotProduct(dir, state->forward);
}

#define SV_TRACE_ENTITY_SOLID 1
#define SV_TRACE_ENTITY_TRIGGER 2
#define SV_TRACE_ENTITY_ANY (SV_TRACE_ENTITY_SOLID | SV_TRACE_ENTITY_TRIGGER)

edict_t* SV_TraceEntity(int kind)
{
	if (svs.clients == NULL || !svs.clients[0].active)
		return NULL;

	qboolean trace_solid = kind & SV_TRACE_ENTITY_SOLID;
	qboolean trace_trigger = kind & SV_TRACE_ENTITY_TRIGGER;

	if (!trace_solid && !trace_trigger)
		return NULL;

	et_state_t state;
	memset(&state, 0, sizeof state);

	moveclip_t* clip = &state.clip;
	edict_t* player = svs.clients[0].edict;
	clip->passedict = player;

	ET_InitEntityTrace(&state);
	state.initialtrace = clip->trace;

	// Trace solid entity
	edict_t* solid_ent = NULL;

	if (trace_solid)
	{
		SV_ClipToLinks(sv_areanodes, clip);
		solid_ent = clip->trace.ent;

		clip->trace = state.initialtrace;
	}

	// Trace trigger entity
	edict_t* trigger_ent = NULL;

	if (trace_trigger)
	{
		ET_TraceTriger(sv_areanodes, clip);
		trigger_ent = clip->trace.ent;
	}

	// Choose entity, solid or trigger, depending on tracing results and proximity
	edict_t* ent = NULL;

	if (!solid_ent || solid_ent == sv.edicts)
		ent = trigger_ent;
	else if (!trigger_ent || trigger_ent == sv.edicts)
		ent = solid_ent;
	else
	{
		vec_t trigger_dist = ET_DistanceToEntity(&state, trigger_ent, /* setend = */ false);
		vec_t solid_dist = ET_DistanceToEntity(&state, solid_ent, /* setend = */ false);

		ent = trigger_dist > solid_dist ? trigger_ent : solid_ent;
	}

	// Nothing is traced yet, try all possible entities
	if (ent == NULL || ent == sv.edicts)
	{
		float bestdist = 0;
		edict_t* check = NEXT_EDICT(sv.edicts);

		for (int i = 1; i < sv.num_edicts; i++, check = NEXT_EDICT(check) )
		{
			if (check == player)
				continue;

			if (check->v.solid == SOLID_NOT)
				continue;

			float dist = ET_DistanceToEntity(&state, check, /* setend = */ true);
			if (dist < bestdist)
				continue;	// to far to turn

			if (check->v.solid == SOLID_TRIGGER && trace_trigger)
			{
				clip->trace = state.initialtrace;
				ET_TraceTriger(sv_areanodes, clip);
			}
			else if (trace_solid)
			{
				clip->trace = state.initialtrace;
				SV_ClipToLinks(sv_areanodes, clip);
			}

			if (clip->trace.ent == check)
			{
				bestdist = dist;
				ent = check;
			}
		}
	}

	return ent == sv.edicts ? NULL : ent;
}

cvar_t sv_traceentity = { "sv_traceentity", "0", CVAR_NONE };

#define et_infosize 1024
char sv_tracedentityinfo[et_infosize];
static char* et_infoptr;

static inline void entity_sprintf(const char* format, ...)
{
	// One character should reserved for terminating zero, i.e. additional '\0' after the last line's null terminator
	size_t size = et_infosize - (et_infoptr - sv_tracedentityinfo) - 1;
	if (size == 0)
		return;

	va_list argptr;
	va_start(argptr, format);

	et_infoptr += q_vsnprintf(et_infoptr, size, format, argptr);
	et_infoptr++;  // set current position after null terminator

	va_end(argptr);
}


static double et_timesinceupdate;

void SV_ResetTracedEntityInfo(cvar_t *var)
{
	(void)var;

	sv_tracedentityinfo[0] = '\0';
	et_timesinceupdate = 0;
}

void SV_UpdateTracedEntityInfo(void)
{
	if (sv_traceentity.value == 0)
		return;

	if (sv_tracedentityinfo[0] == '\0')
		et_timesinceupdate = DBL_MAX;
	else
		et_timesinceupdate += host_frametime;

	if (et_timesinceupdate < 0.1)
		return;

	SV_ResetTracedEntityInfo(NULL);

	edict_t* ent = SV_TraceEntity(SV_TRACE_ENTITY_ANY);

	if (ent == NULL)
		return;

	const char* name = SV_GetEntityName(ent);

	vec_t* min = ent->v.absmin;
	vec_t* max = ent->v.absmax;

	et_infoptr = sv_tracedentityinfo;

	entity_sprintf("%i: %s", NUM_FOR_EDICT(ent), name);
	entity_sprintf("min: %.0f %.0f %.0f", min[0], min[1], min[2]);
	entity_sprintf("max: %.0f %.0f %.0f", max[0], max[1], max[2]);

	{
		float health = ent->v.health;
		if (health != 0.f)
			entity_sprintf("health: %.0f", health);
	}
	{
		const char *target = PR_GetString(ent->v.target);
		if (target[0] != '\0')
			entity_sprintf("target: %s", target);
	}
	{
		const char *targetname = PR_GetString(ent->v.targetname);
		if (targetname[0] != '\0')
			entity_sprintf("targetname: %s", targetname);
	}
	{
		int spawnflags = ent->v.spawnflags;
		if (spawnflags != 0)
			entity_sprintf("spawnflags: 0x%X", spawnflags);
	}
	{
		int flags = ent->v.flags;
		if (flags != 0)
			entity_sprintf("flags: 0x%X", flags);
	}

	// Terminating zero (additional '\0' after the last line's null terminator) designates end of info
	et_infoptr[0] = '\0';
}

#ifndef NDEBUG

static void DumpAreaNodeEdicts(FILE* f, link_t *edlink, const char* name, int level)
{
	link_t *current = edlink->next;
	link_t *next;

	if (current == edlink)
		return;

	fprintf(f, "%*s%s:\n", level * 2, "", name);
	++level;

	for (/* empty */; current != edlink; current = next)
	{
		if (!current)
		{
			fprintf(f, "%*s- edict: null [BROKEN LINK]\n", level * 2, "");
			return;
		}

		edict_t* ed = EDICT_FROM_AREA(current);
		fprintf(f, "%*s- edict: %i (%p)\n", level * 2, "", NUM_FOR_EDICT(ed), ed);
		{
			const char* classname = PR_GetString(ed->v.classname);
			if (classname[0] != '\0')
				fprintf(f, "%*s  classname: '%s'\n", level * 2, "", classname);
		}
		{
			const char* model = PR_GetString(ed->v.model);
			if (model[0] != '\0')
				fprintf(f, "%*s  model: '%s'\n", level * 2, "", model);
		}
		{
			vec_t* min = ed->v.absmin;
			fprintf(f, "%*s  absmin: [%.1f, %.1f, %.1f]\n", level * 2, "", min[0], min[1], min[2]);
		}
		{
			vec_t* max = ed->v.absmax;
			fprintf(f, "%*s  absmax: [%.1f, %.1f, %.1f]\n", level * 2, "", max[0], max[1], max[2]);
		}
		fprintf(f, "%*s  solid: %i\n", level * 2, "", (int)ed->v.solid);
		{
			int flags = ed->v.flags;
			if (flags != 0)
				fprintf(f, "%*s  flags: 0x%x\n", level * 2, "", flags);
		}

		next = current->next;
	}
}

static void DumpAreaNode(FILE* f, areanode_t* areanode, int level)
{
	fprintf(f, "%*s- node: %p\n", level * 2, "", areanode);
	fprintf(f, "%*s  axis: %i\n", level * 2, "", areanode->axis);
	fprintf(f, "%*s  dist: %.1f\n", level * 2, "", areanode->dist);

	DumpAreaNodeEdicts(f, &areanode->solid_edicts, "solids", level + 1);
	DumpAreaNodeEdicts(f, &areanode->trigger_edicts, "triggers", level + 1);

	if (areanode->axis == -1)
		return;

	fprintf(f, "%*s  children:\n", level * 2, "");
	DumpAreaNode(f, areanode->children[0], level + 2);
	DumpAreaNode(f, areanode->children[1], level + 2);
}

void SV_DumpAreaNodes(void)
{
#if 1
	char nowstr[256];
	struct tm* now = localtime(&(time_t){time(NULL)});
	strftime(nowstr, sizeof nowstr, "%Y-%m-%d_%H-%M-%S", now);

	char fname[1024];
	q_snprintf(fname, sizeof fname, "areanodes_%s_%i.yml", nowstr, host_framecount);
#else
	const char* fname = "areanodes.yml";
#endif

	FILE* f = fopen(fname, "w");
	if (f)
	{
		DumpAreaNode(f, sv_areanodes, 0);
		fclose(f);
	}
}

#endif // !NDEBUG
