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
 *                        Tracking/hunting module                           *
 ****************************************************************************/

#include "mud.h"
#include "fight.h"

#define BFS_ERROR	   -1
#define BFS_ALREADY_THERE  -2
#define BFS_NO_PATH	   -3
#define BFS_MARK    75

#define TRACK_THROUGH_DOORS

void start_hunting( CHAR_DATA * ch, CHAR_DATA * victim );
void set_fighting( CHAR_DATA * ch, CHAR_DATA * victim );
bool mob_fire( CHAR_DATA * ch, char *name );
CHAR_DATA *scan_for_victim( CHAR_DATA * ch, EXIT_DATA * pexit, char *name );

/* You can define or not define TRACK_THOUGH_DOORS, above, depending on
 whether or not you want track to find paths which lead through closed
 or hidden doors.
 */

typedef struct bfs_queue_struct BFS_DATA;
struct bfs_queue_struct
{
   ROOM_INDEX_DATA *room;
   char dir;
   BFS_DATA *next;
};

static BFS_DATA *queue_head = NULL, *queue_tail = NULL, *room_queue = NULL;

/* Utility macros */
#define MARK(room)	( SET_ROOM_FLAG( (room), BFS_MARK ) )
#define UNMARK(room)	( REMOVE_ROOM_FLAG( (room), BFS_MARK ) )
#define IS_MARKED(room)	( IS_ROOM_FLAG( (room), BFS_MARK ) )

bool valid_edge( EXIT_DATA * pexit )
{
   if( pexit->to_room
#ifndef TRACK_THROUGH_DOORS
       && !IS_EXIT_FLAG( pexit, EX_CLOSED )
#endif
       && !IS_MARKED( pexit->to_room ) )
      return TRUE;
   else
      return FALSE;
}

void bfs_enqueue( ROOM_INDEX_DATA * room, char dir )
{
   BFS_DATA *curr;

   CREATE( curr, BFS_DATA, 1 );
   curr->room = room;
   curr->dir = dir;
   curr->next = NULL;

   if( queue_tail )
   {
      queue_tail->next = curr;
      queue_tail = curr;
   }
   else
      queue_head = queue_tail = curr;
}

void bfs_dequeue( void )
{
   BFS_DATA *curr;

   curr = queue_head;

   if( !( queue_head = queue_head->next ) )
      queue_tail = NULL;
   DISPOSE( curr );
}

void bfs_clear_queue( void )
{
   while( queue_head )
      bfs_dequeue(  );
}

void room_enqueue( ROOM_INDEX_DATA * room )
{
   BFS_DATA *curr;

   CREATE( curr, BFS_DATA, 1 );
   curr->room = room;
   curr->next = room_queue;

   room_queue = curr;
}

void clean_room_queue( void )
{
   BFS_DATA *curr, *curr_next;

   for( curr = room_queue; curr; curr = curr_next )
   {
      UNMARK( curr->room );
      curr_next = curr->next;
      DISPOSE( curr );
   }
   room_queue = NULL;
}

int find_first_step( ROOM_INDEX_DATA * src, ROOM_INDEX_DATA * target, int maxdist )
{
   int curr_dir, count;
   EXIT_DATA *pexit;

   if( !src || !target )
   {
      bug( "%s", "NULL source and target rooms passed to find_first_step!" );
      return BFS_ERROR;
   }

   if( src == target )
      return BFS_ALREADY_THERE;

   if( src->area != target->area )
      return BFS_NO_PATH;

   room_enqueue( src );
   MARK( src );

   /*
    * first, enqueue the first steps, saving which direction we're going. 
    */
   for( pexit = src->first_exit; pexit; pexit = pexit->next )
      if( valid_edge( pexit ) )
      {
         curr_dir = pexit->vdir;
         MARK( pexit->to_room );
         room_enqueue( pexit->to_room );
         bfs_enqueue( pexit->to_room, curr_dir );
      }

   count = 0;
   while( queue_head )
   {
      if( ++count > maxdist )
      {
         bfs_clear_queue(  );
         clean_room_queue(  );
         return BFS_NO_PATH;
      }
      if( queue_head->room == target )
      {
         curr_dir = queue_head->dir;
         bfs_clear_queue(  );
         clean_room_queue(  );
         return curr_dir;
      }
      else
      {
         for( pexit = queue_head->room->first_exit; pexit; pexit = pexit->next )
            if( valid_edge( pexit ) )
            {
               curr_dir = pexit->vdir;
               MARK( pexit->to_room );
               room_enqueue( pexit->to_room );
               bfs_enqueue( pexit->to_room, queue_head->dir );
            }
         bfs_dequeue(  );
      }
   }
   clean_room_queue(  );

   return BFS_NO_PATH;
}

CMDF do_track( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *vict;
   int dir, maxdist;
   bool found;

   if( !IS_NPC( ch ) && ch->pcdata->learned[gsn_track] <= 0 )
   {
      send_to_char( "You do not know of this skill yet.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Whom are you trying to track?\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_track]->beats );

   found = FALSE;
   for( vict = first_char; vict; vict = vict->next )
   {
      if( IS_NPC( vict ) && vict->in_room && nifty_is_name( argument, vict->name ) )
      {
         found = TRUE;
         break;
      }
   }

   if( !found )
   {
      send_to_char( "You can't find a trail to anyone like that.\n\r", ch );
      return;
   }

   if( IS_AFFECTED( vict, AFF_NOTRACK ) )
   {
      send_to_char( "You can't find a trail to anyone like that.\n\r", ch );
      return;
   }
   if( IS_ACT_FLAG( ch, ACT_IMMORTAL ) || IS_ACT_FLAG( ch, ACT_PACIFIST ) )
   {
      send_to_char( "You are too peaceful to track anyone.\n\r", ch );
      stop_hunting( ch );
      return;
   }
   maxdist = ch->pcdata ? ch->pcdata->learned[gsn_track] : 10;

   if( ch->Class == CLASS_ROGUE )
      maxdist *= 3;
   switch ( ch->race )
   {
      case RACE_HIGH_ELF:
      case RACE_WILD_ELF:
         maxdist *= 2;
         break;
      case RACE_DEVIL:
      case RACE_DEMON:
      case RACE_GOD:
         maxdist = 30000;
         break;
   }

   if( IS_IMMORTAL( ch ) )
      maxdist = 30000;

   if( maxdist <= 0 )
      return;

   dir = find_first_step( ch->in_room, vict->in_room, maxdist );

   switch ( dir )
   {
      case BFS_ERROR:
         send_to_char( "&RHmm... something seems to be wrong.\n\r", ch );
         break;
      case BFS_ALREADY_THERE:
         if( ch->hunting )
         {
            send_to_char( "&RYou have found your prey!\n\r", ch );
            stop_hunting( ch );
         }
         else
            send_to_char( "&RYou're already in the same room!\n\r", ch );
         break;
      case BFS_NO_PATH:
         if( ch->hunting )
            stop_hunting( ch );
         send_to_char( "&RYou can't sense a trail from here.\n\r", ch );
         learn_from_failure( ch, gsn_track );
         break;
      default:
         if( ch->hunting && ch->hunting->who != vict )
            stop_hunting( ch );
         start_hunting( ch, vict );
         ch_printf( ch, "&RYou sense a trail %s from here...\n\r", dir_name[dir] );
         break;
   }
}

void found_prey( CHAR_DATA * ch, CHAR_DATA * victim )
{
   char victname[MSL];

   if( !victim )
   {
      bug( "%s", "Found_prey: null victim" );
      return;
   }

   if( victim->in_room == NULL )
   {
      bug( "Found_prey: null victim->in_room: %s", victim->name );
      return;
   }

   mudstrlcpy( victname, IS_NPC( victim ) ? victim->short_descr : victim->name, MSL );

   if( !can_see( ch, victim, FALSE ) )
   {
      if( number_percent(  ) < 90 )
         return;
      if( IsHumanoid( ch ) )
      {
         switch ( number_bits( 3 ) )
         {
            case 0:
               cmdf( ch, "say Don't make me find you, %s!", victname );
               break;
            case 1:
               act( AT_ACTION, "$n sniffs around the room for $N.", ch, NULL, victim, TO_NOTVICT );
               act( AT_ACTION, "You sniff around the room for $N.", ch, NULL, victim, TO_CHAR );
               act( AT_ACTION, "$n sniffs around the room for you.", ch, NULL, victim, TO_VICT );
               interpret( ch, "say I can smell your blood!" );
               break;
            case 2:
               cmdf( ch, "yell I'm going to tear %s apart!", victname );
               break;
            case 3:
               interpret( ch, "say Just wait until I find you..." );
               break;
            default:
               break;
         }
      }
      return;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SAFE ) )
   {
      if( number_percent(  ) < 90 )
         return;
      if( IsHumanoid( ch ) )
      {
         switch ( number_bits( 3 ) )
         {
            case 0:
               interpret( ch, "say C'mon out, you coward!" );
               cmdf( ch, "yell %s is a bloody coward!", victname );
               break;
            case 1:
               cmdf( ch, "say Let's take this outside, %s", victname );
               break;
            case 2:
               cmdf( ch, "yell %s is a yellow-bellied wimp!", victname );
               break;
            case 3:
               act( AT_ACTION, "$n takes a few swipes at $N.", ch, NULL, victim, TO_NOTVICT );
               act( AT_ACTION, "You try to take a few swipes $N.", ch, NULL, victim, TO_CHAR );
               act( AT_ACTION, "$n takes a few swipes at you.", ch, NULL, victim, TO_VICT );
               break;
            default:
               break;
         }
      }
      return;
   }

   if( IsHumanoid( ch ) && !IS_ACT_FLAG( ch, ACT_IMMORTAL ) )
   {
      switch ( number_bits( 2 ) )
      {
         case 0:
            cmdf( ch, "yell Your blood is mine, %s!", victname );
            break;
         case 1:
            cmdf( ch, "say Alas, we meet again, %s!", victname );
            break;
         case 2:
            cmdf( ch, "say What do you want on your tombstone, %s?", victname );
            break;
         case 3:
            act( AT_ACTION, "$n lunges at $N from out of nowhere!", ch, NULL, victim, TO_NOTVICT );
            act( AT_ACTION, "You lunge at $N catching $M off guard!", ch, NULL, victim, TO_CHAR );
            act( AT_ACTION, "$n lunges at you from out of nowhere!", ch, NULL, victim, TO_VICT );
            break;
         default:
            break;
      }
   }
   else
   {
      if( IS_ACT_FLAG( ch, ACT_IMMORTAL ) )  /*So peaceful mobiles won't fight or spam. Adjani, 06-19-03 */
      {
         send_to_char( "You are too peaceful to fight.\n\r", ch );
         stop_hating( ch );
         stop_hunting( ch );
         stop_fearing( ch );
         stop_fighting( ch, TRUE );
         return;
      }
   }
   stop_hunting( ch );
   set_fighting( ch, victim );
   multi_hit( ch, victim, TYPE_UNDEFINED );
   return;
}

void hunt_victim( CHAR_DATA * ch )
{
   bool found;
   CHAR_DATA *tmp;
   EXIT_DATA *pexit;
   short ret;

   if( !ch || !ch->hunting || ch->position < POS_BERSERK )
      return;

   /*
    * make sure the char still exists 
    */
   for( found = FALSE, tmp = first_char; tmp && !found; tmp = tmp->next )
      if( ch->hunting->who == tmp )
         found = TRUE;

   if( !found )
   {
      interpret( ch, "say Damn! I lost track of my quarry!!" );
      stop_hunting( ch );
      return;
   }

   if( ch->in_room == ch->hunting->who->in_room )
   {
      if( ch->fighting )
         return;
      found_prey( ch, ch->hunting->who );
      return;
   }

   ret = find_first_step( ch->in_room, ch->hunting->who->in_room, 500 + ch->level * 25 );
   if( ret < 0 )
   {
      interpret( ch, "say Damn! I lost track of my quarry!" );
      stop_hunting( ch );
      return;
   }
   else
   {
      if( ( pexit = get_exit( ch->in_room, ret ) ) == NULL )
      {
         bug( "%s", "Hunt_victim: lost exit?" );
         return;
      }

      /*
       * Segment copied from update.c, why should a hunting mob get an automatic move into a room
       * it should otherwise be unable to occupy? Exception for sentinel mobs, you attack it, it
       * gets to hunt you down, and they'll ignore EX_NOMOB in this section too 
       */
      if( !IS_ACT_FLAG( ch, ACT_PROTOTYPE ) && pexit->to_room
          /*
           * &&   !IS_EXIT_FLAG( pexit, EX_CLOSED ) - Testing to see if mobs can open doors this way 
           */
          /*
           * Keep em from wandering through my walls, Marcus 
           */
          && !IS_EXIT_FLAG( pexit, EX_FORTIFIED )
          && !IS_EXIT_FLAG( pexit, EX_HEAVY )
          && !IS_EXIT_FLAG( pexit, EX_MEDIUM )
          && !IS_EXIT_FLAG( pexit, EX_LIGHT )
          && !IS_EXIT_FLAG( pexit, EX_CRUMBLING )
          && !IS_ROOM_FLAG( pexit->to_room, ROOM_NO_MOB )
          && !IS_ROOM_FLAG( pexit->to_room, ROOM_DEATH )
          && ( !IS_ACT_FLAG( ch, ACT_STAY_AREA ) || pexit->to_room->area == ch->in_room->area ) )
      {
         if( pexit->to_room->sector_type == SECT_WATER_NOSWIM && !IS_AFFECTED( ch, AFF_AQUA_BREATH ) )
            return;

         if( pexit->to_room->sector_type == SECT_RIVER && !IS_AFFECTED( ch, AFF_AQUA_BREATH ) )
            return;

         if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && !IS_ROOM_FLAG( pexit->to_room, ROOM_NO_MOB ) )
            cmdf( ch, "open %s", pexit->keyword );
         move_char( ch, pexit, 0, pexit->vdir, FALSE );
      }

      /*
       * Crash bug fix by Shaddai 
       */
      if( char_died( ch ) )
         return;

      if( !ch->hunting )
      {
         if( !ch->in_room )
         {
            bug( "Hunt_victim: no ch->in_room!  Mob #%d, name: %s.  Placing mob in limbo.", ch->pIndexData->vnum, ch->name );
            if( !char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            return;
         }
         interpret( ch, "say Damn! I lost track of my quarry!" );
         return;
      }
      if( ch->in_room == ch->hunting->who->in_room )
         found_prey( ch, ch->hunting->who );
      else
      {
         CHAR_DATA *vch;

         /*
          * perform a ranged attack if possible 
          */
         /*
          * Changed who to name as scan_for_victim expects the name and
          * * Not the char struct. --Shaddai
          */
         if( ( vch = scan_for_victim( ch, pexit, ch->hunting->name ) ) != NULL )
         {
            if( !mob_fire( ch, ch->hunting->who->name ) )
            {
               /*
                * ranged spell attacks go here 
                */
            }
         }
      }
      return;
   }
}
