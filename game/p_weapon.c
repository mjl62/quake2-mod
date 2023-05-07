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
// g_weapon.c

#include "g_local.h"
#include "m_player.h"
#include "m_questgiver.h"


static qboolean	is_quad;
static byte		is_silenced;


void weapon_grenade_fire (edict_t *ent, qboolean held);


static void P_ProjectSource (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t	_distance;

	VectorCopy (distance, _distance);
	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource (point, _distance, forward, right, result);
}


/*
===============
PlayerNoise

Each player can have two noise objects associated with it:
a personal noise (jumping, pain, weapon firing), and a weapon
target noise (bullet wall impacts)

Monsters that don't directly see the player can move
to a noise in hopes of seeing the player from there.
===============
*/
void PlayerNoise(edict_t *who, vec3_t where, int type)
{
	edict_t		*noise;

	if (type == PNOISE_WEAPON)
	{
		if (who->client->silencer_shots)
		{
			who->client->silencer_shots--;
			return;
		}
	}

	if (deathmatch->value)
		return;

	if (who->flags & FL_NOTARGET)
		return;


	if (!who->mynoise)
	{
		noise = G_Spawn();
		noise->classname = "player_noise";
		VectorSet (noise->mins, -8, -8, -8);
		VectorSet (noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise = noise;

		noise = G_Spawn();
		noise->classname = "player_noise";
		VectorSet (noise->mins, -8, -8, -8);
		VectorSet (noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise2 = noise;
	}

	if (type == PNOISE_SELF || type == PNOISE_WEAPON)
	{
		noise = who->mynoise;
		level.sound_entity = noise;
		level.sound_entity_framenum = level.framenum;
	}
	else // type == PNOISE_IMPACT
	{
		noise = who->mynoise2;
		level.sound2_entity = noise;
		level.sound2_entity_framenum = level.framenum;
	}

	VectorCopy (where, noise->s.origin);
	VectorSubtract (where, noise->maxs, noise->absmin);
	VectorAdd (where, noise->maxs, noise->absmax);
	noise->teleport_time = level.time;
	gi.linkentity (noise);
}


qboolean Pickup_Weapon (edict_t *ent, edict_t *other)
{
	int			index;
	gitem_t		*ammo;

	index = ITEM_INDEX(ent->item);

	if ( ( ((int)(dmflags->value) & DF_WEAPONS_STAY) || coop->value) 
		&& other->client->pers.inventory[index])
	{
		if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM) ) )
			return false;	// leave the weapon for others to pickup
	}

	other->client->pers.inventory[index]++;

	if (!(ent->spawnflags & DROPPED_ITEM) )
	{
		// give them some ammo with it
		ammo = FindItem (ent->item->ammo);
		if ( (int)dmflags->value & DF_INFINITE_AMMO )
			Add_Ammo (other, ammo, 1000);
		else
			Add_Ammo (other, ammo, ammo->quantity);

		if (! (ent->spawnflags & DROPPED_PLAYER_ITEM) )
		{
			if (deathmatch->value)
			{
				if ((int)(dmflags->value) & DF_WEAPONS_STAY)
					ent->flags |= FL_RESPAWN;
				else
					SetRespawn (ent, 30);
			}
			if (coop->value)
				ent->flags |= FL_RESPAWN;
		}
	}

	if (other->client->pers.weapon != ent->item && 
		(other->client->pers.inventory[index] == 1) &&
		( !deathmatch->value || other->client->pers.weapon == FindItem("blaster") ) )
		other->client->newweapon = ent->item;

	return true;
}


/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void ChangeWeapon (edict_t *ent)
{
	int i;

	if (ent->client->grenade_time)
	{
		ent->client->grenade_time = level.time;
		ent->client->weapon_sound = 0;
		weapon_grenade_fire (ent, false);
		ent->client->grenade_time = 0;
	}

	ent->client->pers.lastweapon = ent->client->pers.weapon;
	ent->client->pers.weapon = ent->client->newweapon;
	ent->client->newweapon = NULL;
	ent->client->machinegun_shots = 0;

	// set visible model
	if (ent->s.modelindex == 255) {
		if (ent->client->pers.weapon)
			i = ((ent->client->pers.weapon->weapmodel & 0xff) << 8);
		else
			i = 0;
		ent->s.skinnum = (ent - g_edicts - 1) | i;
	}

	if (ent->client->pers.weapon && ent->client->pers.weapon->ammo)
		ent->client->ammo_index = ITEM_INDEX(FindItem(ent->client->pers.weapon->ammo));
	else
		ent->client->ammo_index = 0;

	if (!ent->client->pers.weapon)
	{	// dead
		ent->client->ps.gunindex = 0;
		return;
	}

	ent->client->weaponstate = WEAPON_ACTIVATING;
	ent->client->ps.gunframe = 0;
	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);

	ent->client->anim_priority = ANIM_PAIN;
	if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
			ent->s.frame = FRAME_crpain1;
			ent->client->anim_end = FRAME_crpain4;
	}
	else
	{
			ent->s.frame = FRAME_pain301;
			ent->client->anim_end = FRAME_pain304;
			
	}
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange (edict_t *ent)
{
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("slugs"))]
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("railgun"))] )
	{
		ent->client->newweapon = FindItem ("railgun");
		return;
	}
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("cells"))]
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("hyperblaster"))] )
	{
		ent->client->newweapon = FindItem ("hyperblaster");
		return;
	}
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("chaingun"))] )
	{
		ent->client->newweapon = FindItem ("chaingun");
		return;
	}
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("machinegun"))] )
	{
		ent->client->newweapon = FindItem ("machinegun");
		return;
	}
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("shells"))] > 1
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("super shotgun"))] )
	{
		ent->client->newweapon = FindItem ("super shotgun");
		return;
	}
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("shells"))]
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("shotgun"))] )
	{
		ent->client->newweapon = FindItem ("shotgun");
		return;
	}
	ent->client->newweapon = FindItem ("blaster");
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon (edict_t *ent)
{
	// if just died, put the weapon away
	if (ent->health < 1)
	{
		ent->client->newweapon = NULL;
		ChangeWeapon (ent);
	}

	// call active weapon think routine
	if (ent->client->pers.weapon && ent->client->pers.weapon->weaponthink)
	{
		is_quad = (ent->client->quad_framenum > level.framenum);
		if (ent->client->silencer_shots)
			is_silenced = MZ_SILENCED;
		else
			is_silenced = 0;
		ent->client->pers.weapon->weaponthink (ent);
	}
}


/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon (edict_t *ent, gitem_t *item)
{
	int			ammo_index;
	gitem_t		*ammo_item;

	// see if we're already using it
	if (item == ent->client->pers.weapon)
		return;

	if (item->ammo && !g_select_empty->value && !(item->flags & IT_AMMO))
	{
		ammo_item = FindItem(item->ammo);
		ammo_index = ITEM_INDEX(ammo_item);

		if (!ent->client->pers.inventory[ammo_index])
		{
			gi.cprintf (ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}

		if (ent->client->pers.inventory[ammo_index] < item->quantity)
		{
			gi.cprintf (ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}
	}

	// change to this weapon when down
	ent->client->newweapon = item;
}



/*
================
Drop_Weapon
================
*/
void Drop_Weapon (edict_t *ent, gitem_t *item)
{
	int		index;

	if ((int)(dmflags->value) & DF_WEAPONS_STAY)
		return;

	index = ITEM_INDEX(item);
	// see if we're already using it
	if ( ((item == ent->client->pers.weapon) || (item == ent->client->newweapon))&& (ent->client->pers.inventory[index] == 1) )
	{
		gi.cprintf (ent, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	Drop_Item (ent, item);
	ent->client->pers.inventory[index]--;
}

int getSkillReq(edict_t *ent, char *name) {
	if (name == "weapon_shotgun" || name == "weapon_machinegun") {
		return WEPSKILL_BLADES;
	}
	if (Q_stricmp(name, "weapon_supershotgun") == 0 || Q_stricmp(name, "weapon_chaingun") == 0) {
		return WEPSKILL_TWOHAND;
	}
	if (name == "weapon_blaster") {
		return WEPSKILL_BOW;
	}
	if (name == "weapon_rocketlauncher" || name == "weapon_hyperblaster") {
		return WEPSKILL_DESTRUCTION;
	}
	if (name == "weapon_grenadelauncher" || name == "weapon_railgun") {
		return WEPSKILL_RESTORATION;
	}
	if (name == "weapon_bfg") {
		return WEPSKILL_ALTERATION;
	}
	else {
		gi.cprintf(ent, PRINT_HIGH, "WEAPON CLASS NOT FOUND");
		return -1;
	}
}

void Interact(edict_t* ent, vec3_t start, vec3_t aimdir) {

	trace_t Interact;
	vec3_t		dir;
	vec3_t		forward, right, up;
	vec3_t		end;

	Interact = gi.trace(ent->s.origin, NULL, NULL, start, ent, CONTENTS_MONSTER);
	if (Interact.fraction != 1.0 && Interact.ent->classname == "questgiver") {

		vectoangles(aimdir, dir);
		AngleVectors(dir, forward, right, up);
		VectorMA(start, 60, forward, end);

		if (ent->client->pers.questlog[Interact.ent->questNum].queststarted != true) {
			ent->client->pers.questlog[Interact.ent->questNum] = getQuest(Interact.ent->questNum);
			gi.bprintf(PRINT_CHAT, ent->client->pers.questlog[Interact.ent->questNum].introdiag);

		}
		else if (ent->client->pers.questlog[Interact.ent->questNum].questcompleted == false) {

			if (ent->client->pers.questlog[Interact.ent->questNum].kills >= ent->client->pers.questlog[Interact.ent->questNum].killsneeded) 
			{
				grantXP(ent, ent->client->pers.questlog[Interact.ent->questNum].rewardXP);
				for (int i = 0; i < ent->client->pers.questlog[Interact.ent->questNum].rewardQuantity; i++) {
					AddRPGItem(ent, ent->client->pers.questlog[Interact.ent->questNum].rewardItem);
				}
				gi.bprintf(PRINT_CHAT, ent->client->pers.questlog[Interact.ent->questNum].completediag);
				ent->client->pers.questlog[Interact.ent->questNum].questcompleted = true;
			}
			else {
				gi.bprintf(PRINT_CHAT, ent->client->pers.questlog[Interact.ent->questNum].inprogressdiag);
			}
		}
		else if (ent->client->pers.questlog[Interact.ent->questNum].questcompleted == true) {
			gi.bprintf(PRINT_CHAT, ent->client->pers.questlog[Interact.ent->questNum].postdiag);
		}
		else {
			gi.bprintf(PRINT_CHAT, "Something went wrong with the quest interaction...\n");
		}
		return;
	}
	if (Interact.fraction != 1.0 && Interact.ent->classname == "questitem") {
		ent->client->pers.questlog[Interact.ent->questNum].kills += 1;
		G_FreeEdict(Interact.ent);
		return;
	}
	if (Interact.fraction != 1.0 && Interact.ent->classname == "mailrecipient") {
		
		if (ent->client->pers.questlog[2].queststarted == true && ent->client->pers.questlog[2].questcompleted == false) {
			if (ent->client->pers.questlog[2].kills == 0) {
				gi.bprintf(PRINT_CHAT, "What's this? Ah, tell 'im I said thanks.\n");
				ent->client->pers.questlog[2].kills = 1;
				return;
			}
			gi.bprintf(PRINT_CHAT, "That all? Ok... so why are ya still here? I got nothin' for ya.\n");
		}
		else if (ent->client->pers.questlog[2].questcompleted == true) {
			gi.bprintf(PRINT_CHAT, "So how'dya like bein a mailman? Whaddaya mean? You deliver mail, of course ya'are!\n");
		}
		else {
			gi.bprintf(PRINT_CHAT, "Do I know ya? No? Then quit starin'!\n");
		}
		return;
	}
}

void Melee(edict_t* ent, vec3_t start, vec3_t aimdir, int damage, int mod) {

	trace_t		Swing;
	vec3_t		dir;
	vec3_t		forward, right, up;
	vec3_t		end;

	Swing = gi.trace(ent->s.origin, NULL, NULL, start, ent, CONTENTS_MONSTER);
	if (!(Swing.fraction < 1.0))
	{
		vectoangles(aimdir, dir);
		AngleVectors(dir, forward, right, up);
		VectorMA(start, 70, forward, end); // was 8192

		Swing = gi.trace(start, NULL, NULL, end, ent, CONTENTS_MONSTER);
		if (Swing.ent->classname != "questgiver" && Q_strcasecmp(Swing.ent->classname, "WORLDSPAWN")) {
			float roll = crandom();
			if (roll < 0) {
				roll *= -1;
			}
			roll *= 100; // Now our numbers are integers as percents

			// Calculate chance to hit
			// Morrowind does it like this:
			// (Weapon Skill + (Agility / 5) + (Luck / 10) * (0.75 + 0.5 * Current Fatigue / Maximum Fatigue) + Fortify Attack Magnitude)
			float hitrate = (GetWeaponSkill(ent, getSkillReq(ent, ent->client->pers.weapon->classname)) * 2 + 10 + GetLevelOf(ent, SKILL_AGILITY) / 2.0) * (0.75 + 0.5 * ent->client->pers.fatigue / ent->client->pers.max_fatigue);
			if (hitrate > roll) {

				damage += GetLevelOf(ent, SKILL_STRENGTH) / 2;

				gi.centerprintf(ent, "");
				T_Damage(Swing.ent, ent, ent, aimdir, Swing.endpos, Swing.plane.normal, damage, 2, DAMAGE_BULLET, mod);
				
				// Hit sound
				//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
				
			}
			else {
				gi.centerprintf(ent, "Miss!");
				// Whoosh sound
				//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
			}
			char out[6];
			itoa(hitrate, out, 10);
			gi.cprintf(ent, PRINT_HIGH, out);
			gi.cprintf(ent, PRINT_HIGH, "/");
			itoa(roll, out, 10);
			gi.cprintf(ent, PRINT_HIGH, out);
			gi.cprintf(ent, PRINT_HIGH, "\n");
		}
	}
	if (ent->client->pers.fatigue >= 5) {
		ent->client->pers.fatigue -= 5;
	}
	else {
		ent->client->pers.fatigue = 0;
	}
}



/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
#define FRAME_FIRE_FIRST		(FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST		(FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST	(FRAME_IDLE_LAST + 1)

void Weapon_Generic (edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, int *pause_frames, int *fire_frames, void (*fire)(edict_t *ent))
{
	int		n;

	

	if(ent->deadflag || ent->s.modelindex != 255) // VWep animations screw up corpses
	{
		return;
	}

	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
		{
			ChangeWeapon (ent);
			return;
		}
		else if ((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4)
		{
			ent->client->anim_priority = ANIM_REVERSE;
			if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4+1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304+1;
				ent->client->anim_end = FRAME_pain301;
				
			}
		}

		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING))
	{
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;

		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
		{
			ent->client->anim_priority = ANIM_REVERSE;
			if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4+1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304+1;
				ent->client->anim_end = FRAME_pain301;
				
			}
		}
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) )
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if ((!ent->client->ammo_index) || 
				( ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity))
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;

				// start the animation
				ent->client->anim_priority = ANIM_ATTACK;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crattak1-1;
					ent->client->anim_end = FRAME_crattak9;
				}
				else
				{
					ent->s.frame = FRAME_attack1-1;
					ent->client->anim_end = FRAME_attack8;
				}
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
		}
		else
		{
			if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
			{
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			if (pause_frames)
			{
				for (n = 0; pause_frames[n]; n++)
				{
					if (ent->client->ps.gunframe == pause_frames[n])
					{
						if (rand()&15)
							return;
					}
				}
			}

			ent->client->ps.gunframe++;
			return;
		}
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		for (n = 0; fire_frames[n]; n++)
		{
			if (ent->client->ps.gunframe == fire_frames[n])
			{
				if (ent->client->quad_framenum > level.framenum)
					gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

				fire (ent);
				break;
			}
		}

		if (!fire_frames[n])
			ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST+1)
			ent->client->weaponstate = WEAPON_READY;
	}
}


/*
======================================================================

GRENADE

======================================================================
*/

#define GRENADE_TIMER		3.0
#define GRENADE_MINSPEED	400
#define GRENADE_MAXSPEED	800

void weapon_grenade_fire (edict_t *ent, qboolean held)
{
	vec3_t	offset;
	vec3_t	forward, right;
	vec3_t	start;
	int		damage = 125;
	float	timer;
	int		speed;
	float	radius;

	radius = damage+40;
	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	timer = ent->client->grenade_time - level.time;
	speed = GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER);
	fire_grenade2 (ent, start, forward, damage, speed, timer, radius, held);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index]--;

	ent->client->grenade_time = level.time + 1.0;

	if(ent->deadflag || ent->s.modelindex != 255) // VWep animations screw up corpses
	{
		return;
	}

	if (ent->health <= 0)
		return;

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = FRAME_crattak1-1;
		ent->client->anim_end = FRAME_crattak3;
	}
	else
	{
		ent->client->anim_priority = ANIM_REVERSE;
		ent->s.frame = FRAME_wave08;
		ent->client->anim_end = FRAME_wave01;
	}
}

void Weapon_Grenade (edict_t *ent)
{
	if ((ent->client->newweapon) && (ent->client->weaponstate == WEAPON_READY))
	{
		ChangeWeapon (ent);
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		ent->client->weaponstate = WEAPON_READY;
		ent->client->ps.gunframe = 16;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) )
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if (ent->client->pers.inventory[ent->client->ammo_index])
			{
				ent->client->ps.gunframe = 1;
				ent->client->weaponstate = WEAPON_FIRING;
				ent->client->grenade_time = 0;
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
			return;
		}

		if ((ent->client->ps.gunframe == 29) || (ent->client->ps.gunframe == 34) || (ent->client->ps.gunframe == 39) || (ent->client->ps.gunframe == 48))
		{
			if (rand()&15)
				return;
		}

		if (++ent->client->ps.gunframe > 48)
			ent->client->ps.gunframe = 16;
		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		if (ent->client->ps.gunframe == 5)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1, ATTN_NORM, 0);

		if (ent->client->ps.gunframe == 11)
		{
			if (!ent->client->grenade_time)
			{
				ent->client->grenade_time = level.time + GRENADE_TIMER + 0.2;
				ent->client->weapon_sound = gi.soundindex("weapons/hgrenc1b.wav");
			}

			// they waited too long, detonate it in their hand
			if (!ent->client->grenade_blew_up && level.time >= ent->client->grenade_time)
			{
				ent->client->weapon_sound = 0;
				weapon_grenade_fire (ent, true);
				ent->client->grenade_blew_up = true;
			}

			if (ent->client->buttons & BUTTON_ATTACK)
				return;

			if (ent->client->grenade_blew_up)
			{
				if (level.time >= ent->client->grenade_time)
				{
					ent->client->ps.gunframe = 15;
					ent->client->grenade_blew_up = false;
				}
				else
				{
					return;
				}
			}
		}

		if (ent->client->ps.gunframe == 12)
		{
			ent->client->weapon_sound = 0;
			weapon_grenade_fire (ent, false);
		}

		if ((ent->client->ps.gunframe == 15) && (level.time < ent->client->grenade_time))
			return;

		ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == 16)
		{
			ent->client->grenade_time = 0;
			ent->client->weaponstate = WEAPON_READY;
		}
	}
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire(edict_t* ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius;
	int		radius_damage;
	// If weapon not holstered
	if (ent->client->pers.hand != 2) {

		damage = 30;
		float	radius;

		radius = damage + 40;
		
		damage = GetLevelOf(ent, SKILL_INTELLIGENCE) * 2 + 20;

		VectorSet(offset, 8, 8, ent->viewheight - 8);
		AngleVectors(ent->client->v_angle, forward, right, NULL);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		VectorScale(forward, -2, ent->client->kick_origin);
		ent->client->kick_angles[0] = -1;

		//fire_grenade(ent, start, forward, damage, 600, 2.5, radius);
		
		// Heal self:
		if (canCastSpell(ent, 40)) {
			float roll = crandom();
			if (roll < 0) {
				roll *= -1;
			}
			roll *= 100; // Now our numbers are integers as percents
			// Calculate chance to hit
			// Morrowind does it like this:
			// (Weapon Skill + (Agility / 5) + (Luck / 10) * (0.75 + 0.5 * Current Fatigue / Maximum Fatigue) + Fortify Attack Magnitude)
			float castchance = (GetWeaponSkill(ent, getSkillReq(ent, ent->client->pers.weapon->classname)) * 2 + 10 + GetLevelOf(ent, SKILL_WILLPOWER) / 2.0) * (0.75 + 0.5 * ent->client->pers.fatigue / ent->client->pers.max_fatigue);
			if (castchance > roll) {
				gi.centerprintf(ent, "");
				ent->health += damage;
				if (ent->health > ent->max_health) {
					ent->health = ent->max_health;
					grantCurrWeapXP(ent, .25);
				}
				// Spell sound
				//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
			}
			else {
				gi.centerprintf(ent, "Spellcast Failed!");
				// Failed spell sound
				//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
			}
			char out[6];
			itoa(castchance, out, 10);
			gi.cprintf(ent, PRINT_HIGH, out);
			gi.cprintf(ent, PRINT_HIGH, "/");
			itoa(roll, out, 10);
			gi.cprintf(ent, PRINT_HIGH, out);
			gi.cprintf(ent, PRINT_HIGH, "\n");
		}

		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_GRENADE | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		ent->client->ps.gunframe++;
		ent->client->ps.gunframe++;

		PlayerNoise(ent, start, PNOISE_WEAPON);
	}
	else {

		AngleVectors(ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 8, ent->viewheight - 8);
		VectorAdd(offset, vec3_origin, offset);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		ent->client->ps.gunframe++;
		Interact(ent, start, forward);
	}

}

void Weapon_GrenadeLauncher (edict_t *ent)
{

	/*
	static int	pause_frames[]	= {34, 51, 59, 0};
	static int	fire_frames[]	= {14, 0};
	*/
	static int	pause_frames[] = { 25, 33, 42, 50, 0 }; 
	static int	fire_frames[] = { 12, 0 }; 

	Weapon_Generic(ent, 4, 15, 47, 48, pause_frames, fire_frames, weapon_grenadelauncher_fire);

	//Weapon_Generic (ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_grenadelauncher_fire);
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius;
	int		radius_damage;


	// If weapon not holstered
	if (ent->client->pers.hand != 2) {


		if (canCastSpell(ent, 40)) {

			damage = 20; // was 100
			radius_damage = 120; // was 120
			damage_radius = 120; // was 120

			damage += GetLevelOf(ent, SKILL_INTELLIGENCE) / 2;

			float roll = crandom();
			if (roll < 0) {
				roll *= -1;
			}
			roll *= 100; // Now our numbers are integers as percents
			// Calculate chance to hit
			// Morrowind does it like this:
			// (Weapon Skill + (Agility / 5) + (Luck / 10) * (0.75 + 0.5 * Current Fatigue / Maximum Fatigue) + Fortify Attack Magnitude)
			float castchance = (GetWeaponSkill(ent, getSkillReq(ent, ent->client->pers.weapon->classname)) * 2 + 10 + GetLevelOf(ent, SKILL_WILLPOWER) / 2.0) * (0.75 + 0.5 * ent->client->pers.fatigue / ent->client->pers.max_fatigue);
			if (castchance > roll) {
				gi.centerprintf(ent, "");

				AngleVectors(ent->client->v_angle, forward, right, NULL);

				VectorScale(forward, -2, ent->client->kick_origin);
				ent->client->kick_angles[0] = -1;

				VectorSet(offset, 8, 8, ent->viewheight - 8);
				P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
				fire_rocket(ent, start, forward, damage, 325, damage_radius, radius_damage); // speed was 650

				// send muzzle flash
				gi.WriteByte(svc_muzzleflash);
				gi.WriteShort(ent - g_edicts);
				gi.WriteByte(MZ_ROCKET | is_silenced);
				gi.multicast(ent->s.origin, MULTICAST_PVS);

				//ent->client->ps.gunframe++;
				
				grantCurrWeapXP(ent, .25);
				
				// Spell sound
				//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
			}
			else {
				gi.centerprintf(ent, "Spellcast Failed!");
				// Failed spell sound
				//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
			}
			char out[6];
			itoa(castchance, out, 10);
			gi.cprintf(ent, PRINT_HIGH, out);
			gi.cprintf(ent, PRINT_HIGH, "/");
			itoa(roll, out, 10);
			gi.cprintf(ent, PRINT_HIGH, out);
			gi.cprintf(ent, PRINT_HIGH, "\n");
		}
		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_ROCKET | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		ent->client->ps.gunframe++;

	}
	else {
		vec3_t outspawn;
		outspawn[0] = -999;
		outspawn[1] = -999;
		outspawn[2] = -999;

		AngleVectors(ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 8, ent->viewheight - 8);
		VectorAdd(offset, vec3_origin, offset);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		ent->client->ps.gunframe++;
		Interact(ent, start, forward);
	}
}

void Weapon_RocketLauncher (edict_t *ent)
{
	static int	pause_frames[]	= {25, 33, 42, 50, 0}; // was static int	pause_frames[]	= {25, 33, 42, 50, 0};
	static int	fire_frames[]	= {12, 0}; // was static int	fire_frames[]	= {5, 0};

	Weapon_Generic (ent, 4, 15, 47, 48, pause_frames, fire_frames, Weapon_RocketLauncher_Fire);
}


/*
======================================================================

BLASTER / HYPERBLASTER

======================================================================
*/

void Blaster_Fire (edict_t *ent, vec3_t g_offset, int damage, qboolean hyper, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;
	
	if (is_quad)
		damage *= 4;
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight - 8);
	VectorAdd(offset, g_offset, offset);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	// If weapon not holstered
	if (ent->client->pers.hand != 2) {
		VectorScale(forward, -2, ent->client->kick_origin);
		ent->client->kick_angles[0] = -1;

		fire_blaster(ent, start, forward, damage, 1000, effect, hyper);

		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		if (hyper)
			gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
		else
			gi.WriteByte(MZ_BLASTER | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		PlayerNoise(ent, start, PNOISE_WEAPON);
	}
	else {
		Interact(ent, start, forward);
	}
}


void Weapon_Blaster_Fire (edict_t *ent)
{
	int		damage;

	if (deathmatch->value)
		damage = 15;
	else
		damage = 10;
	Blaster_Fire (ent, vec3_origin, damage, false, EF_BLASTER);
	ent->client->ps.gunframe++;
}

void Weapon_Blaster (edict_t *ent)
{
	static int	pause_frames[]	= {19, 32, 0};
	static int	fire_frames[]	= {5, 0};

	Weapon_Generic (ent, 4, 8, 52, 55, pause_frames, fire_frames, Weapon_Blaster_Fire);
}


void Weapon_HyperBlaster_Fire (edict_t *ent)
{
	float	rotation;
	vec3_t	offset;
	int		effect;
	int		damage;

	ent->client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");

		if (!(ent->client->buttons & BUTTON_ATTACK))
		{
			ent->client->ps.gunframe++;
		}
		else
		{
			if (ent->client->pers.hand != 2) {
				if (!ent->client->pers.inventory[ent->client->ammo_index])
				{
					if (level.time >= ent->pain_debounce_time)
					{
						gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
						ent->pain_debounce_time = level.time + 1;
					}
					NoAmmoWeaponChange(ent);
				}
				else
				{
					rotation = (ent->client->ps.gunframe - 5) * 2 * M_PI / 6;
					offset[0] = -4 * sin(rotation);
					offset[1] = 0;
					offset[2] = 4 * cos(rotation);

					//if ((ent->client->ps.gunframe == 6) || (ent->client->ps.gunframe == 9))
					effect = EF_HYPERBLASTER;

					// Damage to 1 since its a poison spell, and 0 is making it not work
					damage = 1;

					if (canCastSpell(ent, 20)) {
						float roll = crandom();
						if (roll < 0) {
							roll *= -1;
						}
						roll *= 100; // Now our numbers are integers as percents
						// Calculate chance to hit
						// Morrowind does it like this:
						// (Weapon Skill + (Agility / 5) + (Luck / 10) * (0.75 + 0.5 * Current Fatigue / Maximum Fatigue) + Fortify Attack Magnitude)
						float castchance = (GetWeaponSkill(ent, getSkillReq(ent, ent->client->pers.weapon->classname)) * 2 + 10 + GetLevelOf(ent, SKILL_WILLPOWER) / 2.0) * (0.75 + 0.5 * ent->client->pers.fatigue / ent->client->pers.max_fatigue);
						if (castchance > roll) {
							gi.centerprintf(ent, "");
							Blaster_Fire(ent, offset, damage, true, effect);
							grantCurrWeapXP(ent, .25);
							// Spell sound
							//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
						}
						else {
							gi.centerprintf(ent, "Spellcast Failed!");
							// Failed spell sound
							//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
						}
						char out[6];
						itoa(castchance, out, 10);
						gi.cprintf(ent, PRINT_HIGH, out);
						gi.cprintf(ent, PRINT_HIGH, "/");
						itoa(roll, out, 10);
						gi.cprintf(ent, PRINT_HIGH, out);
						gi.cprintf(ent, PRINT_HIGH, "\n");
					}

					ent->client->anim_priority = ANIM_ATTACK;
					if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
					{
						ent->s.frame = FRAME_crattak1 - 1;
						ent->client->anim_end = FRAME_crattak9;
					}
					else
					{
						ent->s.frame = FRAME_attack1 - 1;
						ent->client->anim_end = FRAME_attack8;
					}
				}
			}
			else {
				vec3_t	forward, right;
				vec3_t	start;
				vec3_t	offset;

				AngleVectors(ent->client->v_angle, forward, right, NULL);
				VectorSet(offset, 24, 8, ent->viewheight - 8);
				VectorAdd(offset, vec3_origin, offset);
				P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
				Interact(ent, start, forward);
			}

			ent->client->ps.gunframe++;
			if (ent->client->ps.gunframe == 12 && ent->client->pers.inventory[ent->client->ammo_index])
				ent->client->ps.gunframe = 6;
		}

		if (ent->client->ps.gunframe == 12)
		{
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
			ent->client->weapon_sound = 0;
		}
	
	
}

void Weapon_HyperBlaster (edict_t *ent)
{


	static int	pause_frames[] = { 0 };
	static int	fire_frames[] = { 6, 0 };

	Weapon_Generic(ent, 5, 20, 47, 48, pause_frames, fire_frames, Weapon_HyperBlaster_Fire);

	//static int	pause_frames[]	= {0};
	//static int	fire_frames[]	= {6, 0}; //6, 7, 8, 9, 10, 11, 0

	//Weapon_Generic (ent, 5, 20, 49, 53, pause_frames, fire_frames, Weapon_HyperBlaster_Fire);
}

/*
======================================================================

MACHINEGUN / CHAINGUN

======================================================================
*/

void Machinegun_Fire (edict_t *ent)
{
	int	i;
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		angles;
	int			damage = 8;
	int			kick = 2;
	vec3_t		offset;

	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->machinegun_shots = 0;
		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->ps.gunframe == 5)
		ent->client->ps.gunframe = 4;
	else
		ent->client->ps.gunframe = 5;

	if (ent->client->pers.inventory[ent->client->ammo_index] < 1)
	{
		ent->client->ps.gunframe = 6;
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
		return;
	}

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	for (i=1 ; i<3 ; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.35;
		ent->client->kick_angles[i] = crandom() * 0.7;
	}
	ent->client->kick_origin[0] = crandom() * 0.35;
	ent->client->kick_angles[0] = ent->client->machinegun_shots * -1.5;

	// raise the gun as it is firing
	if (!deathmatch->value)
	{
		ent->client->machinegun_shots++;
		if (ent->client->machinegun_shots > 9)
			ent->client->machinegun_shots = 9;
	}

	// get start / end positions
	VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors (angles, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_bullet (ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MACHINEGUN);

	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_MACHINEGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index]--;

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - (int) (random()+0.25);
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - (int) (random()+0.25);
		ent->client->anim_end = FRAME_attack8;
	}
}

void Weapon_Machinegun (edict_t *ent)
{
	static int	pause_frames[]	= {23, 45, 0};
	static int	fire_frames[]	= {4, 5, 0};

	Weapon_Generic (ent, 3, 5, 45, 49, pause_frames, fire_frames, Machinegun_Fire);
}

void Chaingun_Fire (edict_t *ent)
{
	int			i;
	int			shots;
	vec3_t		start;
	vec3_t		forward, right, up;
	float		r, u;
	vec3_t		offset;
	int			damage;
	int			kick = 2;

	if (deathmatch->value)
		damage = 6;
	else
		damage = 8;

	if (ent->client->ps.gunframe == 5)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);

	if ((ent->client->ps.gunframe == 14) && !(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe = 32;
		ent->client->weapon_sound = 0;
		return;
	}
	else if ((ent->client->ps.gunframe == 21) && (ent->client->buttons & BUTTON_ATTACK)
		&& ent->client->pers.inventory[ent->client->ammo_index])
	{
		ent->client->ps.gunframe = 15;
	}
	else
	{
		ent->client->ps.gunframe++;
	}

	if (ent->client->ps.gunframe == 22)
	{
		ent->client->weapon_sound = 0;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
	}
	else
	{
		ent->client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");
	}

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - (ent->client->ps.gunframe & 1);
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - (ent->client->ps.gunframe & 1);
		ent->client->anim_end = FRAME_attack8;
	}

	if (ent->client->ps.gunframe <= 9)
		shots = 1;
	else if (ent->client->ps.gunframe <= 14)
	{
		if (ent->client->buttons & BUTTON_ATTACK)
			shots = 2;
		else
			shots = 1;
	}
	else
		shots = 3;

	if (ent->client->pers.inventory[ent->client->ammo_index] < shots)
		shots = ent->client->pers.inventory[ent->client->ammo_index];

	if (!shots)
	{
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
		return;
	}

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	for (i=0 ; i<3 ; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.35;
		ent->client->kick_angles[i] = crandom() * 0.7;
	}

	for (i=0 ; i<shots ; i++)
	{
		// get start / end positions
		AngleVectors (ent->client->v_angle, forward, right, up);
		r = 7 + crandom()*4;
		u = crandom()*4;
		VectorSet(offset, 0, r, u + ent->viewheight-8);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

		fire_bullet (ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
	}

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte ((MZ_CHAINGUN1 + shots - 1) | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index] -= shots;
}


void Weapon_Chaingun (edict_t *ent)
{
	static int	pause_frames[]	= {38, 43, 51, 61, 0};
	static int	fire_frames[]	= {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0};

	Weapon_Generic (ent, 4, 31, 61, 64, pause_frames, fire_frames, Chaingun_Fire);
}


/*
======================================================================

SHOTGUN / SUPERSHOTGUN

======================================================================
*/

void weapon_shotgun_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage = 4; // was 4
	int			kick = 8; // was 8
	
	if (ent->client->ps.gunframe == 9)
	{
		ent->client->ps.gunframe++;
		return;
	}

	// If weapon not holstered
	if (ent->client->pers.hand != 2) {	

		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorScale (forward, -2, ent->client->kick_origin);
		ent->client->kick_angles[0] = -2;

		VectorSet(offset, 0, 8, ent->viewheight - 8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	
		if (is_quad)
		{
			damage *= 4;
			kick *= 4;
		}

		int hspread = 500; // was 500
		int vspread = 500; // was 500

		if (deathmatch->value)
			fire_shotgun(ent, start, forward, damage, kick, 500, 500, DEFAULT_DEATHMATCH_SHOTGUN_COUNT, MOD_SHOTGUN);
		else
			fire_shotgun(ent, start, forward, damage, kick, hspread, vspread, 0, MOD_SHOTGUN); // was DEFAULT_SHOTGUN_COUNT

		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_SHOTGUN | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		ent->client->ps.gunframe++;
		//PlayerNoise(ent, start, PNOISE_WEAPON);
		/*
		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			ent->client->pers.inventory[ent->client->ammo_index]--;
		*/
		VectorSet(offset, 0, 8, 0);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		Melee(ent, start, forward, 10, MOD_SHOTGUN);
	}
	else {
		AngleVectors(ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 8, ent->viewheight - 8);
		VectorAdd(offset, vec3_origin, offset);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		fire_shotgun(ent, start, forward, damage, kick, 0, 0, 0, MOD_SHOTGUN);
		ent->client->ps.gunframe++;
		Interact(ent, start, forward);
	}
}

void Weapon_Shotgun (edict_t *ent)
{
	static int	pause_frames[]	= {22, 28, 34, 0};
	static int	fire_frames[]	= {8, 9, 0};

	Weapon_Generic (ent, 7, 18, 36, 39, pause_frames, fire_frames, weapon_shotgun_fire);
}


void weapon_supershotgun_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	vec3_t		v;
	int			damage = 6;
	int			kick = 12;

	// If weapon not holstered
	if (ent->client->pers.hand != 2) {

		AngleVectors(ent->client->v_angle, forward, right, NULL);

		VectorScale(forward, -2, ent->client->kick_origin);
		ent->client->kick_angles[0] = -2;

		VectorSet(offset, 0, 8, ent->viewheight - 8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		if (is_quad)
		{
			damage *= 4;
			kick *= 4;
		}

		v[PITCH] = ent->client->v_angle[PITCH];
		v[YAW] = ent->client->v_angle[YAW] - 5;
		v[ROLL] = ent->client->v_angle[ROLL];
		AngleVectors(v, forward, NULL, NULL);
		fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, 0, MOD_SSHOTGUN);
		v[YAW] = ent->client->v_angle[YAW] + 5;
		AngleVectors(v, forward, NULL, NULL);
		fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, 0, MOD_SSHOTGUN);

		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_SSHOTGUN | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		ent->client->ps.gunframe++;
		PlayerNoise(ent, start, PNOISE_WEAPON);

		Melee(ent, start, forward, 22, MOD_SSHOTGUN);
	}
	else {
		AngleVectors(ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 8, ent->viewheight - 8);
		VectorAdd(offset, vec3_origin, offset);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		fire_shotgun(ent, start, forward, damage, kick, 0, 0, 0, MOD_SHOTGUN);
		ent->client->ps.gunframe++;
		Interact(ent, start, forward);
	}
}

void Weapon_SuperShotgun (edict_t *ent)
{
	static int	pause_frames[]	= {29, 42, 57, 0};
	static int	fire_frames[]	= {7, 0};

	Weapon_Generic (ent, 6, 17, 57, 61, pause_frames, fire_frames, weapon_supershotgun_fire);
}



/*
======================================================================

RAILGUN

======================================================================
*/

void weapon_railgun_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage;
	int			kick;

	// If weapon not holstered
	if (ent->client->pers.hand != 2) {

		damage = GetLevelOf(ent, SKILL_INTELLIGENCE) + 10;

		AngleVectors(ent->client->v_angle, forward, right, NULL);

		VectorScale(forward, -3, ent->client->kick_origin);
		ent->client->kick_angles[0] = -3;

		if (canCastSpell(ent, 30)) {
			float roll = crandom();
			if (roll < 0) {
				roll *= -1;
			}
			roll *= 100; // Now our numbers are integers as percents
			// Calculate chance to hit
			// Morrowind does it like this:
			// (Weapon Skill + (Agility / 5) + (Luck / 10) * (0.75 + 0.5 * Current Fatigue / Maximum Fatigue) + Fortify Attack Magnitude)
			float castchance = (GetWeaponSkill(ent, getSkillReq(ent, ent->client->pers.weapon->classname)) * 2 + 10 + GetLevelOf(ent, SKILL_WILLPOWER) / 2.0) * (0.75 + 0.5 * ent->client->pers.fatigue / ent->client->pers.max_fatigue);
			if (castchance > roll) {
				gi.centerprintf(ent, "");
				inflictStatus(ent, ent, STATUS_FORTIFY_RESISTANCE, 30, damage);
				grantCurrWeapXP(ent, .25);
				// Spell sound
				//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
			}
			else {
				gi.centerprintf(ent, "Spellcast Failed!");
				// Failed spell sound
				//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
			}
			char out[6];
			itoa(castchance, out, 10);
			gi.cprintf(ent, PRINT_HIGH, out);
			gi.cprintf(ent, PRINT_HIGH, "/");
			itoa(roll, out, 10);
			gi.cprintf(ent, PRINT_HIGH, out);
			gi.cprintf(ent, PRINT_HIGH, "\n");
		}

		VectorSet(offset, 0, 7, ent->viewheight - 8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_RAILGUN | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		ent->client->ps.gunframe++;
		PlayerNoise(ent, start, PNOISE_WEAPON);
	}
	else {
		AngleVectors(ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 8, ent->viewheight - 8);
		VectorAdd(offset, vec3_origin, offset);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		fire_shotgun(ent, start, forward, damage, kick, 0, 0, 0, MOD_SHOTGUN);
		ent->client->ps.gunframe++;
		Interact(ent, start, forward);
	}
}


void Weapon_Railgun (edict_t *ent)
{
	static int	pause_frames[]	= {56, 0};
	static int	fire_frames[]	= {4, 0};

	Weapon_Generic (ent, 3, 18, 56, 61, pause_frames, fire_frames, weapon_railgun_fire);
}


/*
======================================================================

BFG10K

======================================================================
*/

void weapon_bfg_fire(edict_t* ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius = 1000;

	damage = 10;
	int kick = 0;

	if (ent->client->pers.hand != 2) {

		AngleVectors(ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 8, ent->viewheight - 8);
		VectorAdd(offset, vec3_origin, offset);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		VectorScale(forward, -3, ent->client->kick_origin);
		ent->client->kick_angles[0] = -3;

		if (canCastSpell(ent, 10)) {

			float roll = crandom();
			if (roll < 0) {
				roll *= -1;
			}
			roll *= 100; // Now our numbers are integers as percents
			// Calculate chance to hit
			// Morrowind does it like this:
			// (Weapon Skill + (Agility / 5) + (Luck / 10) * (0.75 + 0.5 * Current Fatigue / Maximum Fatigue) + Fortify Attack Magnitude)
			float castchance = (GetWeaponSkill(ent, getSkillReq(ent, ent->client->pers.weapon->classname)) * 2 + 10 + GetLevelOf(ent, SKILL_WILLPOWER) / 2.0) * (0.75 + 0.5 * ent->client->pers.fatigue / ent->client->pers.max_fatigue);
			if (castchance > roll) {
				trace_t doorCheck;
				doorCheck = gi.trace(ent->s.origin, NULL, NULL, start, ent, MASK_ALL);
				if (doorCheck.fraction != 1.0 && doorCheck.ent->classname == "func_door") {
					doorCheck.ent->activator->use;
				}
				grantCurrWeapXP(ent, .25);
				// Spell sound
				//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
			}
			else {
				gi.centerprintf(ent, "Spellcast Failed!");
				// Failed spell sound
				//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
			}
			char out[6];
			itoa(castchance, out, 10);
			gi.cprintf(ent, PRINT_HIGH, out);
			gi.cprintf(ent, PRINT_HIGH, "/");
			itoa(roll, out, 10);
			gi.cprintf(ent, PRINT_HIGH, out);
			gi.cprintf(ent, PRINT_HIGH, "\n");


			// send muzzle flash
			gi.WriteByte(svc_muzzleflash);
			gi.WriteShort(ent - g_edicts);
			gi.WriteByte(MZ_BFG | is_silenced);
			gi.multicast(ent->s.origin, MULTICAST_PVS);

			ent->client->ps.gunframe++;

			PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);
			
			AngleVectors(ent->client->v_angle, forward, right, NULL);

			VectorScale(forward, -2, ent->client->kick_origin);

			// make a big pitch kick with an inverse fall
			ent->client->v_dmg_pitch = -40;
			ent->client->v_dmg_roll = crandom() * 8;
			ent->client->v_dmg_time = level.time + DAMAGE_TIME;

			VectorSet(offset, 8, 8, ent->viewheight - 8);
			P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
			fire_shotgun(ent, start, forward, damage, kick, 0, 0, 0, MOD_SHOTGUN);
			fire_shotgun(ent, start, forward, damage, kick, 0, 0, 0, MOD_SHOTGUN);
			//ent->client->ps.gunframe++;

			PlayerNoise(ent, start, PNOISE_WEAPON);
		}
		ent->client->ps.gunframe = 32;
	}
	else {
		AngleVectors(ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 8, ent->viewheight - 8);
		VectorAdd(offset, vec3_origin, offset);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		fire_shotgun(ent, start, forward, damage, kick, 0, 0, 0, MOD_SHOTGUN);
		ent->client->ps.gunframe++;
		Interact(ent, start, forward);
	}

}

void Weapon_BFG (edict_t *ent)
{
	static int	pause_frames[]	= {39, 45, 50, 55, 0};
	static int	fire_frames[]	= {9, 17, 0};

	Weapon_Generic (ent, 8, 32, 55, 58, pause_frames, fire_frames, weapon_bfg_fire);
}


//======================================================================
