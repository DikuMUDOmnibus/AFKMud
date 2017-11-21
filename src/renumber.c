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
 *                          Area Renumbering Module                         *
 ****************************************************************************/

/*
 *  Renumber Imm command
 *  Author: Cronel (cronel_kal@hotmail.com)
 *  of FrozenMUD (empire.digiunix.net 4000)
 *
 *  Permission to use and distribute this code is granted provided
 *  this header is retained and unaltered, and the distribution
 *  package contains all the original files unmodified.
 *  If you modify this code and use/distribute modified versions
 *  you must give credit to the original author(s).
 */
#include <string.h>
#include <ctype.h>
#include "mud.h"
#include "mud_prog.h"
#include "overland.h"
#include "shops.h"

#define NOT_FOUND (-1)
enum
{ REN_ROOM, REN_OBJ, REN_MOB };
struct renumber_data
{
   int old_vnum;
   int new_vnum;

   struct renumber_data *next;
};

typedef struct renumber_data RENUMBER_DATA;

struct renumber_areas
{
   RENUMBER_DATA *r_obj;
   RENUMBER_DATA *r_mob;
   RENUMBER_DATA *r_room;
   int low_obj, hi_obj;
   int low_mob, hi_mob;
   int low_room, hi_room;
};

typedef struct renumber_areas RENUMBER_AREA;

void renumber_area( CHAR_DATA * ch, AREA_DATA * area, RENUMBER_AREA * r_area, bool verbose );

RENUMBER_AREA *gather_renumber_data( AREA_DATA * area, int new_base, bool fill_gaps );
RENUMBER_DATA *gather_one_list( short type, int low, int high, int new_base, bool fill_gaps, int *max_vnum );
void free_renumber_data( RENUMBER_DATA * r_data );

AREA_DATA *find_area( char *filename );
bool check_vnums( CHAR_DATA * ch, AREA_DATA * tarea, RENUMBER_AREA * r_area );

int find_translation( int vnum, RENUMBER_DATA * r_data );
void translate_reset( RESET_DATA * reset, RENUMBER_AREA * r_data );
void translate_objvals( CHAR_DATA * ch, AREA_DATA * area, RENUMBER_AREA * r_data, bool verbose );
void translate_exits( CHAR_DATA * ch, AREA_DATA * area, RENUMBER_AREA * r_area, bool verbose );
void warn_progs( CHAR_DATA * ch, int low, int high, AREA_DATA * area, RENUMBER_AREA * r_area );
void warn_in_prog( CHAR_DATA * ch, int low, int high, char *where, int vnum, MPROG_DATA * mprog, RENUMBER_AREA * r_area );

/* from db.c */
void save_sysdata( SYSTEM_DATA sys );

CMDF do_renumber( CHAR_DATA * ch, char *argument )
{
   RENUMBER_AREA *r_area;
   AREA_DATA *area;
   char arg1[MIL];
   int new_base;
   bool fill_gaps, verbose;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Yeah, right.\n\r", ch );
      return;
   }

   /*
    * parse the first two parameters 
    */
   /*
    * first, area 
    */
   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "What area do you want to renumber?\n\r", ch );
      return;
   }
   area = find_area( arg1 );
   if( area == NULL )
   {
      ch_printf( ch, "No such area '%s'.\n\r", arg1 );
      return;
   }

   /*
    * and new vnum base 
    */
   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "What will be the new vnum base for this area?\n\r", ch );
      return;
   }

   if( !is_number( arg1 ) )
   {
      ch_printf( ch, "Sorry, '%s' is not a valid vnum base number!\n\r", arg1 );
      return;
   }

   new_base = atoi( arg1 );

   /*
    * parse the flags 
    */
   fill_gaps = FALSE;
   verbose = FALSE;
   for( ;; )
   {
      argument = one_argument( argument, arg1 );
      if( !arg1 || arg1[0] == '\0' )
         break;
      else if( !str_prefix( arg1, "fillgaps" ) )
         fill_gaps = TRUE;
      else if( !str_prefix( arg1, "verbose" ) )
         verbose = TRUE;
      else
      {
         ch_printf( ch, "Invalid flag '%s'.\n\r", arg1 );
         return;
      }
   }

   /*
    * sanity check 
    */
   if( new_base == area->low_vnum && !fill_gaps )
   {
      send_to_char( "You don't want to change the base vnum and you don't want to fill gaps.\n\r", ch );
      send_to_char( "So what DO you want to do?\n\r", ch );
      return;
   }

   /*
    * some restrictions 
    */
   if( ch->level < LEVEL_SAVIOR )
   {
      send_to_char( "You don't have enough privileges.\n\r", ch );
      return;
   }

   if( ch->level == LEVEL_SAVIOR )
   {
      if( area->low_vnum < ch->pcdata->low_vnum || area->hi_vnum > ch->pcdata->hi_vnum )
      {
         ch_printf( ch, "You can't renumber that area ('%s').\n\r", area->filename );
         return;
      }
   }

   /*
    * get the renumber data 
    */
   r_area = gather_renumber_data( area, new_base, fill_gaps );

   /*
    * one more restriction 
    */
   if( ch->level == LEVEL_SAVIOR )
   {
      if( r_area->low_room < ch->pcdata->low_vnum || r_area->hi_room > ch->pcdata->hi_vnum ||
          r_area->low_obj < ch->pcdata->low_vnum || r_area->hi_obj > ch->pcdata->hi_vnum ||
          r_area->low_mob < ch->pcdata->low_vnum || r_area->hi_mob > ch->pcdata->hi_vnum )
      {
         send_to_char( "The renumbered area would be outside your assigned vnum range.\n\r", ch );
         return;
      }
   }

   /*
    * OOO! A new sanity check :) - Samson 10-7-02 
    */
   if( new_base >= sysdata.maxvnum )
   {
      ch_printf( ch, "%d is beyond the maximum allowed vnum of %d\n\r", new_base, sysdata.maxvnum );
      return;
   }

   /*
    * no overwriting of dest vnums 
    */
   if( check_vnums( ch, area, r_area ) )
      return;

   /*
    * another sanity check :) 
    */
   if( r_area == NULL || ( r_area->r_obj == NULL && r_area->r_mob == NULL && r_area->r_room == NULL ) )
   {
      send_to_char( "No changes to make.\n\r", ch );
      DISPOSE( r_area );
      return;
   }

   /*
    * ok, do it! 
    */
   pager_printf( ch, "Renumbering area '%s' to new base %d, filling gaps: %s\n\r",
                 area->filename, new_base, fill_gaps ? "yes" : "no" );
   renumber_area( ch, area, r_area, verbose );
   send_to_pager( "Done.\n\r", ch );

   if( area->hi_vnum > sysdata.maxvnum )
   {
      sysdata.maxvnum = area->hi_vnum + 1;
      save_sysdata( sysdata );
      pager_printf( ch, "Maximum vnum was raised to %d by this operation.\n\r", sysdata.maxvnum );
   }

   /*
    * clean up and goodbye 
    */
   if( r_area->r_room != NULL )
      free_renumber_data( r_area->r_room );
   if( r_area->r_obj != NULL )
      free_renumber_data( r_area->r_obj );
   if( r_area->r_mob != NULL )
      free_renumber_data( r_area->r_mob );
   DISPOSE( r_area );
   return;
}

bool check_vnums( CHAR_DATA * ch, AREA_DATA * tarea, RENUMBER_AREA * r_area )
{
   int high, low;
   AREA_DATA *area;

   /*
    * this function assumes all the lows are always gonna be lower or equal to all the highs .. 
    */
   high = UMAX( r_area->hi_room, UMAX( r_area->hi_obj, r_area->hi_mob ) );
   low = UMIN( r_area->low_room, UMIN( r_area->low_obj, r_area->low_mob ) );

   if( high > sysdata.maxvnum )
   {
      ch_printf( ch, "This operation would raise the maximum allowed vnum beyond %d.\n\r", sysdata.maxvnum );
      send_to_char( "Pick a lower base, or have sysdata.maxvnum raised.\n\r", ch );
      return TRUE;
   }

   /*
    * in do_check_vnums they use first_bsort, first_asort but.. i dunno.. 
    */
   for( area = first_area; area; area = area->next )
   {
      if( tarea == area )
         continue;

      if( !( high < area->low_vnum || low > area->hi_vnum ) )
      {
         ch_printf( ch, "This operation would overwrite area %s! Use checkvnums first.\n\r", area->filename );
         return TRUE;
      }
   }
   return FALSE;
}

RENUMBER_AREA *gather_renumber_data( AREA_DATA * area, int new_base, bool fill_gaps )
/* this function actualy gathers all the renumber data for an area */
{
   RENUMBER_AREA *r_area;
   int max;

   CREATE( r_area, RENUMBER_AREA, 1 );

   r_area->r_mob = gather_one_list( REN_MOB, area->low_vnum, area->hi_vnum, new_base, fill_gaps, &max );
   r_area->low_mob = new_base;
   r_area->hi_mob = max;

   r_area->r_obj = gather_one_list( REN_OBJ, area->low_vnum, area->hi_vnum, new_base, fill_gaps, &max );
   r_area->low_obj = new_base;
   r_area->hi_obj = max;

   r_area->r_room = gather_one_list( REN_ROOM, area->low_vnum, area->hi_vnum, new_base, fill_gaps, &max );
   r_area->low_room = new_base;
   r_area->hi_room = max;

   return r_area;
}

/* this function builds a list of renumber data for a type (obj, room, or mob) */
RENUMBER_DATA *gather_one_list( short type, int low, int high, int new_base, bool fill_gaps, int *max_vnum )
{
   int cur_vnum;
   RENUMBER_DATA *r_data, root;
   bool found;
   ROOM_INDEX_DATA *room;
   OBJ_INDEX_DATA *obj;
   MOB_INDEX_DATA *mob;
   int i;
   int highest;

   memset( &root, 0, sizeof( RENUMBER_DATA ) );
   r_data = &root;

   cur_vnum = new_base;
   highest = -1;
   for( i = low; i <= high; i++ )
   {
      found = FALSE;
      switch ( type )
      {
         case REN_ROOM:
            room = get_room_index( i );
            if( room != NULL )
               found = TRUE;
            break;
         case REN_OBJ:
            obj = get_obj_index( i );
            if( obj != NULL )
               found = TRUE;
            break;
         case REN_MOB:
            mob = get_mob_index( i );
            if( mob != NULL )
               found = TRUE;
            break;
      }

      if( found )
      {
         if( cur_vnum > highest )
            highest = cur_vnum;
         if( cur_vnum != i )
         {
            CREATE( r_data->next, RENUMBER_DATA, 1 );
            r_data = r_data->next;
            r_data->old_vnum = i;
            r_data->new_vnum = cur_vnum;
         }
         cur_vnum++;
      }
      else if( !fill_gaps )
         cur_vnum++;
   }
   *max_vnum = highest;
   return root.next;
}

/* disposes of a list of renumber data items */
void free_renumber_data( RENUMBER_DATA * r_data )
{
   RENUMBER_DATA *r_next;

   while( r_data != NULL )
   {
      r_next = r_data->next;
      DISPOSE( r_data );
      r_data = r_next;
   }
}

/* this is the function that actualy does the renumbering of "area" according
 * to the renumber data in "r_area". "ch" is to show messages.
 */
void renumber_area( CHAR_DATA * ch, AREA_DATA * area, RENUMBER_AREA * r_area, bool verbose )
{
   RENUMBER_DATA *r_data;
   ROOM_INDEX_DATA *room, *room_prev, *room_list, *room_next;
   MOB_INDEX_DATA *mob, *mob_prev, *mob_list, *mob_next;
   OBJ_INDEX_DATA *obj, *obj_prev, *obj_list, *obj_next;
   RESET_DATA *preset, *treset;
   int iHash;
   int low, high;

   high = area->hi_vnum;
   low = area->low_vnum;

   send_to_pager( "(Room) Renumbering...\n\r", ch );

   /*
    * what we do here is, for each list (room/obj/mob) first we
    * * take each element out of the hash array, change the vnum,
    * * and move it to our own list. after everything's moved out
    * * we put it in again. this is to avoid problems in situations
    * * where where room A is being moved to position B, but theres
    * * already a room B wich is also being moved to position C.
    * * a straightforward approach would result in us moving A to
    * * position B first, and then again to position C, and room
    * * B being lost inside the hash array, still there, but not
    * * foundable (its "covered" by A because they'd have the same vnum). 
    */

   room_list = NULL;
   for( r_data = r_area->r_room; r_data; r_data = r_data->next )
   {
      if( verbose )
         pager_printf( ch, "(Room) %d -> %d\n\r", r_data->old_vnum, r_data->new_vnum );

      room = get_room_index( r_data->old_vnum );
      if( !room )
      {
         bug( "renumber_area: NULL room %d", r_data->old_vnum );
         continue;
      }

      /*
       * remove it from the hash list 
       */
      iHash = r_data->old_vnum % MAX_KEY_HASH;
      if( room_index_hash[iHash] == room )
         room_index_hash[iHash] = room->next;
      else
      {
         for( room_prev = room_index_hash[iHash]; room_prev && room_prev->next != room; room_prev = room_prev->next )
            ;
         if( room_prev == NULL )
         {
            bug( "%s", "renumber_area: Couldn't find a room in the hash table! Skipping it." );
            continue;
         }
         room_prev->next = room->next;
         room->next = NULL;
      }

      /*
       * change the vnum 
       */
      room->vnum = r_data->new_vnum;

      /*
       * move it to the temporary list 
       */
      room->next = room_list;
      room_list = room;
   }
   /*
    * now move everything back into the hash array 
    */
   for( room = room_list; room; room = room_next )
   {
      room_next = room->next;
      /*
       * add it to the hash list again (new position) 
       */
      iHash = room->vnum % MAX_KEY_HASH;
      room->next = room_index_hash[iHash];
      room_index_hash[iHash] = room;
   }
   /*
    * if nothing was moved, dont change this 
    */
   if( r_area->r_room != NULL )
   {
      area->low_vnum = r_area->low_room;
      area->hi_vnum = r_area->hi_room;
   }

   send_to_pager( "(Mobs) Renumbering...\n\r", ch );
   mob_list = NULL;
   for( r_data = r_area->r_mob; r_data; r_data = r_data->next )
   {
      if( verbose )
         pager_printf( ch, "(Mobs) %d -> %d\n\r", r_data->old_vnum, r_data->new_vnum );

      mob = get_mob_index( r_data->old_vnum );
      if( !mob )
      {
         bug( "renumber_area: NULL mob %d", r_data->old_vnum );
         continue;
      }

      /*
       * fix references to this mob from shops while renumbering this mob 
       */
      if( mob->pShop )
      {
         if( verbose )
            pager_printf( ch, "(Mobs) Fixing shop for mob %d -> %d\n\r", r_data->old_vnum, r_data->new_vnum );
         mob->pShop->keeper = r_data->new_vnum;
      }
      if( mob->rShop )
      {
         if( verbose )
            pager_printf( ch, "(Mobs) Fixing repair shop for mob %d -> %d\n\r", r_data->old_vnum, r_data->new_vnum );
         mob->rShop->keeper = r_data->new_vnum;
      }

      /*
       * remove it from the hash list 
       */
      iHash = r_data->old_vnum % MAX_KEY_HASH;
      if( mob_index_hash[iHash] == mob )
         mob_index_hash[iHash] = mob->next;
      else
      {
         for( mob_prev = mob_index_hash[iHash]; mob_prev && mob_prev->next != mob; mob_prev = mob_prev->next )
            ;
         if( mob_prev == NULL )
         {
            bug( "%s", "renumber_area: Couldn't find a mob in the hash table! Skipping it." );
            continue;
         }
         mob_prev->next = mob->next;
         mob->next = NULL;
      }

      /*
       * change the vnum 
       */
      mob->vnum = r_data->new_vnum;

      /*
       * move to private list 
       */
      mob->next = mob_list;
      mob_list = mob;
   }
   for( mob = mob_list; mob; mob = mob_next )
   {
      mob_next = mob->next;
      /*
       * add it to the hash list again 
       */
      iHash = mob->vnum % MAX_KEY_HASH;
      mob->next = mob_index_hash[iHash];
      mob_index_hash[iHash] = mob;
   }
   if( r_area->r_mob )
   {
      if( r_area->low_mob < r_area->low_room )
         area->low_vnum = r_area->low_mob;
      if( r_area->hi_mob > r_area->hi_room )
         area->hi_vnum = r_area->hi_mob;
   }

   send_to_pager( "(Objs) Renumbering...\n\r", ch );
   obj_list = NULL;
   for( r_data = r_area->r_obj; r_data; r_data = r_data->next )
   {
      if( verbose )
         pager_printf( ch, "(Objs) %d -> %d\n\r", r_data->old_vnum, r_data->new_vnum );
      obj = get_obj_index( r_data->old_vnum );
      if( !obj )
      {
         bug( "renumber_area: NULL obj %d", r_data->old_vnum );
         continue;
      }

      /*
       * remove it from the hash list 
       */
      iHash = r_data->old_vnum % MAX_KEY_HASH;
      if( obj_index_hash[iHash] == obj )
         obj_index_hash[iHash] = obj->next;
      else
      {
         for( obj_prev = obj_index_hash[iHash]; obj_prev && obj_prev->next != obj; obj_prev = obj_prev->next )
            ;
         if( obj_prev == NULL )
         {
            bug( "%s", "renumber_area: Couldn't find an obj in the hash table! Skipping it." );
            continue;
         }
         obj_prev->next = obj->next;
         obj->next = NULL;
      }

      /*
       * change the vnum 
       */
      obj->vnum = r_data->new_vnum;

      /*
       * to our list 
       */
      obj->next = obj_list;
      obj_list = obj;
   }
   for( obj = obj_list; obj; obj = obj_next )
   {
      obj_next = obj->next;
      /*
       * add it to the hash list again 
       */
      iHash = obj->vnum % MAX_KEY_HASH;
      obj->next = obj_index_hash[iHash];
      obj_index_hash[iHash] = obj;
   }
   if( r_area->r_obj )
   {
      if( r_area->low_obj < r_area->low_room && r_area->low_obj < r_area->low_mob )
         area->low_vnum = r_area->low_obj;
      if( r_area->hi_obj > r_area->hi_room && r_area->hi_obj > r_area->hi_mob )
         area->hi_vnum = r_area->hi_obj;
   }

   send_to_pager( "Fixing references...\n\r", ch );

   send_to_pager( "... fixing objvals...\n\r", ch );
   translate_objvals( ch, area, r_area, verbose );

   send_to_pager( "... fixing exits...\n\r", ch );
   translate_exits( ch, area, r_area, verbose );

   send_to_pager( "... fixing resets...\n\r", ch );
   for( room = area->first_room; room; room = room->next_aroom )
   {
      for( preset = room->first_reset; preset; preset = preset->next )
      {
         translate_reset( preset, r_area );
         for( treset = preset->first_reset; treset; treset = preset->next_reset )
            translate_reset( treset, r_area );
      }
   }

   if( verbose )
   {
      send_to_pager( "Searching progs for references to renumbered vnums...\n\r", ch );
      warn_progs( ch, low, high, area, r_area );
   }
}

void translate_exits( CHAR_DATA * ch, AREA_DATA * area, RENUMBER_AREA * r_area, bool verbose )
{
   int i, new_vnum;
   EXIT_DATA *pexit, *rv_exit;
   ENTRANCE_DATA *enter;
   ROOM_INDEX_DATA *room;
   int old_vnum;

   for( i = area->low_vnum; i <= area->hi_vnum; i++ )
   {
      room = get_room_index( i );
      if( !room )
         continue;
      for( pexit = room->first_exit; pexit; pexit = pexit->next )
      {
         /*
          * translate the exit destination, if it was moved 
          */
         new_vnum = find_translation( pexit->vnum, r_area->r_room );
         if( new_vnum != NOT_FOUND )
            pexit->vnum = new_vnum;
         /*
          * if this room was moved 
          */
         if( pexit->rvnum != i )
         {
            old_vnum = pexit->rvnum;
            pexit->rvnum = i;
            /*
             * all reverse exits in other areas will be wrong 
             */
            rv_exit = get_exit_to( pexit->to_room, rev_dir[pexit->vdir], old_vnum );
            if( rv_exit && pexit->to_room->area != area )
            {
               if( rv_exit->vnum != i )
               {
                  pager_printf( ch, "...    fixing reverse exit in area %s.\n\r", pexit->to_room->area->filename );
                  rv_exit->vnum = i;
               }
            }
         }

         /*
          * translate the key 
          */
         if( pexit->key != -1 )
         {
            new_vnum = find_translation( pexit->key, r_area->r_obj );
            if( new_vnum == NOT_FOUND )
               continue;
            pexit->key = new_vnum;
         }
      }
      for( enter = first_entrance; enter; enter = enter->next )
      {
         new_vnum = find_translation( enter->vnum, r_area->r_room );
         if( new_vnum != NOT_FOUND )
         {
            enter->vnum = new_vnum;
            send_to_pager( "...    fixing overland exit to area.\n\r", ch );
         }
      }
   }
}

void translate_objvals( CHAR_DATA * ch, AREA_DATA * area, RENUMBER_AREA * r_area, bool verbose )
{
   int i, new_vnum;
   OBJ_INDEX_DATA *obj;

   for( i = area->low_vnum; i <= area->hi_vnum; i++ )
   {
      obj = get_obj_index( i );
      if( !obj )
         continue;

      if( obj->item_type == ITEM_CONTAINER )
      {
         new_vnum = find_translation( obj->value[2], r_area->r_obj );
         if( new_vnum != NOT_FOUND )
         {
            if( verbose )
               pager_printf( ch, "...   container %d; fixing objval2 (key vnum) %d -> %d\n\r", i, obj->value[2], new_vnum );
            obj->value[2] = new_vnum;
         }
         else if( verbose )
            pager_printf( ch, "...    container %d; no need to fix.\n\r", i );
      }
      else if( obj->item_type == ITEM_SWITCH || obj->item_type == ITEM_LEVER ||
               obj->item_type == ITEM_PULLCHAIN || obj->item_type == ITEM_BUTTON )
      {
         /*
          * levers might have room vnum references in their objvals 
          */
         if( IS_SET( obj->value[0], TRIG_TELEPORT )
             || IS_SET( obj->value[0], TRIG_TELEPORTALL )
             || IS_SET( obj->value[0], TRIG_TELEPORTPLUS )
             || IS_SET( obj->value[0], TRIG_RAND4 )
             || IS_SET( obj->value[0], TRIG_RAND6 ) || IS_SET( obj->value[0], TRIG_DOOR ) )
         {
            new_vnum = find_translation( obj->value[1], r_area->r_room );
            if( new_vnum != NOT_FOUND )
            {
               if( verbose )
                  pager_printf( ch, "...    lever %d: fixing source room (%d -> %d)\n\r", i, obj->value[1], new_vnum );
               obj->value[1] = new_vnum;
            }
            if( IS_SET( obj->value[0], TRIG_DOOR ) && IS_SET( obj->value[0], TRIG_PASSAGE ) )
            {
               new_vnum = find_translation( obj->value[2], r_area->r_room );
               if( new_vnum != NOT_FOUND )
               {
                  if( verbose )
                     pager_printf( ch, "...    lever %d: fixing dest room (passage) (%d -> %d)\n\r",
                                   i, obj->value[2], new_vnum );
                  obj->value[2] = new_vnum;
               }
            }
         }
      }
   }
}

void warn_progs( CHAR_DATA * ch, int low, int high, AREA_DATA * area, RENUMBER_AREA * r_area )
{
   ROOM_INDEX_DATA *room;
   OBJ_INDEX_DATA *obj;
   MOB_INDEX_DATA *mob;
   MPROG_DATA *mprog;
   int i;

   for( i = area->low_vnum; i <= area->hi_vnum; i++ )
   {
      room = get_room_index( i );
      if( !room )
         continue;
      mprog = room->mudprogs;
      while( mprog )
      {
         warn_in_prog( ch, low, high, "room", i, mprog, r_area );
         mprog = mprog->next;
      }
   }

   for( i = area->low_vnum; i <= area->hi_vnum; i++ )
   {
      obj = get_obj_index( i );
      if( !obj )
         continue;
      mprog = obj->mudprogs;
      while( mprog )
      {
         warn_in_prog( ch, low, high, "obj", i, mprog, r_area );
         mprog = mprog->next;
      }
   }

   for( i = area->low_vnum; i <= area->hi_vnum; i++ )
   {
      mob = get_mob_index( i );
      if( !mob )
         continue;
      mprog = mob->mudprogs;
      while( mprog )
      {
         warn_in_prog( ch, low, high, "mob", i, mprog, r_area );
         mprog = mprog->next;
      }
   }
}

void warn_in_prog( CHAR_DATA * ch, int low, int high, char *where, int vnum, MPROG_DATA * mprog, RENUMBER_AREA * r_area )
{
   char *p, *start_number, cTmp;
   int num;

   p = mprog->comlist;
   while( *p )
   {
      if( isdigit( *p ) )
      {
         start_number = p;
         while( isdigit( *p ) && *p )
            p++;
         cTmp = *p;
         *p = 0;
         num = atoi( start_number );
         *p = cTmp;
         if( num >= low && num <= high )
         {
            pager_printf( ch,
                          "Warning! %s prog in %s vnum %d might contain a reference to %d.\n\r(Translation: Room %d, Obj %d, Mob %d)\n\r",
                          mprog_type_to_name( mprog->type ), where, vnum, num, find_translation( num, r_area->r_room ),
                          find_translation( num, r_area->r_obj ), find_translation( num, r_area->r_mob ) );
         }
         if( *p == '\0' )
            break;
      }
      p++;
   }
}

/* this function translates a reset according to the renumber data in r_data */
void translate_reset( RESET_DATA * reset, RENUMBER_AREA * r_data )
{
   /*
    * a list based approach to fixing the resets. instead of having a bunch of several instances of very 
    * similar code, i just made this array that tells the code what to do. it's pretty straightforward 
    */
   char *action_table[] = { "Mm1r3", "Oo1r3", "Ho1", "Po1o3", "Go1", "Eo1", "Dr1", "Rr1", NULL };
   char *p;
   RENUMBER_DATA *r_table;
   int *parg, new_vnum, i;

   /*
    * T is a special case 
    */
   if( reset->command == 'T' )
   {
      if( IS_SET( reset->extra, TRAP_ROOM ) )
         r_table = r_data->r_room;
      else if( IS_SET( reset->extra, TRAP_OBJ ) )
         r_table = r_data->r_obj;
      else
      {
         bug( "%s", "translate_reset: Invalid 'T' reset found." );
         return;
      }
      new_vnum = find_translation( reset->arg3, r_table );
      if( new_vnum != NOT_FOUND )
         reset->arg3 = new_vnum;
      return;
   }

   for( i = 0; action_table[i] != NULL; i++ )
   {
      if( reset->command == action_table[i][0] )
      {
         p = action_table[i] + 1;
         while( *p )
         {
            if( *p == 'm' )
               r_table = r_data->r_mob;
            else if( *p == 'o' )
               r_table = r_data->r_obj;
            else if( *p == 'r' )
               r_table = r_data->r_room;
            else
            {
               bug( "%s", "translate_reset: Invalid action found in action table." );
               p += 2;
               continue;
            }
            p++;

            if( *p == '1' )
               parg = &( reset->arg1 );
            else if( *p == '2' )
               parg = &( reset->arg2 );
            else if( *p == '3' )
               parg = &( reset->arg3 );
            else
            {
               bug( "%s", "translate_reset: Invalid argument number found in action table." );
               p++;
               continue;
            }
            p++;

            new_vnum = find_translation( *parg, r_table );
            if( new_vnum != NOT_FOUND )
               *parg = new_vnum;

         }
         return;
      }
   }

   if( action_table[i] == NULL )
      bug( "translate_reset: Invalid reset '%c' found.", reset->command );
}

/* returns the new vnum for the old vnum "vnum" according to the info in r_data */
int find_translation( int vnum, RENUMBER_DATA * r_data )
{
   RENUMBER_DATA *r_temp;

   for( r_temp = r_data; r_temp; r_temp = r_temp->next )
   {
      if( r_temp->old_vnum == vnum )
         return r_temp->new_vnum;
   }
   return NOT_FOUND;
}

/* simply returns a pointer to a "filename" or NULL if no such area.
 *  stores TRUE in *p_is_proto if the area is proto
 */
AREA_DATA *find_area( char *filename )
{
   bool found;
   AREA_DATA *area;

   found = FALSE;

   for( area = first_area; area; area = area->next )
   {
      if( !str_cmp( area->filename, filename ) )
      {
         found = TRUE;
         break;
      }
   }

   if( found )
      return area;
   else
      return NULL;
}
