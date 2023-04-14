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
	}
	else if (questNum == 2) {
		q.questNum = 2;
		q.queststarted = true;
		q.questcompleted = false;
		strcpy(q.questname, "QuestName");
		strcpy(q.questdesc, "Desc");
		strcpy(q.introdiag, "IntroDialogue\n");
		strcpy(q.inprogressdiag, "InProgressDialogue\n");
		strcpy(q.completediag, "Complete Dialogue\n");
		strcpy(q.postdiag, "PostDialogue\n");
		strcpy(q.killclassname, "none");
		q.kills = 0;
		q.killsneeded = 1;
		q.rewardXP = 50;
	}
	else if (questNum == 3) {
		q.questNum = 3;
		q.queststarted = true;
		q.questcompleted = false;
		strcpy(q.questname, "QuestName");
		strcpy(q.questdesc, "Desc");
		strcpy(q.introdiag, "IntroDialogue\n");
		strcpy(q.inprogressdiag, "InProgressDialogue\n");
		strcpy(q.completediag, "Complete Dialogue\n");
		strcpy(q.postdiag, "PostDialogue\n");
		strcpy(q.killclassname, "none");
		q.kills = 0;
		q.killsneeded = 1;
		q.rewardXP = 50;
	}
	else if (questNum == 4) {
		q.questNum = 4;
		q.queststarted = true;
		q.questcompleted = false;
		strcpy(q.questname, "QuestName");
		strcpy(q.questdesc, "Desc");
		strcpy(q.introdiag, "IntroDialogue\n");
		strcpy(q.inprogressdiag, "InProgressDialogue\n");
		strcpy(q.completediag, "Complete Dialogue\n");
		strcpy(q.postdiag, "PostDialogue\n");
		strcpy(q.killclassname, "none");
		q.kills = 0;
		q.killsneeded = 1;
		q.rewardXP = 50;
	}
	return q;
}



// Spawn function
void SP_QuestGiver(edict_t* ent, vec3_t spawn_origin_q, vec3_t spawn_angles_q, int questNum) {
	edict_t* newQuestGiver;

	newQuestGiver = G_Spawn();
	VectorCopy(spawn_origin_q, spawn_angles_q);
	newQuestGiver->s.origin[0] = spawn_origin_q[0];
	newQuestGiver->s.origin[1] = spawn_origin_q[1];
	newQuestGiver->s.origin[2] = spawn_origin_q[2] - 22;  // Prevents clipping into ground
	newQuestGiver->classname = "questgiver"; // Class name
	newQuestGiver->takedamage = DAMAGE_NO; // Should take no damage, but maybe change this later
	newQuestGiver->mass = 200; // Mass of npc
	newQuestGiver->solid = SOLID_BBOX; // Physics interaction
	newQuestGiver->deadflag = DEAD_NO; // Not dead
	newQuestGiver->clipmask = MASK_MONSTERSOLID; // clipmask layer (Interaction is set to monsters, so if you change one you have to change the other)
	newQuestGiver->model = "players/female/tris.md2"; // Not 100% sure what i'm getting here
	newQuestGiver->s.modelindex = 255; // or here
	newQuestGiver->s.skinnum = 4;
	newQuestGiver->s.frame = 0; // Animation frame start maybe?
	newQuestGiver->waterlevel = 0; // Idk they arent gonna swim
	newQuestGiver->waterlevel = 0;

	newQuestGiver->health = 100;
	newQuestGiver->max_health = 100;
	newQuestGiver->gib_health = -40; // How much damage to gib (non-applicable bc DAMAGE_NO)

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
	gi.cprintf(ent, PRINT_HIGH, "Spawned a new quest giver!\n");

}



