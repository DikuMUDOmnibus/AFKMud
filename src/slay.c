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
 *         Slay V2.0 - Online editable configurable slay options            *
 ****************************************************************************/

/* -----------------------------------------------------------------------
The following snippet was written by Gary McNickle (dharvest) for
Rom 2.4 specific MUDs and is released into the public domain. My thanks to
the originators of Diku, and Rom, as well as to all those others who have
released code for this mud base.  Goes to show that the freeware idea can
actually work. ;)  In any case, all I ask is that you credit this code
properly, and perhaps drop me a line letting me know it's being used.

from: gary@dharvest.com
website: http://www.dharvest.com
or http://www.dharvest.com/resource.html (rom related)

Ported to Smaug 1.02a code by Samson
Updated to Smaug 1.4a code by Samson

Send any comments, flames, bug-reports, suggestions, requests, etc... 
to the above email address.
----------------------------------------------------------------------- */

#include "mud.h"
#include "slay.h"

void raw_kill( CHAR_DATA * ch, CHAR_DATA * victim );

SLAY_DATA *first_slay;
SLAY_DATA *last_slay;

SLAY_DATA *get_slay( char *name )
{
   SLAY_DATA *slay;
   for( slay = first_slay; slay; slay = slay->next )
      if( !str_cmp( name, slay->type ) )
         return slay;
   return NULL;
}

/* Online editing of slays added by Samson 8-3-98 */
/* Read in an individual slaytype */
void fread_slay( SLAY_DATA * slay, FILE * fp )
{
   const char *word;
   bool fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'C':
            KEY( "Cmessage", slay->cmsg, fread_string_nohash( fp ) );
            KEY( "Color", slay->color, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !slay->owner )
                  slay->owner = STRALLOC( "Any" );
               return;
            }
            break;

         case 'O':
            KEY( "Owner", slay->owner, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Rmessage", slay->rmsg, fread_string_nohash( fp ) );
            break;

         case 'T':
            KEY( "Type", slay->type, fread_string( fp ) );
            break;

         case 'V':
            KEY( "Vmessage", slay->vmsg, fread_string_nohash( fp ) );
            break;
      }
      if( !fMatch )
         bug( "Fread_slay: no match: %s", word );
   }
}

/* Load the slay file */
void load_slays( void )
{
   char filename[256];
   SLAY_DATA *slay;
   FILE *fp;
   int slaycount;

   first_slay = NULL;
   last_slay = NULL;

   snprintf( filename, 256, "%s%s", SYSTEM_DIR, SLAY_FILE );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
      slaycount = 0;
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "%s", "Load_slay_file: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "SLAY" ) )
         {
            if( slaycount >= MAX_SLAY_TYPES )
            {
               bug( "load_slays: more slaytypes than MAX_SLAY_TYPES %d", MAX_SLAY_TYPES );
               FCLOSE( fp );
               return;
            }
            CREATE( slay, SLAY_DATA, 1 );
            fread_slay( slay, fp );
            slaycount++;
            LINK( slay, first_slay, last_slay, next, prev );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "Load_slay_file: bad section: %s", word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   return;
}

/* Online slay editing, save the slay table to disk - Samson 8-3-98 */
void save_slays( void )
{
   SLAY_DATA *tslay;
   FILE *fp;
   char filename[256];

   snprintf( filename, 256, "%s%s", SYSTEM_DIR, SLAY_FILE );

   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s", "save_slay: fopen" );
      perror( filename );
   }
   else
   {
      for( tslay = first_slay; tslay; tslay = tslay->next )
      {
         fprintf( fp, "%s", "#SLAY\n" );
         fprintf( fp, "Type		%s~\n", tslay->type );
         fprintf( fp, "Owner		%s~\n", tslay->owner );
         fprintf( fp, "Color		%d\n", tslay->color );
         fprintf( fp, "Cmessage	%s~\n", tslay->cmsg );
         fprintf( fp, "Vmessage	%s~\n", tslay->vmsg );
         fprintf( fp, "Rmessage	%s~\n", tslay->rmsg );
         fprintf( fp, "%s", "End\n\n" );
      }
      fprintf( fp, "%s", "#END\n" );
      FCLOSE( fp );
   }
   return;
}

/** Function: do_slay
  * Descr   : Slays (kills) a player, optionally sending one of several
  *           predefined "slay option" messages to those involved.
  * Returns : (void)
  * Syntax  : slay (who) [option]
  * Written : v1.0 12/97
  * Author  : Gary McNickle <gary@dharvest.com>
  * Ported to Smaug 1.02a by: Samson
  * Updated to work with Smaug 1.4 by Samson 8-3-98
  * v2.0 added support for online editing
  */
CMDF do_slay( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   SLAY_DATA *slay;
   char who[MIL];
   bool found = FALSE;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs can't use the slay command.\n\r", ch );
      return;
   }

   argument = one_argument( argument, who );

   if( !who || who[0] == '\0' || !str_prefix( who, "list" ) )
   {
      send_to_pager( "&gSyntax: slay <victim> [type]\n\r", ch );
      send_to_pager( "Where type is one of the following...\n\r\n\r", ch );
      send_to_pager( "&YSlay 			  &ROwner\n\r", ch );
      send_to_pager( "&g-------------------------+---------------\n\r", ch );
      for( slay = first_slay; slay; slay = slay->next )
         pager_printf( ch, "&G%-14s	&g%13s\n\r", slay->type, slay->owner );

      send_to_char( "\n\r&gTyping just 'slay <player>' will work too...\n\r", ch );
      return;
   }

   if( !( victim = get_char_room( ch, who ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( ch == victim )
   {
      send_to_char( "Suicide is a mortal sin.\n\r", ch );
      return;
   }

   if( !IS_NPC( victim ) && victim->level > ch->level )
   {
      send_to_char( "You cannot slay someone who is above your level.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      act( AT_IMMORT, "You brutally slay $N!", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n chops you up into little pieces!", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n brutally slays $N!", ch, NULL, victim, TO_NOTVICT );
      raw_kill( ch, victim );
      return;
   }
   else
   {
      for( slay = first_slay; slay; slay = slay->next )
      {
         if( ( !str_cmp( argument, slay->type ) && !str_cmp( "Any", slay->owner ) )
             || ( !str_cmp( slay->owner, ch->name ) && !str_cmp( argument, slay->type ) ) )
         {
            found = TRUE;
            act( slay->color, slay->cmsg, ch, NULL, victim, TO_CHAR );
            act( slay->color, slay->vmsg, ch, NULL, victim, TO_VICT );
            act( slay->color, slay->rmsg, ch, NULL, victim, TO_NOTVICT );
            raw_kill( ch, victim );
            return;
         }
      }
   }

   if( !found )
      send_to_char
         ( "&RSlay type not defined, or not owned by you. Type \"slay list\" for a complete listing of types available to you.\n\r",
           ch );
   return;
}

/* Create a slaytype online - Samson 8-3-98 */
CMDF do_makeslay( CHAR_DATA * ch, char *argument )
{
   SLAY_DATA *slay;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: makeslay <slaytype>\n\r", ch );
      return;
   }

   smash_tilde( argument );

   /*
    * Glaring oversight just noticed - Samson 7-5-99 
    */
   if( ( slay = get_slay( argument ) ) != NULL )
   {
      send_to_char( "That slay type already exists.\n\r", ch );
      return;
   }

   CREATE( slay, SLAY_DATA, 1 );
   LINK( slay, first_slay, last_slay, next, prev );
   slay->type = STRALLOC( argument );
   slay->owner = STRALLOC( "Any" );
   slay->color = AT_IMMORT;
   slay->cmsg = str_dup( "You brutally slay $N!" );
   slay->vmsg = str_dup( "$n chops you up into little pieces!" );
   slay->rmsg = str_dup( "$n brutally slays $N!" );
   ch_printf( ch, "New slaytype %s added. Set to default values.\n\r", slay->type );
   save_slays(  );
   return;
}

/* Set slay values online - Samson 8-3-98 */
CMDF do_setslay( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   SLAY_DATA *slay;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_RESTRICTED:
         send_to_char( "You cannot do this while in another command.\n\r", ch );
         return;

      case SUB_SLAYCMSG:
         slay = ( SLAY_DATA * ) ch->pcdata->dest_buf;
         DISPOSE( slay->cmsg );
         slay->cmsg = copy_buffer_nohash( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         save_slays(  );
         return;
      case SUB_SLAYVMSG:
         slay = ( SLAY_DATA * ) ch->pcdata->dest_buf;
         DISPOSE( slay->vmsg );
         slay->vmsg = copy_buffer_nohash( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         save_slays(  );
         return;
      case SUB_SLAYRMSG:
         slay = ( SLAY_DATA * ) ch->pcdata->dest_buf;
         DISPOSE( slay->rmsg );
         slay->rmsg = copy_buffer_nohash( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         save_slays(  );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting slay description.\n\r", ch );
         return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' )
   {
      send_to_char( "Usage: setslay <slaytype> <field> <value>\n\r", ch );
      send_to_char( "Usage: setslay save\n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( "owner color cmsg vmsg rmsg\n\r", ch );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      save_slays(  );
      send_to_char( "Slay table saved.\n\r", ch );
      return;
   }

   if( ( slay = get_slay( arg1 ) ) == NULL )
   {
      send_to_char( "No such slaytype.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "owner" ) )
   {
      STRFREE( slay->owner );
      slay->owner = STRALLOC( argument );
      send_to_char( "New owner set.\n\r", ch );
      save_slays(  );
      return;
   }

   if( !str_cmp( arg2, "cmsg" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_SLAYCMSG;
      ch->pcdata->dest_buf = slay;
      if( !slay->cmsg || slay->cmsg[0] == '\0' )
         slay->cmsg = str_dup( "" );
      start_editing( ch, slay->cmsg );
      set_editor_desc( ch, "A custom slay message." );
      return;
   }

   if( !str_cmp( arg2, "vmsg" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_SLAYVMSG;
      ch->pcdata->dest_buf = slay;
      if( !slay->vmsg || slay->vmsg[0] == '\0' )
         slay->vmsg = str_dup( "" );
      start_editing( ch, slay->vmsg );
      set_editor_desc( ch, "A custom slay message." );
      return;
   }

   if( !str_cmp( arg2, "rmsg" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_SLAYRMSG;
      ch->pcdata->dest_buf = slay;
      if( !slay->rmsg || slay->rmsg[0] == '\0' )
         slay->rmsg = str_dup( "" );
      start_editing( ch, slay->rmsg );
      set_editor_desc( ch, "A custom slay message." );
      return;
   }

   if( !str_cmp( arg2, "color" ) )
   {
      slay->color = atoi( argument );
      send_to_char( "Slay color set.\n\r", ch );
      save_slays(  );
      return;
   }

   do_setslay( ch, "" );
   return;
}

/* Online slay editor, show details of a slaytype - Samson 8-3-98 */
CMDF do_showslay( CHAR_DATA * ch, char *argument )
{
   SLAY_DATA *slay;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: showslay <slaytype>\n\r", ch );
      return;
   }

   if( !( slay = get_slay( argument ) ) )
   {
      send_to_char( "No such slaytype.\n\r", ch );
      return;
   }

   ch_printf( ch, "\n\rSlaytype: %s\n\r", slay->type );
   ch_printf( ch, "Owner:    %s\n\r", slay->owner );
   ch_printf( ch, "Color:    %d\n\r", slay->color );
   ch_printf( ch, "&RCmessage: \n\r%s", slay->cmsg );
   ch_printf( ch, "\n\r&YVmessage: \n\r%s", slay->vmsg );
   ch_printf( ch, "\n\r&GRmessage: \n\r%s", slay->rmsg );

   return;
}

void free_slay( SLAY_DATA * pslay )
{
   UNLINK( pslay, first_slay, last_slay, next, prev );
   STRFREE( pslay->owner );
   STRFREE( pslay->type );
   DISPOSE( pslay->cmsg );
   DISPOSE( pslay->vmsg );
   DISPOSE( pslay->rmsg );
   DISPOSE( pslay );
   return;
}

void free_slays( void )
{
   SLAY_DATA *slay, *slay_next;

   for( slay = first_slay; slay; slay = slay_next )
   {
      slay_next = slay->next;
      free_slay( slay );
   }
   return;
}

/* Of course, to create means you need to be able to destroy as well :P - Samson 8-3-98 */
CMDF do_destroyslay( CHAR_DATA * ch, char *argument )
{
   SLAY_DATA *pslay;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Destroy which slaytype?\n\r", ch );
      return;
   }

   for( pslay = first_slay; pslay; pslay = pslay->next )
   {
      if( !str_cmp( argument, pslay->type ) )
      {
         free_slay( pslay );
         ch_printf( ch, "Slaytype \"%s\" has beed deleted.\n\r", argument );
         save_slays(  );
         return;
      }
   }

   send_to_char( "No such slaytype.\n\r", ch );
   return;
}
