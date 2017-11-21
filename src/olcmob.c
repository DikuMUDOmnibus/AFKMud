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
 *                          Oasis OLC Module                                *
 ****************************************************************************/

/****************************************************************************
 * ResortMUD 4.0 Beta by Ntanel, Garinan, Badastaz, Josh, Digifuzz, Senir,  *
 * Kratas, Scion, Shogar and Tagith.  Special thanks to Thoric, Nivek,      *
 * Altrag, Arlorn, Justice, Samson, Dace, HyperEye and Yakkov.              *
 ****************************************************************************
 * Copyright (C) 1996 - 2001 Haslage Net Electronics: MudWorld              *
 * of Lorain, Ohio - ALL RIGHTS RESERVED                                    *
 * The text and pictures of this publication, or any part thereof, may not  *
 * be reproduced or transmitted in any form or by any means, electronic or  *
 * mechanical, includes photocopying, recording, storage in a information   *
 * retrieval system, or otherwise, without the prior written or e-mail      *
 * consent from the publisher.                                              *
 ****************************************************************************
 * GREETING must mention ResortMUD programmers and the help file named      *
 * CREDITS must remain completely intact as listed in the SMAUG license.    *
 ****************************************************************************/

/**************************************************************************\
 *                                                                        *
 *     OasisOLC II for Smaug 1.40 written by Evan Cortens(Tagith)         *
 *                                                                        *
 *   Based on OasisOLC for CircleMUD3.0bpl9 written by Harvey Gilpin      *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 *               Mobile/Player editing module (medit.c) v1.0              *
 *                                                                        *
\**************************************************************************/

#include "mud.h"
#include "mspecial.h"

/* Global Variables */
int SPEC_MAX;

char *specmenu[100];

/* Function prototypes */

void medit_disp_menu( DESCRIPTOR_DATA * d );
int calc_thac0( CHAR_DATA * ch, CHAR_DATA * victim, int dist );
void cleanup_olc( DESCRIPTOR_DATA * d );
void olc_log( DESCRIPTOR_DATA * d, const char *format, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
int mob_xp( CHAR_DATA * mob );

CMDF do_omedit( CHAR_DATA * ch, char *argument )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *victim;

   if( IS_NPC( ch ) )
   {
      send_to_char( "I don't think so...\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "OEdit what?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "Nothing like that in hell, earth, or heaven.\n\r", ch );
      return;
   }

   if( !IS_NPC( victim ) )
   {
      send_to_char( "PC editing is not allowed through the menu system.\n\r", ch );
      return;
   }

   if( get_trust( ch ) < sysdata.level_modify_proto )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   /*
    * Make sure the mob isnt already being edited 
    */
   for( d = first_descriptor; d; d = d->next )
      if( d->connected == CON_MEDIT )
         if( d->olc && OLC_VNUM( d ) == victim->pIndexData->vnum )
         {
            ch_printf( ch, "That mob is currently being edited by %s.\n\r", d->character->name );
            return;
         }

   if( !can_mmodify( ch, victim ) )
      return;

   d = ch->desc;
   CREATE( d->olc, OLC_DATA, 1 );
   OLC_VNUM( d ) = victim->pIndexData->vnum;
   /*
    * medit_setup( d, OLC_VNUM(d) ); 
    */
   d->character->pcdata->dest_buf = victim;
   d->connected = CON_MEDIT;
   OLC_CHANGE( d ) = FALSE;
   medit_disp_menu( d );

   act( AT_ACTION, "$n starts using OLC.", ch, NULL, NULL, TO_ROOM );
   return;
}

CMDF do_mcopy( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   int ovnum, cvnum;
   MOB_INDEX_DATA *orig;

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: mcopy <original> <new>\n\r", ch );
      return;
   }

   if( !is_number( arg1 ) || !is_number( argument ) )
   {
      send_to_char( "Values must be numeric.\n\r", ch );
      return;
   }

   ovnum = atoi( arg1 );
   cvnum = atoi( argument );

   if( get_trust( ch ) < LEVEL_GREATER )
   {
      AREA_DATA *pArea;

      if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
      {
         send_to_char( "You must have an assigned area to copy objects.\n\r", ch );
         return;
      }
      if( cvnum < pArea->low_vnum || cvnum > pArea->hi_vnum )
      {
         send_to_char( "That number is not in your allocated range.\n\r", ch );
         return;
      }
   }

   if( get_mob_index( cvnum ) )
   {
      send_to_char( "Target vnum already exists.\n\r", ch );
      return;
   }

   if( ( orig = get_mob_index( ovnum ) ) == NULL )
   {
      send_to_char( "Source vnum doesn't exist.\n\r", ch );
      return;
   }
   make_mobile( cvnum, ovnum, orig->player_name );
   send_to_char( "Mobile copied.\n\r", ch );
   return;
}

/**************************************************************************
 Menu Displaying Functions
 **************************************************************************/

/*
 * Display poistions (sitting, standing etc), same for pos and defpos
 */
void medit_disp_positions( DESCRIPTOR_DATA * d )
{
   int i;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( i = 0; i < POS_MAX; i++ )
      ch_printf( d->character, "&g%2d&w) %s\n\r", i, capitalize( npc_position[i] ) );
   send_to_char( "Enter position number : ", d->character );
}

/*
 * Display mobile sexes, this is hard coded cause it just works that way :)
 */
void medit_disp_sex( DESCRIPTOR_DATA * d )
{
   int i;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( i = 0; i < SEX_MAX; i++ )
      ch_printf( d->character, "&g%2d&w) %s\n\r", i, capitalize( npc_sex[i] ) );
   send_to_char( "\n\rEnter gender number : ", d->character );
}

void spec_menu( void )
{
   SPEC_LIST *specfun;
   int j = 0;

   specmenu[0] = "None";

   for( specfun = first_specfun; specfun; specfun = specfun->next )
   {
      j++;
      specmenu[j] = specfun->name;
   }
   specmenu[j + 1] = '\0';
   SPEC_MAX = j + 1;
}

void medit_disp_spec( DESCRIPTOR_DATA * d )
{
   CHAR_DATA *ch = d->character;
   int counter, col = 0;

   /*
    * Initialize this baby - hopefully it works?? 
    */
   spec_menu(  );

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < SPEC_MAX; counter++ )
   {
      ch_printf( ch, "&g%2d&w) %-30.30s ", counter, specmenu[counter] );
      if( ++col % 2 == 0 )
         send_to_char( "\n\r", ch );
   }
   send_to_char( "\n\rEnter number of special: ", ch );
}

/*
 * Used for both mob affected_by and object affect bitvectors
 */
void medit_disp_ris( DESCRIPTOR_DATA * d )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
   int counter;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );

   for( counter = 0; counter <= MAX_RIS_FLAG; counter++ )
      ch_printf( d->character, "&g%2d&w) %-20.20s\n\r", counter + 1, ris_flags[counter] );

   if( d->connected == CON_OEDIT )
   {
      switch ( OLC_MODE( d ) )
      {
         case OEDIT_AFFECT_MODIFIER:
            ch_printf( d->character, "\n\rCurrent flags: &c%s&w\n\r", flag_string( d->character->tempnum, ris_flags ) );
            break;
      }
   }
   else if( d->connected == CON_MEDIT )
   {
      switch ( OLC_MODE( d ) )
      {
         case MEDIT_RESISTANT:
            ch_printf( d->character, "\n\rCurrent flags: &c%s&w\n\r", ext_flag_string( &victim->resistant, ris_flags ) );
            break;
         case MEDIT_IMMUNE:
            ch_printf( d->character, "\n\rCurrent flags: &c%s&w\n\r", ext_flag_string( &victim->immune, ris_flags ) );
            break;
         case MEDIT_SUSCEPTIBLE:
            ch_printf( d->character, "\n\rCurrent flags: &c%s&w\n\r", ext_flag_string( &victim->susceptible, ris_flags ) );
            break;
            /*
             * FIX: Editing Absorb flags did not show current flags 
             */
            /*
             * Zarius 5/19/2003 
             */
         case MEDIT_ABSORB:
            ch_printf( d->character, "\n\rCurrent flags: &c%s&w\n\r", ext_flag_string( &victim->absorb, ris_flags ) );
            break;
      }
   }
   send_to_char( "Enter flag (0 to quit): ", d->character );
}

/*
 * Mobile attacks
 */
void medit_disp_attack_menu( DESCRIPTOR_DATA * d )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
   int i;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( i = 0; i < MAX_ATTACK_TYPE; i++ )
      ch_printf( d->character, "&g%2d&w) %-20.20s\n\r", i + 1, attack_flags[i] );

   ch_printf( d->character, "Current flags: &c%s&w\n\rEnter attack flag (0 to exit): ",
              ext_flag_string( &victim->attacks, attack_flags ) );
}

/*
 * Display menu of NPC defense flags
 */
void medit_disp_defense_menu( DESCRIPTOR_DATA * d )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
   int i;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( i = 0; i < MAX_DEFENSE_TYPE; i++ )
      ch_printf( d->character, "&g%2d&w) %-20.20s\n\r", i + 1, defense_flags[i] );

   ch_printf( d->character, "Current flags: &c%s&w\n\rEnter defense flag (0 to exit): ",
              ext_flag_string( &victim->defenses, defense_flags ) );
}

/*-------------------------------------------------------------------*/
/*. Display mob-flags menu .*/

void medit_disp_mob_flags( DESCRIPTOR_DATA * d )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
   int i, columns = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( i = 0; i < MAX_ACT_FLAG; i++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s  ", i + 1, act_flags[i] );
      if( !( ++columns % 2 ) )
         send_to_char( "\n\r", d->character );
   }
   ch_printf( d->character, "\n\rCurrent flags : &c%s&w\n\rEnter mob flags (0 to quit) : ",
              ext_flag_string( &victim->act, act_flags ) );
}

/*-------------------------------------------------------------------*/
/*. Display aff-flags menu .*/

void medit_disp_aff_flags( DESCRIPTOR_DATA * d )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
   int i, columns = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( i = 0; i < MAX_AFFECTED_BY; i++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s  ", i + 1, a_flags[i] );
      if( !( ++columns % 2 ) )
         send_to_char( "\n\r", d->character );
   }

   if( OLC_MODE( d ) == OEDIT_AFFECT_MODIFIER )
   {
      char buf[MSL];

      buf[0] = '\0';

      for( i = 0; i < 32; i++ )
         if( IS_SET( d->character->tempnum, 1 << i ) )
         {
            mudstrlcat( buf, " ", MSL );
            mudstrlcat( buf, a_flags[i], MSL );
         }
      ch_printf( d->character, "\n\rCurrent flags   : &c%s&w\n\r", buf );
   }
   else
      ch_printf( d->character, "\n\rCurrent flags   : &c%s&w\n\r", affect_bit_name( &victim->affected_by ) );
   send_to_char( "Enter affected flags (0 to quit) : ", d->character );
}

void medit_disp_parts( DESCRIPTOR_DATA * d )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
   int count, columns = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( count = 0; count < MAX_BPART; count++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s    ", count + 1, part_flags[count] );

      if( ++columns % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   ch_printf( d->character, "\n\rCurrent flags: %s\n\rEnter flag or 0 to exit: ",
              flag_string( victim->xflags, part_flags ) );
}

void medit_disp_classes( DESCRIPTOR_DATA * d )
{
   int iClass, columns = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( iClass = 0; iClass < MAX_NPC_CLASS; iClass++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s     ", iClass, npc_class[iClass] );
      if( ++columns % 2 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter Class: ", d->character );
}

void medit_disp_races( DESCRIPTOR_DATA * d )
{
   int iRace, columns = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( iRace = 0; iRace < MAX_NPC_RACE; iRace++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s  ", iRace, npc_race[iRace] );
      if( ++columns % 3 == 0 )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\n\rEnter race: ", d->character );
}

/*
 * Display main menu for NPCs
 */
void medit_disp_menu( DESCRIPTOR_DATA * d )
{
   CHAR_DATA *ch = d->character;
   CHAR_DATA *mob = ( CHAR_DATA * ) d->character->pcdata->dest_buf;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   ch_printf( ch, "&w-- Mob Number:  [&c%d&w]\n\r", mob->pIndexData->vnum );
   ch_printf( ch, "&g1&w) Sex: &O%s          &g2&w) Name: &O%s\n\r", npc_sex[mob->sex], mob->name );
   ch_printf( ch, "&g3&w) Shortdesc: &O%s\n\r", mob->short_descr[0] == '\0' ? "(none set)" : mob->short_descr );
   ch_printf( ch, "&g4&w) Longdesc:\n\r&O%s\n\r", mob->long_descr[0] == '\0' ? "(none set)" : mob->long_descr );
   ch_printf( ch, "&g5&w) Description:\n\r&O%-74.74s\n\r\n\r", mob->chardesc ? mob->chardesc : "(none set)" );
   ch_printf( ch, "&g6&w) Class: [&c%-11.11s&w], &g7&w) Race:   [&c%-11.11s&w]\n\r",
              npc_class[mob->Class], npc_race[mob->race] );
   ch_printf( ch, "&g8&w) Level:       [&c%5d&w], &g9&w) Alignment:    [&c%5d&w]\n\r\n\r", mob->level, mob->alignment );

   ch_printf( ch, " &w) Calc Thac0:      [&c%5d&w]\n\r", calc_thac0( mob, NULL, 0 ) );
   ch_printf( ch, "&gA&w) Real Thac0:  [&c%5d&w]\n\r\n\r", mob->mobthac0 );

   ch_printf( ch, " &w) Calc Experience: [&c%10d&w]\n\r", mob->exp );
   ch_printf( ch, "&gB&w) Real Experience: [&c%10d&w]\n\r\n\r", mob->pIndexData->exp );
   ch_printf( ch, "&gC&w) DamNumDice:  [&c%5d&w], &gD&w) DamSizeDice:  [&c%5d&w], &gE&w) DamPlus:  [&c%5d&w]\n\r",
              mob->pIndexData->damnodice, mob->pIndexData->damsizedice, mob->pIndexData->damplus );
   ch_printf( ch, "&gF&w) HitDice:  [&c%dd%d+%d&w]\n\r",
              mob->pIndexData->hitnodice, mob->pIndexData->hitsizedice, mob->pIndexData->hitplus );
   ch_printf( ch, "&gG&w) Gold:     [&c%8d&w], &gH&w) Spec: &O%-22.22s\n\r",
              mob->gold, mob->spec_funname ? mob->spec_funname : "None" );
   ch_printf( ch, "&gI&w) Resistant   : &O%s\n\r", ext_flag_string( &mob->resistant, ris_flags ) );
   ch_printf( ch, "&gJ&w) Immune      : &O%s\n\r", ext_flag_string( &mob->immune, ris_flags ) );
   ch_printf( ch, "&gK&w) Susceptible : &O%s\n\r", ext_flag_string( &mob->susceptible, ris_flags ) );
   ch_printf( ch, "&gL&w) Absorb      : &O%s\n\r", ext_flag_string( &mob->absorb, ris_flags ) );
   ch_printf( ch, "&gM&w) Position    : &O%s\n\r", npc_position[mob->position] );
   ch_printf( ch, "&gN&w) Default Pos : &O%s\n\r", npc_position[mob->defposition] );
   ch_printf( ch, "&gO&w) Attacks     : &c%s\n\r", ext_flag_string( &mob->attacks, attack_flags ) );
   ch_printf( ch, "&gP&w) Defenses    : &c%s\n\r", ext_flag_string( &mob->defenses, defense_flags ) );
   ch_printf( ch, "&gR&w) Body Parts  : &c%s\n\r", flag_string( mob->xflags, part_flags ) );
   ch_printf( ch, "&gS&w) Act Flags   : &c%s\n\r", ext_flag_string( &mob->act, act_flags ) );
   ch_printf( ch, "&gT&w) Affected    : &c%s\n\r", affect_bit_name( &mob->affected_by ) );
   ch_printf( ch, "&gQ&w) Quit\n\r" );
   send_to_char( "Enter choice : ", ch );

   OLC_MODE( d ) = MEDIT_NPC_MAIN_MENU;
}

/*
 * Bogus command for resetting stuff
 */
CMDF do_medit_reset( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) ch->pcdata->dest_buf;

   switch ( ch->substate )
   {
      default:
         return;

      case SUB_MOB_DESC:
         if( !ch->pcdata->dest_buf )
         {
            send_to_char( "Fatal error, report to Samson.\n\r", ch );
            bug( "%s", "do_medit_reset: sub_mob_desc: NULL ch->pcdata->dest_buf" );
            cleanup_olc( ch->desc );
            ch->substate = SUB_NONE;
            return;
         }
         STRFREE( victim->chardesc );
         victim->chardesc = copy_buffer( ch );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->chardesc );
            victim->pIndexData->chardesc = QUICKLINK( victim->chardesc );
         }
         stop_editing( ch );
         ch->pcdata->dest_buf = victim;
         ch->substate = SUB_NONE;
         ch->desc->connected = CON_MEDIT;
         medit_disp_menu( ch->desc );
         return;
   }
}

/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

void medit_parse( DESCRIPTOR_DATA * d, char *arg )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
   int number = 0, minattr, maxattr;
   char arg1[MIL];

   minattr = 1;
   maxattr = 25;

   switch ( OLC_MODE( d ) )
   {
      case MEDIT_NPC_MAIN_MENU:
         switch ( UPPER( *arg ) )
         {
            case 'Q':
               cleanup_olc( d );
               return;
            case '1':
               OLC_MODE( d ) = MEDIT_SEX;
               medit_disp_sex( d );
               return;
            case '2':
               OLC_MODE( d ) = MEDIT_NAME;
               send_to_char( "\n\rEnter name: ", d->character );
               return;
            case '3':
               OLC_MODE( d ) = MEDIT_S_DESC;
               send_to_char( "\n\rEnter short description: ", d->character );
               return;
            case '4':
               OLC_MODE( d ) = MEDIT_L_DESC;
               send_to_char( "\n\rEnter long description: ", d->character );
               return;
            case '5':
               OLC_MODE( d ) = MEDIT_D_DESC;
               d->character->substate = SUB_MOB_DESC;
               d->character->last_cmd = do_medit_reset;

               send_to_char( "Enter new mob description:\r\n", d->character );
               if( !victim->chardesc )
                  victim->chardesc = STRALLOC( "" );
               start_editing( d->character, victim->chardesc );
               return;
            case '6':
               OLC_MODE( d ) = MEDIT_CLASS;
               medit_disp_classes( d );
               return;
            case '7':
               OLC_MODE( d ) = MEDIT_RACE;
               medit_disp_races( d );
               return;
            case '8':
               OLC_MODE( d ) = MEDIT_LEVEL;
               send_to_char( "\n\rEnter level: ", d->character );
               return;
            case '9':
               OLC_MODE( d ) = MEDIT_ALIGNMENT;
               send_to_char( "\n\rEnter alignment: ", d->character );
               return;
            case 'A':
               OLC_MODE( d ) = MEDIT_THACO;
               send_to_char( "\n\rUse 21 to have the mud autocalculate Thac0\n\rEnter Thac0: ", d->character );
               return;
            case 'B':
               OLC_MODE( d ) = MEDIT_EXP;
               send_to_char( "\n\rUse -1 to have the mud autocalculate Exp\n\rEnter Exp: ", d->character );
               return;
            case 'C':
               OLC_MODE( d ) = MEDIT_DAMNUMDIE;
               send_to_char( "\n\rEnter number of damage dice: ", d->character );
               return;
            case 'D':
               OLC_MODE( d ) = MEDIT_DAMSIZEDIE;
               send_to_char( "\n\rEnter size of damage dice: ", d->character );
               return;
            case 'E':
               OLC_MODE( d ) = MEDIT_DAMPLUS;
               send_to_char( "\n\rEnter amount to add to damage: ", d->character );
               return;
            case 'F':
               OLC_MODE( d ) = MEDIT_HITPLUS;
               send_to_char( "\n\rEnter amount to add to hitpoints: ", d->character );
               return;
            case 'G':
               OLC_MODE( d ) = MEDIT_GOLD;
               send_to_char( "\n\rEnter amount of gold mobile carries: ", d->character );
               return;
            case 'H':
               OLC_MODE( d ) = MEDIT_SPEC;
               medit_disp_spec( d );
               return;
            case 'I':
               OLC_MODE( d ) = MEDIT_RESISTANT;
               medit_disp_ris( d );
               return;
            case 'J':
               OLC_MODE( d ) = MEDIT_IMMUNE;
               medit_disp_ris( d );
               return;
            case 'K':
               OLC_MODE( d ) = MEDIT_SUSCEPTIBLE;
               medit_disp_ris( d );
               return;
            case 'L':
               OLC_MODE( d ) = MEDIT_ABSORB;
               medit_disp_ris( d );
               return;
            case 'M':
               OLC_MODE( d ) = MEDIT_POS;
               medit_disp_positions( d );
               return;
            case 'N':
               OLC_MODE( d ) = MEDIT_DEFPOS;
               medit_disp_positions( d );
               return;
            case 'O':
               OLC_MODE( d ) = MEDIT_ATTACK;
               medit_disp_attack_menu( d );
               return;
            case 'P':
               OLC_MODE( d ) = MEDIT_DEFENSE;
               medit_disp_defense_menu( d );
               return;
            case 'R':
               OLC_MODE( d ) = MEDIT_PARTS;
               medit_disp_parts( d );
               return;
            case 'S':
               OLC_MODE( d ) = MEDIT_NPC_FLAGS;
               medit_disp_mob_flags( d );
               return;
            case 'T':
               OLC_MODE( d ) = MEDIT_AFF_FLAGS;
               medit_disp_aff_flags( d );
               return;
            default:
               medit_disp_menu( d );
               return;
         }
         break;

      case MEDIT_NAME:
         STRFREE( victim->name );
         victim->name = STRALLOC( arg );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->player_name );
            victim->pIndexData->player_name = QUICKLINK( victim->name );
         }
         olc_log( d, "Changed name to %s", arg );
         break;

      case MEDIT_S_DESC:
         STRFREE( victim->short_descr );
         victim->short_descr = STRALLOC( arg );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->short_descr );
            victim->pIndexData->short_descr = QUICKLINK( victim->short_descr );
         }
         olc_log( d, "Changed short desc to %s", arg );
         break;

      case MEDIT_L_DESC:
         stralloc_printf( &victim->long_descr, "%s\n\r", arg );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->long_descr );
            victim->pIndexData->long_descr = QUICKLINK( victim->long_descr );
         }
         olc_log( d, "Changed long desc to %s", arg );
         break;

      case MEDIT_D_DESC:
         /*
          * . We should never get here .
          */
         cleanup_olc( d );
         bug( "%s", "OLC: medit_parse(): Reached D_DESC case!" );
         break;

      case MEDIT_NPC_FLAGS:
         /*
          * REDONE, again, then again 
          */
         if( is_number( arg ) )
            if( atoi( arg ) == 0 )
               break;

         while( arg[0] != '\0' )
         {
            arg = one_argument( arg, arg1 );

            if( is_number( arg1 ) )
            {
               number = atoi( arg1 );
               number -= 1;

               if( number < 0 || number >= MAX_ACT_FLAG )
               {
                  send_to_char( "Invalid flag, try again: ", d->character );
                  return;
               }
            }
            else
            {
               number = get_actflag( arg1 );
               if( number < 0 )
               {
                  send_to_char( "Invalid flag, try again: ", d->character );
                  return;
               }
            }
            if( IS_NPC( victim )
                && number == ACT_PROTOTYPE
                && get_trust( d->character ) < LEVEL_GREATER && !hasname( d->character->pcdata->bestowments, "protoflag" ) )
               send_to_char( "You don't have permission to change the prototype flag.\n\r", d->character );
            else if( IS_NPC( victim ) && number == ACT_IS_NPC )
               send_to_char( "It isn't possible to change that flag.\n\r", d->character );
            else
            {
               xTOGGLE_BIT( victim->act, number );
            }
            if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
               victim->pIndexData->act = victim->act;
         }
         medit_disp_mob_flags( d );
         return;

      case MEDIT_AFF_FLAGS:
         if( is_number( arg ) )
         {
            number = atoi( arg );
            if( number == 0 )
               break;
            if( ( number > 0 ) || ( number < 31 ) )
            {
               number -= 1;
               xTOGGLE_BIT( victim->affected_by, number );
               olc_log( d, "%s the affect %s", IS_AFFECTED( victim, number ) ? "Added" : "Removed", a_flags[number] );
            }
         }
         else
         {
            while( arg[0] != '\0' )
            {
               arg = one_argument( arg, arg1 );
               number = get_actflag( arg1 );
               if( number > 0 )
               {
                  xTOGGLE_BIT( victim->affected_by, number );
                  olc_log( d, "%s the affect %s", IS_AFFECTED( victim, number ) ? "Added" : "Removed", a_flags[number] );
               }
            }
         }
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->affected_by = victim->affected_by;
         medit_disp_aff_flags( d );
         return;

/*-------------------------------------------------------------------*/
/*. Numerical responses .*/
      case MEDIT_HITPOINT:
         victim->max_hit = URANGE( 1, atoi( arg ), 32700 );
         olc_log( d, "Changed hitpoints to %d", victim->max_hit );
         break;

      case MEDIT_MANA:
         victim->max_mana = URANGE( 1, atoi( arg ), 30000 );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->max_mana = victim->max_mana;
         olc_log( d, "Changed mana to %d", victim->max_mana );
         break;

      case MEDIT_MOVE:
         victim->max_move = URANGE( 1, atoi( arg ), 30000 );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->max_move = victim->max_move;
         olc_log( d, "Changed moves to %d", victim->max_move );
         break;

      case MEDIT_SEX:
         victim->sex = URANGE( 0, atoi( arg ), 2 );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->sex = victim->sex;
         olc_log( d, "Changed sex to %s", victim->sex == 1 ? "Male" : victim->sex == 2 ? "Female" : "Neutral" );
         break;

      case MEDIT_HITROLL:
         victim->hitroll = URANGE( 0, atoi( arg ), 85 );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->hitroll = victim->hitroll;
         olc_log( d, "Changed hitroll to %d", victim->hitroll );
         break;

      case MEDIT_DAMROLL:
         victim->damroll = URANGE( 0, atoi( arg ), 65 );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->damroll = victim->damroll;
         olc_log( d, "Changed damroll to %d", victim->damroll );
         break;

      case MEDIT_DAMNUMDIE:
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->damnodice = URANGE( 0, atoi( arg ), 100 );
         olc_log( d, "Changed damnumdie to %d", victim->pIndexData->damnodice );
         break;

      case MEDIT_DAMSIZEDIE:
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->damsizedice = URANGE( 0, atoi( arg ), 100 );
         olc_log( d, "Changed damsizedie to %d", victim->pIndexData->damsizedice );
         break;

      case MEDIT_DAMPLUS:
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->damplus = URANGE( 0, atoi( arg ), 1000 );
         olc_log( d, "Changed damplus to %d", victim->pIndexData->damplus );
         break;

      case MEDIT_HITPLUS:
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->hitplus = URANGE( 0, atoi( arg ), 32767 );
         olc_log( d, "Changed hitplus to %d", victim->pIndexData->hitplus );
         break;

      case MEDIT_AC:
         victim->armor = URANGE( -300, atoi( arg ), 300 );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->ac = victim->armor;
         olc_log( d, "Changed armor to %d", victim->armor );
         break;

      case MEDIT_GOLD:
         victim->gold = URANGE( -1, atoi( arg ), 2000000000 );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->gold = victim->gold;
         olc_log( d, "Changed gold to %d", victim->gold );
         break;

      case MEDIT_POS:
         victim->position = URANGE( 0, atoi( arg ), POS_STANDING );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->position = victim->position;
         olc_log( d, "Changed position to %d", victim->position );
         break;

      case MEDIT_DEFPOS:
         victim->defposition = URANGE( 0, atoi( arg ), POS_STANDING );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->defposition = victim->defposition;
         olc_log( d, "Changed default position to %d", victim->defposition );
         break;

      case MEDIT_MENTALSTATE:
         victim->mental_state = URANGE( -100, atoi( arg ), 100 );
         olc_log( d, "Changed mental state to %d", victim->mental_state );
         break;

      case MEDIT_CLASS:
         number = atoi( arg );
         if( IS_NPC( victim ) )
         {
            victim->Class = URANGE( 0, number, MAX_NPC_CLASS - 1 );
            if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
               victim->pIndexData->Class = victim->Class;
            break;
         }
         victim->Class = URANGE( 0, number, MAX_CLASS );
         olc_log( d, "Changed Class to %s", npc_class[victim->Class] );
         break;

      case MEDIT_RACE:
         number = atoi( arg );
         if( IS_NPC( victim ) )
         {
            victim->race = URANGE( 0, number, MAX_NPC_RACE - 1 );
            if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
               victim->pIndexData->race = victim->race;
            break;
         }
         victim->race = URANGE( 0, number, MAX_RACE - 1 );
         olc_log( d, "Changed race to %s", npc_race[victim->race] );
         break;

      case MEDIT_PARTS:
         number = atoi( arg );
         if( number < 0 || number > MAX_BPART )
         {
            send_to_char( "Invalid part, try again: ", d->character );
            return;
         }
         else
         {
            if( number == 0 )
               break;
            else
            {
               number -= 1;
               TOGGLE_BIT( victim->xflags, 1 << number );
            }
            if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
               victim->pIndexData->xflags = victim->xflags;
         }
         olc_log( d, "%s the body part %s", IS_SET( victim->xflags, 1 << ( number - 1 ) ) ? "Added" : "Removed",
                  part_flags[number] );
         medit_disp_parts( d );
         return;

      case MEDIT_ATTACK:
         if( is_number( arg ) )
         {
            number = atoi( arg );
            if( number == 0 )
               break;

            number -= 1;   /* offset */
            if( number < 0 || number > MAX_ATTACK_TYPE )
            {
               send_to_char( "Invalid flag, try again: ", d->character );
               return;
            }
            else
               xTOGGLE_BIT( victim->attacks, number );
         }
         else
         {
            while( arg[0] != '\0' )
            {
               arg = one_argument( arg, arg1 );
               number = get_attackflag( arg1 );
               if( number < 0 )
               {
                  send_to_char( "Invalid flag, try again: ", d->character );
                  return;
               }
               xTOGGLE_BIT( victim->attacks, number );
            }
         }
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->attacks = victim->attacks;
         medit_disp_attack_menu( d );
         olc_log( d, "%s the attack %s", IS_ATTACK( victim, number ) ? "Added" : "Removed", attack_flags[number] );
         return;

      case MEDIT_DEFENSE:
         if( is_number( arg ) )
         {
            number = atoi( arg );
            if( number == 0 )
               break;

            number -= 1;   /* offset */
            if( number < 0 || number > MAX_DEFENSE_TYPE )
            {
               send_to_char( "Invalid flag, try again: ", d->character );
               return;
            }
            else
               xTOGGLE_BIT( victim->defenses, number );
         }
         else
         {
            while( arg[0] != '\0' )
            {
               arg = one_argument( arg, arg1 );
               number = get_defenseflag( arg1 );
               if( number < 0 )
               {
                  send_to_char( "Invalid flag, try again: ", d->character );
                  return;
               }
               xTOGGLE_BIT( victim->defenses, number );
            }
         }
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->defenses = victim->defenses;
         medit_disp_defense_menu( d );
         olc_log( d, "%s the attack %s", IS_DEFENSE( victim, number ) ? "Added" : "Removed", defense_flags[number] );
         return;

      case MEDIT_LEVEL:
         victim->level = URANGE( 1, atoi( arg ), LEVEL_IMMORTAL + 5 );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->level = victim->level;
         olc_log( d, "Changed level to %d", victim->level );
         break;

      case MEDIT_ALIGNMENT:
         victim->alignment = URANGE( -1000, atoi( arg ), 1000 );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->alignment = victim->alignment;
         olc_log( d, "Changed alignment to %d", victim->alignment );
         break;

      case MEDIT_EXP:
         victim->exp = atoi( arg );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->exp = victim->exp;
         if( victim->exp == -1 )
            victim->exp = mob_xp( victim );
         olc_log( d, "Changed exp to %d", victim->exp );
         break;

      case MEDIT_THACO:
         victim->mobthac0 = atoi( arg );
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->mobthac0 = victim->mobthac0;
         olc_log( d, "Changed thac0 to %d", victim->mobthac0 );
         break;

      case MEDIT_RESISTANT:
         if( is_number( arg ) )
         {
            number = atoi( arg );
            if( number == 0 )
               break;

            number -= 1;   /* offset */
            if( number < 0 || number > MAX_RIS_FLAG + 1 )
            {
               send_to_char( "Invalid flag, try again: ", d->character );
               return;
            }
            xTOGGLE_BIT( victim->resistant, number );
         }
         else
         {
            while( arg[0] != '\0' )
            {
               arg = one_argument( arg, arg1 );
               number = get_risflag( arg1 );
               if( number < 0 )
               {
                  send_to_char( "Invalid flag, try again: ", d->character );
                  return;
               }
               xTOGGLE_BIT( victim->resistant, number );
            }
         }
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->resistant = victim->resistant;
         medit_disp_ris( d );
         olc_log( d, "%s the resistant %s", IS_RESIS( victim, number ) ? "Added" : "Removed", ris_flags[number] );
         return;

      case MEDIT_IMMUNE:
         if( is_number( arg ) )
         {
            number = atoi( arg );
            if( number == 0 )
               break;

            number -= 1;
            if( number < 0 || number > MAX_RIS_FLAG + 1 )
            {
               send_to_char( "Invalid flag, try again: ", d->character );
               return;
            }
            xTOGGLE_BIT( victim->immune, number );
         }
         else
         {
            while( arg[0] != '\0' )
            {
               arg = one_argument( arg, arg1 );
               number = get_risflag( arg1 );
               if( number < 0 )
               {
                  send_to_char( "Invalid flag, try again: ", d->character );
                  return;
               }
               xTOGGLE_BIT( victim->immune, number );
            }
         }
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->immune = victim->immune;

         medit_disp_ris( d );
         olc_log( d, "%s the immune %s", IS_IMMUNE( victim, number ) ? "Added" : "Removed", ris_flags[number] );
         return;

      case MEDIT_SUSCEPTIBLE:
         if( is_number( arg ) )
         {
            number = atoi( arg );
            if( number == 0 )
               break;

            number -= 1;
            if( number < 0 || number > MAX_RIS_FLAG + 1 )
            {
               send_to_char( "Invalid flag, try again: ", d->character );
               return;
            }
            xTOGGLE_BIT( victim->susceptible, number );
         }
         else
         {
            while( arg[0] != '\0' )
            {
               arg = one_argument( arg, arg1 );
               number = get_risflag( arg1 );
               if( number < 0 )
               {
                  send_to_char( "Invalid flag, try again: ", d->character );
                  return;
               }
               xTOGGLE_BIT( victim->susceptible, number );
            }
         }
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->susceptible = victim->susceptible;
         medit_disp_ris( d );
         olc_log( d, "%s the suscept %s", IS_SUSCEP( victim, number ) ? "Added" : "Removed", ris_flags[number] );
         return;

      case MEDIT_ABSORB:
         if( is_number( arg ) )
         {
            number = atoi( arg );
            if( number == 0 )
               break;

            number -= 1;   /* offset */
            if( number < 0 || number > MAX_RIS_FLAG + 1 )
            {
               send_to_char( "Invalid flag, try again: ", d->character );
               return;
            }
            xTOGGLE_BIT( victim->absorb, number );
         }
         else
         {
            while( arg[0] != '\0' )
            {
               arg = one_argument( arg, arg1 );
               number = get_risflag( arg1 );
               if( number < 0 )
               {
                  send_to_char( "Invalid flag, try again: ", d->character );
                  return;
               }
               xTOGGLE_BIT( victim->absorb, number );
            }
         }
         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
            victim->pIndexData->absorb = victim->absorb;
         medit_disp_ris( d );
         olc_log( d, "%s the absorb %s", IS_ABSORB( victim, number ) ? "Added" : "Removed", ris_flags[number] );
         return;

      case MEDIT_SPEC:
         number = atoi( arg );
         /*
          * FIX: Selecting 0 crashed the mud when editing spec procs --Zarius 5/19/2003 
          */
         if( number == 0 )
            break;
         if( number < 1 || number >= SPEC_MAX )
         {
            victim->spec_fun = NULL;
            STRFREE( victim->spec_funname );
         }
         else
         {
            victim->spec_fun = m_spec_lookup( specmenu[number] );
            STRFREE( victim->spec_funname );
            victim->spec_funname = STRALLOC( specmenu[number] );
         }

         if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
         {
            victim->pIndexData->spec_fun = victim->spec_fun;
            STRFREE( victim->pIndexData->spec_funname );
            victim->pIndexData->spec_funname = STRALLOC( victim->spec_funname );
         }
         olc_log( d, "Changes spec_func to %s", victim->spec_funname ? victim->spec_funname : "None" );
         break;

/*-------------------------------------------------------------------*/
      default:
         /*
          * . We should never get here .
          */
         bug( "%s", "OLC: medit_parse(): Reached default case!" );
         cleanup_olc( d );
         return;;
   }
/*-------------------------------------------------------------------*/
/*. END OF CASE 
    If we get here, we have probably changed something, and now want to
    return to main menu.  Use OLC_CHANGE as a 'has changed' flag .*/

   OLC_CHANGE( d ) = TRUE;
   medit_disp_menu( d );
}

/*. End of medit_parse() .*/
