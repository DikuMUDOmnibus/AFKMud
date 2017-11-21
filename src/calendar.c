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
 *                      Calendar Handler/Seasonal Updates                   *
 ****************************************************************************/

#include "mud.h"
#include "calendar.h"
#include "pfiles.h"

HOLIDAY_DATA *first_holiday;
HOLIDAY_DATA *last_holiday;

#define MAX_TZONE   25

extern time_t board_expire_time_t;

struct tzone_type
{
   char *name; /* Name of the time zone */
   char *zone; /* Cities or Zones in zone crossing */
   int gmt_offset;   /* Difference in hours from Greenwich Mean Time */
   int dst_offset;   /* Day Light Savings Time offset, Not used but left it in anyway */
};

struct tzone_type tzone_table[MAX_TZONE] = {
   {"GMT-12", "Eniwetok", -12, 0},
   {"GMT-11", "Samoa", -11, 0},
   {"GMT-10", "Hawaii", -10, 0},
   {"GMT-9", "Alaska", -9, 0},
   {"GMT-8", "Pacific US", -8, -7},
   {"GMT-7", "Mountain US", -7, -6},
   {"GMT-6", "Central US", -6, -5},
   {"GMT-5", "Eastern US", -5, -4},
   {"GMT-4", "Atlantic, Canada", -4, 0},
   {"GMT-3", "Brazilia, Buenos Aries", -3, 0},
   {"GMT-2", "Mid-Atlantic", -2, 0},
   {"GMT-1", "Cape Verdes", -1, 0},
   {"GMT", "Greenwich Mean Time, Greenwich", 0, 0},
   {"GMT+1", "Berlin, Rome", 1, 0},
   {"GMT+2", "Israel, Cairo", 2, 0},
   {"GMT+3", "Moscow, Kuwait", 3, 0},
   {"GMT+4", "Abu Dhabi, Muscat", 4, 0},
   {"GMT+5", "Islamabad, Karachi", 5, 0},
   {"GMT+6", "Almaty, Dhaka", 6, 0},
   {"GMT+7", "Bangkok, Jakarta", 7, 0},
   {"GMT+8", "Hong Kong, Beijing", 8, 0},
   {"GMT+9", "Tokyo, Osaka", 9, 0},
   {"GMT+10", "Sydney, Melbourne, Guam", 10, 0},
   {"GMT+11", "Magadan, Soloman Is.", 11, 0},
   {"GMT+12", "Fiji, Wellington, Auckland", 12, 0},
};

int tzone_lookup( const char *arg )
{
   int i;

   for( i = 0; i < MAX_TZONE; i++ )
   {
      if( !str_cmp( arg, tzone_table[i].name ) )
         return i;
   }

   for( i = 0; i < MAX_TZONE; i++ )
   {
      if( is_name( arg, tzone_table[i].zone ) )
         return i;
   }
   return -1;
}

CMDF do_timezone( CHAR_DATA * ch, char *argument )
{
   int i;

   if( IS_NPC( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      ch_printf( ch, "%-6s %-30s (%s)\n\r", "Name", "City/Zone Crosses", "Time" );
      send_to_char( "-------------------------------------------------------------------------\n\r", ch );
      for( i = 0; i < MAX_TZONE; i++ )
      {
         ch_printf( ch, "%-6s %-30s (%s)\n\r", tzone_table[i].name, tzone_table[i].zone, c_time( current_time, i ) );
      }
      send_to_char( "-------------------------------------------------------------------------\n\r", ch );
      return;
   }

   i = tzone_lookup( argument );

   if( i == -1 )
   {
      send_to_char( "That time zone does not exists. Make sure to use the exact name.\n\r", ch );
      return;
   }

   ch->pcdata->timezone = i;
   ch_printf( ch, "Your time zone is now %s %s (%s)\n\r", tzone_table[i].name,
              tzone_table[i].zone, c_time( current_time, i ) );
}

/* Ever so slightly modified version of "friendly_ctime" provided by Aurora.
 * Merged with the Timezone snippet by Ryan Jennings (Markanth) r-jenn@shaw.ca
 */
char *c_time( time_t curtime, int tz )
{
   static char *day[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
   static char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
   static char strtime[128];
   struct tm *ptime;
   char tzonename[50];

   if( curtime <= 0 )
      curtime = current_time;

   if( tz > -1 && tz < MAX_TZONE )
   {
      mudstrlcpy( tzonename, tzone_table[tz].zone, 50 );
#if defined(__CYGWIN__)
      curtime += ( time_t ) timezone;
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
      /*
       * Hopefully this works 
       */
      ptime = localtime( &curtime );
      curtime += ptime->tm_gmtoff;
#else
      curtime += timezone; /* timezone external variable in time.h holds the 
                            * difference in seconds to GMT. */
#endif
      curtime += ( 60 * 60 * tzone_table[tz].gmt_offset );  /* Add the offset hours */
   }
   ptime = localtime( &curtime );
   if( tz < 0 || tz >= MAX_TZONE )
#if defined(__CYGWIN__)
      mudstrlcpy( tzonename, tzname[ptime->tm_isdst], 50 );
#else
      mudstrlcpy( tzonename, ptime->tm_zone, 50 );
#endif

   snprintf( strtime, 128, "%3s %3s %d, %d %d:%02d:%02d %cM %s",
             day[ptime->tm_wday], month[ptime->tm_mon], ptime->tm_mday, ptime->tm_year + 1900,
             ptime->tm_hour == 0 ? 12 : ptime->tm_hour > 12 ? ptime->tm_hour - 12 : ptime->tm_hour,
             ptime->tm_min, ptime->tm_sec, ptime->tm_hour < 12 ? 'A' : 'P', tzonename );
   return strtime;
}

/* timeZone is not shown as it's a bit .. long.. but it is respected -- Xorith */
char *mini_c_time( time_t curtime, int tz )
{
   static char strtime[128];
   struct tm *ptime;
   char tzonename[50];

   if( curtime <= 0 )
      curtime = current_time;

   if( tz > -1 && tz < MAX_TZONE )
   {
      mudstrlcpy( tzonename, tzone_table[tz].zone, 50 );
#if defined(__CYGWIN__)
      curtime += ( time_t ) timezone;
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
      /*
       * Hopefully this works 
       */
      ptime = localtime( &curtime );
      curtime += ptime->tm_gmtoff;
#else
      curtime += timezone; /* timezone external variable in time.h holds the
                            * difference in seconds to GMT. */
#endif
      curtime += ( 60 * 60 * tzone_table[tz].gmt_offset );  /* Add the offset hours */
   }
   ptime = localtime( &curtime );
   if( tz < 0 || tz >= MAX_TZONE )
#if defined(__CYGWIN__)
      mudstrlcpy( tzonename, tzname[ptime->tm_isdst], 50 );
#else
      mudstrlcpy( tzonename, ptime->tm_zone, 50 );
#endif

   snprintf( strtime, 128, "%02d/%02d/%02d %02d:%02d%c", ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_year - 100,
             ptime->tm_hour == 0 ? 12 : ptime->tm_hour > 12 ? ptime->tm_hour - 12 : ptime->tm_hour, ptime->tm_min,
             ptime->tm_hour < 12 ? 'A' : 'P' );
   return strtime;
}

/* Time values modified to Alsherok calendar - Samson 5-6-99 */
char *const day_name[] = {
   "Agoras", "Archonis", "Ecclesias", "Talentas", "Pentes",
   "Kilinis", "Piloses", "Koses", "Demes", "Camillies", "Eluthes",
   "Aemiles", "Xenophes"
};

char *const month_name[] = {
   "Olympias", "Planting", "Agamemnus", "Athenas", "Pentes", "Sextes", "Harvest", "Octes",
   "Nones", "Deces", "Graecias", "Terminus"
};

char *const season_name[] = {
   "spring", "summer", "fall", "winter"
};

/* Calling function must insure tstr buffer is large enough.
 * Returns the address of the buffer passed, allowing things like
 * this printf example: 123456secs = 1day 10hrs 17mins 36secs
 *   time_t tsecs = 123456;
 *   char   buff[ MSL ];
 *
 *   printf( "Duration is %s\n", duration( tsecs, buff ) );
 */

#define  DUR_SCMN       ( 60 )
#define  DUR_MNHR       ( 60 )
#define  DUR_HRDY       ( 24 )
#define  DUR_DYWK       (  7 )
#define  DUR_ADDS( t )  ( (t) == 1 ? '\0' : 's' )

char *sec_to_hms( time_t loctime, char *tstr )
{
   time_t t_rem;
   int sc, mn, hr, dy, wk;
   int sflg = 0;
   char buff[MSL];

   if( loctime < 1 )
   {
      mudstrlcat( tstr, "no time at all", MSL );
      return ( tstr );
   }

   sc = loctime % DUR_SCMN;
   t_rem = loctime - sc;

   if( t_rem > 0 )
   {
      t_rem /= DUR_SCMN;
      mn = t_rem % DUR_MNHR;
      t_rem -= mn;

      if( t_rem > 0 )
      {
         t_rem /= DUR_MNHR;
         hr = t_rem % DUR_HRDY;
         t_rem -= hr;

         if( t_rem > 0 )
         {
            t_rem /= DUR_HRDY;
            dy = t_rem % DUR_DYWK;
            t_rem -= dy;

            if( t_rem > 0 )
            {
               wk = t_rem / DUR_DYWK;

               if( wk )
               {
                  sflg = 1;
                  snprintf( buff, MSL, "%d week%c", wk, DUR_ADDS( wk ) );
                  mudstrlcat( tstr, buff, MSL );
               }
            }
            if( dy )
            {
               if( sflg == 1 )
                  mudstrlcat( tstr, " ", MSL );
               sflg = 1;
               snprintf( buff, MSL, "%d day%c", dy, DUR_ADDS( dy ) );
               mudstrlcat( tstr, buff, MSL );
            }
         }
         if( hr )
         {
            if( sflg == 1 )
               mudstrlcat( tstr, " ", MSL );
            sflg = 1;
            snprintf( buff, MSL, "%d hour%c", hr, DUR_ADDS( hr ) );
            mudstrlcat( tstr, buff, MSL );
         }
      }
      if( mn )
      {
         if( sflg == 1 )
            mudstrlcat( tstr, " ", MSL );
         sflg = 1;
         snprintf( buff, MSL, "%d minute%c", mn, DUR_ADDS( mn ) );
         mudstrlcat( tstr, buff, MSL );
      }
   }
   if( sc )
   {
      if( sflg == 1 )
         mudstrlcat( tstr, " ", MSL );
      snprintf( buff, MSL, "%d second%c", sc, DUR_ADDS( sc ) );
      mudstrlcat( tstr, buff, MSL );
   }
   return ( tstr );
}

HOLIDAY_DATA *get_holiday( short month, short day )
{
   HOLIDAY_DATA *holiday;
   for( holiday = first_holiday; holiday; holiday = holiday->next )
      if( month + 1 == holiday->month && day + 1 == holiday->day )
         return holiday;
   return NULL;
}

/* Reads the actual time file from disk - Samson 1-21-99 */
void fread_timedata( FILE * fp )
{
   const char *word;
   bool fMatch = FALSE;

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

         case 'B':
            KEY( "Boardtime", board_expire_time_t, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'M':
            KEY( "Mhour", time_info.hour, fread_number( fp ) );
            KEY( "Mday", time_info.day, fread_number( fp ) );
            KEY( "Mmonth", time_info.month, fread_number( fp ) );
            KEY( "Myear", time_info.year, fread_number( fp ) );
            break;
         case 'P':
            KEY( "Purgetime", new_pfile_time_t, fread_number( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "Fread_timedata: no match: %s", word );
         fread_to_eol( fp );
      }
   }
}

/* Load time information from saved file - Samson 1-21-99 */
bool load_timedata( void )
{
   char filename[256];
   FILE *fp;
   bool found;

   found = FALSE;
   snprintf( filename, 256, "%stime.dat", SYSTEM_DIR );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {

      found = TRUE;
      for( ;; )
      {
         char letter = '\0';
         char *word = NULL;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "%s", "Load_timedata: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "TIME" ) )
         {
            fread_timedata( fp );
            break;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "Load_timedata: bad section - %s.", word );
            break;
         }
      }
      FCLOSE( fp );
   }

   return found;
}

/* Saves the current game world time to disk - Samson 1-21-99 */
void save_timedata( void )
{
   FILE *fp;
   char filename[256];

   snprintf( filename, 256, "%stime.dat", SYSTEM_DIR );

   if( ( fp = fopen( filename, "w" ) ) == NULL )
   {
      bug( "%s", "save_timedata: fopen" );
      perror( filename );
   }
   else
   {
      fprintf( fp, "%s", "#TIME\n" );
      fprintf( fp, "Mhour	%d\n", time_info.hour );
      fprintf( fp, "Mday	%d\n", time_info.day );
      fprintf( fp, "Mmonth	%d\n", time_info.month );
      fprintf( fp, "Myear	%d\n", time_info.year );
      fprintf( fp, "Purgetime %ld\n", ( long int )new_pfile_time_t );
      fprintf( fp, "Boardtime %ld\n", ( long int )board_expire_time_t );
      fprintf( fp, "%s", "End\n\n" );
      fprintf( fp, "%s", "#END\n" );
   }
   FCLOSE( fp );
   return;
}

CMDF do_time( CHAR_DATA * ch, char *argument )
{
   HOLIDAY_DATA *holiday;
   char buf[MSL];
   char *suf;
   short day;

   day = time_info.day + 1;

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

   ch_printf( ch, "&YIt is %d o'clock %s, Day of %s, %d%s day in the Month of %s.\n\r"
              "It is the %s season, in the year %d.\n\r"
              "The mud started up at:    %s\n\r"
              "The system time      : %s\n\r",
              ( time_info.hour % sysdata.hournoon == 0 ) ? sysdata.hournoon : time_info.hour % sysdata.hournoon,
              time_info.hour >= sysdata.hournoon ? "pm" : "am", day_name[( time_info.day ) % sysdata.daysperweek], day, suf,
              month_name[time_info.month], season_name[time_info.season], time_info.year, str_boot_time,
              c_time( current_time, -1 ) );

   ch_printf( ch, "Your local time          : %s\n\r\n\r", c_time( current_time, ch->pcdata->timezone ) );
   holiday = get_holiday( time_info.month, day - 1 );

   if( holiday != NULL )
      ch_printf( ch, "It's a holiday today: %s\n\r", holiday->name );

   if( !IS_NPC( ch ) )
   {
      if( day == ch->pcdata->day + 1 && time_info.month == ch->pcdata->month )
         send_to_char( "Today is your birthday!\n\r", ch );
   }

   if( IS_IMMORTAL( ch ) && sysdata.CLEANPFILES == TRUE )
   {
      long ptime, curtime;

      ptime = ( long int )( new_pfile_time_t );
      curtime = ( long int )( current_time );

      buf[0] = '\0';
      sec_to_hms( ptime - curtime, buf );
      ch_printf( ch, "The next pfile cleanup is in %s.\n\r", buf );
   }
   if( IS_IMMORTAL( ch ) && sysdata.RENT == TRUE )
   {
      long qtime, dtime;

      qtime = ( long int )( new_pfile_time_t );
      dtime = ( long int )( current_time );

      buf[0] = '\0';
      sec_to_hms( qtime - dtime, buf );
      ch_printf( ch, "The next rent update is in %s.\n\r", buf );
   }

   return;
}

void start_winter( void )
{
   ROOM_INDEX_DATA *room;
   int iHash;

   echo_to_all( AT_SOCIAL, "The air takes on a chilling cold as winter sets in.", ECHOTAR_ALL );
   echo_to_all( AT_SOCIAL, "Freshwater bodies everywhere have frozen over.\n\r", ECHOTAR_ALL );

   winter_freeze = TRUE;

   for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
   {
      for( room = room_index_hash[iHash]; room; room = room->next )
      {
         switch ( room->sector_type )
         {
            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM:
               room->winter_sector = room->sector_type;
               room->sector_type = SECT_ICE;
               break;
         }
      }
   }
   return;
}

void start_spring( void )
{
   ROOM_INDEX_DATA *room;
   int iHash;

   echo_to_all( AT_SOCIAL, "The chill recedes from the air as spring begins to take hold.", ECHOTAR_ALL );
   echo_to_all( AT_SOCIAL, "Freshwater bodies everywhere have thawed out.\n\r", ECHOTAR_ALL );

   winter_freeze = FALSE;

   for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
   {
      for( room = room_index_hash[iHash]; room; room = room->next )
      {
         if( room->sector_type == SECT_ICE && room->winter_sector != -1 )
         {
            room->sector_type = room->winter_sector;
            room->winter_sector = -1;
         }
      }
   }
   return;
}

void start_summer( void )
{
   echo_to_all( AT_SOCIAL, "The days grow longer and hotter as summer grips the world.\n\r", ECHOTAR_ALL );
   return;
}

void start_fall( void )
{
   echo_to_all( AT_SOCIAL, "The leaves begin changing colors signaling the start of fall.\n\r", ECHOTAR_ALL );
   return;
}

void season_update( void )
{
   HOLIDAY_DATA *day;

   day = get_holiday( time_info.month, time_info.day );

   if( day != NULL )
   {
      if( time_info.day + 1 == day->day && time_info.hour == 0 )
      {
         echo_to_all( AT_IMMORT, day->announce, ECHOTAR_ALL );
      }
   }

   if( time_info.season == SEASON_WINTER && winter_freeze == FALSE )
   {
      ROOM_INDEX_DATA *room;
      int iHash;

      winter_freeze = TRUE;

      for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
      {
         for( room = room_index_hash[iHash]; room; room = room->next )
         {
            switch ( room->sector_type )
            {
               case SECT_WATER_SWIM:
               case SECT_WATER_NOSWIM:
                  room->winter_sector = room->sector_type;
                  room->sector_type = SECT_ICE;
                  break;
            }
         }
      }
   }
   return;
}

/* PaB: Which season are we in? 
 * Notes: Simply figures out which season the current month falls into
 * and returns a proper value.
 */
void calc_season( void )
{
   /*
    * How far along in the year are we, measured in days? 
    */
   /*
    * PaB: Am doing this in days to minimize roundoff impact 
    */
   int day = time_info.month * sysdata.dayspermonth + time_info.day;

   if( day < ( sysdata.daysperyear / 4 ) )
   {
      time_info.season = SEASON_SPRING;
      if( time_info.hour == 0 && day == 0 )
         start_spring(  );
   }
   else if( day < ( sysdata.daysperyear / 4 ) * 2 )
   {
      time_info.season = SEASON_SUMMER;
      if( time_info.hour == 0 && day == ( sysdata.daysperyear / 4 ) )
         start_summer(  );
   }
   else if( day < ( sysdata.daysperyear / 4 ) * 3 )
   {
      time_info.season = SEASON_FALL;
      if( time_info.hour == 0 && day == ( ( sysdata.daysperyear / 4 ) * 2 ) )
         start_fall(  );
   }
   else if( day < sysdata.daysperyear )
   {
      time_info.season = SEASON_WINTER;
      if( time_info.hour == 0 && day == ( ( sysdata.daysperyear / 4 ) * 3 ) )
         start_winter(  );
   }
   else
   {
      time_info.season = SEASON_SPRING;
   }

   season_update(  );   /* Maintain the season in case of reboot, check for holidays */

   return;
}

void free_holiday( HOLIDAY_DATA * day )
{
   UNLINK( day, first_holiday, last_holiday, next, prev );
   DISPOSE( day->announce );
   DISPOSE( day->name );
   DISPOSE( day );
   return;
}

void free_holidays( void )
{
   HOLIDAY_DATA *day, *day_next;

   for( day = first_holiday; day; day = day_next )
   {
      day_next = day->next;
      free_holiday( day );
   }
   return;
}

CMDF do_holidays( CHAR_DATA * ch, char *argument )
{
   HOLIDAY_DATA *day;

   send_to_pager( "&RHoliday		       &YMonth	        &GDay\n\r", ch );
   send_to_pager( "&g----------------------+----------------+---------------\n\r", ch );

   for( day = first_holiday; day; day = day->next )
      pager_printf( ch, "&G%-21s	&g%-11s	%-2d\n\r", day->name, month_name[day->month - 1], day->day );

   return;
}

/* Read in an individual holiday */
void fread_day( HOLIDAY_DATA * day, FILE * fp )
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

         case 'A':
            KEY( "Announce", day->announce, fread_string_nohash( fp ) );
            break;

         case 'D':
            KEY( "Day", day->day, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !day->announce )
                  day->announce = str_dup( "Today is a holiday, but who the hell knows which one." );
               return;
            }
            break;

         case 'M':
            KEY( "Month", day->month, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", day->name, fread_string_nohash( fp ) );
            break;
      }

      if( !fMatch )
         bug( "fread_day: no match: %s", word );
   }
}

/* Load the holiday file */
void load_holidays( void )
{
   char filename[256];
   HOLIDAY_DATA *day;
   FILE *fp;
   short daycount;

   first_holiday = NULL;
   last_holiday = NULL;

   snprintf( filename, 256, "%s%s", SYSTEM_DIR, HOLIDAY_FILE );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
      daycount = 0;
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
            bug( "%s", "load_holidays: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "HOLIDAY" ) )
         {
            if( daycount >= sysdata.maxholiday )
            {
               bug( "load_holidays: more holidays than %d, increase Max Holiday in cset.", sysdata.maxholiday );
               FCLOSE( fp );
               return;
            }
            CREATE( day, HOLIDAY_DATA, 1 );
            fread_day( day, fp );
            daycount++;
            LINK( day, first_holiday, last_holiday, next, prev );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "load_holidays: bad section: %s.", word );
            continue;
         }
      }
      FCLOSE( fp );
   }

   return;
}

/* Save the holidays to disk - Samson 5-6-99 */
void save_holidays( void )
{
   HOLIDAY_DATA *day;
   FILE *fp;
   char filename[256];

   snprintf( filename, 256, "%s%s", SYSTEM_DIR, HOLIDAY_FILE );

   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s", "save_holidays: fopen" );
      perror( filename );
   }
   else
   {
      for( day = first_holiday; day; day = day->next )
      {
         fprintf( fp, "%s", "#HOLIDAY\n" );
         fprintf( fp, "Name		%s~\n", day->name );
         fprintf( fp, "Announce	%s~\n", day->announce );
         fprintf( fp, "Month		%d\n", day->month );
         fprintf( fp, "Day		%d\n", day->day );
         fprintf( fp, "%s", "End\n\n" );
      }
      fprintf( fp, "%s", "#END\n" );
      FCLOSE( fp );
   }
   return;
}

CMDF do_saveholiday( CHAR_DATA * ch, char *argument )
{
   save_holidays(  );
   send_to_char( "Holiday chart saved.\n\r", ch );
   return;
}
