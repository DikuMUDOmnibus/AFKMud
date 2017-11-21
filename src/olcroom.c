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
 *                      Room editing module (redit.c) v1.0                *
 *                                                                        *
\**************************************************************************/

#include <stdarg.h>
#include "mud.h"
#include "overland.h"

/* externs */
extern int top_ed;

void redit_disp_extradesc_menu( DESCRIPTOR_DATA * d );
void redit_disp_exit_menu( DESCRIPTOR_DATA * d );
void redit_disp_exit_flag_menu( DESCRIPTOR_DATA * d );
void redit_disp_flag_menu( DESCRIPTOR_DATA * d );
void redit_disp_sector_menu( DESCRIPTOR_DATA * d );
void redit_disp_menu( DESCRIPTOR_DATA * d );
void redit_parse( DESCRIPTOR_DATA * d, char *arg );
void cleanup_olc( DESCRIPTOR_DATA * d );
void oedit_disp_extra_choice( DESCRIPTOR_DATA * d );
EXIT_DATA *get_exit_num( ROOM_INDEX_DATA * room, short count );

CMDF do_oredit( CHAR_DATA * ch, char *argument )
{
   DESCRIPTOR_DATA *d;
   ROOM_INDEX_DATA *room;

   if( IS_NPC( ch ) || !ch->desc )
   {
      send_to_char( "I don't think so...\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
      room = ch->in_room;
   else
   {
      if( is_number( argument ) )
         room = get_room_index( atoi( argument ) );
      else
      {
         send_to_char( "Vnum must be specified in numbers!\n\r", ch );
         return;
      }
   }

   if( !room )
   {
      send_to_char( "That room does not exist!\n\r", ch );
      return;
   }

   /*
    * Make sure the room isnt already being edited 
    */
   for( d = first_descriptor; d; d = d->next )
      if( d->connected == CON_REDIT )
         if( d->olc && OLC_VNUM( d ) == room->vnum )
         {
            ch_printf( ch, "That room is currently being edited by %s.\n\r", d->character->name );
            return;
         }

   if( !can_rmodify( ch, room ) )
      return;

   d = ch->desc;
   CREATE( d->olc, OLC_DATA, 1 );
   OLC_VNUM( d ) = room->vnum;
   OLC_CHANGE( d ) = FALSE;
   d->character->pcdata->dest_buf = room;
   d->connected = CON_REDIT;
   redit_disp_menu( d );

   act( AT_ACTION, "$n starts using OLC.", ch, NULL, NULL, TO_ROOM );
   return;
}

CMDF do_rcopy( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   int rvnum, cvnum;
   ROOM_INDEX_DATA *orig;
   ROOM_INDEX_DATA *copy;
   EXTRA_DESCR_DATA *ed, *ced;
   EXIT_DATA *xit, *cxit;
   int iHash;

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: rcopy <original> <new>\n\r", ch );
      return;
   }

   if( !is_number( arg1 ) || !is_number( argument ) )
   {
      send_to_char( "Values must be numeric.\n\r", ch );
      return;
   }

   rvnum = atoi( arg1 );
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

   if( get_room_index( cvnum ) )
   {
      send_to_char( "Target vnum already exists.\n\r", ch );
      return;
   }

   if( ( orig = get_room_index( rvnum ) ) == NULL )
   {
      send_to_char( "Source vnum doesn't exist.\n\r", ch );
      return;
   }

   CREATE( copy, ROOM_INDEX_DATA, 1 );
   copy->first_person = NULL;
   copy->last_person = NULL;
   copy->first_content = NULL;
   copy->last_content = NULL;
   copy->first_extradesc = NULL;
   copy->last_extradesc = NULL;
   copy->area = ( ch->pcdata->area ) ? ch->pcdata->area : orig->area;
   copy->vnum = cvnum;
   copy->name = QUICKLINK( orig->name );
   copy->roomdesc = str_dup( orig->roomdesc );
   copy->nitedesc = str_dup( orig->nitedesc );
   copy->room_flags = orig->room_flags;
   copy->sector_type = orig->sector_type;
   copy->light = 0;
   copy->first_exit = NULL;
   copy->last_exit = NULL;
   copy->tele_vnum = orig->tele_vnum;
   copy->tele_delay = orig->tele_delay;
   copy->tunnel = orig->tunnel;

   for( ced = orig->first_extradesc; ced; ced = ced->next )
   {
      CREATE( ed, EXTRA_DESCR_DATA, 1 );
      ed->keyword = QUICKLINK( ced->keyword );
      if( ced->extradesc && ced->extradesc[0] != '\0' )
         ed->extradesc = QUICKLINK( ced->extradesc );
      LINK( ed, copy->first_extradesc, copy->last_extradesc, next, prev );
      top_ed++;
   }

   for( cxit = orig->first_exit; cxit; cxit = cxit->next )
   {
      xit = make_exit( copy, get_room_index( cxit->rvnum ), cxit->vdir );
      xit->keyword = QUICKLINK( cxit->keyword );
      if( cxit->exitdesc && cxit->exitdesc[0] != '\0' )
         xit->exitdesc = QUICKLINK( cxit->exitdesc );
      xit->key = cxit->key;
      xit->exit_info = cxit->exit_info;
   }

   iHash = cvnum % MAX_KEY_HASH;
   copy->next = room_index_hash[iHash];
   room_index_hash[iHash] = copy;
   top_room++;
   send_to_char( "Room copied.\n\r", ch );
   return;
}

bool is_inolc( DESCRIPTOR_DATA * d )
{
   /*
    * safeties, not that its necessary really... 
    */
   if( !d || !d->character )
      return FALSE;

   if( IS_NPC( d->character ) )
      return FALSE;

   /*
    * objs 
    */
   if( d->connected == CON_OEDIT )
      return TRUE;

   /*
    * mobs 
    */
   if( d->connected == CON_MEDIT )
      return TRUE;

   /*
    * rooms 
    */
   if( d->connected == CON_REDIT )
      return TRUE;

   return FALSE;
}

/*
 * Log all changes to catch those sneaky bastards =)
 */
void olc_log( DESCRIPTOR_DATA * d, const char *format, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );

void olc_log( DESCRIPTOR_DATA * d, const char *format, ... )
{
   ROOM_INDEX_DATA *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
   OBJ_DATA *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
   CHAR_DATA *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
   char logline[MSL], lbuf[MSL];
   va_list args;

   if( !d )
   {
      bug( "%s", "olc_log: called with null descriptor" );
      return;
   }

   va_start( args, format );
   vsnprintf( logline, MSL, format, args );
   va_end( args );

   if( d->connected == CON_REDIT )
      snprintf( lbuf, MSL, "OLCLog: %s ROOM(%d): ", d->character->name, room->vnum );

   else if( d->connected == CON_OEDIT )
      snprintf( lbuf, MSL, "OLCLog: %s OBJ(%d): ", d->character->name, obj->pIndexData->vnum );

   else if( d->connected == CON_MEDIT )
   {
      if( IS_NPC( victim ) )
         snprintf( lbuf, MSL, "OLCLog: %s MOB(%d): ", d->character->name, victim->pIndexData->vnum );
      else
         snprintf( lbuf, MSL, "OLCLog: %s PLR(%s): ", d->character->name, victim->name );
   }
   else
   {
      bug( "%s: called with a bad connected state", __FUNCTION__ );
      return;
   }
   log_printf_plus( LOG_BUILD, sysdata.build_level, "%s%s", lbuf, logline );
   return;
}

/**************************************************************************
  Menu functions 
 **************************************************************************/

/*
 * Nice fancy redone Extra Description stuff :)
 */
void redit_disp_extradesc_prompt_menu( DESCRIPTOR_DATA * d )
{
   EXTRA_DESCR_DATA *ed;
   ROOM_INDEX_DATA *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
   int counter = 0;

   for( ed = room->first_extradesc; ed; ed = ed->next )
      ch_printf( d->character, "&g%2d&w) %-40.40s\n\r", counter++, ed->keyword );
   send_to_char( "\n\rWhich extra description do you want to edit? ", d->character );
}

void redit_disp_extradesc_menu( DESCRIPTOR_DATA * d )
{
   ROOM_INDEX_DATA *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
   int count = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   if( room->first_extradesc )
   {
      EXTRA_DESCR_DATA *ed;

      for( ed = room->first_extradesc; ed; ed = ed->next )
         ch_printf( d->character, "&g%2d&w) Keyword: &O%s\n\r", ++count, ed->keyword );

      send_to_char( "\n\r", d->character );
   }

   send_to_char( "&gA&w) Add a new description\n\r", d->character );
   send_to_char( "&gR&w) Remove a description\n\r", d->character );
   send_to_char( "&gQ&w) Quit\n\r", d->character );
   send_to_char( "\n\rEnter choice: ", d->character );

   OLC_MODE( d ) = REDIT_EXTRADESC_MENU;
}

/* For exits */
void redit_disp_exit_menu( DESCRIPTOR_DATA * d )
{
   ROOM_INDEX_DATA *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
   EXIT_DATA *pexit;
   int cnt;

   OLC_MODE( d ) = REDIT_EXIT_MENU;
   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( cnt = 0, pexit = room->first_exit; pexit; pexit = pexit->next )
   {
      ch_printf( d->character, "&g%2d&w) %-10.10s to %-5d. Key: %d Keywords: %s Flags: %s\n\r",
                 ++cnt, dir_name[pexit->vdir], pexit->to_room ? pexit->to_room->vnum : 0,
                 pexit->key, ( pexit->keyword && pexit->keyword[0] != '\0' ) ? pexit->keyword : "(none)",
                 ext_flag_string( &pexit->exit_info, ex_flags ) );
   }

   if( room->first_exit )
      send_to_char( "\n\r", d->character );
   send_to_char( "&gA&w) Add a new exit\n\r", d->character );
   send_to_char( "&gR&w) Remove an exit\n\r", d->character );
   send_to_char( "&gQ&w) Quit\n\r", d->character );

   send_to_char( "\n\rEnter choice: ", d->character );
   return;
}

void redit_disp_exit_edit( DESCRIPTOR_DATA * d )
{
   char flags[MSL];
   EXIT_DATA *pexit = ( EXIT_DATA * ) d->character->pcdata->spare_ptr;
   int i;

   flags[0] = '\0';
   for( i = 0; i < MAX_EXFLAG; i++ )
   {
      if( IS_EXIT_FLAG( pexit, i ) )
      {
         mudstrlcat( flags, ex_flags[i], MSL );
         mudstrlcat( flags, " ", MSL );
      }
   }

   OLC_MODE( d ) = REDIT_EXIT_EDIT;
   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   ch_printf( d->character, "&g1&w) Direction  : &c%s\n\r", dir_name[pexit->vdir] );
   ch_printf( d->character, "&g2&w) To Vnum    : &c%d\n\r", pexit->to_room ? pexit->to_room->vnum : -1 );
   ch_printf( d->character, "&g3&w) Key        : &c%d\n\r", pexit->key );
   ch_printf( d->character, "&g4&w) Keyword    : &c%s\n\r",
              ( pexit->keyword && pexit->keyword[0] != '\0' ) ? pexit->keyword : "(none)" );
   ch_printf( d->character, "&g5&w) Flags      : &c%s\n\r", flags[0] != '\0' ? flags : "(none)" );
   ch_printf( d->character, "&g6&w) Description: &c%s\n\r",
              ( pexit->exitdesc && pexit->exitdesc[0] != '\0' ) ? pexit->exitdesc : "(none)" );
   send_to_char( "&gQ&w) Quit\n\r", d->character );
   send_to_char( "\n\rEnter choice: ", d->character );
   return;
}

void redit_disp_exit_dirs( DESCRIPTOR_DATA * d )
{
   int i;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( i = 0; i <= DIR_SOMEWHERE; i++ )
      ch_printf( d->character, "&g%2d&w) %s\n\r", i, dir_name[i] );

   send_to_char( "\n\rChoose a direction: ", d->character );
   return;
}

/* For exit flags */
void redit_disp_exit_flag_menu( DESCRIPTOR_DATA * d )
{
   EXIT_DATA *pexit = ( EXIT_DATA * ) d->character->pcdata->spare_ptr;
   char buf1[MSL];
   int i;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( i = 0; i < MAX_EXFLAG; i++ )
   {
      if( i == EX_PORTAL )
         continue;
      ch_printf( d->character, "&g%2d&w) %-20.20s\n\r", i + 1, ex_flags[i] );
   }
   buf1[0] = '\0';
   for( i = 0; i < MAX_EXFLAG; i++ )
      if( IS_EXIT_FLAG( pexit, i ) )
      {
         mudstrlcat( buf1, ex_flags[i], MSL );
         mudstrlcat( buf1, " ", MSL );
      }

   ch_printf( d->character, "\n\rExit flags: &c%s&w\n\rEnter room flags, 0 to quit: ", buf1 );
   OLC_MODE( d ) = REDIT_EXIT_FLAGS;
}

/* For room flags */
void redit_disp_flag_menu( DESCRIPTOR_DATA * d )
{
   ROOM_INDEX_DATA *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
   int counter, columns = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < ROOM_MAX; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter + 1, r_flags[counter] );

      if( !( ++columns % 2 ) )
         send_to_char( "\n\r", d->character );
   }
   ch_printf( d->character, "\n\rRoom flags: &c%s&w\n\rEnter room flags, 0 to quit : ",
              ext_flag_string( &room->room_flags, r_flags ) );
   OLC_MODE( d ) = REDIT_FLAGS;
}

/* for sector type */
void redit_disp_sector_menu( DESCRIPTOR_DATA * d )
{
   int counter, columns = 0;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   for( counter = 0; counter < SECT_MAX; counter++ )
   {
      ch_printf( d->character, "&g%2d&w) %-20.20s ", counter, sect_types[counter] );

      if( !( ++columns % 2 ) )
         send_to_char( "\n\r", d->character );
   }
   send_to_char( "\r\nEnter sector type : ", d->character );
   OLC_MODE( d ) = REDIT_SECTOR;
}

/* the main menu */
void redit_disp_menu( DESCRIPTOR_DATA * d )
{
   ROOM_INDEX_DATA *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;

   write_to_buffer( d, "50\x1B[;H\x1B[2J", 0 );
   ch_printf( d->character,
              "&w-- Room number : [&c%d&w]      Room area: [&c%-30.30s&w]\n\r"
              "&g1&w) Room Name   : &O%s\n\r"
              "&g2&w) Description :\n\r&O%s"
              "&g3&w) Night Desc  :\n\r&O%s"
              "&g4&w) Room flags  : &c%s\n\r"
              "&g5&w) Sector type : &c%s\n\r"
              "&g6&w) Tunnel      : &c%d\n\r"
              "&g7&w) TeleDelay   : &c%d\n\r"
              "&g8&w) TeleVnum    : &c%d\n\r"
              "&g9&w) Exit menu\n\r"
              "&gA&w) Extra descriptions menu\r\n"
              "&gQ&w) Quit\r\n"
              "Enter choice : ",
              OLC_NUM( d ), room->area ? room->area->name : "None????",
              room->name, room->roomdesc,
              room->nitedesc ? room->nitedesc : "None Set\n\r", ext_flag_string( &room->room_flags, r_flags ),
              sect_types[room->sector_type], room->tunnel, room->tele_delay, room->tele_vnum );

   OLC_MODE( d ) = REDIT_MAIN_MENU;
}

EXTRA_DESCR_DATA *redit_find_extradesc( ROOM_INDEX_DATA * room, int number )
{
   int count = 0;
   EXTRA_DESCR_DATA *ed;

   for( ed = room->first_extradesc; ed; ed = ed->next )
   {
      if( ++count == number )
         return ed;
   }

   return NULL;
}

CMDF do_redit_reset( CHAR_DATA * ch, char *argument )
{
   ROOM_INDEX_DATA *room = ( ROOM_INDEX_DATA * ) ch->pcdata->dest_buf;
   EXTRA_DESCR_DATA *ed = ( EXTRA_DESCR_DATA * ) ch->pcdata->spare_ptr;

   switch ( ch->substate )
   {
      case SUB_ROOM_DESC:
         if( !ch->pcdata->dest_buf )
         {
            /*
             * If theres no dest_buf, theres no object, so stick em back as playing 
             */
            send_to_char( "Fatal error, report to Samson.\n\r", ch );
            bug( "%s", "do_redit_reset: sub_obj_extra: NULL ch->pcdata->dest_buf" );
            ch->substate = SUB_NONE;
            ch->desc->connected = CON_PLAYING;
            return;
         }
         DISPOSE( room->roomdesc );
         room->roomdesc = copy_buffer_nohash( ch );
         stop_editing( ch );
         ch->pcdata->dest_buf = room;
         ch->desc->connected = CON_REDIT;
         ch->substate = SUB_NONE;

         olc_log( ch->desc, "Edited room description" );
         redit_disp_menu( ch->desc );
         return;

      case SUB_ROOM_DESC_NITE:
         if( !ch->pcdata->dest_buf )
         {
            /*
             * If theres no dest_buf, theres no object, so stick em back as playing 
             */
            send_to_char( "Fatal error, report to Samson.\n\r", ch );
            bug( "%s", "do_redit_reset: sub_obj_extra: NULL ch->pcdata->dest_buf" );
            ch->substate = SUB_NONE;
            ch->desc->connected = CON_PLAYING;
            return;
         }
         DISPOSE( room->nitedesc );
         room->nitedesc = copy_buffer_nohash( ch );
         stop_editing( ch );
         ch->pcdata->dest_buf = room;
         ch->desc->connected = CON_REDIT;
         ch->substate = SUB_NONE;

         olc_log( ch->desc, "Edited room night description" );
         redit_disp_menu( ch->desc );
         return;

      case SUB_ROOM_EXTRA:
         STRFREE( ed->extradesc );
         ed->extradesc = copy_buffer( ch );
         stop_editing( ch );
         ch->pcdata->dest_buf = room;
         ch->pcdata->spare_ptr = ed;
         ch->substate = SUB_NONE;
         ch->desc->connected = CON_REDIT;
         oedit_disp_extra_choice( ch->desc );
         OLC_MODE( ch->desc ) = REDIT_EXTRADESC_CHOICE;
         olc_log( ch->desc, "Edit description for exdesc %s", ed->keyword );

         return;
   }
}

/**************************************************************************
  The main loop
 **************************************************************************/

void redit_parse( DESCRIPTOR_DATA * d, char *arg )
{
   ROOM_INDEX_DATA *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
   ROOM_INDEX_DATA *tmp;
   EXIT_DATA *pexit = ( EXIT_DATA * ) d->character->pcdata->spare_ptr;
   EXTRA_DESCR_DATA *ed = ( EXTRA_DESCR_DATA * ) d->character->pcdata->spare_ptr;
   char arg1[MIL];
   int number = 0;

   switch ( OLC_MODE( d ) )
   {
      case REDIT_MAIN_MENU:
         switch ( *arg )
         {
            case 'q':
            case 'Q':
               cleanup_olc( d );
               return;
            case '1':
               send_to_char( "Enter room name:-\r\n| ", d->character );
               OLC_MODE( d ) = REDIT_NAME;
               break;
            case '2':
               OLC_MODE( d ) = REDIT_DESC;
               d->character->substate = SUB_ROOM_DESC;
               d->character->last_cmd = do_redit_reset;

               send_to_char( "Enter room description:-\r\n", d->character );
               if( !room->roomdesc )
                  room->roomdesc = str_dup( "" );
               start_editing( d->character, room->roomdesc );
               break;
            case '3':
               OLC_MODE( d ) = REDIT_NDESC;
               d->character->substate = SUB_ROOM_DESC_NITE;
               d->character->last_cmd = do_redit_reset;

               send_to_char( "Enter room night description:-\r\n", d->character );
               if( !room->nitedesc )
                  room->nitedesc = str_dup( "" );
               start_editing( d->character, room->nitedesc );
               break;
            case '4':
               redit_disp_flag_menu( d );
               break;
            case '5':
               redit_disp_sector_menu( d );
               break;
            case '6':
               send_to_char( "How many people can fit in the room? ", d->character );
               OLC_MODE( d ) = REDIT_TUNNEL;
               break;
            case '7':
               send_to_char( "How long before people are teleported out? ", d->character );
               OLC_MODE( d ) = REDIT_TELEDELAY;
               break;
            case '8':
               send_to_char( "Where are they teleported to? ", d->character );
               OLC_MODE( d ) = REDIT_TELEVNUM;
               break;
            case '9':
               redit_disp_exit_menu( d );
               break;
            case 'a':
            case 'A':
               redit_disp_extradesc_menu( d );
               break;

            default:
               send_to_char( "Invalid choice!", d->character );
               redit_disp_menu( d );
               break;
         }
         return;

      case REDIT_NAME:
         STRFREE( room->name );
         room->name = STRALLOC( arg );
         olc_log( d, "Changed name to %s", room->name );
         break;

      case REDIT_DESC:
         /*
          * we will NEVER get here 
          */
         bug( "%s", "Reached REDIT_DESC case in redit_parse" );
         break;

      case REDIT_NDESC:
         /*
          * we will NEVER get here 
          */
         bug( "%s", "Reached REDIT_NDESC case in redit_parse" );
         break;

      case REDIT_FLAGS:
         if( is_number( arg ) )
         {
            number = atoi( arg );
            if( number == 0 )
               break;
            else if( number < 0 || number > ROOM_MAX )
            {
               send_to_char( "Invalid flag, try again: ", d->character );
               return;
            }
            else
            {
               number -= 1;   /* Offset for 0 */
               xTOGGLE_BIT( room->room_flags, number );
               olc_log( d, "%s the room flag %s", IS_ROOM_FLAG( room, number ) ? "Added" : "Removed", r_flags[number] );
            }
         }
         else
         {
            while( arg[0] != '\0' )
            {
               arg = one_argument( arg, arg1 );
               number = get_rflag( arg1 );
               if( number > 0 )
               {
                  xTOGGLE_BIT( room->room_flags, number );
                  olc_log( d, "%s the room flag %s", IS_ROOM_FLAG( room, number ) ? "Added" : "Removed", r_flags[number] );
               }
            }
         }
         redit_disp_flag_menu( d );
         return;

      case REDIT_SECTOR:
         number = atoi( arg );
         if( number < 0 || number >= SECT_MAX )
         {
            send_to_char( "Invalid choice!", d->character );
            redit_disp_sector_menu( d );
            return;
         }
         else
            room->sector_type = number;
         olc_log( d, "Changed sector to %s", sect_types[number] );
         break;

      case REDIT_TUNNEL:
         number = atoi( arg );
         room->tunnel = URANGE( 0, number, 1000 );
         olc_log( d, "Changed tunnel amount to %d", room->tunnel );
         break;

      case REDIT_TELEDELAY:
         number = atoi( arg );
         room->tele_delay = number;
         olc_log( d, "Changed teleportation delay to %d", room->tele_delay );
         break;

      case REDIT_TELEVNUM:
         number = atoi( arg );
         room->tele_vnum = URANGE( 1, number, sysdata.maxvnum );
         olc_log( d, "Changed teleportation vnum to %d", room->tele_vnum );
         break;

      case REDIT_EXIT_MENU:
         switch ( UPPER( arg[0] ) )
         {
            default:
               if( is_number( arg ) )
               {
                  number = atoi( arg );
                  pexit = get_exit_num( room, number );
                  if( pexit )
                  {
                     d->character->pcdata->spare_ptr = pexit;
                     redit_disp_exit_edit( d );
                     return;
                  }
               }
               redit_disp_exit_menu( d );
               return;
            case 'A':
               OLC_MODE( d ) = REDIT_EXIT_ADD;
               redit_disp_exit_dirs( d );
               return;
            case 'R':
               OLC_MODE( d ) = REDIT_EXIT_DELETE;
               send_to_char( "Delete which exit? ", d->character );
               return;
            case 'Q':
               d->character->pcdata->spare_ptr = NULL;
               break;
         }
         break;

      case REDIT_EXIT_EDIT:
         switch ( UPPER( arg[0] ) )
         {
            case 'Q':
               d->character->pcdata->spare_ptr = NULL;
               redit_disp_exit_menu( d );
               return;
            case '1':
               OLC_MODE( d ) = REDIT_EXIT_DIR;
               redit_disp_exit_dirs( d );
               return;
            case '2':
               OLC_MODE( d ) = REDIT_EXIT_VNUM;
               send_to_char( "Which room does this exit go to? ", d->character );
               return;
            case '3':
               OLC_MODE( d ) = REDIT_EXIT_KEY;
               send_to_char( "What is the vnum of the key to this exit? ", d->character );
               return;
            case '4':
               OLC_MODE( d ) = REDIT_EXIT_KEYWORD;
               send_to_char( "What is the keyword to this exit? ", d->character );
               return;
            case '5':
               OLC_MODE( d ) = REDIT_EXIT_FLAGS;
               redit_disp_exit_flag_menu( d );
               return;
            case '6':
               OLC_MODE( d ) = REDIT_EXIT_DESC;
               send_to_char( "Description:\n\r] ", d->character );
               return;
         }
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_DESC:
         if( arg && arg[0] != '\0' )
            stralloc_printf( &pexit->exitdesc, "%s\n\r", arg );

         olc_log( d, "Changed %s description to %s", dir_name[pexit->vdir], arg ? arg : "none" );
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_ADD:
         if( is_number( arg ) )
         {
            number = atoi( arg );
            if( number < DIR_NORTH || number > DIR_SOMEWHERE )
            {
               send_to_char( "Invalid direction, try again: ", d->character );
               return;
            }
            pexit = get_exit( room, number );
            if( pexit )
            {
               send_to_char( "An exit in that direction already exists, try again: ", d->character );
               return;
            }
            d->character->tempnum = number;
         }
         else
         {
            number = get_dir( arg );
            pexit = get_exit( room, number );
            if( pexit )
            {
               send_to_char( "An exit in that direction already exists, try again: ", d->character );
               return;
            }
            d->character->tempnum = number;
         }
         OLC_MODE( d ) = REDIT_EXIT_ADD_VNUM;
         send_to_char( "Which room does this exit go to? ", d->character );
         return;

      case REDIT_EXIT_DIR:
         if( is_number( arg ) )
         {
            EXIT_DATA *xit;

            number = atoi( arg );
            if( number < DIR_NORTH || number > DIR_SOMEWHERE )
            {
               send_to_char( "Invalid direction, try again: ", d->character );
               return;
            }
            xit = get_exit( room, number );
            if( xit && xit != pexit )
            {
               send_to_char( "An exit in that direction already exists, try again: ", d->character );
               return;
            }
            pexit->vdir = number;
         }
         else
         {
            EXIT_DATA *xit;

            number = get_dir( arg );
            if( number < DIR_NORTH || number > DIR_SOMEWHERE )
            {
               send_to_char( "Invalid direction, try again: ", d->character );
               return;
            }
            xit = get_exit( room, number );
            if( xit && xit != pexit )
            {
               send_to_char( "An exit in that direction already exists, try again: ", d->character );
               return;
            }
            pexit->vdir = number;
         }
         OLC_MODE( d ) = REDIT_EXIT_EDIT;
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_ADD_VNUM:
         number = atoi( arg );
         if( ( tmp = get_room_index( number ) ) == NULL )
         {
            send_to_char( "Non-existant room, try again: ", d->character );
            return;
         }
         pexit = make_exit( room, tmp, d->character->tempnum );
         pexit->key = -1;
         xCLEAR_BITS( pexit->exit_info );
         act( AT_IMMORT, "$n reveals a hidden passage!", d->character, NULL, NULL, TO_ROOM );
         d->character->pcdata->spare_ptr = pexit;

         olc_log( d, "Added %s exit to %d", dir_name[pexit->vdir], pexit->vnum );

         OLC_MODE( d ) = REDIT_EXIT_EDIT;
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_DELETE:
         if( !is_number( arg ) )
         {
            send_to_char( "Exit must be specified in a number.\n\r", d->character );
            redit_disp_exit_menu( d );
         }
         number = atoi( arg );
         pexit = get_exit_num( room, number );
         if( !pexit )
         {
            send_to_char( "That exit does not exist.\n\r", d->character );
            redit_disp_exit_menu( d );
         }
         olc_log( d, "Removed %s exit", dir_name[pexit->vdir] );
         extract_exit( room, pexit );
         redit_disp_exit_menu( d );
         return;

      case REDIT_EXIT_VNUM:
         number = atoi( arg );
         if( number < 0 || number > sysdata.maxvnum )
         {
            send_to_char( "Invalid room number, try again : ", d->character );
            return;
         }
         if( get_room_index( number ) == NULL )
         {
            send_to_char( "That room does not exist, try again: ", d->character );
            return;
         }
         /*
          * pexit->vnum = number;
          */
         pexit->to_room = get_room_index( number );

         /*
          * olc_log( d, "%s exit vnum changed to %d", dir_name[pexit->vdir], pexit->vnum );
          */
         olc_log( d, "%s exit vnum changed to %d", dir_name[pexit->vdir], pexit->to_room->vnum );
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_KEYWORD:
         STRFREE( pexit->keyword );
         pexit->keyword = STRALLOC( arg );
         olc_log( d, "Changed %s keyword to %s", dir_name[pexit->vdir], pexit->keyword );
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_KEY:
         number = atoi( arg );
         if( number < -1 || number > sysdata.maxvnum )
            send_to_char( "Invalid vnum, try again: ", d->character );
         else
         {
            pexit->key = number;
            redit_disp_exit_edit( d );
         }
         olc_log( d, "%s key vnum is now %d", dir_name[pexit->vdir], pexit->key );
         return;

      case REDIT_EXIT_FLAGS:
         number = atoi( arg );
         if( number == 0 )
         {
            redit_disp_exit_edit( d );
            return;
         }

         if( ( number < 0 ) || ( number >= MAX_EXFLAG ) || ( ( number - 1 ) == EX_PORTAL ) )
         {
            send_to_char( "That's not a valid choice!\r\n", d->character );
            redit_disp_exit_flag_menu( d );
         }
         number -= 1;
         xTOGGLE_BIT( pexit->exit_info, number );
         olc_log( d, "%s %s to %s exit",
                  IS_EXIT_FLAG( pexit, number ) ? "Added" : "Removed", ex_flags[number], dir_name[pexit->vdir] );
         redit_disp_exit_flag_menu( d );
         return;

      case REDIT_EXTRADESC_DELETE:
         ed = redit_find_extradesc( room, atoi( arg ) );
         if( !ed )
         {
            send_to_char( "Not found, try again: ", d->character );
            return;
         }
         olc_log( d, "Deleted exdesc %s", ed->keyword );
         UNLINK( ed, room->first_extradesc, room->last_extradesc, next, prev );
         STRFREE( ed->keyword );
         STRFREE( ed->extradesc );
         DISPOSE( ed );
         top_ed--;
         redit_disp_extradesc_menu( d );
         return;

      case REDIT_EXTRADESC_CHOICE:
         switch ( UPPER( arg[0] ) )
         {
            case 'Q':
               if( !ed->keyword || !ed->extradesc )
               {
                  send_to_char( "No keyword and/or description, junking...", d->character );
                  UNLINK( ed, room->first_extradesc, room->last_extradesc, next, prev );
                  STRFREE( ed->keyword );
                  STRFREE( ed->extradesc );
                  DISPOSE( ed );
                  --top_ed;
               }
               d->character->pcdata->spare_ptr = NULL;
               redit_disp_extradesc_menu( d );
               return;
            case '1':
               OLC_MODE( d ) = REDIT_EXTRADESC_KEY;
               send_to_char( "Keywords, seperated by spaces: ", d->character );
               return;
            case '2':
               OLC_MODE( d ) = REDIT_EXTRADESC_DESCRIPTION;
               d->character->substate = SUB_ROOM_EXTRA;
               d->character->last_cmd = do_redit_reset;

               send_to_char( "Enter new extradesc description: \n\r", d->character );
               if( !ed->extradesc || ed->extradesc[0] == '\0' )
                  ed->extradesc = STRALLOC( "" );
               start_editing( d->character, ed->extradesc );
               return;
         }
         break;

      case REDIT_EXTRADESC_KEY:
         /*
          * if ( SetRExtra( room, arg ) )
          * {
          * send_to_char( "A extradesc with that keyword already exists.\n\r", d->character );
          * redit_disp_extradesc_menu(d);
          * return;
          * } 
          */
         olc_log( d, "Changed exkey %s to %s", ed->keyword, arg );
         STRFREE( ed->keyword );
         ed->keyword = STRALLOC( arg );
         oedit_disp_extra_choice( d );
         OLC_MODE( d ) = REDIT_EXTRADESC_CHOICE;
         return;

      case REDIT_EXTRADESC_MENU:
         switch ( UPPER( arg[0] ) )
         {
            case 'Q':
               break;
            case 'A':
               CREATE( ed, EXTRA_DESCR_DATA, 1 );
               LINK( ed, room->first_extradesc, room->last_extradesc, next, prev );
               ++top_ed;
               d->character->pcdata->spare_ptr = ed;
               olc_log( d, "Added new exdesc" );

               oedit_disp_extra_choice( d );
               OLC_MODE( d ) = REDIT_EXTRADESC_CHOICE;
               return;
            case 'R':
               OLC_MODE( d ) = REDIT_EXTRADESC_DELETE;
               send_to_char( "Delete which extra description? ", d->character );
               return;
            default:
               if( is_number( arg ) )
               {
                  ed = redit_find_extradesc( room, atoi( arg ) );
                  if( !ed )
                  {
                     send_to_char( "Not found, try again: ", d->character );
                     return;
                  }
                  d->character->pcdata->spare_ptr = ed;
                  oedit_disp_extra_choice( d );
                  OLC_MODE( d ) = REDIT_EXTRADESC_CHOICE;
               }
               else
                  redit_disp_extradesc_menu( d );
               return;
         }
         break;

      default:
         /*
          * we should never get here 
          */
         bug( "%s", "Reached default case in parse_redit" );
         break;
   }
   /*
    * Log the changes, so we can keep track of those sneaky bastards 
    */
   /*
    * Don't log on the flags cause it does that above 
    */
   /*
    * if ( OLC_MODE(d) != REDIT_FLAGS )
    * olc_log( d, arg ); 
    */

   /*
    * . If we get this far, something has be changed .
    */
   OLC_CHANGE( d ) = TRUE;
   redit_disp_menu( d );
}
