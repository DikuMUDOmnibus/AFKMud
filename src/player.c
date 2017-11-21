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
 *            Commands for personal player settings/statictics              *
 ****************************************************************************
 *                           Pet handling module                            *
 *                      Created by Samson of Alsherok                       *
 ****************************************************************************/

#include <string.h>
#include <ctype.h>
#include "mud.h"
#include "deity.h"
#include "mxp.h"

char *default_fprompt( CHAR_DATA * ch );
char *default_prompt( CHAR_DATA * ch );
void add_follower( CHAR_DATA * ch, CHAR_DATA * master );
void stop_follower( CHAR_DATA * ch );

/* Zone tracking for where PCs have been */
typedef struct zone_data ZONE_DATA;

struct zone_data
{
   ZONE_DATA *next;
   ZONE_DATA *prev;
   char *name;
};

/* Small utility functions to bind & unbind charmies to their owners etc. - Samson 4-19-00 */
void unbind_follower( CHAR_DATA * mob, CHAR_DATA * ch )
{
   stop_follower( mob );

   if( !IS_NPC( ch ) && IS_NPC( mob ) )
   {
      REMOVE_ACT_FLAG( mob, ACT_PET );
      mob->spec_fun = mob->pIndexData->spec_fun;
      ch->pcdata->charmies--;
      UNLINK( mob, ch->first_pet, ch->last_pet, next_pet, prev_pet );
   }
}

void bind_follower( CHAR_DATA * mob, CHAR_DATA * ch, int sn, int duration )
{
   AFFECT_DATA af;

   /*
    * -2 duration means the mob just got loaded from a saved PC and its duration is
    * already set, and will run out when its done 
    */
   if( duration != -2 )
   {
      af.type = sn;
      af.duration = duration;
      af.location = 0;
      af.modifier = 0;
      af.bit = AFF_CHARM;
      affect_to_char( mob, &af );
   }

   add_follower( mob, ch );
   fix_maps( ch, mob );

   if( !IS_NPC( ch ) && IS_NPC( mob ) )
   {
      SET_ACT_FLAG( mob, ACT_PET );
      STRFREE( mob->spec_funname );

      /*
       * Added by Tarl so they would do useful things in combat whilst following Added: 21 April 2002 
       */
      switch ( mob->Class )
      {
         case CLASS_MAGE:
            mob->spec_fun = m_spec_lookup( "spec_cast_mage" );
            mob->spec_funname = STRALLOC( "spec_cast_mage" );
            break;

         case CLASS_CLERIC:
            mob->spec_fun = m_spec_lookup( "spec_cast_cleric" );
            mob->spec_funname = STRALLOC( "spec_cast_cleric" );
            break;

         case CLASS_WARRIOR:
            mob->spec_fun = m_spec_lookup( "spec_warrior" );
            mob->spec_funname = STRALLOC( "spec_warrior" );
            break;

         case CLASS_ROGUE:
            mob->spec_fun = m_spec_lookup( "spec_thief" );
            mob->spec_funname = STRALLOC( "spec_thief" );
            break;

         case CLASS_RANGER:
            mob->spec_fun = m_spec_lookup( "spec_ranger" );
            mob->spec_funname = STRALLOC( "spec_ranger" );
            break;

         case CLASS_PALADIN:
            mob->spec_fun = m_spec_lookup( "spec_paladin" );
            mob->spec_funname = STRALLOC( "spec_paladin" );
            break;

         case CLASS_DRUID:
            mob->spec_fun = m_spec_lookup( "spec_druid" );
            mob->spec_funname = STRALLOC( "spec_druid" );
            break;

         case CLASS_ANTIPALADIN:
            mob->spec_fun = m_spec_lookup( "spec_antipaladin" );
            mob->spec_funname = STRALLOC( "spec_antipaladin" );
            break;

         case CLASS_BARD:
            mob->spec_fun = m_spec_lookup( "spec_bard" );
            mob->spec_funname = STRALLOC( "spec_bard" );
            break;

         default:
            mob->spec_fun = NULL;
            break;
      }
      ch->pcdata->charmies++;
      LINK( mob, ch->first_pet, ch->last_pet, next_pet, prev_pet );
   }
   return;
}

bool check_pets( CHAR_DATA * ch, MOB_INDEX_DATA * pet )
{
   CHAR_DATA *mob;

   for( mob = ch->first_pet; mob; mob = mob->next_pet )
      if( mob->pIndexData->vnum == pet->vnum )
         return TRUE;
   return FALSE;
}

/* 
 * Edited by Tarl 12 May 2002 to accept a character name as argument.
 */
CMDF do_petlist( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *vch, *pet, *victim;

   if( argument && argument[0] != '\0' )
   {
      if( !( victim = get_char_world( ch, argument ) ) )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }

      if( IS_NPC( victim ) )
      {
         send_to_char( "This doesn't work on NPC's.\n\r", ch );
         return;
      }
      ch_printf( ch, "Pets for %s:\n\r", victim->name );
      send_to_char( "--------------------------------------------------------------------\n\r", ch );

      for( pet = victim->first_pet; pet; pet = pet->next_pet )
         ch_printf( ch, "[5%d] %-28s [%5d] %s\n\r",
                    pet->pIndexData->vnum, pet->short_descr, pet->in_room->vnum, pet->in_room->name );

      return;
   }
   else
   {
      send_to_char( "Character           | Follower\n\r", ch );
      send_to_char( "-----------------------------------------------------------------\n\r", ch );
      for( vch = first_char; vch; vch = vch->next )
         if( !IS_NPC( vch ) )
            for( pet = vch->first_pet; pet; pet = pet->next_pet )
               ch_printf( ch, "[5%d] %-28s [%5d] %s\n\r",
                          pet->pIndexData->vnum, pet->short_descr, pet->in_room->vnum, pet->in_room->name );
      return;
   }
}

/* Smartsac toggle - Samson 6-6-99 */
CMDF do_smartsac( CHAR_DATA * ch, char *argument )
{
   if( IS_PLR_FLAG( ch, PLR_SMARTSAC ) )
   {
      REMOVE_PLR_FLAG( ch, PLR_SMARTSAC );
      send_to_char( "Smartsac off.\n\r", ch );
   }
   else
   {
      SET_PLR_FLAG( ch, PLR_SMARTSAC );
      send_to_char( "Smartsac on.\n\r", ch );
   }
   return;
}

CMDF do_gold( CHAR_DATA * ch, char *argument )
{
   ch_printf( ch, "&[gold]You have %d gold pieces.\n\r", ch->gold );
   return;
}

char *get_class( CHAR_DATA * ch )
{
   if( IS_NPC( ch ) && ch->Class < MAX_NPC_CLASS && ch->Class >= 0 )
      return ( npc_class[ch->Class] );
   else if( !IS_NPC( ch ) && ch->Class < MAX_PC_CLASS && ch->Class >= 0 )
      return ( class_table[ch->Class]->who_name );
   return ( "Unknown" );
}

char *get_race( CHAR_DATA * ch )
{
   if( ch->race < MAX_PC_RACE && ch->race >= 0 )
      return ( race_table[ch->race]->race_name );
   if( ch->race < MAX_NPC_RACE && ch->race >= 0 )
      return ( npc_race[ch->race] );
   return ( "Unknown" );
}

/* Spits back a word for a stat being rolled or viewed in score - Samson 12-20-00 */
char *attribtext( int attribute )
{
   static char atext[25];

   if( attribute < 25 )
      mudstrlcpy( atext, "Excellent", 25 );
   if( attribute < 17 )
      mudstrlcpy( atext, "Good", 25 );
   if( attribute < 13 )
      mudstrlcpy( atext, "Fair", 25 );
   if( attribute < 9 )
      mudstrlcpy( atext, "Poor", 25 );
   if( attribute < 5 )
      mudstrlcpy( atext, "Bad", 25 );

   return atext;
}

/* Return a string for weapon condition - Samson 3-01-02 */
char *condtxt( int current, int base )
{
   static char text[30];

   current *= 100;
   base *= 100;

   if( current < base * 0.25 )
      mudstrlcpy( text, " }R[Falling Apart!]&D", 30 );
   else if( current < base * 0.5 )
      mudstrlcpy( text, " &R[In Need of Repair]&D", 30 );
   else if( current < base * 0.75 )
      mudstrlcpy( text, " &Y[In Fair Condition]&D", 30 );
   else if( current < base )
      mudstrlcpy( text, " &g[In Good Condition]&D", 30 );
   else
      mudstrlcpy( text, " &G[In Perfect Condition]&D", 30 );

   return text;
}

void free_zonedata( CHAR_DATA * ch )
{
   ZONE_DATA *zone, *zone_next;

   if( IS_NPC( ch ) )
      return;

   /*
    * Free up the zone list 
    */
   for( zone = ch->pcdata->first_zone; zone; zone = zone_next )
   {
      zone_next = zone->next;
      UNLINK( zone, ch->pcdata->first_zone, ch->pcdata->last_zone, next, prev );
      STRFREE( zone->name );
      DISPOSE( zone );
   }
   return;
}

void save_zonedata( CHAR_DATA * ch, FILE * fp )
{
   ZONE_DATA *zone;

   /*
    * Save the list of zones PC has visited - Samson 7-11-00 
    */
   for( zone = ch->pcdata->first_zone; zone; zone = zone->next )
      fprintf( fp, "Zone		%s~\n", zone->name );
   return;
}

void load_zonedata( CHAR_DATA * ch, FILE * fp )
{
   ZONE_DATA *zone, *zone_prev;
   AREA_DATA *tarea;
   bool found = FALSE;
   char *zonename = fread_flagstring( fp );

   for( tarea = first_area; tarea; tarea = tarea->next )
      if( !str_cmp( tarea->filename, zonename ) )
      {
         found = TRUE;
         break;
      }

   if( !found )
      return;

   CREATE( zone, ZONE_DATA, 1 );
   zone->name = STRALLOC( zonename );

   for( zone_prev = ch->pcdata->first_zone; zone_prev; zone_prev = zone_prev->next )
      if( strcasecmp( zone_prev->name, zone->name ) >= 0 )
         break;

   if( !zone_prev )
      LINK( zone, ch->pcdata->first_zone, ch->pcdata->last_zone, next, prev );
   else
      INSERT( zone, zone_prev, ch->pcdata->first_zone, next, prev );

   return;
}

/* Functions for use with area visiting code - Samson 7-11-00  */
bool has_visited( CHAR_DATA * ch, AREA_DATA * area )
{
   ZONE_DATA *zone;

   if( IS_NPC( ch ) )
      return FALSE;

   for( zone = ch->pcdata->first_zone; zone; zone = zone->next )
   {
      if( !str_cmp( area->name, zone->name ) )
         return TRUE;
   }
   return FALSE;
}

void update_visits( CHAR_DATA * ch, ROOM_INDEX_DATA * room )
{
   ZONE_DATA *zone, *zone_prev;

   if( IS_NPC( ch ) )
      return;

   if( has_visited( ch, room->area ) )
      return;

   CREATE( zone, ZONE_DATA, 1 );
   zone->name = STRALLOC( room->area->name );

   for( zone_prev = ch->pcdata->first_zone; zone_prev; zone_prev = zone_prev->next )
      if( strcasecmp( zone_prev->name, room->area->name ) >= 0 )
         break;

   if( !zone_prev )
      LINK( zone, ch->pcdata->first_zone, ch->pcdata->last_zone, next, prev );
   else
      INSERT( zone, zone_prev, ch->pcdata->first_zone, next, prev );

   return;
}

void remove_visit( CHAR_DATA * ch, ROOM_INDEX_DATA * room )
{
   ZONE_DATA *zone, *zone_next;

   if( IS_NPC( ch ) )
      return;

   if( !has_visited( ch, room->area ) )
      return;

   for( zone = ch->pcdata->first_zone; zone; zone = zone_next )
   {
      zone_next = zone->next;

      if( !str_cmp( zone->name, room->area->name ) )
      {
         STRFREE( zone->name );
         UNLINK( zone, ch->pcdata->first_zone, ch->pcdata->last_zone, next, prev );
         DISPOSE( zone );
      }
   }
   return;
}

/* Modified to display zones in alphabetical order by Tarl - 5th Feb 2001 */
/* Redone to use the sort methods in load_zonedata and update_visits by Samson 10-4-03 */
CMDF do_visits( CHAR_DATA * ch, char *argument )
{
   ZONE_DATA *zone;
   CHAR_DATA *victim;
   int visits = 0;

   if( IS_NPC( ch ) )
   {
      send_to_char( "NPCs cannot use this command.\n\r", ch );
      return;
   }

   if( !IS_IMMORTAL( ch ) || !argument || argument[0] == '\0' )
   {
      send_to_pager( "You have visited the following areas:\n\r", ch );
      send_to_pager( "-------------------------------------\n\r", ch );
      for( zone = ch->pcdata->first_zone; zone; zone = zone->next )
      {
         pager_printf( ch, "%s\n\r", zone->name );
         visits++;
      }
      pager_printf( ch, "&YTotal areas visited: %d\n\r", visits );
      return;
   }

   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "No such person is online.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "That's an NPC, they don't have this data.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      do_visits( ch, "" );
      return;
   }
   pager_printf( ch, "%s has visited the following areas:\n\r", victim->name );
   send_to_pager( "-------------------------------------------\n\r", ch );
   for( zone = victim->pcdata->first_zone; zone; zone = zone->next )
   {
      pager_printf( ch, "%s\n\r", zone->name );
      visits++;
   }
   pager_printf( ch, "&YTotal areas visited: %d\n\r", visits );
   return;
}

/*								-Thoric
 * Display your current exp, level, and surrounding level exp requirements
 */
CMDF do_level( CHAR_DATA * ch, char *argument )
{
   char buf[MSL], buf2[MSL];
   int x, lowlvl, hilvl;

   if( ch->level == 1 )
      lowlvl = 1;
   else
      lowlvl = UMAX( 2, ch->level - 5 );
   hilvl = URANGE( ch->level, ch->level + 5, MAX_LEVEL );
   ch_printf( ch,
              "\n\r&[score]Experience required, levels %d to %d:\n\r______________________________________________\n\r\n\r",
              lowlvl, hilvl );
   snprintf( buf, MSL, " exp  (You have: %11d)", ch->exp );
   snprintf( buf2, MSL, " exp  (To level: %11ld)", exp_level( ch->level + 1 ) - ch->exp );
   for( x = lowlvl; x <= hilvl; x++ )
      ch_printf( ch, " (%2d) %11ld%s\n\r", x, exp_level( x ),
                 ( x == ch->level ) ? buf : ( x == ch->level + 1 ) ? buf2 : " exp" );
   send_to_char( "______________________________________________\n\r", ch );
}

/* 1997, Blodkai */
CMDF do_remains( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   OBJ_DATA *obj;
   bool found = FALSE;

   if( IS_NPC( ch ) )
      return;
   set_char_color( AT_MAGIC, ch );
   if( !ch->pcdata->deity )
   {
      send_to_pager( "You have no deity from which to seek such assistance...\n\r", ch );
      return;
   }
   if( ch->pcdata->favor < ch->level * 2 )
   {
      send_to_pager( "Your favor is insufficient for such assistance...\n\r", ch );
      return;
   }
   pager_printf( ch, "%s appears in a vision, revealing that your remains... ", ch->pcdata->deity->name );
   snprintf( buf, MSL, "the corpse of %s", ch->name );
   for( obj = first_object; obj; obj = obj->next )
   {
      if( obj->in_room && !str_cmp( buf, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
      {
         found = TRUE;
         pager_printf( ch, "\n\r  - at %s will endure for %d ticks", obj->in_room->name, obj->timer );
      }
   }
   if( !found )
      send_to_pager( " no longer exist.\n\r", ch );
   else
   {
      send_to_pager( "\n\r", ch );
      ch->pcdata->favor -= ch->level * 2;
   }
   return;
}

/* Affects-at-a-glance, Blodkai */
CMDF do_affected( CHAR_DATA * ch, char *argument )
{
   AFFECT_DATA *paf;
   SKILLTYPE *skill;

   if( IS_NPC( ch ) )
      return;

   set_char_color( AT_SCORE, ch );

   if( !str_cmp( argument, "by" ) )
   {
      send_to_char( "\n\r&BImbued with:\n\r", ch );
      ch_printf( ch, "&C%s\n\r", !xIS_EMPTY( ch->affected_by ) ? affect_bit_name( &ch->affected_by ) : "nothing" );
      if( ch->level >= ( LEVEL_AVATAR * 0.20 ) )
      {
         send_to_char( "\n\r", ch );
         if( !xIS_EMPTY( ch->resistant ) )
         {
            send_to_char( "&BResistances:  ", ch );
            ch_printf( ch, "&C%s\n\r", ext_flag_string( &ch->resistant, ris_flags ) );
         }
         if( !xIS_EMPTY( ch->immune ) )
         {
            send_to_char( "&BImmunities:   ", ch );
            ch_printf( ch, "&C%s\n\r", ext_flag_string( &ch->immune, ris_flags ) );
         }
         if( !xIS_EMPTY( ch->susceptible ) )
         {
            send_to_char( "&BSuscepts:     ", ch );
            ch_printf( ch, "&C%s\n\r", ext_flag_string( &ch->susceptible, ris_flags ) );
         }
         if( !xIS_EMPTY( ch->absorb ) )
         {
            send_to_char( "&BAbsorbs:      ", ch );
            ch_printf( ch, "&C%s\n\r", ext_flag_string( &ch->absorb, ris_flags ) );
         }
      }
      return;
   }

   if( !ch->first_affect )
      send_to_char( "\n\r&CNo cantrip or skill affects you.\n\r", ch );
   else
   {
      send_to_char( "\n\r", ch );
      for( paf = ch->first_affect; paf; paf = paf->next )
         if( ( skill = get_skilltype( paf->type ) ) != NULL )
         {
            send_to_char( "&BAffected:  ", ch );
            set_char_color( AT_SCORE, ch );
            if( ch->level >= ( LEVEL_AVATAR * 0.20 ) || IS_PKILL( ch ) )
            {
               if( paf->duration < 25 )
                  set_char_color( AT_WHITE, ch );
               if( paf->duration < 6 )
                  set_char_color( AT_RED, ch );
               /*
                * Color changed by Tarl 29 July 2002 to stand out more. 
                */
               ch_printf( ch, "(%5d)   ", paf->duration );
            }
            ch_printf( ch, "%-18s\n\r", skill->name );
         }
   }
   return;
}

CMDF do_inventory( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( !argument || argument[0] == '\0' || !IS_IMMORTAL( ch ) )
      victim = ch;
   else
   {
      if( !( victim = get_char_world( ch, argument ) ) )
      {
         ch_printf( ch, "There is nobody named %s online.\n\r", argument );
         return;
      }
   }

   if( victim != ch )
      ch_printf( ch, "&R%s is carrying:\n\r", IS_NPC( victim ) ? victim->short_descr : victim->name );
   else
      send_to_char( "&RYou are carrying:\n\r", ch );
   mxpobjmenu = MXP_INV;
   mxptail[0] = '\0';
   show_list_to_char( victim->first_carrying, ch, TRUE, TRUE );
   return;
}

CMDF do_equipment( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj, *obj2;
   bool found = FALSE;
   int iWear, count = 0;

   if( !ch )
      return;

   if( !argument || argument[0] == '\0' || !IS_IMMORTAL( ch ) )
      victim = ch;
   else
   {
      if( !( victim = get_char_world( ch, argument ) ) )
      {
         ch_printf( ch, "There is nobody named %s online.\n\r", argument );
         return;
      }
   }

   if( victim != ch )
      ch_printf( ch, "&R%s is using:\n\r", IS_NPC( victim ) ? victim->short_descr : victim->name );
   else
      send_to_char( "&RYou are using:\n\r", ch );
   set_char_color( AT_OBJECT, ch );
   for( iWear = 0; iWear < MAX_WEAR; iWear++ )
   {
      count = 0;
      if( iWear < ( MAX_WEAR - 3 ) )
      {
         if( ( !IS_NPC( victim ) ) && ( victim->race > 0 ) && ( victim->race < MAX_PC_RACE ) )
            send_to_char( race_table[victim->race]->where_name[iWear], ch );
         else
            send_to_char( where_name[iWear], ch );
      }
      if( ( obj2 = get_eq_char( victim, iWear ) ) == NULL && iWear < ( MAX_WEAR - 3 ) )
         send_to_char( "&R<Nothing>&D", ch );
      for( obj = victim->first_carrying; obj; obj = obj->next_content )
      {
         if( obj->wear_loc == iWear )
         {
            count++;
            if( iWear >= ( MAX_WEAR - 3 ) )
            {
               if( ( !IS_NPC( victim ) ) && ( victim->race > 0 ) && ( victim->race < MAX_PC_RACE ) )
                  send_to_char( race_table[victim->race]->where_name[iWear], ch );
               else
                  send_to_char( where_name[iWear], ch );
            }
            if( count > 1 )
               send_to_char( "&C<&W LAYERED &C>&D ", ch );
            if( can_see_obj( ch, obj, FALSE ) )
            {
               send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );

               if( obj->item_type == ITEM_ARMOR )
                  send_to_char( condtxt( obj->value[1], obj->value[0] ), ch );
               if( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_MISSILE_WEAPON )
                  send_to_char( condtxt( obj->value[6], obj->value[0] ), ch );
               if( obj->item_type == ITEM_PROJECTILE )
                  send_to_char( condtxt( obj->value[5], obj->value[0] ), ch );

               send_to_char( "\n\r", ch );
            }
            else
               send_to_char( "something.\n\r", ch );
            found = TRUE;
         }
      }
      if( count == 0 && iWear < ( MAX_WEAR - 3 ) )
         send_to_char( "\n\r", ch );
   }
   return;
}

void set_title( CHAR_DATA * ch, char *title )
{
   char buf[75];

   if( IS_NPC( ch ) )
   {
      bug( "Set_title: NPC %s", ch->name );
      return;
   }

   if( isalpha( title[0] ) || isdigit( title[0] ) )
   {
      buf[0] = ' ';
      mudstrlcpy( buf + 1, title, 75 );
   }
   else
      mudstrlcpy( buf, title, 75 );

   STRFREE( ch->pcdata->title );
   ch->pcdata->title = STRALLOC( buf );
   return;
}

CMDF do_title( CHAR_DATA * ch, char *argument )
{
   char *p;

   if( IS_NPC( ch ) )
      return;

   if( IS_PCFLAG( ch, PCFLAG_NOTITLE ) )
   {
      send_to_char( "The Gods prohibit you from changing your title.\n\r", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "Change your title to what?\n\r", ch );
      return;
   }

   for( p = argument; *p != '\0'; p++ )
   {
      if( *p == '}' )
      {
         send_to_char( "New title is not acceptable, blinking colors are not allowed.\n\r", ch );
         return;
      }
   }
   smash_tilde( argument );
   set_title( ch, argument );
   send_to_char( "Ok.\n\r", ch );
   return;
}

/*
 * Set your personal description				-Thoric
 */
CMDF do_description( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Monsters are too dumb to do that!\n\r", ch );
      return;
   }

   if( !ch->desc )
   {
      bug( "%s", "do_description: no descriptor" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         bug( "do_description: %s illegal substate %d", ch->name, ch->substate );
         return;

      case SUB_RESTRICTED:
         send_to_char( "You cannot use this command from within another command.\n\r", ch );
         return;

      case SUB_NONE:
         ch->substate = SUB_PERSONAL_DESC;
         ch->pcdata->dest_buf = ch;
         if( !ch->chardesc || ch->chardesc[0] == '\0' )
            ch->chardesc = STRALLOC( "" );
         start_editing( ch, ch->chardesc );
         editor_desc_printf( ch, "Your description (%s)", ch->name );
         return;

      case SUB_PERSONAL_DESC:
         STRFREE( ch->chardesc );
         ch->chardesc = copy_buffer( ch );
         stop_editing( ch );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting description.\n\r", ch );
         return;
   }
}

/* Ripped off do_description for whois bio's -- Scryn*/
CMDF do_bio( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot set a bio.\n\r", ch );
      return;
   }
   if( ch->level < 5 )
   {
      send_to_char( "You must be at least level five to write your bio...\n\r", ch );
      return;
   }
   if( !ch->desc )
   {
      bug( "%s", "do_bio: no descriptor" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         bug( "do_bio: %s illegal substate %d", ch->name, ch->substate );
         return;

      case SUB_RESTRICTED:
         send_to_char( "You cannot use this command from within another command.\n\r", ch );
         return;

      case SUB_NONE:
         ch->substate = SUB_PERSONAL_BIO;
         ch->pcdata->dest_buf = ch;
         if( !ch->pcdata->bio || ch->pcdata->bio[0] == '\0' )
            ch->pcdata->bio = str_dup( "" );
         start_editing( ch, ch->pcdata->bio );
         editor_desc_printf( ch, "Your bio (%s).", ch->name );
         return;

      case SUB_PERSONAL_BIO:
         DISPOSE( ch->pcdata->bio );
         ch->pcdata->bio = copy_buffer_nohash( ch );
         stop_editing( ch );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting Bio.\n\r", ch );
         return;
   }
}

CMDF do_report( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) && ch->fighting )
      return;

   if( IS_AFFECTED( ch, AFF_POSSESS ) )
   {
      send_to_char( "You can't do that in your current state of mind!\n\r", ch );
      return;
   }

   ch_printf( ch, "You report: %d/%d hp %d/%d mana %d/%d mv %d xp %ld tnl.\n\r",
              ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp,
              ( exp_level( ch->level + 1 ) - ch->exp ) );

   act_printf( AT_REPORT, ch, NULL, NULL, TO_ROOM, "$n reports: %d/%d hp %d/%d mana %d/%d mv %d xp %ld tnl.",
               ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp,
               ( exp_level( ch->level + 1 ) - ch->exp ) );

   return;
}

CMDF do_fprompt( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "NPC's can't change their prompt..\n\r", ch );
      return;
   }
   smash_tilde( argument );
   if( !argument || argument[0] == '\0' || !str_cmp( argument, "display" ) )
   {
      send_to_char( "Your current fighting prompt string:\n\r", ch );
      ch_printf( ch, "&W%s\n\r", ch->pcdata->fprompt );
      send_to_char( "&wType 'help prompt' for information on changing your prompt.\n\r", ch );
      return;
   }
   send_to_char( "&wReplacing old prompt of:\n\r", ch );
   ch_printf( ch, "&W%s\n\r", ch->pcdata->fprompt );
   STRFREE( ch->pcdata->fprompt );
   if( strlen( argument ) > 128 )
      argument[128] = '\0';

   /*
    * Can add a list of pre-set prompts here if wanted.. perhaps
    * 'prompt 1' brings up a different, pre-set prompt 
    */
   if( !str_cmp( argument, "default" ) )
      ch->pcdata->fprompt = STRALLOC( default_fprompt( ch ) );
   else
      ch->pcdata->fprompt = STRALLOC( argument );
   return;
}

CMDF do_prompt( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "NPC's can't change their prompt..\n\r", ch );
      return;
   }
   smash_tilde( argument );
   if( !argument || argument[0] == '\0' || !str_cmp( argument, "display" ) )
   {
      send_to_char( "&wYour current prompt string:\n\r", ch );
      ch_printf( ch, "&W%s\n\r", ch->pcdata->prompt );
      send_to_char( "&wType 'help prompt' for information on changing your prompt.\n\r", ch );
      return;
   }
   send_to_char( "&wReplacing old prompt of:\n\r", ch );
   ch_printf( ch, "&W%s\n\r", !str_cmp( ch->pcdata->prompt, "" ) ? "(default prompt)" : ch->pcdata->prompt );
   STRFREE( ch->pcdata->prompt );
   if( strlen( argument ) > 128 )
      argument[128] = '\0';

   /*
    * Can add a list of pre-set prompts here if wanted.. perhaps
    * 'prompt 1' brings up a different, pre-set prompt 
    */
   if( !str_cmp( argument, "default" ) )
      ch->pcdata->prompt = STRALLOC( default_prompt( ch ) );
   else
      ch->pcdata->prompt = STRALLOC( argument );
   return;
}

/* Alternate Self delete command provided by Waldemar Thiel (Swiv) */
/* Allows characters to delete themselves - Added 1-18-98 by Samson */
CMDF do_delet( CHAR_DATA * ch, char *argument )
{
   send_to_char( "If you want to DELETE, spell it out.\n\r", ch );
   return;
}

CMDF do_delete( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Yeah, right. Mobs can't delete themselves.\n\r", ch );
      return;
   }

   if( ch->fighting != NULL )
   {
      send_to_char( "Wait until the fight is over before deleting yourself.\n\r", ch );
      return;
   }

   /*
    * Reimbursement warning added to code by Samson 1-18-98 
    */
   send_to_char( "&YRemember, this decision is IRREVERSABLE. There are no reimbursements!\n\r", ch );

   /*
    * Immortals warning added to code by Samson 1-18-98 
    */
   if( IS_IMMORTAL( ch ) )
   {
      ch_printf( ch, "Consider this carefuly %s, if you delete, you will not\n\rbe reinstated as an immortal!\n\r",
                 ch->name );
      send_to_char( "Any area data you have will also be lost if you proceed.\n\r", ch );
   }
   send_to_char( "\n\r&RType your password if you wish to delete your character.\n\r", ch );
   send_to_char( "[DELETE] Password: ", ch );
   write_to_buffer( ch->desc, echo_off_str, 0 );
   ch->desc->connected = CON_DELETE;
   return;
}

/* New command for players to become pkillers - Samson 4-12-98 */
CMDF do_deadly( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
      return;

   if( IS_PCFLAG( ch, PCFLAG_DEADLY ) )
   {
      send_to_char( "You are already a deadly character!\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "&RTo become a pkiller, you MUST type DEADLY YES to do so.\n\r", ch );
      send_to_char( "Remember, this decision is IRREVERSEABLE, so consider it carefuly!\n\r", ch );
      return;
   }

   if( str_cmp( argument, "yes" ) )
   {
      send_to_char( "&RTo become a pkiller, you MUST type DEADLY YES to do so.\n\r", ch );
      send_to_char( "Remember, this decision is IRREVERSEABLE, so consider it carefuly!\n\r", ch );
      return;
   }

   SET_PCFLAG( ch, PCFLAG_DEADLY );
   send_to_char( "&YYou have joined the ranks of the deadly. The gods cease to protect you!\n\r", ch );
   log_printf( "%s has become a pkiller!", ch->name );
   return;
}

int IsHumanoid( CHAR_DATA * ch )
{
   if( !ch )
      return FALSE;

   switch ( GET_RACE( ch ) )
   {
      case RACE_HUMAN:
      case RACE_GNOME:
      case RACE_HIGH_ELF:
      case RACE_GOLD_ELF:
      case RACE_WILD_ELF:
      case RACE_SEA_ELF:
      case RACE_DWARF:
      case RACE_MINOTAUR:
      case RACE_DUERGAR:
      case RACE_CENTAUR:
      case RACE_HALFLING:
      case RACE_ORC:
      case RACE_LYCANTH:
      case RACE_UNDEAD:
      case RACE_UNDEAD_VAMPIRE:
      case RACE_UNDEAD_LICH:
      case RACE_UNDEAD_WIGHT:
      case RACE_UNDEAD_GHAST:
      case RACE_UNDEAD_SPECTRE:
      case RACE_UNDEAD_ZOMBIE:
      case RACE_UNDEAD_SKELETON:
      case RACE_UNDEAD_GHOUL:
      case RACE_GIANT:
      case RACE_GIANT_HILL:
      case RACE_GIANT_FROST:
      case RACE_GIANT_FIRE:
      case RACE_GIANT_CLOUD:
      case RACE_GIANT_STORM:
      case RACE_GIANT_STONE:
      case RACE_GOBLIN:
      case RACE_DEVIL:
      case RACE_TROLL:
      case RACE_VEGMAN:
      case RACE_MFLAYER:
      case RACE_ENFAN:
      case RACE_PATRYN:
      case RACE_SARTAN:
      case RACE_ROO:
      case RACE_SMURF:
      case RACE_TROGMAN:
      case RACE_IGUANADON:
      case RACE_SKEXIE:
      case RACE_TYTAN:
      case RACE_DROW:
      case RACE_GOLEM:
      case RACE_DEMON:
      case RACE_DRAAGDIM:
      case RACE_ASTRAL:
      case RACE_GOD:
      case RACE_HALF_ELF:
      case RACE_HALF_ORC:
      case RACE_HALF_TROLL:
      case RACE_HALF_OGRE:
      case RACE_HALF_GIANT:
      case RACE_GNOLL:
      case RACE_TIEFLING:
      case RACE_AASIMAR:
      case RACE_VAGABOND:
         return TRUE;

      default:
         return FALSE;
   }

}

int IsRideable( CHAR_DATA * ch )
{
   if( !ch )
      return FALSE;

   if( IS_NPC( ch ) )
   {
      switch ( GET_RACE( ch ) )
      {
         case RACE_HORSE:
         case RACE_DRAGON:
         case RACE_DRAGON_RED:
         case RACE_DRAGON_BLACK:
         case RACE_DRAGON_GREEN:
         case RACE_DRAGON_WHITE:
         case RACE_DRAGON_BLUE:
         case RACE_DRAGON_SILVER:
         case RACE_DRAGON_GOLD:
         case RACE_DRAGON_BRONZE:
         case RACE_DRAGON_COPPER:
         case RACE_DRAGON_BRASS:
            return TRUE;
         default:
            return FALSE;
      }
   }
   else
      return FALSE;
}

int IsUndead( CHAR_DATA * ch )
{
   if( !ch )
      return FALSE;

   switch ( GET_RACE( ch ) )
   {
      case RACE_UNDEAD:
      case RACE_GHOST:
      case RACE_UNDEAD_VAMPIRE:
      case RACE_UNDEAD_LICH:
      case RACE_UNDEAD_WIGHT:
      case RACE_UNDEAD_GHAST:
      case RACE_UNDEAD_SPECTRE:
      case RACE_UNDEAD_ZOMBIE:
      case RACE_UNDEAD_SKELETON:
      case RACE_UNDEAD_GHOUL:
      case RACE_UNDEAD_SHADOW:
         return TRUE;
      default:
         return FALSE;
   }
}

int IsLycanthrope( CHAR_DATA * ch )
{
   if( !ch )
      return FALSE;
   switch ( GET_RACE( ch ) )
   {
      case RACE_LYCANTH:
         return TRUE;
      default:
         return FALSE;
   }
}

int IsDiabolic( CHAR_DATA * ch )
{

   if( !ch )
      return ( FALSE );
   switch ( GET_RACE( ch ) )
   {
      case RACE_DEMON:
      case RACE_DEVIL:
      case RACE_TIEFLING:
      case RACE_DAEMON:
      case RACE_DEMODAND:
         return ( TRUE );
      default:
         return ( FALSE );
   }

}

int IsReptile( CHAR_DATA * ch )
{
   if( !ch )
      return ( FALSE );
   switch ( GET_RACE( ch ) )
   {
      case RACE_REPTILE:
      case RACE_DRAGON:
      case RACE_DRAGON_RED:
      case RACE_DRAGON_BLACK:
      case RACE_DRAGON_GREEN:
      case RACE_DRAGON_WHITE:
      case RACE_DRAGON_BLUE:
      case RACE_DRAGON_SILVER:
      case RACE_DRAGON_GOLD:
      case RACE_DRAGON_BRONZE:
      case RACE_DRAGON_COPPER:
      case RACE_DRAGON_BRASS:
      case RACE_DINOSAUR:
      case RACE_SNAKE:
      case RACE_TROGMAN:
      case RACE_IGUANADON:
      case RACE_SKEXIE:
         return ( TRUE );
      default:
         return ( FALSE );
   }
}

int HasHands( CHAR_DATA * ch )
{
   if( !ch )
      return ( FALSE );

   if( IsHumanoid( ch ) )
      return ( TRUE );
   if( IsUndead( ch ) )
      return ( TRUE );
   if( IsLycanthrope( ch ) )
      return ( TRUE );
   if( IsDiabolic( ch ) )
      return ( TRUE );
   if( GET_RACE( ch ) == RACE_GOLEM || GET_RACE( ch ) == RACE_SPECIAL )
      return ( TRUE );
   if( IS_IMMORTAL( ch ) )
      return ( TRUE );
   return ( FALSE );
}

int IsPerson( CHAR_DATA * ch )
{
   if( !ch )
      return ( FALSE );


   switch ( GET_RACE( ch ) )
   {
      case RACE_HUMAN:
      case RACE_HIGH_ELF:
      case RACE_WILD_ELF:
      case RACE_DROW:
      case RACE_DWARF:
      case RACE_DUERGAR:
      case RACE_HALFLING:
      case RACE_GNOME:
      case RACE_DEEP_GNOME:
      case RACE_GOLD_ELF:
      case RACE_SEA_ELF:
      case RACE_GOBLIN:
      case RACE_ORC:
      case RACE_TROLL:
      case RACE_SKEXIE:
      case RACE_MFLAYER:
      case RACE_HALF_ORC:
      case RACE_HALF_OGRE:
      case RACE_HALF_GIANT:
         return ( TRUE );

      default:
         return ( FALSE );
   }
}

int IsDragon( CHAR_DATA * ch )
{

   if( !ch )
      return ( FALSE );

   switch ( GET_RACE( ch ) )
   {
      case RACE_DRAGON:
      case RACE_DRAGON_RED:
      case RACE_DRAGON_BLACK:
      case RACE_DRAGON_GREEN:
      case RACE_DRAGON_WHITE:
      case RACE_DRAGON_BLUE:
      case RACE_DRAGON_SILVER:
      case RACE_DRAGON_GOLD:
      case RACE_DRAGON_BRONZE:
      case RACE_DRAGON_COPPER:
      case RACE_DRAGON_BRASS:
         return ( TRUE );
      default:
         return ( FALSE );
   }
}

int IsGoodSide( CHAR_DATA * ch )
{

   switch ( GET_RACE( ch ) )
   {
      case RACE_HIGH_ELF:
      case RACE_GOLD_ELF:
      case RACE_WILD_ELF:
      case RACE_SEA_ELF:
      case RACE_DWARF:
      case RACE_HALFLING:
      case RACE_GNOME:
      case RACE_HALF_ELF:
      case RACE_HALF_OGRE:
      case RACE_HALF_ORC:
      case RACE_HALF_GIANT:
      case RACE_AASIMAR:
         return ( TRUE );
   }

   return ( FALSE );
}

int IsBadSide( CHAR_DATA * ch )
{
   if( IsDragon( ch ) || IsUndead( ch ) || IsDiabolic( ch ) )
      return ( TRUE );
   switch ( GET_RACE( ch ) )
   {
      case RACE_GNOLL:
      case RACE_IGUANADON:
      case RACE_SKEXIE:
      case RACE_DEEP_GNOME:
      case RACE_GOBLIN:
      case RACE_DUERGAR:
      case RACE_ORC:
      case RACE_TROLL:
      case RACE_MFLAYER:
      case RACE_DROW:
      case RACE_UNDEAD_VAMPIRE:
      case RACE_DEVIL:
      case RACE_TIEFLING:
      case RACE_GIANT:
      case RACE_GIANT_HILL:
      case RACE_GIANT_FROST:
      case RACE_GIANT_FIRE:
      case RACE_GIANT_CLOUD:
      case RACE_GIANT_STORM:
      case RACE_GIANT_STONE:
         return ( TRUE );
   }
   return ( FALSE );
}

int race_bodyparts( CHAR_DATA * ch )
{
   int xflags = 0;

   if( GET_RACE( ch ) < MAX_RACE )
   {
      if( race_table[GET_RACE( ch )]->body_parts )
         return ( race_table[GET_RACE( ch )]->body_parts );
   }

   SET_BIT( xflags, PART_GUTS );

   if( !ch )
      return ( xflags );

   if( IsHumanoid( ch ) || IsPerson( ch ) )
      SET_BIT( xflags,
               PART_HEAD | PART_ARMS | PART_LEGS | PART_FEET | PART_FINGERS | PART_EAR | PART_EYE | PART_BRAINS | PART_GUTS |
               PART_HEART );

   if( IsUndead( ch ) )
      REMOVE_BIT( xflags, PART_BRAINS | PART_HEART );

   if( IsDragon( ch ) )
      SET_BIT( xflags, PART_WINGS | PART_TAIL | PART_SCALES | PART_CLAWS | PART_TAILATTACK );

   if( IsReptile( ch ) )
      SET_BIT( xflags, PART_TAIL | PART_SCALES | PART_CLAWS | PART_FORELEGS );

   if( HasHands( ch ) )
      SET_BIT( xflags, PART_HANDS );
   else
      REMOVE_BIT( xflags, PART_HANDS );

   return ( xflags );
}
