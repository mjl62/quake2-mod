/*
Copyright (C) 1997-2001 Id Software, Inc.

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
#include "g_local.h"



/*
======================================================================

INTERMISSION

======================================================================
*/

void MoveClientToIntermission (edict_t *ent)
{
	if (deathmatch->value || coop->value)
		ent->client->showscores = true;
	VectorCopy (level.intermission_origin, ent->s.origin);
	ent->client->ps.pmove.origin[0] = level.intermission_origin[0]*8;
	ent->client->ps.pmove.origin[1] = level.intermission_origin[1]*8;
	ent->client->ps.pmove.origin[2] = level.intermission_origin[2]*8;
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunindex = 0;
	ent->client->ps.blend[3] = 0;
	ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	// clean up powerup info
	ent->client->quad_framenum = 0;
	ent->client->invincible_framenum = 0;
	ent->client->breather_framenum = 0;
	ent->client->enviro_framenum = 0;
	ent->client->grenade_blew_up = false;
	ent->client->grenade_time = 0;

	ent->viewheight = 0;
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.modelindex = 0;
	ent->s.effects = 0;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;

	// add the layout

	if (deathmatch->value || coop->value)
	{
		DeathmatchScoreboardMessage (ent, NULL);
		gi.unicast (ent, true);
	}

}

void BeginIntermission (edict_t *targ)
{
	int		i, n;
	edict_t	*ent, *client;

	if (level.intermissiontime)
		return;		// already activated

	game.autosaved = false;

	// respawn any dead clients
	for (i=0 ; i<maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		if (client->health <= 0)
			respawn(client);
	}

	level.intermissiontime = level.time;
	level.changemap = targ->map;

	if (strstr(level.changemap, "*"))
	{
		if (coop->value)
		{
			for (i=0 ; i<maxclients->value ; i++)
			{
				client = g_edicts + 1 + i;
				if (!client->inuse)
					continue;
				// strip players of all keys between units
				for (n = 0; n < MAX_ITEMS; n++)
				{
					if (itemlist[n].flags & IT_KEY)
						client->client->pers.inventory[n] = 0;
				}
			}
		}
	}
	else
	{
		if (!deathmatch->value)
		{
			level.exitintermission = 1;		// go immediately to the next level
			return;
		}
	}

	level.exitintermission = 0;

	// find an intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if (!ent)
	{	// the map creator forgot to put in an intermission point...
		ent = G_Find (NULL, FOFS(classname), "info_player_start");
		if (!ent)
			ent = G_Find (NULL, FOFS(classname), "info_player_deathmatch");
	}
	else
	{	// chose one of four spots
		i = rand() & 3;
		while (i--)
		{
			ent = G_Find (ent, FOFS(classname), "info_player_intermission");
			if (!ent)	// wrap around the list
				ent = G_Find (ent, FOFS(classname), "info_player_intermission");
		}
	}

	VectorCopy (ent->s.origin, level.intermission_origin);
	VectorCopy (ent->s.angles, level.intermission_angle);

	// move all clients to the intermission point
	for (i=0 ; i<maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		MoveClientToIntermission (client);
	}
}


/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage (edict_t *ent, edict_t *killer)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j, k;
	int		sorted[MAX_CLIENTS];
	int		sortedscores[MAX_CLIENTS];
	int		score, total;
	int		picnum;
	int		x, y;
	gclient_t	*cl;
	edict_t		*cl_ent;
	char	*tag;

	// sort the clients by score
	total = 0;
	for (i=0 ; i<game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || game.clients[i].resp.spectator)
			continue;
		score = game.clients[i].resp.score;
		for (j=0 ; j<total ; j++)
		{
			if (score > sortedscores[j])
				break;
		}
		for (k=total ; k>j ; k--)
		{
			sorted[k] = sorted[k-1];
			sortedscores[k] = sortedscores[k-1];
		}
		sorted[j] = i;
		sortedscores[j] = score;
		total++;
	}

	// print level name and exit rules
	string[0] = 0;

	stringlength = strlen(string);

	// add the clients in sorted order
	if (total > 12)
		total = 12;

	for (i=0 ; i<total ; i++)
	{
		cl = &game.clients[sorted[i]];
		cl_ent = g_edicts + 1 + sorted[i];

		picnum = gi.imageindex ("i_fixme");
		x = (i>=6) ? 160 : 0;
		y = 32 + 32 * (i%6);

		// add a dogtag
		if (cl_ent == ent)
			tag = "tag1";
		else if (cl_ent == killer)
			tag = "tag2";
		else
			tag = NULL;
		if (tag)
		{
			Com_sprintf (entry, sizeof(entry),
				"xv %i yv %i picn %s ",x+32, y, tag);
			j = strlen(entry);
			if (stringlength + j > 1024)
				break;
			strcpy (string + stringlength, entry);
			stringlength += j;
		}

		// send the layout
		Com_sprintf (entry, sizeof(entry),
			"client %i %i %i %i %i %i ",
			x, y, sorted[i], cl->resp.score, cl->ping, (level.framenum - cl->resp.enterframe)/600);
		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
}


/*
==================
DeathmatchScoreboard

Draw instead of help message.
Note that it isn't that hard to overflow the 1400 byte message limit!
==================
*/
void DeathmatchScoreboard (edict_t *ent)
{
	DeathmatchScoreboardMessage (ent, ent->enemy);
	gi.unicast (ent, true);
}


/*
==================
Cmd_Score_f

Display the scoreboard
==================
*/
void Cmd_Score_f (edict_t *ent)
{
	ent->client->showinventory = false;
	ent->client->showhelp = false;
	ent->client->showapplystat = false;

	if (!deathmatch->value && !coop->value)
		return;

	if (ent->client->showscores)
	{
		ent->client->showscores = false;
		return;
	}

	ent->client->showscores = true;
	DeathmatchScoreboard (ent);
}


/*
==================
HelpComputer

Draw help computer.
==================
*/

// Is now questlog
void HelpComputer (edict_t *ent)
{
	char	string[2048]; // was 1024
	char	*sk;

	if (skill->value == 0)
		sk = "easy";
	else if (skill->value == 1)
		sk = "medium";
	else if (skill->value == 2)
		sk = "hard";
	else
		sk = "hard+";

	// Matthew LiDonni
	
	char quest0[128] = "";
	char quest1[128] = "";
	char quest2[128] = "";
	char quest3[128] = "";
	char quest4[128] = "";
	
	char objcurrent[2];
	char objtotal[2];
	if (ent->client->pers.questlog[0].queststarted == true && ent->client->pers.questlog[0].questcompleted != true) {
		strcpy(quest0, ent->client->pers.questlog[0].questname);
		strcat(quest0, ": ");
		strcat(quest0, ent->client->pers.questlog[0].questdesc);
		strcat(quest0, "   ");
		itoa(ent->client->pers.questlog[0].kills, objcurrent, 10);
		strcat(quest0, objcurrent);
		strcat(quest0, "/");
		itoa(ent->client->pers.questlog[0].killsneeded, objtotal, 10);
		strcat(quest0, objtotal);
	}
	if (ent->client->pers.questlog[1].queststarted == true && ent->client->pers.questlog[1].questcompleted != true) {
		strcpy(quest1, ent->client->pers.questlog[1].questname);
		strcat(quest1, ": ");
		strcat(quest1, ent->client->pers.questlog[1].questdesc);
		strcat(quest1, "   ");
		itoa(ent->client->pers.questlog[1].kills, objcurrent, 10);
		strcat(quest1, objcurrent);
		strcat(quest1, "/");
		itoa(ent->client->pers.questlog[1].killsneeded, objtotal, 10);
		strcat(quest1, objtotal);
	}
	if (ent->client->pers.questlog[2].queststarted == true && ent->client->pers.questlog[2].questcompleted != true) {
		strcpy(quest2, ent->client->pers.questlog[2].questname);
		strcat(quest2, ": ");
		strcat(quest2, ent->client->pers.questlog[2].questdesc);
		strcat(quest2, "   ");
		itoa(ent->client->pers.questlog[2].kills, objcurrent, 10);
		strcat(quest2, objcurrent);
		strcat(quest2, "/");
		itoa(ent->client->pers.questlog[2].killsneeded, objtotal, 10);
		strcat(quest2, objtotal);
	}
	if (ent->client->pers.questlog[3].queststarted == true && ent->client->pers.questlog[3].questcompleted != true) {
		strcpy(quest3, ent->client->pers.questlog[3].questname);
		strcat(quest3, ": ");
		strcat(quest3, ent->client->pers.questlog[3].questdesc);
		strcat(quest3, "   ");
		itoa(ent->client->pers.questlog[3].kills, objcurrent, 10);
		strcat(quest3, objcurrent);
		strcat(quest3, "/");
		itoa(ent->client->pers.questlog[3].killsneeded, objtotal, 10);
		strcat(quest3, objtotal);
	}
	if (ent->client->pers.questlog[4].queststarted == true && ent->client->pers.questlog[4].questcompleted != true) {
		strcpy(quest4, ent->client->pers.questlog[4].questname);
		strcat(quest4, ": ");
		strcat(quest4, ent->client->pers.questlog[4].questdesc);
		strcat(quest4, "   ");
		itoa(ent->client->pers.questlog[4].kills, objcurrent, 10);
		strcat(quest4, objcurrent);
		strcat(quest4, "/");
		itoa(ent->client->pers.questlog[4].killsneeded, objtotal, 10);
		strcat(quest4, objtotal);
	}

	// send the layout
	Com_sprintf (string, sizeof(string),
		"" // "was xv 32 yv 8 picn help " 															// background
		//"xv 202 yv 12 string2 \"%s\" "															// skill
		"xv 0 yv 24 cstring2 \"%s\" "																// level name
		"xv 0 yv 48 cstring2 \"%s\" "	// quest 0		//was "xv 0 yv 54 cstring2 \"%s\" "			// help 1
		"xv 0 yv 64 cstring2 \"%s\" "  // quest 1		//was "xv 0 yv 110 cstring2 \"%s\" "		// help 2
		"xv 0 yv 80 cstring2 \"%s\" "  // quest 2		//was "xv 0 yv 110 cstring2 \"%s\" "		// help 2
		"xv 0 yv 96 cstring2 \"%s\" "  // quest 3		//was "xv 0 yv 110 cstring2 \"%s\" "		// help 2
		"xv 0 yv 112 cstring2 \"%s\" "  // quest 4		//was "xv 0 yv 110 cstring2 \"%s\" "		// help 2
		//"xv 50 yv 164 string2 \" kills     goals    secrets\" "			
		//"xv 50 yv 172 string2 \"%3i/%3i     %i/%i       %i/%i\" "
		, 
		//sk,											//was sk
		"Journal:",										//was level.level_name
		quest0,											// was game.helpmessage1					// help 1
		
		quest1,											// was game.helpmessage2					// help 2

		quest2,

		quest3,

		quest4

		//,
		//level.killed_monsters, level.total_monsters,                                                
		//level.found_secrets, level.total_secrets
		);

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
	gi.unicast (ent, true);
}


/*
==================
Cmd_Help_f

Display the current help message
==================
*/
void Cmd_Help_f (edict_t *ent)
{
	// this is for backwards compatability
	if (deathmatch->value)
	{
		Cmd_Score_f (ent);
		return;
	}

	ent->client->showinventory = false;
	ent->client->showscores = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->pers.helpchanged = 0;
	ent->client->showapplystat = false;
	HelpComputer (ent);
}

// Matthew LiDonni
// A helpmenu to show RPG stats
void ShowRPGStats(edict_t* ent)
{
	char	string[2048]; // was 1024
	char* sk;

	if (skill->value == 0)
		sk = "easy";
	else if (skill->value == 1)
		sk = "medium";
	else if (skill->value == 2)
		sk = "hard";
	else
		sk = "hard+";

	// Matthew LiDonni
	char level[32] = "";

	char row1[64] = "";
	char row2[64] = "";
	char row3[64] = "";
	char row4[64] = "";
	char row5[64] = "";
	char row6[64] = "";
	char row7[64] = "";

	char skillLevel[2];
	char characterLevel[2];
	char characterXP[6];

	// Level
	strcpy(level, "Level: ");
	itoa(ent->client->pers.level, characterLevel, 10);
	strcat(level, characterLevel);
	strcat(level, "    XP: ");
	itoa(ent->client->pers.xp, characterXP, 10);
	strcat(level, characterXP);

	// Row 1
	strcpy(row1, "Strength: ");
	itoa(ent->client->pers.stat_strength, skillLevel, 10);
	strcat(row1, skillLevel);
	strcat(row1, "    ");
	strcpy(skillLevel, "");
	strcat(row1, "Intelligence: ");
	itoa(ent->client->pers.stat_intelligence, skillLevel, 10);
	strcat(row1, skillLevel);
	
	// Row 2
	strcpy(row2, "Willpower: ");
	itoa(ent->client->pers.stat_willpower, skillLevel, 10);
	strcat(row2, skillLevel);
	strcat(row2, "    ");
	strcpy(skillLevel, "");
	strcat(row2, "Agility: ");
	itoa(ent->client->pers.stat_agility, skillLevel, 10);
	strcat(row2, skillLevel);

	// Row 3
	strcpy(row3, "Endurance: ");
	itoa(ent->client->pers.stat_endurance, skillLevel, 10);
	strcat(row3, skillLevel);
	strcat(row3, "    ");
	strcpy(skillLevel, "");

	// Row 4
	strcpy(row4, "Blades: ");
	itoa(ent->client->pers.skill_blades, skillLevel, 10);
	strcat(row4, skillLevel);
	strcat(row4, "    ");
	strcpy(skillLevel, "");
	strcat(row4, "Two-Hand: ");
	itoa(ent->client->pers.skill_twohand, skillLevel, 10);
	strcat(row4, skillLevel);

	// Row 5
	strcpy(row5, "Bow: ");
	itoa(ent->client->pers.skill_bow, skillLevel, 10);
	strcat(row5, skillLevel);
	strcat(row5, "    ");
	strcpy(skillLevel, "");
	strcat(row5, "Destruction: ");
	itoa(ent->client->pers.skill_destruction, skillLevel, 10);
	strcat(row5, skillLevel);

	// Row 6
	strcpy(row6, "Restoration: ");
	itoa(ent->client->pers.skill_restoration, skillLevel, 10);
	strcat(row6, skillLevel);
	strcat(row6, "    ");
	strcpy(skillLevel, "");
	strcat(row6, "Alteration: ");
	itoa(ent->client->pers.skill_alteration, skillLevel, 10);
	strcat(row6, skillLevel);

	// Row 7
	strcpy(row7, "Health: ");
	itoa(ent->health, skillLevel, 10);
	strcat(row7, skillLevel);
	strcat(row7, "/");
	itoa(ent->max_health, skillLevel, 10);
	strcat(row7, skillLevel);
	strcat(row7, "    ");
	strcat(row7, "Fatigue: ");
	itoa(ent->client->pers.fatigue, skillLevel, 10);
	strcat(row7, skillLevel);
	strcat(row7, "/");
	itoa(ent->client->pers.max_fatigue, skillLevel, 10);
	strcat(row7, skillLevel);
	strcat(row7, "    ");
	strcat(row7, "Magicka: ");
	itoa(ent->client->pers.magicka, skillLevel, 10);
	strcat(row7, skillLevel);
	strcat(row7, "/");
	itoa(ent->client->pers.max_magicka, skillLevel, 10);
	strcat(row7, skillLevel);

	// send the layout
	Com_sprintf(string, sizeof(string),
		""
		"xv 202 yv 12 string2 \"%s\" " // Level
		"xv 0 yv 24 cstring2 \"%s\" "	// Title 1
		"xv 0 yv 48 cstring2 \"%s\" "	// row 1
		"xv 0 yv 64 cstring2 \"%s\" "  // row 2
		"xv 0 yv 80 cstring2 \"%s\" "  // row 3
		"xv 0 yv 112 cstring2 \"%s\" "  // Title 2
		"xv 0 yv 144 cstring2 \"%s\" "  // row 4
		"xv 0 yv 160 cstring2 \"%s\" "  // row 5
		"xv 0 yv 176 cstring2 \"%s\" "  // row 6
		"xv 0 yv 196 cstring2 \"%s\" " // row 7
		,
		level,
		"Main Attributes:",
		row1,
		row2,
		row3,
		"Skills:",
		row4,
		row5,
		row6,
		row7
		);

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
	gi.unicast(ent, true);
}

void Cmd_ShowRPGStat_f(edict_t* ent)
{
	// this is for backwards compatability
	if (deathmatch->value)
	{
		Cmd_Score_f(ent);
		return;
	}

	ent->client->showinventory = false;
	ent->client->showscores = false;

	// TODO set up my own show command like this
	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->pers.helpchanged = 0;
	ent->client->showapplystat = false;
	ShowRPGStats(ent);
}

void ShowApplyStats(edict_t* ent)
{
	char	string[2048]; // was 1024

	// Matthew LiDonni
	char level[32] = "";

	char row1[64] = "";
	char row2[64] = "";
	char row3[64] = "";
	char row4[64] = "";
	char row5[64] = "";
	char row6[64] = "";
	char row7[64] = "";

	char skillLevel[2];
	char characterLevel[2];
	char characterXP[6];

	// Level
	strcpy(level, "Level: ");
	itoa(ent->client->pers.level, characterLevel, 10);
	strcat(level, characterLevel);
	strcat(level, "    XP: ");
	itoa(ent->client->pers.xp, characterXP, 10);
	strcat(level, characterXP);

	// Row 2
	strcpy(row2, "Strength: ");
	itoa(ent->client->pers.stat_strength, skillLevel, 10);
	strcat(row2, skillLevel);
	
	// Row 3
	strcat(row3, "Intelligence: ");
	itoa(ent->client->pers.stat_intelligence, skillLevel, 10);
	strcat(row3, skillLevel);

	// Row 4
	strcpy(row4, "Willpower: ");
	itoa(ent->client->pers.stat_willpower, skillLevel, 10);
	strcat(row4, skillLevel);

	// Row 5
	strcpy(row5, "Agility: ");
	itoa(ent->client->pers.stat_agility, skillLevel, 10);
	strcat(row5, skillLevel);

	// Row 6
	strcpy(row6, "Endurance: ");
	itoa(ent->client->pers.stat_endurance, skillLevel, 10);
	strcat(row6, skillLevel);
	 
	// Row 7
	strcpy(row7, "Health: ");
	itoa(ent->health, skillLevel, 10);
	strcat(row7, skillLevel);
	strcat(row7, "/");
	itoa(ent->max_health, skillLevel, 10);
	strcat(row7, skillLevel);
	strcat(row7, "    ");
	strcat(row7, "Fatigue: ");
	itoa(ent->client->pers.fatigue, skillLevel, 10);
	strcat(row7, skillLevel);
	strcat(row7, "/");
	itoa(ent->client->pers.max_fatigue, skillLevel, 10);
	strcat(row7, skillLevel);
	strcat(row7, "    ");
	strcat(row7, "Magicka: ");
	itoa(ent->client->pers.magicka, skillLevel, 10);
	strcat(row7, skillLevel);
	strcat(row7, "/");
	itoa(ent->client->pers.max_magicka, skillLevel, 10);
	strcat(row7, skillLevel);

	// Row 1 Do this here so we can add button prompts if needed
	if (ent->client->pers.statpoints > 0) {
		strcpy(row1, "Unspent Points: ");
		itoa(ent->client->pers.statpoints, skillLevel, 10);
		strcat(row1, skillLevel);
		strcat(row2, "    Press F8 to apply points");
		strcat(row3, "    Press F9 to apply points");
		strcat(row4, "    Press F10 to apply points");
		strcat(row5, "    Press F11 to apply points");
		strcat(row6, "    Press F12 to apply points");
	}

	// send the layout
	Com_sprintf(string, sizeof(string),
		""
		"xv 202 yv 12 string2 \"%s\" " // Level   XP
		"xv 0 yv 24 cstring2 \"%s\" "	// Points to Apply
		"xv 0 yv 54 cstring2 \"%s\" "	// row 1 - Unspent Points
		"xv 0 yv 74 cstring2 \"%s\" "  // row 2 - Strength
		"xv 0 yv 94 cstring2 \"%s\" "  // row 3 - Intelligence
		"xv 0 yv 114 cstring2 \"%s\" "  // row 4 - Willpower
		"xv 0 yv 134 cstring2 \"%s\" "  // row 5 - Agility
		"xv 0 yv 154 cstring2 \"%s\" "  // row 6 - Endurance
		"xv 0 yv 174 cstring2 \"%s\" "  // row 7 - Health/Fatigue/Magicka
		"xv 0 yv 194 cstring2 \"%s\" " 
		,
		level,
		"",
		row1,
		row2,
		row3,
		row4,
		row5,
		row6,
		row7
	);

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
	gi.unicast(ent, true);
}

void Cmd_ShowApplyStats_f(edict_t* ent)
{
	// this is for backwards compatability
	if (deathmatch->value)
	{
		Cmd_Score_f(ent);
		return;
	}

	ent->client->showinventory = false;
	ent->client->showscores = false;
	ent->client->showapplystat = true;

	// TODO set up my own show command like this
	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->pers.helpchanged = 0;
	ShowApplyStats(ent);
}

// RPG Inventory
// A helpmenu to show RPG stats
void ShowRPGInventory(edict_t* ent)
{
	char	string[2048]; // was 1024
	
	// Matthew LiDonni
	//GetRPGItemName(ent, ent->client->pers.rpgInventory[0])
	char rows[16][32] = {""};

	char* equipTag = " (Equipped)";

	for (int i = 0; i < 16; i++) {
		if (ent->client->pers.rpgCursorLocation == i) {
			strcpy(rows[i], ">");
			strcat(rows[i], GetRPGItemName(ent, ent->client->pers.rpgInventory[i]));
			
			// THIS IS THE LEAST EFFICIENT WAY TO DO THIS BUT I DONT WANT TO DIG THROUGH THE CODE AND CHANGE ALL THE ARMOR VALUES TO MAKE THIS WORK
			if (ent->client->pers.rpgArmorValues[0] > 0) {
				if (ent->client->pers.rpgInventory[i] == 7) {
					strcat(rows[i], equipTag);
				}
			}
			if (ent->client->pers.rpgArmorValues[1] > 0) {
				if (ent->client->pers.rpgInventory[i] == 6) {
					strcat(rows[i], equipTag);
				}
			}
			if (ent->client->pers.rpgArmorValues[2] > 0) {
				if (ent->client->pers.rpgInventory[i] == 8) {
					strcat(rows[i], equipTag);
				}
			}
			strcat(rows[i], "<");
			continue;
		}
		strcpy(rows[i], GetRPGItemName(ent, ent->client->pers.rpgInventory[i]));
		// THIS IS THE LEAST EFFICIENT WAY TO DO THIS BUT I DONT WANT TO DIG THROUGH THE CODE AND CHANGE ALL THE ARMOR VALUES TO MAKE THIS WORK
		if (ent->client->pers.rpgArmorValues[0] > 0) {
			if (ent->client->pers.rpgInventory[i] == 7) {
				strcat(rows[i], equipTag);
			}
		}
		if (ent->client->pers.rpgArmorValues[1] > 0) {
			if (ent->client->pers.rpgInventory[i] == 6) {
				strcat(rows[i], equipTag);
			}
		}
		if (ent->client->pers.rpgArmorValues[2] > 0) {
			if (ent->client->pers.rpgInventory[i] == 8) {
				strcat(rows[i], equipTag);
			}
		}
	}
	


	// send the layout
	Com_sprintf(string, sizeof(string),
		""
		"xv 0 yv 12 cstring2 \"%s\" " // Bag
		"xv 0 yv 24 cstring2 \"%s\" "	// --------
		"xv 0 yv 36 cstring2 \"%s\" "	// rows[0]
		"xv 0 yv 48 cstring2 \"%s\" "  // rows[1]
		"xv 0 yv 60 cstring2 \"%s\" "  // rows[2]
		"xv 0 yv 72 cstring2 \"%s\" "  // rows[3]
		"xv 0 yv 84 cstring2 \"%s\" "  // rows[4]
		"xv 0 yv 96 cstring2 \"%s\" "  // rows[5]
		"xv 0 yv 108 cstring2 \"%s\" "  // rows[6]
		"xv 0 yv 120 cstring2 \"%s\" " // rows[7]
		"xv 0 yv 132 cstring2 \"%s\" " // rows[8]
		"xv 0 yv 144 cstring2 \"%s\" " // rows[9]
		"xv 0 yv 156 cstring2 \"%s\" " // rows[10]
		"xv 0 yv 168 cstring2 \"%s\" " // rows[11]
		"xv 0 yv 180 cstring2 \"%s\" " // rows[12]
		"xv 0 yv 192 cstring2 \"%s\" " // rows[13]
		"xv 0 yv 204 cstring2 \"%s\" " // rows[14]
		"xv 0 yv 216 cstring2 \"%s\" " // rows[15]
		,
		"Bag",
		"------------------",
		rows[0],
		rows[1],
		rows[2],
		rows[3],
		rows[4],
		rows[5],
		rows[6],
		rows[7],
		rows[8],
		rows[9],
		rows[10],
		rows[11],
		rows[12],
		rows[13],
		rows[14],
		rows[15]
	);

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
	gi.unicast(ent, true);
}

void Cmd_ShowRPGInventory_f(edict_t* ent)
{
	// this is for backwards compatability
	if (deathmatch->value)
	{
		Cmd_Score_f(ent);
		return;
	}

	ent->client->showinventory = false;
	ent->client->showscores = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->pers.helpchanged = 0;
	ent->client->showapplystat = false;

	ShowRPGInventory(ent);
}

// A helpmenu to show RPG stats
void ShowRPGHelp(edict_t* ent)
{
	char	string[2048]; // was 1024

	// send the layout
	Com_sprintf(string, sizeof(string),
		""
		"xv 202 yv 12 string2 \"%s\" " // Level
		"xv 0 yv 24 cstring2 \"%s\" "	// Title 1
		"xv 0 yv 48 cstring2 \"%s\" "	// row 1
		"xv 0 yv 64 cstring2 \"%s\" "  // row 2
		"xv 0 yv 80 cstring2 \"%s\" "  // row 3
		"xv 0 yv 112 cstring2 \"%s\" "  // Title 2
		"xv 0 yv 144 cstring2 \"%s\" "  // row 4
		"xv 0 yv 160 cstring2 \"%s\" "  // row 5
		"xv 0 yv 176 cstring2 \"%s\" "  // row 6
		"xv 0 yv 196 cstring2 \"%s\" " // row 7
		,
		"Help:",
		"Controls: F1: Help   I: Inventory   Up/DownArrow: Navigate Inventory   RightArrow: Use Inventory Item",
		"J: Journal   K: Skills   L: Spend Skill Points   F: Sheathe/Draw Weapon   Click (While Sheathed): Interact",
		"You are an adventurer starting from nothing.",
		"Stats are core attributes and are a large part of your build. Skills affect certain weapon classes.",
		"Stats can be upgraded with points but to gain skill with a weapon you must use it in battle.",
		"When sheathed, you can interact with strangers to speak with them, some have",
		"quests that reward experience and items! Check your journal to see your current quests.",
		"In combat, you may not land hits, and you might suck. But practice makes perfect. Good Luck.",
		""
	);

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
	gi.unicast(ent, true);
}

void Cmd_ShowRPGHelp_f(edict_t* ent)
{
	// this is for backwards compatability
	if (deathmatch->value)
	{
		Cmd_Score_f(ent);
		return;
	}

	ent->client->showinventory = false;
	ent->client->showscores = false;

	// TODO set up my own show command like this
	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->pers.helpchanged = 0;
	ent->client->showapplystat = false;
	ShowRPGHelp(ent);
}


//=======================================================================

/*
===============
G_SetStats
===============
*/
void G_SetStats (edict_t *ent)
{
	gitem_t		*item;
	int			index, cells;
	int			power_armor_type;

	//
	// health
	//
	ent->client->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
	ent->client->ps.stats[STAT_HEALTH] = ent->health;

	//
	// ammo
	//
	/* || !ent->client->pers.inventory[ent->client->ammo_index]  This was in THERE V after ent->client->ammo_index but before the ) for some reason...*/
	
	// fatigue (No more ammo get fucked, idiots)
	item = &itemlist[ent->client->ammo_index];
	ent->client->ps.stats[STAT_AMMO_ICON] = level.pic_fatigue;
	ent->client->ps.stats[STAT_AMMO] = ent->client->pers.fatigue;
	

	// magicka (No armor pickup display)
	ent->client->ps.stats[STAT_ARMOR_ICON] = level.pic_magicka;
	ent->client->ps.stats[STAT_ARMOR] = ent->client->pers.magicka;
	/*
	if (!ent->client->ammo_index )
	{
		ent->client->ps.stats[STAT_AMMO_ICON] = 0;
		ent->client->ps.stats[STAT_AMMO] = 0;
	}
	else
	{
		item = &itemlist[ent->client->ammo_index];
		ent->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex (item->icon);
		ent->client->ps.stats[STAT_AMMO] = ent->client->pers.inventory[ent->client->ammo_index];
	}
	*/
	
	//
	// armor
	//
	/*
	power_armor_type = PowerArmorType (ent);
	if (power_armor_type)
	{
		cells = ent->client->pers.inventory[ITEM_INDEX(FindItem ("cells"))];
		if (cells == 0)
		{	// ran out of cells for power armor
			ent->flags &= ~FL_POWER_ARMOR;
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
			power_armor_type = 0;;
		}
	}
	
	index = ArmorIndex (ent);
	if (power_armor_type && (!index || (level.framenum & 8) ) )
	{	// flash between power armor and other armor icon
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex ("i_powershield");
		ent->client->ps.stats[STAT_ARMOR] = cells;
	}
	else if (index)
	{
		item = GetItemByIndex (index);
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex (item->icon);
		ent->client->ps.stats[STAT_ARMOR] = ent->client->pers.inventory[index];
	}
	else
	{
		ent->client->ps.stats[STAT_ARMOR_ICON] = 0;
		ent->client->ps.stats[STAT_ARMOR] = 0;
	}

	*/

	//
	// pickup message
	//
	if (level.time > ent->client->pickup_msg_time)
	{
		ent->client->ps.stats[STAT_PICKUP_ICON] = 0;
		ent->client->ps.stats[STAT_PICKUP_STRING] = 0;
	}

	//
	// timers
	//
	if (ent->client->quad_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_quad");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->quad_framenum - level.framenum)/10;
	}
	else if (ent->client->invincible_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_invulnerability");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->invincible_framenum - level.framenum)/10;
	}
	else if (ent->client->enviro_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_envirosuit");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->enviro_framenum - level.framenum)/10;
	}
	else if (ent->client->breather_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_rebreather");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->breather_framenum - level.framenum)/10;
	}
	else
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = 0;
		ent->client->ps.stats[STAT_TIMER] = 0;
	}

	//
	// selected item
	//
	if (ent->client->pers.selected_item == -1)
		ent->client->ps.stats[STAT_SELECTED_ICON] = 0;
	else
		ent->client->ps.stats[STAT_SELECTED_ICON] = gi.imageindex (itemlist[ent->client->pers.selected_item].icon);

	ent->client->ps.stats[STAT_SELECTED_ITEM] = ent->client->pers.selected_item;

	//
	// layouts
	//
	ent->client->ps.stats[STAT_LAYOUTS] = 0;

	if (deathmatch->value)
	{
		if (ent->client->pers.health <= 0 || level.intermissiontime
			|| ent->client->showscores)
			ent->client->ps.stats[STAT_LAYOUTS] |= 1;
		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= 2;
	}
	else
	{
		if (ent->client->showscores || ent->client->showhelp)
			ent->client->ps.stats[STAT_LAYOUTS] |= 1;
		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= 2;
	}

	//
	// frags
	//
	ent->client->ps.stats[STAT_FRAGS] = ent->client->resp.score;

	//
	// help icon / current weapon if not shown
	//
	if (ent->client->pers.helpchanged && (level.framenum&8) )
		ent->client->ps.stats[STAT_HELPICON] = gi.imageindex ("i_help");
	else if ( (ent->client->pers.hand == CENTER_HANDED || ent->client->ps.fov > 91)
		&& ent->client->pers.weapon)
		ent->client->ps.stats[STAT_HELPICON] = gi.imageindex (ent->client->pers.weapon->icon);
	else
		ent->client->ps.stats[STAT_HELPICON] = 0;

	ent->client->ps.stats[STAT_SPECTATOR] = 0;
}

/*
===============
G_CheckChaseStats
===============
*/
void G_CheckChaseStats (edict_t *ent)
{
	int i;
	gclient_t *cl;

	for (i = 1; i <= maxclients->value; i++) {
		cl = g_edicts[i].client;
		if (!g_edicts[i].inuse || cl->chase_target != ent)
			continue;
		memcpy(cl->ps.stats, ent->client->ps.stats, sizeof(cl->ps.stats));
		G_SetSpectatorStats(g_edicts + i);
	}
}

/*
===============
G_SetSpectatorStats
===============
*/
void G_SetSpectatorStats (edict_t *ent)
{
	gclient_t *cl = ent->client;

	if (!cl->chase_target)
		G_SetStats (ent);

	cl->ps.stats[STAT_SPECTATOR] = 1;

	// layouts are independant in spectator
	cl->ps.stats[STAT_LAYOUTS] = 0;
	if (cl->pers.health <= 0 || level.intermissiontime || cl->showscores)
		cl->ps.stats[STAT_LAYOUTS] |= 1;
	if (cl->showinventory && cl->pers.health > 0)
		cl->ps.stats[STAT_LAYOUTS] |= 2;

	if (cl->chase_target && cl->chase_target->inuse)
		cl->ps.stats[STAT_CHASE] = CS_PLAYERSKINS + 
			(cl->chase_target - g_edicts) - 1;
	else
		cl->ps.stats[STAT_CHASE] = 0;
}

