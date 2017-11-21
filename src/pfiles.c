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
 *                          Pfile Pruning Module                            *
 ****************************************************************************/

#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "mud.h"
#include "clans.h"
#include "deity.h"
#include "pfiles.h"

void verify_clans( void );
CLAN_DATA *get_clan( char *name );
void save_clan( CLAN_DATA * clan );
void remove_member( char *clan_name, char *name );
void prune_sales( void );
void remove_from_auth( char *name );
void rent_update( void );
DEITY_DATA *get_deity( char *name );
void save_timedata( void );
void rent_adjust_pfile( char *argument );

/* Globals */
time_t new_pfile_time_t;
short num_pfiles;   /* Count up number of pfiles */
time_t now_time;
short deleted = 0;
short days = 0;

CMDF do_pcrename( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   char arg1[MIL], newname[256], oldname[256];

   argument = one_argument( argument, arg1 );
   smash_tilde( argument );

   if( IS_NPC( ch ) )
      return;

   if( !arg1 || arg1[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax: rename <victim> <new name>\n\r", ch );
      return;
   }

   if( !check_parse_name( argument, TRUE ) )
   {
      send_to_char( "Illegal name.\n\r", ch );
      return;
   }

   /*
    * Just a security precaution so you don't rename someone you don't mean too --Shaddai 
    */
   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      send_to_char( "That person is not in the room.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "You can't rename NPC's.\n\r", ch );
      return;
   }

   if( get_trust( ch ) < get_trust( victim ) )
   {
      send_to_char( "I don't think they would like that!\n\r", ch );
      return;
   }
   snprintf( newname, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ) );
   snprintf( oldname, 256, "%s%c/%s", PLAYER_DIR, tolower( victim->pcdata->filename[0] ),
             capitalize( victim->pcdata->filename ) );

   if( access( newname, F_OK ) == 0 )
   {
      send_to_char( "That name already exists.\n\r", ch );
      return;
   }

   /*
    * Have to remove the old god entry in the directories 
    */
   if( IS_IMMORTAL( victim ) )
   {
      char godname[256];
      snprintf( godname, 256, "%s%s", GOD_DIR, capitalize( victim->pcdata->filename ) );
      remove( godname );
   }

   /*
    * Remember to change the names of the areas 
    */
   if( victim->pcdata->area )
   {
      char filename[256], newfilename[256];

      snprintf( filename, 256, "%s%s.are", BUILD_DIR, victim->name );
      snprintf( newfilename, 256, "%s%s.are", BUILD_DIR, capitalize( argument ) );
      rename( filename, newfilename );
      snprintf( filename, 256, "%s%s.are.bak", BUILD_DIR, victim->name );
      snprintf( newfilename, 256, "%s%s.are.bak", BUILD_DIR, capitalize( argument ) );
      rename( filename, newfilename );
   }

   /*
    * If they're in a clan/guild, remove them from the roster for it 
    */
   if( victim->pcdata->clan )
      remove_member( victim->pcdata->clan->name, victim->name );

   STRFREE( victim->name );
   victim->name = STRALLOC( capitalize( argument ) );
   STRFREE( victim->pcdata->filename );
   victim->pcdata->filename = STRALLOC( capitalize( argument ) );
   if( remove( oldname ) )
   {
      log_printf( "Error: Couldn't delete file %s in do_rename.", oldname );
      send_to_char( "Couldn't delete the old file!\n\r", ch );
   }
   /*
    * Time to save to force the affects to take place 
    */
   save_char_obj( victim );

   /*
    * Now lets update the wizlist 
    */
   if( IS_IMMORTAL( victim ) )
      make_wizlist(  );
   send_to_char( "Character was renamed.\n\r", ch );
   return;
}

void search_pfiles( CHAR_DATA *ch, char *dirname, char *filename, int cvnum )
{
   FILE *fpChar;
   char fname[256];

   snprintf( fname, 256, "%s/%s", dirname, filename );
   if( !( fpChar = fopen( fname, "r" ) ) )
   {
      perror( fname );
      return;
   }

   for( ;; )
   {
      int vnum, nest = 0, counter = 1;
      bool done = false, fMatch;

      char letter = fread_letter( fpChar );
      if( letter == '\0' )
      {
         log_printf( "%s: EOF encountered reading file: %s!", __FUNCTION__, fname );
         break;
      }
      
      if( letter != '#' )
         continue;

      const char *word = ( feof( fpChar ) ? "End" : fread_word( fpChar ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file: %s!", __FUNCTION__, fname );
         word = "End";
      }

      if( !str_cmp( word, "End" ) )
         break;

      if( !str_cmp( word, "OBJECT" ) )
      {
         while( !done )
         {
            word = ( feof( fpChar ) ? "End" : fread_word( fpChar ) );

            if( word[0] == '\0' )
            {
               log_printf( "%s: EOF encountered reading file: %s!", __FUNCTION__, fname );
               word = "End";
            }

            switch( UPPER( word[0] ) )
            {
               default:
                  fread_to_eol( fpChar );
                  break;

               case 'C':
                  KEY( "Count", counter, fread_number( fpChar ) );
                  break;

               case 'E':
                  if( !str_cmp( word, "End" ) )
                  {
                     done = true;
                     break;
                  }

               case 'N':
                  KEY( "Nest", nest, fread_number( fpChar ) );
                  break;

               case 'O':
                  if( !str_cmp( word, "Ovnum" ) )
                  {
                     vnum = fread_number( fpChar );
                     if( !( get_obj_index( vnum ) ) )
                     {
                        bug( "Bad obj vnum in %s: %d", __FUNCTION__, vnum );
                        rent_adjust_pfile( filename );
                     }
                     else
                     {
                        if( vnum == cvnum )
                           pager_printf( ch, "Player %s: Counted %d of Vnum %d.\r\n", filename, counter, cvnum );
                     }
                  }
                  break;
            }
         }
      }
   }
   FCLOSE( fpChar );
   return;
}

/* Scans the pfiles to count the number of copies of a vnum being stored - Samson 1-3-99 */
void check_stored_objects( CHAR_DATA * ch, int cvnum )
{
   DIR *dp;
   struct dirent *dentry;
   char directory_name[100];
   int alpha_loop;

   for( alpha_loop = 0; alpha_loop <= 25; alpha_loop++ )
   {
      snprintf( directory_name, 100, "%s%c", PLAYER_DIR, 'a' + alpha_loop );
      dp = opendir( directory_name );
      dentry = readdir( dp );
      while( dentry )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( !str_cmp( dentry->d_name, "CVS" ) )
         {
            dentry = readdir( dp );
            continue;
         }
         if( dentry->d_name[0] != '.' )
            search_pfiles( ch, directory_name, dentry->d_name, cvnum );
         dentry = readdir( dp );
      }
      closedir( dp );
   }

   return;
}

void fread_pfile( FILE * fp, time_t tdiff, char *fname, bool count )
{
   char *name = NULL;
   char *clan = NULL;
   char *deity = NULL;
   short level = 0;
   short file_ver = 0;
   EXT_BV pact;

   bool fMatch;

   xCLEAR_BITS( pact );
   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      if( !str_cmp( word, "End" ) )
         break;

      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'A':
            KEY( "Act", pact, fread_bitvector( fp ) );
            if( !str_cmp( word, "ActFlags" ) )
            {
               char *actflags = NULL;
               char flag[MIL];
               int value;

               actflags = fread_flagstring( fp );
               while( actflags[0] != '\0' )
               {
                  actflags = one_argument( actflags, flag );
                  if( file_ver < 18 )
                     value = get_actflag( flag );
                  else
                     value = get_plrflag( flag );
                  if( value < 0 || value > MAX_BITS )
                     bug( "%s: Unknown flag: %s\n\r", fname, flag );
                  else
                     xSET_BIT( pact, value );
               }
               fMatch = TRUE;
               break;
            }
            break;

         case 'C':
            KEY( "Clan", clan, fread_string( fp ) );
            break;

         case 'D':
            KEY( "Deity", deity, fread_string( fp ) );
            break;

         case 'L':
            KEY( "Level", level, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", name, fread_string( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "Status" ) )
            {
               level = fread_number( fp );
               fread_to_eol( fp );
               fMatch = TRUE;
               break;
            }
            break;

         case 'V':
            KEY( "Version", file_ver, fread_number( fp ) );
            break;
      }
      if( !fMatch )
         fread_to_eol( fp );
   }

   if( count == FALSE && !xIS_SET( pact, PLR_EXEMPT ) )
   {
      if( level < 10 && tdiff > sysdata.newbie_purge )
      {
         if( unlink( fname ) == -1 )
            perror( "Unlink" );
         else
         {
            days = sysdata.newbie_purge;
            log_printf( "Player %s was deleted. Exceeded time limit of %d days.", name, days );
            remove_from_auth( name );
            if( clan != NULL )
               remove_member( clan, name );
            deleted++;
            STRFREE( clan );
            STRFREE( deity );
            STRFREE( name );
            return;
         }
      }

      if( level < LEVEL_IMMORTAL && tdiff > sysdata.regular_purge )
      {
         if( level < LEVEL_IMMORTAL )
         {
            if( unlink( fname ) == -1 )
               perror( "Unlink" );
            else
            {
               days = sysdata.regular_purge;
               log_printf( "Player %s was deleted. Exceeded time limit of %d days.", name, days );
               remove_from_auth( name );
               if( clan != NULL )
                  remove_member( clan, name );
               deleted++;
               STRFREE( clan );
               STRFREE( deity );
               STRFREE( name );
               return;
            }
         }
      }
   }

   if( clan != NULL )
   {
      CLAN_DATA *guild = get_clan( clan );

      if( guild )
         guild->members++;
   }

   if( deity != NULL )
   {
      DEITY_DATA *god = get_deity( deity );

      if( god )
         god->worshippers++;
   }
   STRFREE( clan );
   STRFREE( deity );
   STRFREE( name );
   return;
}

void read_pfile( char *dirname, char *filename, bool count )
{
   FILE *fp;
   char fname[256];
   struct stat fst;
   time_t tdiff;

   now_time = time( 0 );

   snprintf( fname, 256, "%s/%s", dirname, filename );

   if( stat( fname, &fst ) != -1 )
   {
      tdiff = ( now_time - fst.st_mtime ) / 86400;

      if( ( fp = fopen( fname, "r" ) ) != NULL )
      {
         for( ;; )
         {
            char letter;
            const char *word;

            letter = fread_letter( fp );

            if( ( letter != '#' ) && ( !feof( fp ) ) )
               continue;

            word = feof( fp ) ? "End" : fread_word( fp );

            if( !str_cmp( word, "End" ) )
               break;

            if( !str_cmp( word, "PLAYER" ) )
               fread_pfile( fp, tdiff, fname, count );
            else if( !str_cmp( word, "END" ) )  /* Done     */
               break;
         }
         FCLOSE( fp );
      }
   }
   return;
}

void pfile_scan( bool count )
{
   DIR *dp;
   struct dirent *dentry;
   CLAN_DATA *clan;
   DEITY_DATA *deity;
   char directory_name[100];

   short alpha_loop;
   short cou = 0;
   deleted = 0;

   now_time = time( 0 );
   nice( 20 );

   /*
    * Reset all clans to 0 members prior to scan - Samson 7-26-00 
    */
   if( !count )
      for( clan = first_clan; clan; clan = clan->next )
         clan->members = 0;

   /*
    * Reset all deities to 0 worshippers prior to scan - Samson 7-26-00 
    */
   if( !count )
      for( deity = first_deity; deity; deity = deity->next )
         deity->worshippers = 0;

   for( alpha_loop = 0; alpha_loop <= 25; alpha_loop++ )
   {
      snprintf( directory_name, 100, "%s%c", PLAYER_DIR, 'a' + alpha_loop );
      /*
       * log_string( directory_name ); 
       */
      dp = opendir( directory_name );
      dentry = readdir( dp );
      while( dentry )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( !str_cmp( dentry->d_name, "CVS" ) )
         {
            dentry = readdir( dp );
            continue;
         }
         if( dentry->d_name[0] != '.' )
         {
            if( !count )
               read_pfile( directory_name, dentry->d_name, count );
            cou++;
         }
         dentry = readdir( dp );
      }
      closedir( dp );
   }

   if( !count )
      log_string( "Pfile cleanup completed." );
   else
      log_string( "Pfile count completed." );

   log_printf( "Total pfiles scanned: %d", cou );

   if( !count )
   {
      log_printf( "Total pfiles deleted: %d", deleted );
      log_printf( "Total pfiles remaining: %d", cou - deleted );
      num_pfiles = cou - deleted;
   }
   else
      num_pfiles = cou;

   if( !count )
   {
      for( clan = first_clan; clan; clan = clan->next )
         save_clan( clan );
      for( deity = first_deity; deity; deity = deity->next )
         save_deity( deity );
      verify_clans(  );
      prune_sales(  );
   }
   return;
}

CMDF do_pfiles( CHAR_DATA * ch, char *argument )
{
   char buf[512];

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use this command!\n\r", ch );
      return;
   }

   if( argument[0] == '\0' || !argument )
   {
      /*
       * Makes a backup copy of existing pfiles just in case - Samson 
       */
      snprintf( buf, 512, "tar -cf %spfiles.tar %s*", PLAYER_DIR, PLAYER_DIR );

      /*
       * GAH, the shell pipe won't process the command that gets pieced
       * together in the preceeding lines! God only knows why. - Samson 
       */
      system( buf );

      log_printf( "Manual pfile cleanup started by %s.", ch->name );
      pfile_scan( FALSE );
      rent_update(  );
      return;
   }

   if( !str_cmp( argument, "settime" ) )
   {
      new_pfile_time_t = current_time + 86400;
      save_timedata(  );
      send_to_char( "New cleanup time set for 24 hrs from now.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "count" ) )
   {
      log_printf( "Pfile count started by %s.", ch->name );
      pfile_scan( TRUE );
      return;
   }

   send_to_char( "Invalid argument.\n\r", ch );
   return;
}

void check_pfiles( time_t reset )
{
   /*
    * This only counts them up on reboot if the cleanup isn't needed - Samson 1-2-00 
    */
   if( reset == 255 && new_pfile_time_t > current_time )
   {
      reset = 0;  /* Call me paranoid, but it might be meaningful later on */
      log_string( "Counting pfiles....." );
      pfile_scan( TRUE );
      return;
   }

   if( new_pfile_time_t <= current_time )
   {
      if( sysdata.CLEANPFILES == TRUE )
      {
         char buf[512];

         /*
          * Makes a backup copy of existing pfiles just in case - Samson 
          */
         snprintf( buf, 512, "tar -cf %spfiles.tar %s*", PLAYER_DIR, PLAYER_DIR );

         /*
          * Would use the shell pipe for this, but alas, it requires a ch in order
          * to work, this also gets called during boot_db before the rare item
          * checks for the rent code - Samson 
          */
         system( buf );

         new_pfile_time_t = current_time + 86400;
         save_timedata(  );
         log_string( "Automated pfile cleanup beginning...." );
         pfile_scan( FALSE );
         if( reset == 0 )
            rent_update(  );
      }
      else
      {
         new_pfile_time_t = current_time + 86400;
         save_timedata(  );
         log_string( "Counting pfiles....." );
         pfile_scan( TRUE );
         if( reset == 0 )
            rent_update(  );
      }
   }
   return;
}
