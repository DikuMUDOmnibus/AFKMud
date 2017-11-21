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
 *                     Completely Revised Boards Module                     *
 ****************************************************************************
 * Revised by:   Xorith                                                     *
 * Last Updated: 10/2/03                                                    *
 ****************************************************************************/

#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include "mud.h"
#include "boards.h"
#include "clans.h"
#include "deity.h"

BOARD_DATA *first_board;
BOARD_DATA *last_board;
PROJECT_DATA *first_project;
PROJECT_DATA *last_project;

char *mini_c_time( time_t curtime, int tz );
void check_boards( time_t board_exp );

/* Global */
time_t board_expire_time_t;

char *const board_flags[] = {
   "r1", "backup_pruned", "private", "announce"
};

int get_board_flag( char *board_flag )
{
   int x;

   for( x = 0; x < MAX_BOARD_FLAGS; x++ )
      if( !str_cmp( board_flag, board_flags[x] ) )
         return x;
   return -1;
}

/* This is a simple function that keeps a string within a certain length. -- Xorith */
char *print_lngstr( char *string, size_t size )
{
   static char rstring[MSL];

   if( strlen( string ) > size )
   {
      mudstrlcpy( rstring, string, size - 2 );
      mudstrlcat( rstring, "...", size + 1 );
   }
   else
      mudstrlcpy( rstring, string, size );

   return rstring;
}

void free_pcboards( CHAR_DATA * ch )
{
   BOARD_CHARDATA *chboard, *chboard_next;

   if( IS_NPC( ch ) )
      return;

   for( chboard = ch->pcdata->first_boarddata; chboard; chboard = chboard_next )
   {
      chboard_next = chboard->next;

      UNLINK( chboard, ch->pcdata->first_boarddata, ch->pcdata->last_boarddata, next, prev );
      STRFREE( chboard->board_name );
      DISPOSE( chboard );
   }
   return;
}

bool can_remove( CHAR_DATA * ch, BOARD_DATA * board )
{
   /*
    * If there's no group on the board, or if the character is Immortal, use the following... 
    */
   if( !board->group || board->group[0] == '\0' || IS_IMMORTAL( ch ) )
   {
      if( get_trust( ch ) >= board->remove_level )
         return TRUE;

      if( board->moderators && board->moderators[0] != '\0' )
      {
         if( is_name( ch->name, board->moderators ) )
            return TRUE;
      }
   }
   /*
    * else, make sure they're a member of the group. this means no outside mortal moderators... 
    */
   else if( ch->pcdata->clan && is_name( ch->pcdata->clan->name, board->group ) )
   {
      if( IS_LEADER( ch ) || IS_NUMBER1( ch ) || IS_NUMBER2( ch ) )
         return TRUE;

      if( get_trust( ch ) >= board->remove_level )
         return TRUE;

      if( board->moderators && board->moderators[0] != '\0' )
      {
         if( is_name( ch->name, board->moderators ) )
            return TRUE;
      }
   }

   /*
    * If I ever put in some sort of deity priest code, similar check here as above 
    */
   return FALSE;
}

bool can_post( CHAR_DATA * ch, BOARD_DATA * board )
{
   /*
    * It makes sense to me that if you are a moderator then you can automatically read/post 
    */
   if( can_remove( ch, board ) )
      return TRUE;

   /*
    * If there's no group on the board, or if the character is Immortal, use the following... 
    */
   if( !board->group || board->group[0] == '\0' || IS_IMMORTAL( ch ) )
   {
      if( get_trust( ch ) >= board->post_level )
         return TRUE;

      if( board->posters && board->posters[0] != '\0' )
      {
         if( is_name( ch->name, board->posters ) )
            return TRUE;
      }
   }
   /*
    * else, make sure they're a member of the group. this means no outside mortal posters... 
    */
   else if( ( ch->pcdata->clan && is_name( ch->pcdata->clan->name, board->group ) ) ||
            ( ch->pcdata->deity && is_name( ch->pcdata->deity->name, board->group ) ) )
   {
      if( get_trust( ch ) >= board->post_level )
         return TRUE;

      if( board->posters && board->posters[0] != '\0' )
      {
         if( is_name( ch->name, board->posters ) )
            return TRUE;
      }
   }
   return FALSE;
}

bool can_read( CHAR_DATA * ch, BOARD_DATA * board )
{
   /*
    * It makes sense to me that if you are a moderator then you can automatically read/post 
    */
   if( can_remove( ch, board ) )
      return TRUE;

   /*
    * If there's no group on the board, or if the character is Immortal, use the following... 
    */
   if( !board->group || board->group[0] == '\0' || IS_IMMORTAL( ch ) )
   {
      if( get_trust( ch ) >= board->read_level )
         return TRUE;

      if( board->readers && board->readers[0] != '\0' )
      {
         if( is_name( ch->name, board->readers ) )
            return TRUE;
      }
   }
   /*
    * else, make sure they're a member of the group. this means no outside mortal readers... 
    */
   else if( ( ch->pcdata->clan && is_name( ch->pcdata->clan->name, board->group ) ) ||
            ( ch->pcdata->deity && is_name( ch->pcdata->deity->name, board->group ) ) )
   {
      if( get_trust( ch ) >= board->read_level )
         return TRUE;

      if( board->readers && board->readers[0] != '\0' )
      {
         if( is_name( ch->name, board->readers ) )
            return TRUE;
      }
   }
   return FALSE;
}

bool is_note_to( CHAR_DATA * ch, NOTE_DATA * pnote )
{
   if( pnote->to_list && pnote->to_list[0] != '\0' )
   {
      if( is_name( "all", pnote->to_list ) )
         return TRUE;

      if( IS_IMMORTAL( ch ) && is_name( "immortal", pnote->to_list ) )
         return TRUE;

      if( is_name( ch->name, pnote->to_list ) )
         return TRUE;
   }
   return FALSE;
}

/* This will get a board by object. This will not get a global board
   as global boards are noted with a 0 objvnum */
BOARD_DATA *get_board_by_obj( OBJ_DATA * obj )
{
   BOARD_DATA *board;

   for( board = first_board; board; board = board->next )
      if( board->objvnum == obj->pIndexData->vnum )
         return board;
   return NULL;
}

/* Gets a board by name, or a number. The number should be the board # given in do_board_list.
   If ch == NULL, then it'll perform the search without checks. Otherwise, it'll perform the
   search and weed out boards that the ch can't view remotely. */
BOARD_DATA *get_board( CHAR_DATA * ch, const char *name )
{
   BOARD_DATA *board;
   int count = 1;

   for( board = first_board; board; board = board->next )
   {
      if( ch != NULL )
      {
         if( !can_read( ch, board ) )
            continue;
         if( board->objvnum > 0 && !can_remove( ch, board ) && !IS_IMMORTAL( ch ) )
            continue;
      }
      if( count == atoi( name ) )
         return board;
      if( !str_cmp( board->name, name ) )
         return board;
      count++;
   }
   return NULL;
}

/* This will find a board on an object in the character's current room */
BOARD_DATA *find_board( CHAR_DATA * ch )
{
   OBJ_DATA *obj;
   BOARD_DATA *board;

   for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
      if( ( board = get_board_by_obj( obj ) ) != NULL )
         return board;
   return NULL;
}

BOARD_CHARDATA *get_chboard( CHAR_DATA * ch, char *board_name )
{
   BOARD_CHARDATA *chboard;

   for( chboard = ch->pcdata->first_boarddata; chboard; chboard = chboard->next )
      if( !str_cmp( chboard->board_name, board_name ) )
         return chboard;
   return NULL;
}

BOARD_CHARDATA *create_chboard( CHAR_DATA * ch, char *board_name )
{
   BOARD_CHARDATA *chboard;

   if( ( chboard = get_chboard( ch, board_name ) ) )
      return chboard;

   CREATE( chboard, BOARD_CHARDATA, 1 );
   chboard->board_name = QUICKLINK( board_name );
   LINK( chboard, ch->pcdata->first_boarddata, ch->pcdata->last_boarddata, next, prev );
   return chboard;
}

void note_attach( CHAR_DATA * ch )
{
   NOTE_DATA *pnote;

   if( IS_NPC( ch ) )
      return;

   if( ch->pcdata->pnote )
   {
      bug( "%s:  ch->pcdata->pnote already exsists!", __FUNCTION__ );
      return;
   }

   CREATE( pnote, NOTE_DATA, 1 );
   pnote->next = NULL;
   pnote->prev = NULL;
   pnote->first_reply = NULL;
   pnote->last_reply = NULL;
   pnote->parent = NULL;
   pnote->sender = QUICKLINK( ch->name );
   pnote->to_list = NULL;
   pnote->subject = NULL;
   pnote->text = NULL;
   pnote->reply_count = 0;
   pnote->date_stamp = 0;
   pnote->expire = 0;
   ch->pcdata->pnote = pnote;
   return;
}

void free_note( NOTE_DATA * pnote )
{
   NOTE_DATA *reply = NULL, *next_reply = NULL;

   if( pnote == NULL )
   {
      bug( "%s: NULL pnote!", __FUNCTION__ );
      return;
   }

   DISPOSE( pnote->text );
   DISPOSE( pnote->subject );
   STRFREE( pnote->to_list );
   STRFREE( pnote->sender );

   for( reply = pnote->first_reply; reply; reply = next_reply )
   {
      next_reply = reply->next;
      UNLINK( reply, pnote->first_reply, pnote->last_reply, next, prev );
      free_note( reply );
   }
   /*
    * Now dispose of the parent 
    */
   DISPOSE( pnote );
}

void free_board( BOARD_DATA * board )
{
   NOTE_DATA *pnote, *next_note;

   STRFREE( board->name );
   STRFREE( board->readers );
   STRFREE( board->posters );
   STRFREE( board->moderators );
   DISPOSE( board->desc );
   STRFREE( board->group );
   DISPOSE( board->filename );

   for( pnote = board->first_note; pnote; pnote = next_note )
   {
      next_note = pnote->next;
      UNLINK( pnote, board->first_note, board->last_note, next, prev );
      free_note( pnote );
   }
   UNLINK( board, first_board, last_board, next, prev );
   DISPOSE( board );
}

void free_boards( void )
{
   BOARD_DATA *board, *board_next;

   for( board = first_board; board; board = board_next )
   {
      board_next = board->next;
      free_board( board );
   }
   return;
}

void write_boards( void )
{
   BOARD_DATA *board, *board_next;
   FILE *fpout;

   if( !( fpout = fopen( BOARD_DIR BOARD_FILE, "w" ) ) )
   {
      perror( BOARD_DIR BOARD_FILE );
      bug( "%s: Unable to open %s%s for writing!", __FUNCTION__, BOARD_DIR, BOARD_FILE );
      return;
   }

   for( board = first_board; board; board = board->next )
   {
      board_next = board->next;
      if( !board->name )
      {
         bug( "%s: Board with a null name! Destroying...", __FUNCTION__ );
         UNLINK( board, first_board, last_board, next, prev );
         free_board( board );
         continue;
      }
      fprintf( fpout, "Name        %s~\n", board->name );
      fprintf( fpout, "Filename    %s~\n", board->filename );
      if( board->desc )
         fprintf( fpout, "Desc        %s~\n", board->desc );
      fprintf( fpout, "ObjVnum     %d\n", board->objvnum );
      fprintf( fpout, "Expire      %d\n", board->expire );
      fprintf( fpout, "Flags       %s\n", print_bitvector( &board->flags ) );
      if( board->readers )
         fprintf( fpout, "Readers     %s~\n", board->readers );
      if( board->posters )
         fprintf( fpout, "Posters     %s~\n", board->posters );
      if( board->moderators )
         fprintf( fpout, "Moderators  %s~\n", board->moderators );
      if( board->group )
         fprintf( fpout, "Group       %s~\n", board->group );
      fprintf( fpout, "ReadLevel   %d\n", board->read_level );
      fprintf( fpout, "PostLevel   %d\n", board->post_level );
      fprintf( fpout, "RemoveLevel %d\n", board->remove_level );
      fprintf( fpout, "%s\n", "#END" );
   }
   FCLOSE( fpout );
}

void fwrite_reply( NOTE_DATA * pnote, FILE * fpout )
{
   fprintf( fpout, "Reply-Sender         %s~\n", pnote->sender );
   fprintf( fpout, "Reply-To             %s~\n", pnote->to_list );
   fprintf( fpout, "Reply-Subject        %s~\n", pnote->subject );
   fprintf( fpout, "Reply-DateStamp      %ld\n", ( long int )pnote->date_stamp );
   fprintf( fpout, "Reply-Text           %s~\n", pnote->text );
   fprintf( fpout, "%s\n", "Reply-End" );
   return;
}

void fwrite_note( NOTE_DATA * pnote, FILE * fpout )
{
   NOTE_DATA *reply;
   int count = 0;

   if( !pnote )
      return;

   if( !pnote->sender )
   {
      bug( "%s", "fwrite_note: fwrite_note called on a note without a valid sender!" );
      return;
   }

   fprintf( fpout, "Sender         %s~\n", pnote->sender );
   if( pnote->to_list )
      fprintf( fpout, "To             %s~\n", pnote->to_list );
   if( pnote->subject )
      fprintf( fpout, "Subject        %s~\n", pnote->subject );
   fprintf( fpout, "DateStamp      %ld\n", ( long int )pnote->date_stamp );
   if( pnote->to_list ) /* Comments and Project Logs do not use to_list or Expire */
      fprintf( fpout, "Expire         %d\n", pnote->expire );
   if( pnote->text )
      fprintf( fpout, "Text           %s~\n", pnote->text );
   if( pnote->to_list ) /* comments and projects should not have replies */
   {
      for( reply = pnote->first_reply; reply; reply = reply->next )
      {
         if( !reply->sender || !reply->to_list || !reply->subject || !reply->text )
         {
            bug( "%s: Destroying a buggy reply on note '%s'!", __FUNCTION__, pnote->subject );
            UNLINK( reply, pnote->first_reply, pnote->last_reply, next, prev );
            --pnote->reply_count;
            free_note( reply );
            continue;
         }
         if( count == MAX_REPLY )
            break;
         fprintf( fpout, "%s\n", "Reply" );
         fwrite_reply( reply, fpout );
         count++;
      }
   }
   fprintf( fpout, "%s\n", "#END" );
   return;
}

void write_board( BOARD_DATA * board )
{
   FILE *fp;
   char filename[256];
   NOTE_DATA *pnote, *pnote_next;

   snprintf( filename, 256, "%s%s.board", BOARD_DIR, board->filename );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      perror( filename );
      bug( "%s: Error opening %s! Board NOT saved!", __FUNCTION__, filename );
      return;
   }

   for( pnote = board->first_note; pnote; pnote = pnote_next )
   {
      pnote_next = pnote->next;
      if( !pnote->sender || !pnote->to_list || !pnote->subject || !pnote->text )
      {
         bug( "%s: Destroying a buggy note on the %s board!", __FUNCTION__, board->name );
         UNLINK( pnote, board->first_note, board->last_note, next, prev );
         --board->msg_count;
         free_note( pnote );
      }
      fwrite_note( pnote, fp );
   }
   FCLOSE( fp );
   return;
}

void note_remove( BOARD_DATA * board, NOTE_DATA * pnote )
{
   if( !board || !pnote )
   {
      bug( "%s: null %s variable.", __FUNCTION__, board ? "pnote" : "board" );
      return;
   }

   if( pnote->parent )
   {
      UNLINK( pnote, pnote->parent->first_reply, pnote->parent->last_reply, next, prev );
      --pnote->parent->reply_count;
   }
   else
   {
      UNLINK( pnote, board->first_note, board->last_note, next, prev );
      --board->msg_count;
   }
   free_note( pnote );
   write_board( board );
}

BOARD_DATA *read_board( FILE * fp )
{
   BOARD_DATA *board;
   const char *word;
   char letter;
   bool fMatch;

   do
   {
      letter = getc( fp );
      if( feof( fp ) )
      {
         FCLOSE( fp );
         return NULL;
      }
   }
   while( isspace( letter ) );

   ungetc( letter, fp );

   CREATE( board, BOARD_DATA, 1 );

   for( ;; )
   {
      word = feof( fp ) ? "#END" : fread_word( fp );
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'D':
            KEY( "Desc", board->desc, fread_string_nohash( fp ) );
            break;

         case 'E':
            KEY( "Expire", board->expire, fread_number( fp ) );
            break;

         case 'F':
            KEY( "Filename", board->filename, fread_string_nohash( fp ) );
            KEY( "Flags", board->flags, fread_bitvector( fp ) );
            break;

         case 'G':
            KEY( "Group", board->group, fread_string( fp ) );
            break;

         case 'M':
            KEY( "Moderators", board->moderators, fread_string( fp ) );
            break;

         case 'N':
            KEY( "Name", board->name, fread_string( fp ) );
            break;

         case 'O':
            KEY( "ObjVnum", board->objvnum, fread_number( fp ) );
            break;

         case 'P':
            KEY( "Posters", board->posters, fread_string( fp ) );
            KEY( "PostLevel", board->post_level, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Readers", board->readers, fread_string( fp ) );
            KEY( "ReadLevel", board->read_level, fread_number( fp ) );
            KEY( "RemoveLevel", board->remove_level, fread_number( fp ) );
            break;

         case '#':
            if( !str_cmp( word, "#END" ) )
            {
               board->first_note = NULL;
               board->last_note = NULL;
               board->next = NULL;
               board->prev = NULL;
               if( !board->objvnum )
                  board->objvnum = 0;  /* default to global */
               return board;
            }
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

BOARD_DATA *read_old_board( FILE * fp )
{
   BOARD_DATA *board;
   const char *word;
   char letter;
   bool fMatch;

   do
   {
      letter = getc( fp );
      if( feof( fp ) )
      {
         FCLOSE( fp );
         return NULL;
      }
   }
   while( isspace( letter ) );

   ungetc( letter, fp );

   CREATE( board, BOARD_DATA, 1 );

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

         case 'E':
            KEY( "Extra_readers", board->readers, fread_string( fp ) );
            KEY( "Extra_removers", board->moderators, fread_string( fp ) );
            if( !str_cmp( word, "End" ) )
            {
               board->first_note = NULL;
               board->last_note = NULL;
               board->next = NULL;
               board->prev = NULL;
               if( !board->objvnum )
                  board->objvnum = 0;  /* default to global */
               board->desc = str_dup( "Newly converted board!" );
               board->expire = MAX_BOARD_EXPIRE;
               board->filename = str_dup( board->name );
               if( board->posters[0] == '\0' )
               {
                  STRFREE( board->posters );
                  board->posters = NULL;
               }
               if( board->readers[0] == '\0' )
               {
                  STRFREE( board->readers );
                  board->readers = NULL;
               }
               if( board->moderators[0] == '\0' )
               {
                  STRFREE( board->moderators );
                  board->moderators = NULL;
               }
               if( board->group[0] == '\0' )
               {
                  STRFREE( board->group );
                  board->group = NULL;
               }
               return board;
            }
            break;

         case 'F':
            KEY( "Filename", board->name, fread_string( fp ) );
            break;

         case 'M':
            KEY( "Min_read_level", board->read_level, fread_number( fp ) );
            KEY( "Min_remove_level", board->remove_level, fread_number( fp ) );
            KEY( "Min_post_level", board->post_level, fread_number( fp ) );
            if( !str_cmp( word, "Max_posts" ) )
            {
               word = fread_word( fp );
               fMatch = TRUE;
               break;
            }
            break;

         case 'P':
            KEY( "Post_group", board->posters, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Read_group", board->group, fread_string( fp ) );
            break;

         case 'T':
            if( !str_cmp( word, "Type" ) )
            {
               fMatch = TRUE;
               if( fread_number( fp ) == 1 )
                  xTOGGLE_BIT( board->flags, BOARD_PRIVATE );
               break;
            }
            break;

         case 'V':
            KEY( "Vnum", board->objvnum, fread_number( fp ) );
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

NOTE_DATA *read_note( FILE * fp )
{
   NOTE_DATA *pnote = NULL;
   NOTE_DATA *reply = NULL;
   const char *word;
   char letter;
   bool fMatch;

   do
   {
      letter = getc( fp );
      if( feof( fp ) )
      {
         FCLOSE( fp );
         return NULL;
      }
   }
   while( isspace( letter ) );
   ungetc( letter, fp );

   CREATE( pnote, NOTE_DATA, 1 );
   pnote->first_reply = NULL;
   pnote->last_reply = NULL;

   for( ;; )
   {
      word = feof( fp ) ? "#END" : fread_word( fp );
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'D':
            if( !str_cmp( word, "Date" ) )
            {
               fread_to_eol( fp );
               fMatch = TRUE;
               break;
            }
            KEY( "DateStamp", pnote->date_stamp, fread_number( fp ) );
            break;

         case 'S':
            KEY( "Sender", pnote->sender, fread_string( fp ) );
            KEY( "Subject", pnote->subject, fread_string_nohash( fp ) );
            break;

         case 'T':
            KEY( "To", pnote->to_list, fread_string( fp ) );
            KEY( "Text", pnote->text, fread_string_nohash( fp ) );
            break;

         case 'E':
            KEY( "Expire", pnote->expire, fread_number( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "Reply" ) )
            {
               if( pnote->reply_count == MAX_REPLY )
               {
                  bug( "%s: Reply found when MAX_REPLY has already been reached!", __FUNCTION__ );
                  continue;
               }
               if( reply != NULL )
               {
                  bug( "%s: Unsupported nested reply found!", __FUNCTION__ );
                  continue;
               }
               CREATE( reply, NOTE_DATA, 1 );
               reply->first_reply = NULL;
               reply->last_reply = NULL;
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Reply-Date" ) && reply != NULL )
            {
               fread_to_eol( fp );
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Reply-DateStamp" ) && reply != NULL )
            {
               reply->date_stamp = fread_number( fp );
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Reply-Sender" ) && reply != NULL )
            {
               reply->sender = fread_string( fp );
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Reply-Subject" ) && reply != NULL )
            {
               reply->subject = fread_string_nohash( fp );
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Reply-To" ) && reply != NULL )
            {
               reply->to_list = fread_string( fp );
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Reply-Text" ) && reply != NULL )
            {
               reply->text = fread_string_nohash( fp );
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Reply-End" ) && reply != NULL )
            {
               reply->expire = 0;
               if( !reply->date_stamp )
                  reply->date_stamp = current_time;
               reply->parent = pnote;
               LINK( reply, pnote->first_reply, pnote->last_reply, next, prev );
               pnote->reply_count += 1;
               reply = NULL;
               fMatch = TRUE;
               break;
            }

         case '#':
            if( !str_cmp( word, "#END" ) )
            {
               pnote->next = NULL;
               pnote->prev = NULL;
               if( !pnote->date_stamp )
                  pnote->date_stamp = current_time;
               return pnote;
            }
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

NOTE_DATA *read_old_note( FILE * fp )
{
   NOTE_DATA *pnote = NULL;
   const char *word;
   char letter;
   bool fMatch;

   do
   {
      letter = getc( fp );
      if( feof( fp ) )
      {
         FCLOSE( fp );
         return NULL;
      }
   }
   while( isspace( letter ) );
   ungetc( letter, fp );

   CREATE( pnote, NOTE_DATA, 1 );
   pnote->first_reply = NULL;
   pnote->last_reply = NULL;
   pnote->parent = NULL;
   pnote->next = NULL;
   pnote->prev = NULL;

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = FALSE;

      /*
       * Keep the damn thing happy. 
       */
      if( !str_cmp( word, "Voting" ) || !str_cmp( word, "Yesvotes" )
          || !str_cmp( word, "Novotes" ) || !str_cmp( word, "Abstentions" ) )
      {
         word = fread_word( fp );
         fMatch = TRUE;
         continue;
      }

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'C':
            if( !str_cmp( word, "Ctime" ) )
            {
               pnote->date_stamp = fread_number( fp );
               pnote->expire = 0;
               if( !pnote->text )
               {
                  pnote->text = str_dup( "---- Delete this post! ----" );
                  pnote->expire = 1;   /* Or we'll do it for you! */
                  bug( "%s: No text on note! Setting to '%s' and expiration to 1 day", __FUNCTION__, pnote->text );
               }

               if( !pnote->date_stamp )
               {
                  bug( "%s: No date_stamp on note -- setting to current time!", __FUNCTION__ );
                  pnote->date_stamp = current_time;
               }

               /*
                * For converted notes, lets make a few exceptions 
                */
               if( !pnote->sender )
               {
                  pnote->sender = STRALLOC( "Converted Msg" );
                  bug( "%s: No sender on converted note! Setting to '%s'", __FUNCTION__, pnote->sender );
               }
               if( !pnote->subject )
               {
                  pnote->subject = str_dup( "Converted Msg" );
                  bug( "%s: No subject on converted note! Setting to '%s'", __FUNCTION__, pnote->subject );
               }
               if( !pnote->to_list )
               {
                  pnote->to_list = STRALLOC( "imm" );
                  bug( "%s: No to_list on converted note! Setting to '%s'", __FUNCTION__, pnote->to_list );
               }
               return pnote;
            }
            break;

         case 'S':
            KEY( "Sender", pnote->sender, fread_string( fp ) );
            KEY( "Subject", pnote->subject, fread_string_nohash( fp ) );
            break;

         case 'T':
            KEY( "To", pnote->to_list, fread_string( fp ) );
            KEY( "Text", pnote->text, fread_string_nohash( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               pnote->expire = 0;
               if( !pnote->text )
               {
                  pnote->text = str_dup( "---- Delete this post! ----" );
                  pnote->expire = 1;   /* Or we'll do it for you! */
                  bug( "%s: No text on note! Setting to '%s' and expiration to 1 day", __FUNCTION__, pnote->text );
               }

               if( !pnote->date_stamp )
               {
                  bug( "%s: No date_stamp on note -- setting to current time!", __FUNCTION__ );
                  pnote->date_stamp = current_time;
               }

               /*
                * For converted notes, lets make a few exceptions 
                */
               if( !pnote->sender )
               {
                  pnote->sender = STRALLOC( "Converted Msg" );
                  bug( "%s: No sender on converted note! Setting to '%s'", __FUNCTION__, pnote->sender );
               }
               if( !pnote->subject )
               {
                  pnote->subject = str_dup( "Converted Msg" );
                  bug( "%s: No subject on converted note! Setting to '%s'", __FUNCTION__, pnote->subject );
               }
               if( !pnote->to_list )
               {
                  pnote->to_list = STRALLOC( "imm" );
                  bug( "%s: No to_list on converted note! Setting to '%s'", __FUNCTION__, pnote->to_list );
               }
               return pnote;
            }
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

void load_boards( void )
{
   FILE *board_fp, *note_fp;
   BOARD_DATA *board;
   NOTE_DATA *pnote;
   char notefile[256];
   bool oldboards = FALSE;

   first_board = last_board = NULL;

   if( !( board_fp = fopen( BOARD_DIR BOARD_FILE, "r" ) ) )
   {
      if( !( board_fp = fopen( BOARD_DIR OLD_BOARD_FILE, "r" ) ) )
         return;
      oldboards = TRUE;
      log_string( "Converting older boards..." );
   }

   if( oldboards )
   {
      while( ( board = read_old_board( board_fp ) ) != NULL )
      {
         LINK( board, first_board, last_board, next, prev );
         snprintf( notefile, 256, "%s%s", BOARD_DIR, board->filename );
         if( ( note_fp = fopen( notefile, "r" ) ) != NULL )
         {
            log_string( notefile );
            while( ( pnote = read_old_note( note_fp ) ) != NULL )
            {
               LINK( pnote, board->first_note, board->last_note, next, prev );
               board->msg_count += 1;
            }
         }
         else
            bug( "%s: Note file '%s' for the '%s' board not found!", __FUNCTION__, notefile, board->name );

         write_board( board );   /* save the converted board */
      }
      write_boards(  ); /* save the new boards file */
   }
   else
   {
      while( ( board = read_board( board_fp ) ) != NULL )
      {
         LINK( board, first_board, last_board, next, prev );
         snprintf( notefile, 256, "%s%s.board", BOARD_DIR, board->filename );
         if( ( note_fp = fopen( notefile, "r" ) ) != NULL )
         {
            log_string( notefile );
            while( ( pnote = read_note( note_fp ) ) != NULL )
            {
               LINK( pnote, board->first_note, board->last_note, next, prev );
               board->msg_count += 1;
            }
         }
         else
            bug( "%s: Note file '%s' for the '%s' board not found!", __FUNCTION__, notefile, board->name );
      }
   }
   /*
    * Run initial pruning 
    */
   check_boards( 0 );
   return;
}

int unread_notes( CHAR_DATA * ch, BOARD_DATA * board )
{
   NOTE_DATA *pnote;
   BOARD_CHARDATA *chboard;
   int count = 0;

   chboard = get_chboard( ch, board->name );

   for( pnote = board->first_note; pnote; pnote = pnote->next )
   {
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, pnote ) )
         continue;
      if( !chboard )
         count++;
      else if( pnote->date_stamp > chboard->last_read )
         count++;
   }
   return count;
}

int total_replies( BOARD_DATA * board )
{
   NOTE_DATA *pnote;
   int count = 0;

   for( pnote = board->first_note; pnote; pnote = pnote->next )
      count += pnote->reply_count;
   return count;
}

/* This is because private boards don't function right with board->msg_count...  :P */
int total_notes( CHAR_DATA * ch, BOARD_DATA * board )
{
   NOTE_DATA *pnote;
   int count = 0;

   for( pnote = board->first_note; pnote; pnote = pnote->next )
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, pnote ) && !can_remove( ch, board ) )
         continue;
      else
         count++;

   return count;
}

/* Only expire root messages. Replies are pointless to expire. */
void board_check_expire( BOARD_DATA * board )
{
   NOTE_DATA *pnote, *pnote_next;
   time_t now_time, time_diff;
   FILE *fp;
   char filename[256];

   if( board->expire == 0 )
      return;

   now_time = time( 0 );

   for( pnote = board->first_note; pnote; pnote = pnote_next )
   {
      pnote_next = pnote->next;
      if( pnote->expire == 0 )
         continue;

      time_diff = ( now_time - pnote->date_stamp ) / 86400;
      if( time_diff >= pnote->expire )
      {
         if( IS_BOARD_FLAG( board, BOARD_BU_PRUNED ) )
         {
            snprintf( filename, 256, "%s%s.purged", BOARD_DIR, board->name );
            if( !( fp = fopen( filename, "a" ) ) )
            {
               perror( filename );
               bug( "%s: Error opening %s!", __FUNCTION__, filename );
               return;
            }
            fwrite_note( pnote, fp );
            log_printf( "Saving expired note '%s' to '%s'", pnote->subject, filename );
            FCLOSE( fp );
         }
         log_printf( "Removing expired note '%s'", pnote->subject );
         note_remove( board, pnote );
         continue;
      }
   }
   return;
}

void check_boards( time_t board_exp )
{
   BOARD_DATA *board;

   if( board_exp > current_time )
      return;

   log_string( "Starting board pruning..." );
   for( board = first_board; board; board = board->next )
      board_check_expire( board );
   log_string( "Next board pruning in 24 hours." );
   board_expire_time_t = current_time + 86400;
   return;
}

void board_announce( CHAR_DATA * ch, BOARD_DATA * board, NOTE_DATA * pnote )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *vch;
   BOARD_CHARDATA *chboard;

   for( d = first_descriptor; d; d = d->next )
   {
      if( d->connected != CON_PLAYING )
         continue;

      vch = d->character ? d->character : d->original;
      if( !vch )
         continue;

      if( !can_read( vch, board ) )
         continue;

      if( !IS_BOARD_FLAG( board, BOARD_ANNOUNCE ) )
      {
         if( !( chboard = get_chboard( ch, board->name ) ) )
            continue;
         if( chboard->alert != BD_ANNOUNCE )
            continue;
      }

      if( !IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
         ch_printf( vch, "&G[&wBoard Announce&G] &w%s has posted a message on the %s board.\n\r", ch->name, board->name );
      else if( is_note_to( vch, pnote ) )
         ch_printf( vch, "&G[&wBoard Announce&G] &w%s has posted a message for you on the %s board.\n\r",
                    ch->name, board->name );
   }
   return;
}

void note_to_char( CHAR_DATA * ch, NOTE_DATA * pnote, BOARD_DATA * board, short id )
{
   int count = 1;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   if( pnote == NULL )
   {
      bug( "%s: null pnote!", __FUNCTION__ );
      return;
   }

   if( id > 0 && board != NULL )
      ch_printf( ch, "%s[%sNote #%s%d%s of %s%d%s]%s           -- %s%s%s --&D\n\r",
                 s3, s1, s2, id, s1, s2, total_notes( ch, board ), s3, s1, s2, board->name, s1 );

   /*
    * Using a negative ID# for a bit of beauty -- Xorith 
    */
   if( id == -1 && !board )
      ch_printf( ch, "%s[%sProject Log%s]&D\n\r", s3, s2, s3 );
   if( id == -2 && !board )
      ch_printf( ch, "%s[%sPlayer Comment%s]&D\n\r", s3, s2, s3 );

   ch_printf( ch, "%sFrom:    %s%-15s", s1, s2, pnote->sender ? pnote->sender : "--Error--" );
   if( pnote->to_list )
      ch_printf( ch, " %sTo:     %s%-15s", s1, s2, pnote->to_list );
   send_to_char( "&D\n\r", ch );
   if( pnote->date_stamp != 0 )
      ch_printf( ch, "%sDate:    %s%s&D\n\r", s1, s2, c_time( pnote->date_stamp, ch->pcdata->timezone ) );

   if( board && can_remove( ch, board ) )
   {
      if( pnote->expire == 0 )
         ch_printf( ch, "%sThis note is sticky and will not expire.&D\n\r", s1 );
      else
      {
         int n_life;
         n_life = pnote->expire - ( ( current_time - pnote->date_stamp ) / 86400 );
         ch_printf( ch, "%sThis note will expire in %s%d%s day%s.&D\n\r", s1, s2, n_life, s1, n_life == 1 ? "" : "s" );
      }
   }

   ch_printf( ch, "%sSubject: %s%s&D\n\r", s1, s2, pnote->subject ? pnote->subject : "" );
   ch_printf( ch, "%s------------------------------------------------------------------------------&D\n\r", s1 );
   ch_printf( ch, "%s%s&D\n\r", s2, pnote->text ? pnote->text : "--Error--" );

   if( pnote->first_reply )
   {
      NOTE_DATA *reply;
      for( reply = pnote->first_reply; reply; reply = reply->next )
      {
         ch_printf( ch, "\n\r%s------------------------------------------------------------------------------&D\n\r", s1 );
         ch_printf( ch, "%s[%sReply #%s%d%s] [%s%s%s]&D\n\r", s3, s1, s2, count, s3, s2,
                    c_time( reply->date_stamp, ch->pcdata->timezone ), s3 );
         ch_printf( ch, "%sFrom:    %s%-15s", s1, s2, reply->sender ? reply->sender : "--Error--" );
         if( reply->to_list )
            ch_printf( ch, "   %sTo:     %s%-15s", s1, s2, reply->to_list );
         send_to_char( "&D\n\r", ch );
         ch_printf( ch, "%s------------------------------------------------------------------------------&D\n\r", s1 );
         ch_printf( ch, "%s%s&D\n\r", s2, reply->text ? reply->text : "--Error--" );
         count++;
      }
   }
   return;
}

CMDF do_note_set( CHAR_DATA * ch, char *argument )
{
   BOARD_DATA *board;
   NOTE_DATA *pnote;
   short n_num = 0, i = 1;
   char s1[16], s2[16], s3[16], arg[MIL];

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   if( !( board = find_board( ch ) ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch_printf( ch,
                    "%sModifies the fields on a note.\n\r%sSyntax: %snset %s<%sboard%s> <%snote#%s> <%sfield%s> <%svalue%s>&D\n\r",
                    s1, s3, s1, s3, s2, s3, s2, s3, s2, s3, s2, s3 );
         ch_printf( ch, "%s  Valid fields are: %ssender to_list subject expire&D\n\r", s1, s2 );
         return;
      }
      argument = one_argument( argument, arg );

      if( !( board = get_board( ch, arg ) ) )
      {
         ch_printf( ch, "%sNo board found!&D\n\r", s1 );
         return;
      }
   }
   else
      ch_printf( ch, "%sUsing current board in room: %s%s&D\n\r", s1, s2, board->name );

   if( !can_remove( ch, board ) )
   {
      send_to_char( "You are unable to modify the notes on this board.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Modify which note?\n\r", ch );
      return;
   }
   argument = one_argument( argument, arg );
   n_num = atoi( arg );

   if( n_num == 0 )
   {
      send_to_char( "Modify which note?\n\r", ch );
      return;
   }

   i = 1;
   for( pnote = board->first_note; pnote; pnote = pnote->next )
   {
      if( i == n_num )
         break;
      else
         i++;
   }

   if( !pnote )
   {
      send_to_char( "Note not found!\n\r", ch );
      return;
   }
   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "expire" ) )
   {
      if( atoi( argument ) < 0 || atoi( argument ) > MAX_BOARD_EXPIRE )
      {
         ch_printf( ch, "%sExpiration days must be a value between %s0%s and %s%d%s.&D\n\r",
                    s1, s2, s1, s2, MAX_BOARD_EXPIRE, s1 );
         return;
      }
      pnote->expire = atoi( argument );
      ch_printf( ch, "%sSet the expiration time for '%s%s%s' to %s%d&D\n\r", s1, s2, pnote->subject, s1, s2, pnote->expire );
      write_board( board );
      return;
   }

   if( !str_cmp( arg, "sender" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch_printf( ch, "%sYou must specify a new sender.&D\n\r", s1 );
         return;
      }
      STRFREE( pnote->sender );
      pnote->sender = STRALLOC( argument );
      ch_printf( ch, "%sSet the sender for '%s%s%s' to %s%s&D\n\r", s1, s2, pnote->subject, s1, s2, pnote->sender );
      write_board( board );
      return;
   }

   if( !str_cmp( arg, "to_list" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch_printf( ch, "%sYou must specify a new to_list.&D\n\r", s1 );
         return;
      }
      STRFREE( pnote->to_list );
      pnote->to_list = STRALLOC( argument );
      ch_printf( ch, "%sSet the to_list for '%s%s%s' to %s%s&D\n\r", s1, s2, pnote->subject, s1, s2, pnote->to_list );
      write_board( board );
      return;
   }

   if( !str_cmp( arg, "subject" ) )
   {
      char buf[MSL];
      if( !argument || argument[0] == '\0' )
      {
         ch_printf( ch, "%sYou must specify a new subject.&D\n\r", s1 );
         return;
      }
      snprintf( buf, MSL, "%s", pnote->subject );
      DISPOSE( pnote->subject );
      pnote->subject = str_dup( argument );
      ch_printf( ch, "%sSet the subject for '%s%s%s' to %s%s&D\n\r", s1, s2, buf, s1, s2, pnote->subject );
      write_board( board );
      return;
   }
   ch_printf( ch,
              "%sModifies the fields on a note.\n\r%sSyntax: %snset %s<%sboard%s> <%snote#%s> <%sfield%s> <%svalue%s>&D\n\r",
              s1, s3, s1, s3, s2, s3, s2, s3, s2, s3, s2, s3 );
   ch_printf( ch, "%s  Valid fields are: %ssender to_list subject expire&D\n\r", s1, s2 );
   return;
}

/* This handles anything in the CON_BOARD state. */
CMDF do_note_write( CHAR_DATA * ch, char *argument );
void board_parse( DESCRIPTOR_DATA * d, char *argument )
{
   CHAR_DATA *ch;
   char s1[16], s2[16], s3[16];

   ch = d->character ? d->character : d->original;

   /*
    * Can NPCs even have substates and connected? 
    */
   if( IS_NPC( ch ) )
   {
      bug( "%s: NPC in %s!", __FUNCTION__, __FUNCTION__ );
      d->connected = CON_PLAYING;
      ch->substate = SUB_NONE;
      return;
   }

   /*
    * If a board sub-system is in place, these checks will need to reflect the proper substates
    * that would require them. For instance. a main menu would not require a note on the char. -- X 
    */
   if( !ch->pcdata->pnote )
   {
      bug( "%s: In %s -- No pnote on character!", __FUNCTION__, "Substate" );
      d->connected = CON_PLAYING;
      ch->substate = SUB_NONE;
      return;
   }

   if( !ch->pcdata->board )
   {
      bug( "%s: In %s -- No board on character!", __FUNCTION__, "Substate" );
      d->connected = CON_PLAYING;
      ch->substate = SUB_NONE;
      return;
   }

   /*
    * This will always kick a player out of the subsystem. I kept it '/a' as to not cause confusion
    * with players in the editor. 
    */
   if( !str_cmp( argument, "/a" ) )
   {
      ch->substate = SUB_EDIT_ABORT;
      d->connected = CON_PLAYING;
      do_note_write( ch, "" );
      return;
   }

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   switch ( ch->substate )
   {
      default:
         ch->substate = SUB_NONE;
         d->connected = CON_PLAYING;
         break;

      case SUB_BOARD_STICKY:
         if( !str_cmp( argument, "yes" ) || !str_cmp( argument, "y" ) )
         {
            ch->pcdata->pnote->expire = 0;
            ch_printf( ch, "%sThis note will not expire during pruning.&D\n\r", s1 );
         }
         else
         {
            ch->pcdata->pnote->expire = ch->pcdata->board->expire;
            ch_printf( ch, "%sNote set to expire after the default of %s%d%s day%s.&D\n\r",
                       s1, s2, ch->pcdata->pnote->expire, s1, ch->pcdata->pnote->expire == 1 ? "" : "s" );
         }

         if( ch->pcdata->pnote->parent )
            ch_printf( ch, "%sTo whom is this note addressed? %s(%sDefault: %s%s%s)&D   ", s1, s3, s1, s2,
                       ch->pcdata->pnote->parent->sender, s3 );
         else
            ch_printf( ch, "%sTo whom is this note addressed? %s(%sDefault: %sAll%s)&D   ", s1, s3, s1, s2, s3 );
         ch->substate = SUB_BOARD_TO;
         return;

      case SUB_BOARD_TO:
         if( !argument || argument[0] == '\0' )
         {
            if( IS_BOARD_FLAG( ch->pcdata->board, BOARD_PRIVATE ) && !IS_IMMORTAL( ch ) )
            {
               ch_printf( ch, "%sYou must specify a recipient:&D   ", s1 );
               return;
            }
            STRFREE( ch->pcdata->pnote->to_list );
            if( ch->pcdata->pnote->parent )
               ch->pcdata->pnote->to_list = STRALLOC( ch->pcdata->pnote->parent->sender );
            else
               ch->pcdata->pnote->to_list = STRALLOC( "All" );

            ch_printf( ch, "%sNo recipient specified. Defaulting to '%s%s%s'&D\n\r",
                       s1, s2, ch->pcdata->pnote->to_list, s1 );
         }
         else if( !str_cmp( argument, "all" )
                  && ( IS_IMMORTAL( ch ) || !IS_BOARD_FLAG( ch->pcdata->board, BOARD_PRIVATE ) ) )
         {
            ch_printf( ch, "%sYou can not send a message to '%sAll%s' on this board!\n\rYou must specify a recipient:&D   ",
                       s1, s2, s1 );
            return;
         }
         else
         {
            struct stat fst;
            char buf[MSL];

            snprintf( buf, MSL, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ) );
            if( stat( buf, &fst ) == -1 || !check_parse_name( capitalize( argument ), FALSE ) )
            {
               ch_printf( ch, "%sNo such player named '%s%s%s'.\n\rTo whom is this note addressed?   &D",
                          s1, s2, argument, s1 );
               return;
            }
            STRFREE( ch->pcdata->pnote->to_list );
            ch->pcdata->pnote->to_list = STRALLOC( argument );
         }

         ch_printf( ch, "%sTo: %s%-15s %sFrom: %s%s&D\n\r", s1, s2, ch->pcdata->pnote->to_list,
                    s1, s2, ch->pcdata->pnote->sender );
         if( ch->pcdata->pnote->subject )
         {
            ch_printf( ch, "%sSubject: %s%s&D\n\r", s1, s2, ch->pcdata->pnote->subject );
            ch->substate = SUB_BOARD_TEXT;
            ch_printf( ch, "%sPlease enter the text for your message:&D\n\r", s1 );
            editor_desc_printf( ch, "Please enter your note text:" );
            if( !ch->pcdata->pnote->text )
               ch->pcdata->pnote->text = str_dup( "" );
            start_editing( ch, ch->pcdata->pnote->text );
            return;
         }
         ch->substate = SUB_BOARD_SUBJECT;
         ch_printf( ch, "%sPlease enter a subject for this note:&D   ", s1 );
         return;

      case SUB_BOARD_SUBJECT:
         if( !argument || argument[0] == '\0' )
         {
            ch_printf( ch, "%sNo subject specified!&D\n\r%sYou must specify a subject:&D  ", s3, s1 );
            return;
         }
         else
         {
            DISPOSE( ch->pcdata->pnote->subject );
            ch->pcdata->pnote->subject = str_dup( argument );
         }

         ch->substate = SUB_BOARD_TEXT;
         ch_printf( ch, "%sTo: %s%-15s %sFrom: %s%s&D\n\r", s1, s2, ch->pcdata->pnote->to_list,
                    s1, s2, ch->pcdata->pnote->sender );
         ch_printf( ch, "%sSubject: %s%s&D\n\r", s1, s2, ch->pcdata->pnote->subject );
         ch_printf( ch, "%sPlease enter the text for your message:&D\n\r", s1 );
         editor_desc_printf( ch, "Please enter your note text:" );
         if( !ch->pcdata->pnote->text )
            ch->pcdata->pnote->text = str_dup( "" );
         start_editing( ch, ch->pcdata->pnote->text );
         return;

      case SUB_BOARD_CONFIRM:
         if( !argument || argument[0] == '\0' )
         {
            note_to_char( ch, ch->pcdata->pnote, NULL, 0 );
            ch_printf( ch, "%sYou %smust%s confirm! Is this correct? %s(%sType: %sY%s or %sN%s)&D   ",
                       s1, s3, s1, s3, s1, s2, s1, s2, s3 );
            return;
         }

         if( !str_cmp( argument, "y" ) || !str_cmp( argument, "yes" ) )
         {
            if( !ch->pcdata->pnote )
            {
               bug( "%s: NULL (ch)%s->pcdata->pnote!", __FUNCTION__, ch->name );
               d->connected = CON_PLAYING;
               ch->substate = SUB_NONE;
               return;
            }
            ch->pcdata->pnote->date_stamp = current_time;
            if( ch->pcdata->pnote->parent )
            {
               if( !IS_BOARD_FLAG( ch->pcdata->board, BOARD_PRIVATE ) )
               {
                  ch->pcdata->pnote->parent->reply_count++;
                  ch->pcdata->pnote->parent->date_stamp = ch->pcdata->pnote->date_stamp;
                  LINK( ch->pcdata->pnote, ch->pcdata->pnote->parent->first_reply, ch->pcdata->pnote->parent->last_reply,
                        next, prev );
               }
               else
               {
                  ch->pcdata->pnote->parent = NULL;
                  LINK( ch->pcdata->pnote, ch->pcdata->board->first_note, ch->pcdata->board->last_note, next, prev );
                  ch->pcdata->board->msg_count++;
               }
            }
            else
            {
               LINK( ch->pcdata->pnote, ch->pcdata->board->first_note, ch->pcdata->board->last_note, next, prev );
               ch->pcdata->board->msg_count++;
            }
            write_board( ch->pcdata->board );

            char_from_room( ch );
            if( !char_to_room( ch, ch->orig_room ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

            ch_printf( ch, "%sYou post the note '%s%s%s' on the %s%s%s board.&D\n\r",
                       s1, s2, ch->pcdata->pnote->subject, s1, s2, ch->pcdata->board->name, s1 );
            act( AT_GREY, "$n posts a note on the board.", ch, NULL, NULL, TO_ROOM );
            board_announce( ch, ch->pcdata->board, ch->pcdata->pnote );
            ch->pcdata->pnote = NULL;
            ch->pcdata->board = NULL;
            ch->substate = SUB_NONE;
            d->connected = CON_PLAYING;
            return;
         }

         if( !str_cmp( argument, "n" ) || !str_cmp( argument, "no" ) )
         {
            ch->substate = SUB_BOARD_REDO_MENU;
            ch_printf( ch, "%sWhat would you like to change?&D\n\r", s1 );
            ch_printf( ch, "   %s1%s) %sRecipients  %s[%s%s%s]&D\n\r", s2, s3, s1, s3, s2, ch->pcdata->pnote->to_list, s3 );
            if( ch->pcdata->pnote->parent )
               ch_printf( ch, "   %s2%s) %sYou cannot change subjects on a reply.&D\n\r", s2, s3, s1 );
            else
               ch_printf( ch, "   %s2%s) %sSubject     %s[%s%s%s]&D\n\r", s2, s3, s1, s3, s2,
                          ch->pcdata->pnote->subject, s3 );
            ch_printf( ch, "   %s3%s) %sText\n\r%s%s&D\n\r", s2, s3, s1, s2, ch->pcdata->pnote->text );
            ch_printf( ch, "%sPlease choose an option, %sQ%s to quit this menu, or %s/a%s to abort:&D   ",
                       s1, s2, s1, s2, s1 );
            return;
         }
         ch_printf( ch, "%sPlease enter either %sY%s or %sN%s:&D   ", s1, s2, s1, s2, s1 );
         return;

      case SUB_BOARD_REDO_MENU:
         if( !argument || argument[0] == '\0' )
         {
            ch_printf( ch, "%sPlease choose an option, %sQ%s to quit this menu, or %s/a%s to abort:&D   ",
                       s1, s2, s1, s2, s1 );
            return;
         }

         if( !str_cmp( argument, "1" ) )
         {
            if( ch->pcdata->pnote->parent )
               ch_printf( ch, "%sTo whom is this note addressed? %s(%sDefault: %s%s%s)&D   ", s1, s3, s1, s2,
                          ch->pcdata->pnote->parent->sender, s3 );
            else
               ch_printf( ch, "%sTo whom is this note addressed? %s(%sDefault: %sAll%s)&D   ", s1, s3, s1, s2, s3 );
            ch->substate = SUB_BOARD_TO;
            return;
         }

         if( !str_cmp( argument, "2" ) )
         {
            if( ch->pcdata->pnote->parent )
            {
               ch_printf( ch, "%sYou cannot change the subject of a reply!&D\n\r", s1 );
               ch_printf( ch, "%sPlease choose an option, %sQ%s to quit this menu, or %s/a%s to abort:&D   ",
                          s1, s2, s1, s2, s1 );
               return;
            }
            ch->substate = SUB_BOARD_SUBJECT;
            ch_printf( ch, "%sPlease enter a subject for this note:&D   ", s1 );
            return;
         }

         if( !str_cmp( argument, "3" ) )
         {
            ch->substate = SUB_BOARD_TEXT;
            ch_printf( ch, "%sPlease enter the text for your message:&D\n\r", s1 );
            editor_desc_printf( ch, "Please enter your note text:" );
            if( !ch->pcdata->pnote->text )
               ch->pcdata->pnote->text = str_dup( "" );
            start_editing( ch, ch->pcdata->pnote->text );
            return;
         }

         if( !str_cmp( argument, "q" ) )
         {
            ch->substate = SUB_BOARD_CONFIRM;
            note_to_char( ch, ch->pcdata->pnote, NULL, 0 );
            ch_printf( ch, "%sIs this correct? %s(%sY%s/%sN%s)&D   ", s1, s3, s2, s3, s2, s3 );
            return;
         }
         ch_printf( ch, "%sPlease choose an option, %sQ%s to quit this menu, or %s/a%s to abort:&D   ", s1, s2, s1, s2, s1 );
         return;
   }
   return;
}

CMDF do_note_write( CHAR_DATA * ch, char *argument )
{
   BOARD_DATA *board = NULL;
   ROOM_INDEX_DATA *board_room;
   char arg[MIL], buf[MSL];
   int n_num = 0;
   char s1[16], s2[16], s3[16];

   if( IS_NPC( ch ) )
      return;

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   switch ( ch->substate )
   {
      default:
         break;
         /*
          * Handle editor abort 
          */
      case SUB_EDIT_ABORT:
         send_to_char( "Aborting note...\n\r", ch );
         ch->substate = SUB_NONE;
         if( ch->pcdata->pnote )
            free_note( ch->pcdata->pnote );
         ch->pcdata->pnote = NULL;
         char_from_room( ch );
         if( !char_to_room( ch, ch->orig_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         ch->desc->connected = CON_PLAYING;
         act( AT_GREY, "$n aborts $s note writing.", ch, NULL, NULL, TO_ROOM );
         ch->pcdata->board = NULL;
         return;
      case SUB_BOARD_TEXT:
         if( ch->pcdata->pnote == NULL )
         {
            bug( "%s: SUB_BOARD_TEXT: Null pnote on character (%s)!", __FUNCTION__, ch->name );
            stop_editing( ch );
            return;
         }
         if( ch->pcdata->board == NULL )
         {
            bug( "%s: SUB_BOARD_TEXT: Null board on character (%s)!", __FUNCTION__, ch->name );
            stop_editing( ch );
            return;
         }
         DISPOSE( ch->pcdata->pnote->text );
         ch->pcdata->pnote->text = copy_buffer_nohash( ch );
         stop_editing( ch );
         if( ch->pcdata->pnote->text[0] == '\0' )
         {
            send_to_char( "You must enter some text in the body of the note!\n\r", ch );
            ch->substate = SUB_BOARD_TEXT;
            if( !ch->pcdata->pnote->text )
               ch->pcdata->pnote->text = str_dup( "" );
            start_editing( ch, ch->pcdata->pnote->text );
            editor_desc_printf( ch, "Please enter your note text:" );
            return;
         }
         note_to_char( ch, ch->pcdata->pnote, NULL, 0 );
         ch_printf( ch, "%sIs this correct? %s(%sY%s/%sN%s)&D   ", s1, s3, s2, s3, s2, s3 );
         ch->desc->connected = CON_BOARD;
         ch->substate = SUB_BOARD_CONFIRM;
         return;
   }

   /*
    * Stop people from dropping into the subsystem to escape a fight 
    */
   if( ch->fighting )
   {
      send_to_char( "Don't you think it best to finish your fight first?\n\r", ch );
      return;
   }

   if( !( board = find_board( ch ) ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch_printf( ch,
                    "%sWrites a new message for a board.\n\r%sSyntax: %swrite %s<%sboard%s> [%ssubject%s/%snote#%s]&D\n\r",
                    s1, s3, s1, s3, s2, s3, s2, s3, s2, s3 );
         ch_printf( ch, "%sNote: Subject and Note# are optional, but you can not specify both.&D\n\r", s1 );
         return;
      }

      argument = one_argument( argument, arg );
      if( !( board = get_board( ch, arg ) ) )
      {
         ch_printf( ch, "%sNo board found!&D\n\r", s1 );
         return;
      }
   }
   else
      ch_printf( ch, "%sUsing current board in room: %s%s&D\n\r", s1, s2, board->name );

   if( !can_post( ch, board ) )
   {
      send_to_char( "You can only read these notes.\n\r", ch );
      return;
   }

   if( is_number( argument ) )
      n_num = atoi( argument );

   ch->pcdata->board = board;
   snprintf( buf, MSL, "%d", ROOM_VNUM_BOARD );
   board_room = find_location( ch, buf );
   if( !board_room )
   {
      bug( "Missing board room: Vnum %d", ROOM_VNUM_BOARD );
      return;
   }
   ch_printf( ch, "%sTyping '%s/a%s' at any time will abort the note.&D\n\r", s3, s2, s3 );

   if( n_num )
   {
      NOTE_DATA *pnote;
      int i = 1;
      for( pnote = board->first_note; pnote; pnote = pnote->next )
      {
         if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, pnote ) )
            continue;
         if( i == n_num )
            break;
         i++;
      }

      if( !pnote )
      {
         send_to_char( "Note not found!\n\r", ch );
         return;
      }

      ch_printf( ch, "%sYou begin to write a reply for %s%s's%s note '%s%s%s'.&D\n\r", s1, s2, pnote->sender, s1, s2,
                 pnote->subject, s1 );
      act( AT_GREY, "$n departs for a moment, replying to a note.", ch, NULL, NULL, TO_ROOM );
      note_attach( ch );
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
      {
         STRFREE( ch->pcdata->pnote->to_list );
         ch->pcdata->pnote->to_list = STRALLOC( pnote->sender );
      }
      ch->pcdata->pnote->parent = pnote;
      DISPOSE( ch->pcdata->pnote->subject );
      strdup_printf( &ch->pcdata->pnote->subject, "Re: %s", ch->pcdata->pnote->parent->subject );
      ch->desc->connected = CON_BOARD;
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
      {
         ch->substate = SUB_BOARD_TEXT;
         ch_printf( ch, "%sTo: %s%-15s %sFrom: %s%s&D\n\r", s1, s2, ch->pcdata->pnote->to_list,
                    s1, s2, ch->pcdata->pnote->sender );
         ch_printf( ch, "%sSubject: %s%s&D\n\r", s1, s2, ch->pcdata->pnote->subject );
         ch_printf( ch, "%sPlease enter the text for your message:&D\n\r", s1 );
         if( !ch->pcdata->pnote->text )
            ch->pcdata->pnote->text = str_dup( "" );
         start_editing( ch, ch->pcdata->pnote->text );
         editor_desc_printf( ch, "Please enter your note text:" );
      }
      else
      {
         ch_printf( ch, "%sTo whom is this note addressed? %s(%sDefault: %s%s%s)&D   ", s1, s3, s1, s2, pnote->sender, s3 );
         ch->substate = SUB_BOARD_TO;
      }
      ch->orig_room = ch->in_room;
      char_from_room( ch );
      if( !char_to_room( ch, board_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }

   note_attach( ch );
   if( argument && argument[0] != '\0' )
   {
      DISPOSE( ch->pcdata->pnote->subject );
      ch->pcdata->pnote->subject = str_dup( argument );
      ch_printf( ch, "%sYou begin to write a new note for the %s%s%s board, titled '%s%s%s'.&D\n\r",
                 s1, s2, board->name, s1, s2, ch->pcdata->pnote->subject, s1 );
   }
   else
      ch_printf( ch, "%sYou begin to write a new note for the %s%s%s board.&D\n\r", s1, s2, board->name, s1 );

   if( can_remove( ch, board ) && !IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
   {
      ch_printf( ch, "%sIs this a sticky note? %s(%sY%s/%sN%s)  (%sDefault: %sN%s)&D   ",
                 s1, s3, s2, s3, s2, s3, s1, s2, s3 );
      ch->substate = SUB_BOARD_STICKY;
   }
   else
   {
      ch->pcdata->pnote->expire = ch->pcdata->board->expire;
      ch->substate = SUB_BOARD_TO;
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !can_remove( ch, board ) )
         ch_printf( ch, "%sTo whom is this note addressed?&D   ", s1 );
      else
         ch_printf( ch, "%sTo whom is this note addressed? %s(%sDefault: %sAll%s)&D   ", s1, s3, s1, s2, s3 );
   }
   act( AT_GREY, "$n begins to write a new note.", ch, NULL, NULL, TO_ROOM );
   ch->desc->connected = CON_BOARD;
   ch->orig_room = ch->in_room;
   char_from_room( ch );
   if( !char_to_room( ch, board_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   return;
}

CMDF do_note_read( CHAR_DATA * ch, char *argument )
{
   BOARD_DATA *board = NULL;
   NOTE_DATA *pnote;
   BOARD_CHARDATA *pboard = NULL;
   int n_num = 0, i = 1;
   char arg[MIL];
   char s1[16], s2[16], s3[16];

   if( IS_NPC( ch ) )
      return;

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   if( !argument || argument[0] == '\0' )
   {
      if( !( board = find_board( ch ) ) )
      {
         board = ch->pcdata->board;
         if( !board )
         {
            for( board = first_board; board; board = board->next )
            {
               if( ( !can_remove( ch, board ) || !IS_IMMORTAL( ch ) ) && board->objvnum > 0 )
                  continue;
               if( !can_read( ch, board ) )
                  continue;
               pboard = get_chboard( ch, board->name );
               if( pboard && pboard->alert == BD_IGNORE )
                  continue;
               if( unread_notes( ch, board ) > 0 )
                  break;
            }
            if( !board )
            {
               ch_printf( ch, "%sThere are no boards with unread messages&D\n\r", s1 );
               return;
            }
         }
      }
      pboard = create_chboard( ch, board->name );
      for( pnote = board->first_note; pnote; pnote = pnote->next )
      {
         if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, pnote ) )
            continue;
         n_num++;
         if( pnote->date_stamp > pboard->last_read )
            break;
      }

      if( !pnote )
      {
         ch_printf( ch, "%sThere are no more unread messages on this board.&D\n\r", s1 );
         ch->pcdata->board = NULL;
         return;
      }

      note_to_char( ch, pnote, board, n_num );
      pboard->last_read = pnote->date_stamp;
      act( AT_GREY, "$n reads a note.", ch, NULL, NULL, TO_ROOM );
      return;
   }

   if( !( board = find_board( ch ) ) )
   {
      argument = one_argument( argument, arg );
      if( !( board = get_board( ch, arg ) ) )
      {
         ch_printf( ch, "%sNo board found!&D\n\r", s1 );
         return;
      }
   }
   else
      ch_printf( ch, "%sUsing current board in room: %s%s&D\n\r", s1, s2, board->name );

   if( !can_read( ch, board ) )
   {
      ch_printf( ch, "%sYou are unable to comprehend the notes on this board.&D\n\r", s1 );
      return;
   }

   n_num = atoi( argument );

   /*
    * If we have a board, but no note specified, then treat it as if we're just scanning through 
    */
   if( n_num == 0 )
   {
      if( board->objvnum == 0 )
         ch->pcdata->board = board;
      interpret( ch, "read" );
      return;
   }

   i = 1;
   for( pnote = board->first_note; pnote; pnote = pnote->next )
   {
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, pnote ) && !can_remove( ch, board ) )
         continue;
      if( i == n_num )
         break;
      i++;
   }

   if( !pnote )
   {
      ch_printf( ch, "%sNote #%s%d%s not found on the %s%s%s board.&D\n\r", s1, s2, n_num, s1, s2, board->name, s1 );
      return;
   }

   note_to_char( ch, pnote, board, n_num );
   pboard = create_chboard( ch, board->name );
   if( pboard->last_read < pnote->date_stamp )
      pboard->last_read = pnote->date_stamp;
   act( AT_GREY, "$n reads a note.", ch, NULL, NULL, TO_ROOM );
   return;
}

CMDF do_note_list( CHAR_DATA * ch, char *argument )
{
   BOARD_DATA *board = NULL;
   NOTE_DATA *pnote;
   BOARD_CHARDATA *chboard;
   char buf[MSL];
   char unread;
   int count = 0;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   if( !( board = find_board( ch ) ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch_printf( ch, "%sLists the note on a board.\n\r%sSyntax: %sreview %s<%sboard%s>&D\n\r", s1, s3, s1, s3, s2, s3 );
         return;
      }

      if( !( board = get_board( ch, argument ) ) )
      {
         ch_printf( ch, "%sNo board found!&D\n\r", s1 );
         return;
      }
   }
   else
      ch_printf( ch, "%sUsing current board in room: %s%s&D\n\r", s1, s2, board->name );

   if( !can_read( ch, board ) )
   {
      send_to_char( "You are unable to comprehend the notes on this board.\n\r", ch );
      return;
   }

   chboard = get_chboard( ch, board->name );

   snprintf( buf, MSL, "%s--[ %sNotes on %s%s%s ]--", s3, s1, s2, board->name, s3 );
   ch_printf( ch, "\n\r%s\n\r", color_align( buf, 80, ALIGN_CENTER ) );
   act_printf( AT_GREY, ch, NULL, NULL, TO_ROOM, "&w$n reviews the notes on the &W%s&w board.", board->name );

   if( total_notes( ch, board ) == 0 )
   {
      send_to_char( "\n\rNo messages...\n\r", ch );
      return;
   }
   else
      ch_printf( ch, "%sNum   %s%-17s %-11s %s&D\n\r", s1,
                 IS_BOARD_FLAG( board, BOARD_PRIVATE ) ? "" : "Replies ", "Date", "Author", "Subject" );

   count = 0;
   for( pnote = board->first_note; pnote; pnote = pnote->next )
   {
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, pnote ) && !can_remove( ch, board ) )
         continue;

      count++;
      if( !chboard || chboard->last_read < pnote->date_stamp )
         unread = '*';
      else
         unread = ' ';

      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
      {
         ch_printf( ch, "%s%2d%s) &C%c %s[%s%-15s%s] %s%-11s %s&D\n\r", s2, count, s3, unread, s3, s2,
                    mini_c_time( pnote->date_stamp, ch->pcdata->timezone ), s3, s2,
                    pnote->sender ? pnote->sender : "--Error--", pnote->subject ? print_lngstr( pnote->subject, 37 ) : "" );
      }
      else
      {
         ch_printf( ch, "%s%2d%s) &C%c %s[ %s%3d%s ] [%s%-15s%s] %s%-11s %-20s&D\n\r", s2, count, s3, unread, s3, s2,
                    pnote->reply_count, s3, s2, mini_c_time( pnote->date_stamp, ch->pcdata->timezone ), s3, s2,
                    pnote->sender ? pnote->sender : "--Error--", pnote->subject ? print_lngstr( pnote->subject, 45 ) : "" );
      }
   }
   ch_printf( ch, "\n\r%sThere %s %s%d%s message%s on this board.&D\n\r", s1, count == 1 ? "is" : "are", s2,
              count, s1, count == 1 ? "" : "s" );
   ch_printf( ch, "%sA &C*%s denotes unread messages.&D\n\r", s1, s1 );
   return;
}

CMDF do_note_remove( CHAR_DATA * ch, char *argument )
{
   BOARD_DATA *board;
   NOTE_DATA *pnote;
   char arg[MIL];
   short n_num = 0, r_num = 0, i = 0;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   if( !( board = find_board( ch ) ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch_printf( ch, "%sRemoves a note from the board.\n\r%sSyntax: %serase %s<%sboard%s> <%snote#%s>.[%sreply#%s]&D\n\r",
                    s1, s3, s1, s3, s2, s3, s2, s3, s2, s3 );
         return;
      }

      argument = one_argument( argument, arg );
      if( !( board = get_board( ch, arg ) ) )
      {
         ch_printf( ch, "%sNo board found!&D\n\r", s1 );
         return;
      }
   }
   else
      ch_printf( ch, "%sUsing current board in room: %s%s&D\n\r", s1, s2, board->name );

   if( !IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !can_remove( ch, board ) )
   {
      send_to_char( "You are unable to remove the notes on this board.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Remove which note?\n\r", ch );
      return;
   }

   while( 1 )
   {
      if( argument[i] == '.' )
      {
         r_num = atoi( argument + i + 1 );
         argument[i] = '\0';
         n_num = atoi( argument );
         break;
      }
      if( argument[i] == '\0' )
      {
         n_num = atoi( argument );
         r_num = -1;
         break;
      }
      i++;
   }

   n_num = atoi( argument );

   if( n_num == 0 )
   {
      send_to_char( "Remove which note?\n\r", ch );
      return;
   }

   if( r_num == 0 )
   {
      send_to_char( "Remove which reply?\n\r", ch );
      return;
   }

   i = 1;
   for( pnote = board->first_note; pnote; pnote = pnote->next )
   {
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, pnote ) && !can_remove( ch, board ) )
         continue;
      if( i == n_num )
         break;
      i++;
   }

   if( !pnote )
   {
      send_to_char( "Note not found!\n\r", ch );
      return;
   }

   if( is_name( "all", pnote->to_list ) && !can_remove( ch, board ) )
   {
      send_to_char( "You can not remove that note.\n\r", ch );
      return;
   }

   if( r_num > 0 )
   {
      NOTE_DATA *reply;

      i = 1;
      for( reply = pnote->first_reply; reply; reply = reply->next )
         if( i == r_num )
            break;
         else
            i++;

      if( !reply )
      {
         send_to_char( "Reply not found!\n\r", ch );
         return;
      }

      ch_printf( ch, "%sYou remove the reply from %s%s%s, titled '%s%s%s' from the %s%s%s board.&D\n\r",
                 s1, s2, reply->sender ? reply->sender : "--Error--", s1,
                 s2, reply->subject ? reply->subject : "--Error--", s1, s2, board->name, s1 );
      note_remove( board, reply );
      act( AT_GREY, "$n removes a reply from the board.", ch, NULL, NULL, TO_ROOM );
      return;
   }

   ch_printf( ch, "%sYou remove the note from %s%s%s, titled '%s%s%s' from the %s%s%s board.&D\n\r",
              s1, s2, pnote->sender ? pnote->sender : "--Error--", s1,
              s2, pnote->subject ? pnote->subject : "--Error--", s1, s2, board->name, s1 );
   note_remove( board, pnote );
   act( AT_GREY, "$n removes a note from the board.", ch, NULL, NULL, TO_ROOM );
   return;
}

CMDF do_board_list( CHAR_DATA * ch, char *argument )
{
   BOARD_DATA *board;
   OBJ_DATA *obj;
   char buf[MSL];
   int count = 0;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   ch_printf( ch, "%s                              Total   Total&D\n\r", s1 );
   ch_printf( ch, "%sNum  Name             Unread  Posts  Replies Description&D\n\r", s1 );
   for( board = first_board; board; board = board->next )
   {
      /*
       * If you can't read the board, you can't see it either. 
       */
      if( !can_read( ch, board ) )
         continue;

      /*
       * Not a global board and you're not a moderator or immortal, you can't see it. 
       */
      if( board->objvnum > 0 && ( !can_remove( ch, board ) && !IS_IMMORTAL( ch ) ) )
         continue;

      /*
       * Everyone who can see it sees this same information 
       */
      ch_printf( ch, "%s%2d%s)  %s%-15s  %s[ %s%3d%s]  ", s2, count + 1, s3, s2, print_lngstr( board->name, 15 ), s3, s2,
                 unread_notes( ch, board ), s3 );
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
         ch_printf( ch, "[%s%3d%s]          ", s2, total_notes( ch, board ), s3 );
      else
         ch_printf( ch, "[%s%3d%s]  [%s%3d%s]   ", s2, total_notes( ch, board ), s3, s2, total_replies( board ), s3 );

      ch_printf( ch, "%s%-25s", s2, board->desc ? print_lngstr( board->desc, 25 ) : "" );

      if( IS_IMMORTAL( ch ) )
      {
         if( board->objvnum )
         {
            snprintf( buf, MSL, "%d", board->objvnum );
            if( ( obj = get_obj_world( ch, buf ) ) )
               ch_printf( ch, " %s[%sV#%s%-5d", s3, s1, s2, board->objvnum );
         }
      }
      send_to_char( "&D\n\r", ch );
      count++;
   }
   if( count == 0 )
      ch_printf( ch, "%sNo boards to list.&D\n\r", s1 );
   else
      ch_printf( ch, "\n\r%s%d%s board%s found.&D\n\r", s2, count, s1, count == 1 ? "" : "s" );
   return;
}

CMDF do_board_alert( CHAR_DATA * ch, char *argument )
{
   BOARD_CHARDATA *chboard;
   BOARD_DATA *board;
   char arg[MIL];
   int bd_value = -1;
   char s1[16], s2[16], s3[16];
   static char *const bd_alert_string[] = { "None Set", "Announce", "Ignoring" };

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   if( !argument || argument[0] == '\0' )
   {
      for( board = first_board; board; board = board->next )
      {
         if( !can_read( ch, board ) )
            continue;
         chboard = get_chboard( ch, board->name );
         ch_printf( ch, "%s%-20s   %sAlert: %s%s&D\n\r", s2, board->name, s1, s2,
                    chboard ? bd_alert_string[chboard->alert] : bd_alert_string[0] );
      }
      ch_printf( ch, "%sTo change an alert for a board, type: %salert <board> <none|announce|ignore>&D\n\r", s1, s2 );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( board = get_board( NULL, arg ) ) || !can_read( ch, board ) )
   {
      ch_printf( ch, "%sSorry, but the board '%s%s%s' does not exsist.&D\n\r", s1, s2, arg, s1 );
      return;
   }

   if( !str_cmp( argument, "none" ) )
      bd_value = 0;
   if( !str_cmp( argument, "announce" ) )
      bd_value = 1;
   if( !str_cmp( argument, "ignore" ) )
      bd_value = 2;

   if( bd_value == -1 )
   {
      ch_printf( ch, "%sSorry, but '%s%s%s' is not a valid argument.&D\n\r", s1, s2, argument, s1 );
      ch_printf( ch, "%sPlease choose one of: %snone announce ignore&D\n\r", s1, s2 );
      return;
   }

   chboard = create_chboard( ch, board->name );
   chboard->alert = bd_value;
   ch_printf( ch, "%sAlert for the %s%s%s board set to %s%s%s.&D\n\r", s1, s2, board->name, s1, s2, argument, s1 );
   return;
}

/* Much like do_board_list, but I cut out some of the details here for simplicity */
CMDF do_checkboards( CHAR_DATA * ch, char *argument )
{
   BOARD_DATA *board;
   BOARD_CHARDATA *chboard;
   OBJ_DATA *obj;
   char buf[MSL];
   int count = 0;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   ch_printf( ch, "%s Num  %-20s  Unread  %-40s&D\n\r", s1, "Name", "Description" );

   for( board = first_board; board; board = board->next )
   {
      /*
       * If you can't read the board, you can't see it either. 
       */
      if( !can_read( ch, board ) )
         continue;

      /*
       * Not a global board and you're not a moderator or immortal, you can't see it. 
       */
      if( board->objvnum > 0 && ( !can_remove( ch, board ) && !IS_IMMORTAL( ch ) ) )
         continue;

      /*
       * We only want to see boards with NEW posts 
       */
      if( unread_notes( ch, board ) == 0 )
         continue;

      chboard = get_chboard( ch, board->name );
      /*
       * If we're ignoring it, then who cares? Well we do if the Immortals set it to ANNOUNCE 
       */
      if( chboard && chboard->alert == BD_IGNORE && !IS_BOARD_FLAG( board, BOARD_ANNOUNCE ) )
         continue;
      count++;
      /*
       * Everyone who can see it sees this same information 
       */
      ch_printf( ch, "%s%3d%s)  %s%-20s  %s[%s%3d%s]  %s%-30s", s2, count, s3, s2, board->name,
                 s3, s2, unread_notes( ch, board ), s3, s2, board->desc ? board->desc : "" );

      if( IS_IMMORTAL( ch ) )
      {
         snprintf( buf, MSL, "%d", board->objvnum );
         if( ( obj = get_obj_world( ch, buf ) ) && ( obj->in_room != NULL ) )
            ch_printf( ch, " %s[%sObj# %s%-5d %s@ %sRoom# %s%-5d%s]", s3, s1, s2, board->objvnum,
                       s3, s1, s2, obj->in_room->vnum, s3 );
         else
            ch_printf( ch, " %s[ %sGlobal Board %s]", s3, s2, s3 );
      }
      send_to_char( "&D\n\r", ch );
   }
   if( count == 0 )
      ch_printf( ch, "%sNo unread messages...\n\r", s1 );
   return;
}

CMDF do_board_stat( CHAR_DATA * ch, char *argument )
{
   BOARD_DATA *board;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: bstat <board>\n\r", ch );
      return;
   }

   if( !( board = get_board( ch, argument ) ) )
   {
      ch_printf( ch, "&wSorry, '&W%s&w' is not a valid board.\n\r", argument );
      return;
   }

   ch_printf( ch, "%sFilename: %s%-20s&D\n\r", s1, s2, board->filename );
   ch_printf( ch, "%sName:       %s%-30s%s ObjVnum:      ", s1, s2, board->name, s1 );
   if( board->objvnum > 0 )
      ch_printf( ch, "%s%d&D\n\r", s2, board->objvnum );
   else
      ch_printf( ch, "%sGlobal Board&D\n\r", s2 );
   ch_printf( ch, "%sReaders:    %s%-30s%s Read Level:   %s%d&D\n\r", s1, s2,
              board->readers ? board->readers : "none set", s1, s2, board->read_level );
   ch_printf( ch, "%sPosters:    %s%-30s%s Post Level:   %s%d&D\n\r", s1, s2,
              board->posters ? board->posters : "none set", s1, s2, board->post_level );
   ch_printf( ch, "%sModerators: %s%-30s%s Remove Level: %s%d&D\n\r", s1, s2,
              board->moderators ? board->moderators : "none set", s1, s2, board->remove_level );
   ch_printf( ch, "%sGroup:      %s%-30s%s Expiration:   %s%d&D\n\r", s1, s2,
              board->group ? board->group : "none set", s1, s2, board->expire );
   ch_printf( ch, "%sFlags: %s[%s%s%s]&D\n\r", s1, s3, s2,
              ext_flag_string( &board->flags, board_flags )[0] == '\0' ? "none set" : ext_flag_string( &board->flags,
                                                                                                       board_flags ), s3 );
   ch_printf( ch, "%sDescription: %s%-30s&D\n\r", s1, s2, board->desc ? board->desc : "none set" );
   return;
}

CMDF do_board_remove( CHAR_DATA * ch, char *argument )
{
   BOARD_DATA *board;
   char arg[MIL];
   char buf[MSL];
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   if( !argument || argument[0] == '\0' )
   {
      ch_printf( ch, "%sYou must select a board to remove.&D\n\r", s1 );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( board = get_board( ch, arg ) ) )
   {
      ch_printf( ch, "%sSorry, '%s%s%s' is not a valid board.&D\n\r", s1, s2, argument, s1 );
      return;
   }

   if( !str_cmp( argument, "yes" ) )
   {
      ch_printf( ch, "&RDeleting board '&W%s&R'.&D\n\r", board->name );
      send_to_char( "&wDeleting note file...   ", ch );
      snprintf( buf, MSL, "%s%s.board", BOARD_DIR, board->filename );
      unlink( buf );
      send_to_char( "&RDeleted&D\n\r&wDeleting board...   ", ch );
      free_board( board );
      send_to_char( "&RDeleted&D\n\r&wSaving boards...   ", ch );
      write_boards(  );
      send_to_char( "&GDone.&D\n\r", ch );
      return;
   }
   else
   {
      ch_printf( ch, "&RRemoving a board will also delete *ALL* posts on that board!\n\r&wTo continue, type: "
                 "&YBOARD REMOVE '%s' YES&w.", board->name );
      return;
   }
   return;
}

CMDF do_board_make( CHAR_DATA * ch, char *argument )
{
   BOARD_DATA *board;
   char arg[MIL];

   argument = one_argument( argument, arg );

   if( !argument || argument[0] == '\0' || !arg || arg[0] == '\0' )
   {
      send_to_char( "Usage: makeboard <filename> <name>\n\r", ch );
      return;
   }

   if( strlen( argument ) > 20 || strlen( arg ) > 20 )
   {
      send_to_char( "Please limit board names and filenames to 20 characters!\n\r", ch );
      return;
   }

   smash_tilde( argument );

   CREATE( board, BOARD_DATA, 1 );

   LINK( board, first_board, last_board, next, prev );
   board->name = STRALLOC( argument );
   board->filename = str_dup( arg );
   board->readers = NULL;
   board->posters = NULL;
   board->moderators = NULL;
   board->group = NULL;
   board->desc = str_dup( "This is a new board!" );
   board->first_note = NULL;
   board->last_note = NULL;
   board->objvnum = 0;
   board->expire = MAX_BOARD_EXPIRE;
   board->read_level = ch->level;
   board->post_level = ch->level;
   board->remove_level = ch->level;
   write_board( board );
   write_boards(  );

   send_to_char( "&wNew board created: \n\r", ch );
   do_board_stat( ch, board->name );
   send_to_char( "&wNote: Boards default to &CGlobal&w when created. See '&Ybset&w' to change this!\n\r", ch );
   return;
}

CMDF do_board_set( CHAR_DATA * ch, char *argument )
{
   BOARD_DATA *board;
   char arg1[MIL], arg2[MIL], arg3[MIL], buf[MSL];
   int value = -1;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", color_str( AT_BOARD, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_BOARD2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_BOARD3, ch ) );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !arg1 || arg1[0] == '\0' || ( str_cmp( arg1, "purge" ) && ( !arg2 || arg2[0] == '\0' ) ) )
   {
      send_to_char( "Usage: bset <board> <field> value\n\r", ch );
      send_to_char( "   Or: bset purge (this option forces a check on expired notes)\n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( "  objvnum read_level post_level remove_level expire group\n\r", ch );
      send_to_char( "  readers posters moderators desc name global flags filename\n\r", ch );
      return;
   }

   if( is_number( argument ) )
      value = atoi( argument );

   if( !str_cmp( arg1, "purge" ) )
   {
      log_printf( "Manual board pruning started by %s.", ch->name );
      check_boards( 0 );
      return;
   }

   if( !( board = get_board( ch, arg1 ) ) )
   {
      ch_printf( ch, "%sSorry, but '%s%s%s' is not a valid board.&D\n\r", s1, s2, arg1, s1 );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      bool fMatch = FALSE, fUnknown = FALSE;

      ch_printf( ch, "%sSetting flags: %s", s1, s2 );
      snprintf( buf, MSL, "\n\r%sUnknown flags: %s", s1, s2 );
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_board_flag( arg3 );
         if( value < 0 || value > MAX_BITS )
         {
            fUnknown = TRUE;
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), " %s", arg3 );
         }
         else
         {
            fMatch = TRUE;
            xTOGGLE_BIT( board->flags, value );
            if( IS_BOARD_FLAG( board, value ) )
               ch_printf( ch, " +%s", arg3 );
            else
               ch_printf( ch, " -%s", arg3 );
         }
      }
      ch_printf( ch, "%s%s&D\n\r", fMatch ? "" : "none", fUnknown ? buf : "" );
      write_boards(  );
      return;
   }

   if( !str_cmp( arg2, "objvnum" ) )
   {
      if( !get_obj_index( value ) )
      {
         send_to_char( "No such object.\n\r", ch );
         return;
      }
      board->objvnum = value;
      write_boards(  );
      ch_printf( ch, "%sThe objvnum for '%s%s%s' has been set to '%s%d%s'.&D\n\r", s1, s2,
                 board->name, s1, s2, board->objvnum, s1 );
      return;
   }

   if( !str_cmp( arg2, "global" ) )
   {
      board->objvnum = 0;
      write_boards(  );
      ch_printf( ch, "%s%s%s is now a global board.\n\r", s2, board->name, s1 );
      return;
   }

   if( !str_cmp( arg2, "read_level" ) || !str_cmp( arg2, "post_level" ) || !str_cmp( arg2, "remove_level" ) )
   {
      if( value < 0 || value > ch->level )
      {
         ch_printf( ch, "%s%d%s is out of range.\n\rValues range from %s1%s to %s%d%s.&D\n\r", s2, value, s1, s2,
                    s1, s2, ch->level, s1 );
         return;
      }
      if( !str_cmp( arg2, "read_level" ) )
         board->read_level = value;
      else if( !str_cmp( arg2, "post_level" ) )
         board->post_level = value;
      else if( !str_cmp( arg2, "remove_level" ) )
         board->remove_level = value;
      else
      {
         ch_printf( ch, "%sUnknown field '%s%s%s'.&D\n\r", s1, s2, arg2, s1 );
         return;
      }
      write_boards(  );
      ch_printf( ch, "%sThe %s%s%s for '%s%s%s' has been set to '%s%d%s'.&D\n\r", s1, s3, arg2,
                 s1, s2, board->name, s1, s2, value, s1 );
      return;
   }

   if( !str_cmp( arg2, "readers" ) || !str_cmp( arg2, "posters" ) || !str_cmp( arg2, "moderators" ) )
   {
      if( !str_cmp( arg2, "readers" ) )
      {
         STRFREE( board->readers );
         if( !argument || argument[0] == '\0' )
            board->readers = NULL;
         else
            board->readers = STRALLOC( argument );
      }
      else if( !str_cmp( arg2, "posters" ) )
      {
         STRFREE( board->posters );
         if( !argument || argument[0] == '\0' )
            board->posters = NULL;
         else
            board->posters = STRALLOC( argument );
      }
      else if( !str_cmp( arg2, "moderators" ) )
      {
         STRFREE( board->moderators );
         if( !argument || argument[0] == '\0' )
            board->moderators = NULL;
         else
            board->moderators = STRALLOC( argument );
      }
      else
      {
         ch_printf( ch, "%sUnknown field '%s%s%s'.&D\n\r", s1, s2, arg2, s1 );
         return;
      }
      write_boards(  );
      ch_printf( ch, "%sThe %s%s%s for '%s%s%s' have been set to '%s%s%s'.\n\r", s1, s3, arg2,
                 s1, s2, board->name, s1, s2, argument[0] != '\0' ? argument : "(nothing)", s1 );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !is_valid_filename( ch, BOARD_DIR, argument ) )
         return;

      if( strlen( argument ) > 20 )
      {
         send_to_char( "Please limit board filenames to 20 characters!\n\r", ch );
         return;
      }
      snprintf( filename, 256, "%s%s.board", BOARD_DIR, board->filename );
      unlink( filename );
      mudstrlcpy( filename, board->filename, 256 );
      DISPOSE( board->filename );
      board->filename = str_dup( argument );
      write_boards(  );
      write_board( board );
      ch_printf( ch, "%sThe filename for '%s%s%s' has been changed to '%s%s%s'.\n\r", s1, s2, filename, s1, s2,
                 board->filename, s1 );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "No name specified.\n\r", ch );
         return;
      }
      if( strlen( argument ) > 20 )
      {
         send_to_char( "Please limit board names to 20 characters!\n\r", ch );
         return;
      }
      mudstrlcpy( buf, board->name, MSL );
      STRFREE( board->name );
      board->name = STRALLOC( argument );
      write_boards(  );
      ch_printf( ch, "%sThe name for '%s%s%s' has been changed to '%s%s%s'.\n\r", s1, s2, buf, s1, s2, board->name, s1 );
      return;
   }

   if( !str_cmp( arg2, "expire" ) )
   {
      if( value < 0 || value > MAX_BOARD_EXPIRE )
      {
         ch_printf( ch, "%sExpire time must be a value between %s0%s and %s%d%s.&D\n\r", s1, s2, s1, s2,
                    MAX_BOARD_EXPIRE, s1 );
         return;
      }

      board->expire = value;
      ch_printf( ch, "%sFrom now on, notes on the %s%s%s board will expire after %s%d%s days.\n\r", s1, s2,
                 board->name, s1, s2, board->expire, s1 );
      ch_printf( ch, "%sPlease note: This will not effect notes currently on the board. To effect %sALL%s notes, "
                 "type: %sbset <board> expireall <days>&D\n\r", s1, s3, s1, s2 );
      write_boards(  );
      return;
   }

   if( !str_cmp( arg2, "expireall" ) )
   {
      NOTE_DATA *pnote;

      if( value < 0 || value > MAX_BOARD_EXPIRE )
      {
         ch_printf( ch, "%sExpire time must be a value between %s0%s and %s%d%s.&D\n\r", s1, s2, s1, s2,
                    MAX_BOARD_EXPIRE, s1 );
         return;
      }

      board->expire = value;
      for( pnote = board->first_note; pnote; pnote = pnote->next )
         pnote->expire = value;

      ch_printf( ch, "%sAll notes on the %s%s%s board will expire after %s%d%s days.&D\n\r", s1, s2, board->name, s1,
                 s2, board->expire, s1 );
      send_to_char( "Performing board pruning now...\n\r", ch );
      board_check_expire( board );
      write_boards(  );
      return;
   }

   if( !str_cmp( arg2, "desc" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "No description specified.\n\r", ch );
         return;
      }
      if( strlen( argument ) > 30 )
      {
         send_to_char( "Please limit your board descriptions to 30 characters.\n\r", ch );
         return;
      }
      DISPOSE( board->desc );
      board->desc = str_dup( argument );
      write_boards(  );
      ch_printf( ch, "%sThe desc for %s%s%s has been set to '%s%s%s'.&D\n\r", s1, s2, board->name, s1, s2, board->desc, s1 );
      return;
   }

   if( !str_cmp( arg2, "group" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         STRFREE( board->group );
         board->group = NULL;
         send_to_char( "Group cleared.\n\r", ch );
         return;
      }
      STRFREE( board->group );
      board->group = STRALLOC( argument );
      write_boards(  );
      ch_printf( ch, "%sThe group for %s%s%s has been set to '%s%s%s'.&D\n\r", s1, s2, board->name,
                 s1, s2, board->group, s1 );
      return;
   }

   if( !str_cmp( arg2, "raise" ) )
   {
      BOARD_DATA *insert_board = NULL;
      if( board == first_board )
      {
         ch_printf( ch, "%sBut '%s%s%s' is already the first board!&D\n\r", s1, s2, board->name, s1 );
         return;
      }

      insert_board = board->prev;
      UNLINK( board, first_board, last_board, next, prev );
      INSERT( board, insert_board, first_board, next, prev );

      ch_printf( ch, "%sMoved '%s%s%s' above '%s%s%s'.&D\n\r", s1, s2, board->name, s1, s2, board->next->name, s1 );
      return;
   }

   if( !str_cmp( arg2, "lower" ) )
   {
      if( board == last_board )
      {
         ch_printf( ch, "%sBut '%s%s%s' is already the last board!&D\n\r", s1, s2, board->name, s1 );
         return;
      }

      if( board->next == last_board )
      {
         UNLINK( board, first_board, last_board, next, prev );
         LINK( board, first_board, last_board, next, prev );
      }
      else
      {
         BOARD_DATA *insert_board = NULL;
         insert_board = board->next;
         UNLINK( board, first_board, last_board, next, prev );
         INSERT( board, insert_board, first_board, next, prev );
      }
      ch_printf( ch, "%sMoved '%s%s%s' below '%s%s%s'.&D\n\r", s1, s2, board->name, s1, s2, board->prev->name, s1 );
      return;
   }

   do_board_set( ch, "" );
   return;
}

/* Begin Project Code */

#define PROJ_ADMIN( proj, ch )   ( !IS_NPC((ch)) && ( (((proj)->type == 1) && ((ch)->pcdata->realm == REALM_HEAD_CODER)) \
                                  || (((proj)->type == 2) && ((ch)->pcdata->realm == REALM_HEAD_BUILDER)) \
                                  || ((ch)->pcdata->realm == REALM_IMP)))

PROJECT_DATA *get_project_by_number( int pnum )
{
   int pcount = 1;
   PROJECT_DATA *pproject;

   for( pproject = first_project; pproject; pproject = pproject->next )
   {
      if( pcount == pnum )
         return pproject;
      else
         pcount++;
   }
   return NULL;
}

NOTE_DATA *get_log_by_number( PROJECT_DATA * pproject, int pnum )
{
   int pcount = 1;
   NOTE_DATA *plog;

   for( plog = pproject->first_log; plog; plog = plog->next )
   {
      if( pcount == pnum )
         return plog;
      else
         pcount++;
   }
   return NULL;
}

void write_projects( void )
{
   PROJECT_DATA *project;
   NOTE_DATA *nlog;
   FILE *fpout;

   fpout = fopen( PROJECTS_FILE, "w" );
   if( !fpout )
   {
      bug( "%s", "FATAL: cannot open projects.txt for writing!" );
      return;
   }
   for( project = first_project; project; project = project->next )
   {
      fprintf( fpout, "%s", "Version        2\n" );
      fprintf( fpout, "Name		   %s~\n", project->name );
      fprintf( fpout, "Owner		   %s~\n", ( project->owner ) ? project->owner : "None" );
      if( project->coder )
         fprintf( fpout, "Coder		    %s~\n", project->coder );
      fprintf( fpout, "Status		   %s~\n", ( project->status ) ? project->status : "No update." );
      fprintf( fpout, "Date_stamp   %ld\n", ( long int )project->date_stamp );
      fprintf( fpout, "Type         %d\n", project->type );
      if( project->description )
         fprintf( fpout, "Description         %s~\n", project->description );
      for( nlog = project->first_log; nlog; nlog = nlog->next )
      {
         fprintf( fpout, "%s\n", "Log" );
         fwrite_note( nlog, fpout );
      }
      fprintf( fpout, "%s\n", "End" );
   }
   FCLOSE( fpout );
}

NOTE_DATA *read_old_log( FILE * fp )
{
   NOTE_DATA *nlog;
   char *word;
   CREATE( nlog, NOTE_DATA, 1 );

   for( ;; )
   {
      word = fread_word( fp );

      if( !str_cmp( word, "Sender" ) )
         nlog->sender = fread_string( fp );
      else if( !str_cmp( word, "Date" ) )
         fread_to_eol( fp );
      else if( !str_cmp( word, "Subject" ) )
         nlog->subject = fread_string_nohash( fp );
      else if( !str_cmp( word, "Text" ) )
         nlog->text = fread_string_nohash( fp );
      else if( !str_cmp( word, "Endlog" ) )
      {
         fread_to_eol( fp );
         nlog->next = NULL;
         nlog->prev = NULL;
         nlog->date_stamp = current_time;
         return nlog;
      }
      else
      {
         DISPOSE( nlog );
         bug( "read_log: bad key word: %s", word );
         return NULL;
      }
   }
}

PROJECT_DATA *read_project( FILE * fp )
{
   PROJECT_DATA *project;
   NOTE_DATA *nlog, *tlog;
   const char *word;
   char letter;
   bool fMatch;
   int version = 0;

   do
   {
      letter = getc( fp );
      if( feof( fp ) )
      {
         FCLOSE( fp );
         return NULL;
      }
   }
   while( isspace( letter ) );
   ungetc( letter, fp );

   CREATE( project, PROJECT_DATA, 1 );

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
            KEY( "Coder", project->coder, fread_string( fp ) );
            break;

         case 'D':
            /*
             * For passive compatibility with older board files 
             */
            if( !str_cmp( word, "Date" ) )
            {
               fread_to_eol( fp );
               fMatch = TRUE;
               break;
            }
            KEY( "Date_stamp", project->date_stamp, fread_number( fp ) );
            KEY( "Description", project->description, fread_string_nohash( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( project->date_stamp == 0 )
                  project->date_stamp = current_time;
               if( !project->status )
                  project->status = STRALLOC( "No update." );
               if( str_cmp( project->owner, "None" ) )
                  project->taken = TRUE;
               return project;
            }
            break;

         case 'L':
            if( !str_cmp( word, "Log" ) )
            {
               fread_to_eol( fp );

               if( version == 2 )
                  nlog = read_note( fp );
               else
                  nlog = read_old_log( fp );

               if( !nlog )
                  bug( "%s", "read_project: couldn't read log!" );
               else
                  LINK( nlog, project->first_log, project->last_log, next, prev );
               fMatch = TRUE;
               break;
            }
            break;

         case 'N':
            KEY( "Name", project->name, fread_string_nohash( fp ) );
            break;

         case 'O':
            KEY( "Owner", project->owner, fread_string( fp ) );
            break;

         case 'S':
            KEY( "Status", project->status, fread_string_nohash( fp ) );
            break;

         case 'T':
            KEY( "Type", project->type, fread_number( fp ) );
            break;

         case 'V':
            if( !str_cmp( word, "Version" ) )
            {
               version = fread_number( fp );
               fMatch = TRUE;
               break;
            }
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }

   nlog = project->last_log;
   while( nlog )
   {
      UNLINK( nlog, project->first_log, project->last_log, next, prev );
      tlog = nlog->prev;
      free_note( nlog );
      nlog = tlog;
   }
   STRFREE( project->coder );
   DISPOSE( project->description );
   DISPOSE( project->name );
   STRFREE( project->owner );
   DISPOSE( project->status );
   DISPOSE( project );
   return project;
}

void load_projects( void ) /* Copied load_boards structure for simplicity */
{
   FILE *fp;
   PROJECT_DATA *project;

   first_project = NULL;
   last_project = NULL;

   if( !( fp = fopen( PROJECTS_FILE, "r" ) ) )
      return;

   while( ( project = read_project( fp ) ) != NULL )
      LINK( project, first_project, last_project, next, prev );

   return;
}

void delete_project( PROJECT_DATA * project )
{
   NOTE_DATA *nlog, *tlog;

   nlog = project->last_log;
   while( nlog )
   {
      UNLINK( nlog, project->first_log, project->last_log, next, prev );
      tlog = nlog->prev;
      free_note( nlog );
      nlog = tlog;
   }
   STRFREE( project->coder );
   DISPOSE( project->description );
   DISPOSE( project->name );
   STRFREE( project->owner );
   DISPOSE( project->status );
   UNLINK( project, first_project, last_project, next, prev );
   DISPOSE( project );
}

void free_projects( void )
{
   PROJECT_DATA *project, *project_next;

   for( project = first_project; project; project = project_next )
   {
      project_next = project->next;
      delete_project( project );
   }
   return;
}

/* Last thing left to revampitize -- Xorith */
CMDF do_project( CHAR_DATA * ch, char *argument )
{
   char arg[MIL], buf[MSL];
   int pcount, pnum, ptype = 0;
   PROJECT_DATA *pproject;

   if( IS_NPC( ch ) )
      return;

   if( !ch->desc )
   {
      bug( "%s", "do_project: no descriptor" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;
      case SUB_WRITING_NOTE:
         if( !ch->pcdata->pnote )
         {
            bug( "%s", "do_project: log got lost?" );
            send_to_char( "Your log was lost!\n\r", ch );
            stop_editing( ch );
            return;
         }
         if( ch->pcdata->dest_buf != ch->pcdata->pnote )
            bug( "%s", "do_project: sub_writing_note: ch->pcdata->dest_buf != ch->pcdata->pnote" );
         DISPOSE( ch->pcdata->pnote->text );
         ch->pcdata->pnote->text = copy_buffer_nohash( ch );
         stop_editing( ch );
         return;
      case SUB_PROJ_DESC:
         if( !ch->pcdata->dest_buf )
         {
            send_to_char( "Your description was lost!", ch );
            bug( "%s", "do_project: sub_project_desc: NULL ch->pcdata->dest_buf" );
            ch->substate = SUB_NONE;
            return;
         }
         pproject = ( PROJECT_DATA * ) ch->pcdata->dest_buf;
         DISPOSE( pproject->description );
         pproject->description = copy_buffer_nohash( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         write_projects(  );
         return;
      case SUB_EDIT_ABORT:
         send_to_char( "Aborting...\n\r", ch );
         ch->substate = SUB_NONE;
         if( ch->pcdata->dest_buf )
            ch->pcdata->dest_buf = NULL;
         if( ch->pcdata->pnote )
         {
            free_note( ch->pcdata->pnote );
            ch->pcdata->pnote = NULL;
         }
         return;
   }

   set_char_color( AT_NOTE, ch );
   argument = one_argument( argument, arg );
   smash_tilde( argument );

   if( !str_cmp( arg, "save" ) )
   {
      write_projects(  );
      send_to_char( "Projects saved.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "code" ) )
   {
      pcount = 0;
      send_to_pager( " #  | Owner       | Project                    \n\r", ch );
      send_to_pager( "----|-------------|----------------------------\n\r", ch );
      for( pproject = first_project; pproject; pproject = pproject->next )
      {
         pcount++;
         if( ( pproject->status && str_cmp( pproject->status, "approved" ) ) || pproject->coder != NULL )
            continue;
         pager_printf( ch, "%2d%c | %-11s | %-26s\n\r",
                       pcount, pproject->type == 1 ? 'C' : 'B', pproject->owner ? pproject->owner : "(None)",
                       pproject->name ? pproject->name : "(None)" );
      }
      return;
   }
   if( !str_cmp( arg, "more" ) || !str_cmp( arg, "mine" ) )
   {
      NOTE_DATA *nlog;
      bool MINE = FALSE;
      int num_logs = 0;

      pcount = 0;

      if( !str_cmp( arg, "mine" ) )
         MINE = TRUE;

      send_to_pager( "\n\r", ch );
      send_to_pager( " #  | Owner       | Project                    | Coder       | Status     | Logs\n\r", ch );
      send_to_pager( "----|-------------|----------------------------|-------------|------------|-----\n\r", ch );
      for( pproject = first_project; pproject; pproject = pproject->next )
      {
         pcount++;
         if( MINE && ( !pproject->owner || str_cmp( ch->name, pproject->owner ) )
             && ( !pproject->coder || str_cmp( ch->name, pproject->coder ) ) )
            continue;
         else if( !MINE && pproject->status && !str_cmp( "Done", pproject->status ) )
            continue;
         num_logs = 0;
         for( nlog = pproject->first_log; nlog; nlog = nlog->next )
            num_logs++;
         pager_printf( ch, "%2d%c | %-11s | %-26s | %-11s | %-10s | %3d\n\r",
                       pcount, pproject->type == 1 ? 'C' : 'B', pproject->owner ? pproject->owner : "(None)",
                       print_lngstr( pproject->name, 26 ), pproject->coder ? pproject->coder : "(None)",
                       pproject->status ? pproject->status : "(None)", num_logs );
      }
      return;
   }
   if( !arg || arg[0] == '\0' || !str_cmp( arg, "list" ) )
   {
      bool aflag, projects_available;
      aflag = FALSE;
      projects_available = FALSE;
      if( !str_cmp( argument, "available" ) )
         aflag = TRUE;

      send_to_pager( "\n\r", ch );
      if( !aflag )
      {
         send_to_pager( " #  | Owner       | Project                    | Date            | Status       \n\r", ch );
         send_to_pager( "----|-------------|----------------------------|-----------------|--------------\n\r", ch );
      }
      else
      {
         send_to_pager( " #  | Project                        | Date            \n\r", ch );
         send_to_pager( "----|--------------------------------|-----------------\n\r", ch );
      }
      pcount = 0;
      for( pproject = first_project; pproject; pproject = pproject->next )
      {
         pcount++;
         if( pproject->status && !str_cmp( "Done", pproject->status ) )
            continue;
         if( !aflag )
         {
            pager_printf( ch, "%2d%c | %-11s | %-26s | %-15s | %-12s\n\r",
                          pcount, pproject->type == 1 ? 'C' : 'B', pproject->owner ? pproject->owner : "(None)",
                          print_lngstr( pproject->name, 26 ),
                          mini_c_time( pproject->date_stamp, ch->pcdata->timezone ),
                          pproject->status ? pproject->status : "(None)" );
         }
         else if( !pproject->taken )
         {
            if( !projects_available )
               projects_available = TRUE;
            pager_printf( ch, "%2d%c | %-30s | %s\n\r", pcount, pproject->type == 1 ? 'C' : 'B',
                          pproject->name ? pproject->name : "(None)",
                          mini_c_time( pproject->date_stamp, ch->pcdata->timezone ) );
         }
      }
      if( pcount == 0 )
         send_to_pager( "No projects exist.\n\r", ch );
      else if( aflag && !projects_available )
         send_to_pager( "No projects available.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "add" ) )
   {
      PROJECT_DATA *new_project; /* Just to be safe */

      if( get_trust( ch ) < LEVEL_GOD && !( ch->pcdata->realm == REALM_HEAD_CODER
                                            || ch->pcdata->realm == REALM_HEAD_BUILDER || ch->pcdata->realm == REALM_IMP ) )
      {
         send_to_char( "You are not powerful enough to add a new project.\n\r", ch );
         return;
      }

      if( ( ch->pcdata->realm == REALM_IMP ) )
      {
         argument = one_argument( argument, arg );

         if( ( !arg || arg[0] == '\0' ) || ( str_cmp( arg, "code" ) && str_cmp( arg, "build" ) ) )
         {
            send_to_char( "Since you are an Implementor, you must specify either Build or Code for a project type.\n\r",
                          ch );
            send_to_char( "Syntax would be: PROJECT ADD [CODE/BUILD] [NAME]\n\r", ch );
            return;
         }
      }

      switch ( ch->pcdata->realm )
      {
         case REALM_HEAD_CODER:
            ptype = 1;
            break;
         default:
         case REALM_HEAD_BUILDER:
            ptype = 2;
            break;
         case REALM_IMP:
            if( !str_cmp( arg, "code" ) )
               ptype = 1;
            else
               ptype = 2;
      }

      CREATE( new_project, PROJECT_DATA, 1 );
      LINK( new_project, first_project, last_project, next, prev );
      new_project->name = str_dup( argument );
      new_project->coder = NULL;
      new_project->taken = FALSE;
      new_project->description = NULL;
      new_project->type = ptype;
      new_project->date_stamp = current_time;
      write_projects(  );
      ch_printf( ch, "New %s Project '%s' added.\n\r", ( new_project->type == 1 ) ? "Code" : "Build", new_project->name );
      return;
   }

   if( !is_number( arg ) )
   {
      ch_printf( ch, "%s is an invalid project!\n\r", arg );
      return;
   }

   pnum = atoi( arg );
   pproject = get_project_by_number( pnum );
   if( !pproject )
   {
      ch_printf( ch, "Project #%d does not exsist.\n\r", pnum );
      return;
   }

   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "description" ) )
   {
      if( get_trust( ch ) < LEVEL_GOD && !PROJ_ADMIN( pproject, ch ) )
         CHECK_SUBRESTRICTED( ch );
      ch->tempnum = SUB_NONE;
      ch->substate = SUB_PROJ_DESC;
      ch->pcdata->dest_buf = pproject;
      if( !pproject->description )
         pproject->description = str_dup( "" );
      start_editing( ch, pproject->description );
      editor_desc_printf( ch, "Project description for project '%s'.", pproject->name ? pproject->name : "(No name)" );
      return;
   }
   if( !str_cmp( arg, "delete" ) )
   {
      if( !PROJ_ADMIN( pproject, ch ) && get_trust( ch ) < LEVEL_ASCENDANT )
      {
         send_to_char( "You are not high enough level to delete a project.\n\r", ch );
         return;
      }
      snprintf( buf, MSL, "%s", pproject->name );
      delete_project( pproject );
      write_projects(  );
      ch_printf( ch, "Project '%s' has been deleted.\n\r", buf );
      return;
   }

   if( !str_cmp( arg, "take" ) )
   {
      if( pproject->taken && pproject->owner && !str_cmp( pproject->owner, ch->name ) )
      {
         pproject->taken = FALSE;
         STRFREE( pproject->owner );
         send_to_char( "You removed yourself as the owner.\n\r", ch );
         write_projects(  );
         return;
      }
      else if( pproject->taken && !PROJ_ADMIN( pproject, ch ) )
      {
         send_to_char( "This project is already taken.\n\r", ch );
         return;
      }
      else if( pproject->taken && PROJ_ADMIN( pproject, ch ) )
         ch_printf( ch, "Taking Project: '%s' from Owner: '%s'!\n\r", pproject->name,
                    pproject->owner ? pproject->owner : "NULL" );

      STRFREE( pproject->owner );
      pproject->owner = QUICKLINK( ch->name );
      pproject->taken = TRUE;
      write_projects(  );
      ch_printf( ch, "You're now the owner of Project '%s'.\n\r", pproject->name );
      return;
   }

   if( ( !str_cmp( arg, "coder" ) ) || ( !str_cmp( arg, "builder" ) ) )
   {
      if( pproject->coder && !str_cmp( ch->name, pproject->coder ) )
      {
         STRFREE( pproject->coder );
         ch_printf( ch, "You removed yourself as the %s.\n\r", pproject->type == 1 ? "coder" : "builder" );
         write_projects(  );
         return;
      }
      else if( pproject->coder && !PROJ_ADMIN( pproject, ch ) )
      {
         ch_printf( ch, "This project already has a %s.\n\r", pproject->type == 1 ? "coder" : "builder" );
         return;
      }
      else if( pproject->coder && PROJ_ADMIN( pproject, ch ) )
      {
         ch_printf( ch, "Removing %s as %s of this project!\n\r", pproject->coder ? pproject->coder : "NULL",
                    pproject->type == 1 ? "coder" : "builder" );
         STRFREE( pproject->coder );
         send_to_char( "Coder removed.\n\r", ch );
         write_projects(  );
         return;
      }

      pproject->coder = QUICKLINK( ch->name );
      write_projects(  );
      ch_printf( ch, "You are now the %s of %s.\n\r", pproject->type == 1 ? "coder" : "builder", pproject->name );
      return;
   }

   if( !str_cmp( arg, "status" ) )
   {
      if( pproject->owner && str_cmp( pproject->owner, ch->name )
          && get_trust( ch ) < LEVEL_GREATER && pproject->coder
          && str_cmp( pproject->coder, ch->name ) && !PROJ_ADMIN( pproject, ch ) )
      {
         send_to_char( "This is not your project!\n\r", ch );
         return;
      }
      DISPOSE( pproject->status );
      pproject->status = str_dup( argument );
      write_projects(  );
      ch_printf( ch, "Project Status set to: %s\n\r", pproject->status );
      return;
   }

   if( !str_cmp( arg, "show" ) )
   {
      if( pproject->description )
         send_to_char( pproject->description, ch );
      else
         send_to_char( "That project does not have a description.\n\r", ch );
      return;
   }
   if( !str_cmp( arg, "log" ) )
   {
      NOTE_DATA *plog;
      if( !str_cmp( argument, "write" ) )
      {
         if( !ch->pcdata->pnote )
            note_attach( ch );
         ch->substate = SUB_WRITING_NOTE;
         ch->pcdata->dest_buf = ch->pcdata->pnote;
         if( !ch->pcdata->pnote->text )
            ch->pcdata->pnote->text = str_dup( "" );
         start_editing( ch, ch->pcdata->pnote->text );
         editor_desc_printf( ch, "A log note in project '%s', entitled '%s'.",
                             pproject->name ? pproject->name : "(No name)",
                             ch->pcdata->pnote->subject ? ch->pcdata->pnote->subject : "(No subject)" );
         return;
      }

      argument = one_argument( argument, arg );

      if( !str_cmp( arg, "subject" ) )
      {
         if( !ch->pcdata->pnote )
            note_attach( ch );
         DISPOSE( ch->pcdata->pnote->subject );
         ch->pcdata->pnote->subject = str_dup( argument );
         ch_printf( ch, "Log Subject set to: %s\n\r", ch->pcdata->pnote->subject );
         return;
      }

      if( !str_cmp( arg, "post" ) )
      {
         if( ( pproject->owner && str_cmp( ch->name, pproject->owner ) )
             || ( pproject->coder && str_cmp( ch->name, pproject->coder ) )
             || ( ( get_trust( ch ) < LEVEL_GREATER ) && !PROJ_ADMIN( pproject, ch ) ) )
         {
            send_to_char( "This is not your project!\n\r", ch );
            return;
         }

         if( !ch->pcdata->pnote )
         {
            send_to_char( "You have no log in progress.\n\r", ch );
            return;
         }

         if( !ch->pcdata->pnote->subject )
         {
            send_to_char( "Your log has no subject.\n\r", ch );
            return;
         }

         if( !ch->pcdata->pnote->text || ch->pcdata->pnote->text[0] == '\0' )
         {
            send_to_char( "Your log has no text!\n\r", ch );
            return;
         }

         ch->pcdata->pnote->date_stamp = current_time;
         plog = ch->pcdata->pnote;
         ch->pcdata->pnote = NULL;

         if( argument && argument[0] != '\0' )
         {
            int plog_num;
            NOTE_DATA *tplog;
            plog_num = atoi( argument );

            tplog = get_log_by_number( pproject, plog_num );

            if( !tplog )
            {
               ch_printf( ch, "Log #%d can not be found.\n\r", plog_num );
               return;
            }

            plog->parent = tplog;
            tplog->reply_count++;
            LINK( plog, tplog->first_reply, tplog->last_reply, next, prev );
            send_to_char( "Your reply was posted successfully.", ch );
            write_projects(  );
            return;
         }
         LINK( plog, pproject->first_log, pproject->last_log, next, prev );
         write_projects(  );
         send_to_char( "Log Posted!\n\r", ch );
         return;
      }

      if( !str_cmp( arg, "list" ) )
      {
         if( ( pproject->owner && str_cmp( ch->name, pproject->owner ) )
             || ( pproject->coder && str_cmp( ch->name, pproject->coder ) )
             || ( ( get_trust( ch ) < LEVEL_SAVIOR ) && !PROJ_ADMIN( pproject, ch ) ) )
         {
            send_to_char( "This is not your project!\n\r", ch );
            return;
         }

         pcount = 0;
         pager_printf( ch, "Project: %-12s: %s\n\r", pproject->owner ? pproject->owner : "(None)", pproject->name );

         for( plog = pproject->first_log; plog; plog = plog->next )
         {
            pcount++;
            pager_printf( ch, "%2d) %-12s: %s\n\r", pcount, plog->sender ? plog->sender : "--Error--",
                          plog->subject ? plog->subject : "" );
         }
         if( pcount == 0 )
            send_to_char( "No logs available.\n\r", ch );
         else
            ch_printf( ch, "%d log%s found.\n\r", pcount, pcount == 1 ? "" : "s" );
         return;
      }

      if( !is_number( arg ) )
      {
         send_to_char( "Invalid log.\n\r", ch );
         return;
      }

      pnum = atoi( arg );

      plog = get_log_by_number( pproject, pnum );
      if( !plog )
      {
         send_to_char( "Invalid log.\n\r", ch );
         return;
      }

      if( !str_cmp( argument, "delete" ) )
      {
         if( ( pproject->owner && str_cmp( ch->name, pproject->owner ) )
             || ( pproject->coder && str_cmp( ch->name, pproject->coder ) )
             || ( ( get_trust( ch ) < LEVEL_ASCENDANT ) && !PROJ_ADMIN( pproject, ch ) ) )
         {
            send_to_char( "This is not your project!\n\r", ch );
            return;
         }

         UNLINK( plog, pproject->first_log, pproject->last_log, next, prev );
         free_note( plog );
         write_projects(  );
         ch_printf( ch, "Log #%d has been deleted.\n\r", pnum );
         return;
      }

      if( !str_cmp( argument, "read" ) )
      {
         if( ( pproject->owner && str_cmp( ch->name, pproject->owner ) )
             || ( pproject->coder && str_cmp( ch->name, pproject->coder ) )
             || ( ( get_trust( ch ) < LEVEL_SAVIOR ) && !PROJ_ADMIN( pproject, ch ) ) )
         {
            send_to_char( "This is not your project!\n\r", ch );
            return;
         }
         note_to_char( ch, plog, NULL, -1 );
         return;
      }
   }
   interpret( ch, "help project" );
   return;
}
