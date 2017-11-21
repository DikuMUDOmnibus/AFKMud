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
 *                            Ban module by Shaddai                         *
 ****************************************************************************/

#include <string.h>
#include <ctype.h>
#include "mud.h"
#include "ban.h"

/* Global Variables */

BAN_DATA *first_ban;
BAN_DATA *last_ban;
WATCH_DATA *first_watch;
WATCH_DATA *last_watch;

void free_watchlist( void )
{
   WATCH_DATA *pw, *pw_next;

   for( pw = first_watch; pw; pw = pw_next )
   {
      pw_next = pw->next;
      UNLINK( pw, first_watch, last_watch, next, prev );
      DISPOSE( pw->imm_name );
      DISPOSE( pw->player_site );
      DISPOSE( pw->target_name );
      DISPOSE( pw );
   }
   return;
}

void save_watchlist( void )
{
   WATCH_DATA *pwatch;
   FILE *fp;

   if( !( fp = fopen( SYSTEM_DIR WATCH_LIST, "w" ) ) )
   {
      bug( "Save_watchlist: Cannot open %s", WATCH_LIST );
      perror( WATCH_LIST );
      return;
   }

   for( pwatch = first_watch; pwatch; pwatch = pwatch->next )
      fprintf( fp, "%d %s~%s~%s~\n", pwatch->imm_level, pwatch->imm_name,
               pwatch->target_name ? pwatch->target_name : " ", pwatch->player_site ? pwatch->player_site : " " );
   fprintf( fp, "%s", "-1\n" );
   FCLOSE( fp );
   return;
}

void load_watchlist( void )
{
   WATCH_DATA *pwatch;
   FILE *fp;
   int number;
   CMDTYPE *cmd;

   /*
    * Bug fix - Samson 8-22-99 
    */
   first_watch = NULL;
   last_watch = NULL;

   if( !( fp = fopen( SYSTEM_DIR WATCH_LIST, "r" ) ) )
      return;

   for( ;; )
   {
      if( feof( fp ) )
      {
         bug( "%s", "Load_watchlist: no -1 found." );
         FCLOSE( fp );
         return;
      }
      number = fread_number( fp );
      if( number == -1 )
      {
         FCLOSE( fp );
         return;
      }

      CREATE( pwatch, WATCH_DATA, 1 );
      pwatch->imm_level = number;
      pwatch->imm_name = fread_string_nohash( fp );
      pwatch->target_name = fread_string_nohash( fp );
      if( strlen( pwatch->target_name ) < 2 )
         DISPOSE( pwatch->target_name );
      pwatch->player_site = fread_string_nohash( fp );
      if( strlen( pwatch->player_site ) < 2 )
         DISPOSE( pwatch->player_site );

      /*
       * Check for command watches 
       */
      if( pwatch->target_name )
         for( cmd = command_hash[( int )pwatch->target_name[0]]; cmd; cmd = cmd->next )
         {
            if( !str_cmp( pwatch->target_name, cmd->name ) )
            {
               SET_BIT( cmd->flags, CMD_WATCH );
               break;
            }
         }
      LINK( pwatch, first_watch, last_watch, next, prev );
   }
}

/*
 * Determine if this input line is eligible for writing to a watch file.
 * We don't want to write movement commands like (n, s, e, w, etc.)
 */
bool valid_watch( char *logline )
{
   int len = strlen( logline );
   char c = logline[0];

   if( len == 1 && ( c == 'n' || c == 's' || c == 'e' || c == 'w' || c == 'u' || c == 'd' ) )
      return FALSE;
   if( len == 2 && c == 'n' && ( logline[1] == 'e' || logline[1] == 'w' ) )
      return FALSE;
   if( len == 2 && c == 's' && ( logline[1] == 'e' || logline[1] == 'w' ) )
      return FALSE;

   return TRUE;
}

/*
 * Write input line to watch files if applicable
 */
void write_watch_files( CHAR_DATA * ch, CMDTYPE * cmd, char *logline )
{
   WATCH_DATA *pw;
   FILE *fp;
   char fname[256];
   struct tm *t = localtime( &current_time );

   if( !first_watch )   /* no active watches */
      return;

   /*
    * if we're watching a command we need to do some special stuff 
    */
   /*
    * to avoid duplicating log lines - relies upon watch list being 
    */
   /*
    * sorted by imm name 
    */
   if( cmd )
   {
      char *cur_imm;
      bool found;

      pw = first_watch;
      while( pw )
      {
         found = FALSE;

         for( cur_imm = pw->imm_name; pw && !str_cmp( pw->imm_name, cur_imm ); pw = pw->next )
         {
            if( !found && ch->desc && get_trust( ch ) < pw->imm_level
                && ( ( pw->target_name && !str_cmp( cmd->name, pw->target_name ) )
                     || ( pw->player_site && !str_prefix( pw->player_site, ch->desc->host ) ) ) )
            {
               snprintf( fname, 256, "%s%s", WATCH_DIR, strlower( pw->imm_name ) );
               if( !( fp = fopen( fname, "a+" ) ) )
               {
                  bug( "%s%s", "Write_watch_files: Cannot open ", fname );
                  perror( fname );
                  return;
               }
               fprintf( fp, "%.2d/%.2d %.2d:%.2d %s: %s\n",
                        t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, ch->name, logline );
               FCLOSE( fp );
               found = TRUE;
            }
         }
      }
   }
   else
   {
      for( pw = first_watch; pw; pw = pw->next )
         if( ( ( pw->target_name && !str_cmp( pw->target_name, ch->name ) )
               || ( pw->player_site && !str_prefix( pw->player_site, ch->desc->host ) ) )
             && get_trust( ch ) < pw->imm_level && ch->desc )
         {
            snprintf( fname, 256, "%s%s", WATCH_DIR, strlower( pw->imm_name ) );
            if( !( fp = fopen( fname, "a+" ) ) )
            {
               bug( "%s%s", "Write_watch_files: Cannot open ", fname );
               perror( fname );
               return;
            }
            fprintf( fp, "%.2d/%.2d %.2d:%.2d %s: %s\n",
                     t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, ch->name, logline );
            FCLOSE( fp );
         }
   }
   return;
}

void check_watch_cmd( CHAR_DATA * ch, CMDTYPE * cmd, char *logline )
{
   if( cmd && IS_SET( cmd->flags, CMD_WATCH ) )
   {
      char file_buf[256];

      snprintf( file_buf, 256, "%s%s", WATCH_DIR, cmd->name );
      append_file( ch, file_buf, "Used command: %s", logline );
   }
   return;
}

/*
 * The "watch" facility allows imms to specify the name of a player or
 * the name of a site to be watched. It is like "logging" a player except
 * the results are written to a file in the "watch" directory named with
 * the same name as the imm. The idea is to allow lower level imms to 
 * watch players or sites without having to have access to the log files.
 */
CMDF do_watch( CHAR_DATA * ch, char *argument )
{
   char arg[MIL], arg2[MIL], arg3[MIL];
   WATCH_DATA *pw;

   if( IS_NPC( ch ) )
      return;

   argument = one_argument( argument, arg );
   set_pager_color( AT_IMMORT, ch );

   if( !arg || arg[0] == '\0' || !str_cmp( arg, "help" ) )
   {
      send_to_pager( "Syntax Examples:\n\r", ch );
      /*
       * Only IMP+ can see all the watches. The rest can just see their own.
       */
      if( IS_IMP( ch ) )
         send_to_pager( "   watch show all          show all watches\n\r", ch );
      send_to_pager( "   watch show              show all my watches\n\r", ch );
      send_to_pager( "   watch size              show the size of my watch file\n\r", ch );
      send_to_pager( "   watch player joe        add a new player watch\n\r", ch );
      send_to_pager( "   watch site 2.3.123      add a new site watch\n\r", ch );
      send_to_pager( "   watch command make      add a new command watch\n\r", ch );
      send_to_pager( "   watch site 2.3.12       matches 2.3.12x\n\r", ch );
      send_to_pager( "   watch site 2.3.12.      matches 2.3.12.x\n\r", ch );
      send_to_pager( "   watch delete n          delete my nth watch\n\r", ch );
      send_to_pager( "   watch print 500         print watch file starting at line 500\n\r", ch );
      send_to_pager( "   watch print 500 1000    print 1000 lines starting at line 500\n\r", ch );
      send_to_pager( "   watch clear             clear my watch file\n\r", ch );
      return;
   }

   set_pager_color( AT_PLAIN, ch );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   /*
    * Clear watch file
    */
   if( !str_cmp( arg, "clear" ) )
   {
      char fname[256];

      snprintf( fname, 256, "%s%s", WATCH_DIR, strlower( ch->name ) );
      if( 0 == remove( fname ) )
      {
         send_to_pager( "Ok. Your watch file has been cleared.\n\r", ch );
         return;
      }
      send_to_pager( "You have no valid watch file to clear.\n\r", ch );
      return;
   }

   /*
    * Display size of watch file
    */
   if( !str_cmp( arg, "size" ) )
   {
      FILE *fp;
      char fname[256], s[MSL];
      int rec_count = 0;

      snprintf( fname, 256, "%s%s", WATCH_DIR, strlower( ch->name ) );

      if( !( fp = fopen( fname, "r" ) ) )
      {
         send_to_pager( "You have no watch file. Perhaps you cleared it?\n\r", ch );
         return;
      }

      fgets( s, MSL, fp );
      while( !feof( fp ) )
      {
         rec_count++;
         fgets( s, MSL, fp );
      }
      pager_printf( ch, "You have %d lines in your watch file.\n\r", rec_count );
      FCLOSE( fp );
      return;
   }

   /*
    * Print watch file
    */
   if( !str_cmp( arg, "print" ) )
   {
      FILE *fp;
      char fname[256], s[MSL];
      const int MAX_DISPLAY_LINES = 1000;
      int start, limit, disp_count = 0, rec_count = 0;

      if( !arg2 || arg2[0] == '\0' )
      {
         send_to_pager( "Sorry. You must specify a starting line number.\n\r", ch );
         return;
      }

      start = atoi( arg2 );
      limit = ( arg3[0] == '\0' ) ? MAX_DISPLAY_LINES : atoi( arg3 );
      limit = UMIN( limit, MAX_DISPLAY_LINES );

      snprintf( fname, 256, "%s%s", WATCH_DIR, strlower( ch->name ) );
      if( !( fp = fopen( fname, "r" ) ) )
         return;
      fgets( s, MSL, fp );

      while( ( disp_count < limit ) && ( !feof( fp ) ) )
      {
         if( ++rec_count >= start )
         {
            send_to_pager( s, ch );
            disp_count++;
         }
         fgets( s, MSL, fp );
      }
      send_to_pager( "\n\r", ch );
      if( disp_count >= MAX_DISPLAY_LINES )
         send_to_pager( "Maximum display lines exceeded. List is terminated.\n\r"
                        "Type 'help watch' to see how to print the rest of the list.\n\r\n\r"
                        "Your watch file is large. Perhaps you should clear it?\n\r", ch );

      FCLOSE( fp );
      return;
   }

   /*
    * Display all watches
    * Only IMP+ can see all the watches. The rest can just see their own.
    */
   if( IS_IMP( ch ) && !str_cmp( arg, "show" ) && !str_cmp( arg2, "all" ) )
   {
      pager_printf( ch, "%-12s %-14s %-15s\n\r", "Imm Name", "Player/Command", "Player Site" );
      if( first_watch )
         for( pw = first_watch; pw; pw = pw->next )
            if( get_trust( ch ) >= pw->imm_level )
               pager_printf( ch, "%-14s %-12s %-15s\n\r", pw->imm_name, pw->target_name ? pw->target_name : " ",
                             pw->player_site ? pw->player_site : " " );
      return;
   }

   /*
    * Display only those watches belonging to the requesting imm 
    */
   if( !str_cmp( arg, "show" ) && arg2[0] == '\0' )
   {
      int cou = 0;
      pager_printf( ch, "%-3s %-12s %-14s %-15s\n\r", " ", "Imm Name", "Player/Command", "Player Site" );
      if( first_watch )
         for( pw = first_watch; pw; pw = pw->next )
            if( !str_cmp( ch->name, pw->imm_name ) )
               pager_printf( ch, "%3d %-12s %-14s %-15s\n\r", ++cou, pw->imm_name, pw->target_name ? pw->target_name : " ",
                             pw->player_site ? pw->player_site : " " );
      return;
   }

   /*
    * Delete a watch belonging to the requesting imm
    */
   if( !str_cmp( arg, "delete" ) && isdigit( *arg2 ) )
   {
      int cou = 0;
      int num;

      num = atoi( arg2 );
      if( first_watch )
      {
         for( pw = first_watch; pw; pw = pw->next )
            if( !str_cmp( ch->name, pw->imm_name ) )
               if( num == ++cou )
               {
                  /*
                   * Oops someone forgot to clear up the memory --Shaddai 
                   */
                  DISPOSE( pw->imm_name );
                  DISPOSE( pw->player_site );
                  DISPOSE( pw->target_name );
                  /*
                   * Now we can unlink and then clear up that final
                   * * pointer -- Shaddai 
                   */
                  UNLINK( pw, first_watch, last_watch, next, prev );
                  DISPOSE( pw );
                  save_watchlist(  );
                  send_to_pager( "Deleted.\n\r", ch );
                  return;
               }
      }
      send_to_pager( "Sorry. I found nothing to delete.\n\r", ch );
      return;
   }

   /*
    * Watch a specific player
    */
   if( !str_cmp( arg, "player" ) && *arg2 )
   {
      WATCH_DATA *pinsert;
      CHAR_DATA *vic;
      char buf[MIL];

      if( first_watch ) /* check for dups */
      {
         for( pw = first_watch; pw; pw = pw->next )
            if( !str_cmp( ch->name, pw->imm_name ) && pw->target_name && !str_cmp( arg2, pw->target_name ) )
            {
               send_to_pager( "You are already watching that player.\n\r", ch );
               return;
            }
      }

      CREATE( pinsert, WATCH_DATA, 1 );   /* create new watch */
      pinsert->imm_level = get_trust( ch );
      pinsert->imm_name = str_dup( strlower( ch->name ) );
      pinsert->target_name = str_dup( strlower( arg2 ) );
      pinsert->player_site = NULL;

      /*
       * stupid get_char_world returns ptr to "samantha" when given "sam" 
       */
      /*
       * so I do a str_cmp to make sure it finds the right player --Gorog 
       */

      snprintf( buf, MIL, "0.%s", arg2 );
      if( ( vic = get_char_world( ch, buf ) ) ) /* if vic is in game now */
         if( ( !IS_NPC( vic ) ) && !str_cmp( arg2, vic->name ) )
            SET_PCFLAG( vic, PCFLAG_WATCH );

      if( first_watch ) /* ins new watch if app */
      {
         for( pw = first_watch; pw; pw = pw->next )
            if( str_cmp( pinsert->imm_name, pw->imm_name ) )
            {
               INSERT( pinsert, pw, first_watch, next, prev );
               save_watchlist(  );
               send_to_pager( "Ok. That player will be watched.\n\r", ch );
               return;
            }
      }

      LINK( pinsert, first_watch, last_watch, next, prev ); /* link new watch */
      save_watchlist(  );
      send_to_pager( "Ok. That player will be watched.\n\r", ch );
      return;
   }

   /*
    * Watch a specific site
    */
   if( !str_cmp( arg, "site" ) && *arg2 )
   {
      WATCH_DATA *pinsert;
      CHAR_DATA *vic;

      if( first_watch ) /* check for dups */
      {
         for( pw = first_watch; pw; pw = pw->next )
            if( !str_cmp( ch->name, pw->imm_name ) && pw->player_site && !str_cmp( arg2, pw->player_site ) )
            {
               send_to_pager( "You are already watching that site.\n\r", ch );
               return;
            }
      }
      CREATE( pinsert, WATCH_DATA, 1 );   /* create new watch */
      pinsert->imm_level = get_trust( ch );
      pinsert->imm_name = str_dup( strlower( ch->name ) );
      pinsert->player_site = str_dup( strlower( arg2 ) );
      pinsert->target_name = NULL;

      for( vic = first_char; vic; vic = vic->next )
         if( !IS_NPC( vic ) && vic->desc && *pinsert->player_site && !str_prefix( pinsert->player_site, vic->desc->host )
             && get_trust( vic ) < pinsert->imm_level )
            SET_PCFLAG( vic, PCFLAG_WATCH );

      if( first_watch ) /* ins new watch if app */
      {
         for( pw = first_watch; pw; pw = pw->next )
            if( str_cmp( pinsert->imm_name, pw->imm_name ) )
            {
               INSERT( pinsert, pw, first_watch, next, prev );
               save_watchlist(  );
               send_to_pager( "Ok. That site will be watched.\n\r", ch );
               return;
            }
      }
      LINK( pinsert, first_watch, last_watch, next, prev );
      save_watchlist(  );
      send_to_pager( "Ok. That site will be watched.\n\r", ch );
      return;
   }

   /*
    * Watch a specific command - FB
    */
   if( !str_cmp( arg, "command" ) && *arg2 )
   {
      WATCH_DATA *pinsert;
      CMDTYPE *cmd;
      bool found = FALSE;

      for( pw = first_watch; pw; pw = pw->next )
      {
         if( !str_cmp( ch->name, pw->imm_name ) && pw->target_name && !str_cmp( arg2, pw->target_name ) )
         {
            send_to_pager( "You are already watching that command.\n\r", ch );
            return;
         }
      }

      for( cmd = command_hash[LOWER( arg2[0] ) % 126]; cmd; cmd = cmd->next )
      {
         if( !str_cmp( arg2, cmd->name ) )
         {
            found = TRUE;
            break;
         }
      }

      if( !found )
      {
         send_to_pager( "No such command exists.\n\r", ch );
         return;
      }
      else
         SET_BIT( cmd->flags, CMD_WATCH );

      CREATE( pinsert, WATCH_DATA, 1 );
      pinsert->imm_level = get_trust( ch );
      pinsert->imm_name = str_dup( strlower( ch->name ) );
      pinsert->player_site = NULL;
      pinsert->target_name = str_dup( arg2 );

      for( pw = first_watch; pw; pw = pw->next )
      {
         if( !str_cmp( pinsert->imm_name, pw->imm_name ) )
         {
            INSERT( pinsert, pw, first_watch, next, prev );
            save_watchlist(  );
            send_to_pager( "Ok, That command will be watched.\n\r", ch );
            return;
         }
      }

      LINK( pinsert, first_watch, last_watch, next, prev );
      save_watchlist(  );
      send_to_pager( "Ok. That site will be watched.\n\r", ch );
      return;
   }

   send_to_pager( "Sorry. I can't do anything with that. Please read the help file.\n\r", ch );
   return;
}

bool check_expire( BAN_DATA * pban )
{
   if( pban->unban_date < 0 )
      return FALSE;

   if( pban->unban_date <= current_time )
   {
      log_printf( "%s ban has expired.", pban->name );
      return TRUE;
   }

   return FALSE;
}

void free_ban( BAN_DATA * pban )
{
   DISPOSE( pban->name );
   DISPOSE( pban->ban_time );
   DISPOSE( pban->note );
   DISPOSE( pban->user );
   DISPOSE( pban->ban_by );
   DISPOSE( pban );
}

void dispose_ban( BAN_DATA * pban )
{
   if( !pban )
      return;

   UNLINK( pban, first_ban, last_ban, next, prev );
   free_ban( pban );
   return;
}

void free_bans( void )
{
   BAN_DATA *ban, *ban_next;

   for( ban = first_ban; ban; ban = ban_next )
   {
      ban_next = ban->next;
      dispose_ban( ban );
   }
   return;
}

/*
 * Load up one Class or one race ban structure.
 */

void fread_ban( FILE * fp )
{
   BAN_DATA *pban;
   unsigned int i = 0;

   CREATE( pban, BAN_DATA, 1 );

   pban->name = fread_string_nohash( fp );
   pban->user = NULL;
   pban->level = fread_number( fp );
   pban->duration = fread_number( fp );
   pban->unban_date = fread_number( fp );
   pban->prefix = fread_number( fp );
   pban->suffix = fread_number( fp );
   pban->warn = fread_number( fp );
   pban->ban_by = fread_string_nohash( fp );
   pban->ban_time = fread_string_nohash( fp );
   pban->note = fread_string_nohash( fp );

   for( i = 0; i < strlen( pban->name ); i++ )
   {
      if( pban->name[i] == '@' )
      {
         char *temp;
         char *temp2;

         temp = str_dup( pban->name );
         temp[i] = '\0';
         temp2 = &pban->name[i + 1];
         DISPOSE( pban->name );
         pban->name = str_dup( temp2 );
         pban->user = str_dup( temp );
         DISPOSE( temp );
         break;
      }
   }
   LINK( pban, first_ban, last_ban, next, prev );
   return;
}

/*
 * Load all those nasty bans up :)
 * 	Shaddai
 */
void load_banlist( void )
{
   const char *word;
   FILE *fp;
   bool fMatch = FALSE;

   first_ban = NULL;
   last_ban = NULL;

   if( !( fp = fopen( SYSTEM_DIR BAN_LIST, "r" ) ) )
   {
      bug( "Save_banlist: Cannot open %s", BAN_LIST );
      perror( BAN_LIST );
      return;
   }
   for( ;; )
   {
      word = feof( fp ) ? "END" : fread_word( fp );
      fMatch = FALSE;
      switch ( UPPER( word[0] ) )
      {
         case 'E':
            if( !str_cmp( word, "END" ) ) /* File should always contain END */
            {
               FCLOSE( fp );
               return;
            }
         case 'S':
            if( !str_cmp( word, "SITE" ) )
            {
               fread_ban( fp );
               fMatch = TRUE;
            }
            break;
      }
      if( !fMatch )
         bug( "Load_banlist: no match: %s", word );
   }  /* End of for loop */
}

/*
 * Saves all bans, for sites, classes and races.
 * 	Shaddai
 */

void save_banlist( void )
{
   BAN_DATA *pban;
   FILE *fp;

   if( !( fp = fopen( SYSTEM_DIR BAN_LIST, "w" ) ) )
   {
      bug( "Save_banlist: Cannot open %s", BAN_LIST );
      perror( BAN_LIST );
      return;
   }

   /*
    * Print out all the site bans 
    */
   for( pban = first_ban; pban; pban = pban->next )
   {
      fprintf( fp, "%s", "SITE\n" );
      if( pban->user )
         fprintf( fp, "%s@%s~\n", pban->user, pban->name );
      else
         fprintf( fp, "%s~\n", pban->name );
      fprintf( fp, "%d %d %d %d %d %d\n", pban->level, pban->duration,
               pban->unban_date, pban->prefix, pban->suffix, pban->warn );
      fprintf( fp, "%s~\n%s~\n%s~\n", pban->ban_by, pban->ban_time, pban->note );
   }
   fprintf( fp, "%s", "END\n" ); /* File must have an END even if empty */
   FCLOSE( fp );
   return;
}

/*
 *  This actually puts the new ban into the proper linked list and
 *  initializes its data.  Shaddai
 */
/* ban <address> <type> <duration> */
void add_ban( CHAR_DATA * ch, char *arg1, char *arg2, int bantime )
{
   BAN_DATA *pban, *temp;
   struct tm *tms;
   char *name;
   int level;
   bool prefix = FALSE, suffix = FALSE, user_name = FALSE;
   char *temp_host = NULL, *temp_user = NULL;
   unsigned int x;

   /*
    * Should we check to see if they have dropped link sometime in between 
    * * writing the note and now?  Not sure but for right now we won't since
    * * do_ban checks for that.  Shaddai
    */

   switch ( ch->substate )
   {
      default:
         bug( "add_ban: illegal substate: %d", ch->substate );
         return;

      case SUB_RESTRICTED:
         send_to_char( "You cannot use this command from within another command.\n\r", ch );
         return;

      case SUB_NONE:
      {
         smash_tilde( arg1 ); /* Make sure the immortals don't put a ~ in it. */

         if( arg1[0] == '\0' || arg2[0] == '\0' )
            return;

         if( is_number( arg2 ) )
         {
            level = atoi( arg2 );
            if( level < 0 || level > LEVEL_SUPREME )
            {
               ch_printf( ch, "Level range is from 0 to %d.\n\r", LEVEL_SUPREME );
               return;
            }
         }
         else if( !str_cmp( arg2, "all" ) )
            level = LEVEL_SUPREME;
         else if( !str_cmp( arg2, "newbie" ) )
            level = 1;
         else if( !str_cmp( arg2, "mortal" ) )
            level = LEVEL_AVATAR;
         else if( !str_cmp( arg2, "warn" ) )
            level = BAN_WARN;
         else
         {
            bug( "Bad string for flag in add_ban: %s", arg2 );
            return;
         }

         for( x = 0; x < strlen( arg1 ); x++ )
         {
            if( arg1[x] == '@' )
            {
               user_name = TRUE;
               temp_host = str_dup( &arg1[x + 1] );
               arg1[x] = '\0';
               temp_user = str_dup( arg1 );
               break;
            }
         }
         if( !user_name )
            name = arg1;
         else
            name = temp_host;

         if( !name ) /* Double check to make sure name isnt null */
         {
            /* Free this stuff if its there */
            if( user_name )
            {
               DISPOSE( temp_host );
               DISPOSE( temp_user );
            }
            send_to_char( "Name was null.\n\r", ch );
            return;
         }

         if( name[0] == '*' )
         {
            prefix = TRUE;
            name++;
         }

         if( name[strlen( name ) - 1] == '*' )
         {
            suffix = TRUE;
            name[strlen( name ) - 1] = '\0';
         }

         for( temp = first_ban; temp; temp = temp->next )
         {
            if( !str_cmp( temp->name, name ) )
            {
               if( temp->level == level && ( prefix && temp->prefix )
                   && ( suffix && temp->suffix ) && ( !user_name || ( user_name && !str_cmp( temp->user, temp_user ) ) ) )
               {
                  /* Free this stuff if its there */
                  if( user_name )
                  {
                     DISPOSE( temp_host );
                     DISPOSE( temp_user );
                  }
                  send_to_char( "That entry already exists.\n\r", ch );
                  return;
               }
               else
               {
                  temp->suffix = suffix;
                  temp->prefix = prefix;
                  if( temp->level == BAN_WARN )
                     temp->warn = TRUE;
                  temp->level = level;
                  strdup_printf( &temp->ban_time, "%24.24s", c_time( current_time, -1 ) );
                  if( bantime > 0 )
                  {
                     temp->duration = bantime;
                     tms = localtime( &current_time );
                     tms->tm_mday += bantime;
                     temp->unban_date = mktime( tms );
                  }
                  else
                  {
                     temp->duration = -1;
                     temp->unban_date = -1;
                  }
                  DISPOSE( temp->ban_by );
                  if( user_name )
                  {
                     DISPOSE( temp_host );
                     DISPOSE( temp_user );
                  }
                  temp->ban_by = str_dup( ch->name );
                  send_to_char( "Updated entry.\n\r", ch );
                  save_banlist(  );
                  return;
               }
            }
         }
         CREATE( pban, BAN_DATA, 1 );
         pban->ban_by = str_dup( ch->name );
         pban->suffix = suffix;
         pban->prefix = prefix;
         pban->name = str_dup( name );
         pban->level = level;
         if( user_name )
         {
            pban->user = str_dup( temp_user );
            DISPOSE( temp_host );
            DISPOSE( temp_user );
         }
         LINK( pban, first_ban, last_ban, next, prev );
         strdup_printf( &pban->ban_time, "%24.24s", c_time( current_time, -1 ) );
         if( bantime > 0 )
         {
            pban->duration = bantime;
            tms = localtime( &current_time );
            tms->tm_mday += bantime;
            pban->unban_date = mktime( tms );
         }
         else
         {
            pban->duration = -1;
            pban->unban_date = -1;
         }
         if( pban->level == BAN_WARN )
            pban->warn = TRUE;
         ch->substate = SUB_BAN_DESC;
         ch->pcdata->dest_buf = pban;
         if( !pban->note )
            pban->note = str_dup( "" );
         start_editing( ch, pban->note );
         set_editor_desc( ch, "A ban description." );
         return;
      }

      case SUB_BAN_DESC:
         pban = ( BAN_DATA * ) ch->pcdata->dest_buf;
         if( !pban )
         {
            bug( "%s", "do_ban: sub_ban_desc: NULL ch->pcdata->dest_buf" );
            ch->substate = SUB_NONE;
            return;
         }
         DISPOSE( pban->note );
         pban->note = copy_buffer_nohash( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         save_banlist(  );
         if( pban->duration > 0 )
         {
            if( !pban->user )
               ch_printf( ch, "%s banned for %d days.\n\r", pban->name, pban->duration );
            else
               ch_printf( ch, "%s@%s banned for %d days.\n\r", pban->user, pban->name, pban->duration );
         }
         else
         {
            if( !pban->user )
               ch_printf( ch, "%s banned forever.\n\r", pban->name );
            else
               ch_printf( ch, "%s@%s banned forever.\n\r", pban->user, pban->name );
         }
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting ban note.\n\r", ch );
         return;
   }
}

/*
 * Print the bans out to the screen.  Shaddai
 */
void show_bans( CHAR_DATA * ch )
{
   BAN_DATA *pban;
   int bnum;

   set_pager_color( AT_IMMORT, ch );

   send_to_pager( "Banned sites:\n\r", ch );
   send_to_pager( "[ #] Warn (Lv) Time                      By              For    Site\n\r", ch );
   send_to_pager( "---- ---- ----- ------------------------ --------------- ----   ---------------\n\r", ch );
   pban = first_ban;
   set_pager_color( AT_PLAIN, ch );
   for( bnum = 1; pban; pban = pban->next, bnum++ )
   {
      if( !pban->user )
         pager_printf( ch, "[%2d] %-4s (%3d) %-24s %-15s %4d  %c%s%c\n\r",
                       bnum, ( pban->warn ) ? "YES" : "no", pban->level, pban->ban_time, pban->ban_by, pban->duration,
                       ( pban->prefix ) ? '*' : ' ', pban->name, ( pban->suffix ) ? '*' : ' ' );
      else
         pager_printf( ch, "[%2d] %-4s (%2d) %-24s %-15s %4d  %s@%c%s%c\n\r",
                       bnum, ( pban->warn ) ? "YES" : "no", pban->level, pban->ban_time, pban->ban_by, pban->duration,
                       pban->user, ( pban->prefix ) ? '*' : ' ', pban->name, ( pban->suffix ) ? '*' : ' ' );
   }
   return;
}

/*
 * The main command for ban, lots of arguments so be carefull what you
 * change here.		Shaddai
 */
/* ban <address> <type> <duration> */
CMDF do_ban( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL], *temp;
   BAN_DATA *pban;
   int value = 0, bantime = -1;

   if( IS_NPC( ch ) )   /* Don't want mobs banning sites ;) */
   {
      send_to_char( "Monsters are too dumb to do that!\n\r", ch );
      return;
   }

   if( !ch->desc )   /* No desc means no go :) */
   {
      bug( "%s", "do_ban: no descriptor" );
      return;
   }

   set_char_color( AT_IMMORT, ch );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   /*
    * Do we have a time duration for the ban? 
    */
   if( argument && argument[0] != '\0' && is_number( argument ) )
      bantime = atoi( argument );
   else
      bantime = -1;

   /*
    * -1 is default, but no reason the time should be greater than 1000
    * * or less than 1, after all if it is greater than 1000 you are talking
    * * around 3 years.
    */
   if( bantime != -1 && ( bantime < 1 || bantime > 1000 ) )
   {
      send_to_char( "Time value is -1 (forever) or from 1 to 1000.\n\r", ch );
      return;
   }

   /*
    * Need to be carefull with sub-states or everything will get messed up.
    */
   switch ( ch->substate )
   {
      default:
         bug( "do_ban: illegal substate: %d", ch->substate );
         return;
      case SUB_RESTRICTED:
         send_to_char( "You cannot use this command from within another command.\n\r", ch );
         return;
      case SUB_NONE:
         ch->tempnum = SUB_NONE;
         break;

         /*
          * Returning to end the editing of the note 
          */
      case SUB_BAN_DESC:
         add_ban( ch, "", "", 0 );
         return;
   }

   if( arg1[0] == '\0' )
   {
      show_bans( ch );
      send_to_char( "Syntax: ban <address> <type> <duration>\n\r", ch );
      send_to_char( "Syntax: ban show <number>\n\r", ch );
      send_to_char( "    No arguments lists the current bans.\n\r", ch );
      send_to_char( "    Duration is how long the ban lasts in days.\n\r", ch );
      send_to_char( "    Type is one of the following:\n\r", ch );
      send_to_char( "      Newbie: Only new characters are banned.\n\r", ch );
      send_to_char( "      Mortal: All mortals, including avatars, are banned.\n\r", ch );
      send_to_char( "      All:    ALL players, including immortals, are banned.\n\r", ch );
      send_to_char( "      Warn:   Simply warns when someone logs on from the site.\n\r", ch );
      send_to_char( "       Or type can be a level.\n\r", ch );
      return;
   }

   /*
    * If no args are sent after the Class/site/race, show the current banned
    * * items.  Shaddai
    */

   /*
    * Modified by Samson - We only ban site on Alsherok, not races and classes.
    * Not even sure WHY the smaugers felt this necessary. 
    */
   if( !str_cmp( arg1, "show" ) )
   {
      /*
       * This will show the note attached to a ban 
       */
      if( arg2[0] == '\0' )
      {
         do_ban( ch, "" );
         return;
      }
      temp = arg2;

      if( arg2[0] == '#' ) /* Use #1 to show the first ban */
      {
         temp = arg2;
         temp++;
         if( !is_number( temp ) )
         {
            send_to_char( "Which ban # to show?\n\r", ch );
            return;
         }
         value = atoi( temp );
         if( value < 1 )
         {
            send_to_char( "You must specify a number greater than 0.\n\r", ch );
            return;
         }
      }
      pban = first_ban;
      if( temp[0] == '*' )
         temp++;
      if( temp[strlen( temp ) - 1] == '*' )
         temp[strlen( temp ) - 1] = '\0';

      for( ; pban; pban = pban->next )
         if( value == 1 || !str_cmp( pban->name, temp ) )
            break;
         else if( value > 1 )
            value--;

      if( !pban )
      {
         send_to_char( "No such ban.\n\r", ch );
         return;
      }
      ch_printf( ch, "Banned by: %s\n\r", pban->ban_by );
      send_to_char( pban->note, ch );
      return;
   }

   if( arg1 != '\0' )
   {
      if( arg2[0] == '\0' )
      {
         do_ban( ch, "" );
         return;
      }
      add_ban( ch, arg1, arg2, bantime );
      return;
   }
   return;
}

/*
 * Allow a already banned site/Class or race.  Shaddai
 */
CMDF do_allow( CHAR_DATA * ch, char *argument )
{
   BAN_DATA *pban;
   char arg1[MIL];
   char *temp = NULL;
   bool fMatch = FALSE;
   int value = 0;

   if( IS_NPC( ch ) )   /* No mobs allowing sites */
   {
      send_to_char( "Monsters are too dumb to do that!\n\r", ch );
      return;
   }

   if( !ch->desc )   /* No desc is a bad thing */
   {
      bug( "%s", "do_allow: no descriptor" );
      return;
   }

   argument = one_argument( argument, arg1 );

   set_char_color( AT_IMMORT, ch );

   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Syntax: allow <address>\n\r", ch );
      return;
   }

   if( arg1[0] == '#' ) /* Use #1 to ban the first ban in the list specified */
   {
      temp = arg1;
      temp++;
      if( !is_number( temp ) )
      {
         send_to_char( "Which ban # to allow?\n\r", ch );
         return;
      }
      value = atoi( temp );
   }

   if( arg1[0] != '\0' )
   {
      if( !value )
      {
         if( strlen( arg1 ) < 2 )
         {
            send_to_char( "You have to have at least 2 chars for a ban\n\r", ch );
            send_to_char( "If you are trying to allow by number use #\n\r", ch );
            return;
         }

         temp = arg1;
         if( arg1[0] == '*' )
            temp++;
         if( temp[strlen( temp ) - 1] == '*' )
            temp[strlen( temp ) - 1] = '\0';
      }

      for( pban = first_ban; pban; pban = pban->next )
      {
         /*
          * Need to make sure we dispose properly of the ban_data 
          * * Or memory problems will be created.
          * * Shaddai
          */
         if( value == 1 || !str_cmp( pban->name, temp ) )
         {
            fMatch = TRUE;
            dispose_ban( pban );
            break;
         }
         if( value > 1 )
            value--;
      }
   }
   if( fMatch )
   {
      save_banlist(  );
      ch_printf( ch, "%s is now allowed.\n\r", arg1 );
   }
   else
      ch_printf( ch, "%s was not banned.\n\r", arg1 );
   return;
}

/* 
 *  Sets the warn flag on bans.
 */
CMDF do_warn( CHAR_DATA * ch, char *argument )
{
   char arg1[MSL], arg2[MSL];
   char *name;
   int count = -1;
   BAN_DATA *pban, *start, *end;

   /*
    * Don't want mobs or link-deads doing this.
    */
   if( IS_NPC( ch ) )
   {
      send_to_char( "Monsters are too dumb to do that!\n\r", ch );
      return;
   }

   if( !ch->desc )
   {
      bug( "%s", "do_warn: no descriptor" );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Syntax: warn site  <field>\n\r", ch );
      send_to_char( "Field is either #(ban_number) or the site.\n\r", ch );
      send_to_char( "Example:  warn 1.2.3.4\n\r", ch );
      return;
   }

   if( arg2[0] == '#' )
   {
      name = arg2;
      name++;
      if( !is_number( name ) )
      {
         do_warn( ch, "" );
         return;
      }
      count = atoi( name );
      if( count < 1 )
      {
         send_to_char( "The number has to be above 0.\n\r", ch );
         return;
      }
   }

   pban = first_ban;
   start = first_ban;
   end = last_ban;

   for( ; pban && count != 0; count--, pban = pban->next )
      if( count == -1 && !str_cmp( pban->name, arg2 ) )
         break;
   if( pban )
   {
      /*
       * If it is just a warn delete it, otherwise remove the warn flag. 
       */
      if( pban->warn )
      {
         if( pban->level == BAN_WARN )
         {
            dispose_ban( pban );
            send_to_char( "Warn has been deleted.\n\r", ch );
         }
         else
         {
            pban->warn = FALSE;
            send_to_char( "Warn turned off.\n\r", ch );
         }
      }
      else
      {
         pban->warn = TRUE;
         send_to_char( "Warn turned on.\n\r", ch );
      }
      save_banlist(  );
   }
   else
   {
      ch_printf( ch, "%s was not found in the ban list.\n\r", arg2 );
      return;
   }
   return;
}

/*
 * Check for totally banned sites.  Need this because we don't have a
 * char struct yet.  Shaddai
 */

bool check_total_bans( DESCRIPTOR_DATA * d )
{
   BAN_DATA *pban;
   char new_host[MSL];
   int i;

   for( i = 0; i < ( int )strlen( d->host ); i++ )
      new_host[i] = LOWER( d->host[i] );
   new_host[i] = '\0';

   for( pban = first_ban; pban; pban = pban->next )
   {
      if( pban->level != LEVEL_SUPREME )
         continue;
      if( pban->prefix && pban->suffix && strstr( new_host, pban->name ) )
      {
         if( check_expire( pban ) )
         {
            dispose_ban( pban );
            save_banlist(  );
            return FALSE;
         }
         else
            return TRUE;
      }
      /*
       *   Bug of switched checks noticed by Cronel
       */
      if( pban->suffix && !str_prefix( pban->name, new_host ) )
      {
         if( check_expire( pban ) )
         {
            dispose_ban( pban );
            save_banlist(  );
            return FALSE;
         }
         else
            return TRUE;
      }
      if( pban->prefix && !str_suffix( pban->name, new_host ) )
      {
         if( check_expire( pban ) )
         {
            dispose_ban( pban );
            save_banlist(  );
            return FALSE;
         }
         else
            return TRUE;
      }
      if( !str_cmp( pban->name, new_host ) )
      {
         if( check_expire( pban ) )
         {
            dispose_ban( pban );
            save_banlist(  );
            return FALSE;
         }
         else
            return TRUE;
      }
   }
   return FALSE;
}

/*
 * The workhose, checks for bans. Shaddai
 */
bool check_bans( CHAR_DATA * ch )
{
   BAN_DATA *pban;
   char new_host[MSL];
   bool fMatch = FALSE;
   int i;

   for( i = 0; i < ( int )( strlen( ch->desc->host ) ); i++ )
      new_host[i] = LOWER( ch->desc->host[i] );
   new_host[i] = '\0';

   for( pban = first_ban; pban; pban = pban->next )
   {
      if( pban->prefix && pban->suffix && strstr( pban->name, new_host ) )
         fMatch = TRUE;
      else if( pban->prefix && !str_suffix( pban->name, new_host ) )
         fMatch = TRUE;
      else if( pban->suffix && !str_prefix( pban->name, new_host ) )
         fMatch = TRUE;
      else if( !str_cmp( pban->name, new_host ) )
         fMatch = TRUE;
      else
         fMatch = FALSE;
      if( fMatch )
      {
         if( check_expire( pban ) )
         {
            dispose_ban( pban );
            save_banlist(  );
            return FALSE;
         }
         if( ch->level > pban->level )
         {
            if( pban->warn )
               log_printf( "%s logging in from site %s.", ch->name, ch->desc->host );
            return FALSE;
         }
         else
            return TRUE;
      }
   }
   return FALSE;
}
