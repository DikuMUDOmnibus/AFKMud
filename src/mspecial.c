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
 *                   "Special procedure" module for Mobs                    *
 ****************************************************************************/

/******************************************************
            Desolation of the Dragon MUD II
      (C) 1997, 1998  Jesse DeFer and Heath Leach
 http://dotd.mudservices.com  dotd@dotd.mudservices.com 
 ******************************************************/

/* Any spec_fun added here needs to be added to specfuns.dat as well.
 * If you don't know what that means, ask Samson to take care of it.
 */

#include <dlfcn.h>
#include "mud.h"
#include "fight.h"
#include "mspecial.h"

SPELLF spell_smaug( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_cure_blindness( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_cure_poison( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_remove_curse( int sn, int level, CHAR_DATA * ch, void *vo );
void start_hunting( CHAR_DATA * ch, CHAR_DATA * victim );
void start_hating( CHAR_DATA * ch, CHAR_DATA * victim );
void set_fighting( CHAR_DATA * ch, CHAR_DATA * victim );

SPEC_LIST *first_specfun;
SPEC_LIST *last_specfun;

void free_specfuns( void )
{
   SPEC_LIST *spec, *spec_next;

   for( spec = first_specfun; spec; spec = spec_next )
   {
      spec_next = spec->next;
      UNLINK( spec, first_specfun, last_specfun, next, prev );
      DISPOSE( spec->name );
      DISPOSE( spec );
   }
   return;
}

/* Simple load function - no OLC support for now.
 * This is probably something you DONT want builders playing with.
 */
void load_specfuns( void )
{
   SPEC_LIST *specfun;
   FILE *fp;
   char filename[256];
   char *word;

   first_specfun = NULL;
   last_specfun = NULL;

   snprintf( filename, 256, "%sspecfuns.dat", SYSTEM_DIR );
   if( !( fp = fopen( filename, "r" ) ) )
   {
      bug( "%s", "load_specfuns: FATAL - cannot load specfuns.dat, exiting." );
      perror( filename );
      exit( 1 );
   }
   else
   {
      for( ;; )
      {
         if( feof( fp ) )
         {
            bug( "%s", "load_specfuns: Premature end of file!" );
            FCLOSE( fp );
            return;
         }
         word = fread_flagstring( fp );
         if( !str_cmp( word, "$" ) )
            break;
         CREATE( specfun, SPEC_LIST, 1 );
         specfun->name = str_dup( word );
         LINK( specfun, first_specfun, last_specfun, next, prev );
      }
      FCLOSE( fp );
   }
   return;
}

/* Simple validation function to be sure a function can be used on mobs */
bool validate_spec_fun( char *name )
{
   SPEC_LIST *specfun;

   for( specfun = first_specfun; specfun; specfun = specfun->next )
   {
      if( !str_cmp( specfun->name, name ) )
         return TRUE;
   }
   return FALSE;
}

/*
 * Given a name, return the appropriate spec_fun.
 */
SPEC_FUN *m_spec_lookup( char *name )
{
   void *funHandle;
   const char *error;

   funHandle = dlsym( sysdata.dlHandle, name );
   if( ( error = dlerror(  ) ) != NULL )
   {
      bug( "Error locating function %s in symbol table.", name );
      return NULL;
   }
   return ( SPEC_FUN * ) funHandle;
}

/* if a spell casting mob is hating someone... try and summon them */
bool summon_if_hating( CHAR_DATA * ch )
{
   CHAR_DATA *victim;
   char name[MIL];
   int sn;
   bool found = FALSE;

   /*
    * Gee, if we have no summon spell for some reason, they can't very well cast it. 
    */
   if( ( sn = skill_lookup( "summon" ) ) == -1 )
      return FALSE;

   if( ch->fighting || ch->fearing || !ch->hating || IS_ROOM_FLAG( ch->in_room, ROOM_SAFE ) )
      return FALSE;

   /*
    * if player is close enough to hunt... don't summon 
    */
   if( ch->hunting )
      return FALSE;

   /*
    * If the mob isn't of sufficient level to cast summon, then why let it? 
    */
   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return FALSE;

   one_argument( ch->hating->name, name );

   /*
    * make sure the char exists - works even if player quits 
    */
   for( victim = first_char; victim; victim = victim->next )
   {
      if( !str_cmp( ch->hating->name, victim->name ) )
      {
         found = TRUE;
         break;
      }
   }

   if( !found )
      return FALSE;
   if( ch->in_room == victim->in_room )
      return FALSE;

   /*
    * Modified so Mobs can't drag you back from the ends of the earth - Samson 12-25-98 
    */
   if( ch->in_room->area != victim->in_room->area )
      return FALSE;

   if( ch->position < POS_STANDING )
      return FALSE;

   if( !IS_NPC( victim ) )
      cmdf( ch, "cast summon 0.%s", name );
   else
      cmdf( ch, "cast summon %s", name );
   return TRUE;
}

/*
 * Core procedure for dragons.
 */
bool dragon( CHAR_DATA * ch, char *fspell_name )
{
   CHAR_DATA *victim, *chosen = NULL;
   int sn, count = 0;

   if( ch->position != POS_FIGHTING
       && ch->position != POS_EVASIVE
       && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE && ch->position != POS_BERSERK )
      return FALSE;

   for( victim = ch->in_room->first_person; victim; victim = victim->next_in_room )
   {
      if( who_fighting( victim ) != ch )
         continue;
      if( !number_range( 0, count ) )
         chosen = victim, count++;        
   }

   if( !chosen )
      return FALSE;

   if( ( sn = skill_lookup( fspell_name ) ) < 0 )
      return FALSE;
   ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, chosen );
   return TRUE;
}

/*
 * Special procedures for mobiles.
 */
SPECF spec_breath_any( CHAR_DATA * ch )
{
   if( ch->position != POS_FIGHTING
       && ch->position != POS_EVASIVE
       && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE && ch->position != POS_BERSERK )
      return FALSE;

   switch ( number_bits( 3 ) )
   {
      case 0:
         return spec_breath_fire( ch );
      case 1:
      case 2:
         return spec_breath_lightning( ch );
      case 3:
         return spec_breath_gas( ch );
      case 4:
         return spec_breath_acid( ch );
      case 5:
      case 6:
      case 7:
         return spec_breath_frost( ch );
   }
   return FALSE;
}

SPECF spec_breath_acid( CHAR_DATA * ch )
{
   return dragon( ch, "acid breath" );
}

SPECF spec_breath_fire( CHAR_DATA * ch )
{
   return dragon( ch, "fire breath" );
}

SPECF spec_breath_frost( CHAR_DATA * ch )
{
   return dragon( ch, "frost breath" );
}

SPECF spec_breath_gas( CHAR_DATA * ch )
{
   int sn;

   if( ch->position != POS_FIGHTING
       && ch->position != POS_EVASIVE
       && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE && ch->position != POS_BERSERK )
      return FALSE;

   if( ( sn = skill_lookup( "gas breath" ) ) < 0 )
      return FALSE;
   ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, NULL );
   return TRUE;
}

SPECF spec_breath_lightning( CHAR_DATA * ch )
{
   return dragon( ch, "lightning breath" );
}

/*
** New Healer Procedure, Modified by Tarl/Lemming
*/
SPECF spec_cast_adept( CHAR_DATA * ch )
{
   CHAR_DATA *victim;
   CHAR_DATA *v_next;
   int percent;

   if( !IS_AWAKE( ch ) || ch->fighting )
      return FALSE;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( victim != ch && can_see( ch, victim, FALSE ) && number_bits( 1 ) == 0 )
         break;
   }

   if( !victim )
      return FALSE;

   /*
    * Wastes too many CPU cycles to let the healer stuff work on them 
    */
   if( IS_NPC( victim ) )
      return FALSE;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return FALSE;

   switch ( number_bits( 4 ) )
   {
      case 0:
         act( AT_MAGIC, "$n utters the word 'ciroht'.", ch, NULL, NULL, TO_ROOM );
         spell_smaug( skill_lookup( "armor" ), ch->level, ch, victim );
         return TRUE;

      case 1:
         act( AT_MAGIC, "$n utters the word 'sunimod'.", ch, NULL, NULL, TO_ROOM );
         spell_smaug( skill_lookup( "bless" ), ch->level, ch, victim );
         return TRUE;

      case 2:
         act( AT_MAGIC, "$n utters the word 'suah'.", ch, NULL, NULL, TO_ROOM );
         spell_cure_blindness( skill_lookup( "cure blindness" ), ch->level, ch, victim );
         return TRUE;

      case 3:
         act( AT_MAGIC, "$n utters the word 'nyrcs'.", ch, NULL, NULL, TO_ROOM );
         spell_cure_poison( skill_lookup( "cure poison" ), ch->level, ch, victim );
         return TRUE;

      case 4:
         act( AT_MAGIC, "$n utters the word 'gartla'.", ch, NULL, NULL, TO_ROOM );
         spell_smaug( skill_lookup( "refresh" ), ch->level, ch, victim );
         return TRUE;

      case 5:
         act( AT_MAGIC, "$n utters the word 'gorog'.", ch, NULL, NULL, TO_ROOM );
         spell_remove_curse( skill_lookup( "remove curse" ), ch->level, ch, victim );
         return TRUE;

      default:
         if( victim->max_hit > 0 )
            percent = ( 100 * victim->hit ) / victim->max_hit;
         else
            percent = -1;

         if( percent >= 90 )
         {
            act( AT_MAGIC, "$n utters the word 'nran'.", ch, NULL, NULL, TO_ROOM );
            spell_smaug( skill_lookup( "cure light" ), ch->level, ch, victim );
         }
         else if( percent >= 75 )
         {
            act( AT_MAGIC, "$n utters the word 'naimad'.", ch, NULL, NULL, TO_ROOM );
            spell_smaug( skill_lookup( "cure serious" ), ch->level, ch, victim );
         }
         else if( percent >= 60 )
         {
            act( AT_MAGIC, "$n utters the word 'piwd'.", ch, NULL, NULL, TO_ROOM );
            spell_smaug( skill_lookup( "cure critical" ), ch->level, ch, victim );
         }
         else
         {
            act( AT_MAGIC, "$n utters the word 'nosmas'.", ch, NULL, NULL, TO_ROOM );
            spell_smaug( skill_lookup( "heal" ), ch->level, ch, victim );
         }
         return TRUE;
   }
}

SPECF spec_cast_cleric( CHAR_DATA * ch )
{
   CHAR_DATA *victim;
   CHAR_DATA *v_next;
   char *spell;
   int sn;

   if( !IS_AWAKE( ch ) )
      return FALSE;

   summon_if_hating( ch );

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return FALSE;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
         break;
   }

   if( !victim || victim == ch )
      return FALSE;

   switch ( number_bits( 5 ) )
   {
      case 0:
         spell = "cause light";
         break;
      case 1:
         spell = "curse";
         break;
      case 2:
         spell = "spiritual hammer";
         break;
      case 3:
         spell = "dispel evil";
         break;
      case 4:
         spell = "cause serious";
         break;
      case 5:
         spell = "blindness";
         break;
      case 6:
         spell = "hold person";
         break;
      case 7:
         spell = "dispel magic";
         break;
      case 8:
         spell = "cause critical";
         break;
      case 9:
         spell = "silence";
         break;
      case 10:
         spell = "harm";
         break;
      case 11:
         spell = "flamestrike";
         break;
      case 12:
         spell = "earthquake";
         break;
      case 13:
         spell = "spectral furor";
         break;
      case 14:
         spell = "holy word";
         break;
      case 15:
         spell = "spiritual wrath";
         break;
      default:
         spell = "cause light";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return FALSE;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return FALSE;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return TRUE;
}

SPECF spec_cast_mage( CHAR_DATA * ch )
{
   CHAR_DATA *victim, *v_next;
   char *spell;
   int sn;

   if( !IS_AWAKE( ch ) )
      return FALSE;

   summon_if_hating( ch );

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return FALSE;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( who_fighting( victim ) && number_bits( 2 ) == 0 )
         break;
   }

   if( !victim || victim == ch )
      return FALSE;

   switch ( number_bits( 5 ) )
   {
      case 0:
         spell = "magic missile";
         break;
      case 1:
         spell = "burning hands";
         break;
      case 2:
         spell = "shocking grasp";
         break;
      case 3:
         spell = "sleep";
         break;
      case 4:
         spell = "weaken";
         break;
      case 5:
         spell = "acid blast";
         break;
      case 6:
         spell = "dispel magic";
         break;
      case 7:
         spell = "magnetic thrust";
         break;
      case 8:
         spell = "blindness";
         break;
      case 9:
         spell = "web";
         break;
      case 10:
         spell = "colour spray";
         break;
      case 11:
         spell = "sulfrous spray";
         break;
      case 12:
         spell = "caustic fount";
         break;
      case 13:
         spell = "lightning bolt";
         break;
      case 14:
         spell = "fireball";
         break;
      case 15:
         spell = "acetum primus";
         break;
      case 16:
         spell = "cone of cold";
         break;
      case 17:
         spell = "quantum spike";
         break;
      case 18:
         spell = "scorching surge";
         break;
      case 19:
         spell = "meteor swarm";
         break;
      case 20:
         spell = "spiral blast";
         break;
      default:
         spell = "magic missile";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return FALSE;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return FALSE;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return TRUE;
}

SPECF spec_cast_undead( CHAR_DATA * ch )
{
   CHAR_DATA *victim, *v_next;
   char *spell = NULL;
   int sn;

   if( !IS_AWAKE( ch ) )
      return FALSE;

   summon_if_hating( ch );

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return FALSE;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
         break;
   }

   if( !victim || victim == ch )
      return FALSE;

   switch ( number_bits( 5 ) )
   {
      case 0:
         spell = "chill touch";
         break;
      case 2:
         spell = "sleep";
         break;
      case 3:
         spell = "weaken";
         break;
      case 4:
         spell = "black hand";
         break;
      case 5:
         spell = "black fist";
         break;
      case 6:
         spell = "dispel magic";
         break;
      case 7:
         spell = "fatigue";
         break;
      case 8:
         spell = "lethargy";
         break;
      case 9:
         spell = "necromantic touch";
         break;
      case 10:
         spell = "withering hand";
         break;
      case 11:
         spell = "death chant";
         break;
      case 12:
         spell = "paralyze";
         break;
      case 13:
         spell = "black lightning";
         break;
      case 14:
         spell = "harm";
         break;
      case 15:
         spell = "death aura";
         break;
      case 16:
         spell = "death spell";
         break;
      case 17:
         spell = "vampiric touch";
         break;
      default:
         spell = "chill touch";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return FALSE;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return FALSE;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return TRUE;
}

SPECF spec_guard( CHAR_DATA * ch )
{
   CHAR_DATA *victim;
   CHAR_DATA *v_next;
   CHAR_DATA *ech;
   int max_evil;

   if( !IS_AWAKE( ch ) || ch->fighting )
      return FALSE;

   max_evil = 300;
   ech = NULL;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;

      if( victim->fighting && who_fighting( victim ) != ch && victim->alignment < max_evil )
      {
         max_evil = victim->alignment;
         ech = victim;
      }
   }

   if( ech )
   {
      act( AT_YELL, "$n screams 'PROTECT THE INNOCENT!!  BANZAI!!  SPOON!!", ch, NULL, NULL, TO_ROOM );
      multi_hit( ch, ech, TYPE_UNDEFINED );
      return TRUE;
   }

   return FALSE;
}

SPECF spec_janitor( CHAR_DATA * ch )
{
   OBJ_DATA *trash;
   OBJ_DATA *trash_next;

   if( !IS_AWAKE( ch ) )
      return FALSE;

   for( trash = ch->in_room->first_content; trash; trash = trash_next )
   {
      trash_next = trash->next_content;
      if( !IS_WEAR_FLAG( trash, ITEM_TAKE ) || IS_OBJ_FLAG( trash, ITEM_BURIED ) )
         continue;
      if( IS_OBJ_FLAG( trash, ITEM_PROTOTYPE ) && !IS_ACT_FLAG( ch, ACT_PROTOTYPE ) )
         continue;

      if( IS_ACT_FLAG( ch, ACT_ONMAP ) )
      {
         if( ch->map != trash->map || ch->x != trash->x || ch->y != trash->y )
            continue;
      }

      if( trash->item_type == ITEM_DRINK_CON || trash->item_type == ITEM_TRASH
          || trash->cost < 10 || ( trash->pIndexData->vnum == OBJ_VNUM_SHOPPING_BAG && !trash->first_content ) )
      {
         act( AT_ACTION, "$n picks up some trash.", ch, NULL, NULL, TO_ROOM );
         obj_from_room( trash );
         obj_to_char( trash, ch );
         return TRUE;
      }
   }

   return FALSE;
}

/* For area conversion compatibility - DON'T REMOVE THIS */
SPECF spec_poison( CHAR_DATA * ch )
{
   return spec_snake( ch );
}

SPECF spec_snake( CHAR_DATA * ch )
{
   CHAR_DATA *victim;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return FALSE;

   if( ( victim = who_fighting( ch ) ) == NULL )
      return FALSE;

   if( number_percent(  ) > ch->level )
      return FALSE;

   act( AT_HIT, "You bite $N!", ch, NULL, victim, TO_CHAR );
   act( AT_ACTION, "$n bites $N!", ch, NULL, victim, TO_NOTVICT );
   act( AT_POISON, "$n bites you!", ch, NULL, victim, TO_VICT );
   spell_smaug( gsn_poison, ch->level, ch, victim );
   return TRUE;
}

SPECF spec_thief( CHAR_DATA * ch )
{
   CHAR_DATA *victim, *v_next;
   int gold, maxgold;

   if( ch->position != POS_STANDING )
      return FALSE;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;

      if( IS_NPC( victim ) || victim->level >= LEVEL_IMMORTAL || number_bits( 2 ) != 0 || !can_see( ch, victim, FALSE ) )
         continue;

      if( IS_AWAKE( victim ) && number_range( 0, ch->level ) == 0 )
      {
         act( AT_ACTION, "You discover $n's hands in your sack of gold!", ch, NULL, victim, TO_VICT );
         act( AT_ACTION, "$N discovers $n's hands in $S sack of gold!", ch, NULL, victim, TO_NOTVICT );
         return TRUE;
      }
      else
      {
         maxgold = ch->level * ch->level * 1000;
         gold = victim->gold * number_range( 1, URANGE( 2, ch->level / 4, 10 ) ) / 100;
         ch->gold += 9 * gold / 10;
         victim->gold -= gold;
         if( ch->gold > maxgold )
            ch->gold = maxgold / 2;
         return TRUE;
      }
   }

   return FALSE;
}

void submit( CHAR_DATA * ch, CHAR_DATA * t )
{
   switch ( number_range( 1, 8 ) )
   {
      case 1:
         cmdf( ch, "bow %s", GET_NAME( t ) );
         break;
      case 2:
         cmdf( ch, "smile %s", GET_NAME( t ) );
         break;
      case 3:
         cmdf( ch, "wink %s", GET_NAME( t ) );
         break;
      case 4:
         cmdf( ch, "wave %s", GET_NAME( t ) );
         break;
      default:
         act( AT_PLAIN, "$n nods $s head at you", ch, 0, t, TO_VICT );
         act( AT_PLAIN, "$n nods $s head at $N", ch, 0, t, TO_NOTVICT );
         break;
   }
}

void sayhello( CHAR_DATA * ch, CHAR_DATA * t )
{
   if( IsBadSide( ch ) )
   {
      switch ( number_range( 1, 10 ) )
      {
         case 1:
            interpret( ch, "say Hey doofus, go get a life!" );
            break;
         case 2:
            if( t->sex == SEX_FEMALE )
               interpret( ch, "say Get lost ...... witch" );
            else
               interpret( ch, "say Are you talking to me, punk?" );
            break;
         case 3:
            interpret( ch, "say May the road you travel be cursed!" );
            break;
         case 4:
            if( t->sex == SEX_FEMALE )
               cmdf( ch, "say Make way!  Make way for me, %s!", GET_NAME( t ) );
            else
               cmdf( ch, "say Make way!  Make way for me, %s!", GET_NAME( t ) );
            break;
         case 5:
            interpret( ch, "say May the evil godling Ixzuul grin evily at you." );
            break;
         case 6:
            interpret( ch, "say Not you again..." );
            break;
         case 7:
            interpret( ch, "say You are always welcome here, great one, now go clean the stables." );
            break;
         case 8:
            interpret( ch, "say Ya know, those smugglers are men after my own heart..." );
            break;
         case 9:
            if( time_info.hour > sysdata.hoursunrise && time_info.hour < sysdata.hournoon )
               cmdf( ch, "say It's morning, %s, do you know where your brains are?", GET_NAME( t ) );
            else if( time_info.hour >= sysdata.hournoon && time_info.hour < sysdata.hoursunset )
               cmdf( ch, "say It's afternoon, %s, do you know where your parents are?", GET_NAME( t ) );
            else if( time_info.hour >= sysdata.hoursunset && time_info.hour <= sysdata.hourmidnight )
               cmdf( ch, "say It's evening, %s, do you know where your kids are?", GET_NAME( t ) );
            else
               cmdf( ch, "say Up for a midnight stroll, %s?\n", GET_NAME( t ) );
            break;
         case 10:
         {
            char buf2[80];
            if( time_info.hour < sysdata.hoursunrise )
               mudstrlcpy( buf2, "evening", 80 );
            else if( time_info.hour < sysdata.hournoon )
               mudstrlcpy( buf2, "morning", 80 );
            else if( time_info.hour < sysdata.hoursunset )
               mudstrlcpy( buf2, "afternoon", 80 );
            else
               mudstrlcpy( buf2, "evening", 80 );

            if( IS_CLOUDY( ch->in_room->area->weather ) )
               cmdf( ch, "say Nice %s to go for a walk, %s, I hate it.", buf2, GET_NAME( t ) );
            else if( IS_RAINING( ch->in_room->area->weather ) )
               cmdf( ch, "say I hope %s's rain never clears up.. don't you %s?", buf2, GET_NAME( t ) );
            else if( IS_SNOWING( ch->in_room->area->weather ) )
               cmdf( ch, "say What a wonderful miserable %s, %s!", buf2, GET_NAME( t ) );
            else
               cmdf( ch, "say Such a terrible %s, don't you think?", buf2 );
            break;
         }
      }
   }
   else
   {
      switch ( number_range( 1, 10 ) )
      {
         case 1:
            interpret( ch, "say Greetings, adventurer" );
            break;
         case 2:
            if( t->sex == SEX_FEMALE )
               interpret( ch, "say Good day, milady" );
            else
               interpret( ch, "say Good day, lord" );
            break;
         case 3:
            if( t->sex == SEX_FEMALE )
               interpret( ch, "say Pleasant Journey, Mistress" );
            else
               interpret( ch, "say Pleasant Journey, Master" );
            break;
         case 4:
            if( t->sex == SEX_FEMALE )
               cmdf( ch, "say Make way!  Make way for the lady %s!", GET_NAME( t ) );
            else
               cmdf( ch, "say Make way!  Make way for the lord %s!", GET_NAME( t ) );
            break;
         case 5:
            interpret( ch, "say May the prophet smile upon you" );
            break;
         case 6:
            interpret( ch, "say It is a pleasure to see you again." );
            break;
         case 7:
            interpret( ch, "say You are always welcome here, great one" );
            break;
         case 8:
            interpret( ch, "say My lord bids you greetings" );
            break;
         case 9:
            if( time_info.hour > sysdata.hoursunrise && time_info.hour < sysdata.hournoon )
               cmdf( ch, "say Good morning, %s", GET_NAME( t ) );
            else if( time_info.hour >= sysdata.hournoon && time_info.hour < sysdata.hoursunset )
               cmdf( ch, "say Good afternoon, %s", GET_NAME( t ) );
            else if( time_info.hour >= sysdata.hoursunset && time_info.hour <= sysdata.hourmidnight )
               cmdf( ch, "say Good evening, %s", GET_NAME( t ) );
            else
               cmdf( ch, "say Up for a midnight stroll, %s?", GET_NAME( t ) );
            break;
         case 10:
         {
            char buf2[80];
            if( time_info.hour < sysdata.hoursunrise )
               mudstrlcpy( buf2, "evening", 80 );
            else if( time_info.hour < sysdata.hournoon )
               mudstrlcpy( buf2, "morning", 80 );
            else if( time_info.hour < sysdata.hoursunset )
               mudstrlcpy( buf2, "afternoon", 80 );
            else
               mudstrlcpy( buf2, "evening", 80 );
            if( IS_CLOUDY( ch->in_room->area->weather ) )
               cmdf( ch, "say Nice %s to go for a walk, %s.", buf2, GET_NAME( t ) );
            else if( IS_RAINING( ch->in_room->area->weather ) )
               cmdf( ch, "say I hope %s's rain clears up.. don't you %s?", buf2, GET_NAME( t ) );
            else if( IS_SNOWING( ch->in_room->area->weather ) )
               cmdf( ch, "say How can you be out on such a miserable %s, %s!", buf2, GET_NAME( t ) );
            else
               cmdf( ch, "say Such a pleasant %s, don't you think?", buf2 );
            break;
         }
      }
   }
}

void greet_people( CHAR_DATA * ch )
{
   CHAR_DATA *tch;

   if( IS_ACT_FLAG( ch, ACT_GREET ) )
   {
      for( tch = ch->in_room->first_person; tch; tch = tch->next_in_room )
      {
         if( !IS_NPC( tch ) && can_see( ch, tch, FALSE ) && number_range( 1, 8 ) == 1 )
         {
            if( tch->level > ch->level )
            {
               submit( ch, tch );
               sayhello( ch, tch );
               break;
            }
         }
      }
   }
}

bool callforhelp( CHAR_DATA * ch, SPEC_FUN * spec )
{
   CHAR_DATA *vch;
   short count = 0;

   for( vch = first_char; vch && count <= 2; vch = vch->next )
   {
      if( ch != vch && !vch->hunting && spec == vch->spec_fun )
      {
         start_hating( vch, who_fighting( ch ) );
         start_hunting( vch, who_fighting( ch ) );
         count++;
      }
   }
   if( count > 0 )
      return TRUE;
   return FALSE;
}

CHAR_DATA *race_align_hatee( CHAR_DATA * ch )
{
   CHAR_DATA *vch;

   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
      if( can_see( ch, vch, FALSE )
          && ( ( IsBadSide( vch ) && IsGoodSide( ch ) ) || ( IsGoodSide( vch ) && IsBadSide( ch ) )
               || ( IsUndead( vch ) && !IsUndead( ch ) ) || ( !IsUndead( vch ) && IsUndead( ch ) ) ) )
         return vch;

   return NULL;
}

SPECF spec_GenericCityguard( CHAR_DATA * ch )
{
   CHAR_DATA *hatee, *fighting;

   if( !IS_AWAKE( ch ) )
      return FALSE;

   fighting = who_fighting( ch );

   if( fighting && fighting->spec_fun == ch->spec_fun )
   {
      stop_fighting( ch, TRUE );
      interpret( ch, "say Pardon me, I didn't mean to attack you!" );
      return TRUE;
   }

   if( fighting )
   {
      if( number_bits( 2 ) > 2 )
         interpret( ch, "shout To me, my fellows!  I need thy aid!" );
      if( !callforhelp( ch, ch->spec_fun ) );
      return TRUE;
   }

   if( ( hatee = race_align_hatee( ch ) ) != NULL )
   {
      interpret( ch, "say Die!" );
      set_fighting( ch, hatee );
      multi_hit( ch, hatee, TYPE_UNDEFINED );
      return TRUE;
   }
   greet_people( ch );
   return FALSE;
}

SPECF spec_GenericCitizen( CHAR_DATA * ch )
{
   if( !IS_AWAKE( ch ) )
      return FALSE;

   if( who_fighting( ch ) )
      if( !callforhelp( ch, ch->spec_fun ) )
      {
         interpret( ch, "say Alas, I am alone!" );
         return TRUE;
      }

   greet_people( ch );
   return TRUE;
}

SPECF spec_fido( CHAR_DATA * ch )
{
   OBJ_DATA *corpse;
   OBJ_DATA *c_next;
   OBJ_DATA *obj;
   OBJ_DATA *obj_next;

   if( !IS_AWAKE( ch ) )
      return FALSE;

   for( corpse = ch->in_room->first_content; corpse; corpse = c_next )
   {
      c_next = corpse->next_content;
      if( corpse->item_type != ITEM_CORPSE_NPC )
         continue;

      if( IS_ACT_FLAG( ch, ACT_ONMAP ) )
      {
         if( ch->map != corpse->map || ch->x != corpse->x || ch->y != corpse->y )
            continue;
      }

      act( AT_ACTION, "$n savagely devours a corpse.", ch, NULL, NULL, TO_ROOM );
      for( obj = corpse->first_content; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         obj_from_obj( obj );
         obj_to_room( obj, ch->in_room, ch );
      }
      extract_obj( corpse );
      return TRUE;
   }

   return FALSE;
}

bool StandUp( CHAR_DATA * ch )
{
   if( ch->wait )
      return FALSE;

   if( GET_POS( ch ) <= POS_STUNNED || GET_POS( ch ) >= POS_BERSERK )
      return FALSE;

   if( GET_HIT( ch ) > ( GET_MAX_HIT( ch ) / 2 ) )
      act( AT_PLAIN, "$n quickly stands up.", ch, NULL, NULL, TO_ROOM );
   else if( GET_HIT( ch ) > ( GET_MAX_HIT( ch ) / 6 ) )
      act( AT_PLAIN, "$n slowly stands up.", ch, NULL, NULL, TO_ROOM );
   else
      act( AT_PLAIN, "$n gets to $s feet very slowly.", ch, NULL, NULL, TO_ROOM );

   if( who_fighting( ch ) )
      GET_POS( ch ) = POS_FIGHTING;
   else
      GET_POS( ch ) = POS_STANDING;

   return TRUE;
}

void MakeNiftyAttack( CHAR_DATA * ch )
{
   CHAR_DATA *fighting;
   int num;

   if( ch->wait )
      return;

   if( GET_POS( ch ) != POS_FIGHTING || !( fighting = who_fighting( ch ) ) )
      return;

   num = number_range( 1, 4 );

   if( num <= 2 )
      cmdf( ch, "bash %s", GET_NAME( fighting ) );
   else if( num == 3 )
   {
      if( get_eq_char( ch, WEAR_WIELD ) && get_eq_char( fighting, WEAR_WIELD ) )
         cmdf( ch, "disarm %s", GET_NAME( fighting ) );
      else
         cmdf( ch, "kick %s", GET_NAME( fighting ) );
   }
   else
      cmdf( ch, "kick %s", GET_NAME( fighting ) );
}

bool FighterMove( CHAR_DATA * ch )
{
   CHAR_DATA *mfriend, *foe;

   if( ch->wait )
      return FALSE;

   if( !( foe = who_fighting( ch ) ) )
      return FALSE;

   if( !( mfriend = who_fighting( foe ) ) )
      return FALSE;

   if( GET_RACE( mfriend ) == GET_RACE( ch ) && GET_HIT( mfriend ) < GET_HIT( ch ) )
      cmdf( ch, "rescue %s", GET_NAME( mfriend ) );
   else
      MakeNiftyAttack( ch );

   return TRUE;
}

bool FindABetterWeapon( CHAR_DATA * ch )
{
   return FALSE;
}

SPECF spec_warrior( CHAR_DATA * ch )
{
   if( !IS_AWAKE( ch ) )
      return FALSE;

   if( who_fighting( ch ) )
   {
      if( StandUp( ch ) )
         return TRUE;

      if( FighterMove( ch ) )
         return TRUE;

      if( FindABetterWeapon( ch ) )
         return TRUE;
   }
   return FALSE;
}

SPECF spec_RustMonster( CHAR_DATA * ch )
{
   OBJ_DATA *eat, *eat_next, *obj = NULL, *obj_next = NULL;

   if( !IS_AWAKE( ch ) )
      return FALSE;

   for( eat = ch->in_room->first_content; eat; eat = eat_next )
   {
      eat_next = eat->next_content;

      if( !IS_WEAR_FLAG( eat, ITEM_TAKE ) || IS_OBJ_FLAG( eat, ITEM_BURIED ) || eat->item_type == ITEM_CORPSE_PC )
         continue;

      if( IS_ACT_FLAG( ch, ACT_ONMAP ) )
      {
         if( ch->map != eat->map || ch->x != eat->x || ch->y != eat->y )
            continue;
      }

      act( AT_ACTION, "$n picks up $p and swallows it.", ch, eat, NULL, TO_ROOM );

      if( eat->item_type == ITEM_CORPSE_NPC )
      {
         act( AT_ACTION, "$n savagely devours a corpse.", ch, NULL, NULL, TO_ROOM );
         for( obj = eat->first_content; obj; obj = obj_next )
         {
            obj_next = obj->next_content;
            obj_from_obj( obj );
            obj_to_room( obj, ch->in_room, ch );
         }
      }
      separate_obj( eat );
      obj_from_room( eat );
      extract_obj( eat );
      return TRUE;
   }
   return FALSE;
}

bool cast_ranger( CHAR_DATA * ch )
{
   CHAR_DATA *victim, *v_next;
   char *spell = NULL;
   int sn;

   if( !IS_AWAKE( ch ) )
      return FALSE;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return FALSE;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
         break;
   }

   if( !victim || victim == ch )
      return FALSE;

   switch ( number_bits( 3 ) )
   {
      case 0:
         spell = "faerie fire";
         break;
      case 1:
         spell = "entangle";
         break;
      case 2:
         spell = "cause light";
         break;
      case 3:
         spell = "faerie fog";
         break;
      case 4:
         spell = "cause serious";
         break;
      case 5:
         spell = "earthquake";
         break;
      case 6:
         spell = "poison";
         break;
      case 7:
         spell = "cause critical";
         break;
      default:
         spell = "cause light";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return FALSE;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return FALSE;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return TRUE;
}

SPECF spec_ranger( CHAR_DATA * ch )
{
   if( !IS_AWAKE( ch ) )
      return FALSE;

   if( who_fighting( ch ) )
   {
      switch ( number_range( 1, 2 ) )
      {
         case 1:
         {
            if( StandUp( ch ) )
               return TRUE;

            if( FighterMove( ch ) )
               return TRUE;

            if( FindABetterWeapon( ch ) )
               return TRUE;
         }
         case 2:
            if( cast_ranger( ch ) )
               return TRUE;
      }
   }
   return FALSE;
}

bool cast_paladin( CHAR_DATA * ch )
{
   CHAR_DATA *victim, *v_next;
   char *spell = NULL;
   int sn;

   if( !IS_AWAKE( ch ) )
      return FALSE;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return FALSE;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
         break;
   }

   if( !victim || victim == ch )
      return FALSE;

   switch ( number_bits( 3 ) )
   {
      case 0:
         spell = "spiritual hammer";
         break;
      case 1:
         spell = "dispel evil";
         break;
      default:
         spell = "spiritual hammer";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return FALSE;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return FALSE;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return TRUE;
}

SPECF spec_paladin( CHAR_DATA * ch )
{
   if( !IS_AWAKE( ch ) )
      return FALSE;

   if( who_fighting( ch ) )
   {
      switch ( number_range( 1, 3 ) )
      {
         case 1:
         case 2:
         {
            if( StandUp( ch ) )
               return TRUE;

            if( FighterMove( ch ) )
               return TRUE;

            if( FindABetterWeapon( ch ) )
               return TRUE;
         }
         case 3:
            if( cast_paladin( ch ) )
               return TRUE;
      }
   }
   return FALSE;
}

SPECF spec_druid( CHAR_DATA * ch )
{
   CHAR_DATA *victim, *v_next;
   char *spell = NULL;
   int sn;

   if( !IS_AWAKE( ch ) )
      return FALSE;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return FALSE;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
         break;
   }

   if( !victim || victim == ch )
      return FALSE;

   switch ( number_bits( 5 ) )
   {
      case 0:
         spell = "cause light";
         break;
      case 1:
         spell = "faerie fire";
         break;
      case 2:
         spell = "cause serious";
         break;
      case 3:
         spell = "entangle";
         break;
      case 4:
         spell = "gust of wind";
         break;
      case 5:
         spell = "earthquake";
         break;
      case 6:
         spell = "harm";
         break;
      case 7:
         spell = "cause critical";
         break;
      case 8:
         spell = "dispel magic";
         break;
      case 9:
         spell = "flamestrike";
         break;
      case 10:
         spell = "call lightning";
         break;
      case 11:
         spell = "firestorm";
         break;
      default:
         spell = "cause light";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return FALSE;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return FALSE;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return TRUE;
}

bool cast_antipaladin( CHAR_DATA * ch )
{
   CHAR_DATA *victim, *v_next;
   char *spell = NULL;
   int sn;

   if( !IS_AWAKE( ch ) )
      return FALSE;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return FALSE;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
         break;
   }

   if( !victim || victim == ch )
      return FALSE;

   switch ( number_bits( 3 ) )
   {
      case 0:
         spell = "curse";
         break;
      case 1:
         spell = "chill touch";
         break;
      case 2:
         spell = "black hand";
         break;
      case 3:
         spell = "withering hand";
         break;
      default:
         spell = "curse";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return FALSE;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return FALSE;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return TRUE;
}

SPECF spec_antipaladin( CHAR_DATA * ch )
{
   if( !IS_AWAKE( ch ) )
      return FALSE;

   if( who_fighting( ch ) )
   {
      switch ( number_range( 1, 3 ) )
      {
         case 1:
         {
            if( StandUp( ch ) )
               return TRUE;

            if( FighterMove( ch ) )
               return TRUE;

            if( FindABetterWeapon( ch ) )
               return TRUE;
         }
         case 2:
         case 3:
            if( cast_antipaladin( ch ) )
               return TRUE;
      }
   }
   return FALSE;
}

SPECF spec_bard( CHAR_DATA * ch )
{
   CHAR_DATA *victim, *v_next;
   char *spell = NULL;
   int sn;

   if( !IS_AWAKE( ch ) )
      return FALSE;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return FALSE;

   for( victim = ch->in_room->first_person; victim; victim = v_next )
   {
      v_next = victim->next_in_room;
      if( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
         break;
   }

   if( !victim || victim == ch )
      return FALSE;

   switch ( number_bits( 3 ) )
   {
      case 0:
         spell = "magic missile";
         break;
      case 1:
         spell = "shocking grasp";
         break;
      case 2:
         spell = "burning hands";
         break;
      case 3:
         spell = "despair";
         break;
      case 4:
         spell = "acid blast";
         break;
      case 5:
         spell = "sonic resonance";
         break;
      case 6:
         spell = "blindness";
         break;
      case 7:
         spell = "colour spray";
         break;
      case 8:
         spell = "lightning bolt";
         break;
      case 9:
         spell = "cacophony";
         break;
      case 10:
         spell = "disintegrate";
         break;
      default:
         spell = "magic missile";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return FALSE;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return FALSE;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return TRUE;
}
