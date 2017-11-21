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
 *                  Noplex's Immhost Verification Module                    *
 ****************************************************************************/

/*******************************************************
		Crimson Blade Codebase
	Copyright 2000-2002 Noplex (John Bellone)
	      http://www.crimsonblade.org
		admin@crimsonblade.org
		Coders: Noplex, Krowe
		 Based on Smaug 1.4a
*******************************************************/

/*
======================
Advanced Immortal Host
======================
By Noplex with help from Senir and Samson
*/

#include <string.h>
#include "mud.h"
#include "imm_host.h"

IMMORTAL_HOST_LOG *fread_imm_host_log( FILE * fp )
{
   IMMORTAL_HOST_LOG *hlog = NULL;
   const char *word;
   bool fMatch;

   CREATE( hlog, IMMORTAL_HOST_LOG, 1 );

   for( ;; )
   {
      word = feof( fp ) ? "LEnd" : fread_word( fp );
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fread_to_eol( fp );
            break;

         case 'L':
            if( !str_cmp( word, "LEnd" ) )
            {
               if( hlog->date && hlog->date[0] != '\0' && hlog->host && hlog->host[0] != '\0' )
                  return hlog;

               STRFREE( hlog->date );
               STRFREE( hlog->host );
               DISPOSE( hlog );
               return NULL;
            }
            KEY( "Log_Date", hlog->date, fread_string( fp ) );
            KEY( "Log_Host", hlog->host, fread_string( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match for %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

IMMORTAL_HOST *fread_imm_host( FILE * fp )
{
   IMMORTAL_HOST *host = NULL;
   IMMORTAL_HOST_LOG *hlog = NULL, *hlognext = NULL;
   const char *word;
   bool fMatch = FALSE;
   short dnum = 0;

   CREATE( host, IMMORTAL_HOST, 1 );

   for( ;; )
   {
      word = feof( fp ) ? "ZEnd" : fread_word( fp );
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fread_to_eol( fp );
            break;

         case 'D':
            if( !str_cmp( word, "Domain_host" ) )
            {
               if( dnum >= MAX_DOMAIN )
                  bug( "%s", "fread_imm_host(): more saved domains than MAX_DOMAIN" );
               else
                  host->domain[dnum++] = fread_string( fp );

               fMatch = TRUE;
               break;
            }
            break;

         case 'L':
            if( !str_cmp( word, "LOG" ) )
            {
               if( ( hlog = fread_imm_host_log( fp ) ) == NULL )
                  bug( "%s", "fread_imm_host(): incomplete log returned" );
               else
                  LINK( hlog, host->first_log, host->last_log, next, prev );

               fMatch = TRUE;
               break;
            }
            break;

         case 'N':
            KEY( "Name", host->name, fread_string( fp ) );
            break;

         case 'Z':
            if( !str_cmp( word, "ZEnd" ) )
            {
               if( !host->name || host->name[0] == '\0' || !host->domain[0] || host->domain[0][0] == '\0' )
               {

                  STRFREE( host->name );
                  for( dnum = 0; dnum < MAX_DOMAIN && host->domain[dnum] && host->domain[dnum][0] != '\0'; dnum++ )
                     STRFREE( host->domain[dnum] );

                  for( hlog = host->first_log; hlog; hlog = hlognext )
                  {
                     hlognext = hlog->next;
                     STRFREE( hlog->date );
                     STRFREE( hlog->host );
                     DISPOSE( hlog );
                  }
                  DISPOSE( host );
                  return NULL;
               }
               return host;
            }
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match for %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

void load_imm_host( void )
{
   FILE *fp;

   first_imm_host = NULL;
   last_imm_host = NULL;

   if( ( fp = fopen( IMM_HOST_FILE, "r" ) ) == NULL )
   {
      bug( "%s", "load_imm_host(): could not open immhost file for reading" );
      return;
   }

   for( ;; )
   {
      char letter = fread_letter( fp );
      char *word;

      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }

      if( letter != '#' )
      {
         bug( "%s", "load_imm_host(): # not found" );
         break;
      }

      word = fread_word( fp );

      if( !str_cmp( word, "IMMORTAL" ) )
      {
         IMMORTAL_HOST *host = NULL;

         if( ( host = fread_imm_host( fp ) ) == NULL )
         {
            bug( "%s", "load_imm_host(): incomplete immhost" );
            continue;
         }

         LINK( host, first_imm_host, last_imm_host, next, prev );
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "load_imm_host(): unknown section %s", word );
         continue;
      }
   }
   FCLOSE( fp );
   return;
}

void save_imm_host( void )
{
   FILE *fp;
   IMMORTAL_HOST *host = NULL;

   if( ( fp = fopen( IMM_HOST_FILE, "w" ) ) == NULL )
   {
      bug( "%s", "load_imm_host(): could not open immhost file for writing" );
      return;
   }

   for( host = first_imm_host; host; host = host->next )
   {
      IMMORTAL_HOST_LOG *nlog = NULL;
      short dnum = 0;

      fprintf( fp, "%s", "\n#IMMORTAL\n" );
      fprintf( fp, "Name	       %s~\n", host->name );

      for( dnum = 0; dnum < MAX_DOMAIN && host->domain[dnum] && host->domain[dnum][0] != '\0'; dnum++ )
         fprintf( fp, "Domain_Host       %s~\n", host->domain[dnum] );

      for( nlog = host->first_log; nlog; nlog = nlog->next )
      {
         fprintf( fp, "%s", "LOG\n" );
         fprintf( fp, "Log_Host	%s~\n", nlog->host );
         fprintf( fp, "Log_Date	%s~\n", nlog->date );
         fprintf( fp, "%s", "LEnd\n" );
      }
      fprintf( fp, "%s", "ZEnd\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   FCLOSE( fp );
   return;
}

bool check_immortal_domain( CHAR_DATA * ch, char *host )
{
   IMMORTAL_HOST *ihost = NULL;
   IMMORTAL_HOST_LOG *nlog = NULL;
   short x = 0;

   for( ihost = first_imm_host; ihost; ihost = ihost->next )
   {
      if( !str_cmp( ihost->name, ch->name ) )
         break;
   }

   /*
    * no immortal host or no domains 
    */
   if( !ihost || !ihost->domain[0] || ihost->domain[0][0] == '\0' )
      return TRUE;

   /*
    * check if the domain is valid 
    */
   for( x = 0; x < MAX_DOMAIN && ihost->domain[x] && ihost->domain[x][0] != '\0'; x++ )
   {
      bool suffix = FALSE, prefix = FALSE;
      char chost[50];
      short s = 0, t = 0;

      if( ihost->domain[x][0] == '*' )
      {
         prefix = TRUE;
         t = 1;
      }

      while( ihost->domain[x][t] != '\0' )
         chost[s++] = ihost->domain[x][t++];

      chost[s] = '\0';

      if( chost[strlen( chost ) - 1] == '*' )
      {
         chost[strlen( chost ) - 1] = '\0';
         suffix = TRUE;
      }

      if( ( prefix && suffix && !str_infix( ch->desc->host, chost ) )
          || ( prefix && !str_suffix( chost, ch->desc->host ) )
          || ( suffix && !str_prefix( chost, ch->desc->host ) ) || ( !str_cmp( chost, ch->desc->host ) ) )
      {
         log_printf( "&C&GImmotal_Host: %s's host authorized.", ch->name );
         return TRUE;
      }
   }

   /*
    * denied attempts now get logged 
    */
   log_printf( "&C&RImmortal_Host: %s's host denied. This hacking attempt has been logged.", ch->name );

   CREATE( nlog, IMMORTAL_HOST_LOG, 1 );
   nlog->host = STRALLOC( host );
   stralloc_printf( &nlog->date, "%.24s", c_time( current_time, -1 ) );
   LINK( nlog, ihost->first_log, ihost->last_log, next, prev );

   save_imm_host(  );

   send_to_char( "You have been caught attempting to hack an immortal's character and have been logged.\n\r", ch );

   return FALSE;
}

CMDF do_immhost( CHAR_DATA * ch, char *argument )
{
   IMMORTAL_HOST *host = NULL;
   IMMORTAL_HOST_LOG *hlog = NULL;
   char arg[MIL];
   char arg2[MIL];
   short x = 0;

   if( IS_NPC( ch ) || !IS_IMMORTAL( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "&C&RSyntax: &Gimmhost list\n\r"
                    "&RSyntax: &Gimmhost add <&rcharacter&G>\n\r"
                    "&RSyntax: &Gimmhost remove <&rcharacter&G>\n\r"
                    "&RSyntax: &Gimmhost viewlogs <&rcharacter&G>\n\r"
                    "&RSyntax: &Gimmhost removelog <&rcharacter&G> <&rlog number&G>\n\r"
                    "&RSyntax: &Gimmhost viewdomains <&rcharacter&G>\n\r"
                    "&RSyntax: &Gimmhost createdomain <&rcharacter&G> <&rhost&G>\n\r"
                    "&RSyntax: &Gimmhost removedomain <&rcharacter&G> <&rdomain number&G>\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "list" ) )
   {

      if( !first_imm_host || !last_imm_host )
      {
         send_to_char( "No immortals are currently protected at this time.\n\r", ch );
         return;
      }

      send_to_pager( "&C&R[&GName&R]     [&GDomains&R]  [&GLogged Attempts&R]\n\r", ch );

      for( host = first_imm_host; host; host = host->next, x++ )
      {
         short lnum = 0, dnum = 0;

         while( dnum < MAX_DOMAIN && host->domain[dnum] && host->domain[dnum][0] != '\0' )
            dnum++;
         for( hlog = host->first_log; hlog; hlog = hlog->next )
            lnum++;

         pager_printf( ch, "&C&G%-10s %-10d %d\n\r", host->name, dnum, lnum );
      }

      pager_printf( ch, "&C&R%d immortals are being protected.&g\n\r", x );
      return;
   }

   argument = one_argument( argument, arg2 );

   if( !arg2 || arg2[0] == '\0' )
   {
      send_to_char( "Which character would you like to use?\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "add" ) )
   {

      CREATE( host, IMMORTAL_HOST, 1 );

      smash_tilde( arg2 );
      host->name = STRALLOC( capitalize( arg2 ) );
      host->first_log = NULL;
      host->last_log = NULL;

      LINK( host, first_imm_host, last_imm_host, next, prev );

      save_imm_host(  );

      send_to_char( "Immortal host added.\n\r", ch );
      return;
   }

   for( host = first_imm_host; host; host = host->next )
   {
      if( !str_cmp( host->name, arg2 ) )
         break;
   }

   if( !host )
   {
      send_to_char( "There is no immortal host with that name.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "remove" ) )
   {
      IMMORTAL_HOST_LOG *nexthlog = NULL;

      UNLINK( host, first_imm_host, last_imm_host, next, prev );

      for( x = 0; x < MAX_DOMAIN && host->domain[x] && host->domain[x][0] != '\0'; x++ )
         STRFREE( host->domain[x] );

      for( hlog = host->first_log; hlog; hlog = nexthlog )
      {
         nexthlog = hlog->next;
         UNLINK( hlog, host->first_log, host->last_log, next, prev );
         STRFREE( hlog->host );
         STRFREE( hlog->date );
         DISPOSE( hlog );
      }
      STRFREE( host->name );
      DISPOSE( host );

      save_imm_host(  );

      send_to_char( "Immortal host removed.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "viewlogs" ) )
   {

      if( !host->first_log || !host->last_log )
      {
         send_to_char( "There are no logs for this immortal host.\n\r", ch );
         return;
      }

      pager_printf( ch, "&C&RImmortal:&W %s\n\r", host->name );
      send_to_pager( "&R[&GNum&R]  [&GLogged Host&R]     [&GDate&R]\n\r", ch );

      for( hlog = host->first_log; hlog; hlog = hlog->next )
         pager_printf( ch, "&C&G%-6d %-17s %s\n\r", ++x, hlog->host, hlog->date );

      pager_printf( ch, "&C&R%d logged hacking attempts.&g\n\r", x );
      return;
   }

   if( !str_cmp( arg, "removelog" ) )
   {
      if( !argument || argument[0] == '\0' || !is_number( argument ) )
      {
         send_to_char( "Syntax: immhost removelog <character> <log number>\n\r", ch );
         return;
      }

      for( hlog = host->first_log; hlog; hlog = hlog->next )
      {
         if( ++x == atoi( argument ) )
            break;
      }

      if( !hlog )
      {
         send_to_char( "That immortal host doesn't have a log with that number.\n\r", ch );
         return;
      }

      UNLINK( hlog, host->first_log, host->last_log, next, prev );

      STRFREE( hlog->host );
      STRFREE( hlog->date );
      DISPOSE( hlog );

      save_imm_host(  );
      send_to_char( "Log removed.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "viewdomains" ) )
   {

      send_to_pager( "&C&R[&GNum&R]  [&GHost&R]\n\r", ch );

      for( x = 0; x < MAX_DOMAIN && host->domain[x] && host->domain[x][0] != '\0'; x++ )
         pager_printf( ch, "&C&G%-5d  %s\n\r", x + 1, host->domain[x] );

      pager_printf( ch, "&C&R%d immortal domains.&g\n\r", x );
      return;
   }

   if( !str_cmp( arg, "createdomain" ) )
   {

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Syntax: immhost createdomain <character> <host>\n\r", ch );
         return;
      }

      smash_tilde( argument );

      for( x = 0; x < MAX_DOMAIN && host->domain[x] && host->domain[x][0] != '\0'; x++ )
      {
         if( !str_cmp( argument, host->domain[x] ) )
         {
            send_to_char( "That immortal host already has an entry like that.\n\r", ch );
            return;
         }
      }

      if( x == MAX_DOMAIN )
      {
         pager_printf( ch, "This immortal host has the maximum allowed, %d domains.\n\r", MAX_DOMAIN );
         return;
      }

      host->domain[x] = STRALLOC( argument );

      save_imm_host(  );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "removedomain" ) )
   {

      if( !argument || argument[0] == '\0' || !is_number( argument ) )
      {
         send_to_char( "Syntax: immhost removedomain <character> <domain number>\n\r", ch );
         return;
      }

      x = URANGE( 1, atoi( argument ), MAX_DOMAIN );
      x--;

      if( !host->domain[x] || host->domain[x][0] == '\0' )
      {
         send_to_char( "That immortal host doesn't have a domain with that number.\n\r", ch );
         return;
      }

      STRFREE( host->domain[x] );

      save_imm_host(  );
      send_to_char( "Domain removed.\n\r", ch );
      return;
   }

   do_immhost( ch, "" );
   return;
}
