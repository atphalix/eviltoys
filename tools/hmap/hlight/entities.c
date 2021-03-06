// entities.c

#include "light.h"

entity_t	entities[MAX_MAP_ENTITIES];
int			num_entities;

/*
==============================================================================

ENTITY FILE PARSING

If a light has a targetname, generate a unique style in the 32-63 range
==============================================================================
*/

int		numlighttargets;
char	lighttargets[32][64];

int LightStyleForTargetname (char *targetname, qboolean alloc)
{
	int		i;

	for (i=0 ; i<numlighttargets ; i++)
		if (!strcmp (lighttargets[i], targetname))
			return 32 + i;
	if (!alloc)
		return -1;
	strcpy (lighttargets[i], targetname);
	numlighttargets++;
	return numlighttargets-1 + 32;
}


/*
==================
MatchTargets
==================
*/
void MatchTargets (void)
{
	int		i,j;

	for (i=0 ; i<num_entities ; i++)
	{
		if (!entities[i].target[0])
			continue;

		for (j=0 ; j<num_entities ; j++)
			if (!strcmp(entities[j].targetname, entities[i].target))
			{
				entities[i].targetent = &entities[j];
				// set up spotlight values for lighting code to use
				VectorSubtract (entities[i].targetent->origin, entities[i].origin, entities[i].spotdir);
				VectorNormalize (entities[i].spotdir);
				if (!entities[i].angle)
					entities[i].spotcone = -cos(20 * Q_PI / 180);
				else
					entities[i].spotcone = -cos(entities[i].angle / 2 * Q_PI / 180);
				break;
			}

		if (j==num_entities)
		{
			printf ("WARNING: entity at (%i,%i,%i) (%s) has unmatched target\n", (int)entities[i].origin[0], (int)entities[i].origin[1], (int)entities[i].origin[2], entities[i].classname);
			continue;
		}

		// set the style on the source ent for switchable lights
		if (entities[j].style)
		{
			char	s[16];
			entities[i].style = entities[j].style;
			sprintf (s,"%i", entities[i].style);
			SetKeyValue (&entities[i], "style", s);
		}

		if (entities[i].spotcone <= 0)
		{
			VectorClear(entities[i].spotdir);
			entities[i].spotcone = 0;
		}
	}
}

void WriteLights(void)
{
	int i;
	FILE *f;
	entity_t *e;
	printf ("building .lights file\n");
	f = fopen(lightsfilename, "wb");
	for (i = 0;i < num_entities;i++)
	{
		e = entities + i;
		if (e->light)
			fprintf(f, "%f %f %f %f %f %f %f %f %f %f %f %f %f %d\n", (double) e->origin[0], (double) e->origin[1], (double) e->origin[2], (double) e->falloff, (double) e->color[0], (double) e->color[1], (double) e->color[2], (double) e->subbrightness, (double) e->spotdir[0], (double) e->spotdir[1], (double) e->spotdir[2], (double) e->spotcone, (double) e->lightoffset, e->style);
	}
	fclose(f);
}


/*
==================
LoadEntities
==================
*/
void LoadEntities (void)
{
	char 		*data;
	entity_t	*entity;
	char		key[64];
	epair_t		*epair;
	double		vec[4];
	double		temp, color2[3];
	int			i;

	data = dentdata;
	//
	// start parsing
	//
	num_entities = 0;

	// go through all the entities
	while (1)
	{
		// parse the opening brace
		data = COM_Parse (data);
		if (!data)
			break;
		if (com_token[0] != '{')
			Error ("LoadEntities: found %s when expecting {", com_token);

		if (num_entities == MAX_MAP_ENTITIES)
			Error ("LoadEntities: MAX_MAP_ENTITIES");
		entity = &entities[num_entities];
		num_entities++;

		entity->light = 0;
		entity->color[0] = entity->color[1] = entity->color[2] = 1.0f;
		color2[0] = color2[1] = color2[2] = 1.0f;
		entity->falloff = DEFAULTFALLOFF * DEFAULTFALLOFF;
		entity->lightradius = 0;
		entity->lightoffset = LIGHTDISTBIAS;

		// go through all the keys in this entity
		while (1)
		{
			int		c;

			// parse key
			data = COM_Parse (data);
			if (!data)
				Error ("LoadEntities: EOF without closing brace");
			if (!strcmp(com_token,"}"))
				break;
			strcpy (key, com_token);

			// parse value
			data = COM_Parse (data);
			if (!data)
				Error ("LoadEntities: EOF without closing brace");
			c = com_token[0];
			if (c == '}')
				Error ("LoadEntities: closing brace without data");

			epair = malloc (sizeof(epair_t));
			memset (epair, 0, sizeof(epair));
			strcpy (epair->key, key);
			strcpy (epair->value, com_token);
			epair->next = entity->epairs;
			entity->epairs = epair;

			if (!strcmp(key, "classname"))
				strcpy (entity->classname, com_token);
			else if (!strcmp(key, "target"))
				strcpy (entity->target, com_token);
			else if (!strcmp(key, "targetname"))
				strcpy (entity->targetname, com_token);
			else if (!strcmp(key, "origin"))
			{
				// scan into doubles, then assign
				// which makes it vec_t size independent
				if (sscanf(com_token, "%lf %lf %lf", &vec[0], &vec[1], &vec[2]) != 3)
					Error ("LoadEntities: not 3 values for origin");
				for (i=0 ; i<3 ; i++)
					entity->origin[i] = vec[i];
			}
			else if (!strncmp(key, "light", 5) || !strcmp (key, "_light") )
			{
				i = sscanf(com_token, "%lf %lf %lf %lf", &vec[0], &vec[1], &vec[2], &vec[3]);
				switch (i)
				{
				case 4:// HalfLife light
					entity->light = vec[3];
					entity->color[0] = vec[0] * (1.0f / 255.0f);
					entity->color[1] = vec[1] * (1.0f / 255.0f);
					entity->color[2] = vec[2] * (1.0f / 255.0f);
					break;
				case 3:
					entity->light = 1;
					entity->color[0] = vec[0];
					entity->color[1] = vec[1];
					entity->color[2] = vec[2];
					break;
				case 1:
					entity->light = vec[0];
					entity->color[0] = 1.0f;
					entity->color[1] = 1.0f;
					entity->color[2] = 1.0f;
					break;
				default:
					Error ("LoadEntities: _light (or light) key must be 1 (Quake), 4 (HalfLife), or 3 (HLight) values, \"%s\" is not valid\n", com_token);
				}
			}
			else if (!strcmp (key, "wait") )
			{
				entity->falloff = atof(com_token);
				entity->falloff *= entity->falloff; // square it for lighting calculations
			}
			else if (!strcmp (key, "_lightradius") )
				entity->lightradius = atof(com_token);
			else if (!strcmp(key, "style"))
			{
				entity->style = atof(com_token);
				if ((unsigned)entity->style > 254)
					Error ("LoadEntities: Bad light style %i (must be 0-254)", entity->style);
			}
			else if (!strcmp(key, "angle"))
				entity->angle = atof(com_token);
			else if (!strcmp(key, "color") || !strcmp (key, "_color"))
			{
				// scan into doubles, then assign
				// which makes it vec_t size independent
				if (sscanf(com_token, "%lf %lf %lf", &vec[0], &vec[1], &vec[2]) != 3)
					Error ("LoadEntities: not 3 values for color");
				// scale the color to have at least one component at 1.0
				temp = vec[0];
				if (vec[1] > temp) temp = vec[1];
				if (vec[2] > temp) temp = vec[2];
				if (temp != 0.0) temp = 1.0 / temp;
				color2[0] = vec[0] * temp;
				color2[1] = vec[1] * temp;
				color2[2] = vec[2] * temp;
			}
		}

		// all fields have been parsed
		if (!strncmp (entity->classname, "light", 5))
			if (!entity->light)
				entity->light = DEFAULTLIGHTLEVEL;

		if (entity->light)
		{
			// convert to subtraction to the brightness for the whole light, so it will fade nicely, rather than being clipped off
			entity->color[0] *= color2[0] * entity->light * /*entity->falloff */ 16384.0 * globallightscale;
			entity->color[1] *= color2[1] * entity->light * /*entity->falloff */ 16384.0 * globallightscale;
			entity->color[2] *= color2[2] * entity->light * /*entity->falloff */ 16384.0 * globallightscale;
			/*
			// optimization of light size
			                             temp = entity->color[0];
			if (temp < entity->color[1]) temp = entity->color[1];
			if (temp < entity->color[2]) temp = entity->color[2];
			entity->light *= temp;
			temp = 1.0f / temp;
			entity->color[0] *= temp;
			entity->color[1] *= temp;
			entity->color[2] *= temp;
			if (entity->lightradius)
				entity->subbrightness = entity->light * 16384.0 / (entity->lightradius * entity->lightradius * entity->falloff + 1.0);
			*/
			if (entity->lightradius)
				entity->subbrightness = 1.0 / (entity->lightradius * entity->lightradius * entity->falloff + LIGHTDISTBIAS);
			if (entity->subbrightness < (1.0 / 1048576.0))
				entity->subbrightness = (1.0 / 1048576.0);
		}

		if (!strcmp (entity->classname, "light"))
			if (entity->targetname[0] && !entity->style)
			{
				char	s[16];
				entity->style = LightStyleForTargetname (entity->targetname, true);
				sprintf (s,"%i", entity->style);
				SetKeyValue (entity, "style", s);
			}
	}

	printf ("%d entities read\n", num_entities);

	MatchTargets ();

	WriteLights();
}

char 	*ValueForKey (entity_t *ent, char *key)
{
	epair_t	*ep;

	for (ep=ent->epairs ; ep ; ep=ep->next)
		if (!strcmp (ep->key, key) )
			return ep->value;
	return "";
}

void 	SetKeyValue (entity_t *ent, char *key, char *value)
{
	epair_t	*ep;

	for (ep=ent->epairs ; ep ; ep=ep->next)
		if (!strcmp (ep->key, key) )
		{
			strcpy (ep->value, value);
			return;
		}
	ep = malloc (sizeof(*ep));
	ep->next = ent->epairs;
	ent->epairs = ep;
	strcpy (ep->key, key);
	strcpy (ep->value, value);
}

entity_t *FindEntityWithKeyPair( char *key, char *value )
{
	entity_t *ent;
	epair_t	*ep;
	int i;

	for (i=0 ; i<num_entities ; i++)
	{
		ent = &entities[ i ];
		for (ep=ent->epairs ; ep ; ep=ep->next)
		{
			if (!strcmp (ep->key, key) )
			{
				if ( !strcmp( ep->value, value ) )
					return ent;
				break;
			}
		}
	}
	return NULL;
}

void GetVectorForKey (entity_t *ent, char *key, vec3_t vec)
{
	char	*k;

	k = ValueForKey (ent, key);
#ifdef DOUBLEVEC_T
	sscanf (k, "%lf %lf %lf", &vec[0], &vec[1], &vec[2]);
#else
	sscanf (k, "%f %f %f", &vec[0], &vec[1], &vec[2]);
#endif
}



/*
================
WriteEntitiesToString
================
*/
void WriteEntitiesToString (void)
{
	char	*buf, *end;
	epair_t	*ep;
	char	line[MAX_KEY + MAX_VALUE + 7]; // LordHavoc: thanks to metlslime for pointing out this fix (was 128)
	int		i;

	buf = dentdata;
	end = buf;
	*end = 0;

	printf ("%i switchable light styles\n", numlighttargets);

	for (i=0 ; i<num_entities ; i++)
	{
		ep = entities[i].epairs;
		if (!ep)
			continue;	// ent got removed

		strcat (end,"{\n");
		end += 2;

		for (ep = entities[i].epairs ; ep ; ep=ep->next)
		{
			sprintf (line, "\"%s\" \"%s\"\n", ep->key, ep->value);
			strcat (end, line);
			end += strlen(line);
		}
		strcat (end,"}\n");
		end += 2;

		if (end > buf + MAX_MAP_ENTSTRING)
			Error ("Entity text too long");
	}
	entdatasize = end - buf + 1;
}

