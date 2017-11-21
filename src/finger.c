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
 *                        Finger and Wizinfo Module                         *
 ****************************************************************************/

/******************************************************
        Additions and changes by Edge of Acedia
              Rewritten do_finger to better
             handle info of offline players.
           E-mail: nevesfirestar2002@yahoo.com
 ******************************************************/

#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include "mud.h"
#include "calendar.h"
#include "finger.h"

/* Begin wizinfo stuff - Samson 6-6-99 */
char *const realm_string[] = {
   "Immortal", "Builder", "C0DE", "Head Coder", "Head Builder", "Implementor"
};

WIZINFO_DATA *first_wizinfo;
WIZINFO_DATA *last_wizinfo;

/* Construct wizinfo list from god dir info - Samson 6-6-99 */
void add_to_wizinfo( char *name, WIZINFO_DATA * wiz )
{
   WIZINFO_DATA *wiz_prev;

   wiz->name = str_dup( name );
   if( !wiz->email )
      wiz->email = str_dup( "Not Set" );

   for( wiz_prev = first_wizinfo; wiz_prev; wiz_prev = wiz_prev->next )
      if( strcasecmp( wiz_prev->name, name ) >= 0 )
         break;

   if( !wiz_prev )
      LINK( wiz, first_wizinfo, last_wizinfo, next, prev );
   else
      INSERT( wiz, wiz_prev, first_wizinfo, next, prev );

   return;
}

void free_wizinfo( WIZINFO_DATA * wiz )
{
   UNLINK( wiz, first_wizinfo, last_wizinfo, next, prev );
   DISPOSE( wiz->name );
   DISPOSE( wiz->email );
   DISPOSE( wiz );
   return;
}

void clear_wizinfo( void )
{
   WIZINFO_DATA *wiz, *next;

   if( !fBootDb )
   {
      for( wiz = first_wizinfo; wiz; wiz = next )
      {
         next = wiz->next;
         free_wizinfo( wiz );
      }
   }

   first_wizinfo = NULL;
   last_wizinfo = NULL;

   return;
}

void fread_info( WIZINFO_DATA * wiz, FILE * fp )
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
            KEY( "Email", wiz->email, fread_string_nohash( fp ) );
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'I':
            KEY( "ICQ", wiz->icq, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Level", wiz->level, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Realm", wiz->realm, fread_number( fp ) );
            break;
      }

      if( !fMatch )
         fread_to_eol( fp );
   }
}

void build_wizinfo( void )
{
   DIR *dp;
   struct dirent *dentry;
   FILE *fp;
   WIZINFO_DATA *wiz;
   char buf[256];

   clear_wizinfo(  );   /* Clear out the table before rebuilding a new one */

   dp = opendir( GOD_DIR );

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
         snprintf( buf, 256, "%s%s", GOD_DIR, dentry->d_name );
         fp = fopen( buf, "r" );
         if( fp )
         {
            CREATE( wiz, WIZINFO_DATA, 1 );
            fread_info( wiz, fp );
            add_to_wizinfo( dentry->d_name, wiz );
            FCLOSE( fp );
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );

   return;
}

/* 
 * Wizinfo information.
 * Added by Samson on 6-6-99
 */
CMDF do_wizinfo( CHAR_DATA * ch, char *argument )
{
   WIZINFO_DATA *wiz;

   send_to_pager( "&cContact Information for the Immortals:\n\r\n\r", ch );
   send_to_pager( "&cName         Email Address                     ICQ#       Realm\n\r", ch );
   send_to_pager( "&c------------+---------------------------------+----------+----------------\n\r", ch );

   for( wiz = first_wizinfo; wiz; wiz = wiz->next )
      ch_printf( ch, "&R%-12s &g%-33s &B%10d &P%s&D\n\r", wiz->name, wiz->email, wiz->icq, realm_string[wiz->realm] );
   return;
}

/* End wizinfo stuff - Samson 6-6-99 */

/* Finger snippet courtesy of unknown author. Installed by Samson 4-6-98 */
/* File read/write code redone using standard Smaug I/O routines - Samson 9-12-98 */
/* Data gathering now done via the pfiles, eliminated separate finger files - Samson 12-21-98 */
/* Improvements for offline players by Edge of Acedia 8-26-03 */
/* Further refined by Samson on 8-26-03 */
CMDF do_finger( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim = NULL;
   CMDTYPE *command;
   ROOM_INDEX_DATA *temproom, *original = NULL;
   int level = LEVEL_IMMORTAL;
   char buf[MIL], fingload[256];
   char *suf, *laston = NULL;
   struct stat fst;
   short day = 0;
   bool loaded = FALSE, skip = FALSE;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs can't use the finger command.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Finger whom?\n\r", ch );
      return;
   }

   snprintf( buf, MIL, "0.%s", argument );

   /*
    * If player is online, check for fingerability (yeah, I coined that one)  -Edge 
    */
   if( ( victim = get_char_world( ch, buf ) ) != NULL )
   {
      if( IS_PCFLAG( victim, PCFLAG_PRIVACY ) && !IS_IMMORTAL( ch ) )
      {
         ch_printf( ch, "%s has privacy enabled.\n\r", victim->name );
         return;
      }

      if( IS_IMMORTAL( victim ) && !IS_IMMORTAL( ch ) )
      {
         send_to_char( "You cannot finger an immortal.\n\r", ch );
         return;
      }
   }

   /*
    * Check for offline players - Edge 
    */
   else
   {
      DESCRIPTOR_DATA *d;

      snprintf( fingload, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ) );
      /*
       * Bug fix here provided by Senir to stop /dev/null crash 
       */
      if( stat( fingload, &fst ) == -1 || !check_parse_name( capitalize( argument ), FALSE ) )
      {
         ch_printf( ch, "&YNo such player named '%s'.\n\r", argument );
         return;
      }

      laston = ctime( &fst.st_mtime );
      temproom = get_room_index( ROOM_VNUM_LIMBO );
      if( !temproom )
      {
         bug( "%s", "do_finger: Limbo room is not available!" );
         send_to_char( "Fatal error, report to the immortals.\n\r", ch );
         return;
      }

      CREATE( d, DESCRIPTOR_DATA, 1 );
      d->next = NULL;
      d->prev = NULL;
      d->connected = CON_PLOADED;
      d->outsize = 2000;
      CREATE( d->outbuf, char, d->outsize );
      argument[0] = UPPER( argument[0] );

      loaded = load_char_obj( d, argument, FALSE, FALSE );  /* Remove second FALSE if compiler complains */
      LINK( d->character, first_char, last_char, next, prev );
      original = d->character->in_room;
      if( !char_to_room( d->character, temproom ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      victim = d->character;  /* Hopefully this will work, if not, we're SOL */
      d->character->desc = NULL;
      d->character = NULL;
      DISPOSE( d->outbuf );
      DISPOSE( d );

      if( IS_PCFLAG( victim, PCFLAG_PRIVACY ) && !IS_IMMORTAL( ch ) )
      {
         ch_printf( ch, "%s has privacy enabled.\n\r", victim->name );
         skip = TRUE;
      }

      if( IS_IMMORTAL( victim ) && !IS_IMMORTAL( ch ) )
      {
         send_to_char( "You cannot finger an immortal.\n\r", ch );
         skip = TRUE;
      }
      loaded = TRUE;
   }

   if( !skip )
   {
      day = victim->pcdata->day + 1;

      if( day > 4 && day < 20 )
         suf = "th";
      else if( day % 10 == 1 )
         suf = "st";
      else if( day % 10 == 2 )
         suf = "nd";
      else if( day % 10 == 3 )
         suf = "rd";
      else
         suf = "th";

      send_to_char( "&w          Finger Info\n\r", ch );
      send_to_char( "          -----------\n\r", ch );
      ch_printf( ch, "&wName    : &G%-20s &wMUD Age: &G%d\n\r", victim->name, get_age( victim ) );
      ch_printf( ch, "&wBirthday: &GDay of %s, %d%s day in the Month of %s, in the year %d.\n\r",
                 day_name[victim->pcdata->day % 13], day, suf, month_name[victim->pcdata->month], victim->pcdata->year );
      ch_printf( ch, "&wLevel   : &G%-20d &w  Class: &G%s\n\r", victim->level, capitalize( get_class( victim ) ) );
      ch_printf( ch, "&wSex     : &G%-20s &w  Race : &G%s\n\r", npc_sex[victim->sex], capitalize( get_race( victim ) ) );
      ch_printf( ch, "&wTitle   :&G%s\n\r", victim->pcdata->title );
      ch_printf( ch, "&wHomepage: &G%s\n\r",
                 victim->pcdata->homepage != NULL ? show_tilde( victim->pcdata->homepage ) : "Not specified" );
      ch_printf( ch, "&wEmail   : &G%s\n\r", victim->pcdata->email != NULL ? victim->pcdata->email : "Not specified" );
      ch_printf( ch, "&wICQ#    : &G%d\n\r", victim->pcdata->icq );
      if( !loaded )
         ch_printf( ch, "&wLast on : &G%s\n\r", c_time( victim->pcdata->logon, ch->pcdata->timezone ) );
      else
         ch_printf( ch, "&wLast on : &G%s\n\r", laston );
      if( IS_IMMORTAL( ch ) )
      {
         send_to_char( "&wImmortal Information\n\r", ch );
         send_to_char( "--------------------\n\r", ch );
         ch_printf( ch, "&wIP Info       : &G%s\n\r", victim->pcdata->lasthost );
         ch_printf( ch, "&wTime played   : &G%ld hours\n\r", ( long int )GET_TIME_PLAYED( victim ) );
         ch_printf( ch, "&wAuthorized by : &G%s\n\r",
                    victim->pcdata->authed_by ? victim->pcdata->authed_by : ( sysdata.
                                                                              WAIT_FOR_AUTH ? "Not Authed" : "The Code" ) );
         ch_printf( ch, "&wPrivacy Status: &G%s\n\r", IS_PCFLAG( victim, PCFLAG_PRIVACY ) ? "Enabled" : "Disabled" );
         if( victim->level < ch->level )
         {
            /*
             * Added by Tarl 19 Dec 02 to remove Huh? when ch not high enough to view comments. 
             */
            command = find_command( "comment" );
            if( !command )
               level = LEVEL_IMMORTAL;
            else
               level = command->level;
            if( ch->level >= command->level )
               cmdf( ch, "comment list %s", victim->name );
         }
      }
      ch_printf( ch, "&wBio:\n\r&G%s\n\r", victim->pcdata->bio ? victim->pcdata->bio : "Not created" );
   }

   if( loaded )
   {
      int x, y;

      char_from_room( victim );
      if( !char_to_room( victim, original ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      quitting_char = victim;

      if( sysdata.save_pets )
      {
         CHAR_DATA *pet;

         while( ( pet = victim->last_pet ) != NULL )
            extract_char( pet, TRUE );
      }
      saving_char = NULL;

      /*
       * After extract_char the ch is no longer valid!
       */
      extract_char( victim, TRUE );
      for( x = 0; x < MAX_WEAR; x++ )
         for( y = 0; y < MAX_LAYERS; y++ )
            save_equipment[x][y] = NULL;
   }
   return;
}

/* Added a clone of homepage to let players input their email addy - Samson 4-18-98 */
CMDF do_email( CHAR_DATA * ch, char *argument )
{
   char buf[75];

   if( IS_NPC( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      if( ch->pcdata->email && ch->pcdata->email[0] != '\0' )
         ch_printf( ch, "Your email address is: %s\n\r", show_tilde( ch->pcdata->email ) );
      else
         send_to_char( "You have no email address set yet.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      DISPOSE( ch->pcdata->email );

      if( IS_IMMORTAL( ch ) );
      {
         save_char_obj( ch );
         build_wizinfo(  );
      }

      send_to_char( "Email address cleared.\n\r", ch );
      return;
   }

   mudstrlcpy( buf, argument, 75 );

   smash_tilde( buf );
   DISPOSE( ch->pcdata->email );
   ch->pcdata->email = str_dup( buf );
   if( IS_IMMORTAL( ch ) );
   {
      save_char_obj( ch );
      build_wizinfo(  );
   }
   send_to_char( "Email address set.\n\r", ch );
}

CMDF do_icq_number( CHAR_DATA * ch, char *argument )
{
   int icq;

   if( IS_NPC( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      ch_printf( ch, "Your ICQ# is: %d\n\r", ch->pcdata->icq );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      ch->pcdata->icq = 0;

      if( IS_IMMORTAL( ch ) );
      {
         save_char_obj( ch );
         build_wizinfo(  );
      }

      send_to_char( "ICQ# cleared.\n\r", ch );
      return;
   }

   if( !is_number( argument ) )
   {
      send_to_char( "You must enter numeric data.\n\r", ch );
      return;
   }

   icq = atoi( argument );

   if( icq < 1 )
   {
      send_to_char( "Valid range is greater than 0.\n\r", ch );
      return;
   }

   ch->pcdata->icq = icq;

   if( IS_IMMORTAL( ch ) );
   {
      save_char_obj( ch );
      build_wizinfo(  );
   }

   send_to_char( "ICQ# set.\n\r", ch );
   return;
}

CMDF do_homepage( CHAR_DATA * ch, char *argument )
{
   char buf[75];

   if( IS_NPC( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      if( ch->pcdata->homepage && ch->pcdata->homepage != '\0' )
         ch_printf( ch, "Your homepage is: %s\n\r", show_tilde( ch->pcdata->homepage ) );
      else
         send_to_char( "You have no homepage set yet.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      DISPOSE( ch->pcdata->homepage );
      send_to_char( "Homepage cleared.\n\r", ch );
      return;
   }

   if( strstr( argument, "://" ) )
      mudstrlcpy( buf, argument, 75 );
   else
      snprintf( buf, 75, "http://%s", argument );

   hide_tilde( buf );
   DISPOSE( ch->pcdata->homepage );
   ch->pcdata->homepage = str_dup( buf );
   send_to_char( "Homepage set.\n\r", ch );
}

CMDF do_privacy( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs can't use the privacy toggle.\n\r", ch );
      return;
   }

   TOGGLE_BIT( ch->pcdata->flags, PCFLAG_PRIVACY );

   if( IS_PCFLAG( ch, PCFLAG_PRIVACY ) )
   {
      send_to_char( "Privacy flag enabled.\n\r", ch );
      return;
   }
   else
   {
      send_to_char( "Privacy flag disabled.\n\r", ch );
      return;
   }
}
