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
 *                      Online Reset Editing Module                         *
 ****************************************************************************/

#include <string.h>
#include "mud.h"
#include "overland.h"

/* Externals */
extern int top_reset;

int get_trapflag( char *flag );
void name_generator( char *array );
void pick_name( char *name, char *file );

/*
 * Find some object with a given index data.
 * Used by area-reset 'P', 'T' and 'H' commands.
 */
OBJ_DATA *get_obj_type( OBJ_INDEX_DATA * pObjIndex )
{
   OBJ_DATA *obj;

   for( obj = first_object; obj; obj = obj->next )
   {
      if( obj->pIndexData == pObjIndex )
         return obj;
   }
   return NULL;
}

/* Find an object in a room so we can check it's dependents. Used by 'O' resets. */
OBJ_DATA *get_obj_room( OBJ_INDEX_DATA *pObjIndex, ROOM_INDEX_DATA *pRoomIndex )
{
   OBJ_DATA *obj;

   for( obj = pRoomIndex->first_content; obj; obj = obj->next_content )
   {
      if( obj->pIndexData == pObjIndex )
         return obj;
   }
   return NULL;
}

/*
 * Make a trap.
 */
OBJ_DATA *make_trap( int v0, int v1, int v2, int v3 )
{
   OBJ_DATA *trap;

   if( !( trap = create_object( get_obj_index( OBJ_VNUM_TRAP ), 0 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return NULL;
   }
   trap->timer = 0;
   trap->value[0] = v0;
   trap->value[1] = v1;
   trap->value[2] = v2;
   trap->value[3] = v3;
   return trap;
}

char *sprint_reset( RESET_DATA * pReset, short &num )
{
   RESET_DATA *tReset, *gReset;
   static char buf[MSL];
   char mobname[MSL], roomname[MSL], objname[MSL];
   static ROOM_INDEX_DATA *room;
   static OBJ_INDEX_DATA *obj, *obj2;
   static MOB_INDEX_DATA *mob;

   switch ( pReset->command )
   {
      default:
         snprintf( buf, MSL, "%2d) *** BAD RESET: %c %d %d %d %d ***\n\r",
                   num, pReset->command, pReset->extra, pReset->arg1, pReset->arg2, pReset->arg3 );
         break;

      case 'M':
         mob = get_mob_index( pReset->arg1 );
         room = get_room_index( pReset->arg3 );
         if( mob )
            mudstrlcpy( mobname, mob->player_name, MSL );
         else
            mudstrlcpy( mobname, "Mobile: *BAD VNUM*", MSL );
         if( room )
            mudstrlcpy( roomname, room->name, MSL );
         else
            mudstrlcpy( roomname, "Room: *BAD VNUM*", MSL );
         if( pReset->arg4 != -1 && pReset->arg5 != -1 && pReset->arg6 != -1 )
            snprintf( buf, MSL, "%2d) %s (%d) -> Overland: %s %d %d [%d]\n\r", num, mobname, pReset->arg1,
                      map_names[pReset->arg4], pReset->arg5, pReset->arg6, pReset->arg2 );
         else
            snprintf( buf, MSL, "%2d) %s (%d) -> %s Room: %d [%d]\n\r", num, mobname, pReset->arg1,
                      roomname, pReset->arg3, pReset->arg2 );

         for( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset )
         {
            num++;
            switch ( tReset->command )
            {
               case 'E':
                  if( !mob )
                     mudstrlcpy( mobname, "* ERROR: NO MOBILE! *", MSL );
                  if( !( obj = get_obj_index( tReset->arg1 ) ) )
                     mudstrlcpy( objname, "Object: *BAD VNUM*", MSL );
                  else
                     mudstrlcpy( objname, obj->name, MSL );
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (equip) %s (%d) -> %s (%s) [%d]\n\r",
                            num, objname, tReset->arg1, mobname, wear_locs[tReset->arg3], tReset->arg2 );
                  break;

               case 'G':
                  if( !mob )
                     mudstrlcpy( mobname, "* ERROR: NO MOBILE! *", MSL );
                  if( !( obj = get_obj_index( tReset->arg1 ) ) )
                     mudstrlcpy( objname, "Object: *BAD VNUM*", MSL );
                  else
                     mudstrlcpy( objname, obj->name, MSL );
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (carry) %s (%d) -> %s [%d]\n\r",
                            num, objname, tReset->arg1, mobname, tReset->arg2 );
                  break;
            }
            if( tReset->first_reset )
            {
               for( gReset = tReset->first_reset; gReset; gReset = gReset->next_reset )
               {
                  num++;
                  switch ( gReset->command )
                  {
                     case 'P':
                        if( !( obj2 = get_obj_index( gReset->arg1 ) ) )
                           mudstrlcpy( objname, "Object1: *BAD VNUM*", MSL );
                        else
                           mudstrlcpy( objname, obj2->name, MSL );
                        if( gReset->arg3 > 0 && ( obj = get_obj_index( gReset->arg3 ) ) == NULL )
                           mudstrlcpy( roomname, "Object2: *BAD VNUM*", MSL );
                        else if( !obj )
                           mudstrlcpy( roomname, "Object2: *NULL obj*", MSL );
                        else
                           mudstrlcpy( roomname, obj->name, MSL );
                        snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (put) %s (%d) -> %s (%d) [%d]\n\r",
                                  num, objname, gReset->arg1, roomname, obj ? obj->vnum : gReset->arg3, gReset->arg2 );
                        break;
                  }
               }
            }
         }
         break;

      case 'O':
         if( !( obj = get_obj_index( pReset->arg1 ) ) )
            mudstrlcpy( objname, "Object: *BAD VNUM*", MSL );
         else
            mudstrlcpy( objname, obj->name, MSL );
         room = get_room_index( pReset->arg3 );
         if( !room )
            mudstrlcpy( roomname, "Room: *BAD VNUM*", MSL );
         else
            mudstrlcpy( roomname, room->name, MSL );
         if( pReset->arg4 != -1 && pReset->arg5 != -1 && pReset->arg6 != -1 )
            snprintf( buf, MSL, "%2d) (object) %s (%d) -> Overland: %s %d %d [%d]\n\r", num, objname, pReset->arg1,
                      map_names[pReset->arg4], pReset->arg5, pReset->arg6, pReset->arg2 );
         else
            snprintf( buf, MSL, "%2d) (object) %s (%d) -> %s Room: %d [%d]\n\r",
                      num, objname, pReset->arg1, roomname, pReset->arg3, pReset->arg2 );

         for( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset )
         {
            num++;
            switch ( tReset->command )
            {
               case 'P':
                  if( !( obj2 = get_obj_index( tReset->arg1 ) ) )
                     mudstrlcpy( objname, "Object1: *BAD VNUM*", MSL );
                  else
                     mudstrlcpy( objname, obj2->name, MSL );
                  if( tReset->arg3 > 0 && ( obj = get_obj_index( tReset->arg3 ) ) == NULL )
                     mudstrlcpy( roomname, "Object2: *BAD VNUM*", MSL );
                  else if( !obj )
                     mudstrlcpy( roomname, "Object2: *NULL obj*", MSL );
                  else
                     mudstrlcpy( roomname, obj->name, MSL );
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (put) %s (%d) -> %s (%d) [%d]\n\r",
                            num, objname, tReset->arg1, roomname, obj ? obj->vnum : tReset->arg3, tReset->arg2 );
                  break;

               case 'T':
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (trap) %d %d %d %d (%s) -> %s (%d)\n\r",
                            num, tReset->extra, tReset->arg1, tReset->arg2, tReset->arg3, flag_string( tReset->extra,
                                                                                                       trap_flags ), objname,
                            obj ? obj->vnum : 0 );
                  break;

               case 'H':
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (hide) -> %s\n\r", num, objname );
                  break;
            }
         }
         break;

      case 'D':
         if( pReset->arg2 < 0 || pReset->arg2 > MAX_DIR + 1 )
            pReset->arg2 = 0;
         if( !( room = get_room_index( pReset->arg1 ) ) )
         {
            mudstrlcpy( roomname, "Room: *BAD VNUM*", MSL );
            snprintf( objname, MSL, "%s (no exit)", dir_name[pReset->arg2] );
         }
         else
         {
            mudstrlcpy( roomname, room->name, MSL );
            snprintf( objname, MSL, "%s%s", dir_name[pReset->arg2], get_exit( room, pReset->arg2 ) ? "" : " (NO EXIT!)" );
         }
         switch ( pReset->arg3 )
         {
            default:
               mudstrlcpy( mobname, "(* ERROR *)", MSL );
               break;
            case 0:
               mudstrlcpy( mobname, "Open", MSL );
               break;
            case 1:
               mudstrlcpy( mobname, "Close", MSL );
               break;
            case 2:
               mudstrlcpy( mobname, "Close and lock", MSL );
               break;
         }
         snprintf( buf, MSL, "%2d) %s [%d] the %s [%d] door %s (%d)\n\r",
                   num, mobname, pReset->arg3, objname, pReset->arg2, roomname, pReset->arg1 );
         break;

      case 'R':
         if( !( room = get_room_index( pReset->arg1 ) ) )
            mudstrlcpy( roomname, "Room: *BAD VNUM*", MSL );
         else
            mudstrlcpy( roomname, room->name, MSL );
         snprintf( buf, MSL, "%2d) Randomize exits 0 to %d -> %s (%d)\n\r", num, pReset->arg2, roomname, pReset->arg1 );
         break;

      case 'T':
         if( !( room = get_room_index( pReset->arg3 ) ) )
            mudstrlcpy( roomname, "Room: *BAD VNUM*", MSL );
         else
            mudstrlcpy( roomname, room->name, MSL );
         snprintf( buf, MSL, "%2d) Trap: %d %d %d %d (%s) -> %s (%d)\n\r",
                   num, pReset->extra, pReset->arg1, pReset->arg2, pReset->arg3, flag_string( pReset->extra, trap_flags ),
                   roomname, room ? room->vnum : 0 );
         break;
   }
   return buf;
}

/*
 * Create a new reset (for online building)			-Thoric
 */
RESET_DATA *make_reset( char letter, int extra, int arg1, int arg2, int arg3, short arg4, short arg5, short arg6 )
{
   RESET_DATA *pReset;

   CREATE( pReset, RESET_DATA, 1 );
   pReset->command = letter;
   pReset->extra = extra;
   pReset->arg1 = arg1;
   pReset->arg2 = arg2;
   pReset->arg3 = arg3;
   pReset->arg4 = arg4;
   pReset->arg5 = arg5;
   pReset->arg6 = arg6;
   top_reset++;
   return pReset;
}

void add_obj_reset( ROOM_INDEX_DATA * room, char cm, OBJ_DATA * obj, int v2, int v3 )
{
   OBJ_DATA *inobj;
   static int iNest;

   if( ( cm == 'O' || cm == 'P' ) && obj->pIndexData->vnum == OBJ_VNUM_TRAP )
   {
      if( cm == 'O' )
         add_reset( room, 'T', obj->value[3], obj->value[1], obj->value[0], v3, -1, -1, -1 );
      return;
   }
   add_reset( room, cm, ( cm == 'P' ? iNest : 0 ), obj->pIndexData->vnum, v2, v3, obj->map, obj->x, obj->y );
   if( cm == 'O' && IS_OBJ_FLAG( obj, ITEM_HIDDEN ) && !IS_WEAR_FLAG( obj, ITEM_TAKE ) )
      add_reset( room, 'H', 1, 0, 0, 0, -1, -1, -1 );
   for( inobj = obj->first_content; inobj; inobj = inobj->next_content )
   {
      if( inobj->pIndexData->vnum == OBJ_VNUM_TRAP )
         add_obj_reset( room, 'O', inobj, 0, 0 );
   }
   if( cm == 'P' )
      iNest++;
   for( inobj = obj->first_content; inobj; inobj = inobj->next_content )
      add_obj_reset( room, 'P', inobj, inobj->count, obj->pIndexData->vnum );
   if( cm == 'P' )
      iNest--;
   return;
}

void delete_reset( RESET_DATA * pReset )
{
   RESET_DATA *tReset, *tReset_next;

   for( tReset = pReset->first_reset; tReset; tReset = tReset_next )
   {
      tReset_next = tReset->next_reset;

      UNLINK( tReset, pReset->first_reset, pReset->last_reset, next_reset, prev_reset );
      delete_reset( tReset );
   }
   pReset->first_reset = pReset->last_reset = NULL;
   DISPOSE( pReset );
   return;
}

void instaroom( CHAR_DATA * ch, ROOM_INDEX_DATA * pRoom, bool dodoors )
{
   CHAR_DATA *rch;
   OBJ_DATA *obj;
   bool added;

   for( rch = pRoom->first_person; rch; rch = rch->next_in_room )
   {
      if( !IS_NPC( rch ) )
         continue;

      added = false;
      if( IS_ROOM_FLAG( pRoom, ROOM_MAP ) && is_same_map( ch, rch ) )
      {
         add_reset( pRoom, 'M', 1, rch->pIndexData->vnum, rch->pIndexData->count, pRoom->vnum, ch->map, ch->x, ch->y );
         added = true;
      }
      else if( !IS_ROOM_FLAG( pRoom, ROOM_MAP ) )
      {
         add_reset( pRoom, 'M', 1, rch->pIndexData->vnum, rch->pIndexData->count, pRoom->vnum, -1, -1, -1 );
         added = true;
      }
      if( added )
      {
         for( obj = rch->first_carrying; obj; obj = obj->next_content )
         {
            if( obj->wear_loc == WEAR_NONE )
               add_obj_reset( pRoom, 'G', obj, 1, 0 );
            else
               add_obj_reset( pRoom, 'E', obj, 1, obj->wear_loc );
         }
      }
   }
   for( obj = pRoom->first_content; obj; obj = obj->next_content )
   {
      if( IS_ROOM_FLAG( pRoom, ROOM_MAP ) && ch->map == obj->map && ch->x == obj->x && ch->y == obj->y )
         add_obj_reset( pRoom, 'O', obj, obj->count, pRoom->vnum );
      else if( !IS_ROOM_FLAG( pRoom, ROOM_MAP ) )
         add_obj_reset( pRoom, 'O', obj, obj->count, pRoom->vnum );
   }
   if( dodoors )
   {
      EXIT_DATA *pexit;

      for( pexit = pRoom->first_exit; pexit; pexit = pexit->next )
      {
         int state = 0;

         if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
            continue;

         if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
         {
            if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
               state = 2;
            else
               state = 1;
         }
         add_reset( pRoom, 'D', 0, pRoom->vnum, pexit->vdir, state, -1, -1, -1 );
      }
   }
   return;
}

void wipe_resets( ROOM_INDEX_DATA * room )
{
   RESET_DATA *pReset, *pReset_next;

   for( pReset = room->first_reset; pReset; pReset = pReset_next )
   {
      pReset_next = pReset->next;

      UNLINK( pReset, room->first_reset, room->last_reset, next, prev );
      delete_reset( pReset );
   }
   room->first_reset = room->last_reset = NULL;
   return;
}

void wipe_area_resets( AREA_DATA * area )
{
   ROOM_INDEX_DATA *room;

   if( !mud_down )
   {
      for( room = area->first_room; room; room = room->next_aroom )
         wipe_resets( room );
   }
   return;
}

/* Function modified from original form - Samson */
CMDF do_instaroom( CHAR_DATA * ch, char *argument )
{
   bool dodoors;

#ifdef MULTIPORT
   if( port == MAINPORT )
   {
      send_to_char( "Instaroom is disabled on this port.\n\r", ch );
      return;
   }
#endif

   if( IS_NPC( ch ) || get_trust( ch ) < LEVEL_SAVIOR || !ch->pcdata->area )
   {
      send_to_char( "You don't have an assigned area to create resets for.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "nodoors" ) )
      dodoors = false;
   else
      dodoors = true;

   if( !can_rmodify( ch, ch->in_room ) )
      return;
   if( ch->in_room->area != ch->pcdata->area && get_trust( ch ) < LEVEL_GREATER )
   {
      send_to_char( "You cannot reset this room.\n\r", ch );
      return;
   }
   if( ch->in_room->first_reset )
      wipe_resets( ch->in_room );
   instaroom( ch, ch->in_room, dodoors );
   send_to_char( "Room resets installed.\n\r", ch );
}

/* Function modified from original form - Samson */
CMDF do_instazone( CHAR_DATA * ch, char *argument )
{
   AREA_DATA *pArea;
   ROOM_INDEX_DATA *pRoom;
   bool dodoors;

#ifdef MULTIPORT
   if( port == MAINPORT )
   {
      send_to_char( "Instazone is disabled on this port.\n\r", ch );
      return;
   }
#endif

   if( IS_NPC( ch ) || get_trust( ch ) < LEVEL_SAVIOR || !ch->pcdata->area )
   {
      send_to_char( "You don't have an assigned area to create resets for.\n\r", ch );
      return;
   }
   if( !str_cmp( argument, "nodoors" ) )
      dodoors = false;
   else
      dodoors = true;
   pArea = ch->pcdata->area;
   wipe_area_resets( pArea );
   for( pRoom = pArea->first_room; pRoom; pRoom = pRoom->next_aroom )
      instaroom( ch, pRoom, dodoors );
   send_to_char( "Area resets installed.\n\r", ch );
   return;
}

int generate_itemlevel( AREA_DATA * pArea, OBJ_INDEX_DATA * pObjIndex )
{
   int olevel;
   int min = UMAX( pArea->low_soft_range, 1 );
   int max = UMIN( pArea->hi_soft_range, min + 15 );

   if( pObjIndex->level > 0 )
      olevel = UMIN( pObjIndex->level, MAX_LEVEL );
   else
      switch ( pObjIndex->item_type )
      {
         default:
            olevel = 0;
            break;
         case ITEM_PILL:
            olevel = number_range( min, max );
            break;
         case ITEM_POTION:
            olevel = number_range( min, max );
            break;
         case ITEM_SCROLL:
            olevel = pObjIndex->value[0];
            break;
         case ITEM_WAND:
            olevel = number_range( min + 4, max + 1 );
            break;
         case ITEM_STAFF:
            olevel = number_range( min + 9, max + 5 );
            break;
         case ITEM_ARMOR:
            olevel = number_range( min + 4, max + 1 );
            break;
         case ITEM_WEAPON:
            olevel = number_range( min + 4, max + 1 );
            break;
      }
   return olevel;
}

/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list( RESET_DATA * pReset, OBJ_INDEX_DATA * pObjIndex, OBJ_DATA * list )
{
   OBJ_DATA *obj;
   int nMatch = 0;

   for( obj = list; obj; obj = obj->next_content )
   {
      if( obj->pIndexData == pObjIndex )
      {
         if( pReset->command == 'M' || pReset->command == 'O' )
         {
            if( pReset->arg4 == obj->map && pReset->arg5 == obj->x && pReset->arg6 == obj->y )
            {
               if( obj->count > 1 )
                  nMatch += obj->count;
               else
                  ++nMatch;
            }
         }
         else
         {
            if( obj->count > 1 )
               nMatch += obj->count;
            else
               ++nMatch;
         }
      }
   }
   return nMatch;
}

/*
 * Reset one room.
 */
void reset_room( ROOM_INDEX_DATA * room )
{
   RESET_DATA *pReset, *tReset, *gReset;
   OBJ_DATA *nestmap[MAX_NEST];
   CHAR_DATA *mob;
   OBJ_DATA *obj, *lastobj, *to_obj;
   ROOM_INDEX_DATA *pRoomIndex = NULL;
   MOB_INDEX_DATA *pMobIndex = NULL;
   OBJ_INDEX_DATA *pObjIndex = NULL, *pObjToIndex;
   EXIT_DATA *pexit;
   char mob_keywords[MSL];
   char *filename = room->area->filename;
   int level = 0, n, num, lastnest;

   mob = NULL;
   obj = NULL;
   lastobj = NULL;
   if( !room->first_reset )
      return;
   level = 0;
   for( pReset = room->first_reset; pReset; pReset = pReset->next )
   {
      switch ( pReset->command )
      {
         default:
            log_printf( "%s: %s: bad command %c.", __FUNCTION__, filename, pReset->command );
            break;

         case 'M':
            if( !( pMobIndex = get_mob_index( pReset->arg1 ) ) )
            {
               log_printf( "%s: %s: 'M': bad mob vnum %d.", __FUNCTION__, filename, pReset->arg1 );
               continue;
            }
            if( !( pRoomIndex = get_room_index( pReset->arg3 ) ) )
            {
               log_printf( "%s: %s: 'M': bad room vnum %d.", __FUNCTION__, filename, pReset->arg3 );
               continue;
            }
            if( pMobIndex->count >= pReset->arg2 )
            {
               mob = NULL;
               break;
            }
            mob = create_mobile( pMobIndex );
            if( pReset->arg4 != -1 && pReset->arg5 != -1 && pReset->arg6 != -1 )
            {
               SET_ACT_FLAG( mob, ACT_ONMAP );
               mob->map = pReset->arg4;
               mob->x = pReset->arg5;
               mob->y = pReset->arg6;
            }

            {
               ROOM_INDEX_DATA *pRoomPrev = get_room_index( pReset->arg3 - 1 );

               if( pRoomPrev && IS_ROOM_FLAG( pRoomPrev, ROOM_PET_SHOP ) )
                  SET_ACT_FLAG( mob, ACT_PET );
            }
            if( room_is_dark( pRoomIndex, mob ) )
               SET_AFFECTED( mob, AFF_INFRARED );
            if( !char_to_room( mob, pRoomIndex ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            level = URANGE( 0, mob->level - 2, LEVEL_AVATAR );

            /*
             * Added by Tarl 4 Dec 02 so that if a mob is 'flagged' namegen in
             * his name, it will auto assign a random name to it. Similarly,
             * occurances of namegen in the long_descr and description will be
             * replaced with the name.
             *
             * Modified by Tarl 5 Dec 02 to add extra namegen options. ie, namegen_gr will pick a name
             * suitable for Graecian mobs.
             * 
             * To add more, edit the line below this, and add an if-check similar to the one starting
             * with if( strstr( mob->name, "namegen_gr" ) )
             *
             * And then Samson shows up and cleans up the code, kills off the memory leaks, and life was good.
             * Or something like that anyway. Merry Christmas 2005!
             */
            if( strstr( mob->name, "namegen" ) )
            {
               char nameg[MSL];
               char *genstring = "namegen";
               bool namePicked = false;
               char tempStr[MSL], tempStr2[MSL];
               char file[256];

               if( strstr( mob->name, "namegen_gr" ) )
               {
                  genstring = "namegen_gr";
                  namePicked = true;
                  if( mob->sex == SEX_FEMALE )
                     snprintf( file, 256, "%s%s", SYSTEM_DIR, "namegen_gr_female.txt" );
                  else
                     snprintf( file, 256, "%s%s", SYSTEM_DIR, "namegen_gr_other.txt" );
               }
               else if( strstr( mob->name, "namegen_ven" ) )
               {
                  genstring = "namegen_ven";
                  namePicked = true;
                  if( mob->sex == SEX_FEMALE )
                     snprintf( file, 256, "%s%s", SYSTEM_DIR, "namegen_ven_female.txt" );
                  else
                     snprintf( file, 256, "%s%s", SYSTEM_DIR, "namegen_ven_other.txt" );
               }
               else if( strstr( mob->name, "namegen_orc" ) )
               {
                  genstring = "namegen_orc";
                  namePicked = true;
                  if( mob->sex == SEX_FEMALE )
                     snprintf( file, 256, "%s%s", SYSTEM_DIR, "namegen_orc_female.txt" );
                  else
                     snprintf( file, 256, "%s%s", SYSTEM_DIR, "namegen_orc_other.txt" );
               }

               nameg[0] = '\0';
               if( !namePicked )
                  name_generator( nameg );
               else
                  pick_name( nameg, file );

               mudstrlcpy( mob_keywords, mob->name, MSL );
               mudstrlcat( mob_keywords, " ", MSL );
               mudstrlcat( mob_keywords, nameg, MSL );
               STRFREE( mob->name );
               mob->name = STRALLOC( mob_keywords );
               STRFREE( mob->short_descr );
               mob->short_descr = STRALLOC( nameg );

               mudstrlcpy( tempStr, strrep( mob->long_descr, genstring, nameg ), MSL );
               STRFREE( mob->long_descr );
               mob->long_descr = STRALLOC( tempStr );

               if( mob->chardesc )
               {
                  mudstrlcpy( tempStr2, strrep( mob->chardesc, genstring, nameg ), MSL );
                  STRFREE( mob->chardesc );
                  mob->chardesc = STRALLOC( tempStr2 );
               }
            }
            if( pReset->first_reset )
            {
               for( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset )
               {
                  switch ( tReset->command )
                  {
                     case 'G':
                     case 'E':
                        if( !( pObjIndex = get_obj_index( tReset->arg1 ) ) )
                        {
                           log_printf( "%s: %s: 'E' or 'G': bad obj vnum %d.", __FUNCTION__, filename, tReset->arg1 );
                           continue;
                        }
                        if( !mob )
                        {
                           lastobj = NULL;
                           break;
                        }

                        if( pObjIndex->count >= pObjIndex->limit )
                        {
                           obj = NULL;
                           lastobj = NULL;
                           break;
                        }

                        if( mob->pIndexData->pShop )
                        {
                           int olevel = generate_itemlevel( room->area, pObjIndex );
                           obj = create_object( pObjIndex, olevel );
                           SET_OBJ_FLAG( obj, ITEM_INVENTORY );
                        }
                        else
                           obj = create_object( pObjIndex, number_fuzzy( level ) );
                        obj->level = URANGE( 0, obj->level, LEVEL_AVATAR );
                        obj = obj_to_char( obj, mob );
                        if( tReset->command == 'E' )
                        {
                           if( obj->carried_by != mob )
                           {
                              log_printf( "'E' reset: can't give object %d to mob %d.", obj->pIndexData->vnum,
                                   mob->pIndexData->vnum );
                              break;
                           }
                           equip_char( mob, obj, tReset->arg3 );
                        }
                        for( n = 0; n < MAX_NEST; n++ )
                           nestmap[n] = NULL;
                        nestmap[0] = obj;
                        lastobj = nestmap[0];
                        lastnest = 0;

                        if( tReset->first_reset )
                        {
                           for( gReset = tReset->first_reset; gReset; gReset = gReset->next_reset )
                           {
                              int iNest;
                              to_obj = lastobj;

                              switch ( gReset->command )
                              {
                                 case 'H':
                                    if( !lastobj )
                                       break;
                                    SET_OBJ_FLAG( lastobj, ITEM_HIDDEN );
                                    break;

                                 case 'P':
                                    if( !( pObjIndex = get_obj_index( gReset->arg1 ) ) )
                                    {
                                       log_printf( "%s: %s: 'P': bad obj vnum %d.", __FUNCTION__, filename, gReset->arg1 );
                                       continue;
                                    }
                                    iNest = gReset->extra;

                                    if( !( pObjToIndex = get_obj_index( gReset->arg3 ) ) )
                                    {
                                       log_printf( "%s: %s: 'P': bad objto vnum %d.", __FUNCTION__, filename, gReset->arg3 );
                                       continue;
                                    }
                                    if( iNest >= MAX_NEST )
                                    {
                                       log_printf( "%s: %s: 'P': Exceeded nesting limit of %d", __FUNCTION__, filename, MAX_NEST );
                                       obj = NULL;
                                       break;
                                    }
                                    if( pObjIndex->count >= pObjIndex->limit
                                        || count_obj_list( gReset, pObjIndex, to_obj->first_content ) > 0 )
                                    {
                                       obj = NULL;
                                       break;
                                    }

                                    if( iNest < lastnest )
                                       to_obj = nestmap[iNest];
                                    else if( iNest == lastnest )
                                       to_obj = nestmap[lastnest];
                                    else
                                       to_obj = lastobj;

                                    if( pObjIndex->count + gReset->arg2 > pObjIndex->limit )
                                    {
                                       num = pObjIndex->limit - gReset->arg2;
                                       if( num < 1 )
                                       {
                                          obj = NULL;
                                          break;
                                       }
                                    }
                                    else
                                       num = gReset->arg2;

                                    obj =
                                       create_object( pObjIndex,
                                                      number_fuzzy( UMAX
                                                                    ( generate_itemlevel( room->area, pObjIndex ),
                                                                      to_obj->level ) ) );
                                    if( num > 1 )
                                       pObjIndex->count += ( num - 1 );
                                    obj->count = num;
                                    obj->level = UMIN( obj->level, LEVEL_AVATAR );
                                    obj->count = gReset->arg2;
                                    obj_to_obj( obj, to_obj );
                                    if( iNest > lastnest )
                                    {
                                       nestmap[iNest] = to_obj;
                                       lastnest = iNest;
                                    }
                                    lastobj = obj;
                                    // Hackish fix for nested puts
                                    if( gReset->arg3 == OBJ_VNUM_DUMMYOBJ )
                                       gReset->arg3 = to_obj->pIndexData->vnum;
                                    break;
                              }
                           }
                        }
                        break;
                  }
               }
            }
            break;

         case 'O':
            if( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
            {
               log_printf( "%s: %s: 'O': bad obj vnum %d.", __FUNCTION__, filename, pReset->arg1 );
               continue;
            }
            if( !( pRoomIndex = get_room_index( pReset->arg3 ) ) )
            {
               log_printf( "%s: %s: 'O': bad room vnum %d.", __FUNCTION__, filename, pReset->arg3 );
               continue;
            }
            /*
             * Rent item limits here 
             */
            if( pObjIndex->count >= pObjIndex->limit )
            {
               obj = NULL;
               lastobj = NULL;
               break;
            }
            if( pObjIndex->count + pReset->arg2 > pObjIndex->limit )
            {
               num = pObjIndex->limit - pReset->arg2;
               if( num < 1 )
               {
                  obj = NULL;
                  lastobj = NULL;
                  break;
               }
            }
            else
               num = pReset->arg2;

            if( count_obj_list( pReset, pObjIndex, pRoomIndex->first_content ) < 1 )
            {
               obj = create_object( pObjIndex, number_fuzzy( generate_itemlevel( room->area, pObjIndex ) ) );
               if( num > 1 )
                  pObjIndex->count += ( num - 1 );
               obj->count = num;
               obj->level = UMIN( obj->level, LEVEL_AVATAR );
               obj->cost = 0;
               if( pReset->arg4 != -1 && pReset->arg5 != -1 && pReset->arg6 != -1 )
               {
                  SET_OBJ_FLAG( obj, ITEM_ONMAP );
                  obj->map = pReset->arg4;
                  obj->x = pReset->arg5;
                  obj->y = pReset->arg6;
               }
               obj_to_room( obj, pRoomIndex, NULL );
            }
            else
            {
               if( !( obj = get_obj_room( pObjIndex, pRoomIndex ) ) )
               {
                  obj = NULL;
                  lastobj = NULL;
                  break;
               }
               obj->extra_flags = pObjIndex->extra_flags;
               if( pReset->arg4 != -1 && pReset->arg5 != -1 && pReset->arg6 != -1 )
               {
                  SET_OBJ_FLAG( obj, ITEM_ONMAP );
                  obj->map = pReset->arg4;
                  obj->x = pReset->arg5;
                  obj->y = pReset->arg6;
               }
               for( int x = 0; x < 11; ++x )
                  obj->value[x] = pObjIndex->value[x];
            }
            for( n = 0; n < MAX_NEST; n++ )
               nestmap[n] = NULL;
            nestmap[0] = obj;
            lastobj = nestmap[0];
            lastnest = 0;
            if( pReset->first_reset )
            {
               for( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset )
               {
                  int iNest;

                  to_obj = lastobj;

                  switch ( tReset->command )
                  {
                     case 'H':
                        if( !lastobj )
                           break;
                        SET_OBJ_FLAG( lastobj, ITEM_HIDDEN );
                        break;

                     case 'T':
                        if( !IS_SET( tReset->extra, TRAP_OBJ ) )
                        {
                           log_printf( "%s: Room reset found on object reset list", __FUNCTION__ );
                           break;
                        }
                        else
                        {
                           /*
                            * We need to preserve obj for future 'T' checks 
                            */
                           obj_data *pobj;

                           if( tReset->arg3 > 0 )
                           {
                              if( !( pObjToIndex = get_obj_index( tReset->arg3 ) ) )
                              {
                                 log_printf( "%s: %s: 'T': bad objto vnum %d.", __FUNCTION__, filename, tReset->arg3 );
                                 continue;
                              }
                              if( room->area->nplayer > 0 || !( to_obj = get_obj_type( pObjToIndex ) ) ||
                                  ( to_obj->carried_by && !IS_NPC( to_obj->carried_by ) ) || is_trapped( to_obj ) )
                                 break;
                           }
                           else
                           {
                              if( !lastobj || !obj )
                                 break;
                              to_obj = obj;
                           }
                           pobj = make_trap( tReset->arg2, tReset->arg1, number_fuzzy( to_obj->level ), tReset->extra );
                           obj_to_obj( pobj, to_obj );
                        }
                        break;

                     case 'P':
                        if( !( pObjIndex = get_obj_index( tReset->arg1 ) ) )
                        {
                           log_printf( "%s: %s: 'P': bad obj vnum %d.", __FUNCTION__, filename, tReset->arg1 );
                           continue;
                        }
                        iNest = tReset->extra;

                        if( !( pObjToIndex = get_obj_index( tReset->arg3 ) ) )
                        {
                           log_printf( "%s: %s: 'P': bad objto vnum %d.", __FUNCTION__, filename, tReset->arg3 );
                           continue;
                        }

                        if( iNest >= MAX_NEST )
                        {
                           log_printf( "%s: %s: 'P': Exceeded nesting limit of %d. Room %d.", __FUNCTION__, filename, MAX_NEST,
                                room->vnum );
                           obj = NULL;
                           break;
                        }

                        if( pObjIndex->count >= pObjIndex->limit
                            || count_obj_list( tReset, pObjIndex, to_obj->first_content ) > 0 )
                        {
                           obj = NULL;
                           break;
                        }
                        if( iNest < lastnest )
                           to_obj = nestmap[iNest];
                        else if( iNest == lastnest )
                           to_obj = nestmap[lastnest];
                        else
                           to_obj = lastobj;

                        if( pObjIndex->count + tReset->arg2 > pObjIndex->limit )
                        {
                           num = pObjIndex->limit - tReset->arg2;
                           if( num < 1 )
                           {
                              obj = NULL;
                              break;
                           }
                        }
                        else
                           num = tReset->arg2;

                        obj =
                           create_object( pObjIndex,
                                          number_fuzzy( UMAX
                                                        ( generate_itemlevel( room->area, pObjIndex ), to_obj->level ) ) );
                        if( num > 1 )
                           pObjIndex->count += ( num - 1 );
                        obj->count = num;
                        obj->level = UMIN( obj->level, LEVEL_AVATAR );
                        obj->count = tReset->arg2;
                        obj_to_obj( obj, to_obj );
                        if( iNest > lastnest )
                        {
                           nestmap[iNest] = to_obj;
                           lastnest = iNest;
                        }
                        lastobj = obj;
                        // Hackish fix for nested puts
                        if( tReset->arg3 == OBJ_VNUM_DUMMYOBJ )
                           tReset->arg3 = to_obj->pIndexData->vnum;
                        break;
                  }
               }
            }
            break;

         case 'T':
            if( IS_SET( pReset->extra, TRAP_OBJ ) )
            {
               log_printf( "%s: Object trap found in room %d reset list", __FUNCTION__, room->vnum );
               break;
            }
            else
            {
               if( !( pRoomIndex = get_room_index( pReset->arg3 ) ) )
               {
                  log_printf( "%s: %s: 'T': bad room %d.", __FUNCTION__, filename, pReset->arg3 );
                  continue;
               }
               if( room->area->nplayer > 0
                   || count_obj_list( pReset, get_obj_index( OBJ_VNUM_TRAP ), pRoomIndex->first_content ) > 0 )
                  break;
               to_obj = make_trap( pReset->arg1, pReset->arg1, 10, pReset->extra );
               obj_to_room( to_obj, pRoomIndex, NULL );
            }
            break;

         case 'D':
            if( !( pRoomIndex = get_room_index( pReset->arg1 ) ) )
            {
               log_printf( "%s: %s: 'D': bad room vnum %d.", __FUNCTION__, filename, pReset->arg1 );
               continue;
            }
            if( !( pexit = get_exit( pRoomIndex, pReset->arg2 ) ) )
               break;
            switch ( pReset->arg3 )
            {
               case 0:
                  REMOVE_EXIT_FLAG( pexit, EX_CLOSED );
                  REMOVE_EXIT_FLAG( pexit, EX_LOCKED );
                  break;
               case 1:
                  SET_EXIT_FLAG( pexit, EX_CLOSED );
                  REMOVE_EXIT_FLAG( pexit, EX_LOCKED );
                  if( IS_EXIT_FLAG( pexit, EX_xSEARCHABLE ) )
                     SET_EXIT_FLAG( pexit, EX_SECRET );
                  break;
               case 2:
                  SET_EXIT_FLAG( pexit, EX_CLOSED );
                  SET_EXIT_FLAG( pexit, EX_LOCKED );
                  if( IS_EXIT_FLAG( pexit, EX_xSEARCHABLE ) )
                     SET_EXIT_FLAG( pexit, EX_SECRET );
                  break;
            }
            break;

         case 'R':
            if( !( pRoomIndex = get_room_index( pReset->arg1 ) ) )
            {
               log_printf( "%s: %s: 'R': bad room vnum %d.", __FUNCTION__, filename, pReset->arg1 );
               continue;
            }
            randomize_exits( pRoomIndex, pReset->arg2 - 1 );
            break;
      }
   }
   return;
}

void reset_area( AREA_DATA * area )
{
   ROOM_INDEX_DATA *room;

   if( !area->first_room )
      return;

   for( room = area->first_room; room; room = room->next_aroom )
      reset_room( room );
}

/* Setup put nesting levels, regardless of whether or not the resets will
   actually reset, or if they're bugged. */
void renumber_put_resets( ROOM_INDEX_DATA * room )
{
   RESET_DATA *pReset, *tReset, *lastobj = NULL;

   for( pReset = room->first_reset; pReset; pReset = pReset->next )
   {
      switch ( pReset->command )
      {
         default:
            break;

         case 'O':
            lastobj = pReset;
            for( tReset = pReset->first_reset; tReset; tReset = tReset->next_reset )
            {
               switch ( tReset->command )
               {
                  case 'P':
                     if( tReset->arg3 == 0 )
                     {
                        if( !lastobj )
                           tReset->extra = 1000000;
                        else if( lastobj->command != 'P' || lastobj->arg3 > 0 )
                           tReset->extra = 0;
                        else
                           tReset->extra = lastobj->extra + 1;
                        lastobj = tReset;
                     }
                     break;
               }
            }
            break;
      }
   }
   return;
}

/*
 * Add a reset to an area -Thoric
 */
RESET_DATA *add_reset( ROOM_INDEX_DATA * room, char letter, int extra, int arg1, int arg2, int arg3, short arg4,
                       short arg5, short arg6 )
{
   RESET_DATA *pReset;

   if( !room )
   {
      bug( "%s: NULL room!", __FUNCTION__ );
      return NULL;
   }

   letter = UPPER( letter );
   pReset = make_reset( letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
   switch ( letter )
   {
      case 'M':
         room->last_mob_reset = pReset;
         break;

      case 'E':
      case 'G':
         if( !room->last_mob_reset )
         {
            log_printf( "%s: Can't add '%c' reset to room: last_mob_reset is NULL.", __FUNCTION__, letter );
            return NULL;
         }
         room->last_obj_reset = pReset;
         LINK( pReset, room->last_mob_reset->first_reset, room->last_mob_reset->last_reset, next_reset, prev_reset );
         return pReset;

      case 'P':
         if( !room->last_obj_reset )
         {
            log_printf( "%s: Can't add '%c' reset to room: last_obj_reset is NULL.", __FUNCTION__, letter );
            return NULL;
         }
         LINK( pReset, room->last_obj_reset->first_reset, room->last_obj_reset->last_reset, next_reset, prev_reset );
         return pReset;

      case 'O':
         room->last_obj_reset = pReset;
         break;

      case 'T':
         if( IS_SET( extra, TRAP_OBJ ) )
         {
            pReset->prev_reset = NULL;
            pReset->next_reset = room->last_obj_reset->first_reset;
            if( room->last_obj_reset->first_reset )
               room->last_obj_reset->first_reset->prev_reset = pReset;
            room->last_obj_reset->first_reset = pReset;
            if( !room->last_obj_reset->last_reset )
               room->last_obj_reset->last_reset = pReset;
            return pReset;
         }
         break;

      case 'H':
         pReset->prev_reset = NULL;
         pReset->next_reset = room->last_obj_reset->first_reset;
         if( room->last_obj_reset->first_reset )
            room->last_obj_reset->first_reset->prev_reset = pReset;
         room->last_obj_reset->first_reset = pReset;
         if( !room->last_obj_reset->last_reset )
            room->last_obj_reset->last_reset = pReset;
         return pReset;
   }
   LINK( pReset, room->first_reset, room->last_reset, next, prev );
   return pReset;
}

RESET_DATA *find_oreset( ROOM_INDEX_DATA * room, char *oname )
{
   RESET_DATA *pReset;
   OBJ_INDEX_DATA *pobj;
   char arg[MIL];
   int cnt = 0, num = number_argument( oname, arg );

   for( pReset = room->first_reset; pReset; pReset = pReset->next )
   {
      // Only going to allow traps/hides on room reset objects. Unless someone can come up with a better way to do this.
      if( pReset->command != 'O' )
         continue;

      if( !( pobj = get_obj_index( pReset->arg1 ) ) )
         continue;

      if( is_name( arg, pobj->name ) && ++cnt == num )
         return pReset;
   }
   return NULL;
}

CMDF do_reset( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: reset area\n\r", ch );
      send_to_char( "Usage: reset randomize <direction>\n\r", ch );
      send_to_char( "Usage: reset delete <number>\n\r", ch );
      send_to_char( "Usage: reset hide <objname>\n\r", ch );
      send_to_char( "Usage: reset trap room <type> <charges> [flags]\n\r", ch );
      send_to_char( "Usage: reset trap obj <name> <type> <charges> [flags]\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "area" ) )
   {
      reset_area( ch->in_room->area );
      send_to_char( "Area has been reset.\n\r", ch );
      return;
   }

   // Yeah, I know, this function is mucho ugly... but...
   if( !str_cmp( arg, "delete" ) )
   {
      RESET_DATA *pReset, *tReset, *pReset_next, *tReset_next, *gReset, *gReset_next;
      int num, nfind = 0;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You must specify a reset # in this room to delete one.\n\r", ch );
         return;
      }

      if( !is_number( argument ) )
      {
         send_to_char( "Specified reset must be designated by number. See &Wredit rlist&D.\n\r", ch );
         return;
      }
      num = atoi( argument );

      for( pReset = ch->in_room->first_reset; pReset; pReset = pReset_next )
      {
         pReset_next = pReset->next;

         nfind++;
         if( nfind == num )
         {
            UNLINK( pReset, ch->in_room->first_reset, ch->in_room->last_reset, next, prev );
            delete_reset( pReset );
            send_to_char( "Reset deleted.\n\r", ch );
            return;
         }

         for( tReset = pReset->first_reset; tReset; tReset = tReset_next )
         {
            tReset_next = tReset->next_reset;

            nfind++;
            if( nfind == num )
            {
               UNLINK( tReset, pReset->first_reset, pReset->last_reset, next_reset, prev_reset );
               delete_reset( tReset );
               send_to_char( "Reset deleted.\n\r", ch );
               return;
            }

            for( gReset = tReset->first_reset; gReset; gReset = gReset_next )
            {
               gReset_next = gReset->next_reset;

               nfind++;
               if( nfind == num )
               {
                  UNLINK( gReset, tReset->first_reset, tReset->last_reset, next_reset, prev_reset );
                  delete_reset( gReset );
                  send_to_char( "Reset deleted.\n\r", ch );
                  return;
               }
            }
         }
      }
      send_to_char( "No reset matching that number was found in this room.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "random" ) )
   {
      argument = one_argument( argument, arg );
      int vnum = get_dir( arg );

      if( vnum < 0 || vnum > 9 )
      {
         send_to_char( "Reset which random doors?\n\r", ch );
         return;
      }

      if( vnum == 0 )
      {
         send_to_char( "There is no point in randomizing one door.\n\r", ch );
         return;
      }

      if( !get_room_index( vnum ) )
      {
         send_to_char( "Target room does not exist.\n\r", ch );
         return;
      }

      RESET_DATA *pReset = make_reset( 'R', 0, ch->in_room->vnum, vnum, 0, -1, -1, -1 );
      pReset->prev = NULL;
      pReset->next = ch->in_room->first_reset;
      if( ch->in_room->first_reset )
         ch->in_room->first_reset->prev = pReset;
      ch->in_room->first_reset = pReset;
      if( !ch->in_room->last_reset )
         ch->in_room->last_reset = pReset;
      send_to_char( "Reset random doors created.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "trap" ) )
   {
      RESET_DATA *pReset = NULL;
      char oname[MIL], arg2[MIL];
      int num, chrg, value, extra = 0, vnum;

      argument = one_argument( argument, arg2 );

      if( !str_cmp( arg2, "room" ) )
      {
         vnum = ch->in_room->vnum;
         extra = TRAP_ROOM;

         argument = one_argument( argument, arg );
         num = is_number( arg ) ? atoi( arg ) : -1;
         argument = one_argument( argument, arg );
         chrg = is_number( arg ) ? atoi( arg ) : -1;
      }
      else if( !str_cmp( arg2, "obj" ) )
      {
         argument = one_argument( argument, oname );
         if( !( pReset = find_oreset( ch->in_room, oname ) ) )
         {
            send_to_char( "No matching reset found to set a trap on.\n\r", ch );
            return;
         }
         vnum = 0;
         extra = TRAP_OBJ;

         argument = one_argument( argument, arg );
         num = is_number( arg ) ? atoi( arg ) : -1;
         argument = one_argument( argument, arg );
         chrg = is_number( arg ) ? atoi( arg ) : -1;
      }
      else
      {
         send_to_char( "Trap reset must be on 'room' or 'obj'\n\r", ch );
         return;
      }

      if( num < 1 || num > MAX_TRAPTYPE )
      {
         send_to_char( "Invalid trap type.\n\r", ch );
         return;
      }

      if( chrg < 0 || chrg > 10000 )
      {
         send_to_char( "Invalid trap charges. Must be between 1 and 10000.\n\r", ch );
         return;
      }

      while( *argument )
      {
         argument = one_argument( argument, arg );
         value = get_trapflag( arg );
         if( value < 0 || value > 31 )
         {
            ch_printf( ch, "Bad trap flag: %s\n\r", arg );
            continue;
         }
         SET_BIT( extra, 1 << value );
      }
      RESET_DATA *tReset = make_reset( 'T', extra, num, chrg, vnum, -1, -1, -1 );
      if( pReset )
      {
         tReset->prev_reset = NULL;
         tReset->next_reset = pReset->first_reset;
         if( pReset->first_reset )
            pReset->first_reset->prev_reset = tReset;
         pReset->first_reset = tReset;
         if( !pReset->last_reset )
            pReset->last_reset = tReset;
      }
      else
      {
         tReset->prev = NULL;
         tReset->next = ch->in_room->first_reset;
         if( ch->in_room->first_reset )
            ch->in_room->first_reset->prev = tReset;
         ch->in_room->first_reset = tReset;
         if( !ch->in_room->last_reset )
            ch->in_room->last_reset = tReset;
      }
      send_to_char( "Trap created.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "hide" ) )
   {
      RESET_DATA *pReset = NULL;

      if( !( pReset = find_oreset( ch->in_room, argument ) ) )
      {
         send_to_char( "No such object to hide in this room.\n\r", ch );
         return;
      }
      RESET_DATA *tReset = make_reset( 'H', 1, 0, 0, 0, -1, -1, -1 );
      if( pReset )
      {
         tReset->prev_reset = NULL;
         tReset->next_reset = pReset->first_reset;
         if( pReset->first_reset )
            pReset->first_reset->prev_reset = tReset;
         pReset->first_reset = tReset;
         if( !pReset->last_reset )
            pReset->last_reset = tReset;
      }
      else
      {
         tReset->prev = NULL;
         tReset->next = ch->in_room->first_reset;
         if( ch->in_room->first_reset )
            ch->in_room->first_reset->prev = tReset;
         ch->in_room->first_reset = tReset;
         if( !ch->in_room->last_reset )
            ch->in_room->last_reset = tReset;
      }
      send_to_char( "Hide reset created.\n\r", ch );
      return;
   }
   do_reset( ch, "" );
   return;
}
