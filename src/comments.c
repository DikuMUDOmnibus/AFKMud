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
 * Comments: 'notes' attached to players to keep track of outstanding       * 
 *           and problem players.  -haus 6/25/1995                          * 
 ****************************************************************************/

#include <ctype.h>
#include "mud.h"
#include "boards.h"

void note_attach( CHAR_DATA * ch );
void free_note( NOTE_DATA * pnote );
void fwrite_note( NOTE_DATA * pnote, FILE * fp );
NOTE_DATA *read_note( FILE * fp );
void note_to_char( CHAR_DATA * ch, NOTE_DATA * pnote, BOARD_DATA * board, short id );
char *mini_c_time( time_t curtime, int tz );

void free_comments( CHAR_DATA * ch )
{
   NOTE_DATA *comment, *comment_next;

   for( comment = ch->pcdata->first_comment; comment; comment = comment_next )
   {
      comment_next = comment->next;
      UNLINK( comment, ch->pcdata->first_comment, ch->pcdata->last_comment, next, prev );
      free_note( comment );
   }
   return;
}

void comment_remove( CHAR_DATA * ch, NOTE_DATA * pnote )
{
   if( !ch )
   {
      bug( "%s: Null ch!", __FUNCTION__ );
      return;
   }

   if( !ch->pcdata->first_comment )
   {
      bug( "%s: Null %s->pcdata->first_comment!", __FUNCTION__, ch->name );
      return;
   }

   if( !pnote )
   {
      bug( "%s: Null pnote, removing comment from %s!", __FUNCTION__, ch->name );
      return;
   }

   UNLINK( pnote, ch->pcdata->first_comment, ch->pcdata->last_comment, next, prev );
   free_note( pnote );

   /*
    * Rewrite entire list.
    * Right, so you mean to tell me this gets called from here, which calls fwrite_char, which calls fwrite_comments...
    * ... Good GODS.. -- Xorith
    */
   save_char_obj( ch );
   return;
}

CMDF do_comment( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   NOTE_DATA *pnote;
   CHAR_DATA *victim;
   int vnum;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs can't use the comment command.\n\r", ch );
      return;
   }

   if( !ch->desc )
   {
      bug( "%s", "do_comment: no descriptor" );
      return;
   }

   /*
    * Put in to prevent crashing when someone issues a comment command
    * from within the editor. -Narn 
    */
   if( ch->desc->connected == CON_EDITING )
   {
      send_to_char( "You can't use the comment command from within the editor.\n\r", ch );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_WRITING_NOTE:
         if( !ch->pcdata->pnote )
         {
            bug( "%s", "do_comment: note got lost?" );
            send_to_char( "Your note got lost!\n\r", ch );
            stop_editing( ch );
            return;
         }
         if( ch->pcdata->dest_buf != ch->pcdata->pnote )
            bug( "%s", "do_comment: sub_writing_note: ch->pcdata->dest_buf != ch->pcdata->pnote" );
         DISPOSE( ch->pcdata->pnote->text );
         ch->pcdata->pnote->text = copy_buffer_nohash( ch );
         stop_editing( ch );
         return;
      case SUB_EDIT_ABORT:
         send_to_char( "Aborting note...\n\r", ch );
         ch->substate = SUB_NONE;
         if( ch->pcdata->pnote )
            free_note( ch->pcdata->pnote );
         ch->pcdata->pnote = NULL;
         return;
   }

   set_char_color( AT_NOTE, ch );
   argument = one_argument( argument, arg );
   smash_tilde( argument );

   if( !str_cmp( arg, "list" ) || !str_cmp( arg, "about" ) )
   {
      if( !( victim = get_char_world( ch, argument ) ) )
      {
         send_to_char( "They're not logged on!\n\r", ch );  /* maybe fix this? */
         return;
      }

      if( IS_NPC( victim ) )
      {
         send_to_char( "No comments about mobs\n\r", ch );
         return;
      }

      if( get_trust( victim ) >= get_trust( ch ) )
      {
         send_to_char( "You're not of the right caliber to do this...\n\r", ch );
         return;
      }

      if( !victim->pcdata->first_comment )
      {
         send_to_char( "There are no relevant comments.\n\r", ch );
         return;
      }

      vnum = 0;
      for( pnote = victim->pcdata->first_comment; pnote; pnote = pnote->next )
      {
         vnum++;
         ch_printf( ch, "%2d) %-10s [%s] %s\n\r", vnum, pnote->sender ? pnote->sender : "--Error--",
                    mini_c_time( pnote->date_stamp, -1 ), pnote->subject ? pnote->subject : "--Error--" );
         /*
          * Brittany added date to comment list and whois with above change 
          */
      }
      return;
   }

   if( !str_cmp( arg, "read" ) )
   {
      bool fAll;

      argument = one_argument( argument, arg );
      if( !( victim = get_char_world( ch, arg ) ) )
      {
         send_to_char( "They're not logged on!\n\r", ch );  /* maybe fix this? */
         return;
      }

      if( IS_NPC( victim ) )
      {
         send_to_char( "No comments about mobs\n\r", ch );
         return;
      }

      if( get_trust( victim ) >= get_trust( ch ) )
      {
         send_to_char( "You're not of the right caliber to do this...\n\r", ch );
         return;
      }

      if( !victim->pcdata->first_comment )
      {
         send_to_char( "There are no relevant comments.\n\r", ch );
         return;
      }

      if( !str_cmp( argument, "all" ) )
         fAll = TRUE;
      else if( is_number( argument ) )
         fAll = FALSE;
      else
      {
         send_to_char( "Read which comment?\n\r", ch );
         return;
      }

      vnum = 0;
      for( pnote = victim->pcdata->first_comment; pnote; pnote = pnote->next )
      {
         vnum++;
         if( fAll || vnum == atoi( argument ) )
         {
            note_to_char( ch, pnote, NULL, 0 );
            return;
         }
      }

      send_to_char( "No such comment.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "write" ) )
   {
      if( !ch->pcdata->pnote )
         note_attach( ch );
      ch->substate = SUB_WRITING_NOTE;
      ch->pcdata->dest_buf = ch->pcdata->pnote;
      if( !ch->pcdata->pnote->text || ch->pcdata->pnote->text[0] == '\0' )
         ch->pcdata->pnote->text = str_dup( "" );
      start_editing( ch, ch->pcdata->pnote->text );
      set_editor_desc( ch, "A player comment." );
      return;
   }

   if( !str_cmp( arg, "subject" ) )
   {
      if( !ch->pcdata->pnote )
         note_attach( ch );
      DISPOSE( ch->pcdata->pnote->subject );
      ch->pcdata->pnote->subject = str_dup( argument );
      send_to_char( "Ok.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "clear" ) )
   {
      if( ch->pcdata->pnote )
      {
         free_note( ch->pcdata->pnote );
         send_to_char( "Comment cleared.\n\r", ch );
         return;
      }
      send_to_char( "You arn't working on a comment!\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "show" ) )
   {
      if( !ch->pcdata->pnote )
      {
         send_to_char( "You have no comment in progress.\n\r", ch );
         return;
      }
      note_to_char( ch, ch->pcdata->pnote, NULL, 0 );
      return;
   }

   if( !str_cmp( arg, "post" ) )
   {
      if( !ch->pcdata->pnote )
      {
         send_to_char( "You have no comment in progress.\n\r", ch );
         return;
      }

      argument = one_argument( argument, arg );
      if( !( victim = get_char_world( ch, arg ) ) )
      {
         send_to_char( "They're not logged on!\n\r", ch );  /* maybe fix this? */
         return;
      }

      if( IS_NPC( victim ) )
      {
         send_to_char( "No comments about mobs\n\r", ch );
         return;
      }

      if( get_trust( victim ) > get_trust( ch ) )
      {
         send_to_char( "You failed, and they saw!\n\r", ch );
         ch_printf( victim, "%s has just tried to comment your character!\n\r", ch->name );
         return;
      }

      ch->pcdata->pnote->date_stamp = current_time;

      pnote = ch->pcdata->pnote;
      ch->pcdata->pnote = NULL;

      LINK( pnote, victim->pcdata->first_comment, victim->pcdata->last_comment, next, prev );
      save_char_obj( victim );
      send_to_char( "Comment posted!\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "remove" ) )
   {
      argument = one_argument( argument, arg );
      if( !( victim = get_char_world( ch, arg ) ) )
      {
         send_to_char( "They're not logged on!\n\r", ch );  /* maybe fix this? */
         return;
      }

      if( IS_NPC( victim ) )
      {
         send_to_char( "No comments about mobs\n\r", ch );
         return;
      }

      if( ( get_trust( victim ) >= get_trust( ch ) ) || ( get_trust( ch ) < LEVEL_KL ) )
      {
         send_to_char( "You're not of the right caliber to do this...\n\r", ch );
         return;
      }

      if( !is_number( argument ) )
      {
         send_to_char( "Comment remove which number?\n\r", ch );
         return;
      }

      vnum = 0;
      for( pnote = victim->pcdata->first_comment; pnote; pnote = pnote->next )
      {
         vnum++;
         if( ( LEVEL_KL <= get_trust( ch ) ) && ( vnum == atoi( argument ) ) )
         {
            comment_remove( victim, pnote );
            send_to_char( "Comment removed..\n\r", ch );
            return;
         }
      }

      send_to_char( "No such comment.\n\r", ch );
      return;
   }
   send_to_char( "Syntax: comment <argument> <args...>\n\r", ch );
   send_to_char( "  Where argument can equal:\n\r", ch );
   send_to_char( "     write, subject, clear, show\n\r", ch );
   send_to_char( "     list <player>, read <player> <#/all>, post <player>\n\r", ch );
   if( get_trust( ch ) >= LEVEL_KL )
      send_to_char( "     remove <player> <#>\n\r", ch );
   return;
}

void fwrite_comments( CHAR_DATA * ch, FILE * fp )
{
   NOTE_DATA *pnote;

   if( !ch->pcdata->first_comment )
      return;

   for( pnote = ch->pcdata->first_comment; pnote; pnote = pnote->next )
   {
      fprintf( fp, "%s", "#COMMENT2\n" ); /* Set to COMMENT2 as to tell from older comments */
      fwrite_note( pnote, fp );
   }
   return;
}

void fread_comment( CHAR_DATA * ch, FILE * fp )
{
   NOTE_DATA *pnote;

   pnote = read_note( fp );
   LINK( pnote, ch->pcdata->first_comment, ch->pcdata->last_comment, next, prev );
   return;
}

/* Function kept for backwards compatibility */
void fread_old_comment( CHAR_DATA * ch, FILE * fp )
{
   NOTE_DATA *pnote;

   log_string( "Starting comment conversion..." );
   for( ;; )
   {
      char letter;

      do
      {
         letter = getc( fp );
         if( feof( fp ) )
         {
            FCLOSE( fp );
            return;
         }
      }
      while( isspace( letter ) );
      ungetc( letter, fp );

      CREATE( pnote, NOTE_DATA, 1 );

      if( !str_cmp( fread_word( fp ), "sender" ) )
         pnote->sender = fread_string( fp );

      if( !str_cmp( fread_word( fp ), "date" ) )
         fread_to_eol( fp );

      if( !str_cmp( fread_word( fp ), "to" ) )
         fread_to_eol( fp );

      if( !str_cmp( fread_word( fp ), "subject" ) )
         pnote->subject = fread_string_nohash( fp );

      if( !str_cmp( fread_word( fp ), "text" ) )
         pnote->text = fread_string_nohash( fp );

      if( !pnote->sender )
         pnote->sender = STRALLOC( "None" );
      if( !pnote->subject )
         pnote->subject = str_dup( "Error: Subject not found" );
      if( !pnote->text )
         pnote->text = str_dup( "Error: Comment text not found." );

      pnote->date_stamp = current_time;
      LINK( pnote, ch->pcdata->first_comment, ch->pcdata->last_comment, next, prev );
      return;
   }
}
