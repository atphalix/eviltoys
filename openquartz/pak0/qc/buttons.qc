/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/

// button and multiple button

void() button_wait;
void() button_return;

void() button_wait =
{
	self.state = STATE_TOP;
	self.nextthink = self.ltime + self.wait;
	self.think = button_return;
	activator = self.enemy;
	SUB_UseTargets();
	self.frame = 1;			// use alternate textures
};

void() button_done =
{
	self.state = STATE_BOTTOM;
};

void() button_return =
{
	self.state = STATE_DOWN;
	SUB_CalcMove (self.pos1, self.speed, button_done);
	self.frame = 0;			// use normal textures
	if (self.health)
		self.takedamage = DAMAGE_YES;	// can be shot again
};


void() button_blocked =
{	// do nothing, just don't ome all the way back out
};


void() button_fire =
{
	if (self.state == STATE_UP || self.state == STATE_TOP)
		return;

	sound (self, CHAN_VOICE, self.noise, 1, ATTN_NORM);

	self.state = STATE_UP;
	SUB_CalcMove (self.pos2, self.speed, button_wait);
};


void() button_use =
{
	self.enemy = activator;
	button_fire ();
};

void() button_touch =
{
	if (other.classname != "player")
		return;
	self.enemy = other;
	button_fire ();
};

void() button_killed =
{
	self.enemy = damage_attacker;
	self.health = self.max_health;
	self.takedamage = DAMAGE_NO;	// wil be reset upon return
	button_fire ();
};


/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"sounds"
0) steam metal
1) wooden clunk
2) metallic click
3) in-out
*/

float		correct=9999, wrong=8888;
void() func_button =
{
local float		gtemp, ftemp;
	if (self.sounds == correct)
	{
		precache_sound ("gcompris/misc/good.wav");
		self.noise = "gcompris/misc/good.wav";
	}
	if (self.sounds == wrong)
	{
		precache_sound ("gcompris/misc/check_answer.wav");
		self.noise = "gcompris/misc/check_answer.wav";
	}

	if (self.sounds == 0)
	{
		precache_sound ("gcompris/alphabet/U0030.wav");
		self.noise = "gcompris/alphabet/U0030.wav";
	}
	if (self.sounds == 1)
	{
		precache_sound ("gcompris/alphabet/U0031.wav");
		self.noise = "gcompris/alphabet/U0031.wav";
	}
	if (self.sounds == 2)
	{
		precache_sound ("gcompris/alphabet/U0032.wav");
		self.noise = "gcompris/alphabet/U0032.wav";
	}
	if (self.sounds == 3)
	{
		precache_sound ("gcompris/alphabet/U0033.wav");
		self.noise = "gcompris/alphabet/U0033.wav";
	}
	else if (self.sounds == 4)
	{
		precache_sound ("gcompris/alphabet/U0034.wav");
		self.noise = "gcompris/alphabet/U0034.wav";
	}
	else if (self.sounds == 5)
	{
		precache_sound ("gcompris/alphabet/U0035.wav");
		self.noise = "gcompris/alphabet/U0035.wav";
	}
	else if (self.sounds == 6)
	{
		precache_sound ("gcompris/alphabet/U0036.wav");
		self.noise = "gcompris/alphabet/U0036.wav";
	}
	
	else if (self.sounds == 7)
	{
		precache_sound ("gcompris/alphabet/U0037.wav");
		self.noise = "gcompris/alphabet/U0037.wav";
	}
	else if (self.sounds == 8)
	{
		precache_sound ("gcompris/alphabet/U0038.wav");
		self.noise = "gcompris/alphabet/U0038.wav";
	}
	else if (self.sounds == 9)
	{
		precache_sound ("gcompris/alphabet/U0039.wav");
		self.noise = "gcompris/alphabet/U0039.wav";
	}
	else if (self.sounds == 10)
	{
		precache_sound ("gcompris/alphabet/U0061.wav");
		self.noise = "gcompris/alphabet/U0061.wav";
	}
	else if (self.sounds == 11)
	{
		precache_sound ("gcompris/alphabet/U0062.wav");
		self.noise = "gcompris/alphabet/U0062.wav";
	}
	else if (self.sounds == 12)
	{
		precache_sound ("gcompris/alphabet/U0063.wav");
		self.noise = "gcompris/alphabet/U0063.wav";
	}
	else if (self.sounds == 13)
	{
		precache_sound ("gcompris/alphabet/U0063.wav");
		self.noise = "gcompris/alphabet/U0064.wav";
	}
	else if (self.sounds == 12)
	{
		precache_sound ("gcompris/alphabet/U0063.wav");
		self.noise = "gcompris/alphabet/U0063.wav";
	}
	else if (self.sounds == 13)
	{
		precache_sound ("gcompris/alphabet/U0063.wav");
		self.noise = "gcompris/alphabet/U0063.wav";
	}
	else if (self.sounds == 14)
	{
		precache_sound ("gcompris/alphabet/U0064.wav");
		self.noise = "gcompris/alphabet/U0064.wav";
	}
	else if (self.sounds == 15)
	{
		precache_sound ("gcompris/alphabet/U0065.wav");
		self.noise = "gcompris/alphabet/U0065.wav";
	}
	
	SetMovedir ();

	self.movetype = MOVETYPE_PUSH;
	self.solid = SOLID_BSP;
	setmodel (self, self.model);

	self.blocked = button_blocked;
	self.use = button_use;

	if (self.health)
	{
		self.max_health = self.health;
		self.th_die = button_killed;
		self.takedamage = DAMAGE_YES;
	}
	else
		self.touch = button_touch;

	if (!self.speed)
		self.speed = 40;
	if (!self.wait)
		self.wait = 1;
	if (!self.lip)
		self.lip = 4;

	self.state = STATE_BOTTOM;

	self.pos1 = self.origin;
	self.pos2 = self.pos1 + self.movedir*(fabs(self.movedir*self.size) - self.lip);
};

