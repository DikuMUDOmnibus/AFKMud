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
 *                        Player movement module                            *
 ****************************************************************************/

#include "mud.h"
#include "overland.h"

ch_ret move_ship( CHAR_DATA * ch, EXIT_DATA * pexit, int direction );   /* Movement if your on a ship */
ch_ret check_room_for_traps( CHAR_DATA * ch, int flag );

char *const dir_name[] = {
   "north", "east", "south", "west", "up", "down",
   "northeast", "northwest", "southeast", "southwest", "somewhere"
};

char *const short_dirname[] = {
   "n", "e", "s", "w", "u", "d", "ne", "nw", "se", "sw", "?"
};

const int trap_door[] = {
   TRAP_N, TRAP_E, TRAP_S, TRAP_W, TRAP_U, TRAP_D,
   TRAP_NE, TRAP_NW, TRAP_SE, TRAP_SW
};

const short rev_dir[] = {
   2, 3, 0, 1, 5, 4, 9, 8, 7, 6, 10
};

int get_dirnum( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( dir_name ) / sizeof( dir_name[0] ) ); x++ )
      if( !str_cmp( flag, dir_name[x] ) )
         return x;

   for( x = 0; x < ( sizeof( short_dirname ) / sizeof( short_dirname[0] ) ); x++ )
      if( !str_cmp( flag, short_dirname[x] ) )
         return x;

   return -1;
}

char *rev_exit( short vdir )
{
   switch ( vdir )
   {
      default:
         return "somewhere";
      case 0:
         return "the south";
      case 1:
         return "the west";
      case 2:
         return "the north";
      case 3:
         return "the east";
      case 4:
         return "below";
      case 5:
         return "above";
      case 6:
         return "the southwest";
      case 7:
         return "the southeast";
      case 8:
         return "the northwest";
      case 9:
         return "the northeast";
   }
}

/*
 * Function to get the equivelant exit of DIR 0-MAXDIR out of linked list.
 * Made to allow old-style diku-merc exit functions to work.	-Thoric
 */
EXIT_DATA *get_exit( ROOM_INDEX_DATA * room, short dir )
{
   EXIT_DATA *xit;

   if( !room )
   {
      bug( "%s", "Get_exit: NULL room" );
      return NULL;
   }

   for( xit = room->first_exit; xit; xit = xit->next )
      if( xit->vdir == dir )
         return xit;
   return NULL;
}

/* Return the hide affect for sneaking PCs when they enter a new room */
void check_sneaks( CHAR_DATA * ch )
{
   if( is_affected( ch, gsn_sneak ) && !IS_AFFECTED( ch, AFF_HIDE ) )
   {
      SET_AFFECTED( ch, AFF_HIDE );
      SET_AFFECTED( ch, AFF_SNEAK );
   }
   return;
}

/*
 * Modify movement due to encumbrance				-Thoric
 */
short encumbrance( CHAR_DATA * ch, short move )
{
   int cur, max;

   max = can_carry_w( ch );
   cur = ch->carry_weight;
   if( cur >= max )
      return move * 4;
   else if( cur >= max * 0.95 )
      return ( short ) ( move * 3.5 );
   else if( cur >= max * 0.90 )
      return move * 3;
   else if( cur >= max * 0.85 )
      return ( short ) ( move * 2.5 );
   else if( cur >= max * 0.80 )
      return move * 2;
   else if( cur >= max * 0.75 )
      return ( short ) ( move * 1.5 );
   else
      return move;
}

/*
 * Check to see if a character can fall down, checks for looping   -Thoric
 */
bool will_fall( CHAR_DATA * ch, int fall )
{
   if( !ch )
   {
      bug( "%s", "will_fall: NULL *ch!!" );
      return FALSE;
   }

   if( !ch->in_room )
   {
      bug( "will_fall: Character in NULL room: %s", ch->name ? ch->name : "Unknown?!?" );
      return FALSE;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_NOFLOOR ) && CAN_GO( ch, DIR_DOWN )
       && ( !IS_AFFECTED( ch, AFF_FLYING ) || ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLYING ) ) ) )
   {
      if( fall > 80 )
      {
         bug( "Falling (in a loop?) more than 80 rooms: vnum %d", ch->in_room->vnum );
         leave_map( ch, NULL, get_room_index( ROOM_VNUM_TEMPLE ) );
         fall = 0;
         return TRUE;
      }
      send_to_char( "&[falling]You're falling down...\n\r", ch );
      move_char( ch, get_exit( ch->in_room, DIR_DOWN ), ++fall, DIR_DOWN, FALSE );
      return TRUE;
   }
   return FALSE;
}

/* Run command taken from DOTD codebase - Samson 2-25-99 */
CMDF do_run( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   ROOM_INDEX_DATA *from_room;
   EXIT_DATA *pexit;
   int amount = 0, x, fromx, fromy, frommap;
   bool limited = FALSE;

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Run where?\n\r", ch );
      return;
   }

   if( ch->position != POS_STANDING && ch->position != POS_MOUNTED )
   {
      send_to_char( "You are not in the correct position for that.\n\r", ch );
      return;
   }

   if( argument )
   {
      if( is_number( argument ) )
      {
         limited = TRUE;
         amount = atoi( argument );
      }
   }

   from_room = ch->in_room;
   frommap = ch->map;
   fromx = ch->x;
   fromy = ch->y;

   if( limited )
   {
      for( x = 1; x <= amount; x++ )
      {
         if( ( pexit = find_door( ch, arg, TRUE ) ) != NULL )
         {
            if( ch->move < 1 )
            {
               send_to_char( "You are too exhausted to run anymore.\n\r", ch );
               ch->move = 0;
               break;
            }
            if( move_char( ch, pexit, 0, pexit->vdir, TRUE ) == rSTOP )
               break;
         }

         if( ch->position != POS_STANDING && ch->position != POS_MOUNTED )
         {
            send_to_char( "Your run has been interrupted!\r\n", ch );
            break;
         }
      }
   }
   else
   {
      while( ( pexit = find_door( ch, arg, TRUE ) ) != NULL )
      {
         if( ch->move < 1 )
         {
            send_to_char( "You are too exhausted to run anymore.\n\r", ch );
            ch->move = 0;
            break;
         }
         if( move_char( ch, pexit, 0, pexit->vdir, TRUE ) == rSTOP )
            break;

         if( ch->position != POS_STANDING && ch->position != POS_MOUNTED )
         {
            send_to_char( "Your run has been interrupted!\r\n", ch );
            break;
         }
      }
   }

   if( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_ACT_FLAG( ch, ACT_ONMAP ) )
   {
      if( ch->x == fromx && ch->y == fromy && ch->map == frommap )
      {
         send_to_char( "You try to run but don't get anywhere.\n\r", ch );
         act( AT_ACTION, "$n tries to run but doesn't get anywhere.", ch, NULL, NULL, TO_ROOM );
         return;
      }
   }
   else
   {
      if( ch->in_room == from_room )
      {
         send_to_char( "You try to run but don't get anywhere.\n\r", ch );
         act( AT_ACTION, "$n tries to run but doesn't get anywhere.", ch, NULL, NULL, TO_ROOM );
         return;
      }
   }

   send_to_char( "You slow down after your run.\n\r", ch );
   act( AT_ACTION, "$n slows down after $s run.", ch, NULL, NULL, TO_ROOM );

   interpret( ch, "look" );
   return;
}

ch_ret move_char( CHAR_DATA * ch, EXIT_DATA * pexit, int fall, int direction, bool running )
{
   ROOM_INDEX_DATA *in_room, *to_room, *from_room;
   OBJ_DATA *boat;
   char *txt, *dtxt;
   ch_ret retcode;
   short door;
   bool drunk = FALSE, brief = FALSE;

   retcode = rNONE;
   txt = NULL;

   if( IS_PLR_FLAG( ch, PLR_ONSHIP ) && ch->on_ship != NULL )
   {
      retcode = move_ship( ch, pexit, direction );
      return retcode;
   }

   if( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_ACT_FLAG( ch, ACT_ONMAP ) )
   {
      int newx = ch->x;
      int newy = ch->y;

      if( ch->inflight )
      {
         send_to_char( "Sit still! You cannot go anywhere until the skyship has landed.\n\r", ch );
         return rSTOP;
      }

      if( IS_ROOM_FLAG( ch->in_room, ROOM_WATCHTOWER ) )
      {
         if( direction != DIR_DOWN )
         {
            send_to_char( "Alas, you cannot go that way.\n\r", ch );
            return rSTOP;
         }

         pexit = get_exit( ch->in_room, DIR_DOWN );

         if( !pexit || !pexit->to_room )
         {
            bug( "%s: Broken Watchtower exit in room %d!", __FUNCTION__, ch->in_room->vnum );
            ch_printf( ch, "Ooops! The watchtower here is broken. Please contact the immortals. You are in room %d.\n\r",
                       ch->in_room->vnum );
            return rSTOP;
         }

         leave_map( ch, NULL, pexit->to_room );
         return rSTOP;
      }

      switch ( direction )
      {
         default:
            break;
         case DIR_NORTH:
            newy = ch->y - 1;
            break;
         case DIR_EAST:
            newx = ch->x + 1;
            break;
         case DIR_SOUTH:
            newy = ch->y + 1;
            break;
         case DIR_WEST:
            newx = ch->x - 1;
            break;
         case DIR_NORTHEAST:
            newx = ch->x + 1;
            newy = ch->y - 1;
            break;
         case DIR_NORTHWEST:
            newx = ch->x - 1;
            newy = ch->y - 1;
            break;
         case DIR_SOUTHEAST:
            newx = ch->x + 1;
            newy = ch->y + 1;
            break;
         case DIR_SOUTHWEST:
            newx = ch->x - 1;
            newy = ch->y + 1;
            break;
      }
      if( newx == ch->x && newy == ch->y )
         return rSTOP;

      retcode = process_exit( ch, ch->map, newx, newy, direction, running );
      return retcode;
   }

   if( !IS_NPC( ch ) )
   {
      if( IS_DRUNK( ch, 2 ) && ( ch->position != POS_SHOVE ) && ( ch->position != POS_DRAG ) )
         drunk = TRUE;
   }

   if( drunk && !fall )
   {
      door = number_door(  );
      pexit = get_exit( ch->in_room, door );
   }

   in_room = ch->in_room;
   from_room = in_room;
   if( !pexit || ( to_room = pexit->to_room ) == NULL )
   {
      if( drunk && ch->position != POS_MOUNTED
          && ch->in_room->sector_type != SECT_WATER_SWIM
          && ch->in_room->sector_type != SECT_WATER_NOSWIM
          && ch->in_room->sector_type != SECT_RIVER
          && ch->in_room->sector_type != SECT_UNDERWATER && ch->in_room->sector_type != SECT_OCEANFLOOR )
      {
         switch ( number_bits( 4 ) )
         {
            default:
               act( AT_ACTION, "You drunkenly stumble into some obstacle.", ch, NULL, NULL, TO_CHAR );
               act( AT_ACTION, "$n drunkenly stumbles into a nearby obstacle.", ch, NULL, NULL, TO_ROOM );
               break;
            case 3:
               act( AT_ACTION, "In your drunken stupor you trip over your own feet and tumble to the ground.", ch, NULL,
                    NULL, TO_CHAR );
               act( AT_ACTION, "$n stumbles drunkenly, trips and tumbles to the ground.", ch, NULL, NULL, TO_ROOM );
               ch->position = POS_RESTING;
               break;
            case 4:
               act( AT_SOCIAL, "You utter a string of slurred obscenities.", ch, NULL, NULL, TO_CHAR );
               act( AT_ACTION, "Something blurry and immovable has intercepted you as you stagger along.", ch, NULL, NULL,
                    TO_CHAR );
               act( AT_HURT, "Oh geez... THAT really hurt.  Everything slowly goes dark and numb...", ch, NULL, NULL,
                    TO_CHAR );
               act( AT_ACTION, "$n drunkenly staggers into something.", ch, NULL, NULL, TO_ROOM );
               act( AT_SOCIAL, "$n utters a string of slurred obscenities: @*&^%@*&!", ch, NULL, NULL, TO_ROOM );
               act( AT_ACTION, "$n topples to the ground with a thud.", ch, NULL, NULL, TO_ROOM );
               ch->position = POS_INCAP;
               break;
         }
      }
      else if( drunk )
         act( AT_ACTION, "You stare around trying to make sense of things through your drunken stupor.", ch, NULL, NULL,
              TO_CHAR );
      else
         send_to_char( "Alas, you cannot go that way.\n\r", ch );

      check_sneaks( ch );
      return rSTOP;
   }

   door = pexit->vdir;

   if( pexit->to_room->sector_type == SECT_TREE && !IS_AFFECTED( ch, AFF_TREE_TRAVEL ) )
   {
      send_to_char( "The forest is too thick for you to pass through that way.\n\r", ch );
      check_sneaks( ch );
      return rSTOP;
   }

   /*
    * Exit is only a "window", there is no way to travel in that direction
    * unless it's a door with a window in it      -Thoric
    */
   if( IS_EXIT_FLAG( pexit, EX_WINDOW ) && !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
   {
      send_to_char( "There is a window blocking the way.\n\r", ch );
      check_sneaks( ch );
      return rSTOP;
   }

   /*
    * Keeps people from walking through walls 
    */
   if( ( IS_EXIT_FLAG( pexit, EX_FORTIFIED )
         || IS_EXIT_FLAG( pexit, EX_HEAVY )
         || IS_EXIT_FLAG( pexit, EX_MEDIUM )
         || IS_EXIT_FLAG( pexit, EX_LIGHT )
         || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) ) && ( IS_NPC( ch ) || !IS_PCFLAG( ch, PCFLAG_PASSDOOR ) ) )
   {
      act( AT_PLAIN, "There is a $d blocking the way.", ch, NULL, pexit->keyword, TO_CHAR );
      check_sneaks( ch );
      return rSTOP;
   }

   /*
    * Overland Map stuff - Samson 7-31-99 
    */
   /*
    * Upgraded 4-28-00 to allow mounts and charmies to follow PC - Samson 
    */
   if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
   {
      CHAR_DATA *fch;
      CHAR_DATA *nextinroom;
      int chars = 0, count = 0;

      if( pexit->x < 0 || pexit->x >= MAX_X || pexit->y < 0 || pexit->y >= MAX_Y )
      {
         bug( "%s: Room #%d - Invalid exit coordinates: %d %d", __FUNCTION__, in_room->vnum, pexit->x, pexit->y );
         send_to_char( "Oops. Something is wrong with this map exit - notify the immortals.\n\r", ch );
         check_sneaks( ch );
         return rSTOP;
      }

      if( !IS_NPC( ch ) )
      {
         enter_map( ch, pexit, pexit->x, pexit->y, -1 );

         for( fch = from_room->first_person; fch; fch = fch->next_in_room )
            chars++;

         for( fch = from_room->first_person; fch && ( count < chars ); fch = nextinroom )
         {
            nextinroom = fch->next_in_room;
            count++;
            if( fch != ch  /* loop room bug fix here by Thoric */
                && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
            {
               if( !IS_NPC( fch ) )
               {
                  /*
                   * Added checks so morts don't blindly follow the leader into DT's. 
                   * -- Tarl 16 July 2002 
                   */
                  if( IS_ROOM_FLAG( pexit->to_room, ROOM_DEATH ) )
                     send_to_char( "You stand your ground.", fch );
                  else
                  {
                     if( !get_exit( from_room, direction ) )
                     {
                        act( AT_ACTION, "The entrance closes behind $N, preventing you from following!", fch, NULL, ch, TO_CHAR );
                        continue;
                     }
                     act( AT_ACTION, "You follow $N.", fch, NULL, ch, TO_CHAR );
                     move_char( fch, pexit, 0, direction, running );
                  }
               }
               else
                  enter_map( fch, pexit, pexit->x, pexit->y, -1 );
            }
         }
      }
      else
      {
         if( !IS_EXIT_FLAG( pexit, EX_NOMOB ) )
         {
            enter_map( ch, pexit, pexit->x, pexit->y, -1 );

            for( fch = from_room->first_person; fch; fch = fch->next_in_room )
               chars++;

            for( fch = from_room->first_person; fch && ( count < chars ); fch = nextinroom )
            {
               nextinroom = fch->next_in_room;
               count++;
               if( fch != ch  /* loop room bug fix here by Thoric */
                   && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
               {
                  if( !IS_NPC( fch ) )
                  {
                     /*
                      * Added checks so morts don't blindly follow the leader into DT's. 
                      * -- Tarl 16 July 2002 
                      */
                     if( IS_ROOM_FLAG( pexit->to_room, ROOM_DEATH ) )
                        send_to_char( "You stand your ground.", fch );
                     else
                     {
                        if( !get_exit( from_room, direction ) )
                        {
                           act( AT_ACTION, "The entrance closes behind $N, preventing you from following!", fch, NULL, ch, TO_CHAR );
                           continue;
                        }
                        act( AT_ACTION, "You follow $N.", fch, NULL, ch, TO_CHAR );
                        move_char( fch, pexit, 0, direction, running );
                     }
                  }
                  else
                     enter_map( fch, pexit, pexit->x, pexit->y, -1 );
               }
            }
         }
      }
      check_sneaks( ch );
      return rSTOP;
   }

   if( IS_EXIT_FLAG( pexit, EX_PORTAL ) )
   {
      if( IS_NPC( ch ) )
      {
         act( AT_PLAIN, "Mobs can't use portals.", ch, NULL, NULL, TO_CHAR );
         check_sneaks( ch );
         return rSTOP;
      }
      else
      {
         if( !has_visited( ch, pexit->to_room->area ) )
         {
            send_to_char( "Magic from the portal repulses your attempt to enter!\n\r", ch );
            check_sneaks( ch );
            return rSTOP;
         }
      }
   }

   if( IS_EXIT_FLAG( pexit, EX_NOMOB ) && IS_NPC( ch ) )
   {
      act( AT_PLAIN, "Mobs can't enter there.", ch, NULL, NULL, TO_CHAR );
      check_sneaks( ch );
      return rSTOP;
   }

   if( IS_ROOM_FLAG( to_room, ROOM_NO_MOB ) && IS_NPC( ch ) )
   {
      act( AT_PLAIN, "Mobs can't enter there.", ch, NULL, NULL, TO_CHAR );
      check_sneaks( ch );
      return rSTOP;
   }

   if( IS_EXIT_FLAG( pexit, EX_CLOSED )
       && ( !IS_AFFECTED( ch, AFF_PASS_DOOR ) || IS_EXIT_FLAG( pexit, EX_NOPASSDOOR ) )
       && ( IS_NPC( ch ) || !IS_PCFLAG( ch, PCFLAG_PASSDOOR ) ) )
   {
      if( !IS_EXIT_FLAG( pexit, EX_SECRET ) && !IS_EXIT_FLAG( pexit, EX_DIG ) )
      {
         if( drunk )
         {
            act( AT_PLAIN, "$n runs into the $d in $s drunken state.", ch, NULL, pexit->keyword, TO_ROOM );
            act( AT_PLAIN, "You run into the $d in your drunken state.", ch, NULL, pexit->keyword, TO_CHAR );
         }
         else
            act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
      }
      else
      {
         if( drunk )
            send_to_char( "You stagger around in your drunken state.\n\r", ch );
         else
            send_to_char( "Alas, you cannot go that way.\n\r", ch );
      }

      check_sneaks( ch );
      return rSTOP;
   }

   if( !fall && IS_AFFECTED( ch, AFF_CHARM ) && ch->master && in_room == ch->master->in_room )
   {
      send_to_char( "What? And leave your beloved master?\n\r", ch );
      check_sneaks( ch );
      return rSTOP;
   }

   if( room_is_private( to_room ) )
   {
      send_to_char( "That room is private right now.\n\r", ch );
      check_sneaks( ch );
      return rSTOP;
   }

   /*
    * Room flag to set TOTAL isolation, for those absolutely private moments - Samson 3-26-01 
    */
   if( IS_ROOM_FLAG( to_room, ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      send_to_char( "Go away! That room has been sealed for privacy!\n\r", ch );
      check_sneaks( ch );
      return rSTOP;
   }

   if( !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && ch->in_room->area != to_room->area )
   {
      if( ch->level < to_room->area->low_hard_range )
      {
         set_char_color( AT_TELL, ch );
         switch ( to_room->area->low_hard_range - ch->level )
         {
            case 1:
               send_to_char( "A voice in your mind says, 'You are nearly ready to go that way...'", ch );
               break;
            case 2:
               send_to_char( "A voice in your mind says, 'Soon you shall be ready to travel down this path... soon.'", ch );
               break;
            case 3:
               send_to_char( "A voice in your mind says, 'You are not ready to go down that path... yet.'.\n\r", ch );
               break;
            default:
               send_to_char( "A voice in your mind says, 'You are not ready to go down that path.'.\n\r", ch );
         }
         check_sneaks( ch );
         return rSTOP;
      }
      else if( ch->level > to_room->area->hi_hard_range )
      {
         set_char_color( AT_TELL, ch );
         send_to_char( "A voice in your mind says, 'There is nothing more for you down that path.'", ch );
         check_sneaks( ch );
         return rSTOP;
      }
   }

   if( !fall )
   {
      int move;

      if( in_room->sector_type == SECT_AIR || to_room->sector_type == SECT_AIR || IS_EXIT_FLAG( pexit, EX_FLY ) )
      {
         if( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLYING ) )
         {
            send_to_char( "Your mount can't fly.\n\r", ch );
            check_sneaks( ch );
            return rSTOP;
         }
         if( !ch->mount && !IS_AFFECTED( ch, AFF_FLYING ) )
         {
            send_to_char( "You'd need to fly to go there.\n\r", ch );
            check_sneaks( ch );
            return rSTOP;
         }
      }

      /*
       * Water_swim sector information added by Samson on unknown date 
       */
      if( ( in_room->sector_type == SECT_WATER_SWIM && to_room->sector_type == SECT_WATER_SWIM )
          || ( in_room->sector_type != SECT_WATER_SWIM && to_room->sector_type == SECT_WATER_SWIM ) )
      {
         if( !IS_FLOATING( ch ) )
         {
            if( ( ch->mount && !IS_FLOATING( ch->mount ) ) || !ch->mount )
            {
               /*
                * Look for a boat.
                * We can use the boat obj for a more detailed description.
                */
               if( ( boat = get_objtype( ch, ITEM_BOAT ) ) != NULL )
               {
                  if( drunk )
                     txt = "paddles unevenly";
                  else
                     txt = "paddles";
               }
               else
               {
                  if( ch->mount )
                     send_to_char( "Your mount would drown!\n\r", ch );
                  else if( !IS_NPC( ch ) && number_percent(  ) > ch->pcdata->learned[gsn_swim]
                           && ch->pcdata->learned[gsn_swim] > 0 )
                  {
                     send_to_char( "Your swimming skills need improvement first.\n\r", ch );
                     learn_from_failure( ch, gsn_swim );
                  }
                  else
                     send_to_char( "You'd need a boat to go there.\n\r", ch );
                  check_sneaks( ch );
                  return rSTOP;
               }
            }
         }
      }

      /*
       * River sector information added by Samson on unknown date 
       */
      if( ( in_room->sector_type == SECT_RIVER && to_room->sector_type == SECT_RIVER )
          || ( in_room->sector_type != SECT_RIVER && to_room->sector_type == SECT_RIVER ) )
      {
         if( !IS_FLOATING( ch ) )
         {
            if( ( ch->mount && !IS_FLOATING( ch->mount ) ) || !ch->mount )
            {
               /*
                * Look for a boat.
                * We can use the boat obj for a more detailed description.
                */
               if( ( boat = get_objtype( ch, ITEM_BOAT ) ) != NULL )
               {
                  if( drunk )
                     txt = "paddles unevenly";
                  else
                     txt = "paddles";
               }
               else
               {
                  if( ch->mount )
                     send_to_char( "Your mount would drown!\n\r", ch );
                  else
                     send_to_char( "You'd need a boat to go there.\n\r", ch );
                  check_sneaks( ch );
                  return rSTOP;
               }
            }
         }
      }

      /*
       * Water_noswim sector information fixed by Samson on unknown date 
       */
      if( ( in_room->sector_type == SECT_WATER_NOSWIM && to_room->sector_type == SECT_WATER_NOSWIM )
          || ( in_room->sector_type != SECT_WATER_NOSWIM && to_room->sector_type == SECT_WATER_NOSWIM ) )
      {
         if( !IS_FLOATING( ch ) )
         {
            if( ( ch->mount && !IS_FLOATING( ch->mount ) ) || !ch->mount )
            {
               /*
                * Look for a boat.
                * We can use the boat obj for a more detailed description.
                */
               if( ( boat = get_objtype( ch, ITEM_BOAT ) ) != NULL )
               {
                  if( drunk )
                     txt = "paddles unevenly";
                  else
                     txt = "paddles";
               }
               else
               {
                  if( ch->mount )
                     send_to_char( "Your mount would drown!\n\r", ch );
                  else
                     send_to_char( "You'd need a boat to go there.\n\r", ch );
                  check_sneaks( ch );
                  return rSTOP;
               }
            }
         }
      }

      if( IS_EXIT_FLAG( pexit, EX_CLIMB ) )
      {
         bool found;

         found = FALSE;
         if( ch->mount && IS_AFFECTED( ch->mount, AFF_FLYING ) )
            found = TRUE;
         else if( IS_AFFECTED( ch, AFF_FLYING ) )
            found = TRUE;

         if( !found && !ch->mount )
         {
            if( ( !IS_NPC( ch ) && number_percent(  ) > LEARNED( ch, gsn_climb ) ) || drunk || ch->mental_state < -90 )
            {
               send_to_char( "You start to climb... but lose your grip and fall!\n\r", ch );
               learn_from_failure( ch, gsn_climb );
               if( pexit->vdir == DIR_DOWN )
               {
                  retcode = move_char( ch, pexit, 1, DIR_DOWN, FALSE );
                  return retcode;
               }
               send_to_char( "&[hurt]OUCH! You hit the ground!\n\r", ch );
               WAIT_STATE( ch, 20 );
               retcode = damage( ch, ch, ( pexit->vdir == DIR_UP ? 10 : 5 ), TYPE_UNDEFINED );
               return retcode;
            }
            found = TRUE;
            WAIT_STATE( ch, skill_table[gsn_climb]->beats );
            txt = "climbs";
         }

         if( !found )
         {
            send_to_char( "You can't climb.\n\r", ch );
            check_sneaks( ch );
            return rSTOP;
         }
      }

      if( ch->mount )
      {
         switch ( ch->mount->position )
         {
            case POS_DEAD:
               send_to_char( "Your mount is dead!\n\r", ch );
               check_sneaks( ch );
               return rSTOP;

            case POS_MORTAL:
            case POS_INCAP:
               send_to_char( "Your mount is hurt far too badly to move.\n\r", ch );
               check_sneaks( ch );
               return rSTOP;

            case POS_STUNNED:
               send_to_char( "Your mount is too stunned to do that.\n\r", ch );
               check_sneaks( ch );
               return rSTOP;

            case POS_SLEEPING:
               send_to_char( "Your mount is sleeping.\n\r", ch );
               check_sneaks( ch );
               return rSTOP;

            case POS_RESTING:
               send_to_char( "Your mount is resting.\n\r", ch );
               check_sneaks( ch );
               return rSTOP;

            case POS_SITTING:
               send_to_char( "Your mount is sitting down.\n\r", ch );
               check_sneaks( ch );
               return rSTOP;

            default:
               break;
         }

         if( !IS_FLOATING( ch->mount ) )
            move = sect_show[in_room->sector_type].move;
         else
            move = 1;
         if( ch->mount->move < move )
         {
            send_to_char( "Your mount is too exhausted.\n\r", ch );
            check_sneaks( ch );
            return rSTOP;
         }
      }
      else
      {
         if( !IS_FLOATING( ch ) )
            move = encumbrance( ch, sect_show[in_room->sector_type].move );
         else
            move = 1;
         if( ch->move < move )
         {
            send_to_char( "You are too exhausted.\n\r", ch );
            check_sneaks( ch );
            return rSTOP;
         }
      }

      if( !IS_IMMORTAL( ch ) )
         WAIT_STATE( ch, move );
      if( ch->mount )
         ch->mount->move -= move;
      else
         ch->move -= move;
   }

   /*
    * Check if player can fit in the room
    */
   if( to_room->tunnel > 0 )
   {
      CHAR_DATA *ctmp;
      int count = ch->mount ? 1 : 0;

      for( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room )
         if( ++count >= to_room->tunnel )
         {
            if( ch->mount && count == to_room->tunnel )
               send_to_char( "There is no room for both you and your mount there.\n\r", ch );
            else
               send_to_char( "There is no room for you there.\n\r", ch );
            check_sneaks( ch );
            return rSTOP;
         }
   }

   /*
    * check for traps on exit - later 
    */
   if( !IS_AFFECTED( ch, AFF_SNEAK ) && !IS_PLR_FLAG( ch, PLR_WIZINVIS ) )
   {
      if( fall )
         txt = "falls";
      else if( !txt )
      {
         if( ch->mount )
         {
            if( IS_AFFECTED( ch->mount, AFF_FLOATING ) )
               txt = "floats";
            else if( IS_AFFECTED( ch->mount, AFF_FLYING ) )
               txt = "flies";
            else
               txt = "rides";
         }
         else
         {
            if( IS_AFFECTED( ch, AFF_FLOATING ) )
            {
               if( drunk )
                  txt = "floats unsteadily";
               else
                  txt = "floats";
            }
            else if( IS_AFFECTED( ch, AFF_FLYING ) )
            {
               if( drunk )
                  txt = "flies shakily";
               else
                  txt = "flies";
            }
            else if( ch->position == POS_SHOVE )
               txt = "is shoved";
            else if( ch->position == POS_DRAG )
               txt = "is dragged";
            else
            {
               if( drunk )
                  txt = "stumbles drunkenly";
               else
                  txt = "leaves";
            }
         }
      }
      if( !running )
      {
         if( ch->mount )
            act_printf( AT_ACTION, ch, NULL, ch->mount, TO_NOTVICT, "$n %s %s upon $N.", txt, dir_name[door] );
         else
            act_printf( AT_ACTION, ch, NULL, dir_name[door], TO_ROOM, "$n %s $T.", txt );
      }
   }

   rprog_leave_trigger( ch );
   if( char_died( ch ) )
      return global_retcode;

   char_from_room( ch );
   if( !char_to_room( ch, to_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   check_sneaks( ch );
   if( ch->mount )
   {
      rprog_leave_trigger( ch->mount );
      if( char_died( ch->mount ) )
         return global_retcode;
      if( ch->mount )
      {
         char_from_room( ch->mount );
         if( !char_to_room( ch->mount, to_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      }
   }

   if( !IS_AFFECTED( ch, AFF_SNEAK ) && ( IS_NPC( ch ) || !IS_PLR_FLAG( ch, PLR_WIZINVIS ) ) )
   {
      if( fall )
         txt = "falls";
      else if( ch->mount )
      {
         if( IS_AFFECTED( ch->mount, AFF_FLOATING ) )
            txt = "floats in";
         else if( IS_AFFECTED( ch->mount, AFF_FLYING ) )
            txt = "flies in";
         else
            txt = "rides in";
      }
      else
      {
         if( IS_AFFECTED( ch, AFF_FLOATING ) )
         {
            if( drunk )
               txt = "floats in unsteadily";
            else
               txt = "floats in";
         }
         else if( IS_AFFECTED( ch, AFF_FLYING ) )
         {
            if( drunk )
               txt = "flies in shakily";
            else
               txt = "flies in";
         }
         else if( ch->position == POS_SHOVE )
            txt = "is shoved in";
         else if( ch->position == POS_DRAG )
            txt = "is dragged in";
         else
         {
            if( drunk )
               txt = "stumbles drunkenly in";
            else
               txt = "arrives";
         }
      }
      dtxt = rev_exit( door );
      if( !running )
      {
         if( ch->mount )
            act_printf( AT_ACTION, ch, NULL, ch->mount, TO_ROOM, "$n %s from %s upon $N.", txt, dtxt );
         else
            act_printf( AT_ACTION, ch, NULL, NULL, TO_ROOM, "$n %s from %s.", txt, dtxt );
      }
   }

   /*
    * Make sure everyone sees the room description of death traps. 
    */
   if( IS_ROOM_FLAG( ch->in_room, ROOM_DEATH ) && !IS_IMMORTAL( ch ) )
   {
      if( IS_PLR_FLAG( ch, PLR_BRIEF ) )
         brief = TRUE;
      REMOVE_PLR_FLAG( ch, PLR_BRIEF );
   }

   /*
    * BIG ugly looping problem here when the character is mptransed back
    * to the starting room.  To avoid this, check how many chars are in 
    * the room at the start and stop processing followers after doing
    * the right number of them.  -- Narn
    */
   if( !fall )
   {
      CHAR_DATA *fch;
      CHAR_DATA *nextinroom;
      int chars = 0, count = 0;

      for( fch = from_room->first_person; fch; fch = fch->next_in_room )
         chars++;

      for( fch = from_room->first_person; fch && ( count < chars ); fch = nextinroom )
      {
         nextinroom = fch->next_in_room;
         count++;

         if( fch != ch  /* loop room bug fix here by Thoric */
             && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
         {
            if( !running )
            {
               /*
                * Added checks so morts don't blindly follow the leader into DT's.
                * -- Tarl 16 July 2002 
                */
               if( IS_ROOM_FLAG( pexit->to_room, ROOM_DEATH ) )
                  send_to_char( "You stand your ground.", fch );
               else
               {
                  if( !get_exit( from_room, direction ) )
                  {
                     act( AT_ACTION, "The entrance closes behind $N, preventing you from following!", fch, NULL, ch, TO_CHAR );
                     continue;
                  }
                  act( AT_ACTION, "You follow $N.", fch, NULL, ch, TO_CHAR );
               }
            }
            if( IS_ROOM_FLAG( pexit->to_room, ROOM_DEATH ) )
               send_to_char( "You decide to wait here.", fch );
            else
               move_char( fch, pexit, 0, direction, running );
         }
      }
   }

   if( !running )
      interpret( ch, "look" );

   if( brief )
      SET_PLR_FLAG( ch, PLR_BRIEF );

   /*
    * Put good-old EQ-munching death traps back in!     -Thoric
    */
   if( IS_ROOM_FLAG( ch->in_room, ROOM_DEATH ) && !IS_IMMORTAL( ch ) )
   {
      act( AT_DEAD, "$n falls prey to a terrible death!", ch, NULL, NULL, TO_ROOM );
      send_to_char( "&[dead]Oopsie... you're dead!\n\r", ch );
      log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "%s hit a DEATH TRAP in room %d!", ch->name, ch->in_room->vnum );
      if( IS_NPC( ch ) )
         extract_char( ch, TRUE );
      else
         extract_char( ch, FALSE );
      return rCHAR_DIED;
   }

   /*
    * Do damage to PC for the hostile sector types - Samson 6-1-00 ( Where the hell does time go?!?!? ) 
    */
   if( ch->in_room->sector_type == SECT_LAVA )
   {
      set_char_color( AT_FIRE, ch );

      if( IS_IMMUNE( ch, RIS_FIRE ) )
      {
         send_to_char( "The lava beneath your feet burns hot, but does you no harm.\n\r", ch );
         retcode = damage( ch, ch, 0, TYPE_UNDEFINED );
         return retcode;
      }

      if( IS_RESIS( ch, RIS_FIRE ) && !IS_IMMUNE( ch, RIS_FIRE ) )
      {
         send_to_char( "The lava beneath your feet burns hot, but you are partially protected from it.\n\r", ch );
         retcode = damage( ch, ch, 10, TYPE_UNDEFINED );
         return retcode;
      }

      if( !IS_IMMUNE( ch, RIS_FIRE ) && !IS_RESIS( ch, RIS_FIRE ) )
      {
         send_to_char( "The lava beneath your feet burns you!!\n\r", ch );
         retcode = damage( ch, ch, 20, TYPE_UNDEFINED );
         return retcode;
      }
   }

   if( ch->in_room->sector_type == SECT_TUNDRA )
   {
      set_char_color( AT_WHITE, ch );

      if( IS_IMMUNE( ch, RIS_COLD ) )
      {
         send_to_char( "The air is freezing cold, but does you no harm.\n\r", ch );
         retcode = damage( ch, ch, 0, TYPE_UNDEFINED );
         return retcode;
      }

      if( IS_RESIS( ch, RIS_COLD ) && !IS_IMMUNE( ch, RIS_COLD ) )
      {
         send_to_char( "The icy chill bites deep, but you are partially protected from it.\n\r", ch );
         retcode = damage( ch, ch, 10, TYPE_UNDEFINED );
         return retcode;
      }

      if( !IS_IMMUNE( ch, RIS_COLD ) && !IS_RESIS( ch, RIS_COLD ) )
      {
         send_to_char( "The icy chill of the tundra bites deep!!\n\r", ch );
         retcode = damage( ch, ch, 20, TYPE_UNDEFINED );
         return retcode;
      }
   }

   if( ch->in_room->first_content )
      retcode = check_room_for_traps( ch, TRAP_ENTER_ROOM );
   if( retcode != rNONE )
      return retcode;

   if( char_died( ch ) )
      return retcode;

   mprog_entry_trigger( ch );
   if( char_died( ch ) )
      return retcode;

   rprog_enter_trigger( ch );
   if( char_died( ch ) )
      return retcode;

   mprog_greet_trigger( ch );
   if( char_died( ch ) )
      return retcode;

   oprog_greet_trigger( ch );
   if( char_died( ch ) )
      return retcode;

   if( !will_fall( ch, fall ) && fall > 0 )
   {
      if( !IS_AFFECTED( ch, AFF_FLOATING ) || ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLOATING ) ) )
      {
         send_to_char( "&[hurt]OUCH! You hit the ground!\n\r", ch );
         WAIT_STATE( ch, 20 );
         retcode = damage( ch, ch, 20 * fall, TYPE_UNDEFINED );
      }
      else
         send_to_char( "&[magic]You lightly float down to the ground.\n\r", ch );
   }
   return retcode;
}

CMDF do_north( CHAR_DATA * ch, char *argument )
{
   move_char( ch, get_exit( ch->in_room, DIR_NORTH ), 0, DIR_NORTH, FALSE );
   return;
}

CMDF do_east( CHAR_DATA * ch, char *argument )
{
   move_char( ch, get_exit( ch->in_room, DIR_EAST ), 0, DIR_EAST, FALSE );
   return;
}

CMDF do_south( CHAR_DATA * ch, char *argument )
{
   move_char( ch, get_exit( ch->in_room, DIR_SOUTH ), 0, DIR_SOUTH, FALSE );
   return;
}

CMDF do_west( CHAR_DATA * ch, char *argument )
{
   move_char( ch, get_exit( ch->in_room, DIR_WEST ), 0, DIR_WEST, FALSE );
   return;
}

CMDF do_up( CHAR_DATA * ch, char *argument )
{
   move_char( ch, get_exit( ch->in_room, DIR_UP ), 0, DIR_UP, FALSE );
   return;
}

CMDF do_down( CHAR_DATA * ch, char *argument )
{
   move_char( ch, get_exit( ch->in_room, DIR_DOWN ), 0, DIR_DOWN, FALSE );
   return;
}

CMDF do_northeast( CHAR_DATA * ch, char *argument )
{
   move_char( ch, get_exit( ch->in_room, DIR_NORTHEAST ), 0, DIR_NORTHEAST, FALSE );
   return;
}

CMDF do_northwest( CHAR_DATA * ch, char *argument )
{
   move_char( ch, get_exit( ch->in_room, DIR_NORTHWEST ), 0, DIR_NORTHWEST, FALSE );
   return;
}

CMDF do_southeast( CHAR_DATA * ch, char *argument )
{
   move_char( ch, get_exit( ch->in_room, DIR_SOUTHEAST ), 0, DIR_SOUTHEAST, FALSE );
   return;
}

CMDF do_southwest( CHAR_DATA * ch, char *argument )
{
   move_char( ch, get_exit( ch->in_room, DIR_SOUTHWEST ), 0, DIR_SOUTHWEST, FALSE );
   return;
}

EXIT_DATA *find_door( CHAR_DATA * ch, char *arg, bool quiet )
{
   EXIT_DATA *pexit;
   int door;

   if( !arg || arg[0] == '\0' || !str_cmp( arg, "" ) )
      return NULL;

   pexit = NULL;
   door = get_dirnum( arg );
   if( door < 0 || door > MAX_DIR )
   {
      for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
      {
         if( ( quiet || IS_EXIT_FLAG( pexit, EX_ISDOOR ) ) && pexit->keyword && nifty_is_name( arg, pexit->keyword ) )
            return pexit;
      }
      if( !quiet )
         act( AT_PLAIN, "You see no $T here.", ch, NULL, arg, TO_CHAR );
      return NULL;
   }

   if( !( pexit = get_exit( ch->in_room, door ) ) )
   {
      if( !quiet )
         act( AT_PLAIN, "You see no $T here.", ch, NULL, arg, TO_CHAR );
      return NULL;
   }

   if( quiet )
      return pexit;

   if( IS_EXIT_FLAG( pexit, EX_SECRET ) )
   {
      act( AT_PLAIN, "You see no $T here.", ch, NULL, arg, TO_CHAR );
      return NULL;
   }

   if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
   {
      send_to_char( "You can't do that.\n\r", ch );
      return NULL;
   }

   return pexit;
}

void set_bexit_flag( EXIT_DATA * pexit, int flag )
{
   EXIT_DATA *pexit_rev;

   SET_EXIT_FLAG( pexit, flag );
   if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev != pexit )
      SET_EXIT_FLAG( pexit_rev, flag );
}

void remove_bexit_flag( EXIT_DATA * pexit, int flag )
{
   EXIT_DATA *pexit_rev;

   REMOVE_EXIT_FLAG( pexit, flag );
   if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev != pexit )
      REMOVE_EXIT_FLAG( pexit_rev, flag );
}

CMDF do_open( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj;
   EXIT_DATA *pexit;
   int door;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Open what?\n\r", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, TRUE ) ) != NULL )
   {
      /*
       * 'open door' 
       */
      EXIT_DATA *pexit_rev;

      /*
       * Added by Tarl 11 July 2002 so mobs don't attempt to open doors to nomob rooms. 
       */
      if( IS_NPC( ch ) )
      {
         if( IS_ROOM_FLAG( pexit->to_room, ROOM_NO_MOB ) )
            return;
      }
      if( IS_EXIT_FLAG( pexit, EX_SECRET ) && pexit->keyword && !nifty_is_name( argument, pexit->keyword ) )
      {
         ch_printf( ch, "You see no %s here.\n\r", argument );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         send_to_char( "It's already open.\n\r", ch );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_LOCKED ) && IS_EXIT_FLAG( pexit, EX_BOLTED ) )
      {
         send_to_char( "The bolts locked.\n\r", ch );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_BOLTED ) )
      {
         send_to_char( "It's bolted.\n\r", ch );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         send_to_char( "It's locked.\n\r", ch );
         return;
      }

      if( !IS_EXIT_FLAG( pexit, EX_SECRET ) || ( pexit->keyword && nifty_is_name( argument, pexit->keyword ) ) )
      {
         act( AT_ACTION, "$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM );
         act( AT_ACTION, "You open the $d.", ch, NULL, pexit->keyword, TO_CHAR );
         if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
         {
            CHAR_DATA *rch;

            for( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
               act( AT_ACTION, "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR );
         }
         remove_bexit_flag( pexit, EX_CLOSED );
         if( ( door = pexit->vdir ) >= 0 && door < 10 )
            check_room_for_traps( ch, trap_door[door] );
         return;
      }
   }

   if( ( obj = get_obj_here( ch, argument ) ) != NULL )
   {
      /*
       * 'open object' 
       */
      if( obj->item_type != ITEM_CONTAINER )
      {
         ch_printf( ch, "%s is not a container.\n\r", capitalize( obj->short_descr ) );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         ch_printf( ch, "%s is already open.\n\r", capitalize( obj->short_descr ) );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSEABLE ) )
      {
         ch_printf( ch, "%s cannot be opened or closed.\n\r", capitalize( obj->short_descr ) );
         return;
      }
      if( IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         ch_printf( ch, "%s is locked.\n\r", capitalize( obj->short_descr ) );
         return;
      }

      REMOVE_BIT( obj->value[1], CONT_CLOSED );
      act( AT_ACTION, "You open $p.", ch, obj, NULL, TO_CHAR );
      act( AT_ACTION, "$n opens $p.", ch, obj, NULL, TO_ROOM );
      check_for_trap( ch, obj, TRAP_OPEN );
      return;
   }
   ch_printf( ch, "You see no %s here.\n\r", argument );
   return;
}

CMDF do_close( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj;
   EXIT_DATA *pexit;
   int door;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Close what?\n\r", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, TRUE ) ) != NULL )
   {
      /*
       * 'close door' 
       */
      EXIT_DATA *pexit_rev;

      if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         send_to_char( "It's already closed.\n\r", ch );
         return;
      }
      act( AT_ACTION, "$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM );
      act( AT_ACTION, "You close the $d.", ch, NULL, pexit->keyword, TO_CHAR );

      /*
       * close the other side 
       */
      if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
      {
         CHAR_DATA *rch;

         SET_EXIT_FLAG( pexit_rev, EX_CLOSED );
         for( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
            act( AT_ACTION, "The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR );
      }
      set_bexit_flag( pexit, EX_CLOSED );
      if( ( door = pexit->vdir ) >= 0 && door < 10 )
         check_room_for_traps( ch, trap_door[door] );
      return;
   }

   if( ( obj = get_obj_here( ch, argument ) ) != NULL )
   {
      /*
       * 'close object' 
       */
      if( obj->item_type != ITEM_CONTAINER )
      {
         ch_printf( ch, "%s is not a container.\n\r", capitalize( obj->short_descr ) );
         return;
      }
      if( IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         ch_printf( ch, "%s is already closed.\n\r", capitalize( obj->short_descr ) );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSEABLE ) )
      {
         ch_printf( ch, "%s cannot be opened or closed.\n\r", capitalize( obj->short_descr ) );
         return;
      }
      SET_BIT( obj->value[1], CONT_CLOSED );
      act( AT_ACTION, "You close $p.", ch, obj, NULL, TO_CHAR );
      act( AT_ACTION, "$n closes $p.", ch, obj, NULL, TO_ROOM );
      check_for_trap( ch, obj, TRAP_CLOSE );
      return;
   }
   ch_printf( ch, "You see no %s here.\n\r", argument );
   return;
}


/*
 * Keyring support added by Thoric
 * Idea suggested by Onyx <MtRicmer@worldnet.att.net> of Eldarion
 *
 * New: returns pointer to key/NULL instead of TRUE/FALSE
 *
 * If you want a feature like having immortals always have a key... you'll
 * need to code in a generic key, and make sure extract_obj doesn't extract it
 */
OBJ_DATA *has_key( CHAR_DATA * ch, int key )
{
   OBJ_DATA *obj, *obj2;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->pIndexData->vnum == key || ( obj->item_type == ITEM_KEY && obj->value[0] == key ) )
         return obj;
      else if( obj->item_type == ITEM_KEYRING )
         for( obj2 = obj->first_content; obj2; obj2 = obj2->next_content )
            if( obj2->pIndexData->vnum == key || obj2->value[0] == key )
               return obj2;
   }

   return NULL;
}

CMDF do_lock( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj, *key;
   EXIT_DATA *pexit;
   int count;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Lock what?\n\r", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, TRUE ) ) != NULL )
   {
      /*
       * 'lock door' 
       */
      if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         send_to_char( "It's not closed.\n\r", ch );
         return;
      }
      if( pexit->key < 0 )
      {
         send_to_char( "It can't be locked.\n\r", ch );
         return;
      }
      if( ( key = has_key( ch, pexit->key ) ) == NULL )
      {
         send_to_char( "You lack the key.\n\r", ch );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         send_to_char( "It's already locked.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_SECRET ) || ( pexit->keyword && nifty_is_name( argument, pexit->keyword ) ) )
      {
         send_to_char( "*Click*\n\r", ch );
         count = key->count;
         key->count = 1;
         act( AT_ACTION, "$n locks the $d with $p.", ch, key, pexit->keyword, TO_ROOM );
         key->count = count;
         set_bexit_flag( pexit, EX_LOCKED );
         return;
      }
   }
   if( ( obj = get_obj_here( ch, argument ) ) != NULL )
   {
      /*
       * 'lock object' 
       */
      if( obj->item_type != ITEM_CONTAINER )
      {
         send_to_char( "That's not a container.\n\r", ch );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         send_to_char( "It's not closed.\n\r", ch );
         return;
      }
      if( obj->value[2] < 0 )
      {
         send_to_char( "It can't be locked.\n\r", ch );
         return;
      }
      if( ( key = has_key( ch, obj->value[2] ) ) == NULL )
      {
         send_to_char( "You lack the key.\n\r", ch );
         return;
      }
      if( IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         send_to_char( "It's already locked.\n\r", ch );
         return;
      }
      SET_BIT( obj->value[1], CONT_LOCKED );
      send_to_char( "*Click*\n\r", ch );
      count = key->count;
      key->count = 1;
      act( AT_ACTION, "$n locks $p with $P.", ch, obj, key, TO_ROOM );
      key->count = count;
      return;
   }
   ch_printf( ch, "You see no %s here.\n\r", argument );
   return;
}

CMDF do_unlock( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj, *key;
   EXIT_DATA *pexit;
   int count;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Unlock what?\n\r", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, TRUE ) ) != NULL )
   {
      /*
       * 'unlock door' 
       */
      if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         send_to_char( "It's not closed.\n\r", ch );
         return;
      }
      if( pexit->key < 0 )
      {
         send_to_char( "It can't be unlocked.\n\r", ch );
         return;
      }
      if( ( key = has_key( ch, pexit->key ) ) == NULL )
      {
         send_to_char( "You lack the key.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         send_to_char( "It's already unlocked.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_SECRET ) || ( pexit->keyword && nifty_is_name( argument, pexit->keyword ) ) )
      {
         send_to_char( "*Click*\n\r", ch );
         count = key->count;
         key->count = 1;
         act( AT_ACTION, "$n unlocks the $d with $p.", ch, key, pexit->keyword, TO_ROOM );
         key->count = count;
         if( IS_EXIT_FLAG( pexit, EX_EATKEY ) )
         {
            separate_obj( key );
            extract_obj( key );
         }
         remove_bexit_flag( pexit, EX_LOCKED );
         return;
      }
   }

   if( ( obj = get_obj_here( ch, argument ) ) != NULL )
   {
      /*
       * 'unlock object' 
       */
      if( obj->item_type != ITEM_CONTAINER )
      {
         send_to_char( "That's not a container.\n\r", ch );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         send_to_char( "It's not closed.\n\r", ch );
         return;
      }
      if( obj->value[2] < 0 )
      {
         send_to_char( "It can't be unlocked.\n\r", ch );
         return;
      }
      if( ( key = has_key( ch, obj->value[2] ) ) == NULL )
      {
         send_to_char( "You lack the key.\n\r", ch );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         send_to_char( "It's already unlocked.\n\r", ch );
         return;
      }
      REMOVE_BIT( obj->value[1], CONT_LOCKED );
      send_to_char( "*Click*\n\r", ch );
      count = key->count;
      key->count = 1;
      act( AT_ACTION, "$n unlocks $p with $P.", ch, obj, key, TO_ROOM );
      key->count = count;
      if( IS_SET( obj->value[1], CONT_EATKEY ) )
      {
         separate_obj( key );
         extract_obj( key );
      }
      return;
   }
   ch_printf( ch, "You see no %s here.\n\r", argument );
   return;
}

CMDF do_bashdoor( CHAR_DATA * ch, char *argument )
{
   EXIT_DATA *pexit;

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_bashdoor]->skill_level[ch->Class] )
   {
      send_to_char( "You're not enough of a warrior to bash doors!\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Bash what?\n\r", ch );
      return;
   }

   if( ch->fighting )
   {
      send_to_char( "You can't break off your fight.\n\r", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, FALSE ) ) != NULL )
   {
      ROOM_INDEX_DATA *to_room;
      EXIT_DATA *pexit_rev;
      int bashchance;
      char *keyword;

      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         send_to_char( "Calm down.  It is already open.\n\r", ch );
         return;
      }
      WAIT_STATE( ch, skill_table[gsn_bashdoor]->beats );

      if( IS_EXIT_FLAG( pexit, EX_SECRET ) )
         keyword = "wall";
      else
         keyword = pexit->keyword;
      if( !IS_NPC( ch ) )
         bashchance = LEARNED( ch, gsn_bashdoor ) / 2;
      else
         bashchance = 90;
      if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
         bashchance /= 3;

      if( !IS_EXIT_FLAG( pexit, EX_BASHPROOF )
          && ch->move >= 15 && number_percent(  ) < ( bashchance + 4 * ( get_curr_str( ch ) - 19 ) ) )
      {
         REMOVE_EXIT_FLAG( pexit, EX_CLOSED );
         if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
            REMOVE_EXIT_FLAG( pexit, EX_LOCKED );
         SET_EXIT_FLAG( pexit, EX_BASHED );

         act( AT_SKILL, "Crash!  You bashed open the $d!", ch, NULL, keyword, TO_CHAR );
         act( AT_SKILL, "$n bashes open the $d!", ch, NULL, keyword, TO_ROOM );

         if( ( to_room = pexit->to_room ) != NULL && ( pexit_rev = pexit->rexit ) != NULL
             && pexit_rev->to_room == ch->in_room )
         {
            CHAR_DATA *rch;

            REMOVE_EXIT_FLAG( pexit_rev, EX_CLOSED );
            if( IS_EXIT_FLAG( pexit_rev, EX_LOCKED ) )
               REMOVE_EXIT_FLAG( pexit_rev, EX_LOCKED );
            SET_EXIT_FLAG( pexit_rev, EX_BASHED );

            for( rch = to_room->first_person; rch; rch = rch->next_in_room )
               act( AT_SKILL, "The $d crashes open!", rch, NULL, pexit_rev->keyword, TO_CHAR );
         }
         damage( ch, ch, ( ch->max_hit / 20 ), gsn_bashdoor );
      }
      else
      {
         act( AT_SKILL, "WHAAAAM!!!  You bash against the $d, but it doesn't budge.", ch, NULL, keyword, TO_CHAR );
         act( AT_SKILL, "WHAAAAM!!!  $n bashes against the $d, but it holds strong.", ch, NULL, keyword, TO_ROOM );
         learn_from_failure( ch, gsn_bashdoor );
         damage( ch, ch, ( ch->max_hit / 20 ) + 10, gsn_bashdoor );
      }
   }
   else
   {
      act( AT_SKILL, "WHAAAAM!!!  You bash against the wall, but it doesn't budge.", ch, NULL, NULL, TO_CHAR );
      act( AT_SKILL, "WHAAAAM!!!  $n bashes against the wall, but it holds strong.", ch, NULL, NULL, TO_ROOM );
      learn_from_failure( ch, gsn_bashdoor );
      damage( ch, ch, ( ch->max_hit / 20 ) + 10, gsn_bashdoor );
   }
   return;
}

/* Orginal furniture taken from Russ Walsh's Rot copyright 1996-1997
   Furniture 1.0 is provided by Xerves
   Allows you to stand/sit/rest/sleep on/at/in objects -- Xerves */
CMDF do_stand( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj = NULL;
   int aon = 0;

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      send_to_char( "Maybe you should finish this fight first?\n\r", ch );
      return;
   }

   if( ch->position == POS_MOUNTED )
   {
      send_to_char( "Try dismounting first.\n\r", ch );
      return;
   }

   /*
    * okay, now that we know we can sit, find an object to sit on 
    */
   if( argument && argument[0] != '\0' )
   {
      obj = get_obj_list( ch, argument, ch->in_room->first_content );
      if( obj == NULL )
      {
         send_to_char( "You don't see that here.\n\r", ch );
         return;
      }
      if( obj->item_type != ITEM_FURNITURE )
      {
         send_to_char( "It has to be furniture silly.\n\r", ch );
         return;
      }
      if( !IS_SET( obj->value[2], STAND_ON ) && !IS_SET( obj->value[2], STAND_IN ) && !IS_SET( obj->value[2], STAND_AT ) )
      {
         send_to_char( "You can't stand on that.\n\r", ch );
         return;
      }
      if( ch->on == obj )
         aon = 1;
      else
         ch->on = obj;
   }

   switch ( ch->position )
   {
      case POS_SLEEPING:
         if( IS_AFFECTED( ch, AFF_SLEEP ) )
         {
            send_to_char( "You can't wake up!\n\r", ch );
            return;
         }

         if( obj == NULL )
         {
            send_to_char( "You wake and stand up.\n\r", ch );
            act( AT_ACTION, "$n wakes and stands up.", ch, NULL, NULL, TO_ROOM );
            ch->on = NULL;
         }
         else if( IS_SET( obj->value[2], STAND_AT ) )
         {
            act( AT_ACTION, "You wake and stand at $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n wakes and stands at $p.", ch, obj, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], STAND_ON ) )
         {
            act( AT_ACTION, "You wake and stand on $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n wakes and stands on $p.", ch, obj, NULL, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You wake and stand in $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n wakes and stands in $p.", ch, obj, NULL, TO_ROOM );
         }
         ch->position = POS_STANDING;
         interpret( ch, "look" );
         break;

      case POS_RESTING:
      case POS_SITTING:
         if( obj == NULL )
         {
            send_to_char( "You stand up.\n\r", ch );
            act( AT_ACTION, "$n stands up.", ch, NULL, NULL, TO_ROOM );
            ch->on = NULL;
         }
         else if( IS_SET( obj->value[2], STAND_AT ) )
         {
            act( AT_ACTION, "You stand at $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n stands at $p.", ch, obj, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], STAND_ON ) )
         {
            act( AT_ACTION, "You stand on $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n stands on $p.", ch, obj, NULL, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You stand in $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n stands on $p.", ch, obj, NULL, TO_ROOM );
         }
         ch->position = POS_STANDING;
         break;

      case POS_STANDING:
         if( obj != NULL && aon != 1 )
         {
            if( IS_SET( obj->value[2], STAND_AT ) )
            {
               act( AT_ACTION, "You stand at $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n stands at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if( IS_SET( obj->value[2], STAND_ON ) )
            {
               act( AT_ACTION, "You stand on $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n stands on $p.", ch, obj, NULL, TO_ROOM );
            }
            else
            {
               act( AT_ACTION, "You stand in $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n stands on $p.", ch, obj, NULL, TO_ROOM );
            }
         }
         else if( aon == 1 )
            act( AT_ACTION, "You are already using $p for furniture.", ch, obj, NULL, TO_CHAR );
         else if( ch->on != NULL && obj == NULL )
         {
            act( AT_ACTION, "You hop off of $p and stand on the ground.", ch, ch->on, NULL, TO_CHAR );
            act( AT_ACTION, "$n hops off of $p and stands on the ground.", ch, ch->on, NULL, TO_ROOM );
            ch->on = NULL;
         }
         else
            send_to_char( "You are already standing.\n\r", ch );
         break;
   }
   return;
}

CMDF do_sit( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj = NULL;
   int aon = 0;

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      send_to_char( "Maybe you should finish this fight first?\n\r", ch );
      return;
   }

   if( ch->position == POS_MOUNTED )
   {
      send_to_char( "You are already sitting - on your mount.\n\r", ch );
      return;
   }

   /*
    * okay, now that we know we can sit, find an object to sit on 
    */
   if( argument && argument[0] != '\0' )
   {
      obj = get_obj_list( ch, argument, ch->in_room->first_content );
      if( obj == NULL )
      {
         send_to_char( "You don't see that here.\n\r", ch );
         return;
      }
      if( obj->item_type != ITEM_FURNITURE )
      {
         send_to_char( "It has to be furniture silly.\n\r", ch );
         return;
      }
      if( !IS_SET( obj->value[2], SIT_ON ) && !IS_SET( obj->value[2], SIT_IN ) && !IS_SET( obj->value[2], SIT_AT ) )
      {
         send_to_char( "You can't sit on that.\n\r", ch );
         return;
      }
      if( ch->on == obj )
         aon = 1;
      else
         ch->on = obj;
   }

   switch ( ch->position )
   {
      case POS_SLEEPING:
         if( obj == NULL )
         {
            send_to_char( "You wake and sit up.\n\r", ch );
            act( AT_ACTION, "$n wakes and sits up.", ch, NULL, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SIT_AT ) )
         {
            act( AT_ACTION, "You wake up and sit at $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SIT_ON ) )
         {
            act( AT_ACTION, "You wake and sit on $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You wake and sit in $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n wakes and sits in $p.", ch, obj, NULL, TO_ROOM );
         }
         ch->position = POS_SITTING;
         break;
      case POS_RESTING:
         if( obj == NULL )
            send_to_char( "You stop resting.\n\r", ch );
         else if( IS_SET( obj->value[2], SIT_AT ) )
         {
            act( AT_ACTION, "You sit at $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sits at $p.", ch, obj, NULL, TO_ROOM );
         }

         else if( IS_SET( obj->value[2], SIT_ON ) )
         {
            act( AT_ACTION, "You sit on $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sits on $p.", ch, obj, NULL, TO_ROOM );
         }
         ch->position = POS_SITTING;
         break;
      case POS_SITTING:
         if( obj != NULL && aon != 1 )
         {
            if( IS_SET( obj->value[2], SIT_AT ) )
            {
               act( AT_ACTION, "You sit at $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n sits at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if( IS_SET( obj->value[2], STAND_ON ) )
            {
               act( AT_ACTION, "You sit on $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n sits on $p.", ch, obj, NULL, TO_ROOM );
            }
            else
            {
               act( AT_ACTION, "You sit in $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n sits on $p.", ch, obj, NULL, TO_ROOM );
            }
         }
         else if( aon == 1 )
            act( AT_ACTION, "You are already using $p for furniture.", ch, obj, NULL, TO_CHAR );
         else if( ch->on != NULL && obj == NULL )
         {
            act( AT_ACTION, "You hop off of $p and sit on the ground.", ch, ch->on, NULL, TO_CHAR );
            act( AT_ACTION, "$n hops off of $p and sits on the ground.", ch, ch->on, NULL, TO_ROOM );
            ch->on = NULL;
         }
         else
            send_to_char( "You are already sitting.\n\r", ch );
         break;
      case POS_STANDING:
         if( obj == NULL )
         {
            send_to_char( "You sit down.\n\r", ch );
            act( AT_ACTION, "$n sits down on the ground.", ch, NULL, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SIT_AT ) )
         {
            act( AT_ACTION, "You sit down at $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sits down at $p.", ch, obj, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SIT_ON ) )
         {
            act( AT_ACTION, "You sit on $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sits on $p.", ch, obj, NULL, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You sit down in $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sits down in $p.", ch, obj, NULL, TO_ROOM );
         }
         ch->position = POS_SITTING;
         break;
   }
   return;
}

CMDF do_rest( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj = NULL;
   int aon = 0;

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      send_to_char( "Maybe you should finish this fight first?\n\r", ch );
      return;
   }

   if( ch->position == POS_MOUNTED )
   {
      send_to_char( "You are already sitting - on your mount.\n\r", ch );
      return;
   }

   /*
    * okay, now that we know we can sit, find an object to sit on 
    */
   if( argument && argument[0] != '\0' )
   {
      obj = get_obj_list( ch, argument, ch->in_room->first_content );
      if( obj == NULL )
      {
         send_to_char( "You don't see that here.\n\r", ch );
         return;
      }
      if( obj->item_type != ITEM_FURNITURE )
      {
         send_to_char( "It has to be furniture silly.\n\r", ch );
         return;
      }
      if( !IS_SET( obj->value[2], REST_ON ) && !IS_SET( obj->value[2], REST_IN ) && !IS_SET( obj->value[2], REST_AT ) )
      {
         send_to_char( "You can't rest on that.\n\r", ch );
         return;
      }
      if( ch->on == obj )
         aon = 1;
      else
         ch->on = obj;
   }
   switch ( ch->position )
   {
      case POS_SLEEPING:
         if( obj == NULL )
         {
            send_to_char( "You wake up and start resting.\n\r", ch );
            act( AT_ACTION, "$n wakes up and starts resting.", ch, NULL, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], REST_AT ) )
         {
            act( AT_ACTION, "You wake up and rest at $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n wakes up and rests at $p.", ch, obj, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], REST_ON ) )
         {
            act( AT_ACTION, "You wake up and rest on $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n wakes up and rests on $p.", ch, obj, NULL, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You wake up and rest in $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n wakes up and rests in $p.", ch, obj, NULL, TO_ROOM );
         }
         ch->position = POS_RESTING;
         break;

      case POS_RESTING:
         if( obj != NULL && aon != 1 )
         {

            if( IS_SET( obj->value[2], REST_AT ) )
            {
               act( AT_ACTION, "You rest at $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n rests at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if( IS_SET( obj->value[2], REST_ON ) )
            {
               act( AT_ACTION, "You rest on $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n rests on $p.", ch, obj, NULL, TO_ROOM );
            }
            else
            {
               act( AT_ACTION, "You rest in $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n rests on $p.", ch, obj, NULL, TO_ROOM );
            }
         }
         else if( aon == 1 )
            act( AT_ACTION, "You are already using $p for furniture.", ch, obj, NULL, TO_CHAR );
         else if( ch->on != NULL && obj == NULL )
         {
            act( AT_ACTION, "You hop off of $p and start resting on the ground.", ch, ch->on, NULL, TO_CHAR );
            act( AT_ACTION, "$n hops off of $p and starts to rest on the ground.", ch, ch->on, NULL, TO_ROOM );
            ch->on = NULL;
         }
         else
            send_to_char( "You are already resting.\n\r", ch );
         break;

      case POS_STANDING:
         if( obj == NULL )
         {
            send_to_char( "You rest.\n\r", ch );
            act( AT_ACTION, "$n sits down and rests.", ch, NULL, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], REST_AT ) )
         {
            act( AT_ACTION, "You sit down at $p and rest.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sits down at $p and rests.", ch, obj, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], REST_ON ) )
         {
            act( AT_ACTION, "You sit on $p and rest.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sits on $p and rests.", ch, obj, NULL, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You rest in $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n rests in $p.", ch, obj, NULL, TO_ROOM );
         }
         ch->position = POS_RESTING;
         break;

      case POS_SITTING:
         if( obj == NULL )
         {
            send_to_char( "You rest.\n\r", ch );
            act( AT_ACTION, "$n rests.", ch, NULL, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], REST_AT ) )
         {
            act( AT_ACTION, "You rest at $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n rests at $p.", ch, obj, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], REST_ON ) )
         {
            act( AT_ACTION, "You rest on $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n rests on $p.", ch, obj, NULL, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You rest in $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n rests in $p.", ch, obj, NULL, TO_ROOM );
         }
         ch->position = POS_RESTING;
         break;
   }

   rprog_rest_trigger( ch );
   return;
}

CMDF do_sleep( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj = NULL;
   int aon = 0;

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      send_to_char( "Maybe you should finish this fight first?\n\r", ch );
      return;
   }

   if( ch->position == POS_MOUNTED )
   {
      send_to_char( "If you wish to go to sleep, get off of your mount first.\n\r", ch );
      return;
   }

   /*
    * okay, now that we know we can sit, find an object to sit on 
    */
   if( argument && argument[0] != '\0' )
   {
      obj = get_obj_list( ch, argument, ch->in_room->first_content );
      if( obj == NULL )
      {
         send_to_char( "You don't see that here.\n\r", ch );
         return;
      }
      if( obj->item_type != ITEM_FURNITURE )
      {
         send_to_char( "It has to be furniture silly.\n\r", ch );
         return;
      }
      if( !IS_SET( obj->value[2], SLEEP_ON ) && !IS_SET( obj->value[2], SLEEP_IN ) && !IS_SET( obj->value[2], SLEEP_AT ) )
      {
         send_to_char( "You can't sleep on that.\n\r", ch );
         return;
      }
      if( ch->on == obj )
         aon = 1;
      else
         ch->on = obj;
   }
   switch ( ch->position )
   {
      case POS_SLEEPING:
         if( obj != NULL && aon != 1 )
         {

            if( IS_SET( obj->value[2], SLEEP_AT ) )
            {
               act( AT_ACTION, "You sleep at $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n sleeps at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if( IS_SET( obj->value[2], SLEEP_ON ) )
            {
               act( AT_ACTION, "You sleep on $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n sleeps on $p.", ch, obj, NULL, TO_ROOM );
            }
            else
            {
               act( AT_ACTION, "You sleep in $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n sleeps on $p.", ch, obj, NULL, TO_ROOM );
            }
         }
         else if( aon == 1 )
            act( AT_ACTION, "You are already using $p for furniture.", ch, obj, NULL, TO_CHAR );
         else if( ch->on != NULL && obj == NULL )
         {
            act( AT_ACTION, "You hop off of $p and try to sleep on the ground.", ch, ch->on, NULL, TO_CHAR );
            act( AT_ACTION, "$n hops off of $p and falls quickly asleep on the ground.", ch, ch->on, NULL, TO_ROOM );
            ch->on = NULL;
         }
         else
            send_to_char( "You are already sleeping.\n\r", ch );
         break;
      case POS_RESTING:
         if( obj == NULL )
         {
            send_to_char( "You lean your head back more and go to sleep.\n\r", ch );
            act( AT_ACTION, "$n lies back and falls asleep on the ground.", ch, NULL, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SLEEP_AT ) )
         {
            act( AT_ACTION, "You sleep at $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sleeps at $p.", ch, obj, NULL, TO_ROOM );
         }

         else if( IS_SET( obj->value[2], SLEEP_ON ) )
         {
            act( AT_ACTION, "You sleep on $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sleeps on $p.", ch, obj, NULL, TO_ROOM );
         }

         else
         {
            act( AT_ACTION, "You sleep in $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sleeps in $p.", ch, obj, NULL, TO_ROOM );
         }
         ch->position = POS_SLEEPING;
         break;

      case POS_SITTING:
         if( obj == NULL )
         {
            send_to_char( "You lay down and go to sleep.\n\r", ch );
            act( AT_ACTION, "$n lies back and falls asleep on the ground.", ch, NULL, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SLEEP_AT ) )
         {
            act( AT_ACTION, "You sleep at $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sleeps at $p.", ch, obj, NULL, TO_ROOM );
         }

         else if( IS_SET( obj->value[2], SLEEP_ON ) )
         {
            act( AT_ACTION, "You sleep on $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sleeps on $p.", ch, obj, NULL, TO_ROOM );
         }

         else
         {
            act( AT_ACTION, "You sleep in $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sleeps in $p.", ch, obj, NULL, TO_ROOM );
         }
         ch->position = POS_SLEEPING;

         break;
      case POS_STANDING:
         if( obj == NULL )
         {
            send_to_char( "You drop down and fall asleep on the ground.\n\r", ch );
            act( AT_ACTION, "$n drops down and falls asleep on the ground.", ch, NULL, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SLEEP_AT ) )
         {
            act( AT_ACTION, "You sleep at $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sleeps at $p.", ch, obj, NULL, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SLEEP_ON ) )
         {
            act( AT_ACTION, "You sleep on $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sleeps on $p.", ch, obj, NULL, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You sleep down in $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n sleeps down in $p.", ch, obj, NULL, TO_ROOM );
         }
         ch->position = POS_SLEEPING;
         break;
   }

   rprog_sleep_trigger( ch );
   return;
}

CMDF do_wake( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( !argument || argument[0] == '\0' )
   {
      do_stand( ch, "" );
      return;
   }

   if( !IS_AWAKE( ch ) )
   {
      send_to_char( "You are asleep yourself!\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, argument ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( IS_AWAKE( victim ) )
   {
      act( AT_PLAIN, "$N is already awake.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( IS_AFFECTED( victim, AFF_SLEEP ) || victim->position < POS_SLEEPING )
   {
      act( AT_PLAIN, "You can't seem to wake $M!", ch, NULL, victim, TO_CHAR );
      return;
   }
   act( AT_ACTION, "You wake $M.", ch, NULL, victim, TO_CHAR );
   victim->position = POS_STANDING;
   act( AT_ACTION, "$n wakes you.", ch, NULL, victim, TO_VICT );
   return;
}

/*
 * teleport a character to another room
 */
void teleportch( CHAR_DATA * ch, ROOM_INDEX_DATA * room, bool show )
{
   if( room_is_private( room ) )
      return;

   if( IS_AFFECTED( ch, AFF_FLYING ) && IS_ROOM_FLAG( ch->in_room, ROOM_TELENOFLY ) )
      return;

   act( AT_ACTION, "$n disappears suddenly!", ch, NULL, NULL, TO_ROOM );
   char_from_room( ch );
   if( !char_to_room( ch, room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   act( AT_ACTION, "$n arrives suddenly!", ch, NULL, NULL, TO_ROOM );
   if( show )
      interpret( ch, "look" );
   if( IS_ROOM_FLAG( ch->in_room, ROOM_DEATH ) && !IS_IMMORTAL( ch ) )
   {
      send_to_char( "&[dead]Oopsie... you're dead!\n\r", ch );
      log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "%s hit a DEATH TRAP in room %d!", ch->name, ch->in_room->vnum );
      extract_char( ch, FALSE );
   }
}

void teleport( CHAR_DATA * ch, int room, int flags )
{
   CHAR_DATA *nch, *nch_next;
   ROOM_INDEX_DATA *start = ch->in_room, *dest;
   bool show;

   dest = get_room_index( room );
   if( !dest )
   {
      bug( "teleport: bad room vnum %d", room );
      return;
   }

   if( IS_SET( flags, TELE_SHOWDESC ) )
      show = TRUE;
   else
      show = FALSE;
   if( !IS_SET( flags, TELE_TRANSALL ) )
   {
      teleportch( ch, dest, show );
      return;
   }

   /*
    * teleport everybody in the room 
    */
   for( nch = start->first_person; nch; nch = nch_next )
   {
      nch_next = nch->next_in_room;
      teleportch( nch, dest, show );
   }

   /*
    * teleport the objects on the ground too 
    */
   if( IS_SET( flags, TELE_TRANSALLPLUS ) )
   {
      OBJ_DATA *obj, *obj_next;

      for( obj = start->first_content; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         obj_from_room( obj );
         obj_to_room( obj, dest, NULL );
      }
   }
}

/*
 * "Climb" in a certain direction.				-Thoric
 */
CMDF do_climb( CHAR_DATA * ch, char *argument )
{
   EXIT_DATA *pexit;
   bool found;

   found = FALSE;
   if( argument[0] == '\0' )
   {
      for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
         if( IS_EXIT_FLAG( pexit, EX_xCLIMB ) )
         {
            move_char( ch, pexit, 0, pexit->vdir, FALSE );
            return;
         }
      send_to_char( "You cannot climb here.\n\r", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, TRUE ) ) != NULL && IS_EXIT_FLAG( pexit, EX_xCLIMB ) )
   {
      move_char( ch, pexit, 0, pexit->vdir, FALSE );
      return;
   }
   send_to_char( "You cannot climb there.\n\r", ch );
   return;
}

/*
 * "enter" something (moves through an exit)			-Thoric
 */
CMDF do_enter( CHAR_DATA * ch, char *argument )
{
   EXIT_DATA *pexit;
   bool found;

   found = FALSE;
   if( argument[0] == '\0' )
   {
      for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
         if( IS_EXIT_FLAG( pexit, EX_xENTER ) )
         {
            if( IS_EXIT_FLAG( pexit, EX_PORTAL ) && !has_visited( ch, pexit->to_room->area ) )
            {
               send_to_char( "Magic from the portal repulses your attempt to enter!\n\r", ch );
               return;
            }
            move_char( ch, pexit, 0, DIR_SOMEWHERE, FALSE );
            return;
         }
      if( ch->in_room->sector_type != SECT_INDOORS && IS_OUTSIDE( ch ) )
         for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
            if( pexit->to_room && ( pexit->to_room->sector_type == SECT_INDOORS
                                    || IS_ROOM_FLAG( pexit->to_room, ROOM_INDOORS ) ) )
            {
               move_char( ch, pexit, 0, DIR_SOMEWHERE, FALSE );
               return;
            }
      send_to_char( "You cannot find an entrance here.\n\r", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, TRUE ) ) != NULL && IS_EXIT_FLAG( pexit, EX_xENTER ) )
   {
      if( IS_EXIT_FLAG( pexit, EX_PORTAL ) && !has_visited( ch, pexit->to_room->area ) )
      {
         send_to_char( "Magic from the portal repulses your attempt to enter!\n\r", ch );
         return;
      }
      move_char( ch, pexit, 0, DIR_SOMEWHERE, FALSE );
      return;
   }
   send_to_char( "You cannot enter that.\n\r", ch );
   return;
}

/*
 * Leave through an exit.					-Thoric
 */
CMDF do_leave( CHAR_DATA * ch, char *argument )
{
   EXIT_DATA *pexit;
   bool found;

   found = FALSE;
   if( argument[0] == '\0' )
   {
      for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
         if( IS_EXIT_FLAG( pexit, EX_xLEAVE ) )
         {
            if( IS_EXIT_FLAG( pexit, EX_PORTAL ) && !has_visited( ch, pexit->to_room->area ) )
            {
               send_to_char( "Magic from the portal repulses your attempt to leave!\n\r", ch );
               return;
            }
            move_char( ch, pexit, 0, DIR_SOMEWHERE, FALSE );
            return;
         }
      if( ch->in_room->sector_type == SECT_INDOORS || !IS_OUTSIDE( ch ) )
         for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
            if( pexit->to_room && pexit->to_room->sector_type != SECT_INDOORS
                && !IS_ROOM_FLAG( pexit->to_room, ROOM_INDOORS ) )
            {
               move_char( ch, pexit, 0, DIR_SOMEWHERE, FALSE );
               return;
            }
      send_to_char( "You cannot find an exit here.\n\r", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, TRUE ) ) != NULL && IS_EXIT_FLAG( pexit, EX_xLEAVE ) )
   {
      if( IS_EXIT_FLAG( pexit, EX_PORTAL ) && !has_visited( ch, pexit->to_room->area ) )
      {
         send_to_char( "Magic from the portal repulses your attempt to leave!\n\r", ch );
         return;
      }
      move_char( ch, pexit, 0, DIR_SOMEWHERE, FALSE );
      return;
   }
   send_to_char( "You cannot leave that way.\n\r", ch );
   return;
}

/*
 * Check to see if an exit in the room is pulling (or pushing) players around.
 * Some types may cause damage.					-Thoric
 *
 * People kept requesting currents (like SillyMUD has), so I went all out
 * and added the ability for an exit to have a "pull" or a "push" force
 * and to handle different types much beyond a simple water current.
 *
 * This check is called by violence_update().  I'm not sure if this is the
 * best way to do it, or if it should be handled by a special queue.
 *
 * Future additions to this code may include equipment being blown away in
 * the wind (mostly headwear), and people being hit by flying objects
 *
 * TODO:
 *	handle more pulltypes
 *	give "entrance" messages for players and objects
 *	proper handling of player resistance to push/pulling
 */
ch_ret pullcheck( CHAR_DATA * ch, int pulse )
{
   ROOM_INDEX_DATA *room;
   EXIT_DATA *xtmp, *xit = NULL;
   OBJ_DATA *obj, *obj_next;
   bool move = FALSE, moveobj = TRUE, showroom = TRUE;
   int pullfact, pull;
   int resistance;
   char *tochar = NULL, *toroom = NULL, *objmsg = NULL;
   char *destrm = NULL, *destob = NULL, *dtxt = "somewhere";

   if( ( room = ch->in_room ) == NULL )
   {
      bug( "pullcheck: %s not in a room?!?", ch->name );
      return rNONE;
   }

   /*
    * Find the exit with the strongest force (if any) 
    */
   for( xtmp = room->first_exit; xtmp; xtmp = xtmp->next )
      if( xtmp->pull && xtmp->to_room && ( !xit || abs( xtmp->pull ) > abs( xit->pull ) ) )
         xit = xtmp;

   if( !xit )
      return rNONE;

   pull = xit->pull;

   /*
    * strength also determines frequency 
    */
   pullfact = URANGE( 1, 20 - ( abs( pull ) / 5 ), 20 );

   /*
    * strongest pull not ready yet... check for one that is 
    */
   if( ( pulse % pullfact ) != 0 )
   {
      for( xit = room->first_exit; xit; xit = xit->next )
         if( xit->pull && xit->to_room )
         {
            pull = xit->pull;
            pullfact = URANGE( 1, 20 - ( abs( pull ) / 5 ), 20 );
            if( ( pulse % pullfact ) != 0 )
               break;
         }

      if( !xit )
         return rNONE;
   }

   /*
    * negative pull = push... get the reverse exit if any 
    */
   if( pull < 0 )
      if( ( xit = get_exit( room, rev_dir[xit->vdir] ) ) == NULL )
         return rNONE;

   dtxt = rev_exit( xit->vdir );

   /*
    * First determine if the player should be moved or not
    * Check various flags, spells, the players position and strength vs.
    * the pull, etc... any kind of checks you like.
    */
   switch ( xit->pulltype )
   {
      case PULL_CURRENT:
      case PULL_WHIRLPOOL:
         switch ( room->sector_type )
         {
               /*
                * allow whirlpool to be in any sector type 
                */
            default:
               if( xit->pulltype == PULL_CURRENT )
                  break;
            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM:
            case SECT_RIVER: /* River drift currents added - Samson 8-2-98 */
               if( ( ch->mount && !IS_FLOATING( ch->mount ) ) || ( !ch->mount && !IS_FLOATING( ch ) ) )
                  move = TRUE;
               break;

            case SECT_UNDERWATER:
            case SECT_OCEANFLOOR:
               move = TRUE;
               break;
         }
         break;
      case PULL_GEYSER:
      case PULL_WAVE:
         move = TRUE;
         break;

      case PULL_WIND:
      case PULL_STORM:
         /*
          * if not flying... check weight, position & strength 
          */
         move = TRUE;
         break;

      case PULL_COLDWIND:
         /*
          * if not flying... check weight, position & strength 
          */
         /*
          * also check for damage due to bitter cold 
          */
         move = TRUE;
         break;

      case PULL_HOTAIR:
         /*
          * if not flying... check weight, position & strength 
          */
         /*
          * also check for damage due to heat 
          */
         move = TRUE;
         break;

         /*
          * light breeze -- very limited moving power 
          */
      case PULL_BREEZE:
         move = FALSE;
         break;

         /*
          * exits with these pulltypes should also be blocked from movement
          * ie: a secret locked pickproof door with the name "_sinkhole_", etc
          */
      case PULL_EARTHQUAKE:
      case PULL_SINKHOLE:
      case PULL_QUICKSAND:
      case PULL_LANDSLIDE:
      case PULL_SLIP:
      case PULL_LAVA:
         if( ( ch->mount && !IS_FLOATING( ch->mount ) ) || ( !ch->mount && !IS_FLOATING( ch ) ) )
            move = TRUE;
         break;

         /*
          * as if player moved in that direction him/herself 
          */
      case PULL_UNDEFINED:
         return move_char( ch, xit, 0, xit->vdir, FALSE );

         /*
          * all other cases ALWAYS move 
          */
      default:
         move = TRUE;
         break;
   }

   /*
    * assign some nice text messages 
    */
   switch ( xit->pulltype )
   {
      case PULL_MYSTERIOUS:
         /*
          * no messages to anyone 
          */
         showroom = FALSE;
         break;
      case PULL_WHIRLPOOL:
      case PULL_VACUUM:
         tochar = "You are sucked $T!";
         toroom = "$n is sucked $T!";
         destrm = "$n is sucked in from $T!";
         objmsg = "$p is sucked $T.";
         destob = "$p is sucked in from $T!";
         break;
      case PULL_CURRENT:
      case PULL_LAVA:
         tochar = "You drift $T.";
         toroom = "$n drifts $T.";
         destrm = "$n drifts in from $T.";
         objmsg = "$p drifts $T.";
         destob = "$p drifts in from $T.";
         break;
      case PULL_BREEZE:
         tochar = "You drift $T.";
         toroom = "$n drifts $T.";
         destrm = "$n drifts in from $T.";
         objmsg = "$p drifts $T in the breeze.";
         destob = "$p drifts in from $T.";
         break;
      case PULL_GEYSER:
      case PULL_WAVE:
         tochar = "You are pushed $T!";
         toroom = "$n is pushed $T!";
         destrm = "$n is pushed in from $T!";
         destob = "$p floats in from $T.";
         break;
      case PULL_EARTHQUAKE:
         tochar = "The earth opens up and you fall $T!";
         toroom = "The earth opens up and $n falls $T!";
         destrm = "$n falls from $T!";
         objmsg = "$p falls $T.";
         destob = "$p falls from $T.";
         break;
      case PULL_SINKHOLE:
         tochar = "The ground suddenly gives way and you fall $T!";
         toroom = "The ground suddenly gives way beneath $n!";
         destrm = "$n falls from $T!";
         objmsg = "$p falls $T.";
         destob = "$p falls from $T.";
         break;
      case PULL_QUICKSAND:
         tochar = "You begin to sink $T into the quicksand!";
         toroom = "$n begins to sink $T into the quicksand!";
         destrm = "$n sinks in from $T.";
         objmsg = "$p begins to sink $T into the quicksand.";
         destob = "$p sinks in from $T.";
         break;
      case PULL_LANDSLIDE:
         tochar = "The ground starts to slide $T, taking you with it!";
         toroom = "The ground starts to slide $T, taking $n with it!";
         destrm = "$n slides in from $T.";
         objmsg = "$p slides $T.";
         destob = "$p slides in from $T.";
         break;
      case PULL_SLIP:
         tochar = "You lose your footing!";
         toroom = "$n loses $s footing!";
         destrm = "$n slides in from $T.";
         objmsg = "$p slides $T.";
         destob = "$p slides in from $T.";
         break;
      case PULL_VORTEX:
         tochar = "You are sucked into a swirling vortex of colors!";
         toroom = "$n is sucked into a swirling vortex of colors!";
         toroom = "$n appears from a swirling vortex of colors!";
         objmsg = "$p is sucked into a swirling vortex of colors!";
         objmsg = "$p appears from a swirling vortex of colors!";
         break;
      case PULL_HOTAIR:
         tochar = "A blast of hot air blows you $T!";
         toroom = "$n is blown $T by a blast of hot air!";
         destrm = "$n is blown in from $T by a blast of hot air!";
         objmsg = "$p is blown $T.";
         destob = "$p is blown in from $T.";
         break;
      case PULL_COLDWIND:
         tochar = "A bitter cold wind forces you $T!";
         toroom = "$n is forced $T by a bitter cold wind!";
         destrm = "$n is forced in from $T by a bitter cold wind!";
         objmsg = "$p is blown $T.";
         destob = "$p is blown in from $T.";
         break;
      case PULL_WIND:
         tochar = "A strong wind pushes you $T!";
         toroom = "$n is blown $T by a strong wind!";
         destrm = "$n is blown in from $T by a strong wind!";
         objmsg = "$p is blown $T.";
         destob = "$p is blown in from $T.";
         break;
      case PULL_STORM:
         tochar = "The raging storm drives you $T!";
         toroom = "$n is driven $T by the raging storm!";
         destrm = "$n is driven in from $T by a raging storm!";
         objmsg = "$p is blown $T.";
         destob = "$p is blown in from $T.";
         break;
      default:
         if( pull > 0 )
         {
            tochar = "You are pulled $T!";
            toroom = "$n is pulled $T.";
            destrm = "$n is pulled in from $T.";
            objmsg = "$p is pulled $T.";
            objmsg = "$p is pulled in from $T.";
         }
         else
         {
            tochar = "You are pushed $T!";
            toroom = "$n is pushed $T.";
            destrm = "$n is pushed in from $T.";
            objmsg = "$p is pushed $T.";
            objmsg = "$p is pushed in from $T.";
         }
         break;
   }


   /*
    * Do the moving 
    */
   if( move )
   {
      /*
       * display an appropriate exit message 
       */
      if( tochar )
      {
         act( AT_PLAIN, tochar, ch, NULL, dir_name[xit->vdir], TO_CHAR );
         send_to_char( "\n\r", ch );
      }
      if( toroom )
         act( AT_PLAIN, toroom, ch, NULL, dir_name[xit->vdir], TO_ROOM );

      /*
       * display an appropriate entrance message 
       */
      if( destrm && xit->to_room->first_person )
      {
         act( AT_PLAIN, destrm, xit->to_room->first_person, NULL, dtxt, TO_CHAR );
         act( AT_PLAIN, destrm, xit->to_room->first_person, NULL, dtxt, TO_ROOM );
      }


      /*
       * move the char 
       */
      if( xit->pulltype == PULL_SLIP )
         return move_char( ch, xit, 1, xit->vdir, FALSE );
      char_from_room( ch );
      if( !char_to_room( ch, xit->to_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      if( showroom )
         interpret( ch, "look" );

      /*
       * move the mount too 
       */
      if( ch->mount )
      {
         char_from_room( ch->mount );
         if( !char_to_room( ch->mount, xit->to_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         if( showroom )
            interpret( ch->mount, "look" );
      }
   }

   /*
    * move objects in the room 
    */
   if( moveobj )
   {
      for( obj = room->first_content; obj; obj = obj_next )
      {
         obj_next = obj->next_content;

         if( IS_OBJ_FLAG( obj, ITEM_BURIED ) || !CAN_WEAR( obj, ITEM_TAKE ) )
            continue;

         resistance = get_obj_weight( obj );
         if( IS_OBJ_FLAG( obj, ITEM_METAL ) )
            resistance = ( resistance * 6 ) / 5;
         switch ( obj->item_type )
         {
            case ITEM_SCROLL:
            case ITEM_TRASH:
               resistance >>= 2;
               break;
            case ITEM_SCRAPS:
            case ITEM_CONTAINER:
               resistance >>= 1;
               break;
            case ITEM_PEN:
            case ITEM_WAND:
               resistance = ( resistance * 5 ) / 6;
               break;

            case ITEM_CORPSE_PC:
            case ITEM_CORPSE_NPC:
            case ITEM_FOUNTAIN:
               resistance <<= 2;
               break;
         }

         /*
          * is the pull greater than the resistance of the object? 
          */
         if( ( abs( pull ) * 10 ) > resistance )
         {
            if( objmsg && room->first_person )
            {
               act( AT_PLAIN, objmsg, room->first_person, obj, dir_name[xit->vdir], TO_CHAR );
               act( AT_PLAIN, objmsg, room->first_person, obj, dir_name[xit->vdir], TO_ROOM );
            }
            if( destob && xit->to_room->first_person )
            {
               act( AT_PLAIN, destob, xit->to_room->first_person, obj, dtxt, TO_CHAR );
               act( AT_PLAIN, destob, xit->to_room->first_person, obj, dtxt, TO_ROOM );
            }
            obj_from_room( obj );
            obj_to_room( obj, xit->to_room, NULL );
         }
      }
   }

   return rNONE;
}

/*
 * This function bolts a door. Written by Blackmane
 */
CMDF do_bolt( CHAR_DATA * ch, char *argument )
{
   EXIT_DATA *pexit;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Bolt what?\n\r", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, TRUE ) ) != NULL )
   {
      if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         send_to_char( "It's not closed.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_ISBOLT ) )
      {
         send_to_char( "You don't see a bolt.\n\r", ch );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_BOLTED ) )
      {
         send_to_char( "It's already bolted.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_SECRET ) || ( pexit->keyword && nifty_is_name( argument, pexit->keyword ) ) )
      {
         send_to_char( "*Clunk*\n\r", ch );
         act( AT_ACTION, "$n bolts the $d.", ch, NULL, pexit->keyword, TO_ROOM );
         set_bexit_flag( pexit, EX_BOLTED );
         return;
      }
   }
   ch_printf( ch, "You see no %s here.\n\r", argument );
   return;
}

/*
 * This function unbolts a door.  Written by Blackmane
 */
CMDF do_unbolt( CHAR_DATA * ch, char *argument )
{
   EXIT_DATA *pexit;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Unbolt what?\n\r", ch );
      return;
   }

   if( ( pexit = find_door( ch, argument, TRUE ) ) != NULL )
   {
      if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         send_to_char( "It's not closed.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_ISBOLT ) )
      {
         send_to_char( "You don't see a bolt.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_BOLTED ) )
      {
         send_to_char( "It's already unbolted.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_SECRET ) || ( pexit->keyword && nifty_is_name( argument, pexit->keyword ) ) )
      {
         send_to_char( "*Clunk*\n\r", ch );
         act( AT_ACTION, "$n unbolts the $d.", ch, NULL, pexit->keyword, TO_ROOM );
         remove_bexit_flag( pexit, EX_BOLTED );
         return;
      }
   }
   ch_printf( ch, "You see no %s here.\n\r", argument );
   return;
}
