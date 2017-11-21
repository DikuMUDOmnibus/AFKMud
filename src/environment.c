/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2007 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine, and Adjani.    *
 * All Rights Reserved.                                                     *
 * Registered with the United States Copyright Office. TX 5-877-286         *
 *                                                                          *
 * External contributions from Xorith, Quixadhal, Zarius, and many others.  *
 *                                                                          *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                Overland Map Environmental Affects Code                   *
 *  Modified from code used in Altanos 9 (Shatai and Eizneckam) by Samson   *
 ****************************************************************************/

#include "mud.h"
#include "overland.h"
#include "environment.h"

ENV_DATA *first_env;
ENV_DATA *last_env;

typedef enum
{
   ENV_QUAKE, ENV_FIRE, ENV_RAIN, ENV_THUNDER, ENV_TORNADO, ENV_MAX
} env_types;

char *const env_name[] = {
   "quake", "fire", "rainstorm", "thunderstorm", "tornado"
};

char *env_distances[] = {
   "hundreds of miles away in the distance",
   "far off in the skyline",
   "many miles away at great distance",
   "far off many miles away",
   "tens of miles away in the distance",
   "far off in the distance",
   "several miles away",
   "off in the distance",
   "not far from here",
   "in the near vicinity",
   "in the immediate area"
};

void free_env( ENV_DATA * env )
{
   UNLINK( env, first_env, last_env, next, prev );
   DISPOSE( env );
}

void free_envs( void )
{
   ENV_DATA *env, *env_next;

   for( env = first_env; env; env = env_next )
   {
      env_next = env->next;
      free_env( env );
   }
   return;
}

int get_env_type( char *type )
{
   int x;

   for( x = 0; x < ENV_MAX; x++ )
      if( !str_cmp( type, env_name[x] ) )
         return x;
   return -1;
}

void environment_pulse_update( void )
{
   ENV_DATA *env, *env_next;

   for( env = first_env; env; env = env_next )
   {
      env_next = env->next;
      switch ( env->type )
      {
         case ENV_FIRE:
         case ENV_RAIN:
         case ENV_THUNDER:
         case ENV_TORNADO:
            switch ( env->direction )
            {
               case DIR_NORTH:
                  env->y--;
                  break;
               case DIR_EAST:
                  env->x++;
                  break;
               case DIR_SOUTH:
                  env->y++;
                  break;
               case DIR_WEST:
                  env->x--;
                  break;
               case DIR_SOUTHWEST:
                  env->y++;
                  env->x--;
                  break;
               case DIR_NORTHWEST:
                  env->y--;
                  env->x--;
                  break;
               case DIR_NORTHEAST:
                  env->y--;
                  env->x++;
                  break;
               case DIR_SOUTHEAST:
                  env->y++;
                  env->x++;
                  break;
               default:
                  break;
            }
            break;
         case ENV_QUAKE:
            env->x += 1;
            env->y += 1;
            break;
      }
      env->time_left--;
      if( env->time_left < 1 )
         free_env( env );
   }
}

void environment_actual_update( void )
{
   ENV_DATA *env;
   DESCRIPTOR_DATA *d;
   OBJ_DATA *obj;
   CHAR_DATA *ch = NULL;
   int atx, aty, atmap;

   for( d = first_descriptor; d; d = d->next )
   {
      if( d->connected != CON_PLAYING )
         continue;

      ch = d->character;

      if( !IS_PLR_FLAG( ch, PLR_ONMAP ) )
         continue;

      atx = ch->x;
      aty = ch->y;
      atmap = ch->map;

      for( env = first_env; env; env = env->next )
      {
         if( distance( atx, aty, env->x, env->y ) > env->radius )
            continue;

         switch ( env->type )
         {
            case ENV_FIRE:
               if( number_range( 0, 3 ) == 2 )
               {
                  OBJ_DATA *fire;
                  bool firefound = FALSE;

                  for( fire = ch->in_room->first_content; fire; fire = fire->next_content )
                  {
                     if( fire->pIndexData->vnum != OBJ_VNUM_OVFIRE )
                        continue;

                     if( fire->x == ch->x && fire->y == ch->y )
                     {
                        firefound = TRUE;
                        break;
                     }
                  }

                  if( !firefound )
                  {
                     send_to_char( "&RScorching flames erupt all around!\n\r", ch );
                     if( !( obj = create_object( get_obj_index( OBJ_VNUM_OVFIRE ), 0 ) ) )
                        log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
                     else
                     {
                        obj->timer = env->time_left;
                        obj->x = ch->x;
                        obj->y = ch->y;
                        obj->map = ch->map;
                        obj_to_room( obj, ch->in_room, ch );
                     }
                  }
               }
               break;
            case ENV_QUAKE:
               send_to_char( "&OThe rumble of an earthquake roars loudly!\n\r", ch );
               break;
            case ENV_RAIN:
               send_to_char( "&cA gentle rain falls from the sky.\n\r", ch );
               break;
            case ENV_THUNDER:
               switch ( number_range( 0, 5 ) )
               {
                  case 0:
                     send_to_char( "&YCracks of lightning hit the ground!\n\r", ch );
                     break;
                  case 1:
                     send_to_char( "&YThe sky lights up brightly!\n\r", ch );
                     break;
                  case 2:
                     send_to_char( "&YYou are almost blinded by lightning!\n\r", ch );
                     break;
                  case 3:
                     send_to_char( "&YLightning crashes down in front of you!\n\r", ch );
                     break;
                  default:
                     send_to_char( "&YLightning flashes in the sky!\n\r", ch );
                     break;
               }
               break;
            case ENV_TORNADO:
               send_to_char( "&cThe roar of the tornado hurts your ears!\n\r", ch );
               ch->x = ( atx + number_range( 5, 10 ) + env->radius / 2 );
               ch->y = ( aty + number_range( 5, 15 ) + env->radius / 2 );
               send_to_char( "&RThe force of the tornado TOSSES you around!\n\r", ch );
               interpret( ch, "look" );
               break;
         }
      }
   }
   return;
}

void generate_random_environment( int type )
{
   ENV_DATA *q;
   int schance = 0;

   schance = number_range( 1, 100 );

   CREATE( q, ENV_DATA, 1 );
   LINK( q, first_env, last_env, next, prev );
   q->type = type;
   q->x = number_range( 0, MAX_X );
   q->y = number_range( 0, MAX_Y );
   q->map = number_range( MAP_ALSHEROK, MAP_MAX - 1 );
   switch ( type )
   {
      case ENV_FIRE:
         q->radius = number_range( 1, 10 );
         q->damage_per_shake = number_range( 5, 25 );
         break;
      case ENV_QUAKE:
         q->time_left = number_range( 10, 50 );
         q->radius = number_range( 50, 75 );
         q->damage_per_shake = number_range( 5, 25 );
         q->intensity = number_range( 1, 8 );
         break;
      case ENV_RAIN:
      case ENV_THUNDER:
         q->radius = number_range( 1, 25 );
         q->damage_per_shake = number_range( 5, 25 );
         break;
      case ENV_TORNADO:
         q->time_left = number_range( 10, 50 );
         q->radius = number_range( 1, 4 );
         q->damage_per_shake = number_range( 5, 25 );
         break;
   }
   if( type == ENV_QUAKE )
   {
      q->direction = 10;
      return;
   }
   if( schance < 15 )
   {
      q->direction = DIR_NORTH;
      return;
   }
   else if( schance < 30 )
   {
      q->direction = DIR_SOUTHEAST;
      return;
   }
   else if( schance < 50 )
   {
      q->direction = DIR_EAST;
      return;
   }
   else if( schance < 75 )
   {
      q->direction = DIR_SOUTHWEST;
      return;
   }
   else
   {
      q->direction = 15;
      return;
   }
}

void environment_update( void )
{
   if( number_range( 1, 100 ) > 95 )
   {
      int schance, type = ENV_RAIN;

      schance = number_range( 1, 100 );
      if( schance < 100 )
         type = ENV_QUAKE;
      if( schance < 80 )
         type = ENV_TORNADO;
      if( schance < 60 )
         type = ENV_FIRE;
      if( schance < 40 )
         type = ENV_THUNDER;
      if( schance < 20 )
         type = ENV_RAIN;

      generate_random_environment( type );
   }
   environment_actual_update(  );
   environment_pulse_update(  );
   return;
}

CMDF do_makeenv( CHAR_DATA * ch, char *argument )
{
   char etype[MIL], arg2[MIL], arg[MIL];
   int chosenradius;
   int door, atype, intensity = 0;
   int atx = -1, aty = -1, atmap = -1;
   ENV_DATA *t;

   if( IS_NPC( ch ) )
   {
      send_to_char( "NPCs cannot create environmental affects.\n\r", ch );
      return;
   }

   if( !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      send_to_char( "This can only be done on the overland.\n\r", ch );
      return;
   }

   argument = one_argument( argument, etype );
   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( !etype || etype[0] == '\0' )
   {
      send_to_char( "What type of affect are you trying to create?\n\r", ch );
      send_to_char( "Possible affects are:\n\r\n\r", ch );
      send_to_char( "quake fire rainstorm thunderstorm tornado\n\r", ch );
      return;
   }

   atype = get_env_type( etype );
   if( atype < 0 || atype >= ENV_MAX )
   {
      do_makeenv( ch, "" );
      return;
   }

   atx = ch->x;
   aty = ch->y;
   atmap = ch->map;

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Usage: makeenv <type> <radius> <direction> <intensity>\n\r", ch );
      send_to_char( "Intensity is used only for earthquakes and is ignored otherwise.\n\r", ch );
      return;
   }

   if( !is_number( arg ) )
   {
      send_to_char( "Radius value must be numerical.\n\r", ch );
      return;
   }

   if( !is_number( argument ) && atype == ENV_QUAKE )
   {
      send_to_char( "Intensity value must be numerical.\n\r", ch );
      return;
   }

   intensity = atoi( argument );
   if( intensity < 1 && atype == ENV_QUAKE )
   {
      send_to_char( "Intensity must be greater than zero.\n\r", ch );
      return;
   }

   chosenradius = atoi( arg );

   door = get_dirnum( arg2 );
   if( door < 0 || door > MAX_DIR )
      door = DIR_SOMEWHERE;

   if( door != DIR_SOMEWHERE && atype != ENV_QUAKE )
      ch_printf( ch, "You create a %s to the %s!\n\r", etype, dir_name[door] );
   else
   {
      if( atype == ENV_QUAKE )
         send_to_char( "You create an earthquake!\n\r", ch );
      else
         ch_printf( ch, "You create a non-moving %s!\n\r", etype );
   }
   CREATE( t, ENV_DATA, 1 );
   t->x = atx;
   t->y = aty;
   t->map = atmap;
   t->type = atype;
   t->direction = door;
   t->radius = chosenradius;
   t->damage_per_shake = ( ch->level / 4 );
   t->time_left = number_range( 10, 50 );
   t->intensity = intensity;
   LINK( t, first_env, last_env, next, prev );
}

CMDF do_env( CHAR_DATA * ch, char *argument )
{
   int count = 0;
   ENV_DATA *env;

   for( env = first_env; env; env = env->next )
   {
      count++;
      if( env->type == ENV_QUAKE )
      {
         ch_printf( ch, "&GA %d by %d, intensity %d, earthquake at coordinates %dX %dY on %s.\n\r",
                    env->radius, env->radius, env->intensity, env->x, env->y, map_names[env->map] );
      }
      else
      {
         ch_printf( ch, "&GA %d by %d %s bound %s at coordinates %d,%d on %s.\n\r",
                    env->radius, env->radius,
                    ( env->direction < 10 ) ? dir_name[env->direction] : "nowhere",
                    env_name[env->type], env->x, env->y, map_names[env->map] );
      }
   }

   if( count == 0 )
   {
      send_to_char( "No environmental effects found.\n\r", ch );
      return;
   }
   ch_printf( ch, "&G%d total environmental effects found.\n\r", count );
   return;
}

bool survey_environment( CHAR_DATA * ch )
{
   ENV_DATA *env;
   double dist, angle, eta;
   int dir = -1, iMes;
   bool found = FALSE;

   if( !IS_PLR_FLAG( ch, PLR_ONMAP ) )
      return FALSE;

   send_to_char( "&WAn imp appears with an ear-splitting BANG!\n\r", ch );

   for( env = first_env; env; env = env->next )
   {
      if( ch->map != env->map )
         continue;

      dist = ( int )distance( ch->x, ch->y, env->x, env->y );

      if( dist > 300 )
         continue;

      found = TRUE;

      angle = calc_angle( ch->x, ch->y, env->x, env->y, &dist );

      if( angle == -1 )
         dir = -1;
      else if( angle >= 360 )
         dir = DIR_NORTH;
      else if( angle >= 315 )
         dir = DIR_NORTHWEST;
      else if( angle >= 270 )
         dir = DIR_WEST;
      else if( angle >= 225 )
         dir = DIR_SOUTHWEST;
      else if( angle >= 180 )
         dir = DIR_SOUTH;
      else if( angle >= 135 )
         dir = DIR_SOUTHEAST;
      else if( angle >= 90 )
         dir = DIR_EAST;
      else if( angle >= 45 )
         dir = DIR_NORTHEAST;
      else if( angle >= 0 )
         dir = DIR_NORTH;

      if( dist > 200 )
         iMes = 0;
      else if( dist > 150 )
         iMes = 1;
      else if( dist > 100 )
         iMes = 2;
      else if( dist > 75 )
         iMes = 3;
      else if( dist > 50 )
         iMes = 4;
      else if( dist > 25 )
         iMes = 5;
      else if( dist > 15 )
         iMes = 6;
      else if( dist > 10 )
         iMes = 7;
      else if( dist > 5 )
         iMes = 8;
      else if( dist > 1 )
         iMes = 9;
      else
         iMes = 10;

      eta = dist * 10;
      if( dir == -1 )
         ch_printf( ch, "&GAn %s bound &R%s&G within the immediate vicinity.\n\r",
                    dir_name[env->direction], env_name[env->type] );
      else
         ch_printf( ch, "&GA &R%s&G %s in the general %sern direction.\n\r",
                    env_name[env->type], env_distances[iMes], dir_name[dir] );
      if( eta / 60 > 0 )
         ch_printf( ch, "&GThis %s is about %d minutes away from you.\n\r", env_name[env->type], ( int )( eta / 60 ) );
      else
         ch_printf( ch, "&GThis %s should be in the vicinity now.\n\r", env_name[env->type] );
   }

   if( !found )
   {
      send_to_char( "&GNo environmental affects to report!\n\r", ch );
      send_to_char( "&WThe imp vanishes with an ear-splitting BANG!\n\r", ch );
      return FALSE;
   }

   send_to_char( "&WThe imp vanishes with an ear-splitting BANG!\n\r", ch );
   return TRUE;
}
