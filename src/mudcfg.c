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
 *                          MUD Specific Functions                          *
 ****************************************************************************/

/* This file will not be covered by future patches. Any specific code you don't
 * want changed later should go here.
 */
#include "mud.h"
#include "clans.h"
#include "deity.h"

double distance( short chX, short chY, short lmX, short lmY );   /* For check_room */

extern int astral_target;

/* New continent and plane based death relocation - Samson 3-29-98 */















ROOM_INDEX_DATA *check_room( CHAR_DATA * ch, ROOM_INDEX_DATA * dieroom )
{
   ROOM_INDEX_DATA *location = NULL;

   if( dieroom->area->continent == ACON_ALSHEROK )




























      location = get_room_index( ROOM_VNUM_ALTAR );

   if( dieroom->area->continent == ACON_ELETAR )
      location = get_room_index( ROOM_VNUM_ELETAR_DEATH );
   if( dieroom->area->continent == ACON_ALATIA )
      location = get_room_index( ROOM_VNUM_ALATIA_DEATH );





   if( !location )
      location = get_room_index( ROOM_VNUM_ALTAR );

   return location;
}

ROOM_INDEX_DATA *recall_room( CHAR_DATA * ch )
{
   ROOM_INDEX_DATA *location = NULL;

   if( IS_NPC( ch ) )
   {
      location = get_room_index( ROOM_VNUM_TEMPLE );
      if( !location )
         location = get_room_index( ROOM_VNUM_LIMBO );
      return location;
   }



   if( ch->pcdata->clan )
      location = get_room_index( ch->pcdata->clan->recall );
   if( !location && ch->pcdata->home )
      location = get_room_index( ch->pcdata->home );


   if( !location )
   {
      if( ch->in_room->area->continent == ACON_ALSHEROK )
      {
         location = get_room_index( ch->pcdata->alsherok );

         if( !location )
            location = get_room_index( ROOM_VNUM_TEMPLE );
      }

      if( ch->in_room->area->continent == ACON_ELETAR )
      {
         location = get_room_index( ch->pcdata->eletar );

         if( !location )
            location = get_room_index( ROOM_VNUM_ELETAR_RECALL );
      }

      if( ch->in_room->area->continent == ACON_ALATIA )
      {
         location = get_room_index( ch->pcdata->alatia );

         if( !location )
            location = get_room_index( ROOM_VNUM_ALATIA_RECALL );
      }





      if( ch->in_room->area->continent == ACON_ASTRAL )
         location = get_room_index( astral_target );
   }
   if( !location && ch->pcdata->deity && ch->pcdata->deity->recallroom )
      location = get_room_index( ch->pcdata->deity->recallroom );

   if( !location )
      location = get_room_index( ROOM_VNUM_TEMPLE );

   /*
    * Hey, look, if you get *THIS* damn far and still come up with nothing, you *DESERVE* to crash! 
    */
   if( !location )
      location = get_room_index( ROOM_VNUM_LIMBO );

   return location;
}

bool beacon_check( CHAR_DATA * ch, ROOM_INDEX_DATA * beacon )
{























   return TRUE;
}
