#include "g_local.h"
#include "m_questgiver.h"

/**********************************
*         Quest Giver NPC         *
*                                 *
*       by Matthew LiDonni        *
**********************************/




void QuestGiver_RunFrames(edict_t* self, int start, int end) {
	if (self->s.frame < end) {
		self->s.frame++;
	}
	else {
		self->s.frame = start;
	}
}


mframe_t questgiver_frames_stand[] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
};
mmove_t questgiver_move_stand = { FRAME_stand50, FRAME_stand71, questgiver_frames_stand, NULL };

void questgiver_stand(edict_t* self)
{
	self->monsterinfo.currentmove = &questgiver_move_stand;
	QuestGiver_RunFrames(self, FRAME_stand01, FRAME_stand49);
}


mframe_t questgiver_frames_fidget[] =
{
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 3,  NULL,
	ai_stand, 6,  NULL,
	ai_stand, 3,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -2, NULL,
	ai_stand, 1,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -1, NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, -3, NULL,
	ai_stand, -2, NULL,
	ai_stand, -3, NULL,
	ai_stand, -3, NULL,
	ai_stand, -2, NULL
};
mmove_t questgiver_move_fidget = { FRAME_stand01, FRAME_stand49, questgiver_frames_fidget, questgiver_stand };

void questgiver_fidget(edict_t* self)
{
	self->monsterinfo.currentmove = &questgiver_move_fidget;
	QuestGiver_RunFrames(self, FRAME_stand01, FRAME_stand49);
}

quest getQuest(int questNum) {
	quest q;
	if (questNum == 0) {
		q.questNum = 0;
		q.queststarted = true;
		q.questcompleted = false;
		strcpy(q.questname, "Bandit Season");
		strcpy(q.questdesc, "Kill 5 Bandits");
		// PUT \n FOR DIALOGUE BUT NOT NAMES AND OTHER STUFF THAT MAY NEED TO BE COMBINED!
		strcpy(q.introdiag, "Hail, adventurer. There are bandits about, clear them out and I can make it worth your while.\n");
		strcpy(q.inprogressdiag, "Have you done it yet? Quit standing about and get on with it!\n");
		strcpy(q.completediag, "Thank you, take this as a token of my appreciation.\n");
		strcpy(q.postdiag, "Stay safe, adventurer.\n");
		strcpy(q.killclassname, "monster_soldier");
		q.kills = 0;
		q.killsneeded = 5;
		q.rewardXP = 25;
		q.rewardItem = RPGITEM_HEALTHPOT;
		q.rewardQuantity = 2;
	}
	else if (questNum == 1) {
		q.questNum = 1;
		q.queststarted = true;
		q.questcompleted = false;
		strcpy(q.questname, "Missing Jewelry");
		strcpy(q.questdesc, "Find the persons Emerald Ring");
		strcpy(q.introdiag, "Pardon me, but have you seen an Emerald Ring? Please, that ring is important to me.\n");
		strcpy(q.inprogressdiag, "Any luck? Keep searching, I know its around here somewhere!\n");
		strcpy(q.completediag, "You found it? Oh Divines bless you! Here take this.\n");
		strcpy(q.postdiag, "Thank you thank you thank you!\n");
		strcpy(q.killclassname, "none");
		q.kills = 0;
		q.killsneeded = 1;
		q.rewardXP = 25;
		q.rewardItem = RPGITEM_MAGICKAPOT;
		q.rewardQuantity = 3;
	}
	else if (questNum == 2) {
		q.questNum = 2;
		q.queststarted = true;
		q.questcompleted = false;
		strcpy(q.questname, "Mail Man");
		strcpy(q.questdesc, "Deliver the letter.");
		strcpy(q.introdiag, "Hey friend, I've got a letter that needs to be delivered, could you help me out?\n");
		strcpy(q.inprogressdiag, "The guy lives near a sewer entrance.\n");
		strcpy(q.completediag, "Thanks.\n");
		strcpy(q.postdiag, "Thanks again, I appreciate the help!\n");
		strcpy(q.killclassname, "");
		q.kills = 0;
		q.killsneeded = 1;
		q.rewardXP = 25;
		q.rewardItem = RPGITEM_CHESTARMOR;
		q.rewardQuantity = 1;
	}
	else if (questNum == 3) {
		q.questNum = 3;
		q.queststarted = true;
		q.questcompleted = false;
		strcpy(q.questname, "Big and Ugly");
		strcpy(q.questdesc, "Kill 3 Soldiers");
		strcpy(q.introdiag, "Wanna kill 3 of those big uglies for a fortify potion?\n");
		strcpy(q.inprogressdiag, "What'd they do? Nothing, who cares?\n");
		strcpy(q.completediag, "You know those guys had families? Eh, you don't care...\n");
		strcpy(q.postdiag, "Watch your back, pal.\n");
		strcpy(q.killclassname, "monster_soldier");
		q.kills = 0;
		q.killsneeded = 1;
		q.rewardXP = 50;
		q.rewardItem = RPGITEM_FORTIFYPOT;
		q.rewardQuantity = 2;
	}
	else if (questNum == 4) {
		q.questNum = 4;
		q.queststarted = true;
		q.questcompleted = false;
		strcpy(q.questname, "Medic!!!");
		strcpy(q.questdesc, "Find the man a health potion");
		strcpy(q.introdiag, "Hey, can you spare a healing potion, I'm hurt pretty bad...\n");
		strcpy(q.inprogressdiag, "*cough cough*\n");
		strcpy(q.completediag, "You're my hero, stranger.\n");
		strcpy(q.postdiag, "I wish I could offer you more, but I hope that's enough.\n");
		strcpy(q.killclassname, "none");
		q.kills = 0;
		q.killsneeded = 1;
		q.rewardXP = 50;
		q.rewardItem = RPGITEM_LEVITATEPOT;
		q.rewardQuantity = 2;
	}
	return q;
}

void questgiver_die (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point) {
	int i;
	if (self->health > self->gib_health) {
		self->takedamage = DAMAGE_NO;
	}
	G_FreeEdict(self);
}



// Spawn function
void SP_QuestGiver(edict_t* ent, vec3_t spawn_origin_q, vec3_t spawn_angles_q, int questNum) {
	edict_t* newQuestGiver;
	 
	newQuestGiver = G_Spawn();
	VectorCopy(spawn_origin_q, spawn_angles_q);
	newQuestGiver->s.origin[0] = spawn_origin_q[0];
	newQuestGiver->s.origin[1] = spawn_origin_q[1];
	newQuestGiver->s.origin[2] = spawn_origin_q[2] - 18;  // Prevents clipping into ground
	newQuestGiver->classname = "questgiver"; // Class name
	newQuestGiver->takedamage = DAMAGE_NO_KNOCKBACK; 
	newQuestGiver->movetype = MOVETYPE_STOP;
	newQuestGiver->mass = 200; // Mass of npc
	newQuestGiver->solid = SOLID_BBOX; // Physics interaction
	newQuestGiver->deadflag = DEAD_NO; // Not dead
	newQuestGiver->clipmask = MASK_MONSTERSOLID; // clipmask layer (Interaction is set to monsters, so if you change one you have to change the other)
	//newQuestGiver->model = "players/male/tris.md2"; // Not 100% sure what i'm getting here
	//newQuestGiver->s.modelindex = 255; // or here
	//newQuestGiver->s.skinnum = 2;
	//newQuestGiver->s.modelindex = gi.modelindex("models/npcs/fem/tris.md2");
	newQuestGiver->model = "players/male/tris.md2";
	newQuestGiver->s.modelindex = 255;
	newQuestGiver->s.frame = 0; // Animation frame start maybe?
	newQuestGiver->waterlevel = 0; // Idk they arent gonna swim
	newQuestGiver->waterlevel = 0;

	newQuestGiver->health = 500;
	newQuestGiver->max_health = 500;
	newQuestGiver->gib_health = -999999; // How much damage to gib (non-applicable bc DAMAGE_NO)

	newQuestGiver->die = questgiver_die;

	// Uses infantry stand and fidget as animations
	newQuestGiver->monsterinfo.stand = questgiver_stand; 
	newQuestGiver->monsterinfo.idle = questgiver_fidget;
	
	newQuestGiver->think = questgiver_fidget;
	newQuestGiver->nextthink = level.time + 0.1;

	// Set rotation 
	newQuestGiver->yaw_speed = 20;
	newQuestGiver->s.angles[0] = 0;
	newQuestGiver->s.angles[1] = spawn_angles_q[1];
	newQuestGiver->s.angles[2] = 0;

	// Quest information
	newQuestGiver->questNum = questNum;
	
	VectorSet(newQuestGiver->mins, -16, -16, -32);
	VectorSet(newQuestGiver->maxs, 16, 16, 32);
	VectorClear(newQuestGiver->velocity);

	KillBox(newQuestGiver); // Kill anything in the way of our new NPC when spawning (MUST BE DONE WHEN NOT LINKED)

	gi.linkentity(newQuestGiver); // Link entity to game

}

void SP_QuestGiver_zero(edict_t* self) {
	self->classname = "questgiver"; // Class name
	self->takedamage = DAMAGE_NO_KNOCKBACK; // Should take no damage, but maybe change this later
	self->mass = 200; // Mass of npc
	self->solid = SOLID_BBOX; // Physics interaction
	self->deadflag = DEAD_NO; // Not dead
	self->clipmask = MASK_MONSTERSOLID; // clipmask layer (Interaction is set to monsters, so if you change one you have to change the other)
	self->model = "players/male/tris.md2"; // Not 100% sure what i'm getting here
	self->s.modelindex = 255; // or here
	self->s.skinnum = 2;
	self->s.frame = 0; // Animation frame start maybe?
	self->waterlevel = 0; // Idk they arent gonna swim
	self->waterlevel = 0;

	self->health = 100;
	self->max_health = 100;
	self->gib_health = -999999; // How much damage to gib (non-applicable bc DAMAGE_NO)

	// Uses infantry stand and fidget as animations
	self->monsterinfo.stand = questgiver_stand;
	self->monsterinfo.idle = questgiver_fidget;

	self->think = questgiver_fidget;
	self->nextthink = level.time + 0.1;

	// Quest information
	self->questNum = 0;

	VectorSet(self->mins, -16, -16, -32);
	VectorSet(self->maxs, 16, 16, 32);
	VectorClear(self->velocity);

	KillBox(self); // Kill anything in the way of our new NPC when spawning (MUST BE DONE WHEN NOT LINKED)

	gi.linkentity(self); // Link entity to game
}

void questItemThink(edict_t* self) {
	// Game crashes without a think method here


}

void SP_QuestItem_ring(edict_t* ent, vec3_t spawn_origin_q, vec3_t spawn_angles_q, int questNum) {

	edict_t* newQuestItem;

	newQuestItem = G_Spawn();

	VectorCopy(spawn_origin_q, spawn_angles_q);
	newQuestItem->s.origin[0] = spawn_origin_q[0];
	newQuestItem->s.origin[1] = spawn_origin_q[1];
	newQuestItem->s.origin[2] = spawn_origin_q[2] - 18;  // Prevents clipping into ground
	newQuestItem->classname = "questitem"; // Class name
	newQuestItem->takedamage = DAMAGE_NO_KNOCKBACK;
	newQuestItem->movetype = MOVETYPE_STOP;
	newQuestItem->mass = 200; // Mass of npc
	newQuestItem->solid = SOLID_BBOX; // Physics interaction
	newQuestItem->deadflag = DEAD_NO; // Not dead
	newQuestItem->clipmask = MASK_MONSTERSOLID; // clipmask layer (Interaction is set to monsters, so if you change one you have to change the other)
	newQuestItem->s.modelindex = gi.modelindex("models/objects/quest2item/tris.md2");
	newQuestItem->s.frame = 0; // Animation frame start maybe?
	newQuestItem->waterlevel = 0; // Idk they arent gonna swim
	newQuestItem->waterlevel = 0;

	newQuestItem->health = 500;
	newQuestItem->max_health = 500;
	newQuestItem->gib_health = -999999; // How much damage to gib (non-applicable bc DAMAGE_NO)

	newQuestItem->die = questgiver_die;

	newQuestItem->nextthink = level.time + 0.1;

	// Set rotation 
	newQuestItem->yaw_speed = 20;
	newQuestItem->s.angles[0] = 0;
	newQuestItem->s.angles[1] = spawn_angles_q[1];
	newQuestItem->s.angles[2] = 0;

	newQuestItem->think = questItemThink;

	// Quest information
	newQuestItem->questNum = questNum;

	VectorSet(newQuestItem->mins, -16, -16, -32);
	VectorSet(newQuestItem->maxs, 16, 16, 32);
	VectorClear(newQuestItem->velocity);

	KillBox(newQuestItem); // Kill anything in the way of our new NPC when spawning (MUST BE DONE WHEN NOT LINKED)

	gi.linkentity(newQuestItem); // Link entity to game
}

// Spawn function
void SP_MailRecipient(edict_t* ent, vec3_t spawn_origin_q, vec3_t spawn_angles_q, int questNum) {
	edict_t* newQuestGiver;

	newQuestGiver = G_Spawn();
	VectorCopy(spawn_origin_q, spawn_angles_q);
	newQuestGiver->s.origin[0] = spawn_origin_q[0];
	newQuestGiver->s.origin[1] = spawn_origin_q[1];
	newQuestGiver->s.origin[2] = spawn_origin_q[2] - 18;  // Prevents clipping into ground
	newQuestGiver->classname = "mailrecipient"; // Class name
	newQuestGiver->takedamage = DAMAGE_NO_KNOCKBACK;
	newQuestGiver->movetype = MOVETYPE_STOP;
	newQuestGiver->mass = 200; // Mass of npc
	newQuestGiver->solid = SOLID_BBOX; // Physics interaction
	newQuestGiver->deadflag = DEAD_NO; // Not dead
	newQuestGiver->clipmask = MASK_MONSTERSOLID; // clipmask layer (Interaction is set to monsters, so if you change one you have to change the other)
	//newQuestGiver->model = "players/male/tris.md2"; // Not 100% sure what i'm getting here
	//newQuestGiver->s.modelindex = 255; // or here
	//newQuestGiver->s.skinnum = 2;
	//newQuestGiver->s.modelindex = gi.modelindex("models/npcs/fem/tris.md2");
	newQuestGiver->model = "players/male/tris.md2";
	newQuestGiver->s.modelindex = 255;
	newQuestGiver->s.frame = 0; // Animation frame start maybe?
	newQuestGiver->waterlevel = 0; // Idk they arent gonna swim
	newQuestGiver->waterlevel = 0;

	newQuestGiver->health = 500;
	newQuestGiver->max_health = 500;
	newQuestGiver->gib_health = -999999; // How much damage to gib (non-applicable bc DAMAGE_NO)

	newQuestGiver->die = questgiver_die;

	// Uses infantry stand and fidget as animations
	newQuestGiver->monsterinfo.stand = questgiver_stand;
	newQuestGiver->monsterinfo.idle = questgiver_fidget;

	newQuestGiver->think = questgiver_fidget;
	newQuestGiver->nextthink = level.time + 0.1;

	// Set rotation 
	newQuestGiver->yaw_speed = 20;
	newQuestGiver->s.angles[0] = 0;
	newQuestGiver->s.angles[1] = spawn_angles_q[1];
	newQuestGiver->s.angles[2] = 0;

	// Quest information
	newQuestGiver->questNum = questNum;

	VectorSet(newQuestGiver->mins, -16, -16, -32);
	VectorSet(newQuestGiver->maxs, 16, 16, 32);
	VectorClear(newQuestGiver->velocity);

	KillBox(newQuestGiver); // Kill anything in the way of our new NPC when spawning (MUST BE DONE WHEN NOT LINKED)

	gi.linkentity(newQuestGiver); // Link entity to game

}






