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
 *                       Xorith's Connection History                        *
 ****************************************************************************/

/* ConnHistory Feature
 *
 * Based loosely on Samson's Channel History functions. (basic idea)
 * Written by: Xorith on 5/7/03, last updated: 9/20/03
 *
 * Stores connection data in an array so that it can be reviewed later.
 *
 */

#include <sys/time.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "mud.h"
#include "connhist.h"

/* Globals */
CONN_DATA *first_conn;
CONN_DATA *last_conn;

/* Removes a single conn entry */
void remove_conn( CONN_DATA * conn )
{
   if( !conn )
      return;

   UNLINK( conn, first_conn, last_conn, next, prev );
   STRFREE( conn->user );
   DISPOSE( conn->when );
   STRFREE( conn->host );
   DISPOSE( conn );
   return;
}

/* Checks an entry for validity. Removes an invalid entry. */
/* This could possibly be useless, as the code that handles updating the file
 * already does most of this. Why not be extra-safe though? */
int check_conn_entry( CONN_DATA * conn )
{
   if( !conn )
      return CHK_CONN_REMOVED;

   if( !conn->user || !conn->host || !conn->when )
   {
      remove_conn( conn );
      bug( "%s: Removed bugged conn entry!", __FUNCTION__ );
      return CHK_CONN_REMOVED;
   }
   return CHK_CONN_OK;
}

void fread_connhist( CONN_DATA * conn, FILE * fp )
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

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'H':
            KEY( "Host", conn->host, fread_string( fp ) );
            break;

         case 'I':
            KEY( "Invis", conn->invis_lvl, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Level", conn->level, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Type", conn->type, fread_number( fp ) );
            break;

         case 'U':
            KEY( "User", conn->user, fread_string( fp ) );
            break;

         case 'W':
            KEY( "When", conn->when, fread_string_nohash( fp ) );
            break;
      }
      if( !fMatch )
         bug( "Fread_connhist: no match: %s", word );
   }
}

/* Loads the conn.hist file into memory */
void load_connhistory( void )
{
   CONN_DATA *conn;
   FILE *fp;
   int conncount;

   first_conn = NULL;
   last_conn = NULL;

   if( ( fp = fopen( CH_FILE, "r" ) ) != NULL )
   {
      conncount = 0;
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
            bug( "%s", "Load_connhistory: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "CONN" ) )
         {
            if( conncount >= MAX_CONNHISTORY )
            {
               bug( "load_connhistory: more connection histories than MAX_CONNHISTORY %d", MAX_CONNHISTORY );
               FCLOSE( fp );
               return;
            }
            CREATE( conn, CONN_DATA, 1 );
            fread_connhist( conn, fp );
            conncount++;
            LINK( conn, first_conn, last_conn, next, prev );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "Load_connhistory: bad section: %s", word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   return;
}

/* Saves the conn.hist file */
void save_connhistory( void )
{
   FILE *tempfile;
   CONN_DATA *conn;

   if( !first_conn )
      return;

   if( !( tempfile = fopen( CH_FILE, "w" ) ) )
   {
      bug( "%s: Error opening '%s'", __FUNCTION__, CH_FILE );
      return;
   }

   for( conn = first_conn; conn; conn = conn->next )
   {
      /*
       * Only save OK conn entries 
       */
      if( ( check_conn_entry( conn ) ) == CHK_CONN_OK )
      {
         fprintf( tempfile, "%s", "#CONN\n" );
         fprintf( tempfile, "User   %s~\n", conn->user );
         fprintf( tempfile, "When   %s~\n", conn->when );
         fprintf( tempfile, "Host   %s~\n", conn->host );
         fprintf( tempfile, "Level  %d\n", conn->level );
         fprintf( tempfile, "Type   %d\n", conn->type );
         fprintf( tempfile, "Invis  %d\n", conn->invis_lvl );
         fprintf( tempfile, "%s", "End\n\n" );
      }
   }
   fprintf( tempfile, "%s", "#END\n" );
   FCLOSE( tempfile );
   return;
}

/* Frees all the histories from memory.
 * If Arg == 1, then it also deletes the conn.hist file
 */
void free_connhistory( int arg )
{
   CONN_DATA *conn, *conn_next;

   for( conn = first_conn; conn; conn = conn_next )
   {
      conn_next = conn->next;
      remove_conn( conn );
   }

   if( arg == 1 )
      unlink( CH_FILE );

   return;
}

/* NeoCode 0.08 Revamp of this! I now base it off of descriptor data.
 * It will pull needed data from the descriptor. If there's no character
 * than it will use default information. -- X
 */
void update_connhistory( DESCRIPTOR_DATA * d, int type )
{
   CONN_DATA *conn;
   CHAR_DATA *vch;
   struct tm *local;
   time_t t;
   int conn_count = 0;

   if( !d )
   {
      bug( "%s: NULL descriptor!", __FUNCTION__ );
      return;
   }

   vch = d->original ? d->original : d->character;
   if( !vch )
      return;

   if( IS_NPC( vch ) )
      return;

   /*
    * Count current histories, if more than the defined MAX, then remove the first one. -- X 
    */
   conn = first_conn;
   for( conn = first_conn; conn; conn = conn->next )
      conn_count++;

   if( conn_count >= MAX_CONNHISTORY )
   {
      conn = first_conn;
      remove_conn( conn );
   }

   /*
    * Build our time string... 
    */
   t = time( NULL );
   local = localtime( &t );

   /*
    * Create our entry and fill the fields! 
    */
   CREATE( conn, CONN_DATA, 1 );
   conn->user = vch->name ? STRALLOC( vch->name ) : STRALLOC( "NoName" );
   strdup_printf( &conn->when,
                  "%-2.2d/%-2.2d %-2.2d:%-2.2d", local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min );
   conn->host = d->host ? STRALLOC( d->host ) : STRALLOC( "unknown" );
   conn->type = type;
   conn->level = vch->level;
   conn->invis_lvl = IS_PLR_FLAG( vch, PLR_WIZINVIS ) ? vch->pcdata->wizinvis : 0;
   LINK( conn, first_conn, last_conn, next, prev );
   save_connhistory(  );
   return;
}

/* The logins command */
/* Those who are equal or greather than the CH_LVL_ADMIN defined in connhist.h
 * can also prompt a complete purging of the histories and the conn.hist file. */
CMDF do_logins( CHAR_DATA * ch, char *argument )
{
   int conn_count = 0;
   CONN_DATA *conn;
   char user[MSL], typebuf[MSL];

   if( ( argument ) && ( !str_cmp( argument, "clear" ) && ch->level >= CH_LVL_ADMIN ) )
   {
      send_to_char( "Clearing Connection history...\n\r", ch );
      free_connhistory( 1 );  /* Remember - Arg must = 1 to wipe the file on disk! -- X */
      return;
   }

   /*
    * Modify this line to fit your tastes 
    */
   ch_printf( ch, "&c----[&WConnection History for %s&c]----&w\n\r", sysdata.mud_name );

   for( conn = first_conn; conn; conn = conn->next )
   {
      if( ( check_conn_entry( conn ) ) != CHK_CONN_OK )
         continue;

      if( conn->invis_lvl <= ch->level )
      {
         conn_count++;
         switch ( conn->type )
         {
            case CONNTYPE_LOGIN:
               mudstrlcpy( typebuf, " has logged in.", MSL );
               break;
            case CONNTYPE_QUIT:
               mudstrlcpy( typebuf, " has logged out.", MSL );
               break;
            case CONNTYPE_IDLE:
               mudstrlcpy( typebuf, " has been logged out due to inactivity.", MSL );
               break;
            case CONNTYPE_LINKDEAD:
               mudstrlcpy( typebuf, " has lost their link.", MSL );
               break;
            case CONNTYPE_NEWPLYR:
               mudstrlcpy( typebuf, " has logged in for the first time!", MSL );
               break;
            case CONNTYPE_RECONN:
               mudstrlcpy( typebuf, " has reconnected.", MSL );
               break;
            default:
               mudstrlcpy( typebuf, ".", MSL );
               break;
         }

         /*
          * If a player is wizinvis, and an immortal can see them, then tell that immortal
          * * what invis level they were. Note: change color for the Invis tag here.
          */
         if( conn->invis_lvl > 0 && IS_IMMORTAL( ch ) )
            snprintf( user, MSL, "(&cInvis &p%d&w) %s", conn->invis_lvl, conn->user );
         else
            mudstrlcpy( user, conn->user, MSL );

         /*
          * The format for the history are these two lines below. First is for Immortals, second for players. 
          */
         /*
          * If you know what you're doing, than you can modify the output here 
          */
         if( IS_IMMORTAL( ch ) )
            ch_printf( ch, "&c[&O%s&c] &w%s&g@&w%s&c%s&w\n\r", conn->when, user, conn->host, typebuf );
         else
            ch_printf( ch, "&c[&O%s&c] &w%s&c%s&w\n\r", conn->when, user, typebuf );

      }
   }

   if( !conn_count )
      send_to_char( "&WNo Data.&w\n\r", ch );

   if( ch->level >= CH_LVL_ADMIN )
      send_to_char( "\n\rAdmin: You may type LOGIN CLEAR to wipe the current history.\n\r", ch );
   return;
}
