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
 *                       Database management module                         *
 ****************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <math.h> /* For fread_float */
#include <dlfcn.h>   /* Required for libdl - Trax */
#if !defined(__CYGWIN__) && !defined(__FreeBSD__)
#include <execinfo.h>
#endif
#include "mud.h"
#include "auction.h"
#include "bits.h"
#include "connhist.h"
#include "event.h"
#include "help.h"
#include "mud_prog.h"
#include "overland.h"
#include "shops.h"

typedef struct wizent WIZENT;

/*
 * Structure used to build wizlist
 */
struct wizent
{
   WIZENT *next;
   WIZENT *last;
   char *name;
   char *http;
   short level;
};

void init_supermob( void );

/*
 * Globals.
 */
WIZENT *first_wiz;
WIZENT *last_wiz;

extern OBJ_DATA *extracted_obj_queue;
extern struct extracted_char_data *extracted_char_queue;

time_t last_restore_all_time = 0;

TELEPORT_DATA *first_teleport;
TELEPORT_DATA *last_teleport;

CHAR_DATA *first_char;
CHAR_DATA *last_char;

OBJ_DATA *first_object;
OBJ_DATA *last_object;
TIME_INFO_DATA time_info;
extern REL_DATA *first_relation;
extern REL_DATA *last_relation;
extern char *alarm_section;

int weath_unit;   /* global weather param */
int rand_factor;
int climate_factor;
int neigh_factor;
int max_vector;
int mobs_deleted = 0;
int cur_qobjs;
int cur_qchars;
int nummobsloaded;
int numobjsloaded;
int physicalobjects;
int last_pkroom;

AUCTION_DATA *auction;  /* auctions */
OBJ_DATA *supermob_obj;

/*
 * Locals.
 */
MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];

AREA_DATA *first_area;
AREA_DATA *last_area;
AREA_DATA *first_area_nsort;
AREA_DATA *last_area_nsort;
AREA_DATA *first_area_vsort;
AREA_DATA *last_area_vsort;

SYSTEM_DATA sysdata;

int top_affect;
int top_area;
int top_ed;
int top_exit;
int top_mob_index;
int top_obj_index;
int top_reset;
int top_room;
int top_shop;
int top_repair;
extern int top_help;

/*
 * Semi-locals.
 */
bool fBootDb;
FILE *fpArea;
char strArea[MIL];

extern int astral_target;

/*
 * External booting function
 */
void set_alarm( long seconds );
#ifdef MULTIPORT
void load_shellcommands( void );
#endif
void load_area_file( AREA_DATA * tarea, char *filename, bool isproto );
int set_obj_rent( OBJ_INDEX_DATA * obj );
void armorgen( OBJ_DATA * obj );
bool load_timedata( void );
void load_shopkeepers( void );
void load_auth_list( void );  /* New Auth Code */
void save_auth_list( void );
void build_wizinfo( void );
void load_maps( void ); /* Load in Overland Maps - Samson 8-1-99 */
void load_ships( void );   /* Load ships - Samson 1-6-01 */
void load_world( CHAR_DATA * ch );
void load_morphs( void );
void load_skill_table( void );
void load_mxpobj_cmds( void );
void load_quotes( void );
void load_sales( void );   /* Samson 6-24-99 for new auction system */
void load_aucvaults( void );  /* Samson 6-20-99 for new auction system */
void load_corpses( void );
void renumber_put_resets( ROOM_INDEX_DATA * room );
void load_banlist( void );
void update_timers( void );
void update_calendar( void );
void load_specfuns( void );
void free_aliases( CHAR_DATA * ch );
#ifdef I3
void free_i3chardata( CHAR_DATA * ch );
#endif
#ifdef IMC
void imc_freechardata( CHAR_DATA * ch );
#endif
void free_zonedata( CHAR_DATA * ch );
void free_pcboards( CHAR_DATA * ch );
void load_equipment_totals( bool fCopyOver );
void load_slays( void );
void load_holidays( void );
void load_bits( void );
void load_liquids( void );
void load_mixtures( void );
void load_imm_host( void );
void load_dns( void );
void load_mudchannels( void );
void to_channel( const char *argument, char *xchannel, int level );
void load_runes( void );
void load_clans( void );
void load_councils( void );
void load_socials( void );
void load_commands( void );
void load_deity( void );
void free_comments( CHAR_DATA * ch );
void load_boards( void );
void load_projects( void );
void free_note( struct note_data *pnote );
void load_watchlist( void );
void free_fight( CHAR_DATA * ch );
void assign_gsn_data( void );
int get_continent( char *continent );
int mob_xp( CHAR_DATA * mob );
void free_ignores( CHAR_DATA * ch );
void load_connhistory( void );
void wipe_resets( ROOM_INDEX_DATA * room );

void shutdown_mud( char *reason )
{
   FILE *fp;

   if( ( fp = fopen( SHUTDOWN_FILE, "a" ) ) != NULL )
   {
      fprintf( fp, "%s\n", reason );
      FCLOSE( fp );
   }
}

bool exists_file( char *name )
{
   struct stat fst;

   /*
    * Stands to reason that if there ain't a name to look at, it damn well don't exist! 
    */
   if( !name || name[0] == '\0' || !str_cmp( name, "" ) )
      return FALSE;

   if( stat( name, &fst ) != -1 )
      return TRUE;
   else
      return FALSE;
}

bool is_valid_filename( CHAR_DATA *ch, const char *direct, const char *filename )
{
   char newfilename[256];
   struct stat fst;

   /* Length restrictions */
   if( !filename || filename[0] == '\0' || strlen( filename ) < 3 )
   {
      if( !filename || !str_cmp( filename, "" ) )
         send_to_char( "Empty filename is not valid.\r\n", ch );
      else
         ch_printf( ch, "%s: Filename is too short.\r\n", filename );
      return false;
   }

   /* Illegal characters */
   if( strstr( filename, ".." ) || strstr( filename, "/" ) || strstr( filename, "\\" ) )
   {
      send_to_char( "A filename may not contain a '..', '/', or '\\' in it.\r\n", ch );
      return false;
   }

   /* If that filename is already being used lets not allow it now to be on the safe side */
   snprintf( newfilename, sizeof( newfilename ), "%s%s", direct, filename );
   if( stat( newfilename, &fst ) != -1 )
   {
      ch_printf( ch, "%s is already an existing filename.\r\n", newfilename );
      return false;
   }

   /* If we got here assume its valid */
   return true;
}

/*
 * Added lots of EOF checks, as most of the file crashes are based on them.
 * If an area file encounters EOF, the fread_* functions will shutdown the
 * MUD, as all area files should be read in in full or bad things will
 * happen during the game.  Any files loaded in without fBootDb which
 * encounter EOF will return what they have read so far.   These files
 * should include player files, and in-progress areas that are not loaded
 * upon bootup.
 * -- Altrag
 */
/*
 * Read a letter from a file.
 */
char fread_letter( FILE * fp )
{
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return '\0';
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   return c;
}

/*
 * Read a float number from a file. Turn the result into a float value.
 */
float fread_float( FILE * fp )
{
   float number;
   bool sign, decimal;
   char c;
   int place = 0;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_float: EOF encountered on read." );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0;

   sign = FALSE;
   decimal = FALSE;

   if( c == '+' )
      c = getc( fp );
   else if( c == '-' )
   {
      sign = TRUE;
      c = getc( fp );
   }

   if( !isdigit( c ) )
   {
      bug( "Fread_float: bad format. (%c)", c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( 1 )
   {
      if( c == '.' || isdigit( c ) )
      {
         if( c == '.' )
         {
            decimal = TRUE;
            c = getc( fp );
         }

         if( feof( fp ) )
         {
            bug( "%s", "fread_float: EOF encountered on read." );
            if( fBootDb )
               exit( 1 );
            return number;
         }
         if( !decimal )
            number = number * 10 + c - '0';
         else
         {
            place++;
            number += pow( 10, ( -1 * place ) ) * ( c - '0' );
         }
         c = getc( fp );
      }
      else
         break;
   }

   if( sign )
      number = 0 - number;

   if( c == '|' )
      number += fread_float( fp );
   else if( c != ' ' )
      ungetc( c, fp );

   return number;
}

/*
 * Read a number from a file. Convert to long integer.
 */
long fread_long( FILE * fp )
{
   long number;
   bool sign;
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_long: EOF encountered on read." );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0;

   sign = FALSE;
   if( c == '+' )
      c = getc( fp );
   else if( c == '-' )
   {
      sign = TRUE;
      c = getc( fp );
   }

   if( !isdigit( c ) )
   {
      bug( "Fread_long: bad format. (%c)", c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_long: EOF encountered on read." );
         if( fBootDb )
            exit( 1 );
         return number;
      }
      number = number * 10 + c - '0';
      c = getc( fp );
   }

   if( sign )
      number = 0 - number;

   if( c == '|' )
      number += fread_long( fp );
   else if( c != ' ' )
      ungetc( c, fp );

   return number;
}

/*
 * Read a number from a file. Convert to short integer.
 */
short fread_short( FILE * fp )
{
   short number;
   bool sign;
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_short: EOF encountered on read." );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0;

   sign = FALSE;
   if( c == '+' )
      c = getc( fp );
   else if( c == '-' )
   {
      sign = TRUE;
      c = getc( fp );
   }

   if( !isdigit( c ) )
   {
      bug( "Fread_short: bad format. (%c)", c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_short: EOF encountered on read." );
         if( fBootDb )
            exit( 1 );
         return number;
      }
      number = number * 10 + c - '0';
      c = getc( fp );
   }

   if( sign )
      number = 0 - number;

   if( c == '|' )
      number += fread_short( fp );
   else if( c != ' ' )
      ungetc( c, fp );

   return number;
}

/*
 * Read a number from a file.
 */
int fread_number( FILE * fp )
{
   int number;
   bool sign;
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_number: EOF encountered on read." );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0;

   sign = FALSE;
   if( c == '+' )
   {
      c = getc( fp );
   }
   else if( c == '-' )
   {
      sign = TRUE;
      c = getc( fp );
   }

   if( !isdigit( c ) )
   {
      bug( "Fread_number: bad format. (%c)", c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_number: EOF encountered on read." );
         if( fBootDb )
            exit( 1 );
         return number;
      }
      number = number * 10 + c - '0';
      c = getc( fp );
   }

   if( sign )
      number = 0 - number;

   if( c == '|' )
      number += fread_number( fp );
   else if( c != ' ' )
      ungetc( c, fp );

   return number;
}

/*
 * Read a string of text based flags from file fp. Ending in ~
 */
char *fread_flagstring( FILE * fp )
{
   static char buf[MSL];
   char *plast;
   char c;
   int ln;

   plast = buf;
   buf[0] = '\0';
   ln = 0;

   /*
    * Skip blanks.
    * Read first char.
    */
   do
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_string: EOF encountered on read." );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return "";
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   if( ( *plast++ = c ) == '~' )
      return "";

   for( ;; )
   {
      if( ln >= ( MSL - 1 ) )
      {
         bug( "%s", "fread_flagstring: string too long" );
         *plast = '\0';
         return ( buf );
      }
      switch ( *plast = getc( fp ) )
      {
         default:
            plast++;
            ln++;
            break;

         case EOF:
            bug( "%s", "Fread_string: EOF" );
            if( fBootDb )
               exit( 1 );
            *plast = '\0';
            return ( buf );

         case '\n':
            plast++;
            ln++;
            *plast++ = '\r';
            ln++;
            break;

         case '\r':
            break;

         case '~':
            *plast = '\0';
            return ( buf );
      }
   }
}

/*
 * Read a string from file fp
 */
char *fread_string( FILE * fp )
{
   char buf[MSL];
   char *plast;
   char c;
   int ln;

   plast = buf;
   buf[0] = '\0';
   ln = 0;

   /*
    * Skip blanks.
    * Read first char.
    */
   do
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_string: EOF encountered on read." );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return NULL;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   if( ( *plast++ = c ) == '~' )
      return NULL;

   for( ;; )
   {
      if( ln >= ( MSL - 1 ) )
      {
         bug( "%s", "fread_string: string too long" );
         *plast = '\0';
         return STRALLOC( buf );
      }
      switch ( *plast = getc( fp ) )
      {
         default:
            plast++;
            ln++;
            break;

         case EOF:
            bug( "%s", "Fread_string: EOF" );
            if( fBootDb )
               exit( 1 );
            *plast = '\0';
            return STRALLOC( buf );

         case '\n':
            plast++;
            ln++;
            *plast++ = '\r';
            ln++;
            break;

         case '\r':
            break;

         case '~':
            *plast = '\0';
            return STRALLOC( buf );
      }
   }
}

/*
 * Read a string from file fp using str_dup (ie: no string hashing)
 */
char *fread_string_nohash( FILE * fp )
{
   char buf[MSL];
   char *plast;
   char c;
   int ln;

   plast = buf;
   buf[0] = '\0';
   ln = 0;

   /*
    * Skip blanks.
    * Read first char.
    */
   do
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_string_no_hash: EOF encountered on read." );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return str_dup( "" );
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   if( ( *plast++ = c ) == '~' )
      return str_dup( "" );

   for( ;; )
   {
      if( ln >= ( MSL - 1 ) )
      {
         bug( "%s", "fread_string_no_hash: string too long" );
         *plast = '\0';
         return str_dup( buf );
      }
      switch ( *plast = getc( fp ) )
      {
         default:
            plast++;
            ln++;
            break;

         case EOF:
            bug( "%s", "Fread_string_no_hash: EOF" );
            if( fBootDb )
               exit( 1 );
            *plast = '\0';
            return str_dup( buf );

         case '\n':
            plast++;
            ln++;
            *plast++ = '\r';
            ln++;
            break;

         case '\r':
            break;

         case '~':
            *plast = '\0';
            return str_dup( buf );
      }
   }
}

/*
 * Read to end of line (for comments).
 */
void fread_to_eol( FILE * fp )
{
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_to_eol: EOF encountered on read." );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return;
      }
      c = getc( fp );
   }
   while( c != '\n' && c != '\r' );

   do
   {
      c = getc( fp );
   }
   while( c == '\n' || c == '\r' );

   ungetc( c, fp );
   return;
}

/*
 * Read to end of line into static buffer			-Thoric
 */
char *fread_line( FILE * fp )
{
   static char line[MSL];
   char *pline;
   char c;
   int ln;

   pline = line;
   line[0] = '\0';
   ln = 0;

   /*
    * Skip blanks.
    * Read first char.
    */
   do
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_line: EOF encountered on read." );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return "";
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   ungetc( c, fp );

   do
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_line: EOF encountered on read." );
         if( fBootDb )
            exit( 1 );
         *pline = '\0';
         return line;
      }
      c = getc( fp );
      *pline++ = c;
      ln++;
      if( ln >= ( MSL - 1 ) )
      {
         bug( "%s", "fread_line: line too long" );
         break;
      }
   }
   while( c != '\n' && c != '\r' );

   do
      c = getc( fp );
   while( c == '\n' || c == '\r' );

   ungetc( c, fp );
   *pline = '\0';
   return line;
}

/*
 * Read one word (into static buffer).
 */
char *fread_word( FILE * fp )
{
   static char word[MIL];
   char *pword;
   char cEnd;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_word: EOF encountered on read." );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }

         word[0] = '\0';
         return word;
      }
      cEnd = getc( fp );
   }
   while( isspace( cEnd ) );

   if( cEnd == '\'' || cEnd == '"' )
      pword = word;
   else
   {
      word[0] = cEnd;
      pword = word + 1;
      cEnd = ' ';
   }

   for( ; pword < word + MIL; pword++ )
   {
      if( feof( fp ) )
      {
         bug( "%s", "fread_word: EOF encountered on read." );
         if( fBootDb )
            exit( 1 );
         word[0] = '\0';
         return word;
      }
      *pword = getc( fp );
      if( cEnd == ' ' ? isspace( *pword ) : *pword == cEnd )
      {
         if( cEnd == ' ' )
            ungetc( *pword, fp );
         *pword = '\0';
         return word;
      }
   }
   bug( "%s", "Fread_word: word too long" );
   *pword = '\0';
   return word;
}

/*
 * I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */
static int rgiState[2 + 55];

void init_mm( void )
{
   int *piState;
   int iState;

   piState = &rgiState[2];

   piState[-2] = 55 - 55;
   piState[-1] = 55 - 24;

   piState[0] = ( ( int )current_time ) & ( ( 1 << 30 ) - 1 );
   piState[1] = 1;

   for( iState = 2; iState < 55; iState++ )
   {
      piState[iState] = ( piState[iState - 1] + piState[iState - 2] ) & ( ( 1 << 30 ) - 1 );
   }
   return;
}

/*
 * Add a string to the boot-up log				-Thoric
 */
void boot_log( const char *str, ... )
{
   char buf[MSL];
   FILE *fp;
   va_list param;

   mudstrlcpy( buf, "[*****] BOOT: ", MSL );
   va_start( param, str );
   vsnprintf( buf + strlen( buf ), MSL, str, param );
   va_end( param );
   log_string( buf );

   if( ( fp = fopen( BOOTLOG_FILE, "a" ) ) != NULL )
   {
      fprintf( fp, "%s\n", buf );
      FCLOSE( fp );
   }
   return;
}

/* Build list of in_progress areas.  Do not load areas.
 * define AREA_READ if you want it to build area names rather than reading
 * them out of the area files. -- Altrag */
/* The above info is obsolete - this will now simply load whatever is in the
 * BUILD_DIR and assume it to be a valid prototype zone. -- Samson 2-13-04
 */
void load_buildlist( void )
{
   DIR *dp;
   struct dirent *dentry;
   char buf[256];

   dp = opendir( BUILD_DIR );
   dentry = readdir( dp );
   while( dentry )
   {
      if( dentry->d_name[0] != '.' )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( str_cmp( dentry->d_name, "CVS" ) && !str_infix( ".are", dentry->d_name ) )
         {
            if( str_infix( ".bak", dentry->d_name ) )
            {
               snprintf( buf, 256, "%s%s", BUILD_DIR, dentry->d_name );
               fBootDb = TRUE;
               snprintf( strArea, MIL, "%s", dentry->d_name );
               load_area_file( last_area, buf, TRUE );
               fBootDb = FALSE;
            }
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );
}

/*
 * Save system info to data file
 */
void save_sysdata( SYSTEM_DATA sys )
{
   FILE *fp;
   char filename[256];

   snprintf( filename, 256, "%ssysdata.dat", SYSTEM_DIR );

   if( ( fp = fopen( filename, "w" ) ) == NULL )
   {
      bug( "%s", "save_sysdata: fopen" );
      perror( filename );
   }
   else
   {
      fprintf( fp, "%s", "#SYSTEM\n" );
      fprintf( fp, "MudName	     %s~\n", sys.mud_name );
      fprintf( fp, "Password       %s~\n", sys.password );
      fprintf( fp, "Highplayers    %d\n", sys.alltimemax );
      fprintf( fp, "Highplayertime %s~\n", sys.time_of_max );
      fprintf( fp, "CheckImmHost   %d\n", sys.check_imm_host );
      fprintf( fp, "Nameresolving  %d\n", sys.NO_NAME_RESOLVING );
      fprintf( fp, "Waitforauth    %d\n", sys.WAIT_FOR_AUTH );
      fprintf( fp, "Readallmail    %d\n", sys.read_all_mail );
      fprintf( fp, "Readmailfree   %d\n", sys.read_mail_free );
      fprintf( fp, "Writemailfree  %d\n", sys.write_mail_free );
      fprintf( fp, "Takeothersmail %d\n", sys.take_others_mail );
      fprintf( fp, "Getnotake      %d\n", sys.level_getobjnotake );
      fprintf( fp, "Build          %d\n", sys.build_level );
      fprintf( fp, "Protoflag      %d\n", sys.level_modify_proto );
      fprintf( fp, "Overridepriv   %d\n", sys.level_override_private );
      fprintf( fp, "Msetplayer     %d\n", sys.level_mset_player );
      fprintf( fp, "Stunplrvsplr   %d\n", sys.stun_plr_vs_plr );
      fprintf( fp, "Stunregular    %d\n", sys.stun_regular );
      fprintf( fp, "Gougepvp       %d\n", sys.gouge_plr_vs_plr );
      fprintf( fp, "Gougenontank   %d\n", sys.gouge_nontank );
      fprintf( fp, "Bashpvp        %d\n", sys.bash_plr_vs_plr );
      fprintf( fp, "Bashnontank    %d\n", sys.bash_nontank );
      fprintf( fp, "Dodgemod       %d\n", sys.dodge_mod );
      fprintf( fp, "Parrymod       %d\n", sys.parry_mod );
      fprintf( fp, "Tumblemod      %d\n", sys.tumble_mod );
      fprintf( fp, "Damplrvsplr    %d\n", sys.dam_plr_vs_plr );
      fprintf( fp, "Damplrvsmob    %d\n", sys.dam_plr_vs_mob );
      fprintf( fp, "Dammobvsplr    %d\n", sys.dam_mob_vs_plr );
      fprintf( fp, "Dammobvsmob    %d\n", sys.dam_mob_vs_mob );
      fprintf( fp, "Forcepc        %d\n", sys.level_forcepc );
      fprintf( fp, "Saveflags      %d\n", sys.save_flags );
      fprintf( fp, "Savefreq       %d\n", sys.save_frequency );
      fprintf( fp, "Bestowdif      %d\n", sys.bestow_dif );
      fprintf( fp, "PetSave	     %d\n", sys.save_pets );
      fprintf( fp, "Wizlock		%d\n", sys.WIZLOCK );
      fprintf( fp, "Implock		%d\n", sys.IMPLOCK );
      fprintf( fp, "Lockdown		%d\n", sys.LOCKDOWN );
      fprintf( fp, "Admin_Email	%s~\n", sys.admin_email );
      fprintf( fp, "Newbie_purge	%d\n", sys.newbie_purge );
      fprintf( fp, "Regular_purge	%d\n", sys.regular_purge );
      fprintf( fp, "Autopurge		%d\n", sys.CLEANPFILES );
      fprintf( fp, "Testmode		%d\n", sys.TESTINGMODE );
      fprintf( fp, "Rent		%d\n", sys.RENT );
      fprintf( fp, "Mapsize		%d\n", sys.mapsize );
      fprintf( fp, "Motd		%ld\n", ( long int )sys.motd );
      fprintf( fp, "Imotd		%ld\n", ( long int )sys.imotd );
      fprintf( fp, "Webcounter	%d\n", sys.webcounter );
      fprintf( fp, "Webtoggle		%d\n", sys.webtoggle );
      fprintf( fp, "Telnet		%s~\n", sys.telnet );
      fprintf( fp, "HTTP		%s~\n", sys.http );
      fprintf( fp, "Maxvnum		%d\n", sys.maxvnum );
      fprintf( fp, "Minguild		%d\n", sys.minguildlevel );
      fprintf( fp, "Maxcond		%d\n", sys.maxcondval );
      fprintf( fp, "Maxignore		%d\n", sys.maxign );
      fprintf( fp, "Maximpact		%d\n", sys.maximpact );
      fprintf( fp, "Maxholiday	%d\n", sys.maxholiday );
      fprintf( fp, "Initcond		%d\n", sys.initcond );
      fprintf( fp, "Secpertick	%d\n", sys.secpertick );
      fprintf( fp, "Pulsepersec	%d\n", sys.pulsepersec );
      fprintf( fp, "Hoursperday	%d\n", sys.hoursperday );
      fprintf( fp, "Daysperweek	%d\n", sys.daysperweek );
      fprintf( fp, "Dayspermonth	%d\n", sys.dayspermonth );
      fprintf( fp, "Monthsperyear	%d\n", sys.monthsperyear );
      fprintf( fp, "Minrent		%d\n", sys.minrent );
      fprintf( fp, "Rebootcount	%d\n", sys.rebootcount );
      fprintf( fp, "Auctionseconds  %d\n", sys.auctionseconds );
      fprintf( fp, "Crashhandler    %d\n", sys.crashhandler );
      fprintf( fp, "%s", "End\n\n" );
      fprintf( fp, "%s", "#END\n" );
   }
   FCLOSE( fp );
   return;
}

void fread_sysdata( SYSTEM_DATA * sys, FILE * fp )
{
   const char *word;
   bool fMatch;

   sys->time_of_max = NULL;
   sys->mud_name = NULL;
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
            KEY( "Admin_Email", sys->admin_email, fread_string_nohash( fp ) );
            KEY( "Auctionseconds", sys->auctionseconds, fread_number( fp ) );
            KEY( "Autopurge", sys->CLEANPFILES, fread_number( fp ) );
            break;

         case 'B':
            KEY( "Bashpvp", sys->bash_plr_vs_plr, fread_number( fp ) );
            KEY( "Bashnontank", sys->bash_nontank, fread_number( fp ) );
            KEY( "Bestowdif", sys->bestow_dif, fread_number( fp ) );
            KEY( "Build", sys->build_level, fread_number( fp ) );
            break;

         case 'C':
            KEY( "CheckImmHost", sys->check_imm_host, fread_number( fp ) );
            KEY( "Crashhandler", sys->crashhandler, fread_number( fp ) );
            break;

         case 'D':
            KEY( "Damplrvsplr", sys->dam_plr_vs_plr, fread_number( fp ) );
            KEY( "Damplrvsmob", sys->dam_plr_vs_mob, fread_number( fp ) );
            KEY( "Dammobvsplr", sys->dam_mob_vs_plr, fread_number( fp ) );
            KEY( "Dammobvsmob", sys->dam_mob_vs_mob, fread_number( fp ) );
            KEY( "Dodgemod", sys->dodge_mod, fread_number( fp ) );
            KEY( "Daysperweek", sys->daysperweek, fread_number( fp ) );
            KEY( "Dayspermonth", sys->dayspermonth, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !sys->time_of_max )
                  sys->time_of_max = str_dup( "(not recorded)" );
               if( !sys->mud_name )
                  sys->mud_name = str_dup( "(Name Not Set)" );
               if( !sys->http )
                  sys->http = str_dup( "No page set" );
               if( !sys->telnet )
                  sys->telnet = str_dup( "Not set" );
               return;
            }
            break;

         case 'F':
            KEY( "Forcepc", sys->level_forcepc, fread_number( fp ) );
            break;

         case 'G':
            KEY( "Getnotake", sys->level_getobjnotake, fread_number( fp ) );
            KEY( "Gougepvp", sys->gouge_plr_vs_plr, fread_number( fp ) );
            KEY( "Gougenontank", sys->gouge_nontank, fread_number( fp ) );
            break;

         case 'H':
            KEY( "Highplayers", sys->alltimemax, fread_number( fp ) );
            KEY( "Highplayertime", sys->time_of_max, fread_string_nohash( fp ) );
            KEY( "HTTP", sys->http, fread_string_nohash( fp ) );
            KEY( "Hoursperday", sys->hoursperday, fread_number( fp ) );
            break;

         case 'I':
            KEY( "Imotd", sys->imotd, fread_number( fp ) );
            KEY( "Implock", sys->IMPLOCK, fread_number( fp ) );
            KEY( "Initcond", sys->initcond, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Lockdown", sys->LOCKDOWN, fread_number( fp ) );
            break;

         case 'M':
            KEY( "Mapsize", sys->mapsize, fread_number( fp ) );
            KEY( "Motd", sys->motd, fread_number( fp ) );
            KEY( "Msetplayer", sys->level_mset_player, fread_number( fp ) );
            KEY( "MudName", sys->mud_name, fread_string_nohash( fp ) );
            KEY( "Maxvnum", sys->maxvnum, fread_number( fp ) );
            KEY( "Minguild", sys->minguildlevel, fread_number( fp ) );
            KEY( "Maxcond", sys->maxcondval, fread_number( fp ) );
            KEY( "Maxignore", sys->maxign, fread_number( fp ) );
            KEY( "Maximpact", sys->maximpact, fread_number( fp ) );
            KEY( "Maxholiday", sys->maxholiday, fread_number( fp ) );
            KEY( "Monthsperyear", sys->monthsperyear, fread_number( fp ) );
            KEY( "Minrent", sys->minrent, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Nameresolving", sys->NO_NAME_RESOLVING, fread_number( fp ) );
            KEY( "Newbie_purge", sys->newbie_purge, fread_number( fp ) );
            break;

         case 'O':
            KEY( "Overridepriv", sys->level_override_private, fread_number( fp ) );
            break;

         case 'P':
            KEY( "Parrymod", sys->parry_mod, fread_number( fp ) );
            KEY( "Password", sys->password, fread_string_nohash( fp ) );   /* Samson 2-8-01 */
            KEY( "PetSave", sys->save_pets, fread_number( fp ) );
            KEY( "Protoflag", sys->level_modify_proto, fread_number( fp ) );
            KEY( "Pulsepersec", sys->pulsepersec, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Readallmail", sys->read_all_mail, fread_number( fp ) );
            KEY( "Readmailfree", sys->read_mail_free, fread_number( fp ) );
            KEY( "Rebootcount", sys->rebootcount, fread_number( fp ) );
            KEY( "Regular_purge", sys->regular_purge, fread_number( fp ) );
            KEY( "Rent", sys->RENT, fread_number( fp ) );
            break;

         case 'S':
            KEY( "Stunplrvsplr", sys->stun_plr_vs_plr, fread_number( fp ) );
            KEY( "Stunregular", sys->stun_regular, fread_number( fp ) );
            KEY( "Saveflags", sys->save_flags, fread_number( fp ) );
            KEY( "Savefreq", sys->save_frequency, fread_number( fp ) );
            KEY( "Secpertick", sys->secpertick, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Takeothersmail", sys->take_others_mail, fread_number( fp ) );
            KEY( "Telnet", sys->telnet, fread_string_nohash( fp ) );
            KEY( "Testmode", sys->TESTINGMODE, fread_number( fp ) );
            KEY( "Tumblemod", sys->tumble_mod, fread_number( fp ) );
            break;


         case 'W':
            KEY( "Waitforauth", sys->WAIT_FOR_AUTH, fread_number( fp ) );
            KEY( "Webcounter", sys->webcounter, fread_number( fp ) );
            KEY( "Webtoggle", sys->webtoggle, fread_number( fp ) );
            KEY( "Wizlock", sys->WIZLOCK, fread_number( fp ) );
            KEY( "Writemailfree", sys->write_mail_free, fread_number( fp ) );
            break;
      }

      if( !fMatch )
         bug( "Fread_sysdata: no match: %s", word );
   }
}

/*
 * Load the sysdata file
 */
bool load_systemdata( SYSTEM_DATA * sys )
{
   char filename[256];
   FILE *fp;
   bool found;

   found = FALSE;
   snprintf( filename, 256, "%ssysdata.dat", SYSTEM_DIR );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {

      found = TRUE;
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
            bug( "%s", "Load_sysdata_file: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "SYSTEM" ) )
         {
            fread_sysdata( sys, fp );
            break;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "Load_sysdata_file: bad section: %s", word );
            break;
         }
      }
      FCLOSE( fp );
      update_timers(  );
      update_calendar(  );
   }
   return found;
}

/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits( void )
{
   ROOM_INDEX_DATA *pRoomIndex;
   EXIT_DATA *pexit, *pexit_next, *rv_exit;
   int iHash;

   for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
   {
      for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
      {
         for( pexit = pRoomIndex->first_exit; pexit; pexit = pexit_next )
         {
            pexit_next = pexit->next;
            pexit->rvnum = pRoomIndex->vnum;
            if( pexit->vnum <= 0 || ( pexit->to_room = get_room_index( pexit->vnum ) ) == NULL )
            {
               if( fBootDb )
                  boot_log( "Fix_exits: room %d, exit %s leads to bad vnum (%d)",
                            pRoomIndex->vnum, dir_name[pexit->vdir], pexit->vnum );

               bug( "Deleting %s exit in room %d", dir_name[pexit->vdir], pRoomIndex->vnum );
               extract_exit( pRoomIndex, pexit );
            }
         }
      }
   }

   /*
    * Set all the rexit pointers - Thoric 
    */
   for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
   {
      for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
      {
         for( pexit = pRoomIndex->first_exit; pexit; pexit = pexit->next )
         {
            if( pexit->to_room && !pexit->rexit )
            {
               rv_exit = get_exit_to( pexit->to_room, rev_dir[pexit->vdir], pRoomIndex->vnum );
               if( rv_exit )
               {
                  pexit->rexit = rv_exit;
                  rv_exit->rexit = pexit;
               }
            }
         }
      }
   }
   return;
}

/*
 * Initialize the weather for all the areas
 * Last Modified: July 21, 1997
 * Fireblade
 */
void init_area_weather( void )
{
   AREA_DATA *pArea;
   NEIGHBOR_DATA *neigh;
   NEIGHBOR_DATA *next_neigh;

   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      int cf;

      /*
       * init temp and temp vector 
       */
      cf = pArea->weather->climate_temp - 2;
      pArea->weather->temp = number_range( -weath_unit, weath_unit ) + cf * number_range( 0, weath_unit );
      pArea->weather->temp_vector = cf + number_range( -rand_factor, rand_factor );

      /*
       * init precip and precip vector 
       */
      cf = pArea->weather->climate_precip - 2;
      pArea->weather->precip = number_range( -weath_unit, weath_unit ) + cf * number_range( 0, weath_unit );
      pArea->weather->precip_vector = cf + number_range( -rand_factor, rand_factor );

      /*
       * init wind and wind vector 
       */
      cf = pArea->weather->climate_wind - 2;
      pArea->weather->wind = number_range( -weath_unit, weath_unit ) + cf * number_range( 0, weath_unit );
      pArea->weather->wind_vector = cf + number_range( -rand_factor, rand_factor );

      /*
       * check connections between neighbors 
       */
      for( neigh = pArea->weather->first_neighbor; neigh; neigh = next_neigh )
      {
         AREA_DATA *tarea;
         NEIGHBOR_DATA *tneigh;

         /*
          * get the address if needed 
          */
         if( !neigh->address )
            neigh->address = get_area( neigh->name );

         /*
          * area does not exist 
          */
         if( !neigh->address )
         {
            tneigh = neigh;
            next_neigh = tneigh->next;
            UNLINK( tneigh, pArea->weather->first_neighbor, pArea->weather->last_neighbor, next, prev );
            STRFREE( tneigh->name );
            DISPOSE( tneigh );
            fold_area( pArea, pArea->filename, FALSE );
            continue;
         }

         /*
          * make sure neighbors both point to each other 
          */
         tarea = neigh->address;
         for( tneigh = tarea->weather->first_neighbor; tneigh; tneigh = tneigh->next )
         {
            if( !str_cmp( pArea->name, tneigh->name ) )
               break;
         }

         if( !tneigh )
         {
            CREATE( tneigh, NEIGHBOR_DATA, 1 );
            tneigh->name = STRALLOC( pArea->name );
            LINK( tneigh, tarea->weather->first_neighbor, tarea->weather->last_neighbor, next, prev );
            fold_area( tarea, tarea->filename, FALSE );
         }
         tneigh->address = pArea;
         next_neigh = neigh->next;
      }
   }
   return;
}

/*
 * Load weather data from appropriate file in system dir
 * Last Modified: July 24, 1997
 * Fireblade
 */
void load_weatherdata( void )
{
   char filename[256];
   FILE *fp;

   snprintf( filename, 256, "%sweather.dat", SYSTEM_DIR );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );

         if( letter != '#' )
         {
            bug( "%s", "load_weatherdata: # not found" );
            return;
         }
         word = fread_word( fp );

         if( !str_cmp( word, "RANDOM" ) )
            rand_factor = fread_number( fp );
         else if( !str_cmp( word, "CLIMATE" ) )
            climate_factor = fread_number( fp );
         else if( !str_cmp( word, "NEIGHBOR" ) )
            neigh_factor = fread_number( fp );
         else if( !str_cmp( word, "UNIT" ) )
         {
            int unit = fread_number( fp );

            if( unit == 0 )
               unit = 1;

            weath_unit = unit;
         }
         else if( !str_cmp( word, "MAXVECTOR" ) )
            max_vector = fread_number( fp );
         else if( !str_cmp( word, "END" ) )
         {
            FCLOSE( fp );
            break;
         }
         else
         {
            bug( "%s", "load_weatherdata: unknown field" );
            FCLOSE( fp );
            break;
         }
      }
   }
   return;
}

/*
 * Write data for global weather parameters
 * Last Modified: July 24, 1997
 * Fireblade
 */
void save_weatherdata( void )
{
   char filename[256];
   FILE *fp;

   snprintf( filename, 256, "%sweather.dat", SYSTEM_DIR );

   if( ( fp = fopen( filename, "w" ) ) != NULL )
   {
      fprintf( fp, "#RANDOM %d\n", rand_factor );
      fprintf( fp, "#CLIMATE %d\n", climate_factor );
      fprintf( fp, "#NEIGHBOR %d\n", neigh_factor );
      fprintf( fp, "#UNIT %d\n", weath_unit );
      fprintf( fp, "#MAXVECTOR %d\n", max_vector );
      fprintf( fp, "%s", "#END\n" );
      FCLOSE( fp );
   }
   else
      bug( "%s", "save_weatherdata: could not open file" );
   return;
}

/*
 * Command to control global weather variables and to reset weather
 * Last Modified: July 23, 1997
 * Fireblade
 */
CMDF do_setweather( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];

   set_char_color( AT_BLUE, ch );

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      ch_printf( ch, "%-15s%-6s\n\r", "Parameters:", "Value:" );
      ch_printf( ch, "%-15s%-6d\n\r", "random", rand_factor );
      ch_printf( ch, "%-15s%-6d\n\r", "climate", climate_factor );
      ch_printf( ch, "%-15s%-6d\n\r", "neighbor", neigh_factor );
      ch_printf( ch, "%-15s%-6d\n\r", "unit", weath_unit );
      ch_printf( ch, "%-15s%-6d\n\r", "maxvector", max_vector );

      send_to_char( "\n\rResulting values:\n\r", ch );
      ch_printf( ch, "Weather variables range from %d to %d.\n\r", -3 * weath_unit, 3 * weath_unit );
      ch_printf( ch, "Weather vectors range from %d to %d.\n\r", -1 * max_vector, max_vector );
      ch_printf( ch, "The maximum a vector can change in one update is %d.\n\r",
                 rand_factor + 2 * climate_factor + ( 6 * weath_unit / neigh_factor ) );
   }

   else if( !str_cmp( arg, "random" ) )
   {
      if( !is_number( argument ) )
         send_to_char( "Set maximum random change in vectors to what?\n\r", ch );
      else
      {
         rand_factor = atoi( argument );
         ch_printf( ch, "Maximum random change in vectors now equals %d.\n\r", rand_factor );
         save_weatherdata(  );
      }
   }

   else if( !str_cmp( arg, "climate" ) )
   {
      if( !is_number( argument ) )
         send_to_char( "Set climate effect coefficient to what?\n\r", ch );
      else
      {
         climate_factor = atoi( argument );
         ch_printf( ch, "Climate effect coefficient now equals %d.\n\r", climate_factor );
         save_weatherdata(  );
      }
   }

   else if( !str_cmp( arg, "neighbor" ) )
   {
      if( !is_number( argument ) )
         send_to_char( "Set neighbor effect divisor to what?\n\r", ch );
      else
      {
         neigh_factor = atoi( argument );

         if( neigh_factor <= 0 )
            neigh_factor = 1;

         ch_printf( ch, "Neighbor effect coefficient now equals 1/%d.\n\r", neigh_factor );
         save_weatherdata(  );
      }
   }

   else if( !str_cmp( arg, "unit" ) )
   {
      if( !is_number( argument ) )
         ch_printf( ch, "Set weather unit size to what?\r\n" );
      else
      {
         int unit = atoi( argument );

         if( unit == 0 )
         {
            send_to_char( "Weather unit size cannot be zero.\r\n", ch );
            return;
         }
         weath_unit = unit;
         ch_printf( ch, "Weather unit size now equals %d.\r\n", weath_unit );
         save_weatherdata(  );
      }
   }

   else if( !str_cmp( arg, "maxvector" ) )
   {
      if( !is_number( argument ) )
         send_to_char( "Set maximum vector size to what?\n\r", ch );
      else
      {
         max_vector = atoi( argument );
         ch_printf( ch, "Maximum vector size now equals %d.\n\r", max_vector );
         save_weatherdata(  );
      }
   }

   else if( !str_cmp( arg, "reset" ) )
   {
      init_area_weather(  );
      send_to_char( "Weather system reinitialized.\n\r", ch );
   }

   else if( !str_cmp( arg, "update" ) )
   {
      int i, number;

      number = atoi( argument );

      if( number < 1 )
         number = 1;

      for( i = 0; i < number; i++ )
         weather_update(  );

      send_to_char( "Weather system updated.\n\r", ch );
   }

   else
   {
      send_to_char( "You may only use one of the following fields:\n\r", ch );
      send_to_char( "\trandom\n\r\tclimate\n\r\tneighbor\n\r\tunit\n\r\tmaxvector\n\r", ch );
      send_to_char( "You may also reset or update the system using the fields 'reset' and 'update' respectively.\n\r", ch );
   }
   return;
}

/*
 * wizlist builder!						-Thoric
 */
void towizfile( const char *line )
{
   int filler, xx;
   char outline[MSL];
   FILE *wfp;

   outline[0] = '\0';

   if( line && line[0] != '\0' )
   {
      filler = ( 78 - strlen( line ) );
      if( filler < 1 )
         filler = 1;
      filler /= 2;
      for( xx = 0; xx < filler; xx++ )
         mudstrlcat( outline, " ", MSL );
      mudstrlcat( outline, line, MSL );
   }
   mudstrlcat( outline, "\n\r", MSL );
   wfp = fopen( WIZLIST_FILE, "a" );
   if( wfp )
   {
      fputs( outline, wfp );
      FCLOSE( wfp );
   }
}

void towebwiz( const char *line )
{
   char outline[MSL];
   FILE *wfp;

   outline[0] = '\0';

   mudstrlcat( outline, " ", MSL );
   mudstrlcat( outline, line, MSL );
   mudstrlcat( outline, "\n\r", MSL );
   wfp = fopen( WEBWIZ_FILE, "a" );
   if( wfp )
   {
      fputs( outline, wfp );
      FCLOSE( wfp );
   }
}

void add_to_wizlist( char *name, char *http, int level )
{
   WIZENT *wiz, *tmp;

   CREATE( wiz, WIZENT, 1 );
   wiz->name = str_dup( name );
   if( http != NULL )
      wiz->http = str_dup( http );
   wiz->level = level;

   if( !first_wiz )
   {
      wiz->last = NULL;
      wiz->next = NULL;
      first_wiz = wiz;
      last_wiz = wiz;
      return;
   }

   /*
    * insert sort, of sorts 
    */
   for( tmp = first_wiz; tmp; tmp = tmp->next )
      if( level > tmp->level )
      {
         if( !tmp->last )
            first_wiz = wiz;
         else
            tmp->last->next = wiz;
         wiz->last = tmp->last;
         wiz->next = tmp;
         tmp->last = wiz;
         return;
      }

   wiz->last = last_wiz;
   wiz->next = NULL;
   last_wiz->next = wiz;
   last_wiz = wiz;
   return;
}

/*
 * Wizlist builder						-Thoric
 */
void make_wizlist(  )
{
   DIR *dp;
   struct dirent *dentry;
   FILE *gfp;
   const char *word;
   int ilevel, iflags;
   WIZENT *wiz, *wiznext;
   char buf[256];

   first_wiz = NULL;
   last_wiz = NULL;

   dp = opendir( GOD_DIR );

   ilevel = 0;
   dentry = readdir( dp );
   while( dentry )
   {
      if( dentry->d_name[0] != '.' )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( str_cmp( dentry->d_name, "CVS" ) )
         {
            snprintf( buf, 256, "%s%s", GOD_DIR, dentry->d_name );
            gfp = fopen( buf, "r" );
            if( gfp )
            {
               word = feof( gfp ) ? "End" : fread_word( gfp );
               ilevel = fread_number( gfp );
               fread_to_eol( gfp );
               word = feof( gfp ) ? "End" : fread_word( gfp );
               if( !str_cmp( word, "Pcflags" ) )
                  iflags = fread_number( gfp );
               else
                  iflags = 0;
               FCLOSE( gfp );
               if( IS_SET( iflags, PCFLAG_RETIRED ) )
                  ilevel = MAX_LEVEL - 15;
               if( IS_SET( iflags, PCFLAG_GUEST ) )
                  ilevel = MAX_LEVEL - 16;
               add_to_wizlist( dentry->d_name, NULL, ilevel );
            }
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );

   unlink( WIZLIST_FILE );
   snprintf( buf, 256, "The Immortal Masters of %s", sysdata.mud_name );
   towizfile( buf );

   buf[0] = '\0';
   ilevel = 65535;
   for( wiz = first_wiz; wiz; wiz = wiz->next )
   {
      if( wiz->level < ilevel )
      {
         if( buf[0] )
         {
            towizfile( buf );
            buf[0] = '\0';
         }
         towizfile( "" );
         ilevel = wiz->level;
         switch ( ilevel )
         {
            case MAX_LEVEL - 0:
               towizfile( " Supreme Entity" );
               break;
            case MAX_LEVEL - 1:
               towizfile( " Realm Lords" );
               break;
            case MAX_LEVEL - 2:
               towizfile( " Eternals" );
               break;
            case MAX_LEVEL - 3:
               towizfile( " Ancients" );
               break;
            case MAX_LEVEL - 4:
               towizfile( " Astral Gods" );
               break;
            case MAX_LEVEL - 5:
               towizfile( " Elemental Gods" );
               break;
            case MAX_LEVEL - 6:
               towizfile( " Dream Gods" );
               break;
            case MAX_LEVEL - 7:
               towizfile( " Greater Gods" );
               break;
            case MAX_LEVEL - 8:
               towizfile( " Gods" );
               break;
            case MAX_LEVEL - 9:
               towizfile( " Demi Gods" );
               break;
            case MAX_LEVEL - 10:
               towizfile( " Deities" );
               break;
            case MAX_LEVEL - 11:
               towizfile( " Saviors" );
               break;
            case MAX_LEVEL - 12:
               towizfile( " Creators" );
               break;
            case MAX_LEVEL - 13:
               towizfile( " Acolytes" );
               break;
            case MAX_LEVEL - 14:
               towizfile( " Angels" );
               break;
            case MAX_LEVEL - 15:
               towizfile( " Retired" );
               break;
            case MAX_LEVEL - 16:
               towizfile( " Guests" );
               break;
            default:
               towizfile( " Servants" );
               break;
         }
      }
      if( strlen( buf ) + strlen( wiz->name ) > 76 )
      {
         towizfile( buf );
         buf[0] = '\0';
      }
      mudstrlcat( buf, " ", 256 );
      mudstrlcat( buf, wiz->name, 256 );
      if( strlen( buf ) > 70 )
      {
         towizfile( buf );
         buf[0] = '\0';
      }
   }
   if( buf[0] )
      towizfile( buf );

   for( wiz = first_wiz; wiz; wiz = wiznext )
   {
      wiznext = wiz->next;
      DISPOSE( wiz->name );
      DISPOSE( wiz->http );
      DISPOSE( wiz );
   }
   first_wiz = NULL;
   last_wiz = NULL;
}

/*
 *	Makes a wizlist for showing on the Telnet Interface WWW Site -- KCAH
 */
void make_webwiz( void )
{
   DIR *dp;
   struct dirent *dentry;
   FILE *gfp;
   const char *word;
   int ilevel, iflags;
   WIZENT *wiz, *wiznext;
   char buf[MSL], http[MIL];

   first_wiz = NULL;
   last_wiz = NULL;

   dp = opendir( GOD_DIR );

   ilevel = 0;
   dentry = readdir( dp );
   while( dentry )
   {
      if( dentry->d_name[0] != '.' )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( !str_cmp( dentry->d_name, "CVS" ) )
         {
            dentry = readdir( dp );
            continue;
         }
         if( strstr( dentry->d_name, "immlist" ) )
         {
            dentry = readdir( dp );
            continue;
         }

         snprintf( buf, 256, "%s%s", GOD_DIR, dentry->d_name );
         gfp = fopen( buf, "r" );
         if( gfp )
         {
            word = feof( gfp ) ? "End" : fread_word( gfp );
            ilevel = fread_number( gfp );
            fread_to_eol( gfp );
            word = feof( gfp ) ? "End" : fread_word( gfp );
            if( !str_cmp( word, "Pcflags" ) )
               iflags = fread_number( gfp );
            else
               iflags = 0;
            word = feof( gfp ) ? "End" : fread_word( gfp );
            if( !str_cmp( word, "Homepage" ) )
               mudstrlcpy( http, fread_flagstring( gfp ), MIL );
            else
               http[0] = '\0';
            FCLOSE( gfp );
            if( IS_SET( iflags, PCFLAG_RETIRED ) )
               ilevel = MAX_LEVEL - 15;
            if( IS_SET( iflags, PCFLAG_GUEST ) )
               ilevel = MAX_LEVEL - 16;
            add_to_wizlist( dentry->d_name, http, ilevel );
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );

   unlink( WEBWIZ_FILE );

   buf[0] = '\0';
   ilevel = 65535;
   for( wiz = first_wiz; wiz; wiz = wiz->next )
   {
      if( wiz->level < ilevel )
      {
         if( buf[0] )
         {
            towebwiz( buf );
            buf[0] = '\0';
         }
         towebwiz( "" );
         ilevel = wiz->level;

         switch ( ilevel )
         {
            case MAX_LEVEL - 0:
               towebwiz
                  ( "<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Supreme Entity and Implementor</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 1:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Realm Lords</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 2:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Eternals</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 3:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Ancients</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 4:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Astral Gods</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 5:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Elemental Gods</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 6:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Dream Gods</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 7:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Greater Gods</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 8:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Gods</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 9:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Demi Gods</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 10:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Deities</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 11:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Saviors</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 12:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Creators</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 13:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Acolytes</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 14:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Angels</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 15:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">Retired</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 16:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">Guests</font></p>\n<p align=\"center\">" );
               break;
            default:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">Servants</font></p>\n<p align=\"center\">" );
               break;
         }
      }

      mudstrlcat( buf, "<font size=\"+1\" color=\"#FF0000\">", MSL );

      if( strlen( buf ) + strlen( wiz->name ) > 999 )
      {
         towebwiz( buf );
         buf[0] = '\0';
      }

      mudstrlcat( buf, " ", MSL );
      if( wiz->http && wiz->http[0] != '\0' )
      {
         char wbuf[MSL];

         snprintf( wbuf, MSL, "<a href=\"%s\" target=\"_blank\">%s</a>", wiz->http, wiz->name );
         mudstrlcat( buf, wbuf, MSL );
      }
      else
         mudstrlcat( buf, wiz->name, MSL );

      if( strlen( buf ) > 999 )
      {
         towebwiz( buf );
         buf[0] = '\0';
      }
      mudstrlcat( buf, "</font>\n", MSL );
   }

   if( buf[0] )
   {
      mudstrlcat( buf, "</p>\n", MSL );
      towebwiz( buf );
   }

   for( wiz = first_wiz; wiz; wiz = wiznext )
   {
      wiznext = wiz->next;
      DISPOSE( wiz->name );
      DISPOSE( wiz->http );
      DISPOSE( wiz );
   }
   first_wiz = NULL;
   last_wiz = NULL;
}

CMDF do_makewizlist( CHAR_DATA * ch, char *argument )
{
   make_wizlist(  );
   make_webwiz(  );
   build_wizinfo(  );
}

/*
 * Remap slot numbers to sn values
 */
void remap_slot_numbers( void )
{
   SKILLTYPE *skill;
   SMAUG_AFF *aff;
   int sn;

   log_string( "Remapping slots to sns" );

   for( sn = 0; sn <= top_sn; sn++ )
   {
      if( ( skill = skill_table[sn] ) != NULL )
      {
         if( !str_cmp( skill_table[sn]->name, "doiwork" ) )
         {
            bug( "%s", "Doiwork survived up to here." );
            bug( "It's SN should be: %d", sn );
         }

         if( !str_cmp( skill_table[sn]->name, "testability" ) )
         {
            bug( "%s", "Testability survived up to here." );
            bug( "It's SN should be: %d", sn );
         }

         for( aff = skill->affects; aff; aff = aff->next )
            if( aff->location == APPLY_WEAPONSPELL
                || aff->location == APPLY_WEARSPELL
                || aff->location == APPLY_REMOVESPELL
                || aff->location == APPLY_STRIPSN || aff->location == APPLY_RECURRINGSPELL )
            {
               strdup_printf( &aff->modifier, "%d", slot_lookup( atoi( aff->modifier ) ) );
            }
      }
   }
}

/*
 * Repopulate areas periodically.
 */
void area_update( void )
{
   AREA_DATA *pArea;

   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      CHAR_DATA *pch;
      int reset_age = pArea->reset_frequency ? pArea->reset_frequency : 15;

      /*
       * Proto zones don't reset 
       */
      if( IS_AREA_FLAG( pArea, AFLAG_PROTOTYPE ) )
         continue;

      if( ( reset_age == -1 && pArea->age == -1 ) || ++pArea->age < ( reset_age - 1 ) )
         continue;

      /*
       * Check for PC's.
       */
      if( pArea->nplayer > 0 && pArea->age == ( reset_age - 1 ) )
      {
         if( !pArea->resetmsg || pArea->resetmsg[0] == '\0' )
            continue;

         /*
          * Rennard 
          */
         for( pch = first_char; pch; pch = pch->next )
         {
            if( !IS_NPC( pch ) && IS_AWAKE( pch ) && pch->in_room && pch->in_room->area == pArea )
               ch_printf( pch, "&[reset]%s\n\r", pArea->resetmsg );
         }
      }

      /*
       * Check age and reset.
       * Note: Mud Academy resets every 3 minutes (not 15).
       */
      if( pArea->nplayer == 0 || pArea->age >= reset_age )
      {
         ROOM_INDEX_DATA *pRoomIndex;
         reset_area( pArea );
         if( reset_age == -1 )
            pArea->age = -1;
         else
            pArea->age = number_range( 0, reset_age / 5 );
         pRoomIndex = get_room_index( ROOM_AUTH_START );
         if( pRoomIndex != NULL && pArea == pRoomIndex->area && pArea->reset_frequency == 0 )
            pArea->age = 15 - 3;
      }
   }
   return;
}

/*
 * Big mama top level function.
 */
void boot_db( bool fCopyOver )
{
   char buf[MSL];
   short wear = 0;
   short x;

   fpArea = NULL;
   show_hash( 32 );
   unlink( BOOTLOG_FILE );
   boot_log( "---------------------[ Boot Log ]--------------------" );

   fBootDb = TRUE;   /* Supposed to help with EOF bugs, so it got moved up */

   log_string( "Initializing libdl support..." );
   /*
    * Open up a handle to the executable's symbol table for later use
    * when working with commands
    */
   sysdata.dlHandle = dlopen( NULL, RTLD_NOW );
   if( !sysdata.dlHandle )
   {
      log_string( "dl: Error opening local system executable as handle, please check compile flags." );
      shutdown_mud( "libdl failure" );
      exit( 1 );
   }

   log_string( "Verifying existance of login greeting..." );
   snprintf( buf, MSL, "%sgreeting.dat", MOTD_DIR );
   if( !exists_file( buf ) )
   {
      bug( "%s", "Login greeting not found!" );
      shutdown_mud( "Missing login greeting" );
      exit( 1 );
   }
   else
      log_string( "Login greeting located." );

   log_string( "Loading commands..." );
   load_commands(  );

#ifdef MULTIPORT
   log_string( "Loading shell commands..." );
   load_shellcommands(  );
#endif

   log_string( "Loading spec_funs..." );
   load_specfuns(  );

   load_mudchannels(  );

   log_string( "Loading sysdata configuration..." );

   /*
    * default values 
    */
   sysdata.read_all_mail = LEVEL_DEMI;
   sysdata.read_mail_free = LEVEL_IMMORTAL;
   sysdata.write_mail_free = LEVEL_IMMORTAL;
   sysdata.take_others_mail = LEVEL_DEMI;
   sysdata.build_level = LEVEL_DEMI;
   sysdata.level_modify_proto = LEVEL_LESSER;
   sysdata.level_override_private = LEVEL_GREATER;
   sysdata.level_mset_player = LEVEL_LESSER;
   sysdata.stun_plr_vs_plr = 65;
   sysdata.stun_regular = 15;
   sysdata.gouge_nontank = 0;
   sysdata.gouge_plr_vs_plr = 0;
   sysdata.bash_nontank = 0;
   sysdata.bash_plr_vs_plr = 0;
   sysdata.dodge_mod = 2;
   sysdata.parry_mod = 2;
   sysdata.tumble_mod = 4;
   sysdata.dam_plr_vs_plr = 100;
   sysdata.dam_plr_vs_mob = 100;
   sysdata.dam_mob_vs_plr = 100;
   sysdata.dam_mob_vs_mob = 100;
   sysdata.level_getobjnotake = LEVEL_GREATER;
   sysdata.save_frequency = 20;  /* minutes */
   sysdata.bestow_dif = 5;
   sysdata.check_imm_host = 1;
   sysdata.save_pets = 1;
   sysdata.save_flags = SV_DEATH | SV_PASSCHG | SV_AUTO | SV_PUT | SV_DROP | SV_GIVE | SV_AUCTION | SV_ZAPDROP | SV_IDLE;
   sysdata.motd = current_time;
   sysdata.imotd = current_time;
   sysdata.mapsize = 7;
   sysdata.maxvnum = 60000;
   sysdata.minguildlevel = 10;
   sysdata.maxcondval = 100;
   sysdata.maxign = 6;
   sysdata.maximpact = 30;
   sysdata.maxholiday = 30;
   sysdata.initcond = 12;
   sysdata.minrent = 25000;
   sysdata.secpertick = 70;
   sysdata.pulsepersec = 4;
   sysdata.hoursperday = 28;
   sysdata.daysperweek = 13;
   sysdata.dayspermonth = 26;
   sysdata.monthsperyear = 12;
   sysdata.rebootcount = 5;
   sysdata.auctionseconds = 15;
   sysdata.crashhandler = FALSE;

   if( !load_systemdata( &sysdata ) )
   {
      log_string( "Not found.  Creating new configuration." );
      sysdata.alltimemax = 0;
      sysdata.mud_name = str_dup( "(Name not set)" );
      update_timers(  );
      update_calendar(  );
   }

   log_string( "Loading overland maps..." );
   load_maps(  );

   log_string( "Loading socials..." );
   load_socials(  );

   log_string( "Loading skill table..." );
   load_skill_table(  );
   sort_skill_table(  );
   remap_slot_numbers(  ); /* must be after the sort */

   gsn_first_spell = 0;
   gsn_first_skill = 0;
   gsn_first_weapon = 0;
   gsn_first_tongue = 0;
   gsn_first_ability = 0;
   gsn_first_lore = 0;
   gsn_top_sn = top_sn;

   for( x = 0; x < top_sn; x++ )
      if( !gsn_first_spell && skill_table[x]->type == SKILL_SPELL )
         gsn_first_spell = x;
      else if( !gsn_first_skill && skill_table[x]->type == SKILL_SKILL )
         gsn_first_skill = x;
      else if( !gsn_first_weapon && skill_table[x]->type == SKILL_WEAPON )
         gsn_first_weapon = x;
      else if( !gsn_first_tongue && skill_table[x]->type == SKILL_TONGUE )
         gsn_first_tongue = x;
      else if( !gsn_first_ability && skill_table[x]->type == SKILL_RACIAL )
         gsn_first_ability = x;
      else if( !gsn_first_lore && skill_table[x]->type == SKILL_LORE )
         gsn_first_lore = x;

   log_string( "Loading classes" );
   load_classes(  );

   log_string( "Loading races" );
   load_races(  );

   log_string( "Loading Connection History" );
   load_connhistory(  );

   /*
    * Noplex's liquid system code 
    */
   log_string( "Loading liquid table" );
   load_liquids(  );

   log_string( "Loading mixture table" );
   load_mixtures(  );

   log_string( "Loading herb table" );
   load_herb_table(  );

   log_string( "Loading tongues" );
   load_tongues(  );

   /*
    * abit/qbit code 
    */
   log_string( "Loading quest bit tables" );
   load_bits(  );

   log_string( "Making wizlist" );
   make_wizlist(  );
   log_string( "Making webwiz" );
   make_webwiz(  );

   log_string( "Building wizinfo" );
   build_wizinfo(  );

   nummobsloaded = 0;
   numobjsloaded = 0;
   physicalobjects = 0;
   sysdata.maxplayers = 0;
   first_object = NULL;
   last_object = NULL;
   first_char = NULL;
   last_char = NULL;
   first_area = NULL;
   last_area = NULL;
   first_area_nsort = NULL;
   last_area_nsort = NULL;
   first_area_vsort = NULL;
   last_area_vsort = NULL;
   first_shop = NULL;
   last_shop = NULL;
   first_repair = NULL;
   last_repair = NULL;
   first_teleport = NULL;
   last_teleport = NULL;
   extracted_obj_queue = NULL;
   extracted_char_queue = NULL;
   cur_qobjs = 0;
   cur_qchars = 0;
   quitting_char = NULL;
   loading_char = NULL;
   saving_char = NULL;
   last_pkroom = 1;

   CREATE( auction, AUCTION_DATA, 1 );
   auction->item = NULL;

   weath_unit = 10;
   rand_factor = 2;
   climate_factor = 1;
   neigh_factor = 3;
   max_vector = weath_unit * 3;

   for( wear = 0; wear < MAX_WEAR; wear++ )
      for( x = 0; x < MAX_LAYERS; x++ )
         save_equipment[wear][x] = NULL;

   /*
    * Init random number generator.
    */
   log_string( "Initializing random number generator" );
   init_mm(  );

   log_string( "Setting Astral Walk target room" );
   astral_target = number_range( 4350, 4449 );  /* Added by Samson for Astral Walk spell */

   /*
    * Set time and weather.
    */
   {
      long lhour, lday, lmonth;

      log_string( "Setting time and weather" );

      if( !load_timedata(  ) )   /* Loads time from stored file if TRUE - Samson 1-21-99 */
      {
         boot_log( "Resetting mud time based on current system time." );
         lhour = ( current_time - 650336715 ) / ( sysdata.pulsetick / sysdata.pulsepersec );
         time_info.hour = lhour % sysdata.hoursperday;
         lday = lhour / sysdata.hoursperday;
         time_info.day = lday % sysdata.dayspermonth;
         lmonth = lday / sysdata.dayspermonth;
         time_info.month = lmonth % sysdata.monthsperyear;
         time_info.year = lmonth / sysdata.monthsperyear;
      }

      if( time_info.hour < sysdata.hoursunrise )
         time_info.sunlight = SUN_DARK;
      else if( time_info.hour < sysdata.hourdaybegin )
         time_info.sunlight = SUN_RISE;
      else if( time_info.hour < sysdata.hoursunset )
         time_info.sunlight = SUN_LIGHT;
      else if( time_info.hour < sysdata.hournightbegin )
         time_info.sunlight = SUN_SET;
      else
         time_info.sunlight = SUN_DARK;
   }

   log_string( "Loading holiday chart..." ); /* Samson 5-13-99 */
   load_holidays(  );

   /*
    * Assign gsn's for skills which need them.
    */
   log_string( "Assigning gsn's" );
   assign_gsn_data(  );

   log_string( "Loading DNS cache..." );  /* Samson 1-30-02 */
   load_dns(  );

   /*
    * Read in all the area files.
    */
   {
      FILE *fpList;

      log_string( "Reading in area files..." );
      if( !( fpList = fopen( AREA_LIST, "r" ) ) )
      {
         perror( AREA_LIST );
         shutdown_mud( "Boot_db: Unable to open area list" );
         exit( 1 );
      }

      for( ;; )
      {
         mudstrlcpy( strArea, fread_word( fpList ), MIL );
         if( strArea[0] == '$' )
            break;

         set_alarm( 45 );
         alarm_section = "boot_db: read area files";
         load_area_file( last_area, strArea, FALSE );
         set_alarm( 0 );
      }
      FCLOSE( fpList );
   }

   log_string( "Loading ships..." );
   load_ships(  );

   load_runes(  );

   /*
    *   initialize supermob.
    *    must be done before reset_area!
    *
    */
   init_supermob(  );
   /*
    * Has some bad memory bugs in it
    */
   /*
    * Fix up exits.
    * Declare db booting over.
    * Reset all areas once.
    * Load up the notes file.
    */
   {
      log_string( "Fixing exits..." );
      fix_exits(  );

      load_clans(  );

      load_deity(  );

      load_equipment_totals( fCopyOver ); /* Samson 10-16-98 - scans pfiles for rares */

      log_string( "Loading corpses..." );
      load_corpses(  );

      fBootDb = FALSE;

      if( fCopyOver == TRUE )
      {
         log_string( "Loading world state..." );
         load_world( supermob );
      }

      log_string( "Initializing area reset events..." );
      /*
       * area_update( ); 
       */
      {
         AREA_DATA *area;

         /*
          * Putting some random fuzz on this to scatter the times around more 
          */
         for( area = first_area; area; area = area->next )
         {
            reset_area( area );
            area->last_resettime = current_time;
            add_event( number_range( ( area->reset_frequency * 60 ) / 2, 3 * ( area->reset_frequency * 60 ) / 2 ),
                       ev_area_reset, area );
         }
      }

      log_string( "Loading auction sales list..." );  /* Samson 6-24-99 */
      load_sales(  );
      log_string( "Loading auction houses..." );   /* Samson 6-20-99 */
      load_aucvaults(  );

      /*
       * Clan/Guild shopkeepers - Samson 7-16-00 
       */
      log_string( "Loading clan/guild shops..." );
      load_shopkeepers(  );

      log_string( "Loading prototype area files..." );
      load_buildlist(  );

      log_string( "Fixing prototype zone exits..." );
      fix_exits(  );

      log_string( "Loading boards..." );
      load_boards(  );

      load_councils(  );

      log_string( "Loading watches..." );
      load_watchlist(  );

      log_string( "Loading bans..." );
      load_banlist(  );

      log_string( "Loading auth namelist..." );
      load_auth_list(  );
      save_auth_list(  );

      log_string( "Loading slay table..." ); /* Online slay table - Samson 8-3-98 */
      load_slays(  );

      log_string( "Loading Immortal Hosts..." );
      load_imm_host(  );

      log_string( "Loading Projects..." );
      load_projects(  );

      log_string( "Loading MXP object commands..." );
      load_mxpobj_cmds(  );

      load_quotes(  );

      /*
       * Morphs MUST be loaded after Class and race tables are set up --Shaddai 
       */
      log_string( "Loading Morphs..." );
      load_morphs(  );
      MOBtrigger = TRUE;
   }

   /*
    * Initialize area weather data 
    */
   load_weatherdata(  );
   init_area_weather(  );

   return;
}

AREA_DATA *create_area( void )
{
   AREA_DATA *pArea;

   CREATE( pArea, AREA_DATA, 1 );
   pArea->first_room = pArea->last_room = NULL;
   pArea->age = 15;
   pArea->reset_frequency = 5;
   pArea->nplayer = 0;
   pArea->low_vnum = 0;
   pArea->hi_vnum = 0;
   pArea->low_soft_range = 0;
   pArea->hi_soft_range = MAX_LEVEL;
   pArea->low_hard_range = 0;
   pArea->hi_hard_range = MAX_LEVEL;
   pArea->continent = 0;

   /*
    * initialize weather data - FB 
    */
   CREATE( pArea->weather, WEATHER_DATA, 1 );
   pArea->weather->temp = 0;
   pArea->weather->precip = 0;
   pArea->weather->wind = 0;
   pArea->weather->temp_vector = 0;
   pArea->weather->precip_vector = 0;
   pArea->weather->wind_vector = 0;
   pArea->weather->climate_temp = 2;
   pArea->weather->climate_precip = 2;
   pArea->weather->climate_wind = 2;
   pArea->weather->first_neighbor = NULL;
   pArea->weather->last_neighbor = NULL;
   pArea->weather->echo = NULL;
   pArea->weather->echo_color = AT_GREY;
   pArea->version = 0;
   pArea->x = 0;
   pArea->y = 0;
   pArea->tg_nothing = 20;
   pArea->tg_gold = 74;
   pArea->tg_item = 85;
   pArea->tg_gem = 93;
   pArea->tg_scroll = 20;
   pArea->tg_potion = 50;
   pArea->tg_wand = 60;
   pArea->tg_armor = 75;
   LINK( pArea, first_area, last_area, next, prev );
   top_area++;
   return pArea;
};

/*
 * Load an 'area' header line.
 */
void load_area( FILE * fp )
{
   AREA_DATA *pArea;

   pArea = create_area(  );
   pArea->name = fread_string_nohash( fp );
   pArea->author = STRALLOC( "unknown" );
   pArea->filename = str_dup( strArea );

   return;
}

/* Load the version number of the area file if none exists, then it
 * is set to version 0 when #AREA is read in which is why we check for
 * the #AREA here.  --Shaddai
 */
CMDF do_areaconvert( CHAR_DATA * ch, char *argument );

void load_version( AREA_DATA * tarea, FILE * fp, char *filename )
{
   int aversion = fread_number( fp );

   if( aversion == 1000 )
   {
      FCLOSE( fp );
      do_areaconvert( NULL, filename );
      return;
   }

   if( !tarea )
   {
      bug( "%s", "Load_version: no #AREA seen yet." );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   tarea->version = aversion;
   return;
}

void load_vnums( AREA_DATA * tarea, FILE * fp )
{
   if( !tarea )
   {
      bug( "%s", "Load_vnums: no #AREA seen yet." );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   tarea->low_vnum = fread_number( fp );
   tarea->hi_vnum = fread_number( fp );

   /*
    * Protection against forgetting to raise the MaxVnum value before adding a new zone that would exceed it.
    * * Potentially dangerous if some blockhead makes insanely high vnums and then installs the area.
    */
   if( tarea->hi_vnum >= sysdata.maxvnum )
   {
      sysdata.maxvnum = tarea->hi_vnum + 1;
      bug( "MaxVnum value raised to %d to accomadate new zone.", sysdata.maxvnum );
      save_sysdata( sysdata );
   }
}

/*
 * Load an author section. Scryn 2/1/96
 */
void load_author( AREA_DATA * tarea, FILE * fp, char *filename )
{
   if( !tarea )
   {
      bug( "%s", "Load_author: no #AREA seen yet." );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   if( tarea->version < 2 )
   {
      log_string( "Smaug 1.02a or 1.4a area encountered. Attempting to pass to Area Convertor." );
      FCLOSE( fp );
      DISPOSE( tarea->name );
      DISPOSE( tarea->filename );
      STRFREE( tarea->author );
      DISPOSE( tarea->weather );
      UNLINK( tarea, first_area, last_area, next, prev );
      DISPOSE( tarea );
      top_area--;
      do_areaconvert( NULL, filename );
      return;
   }
   STRFREE( tarea->author );
   tarea->author = fread_string( fp );
   return;
}

/* Reset frequency - Samson 5-10-99 */
void load_resetfreq( AREA_DATA * tarea, FILE * fp )
{
   if( !tarea )
   {
      bug( "%s", "Load_resetfreq: no #AREA seen yet." );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   tarea->reset_frequency = fread_number( fp );
   tarea->age = tarea->reset_frequency;
   return;
}

/* Reset Message Load, Rennard */
void load_resetmsg( AREA_DATA * tarea, FILE * fp )
{
   if( !tarea )
   {
      bug( "%s", "Load_resetmsg: no #AREA seen yet." );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   DISPOSE( tarea->resetmsg );
   tarea->resetmsg = fread_string_nohash( fp );
   return;
}

/* Load a continent - Samson 9-16-00 */
void load_continent( AREA_DATA * tarea, FILE * fp )
{
   int value;

   if( !tarea )
   {
      bug( "%s", "Load_continent: no #AREA seen yet." );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   value = get_continent( fread_flagstring( fp ) );

   if( value < 0 || value > ACON_MAX )
   {
      tarea->continent = 0;
      bug( "%s", "load_continent: Invalid area continent, set to 'alsherok' by default." );
   }
   else
      tarea->continent = value;

   return;
}

/* Load area coordinates - Samson 12-25-00 */
void load_coords( AREA_DATA * tarea, FILE * fp )
{
   int x, y;

   if( !tarea )
   {
      bug( "%s", "Load_coords: no #AREA seen yet." );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   x = fread_number( fp );
   y = fread_number( fp );

   if( x < 0 || x >= MAX_X )
   {
      bug( "%s", "load_coords: Area has bad x coord - setting X to 0" );
      x = 0;
   }

   if( y < 0 || y >= MAX_Y )
   {
      bug( "%s", "load_coords: Area has bad y coord - setting Y to 0" );
      y = 0;
   }

   tarea->x = x;
   tarea->y = y;

   return;
}

/*
 * Load area flags. Narn, Mar/96 
 */
void load_flags( AREA_DATA * tarea, FILE * fp )
{
   char *areaflags = NULL;
   char flag[MIL];
   int value;

   if( !tarea )
   {
      bug( "%s", "Load_flags: no #AREA seen yet." );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   areaflags = fread_flagstring( fp );

   while( areaflags[0] != '\0' )
   {
      areaflags = one_argument( areaflags, flag );
      value = get_areaflag( flag );
      if( value < 0 || value > 31 )
         bug( "Unknown area flag: %s", flag );
      else
         SET_BIT( tarea->flags, 1 << value );
   }
   return;
}

/*
 * Load a help section.
 */
void load_helps( AREA_DATA * tarea, FILE * fp )
{
   HELP_DATA *pHelp;

   for( ;; )
   {
      CREATE( pHelp, HELP_DATA, 1 );
      pHelp->level = fread_number( fp );
      pHelp->keyword = fread_string_nohash( fp );
      if( pHelp->keyword[0] == '$' )
      {
         free_help( pHelp );
         break;
      }
      pHelp->text = fread_string_nohash( fp );
      if( pHelp->keyword[0] == '\0' )
      {
         free_help( pHelp );
         continue;
      }
      add_help( pHelp );
   }
   return;
}

/* This routine reads in scripts of MUDprograms from a file */
int mprog_name_to_type( char *name )
{
   if( !str_cmp( name, "in_file_prog" ) )
      return IN_FILE_PROG;
   if( !str_cmp( name, "act_prog" ) )
      return ACT_PROG;
   if( !str_cmp( name, "speech_prog" ) )
      return SPEECH_PROG;
   if( !str_cmp( name, "speech_and_prog" ) )
      return SPEECH_AND_PROG;
   if( !str_cmp( name, "rand_prog" ) )
      return RAND_PROG;
   if( !str_cmp( name, "fight_prog" ) )
      return FIGHT_PROG;
   if( !str_cmp( name, "hitprcnt_prog" ) )
      return HITPRCNT_PROG;
   if( !str_cmp( name, "death_prog" ) )
      return DEATH_PROG;
   if( !str_cmp( name, "entry_prog" ) )
      return ENTRY_PROG;
   if( !str_cmp( name, "greet_prog" ) )
      return GREET_PROG;
   if( !str_cmp( name, "all_greet_prog" ) )
      return ALL_GREET_PROG;
   if( !str_cmp( name, "give_prog" ) )
      return GIVE_PROG;
   if( !str_cmp( name, "bribe_prog" ) )
      return BRIBE_PROG;
   if( !str_cmp( name, "time_prog" ) )
      return TIME_PROG;
   if( !str_cmp( name, "month_prog" ) )
      return MONTH_PROG;
   if( !str_cmp( name, "hour_prog" ) )
      return HOUR_PROG;
   if( !str_cmp( name, "wear_prog" ) )
      return WEAR_PROG;
   if( !str_cmp( name, "remove_prog" ) )
      return REMOVE_PROG;
   if( !str_cmp( name, "sac_prog" ) )
      return SAC_PROG;
   if( !str_cmp( name, "look_prog" ) )
      return LOOK_PROG;
   if( !str_cmp( name, "exa_prog" ) )
      return EXA_PROG;
   if( !str_cmp( name, "zap_prog" ) )
      return ZAP_PROG;
   if( !str_cmp( name, "get_prog" ) )
      return GET_PROG;
   if( !str_cmp( name, "drop_prog" ) )
      return DROP_PROG;
   if( !str_cmp( name, "damage_prog" ) )
      return DAMAGE_PROG;
   if( !str_cmp( name, "repair_prog" ) )
      return REPAIR_PROG;
   if( !str_cmp( name, "greet_prog" ) )
      return GREET_PROG;
   if( !str_cmp( name, "randiw_prog" ) )
      return RANDIW_PROG;
   if( !str_cmp( name, "speechiw_prog" ) )
      return SPEECHIW_PROG;
   if( !str_cmp( name, "pull_prog" ) )
      return PULL_PROG;
   if( !str_cmp( name, "push_prog" ) )
      return PUSH_PROG;
   if( !str_cmp( name, "sleep_prog" ) )
      return SLEEP_PROG;
   if( !str_cmp( name, "rest_prog" ) )
      return REST_PROG;
   if( !str_cmp( name, "rfight_prog" ) )
      return FIGHT_PROG;
   if( !str_cmp( name, "enter_prog" ) )
      return ENTRY_PROG;
   if( !str_cmp( name, "leave_prog" ) )
      return LEAVE_PROG;
   if( !str_cmp( name, "rdeath_prog" ) )
      return DEATH_PROG;
   if( !str_cmp( name, "script_prog" ) )
      return SCRIPT_PROG;
   if( !str_cmp( name, "use_prog" ) )
      return USE_PROG;
   if( !str_cmp( name, "keyword_prog" ) )
      return KEYWORD_PROG;
   return ( ERROR_PROG );
}

/* Removal of this function constitutes a license violation */
CMDF do_basereport( CHAR_DATA * ch, char *argument )
{
   ch_printf( ch, "&RCodebase revision: %s %s - %s\n\r", CODENAME, CODEVERSION, COPYRIGHT );
   send_to_char( "&YContributors: Samson, Dwip, Whir, Cyberfox, Karangi, Rathian, Cam, Raine, and Tarl.\n\r", ch );
   send_to_char( "&BDevelopment site: www.alsherok.net\n\r", ch );
   send_to_char( "&GThis function is included as a means to verify license compliance.\n\r", ch );
   send_to_char( "Removal is a violation of your license.\n\r", ch );
   send_to_char( "Copies of AFKMud beginning with 1.4 as of December 28, 2002 include this.\n\r", ch );
   send_to_char( "Copies found to be running without this command will be subject to a violation report.\n\r", ch );
}

void mobprog_file_read( MOB_INDEX_DATA *mob, char *f )
{
   MPROG_DATA *mprg = NULL;
   char MUDProgfile[256];
   FILE *progfile;
   char letter;

   snprintf( MUDProgfile, 256, "%s%s", PROG_DIR, f );

   if( !( progfile = fopen( MUDProgfile, "r" ) ) )
   {
      bug( "%s: couldn't open mudprog file", __FUNCTION__ );
      return;
   }

   for( ; ; )
   {
      letter = fread_letter( progfile );

      if( letter == '|' )
         break;

      if( letter != '>' )
      {
         bug( "%s: MUDPROG char", __FUNCTION__ );
         break;
      }

      CREATE( mprg, MPROG_DATA, 1 );
      mprg->type = mprog_name_to_type( fread_word( progfile ) );
      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: mudprog file type error", __FUNCTION__ );
            DISPOSE( mprg );
            continue;

         case IN_FILE_PROG:
            bug( "%s: Nested file programs are not allowed.", __FUNCTION__ );
            DISPOSE( mprg );
            continue;

         default:
            mprg->arglist = fread_string( progfile );
            mprg->comlist = fread_string( progfile );
            mprg->fileprog = TRUE;
            xSET_BIT( mob->progtypes, mprg->type );
            mprg->next = mob->mudprogs;
            mob->mudprogs = mprg;
            break;
      }
   }
   FCLOSE( progfile );
   return;
}

void mprog_read_programs( FILE *fp, MOB_INDEX_DATA *mob )
{
   MPROG_DATA *mprg;
   char letter;
   char *word;

   for( ; ; )
   {
      letter = fread_letter( fp );

      if( letter == '|' )
         return;

      if( letter != '>' )
      {
         bug( "%s: vnum %d MUDPROG char", __FUNCTION__, mob->vnum );
         exit( 1 );
      }
      CREATE( mprg, MPROG_DATA, 1 );
      mprg->next = mob->mudprogs;
      mob->mudprogs = mprg;

      word = fread_word( fp );
      mprg->type = mprog_name_to_type( word );

      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: vnum %d MUDPROG type.", __FUNCTION__, mob->vnum );
            exit( 1 );

         case IN_FILE_PROG:
            mprg->arglist = fread_string( fp );
            mprg->fileprog = FALSE;
            mobprog_file_read( mob, mprg->arglist );
            break;

         default:
            xSET_BIT( mob->progtypes, mprg->type );
            mprg->fileprog = FALSE;
            mprg->arglist = fread_string( fp );
            mprg->comlist = fread_string( fp );
            break;
      }
   }
   return;
}

/*
 * Load a mob section.
 */
void load_mobiles( AREA_DATA * tarea, FILE * fp )
{
   MOB_INDEX_DATA *pMobIndex;
   char *ln;
   int x1, x2, x3, x4, x5, x6, x7, x8;

   if( !tarea )
   {
      bug( "%s", "Load_mobiles: no #AREA seen yet." );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   for( ;; )
   {
      int vnum, iHash;
      char letter;
      bool oldmob, tmpBootDb;

      letter = fread_letter( fp );
      if( letter != '#' )
      {
         bug( "%s", "Load_mobiles: # not found." );
         if( fBootDb )
         {
            shutdown_mud( "# not found" );
            exit( 1 );
         }
         else
            return;
      }

      vnum = fread_number( fp );
      if( vnum == 0 )
         break;

      tmpBootDb = fBootDb;
      fBootDb = FALSE;
      if( get_mob_index( vnum ) )
      {
         if( tmpBootDb )
         {
            bug( "Load_mobiles: vnum %d duplicated.", vnum );
            shutdown_mud( "duplicate vnum" );
            exit( 1 );
         }
         else
         {
            pMobIndex = get_mob_index( vnum );
            log_printf_plus( LOG_BUILD, sysdata.build_level, "Cleaning mobile: %d", vnum );
            clean_mob( pMobIndex );
            oldmob = TRUE;
         }
      }
      else
      {
         oldmob = FALSE;
         CREATE( pMobIndex, MOB_INDEX_DATA, 1 );
      }
      fBootDb = tmpBootDb;

      pMobIndex->vnum = vnum;
      if( fBootDb )
      {
         if( !tarea->low_vnum )
            tarea->low_vnum = vnum;
         if( vnum > tarea->hi_vnum )
            tarea->hi_vnum = vnum;
      }
      pMobIndex->area = tarea;
      pMobIndex->player_name = fread_string( fp );
      pMobIndex->short_descr = fread_string( fp );
      pMobIndex->long_descr = fread_string( fp );

      char *desc = fread_flagstring( fp );
      if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
      {
         pMobIndex->chardesc = STRALLOC( desc );
         if( str_prefix( "namegen", desc ) )
            pMobIndex->chardesc[0] = UPPER( pMobIndex->chardesc[0] );
      }

      if( pMobIndex->long_descr != NULL && str_prefix( "namegen", pMobIndex->long_descr ) )
         pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );

      char *actflags = NULL;
      char *affectflags = NULL;
      char flag[MIL];
      int value;

      actflags = fread_flagstring( fp );

      while( actflags[0] != '\0' )
      {
         actflags = one_argument( actflags, flag );
         value = get_actflag( flag );
         if( value < 0 || value > MAX_BITS )
            bug( "Unknown actflag: %s\n\r", flag );
         else
            SET_ACT_FLAG( pMobIndex, value );
      }

      affectflags = fread_flagstring( fp );

      while( affectflags[0] != '\0' )
      {
         affectflags = one_argument( affectflags, flag );
         value = get_aflag( flag );
         if( value < 0 || value > MAX_BITS )
            bug( "Unknown affectflag: %s\n\r", flag );
         else
            SET_AFFECTED( pMobIndex, value );
      }

      SET_ACT_FLAG( pMobIndex, ACT_IS_NPC );
      pMobIndex->pShop = NULL;
      pMobIndex->rShop = NULL;

      float x9 = 0;
      ln = fread_line( fp );
      x1 = x2 = x3 = x4 = x5 = x8 = 0;
      x6 = 150;
      x7 = 100;
      sscanf( ln, "%d %d %d %d %d %d %d %f", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x9 );

      pMobIndex->alignment = x1;
      pMobIndex->gold = x2;
      pMobIndex->height = x4;
      pMobIndex->weight = x5;
      pMobIndex->max_move = x6;
      pMobIndex->max_mana = x7;
      pMobIndex->numattacks = x9;

      if( pMobIndex->max_move < 1 )
         pMobIndex->max_move = 150;

      if( pMobIndex->max_mana < 1 )
         pMobIndex->max_mana = 100;

      /*
       * To catch old area file mobs and force those with preset gold amounts to use the treasure generator instead 
       */
      if( tarea->version < 17 && pMobIndex->gold > 0 )
         pMobIndex->gold = -1;

      pMobIndex->level = fread_number( fp );
      pMobIndex->mobthac0 = fread_number( fp );
      pMobIndex->ac = fread_number( fp );
      pMobIndex->hitnodice = pMobIndex->level;
      pMobIndex->hitsizedice = 8;
      pMobIndex->hitplus = fread_number( fp );
      pMobIndex->damnodice = fread_number( fp );
      /*
       * 'd'      
       */ fread_letter( fp );
      pMobIndex->damsizedice = fread_number( fp );
      /*
       * '+'      
       */ fread_letter( fp );
      pMobIndex->damplus = fread_number( fp );

      char *speaks = NULL;
      char *speaking = NULL;

      speaks = fread_flagstring( fp );

      while( speaks[0] != '\0' )
      {
         speaks = one_argument( speaks, flag );
         value = get_langnum( flag );
         if( value == -1 )
            bug( "Unknown speaks language: %s\n\r", flag );
         else
            SET_BIT( pMobIndex->speaks, 1 << value );
      }

      speaking = fread_flagstring( fp );

      while( speaking[0] != '\0' )
      {
         speaking = one_argument( speaking, flag );
         value = get_langnum( flag );
         if( value == -1 )
            bug( "Unknown speaking language: %s\n\r", flag );
         else
            SET_BIT( pMobIndex->speaking, 1 << value );
      }
      if( !pMobIndex->speaks )
         pMobIndex->speaks = LANG_COMMON;
      if( !pMobIndex->speaking )
         pMobIndex->speaking = LANG_COMMON;

      int position = get_npc_position( fread_flagstring( fp ) );
      if( position < 0 || position >= POS_MAX )
      {
         bug( "load_mobiles: vnum %d: Mobile in invalid position! Defaulting to standing.", vnum );
         position = POS_STANDING;
      }
      pMobIndex->position = position;

      position = get_npc_position( fread_flagstring( fp ) );

      if( position < 0 || position >= POS_MAX )
      {
         bug( "load_mobiles: vnum %d: Mobile in invalid default position! Defaulting to standing.", vnum );
         position = POS_STANDING;
      }
      pMobIndex->defposition = position;

      int sex = get_npc_sex( fread_flagstring( fp ) );

      if( sex < 0 || sex >= SEX_MAX )
      {
         bug( "load_mobiles: vnum %d: Mobile has invalid sex! Defaulting to neuter.", vnum );
         sex = SEX_NEUTRAL;
      }
      pMobIndex->sex = sex;

      int Class = -1;
      int race = -1;

      race = get_npc_race( fread_flagstring( fp ) );

      if( race < 0 || race >= MAX_NPC_RACE )
      {
         bug( "load_mobiles: vnum %d: Mob has invalid race! Defaulting to monster.", vnum );
         race = get_npc_race( "monster" );
      }

      pMobIndex->race = race;

      Class = get_npc_class( fread_flagstring( fp ) );

      if( Class < 0 || Class >= MAX_NPC_CLASS )
      {
         bug( "%s: vnum %d: Mob has invalid Class! Defaulting to warrior.", __FUNCTION__, vnum );
         Class = get_npc_class( "warrior" );
      }

      pMobIndex->Class = Class;

      char *bodyparts = NULL;
      char *resist = NULL;
      char *immune = NULL;
      char *suscep = NULL;
      char *absorb = NULL;
      char *attacks = NULL;
      char *defenses = NULL;

      bodyparts = fread_flagstring( fp );

      while( bodyparts[0] != '\0' )
      {
         bodyparts = one_argument( bodyparts, flag );
         value = get_partflag( flag );
         if( value < 0 || value > 31 )
            bug( "Unknown bodypart: %s", flag );
         else
            SET_BIT( pMobIndex->xflags, 1 << value );
      }

      resist = fread_flagstring( fp );

      while( resist[0] != '\0' )
      {
         resist = one_argument( resist, flag );
         value = get_risflag( flag );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "Unknown RIS flag (R): %s", flag );
         else
            SET_RESIS( pMobIndex, value );
      }

      immune = fread_flagstring( fp );

      while( immune[0] != '\0' )
      {
         immune = one_argument( immune, flag );
         value = get_risflag( flag );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "Unknown RIS flag (I): %s", flag );
         else
            SET_IMMUNE( pMobIndex, value );
      }

      suscep = fread_flagstring( fp );

      while( suscep[0] != '\0' )
      {
         suscep = one_argument( suscep, flag );
         value = get_risflag( flag );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "Unknown RIS flag (S): %s", flag );
         else
            SET_SUSCEP( pMobIndex, value );
      }

      absorb = fread_flagstring( fp );

      while( absorb[0] != '\0' )
      {
         absorb = one_argument( absorb, flag );
         value = get_risflag( flag );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "Unknown RIS flag (A): %s", flag );
         else
            SET_ABSORB( pMobIndex, value );
      }

      attacks = fread_flagstring( fp );

      while( attacks[0] != '\0' )
      {
         attacks = one_argument( attacks, flag );
         value = get_attackflag( flag );
         if( value < 0 || value > MAX_BITS )
            bug( "Unknown attackflag: %s\n\r", flag );
         else
            SET_ATTACK( pMobIndex, value );
      }

      defenses = fread_flagstring( fp );

      while( defenses[0] != '\0' )
      {
         defenses = one_argument( defenses, flag );
         value = get_defenseflag( flag );
         if( value < 0 || value > MAX_BITS )
            bug( "Unknown defenseflag: %s\n\r", flag );
         else
            SET_DEFENSE( pMobIndex, value );
      }

      letter = fread_letter( fp );
      if( letter == '>' )
      {
         ungetc( letter, fp );
         mprog_read_programs( fp, pMobIndex );
      }
      else
         ungetc( letter, fp );

      if( !oldmob )
      {
         iHash = vnum % MAX_KEY_HASH;
         pMobIndex->next = mob_index_hash[iHash];
         mob_index_hash[iHash] = pMobIndex;
         top_mob_index++;
      }
   }

   return;
}

void objprog_file_read( OBJ_INDEX_DATA *obj, char *f )
{
   MPROG_DATA *mprg = NULL;
   char MUDProgfile[256];
   FILE *progfile;
   char letter;

   snprintf( MUDProgfile, 256, "%s%s", PROG_DIR, f );

   if( !( progfile = fopen( MUDProgfile, "r" ) ) )
   {
      bug( "%s: couldn't open mudprog file", __FUNCTION__ );
      return;
   }

   for( ; ; )
   {
      letter = fread_letter( progfile );

      if( letter == '|' )
         break;

      if( letter != '>' )
      {
         bug( "%s: MUDPROG char", __FUNCTION__ );
         break;
      }

      CREATE( mprg, MPROG_DATA, 1 );
      mprg->type = mprog_name_to_type( fread_word( progfile ) );
      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: mudprog file type error", __FUNCTION__ );
            DISPOSE( mprg );
            continue;

         case IN_FILE_PROG:
            bug( "%s: Nested file programs are not allowed.", __FUNCTION__ );
            DISPOSE( mprg );
            continue;

         default:
            mprg->arglist = fread_string( progfile );
            mprg->comlist = fread_string( progfile );
            mprg->fileprog = TRUE;
            xSET_BIT( obj->progtypes, mprg->type );
            mprg->next = obj->mudprogs;
            obj->mudprogs = mprg;
            break;
      }
   }
   FCLOSE( progfile );
   return;
}

void oprog_read_programs( FILE *fp, OBJ_INDEX_DATA *obj )
{
   MPROG_DATA *mprg;
   char letter;
   char *word;

   for( ; ; )
   {
      letter = fread_letter( fp );

      if( letter == '|' )
         return;

      if( letter != '>' )
      {
         bug( "%s: vnum %d MUDPROG char", __FUNCTION__, obj->vnum );
         exit( 1 );
      }
      CREATE( mprg, MPROG_DATA, 1 );
      mprg->next = obj->mudprogs;
      obj->mudprogs = mprg;

      word = fread_word( fp );
      mprg->type = mprog_name_to_type( word );

      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: vnum %d MUDPROG type.", __FUNCTION__, obj->vnum );
            exit( 1 );

         case IN_FILE_PROG:
            mprg->arglist = fread_string( fp );
            mprg->fileprog = FALSE;
            objprog_file_read( obj, mprg->arglist );
            break;

         default:
            xSET_BIT( obj->progtypes, mprg->type );
            mprg->fileprog = FALSE;
            mprg->arglist = fread_string( fp );
            mprg->comlist = fread_string( fp );
            break;
      }
   }
   return;
}

/*
 * Load an obj section.
 */
void load_objects( AREA_DATA * tarea, FILE * fp )
{
   OBJ_INDEX_DATA *pObjIndex;
   char letter;
   char *ln;
   char temp[3][MSL];
   int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11;

   if( !tarea )
   {
      bug( "%s: no #AREA seen yet.", __FUNCTION__ );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   for( ;; )
   {
      int vnum, iHash;
      bool tmpBootDb, oldobj;

      letter = fread_letter( fp );
      if( letter != '#' )
      {
         bug( "%s: # not found.", __FUNCTION__ );
         if( fBootDb )
         {
            shutdown_mud( "# not found" );
            exit( 1 );
         }
         else
            return;
      }

      vnum = fread_number( fp );
      if( vnum == 0 )
         break;

      tmpBootDb = fBootDb;
      fBootDb = FALSE;
      if( get_obj_index( vnum ) )
      {
         if( tmpBootDb )
         {
            bug( "%s: vnum %d duplicated.", __FUNCTION__, vnum );
            shutdown_mud( "duplicate vnum" );
            exit( 1 );
         }
         else
         {
            pObjIndex = get_obj_index( vnum );
            log_printf_plus( LOG_BUILD, sysdata.build_level, "Cleaning object: %d", vnum );
            clean_obj( pObjIndex );
            oldobj = TRUE;
         }
      }
      else
      {
         oldobj = FALSE;
         CREATE( pObjIndex, OBJ_INDEX_DATA, 1 );
      }
      fBootDb = tmpBootDb;

      pObjIndex->vnum = vnum;
      if( fBootDb )
      {
         if( !tarea->low_vnum )
            tarea->low_vnum = vnum;
         if( vnum > tarea->hi_vnum )
            tarea->hi_vnum = vnum;
      }
      pObjIndex->area = tarea;
      pObjIndex->name = fread_string( fp );
      pObjIndex->short_descr = fread_string( fp );
      {
         char *desc = fread_flagstring( fp );
         if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
            pObjIndex->objdesc = STRALLOC( desc );
      }
      {
         char *desc2 = fread_flagstring( fp );
         if( desc2 && desc2[0] != '\0' && str_cmp( desc2, "(null)" ) )
            pObjIndex->action_desc = STRALLOC( desc2 );
      }
      if( pObjIndex->objdesc != NULL )
         pObjIndex->objdesc[0] = UPPER( pObjIndex->objdesc[0] );
      {
         int value = -1;

         value = get_otype( fread_flagstring( fp ) );

         if( value < 0 )
         {
            bug( "load_objects: vnum %d: Object has invalid type! Defaulting to trash.", vnum );
            value = get_otype( "trash" );
         }

         pObjIndex->item_type = value;

         {
            char *eflags = NULL;
            char *wflags = NULL;
            char flag[MIL];

            eflags = fread_flagstring( fp );

            while( eflags[0] != '\0' )
            {
               eflags = one_argument( eflags, flag );
               value = get_oflag( flag );
               if( value < 0 || value > MAX_BITS )
                  bug( "Unknown object extraflag: %s\n\r", flag );
               else
                  SET_OBJ_FLAG( pObjIndex, value );
            }

            wflags = fread_flagstring( fp );

            while( wflags[0] != '\0' )
            {
               wflags = one_argument( wflags, flag );
               value = get_wflag( flag );
               if( value < 0 || value > 31 )
                  bug( "Unknown wear flag: %s", flag );
               else
                  SET_WEAR_FLAG( pObjIndex, 1 << value );
            }
         }
      }

      {
         char *magflags = NULL;
         char flag[MIL];
         int value;

         magflags = fread_flagstring( fp );

         while( magflags[0] != '\0' )
         {
            magflags = one_argument( magflags, flag );
            value = get_magflag( flag );
            if( value < 0 || value > 31 )
               bug( "Unknown magic flag: %s", flag );
            else
               SET_MAGIC_FLAG( pObjIndex, 1 << value );
         }
      }

      ln = fread_line( fp );
      x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = x9 = x10 = x11 = 0;
      sscanf( ln, "%d %d %d %d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8, &x9, &x10, &x11 );

      if( x1 == 0 && ( pObjIndex->item_type == ITEM_WEAPON || pObjIndex->item_type == ITEM_MISSILE_WEAPON ) )
      {
         x1 = sysdata.initcond;
         x7 = x1;
      }

      if( x1 == 0 && pObjIndex->item_type == ITEM_PROJECTILE )
      {
         x1 = sysdata.initcond;
         x6 = x1;
      }

      pObjIndex->value[0] = x1;
      pObjIndex->value[1] = x2;
      pObjIndex->value[2] = x3;
      pObjIndex->value[3] = x4;
      pObjIndex->value[4] = x5;
      pObjIndex->value[5] = x6;
      pObjIndex->value[6] = x7;
      pObjIndex->value[7] = x8;
      pObjIndex->value[8] = x9;
      pObjIndex->value[9] = x10;
      pObjIndex->value[10] = x11;

      ln = fread_line( fp );
      x1 = x2 = x3 = x5 = 0;
      x4 = 9999;
      temp[0][0] = '\0';
      temp[1][0] = '\0';
      temp[2][0] = '\0';
      sscanf( ln, "%d %d %d %d %d %s %s %s", &x1, &x2, &x3, &x4, &x5, temp[0], temp[1], temp[2] );
      pObjIndex->weight = x1;
      pObjIndex->weight = UMAX( 1, pObjIndex->weight );
      pObjIndex->cost = x2;
      pObjIndex->rent = x3;
      pObjIndex->limit = x4;
      pObjIndex->layers = x5;

      if( !temp[0] || temp[0][0] == '\0' )
         pObjIndex->socket[0] = STRALLOC( "None" );
      else
         pObjIndex->socket[0] = STRALLOC( temp[0] );

      if( !temp[1] || temp[1][0] == '\0' )
         pObjIndex->socket[1] = STRALLOC( "None" );
      else
         pObjIndex->socket[1] = STRALLOC( temp[1] );

      if( !temp[2] || temp[2][0] == '\0' )
         pObjIndex->socket[2] = STRALLOC( "None" );
      else
         pObjIndex->socket[2] = STRALLOC( temp[2] );

      switch ( pObjIndex->item_type )
      {
         case ITEM_PILL:
         case ITEM_POTION:
         case ITEM_SCROLL:
            pObjIndex->value[1] = skill_lookup( fread_word( fp ) );
            pObjIndex->value[2] = skill_lookup( fread_word( fp ) );
            pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
            break;
         case ITEM_STAFF:
         case ITEM_WAND:
            pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
            break;
         case ITEM_SALVE:
            pObjIndex->value[4] = skill_lookup( fread_word( fp ) );
            pObjIndex->value[5] = skill_lookup( fread_word( fp ) );
            break;
      }

      for( ;; )
      {
         letter = fread_letter( fp );

         if( letter == 'A' )
         {
            AFFECT_DATA *paf;
            bool setaff = true;

            CREATE( paf, AFFECT_DATA, 1 );
            paf->location = APPLY_NONE;
            paf->type = -1;
            paf->duration = -1;
            paf->bit = 0;
            paf->modifier = 0;
            xCLEAR_BITS( paf->rismod );

            if( tarea->version < 20 )
            {
               char *aff = NULL;
               char *risa = NULL;
               char flag[MIL];
               int value;

               paf->location = fread_number( fp );

               if( paf->location == APPLY_WEAPONSPELL
                   || paf->location == APPLY_WEARSPELL
                   || paf->location == APPLY_REMOVESPELL
                   || paf->location == APPLY_STRIPSN
                   || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
                  paf->modifier = slot_lookup( fread_number( fp ) );
               else if( paf->location == APPLY_AFFECT )
               {
                  paf->modifier = fread_number( fp );
                  aff = flag_string( paf->modifier, a_flags );
                  value = get_aflag( aff );
                  if( value < 0 || value >= MAX_AFFECTED_BY )
                  {
                     bug( "%s: Unsupportable value for affect flag: %s", __FUNCTION__, aff );
                     setaff = false;
                  }
                  else
                  {
                     value++;
                     paf->modifier = value;
                  }
               }
               else if( paf->location == APPLY_RESISTANT
                        || paf->location == APPLY_IMMUNE
                        || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
               {
                  value = fread_number( fp );
                  risa = flag_string( value, ris_flags );

                  while( risa[0] != '\0' )
                  {
                     risa = one_argument( risa, flag );
                     value = get_risflag( flag );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "%s: Unsupportable value for RISA flag: %s", __FUNCTION__, flag );
                     else
                        xSET_BIT( paf->rismod, value );
                  }
               }
               else
                  paf->modifier = fread_number( fp );
            }
            else
            {
               char *loc = NULL;
               char *aff = NULL;
               char *risa = NULL;
               char flag[MIL];
               int value;

               loc = fread_word( fp );
               value = get_atype( loc );
               if( value < 0 || value >= MAX_APPLY_TYPE )
               {
                  bug( "%s: Invalid apply type: %s", __FUNCTION__, loc );
                  setaff = false;
               }
               paf->location = value;

               if( paf->location == APPLY_WEAPONSPELL
                   || paf->location == APPLY_WEARSPELL
                   || paf->location == APPLY_REMOVESPELL
                   || paf->location == APPLY_STRIPSN
                   || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
                  paf->modifier = skill_lookup( fread_word( fp ) );
               else if( paf->location == APPLY_AFFECT )
               {
                  aff = fread_word( fp );
                  value = get_aflag( aff );
                  if( value < 0 || value >= MAX_AFFECTED_BY )
                  {
                     bug( "%s: Unsupportable value for affect flag: %s", __FUNCTION__, aff );
                     setaff = false;
                  }
                  else
                     paf->modifier = value;
               }
               else if( paf->location == APPLY_RESISTANT
                        || paf->location == APPLY_IMMUNE
                        || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
               {
                  risa = fread_flagstring( fp );

                  while( risa[0] != '\0' )
                  {
                     risa = one_argument( risa, flag );
                     value = get_risflag( flag );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "%s: Unsupportable value for RISA flag: %s", __FUNCTION__, flag );
                     else
                        xSET_BIT( paf->rismod, value );
                  }
               }
               else
                  paf->modifier = fread_number( fp );
            }

            if( !setaff )
               DISPOSE( paf );
            else
            {
               LINK( paf, pObjIndex->first_affect, pObjIndex->last_affect, next, prev );
               top_affect++;
            }
         }

         else if( letter == 'E' )
         {
            EXTRA_DESCR_DATA *ed;

            CREATE( ed, EXTRA_DESCR_DATA, 1 );
            ed->keyword = fread_string( fp );
            ed->extradesc = fread_string( fp );
            LINK( ed, pObjIndex->first_extradesc, pObjIndex->last_extradesc, next, prev );
            top_ed++;
         }
         else if( letter == '>' )
         {
            ungetc( letter, fp );
            oprog_read_programs( fp, pObjIndex );
         }
         else
         {
            ungetc( letter, fp );
            break;
         }
      }

      if( !oldobj )
      {
         iHash = vnum % MAX_KEY_HASH;
         pObjIndex->next = obj_index_hash[iHash];
         obj_index_hash[iHash] = pObjIndex;
         top_obj_index++;
      }
   }

   return;
}

/*
 * Load a reset section.
 */
void load_resets( AREA_DATA * tarea, FILE * fp )
{
   ROOM_INDEX_DATA *pRoomIndex = NULL;
   ROOM_INDEX_DATA *roomlist;
   bool not01 = FALSE;
   int count = 0;

   if( !tarea )
   {
      bug( "%s", "Load_resets: no #AREA seen yet." );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   if( !tarea->first_room )
   {
      bug( "%s: No #ROOMS section found. Cannot load resets.", __FUNCTION__ );
      if( fBootDb )
      {
         shutdown_mud( "No #ROOMS" );
         exit( 1 );
      }
      else
         return;
   }

   for( ;; )
   {
      EXIT_DATA *pexit;
      char letter;
      int extra, arg1, arg2, arg3;
      short arg4, arg5, arg6;

      if( ( letter = fread_letter( fp ) ) == 'S' )
         break;

      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }

      extra = fread_number( fp );
      if( letter == 'M' || letter == 'O' )
         extra = 0;
      arg1 = fread_number( fp );
      arg2 = fread_number( fp );
      arg3 = ( letter == 'G' || letter == 'R' ) ? 0 : fread_number( fp );
      arg4 = arg5 = arg6 = -1;
      if( tarea->version > 18 )
      {
         if( letter == 'O' || letter == 'M' )
         {
            arg4 = fread_short( fp );
            arg5 = fread_short( fp );
            arg6 = fread_short( fp );
         }
      }
      fread_to_eol( fp );
      ++count;

      /*
       * Validate parameters.
       * We're calling the index functions for the side effect.
       */
      switch ( letter )
      {
         default:
            bug( "%s: bad command '%c'.", __FUNCTION__, letter );
            if( fBootDb )
               boot_log( "%s: %s (%d) bad command '%c'.", __FUNCTION__, tarea->filename, count, letter );
            return;

         case 'M':
            if( get_mob_index( arg1 ) == NULL && fBootDb )
               boot_log( "%s: %s (%d) 'M': mobile %d doesn't exist.", __FUNCTION__, tarea->filename, count, arg1 );

            if( ( pRoomIndex = get_room_index( arg3 ) ) == NULL && fBootDb )
               boot_log( "%s: %s (%d) 'M': room %d doesn't exist.", __FUNCTION__, tarea->filename, count, arg3 );
            else
               add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );

            if( arg4 != -1 && ( arg4 < 0 || arg4 >= MAP_MAX ) )
               boot_log( "%s: %s (%d) 'M': Map %d does not exist.", __FUNCTION__, tarea->filename, count, arg4 );
            if( arg5 != -1 && ( arg5 < 0 || arg5 >= MAX_X ) )
               boot_log( "%s: %s (%d) 'M': X coordinate %d is out of range.", __FUNCTION__, tarea->filename, count, arg5 );
            if( arg6 != -1 && ( arg6 < 0 || arg6 >= MAX_Y ) )
               boot_log( "%s: %s (%d) 'M': Y coordinate %d is out of range.", __FUNCTION__, tarea->filename, count, arg6 );
            break;

         case 'O':
            if( get_obj_index( arg1 ) == NULL && fBootDb )
               boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, tarea->filename, count, letter, arg1 );

            if( ( pRoomIndex = get_room_index( arg3 ) ) == NULL && fBootDb )
               boot_log( "%s: %s (%d) '%c': room %d doesn't exist.", __FUNCTION__, tarea->filename, count, letter, arg3 );
            else
            {
               if( !pRoomIndex )
                  bug( "%s: Unable to add room reset - room not found.", __FUNCTION__ );
               else
                  add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            }
            if( arg4 != -1 && ( arg4 < 0 || arg4 >= MAP_MAX ) )
               boot_log( "%s: %s (%d) 'M': Map %d does not exist.", __FUNCTION__, tarea->filename, count, arg4 );
            if( arg5 != -1 && ( arg5 < 0 || arg5 >= MAX_X ) )
               boot_log( "%s: %s (%d) 'M': X coordinate %d is out of range.", __FUNCTION__, tarea->filename, count, arg5 );
            if( arg6 != -1 && ( arg6 < 0 || arg6 >= MAX_Y ) )
               boot_log( "%s: %s (%d) 'M': Y coordinate %d is out of range.", __FUNCTION__, tarea->filename, count, arg6 );
            break;

         case 'P':
            if( get_obj_index( arg1 ) == NULL && fBootDb )
               boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, tarea->filename, count, letter, arg1 );
            if( arg3 > 0 )
            {
               if( get_obj_index( arg3 ) == NULL && fBootDb )
                  boot_log( "%s: %s (%d) 'P': destination object %d doesn't exist.", __FUNCTION__, tarea->filename, count,
                            arg3 );
               if( extra > 1 )
                  not01 = true;
            }
            if( !pRoomIndex )
               bug( "%s: Unable to add room reset - room not found.", __FUNCTION__ );
            else
            {
               if( arg3 == 0 )
                  arg3 = OBJ_VNUM_DUMMYOBJ;  // This may look stupid, but for some reason it works.
               add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            }
            break;

         case 'G':
         case 'E':
            if( get_obj_index( arg1 ) == NULL && fBootDb )
               boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, tarea->filename, count, letter, arg1 );
            if( !pRoomIndex )
               bug( "%s: Unable to add room reset - room not found.", __FUNCTION__ );
            else
               add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            break;

         case 'T':
            if( IS_SET( extra, TRAP_OBJ ) )
               bug( "%s: Unable to add legacy object trap reset. Must be converted manually.", __FUNCTION__ );
            else
            {
               if( !( pRoomIndex = get_room_index( arg3 ) ) )
                  bug( "%s: Unable to add trap reset - room not found.", __FUNCTION__ );
               else
                  add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            }
            break;

         case 'H':
            bug( "%s: Unable to convert legacy hide reset. Must be converted manually.", __FUNCTION__ );
            break;

         case 'D':
            if( !( pRoomIndex = get_room_index( arg1 ) ) )
            {
               bug( "%s: 'D': room %d doesn't exist.", __FUNCTION__, arg1 );
               bug( "Reset: %c %d %d %d %d", letter, extra, arg1, arg2, arg3 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'D': room %d doesn't exist.", __FUNCTION__, tarea->filename, count, arg1 );
               break;
            }

            if( arg2 < 0 || arg2 > MAX_DIR + 1
                || !( pexit = get_exit( pRoomIndex, arg2 ) ) || !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
            {
               bug( "%s: 'D': exit %d not door.", __FUNCTION__, arg2 );
               bug( "Reset: %c %d %d %d %d", letter, extra, arg1, arg2, arg3 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'D': exit %d not door.", __FUNCTION__, tarea->filename, count, arg2 );
            }

            if( arg3 < 0 || arg3 > 2 )
            {
               bug( "%s: 'D': bad 'locks': %d.", __FUNCTION__, arg3 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'D': bad 'locks': %d.", __FUNCTION__, tarea->filename, count, arg3 );
            }
            add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            break;

         case 'R':
            if( !( pRoomIndex = get_room_index( arg1 ) ) && fBootDb )
               boot_log( "%s: %s (%d) 'R': room %d doesn't exist.", __FUNCTION__, tarea->filename, count, arg1 );
            else
               add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            if( arg2 < 0 || arg2 > 10 )
            {
               bug( "%s: 'R': bad exit %d.", __FUNCTION__, arg2 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'R': bad exit %d.", __FUNCTION__, tarea->filename, count, arg2 );
               break;
            }
            break;
      }
   }
   if( !not01 )
   {
      for( roomlist = tarea->first_room; roomlist; roomlist = roomlist->next_aroom )
         renumber_put_resets( roomlist );
   }
   return;
}

void roomprog_file_read( ROOM_INDEX_DATA *room, char *f )
{
   MPROG_DATA *mprg = NULL;
   char MUDProgfile[256];
   FILE *progfile;
   char letter;

   snprintf( MUDProgfile, 256, "%s%s", PROG_DIR, f );

   if( !( progfile = fopen( MUDProgfile, "r" ) ) )
   {
      bug( "%s: couldn't open mudprog file", __FUNCTION__ );
      return;
   }

   for( ; ; )
   {
      letter = fread_letter( progfile );

      if( letter == '|' )
         break;

      if( letter != '>' )
      {
         bug( "%s: MUDPROG char", __FUNCTION__ );
         break;
      }

      CREATE( mprg, MPROG_DATA, 1 );
      mprg->type = mprog_name_to_type( fread_word( progfile ) );
      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: mudprog file type error", __FUNCTION__ );
            DISPOSE( mprg );
            continue;

         case IN_FILE_PROG:
            bug( "%s: Nested file programs are not allowed.", __FUNCTION__ );
            DISPOSE( mprg );
            continue;

         default:
            mprg->arglist = fread_string( progfile );
            mprg->comlist = fread_string( progfile );
            mprg->fileprog = TRUE;
            xSET_BIT( room->progtypes, mprg->type );
            mprg->next = room->mudprogs;
            room->mudprogs = mprg;
            break;
      }
   }
   FCLOSE( progfile );
   return;
}

void rprog_read_programs( FILE *fp, ROOM_INDEX_DATA *room )
{
   MPROG_DATA *mprg;
   char letter;
   char *word;

   for( ; ; )
   {
      letter = fread_letter( fp );

      if( letter == '|' )
         return;

      if( letter != '>' )
      {
         bug( "%s: vnum %d MUDPROG char", __FUNCTION__, room->vnum );
         exit( 1 );
      }
      CREATE( mprg, MPROG_DATA, 1 );
      mprg->next = room->mudprogs;
      room->mudprogs = mprg;

      word = fread_word( fp );
      mprg->type = mprog_name_to_type( word );

      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: vnum %d MUDPROG type.", __FUNCTION__, room->vnum );
            exit( 1 );

         case IN_FILE_PROG:
            mprg->arglist = fread_string( fp );
            mprg->fileprog = FALSE;
            roomprog_file_read( room, mprg->arglist );
            break;

         default:
            xSET_BIT( room->progtypes, mprg->type );
            mprg->fileprog = FALSE;
            mprg->arglist = fread_string( fp );
            mprg->comlist = fread_string( fp );
            break;
      }
   }
   return;
}

void load_room_reset( ROOM_INDEX_DATA * room, FILE * fp )
{
   EXIT_DATA *pexit;
   char letter;
   int extra, arg1, arg2, arg3;
   short arg4, arg5, arg6;
   bool not01 = false;
   int count = 0;

   letter = fread_letter( fp );
   extra = fread_number( fp );
   if( letter == 'M' || letter == 'O' )
      extra = 0;
   arg1 = fread_number( fp );
   arg2 = fread_number( fp );
   arg3 = ( letter == 'G' || letter == 'R' ) ? 0 : fread_number( fp );
   arg4 = arg5 = arg6 = -1;
   if( room->area->version > 18 )
   {
      if( letter == 'O' || letter == 'M' )
      {
         arg4 = fread_short( fp );
         arg5 = fread_short( fp );
         arg6 = fread_short( fp );
      }
   }
   fread_to_eol( fp );
   ++count;

   /*
    * Validate parameters.
    * We're calling the index functions for the side effect.
    */
   switch ( letter )
   {
      default:
         bug( "%s: bad command '%c'.", __FUNCTION__, letter );
         if( fBootDb )
            boot_log( "%s: %s (%d) bad command '%c'.", __FUNCTION__, room->area->filename, count, letter );
         return;

      case 'M':
         if( get_mob_index( arg1 ) == NULL && fBootDb )
            boot_log( "%s: %s (%d) 'M': mobile %d doesn't exist.", __FUNCTION__, room->area->filename, count, arg1 );
         if( arg4 != -1 && ( arg4 < 0 || arg4 >= MAP_MAX ) )
            boot_log( "%s: %s (%d) 'M': Map %d does not exist.", __FUNCTION__, room->area->filename, count, arg4 );
         if( arg5 != -1 && ( arg5 < 0 || arg5 >= MAX_X ) )
            boot_log( "%s: %s (%d) 'M': X coordinate %d is out of range.", __FUNCTION__, room->area->filename, count, arg5 );
         if( arg6 != -1 && ( arg6 < 0 || arg6 >= MAX_Y ) )
            boot_log( "%s: %s (%d) 'M': Y coordinate %d is out of range.", __FUNCTION__, room->area->filename, count, arg6 );
         break;

      case 'O':
         if( get_obj_index( arg1 ) == NULL && fBootDb )
            boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, room->area->filename, count, letter,
                      arg1 );
         if( arg4 != -1 && ( arg4 < 0 || arg4 >= MAP_MAX ) )
            boot_log( "%s: %s (%d) 'M': Map %d does not exist.", __FUNCTION__, room->area->filename, count, arg4 );
         if( arg5 != -1 && ( arg5 < 0 || arg5 >= MAX_X ) )
            boot_log( "%s: %s (%d) 'M': X coordinate %d is out of range.", __FUNCTION__, room->area->filename, count, arg5 );
         if( arg6 != -1 && ( arg6 < 0 || arg6 >= MAX_Y ) )
            boot_log( "%s: %s (%d) 'M': Y coordinate %d is out of range.", __FUNCTION__, room->area->filename, count, arg6 );
         break;

      case 'P':
         if( get_obj_index( arg1 ) == NULL && fBootDb )
            boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, room->area->filename, count, letter,
                      arg1 );

         if( arg3 <= 0 )
            arg3 = OBJ_VNUM_DUMMYOBJ;  // This may look stupid, but for some reason it works.
         if( get_obj_index( arg3 ) == NULL && fBootDb )
            boot_log( "%s: %s (%d) 'P': destination object %d doesn't exist.", __FUNCTION__, room->area->filename, count,
                      arg3 );
         if( extra > 1 )
            not01 = true;
         break;

      case 'G':
      case 'E':
         if( get_obj_index( arg1 ) == NULL && fBootDb )
            boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, room->area->filename, count, letter,
                      arg1 );
         break;

      case 'T':
      case 'H':
         break;

      case 'D':
         if( arg2 < 0 || arg2 > MAX_DIR + 1 || !( pexit = get_exit( room, arg2 ) ) || !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
         {
            bug( "%s: 'D': exit %d not door.", __FUNCTION__, arg2 );
            bug( "Reset: %c %d %d %d %d", letter, extra, arg1, arg2, arg3 );
            if( fBootDb )
               boot_log( "%s: %s (%d) 'D': exit %d not door.", __FUNCTION__, room->area->filename, count, arg2 );
         }

         if( arg3 < 0 || arg3 > 2 )
         {
            bug( "%s: 'D': bad 'locks': %d.", __FUNCTION__, arg3 );
            if( fBootDb )
               boot_log( "%s: %s (%d) 'D': bad 'locks': %d.", __FUNCTION__, room->area->filename, count, arg3 );
         }
         break;

      case 'R':
         if( arg2 < 0 || arg2 > 10 )
         {
            bug( "%s: 'R': bad exit %d.", __FUNCTION__, arg2 );
            if( fBootDb )
               boot_log( "%s: %s (%d) 'R': bad exit %d.", __FUNCTION__, room->area->filename, count, arg2 );
            break;
         }
         break;
   }
   add_reset( room, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );

   if( !not01 )
      renumber_put_resets( room );
   return;
}

/*
 * Load a room section.
 */
void load_rooms( AREA_DATA * tarea, FILE * fp )
{
   ROOM_INDEX_DATA *pRoomIndex;
   char *ln;
   int area_number;

   if( !tarea )
   {
      bug( "%s", "Load_rooms: no #AREA seen yet." );
      shutdown_mud( "No #AREA" );
      exit( 1 );
   }

   tarea->first_room = tarea->last_room = NULL;

   for( ;; )
   {
      int vnum;
      char letter;
      int door;
      int iHash;
      bool tmpBootDb;
      bool oldroom;
      int x1, x2, x3, x4, x5, x6;

      letter = fread_letter( fp );
      if( letter != '#' )
      {
         bug( "%s", "Load_rooms: # not found." );
         if( fBootDb )
         {
            shutdown_mud( "# not found" );
            exit( 1 );
         }
         else
            return;
      }

      vnum = fread_number( fp );
      if( vnum == 0 )
         break;

      tmpBootDb = fBootDb;
      fBootDb = FALSE;
      if( get_room_index( vnum ) != NULL )
      {
         if( tmpBootDb )
         {
            bug( "Load_rooms: vnum %d duplicated.", vnum );
            shutdown_mud( "duplicate vnum" );
            exit( 1 );
         }
         else
         {
            pRoomIndex = get_room_index( vnum );
            log_printf_plus( LOG_BUILD, sysdata.build_level, "Cleaning room: %d", vnum );
            clean_room( pRoomIndex );
            oldroom = TRUE;
         }
      }
      else
      {
         oldroom = FALSE;
         CREATE( pRoomIndex, ROOM_INDEX_DATA, 1 );
         pRoomIndex->first_person = NULL;
         pRoomIndex->last_person = NULL;
         pRoomIndex->first_content = NULL;
         pRoomIndex->last_content = NULL;
      }

      fBootDb = tmpBootDb;
      pRoomIndex->area = tarea;
      pRoomIndex->vnum = vnum;
      pRoomIndex->first_extradesc = NULL;
      pRoomIndex->last_extradesc = NULL;

      if( fBootDb )
      {
         if( !tarea->low_vnum )
            tarea->low_vnum = vnum;
         if( vnum > tarea->hi_vnum )
            tarea->hi_vnum = vnum;
      }
      pRoomIndex->name = fread_string( fp );
      {
         char *ndesc = fread_flagstring( fp );
         if( ndesc && ndesc[0] != '\0' && str_cmp( ndesc, "(null)" ) )
            pRoomIndex->roomdesc = str_dup( ndesc );
      }
      /*
       * Check for NiteDesc's  -- Dracones 
       */
      if( tarea->version > 13 )
      {
         char *ndesc = fread_flagstring( fp );
         if( ndesc && ndesc[0] != '\0' && str_cmp( ndesc, "(null)" ) )
            pRoomIndex->nitedesc = str_dup( ndesc );
      }
      {
         int sector = get_sectypes( fread_flagstring( fp ) );

         if( sector < 0 || sector >= SECT_MAX )
         {
            bug( "Room #%d has bad sector type.", vnum );
            sector = 1;
         }

         pRoomIndex->sector_type = sector;
         pRoomIndex->winter_sector = -1;
      }

      {
         char *roomflags = NULL;
         char flag[MIL];
         int value;

         roomflags = fread_flagstring( fp );

         while( roomflags[0] != '\0' )
         {
            roomflags = one_argument( roomflags, flag );
            value = get_rflag( flag );
            if( value < 0 || value > MAX_BITS )
               bug( "Unknown roomflag: %s\n\r", flag );
            else
               SET_ROOM_FLAG( pRoomIndex, value );
         }

         area_number = fread_number( fp );
      }

      {
         if( area_number > 0 )
         {
            ln = fread_line( fp );
            x1 = x2 = x3 = 0;
            sscanf( ln, "%d %d %d", &x1, &x2, &x3 );

            pRoomIndex->tele_delay = x1;
            pRoomIndex->tele_vnum = x2;
            pRoomIndex->tunnel = x3;
         }
         else
         {
            pRoomIndex->tele_delay = 0;
            pRoomIndex->tele_vnum = 0;
            pRoomIndex->tunnel = 0;
         }
      }
      if( pRoomIndex->sector_type < 0 || pRoomIndex->sector_type >= SECT_MAX )
      {
         bug( "Fread_rooms: vnum %d has bad sector_type %d.", vnum, pRoomIndex->sector_type );
         pRoomIndex->sector_type = 1;
      }
      pRoomIndex->light = 0;
      pRoomIndex->first_exit = NULL;
      pRoomIndex->last_exit = NULL;

      for( ;; )
      {
         letter = fread_letter( fp );

         if( letter == 'S' )
            break;

         if( letter == 'D' )
         {
            EXIT_DATA *pexit;

            door = get_dir( fread_flagstring( fp ) );

            if( door < 0 || door > DIR_SOMEWHERE )
            {
               bug( "Load_rooms: vnum %d has bad door number %d.", vnum, door );
               if( fBootDb )
                  exit( 1 );
            }
            else
            {
               {
                  char *exitflags = NULL;
                  char flag[MIL];
                  int value;

                  pexit = make_exit( pRoomIndex, NULL, door );
                  pexit->exitdesc = fread_string( fp );
                  pexit->keyword = fread_string( fp );

                  exitflags = fread_flagstring( fp );

                  while( exitflags[0] != '\0' )
                  {
                     exitflags = one_argument( exitflags, flag );
                     value = get_exflag( flag );
                     if( value < 0 || value > MAX_BITS )
                        bug( "Unknown exitflag: %s\n\r", flag );
                     else
                        SET_EXIT_FLAG( pexit, value );
                  }

                  ln = fread_line( fp );
                  x1 = x2 = x3 = x4 = x5 = x6 = 0;
                  sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );

                  pexit->key = x1;
                  pexit->vnum = x2;
                  pexit->vdir = door;
                  pexit->x = x3;
                  pexit->y = x4;
                  pexit->pulltype = x5;
                  pexit->pull = x6;

                  if( tarea->version < 13 )
                  {
                     pexit->x -= 1;
                     pexit->y -= 1;
                  }
               }
            }
         }
         else if( letter == 'E' )
         {
            EXTRA_DESCR_DATA *ed;

            CREATE( ed, EXTRA_DESCR_DATA, 1 );
            ed->keyword = fread_string( fp );
            ed->extradesc = fread_string( fp );
            LINK( ed, pRoomIndex->first_extradesc, pRoomIndex->last_extradesc, next, prev );
            top_ed++;
         }

         else if( letter == 'R' )
            load_room_reset( pRoomIndex, fp );

         else if( letter == '>' )
         {
            ungetc( letter, fp );
            rprog_read_programs( fp, pRoomIndex );
         }
         else
         {
            bug( "Load_rooms: vnum %d has flag '%c' not 'DES'.", vnum, letter );
            shutdown_mud( "Room flag not DES" );
            exit( 1 );
         }

      }

      if( !oldroom )
      {
         iHash = vnum % MAX_KEY_HASH;
         pRoomIndex->next = room_index_hash[iHash];
         room_index_hash[iHash] = pRoomIndex;
         LINK( pRoomIndex, tarea->first_room, tarea->last_room, next_aroom, prev_aroom );
         top_room++;
      }
   }

   return;
}

/*
 * Load a shop section.
 */
void load_shops( AREA_DATA * tarea, FILE * fp )
{
   SHOP_DATA *pShop;

   for( ;; )
   {
      MOB_INDEX_DATA *pMobIndex;
      int iTrade;

      CREATE( pShop, SHOP_DATA, 1 );
      pShop->keeper = fread_number( fp );
      if( pShop->keeper == 0 )
      {
         DISPOSE( pShop );
         break;
      }
      for( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
         pShop->buy_type[iTrade] = fread_number( fp );
      pShop->profit_buy = fread_number( fp );
      pShop->profit_sell = fread_number( fp );
      pShop->profit_buy = URANGE( pShop->profit_sell + 5, pShop->profit_buy, 1000 );
      pShop->profit_sell = URANGE( 0, pShop->profit_sell, pShop->profit_buy - 5 );
      pShop->open_hour = fread_number( fp );
      pShop->close_hour = fread_number( fp );
      fread_to_eol( fp );
      pMobIndex = get_mob_index( pShop->keeper );
      pMobIndex->pShop = pShop;

      if( !first_shop )
         first_shop = pShop;
      else
         last_shop->next = pShop;
      pShop->next = NULL;
      pShop->prev = last_shop;
      last_shop = pShop;
      top_shop++;
   }
   return;
}

/*
 * Load a repair shop section.					-Thoric
 */
void load_repairs( AREA_DATA * tarea, FILE * fp )
{
   REPAIR_DATA *rShop;

   for( ;; )
   {
      MOB_INDEX_DATA *pMobIndex;
      int iFix;

      CREATE( rShop, REPAIR_DATA, 1 );
      rShop->keeper = fread_number( fp );
      if( rShop->keeper == 0 )
      {
         DISPOSE( rShop );
         break;
      }
      for( iFix = 0; iFix < MAX_FIX; iFix++ )
         rShop->fix_type[iFix] = fread_number( fp );
      rShop->profit_fix = fread_number( fp );
      rShop->shop_type = fread_number( fp );
      rShop->open_hour = fread_number( fp );
      rShop->close_hour = fread_number( fp );
      fread_to_eol( fp );
      pMobIndex = get_mob_index( rShop->keeper );
      pMobIndex->rShop = rShop;

      if( !first_repair )
         first_repair = rShop;
      else
         last_repair->next = rShop;
      rShop->next = NULL;
      rShop->prev = last_repair;
      last_repair = rShop;
      top_repair++;
   }
   return;
}

/*
 * Load spec proc declarations.
 */
void load_specials( AREA_DATA * tarea, FILE * fp )
{
   for( ;; )
   {
      MOB_INDEX_DATA *pMobIndex;
      char *temp;
      char letter;

      switch ( letter = fread_letter( fp ) )
      {
         default:
            bug( "Load_specials: letter '%c' not *MSOR.", letter );
            exit( 1 );

         case 'S':
            return;

         case '*':
            break;

         case 'M':
            pMobIndex = get_mob_index( fread_number( fp ) );
            temp = fread_word( fp );
            if( !pMobIndex )
            {
               bug( "%s", "Load_specials: 'M': Invalid mob vnum!" );
               break;
            }
            pMobIndex->spec_fun = m_spec_lookup( temp );
            if( pMobIndex->spec_fun == NULL )
            {
               bug( "Load_specials: 'M': vnum %d.", pMobIndex->vnum );
               pMobIndex->spec_funname = NULL;
            }
            else
               pMobIndex->spec_funname = STRALLOC( temp );
            break;
      }
      fread_to_eol( fp );
   }
}


/*
 * Load soft / hard area ranges.
 */
void load_ranges( AREA_DATA * tarea, FILE * fp )
{
   int x1, x2, x3, x4;
   char *ln;

   if( !tarea )
   {
      bug( "%s", "Load_ranges: no #AREA seen yet." );
      shutdown_mud( "No #AREA" );
      exit( 1 );
   }

   for( ;; )
   {
      ln = fread_line( fp );

      if( ln[0] == '$' )
         break;

      x1 = x2 = x3 = x4 = 0;
      sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );

      tarea->low_soft_range = x1;
      tarea->hi_soft_range = x2;
      tarea->low_hard_range = x3;
      tarea->hi_hard_range = x4;
   }
   return;

}

/*
 * Load climate information for the area
 * Last modified: July 13, 1997
 * Fireblade
 */
void load_climate( AREA_DATA * tarea, FILE * fp )
{
   if( !tarea )
   {
      bug( "%s", "load_climate: no #AREA seen yet" );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   tarea->weather->climate_temp = fread_number( fp );
   tarea->weather->climate_precip = fread_number( fp );
   tarea->weather->climate_wind = fread_number( fp );

   return;
}

/*
 * Load data for a neghboring weather system
 * Last modified: July 13, 1997
 * Fireblade
 */
void load_neighbor( AREA_DATA * tarea, FILE * fp )
{
   NEIGHBOR_DATA *anew;

   if( !tarea )
   {
      bug( "%s", "load_neighbor: no #AREA seen yet." );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   CREATE( anew, NEIGHBOR_DATA, 1 );
   anew->next = NULL;
   anew->prev = NULL;
   anew->address = NULL;
   anew->name = fread_string( fp );
   LINK( anew, tarea->weather->first_neighbor, tarea->weather->last_neighbor, next, prev );
   return;
}

/*
 * (prelude...) This is going to be fun... NOT!
 * (conclusion) QSort is f*cked!
 */
int exit_comp( EXIT_DATA ** xit1, EXIT_DATA ** xit2 )
{
   int d1, d2;

   d1 = ( *xit1 )->vdir;
   d2 = ( *xit2 )->vdir;

   if( d1 < d2 )
      return -1;
   if( d1 > d2 )
      return 1;
   return 0;
}

void sort_exits( ROOM_INDEX_DATA * room )
{
   EXIT_DATA *pexit; /* *texit *//* Unused */
   EXIT_DATA *exits[MAX_REXITS];
   int x, nexits;

   nexits = 0;
   for( pexit = room->first_exit; pexit; pexit = pexit->next )
   {
      exits[nexits++] = pexit;
      if( nexits > MAX_REXITS )
      {
         bug( "sort_exits: more than %d exits in room... fatal", nexits );
         return;
      }
   }
   qsort( &exits[0], nexits, sizeof( EXIT_DATA * ), ( int ( * )( const void *, const void * ) )exit_comp );
   for( x = 0; x < nexits; x++ )
   {
      if( x > 0 )
         exits[x]->prev = exits[x - 1];
      else
      {
         exits[x]->prev = NULL;
         room->first_exit = exits[x];
      }
      if( x >= ( nexits - 1 ) )
      {
         exits[x]->next = NULL;
         room->last_exit = exits[x];
      }
      else
         exits[x]->next = exits[x + 1];
   }
}

void randomize_exits( ROOM_INDEX_DATA * room, short maxdir )
{
   EXIT_DATA *pexit;
   int nexits, /* maxd, */ d0, d1, count, door; /* Maxd unused */
   int vdirs[MAX_REXITS];

   nexits = 0;
   for( pexit = room->first_exit; pexit; pexit = pexit->next )
      vdirs[nexits++] = pexit->vdir;

   for( d0 = 0; d0 < nexits; d0++ )
   {
      if( vdirs[d0] > maxdir )
         continue;
      count = 0;
      while( vdirs[( d1 = number_range( d0, nexits - 1 ) )] > maxdir || ++count < 5 );
      if( vdirs[d1] > maxdir )
         continue;
      door = vdirs[d0];
      vdirs[d0] = vdirs[d1];
      vdirs[d1] = door;
   }
   count = 0;
   for( pexit = room->first_exit; pexit; pexit = pexit->next )
      pexit->vdir = vdirs[count++];

   sort_exits( room );
}

/*
 * Simple linear interpolation.
 */
int interpolate( int level, int value_00, int value_32 )
{
   return value_00 + level * ( value_32 - value_00 ) / 32;
}

/* Auto-calc formula to set numattacks for PCs & mobs - Samson 5-6-99 */
void set_attacks( CHAR_DATA * ch )
{
   /*
    * Attack formulas modified to use Shard formulas - Samson 5-3-99 
    */
   ch->numattacks = 1.0;

   switch ( ch->Class )
   {
      case CLASS_MAGE:
      case CLASS_NECROMANCER:
         /*
          * 2.72 attacks at level 100 
          */
         ch->numattacks += ( float )( UMIN( 86, ch->level ) * .02 );
         break;

      case CLASS_CLERIC:
      case CLASS_DRUID:
         /*
          * 3.5 attacks at level 100 
          */
         ch->numattacks += ( float )( ch->level / 40.0 );
         break;

      case CLASS_WARRIOR:
      case CLASS_PALADIN:
      case CLASS_RANGER:
      case CLASS_ANTIPALADIN:
         /*
          * 5.3 attacks at level 100 
          */
         ch->numattacks += ( float )( UMIN( 86, ch->level ) * .05 );
         break;

      case CLASS_ROGUE:
      case CLASS_BARD:
         /*
          * 4 attacks at level 100 
          */
         ch->numattacks += ( float )( ch->level / 33.3 );
         break;

      case CLASS_MONK:
         /*
          * 7.6 attacks at level 100 
          */
         ch->numattacks += ( float )( ch->level / 15.0 );
         break;

      default:
         break;
   }
   return;
}

/*
 * Create an instance of a mobile.
 */
/* Modified for mob randomizations by Whir - 4-5-98 */
CHAR_DATA *create_mobile( MOB_INDEX_DATA * pMobIndex )
{
   CHAR_DATA *mob;

   if( !pMobIndex )
   {
      bug( "%s", "Create_mobile: NULL pMobIndex." );
      exit( 1 );
   }

   CREATE( mob, CHAR_DATA, 1 );
   clear_char( mob );
   mob->pIndexData = pMobIndex;
   mob->name = QUICKLINK( pMobIndex->player_name );
   if( pMobIndex->short_descr && pMobIndex->short_descr[0] != '\0' )
      mob->short_descr = QUICKLINK( pMobIndex->short_descr );
   if( pMobIndex->long_descr && pMobIndex->long_descr[0] != '\0' )
      mob->long_descr = QUICKLINK( pMobIndex->long_descr );
   if( pMobIndex->chardesc && pMobIndex->chardesc[0] != '\0' )
      mob->chardesc = QUICKLINK( pMobIndex->chardesc );
   mob->spec_fun = pMobIndex->spec_fun;
   if( pMobIndex->spec_funname )
      mob->spec_funname = QUICKLINK( pMobIndex->spec_funname );
   mob->mpscriptpos = 0;
   mob->level = number_fuzzy( pMobIndex->level );
   mob->act = pMobIndex->act;
   mob->home_vnum = -1;
   mob->sector = -1;
   mob->timer = 0;

   if( IS_ACT_FLAG( mob, ACT_MOBINVIS ) )
      mob->mobinvis = mob->level;

   mob->affected_by = pMobIndex->affected_by;
   mob->alignment = pMobIndex->alignment;
   mob->sex = pMobIndex->sex;

   /*
    * Bug fix from mailing list by stu (sprice@ihug.co.nz)
    * was:  if ( !pMobIndex->ac )
    */
   if( pMobIndex->ac )
      mob->armor = pMobIndex->ac;
   else
      mob->armor = interpolate( mob->level, 100, -100 );

   /*
    * Formula altered to conform to Shard mobs: leveld8 + bonus 
    */
   /*
    * Samson 5-3-99 
    */
   mob->max_hit = dice( mob->level, 8 ) + pMobIndex->hitplus;

   mob->hit = mob->max_hit;
   mob->gold = pMobIndex->gold;
   mob->position = pMobIndex->position;
   mob->defposition = pMobIndex->defposition;
   mob->barenumdie = pMobIndex->damnodice;
   mob->baresizedie = pMobIndex->damsizedice;
   mob->mobthac0 = pMobIndex->mobthac0;
   mob->hitplus = pMobIndex->hitplus;
   mob->damplus = pMobIndex->damplus;
   mob->perm_str = number_range( 9, 18 );
   mob->perm_str = number_range( 9, 18 );
   mob->perm_wis = number_range( 9, 18 );
   mob->perm_int = number_range( 9, 18 );
   mob->perm_dex = number_range( 9, 18 );
   mob->perm_con = number_range( 9, 18 );
   mob->perm_cha = number_range( 9, 18 );
   mob->perm_lck = number_range( 9, 18 );
   mob->max_move = pMobIndex->max_move;
   mob->move = mob->max_move;
   mob->max_mana = pMobIndex->max_mana;
   mob->mana = mob->max_mana;

   mob->hitroll = 0;
   mob->damroll = 0;
   mob->race = pMobIndex->race;
   mob->Class = pMobIndex->Class;
   mob->xflags = pMobIndex->xflags;

   /*
    * Saving throw calculations now ported from Sillymud - Samson 5-15-98 
    */
   mob->saving_poison_death = UMAX( 20 - mob->level, 2 );
   mob->saving_wand = UMAX( 20 - mob->level, 2 );
   mob->saving_para_petri = UMAX( 20 - mob->level, 2 );
   mob->saving_breath = UMAX( 20 - mob->level, 2 );
   mob->saving_spell_staff = UMAX( 20 - mob->level, 2 );

   mob->height = pMobIndex->height;
   mob->weight = pMobIndex->weight;
   mob->resistant = pMobIndex->resistant;
   mob->immune = pMobIndex->immune;
   mob->susceptible = pMobIndex->susceptible;
   mob->absorb = pMobIndex->absorb;
   mob->attacks = pMobIndex->attacks;
   mob->defenses = pMobIndex->defenses;

   /*
    * Samson 5-6-99 
    */
   if( pMobIndex->numattacks )
      mob->numattacks = pMobIndex->numattacks;
   else
      set_attacks( mob );

   mob->speaks = pMobIndex->speaks;
   mob->speaking = pMobIndex->speaking;

   if( pMobIndex->xflags == 0 )
      pMobIndex->xflags = race_bodyparts( mob );

   mob->xflags = pMobIndex->xflags;

   if( mob->numattacks > 10 )
      log_printf_plus( LOG_BUILD, sysdata.build_level, "Mob vnum %d has too many attacks: %f", pMobIndex->vnum,
                       mob->numattacks );

   /*
    * Exp modification added by Samson - 5-15-98
    * * Moved here because of the new exp autocalculations : Samson 5-18-01 
    * * Need to flush all the old values because the old code had a bug in it on top of everything else.
    */
   if( pMobIndex->exp < 1 )
   {
      mob->exp = mob_xp( mob );
      pMobIndex->exp = -1;
   }
   else
      mob->exp = pMobIndex->exp;

   /*
    * Perhaps add this to the index later --Shaddai
    */
   xCLEAR_BITS( mob->no_affected_by );
   xCLEAR_BITS( mob->no_resistant );
   xCLEAR_BITS( mob->no_immune );
   xCLEAR_BITS( mob->no_susceptible );

   /*
    * Insert in list.
    */
   LINK( mob, first_char, last_char, next, prev );
   pMobIndex->count++;
   nummobsloaded++;
   return mob;
}

/*
 * Create an instance of an object.
 */
OBJ_DATA *create_object( OBJ_INDEX_DATA * pObjIndex, int level )
{
   OBJ_DATA *obj;

   if( !pObjIndex )
   {
      bug( "%s", "Create_object: NULL pObjIndex." );
      return NULL;
   }

   CREATE( obj, OBJ_DATA, 1 );

   obj->pIndexData = pObjIndex;
   obj->in_room = NULL;
   obj->level = level;
   obj->wear_loc = -1;
   obj->count = 1;
   obj->name = QUICKLINK( pObjIndex->name );
   if( pObjIndex->short_descr && pObjIndex->short_descr[0] != '\0' )
      obj->short_descr = QUICKLINK( pObjIndex->short_descr );
   if( pObjIndex->objdesc && pObjIndex->objdesc[0] != '\0' )
      obj->objdesc = QUICKLINK( pObjIndex->objdesc );
   if( pObjIndex->action_desc && pObjIndex->action_desc[0] != '\0' )
      obj->action_desc = QUICKLINK( pObjIndex->action_desc );
   obj->socket[0] = QUICKLINK( pObjIndex->socket[0] );
   obj->socket[1] = QUICKLINK( pObjIndex->socket[1] );
   obj->socket[2] = QUICKLINK( pObjIndex->socket[2] );
   obj->item_type = pObjIndex->item_type;
   obj->extra_flags = pObjIndex->extra_flags;
   obj->wear_flags = pObjIndex->wear_flags;
   obj->value[0] = pObjIndex->value[0];
   obj->value[1] = pObjIndex->value[1];
   obj->value[2] = pObjIndex->value[2];
   obj->value[3] = pObjIndex->value[3];
   obj->value[4] = pObjIndex->value[4];
   obj->value[5] = pObjIndex->value[5];
   obj->value[6] = pObjIndex->value[6];
   obj->value[7] = pObjIndex->value[7];
   obj->value[8] = pObjIndex->value[8];
   obj->value[9] = pObjIndex->value[9];
   obj->value[10] = pObjIndex->value[10];
   obj->owner = obj->buyer = obj->seller = NULL;
   obj->weight = pObjIndex->weight;
   obj->cost = pObjIndex->cost;
   if( pObjIndex->rent == -2 )   /* Calculate */
      obj->rent = set_obj_rent( pObjIndex );
   else
      obj->rent = pObjIndex->rent;

   obj->x = -1;
   obj->y = -1;
   obj->map = -1;
   obj->day = 0;
   obj->month = 0;
   obj->year = 0;

   /*
    * Mess with object properties.
    */
   switch ( obj->item_type )
   {
      default:
         bug( "%s: vnum %d bad type.", __FUNCTION__, pObjIndex->vnum );
         log_printf( "------------------------>    %d", obj->item_type );
         break;

      case ITEM_TREE:
      case ITEM_LIGHT:
      case ITEM_TREASURE:
      case ITEM_FURNITURE:
      case ITEM_TRASH:
      case ITEM_CONTAINER:
      case ITEM_CAMPGEAR:
      case ITEM_DRINK_CON:
      case ITEM_KEY:
      case ITEM_KEYRING:
      case ITEM_ODOR:
      case ITEM_CLOTHING:
         break;
      case ITEM_COOK:
      case ITEM_FOOD:
         /*
          * optional food condition (rotting food)    -Thoric
          * value1 is the max condition of the food
          * value4 is the optional initial condition
          */
         if( obj->value[4] )
            obj->timer = obj->value[4];
         else
            obj->timer = obj->value[1];
         break;
      case ITEM_BOAT:
      case ITEM_INSTRUMENT:
      case ITEM_CORPSE_NPC:
      case ITEM_CORPSE_PC:
      case ITEM_FOUNTAIN:
      case ITEM_BLOOD:
      case ITEM_BLOODSTAIN:
      case ITEM_SCRAPS:
      case ITEM_PIPE:
      case ITEM_HERB_CON:
      case ITEM_HERB:
      case ITEM_INCENSE:
      case ITEM_FIRE:
      case ITEM_BOOK:
      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
      case ITEM_BUTTON:
      case ITEM_DIAL:
      case ITEM_RUNE:
      case ITEM_RUNEPOUCH:
      case ITEM_MATCH:
      case ITEM_TRAP:
      case ITEM_MAP:
      case ITEM_PORTAL:
      case ITEM_PAPER:
      case ITEM_PEN:
      case ITEM_TINDER:
      case ITEM_LOCKPICK:
      case ITEM_SPIKE:
      case ITEM_DISEASE:
      case ITEM_OIL:
      case ITEM_FUEL:
      case ITEM_QUIVER:
      case ITEM_SHOVEL:
      case ITEM_ORE:
      case ITEM_PIECE:
         break;

      case ITEM_SALVE:
         obj->value[3] = number_fuzzy( obj->value[3] );
         break;

      case ITEM_SCROLL:
         obj->value[0] = number_fuzzy( obj->value[0] );
         break;

      case ITEM_WAND:
      case ITEM_STAFF:
         obj->value[0] = number_fuzzy( obj->value[0] );
         obj->value[1] = number_fuzzy( obj->value[1] );
         obj->value[2] = obj->value[1];
         break;

      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
      case ITEM_PROJECTILE:
         if( obj->value[1] && obj->value[2] )
            obj->value[2] *= obj->value[1];
         else
         {
            obj->value[1] = number_fuzzy( number_fuzzy( 1 * level / 4 + 2 ) );
            obj->value[2] = number_fuzzy( number_fuzzy( 3 * level / 4 + 6 ) );
         }
         if( obj->value[0] == 0 )
            obj->value[0] = sysdata.initcond;
         if( obj->item_type == ITEM_PROJECTILE )
            obj->value[5] = obj->value[0];
         else
            obj->value[6] = obj->value[0];
         break;

      case ITEM_ARMOR:
         if( obj->value[0] == 0 )
            obj->value[0] = number_fuzzy( level / 4 + 2 );
         if( obj->value[1] == 0 )
            obj->value[1] = obj->value[0];
         break;

      case ITEM_POTION:
      case ITEM_PILL:
         obj->value[0] = number_fuzzy( number_fuzzy( obj->value[0] ) );
         break;

      case ITEM_MONEY:
         obj->value[0] = obj->cost;
         if( obj->value[0] == 0 )
            obj->value[0] = 1;
         break;
   }

   /*
    * Wow. This hackish looking thing is pretty bad isn't it?
    * * I thought so too, but hey. Dwip wanted to bring in a bunch of old stuff that needed to be armorgen'd.
    * * This was about the only way I could think to do it.
    * * Won't bother you much if you haven't set v3 or v4 on an armor though.
    * * All in the name of being able to retain stats if deviating from the armorgen specs.
    * * Samson 12-23-02
    */
   if( obj->pIndexData->area && obj->pIndexData->area->version < 18 )
   {
      if( obj->item_type == ITEM_ARMOR && obj->value[3] > 0 && obj->value[4] > 0 )
      {
         bool pflag = FALSE;

         if( IS_OBJ_FLAG( obj->pIndexData, ITEM_PROTOTYPE ) )
            pflag = TRUE;
         SET_OBJ_FLAG( obj, ITEM_PROTOTYPE );
         armorgen( obj );
         if( !pflag )
         {
            REMOVE_OBJ_FLAG( obj, ITEM_PROTOTYPE );
            REMOVE_OBJ_FLAG( obj->pIndexData, ITEM_PROTOTYPE );
         }
      }
   }
   LINK( obj, first_object, last_object, next, prev );
   ++pObjIndex->count;
   ++numobjsloaded;
   ++physicalobjects;

   return obj;
}

/* Deallocates the memory used by a single object after it's been extracted. */
void free_obj( OBJ_DATA * obj )
{
   AFFECT_DATA *paf, *paf_next;
   EXTRA_DESCR_DATA *ed, *ed_next;
   REL_DATA *RQueue, *rq_next;
   MPROG_ACT_LIST *mpact, *mpact_next;

   for( mpact = obj->mpact; mpact; mpact = mpact_next )
   {
      mpact_next = mpact->next;
      DISPOSE( mpact->buf );
      DISPOSE( mpact );
   }

   /*
    * remove affects 
    */
   for( paf = obj->first_affect; paf; paf = paf_next )
   {
      paf_next = paf->next;
      DISPOSE( paf );
   }
   obj->first_affect = obj->last_affect = NULL;

   /*
    * remove extra descriptions 
    */
   for( ed = obj->first_extradesc; ed; ed = ed_next )
   {
      ed_next = ed->next;
      STRFREE( ed->extradesc );
      STRFREE( ed->keyword );
      DISPOSE( ed );
   }
   obj->first_extradesc = obj->last_extradesc = NULL;

   for( RQueue = first_relation; RQueue; RQueue = rq_next )
   {
      rq_next = RQueue->next;
      if( RQueue->Type == relOSET_ON )
      {
         if( obj == RQueue->Subject )
            ( ( CHAR_DATA * ) RQueue->Actor )->pcdata->dest_buf = NULL;
         else
            continue;
         UNLINK( RQueue, first_relation, last_relation, next, prev );
         DISPOSE( RQueue );
      }
   }
   STRFREE( obj->name );
   STRFREE( obj->objdesc );
   STRFREE( obj->short_descr );
   STRFREE( obj->action_desc );
   STRFREE( obj->socket[0] );
   STRFREE( obj->socket[1] );
   STRFREE( obj->socket[2] );
   STRFREE( obj->owner );
   STRFREE( obj->seller );
   STRFREE( obj->buyer );
   DISPOSE( obj );
   return;
}

/*
 * Clear a new character.
 */
void clear_char( CHAR_DATA * ch )
{
   ch->hunting = NULL;
   ch->fearing = NULL;
   ch->hating = NULL;
   ch->name = NULL;
   ch->short_descr = NULL;
   ch->long_descr = NULL;
   ch->chardesc = NULL;
   ch->next = NULL;
   ch->prev = NULL;
   ch->reply = NULL;
   ch->first_carrying = NULL;
   ch->last_carrying = NULL;
   ch->next_in_room = NULL;
   ch->prev_in_room = NULL;
   ch->fighting = NULL;
   ch->switched = NULL;
   ch->first_affect = NULL;
   ch->last_affect = NULL;
   ch->alloc_ptr = NULL;
   ch->mount = NULL;
   ch->morph = NULL;
   xCLEAR_BITS( ch->affected_by );
   ch->armor = 100;
   ch->position = POS_STANDING;
   ch->hit = 20;
   ch->max_hit = 20;
   ch->hit_regen = 0;
   ch->mana = 100;
   ch->max_mana = 100;
   ch->mana_regen = 0;
   ch->move = 150;
   ch->max_move = 150;
   ch->move_regen = 0;
   ch->height = 72;
   ch->weight = 180;
   ch->xflags = 0;
   ch->spellfail = 101;
   ch->race = 0;
   ch->Class = 3;
   ch->speaking = LANG_COMMON;
   ch->speaks = LANG_COMMON;
   ch->barenumdie = 1;
   ch->baresizedie = 4;
   ch->perm_str = 13;
   ch->perm_dex = 13;
   ch->perm_int = 13;
   ch->perm_wis = 13;
   ch->perm_cha = 13;
   ch->perm_con = 13;
   ch->perm_lck = 13;
   ch->mod_str = 0;
   ch->mod_dex = 0;
   ch->mod_int = 0;
   ch->mod_wis = 0;
   ch->mod_cha = 0;
   ch->mod_con = 0;
   ch->mod_lck = 0;
   ch->x = -1; /* Overland Map - Samson 7-31-99 */
   ch->y = -1;
   ch->map = -1;
   return;
}

/*
 * Free a character.
 */
void free_char( CHAR_DATA * ch )
{
   OBJ_DATA *obj;
   AFFECT_DATA *paf;
   TIMER *timer;
   MPROG_ACT_LIST *mpact, *mpact_next;
   BIT_DATA *abit, *abit_next;

   if( !ch )
   {
      bug( "%s: null ch!", __FUNCTION__ );
      return;
   }

   if( ch->desc )
      bug( "%s", "Free_char: char still has descriptor." );

   DISPOSE( ch->morph );

   while( ( obj = ch->last_carrying ) != NULL )
      extract_obj( obj );

   while( ( paf = ch->last_affect ) != NULL )
      affect_remove( ch, paf );

   while( ( timer = ch->first_timer ) != NULL )
      extract_timer( ch, timer );

   STRFREE( ch->name );
   STRFREE( ch->short_descr );
   STRFREE( ch->long_descr );
   STRFREE( ch->chardesc );
   STRFREE( ch->spec_funname );
   DISPOSE( ch->alloc_ptr );

   stop_hunting( ch );
   stop_hating( ch );
   stop_fearing( ch );
   free_fight( ch );

   for( abit = ch->first_abit; abit; abit = abit_next )
   {
      abit_next = abit->next;

      UNLINK( abit, ch->first_abit, ch->last_abit, next, prev );
      DISPOSE( abit );
   }

   if( ch->pcdata )
   {
      BIT_DATA *qbit, *qbit_next;
      int x;

      if( ch->pcdata->editor )
         stop_editing( ch );

      if( ch->pcdata->pnote )
         free_note( ch->pcdata->pnote );

      STRFREE( ch->pcdata->filename );
      STRFREE( ch->pcdata->deity_name );
      STRFREE( ch->pcdata->clan_name );
      STRFREE( ch->pcdata->council_name );
      STRFREE( ch->pcdata->prompt );
      STRFREE( ch->pcdata->fprompt );
      DISPOSE( ch->pcdata->pwd );   /* no hash */
      DISPOSE( ch->pcdata->bamfin );   /* no hash */
      DISPOSE( ch->pcdata->bamfout );  /* no hash */
      STRFREE( ch->pcdata->rank );
      STRFREE( ch->pcdata->title );
      DISPOSE( ch->pcdata->bio );   /* no hash */
      DISPOSE( ch->pcdata->bestowments ); /* no hash */
      DISPOSE( ch->pcdata->homepage ); /* no hash */
      DISPOSE( ch->pcdata->email ); /* no hash */
      DISPOSE( ch->pcdata->lasthost ); /* no hash */
      STRFREE( ch->pcdata->authed_by );
      STRFREE( ch->pcdata->prompt );
      STRFREE( ch->pcdata->fprompt );
      STRFREE( ch->pcdata->helled_by );
      STRFREE( ch->pcdata->subprompt );
      STRFREE( ch->pcdata->chan_listen ); /* DOH! Forgot about this! */
      DISPOSE( ch->pcdata->afkbuf );
      DISPOSE( ch->pcdata->motd_buf );

      free_ignores( ch );
      free_zonedata( ch );
      free_aliases( ch );
      free_comments( ch );
      free_pcboards( ch );

      /*
       * God. How does one forget something like this anyway? BAD SAMSON! 
       */
      for( x = 0; x < MAX_SAYHISTORY; x++ )
         DISPOSE( ch->pcdata->say_history[x] );

      /*
       * Dammit! You forgot another one you git! 
       */
      for( x = 0; x < MAX_TELLHISTORY; x++ )
         DISPOSE( ch->pcdata->tell_history[x] );

      for( qbit = ch->pcdata->first_qbit; qbit; qbit = qbit_next )
      {
         qbit_next = qbit->next;

         UNLINK( qbit, ch->pcdata->first_qbit, ch->pcdata->last_qbit, next, prev );
         DISPOSE( qbit );
      }

#ifdef I3
      free_i3chardata( ch );
#endif
#ifdef IMC
      imc_freechardata( ch );
#endif
      DISPOSE( ch->pcdata );
   }

   for( mpact = ch->mpact; mpact; mpact = mpact_next )
   {
      mpact_next = mpact->next;
      DISPOSE( mpact->buf );
      DISPOSE( mpact );
   }
   DISPOSE( ch );
   return;
}

/*
 * Get an extra description from a list.
 */
char *get_extra_descr( const char *name, EXTRA_DESCR_DATA * ed )
{
   for( ; ed; ed = ed->next )
   {
      if( !ed->keyword || ed->keyword[0] == '\0' )
         continue;

      if( is_name( name, ed->keyword ) && ( ed->extradesc && ed->extradesc[0] != '\0' ) )
         return ed->extradesc;
   }
   return NULL;
}

/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *get_mob_index( int vnum )
{
   MOB_INDEX_DATA *pMobIndex;

   if( vnum < 0 )
      vnum = 0;

   for( pMobIndex = mob_index_hash[vnum % MAX_KEY_HASH]; pMobIndex; pMobIndex = pMobIndex->next )
      if( pMobIndex->vnum == vnum )
         return pMobIndex;

   if( fBootDb )
      bug( "Get_mob_index: bad vnum %d.", vnum );

   return NULL;
}

/*
 * Translates obj virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *get_obj_index( int vnum )
{
   OBJ_INDEX_DATA *pObjIndex;

   if( vnum < 0 )
      vnum = 0;

   for( pObjIndex = obj_index_hash[vnum % MAX_KEY_HASH]; pObjIndex; pObjIndex = pObjIndex->next )
      if( pObjIndex->vnum == vnum )
         return pObjIndex;

   if( fBootDb )
      bug( "Get_obj_index: bad vnum %d.", vnum );

   return NULL;
}

/*
 * Translates room virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA *get_room_index( int vnum )
{
   ROOM_INDEX_DATA *pRoomIndex;

   if( vnum < 0 )
      vnum = 0;

   for( pRoomIndex = room_index_hash[vnum % MAX_KEY_HASH]; pRoomIndex; pRoomIndex = pRoomIndex->next )
      if( pRoomIndex->vnum == vnum )
         return pRoomIndex;

   if( fBootDb )
      bug( "Get_room_index: bad vnum %d.", vnum );

   return NULL;
}

CMDF do_memory( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   int hash;

   argument = one_argument( argument, arg );
   send_to_char( "\n\r&wSystem Memory [arguments - hash, check, showhigh]\n\r", ch );
   ch_printf( ch, "&wAffects: &W%5d\t\t\t&wAreas:   &W%5d\n\r", top_affect, top_area );
   ch_printf( ch, "&wExtDes:  &W%5d\t\t\t&wExits:   &W%5d\n\r", top_ed, top_exit );
   ch_printf( ch, "&wHelps:   &W%5d\t\t\t&wResets:  &W%5d\n\r", top_help, top_reset );
   ch_printf( ch, "&wIdxMobs: &W%5d\t\t\t&wMobiles: &W%5d\n\r", top_mob_index, nummobsloaded );
   ch_printf( ch, "&wIdxObjs: &W%5d\t\t\t&wObjs:    &W%5d(%d)\n\r", top_obj_index, numobjsloaded, physicalobjects );
   ch_printf( ch, "&wRooms:   &W%5d\n\r", top_room );
   ch_printf( ch, "&wShops:   &W%5d\t\t\t&wRepShps: &W%5d\n\r", top_shop, top_repair );
   ch_printf( ch, "&wCurOq's: &W%5d\t\t\t&wCurCq's: &W%5d\n\r", cur_qobjs, cur_qchars );
   ch_printf( ch, "&wPlayers: &W%5d\t\t\t&wMaxplrs: &W%5d\n\r", num_descriptors, sysdata.maxplayers );
   ch_printf( ch, "&wMaxEver: &W%5d\t\t\t&wTopsn:   &W%5d(%d)\n\r", sysdata.alltimemax, top_sn, MAX_SKILL );
   ch_printf( ch, "&wMaxEver was recorded on:  &W%s\n\r\n\r", sysdata.time_of_max );

   if( !str_cmp( arg, "check" ) )
   {
      send_to_char( check_hash( argument ), ch );
      return;
   }
   if( !str_cmp( arg, "showhigh" ) )
   {
      show_high_hash( atoi( argument ) );
      return;
   }
   if( argument[0] != '\0' )
      hash = atoi( argument );
   else
      hash = -1;
   if( !str_cmp( arg, "hash" ) )
   {
      ch_printf( ch, "Hash statistics:\n\r%s", hash_stats(  ) );
      if( hash != -1 )
         hash_dump( hash );
   }
   return;
}

/* Dummy code added to block number_fuzzy from messing things up - Samson 3-28-98 */
int number_fuzzy( int number )
{
   return number;
}

int number_mm( void )
{
   int *piState;
   int iState1, iState2, iRand;

   piState = &rgiState[2];
   iState1 = piState[-2];
   iState2 = piState[-1];
   iRand = ( piState[iState1] + piState[iState2] ) & ( ( 1 << 30 ) - 1 );
   piState[iState1] = iRand;
   if( ++iState1 == 55 )
      iState1 = 0;
   if( ++iState2 == 55 )
      iState2 = 0;
   piState[-2] = iState1;
   piState[-1] = iState2;
   return iRand >> 6;
}

/*
 * Generate a random number.
 * Ooops was (number_mm() % to) + from which doesn't work -Shaddai
 */
int number_range( int from, int to )
{
   if( ( to - from ) < 1 )
      return from;
   return ( ( number_mm(  ) % ( to - from + 1 ) ) + from );
}

/*
 * Generate a percentile roll.
 * number_mm() % 100 only does 0-99, changed to do 1-100 -Shaddai
 */
int number_percent( void )
{
   return ( number_mm(  ) % 100 ) + 1;
}

/*
 * Generate a random door.
 */
int number_door( void )
{
   int door;

   while( ( door = number_mm(  ) & ( 16 - 1 ) ) > 9 )
      ;

   return door;
}

int number_bits( int width )
{
   return number_mm(  ) & ( ( 1 << width ) - 1 );
}

/*
 * Roll some dice.						-Thoric
 */
int dice( int number, int size )
{
   int idice, sum;

   switch ( size )
   {
      case 0:
         return 0;
      case 1:
         return number;
   }

   for( idice = 0, sum = 0; idice < number; idice++ )
      sum += number_range( 1, size );

   return sum;
}

/*
 * Append a string to a file.
 */
void append_file( CHAR_DATA * ch, char *file, char *fmt, ... )
{
   FILE *fp;
   va_list arg;
   char str[MSL];

   va_start( arg, fmt );
   vsnprintf( str, MSL, fmt, arg );
   va_end( arg );

   if( IS_NPC( ch ) || !str || str[0] == '\0' )
      return;

   if( strlen( str ) < 1 || str[strlen( str ) - 1] != '\n' )
      mudstrlcat( str, "\n", MSL );

   if( !( fp = fopen( file, "a" ) ) )
   {
      perror( file );
      send_to_char( "Could not open the file!\n\r", ch );
   }
   else
   {
      fprintf( fp, "[%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name, str );
      FCLOSE( fp );
   }
   return;
}

/*
 * Append a string to a file.
 */
void append_to_file( char *file, char *fmt, ... )
{
   FILE *fp;
   va_list arg;
   char str[MSL];

   va_start( arg, fmt );
   vsnprintf( str, MSL, fmt, arg );
   va_end( arg );

   if( !str || str[0] == '\0' )
      return;

   if( strlen( str ) < 1 || str[strlen( str ) - 1] != '\n' )
      mudstrlcat( str, "\n", MSL );

   if( !( fp = fopen( file, "a" ) ) )
      perror( file );
   else
   {
      fprintf( fp, "%s\n", str );
      FCLOSE( fp );
   }

   return;
}

/* Reports a bug. */
/* Now includes backtrace data for that supernifty bug report! - Samson 10-11-03 */
void bug( const char *str, ... )
{
   char buf[MSL];
#if !defined(__CYGWIN__) && !defined(__FreeBSD__)
   void *array[20];
   size_t size, i;
   char **strings;
#endif

   if( fpArea != NULL )
   {
      int iLine;
      int iChar;

      if( fpArea == stdin )
         iLine = 0;
      else
      {
         iChar = ftell( fpArea );
         fseek( fpArea, 0, 0 );
         for( iLine = 0; ftell( fpArea ) < iChar; iLine++ )
         {
            int letter;

            while( ( letter = getc( fpArea ) ) && letter != EOF && letter != '\n' )
               ;
         }
         fseek( fpArea, iChar, 0 );
      }

      log_printf( "[*****] FILE: %s LINE: %d", strArea, iLine );
   }

   mudstrlcpy( buf, "[*****] BUG: ", MSL );
   {
      va_list param;

      va_start( param, str );
      vsnprintf( buf + strlen( buf ), MSL, str, param );
      va_end( param );
   }
   log_string( buf );

#if !defined(__CYGWIN__) && !defined(__FreeBSD__)
   if( !fBootDb )
   {
      size = backtrace( array, 20 );
      strings = backtrace_symbols( array, size );

      log_printf( "Obtained %d stack frames.", size );

      for( i = 0; i < size; i++ )
         log_string( strings[i] );

      free( strings );
   }
#endif

   return;
}

/*
 * Dump a text file to a player, a line at a time		-Thoric
 */
void show_file( CHAR_DATA * ch, char *filename )
{
   FILE *fp;
   char buf[MSL];
   int c, num = 0;

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
      send_to_pager( "\n\r", ch );
      while( !feof( fp ) )
      {
         while( ( buf[num] = fgetc( fp ) ) != EOF && buf[num] != '\n' && buf[num] != '\r' && num < ( MSL - 4 ) )
            num++;

         c = fgetc( fp );
         if( ( c != '\n' && c != '\r' ) || c == buf[num] )
            ungetc( c, fp );

         buf[num++] = '\n';
         buf[num++] = '\r';
         buf[num] = '\0';
         send_to_pager( buf, ch );
         num = 0;
      }
      /*
       * Thanks to stu <sprice@ihug.co.nz> from the mailing list in pointing This out. 
       */
      FCLOSE( fp );
   }
}

/*
 * Writes a string to the log, extended version			-Thoric
 */
void log_string_plus( const char *str, short log_type, short level )
{
   struct timeval last_time;
   char *strtime;
   int offset;

   if( fBootDb )
   {
      gettimeofday( &last_time, NULL );
      current_time = ( time_t ) last_time.tv_sec;
   }

   strtime = c_time( current_time, -1 );
   fprintf( stderr, "%s :: %s\n", strtime, str );
   if( strncmp( str, "Log ", 4 ) == 0 )
      offset = 4;
   else
      offset = 0;
   switch ( log_type )
   {
      default:
         to_channel( str + offset, "Log", level );
         break;
      case LOG_BUILD:
         to_channel( str + offset, "Build", level );
         break;
      case LOG_COMM:
         to_channel( str + offset, "Comm", level );
         break;
      case LOG_WARN:
         to_channel( str + offset, "Warn", level );
         break;
      case LOG_INFO:
         to_channel( str + offset, "Info", level );
         break;
      case LOG_AUTH:
         to_channel( str + offset, "Auth", level );
         break;
      case LOG_ALL:
         break;
   }
   return;
}

void log_printf_plus( short log_type, short level, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   log_string_plus( buf, log_type, level );
}

void log_printf( const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   log_string_plus( buf, LOG_NORMAL, LEVEL_LOG );
}

/*************************************************************/
/* Function to delete a room index.  Called from do_rdelete in build.c
   Narn, May/96
   Don't ask me why they return bool.. :).. oh well.. -- Alty
   Don't ask me either, so I changed it to void. - Samson
*/
void delete_room( ROOM_INDEX_DATA * room )
{
   int hash;
   char filename[256];
   ROOM_INDEX_DATA *prev, *limbo = get_room_index( ROOM_VNUM_LIMBO );
   OBJ_DATA *o;
   CHAR_DATA *ch;
   EXTRA_DESCR_DATA *ed;
   EXIT_DATA *ex, *pexit;
   MPROG_ACT_LIST *mpact;
   MPROG_DATA *mp;
   AREA_DATA *pArea;

   UNLINK( room, room->area->first_room, room->area->last_room, next_aroom, prev_aroom );

   while( ( ch = room->first_person ) != NULL )
   {
      if( !IS_NPC( ch ) )
      {
         char_from_room( ch );
         if( !char_to_room( ch, limbo ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      }
      else
         extract_char( ch, TRUE );
   }

   for( ch = first_char; ch; ch = ch->next )
   {
      if( ch->was_in_room == room )
         ch->was_in_room = ch->in_room;
      if( ch->substate == SUB_ROOM_DESC && ch->pcdata->dest_buf == room )
      {
         send_to_char( "The room is no more.\r\n", ch );
         stop_editing( ch );
         ch->substate = SUB_NONE;
         ch->pcdata->dest_buf = NULL;
      }
      else if( ch->substate == SUB_ROOM_EXTRA &&ch->pcdata->dest_buf )
      {
         for( ed = room->first_extradesc; ed; ed = ed->next )
         {
            if( ed == ch->pcdata->dest_buf )
            {
               send_to_char( "The room is no more.\r\n", ch );
               stop_editing( ch );
               ch->substate = SUB_NONE;
               ch->pcdata->dest_buf = NULL;
               break;
            }
         }
      }
   }

   while( ( o = room->first_content ) != NULL )
      extract_obj( o );

   wipe_resets( room );

   while( ( ed = room->first_extradesc ) != NULL )
   {
      room->first_extradesc = ed->next;
      STRFREE( ed->keyword );
      STRFREE( ed->extradesc );
      DISPOSE( ed );
      --top_ed;
   }

   if( !mud_down )
   {
      for( ex = room->first_exit; ex; ex = ex->next )
      {
         pArea = ex->to_room->area;

         if( ( ( pexit = ex->rexit ) != NULL ) && pexit != ex )
         {
            extract_exit( ex->to_room, pexit );

            if( pArea != room->area )
            {
               if( !IS_AREA_FLAG( pArea, AFLAG_PROTOTYPE ) )
                  mudstrlcpy( filename, pArea->filename, 256 );
               else
                  snprintf( filename, 256, "%s%s", BUILD_DIR, pArea->filename );
               fold_area( pArea, filename, FALSE );
            }
         }
      }
   }

   while( ( ex = room->first_exit ) != NULL )
      extract_exit( room, ex );
   while( ( mpact = room->mpact ) != NULL )
   {
      room->mpact = mpact->next;
      DISPOSE( mpact->buf );
      DISPOSE( mpact );
   }
   while( ( mp = room->mudprogs ) != NULL )
   {
      room->mudprogs = mp->next;
      STRFREE( mp->arglist );
      STRFREE( mp->comlist );
      DISPOSE( mp );
   }
   STRFREE( room->name );
   DISPOSE( room->roomdesc );
   DISPOSE( room->nitedesc );

   hash = room->vnum % MAX_KEY_HASH;
   if( room == room_index_hash[hash] )
      room_index_hash[hash] = room->next;
   else
   {
      for( prev = room_index_hash[hash]; prev; prev = prev->next )
         if( prev->next == room )
            break;
      if( prev )
         prev->next = room->next;
      else
         bug( "%s: room %d not in hash bucket %d.", __FUNCTION__, room->vnum, hash );
   }
   DISPOSE( room );
   --top_room;
   return;
}

/* See comment on delete_room. */
void delete_obj( OBJ_INDEX_DATA * obj )
{
   int hash;
   OBJ_INDEX_DATA *prev;
   OBJ_DATA *o, *o_next;
   EXTRA_DESCR_DATA *ed;
   AFFECT_DATA *af;
   MPROG_DATA *mp;
   CHAR_DATA *ch;

   /*
    * Remove references to object index 
    */
   for( o = first_object; o; o = o_next )
   {
      o_next = o->next;
      if( o->pIndexData == obj )
         extract_obj( o );
   }

   for( ch = first_char; ch; ch = ch->next )
   {
      if( ch->substate == SUB_OBJ_EXTRA && ch->pcdata->dest_buf )
      {
         for( ed = obj->first_extradesc; ed; ed = ed->next )
         {
            if( ed == ch->pcdata->dest_buf )
            {
               send_to_char( "You suddenly forget which object you were editing!\r\n", ch );
               stop_editing( ch );
               ch->substate = SUB_NONE;
               break;
            }
         }
      }
      else if( ch->substate == SUB_MPROG_EDIT && ch->pcdata->dest_buf )
      {
         for( mp = obj->mudprogs; mp; mp = mp->next )
         {
            if( mp == ch->pcdata->dest_buf )
            {
               send_to_char( "You suddenly forget which object you were working on.\r\n", ch );
               stop_editing( ch );
               ch->pcdata->dest_buf = NULL;
               ch->substate = SUB_NONE;
               break;
            }
         }
      }
   }

   while( ( ed = obj->first_extradesc ) != NULL )
   {
      obj->first_extradesc = ed->next;
      STRFREE( ed->keyword );
      STRFREE( ed->extradesc );
      DISPOSE( ed );
      --top_ed;
   }

   while( ( af = obj->first_affect ) != NULL )
   {
      obj->first_affect = af->next;
      DISPOSE( af );
      --top_affect;
   }

   while( ( mp = obj->mudprogs ) != NULL )
   {
      obj->mudprogs = mp->next;
      STRFREE( mp->arglist );
      STRFREE( mp->comlist );
      DISPOSE( mp );
   }

   STRFREE( obj->name );
   STRFREE( obj->short_descr );
   STRFREE( obj->objdesc );
   STRFREE( obj->action_desc );
   STRFREE( obj->socket[0] );
   STRFREE( obj->socket[1] );
   STRFREE( obj->socket[2] );

   hash = obj->vnum % MAX_KEY_HASH;
   if( obj == obj_index_hash[hash] )
      obj_index_hash[hash] = obj->next;
   else
   {
      for( prev = obj_index_hash[hash]; prev; prev = prev->next )
         if( prev->next == obj )
            break;
      if( prev )
         prev->next = obj->next;
      else
         bug( "%s: object %d not in hash bucket %d.", __FUNCTION__, obj->vnum, hash );
   }
   DISPOSE( obj );
   --top_obj_index;
   return;
}

/* See comment on delete_room. */
void delete_mob( MOB_INDEX_DATA * mob )
{
   int hash;
   MOB_INDEX_DATA *prev;
   CHAR_DATA *ch, *ch_next;
   MPROG_DATA *mp;

   for( ch = first_char; ch; ch = ch_next )
   {
      ch_next = ch->next;

      if( ch->pIndexData == mob )
         extract_char( ch, TRUE );
      else if( ch->substate == SUB_MPROG_EDIT && ch->pcdata->dest_buf )
      {
         for( mp = mob->mudprogs; mp; mp = mp->next )
         {
            if( mp == ch->pcdata->dest_buf )
            {
               send_to_char( "Your victim has departed.\r\n", ch );
               stop_editing( ch );
               ch->pcdata->dest_buf = NULL;
               ch->substate = SUB_NONE;
               break;
            }
         }
      }
   }

   while( ( mp = mob->mudprogs ) != NULL )
   {
      mob->mudprogs = mp->next;
      STRFREE( mp->arglist );
      STRFREE( mp->comlist );
      DISPOSE( mp );
   }

   if( mob->pShop )
   {
      UNLINK( mob->pShop, first_shop, last_shop, next, prev );
      DISPOSE( mob->pShop );
      --top_shop;
   }

   if( mob->rShop )
   {
      UNLINK( mob->rShop, first_repair, last_repair, next, prev );
      DISPOSE( mob->rShop );
      --top_repair;
   }

   STRFREE( mob->player_name );
   STRFREE( mob->short_descr );
   STRFREE( mob->long_descr );
   STRFREE( mob->chardesc );
   STRFREE( mob->spec_funname );

   hash = mob->vnum % MAX_KEY_HASH;
   if( mob == mob_index_hash[hash] )
      mob_index_hash[hash] = mob->next;
   else
   {
      for( prev = mob_index_hash[hash]; prev; prev = prev->next )
         if( prev->next == mob )
            break;
      if( prev )
         prev->next = mob->next;
      else
         bug( "delete_mob: mobile %d not in hash bucket %d.", mob->vnum, hash );
   }
   mobs_deleted++;
   DISPOSE( mob );
   --top_mob_index;
   return;
}

/*
 * Creat a new room (for online building)			-Thoric
 */
ROOM_INDEX_DATA *make_room( int vnum, AREA_DATA *area )
{
   ROOM_INDEX_DATA *pRoomIndex;
   int iHash;

   CREATE( pRoomIndex, ROOM_INDEX_DATA, 1 );
   pRoomIndex->first_reset = pRoomIndex->last_reset = NULL;
   pRoomIndex->first_person = NULL;
   pRoomIndex->last_person = NULL;
   pRoomIndex->first_content = NULL;
   pRoomIndex->last_content = NULL;
   pRoomIndex->first_extradesc = NULL;
   pRoomIndex->last_extradesc = NULL;
   pRoomIndex->area = area;
   pRoomIndex->vnum = vnum;
   pRoomIndex->winter_sector = -1;
   pRoomIndex->name = STRALLOC( "Engulfed in a Swirling Mass of Void Space" );
   xCLEAR_BITS( pRoomIndex->room_flags );
   SET_ROOM_FLAG( pRoomIndex, ROOM_PROTOTYPE );
   pRoomIndex->sector_type = 0;
   pRoomIndex->light = 0;
   pRoomIndex->first_exit = NULL;
   pRoomIndex->last_exit = NULL;
   LINK( pRoomIndex, area->first_room, area->last_room, next_aroom, prev_aroom );

   iHash = vnum % MAX_KEY_HASH;
   pRoomIndex->next = room_index_hash[iHash];
   room_index_hash[iHash] = pRoomIndex;
   ++top_room;

   return pRoomIndex;
}

/*
 * Create a new INDEX object (for online building)		-Thoric
 * Option to clone an existing index object.
 */
OBJ_INDEX_DATA *make_object( int vnum, int cvnum, char *name )
{
   OBJ_INDEX_DATA *pObjIndex, *cObjIndex;
   AREA_DATA *area;
   int iHash;

   if( cvnum > 0 )
      cObjIndex = get_obj_index( cvnum );
   else
      cObjIndex = NULL;
   CREATE( pObjIndex, OBJ_INDEX_DATA, 1 );
   pObjIndex->vnum = vnum;
   pObjIndex->name = STRALLOC( name );
   pObjIndex->first_affect = NULL;
   pObjIndex->last_affect = NULL;
   pObjIndex->first_extradesc = NULL;
   pObjIndex->last_extradesc = NULL;

   for( area = first_area; area; area = area->next )
      if( vnum >= area->low_vnum && vnum <= area->hi_vnum )
         pObjIndex->area = area;

   if( !cObjIndex )
   {
      stralloc_printf( &pObjIndex->short_descr, "A newly created %s", name );
      stralloc_printf( &pObjIndex->objdesc, "Some god dropped a newly created %s here.", name );
      pObjIndex->socket[0] = STRALLOC( "None" );
      pObjIndex->socket[1] = STRALLOC( "None" );
      pObjIndex->socket[2] = STRALLOC( "None" );
      pObjIndex->short_descr[0] = LOWER( pObjIndex->short_descr[0] );
      pObjIndex->objdesc[0] = UPPER( pObjIndex->objdesc[0] );
      pObjIndex->item_type = ITEM_TRASH;
      xCLEAR_BITS( pObjIndex->extra_flags );
      SET_OBJ_FLAG( pObjIndex, ITEM_PROTOTYPE );
      pObjIndex->wear_flags = 0;
      pObjIndex->value[0] = 0;
      pObjIndex->value[1] = 0;
      pObjIndex->value[2] = 0;
      pObjIndex->value[3] = 0;
      pObjIndex->value[4] = 0;
      pObjIndex->value[5] = 0;
      pObjIndex->value[6] = 0;
      pObjIndex->value[7] = 0;
      pObjIndex->value[8] = 0;
      pObjIndex->value[9] = 0;
      pObjIndex->value[10] = 0;
      pObjIndex->weight = 1;
      pObjIndex->cost = 0;
      pObjIndex->rent = -2;
      pObjIndex->limit = 9999;
   }
   else
   {
      EXTRA_DESCR_DATA *ed, *ced;
      AFFECT_DATA *paf, *cpaf;

      pObjIndex->short_descr = QUICKLINK( cObjIndex->short_descr );
      if( cObjIndex->objdesc && cObjIndex->objdesc[0] != '\0' )
         pObjIndex->objdesc = QUICKLINK( cObjIndex->objdesc );
      if( cObjIndex->action_desc && cObjIndex->action_desc[0] != '\0' )
         pObjIndex->action_desc = QUICKLINK( cObjIndex->action_desc );
      pObjIndex->socket[0] = QUICKLINK( cObjIndex->socket[0] );
      pObjIndex->socket[1] = QUICKLINK( cObjIndex->socket[1] );
      pObjIndex->socket[2] = QUICKLINK( cObjIndex->socket[2] );
      pObjIndex->item_type = cObjIndex->item_type;
      pObjIndex->extra_flags = cObjIndex->extra_flags;
      SET_OBJ_FLAG( pObjIndex, ITEM_PROTOTYPE );
      pObjIndex->wear_flags = cObjIndex->wear_flags;
      pObjIndex->value[0] = cObjIndex->value[0];
      pObjIndex->value[1] = cObjIndex->value[1];
      pObjIndex->value[2] = cObjIndex->value[2];
      pObjIndex->value[3] = cObjIndex->value[3];
      pObjIndex->value[4] = cObjIndex->value[4];
      pObjIndex->value[5] = cObjIndex->value[5];
      pObjIndex->value[6] = cObjIndex->value[6];
      pObjIndex->value[7] = cObjIndex->value[7];
      pObjIndex->value[8] = cObjIndex->value[8];
      pObjIndex->value[9] = cObjIndex->value[9];
      pObjIndex->value[10] = cObjIndex->value[10];
      pObjIndex->weight = cObjIndex->weight;
      pObjIndex->cost = cObjIndex->cost;
      pObjIndex->rent = cObjIndex->rent;
      pObjIndex->limit = cObjIndex->limit;
      for( ced = cObjIndex->first_extradesc; ced; ced = ced->next )
      {
         CREATE( ed, EXTRA_DESCR_DATA, 1 );
         ed->keyword = QUICKLINK( ced->keyword );
         if( ced->extradesc && ced->extradesc[0] != '\0' )
            ed->extradesc = QUICKLINK( ced->extradesc );
         LINK( ed, pObjIndex->first_extradesc, pObjIndex->last_extradesc, next, prev );
         top_ed++;
      }
      for( cpaf = cObjIndex->first_affect; cpaf; cpaf = cpaf->next )
      {
         CREATE( paf, AFFECT_DATA, 1 );
         paf->type = cpaf->type;
         paf->duration = cpaf->duration;
         paf->location = cpaf->location;
         paf->modifier = cpaf->modifier;
         paf->bit = cpaf->bit;
         paf->rismod = cpaf->rismod;
         LINK( paf, pObjIndex->first_affect, pObjIndex->last_affect, next, prev );
         top_affect++;
      }
   }
   pObjIndex->count = 0;
   iHash = vnum % MAX_KEY_HASH;
   pObjIndex->next = obj_index_hash[iHash];
   obj_index_hash[iHash] = pObjIndex;
   top_obj_index++;

   return pObjIndex;
}

/*
 * Create a new INDEX mobile (for online building)		-Thoric
 * Option to clone an existing index mobile.
 */
MOB_INDEX_DATA *make_mobile( int vnum, int cvnum, char *name )
{
   MOB_INDEX_DATA *pMobIndex, *cMobIndex;
   AREA_DATA *area;
   int iHash;

   if( cvnum > 0 )
      cMobIndex = get_mob_index( cvnum );
   else
      cMobIndex = NULL;
   CREATE( pMobIndex, MOB_INDEX_DATA, 1 );
   pMobIndex->vnum = vnum;
   pMobIndex->count = 0;
   pMobIndex->killed = 0;
   pMobIndex->player_name = STRALLOC( name );

   for( area = first_area; area; area = area->next )
      if( vnum >= area->low_vnum && vnum <= area->hi_vnum )
         pMobIndex->area = area;

   if( !cMobIndex )
   {
      stralloc_printf( &pMobIndex->short_descr, "A newly created %s", name );
      stralloc_printf( &pMobIndex->long_descr, "Some god abandoned a newly created %s here.\n\r", name );
      pMobIndex->short_descr[0] = LOWER( pMobIndex->short_descr[0] );
      pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );
      xCLEAR_BITS( pMobIndex->act );
      SET_ACT_FLAG( pMobIndex, ACT_IS_NPC );
      SET_ACT_FLAG( pMobIndex, ACT_PROTOTYPE );
      xCLEAR_BITS( pMobIndex->affected_by );
      pMobIndex->pShop = NULL;
      pMobIndex->rShop = NULL;
      pMobIndex->spec_fun = NULL;
      pMobIndex->mudprogs = NULL;
      xCLEAR_BITS( pMobIndex->progtypes );
      pMobIndex->alignment = 0;
      pMobIndex->level = 1;
      pMobIndex->mobthac0 = 21;
      pMobIndex->exp = -1;
      pMobIndex->ac = 0;
      pMobIndex->hitnodice = 0;
      pMobIndex->hitsizedice = 0;
      pMobIndex->hitplus = 0;
      pMobIndex->damnodice = 0;
      pMobIndex->damsizedice = 0;
      pMobIndex->damplus = 0;
      pMobIndex->hitroll = 0;
      pMobIndex->damroll = 0;
      pMobIndex->max_move = 150;
      pMobIndex->max_mana = 100;
      pMobIndex->gold = 0;
      pMobIndex->position = POS_STANDING;
      pMobIndex->defposition = POS_STANDING;
      pMobIndex->sex = SEX_NEUTRAL;
      pMobIndex->perm_str = 13;
      pMobIndex->perm_dex = 13;
      pMobIndex->perm_int = 13;
      pMobIndex->perm_wis = 13;
      pMobIndex->perm_cha = 13;
      pMobIndex->perm_con = 13;
      pMobIndex->perm_lck = 13;
      pMobIndex->race = RACE_HUMAN;
      pMobIndex->Class = CLASS_WARRIOR;
      pMobIndex->xflags = 0;
      xCLEAR_BITS( pMobIndex->resistant );
      xCLEAR_BITS( pMobIndex->immune );
      xCLEAR_BITS( pMobIndex->susceptible );
      xCLEAR_BITS( pMobIndex->absorb );
      pMobIndex->numattacks = 0;
      xCLEAR_BITS( pMobIndex->attacks );
      xCLEAR_BITS( pMobIndex->defenses );
      pMobIndex->height = 0;
      pMobIndex->weight = 0;
      pMobIndex->speaks = LANG_COMMON;
      pMobIndex->speaking = LANG_COMMON;
   }
   else
   {
      pMobIndex->short_descr = QUICKLINK( cMobIndex->short_descr );
      pMobIndex->long_descr = QUICKLINK( cMobIndex->long_descr );
      if( cMobIndex->chardesc && cMobIndex->chardesc[0] != '\0' )
         pMobIndex->chardesc = QUICKLINK( cMobIndex->chardesc );
      pMobIndex->act = cMobIndex->act;
      SET_ACT_FLAG( pMobIndex, ACT_PROTOTYPE );
      pMobIndex->affected_by = cMobIndex->affected_by;
      pMobIndex->pShop = NULL;
      pMobIndex->rShop = NULL;
      pMobIndex->spec_fun = cMobIndex->spec_fun;
      pMobIndex->mudprogs = NULL;
      xCLEAR_BITS( pMobIndex->progtypes );
      pMobIndex->alignment = cMobIndex->alignment;
      pMobIndex->level = cMobIndex->level;
      pMobIndex->mobthac0 = cMobIndex->mobthac0;
      pMobIndex->ac = cMobIndex->ac;
      pMobIndex->hitnodice = cMobIndex->hitnodice;
      pMobIndex->hitsizedice = cMobIndex->hitsizedice;
      pMobIndex->hitplus = cMobIndex->hitplus;
      pMobIndex->damnodice = cMobIndex->damnodice;
      pMobIndex->damsizedice = cMobIndex->damsizedice;
      pMobIndex->damplus = cMobIndex->damplus;
      pMobIndex->hitroll = 0; /* Yes, this is right. We don't want them to
                               * pMobIndex->damroll    = 0;    retain this when saved - Samson 5-5-00 */
      pMobIndex->gold = cMobIndex->gold;
      pMobIndex->exp = cMobIndex->exp;
      pMobIndex->position = cMobIndex->position;
      pMobIndex->defposition = cMobIndex->defposition;
      pMobIndex->sex = cMobIndex->sex;
      pMobIndex->perm_str = cMobIndex->perm_str;
      pMobIndex->perm_dex = cMobIndex->perm_dex;
      pMobIndex->perm_int = cMobIndex->perm_int;
      pMobIndex->perm_wis = cMobIndex->perm_wis;
      pMobIndex->perm_cha = cMobIndex->perm_cha;
      pMobIndex->perm_con = cMobIndex->perm_con;
      pMobIndex->perm_lck = cMobIndex->perm_lck;
      pMobIndex->race = cMobIndex->race;
      pMobIndex->Class = cMobIndex->Class;
      pMobIndex->xflags = cMobIndex->xflags;
      pMobIndex->resistant = cMobIndex->resistant;
      pMobIndex->immune = cMobIndex->immune;
      pMobIndex->susceptible = cMobIndex->susceptible;
      pMobIndex->absorb = cMobIndex->absorb;
      pMobIndex->numattacks = cMobIndex->numattacks;
      pMobIndex->attacks = cMobIndex->attacks;
      pMobIndex->defenses = cMobIndex->defenses;
      pMobIndex->height = cMobIndex->height;
      pMobIndex->weight = cMobIndex->weight;
      pMobIndex->speaks = cMobIndex->speaks;
      pMobIndex->speaking = cMobIndex->speaking;
   }
   iHash = vnum % MAX_KEY_HASH;
   pMobIndex->next = mob_index_hash[iHash];
   mob_index_hash[iHash] = pMobIndex;
   top_mob_index++;

   return pMobIndex;
}

/*
 * Creates a simple exit with no fields filled but rvnum and optionally
 * to_room and vnum.						-Thoric
 * Exits are inserted into the linked list based on vdir.
 */
EXIT_DATA *make_exit( ROOM_INDEX_DATA * pRoomIndex, ROOM_INDEX_DATA * to_room, short door )
{
   EXIT_DATA *pexit, *texit;
   bool broke;

   CREATE( pexit, EXIT_DATA, 1 );
   pexit->vdir = door;
   pexit->rvnum = pRoomIndex->vnum;
   pexit->to_room = to_room;
   xCLEAR_BITS( pexit->exit_info );
   pexit->x = 0;
   pexit->y = 0;
   if( to_room )
   {
      pexit->vnum = to_room->vnum;
      texit = get_exit_to( to_room, rev_dir[door], pRoomIndex->vnum );
      if( texit ) /* assign reverse exit pointers */
      {
         texit->rexit = pexit;
         pexit->rexit = texit;
      }
   }
   broke = FALSE;
   for( texit = pRoomIndex->first_exit; texit; texit = texit->next )
      if( door < texit->vdir )
      {
         broke = TRUE;
         break;
      }
   if( !pRoomIndex->first_exit )
      pRoomIndex->first_exit = pexit;
   else
   {
      /*
       * keep exits in incremental order - insert exit into list 
       */
      if( broke && texit )
      {
         if( !texit->prev )
            pRoomIndex->first_exit = pexit;
         else
            texit->prev->next = pexit;
         pexit->prev = texit->prev;
         pexit->next = texit;
         texit->prev = pexit;
         top_exit++;
         return pexit;
      }
      pRoomIndex->last_exit->next = pexit;
   }
   pexit->next = NULL;
   pexit->prev = pRoomIndex->last_exit;
   pRoomIndex->last_exit = pexit;
   top_exit++;
   return pexit;
}

void fix_area_exits( AREA_DATA * tarea )
{
   ROOM_INDEX_DATA *pRoomIndex;
   EXIT_DATA *pexit, *rv_exit;
   int rnum;

   for( rnum = tarea->low_vnum; rnum <= tarea->hi_vnum; rnum++ )
   {
      if( ( pRoomIndex = get_room_index( rnum ) ) == NULL )
         continue;

      for( pexit = pRoomIndex->first_exit; pexit; pexit = pexit->next )
      {
         pexit->rvnum = pRoomIndex->vnum;
         if( pexit->vnum <= 0 )
            pexit->to_room = NULL;
         else
            pexit->to_room = get_room_index( pexit->vnum );
      }
   }

   for( rnum = tarea->low_vnum; rnum <= tarea->hi_vnum; rnum++ )
   {
      if( ( pRoomIndex = get_room_index( rnum ) ) == NULL )
         continue;

      for( pexit = pRoomIndex->first_exit; pexit; pexit = pexit->next )
      {
         if( pexit->to_room && !pexit->rexit )
         {
            rv_exit = get_exit_to( pexit->to_room, rev_dir[pexit->vdir], pRoomIndex->vnum );
            if( rv_exit )
            {
               pexit->rexit = rv_exit;
               rv_exit->rexit = pexit;
            }
         }
      }
   }
}

/*
 * Sort by names -Altrag & Thoric
 * Modified by Samson to cut out all the extra damn lists.
 */
void sort_area_name( AREA_DATA * pArea )
{
   AREA_DATA *temp_area;

   if( !pArea )
   {
      bug( "Sort_area_name: NULL pArea" );
      return;
   }

   for( temp_area = first_area_nsort; temp_area; temp_area = temp_area->next_sort_name )
   {
      if( strcmp( pArea->name, temp_area->name ) < 0 )
      {
         INSERT( pArea, temp_area, first_area_nsort, next_sort_name, prev_sort_name );
         break;
      }
   }
   if( !temp_area )
      LINK( pArea, first_area_nsort, last_area_nsort, next_sort_name, prev_sort_name );
   return;
}

/*
 * Sort by Vnums -Altrag & Thoric
 * Modified by Samson to cut out all the extra damn lists.
 */
void sort_area_vnums( AREA_DATA * pArea )
{
   AREA_DATA *area;

   if( !pArea )
   {
      bug( "%s", "Sort_area_vnums: NULL pArea" );
      return;
   }

   for( area = first_area_vsort; area; area = area->next_sort )
      if( pArea->low_vnum < area->low_vnum )
         break;

   if( !area )
      LINK( pArea, first_area_vsort, last_area_vsort, next_sort, prev_sort );
   else
      INSERT( pArea, area, first_area_vsort, next_sort, prev_sort );
   return;
}

/* Warns if the settings are improperly done */
void validate_treasure_settings( area_data * area )
{
   if( area->tg_nothing > area->tg_gold && area->tg_gold != 0 )
   {
      log_printf( "%s: Nothing setting is larger than gold setting.", area->filename );
      if( area->tg_nothing > 100 )
      {
         log_printf( "%s: Nothing setting exceeds 100%%, correcting.", area->filename );
         area->tg_nothing = 100;
      }
   }

   if( area->tg_gold > area->tg_item && area->tg_item != 0 )
   {
      log_printf( "%s: Gold setting is larger than item setting.", area->filename );
      if( area->tg_gold > 100 )
      {
         log_printf( "%s: Gold setting exceeds 100%%, correcting.", area->filename );
         area->tg_gold = 100;
      }
   }

   if( area->tg_item > area->tg_gem && area->tg_gem != 0 )
   {
      log_printf( "%s: Item setting is larger than gem setting.", area->filename );
      if( area->tg_item > 100 )
      {
         log_printf( "%s: Item setting exceeds 100%%, correcting.", area->filename );
         area->tg_item = 100;
      }
   }

   if( area->tg_gem > 100 )
   {
      log_printf( "%s: Gem setting exceeds 100%%, correcting.", area->filename );
      area->tg_gem = 100;
   }

   if( area->tg_scroll > area->tg_potion && area->tg_potion != 0 )
   {
      log_printf( "%s: Scroll setting is larger than potion setting.", area->filename );
      if( area->tg_scroll > 100 )
      {
         log_printf( "%s: Scroll setting exceeds 100%%, correcting.", area->filename );
         area->tg_scroll = 100;
      }
   }

   if( area->tg_potion > area->tg_wand && area->tg_wand != 0 )
   {
      log_printf( "%s: Potion setting is larger than wand setting.", area->filename );
      if( area->tg_wand > 100 )
      {
         log_printf( "%s: Wand setting exceeds 100%%, correcting.", area->filename );
         area->tg_wand = 100;
      }
   }

   if( area->tg_wand > area->tg_armor && area->tg_armor != 0 )
   {
      log_printf( "%s: Wand setting is larger than armor setting.", area->filename );
      if( area->tg_wand > 100 )
      {
         log_printf( "%s: Wand setting exceeds 100%%, correcting.", area->filename );
         area->tg_wand = 100;
      }
   }

   if( area->tg_armor > 100 )
   {
      log_printf( "%s: Armor setting exceeds 100%%, correcting.", area->filename );
      area->tg_armor = 100;
   }

   return;
}

void load_area_file( AREA_DATA * tarea, char *filename, bool isproto )
{
   if( fBootDb )
      tarea = last_area;
   if( !fBootDb && !tarea )
   {
      bug( "%s: null area!", __FUNCTION__ );
      return;
   }

   if( !( fpArea = fopen( filename, "r" ) ) )
   {
      perror( filename );
      bug( "%s: error loading file (can't open) %s", __FUNCTION__, filename );
      return;
   }
   for( ;; )
   {
      char *word;

      if( !fpArea )  /* Should only happen if a stock conversion takes place */
         return;

      if( fread_letter( fpArea ) != '#' )
      {
         bug( "%s: # not found %s", __FUNCTION__, tarea->filename );
         exit( 1 );
      }

      word = fread_word( fpArea );

      if( word[0] == '$' )
         break;
      else if( !str_cmp( word, "VERSION" ) && !tarea )
      {
         int aversion = fread_number( fpArea );

         if( aversion == 1000 )
         {
            FCLOSE( fpArea );
            do_areaconvert( NULL, filename );
            return;
         }
         bug( "%s: %s bad section name %s (1)", __FUNCTION__, tarea->filename, word );
         if( fBootDb )
            exit( 1 );
         else
         {
            FCLOSE( fpArea );
            return;
         }
      }
      else if( !str_cmp( word, "AREA" ) )
      {
         if( fBootDb )
         {
            load_area( fpArea );
            tarea = last_area;
            tarea->version = 0;
         }
         else
         {
            DISPOSE( tarea->name );
            tarea->name = fread_string_nohash( fpArea );
            tarea->x = 0;
            tarea->y = 0;
            tarea->version = 0;
         }
      }
      else if( !str_cmp( word, "AUTHOR" ) )
         load_author( tarea, fpArea, filename );
      else if( !str_cmp( word, "VNUMS" ) )
         load_vnums( tarea, fpArea );
      else if( !str_cmp( word, "FLAGS" ) )
         load_flags( tarea, fpArea );
      /*
       * Frequency loader - Samson 5-10-99 
       */
      else if( !str_cmp( word, "RESETFREQUENCY" ) )
         load_resetfreq( tarea, fpArea );
      else if( !str_cmp( word, "RANGES" ) )
         load_ranges( tarea, fpArea );
      else if( !str_cmp( word, "COORDS" ) )
         load_coords( tarea, fpArea );
      else if( !str_cmp( word, "CONTINENT" ) )
         load_continent( tarea, fpArea );
      else if( !str_cmp( word, "RESETMSG" ) )
         load_resetmsg( tarea, fpArea );
      /*
       * Rennard 
       */
      else if( !str_cmp( word, "HELPS" ) )
         load_helps( tarea, fpArea );
      else if( !str_cmp( word, "MOBILES" ) )
         load_mobiles( tarea, fpArea );
      else if( !str_cmp( word, "OBJECTS" ) )
         load_objects( tarea, fpArea );
      else if( !str_cmp( word, "RESETS" ) )
         load_resets( tarea, fpArea );
      else if( !str_cmp( word, "ROOMS" ) )
         load_rooms( tarea, fpArea );
      else if( !str_cmp( word, "SHOPS" ) )
         load_shops( tarea, fpArea );
      else if( !str_cmp( word, "REPAIRS" ) )
         load_repairs( tarea, fpArea );
      else if( !str_cmp( word, "SPECIALS" ) )
         load_specials( tarea, fpArea );
      else if( !str_cmp( word, "CLIMATE" ) )
         load_climate( tarea, fpArea );
      else if( !str_cmp( word, "NEIGHBOR" ) )
         load_neighbor( tarea, fpArea );
      else if( !str_cmp( word, "VERSION" ) )
         load_version( tarea, fpArea, filename );
      else if( !str_cmp( word, "TREASURE" ) )
      {
         short x1, x2, x3, x4;
         char *ln = fread_line( fpArea );

         x1 = 20;
         x2 = 74;
         x3 = 85;
         x4 = 93;
         sscanf( ln, "%hu %hu %hu %hu", &x1, &x2, &x3, &x4 );

         tarea->tg_nothing = x1;
         tarea->tg_gold = x2;
         tarea->tg_item = x3;
         tarea->tg_gem = x4;

         ln = fread_line( fpArea );

         x1 = 20;
         x2 = 50;
         x3 = 60;
         x4 = 75;
         sscanf( ln, "%hu %hu %hu %hu", &x1, &x2, &x3, &x4 );

         tarea->tg_scroll = x1;
         tarea->tg_potion = x2;
         tarea->tg_wand = x3;
         tarea->tg_armor = x4;

         validate_treasure_settings( tarea );
      }
      else
      {
         bug( "%s: %s bad section name %s (2)", __FUNCTION__, tarea->filename, word );
         if( fBootDb )
            exit( 1 );
         else
         {
            FCLOSE( fpArea );
            return;
         }
      }
   }
   FCLOSE( fpArea );
   if( tarea )
   {
      sort_area_name( tarea );
      sort_area_vnums( tarea );
      if( isproto )
         SET_BIT( tarea->flags, AFLAG_PROTOTYPE );
      log_printf( "%-20s: Version %-3d Vnums: %5d - %-5d", tarea->filename, tarea->version, tarea->low_vnum,
                  tarea->hi_vnum );
      if( tarea->low_vnum < 0 || tarea->hi_vnum < 0 )
         log_printf( "%-24s: Bad Vnum Range\n", tarea->filename );
      if( !tarea->author )
         tarea->author = STRALLOC( "Alsherok" );
   }
   else
      log_printf( "(%s)", filename );
}

/* Displays zone list. Will show proto, non-proto, or all. */
void show_vnums( CHAR_DATA * ch, short proto )
{
   AREA_DATA *pArea;
   int count = 0;

   pager_printf( ch, "&W%-15.15s %-40.40s %5.5s\n\r\n\r", "Filename", "Area Name", "Vnums" );
   for( pArea = first_area_vsort; pArea; pArea = pArea->next_sort )
   {
      if( proto == 0 && !IS_AREA_FLAG( pArea, AFLAG_PROTOTYPE ) )
      {
         pager_printf( ch, "&c%-15.15s %-40.40s V: %5d - %-5d\n\r",
                       pArea->filename, pArea->name, pArea->low_vnum, pArea->hi_vnum );
         count++;
      }
      else if( proto == 1 && IS_AREA_FLAG( pArea, AFLAG_PROTOTYPE ) )
      {
         pager_printf( ch, "&c%-15.15s %-40.40s V: %5d - %-5d &W[Proto]\n\r",
                       pArea->filename, pArea->name, pArea->low_vnum, pArea->hi_vnum );
         count++;
      }
      else if( proto == 2 )
      {
         pager_printf( ch, "&c%-15.15s %-40.40s V: %5d - %-5d %s\n\r",
                       pArea->filename, pArea->name, pArea->low_vnum, pArea->hi_vnum,
                       IS_AREA_FLAG( pArea, AFLAG_PROTOTYPE ) ? "&W[Proto]" : "" );
         count++;
      }
   }
   pager_printf( ch, "&CAreas listed: %d\n\r", count );
   pager_printf( ch, "Maximum allowed vnum is currently %d.&D\n\r", sysdata.maxvnum );
   return;
}

CMDF do_zones( CHAR_DATA * ch, char *argument )
{
   char arg[MIL], arg2[MIL];

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( !arg || arg[0] == '\0' )
   {
      show_vnums( ch, 0 );
      return;
   }
   if( !str_cmp( arg, "proto" ) )
   {
      show_vnums( ch, 1 );
      return;
   }
   if( !str_cmp( arg, "all" ) )
   {
      show_vnums( ch, 2 );
      return;
   }
   send_to_char( "Usage: zones [proto/all]\n\r", ch );
   return;
}

/* Revised version of do_areas, orgininally written by Fireblade 4/27/97,
 * rewritten by Dwip to fix horrid argument bugs 4/14/02.  Happy 5 year
 * anniversary, do_areas! :)
 */
CMDF do_areas( CHAR_DATA * ch, char *argument )
{
   char *header_string1 = "\n\r   Author    |             Area                     | Recommended |  Enforced\n\r";
   char *header_string2 = "-------------+--------------------------------------+-------------+-----------\n\r";
   char *print_string = "%-12s | %-36s | %4d - %-4d | %3d - %-3d \n\r";

   AREA_DATA *pArea;
   int lower_bound = 0;
   int upper_bound = MAX_LEVEL + 1;
   int num_args = 0;
   /*
    * 0-2 = x arguments, 3 = old style 
    */

   int swap;
   char arg[MSL];

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' )
      num_args = 0;
   else
   {
      if( !is_number( arg ) )
      {
         if( !str_cmp( arg, "old" ) )
            num_args = 3;
         else
         {
            send_to_char( "Area may only be followed by numbers or old.\n\r", ch );
            return;
         }
      }
      else
      {
         num_args = 1;

         upper_bound = atoi( arg );
         /*
          * Will need to swap this with ubound later 
          */

         argument = one_argument( argument, arg );

         if( arg[0] != '\0' )
         {
            if( !is_number( arg ) )
            {
               send_to_char( "Area may only be followed by numbers or old.\n\r", ch );
               return;
            }
            num_args = 2;

            lower_bound = upper_bound;
            upper_bound = atoi( arg );
         }

         argument = one_argument( argument, arg );

         if( arg[0] != '\0' )
         {
            send_to_char( "Only two level numbers allowed.\n\r", ch );
            return;
         }
      }

      if( lower_bound > upper_bound )
      {
         swap = lower_bound;
         lower_bound = upper_bound;
         upper_bound = swap;
      }
   }
   set_pager_color( AT_PLAIN, ch );
   send_to_pager( header_string1, ch );
   send_to_pager( header_string2, ch );

   switch ( num_args )
   {
      case 0:
         for( pArea = first_area_nsort; pArea; pArea = pArea->next_sort_name )
         {
            pager_printf( ch, print_string, pArea->author, pArea->name, pArea->low_soft_range,
                          pArea->hi_soft_range, pArea->low_hard_range, pArea->hi_hard_range );
         }
         break;

      case 1:
         for( pArea = first_area_nsort; pArea; pArea = pArea->next_sort_name )
         {
            if( pArea->hi_soft_range >= upper_bound && pArea->low_soft_range <= upper_bound )
            {
               pager_printf( ch, print_string, pArea->author, pArea->name, pArea->low_soft_range,
                             pArea->hi_soft_range, pArea->low_hard_range, pArea->hi_hard_range );
            }
         }
         break;

      case 2:
         for( pArea = first_area_nsort; pArea; pArea = pArea->next_sort_name )
         {
            if( pArea->hi_soft_range >= upper_bound && pArea->low_soft_range <= lower_bound )
            {
               pager_printf( ch, print_string, pArea->author, pArea->name, pArea->low_soft_range,
                             pArea->hi_soft_range, pArea->low_hard_range, pArea->hi_hard_range );
            }
         }
         break;

      case 3:
         for( pArea = first_area_nsort; pArea; pArea = pArea->next_sort_name )
         {
            pager_printf( ch, print_string, pArea->author, pArea->name, pArea->low_soft_range,
                          pArea->hi_soft_range, pArea->low_hard_range, pArea->hi_hard_range );
         }
         break;

      default:
         for( pArea = first_area_nsort; pArea; pArea = pArea->next_sort_name )
         {
            pager_printf( ch, print_string, pArea->author, pArea->name, pArea->low_soft_range,
                          pArea->hi_soft_range, pArea->low_hard_range, pArea->hi_hard_range );
         }
         break;
   }
   return;
}

/* Shogar's code to hunt for exits/entrances to/from a zone, very nice - Samson 12-30-00 */
CMDF do_aexit( CHAR_DATA * ch, char *argument )
{
   ROOM_INDEX_DATA *room;
   AREA_DATA *tarea, *otherarea;
   EXIT_DATA *pexit;
   ENTRANCE_DATA *enter;
   int i, vnum, lrange, trange;
   bool found = FALSE;

   if( argument[0] == '\0' )
      tarea = ch->in_room->area;
   else
   {
      for( tarea = first_area; tarea; tarea = tarea->next )
         if( !str_cmp( tarea->filename, argument ) )
         {
            found = TRUE;
            break;
         }

      if( !found )
      {
         send_to_char( "Area not found.\n\r", ch );
         return;
      }
   }

   trange = tarea->hi_vnum;
   lrange = tarea->low_vnum;

   for( vnum = lrange; vnum <= trange; vnum++ )
   {
      if( !( room = get_room_index( vnum ) ) )
         continue;

      if( IS_ROOM_FLAG( room, ROOM_TELEPORT ) && ( room->tele_vnum < lrange || room->tele_vnum > trange ) )
      {
         pager_printf( ch, "From: %-20.20s Room: %5d To: Room: %5d (Teleport)\n\r", tarea->filename, vnum, room->tele_vnum );
      }

      for( i = 0; i < MAX_DIR + 2; i++ )  /* MAX_DIR+2 added to include ? exits.  Dwip 5/7/02 */
      {
         if( ( pexit = get_exit( room, i ) ) == NULL )
            continue;

         if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
         {
            pager_printf( ch, "To: Overland %4dX %4dY From: %20.20s Room: %5d (%s)\n\r",
                          pexit->x, pexit->y, tarea->filename, vnum, dir_name[i] );
            continue;
         }
         if( pexit->to_room->area != tarea )
         {
            pager_printf( ch, "To: %-20.20s Room: %5d From: %-20.20s Room: %5d (%s)\n\r",
                          pexit->to_room->area->filename, pexit->vnum, tarea->filename, vnum, dir_name[i] );
         }
      }
   }

   for( otherarea = first_area; otherarea; otherarea = otherarea->next )
   {
      if( tarea == otherarea )
         continue;
      trange = otherarea->hi_vnum;
      lrange = otherarea->low_vnum;
      for( vnum = lrange; vnum <= trange; vnum++ )
      {
         if( !( room = get_room_index( vnum ) ) )
            continue;

         if( IS_ROOM_FLAG( room, ROOM_TELEPORT ) )
         {
            if( room->tele_vnum >= tarea->low_vnum && room->tele_vnum <= tarea->hi_vnum )
               pager_printf( ch, "From: %-20.20s Room: %5d To: %-20.20s Room: %5d (Teleport)\n\r",
                             otherarea->filename, vnum, tarea->filename, room->tele_vnum );
         }

         for( i = 0; i < MAX_DIR + 2; i++ )
         {
            if( !( pexit = get_exit( room, i ) ) )
               continue;

            if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
               continue;

            if( pexit->to_room->area == tarea )
            {
               pager_printf( ch, "From: %-20.20s Room: %5d To: %-20.20s Room: %5d (%s)\n\r",
                             otherarea->filename, vnum, pexit->to_room->area->filename, pexit->vnum, dir_name[i] );
            }
         }
      }
   }
   for( enter = first_entrance; enter; enter = enter->next )
   {
      if( enter->vnum >= tarea->low_vnum && enter->vnum <= tarea->hi_vnum )
         pager_printf( ch, "From: Overland %4dX %4dY To: Room: %5d\n\r", enter->herex, enter->herey, enter->vnum );
   }
   return;
}

/* Similar to checkvnum, but will list the freevnums -- Xerves 3-11-01 */
CMDF do_freevnums( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   AREA_DATA *pArea;
   bool area_conflict;
   int low_range, high_range, xfin = 0, w = 0, x = 0, y = 0, z = 0;
   int lohi[600]; /* Up to 300 areas, increase if you have more -- Xerves */

   argument = one_argument( argument, arg1 );

   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Please specify the low end of the range to be searched.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Please specify the high end of the range to be searched.\n\r", ch );
      return;
   }

   low_range = atoi( arg1 );
   high_range = atoi( argument );

   if( low_range < 1 || low_range > sysdata.maxvnum )
   {
      ch_printf( ch, "Invalid low range. Valid vnums are from 1 to %d.\n\r", sysdata.maxvnum );
      return;
   }

   if( high_range < 1 || high_range > sysdata.maxvnum )
   {
      ch_printf( ch, "Invalid high range. Valid vnums are from 1 to %d.\n\r", sysdata.maxvnum );
      return;
   }

   if( high_range < low_range )
   {
      send_to_char( "Low range must be below high range.\n\r", ch );
      return;
   }

   set_char_color( AT_PLAIN, ch );

   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      area_conflict = FALSE;

      if( low_range < pArea->low_vnum && pArea->low_vnum < high_range )
         area_conflict = TRUE;

      if( low_range < pArea->hi_vnum && pArea->hi_vnum < high_range )
         area_conflict = TRUE;

      if( ( low_range >= pArea->low_vnum ) && ( low_range <= pArea->hi_vnum ) )
         area_conflict = TRUE;

      if( ( high_range <= pArea->hi_vnum ) && ( high_range >= pArea->low_vnum ) )
         area_conflict = TRUE;

      if( area_conflict )
      {
         lohi[x] = pArea->low_vnum;
         x++;
         lohi[x] = pArea->hi_vnum;
         x++;
      }
   }
   xfin = x;
   for( y = low_range; y < high_range; y = y + 50 )
   {
      area_conflict = FALSE;
      z = y + 49; /* y is min, z is max */
      for( x = 0; x < xfin; x = x + 2 )
      {
         w = x + 1;

         if( y < lohi[x] && lohi[x] < z )
         {
            area_conflict = TRUE;
            break;
         }
         if( y < lohi[w] && lohi[w] < z )
         {
            area_conflict = TRUE;
            break;
         }
         if( y >= lohi[x] && y <= lohi[w] )
         {
            area_conflict = TRUE;
            break;
         }
         if( z <= lohi[w] && z >= lohi[x] )
         {
            area_conflict = TRUE;
            break;
         }
      }
      if( area_conflict == FALSE )
         ch_printf( ch, "Open: %5d - %-5d\n\r", y, z );
   }
   return;
}

/* Check to make sure range of vnums is free - Scryn 2/27/96 */
CMDF do_check_vnums( CHAR_DATA * ch, char *argument )
{
   AREA_DATA *pArea;
   char arg[MSL];
   bool area_conflict;
   int low_range, high_range;

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Please specify the low end of the range to be searched.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Please specify the high end of the range to be searched.\n\r", ch );
      return;
   }

   low_range = atoi( arg );
   high_range = atoi( argument );

   if( low_range < 1 || low_range > sysdata.maxvnum )
   {
      send_to_char( "Invalid argument for bottom of range.\n\r", ch );
      return;
   }

   if( high_range < 1 || high_range > sysdata.maxvnum )
   {
      send_to_char( "Invalid argument for top of range.\n\r", ch );
      return;
   }

   if( high_range < low_range )
   {
      send_to_char( "Bottom of range must be below top of range.\n\r", ch );
      return;
   }

   set_char_color( AT_PLAIN, ch );

   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      area_conflict = FALSE;

      if( low_range < pArea->low_vnum && pArea->low_vnum < high_range )
         area_conflict = TRUE;

      if( low_range < pArea->hi_vnum && pArea->hi_vnum < high_range )
         area_conflict = TRUE;

      if( ( low_range >= pArea->low_vnum ) && ( low_range <= pArea->hi_vnum ) )
         area_conflict = TRUE;

      if( ( high_range <= pArea->hi_vnum ) && ( high_range >= pArea->low_vnum ) )
         area_conflict = TRUE;

      if( area_conflict )
      {
         ch_printf( ch, "Conflict:%-15s| ", ( pArea->filename ? pArea->filename : "(invalid)" ) );
         ch_printf( ch, "Vnums: %5d - %-5d\n\r", pArea->low_vnum, pArea->hi_vnum );
      }
   }
   return;
}

/*  Dump command...This command creates a text file with the stats of every  *
 *  mob, or object in the mud, depending on the argument given.              *
 *  Obviously, this will tend to create HUGE files, so it is recommended     *
 *  that it be only given to VERY high level imms, and preferably those      *
 *  with shell access, so that they may clean it out, when they are done     *
 *  with it.
 */
CMDF do_dump( CHAR_DATA * ch, char *argument )
{
   MOB_INDEX_DATA *mob;
   OBJ_INDEX_DATA *obj;
   ROOM_INDEX_DATA *room;
   AFFECT_DATA *paf;

   int counter;

   if( IS_NPC( ch ) )
      return;

   if( !IS_IMP( ch ) )
   {
      send_to_char( "Sorry, only an Implementor may use this command!\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "mobs" ) )
   {
      FILE *fp = fopen( "../mobdata.txt", "w" );
      send_to_char( "Writing to file...\n\r", ch );

      for( counter = 0; counter < sysdata.maxvnum; counter++ )
      {
         if( ( mob = get_mob_index( counter ) ) != NULL )
         {
            fprintf( fp, "VNUM:  %d\n", mob->vnum );
            fprintf( fp, "S_DESC:  %s\n", mob->short_descr );
            fprintf( fp, "LEVEL:  %d\n", mob->level );
            fprintf( fp, "HITROLL:  %d\n", mob->hitroll );
            fprintf( fp, "DAMROLL:  %d\n", mob->damroll );
            fprintf( fp, "HITDIE:  %dd%d+%d\n", mob->hitnodice, mob->hitsizedice, mob->hitplus );
            fprintf( fp, "DAMDIE:  %dd%d+%d\n", mob->damnodice, mob->damsizedice, mob->damplus );
            fprintf( fp, "ACT FLAGS:  %s\n", ext_flag_string( &mob->act, act_flags ) );
            fprintf( fp, "AFFECTED_BY:  %s\n", affect_bit_name( &mob->affected_by ) );
            fprintf( fp, "RESISTS:  %s\n", ext_flag_string( &mob->resistant, ris_flags ) );
            fprintf( fp, "SUSCEPTS:  %s\n", ext_flag_string( &mob->susceptible, ris_flags ) );
            fprintf( fp, "IMMUNE:  %s\n", ext_flag_string( &mob->immune, ris_flags ) );
            fprintf( fp, "ABSORB:  %s\n", ext_flag_string( &mob->absorb, ris_flags ) );
            fprintf( fp, "ATTACKS:  %s\n", ext_flag_string( &mob->attacks, attack_flags ) );
            fprintf( fp, "DEFENSES:  %s\n\n\n", ext_flag_string( &mob->defenses, defense_flags ) );
         }
      }
      FCLOSE( fp );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "rooms" ) )
   {
      FILE *fp = fopen( "../roomdata.txt", "w" );
      send_to_char( "Writing to file...\n\r", ch );

      for( counter = 0; counter < sysdata.maxvnum; counter++ )
      {
         if( ( room = get_room_index( counter ) ) != NULL )
         {
            fprintf( fp, "VNUM:  %d\n", room->vnum );
            fprintf( fp, "NAME:  %s\n", room->name );
            fprintf( fp, "SECTOR:  %s\n", sect_types[room->sector_type] );
            fprintf( fp, "FLAGS:  %s\n\n\n", ext_flag_string( &room->room_flags, r_flags ) );
         }
      }
      FCLOSE( fp );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "objects" ) )
   {
      FILE *fp = fopen( "../objdata.txt", "w" );
      send_to_char( "Writing objects to file...\n\r", ch );

      for( counter = 0; counter < sysdata.maxvnum; counter++ )
      {
         if( ( obj = get_obj_index( counter ) ) != NULL )
         {
            fprintf( fp, "VNUM: %d\n", obj->vnum );
            fprintf( fp, "KEYWORDS: %s\n", obj->name );
            fprintf( fp, "TYPE: %s\n", o_types[obj->item_type] );
            fprintf( fp, "SHORT DESC: %s\n", obj->short_descr );
            fprintf( fp, "WEARFLAGS: %s\n", flag_string( obj->wear_flags, w_flags ) );
            fprintf( fp, "FLAGS: %s\n", ext_flag_string( &obj->extra_flags, o_flags ) );
            fprintf( fp, "WEIGHT: %d\n", obj->weight );
            fprintf( fp, "COST: %d\n", obj->cost );
            fprintf( fp, "RENT: %d\n", obj->rent );
            fprintf( fp, "ZONE: %s\n", obj->area->name );
            fprintf( fp, "%s", "AFFECTS:\n" );

            for( paf = obj->first_affect; paf; paf = paf->next )
               fprintf( fp, "%s %d: \n", affect_loc_name( paf->location ), paf->modifier );

            if( obj->layers > 0 )
               fprintf( fp, "Layerable - Wear layer: %d\n", obj->layers );

            fprintf( fp, "VAL0: %d\n", obj->value[0] );
            fprintf( fp, "VAL1: %d\n", obj->value[1] );
            fprintf( fp, "VAL2: %d\n", obj->value[2] );
            fprintf( fp, "VAL3: %d\n", obj->value[3] );
            fprintf( fp, "VAL4: %d\n", obj->value[4] );
            fprintf( fp, "VAL5: %d\n", obj->value[5] );
         }
      }
      FCLOSE( fp );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   send_to_char( "Syntax: dump <mobs/objects>\n\r", ch );
   return;
}

/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain( void )
{
   return;
}

/*
 * Extended Bitvector Routines					-Thoric
 */

/* check to see if the extended bitvector is completely empty */
bool ext_is_empty( EXT_BV * bits )
{
   int x;

   for( x = 0; x < XBI; x++ )
      if( bits->bits[x] != 0 )
         return FALSE;

   return TRUE;
}

void ext_clear_bits( EXT_BV * bits )
{
   int x;

   for( x = 0; x < XBI; x++ )
      bits->bits[x] = 0;
}

/* for use by xHAS_BITS() -- works like IS_SET() */
int ext_has_bits( EXT_BV * var, EXT_BV * bits )
{
   int x, bit;

   for( x = 0; x < XBI; x++ )
      if( ( bit = ( var->bits[x] & bits->bits[x] ) ) != 0 )
         return bit;

   return 0;
}

/* for use by xSAME_BITS() -- works like == */
bool ext_same_bits( EXT_BV * var, EXT_BV * bits )
{
   int x;

   for( x = 0; x < XBI; x++ )
      if( var->bits[x] != bits->bits[x] )
         return FALSE;

   return TRUE;
}

/* for use by xSET_BITS() -- works like SET_BIT() */
void ext_set_bits( EXT_BV * var, EXT_BV * bits )
{
   int x;

   for( x = 0; x < XBI; x++ )
      var->bits[x] |= bits->bits[x];
}

/* for use by xREMOVE_BITS() -- works like REMOVE_BIT() */
void ext_remove_bits( EXT_BV * var, EXT_BV * bits )
{
   int x;

   for( x = 0; x < XBI; x++ )
      var->bits[x] &= ~( bits->bits[x] );
}

/* for use by xTOGGLE_BITS() -- works like TOGGLE_BIT() */
void ext_toggle_bits( EXT_BV * var, EXT_BV * bits )
{
   int x;

   for( x = 0; x < XBI; x++ )
      var->bits[x] ^= bits->bits[x];
}

/*
 * Read an extended bitvector from a file.			-Thoric
 */
EXT_BV fread_bitvector( FILE * fp )
{
   EXT_BV ret;
   int c, x = 0;
   int num = 0;

   memset( &ret, '\0', sizeof( ret ) );
   for( ;; )
   {
      num = fread_number( fp );
      if( x < XBI )
         ret.bits[x] = num;
      ++x;
      if( ( c = getc( fp ) ) != '&' )
      {
         ungetc( c, fp );
         break;
      }
   }

   return ret;
}

/* return a string for writing a bitvector to a file */
char *print_bitvector( EXT_BV * bits )
{
   static char buf[XBI * 12];
   char *p = buf;
   int x, cnt = 0;

   for( cnt = XBI - 1; cnt > 0; cnt-- )
      if( bits->bits[cnt] )
         break;
   for( x = 0; x <= cnt; x++ )
   {
      snprintf( p, ( XBI * 12 ) - ( p - buf ), "%d", bits->bits[x] );
      p += strlen( p );
      if( x < cnt )
         *p++ = '&';
   }
   *p = '\0';

   return buf;
}

EXT_BV meb( int bit )
{
   EXT_BV bits;

   xCLEAR_BITS( bits );
   if( bit >= 0 )
      xSET_BIT( bits, bit );

   return bits;
}

EXT_BV multimeb( int bit, ... )
{
   EXT_BV bits;
   va_list param;
   int b;

   xCLEAR_BITS( bits );
   if( bit < 0 )
      return bits;

   xSET_BIT( bits, bit );

   va_start( param, bit );

   while( ( b = va_arg( param, int ) ) != -1 )
        xSET_BIT( bits, b );

   va_end( param );
   return bits;
}
