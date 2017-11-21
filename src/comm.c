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
 *                     Low-level communication module                       *
 ****************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <dlfcn.h>
#if defined(__CYGWIN__)
#define WAIT_ANY -1  /* This is not guaranteed to work! */
#ifdef OLD_CRYPT
#include <crypt.h>
#endif
#endif
#include "mud.h"
#include "alias.h"
#include "auction.h"
#include "ban.h"
#include "calendar.h"
#include "channels.h"
#include "connhist.h"
#include "dns.h"
#include "fight.h"
#ifdef I3
#include "i3.h"
#endif
#ifdef IMC
#include "imc.h"
#endif
#include "mccp.h"
#include "md5.h"
#include "msp.h"
#include "mxp.h"
#include "new_auth.h"
#include "pfiles.h"
#include "polymorph.h"
#include "sha256.h"
#ifdef MULTIPORT
#include "shell.h"
#endif

#ifdef sun
int gethostname( char *name, int namelen );
#endif

/* Terminal detection stuff start */
#define  IS                 '\x00'
#define  TERMINAL_TYPE      '\x18'
#define  SEND	          '\x01'

const char term_call_back_str[] = { IAC, SB, TERMINAL_TYPE, IS };
const char req_termtype_str[] = { IAC, SB, TERMINAL_TYPE, SEND, IAC, SE, '\0' };
const char do_term_type[] = { IAC, DO, TERMINAL_TYPE, '\0' };

/* Terminal detection stuff end */

const char echo_off_str[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const char echo_on_str[] = { IAC, WONT, TELOPT_ECHO, '\0' };
const char go_ahead_str[] = { IAC, GA, '\0' };

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

/*
 * Global variables.
 */
DESCRIPTOR_DATA *first_descriptor;  /* First descriptor     */
DESCRIPTOR_DATA *last_descriptor;   /* Last descriptor      */
DESCRIPTOR_DATA *d_next;   /* Next descriptor in loop */
DNS_DATA *first_cache;
DNS_DATA *last_cache;
int num_descriptors;
int num_logins;
bool mud_down; /* Shutdown       */
char str_boot_time[MIL];
char lastplayercmd[MIL * 2];
time_t current_time; /* Time of this pulse      */
int control;   /* Controlling descriptor  */
int newdesc;   /* New descriptor    */
fd_set in_set; /* Set of desc's for reading  */
fd_set out_set;   /* Set of desc's for writing  */
fd_set exc_set;   /* Set of desc's with errors  */
int maxdesc;
char *alarm_section = "(unknown)";
bool winter_freeze = FALSE;
int port;
bool DONTSAVE = FALSE;  /* For reboots, shutdowns, etc. */
bool bootlock = FALSE;
bool sigsegv = FALSE;
int crash_count = 0;
bool DONT_UPPER;
#ifdef MULTIPORT
extern bool compilelock;
#endif
extern char will_msp_str[];
extern char will_mxp_str[];

#ifdef IMC
void free_imcdata( bool complete );
void imc_delete_info( void );
#endif
#ifdef I3
void destroy_I3_mud( I3_MUD * mud );
void free_i3data( bool freerouters );
#endif

void game_loop( void );
void cleanup_memory( void );

/*
 * External functions
 */
void boot_db( bool fCopyOver );
#ifdef WEBSVR
   /*
    * Web Server Handler Functions 
    */
void handle_web( void );
bool init_web( int webport, int wsocket, bool hotboot );
void shutdown_web( bool hotboot );
#endif
#ifdef I3
void I3_main( bool forced, int mudport, bool isconnected );
void I3_shutdown( int delay );
void I3_loop( void );
I3_CHANNEL *find_I3_channel_by_localname( char *name );
#endif

void invert( char *arg1, char *arg2 );
void color_send_to_desc( const char *txt, DESCRIPTOR_DATA * d );
void sale_count( CHAR_DATA * ch );  /* For new auction system - Samson 6-24-99 */
void save_sysdata( SYSTEM_DATA sys );
void save_auth_list( void );
void check_clan_info( CHAR_DATA * ch );
CMDF do_help( CHAR_DATA * ch, char *argument );
CMDF do_destroy( CHAR_DATA * ch, char *argument );
void bid( CHAR_DATA * ch, CHAR_DATA * buyer, char *argument );
ALIAS_DATA *find_alias( CHAR_DATA * ch, char *argument );
AUTH_LIST *get_auth_name( char *name );
void check_auth_state( CHAR_DATA * ch );
void scan_rent( CHAR_DATA * ch );
MUD_CHANNEL *find_channel( char *name );
#ifdef MULTIPORT
#if !defined(__CYGWIN__)
void mud_recv_message( void );
#endif
#endif
HOLIDAY_DATA *get_holiday( short month, short day );
void save_ships( void );
void oedit_parse( DESCRIPTOR_DATA * d, char *arg );
void medit_parse( DESCRIPTOR_DATA * d, char *arg );
void redit_parse( DESCRIPTOR_DATA * d, char *arg );
bool is_inolc( DESCRIPTOR_DATA * d );
void send_msp_startup( DESCRIPTOR_DATA * d );
void music_to_char( const char *fname, int volume, CHAR_DATA * ch, bool toroom );
bool check_immortal_domain( CHAR_DATA * ch, char *host );
void send_mxp_stylesheet( DESCRIPTOR_DATA * d );
bool check_total_bans( DESCRIPTOR_DATA * d );
void save_timedata( void );
void edit_buffer( CHAR_DATA * ch, char *argument );
void name_stamp_stats( CHAR_DATA * ch );
void break_camp( CHAR_DATA * ch );
void setup_newbie( CHAR_DATA * ch, bool NEWLOGIN );
bool arg_cmp( char *haystack, char *needle );
void mprog_act_trigger( char *buf, CHAR_DATA * mob, CHAR_DATA * ch, OBJ_DATA * obj, void *vo );
void save_morphs( void );
char *attribtext( int attribute );
void quotes( CHAR_DATA * ch );
void hotboot_recover( void );
void update_connhistory( DESCRIPTOR_DATA * d, int type );   /* connhist.c */
void free_connhistory( int arg );   /* connhist.c */
void board_parse( DESCRIPTOR_DATA * d, char *argument );

/* Used during memory cleanup */
void clean_obj_queue( void );
void clean_char_queue( void );
void free_morphs( void );
void free_quotes( void );
void free_envs( void );
void free_sales( void );
void free_mxpobj_cmds( void );
void free_bans( void );
void free_all_auths( void );
void free_runedata( void );
void free_slays( void );
void free_holidays( void );
void free_landings( void );
void free_ships( void );
void free_mapexits( void );
void free_landmarks( void );
void free_liquiddata( void );
void free_mudchannels( void );
void free_commands( void );
void free_deities( void );
void free_clans( void );
void free_socials( void );
void free_helps( void );
void free_watchlist( void );
void free_boards( void );
void free_teleports( void );
void close_all_areas( void );
void free_prog_actlists( void );
void free_questbits( void );
void free_projects( void );
void free_specfuns( void );
void clear_wizinfo( void );
void free_tongues( void );
void free_skills( void );
#ifdef MULTIPORT
void free_shellcommands( void );
#endif
void free_events( void );

void set_alarm( long seconds )
{
   alarm( seconds );
}

/*
* This is the MCCP version. Use write_to_descriptor_old to send non-compressed
* text.
* Updated to run with the block checks by Orion... if it doesn't work, blame
* him.;P -Orion
*/
bool write_to_descriptor( DESCRIPTOR_DATA * d, char *txt, int length )
{
   int iStart = 0;
   int nWrite = 0;
   int nBlock;
   int iErr;
   int len;

   if( length <= 0 )
      length = strlen( txt );

   if( d && d->mccp->out_compress )
   {
      d->mccp->out_compress->next_in = ( unsigned char * )txt;
      d->mccp->out_compress->avail_in = length;

      while( d->mccp->out_compress->avail_in )
      {
         d->mccp->out_compress->avail_out =
            COMPRESS_BUF_SIZE - ( d->mccp->out_compress->next_out - d->mccp->out_compress_buf );

         if( d->mccp->out_compress->avail_out )
         {
            int status = deflate( d->mccp->out_compress, Z_SYNC_FLUSH );

            if( status != Z_OK )
               return FALSE;
         }

         len = d->mccp->out_compress->next_out - d->mccp->out_compress_buf;
         if( len > 0 )
         {
            for( iStart = 0; iStart < len; iStart += nWrite )
            {
               nBlock = UMIN( len - iStart, 4096 );
               nWrite = send( d->descriptor, d->mccp->out_compress_buf + iStart, nBlock, 0 );
               if( nWrite == -1 )
               {
                  iErr = errno;
                  if( iErr == EWOULDBLOCK )
                  {
                     /*
                      * This is a SPAMMY little bug error. I would suggest
                      * not using it, but I've included it in case. -Orion
                      *
                      perror( "Write_to_descriptor: Send is blocking" );
                      */
                     nWrite = 0;
                     continue;
                  }
                  else
                  {
                     perror( "Write_to_descriptor" );
                     return FALSE;
                  }
               }

               if( !nWrite )
                  break;
            }

            if( !iStart )
               break;

            if( iStart < len )
               memmove( d->mccp->out_compress_buf, d->mccp->out_compress_buf + iStart, len - iStart );

            d->mccp->out_compress->next_out = d->mccp->out_compress_buf + len - iStart;
         }
      }
      return TRUE;
   }

   for( iStart = 0; iStart < length; iStart += nWrite )
   {
      nBlock = UMIN( length - iStart, 4096 );
      nWrite = send( d->descriptor, txt + iStart, nBlock, 0 );
      if( nWrite == -1 )
      {
         iErr = errno;
         if( iErr == EWOULDBLOCK )
         {
            /*
             * This is a SPAMMY little bug error. I would suggest
             * not using it, but I've included it in case. -Orion
             *
             perror( "Write_to_descriptor: Send is blocking" );
             */
            nWrite = 0;
            continue;
         }
         else
         {
            perror( "Write_to_descriptor" );
            return FALSE;
         }
      }
   }
   return TRUE;
}

/*
 *
 * Added block checking to prevent random booting of the descriptor. Thanks go
 * out to Rustry for his suggestions. -Orion
 */
bool write_to_descriptor_old( int desc, char *txt, int length )
{
   int iStart = 0;
   int nWrite = 0;
   int nBlock = 0;
   int iErr = 0;

   if( length <= 0 )
      length = strlen( txt );

   for( iStart = 0; iStart < length; iStart += nWrite )
   {
      nBlock = UMIN( length - iStart, 4096 );
      nWrite = send( desc, txt + iStart, nBlock, 0 );

      if( nWrite == -1 )
      {
         iErr = errno;
         if( iErr == EWOULDBLOCK )
         {
            /*
             * This is a SPAMMY little bug error. I would suggest
             * not using it, but I've included it in case. -Orion
             *
             perror( "Write_to_descriptor: Send is blocking" );
             */
            nWrite = 0;
            continue;
         }
         else
         {
            perror( "Write_to_descriptor" );
            return FALSE;
         }
      }
   }
   return TRUE;
}

bool read_from_descriptor( DESCRIPTOR_DATA * d )
{
   unsigned int iStart, iErr;

   /*
    * Hold horses if pending command already. 
    */
   if( d->incomm[0] != '\0' )
      return TRUE;

   /*
    * Check for overflow. 
    */
   iStart = strlen( d->inbuf );
   if( iStart >= sizeof( d->inbuf ) - 10 )
   {
      log_printf( "%s input overflow!", d->host );
      write_to_descriptor( d, "\n\r*** PUT A LID ON IT!!! ***\n\r", 0 );
      return FALSE;
   }

   for( ;; )
   {
      int nRead;

      nRead = recv( d->descriptor, d->inbuf + iStart, sizeof( d->inbuf ) - 10 - iStart, 0 );
      iErr = errno;
      if( nRead > 0 )
      {
         iStart += nRead;
         if( d->inbuf[iStart - 1] == '\n' || d->inbuf[iStart - 1] == '\r' )
            break;
      }
      else if( nRead == 0 && d->connected >= CON_PLAYING )
      {
         log_string_plus( "EOF encountered on read.", LOG_COMM, LEVEL_IMMORTAL );
         return FALSE;
      }
      else if( iErr == EWOULDBLOCK )
         break;
      else
      {
         perror( "Read_from_descriptor" );
         return FALSE;
      }
   }

   d->inbuf[iStart] = '\0';
   return TRUE;
}

void open_mud_log( void )
{
   FILE *error_log;
   char buf[256];
   int logindex;

   for( logindex = 1000;; logindex++ )
   {
      snprintf( buf, 256, "../log/%d.log", logindex );
      if( exists_file( buf ) )
         continue;
      else
         break;
   }

   if( !( error_log = fopen( buf, "a" ) ) )
   {
      fprintf( stderr, "Unable to append to %s.", buf );
      exit( 1 );
   }

   dup2( fileno( error_log ), STDERR_FILENO );
   FCLOSE( error_log );
}

int init_socket( int mudport )
{
   char hostname[64];
   struct sockaddr_in sa;
   int x = 1;
   int fd;

   gethostname( hostname, sizeof( hostname ) );

   if( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
   {
      perror( "Init_socket: socket" );
      exit( 1 );
   }

   if( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, ( void * )&x, sizeof( x ) ) < 0 )
   {
      perror( "Init_socket: SO_REUSEADDR" );
      close( fd );
      exit( 1 );
   }

#if defined(SO_DONTLINGER) && !defined(SYSV)
   {
      struct linger ld;

      ld.l_onoff = 1;
      ld.l_linger = 1000;

      if( setsockopt( fd, SOL_SOCKET, SO_DONTLINGER, ( void * )&ld, sizeof( ld ) ) < 0 )
      {
         perror( "Init_socket: SO_DONTLINGER" );
         close( fd );
         exit( 1 );
      }
   }
#endif

   memset( &sa, '\0', sizeof( sa ) );
   sa.sin_family = AF_INET;
   sa.sin_port = htons( mudport );

   /*
    * IP binding: uncomment if server requires it, and set x.x.x.x to proper IP - Samson 
    */
   /*
    * sa.sin_addr.s_addr = inet_addr( "x.x.x.x" ); 
    */

   if( bind( fd, ( struct sockaddr * )&sa, sizeof( sa ) ) == -1 )
   {
      perror( "Init_socket: bind" );
      close( fd );
      exit( 1 );
   }

   if( listen( fd, 50 ) < 0 )
   {
      perror( "Init_socket: listen" );
      close( fd );
      exit( 1 );
   }

   return fd;
}

char *const directory_table[] =
{
   AREA_CONVERT_DIR, PLAYER_DIR, GOD_DIR, BUILD_DIR, SYSTEM_DIR,
   PROG_DIR, CORPSE_DIR, CLASS_DIR, RACE_DIR, MOTD_DIR, HOTBOOT_DIR, AUC_DIR,
   BOARD_DIR, COLOR_DIR, WATCH_DIR, MAP_DIR
};

void directory_check( void )
{
   char buf[256];
   size_t x;

   // Successful directory check will drop this file in the area dir once done.
   if( exists_file( "DIR_CHECK_PASSED" ) )
      return;

   fprintf( stderr, "Checking for required directories...\n" );

   // This should really never happen but you never know.
   if( chdir( "../log" ) )
   {
      fprintf( stderr, "Creating required directory: ../log\n" );
      snprintf( buf, 256, "mkdir ../log" );

      if( system( buf ) )
      {
         fprintf( stderr, "FATAL ERROR :: Unable to create required directrory: ../log\n" );
         exit(1);
      }
   }

   for( x = 0; x < sizeof( directory_table ) / sizeof( directory_table[0] ); ++x )
   {
      if( chdir( directory_table[x] ) )
      {
         snprintf( buf, 256, "mkdir %s", directory_table[x] );
         log_printf( "Creating required directory: %s", directory_table[x] );

         if( system(buf) )
         {
            log_printf( "FATAL ERROR :: Unable to create required directory: %s. Must be corrected manually.", directory_table[x] );
            exit(1);
         }
      }

      if( !str_cmp( directory_table[x], PLAYER_DIR ) )
      {
         short alpha_loop;
         char dirname[256];

         for( alpha_loop = 0; alpha_loop <= 25; ++alpha_loop )
         {
            snprintf( dirname, 256, "%s%c", PLAYER_DIR, 'a' + alpha_loop );
            if( chdir( dirname ) )
            {
               log_printf( "Creating required directory: %s", dirname );
               snprintf( buf, 256, "mkdir %s", dirname );

               if( system(buf) )
               {
                  log_printf( "FATAL ERROR :: Unable to create required directory: %s. Must be corrected manually.", dirname );
                  exit(1);
               }
            }
            else
               chdir( ".." );
         }
      }
   }

   // Made it? Sweet. Drop the check file so we don't do this on every last reboot.
   log_string( "Directory check passed." );
   chdir( "../area" );
   system( "touch DIR_CHECK_PASSED" );
}

/* This functions purpose is to open up all of the various things the mud needs to have
 * up before the game_loop is entered. If something needs to be added to the mud
 * startup proceedures it should be placed in here.
 */
void init_mud( bool fCopyOver, int gameport, int wsocket, int imcsocket )
{
   // Scan for and create necessary dirs if they don't exit.
   directory_check();

   /*
    * If this all goes well, we should be able to open a new log file during hotboot 
    */
   if( fCopyOver )
   {
      open_mud_log(  );
      log_string( "Hotboot: Spawning new log file" );
   }

   log_string( "Booting Database" );
   boot_db( fCopyOver );

   if( !fCopyOver )  /* We have already the port if copyover'ed */
   {
      log_string( "Initializing main socket" );
      control = init_socket( gameport );
      log_string( "Main socket initialized" );
   }

#ifdef MULTIPORT
   switch ( gameport )
   {
      case MAINPORT:
         log_printf( "%s game server ready on port %d.", sysdata.mud_name, gameport );
         break;
      case BUILDPORT:
         log_printf( "%s builders' server ready on port %d.", sysdata.mud_name, gameport );
         break;
      case CODEPORT:
         log_printf( "%s coding server ready on port %d.", sysdata.mud_name, gameport );
         break;
      default:
         log_printf( "%s - running on unsupported port %d!!", sysdata.mud_name, gameport );
         break;
   }
#else
   log_printf( "%s ready on port %d.", sysdata.mud_name, gameport );
#endif

#ifdef I3
   /*
    * Initialize and connect to Intermud-3 
    */
   I3_main( FALSE, gameport, fCopyOver );
#endif

#ifdef IMC
   imc_startup( FALSE, imcsocket, fCopyOver );
#endif

#ifdef WEBSVR
   if( sysdata.webtoggle )
   {
      sysdata.webrunning = FALSE;

      if( !init_web( gameport + 1, wsocket, fCopyOver ) )
         log_string( "Unable to start webserver." );
      else
         sysdata.webrunning = TRUE;
   }
#endif

   if( fCopyOver )
   {
      log_string( "Initiating hotboot recovery." );
      hotboot_recover(  );
   }

   return;
}

/* This function is called from 'main' or 'SigTerm'. Its purpose is to clean up
 * the various loose ends the mud will have running before it shuts down. Put anything
 * which needs to be added to the shutdown proceedures in here.
 */
void close_mud( void )
{
   CHAR_DATA *vch;

   if( auction->item )
      bid( supermob, NULL, "stop" );

   if( !DONTSAVE )
   {
      log_string( "Saving players...." );
      for( vch = first_char; vch; vch = vch->next )
         if( !IS_NPC( vch ) )
         {
            save_char_obj( vch );
            log_printf( "%s saved.", vch->name );
            if( vch->desc )
               write_to_descriptor( vch->desc, "You have been saved to disk.\033[0m\n\r", 0 );
         }
   }

   /*
    * Save game world time - Samson 1-21-99 
    */
   log_string( "Saving game world time...." );
   save_timedata(  );

   /*
    * Save ship information - Samson 1-8-01 
    */
   save_ships(  );

   fflush( stderr ); /* make sure stderr is flushed */

#ifdef WEBSVR
   if( sysdata.webrunning )
      shutdown_web( FALSE );
#endif

   close( control );

#ifdef IMC
   imc_shutdown( FALSE );
#endif

#ifdef I3
   I3_shutdown( 0 );
#endif

   return;
}

bool check_bad_desc( int desc )
{
   if( FD_ISSET( desc, &exc_set ) )
   {
      FD_CLR( desc, &in_set );
      FD_CLR( desc, &out_set );
      log_string( "Bad FD caught and disposed." );
      return TRUE;
   }
   return FALSE;
}

void free_desc( DESCRIPTOR_DATA * d )
{
   close( d->descriptor );
   DISPOSE( d->host );
   DISPOSE( d->outbuf );
   DISPOSE( d->pagebuf );
   STRFREE( d->client );

   compressEnd( d );
   DISPOSE( d->mccp );

   DISPOSE( d );
   return;
}

void send_greeting( DESCRIPTOR_DATA * d )
{
   FILE *rpfile;
   int num = 0;
   char BUFF[MSL], filename[256];

   snprintf( filename, 256, "%sgreeting.dat", MOTD_DIR );
   if( ( rpfile = fopen( filename, "r" ) ) != NULL )
   {
      while( ( ( BUFF[num] = fgetc( rpfile ) ) != EOF ) && num < MSL - 1 )
         num++;
      FCLOSE( rpfile );
      BUFF[num] = '\0';
      color_send_to_desc( BUFF, d );
   }
}

void save_dns( void )
{
   DNS_DATA *cache;
   FILE *fp;
   char filename[256];

   snprintf( filename, 256, "%s", DNS_FILE );

   if( ( fp = fopen( filename, "w" ) ) == NULL )
   {
      bug( "%s", "save_dns: fopen" );
      perror( filename );
   }
   else
   {
      for( cache = first_cache; cache; cache = cache->next )
      {
         fprintf( fp, "%s", "#CACHE\n" );
         fprintf( fp, "IP		%s~\n", cache->ip );
         fprintf( fp, "Name		%s~\n", cache->name );
         fprintf( fp, "Time		%ld\n", ( long int )cache->time );
         fprintf( fp, "%s", "End\n\n" );
      }
      fprintf( fp, "%s", "#END\n" );
      FCLOSE( fp );
   }
   return;
}

void free_dns( DNS_DATA * cache )
{
   UNLINK( cache, first_cache, last_cache, next, prev );
   DISPOSE( cache->ip );
   DISPOSE( cache->name );
   DISPOSE( cache );
   return;
}

void prune_dns( void )
{
   DNS_DATA *cache, *cache_next;

   for( cache = first_cache; cache; cache = cache_next )
   {
      cache_next = cache->next;

      /*
       * Stay in cache for 14 days 
       */
      if( current_time - cache->time >= 1209600 || !str_cmp( cache->ip, "Unknown??" )
          || !str_cmp( cache->name, "Unknown??" ) )
         free_dns( cache );
   }
   save_dns(  );
   return;
}

void add_dns( char *dhost, char *address )
{
   DNS_DATA *cache;

   CREATE( cache, DNS_DATA, 1 );
   cache->ip = str_dup( dhost );
   cache->name = str_dup( address );
   cache->time = current_time;
   LINK( cache, first_cache, last_cache, next, prev );

   save_dns(  );
   return;
}

char *in_dns_cache( char *ip )
{
   DNS_DATA *cache;
   static char dnsbuf[MSL];

   dnsbuf[0] = '\0';

   for( cache = first_cache; cache; cache = cache->next )
   {
      if( !str_cmp( ip, cache->ip ) )
      {
         mudstrlcpy( dnsbuf, cache->name, MSL );
         break;
      }
   }
   return dnsbuf;
}

void fread_dns( DNS_DATA * cache, FILE * fp )
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
            {
               if( !cache->ip )
                  cache->ip = str_dup( "Unknown??" );
               if( !cache->name )
                  cache->name = str_dup( "Unknown??" );
               return;
            }
            break;

         case 'I':
            KEY( "IP", cache->ip, fread_string_nohash( fp ) );
            break;

         case 'N':
            KEY( "Name", cache->name, fread_string_nohash( fp ) );
            break;

         case 'T':
            KEY( "Time", cache->time, fread_number( fp ) );
            break;
      }

      if( !fMatch )
         bug( "fread_dns: no match: %s", word );
   }
}

void load_dns( void )
{
   char filename[256];
   DNS_DATA *cache;
   FILE *fp;

   first_cache = NULL;
   last_cache = NULL;

   snprintf( filename, 256, "%s", DNS_FILE );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
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
            bug( "%s", "load_dns: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "CACHE" ) )
         {
            CREATE( cache, DNS_DATA, 1 );
            fread_dns( cache, fp );
            LINK( cache, first_cache, last_cache, next, prev );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "load_dns: bad section: %s.", word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   prune_dns(  ); /* Clean out entries beyond 14 days */
   return;
}

/* DNS Resolver code by Trax of Forever's End */
/*
 * Almost the same as read_from_buffer...
 */
bool read_from_dns( int fd, char *buffer )
{
   static char inbuf[MSL * 2];
   unsigned int iStart, i, j, k;

   /*
    * Check for overflow. 
    */
   iStart = strlen( inbuf );
   if( iStart >= sizeof( inbuf ) - 10 )
   {
      bug( "%s", "DNS input overflow!!!" );
      return FALSE;
   }

   /*
    * Snarf input. 
    */
   for( ;; )
   {
      int nRead;

      nRead = read( fd, inbuf + iStart, sizeof( inbuf ) - 10 - iStart );
      if( nRead > 0 )
      {
         iStart += nRead;
         if( inbuf[iStart - 2] == '\n' || inbuf[iStart - 2] == '\r' )
            break;
      }
      else if( nRead == 0 )
      {
         return FALSE;
      }
      else if( errno == EWOULDBLOCK )
         break;
      else
      {
         perror( "Read_from_dns" );
         return FALSE;
      }
   }

   inbuf[iStart] = '\0';

   /*
    * Look for at least one new line.
    */
   for( i = 0; inbuf[i] != '\n' && inbuf[i] != '\r'; i++ )
   {
      if( inbuf[i] == '\0' )
         return FALSE;
   }

   /*
    * Canonical input processing.
    */
   for( i = 0, k = 0; inbuf[i] != '\n' && inbuf[i] != '\r'; i++ )
   {
      if( inbuf[i] == '\b' && k > 0 )
         --k;
      else if( isascii( inbuf[i] ) && isprint( inbuf[i] ) )
         buffer[k++] = inbuf[i];
   }

   /*
    * Finish off the line.
    */
   if( k == 0 )
      buffer[k++] = ' ';
   buffer[k] = '\0';

   /*
    * Shift the input buffer.
    */
   while( inbuf[i] == '\n' || inbuf[i] == '\r' )
      i++;
   for( j = 0; ( inbuf[j] = inbuf[i + j] ) != '\0'; j++ )
      ;

   return TRUE;
}

/* DNS Resolver code by Trax of Forever's End */
/*
 * Process input that we got from resolve_dns.
 */
void process_dns( DESCRIPTOR_DATA * d )
{
   char address[MIL];
   int status;

   address[0] = '\0';

   if( !read_from_dns( d->ifd, address ) || address[0] == '\0' )
      return;

   if( address[0] != '\0' )
   {
      add_dns( d->host, address );  /* Add entry to DNS cache */
      DISPOSE( d->host );
      d->host = str_dup( address );
      if( d->character )
      {
         DISPOSE( d->character->pcdata->lasthost );
         d->character->pcdata->lasthost = str_dup( address );
      }
   }

   /*
    * close descriptor and kill dns process 
    */
   if( d->ifd != -1 )
   {
      close( d->ifd );
      d->ifd = -1;
   }

   /*
    * we don't have to check here, 
    * cos the child is probably dead already. (but out of safety we do)
    * 
    * (later) I found this not to be TRUE. The call to waitpid( ) is
    * necessary, because otherwise the child processes become zombie
    * and keep lingering around... The waitpid( ) removes them.
    */
   if( d->ipid != -1 )
   {
      waitpid( d->ipid, &status, 0 );
      d->ipid = -1;
   }
   return;
}

/* DNS Resolver hook. Code written by Trax of Forever's End */
void resolve_dns( DESCRIPTOR_DATA * d, long ip )
{
   int fds[2];
#ifndef S_SPLINT_S
   pid_t pid;
#endif

   /*
    * create pipe first 
    */
   if( pipe( fds ) != 0 )
   {
      perror( "resolve_dns: pipe: " );
      return;
   }

   if( dup2( fds[1], STDOUT_FILENO ) != STDOUT_FILENO )
   {
      perror( "resolve_dns: dup2(stdout): " );
      return;
   }

   if( ( pid = fork(  ) ) > 0 )
   {
      /*
       * parent process 
       */
      d->ifd = fds[0];
      d->ipid = pid;
      close( fds[1] );
   }
   else if( pid == 0 )
   {
      /*
       * child process 
       */
      char str_ip[64];
      int i;

      d->ifd = fds[0];
      d->ipid = pid;

      for( i = 2; i < 255; ++i )
         close( i );

      snprintf( str_ip, 64, "%ld", ip );
#if defined(__CYGWIN__)
      execl( "../src/resolver.exe", "AFKMud Resolver", str_ip, ( char * )NULL );
#else
      execl( "../src/resolver", "AFKMud Resolver", str_ip, ( char * )NULL );
#endif
      /*
       * Still here --> hmm. An error. 
       */
      bug( "%s", "resolve_dns: Exec failed; Closing child." );
      d->ifd = -1;
      d->ipid = -1;
      exit( 0 );
   }
   else
   {
      /*
       * error 
       */
      perror( "resolve_dns: failed fork" );
      close( fds[0] );
      close( fds[1] );
   }
}

CMDF do_cache( CHAR_DATA * ch, char *argument )
{
   DNS_DATA *cache;
   int ip = 0;

   send_to_pager( "&YCached DNS Information\n\r", ch );
   send_to_pager( "IP               | Address\n\r", ch );
   send_to_pager( "------------------------------------------------------------------------------\n\r", ch );
   for( cache = first_cache; cache; cache = cache->next )
   {
      pager_printf( ch, "&W%16.16s  &Y%s\n\r", cache->ip, cache->name );
      ip++;
   }
   pager_printf( ch, "\n\r&W%d IPs in the cache.\n\r", ip );
   return;
}

void new_descriptor( int new_desc )
{
   char buf[MSL];
   DESCRIPTOR_DATA *dnew;
   struct sockaddr_in sock;
   int desc;
   socklen_t size;

   size = sizeof( sock );
   if( check_bad_desc( new_desc ) )
   {
      set_alarm( 0 );
      return;
   }
   set_alarm( 20 );
   alarm_section = "new_descriptor: accept";
   if( ( desc = accept( new_desc, ( struct sockaddr * )&sock, &size ) ) < 0 )
   {
      perror( "New_descriptor: accept" );
      set_alarm( 0 );
      return;
   }
   if( check_bad_desc( new_desc ) )
   {
      set_alarm( 0 );
      return;
   }

   set_alarm( 20 );
   alarm_section = "new_descriptor: after accept";

   if( fcntl( desc, F_SETFL, FNDELAY ) == -1 )
   {
      perror( "New_descriptor: fcntl: FNDELAY" );
      set_alarm( 0 );
      return;
   }
   if( check_bad_desc( new_desc ) )
      return;

   CREATE( dnew, DESCRIPTOR_DATA, 1 );
   dnew->next = NULL;
   dnew->process = 0;   /* Samson 4-16-98 - For new command pipe */
   dnew->descriptor = desc;
   dnew->connected = CON_GET_NAME;
   dnew->outsize = 2000;
   dnew->idle = 0;
   dnew->lines = 0;
   dnew->scrlen = 24;
   dnew->port = ntohs( sock.sin_port );
   dnew->newstate = 0;
   dnew->prevcolor = 0x08;
   dnew->ifd = -1;   /* Descriptor pipes, used for DNS resolution and such */
   dnew->ipid = -1;
   dnew->client = STRALLOC( "Unidentified" );   /* Terminal detect */
   dnew->msp_detected = FALSE;
   dnew->mxp_detected = FALSE;
   dnew->can_compress = FALSE;

   CREATE( dnew->mccp, MCCP, 1 );

   CREATE( dnew->outbuf, char, dnew->outsize );
   strdup_printf( &dnew->host, "%s", inet_ntoa( sock.sin_addr ) );

   if( !sysdata.NO_NAME_RESOLVING )
   {
      mudstrlcpy( buf, in_dns_cache( dnew->host ), MSL );

      if( buf[0] == '\0' )
         resolve_dns( dnew, sock.sin_addr.s_addr );
      else
      {
         DISPOSE( dnew->host );
         dnew->host = str_dup( buf );
      }
   }

   if( check_total_bans( dnew ) )
   {
      write_to_descriptor( dnew, "Your site has been banned from this Mud.\n\r", 0 );
      free_desc( dnew );
      set_alarm( 0 );
      return;
   }

   /*
    * Init descriptor data.
    */

   if( !last_descriptor && first_descriptor )
   {
      DESCRIPTOR_DATA *d;

      bug( "%s", "New_descriptor: last_desc is NULL, but first_desc is not! ...fixing" );
      for( d = first_descriptor; d; d = d->next )
         if( !d->next )
            last_descriptor = d;
   }

   LINK( dnew, first_descriptor, last_descriptor, next, prev );

   /*
    * Terminal detect 
    */
   write_to_buffer( dnew, do_term_type, 0 );

   /*
    * MCCP Compression 
    */
   write_to_buffer( dnew, will_compress2_str, 0 );

   /*
    * Mud eXtention Protocol 
    */
   write_to_buffer( dnew, will_mxp_str, 0 );

   /*
    * Mud Sound Protocol 
    */
   write_to_buffer( dnew, will_msp_str, 0 );

   /*
    * Send the greeting. No longer handled kludgely by a global variable.
    */
   send_greeting( dnew );

   write_to_buffer( dnew, "Enter your character's name, or type new: ", 0 );

   if( ++num_descriptors > sysdata.maxplayers )
      sysdata.maxplayers = num_descriptors;
   if( sysdata.maxplayers > sysdata.alltimemax )
   {
      strdup_printf( &sysdata.time_of_max, "%24.24s", c_time( current_time, -1 ) );
      sysdata.alltimemax = sysdata.maxplayers;
      log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "Broke all-time maximum player record: %d", sysdata.alltimemax );
      save_sysdata( sysdata );
   }
   set_alarm( 0 );
   return;
}

void accept_new( int ctrl )
{
   static struct timeval null_time;
   DESCRIPTOR_DATA *d;

   /*
    * Poll all active descriptors.
    */
   FD_ZERO( &in_set );
   FD_ZERO( &out_set );
   FD_ZERO( &exc_set );
   FD_SET( ctrl, &in_set );

   maxdesc = ctrl;
   newdesc = 0;
   for( d = first_descriptor; d; d = d->next )
   {
      maxdesc = UMAX( maxdesc, d->descriptor );
      FD_SET( d->descriptor, &in_set );
      FD_SET( d->descriptor, &out_set );
      FD_SET( d->descriptor, &exc_set );
      if( d->ifd != -1 && d->ipid != -1 )
      {
         maxdesc = UMAX( maxdesc, d->ifd );
         FD_SET( d->ifd, &in_set );
      }
      if( d == last_descriptor )
         break;
   }

   if( select( maxdesc + 1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
   {
      perror( "accept_new: select: poll" );
      exit( 1 );
   }

   if( FD_ISSET( ctrl, &exc_set ) )
   {
      bug( "Exception raise on controlling descriptor %d", ctrl );
      FD_CLR( ctrl, &in_set );
      FD_CLR( ctrl, &out_set );
   }
   else if( FD_ISSET( ctrl, &in_set ) )
   {
      newdesc = ctrl;
      new_descriptor( newdesc );
   }
}

/*
 * Determine whether this player is to be watched  --Gorog
 */
bool chk_watch( short player_level, char *player_name, char *player_site )
{
   WATCH_DATA *pw;

   if( !first_watch )
      return FALSE;

   for( pw = first_watch; pw; pw = pw->next )
   {
      if( pw->target_name )
      {
         if( !str_cmp( pw->target_name, player_name ) && player_level < pw->imm_level )
            return TRUE;
      }
      else if( pw->player_site )
      {
         if( !str_prefix( pw->player_site, player_site ) && player_level < pw->imm_level )
            return TRUE;
      }
   }
   return FALSE;
}

static void SegVio( int signum )
{
   CHAR_DATA *ch;

   bug( "}RSEGMENTATION FAULT: Invalid Memory Access&D" );
   log_string( lastplayercmd );

   if( first_char )
   {
      for( ch = first_char; ch; ch = ch->next )
      {
         if( ch && ch->name && ch->in_room && !IS_NPC( ch ) )
            log_printf( "%-20s in room: %d", ch->name, ch->in_room->vnum );
      }
   }

   if( sigsegv == TRUE )
      abort(  );
   else
      sigsegv = TRUE;

   crash_count++;

   game_loop(  );

   /*
    * Clean up the loose ends. 
    */
   close_mud(  );

   /*
    * That's all, folks.
    */
   log_string( "Normal termination of game." );
   log_string( "Cleaning up Memory.&d" );
   cleanup_memory(  );
   exit( 0 );
}

static void SigUser1( int signum )
{
   log_string( "Received User1 signal from server." );
   return;
}

static void SigUser2( int signum )
{
   log_string( "Received User2 signal from server." );
   return;
}

#ifdef MULTIPORT
static void SigChld( int signum )
{
   int pid, status;
   DESCRIPTOR_DATA *d;
   CHAR_DATA *ch;

   while( 1 )
   {
      pid = waitpid( WAIT_ANY, &status, WNOHANG );
      if( pid < 0 )
         break;

      if( pid == 0 )
         break;

      for( d = first_descriptor; d; d = d->next )
      {
         if( d->connected == CON_FORKED && d->process == pid )
         {
            if( compilelock )
            {
               echo_to_all( AT_GREEN, "Compiler operation completed. Reboot and shutdown commands unlocked.", ECHOTAR_IMM );
               compilelock = FALSE;
            }
            d->process = 0;
            d->connected = CON_PLAYING;
            fcntl( d->descriptor, F_SETFL, FNDELAY );
            ch = d->original ? d->original : d->character;
            if( ch )
               ch_printf( ch, "Process exited with status code %d.\n\r", status );
         }
      }
   }
}
#endif

static void SigTerm( int signum )
{
   CHAR_DATA *vch;

   echo_to_all( AT_RED, "&RATTENTION!! Message from game server: &YEmergency shutdown called.\a", ECHOTAR_ALL );
   echo_to_all( AT_YELLOW, "Executing emergency shutdown proceedure.", ECHOTAR_ALL );
   log_string( "Message from server: Executing emergency shutdown proceedure." );
   shutdown_mud( "Emergency Shutdown" );

   for( vch = first_char; vch; vch = vch->next )
   {
      /*
       * One of two places this gets changed 
       */
      if( !IS_NPC( vch ) )
         vch->pcdata->hotboot = TRUE;
   }

   close_mud(  );
   log_string( "Emergency shutdown complete." );

   /*
    * Using exit here instead of mud_down because the thing sometimes failed to kill when asked!! 
    */
   exit( 0 );
}

/*
 * LAG alarm!							-Thoric
 */
static void caught_alarm( int signum )
{
   bug( "ALARM CLOCK!  In section %s", alarm_section );
   echo_to_all( AT_IMMORT, "Alas, the hideous malevalent entity known only as 'Lag' rises once more!", ECHOTAR_IMM );
   if( newdesc )
   {
      FD_CLR( newdesc, &in_set );
      FD_CLR( newdesc, &out_set );
      FD_CLR( newdesc, &exc_set );
      log_string( "clearing newdesc" );
   }

   if( fBootDb )
   {
      log_string( "Terminating program. Infinite loop detected during bootup." );
      shutdown_mud( "Infinite loop detected during bootup." );
      abort(  );
   }

   game_loop(  );

   /*
    * Clean up the loose ends. 
    */
   close_mud(  );

   /*
    * That's all, folks.
    */
   log_string( "Normal termination of game." );
   log_string( "Cleaning up Memory.&d" );
   cleanup_memory(  );
   exit( 0 );
}

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA * d )
{
   AFFECT_DATA af;   /* Spamguard abusers beware! Muahahahahah! */
   int i, j, k;
   int iac = 0;
   unsigned char *p;

   /*
    * Hold horses if pending command already.
    */
   if( d->incomm[0] != '\0' )
      return;

   /*
    * Thanks Nick! 
    */
   for( p = ( unsigned char * )d->inbuf; *p; p++ )
   {
      if( *p == IAC )
      {
         if( memcmp( p, term_call_back_str, sizeof( term_call_back_str ) ) == 0 )
         {
            int pos = ( char * )p - d->inbuf;   /* where we are in buffer */
            int len = sizeof( d->inbuf ) - pos - sizeof( term_call_back_str );   /* how much to go */
            char tmp[100];
            unsigned int x = 0;
            unsigned char *oldp = p;

            p += sizeof( term_call_back_str );  /* skip TERMINAL_TYPE / IS characters */

            for( x = 0; x < ( sizeof( tmp ) - 1 ) && *p != 0   /* null marks end of buffer */
                 && *p != IAC;   /* should terminate with IAC */
                 x++, p++ )
               tmp[x] = *p;

            tmp[x] = '\0';
            STRFREE( d->client );
            d->client = STRALLOC( tmp );
            p += 2;  /* skip IAC and SE */
            len -= strlen( tmp ) + 2;
            if( len < 0 )
               len = 0;

            /*
             * remove string from input buffer 
             */
            memmove( oldp, p, len );
         }  /* end of getting terminal type */
      }  /* end of finding an IAC */
   }

   /*
    * Look for at least one new line.
    */
   for( i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r' && i < MAX_INBUF_SIZE; i++ )
   {
      if( d->inbuf[i] == '\0' )
         return;
   }

   /*
    * Canonical input processing.
    */
   for( i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
   {
      if( k >= MIL / 2 )   /* Reasonable enough to allow unless someone floods it with color tags */
      {
         write_to_descriptor( d, "Line too long.\n\r", 0 );
         d->inbuf[i] = '\n';
         d->inbuf[i + 1] = '\0';
         break;
      }

      if( d->can_compress == TRUE )
         compressStart( d );

      if( d->inbuf[i] == ( signed char )IAC )
         iac = 1;
      else if( iac == 1
               && ( d->inbuf[i] == ( signed char )DO || d->inbuf[i] == ( signed char )DONT
                    || d->inbuf[i] == ( signed char )WILL ) )
         iac = 2;
      else if( iac == 2 )
      {
         iac = 0;
         if( d->inbuf[i] == ( signed char )TELOPT_COMPRESS2 )
         {
            if( d->inbuf[i - 1] == ( signed char )DO )
               compressStart( d );
            else if( d->inbuf[i - 1] == ( signed char )DONT )
               compressEnd( d );
         }
         else if( d->inbuf[i] == ( signed char )TELOPT_MXP )
         {
            if( d->inbuf[i - 1] == ( signed char )DO )
               send_mxp_stylesheet( d );
            else if( d->inbuf[i - 1] == ( signed char )DONT )
               d->mxp_detected = FALSE;
         }
         else if( d->inbuf[i] == ( signed char )TELOPT_MSP )
         {
            if( d->inbuf[i - 1] == ( signed char )DO )
               send_msp_startup( d );
            else if( d->inbuf[i - 1] == ( signed char )DONT )
               d->msp_detected = FALSE;
         }
         else if( d->inbuf[i] == ( signed char )TERMINAL_TYPE )
         {
            if( d->inbuf[i - 1] == ( signed char )WILL )
               write_to_buffer( d, req_termtype_str, 0 );
         }
      }
      else if( d->inbuf[i] == '\b' && k > 0 )
         --k;
      /*
       * Note to the future curious: Leave this alone. Extended ascii isn't standardized yet.
       * * You'd think being the 21st century and all that this wouldn't be the case, but you can
       * * thank the bastards in Redmond for this.
       */
      else if( isascii( d->inbuf[i] ) && isprint( d->inbuf[i] ) )
         d->incomm[k++] = d->inbuf[i];
   }

   /*
    * Finish off the line.
    */
   if( k == 0 )
      d->incomm[k++] = ' ';
   d->incomm[k] = '\0';

   /*
    * Deal with bozos with #repeat 1000 ...
    */
   if( k > 1 || d->incomm[0] == '!' )
   {
      if( d->incomm[0] != '!' && str_cmp( d->incomm, d->inlast ) )
         d->repeat = 0;
      else
      {
         /*
          * What this is SUPPOSED to do is make sure the command or alias being used isn't a public channel.
          * * As we know, code rarely does what we expect, and there could still be problems here.
          * * The only other solution seen as viable beyond this is to remove the spamguard entirely.
          */
         CMDTYPE *cmd = NULL;
         MUD_CHANNEL *channel = NULL;
         ALIAS_DATA *pal = NULL;
         char *arg;
         char arg2[MIL];

         arg = d->incomm;
         arg = one_argument( arg, arg2 );
         cmd = find_command( arg2 );

         if( !cmd && d->character && ( pal = find_alias( d->character, d->incomm ) ) != NULL )
         {
            char *arg3;
            char arg4[MIL];

            arg3 = pal->cmd;
            arg3 = one_argument( arg3, arg4 );
            cmd = find_command( arg4 );
         }

         if( !cmd )
         {
            if( ( channel = find_channel( arg2 ) ) != NULL && !str_cmp( d->incomm, d->inlast ) )
               ++d->repeat;
         }
         else if( IS_SET( cmd->flags, CMD_NOSPAM ) && !str_cmp( d->incomm, d->inlast ) )
            ++d->repeat;

#ifdef I3
         {
            I3_CHANNEL *i3chan;

            if( ( i3chan = find_I3_channel_by_localname( arg2 ) ) != NULL && !str_cmp( d->incomm, d->inlast ) )
               ++d->repeat;
         }
#endif
#ifdef IMC
         {
            IMC_CHANNEL *imcchan;

            if( ( imcchan = imc_findchannel( arg2 ) ) != NULL && !str_cmp( d->incomm, d->inlast ) )
               ++d->repeat;
         }
#endif
         if( d->repeat == 3 && d->character && d->character->level < LEVEL_IMMORTAL )
            send_to_char
               ( "}R\n\rYou have repeated the same command 3 times now.\n\rRepeating it 7 more will result in an autofreeze by the spamguard code.&D\n\r",
                 d->character );

         if( d->repeat == 6 && d->character && d->character->level < LEVEL_IMMORTAL )
            send_to_char
               ( "}R\n\rYou have repeated the same command 6 times now.\n\rRepeating it 4 more will result in an autofreeze by the spamguard code.&D\n\r",
                 d->character );

         if( d->repeat >= 10 && d->character && d->character->level < LEVEL_IMMORTAL )
         {
            ++d->character->pcdata->spam;
            log_printf( "%s was autofrozen by the spamguard - spamming: %s", d->character->name, d->incomm );
            log_printf( "%s has spammed %d times this login.", d->character->name, d->character->pcdata->spam );

            write_to_descriptor( d,
                                 "\n\r*** PUT A LID ON IT!!! ***\n\rYou cannot enter the same command more than 10 consecutive times!\n\r",
                                 0 );
            write_to_descriptor( d, "The Spamguard has spoken!\n\r", 0 );
            write_to_descriptor( d, "It suddenly becomes very cold, and very QUIET!\n\r", 0 );
            mudstrlcpy( d->incomm, "spam", MIL );

            af.type = skill_lookup( "spamguard" );
            af.duration = 115 * d->character->pcdata->spam; /* One game hour per offense, this can get ugly FAST */
            af.modifier = 0;
            af.location = APPLY_NONE;
            af.bit = AFF_SPAMGUARD;
            affect_to_char( d->character, &af );
            SET_PLR_FLAG( d->character, PLR_IDLING );
            d->repeat = 0; /* Just so it doesn't get haywire */
         }
      }
   }

   /*
    * Do '!' substitution.
    */
   if( d->incomm[0] == '!' )
      mudstrlcpy( d->incomm, d->inlast, MIL );
   else
      mudstrlcpy( d->inlast, d->incomm, MIL );

   /*
    * Shift the input buffer.
    */
   while( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
      i++;
   for( j = 0; ( d->inbuf[j] = d->inbuf[i + j] ) != '\0'; j++ )
      ;
   return;
}

/*Prompt, fprompt made to include exp and victim's condition, prettyfied too - Adjani, 12-07-2002*/
char *default_fprompt( CHAR_DATA * ch )
{
   static char buf[60];

   mudstrlcpy( buf, "&z[&W%h&whp ", 60 );
   mudstrlcat( buf, "&W%m&wm", 60 );
   mudstrlcat( buf, " &W%v&wmv&z] ", 60 );
   mudstrlcat( buf, " [&R%c&z] ", 60 );
   if( IS_NPC( ch ) || IS_IMMORTAL( ch ) )
      mudstrlcat( buf, "&W%i%R&D", 60 );
   return buf;
}

char *default_prompt( CHAR_DATA * ch )
{
   static char buf[60];

   mudstrlcpy( buf, "&z[&W%h&whp ", 60 );
   mudstrlcat( buf, "&W%m&wm", 60 );
   mudstrlcat( buf, " &W%v&wmv&z] ", 60 );
   mudstrlcat( buf, " [&c%X&wexp&z]&D ", 60 );
   if( IS_NPC( ch ) || IS_IMMORTAL( ch ) )
      mudstrlcat( buf, "&W%i%R&D", 60 );
   return buf;
}

void display_prompt( DESCRIPTOR_DATA * d )
{
   CHAR_DATA *ch = d->character;
   CHAR_DATA *och = ( d->original ? d->original : d->character );
   CHAR_DATA *victim;
   bool ansi = ( IS_PLR_FLAG( och, PLR_ANSI ) );
   const char *prompt;
   const char *helpstart = "&w[Type HELP START]";
   char buf[MSL];
   char *pbuf = buf;
   unsigned int pstat;
   int percent;

   if( !ch )
   {
      bug( "%s", "display_prompt: NULL ch" );
      return;
   }

   if( !IS_NPC(ch) && !IS_PCFLAG( ch, PCFLAG_HELPSTART ) )
      prompt = helpstart;

   else if( !IS_NPC( ch ) && ch->substate != SUB_NONE && ch->pcdata->subprompt && ch->pcdata->subprompt[0] != '\0' )
      prompt = ch->pcdata->subprompt;

   else if( IS_NPC( ch ) || ( !ch->fighting && ( !ch->pcdata->prompt || !*ch->pcdata->prompt ) ) )
      prompt = default_prompt( ch );

   else if( ch->fighting )
   {
      if( !ch->pcdata->fprompt || !*ch->pcdata->fprompt )
         prompt = default_fprompt( ch );
      else
         prompt = ch->pcdata->fprompt;
   }
   else
      prompt = ch->pcdata->prompt;

   if( ansi )
   {
      mudstrlcpy( pbuf, ANSI_RESET, MSL - ( pbuf - buf ) );
      d->prevcolor = 0x08;
      pbuf += 4;
   }

   for( ; *prompt; prompt++ )
   {
      /*
       * '%' = prompt commands
       * Note: foreground changes will revert background to 0 (black)
       */
      if( *prompt != '%' )
      {
         *( pbuf++ ) = *prompt;
         continue;
      }

      ++prompt;
      if( !*prompt )
         break;
      if( *prompt == *( prompt - 1 ) )
      {
         *( pbuf++ ) = *prompt;
         continue;
      }
      switch ( *( prompt - 1 ) )
      {
         default:
            bug( "Display_prompt: bad command char '%c'.", *( prompt - 1 ) );
            break;

         case '%':
            *pbuf = '\0';
            pstat = 0x80000000;
            switch ( *prompt )
            {
               case '%':
                  *pbuf++ = '%';
                  *pbuf = '\0';
                  break;
               case 'a':
                  if( ch->level >= 10 )
                     pstat = ch->alignment;
                  else if( IS_GOOD( ch ) )
                     mudstrlcpy( pbuf, "good", MSL - ( pbuf - buf ) );
                  else if( IS_EVIL( ch ) )
                     mudstrlcpy( pbuf, "evil", MSL - ( pbuf - buf ) );
                  else
                     mudstrlcpy( pbuf, "neutral", MSL - ( pbuf - buf ) );
                  break;
               case 'A':
                  snprintf( pbuf, MSL - ( pbuf - buf ), "%s%s%s", IS_AFFECTED( ch, AFF_INVISIBLE ) ? "I" : "",
                            IS_AFFECTED( ch, AFF_HIDE ) ? "H" : "", IS_AFFECTED( ch, AFF_SNEAK ) ? "S" : "" );
                  break;
               case 'C':  /* Tank */
                  if( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
                     mudstrlcpy( pbuf, "N/A", MSL - ( pbuf - buf ) );
                  else if( !victim->fighting || ( victim = victim->fighting->who ) == NULL )
                     mudstrlcpy( pbuf, "N/A", MSL - ( pbuf - buf ) );
                  else
                  {
                     if( victim->max_hit > 0 )
                        percent = ( 100 * victim->hit ) / victim->max_hit;
                     else
                        percent = -1;
                     if( percent >= 100 )
                        mudstrlcpy( pbuf, "perfect health", MSL - ( pbuf - buf ) );
                     else if( percent >= 90 )
                        mudstrlcpy( pbuf, "slightly scratched", MSL - ( pbuf - buf ) );
                     else if( percent >= 80 )
                        mudstrlcpy( pbuf, "few bruises", MSL - ( pbuf - buf ) );
                     else if( percent >= 70 )
                        mudstrlcpy( pbuf, "some cuts", MSL - ( pbuf - buf ) );
                     else if( percent >= 60 )
                        mudstrlcpy( pbuf, "several wounds", MSL - ( pbuf - buf ) );
                     else if( percent >= 50 )
                        mudstrlcpy( pbuf, "nasty wounds", MSL - ( pbuf - buf ) );
                     else if( percent >= 40 )
                        mudstrlcpy( pbuf, "bleeding freely", MSL - ( pbuf - buf ) );
                     else if( percent >= 30 )
                        mudstrlcpy( pbuf, "covered in blood", MSL - ( pbuf - buf ) );
                     else if( percent >= 20 )
                        mudstrlcpy( pbuf, "leaking guts", MSL - ( pbuf - buf ) );
                     else if( percent >= 10 )
                        mudstrlcpy( pbuf, "almost dead", MSL - ( pbuf - buf ) );
                     else
                        mudstrlcpy( pbuf, "DYING", MSL - ( pbuf - buf ) );
                  }
                  break;
               case 'c':
                  if( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
                     mudstrlcpy( pbuf, "N/A", MSL - ( pbuf - buf ) );
                  else
                  {
                     if( victim->max_hit > 0 )
                        percent = ( 100 * victim->hit ) / victim->max_hit;
                     else
                        percent = -1;
                     if( percent >= 100 )
                        mudstrlcpy( pbuf, "perfect health", MSL - ( pbuf - buf ) );
                     else if( percent >= 90 )
                        mudstrlcpy( pbuf, "slightly scratched", MSL - ( pbuf - buf ) );
                     else if( percent >= 80 )
                        mudstrlcpy( pbuf, "few bruises", MSL - ( pbuf - buf ) );
                     else if( percent >= 70 )
                        mudstrlcpy( pbuf, "some cuts", MSL - ( pbuf - buf ) );
                     else if( percent >= 60 )
                        mudstrlcpy( pbuf, "several wounds", MSL - ( pbuf - buf ) );
                     else if( percent >= 50 )
                        mudstrlcpy( pbuf, "nasty wounds", MSL - ( pbuf - buf ) );
                     else if( percent >= 40 )
                        mudstrlcpy( pbuf, "bleeding freely", MSL - ( pbuf - buf ) );
                     else if( percent >= 30 )
                        mudstrlcpy( pbuf, "covered in blood", MSL - ( pbuf - buf ) );
                     else if( percent >= 20 )
                        mudstrlcpy( pbuf, "leaking guts", MSL - ( pbuf - buf ) );
                     else if( percent >= 10 )
                        mudstrlcpy( pbuf, "almost dead", MSL - ( pbuf - buf ) );
                     else
                        mudstrlcpy( pbuf, "DYING", MSL - ( pbuf - buf ) );
                  }
                  break;
               case 'h':
                  if( MXP_ON( ch ) )
                     snprintf( pbuf, MSL - ( pbuf - buf ), MXP_TAG_HP "%d" MXP_TAG_HP_CLOSE, ch->hit );
                  else
                     pstat = ch->hit;
                  break;
               case 'H':
                  if( MXP_ON( ch ) )
                     snprintf( pbuf, MSL - ( pbuf - buf ), MXP_TAG_MAXHP "%d" MXP_TAG_MAXHP_CLOSE, ch->max_hit );
                  else
                     pstat = ch->max_hit;
                  break;
               case 'm':
                  if( MXP_ON( ch ) )
                     snprintf( pbuf, MSL - ( pbuf - buf ), MXP_TAG_MANA "%d" MXP_TAG_MANA_CLOSE, ch->mana );
                  else
                     pstat = ch->mana;
                  break;
               case 'M':
                  if( MXP_ON( ch ) )
                     snprintf( pbuf, MSL - ( pbuf - buf ), MXP_TAG_MAXMANA "%d" MXP_TAG_MAXMANA_CLOSE, ch->max_mana );
                  else
                     pstat = ch->max_mana;
                  break;
               case 'N':  /* Tank */
                  if( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
                     mudstrlcpy( pbuf, "N/A", MSL - ( pbuf - buf ) );
                  else if( !victim->fighting || ( victim = victim->fighting->who ) == NULL )
                     mudstrlcpy( pbuf, "N/A", MSL - ( pbuf - buf ) );
                  else
                  {
                     if( ch == victim )
                        mudstrlcpy( pbuf, "You", MSL - ( pbuf - buf ) );
                     else if( IS_NPC( victim ) )
                        mudstrlcpy( pbuf, victim->short_descr, MSL - ( pbuf - buf ) );
                     else
                        mudstrlcpy( pbuf, victim->name, MSL - ( pbuf - buf ) );
                     pbuf[0] = UPPER( pbuf[0] );
                  }
                  break;
               case 'n':
                  if( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
                     mudstrlcpy( pbuf, "N/A", MSL - ( pbuf - buf ) );
                  else
                  {
                     if( ch == victim )
                        mudstrlcpy( pbuf, "You", MSL - ( pbuf - buf ) );
                     else if( IS_NPC( victim ) )
                        mudstrlcpy( pbuf, victim->short_descr, MSL - ( pbuf - buf ) );
                     else
                        mudstrlcpy( pbuf, victim->name, MSL - ( pbuf - buf ) );
                     pbuf[0] = UPPER( pbuf[0] );
                  }
                  break;
               case 'T':
                  if( time_info.hour < sysdata.hoursunrise )
                     mudstrlcpy( pbuf, "night", MSL - ( pbuf - buf ) );
                  else if( time_info.hour < sysdata.hourdaybegin )
                     mudstrlcpy( pbuf, "dawn", MSL - ( pbuf - buf ) );
                  else if( time_info.hour < sysdata.hoursunset )
                     mudstrlcpy( pbuf, "day", MSL - ( pbuf - buf ) );
                  else if( time_info.hour < sysdata.hournightbegin )
                     mudstrlcpy( pbuf, "dusk", MSL - ( pbuf - buf ) );
                  else
                     mudstrlcpy( pbuf, "night", MSL - ( pbuf - buf ) );
                  break;
               case 'u':
                  pstat = num_descriptors;
                  break;
               case 'U':
                  pstat = sysdata.maxplayers;
                  break;
               case 'v':
                  pstat = ch->move;
                  break;
               case 'V':
                  pstat = ch->max_move;
                  break;
               case 'g':
                  pstat = ch->gold;
                  break;
               case 'r':
                  if( IS_IMMORTAL( och ) )
                     pstat = ch->in_room->vnum;
                  break;
               case 'F':
                  if( IS_IMMORTAL( och ) )
                     mudstrlcpy( pbuf, ext_flag_string( &ch->in_room->room_flags, r_flags ), MSL - ( pbuf - buf ) );
                  break;
               case 'R':
                  if( IS_PLR_FLAG( och, PLR_ROOMVNUM ) )
                     snprintf( pbuf, MSL - ( pbuf - buf ), "<#%d> ", ch->in_room->vnum );
                  break;
               case 'x':
                  pstat = ch->exp;
                  break;
               case 'X':
                  pstat = exp_level( ch->level + 1 ) - ch->exp;
                  break;

               case 'o':  /* display name of object on auction */
                  if( auction->item )
                     mudstrlcpy( pbuf, auction->item->name, MSL - ( pbuf - buf ) );
                  break;

               case 'S':
                  if( ch->style == STYLE_BERSERK )
                     mudstrlcpy( pbuf, "B", MSL - ( pbuf - buf ) );
                  else if( ch->style == STYLE_AGGRESSIVE )
                     mudstrlcpy( pbuf, "A", MSL - ( pbuf - buf ) );
                  else if( ch->style == STYLE_DEFENSIVE )
                     mudstrlcpy( pbuf, "D", MSL - ( pbuf - buf ) );
                  else if( ch->style == STYLE_EVASIVE )
                     mudstrlcpy( pbuf, "E", MSL - ( pbuf - buf ) );
                  else
                     mudstrlcpy( pbuf, "S", MSL - ( pbuf - buf ) );
                  break;
               case 'i':
                  if( IS_PLR_FLAG( ch, PLR_WIZINVIS ) || IS_ACT_FLAG( ch, ACT_MOBINVIS ) )
                     snprintf( pbuf, MSL - ( pbuf - buf ), "(Invis %d) ",
                               ( IS_NPC( ch ) ? ch->mobinvis : ch->pcdata->wizinvis ) );
                  else if( IS_AFFECTED( ch, AFF_INVISIBLE ) )
                     mudstrlcpy( pbuf, "(Invis) ", MSL - ( pbuf - buf ) );
                  break;
               case 'I':
                  if( IS_ACT_FLAG( ch, ACT_MOBINVIS ) )
                     pstat = ch->mobinvis;
                  else if( IS_PLR_FLAG( ch, PLR_WIZINVIS ) )
                     pstat = ch->pcdata->wizinvis;
                  else
                     pstat = 0;
                  break;
               case 'Z':
                  if( sysdata.WIZLOCK )
                     mudstrlcpy( pbuf, "[Wizlock]", MSL - ( pbuf - buf ) );
                  if( sysdata.IMPLOCK )
                     mudstrlcpy( pbuf, "[Implock]", MSL - ( pbuf - buf ) );
                  if( sysdata.LOCKDOWN )
                     mudstrlcpy( pbuf, "[Lockdown]", MSL - ( pbuf - buf ) );
                  if( bootlock )
                     mudstrlcpy( pbuf, "[Rebooting]", MSL - ( pbuf - buf ) );


                  break;
            }
            if( pstat != 0x80000000 )
               snprintf( pbuf, MSL - ( pbuf - buf ), "%d", pstat );
            pbuf += strlen( pbuf );
            break;
      }
   }
   *pbuf = '\0';

   /*
    * An attempt to provide a short common MXP command menu above the prompt line 
    */
   if( MXP_ON( ch ) && IS_PLR_FLAG( ch, PLR_MXPPROMPT ) )
   {
      send_to_char( color_str( AT_MXPPROMPT, ch ), ch );
      send_to_char( MXP_TAG_SECURE
                    "<look>look</look> "
                    "<inventory>inv</inventory> "
                    "<equipment>eq</equipment> " "<score>sc</score> " "<who>who</who>" MXP_TAG_LOCKED "\n\r", ch );

      send_to_char( MXP_TAG_SECURE
                    "<attrib>att</attrib> "
                    "<level>lev</level> " "<practice>prac</practice> " "<slist>slist</slist>" MXP_TAG_LOCKED "\n\r", ch );
   }

   if( MXP_ON( ch ) )
      send_to_char( MXP_TAG_PROMPT, ch );

   send_to_char( buf, ch );

   if( MXP_ON( ch ) )
      send_to_char( MXP_TAG_PROMPT_CLOSE, ch );

   /*
    * The miracle cure for color bleeding? 
    */
   send_to_char( ANSI_RESET, ch );
   return;
}

/*
 * Low level output function.
 */
bool flush_buffer( DESCRIPTOR_DATA * d, bool fPrompt )
{
   char buf[MIL];

   /*
    * If buffer has more than 4K inside, spit out .5K at a time   -Thoric
    */
   if( !mud_down && d->outtop > 4096 )
   {
      memcpy( buf, d->outbuf, 2048 );
      d->outtop -= 2048;
      memmove( d->outbuf, d->outbuf + 2048, d->outtop );
      if( d->snoop_by )
      {
         buf[2048] = '\0';
         if( d->character && d->character->name )
         {
            if( d->original && d->original->name )
               buffer_printf( d->snoop_by, "%s (%s)", d->character->name, d->original->name );
            else
               buffer_printf( d->snoop_by, "%s", d->character->name );
         }
         write_to_buffer( d->snoop_by, "% ", 2 );
         write_to_buffer( d->snoop_by, buf, 0 );
      }
      if( !write_to_descriptor( d, buf, 2048 ) )
      {
         d->outtop = 0;
         return FALSE;
      }
      return TRUE;
   }

   /*
    * Bust a prompt.
    */
   if( fPrompt && !mud_down && d->connected == CON_PLAYING )
   {
      CHAR_DATA *ch;

      ch = d->original ? d->original : d->character;
      if( IS_PLR_FLAG( ch, PLR_BLANK ) )
         write_to_buffer( d, "\n\r", 2 );

      if( IS_PLR_FLAG( ch, PLR_PROMPT ) )
         display_prompt( d );
      else if( !IS_NPC( ch ) )
      {
         write_to_buffer( d, color_str( AT_PLAIN, ch ), 0 );
         d->pagecolor = ch->pcdata->colors[AT_PLAIN];
      }
      else
         write_to_buffer( d, ANSI_RESET, 0 );

      if( IS_PLR_FLAG( ch, PLR_TELNET_GA ) )
         write_to_buffer( d, go_ahead_str, 0 );
   }

   /*
    * Short-circuit if nothing to write.
    */
   if( d->outtop == 0 )
      return TRUE;

   /*
    * Snoop-o-rama.
    */
   if( d->snoop_by )
   {
      /*
       * without check, 'force mortal quit' while snooped caused crash, -h 
       */
      if( d->character && d->character->name )
      {
         /*
          * Show original snooped names. -- Altrag 
          */
         if( d->original && d->original->name )
            buffer_printf( d->snoop_by, "%s (%s)", d->character->name, d->original->name );
         else
            write_to_buffer( d->snoop_by, d->character->name, 0 );
      }
      write_to_buffer( d->snoop_by, "% ", 2 );
      write_to_buffer( d->snoop_by, d->outbuf, d->outtop );
   }

   /*
    * OS-dependent output.
    */
   if( !write_to_descriptor( d, d->outbuf, d->outtop ) )
   {
      d->outtop = 0;
      return FALSE;
   }
   else
   {
      d->outtop = 0;
      return TRUE;
   }
}

/* Disabled Limbo transfer - Samson 5-8-99 */
void stop_idling( CHAR_DATA * ch )
{
   if( !ch || !ch->desc || ch->desc->connected != CON_PLAYING || !IS_PLR_FLAG( ch, PLR_IDLING ) )
      return;

   ch->timer = 0;

   REMOVE_PLR_FLAG( ch, PLR_IDLING );
   act( AT_ACTION, "$n returns to normal.", ch, NULL, NULL, TO_ROOM );
   return;
}

void set_pager_input( DESCRIPTOR_DATA * d, char *argument )
{
   while( isspace( *argument ) )
      argument++;
   d->pagecmd = *argument;
   return;
}

bool pager_output( DESCRIPTOR_DATA * d )
{
   register char *last;
   CHAR_DATA *ch;
   int pclines;
   register int lines;
   bool ret;

   if( !d || !d->pagepoint || d->pagecmd == -1 )
      return TRUE;
   ch = d->original ? d->original : d->character;
   pclines = UMAX( ch->pcdata->pagerlen, 5 ) - 1;
   switch ( LOWER( d->pagecmd ) )
   {
      default:
         lines = 0;
         break;
      case 'b':
         lines = -1 - ( pclines * 2 );
         break;
      case 'r':
         lines = -1 - pclines;
         break;
      case 'n':
         lines = 0;
         pclines = 0x7FFFFFFF;   /* As many lines as possible */
         break;
      case 'q':
         d->pagetop = 0;
         d->pagepoint = NULL;
         flush_buffer( d, TRUE );
         DISPOSE( d->pagebuf );
         d->pagesize = MSL;
         return TRUE;
   }
   while( lines < 0 && d->pagepoint >= d->pagebuf )
      if( *( --d->pagepoint ) == '\n' )
         ++lines;
   if( *d->pagepoint == '\n' && *( ++d->pagepoint ) == '\r' )
      ++d->pagepoint;
   if( d->pagepoint < d->pagebuf )
      d->pagepoint = d->pagebuf;
   for( lines = 0, last = d->pagepoint; lines < pclines; ++last )
   {
      if( !*last )
         break;
      else if( *last == '\n' )
         ++lines;
   }
   if( *last == '\r' )
      ++last;
   if( last != d->pagepoint )
   {
      if( !write_to_descriptor( d, d->pagepoint, ( last - d->pagepoint ) ) )
         return FALSE;
      d->pagepoint = last;
   }
   while( isspace( *last ) )
      ++last;
   if( !*last )
   {
      d->pagetop = 0;
      d->pagepoint = NULL;
      flush_buffer( d, TRUE );
      DISPOSE( d->pagebuf );
      d->pagesize = MSL;
      return TRUE;
   }
   d->pagecmd = -1;
   if( IS_PLR_FLAG( ch, PLR_ANSI ) )
      if( write_to_descriptor( d, ANSI_LBLUE, 0 ) == FALSE )
         return FALSE;
   if( ( ret = write_to_descriptor( d, "(C)ontinue, (N)on-stop, (R)efresh, (B)ack, (Q)uit: [C] ", 0 ) ) == FALSE )
      return FALSE;
   /*
    * Telnet GA bit here suggested by Garil 
    */
   if( IS_PLR_FLAG( ch, PLR_TELNET_GA ) )
      write_to_buffer( d, go_ahead_str, 0 );
   if( IS_PLR_FLAG( ch, PLR_ANSI ) )
   {
      char buf[32];

      snprintf( buf, 32, "%s", color_str( d->pagecolor, ch ) );
      ret = write_to_descriptor( d, buf, 0 );
   }
   return ret;
}

void close_socket( DESCRIPTOR_DATA * dclose, bool force )
{
   CHAR_DATA *ch;
   DESCRIPTOR_DATA *d;
   AUTH_LIST *old_auth;
   bool DoNotUnlink = FALSE;

   if( dclose->ipid != -1 )
   {
      int status;

      kill( dclose->ipid, SIGKILL );
      waitpid( dclose->ipid, &status, 0 );
   }
   if( dclose->ifd != -1 )
      close( dclose->ifd );

   /*
    * flush outbuf 
    */
   if( !force && dclose->outtop > 0 )
      flush_buffer( dclose, FALSE );

   /*
    * say bye to whoever's snooping this descriptor 
    */
   if( dclose->snoop_by )
      write_to_buffer( dclose->snoop_by, "Your victim has left the game.\n\r", 0 );

   /*
    * stop snooping everyone else 
    */
   for( d = first_descriptor; d; d = d->next )
      if( d->snoop_by == dclose )
         d->snoop_by = NULL;

   /*
    * Check for switched people who go link-dead. -- Altrag 
    */
   if( dclose->original )
   {
      if( ( ch = dclose->character ) != NULL )
         interpret( ch, "return" );
      else
      {
         bug( "Close_socket: dclose->original without character %s",
              ( dclose->original->name ? dclose->original->name : "unknown" ) );
         dclose->character = dclose->original;
         dclose->original = NULL;
      }
   }

   ch = dclose->character;

   /*
    * sanity check :( 
    */
   if( !dclose->prev && dclose != first_descriptor )
   {
      DESCRIPTOR_DATA *dp, *dn;
      bug( "Close_socket: %s desc:%p != first_desc:%p and desc->prev = NULL!",
           ch ? ch->name : d->host, ( void * )dclose, ( void * )first_descriptor );
      dp = NULL;
      for( d = first_descriptor; d; d = dn )
      {
         dn = d->next;
         if( d == dclose )
         {
            bug( "Close_socket: %s desc:%p found, prev should be:%p, fixing.",
                 ch ? ch->name : d->host, ( void * )dclose, ( void * )dp );
            dclose->prev = dp;
            break;
         }
         dp = d;
      }
      if( !dclose->prev )
      {
         bug( "Close_socket: %s desc:%p could not be found!.", ch ? ch->name : dclose->host, ( void * )dclose );
         DoNotUnlink = TRUE;
      }
   }
   if( !dclose->next && dclose != last_descriptor )
   {
      DESCRIPTOR_DATA *dp, *dn;
      bug( "Close_socket: %s desc:%p != last_desc:%p and desc->next = NULL!",
           ch ? ch->name : d->host, ( void * )dclose, ( void * )last_descriptor );
      dn = NULL;
      for( d = last_descriptor; d; d = dp )
      {
         dp = d->prev;
         if( d == dclose )
         {
            bug( "Close_socket: %s desc:%p found, next should be:%p, fixing.",
                 ch ? ch->name : d->host, ( void * )dclose, ( void * )dn );
            dclose->next = dn;
            break;
         }
         dn = d;
      }
      if( !dclose->next )
      {
         bug( "Close_socket: %s desc:%p could not be found!.", ch ? ch->name : dclose->host, ( void * )dclose );
         DoNotUnlink = TRUE;
      }
   }

   if( dclose->character )
   {
      log_printf_plus( LOG_COMM, ch->level, "Closing link to %s.", ch->pcdata->filename );

      /*
       * Link dead auth -- Rantic 
       */
      old_auth = get_auth_name( ch->name );
      if( old_auth != NULL && old_auth->state == AUTH_ONLINE )
      {
         old_auth->state = AUTH_LINK_DEAD;
         save_auth_list(  );
      }

      if( dclose->connected == CON_PLAYING
          || dclose->connected == CON_EDITING
          || dclose->connected == CON_DELETE
          || dclose->connected == CON_ROLL_STATS
          || dclose->connected == CON_RAISE_STAT
          || dclose->connected == CON_PRIZENAME
          || dclose->connected == CON_CONFIRMPRIZENAME
          || dclose->connected == CON_PRIZEKEY
          || dclose->connected == CON_CONFIRMPRIZEKEY
          || dclose->connected == CON_OEDIT
          || dclose->connected == CON_REDIT || dclose->connected == CON_MEDIT || dclose->connected == CON_BOARD )
      {
         act( AT_ACTION, "$n has lost $s link.", ch, NULL, NULL, TO_CANSEE );
         ch->desc = NULL;
      }
      else
      {
         /*
          * clear descriptor pointer to get rid of bug message in log 
          */
         dclose->character->desc = NULL;
         free_char( dclose->character );
      }
   }

   if( !DoNotUnlink )
   {
      /*
       * make sure loop doesn't get messed up 
       */
      if( d_next == dclose )
         d_next = d_next->next;
      UNLINK( dclose, first_descriptor, last_descriptor, next, prev );
   }

   compressEnd( dclose );

   if( dclose->descriptor == maxdesc )
      --maxdesc;

   free_desc( dclose );
   --num_descriptors;
   return;
}

/*
 * Append onto an output buffer.
 */
void write_to_buffer( DESCRIPTOR_DATA * d, const char *txt, unsigned int length )
{
   if( !d )
   {
      bug( "%s", "Write_to_buffer: NULL descriptor" );
      return;
   }

   /*
    * Normally a bug... but can happen if loadup is used.
    */
   if( !d->outbuf )
      return;

   /*
    * Find length in case caller didn't.
    */
   if( length <= 0 )
      length = strlen( txt );

   /*
    * Initial \n\r if needed.
    */
   if( d->outtop == 0 && !d->fcommand )
   {
      d->outbuf[0] = '\n';
      d->outbuf[1] = '\r';
      d->outtop = 2;
   }

   /*
    * Expand the buffer as needed.
    */
   while( d->outtop + length >= d->outsize )
   {
      if( d->outsize > 32000 )
      {
         /*
          * empty buffer 
          */
         d->outtop = 0;
         bug( "Buffer overflow: %ld. Closing (%s).", d->outsize, d->character ? d->character->name : "???" );
         close_socket( d, TRUE );
         return;
      }
      d->outsize *= 2;
      RECREATE( d->outbuf, char, d->outsize );
   }

   /*
    * Copy.
    */
   strncpy( d->outbuf + d->outtop, txt, length );  /* Leave this one alone! BAD THINGS(TM) will happen if you don't! */
   d->outtop += length;
   d->outbuf[d->outtop] = '\0';
   return;
}

void buffer_printf( DESCRIPTOR_DATA * d, const char *fmt, ... )
{
   char buf[MSL * 4];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 4, fmt, args );
   va_end( args );

   write_to_buffer( d, buf, 0 );
}

void show_title( DESCRIPTOR_DATA * d )
{
   CHAR_DATA *ch;

   ch = d->character;

   if( !IS_PCFLAG( ch, PCFLAG_NOINTRO ) )
      show_file( ch, ANSITITLE_FILE );
   else
      write_to_buffer( d, "Press enter...\n\r", 0 );
   d->connected = CON_PRESS_ENTER;
}

void show_stats( CHAR_DATA * ch, DESCRIPTOR_DATA * d )
{
   set_char_color( AT_SKILL, ch );
   buffer_printf( d, "\n\r1. Str: %d\n\r", ch->perm_str );
   buffer_printf( d, "2. Int: %d\n\r", ch->perm_int );
   buffer_printf( d, "3. Wis: %d\n\r", ch->perm_wis );
   buffer_printf( d, "4. Dex: %d\n\r", ch->perm_dex );
   buffer_printf( d, "5. Con: %d\n\r", ch->perm_con );
   buffer_printf( d, "6. Cha: %d\n\r", ch->perm_cha );
   buffer_printf( d, "7. Lck: %d\n\r\n\r", ch->perm_lck );
   set_char_color( AT_ACTION, ch );
   write_to_buffer( d, "Choose an attribute to raise by +1\n\r", 0 );
   return;
}

void char_to_game( CHAR_DATA * ch )
{
   HOLIDAY_DATA *day;   /* To inform incoming players of holidays - Samson 6-3-99 */

   if( str_cmp( ch->desc->client, "Unidentified" ) )
      log_printf( "%s client detected for %s.", capitalize( ch->desc->client ), ch->name );
   if( ch->desc->can_compress )
      log_printf( "MCCP support detected for %s.", ch->name );
   if( ch->desc->mxp_detected )
      log_printf( "MXP support detected for %s.", ch->name );
   if( ch->desc->msp_detected )
      log_printf( "MSP support detected for %s.", ch->name );

   LINK( ch, first_char, last_char, next, prev );
   ch->desc->connected = CON_PLAYING;

   /*
    * Connection History Updating 
    */
   if( ch->level == 0 )
      update_connhistory( ch->desc, CONNTYPE_NEWPLYR );
   else
      update_connhistory( ch->desc, CONNTYPE_LOGIN );

   if( ch->level == 0 )
      setup_newbie( ch, TRUE );

   else if( !IS_IMMORTAL( ch ) && ch->pcdata->release_date > 0 && ch->pcdata->release_date > current_time )
   {
      if( !char_to_room( ch, get_room_index( ROOM_VNUM_HELL ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }

   else if( ch->in_room && ( IS_IMMORTAL( ch ) || !IS_ROOM_FLAG( ch->in_room, ROOM_PROTOTYPE ) ) )
   {
      if( !char_to_room( ch, ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }

   else if( IS_IMMORTAL( ch ) )
   {
      if( !char_to_room( ch, get_room_index( ROOM_VNUM_CHAT ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }

   else
   {
      if( !char_to_room( ch, get_room_index( ROOM_VNUM_TEMPLE ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }

   if( get_timer( ch, TIMER_SHOVEDRAG ) > 0 )
      remove_timer( ch, TIMER_SHOVEDRAG );

   if( get_timer( ch, TIMER_PKILLED ) > 0 )
      remove_timer( ch, TIMER_PKILLED );

   act( AT_ACTION, "$n has entered the game.", ch, NULL, NULL, TO_CANSEE );

   if( !ch->was_in_room && ch->in_room == get_room_index( ROOM_VNUM_TEMPLE ) )
      ch->was_in_room = get_room_index( ROOM_VNUM_TEMPLE );

   else if( ch->was_in_room == get_room_index( ROOM_VNUM_TEMPLE ) )
      ch->was_in_room = get_room_index( ROOM_VNUM_TEMPLE );

   else if( !ch->was_in_room )
      ch->was_in_room = ch->in_room;

   interpret( ch, "look" );

   /*
    * Inform incoming player of any holidays - Samson 6-3-99 
    */
   day = get_holiday( time_info.month, time_info.day );

   if( day != NULL )
      ch_printf( ch, "&Y%s\n\r", day->announce );

   send_to_char( "&R\n\r", ch );
   if( IS_PCFLAG( ch, PCFLAG_CHECKBOARD ) )
      interpret( ch, "checkboards" );

   if( ch->pcdata->camp == 1 )
      break_camp( ch );
   else
      scan_rent( ch );

   DISPOSE( ch->pcdata->lasthost );
   ch->pcdata->lasthost = str_dup( ch->desc->host );

   /*
    * Auction house system disabled as of 8-24-99 due to abuse by the players - Samson 
    */
   /*
    * Reinstated 11-1-99 - Samson 
    */
   sale_count( ch ); /* New auction system - Samson 6-24-99 */

   check_auth_state( ch ); /* new auth */
   check_clan_info( ch );  /* see if this guy got a promo to clan admin */

   /*
    * @shrug, why not? :P 
    */
   if( IS_PLR_FLAG( ch, PLR_ONMAP ) )
      music_to_char( "wilderness.mid", 100, ch, FALSE );

   quotes( ch );

   if( ch->tempnum > 0 )
   {
      send_to_char( "&YUpdating character for changes to experience system.\n\r", ch );
      show_stats( ch, ch->desc );
      ch->desc->connected = CON_RAISE_STAT;
   }
   save_char_obj( ch ); /* Just making sure their status is saved at least once after login, in case of crashes */
   ++num_logins;
   return;
}

void display_motd( CHAR_DATA * ch )
{
   if( IS_IMMORTAL( ch ) && ch->pcdata->imotd < sysdata.imotd )
   {
      if( IS_PLR_FLAG( ch, PLR_ANSI ) )
         send_to_pager( "\033[2J", ch );
      else
         send_to_pager( "\014", ch );

      show_file( ch, IMOTD_FILE );
      send_to_pager( "\n\rPress [ENTER] ", ch );
      ch->pcdata->imotd = current_time;
      ch->desc->connected = CON_READ_MOTD;
      return;
   }
   if( ch->level < LEVEL_IMMORTAL && ch->level > 0 && ch->pcdata->motd < sysdata.motd )
   {
      if( IS_PLR_FLAG( ch, PLR_ANSI ) )
         send_to_pager( "\033[2J", ch );
      else
         send_to_pager( "\014", ch );

      show_file( ch, MOTD_FILE );
      send_to_pager( "\n\rPress [ENTER] ", ch );
      ch->pcdata->motd = current_time;
      ch->desc->connected = CON_READ_MOTD;
      return;
   }
   if( ch->level == 0 )
   {
      if( IS_PLR_FLAG( ch, PLR_ANSI ) )
         send_to_pager( "\033[2J", ch );
      else
         send_to_pager( "\014", ch );

      do_help( ch, "nmotd" );
      send_to_pager( "\n\rPress [ENTER] ", ch );
      ch->desc->connected = CON_READ_MOTD;
      return;
   }

   if( exists_file( SPEC_MOTD ) )
   {
      if( IS_PLR_FLAG( ch, PLR_ANSI ) )
         send_to_pager( "\033[2J", ch );
      else
         send_to_pager( "\014", ch );

      show_file( ch, SPEC_MOTD );
   }

   buffer_printf( ch->desc, "\n\rWelcome to %s...\n\r", sysdata.mud_name );
   char_to_game( ch );

   return;
}

/*
 * Parse a name for acceptability.
 */
bool check_parse_name( char *name, bool newchar )
{
   CHAR_DATA *vch;
   char buf[MSL], invname[MSL];

   /*
    * Names checking should really only be done on new characters, otherwise
    * we could end up with people who can't access their characters.  Would
    * have also provided for that new area havoc mentioned below, while still
    * disallowing current area mobnames.  I personally think that if we can
    * have more than one mob with the same keyword, then may as well have
    * players too though, so I don't mind that removal.  -- Alty
    */

   /*
    * Length restrictions.
    */
   if( strlen( name ) < 3 )
      return FALSE;

   if( strlen( name ) > 12 )
      return FALSE;

   /*
    * Alphanumerics only.
    * Lock out IllIll twits.
    */
   {
      char *pc;
      bool fIll;

      fIll = TRUE;
      for( pc = name; *pc != '\0'; pc++ )
      {
         if( !isalpha( *pc ) )
            return FALSE;
         if( LOWER( *pc ) != 'i' && LOWER( *pc ) != 'l' )
            fIll = FALSE;
      }

      if( fIll )
         return FALSE;
   }

   /*
    * Mob names illegal for newbies now - Samson 7-24-00 
    */
   for( vch = first_char; vch; vch = vch->next )
   {
      if( IS_NPC( vch ) )
      {
         if( arg_cmp( vch->name, name ) && newchar )
            return FALSE;
      }
   }

   /*
    * This grep idea was borrowed from SunderMud.
    * * Reserved names list was getting much too large to load into memory.
    * * Placed last so as to avoid problems from any of the previous conditions causing a problem in shell.
    */
   snprintf( buf, MSL, "grep -i -x %s ../system/reserved.lst > /dev/null", name );

   if( system( buf ) == 0 && newchar )
   {
      buf[0] = '\0';
      return FALSE;
   }

   /*
    * Check for inverse naming as well 
    */
   invert( name, invname );
   snprintf( buf, MSL, "grep -i -x %s ../system/reserved.lst > /dev/null", invname );

   if( system( buf ) == 0 && newchar )
   {
      buf[0] = '\0';
      return FALSE;
   }
   return TRUE;
}

/* This may seem silly, but this stuff was called in several spots. So I consolidated. - Samson 2-7-04 */
void show_stateflags( CHAR_DATA * ch )
{







#ifdef MULTIPORT
   /*
    * Make sure Those who can reboot know the compiler is running - Samson 
    */
   if( ch->level >= LEVEL_GOD && compilelock )
      send_to_char( "\n\r&RNOTE: The system compiler is in use. Reboot and Shutdown commands are locked.\n\r", ch );
#endif

   if( bootlock )
      send_to_char( "\n\r&GNOTE: The system is preparing for a reboot.\n\r", ch );

   if( sysdata.LOCKDOWN && ch->level == LEVEL_SUPREME )
      send_to_char( "\n\r&RReminder, Mr. Imp sir, the game is under lockdown. Nobody else can connect now.\n\r", ch );

   if( sysdata.IMPLOCK && IS_IMP( ch ) )
      ch_printf( ch, "\n\r&RNOTE: The game is implocked. Only level %d and above gods can log on.\n\r", LEVEL_KL );

   if( sysdata.WIZLOCK )
      send_to_char( "\n\r&YNOTE: The game is wizlocked. No mortals can log on.\n\r", ch );

   if( crash_count > 0 && IS_IMP( ch ) )
      ch_printf( ch, "\n\r}RThere ha%s been %d intercepted SIGSEGV since reboot. Check the logs.\n\r",
                 crash_count == 1 ? "s" : "ve", crash_count );
   return;
}

void show_status( CHAR_DATA * ch )
{
   if( !IS_IMMORTAL( ch ) && ch->pcdata->motd < sysdata.motd )
   {
      show_file( ch, MOTD_FILE );
      ch->pcdata->motd = current_time;
   }
   else if( IS_IMMORTAL( ch ) && ch->pcdata->imotd < sysdata.imotd )
   {
      show_file( ch, IMOTD_FILE );
      ch->pcdata->imotd = current_time;
   }

   if( exists_file( SPEC_MOTD ) )
      show_file( ch, SPEC_MOTD );

   interpret( ch, "look" );

   /*
    * @shrug, why not? :P 
    */
   if( IS_PLR_FLAG( ch, PLR_ONMAP ) )
      music_to_char( "wilderness.mid", 100, ch, FALSE );

   send_to_char( "&R\n\r", ch );
   if( IS_PCFLAG( ch, PCFLAG_CHECKBOARD ) )
      interpret( ch, "checkboards" );

   if( str_cmp( ch->desc->client, "Unidentified" ) )
      log_printf( "%s client detected for %s.", capitalize( ch->desc->client ), ch->name );
   if( ch->desc->can_compress )
      log_printf( "MCCP support detected for %s.", ch->name );
   if( ch->desc->mxp_detected )
      log_printf( "MXP support detected for %s.", ch->name );
   if( ch->desc->msp_detected )
      log_printf( "MSP support detected for %s.", ch->name );
   quotes( ch );
   show_stateflags( ch );
   return;
}

/*
 * Look for link-dead player to reconnect.
 */
short check_reconnect( DESCRIPTOR_DATA * d, char *name, bool fConn )
{
   CHAR_DATA *ch;

   for( ch = first_char; ch; ch = ch->next )
   {
      if( !IS_NPC( ch ) && ( !fConn || !ch->desc ) && ch->pcdata->filename && !str_cmp( name, ch->pcdata->filename ) )
      {
         if( fConn && ch->switched )
         {
            write_to_buffer( d, "Already playing.\n\rName: ", 0 );
            d->connected = CON_GET_NAME;
            if( d->character )
            {
               /*
                * clear descriptor pointer to get rid of bug message in log 
                */
               d->character->desc = NULL;
               free_char( d->character );
               d->character = NULL;
            }
            return BERR;
         }
         if( fConn == FALSE )
         {
            DISPOSE( d->character->pcdata->pwd );
            d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
         }
         else
         {
            /*
             * clear descriptor pointer to get rid of bug message in log 
             */
            d->character->desc = NULL;
            free_char( d->character );
            d->character = ch;
            ch->desc = d;
            ch->timer = 0;
            send_to_char( "Reconnecting.\n\r\n\r", ch );
            update_connhistory( ch->desc, CONNTYPE_RECONN );

            act( AT_ACTION, "$n has reconnected.", ch, NULL, NULL, TO_CANSEE );
            log_printf_plus( LOG_COMM, ch->level, "%s [%s] reconnected.", ch->name, d->host );
            d->connected = CON_PLAYING;
            check_auth_state( ch ); /* Link dead support -- Rantic */
            show_status( ch );
         }
         return TRUE;
      }
   }

   return FALSE;
}

/*
 * Check if already playing.
 */
short check_playing( DESCRIPTOR_DATA * d, char *name, bool kick )
{
   CHAR_DATA *ch;

   DESCRIPTOR_DATA *dold;
   int cstate;

   for( dold = first_descriptor; dold; dold = dold->next )
   {
      if( dold != d && ( dold->character || dold->original ) && !str_cmp( name, dold->original
                                                                          ? dold->original->pcdata->filename : dold->
                                                                          character->pcdata->filename ) )
      {
         cstate = dold->connected;
         ch = dold->original ? dold->original : dold->character;
         if( !ch->name
             || ( cstate != CON_PLAYING && cstate != CON_EDITING && cstate != CON_DELETE
                  && cstate != CON_ROLL_STATS && cstate != CON_PRIZENAME && cstate != CON_CONFIRMPRIZENAME
                  && cstate != CON_PRIZEKEY && cstate != CON_CONFIRMPRIZEKEY && cstate != CON_RAISE_STAT ) )
         {
            write_to_buffer( d, "Already connected - try again.\n\r", 0 );
            log_printf_plus( LOG_COMM, ch->level, "%s already connected.", ch->pcdata->filename );
            return BERR;
         }
         if( !kick )
            return TRUE;
         write_to_buffer( d, "Already playing... Kicking off old connection.\n\r", 0 );
         write_to_buffer( dold, "Kicking off old connection... bye!\n\r", 0 );
         close_socket( dold, FALSE );
         /*
          * clear descriptor pointer to get rid of bug message in log 
          */
         d->character->desc = NULL;
         free_char( d->character );
         d->character = ch;
         ch->desc = d;
         ch->timer = 0;
         if( ch->switched )
            interpret( ch->switched, "return" );
         ch->switched = NULL;
         send_to_char( "Reconnecting.\n\r\n\r", ch );

         act( AT_ACTION, "$n has reconnected, kicking off old link.", ch, NULL, NULL, TO_CANSEE );
         log_printf_plus( LOG_COMM, ch->level, "%s [%s] reconnected, kicking off old link.", ch->name, d->host );
         d->connected = cstate;
         show_status( ch );
         return TRUE;
      }
   }

   return FALSE;
}

char *md5_crypt( const char *pwd )
{
   md5_state_t state;
   static md5_byte_t digest[17];
   static char passwd[17];
   unsigned int x;

   md5_init( &state );
   md5_append( &state, ( const md5_byte_t * )pwd, strlen( pwd ) );
   md5_finish( &state, digest );

   mudstrlcpy( passwd, ( const char * )digest, 16 );

   /*
    * The listed exceptions below will fubar the MD5 authentication packets, so change them 
    */
   for( x = 0; x < strlen( passwd ); x++ )
   {
      if( passwd[x] == '\n' )
         passwd[x] = 'n';
      if( passwd[x] == '\r' )
         passwd[x] = 'r';
      if( passwd[x] == '\t' )
         passwd[x] = 't';
      if( passwd[x] == ' ' )
         passwd[x] = 's';
      if( ( int )passwd[x] == 11 )
         passwd[x] = 'x';
      if( ( int )passwd[x] == 12 )
         passwd[x] = 'X';
      if( passwd[x] == '~' )
         passwd[x] = '+';
      if( passwd[x] == EOF )
         passwd[x] = 'E';
   }
   return ( passwd );
}

/*
 * Deal with sockets that haven't logged in yet.
 */
/* Function modified from original form on varying dates - Samson */
/* Password encryption sections upgraded to use MD5 Encryption - Samson 7-10-00 */
void nanny( DESCRIPTOR_DATA * d, char *argument )
{
   char buf[MSL];
   CHAR_DATA *ch;
   char *pwdnew;
   bool fOld;
   short chk;

   while( isspace( *argument ) )
      argument++;

   ch = d->character;

   switch ( d->connected )
   {

      default:
         bug( "%s: bad d->connected %d.", __FUNCTION__, d->connected );
         close_socket( d, TRUE );
         return;

      case CON_OEDIT:
         oedit_parse( d, argument );
         break;

      case CON_REDIT:
         redit_parse( d, argument );
         break;

      case CON_MEDIT:
         medit_parse( d, argument );
         break;

      case CON_BOARD:
         board_parse( d, argument );
         break;

      case CON_GET_NAME:
         if( !argument || argument[0] == '\0' )
         {
            close_socket( d, FALSE );
            return;
         }

         /*
          * Old players can keep their characters. -- Alty 
          */
         argument = strlower( argument ); /* Modification to force proper name display */
         argument[0] = UPPER( argument[0] ); /* Samson 5-22-98 */
         if( !check_parse_name( argument, ( d->newstate != 0 ) ) )
         {
            write_to_buffer( d, "You have chosen a name which is unnacceptable.\n\r", 0 );
            write_to_buffer( d, "Acceptable names cannot be:\n\r\n\r", 0 );
            write_to_buffer( d, "- Nonsensical, unpronounceable or ridiculous.\n\r", 0 );
            write_to_buffer( d, "- Profane or derogatory as interpreted in any language.\n\r", 0 );
            write_to_buffer( d, "- Modern, futuristic, or common, such as 'Jill' or 'Laser'.\n\r", 0 );
            write_to_buffer( d, "- Similar to that of any Immortal, monster, or object.\n\r", 0 );
            write_to_buffer( d, "- Comprised of non-alphanumeric characters.\n\r", 0 );
            write_to_buffer( d, "- Comprised of various capital letters, such as 'BrACkkA' or 'CORTO'.\n\r", 0 );
            write_to_buffer( d, "- Comprised of ranks or titles, such as 'Lord' or 'Master'.\n\r", 0 );
            write_to_buffer( d, "- Composed of singular descriptive nouns, adverbs or adjectives,\n\r", 0 );
            write_to_buffer( d, "  as in 'Heart', 'Big', 'Flying', 'Broken', 'Slick' or 'Tricky'.\n\r", 0 );
            write_to_buffer( d, "- Any of the above in reverse, i.e., writing Joe as 'Eoj'.\n\r", 0 );
            write_to_buffer( d, "- Anything else the Immortal staff deems inappropriate.\n\r\n\r", 0 );
            write_to_buffer( d, "Please choose a new name: ", 0 );
            return;
         }

         if( !str_cmp( argument, "New" ) )
         {
            if( d->newstate == 0 )
            {
               /*
                * New player 
                */
               /*
                * Don't allow new players if DENY_NEW_PLAYERS is TRUE 
                */
               if( sysdata.DENY_NEW_PLAYERS == TRUE )
               {
                  write_to_buffer( d, "The mud is currently preparing for a reboot.\n\r", 0 );
                  write_to_buffer( d, "New players are not accepted during this time.\n\r", 0 );
                  write_to_buffer( d, "Please try again in a few minutes.\n\r", 0 );
                  close_socket( d, FALSE );
               }
               write_to_buffer( d, "\n\rChoosing a name is one of the most important parts of this game...\n\r"
                                "Make sure to pick a name appropriate to the character you are going\n\r"
                                "to role play, and be sure that it suits a medieval theme.\n\r"
                                "If the name you select is not acceptable, you will be asked to choose\n\r"
                                "another one.\n\r\n\rPlease choose a name for your character: ", 0 );
               d->newstate++;
               d->connected = CON_GET_NAME;
               return;
            }
            else
            {
               write_to_buffer( d, "You have chosen a name which is unnacceptable.\n\r", 0 );
               write_to_buffer( d, "Acceptable names cannot be:\n\r\n\r", 0 );
               write_to_buffer( d, "- Nonsensical, unpronounceable or ridiculous.\n\r", 0 );
               write_to_buffer( d, "- Profane or derogatory as interpreted in any language.\n\r", 0 );
               write_to_buffer( d, "- Modern, futuristic, or common, such as 'Jill' or 'Laser'.\n\r", 0 );
               write_to_buffer( d, "- Similar to that of any Immortal, monster, or object.\n\r", 0 );
               write_to_buffer( d, "- Comprised of non-alphanumeric characters.\n\r", 0 );
               write_to_buffer( d, "- Comprised of various capital letters, such as 'BrACkkA' or 'CORTO'.\n\r", 0 );
               write_to_buffer( d, "- Comprised of ranks or titles, such as 'Lord' or 'Master'.\n\r", 0 );
               write_to_buffer( d, "- Composed of singular descriptive nouns, adverbs or adjectives,\n\r", 0 );
               write_to_buffer( d, "  as in 'Heart', 'Big', 'Flying', 'Broken', 'Slick' or 'Tricky'.\n\r", 0 );
               write_to_buffer( d, "- Any of the above in reverse, i.e., writing Joe as 'Eoj'.\n\r", 0 );
               write_to_buffer( d, "- Anything else the Immortal staff deems inappropriate.\n\r\n\r", 0 );
               write_to_buffer( d, "Please choose a new name: ", 0 );
               return;
            }
         }

         if( check_playing( d, argument, FALSE ) == BERR )
         {
            write_to_buffer( d, "Name: ", 0 );
            return;
         }

         log_printf_plus( LOG_COMM, LEVEL_KL, "Incoming connection: %s, port %d.", d->host, d->port );

         fOld = load_char_obj( d, argument, TRUE, FALSE );

         if( !d->character )
         {
            bug( "Bad player file %s@%s.", argument, d->host );
            buffer_printf( d, "Your playerfile is corrupt...Please notify %s\n\r", sysdata.admin_email );
            close_socket( d, FALSE );
            return;
         }
         ch = d->character;
         if( check_bans( ch ) )
         {
            write_to_buffer( d, "Your site has been banned from this Mud.\n\r", 0 );
            close_socket( d, FALSE );
            return;
         }

         if( IS_PLR_FLAG( ch, PLR_DENY ) )
         {
            log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Denying access to %s@%s.", argument, d->host );
            if( d->newstate != 0 )
            {
               write_to_buffer( d, "That name is already taken.  Please choose another: ", 0 );
               d->connected = CON_GET_NAME;
               d->character->desc = NULL;
               free_char( d->character ); /* Big Memory Leak before --Shaddai */
               d->character = NULL;
               return;
            }
            write_to_buffer( d, "You are denied access.\n\r", 0 );
            close_socket( d, FALSE );
            return;
         }
         /*
          * Make sure the immortal host is from the correct place. Shaddai 
          */

         if( IS_IMMORTAL( ch ) && sysdata.check_imm_host && !check_immortal_domain( ch, d->host ) )
         {
            write_to_buffer( d, "Invalid host profile. This event has been logged for inspection.\n\r", 0 );
            close_socket( d, FALSE );
            return;
         }

         chk = check_reconnect( d, argument, FALSE );
         if( chk == BERR )
            return;

         if( chk )
            fOld = TRUE;

         if( ( sysdata.WIZLOCK || sysdata.IMPLOCK || sysdata.LOCKDOWN ) && !IS_IMMORTAL( ch ) )
         {
            write_to_buffer( d, "The game is wizlocked. Only immortals can connect now.\n\r", 0 );
            write_to_buffer( d, "Please try back later.\n\r", 0 );
            log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Player: %s disconnected due to %s.", ch->name,
                             sysdata.WIZLOCK ? "wizlock" : sysdata.IMPLOCK ? "implock" : "lockdown" );
            close_socket( d, FALSE );
            return;
         }
         else if( sysdata.LOCKDOWN && ch->level < LEVEL_SUPREME )
         {
            write_to_buffer( d, "The game is locked down. Only the head implementor can connect now.\n\r", 0 );
            write_to_buffer( d, "Please try back later.\n\r", 0 );
            log_printf_plus( LOG_COMM, LEVEL_SUPREME, "Immortal: %s disconnected due to lockdown.", ch->name );
            close_socket( d, FALSE );
            return;
         }
         else if( sysdata.IMPLOCK && !IS_IMP( ch ) )
         {
            write_to_buffer( d, "The game is implocked. Only implementors can connect now.\n\r", 0 );
            write_to_buffer( d, "Please try back later.\n\r", 0 );
            log_printf_plus( LOG_COMM, LEVEL_KL, "Immortal: %s disconnected due to implock.", ch->name );
            close_socket( d, FALSE );
            return;
         }
         else if( bootlock && !IS_IMMORTAL( ch ) )
         {
            write_to_buffer( d, "The game is preparing to reboot. Please try back in about 5 minutes.\n\r", 0 );
            log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Player: %s disconnected due to bootlock.", ch->name );
            close_socket( d, FALSE );
            return;
         }

         if( fOld )
         {
            if( d->newstate != 0 )
            {
               write_to_buffer( d, "That name is already taken.  Please choose another: ", 0 );
               d->connected = CON_GET_NAME;
               d->character->desc = NULL;
               free_char( d->character ); /* Big Memory Leak before --Shaddai */
               d->character = NULL;
               return;
            }
            /*
             * Old player 
             */
            write_to_buffer( d, "Enter your password: ", 0 );
            write_to_buffer( d, echo_off_str, 0 );
            d->connected = CON_GET_OLD_PASSWORD;
            return;
         }
         else
         {
            if( d->newstate == 0 )
            {
               /*
                * No such player 
                */
               write_to_buffer( d,
                                "\n\rNo such player exists.\n\rPlease check your spelling, or type new to start a new player.\n\r\n\rName: ",
                                0 );
               d->connected = CON_GET_NAME;
               d->character->desc = NULL;
               free_char( d->character ); /* Big Memory Leak before --Shaddai */
               d->character = NULL;
               return;
            }
            argument = strlower( argument ); /* Added to force names to display properly */
            argument = capitalize( argument );  /* Samson 5-22-98 */
            STRFREE( ch->name );
            ch->name = STRALLOC( argument );
            buffer_printf( d, "Did I get that right, %s (Y/N)? ", argument );
            d->connected = CON_CONFIRM_NEW_NAME;
            return;
         }

      case CON_GET_OLD_PASSWORD:
         write_to_buffer( d, "\n\r", 2 );

         if( ch->pcdata->version < 22 )
         {
            if( str_cmp( md5_crypt( argument ), ch->pcdata->pwd ) )
            {
               write_to_buffer( d, "Wrong password, disconnecting.\n\r", 0 );
               /*
                * clear descriptor pointer to get rid of bug message in log 
                */
               d->character->desc = NULL;
               close_socket( d, FALSE );
               return;
            }
         }
         else
         {
            if( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) )
            {
               write_to_buffer( d, "Wrong password, disconnecting.\n\r", 0 );
               /*
                * clear descriptor pointer to get rid of bug message in log 
                */
               d->character->desc = NULL;
               close_socket( d, FALSE );
               return;
            }
         }

         write_to_buffer( d, echo_on_str, 0 );

         if( check_playing( d, ch->pcdata->filename, TRUE ) )
            return;

         chk = check_reconnect( d, ch->pcdata->filename, TRUE );
         if( chk == BERR )
         {
            if( d->character && d->character->desc )
               d->character->desc = NULL;
            close_socket( d, FALSE );
            return;
         }
         if( chk == TRUE )
            return;

         mudstrlcpy( buf, ch->pcdata->filename, MSL );
         d->character->desc = NULL;
         free_char( d->character );
         d->character = NULL;
         fOld = load_char_obj( d, buf, FALSE, FALSE );
         ch = d->character;
         if( ch->position > POS_SITTING && ch->position < POS_STANDING )
            ch->position = POS_STANDING;

         log_printf_plus( LOG_COMM, LEVEL_KL, "%s [%s] has connected.", ch->name, d->host );

         if( ch->pcdata->version < 22 )
         {
            DISPOSE( ch->pcdata->pwd );
            ch->pcdata->pwd = str_dup( sha256_crypt( argument ) );
         }
         show_title( d );
         break;

      case CON_CONFIRM_NEW_NAME:
         switch ( *argument )
         {
            case 'y':
            case 'Y':
               buffer_printf( d, "\n\rMake sure to use a password that won't be easily guessed by someone else."
                              "\n\rPick a good password for %s: %s", ch->name, echo_off_str );
               d->connected = CON_GET_NEW_PASSWORD;
               break;

            case 'n':
            case 'N':
               write_to_buffer( d, "Ok, what IS it, then? ", 0 );
               /*
                * clear descriptor pointer to get rid of bug message in log 
                */
               d->character->desc = NULL;
               free_char( d->character );
               d->character = NULL;
               d->connected = CON_GET_NAME;
               break;

            default:
               write_to_buffer( d, "Please type Yes or No. ", 0 );
               break;
         }
         break;

      case CON_GET_NEW_PASSWORD:
         write_to_buffer( d, "\n\r", 2 );

         if( strlen( argument ) < 5 )
         {
            write_to_buffer( d, "Password must be at least five characters long.\n\rPassword: ", 0 );
            return;
         }

         if( argument[0] == '!' )
         {
            write_to_buffer( d, "Password cannot begin with the '!' character.\n\rPassword: ", 0 );
            return;
         }

         pwdnew = sha256_crypt( argument );  /* SHA-256 Encryption */
         DISPOSE( ch->pcdata->pwd );
         ch->pcdata->pwd = str_dup( pwdnew );
         write_to_buffer( d, "\n\rPlease retype the password to confirm: ", 0 );
         d->connected = CON_CONFIRM_NEW_PASSWORD;
         break;

      case CON_CONFIRM_NEW_PASSWORD:
         write_to_buffer( d, "\n\r", 2 );

         if( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) )
         {
            write_to_buffer( d, "Passwords don't match.\n\rRetype password: ", 0 );
            d->connected = CON_GET_NEW_PASSWORD;
            return;
         }

#ifdef MULTIPORT
         if( port != MAINPORT )
         {
            write_to_buffer( d, "This is a restricted access port. Only immortals and their test players are allowed.\n\r",
                             0 );
            write_to_buffer( d, "Enter access code: ", 0 );
            write_to_buffer( d, echo_off_str, 0 );
            d->connected = CON_GET_PORT_PASSWORD;
            return;
         }
#endif

         write_to_buffer( d, echo_on_str, 0 );

         write_to_buffer( d, "\n\rPlease note: You will be able to pick race and Class after entering the game.", 0 );
         write_to_buffer( d, "\n\rWhat is your sex? ", 0 );
         write_to_buffer( d, "\n\r(M)ale, (F)emale, (N)euter, or (H)ermaphrodyte ?", 0 );
         d->connected = CON_GET_NEW_SEX;
         break;

      case CON_GET_PORT_PASSWORD:
         write_to_buffer( d, "\n\r", 2 );

         if( str_cmp( sha256_crypt( argument ), sysdata.password ) )
         {
            write_to_buffer( d, "Invalid access code.\n\r", 0 );
            /*
             * clear descriptor pointer to get rid of bug message in log 
             */
            d->character->desc = NULL;
            close_socket( d, FALSE );
            return;
         }
         write_to_buffer( d, echo_on_str, 0 );
         write_to_buffer( d, "\n\rPlease note: You will be able to pick race and class after entering the game.", 0 );
         write_to_buffer( d, "\n\rWhat is your sex? ", 0 );
         write_to_buffer( d, "\n\r(M)ale, (F)emale, (N)euter, or (H)ermaphrodyte ?", 0 );
         d->connected = CON_GET_NEW_SEX;
         break;

      case CON_GET_NEW_SEX:
         switch ( argument[0] )
         {
            case 'm':
            case 'M':
               ch->sex = SEX_MALE;
               break;
            case 'f':
            case 'F':
               ch->sex = SEX_FEMALE;
               break;
            case 'n':
            case 'N':
               ch->sex = SEX_NEUTRAL;
               break;
            case 'h':
            case 'H':
               ch->sex = SEX_HERMAPHRODYTE;
               break;
            default:
               write_to_buffer( d, "That's not a sex.\n\rWhat IS your sex? ", 0 );
               return;
         }

         /*
          * Default to AFKMud interface. They can change this if they want. Samson 7-24-03 
          */
         ch->pcdata->interface = INT_AFKMUD;

         /*
          * ANSI defaults to on now. This is 2003 folks. Grow up. Samson 7-24-03 
          */
         SET_PLR_FLAG( ch, PLR_ANSI );
         reset_colors( ch );  /* Added for new configurable color code - Samson 3-27-98 */

         /*
          * MXP and MSP terminal support. Activate once at creation - Samson 8-21-01 
          */
         if( ch->desc->msp_detected )
            SET_PLR_FLAG( ch, PLR_MSP );

         if( ch->desc->mxp_detected )
            SET_PLR_FLAG( ch, PLR_MXP );

         write_to_buffer( d, "Press [ENTER] ", 0 );
         show_title( d );
         ch->level = 0;
         ch->position = POS_STANDING;
         d->connected = CON_PRESS_ENTER;
         return;

      case CON_PRESS_ENTER:
         set_pager_color( AT_PLAIN, ch );
         if( chk_watch( ch->level, ch->name, d->host ) ) /*  --Gorog */
            SET_PCFLAG( ch, PCFLAG_WATCH );
         else
            REMOVE_PCFLAG( ch, PCFLAG_WATCH );

         set_pager_color( AT_PLAIN, ch ); /* Set default color so they don't see blank space */

         if( ch->position == POS_MOUNTED )
            ch->position = POS_STANDING;

         /*
          * MOTD messages now handled in separate function - Samson 12-31-00 
          */
         display_motd( ch );

         break;

         /*
          * Con state for self delete code, added by Samson 1-18-98
          * Code courtesy of Waldemar Thiel (Swiv) 
          */
      case CON_DELETE:
         write_to_buffer( d, "\n\r", 2 );

         if( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) )
         {
            write_to_buffer( d, "Wrong password entered, deletion cancelled.\n\r", 0 );
            write_to_buffer( d, echo_on_str, 0 );
            d->connected = CON_PLAYING;
            return;
         }
         else
         {
            ROOM_INDEX_DATA *donate = get_room_index( ROOM_VNUM_DONATION );
            OBJ_DATA *obj, *obj_next;

            write_to_buffer( d, "\n\rYou've deleted your character!!!\n\r", 0 );
            log_printf( "Player: %s has deleted.", capitalize( ch->name ) );

            if( donate != NULL && ch->level > 1 )  /* No more deleting to remove goodies from play */
            {
               for( obj = ch->first_carrying; obj != NULL; obj = obj_next )
               {
                  obj_next = obj->next_content;
                  if( obj->wear_loc != WEAR_NONE )
                     unequip_char( ch, obj );
               }

               for( obj = ch->first_carrying; obj; obj = obj_next )
               {
                  obj_next = obj->next_content;
                  separate_obj( obj );
                  obj_from_char( obj );
                  if( !obj_next )
                     obj_next = ch->first_carrying;
                  if( IS_OBJ_FLAG( obj, ITEM_PERSONAL ) || !can_drop_obj( ch, obj ) )
                     extract_obj( obj );
                  else
                  {
                     obj = obj_to_room( obj, donate, ch );
                     SET_OBJ_FLAG( obj, ITEM_DONATION );
                  }
               }
            }

            do_destroy( ch, ch->name );
            return;
         }

         /*
          * Begin Sindhae Section 
          */
      case CON_PRIZENAME:
      {
         OBJ_DATA *prize;

         prize = ( OBJ_DATA * ) ch->pcdata->spare_ptr;
         if( !prize )
         {
            bug( "%s", "con_sindhae: Prize object turned NULL somehow!" );
            write_to_buffer( d, "A fatal internal error has occured. Seek immortal assistance.\n\r", 0 );
            char_from_room( ch );
            if( !char_to_room( ch, get_room_index( ROOM_VNUM_REDEEM ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            d->connected = CON_PLAYING;
            break;
         }

         if( !argument || argument[0] == '\0' )
         {
            write_to_buffer( d, "You must specify a name for your prize.\n\r\n\r", 0 );
            ch_printf( ch, "You are editing %s.\n\r", prize->short_descr );
            write_to_buffer( d, "[SINDHAE] Prizename: ", 0 );
            return;
         }

         if( !str_cmp( argument, "help" ) )
         {
            do_help( ch, "prizenaming" );
            ch_printf( ch, "\n\r&RYou are editing %s&R.\n\r", prize->short_descr );
            write_to_buffer( d, "[SINDHAE] Prizename: ", 0 );
            return;
         }

         if( !str_cmp( argument, "help pcolors" ) )
         {
            do_help( ch, "pcolors" );
            set_char_color( AT_RED, ch );
            ch_printf( ch, "\n\r&RYou are editing %s&R.\n\r", prize->short_descr );
            write_to_buffer( d, "[SINDHAE] Prizename: ", 0 );
            return;
         }

         /*
          * God, I hope this works 
          */
         STRFREE( prize->short_descr );
         prize->short_descr = STRALLOC( argument );

         send_to_char( "\n\rYour prize will look like this when worn:\n\r", ch );
         send_to_char( format_obj_to_char( prize, ch, TRUE ), ch );
         set_char_color( AT_RED, ch );
         write_to_buffer( d, "\n\rIs this correct? (Y/N)", 0 );
         ch->pcdata->spare_ptr = prize;
         d->connected = CON_CONFIRMPRIZENAME;
      }
         break;

      case CON_CONFIRMPRIZENAME:
      {
         OBJ_DATA *prize;

         prize = ( OBJ_DATA * ) ch->pcdata->spare_ptr;
         if( !prize )
         {
            bug( "%s", "con_sindhae: Prize object turned NULL somehow!" );
            write_to_buffer( d, "A fatal internal error has occured. Seek immortal assistance.\n\r", 0 );
            char_from_room( ch );
            if( !char_to_room( ch, get_room_index( ROOM_VNUM_REDEEM ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            d->connected = CON_PLAYING;
            break;
         }

         switch ( argument[0] )
         {
            case 'y':
            case 'Y':
               write_to_buffer( d, "\n\rPrize name is confirmed.\n\r\n\r", 0 );
               set_char_color( AT_YELLOW, ch );
               write_to_buffer( d, "You will now select at least one keyword for your prize.\n\r", 0 );
               write_to_buffer( d, "A keyword is a word you use to manipulate the object.\n\r", 0 );
               write_to_buffer( d, "Your keywords many NOT contain any color tags.\n\r\n\r", 0 );
               ch_printf( ch, "&RYou are editing %s&R.\n\r", prize->short_descr );
               write_to_buffer( d, "[SINDHAE] Prizekey: ", 0 );
               ch->pcdata->spare_ptr = prize;
               d->connected = CON_PRIZEKEY;
               break;

            case 'n':
            case 'N':
               write_to_buffer( d, "\n\rOk, then please enter the correct name.\n\r", 0 );
               ch_printf( ch, "\n\r&RYou are editing %s&R.\n\r", prize->short_descr );
               write_to_buffer( d, "[SINDHAE] Prizename: ", 0 );
               ch->pcdata->spare_ptr = prize;
               d->connected = CON_PRIZENAME;
               return;

            default:
               write_to_buffer( d, "Yes or No? ", 0 );
               return;
         }
      }
         break;

      case CON_PRIZEKEY:
      {
         OBJ_DATA *prize;

         prize = ( OBJ_DATA * ) ch->pcdata->spare_ptr;
         if( !prize )
         {
            bug( "%s", "con_prizekey: Prize object turned NULL somehow!" );
            write_to_buffer( d, "A fatal internal error has occured. Seek immortal assistance.\n\r", 0 );
            char_from_room( ch );
            if( !char_to_room( ch, get_room_index( ROOM_VNUM_REDEEM ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            d->connected = CON_PLAYING;
            break;
         }

         if( !argument || argument[0] == '\0' )
         {
            write_to_buffer( d, "You must specify keywords for your prize.\n\r\n\r", 0 );
            ch_printf( ch, "&RYou are editing %s&R.\n\r", prize->short_descr );
            write_to_buffer( d, "[SINDHAE] Prizekey: ", 0 );
            return;
         }

         if( !str_cmp( argument, "help" ) )
         {
            do_help( ch, "prizekey" );
            ch_printf( ch, "\n\r&RYou are editing %s&R.", prize->short_descr );
            write_to_buffer( d, "\n\r[SINDHAE] Prizekey: ", 0 );
            return;
         }
         stralloc_printf( &prize->name, "%s %s", ch->name, argument );
         ch_printf( ch, "\n\rYou chose these keywords: %s\n\r", prize->name );
         write_to_buffer( d, "Is this correct? (Y/N)", 0 );
         ch->pcdata->spare_ptr = prize;
         d->connected = CON_CONFIRMPRIZEKEY;
      }
         break;

      case CON_CONFIRMPRIZEKEY:
      {
         OBJ_DATA *prize;

         prize = ( OBJ_DATA * ) ch->pcdata->spare_ptr;
         if( !prize )
         {
            bug( "%s", "con_prizekey: Prize object turned NULL somehow!" );
            write_to_buffer( d, "A fatal internal error has occured. Seek immortal assistance.\n\r", 0 );
            char_from_room( ch );
            if( !char_to_room( ch, get_room_index( ROOM_VNUM_REDEEM ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            d->connected = CON_PLAYING;
            break;
         }

         switch ( argument[0] )
         {
            case 'y':
            case 'Y':
               write_to_buffer( d, "\n\rPrize keywords confirmed.\n\r\n\r", 0 );
               ch->pcdata->spare_ptr = NULL;
               char_from_room( ch );
               if( !char_to_room( ch, get_room_index( ROOM_VNUM_ENDREDEEM ) ) )
                  log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
               interpret( ch, "look" );
               set_char_color( AT_BLUE, ch );
               if( ch->Class == CLASS_BARD && prize->item_type == ITEM_INSTRUMENT )
                  stralloc_printf( &prize->name, "minstru %s", prize->name );

               write_to_buffer( d, "Congratulations! You've completed the redemption.\n\r", 0 );
               write_to_buffer( d, "Your prize should be in your inventory.\n\r", 0 );
               write_to_buffer( d, "If anything appears wrong, seek immortal assistance.\n\r\n\r", 0 );
               d->connected = CON_PLAYING;
               break;

            case 'n':
            case 'N':
               write_to_buffer( d, "\n\rOk, then please enter the correct keywords.\n\r", 0 );
               ch_printf( ch, "\n\r&RYou are editing %s&R.", prize->short_descr );
               write_to_buffer( d, "[SINDHAE] Prizekey: ", 0 );
               d->connected = CON_PRIZEKEY;
               return;

            default:
               write_to_buffer( d, "Yes or No? ", 0 );
               return;
         }
      }
         break;
         /*
          * End Sindhae Section 
          */

      case CON_RAISE_STAT:
      {
         if( ch->tempnum < 1 )
         {
            bug( "CON_RAISE_STAT: ch loop counter is < 1 for %s", ch->name );
            d->connected = CON_PLAYING;
            return;
         }

         if( ch->perm_str >= 18 + race_table[ch->race]->str_plus
             && ch->perm_int >= 18 + race_table[ch->race]->int_plus
             && ch->perm_wis >= 18 + race_table[ch->race]->wis_plus
             && ch->perm_dex >= 18 + race_table[ch->race]->dex_plus
             && ch->perm_con >= 18 + race_table[ch->race]->con_plus
             && ch->perm_cha >= 18 + race_table[ch->race]->cha_plus && ch->perm_lck >= 18 + race_table[ch->race]->lck_plus )
         {
            bug( "CON_RAISE_STAT: %s is unable to raise anything.", ch->name );
            write_to_buffer( d, "All of your stats are already at their maximum values!\n\r", 0 );
            d->connected = CON_PLAYING;
            return;
         }

         switch ( argument[0] )
         {
            default:
               send_to_char( "Invalid choice. Choose 1-7.\n\r", ch );
               return;

            case '1':
               if( ch->perm_str >= 18 + race_table[ch->race]->str_plus )
                  buffer_printf( d, "You cannot raise your strength beyond %d\n\r", ch->perm_str );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_str += 1;
                  buffer_printf( d, "You've raised your strength to %d!\n\r", ch->perm_str );
               }
               if( ch->tempnum < 1 )
                  d->connected = CON_PLAYING;
               else
                  show_stats( ch, d );
               break;

            case '2':
               if( ch->perm_int >= 18 + race_table[ch->race]->int_plus )
                  buffer_printf( d, "You cannot raise your intelligence beyond %d\n\r", ch->perm_int );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_int += 1;
                  buffer_printf( d, "You've raised your intelligence to %d!\n\r", ch->perm_int );
               }
               if( ch->tempnum < 1 )
                  d->connected = CON_PLAYING;
               else
                  show_stats( ch, d );
               break;

            case '3':
               if( ch->perm_wis >= 18 + race_table[ch->race]->wis_plus )
                  buffer_printf( d, "You cannot raise your wisdom beyond %d\n\r", ch->perm_wis );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_wis += 1;
                  buffer_printf( d, "You've raised your wisdom to %d!\n\r", ch->perm_wis );
               }
               if( ch->tempnum < 1 )
                  d->connected = CON_PLAYING;
               else
                  show_stats( ch, d );
               break;

            case '4':
               if( ch->perm_dex >= 18 + race_table[ch->race]->dex_plus )
                  buffer_printf( d, "You cannot raise your dexterity beyond %d\n\r", ch->perm_dex );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_dex += 1;
                  buffer_printf( d, "You've raised your dexterity to %d!\n\r", ch->perm_dex );
               }
               if( ch->tempnum < 1 )
                  d->connected = CON_PLAYING;
               else
                  show_stats( ch, d );
               break;

            case '5':
               if( ch->perm_con >= 18 + race_table[ch->race]->con_plus )
                  buffer_printf( d, "You cannot raise your constitution beyond %d\n\r", ch->perm_con );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_con += 1;
                  buffer_printf( d, "You've raised your constitution to %d!\n\r", ch->perm_con );
               }
               if( ch->tempnum < 1 )
                  d->connected = CON_PLAYING;
               else
                  show_stats( ch, d );
               break;

            case '6':
               if( ch->perm_cha >= 18 + race_table[ch->race]->cha_plus )
                  buffer_printf( d, "You cannot raise your charisma beyond %d\n\r", ch->perm_cha );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_cha += 1;
                  buffer_printf( d, "You've raised your charisma to %d!\n\r", ch->perm_cha );
               }
               if( ch->tempnum < 1 )
                  d->connected = CON_PLAYING;
               else
                  show_stats( ch, d );
               break;

            case '7':
               if( ch->perm_lck >= 18 + race_table[ch->race]->lck_plus )
                  buffer_printf( d, "You cannot raise your luck beyond %d\n\r", ch->perm_lck );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_lck += 1;
                  buffer_printf( d, "You've raised your luck to %d!\n\r", ch->perm_lck );
               }
               if( ch->tempnum < 1 )
                  d->connected = CON_PLAYING;
               else
                  show_stats( ch, d );
               break;
         }
         break;
      }

      case CON_ROLL_STATS:
      {
         switch ( argument[0] )
         {
            case 'y':
            case 'Y':
               write_to_buffer( d, "\n\rYour stats have been set.", 0 );
               d->connected = CON_PLAYING;
               break;

            case 'n':
            case 'N':
               name_stamp_stats( ch );
               buffer_printf( d, "\n\rStr: %s\n\r", attribtext( ch->perm_str ) );
               buffer_printf( d, "Int: %s\n\r", attribtext( ch->perm_int ) );
               buffer_printf( d, "Wis: %s\n\r", attribtext( ch->perm_wis ) );
               buffer_printf( d, "Dex: %s\n\r", attribtext( ch->perm_dex ) );
               buffer_printf( d, "Con: %s\n\r", attribtext( ch->perm_con ) );
               buffer_printf( d, "Cha: %s\n\r", attribtext( ch->perm_cha ) );
               buffer_printf( d, "Lck: %s\n\r", attribtext( ch->perm_lck ) );
               write_to_buffer( d, "\n\rKeep these stats? (Y/N)", 0 );
               return;

            default:
               write_to_buffer( d, "Yes or No? ", 0 );
               return;
         }
         break;
      }

      case CON_READ_MOTD:
      {
         if( exists_file( SPEC_MOTD ) )
         {
            if( IS_PLR_FLAG( ch, PLR_ANSI ) )
               send_to_pager( "\033[2J", ch );
            else
               send_to_pager( "\014", ch );

            show_file( ch, SPEC_MOTD );
         }
         buffer_printf( d, "\n\rWelcome to %s...\n\r", sysdata.mud_name );
         char_to_game( ch );
      }
         break;

   }  /* End Nanny switch */

   return;
}

void game_loop( void )
{
   struct timeval last_time;
   char cmdline[MIL];
   DESCRIPTOR_DATA *d;

   gettimeofday( &last_time, NULL );
   current_time = ( time_t ) last_time.tv_sec;

   /*
    * Main loop 
    */
   while( !mud_down )
   {
      accept_new( control );

      /*
       * Primative, yet effective infinite loop catcher 
       */
      set_alarm( 30 );
      alarm_section = "game_loop";

#ifdef WEBSVR
      if( sysdata.webrunning )
         handle_web(  );
#endif

      /*
       * Kick out descriptors with raised exceptions
       * or have been idle, then check for input.
       */
      for( d = first_descriptor; d; d = d_next )
      {
         if( d == d->next )
         {
            bug( "%s", "descriptor_loop: loop found & fixed" );
            d->next = NULL;
         }
         d_next = d->next;

#ifdef MULTIPORT
         /*
          * Shell code - checks for forked descriptors 
          */
         if( d->connected == CON_FORKED )
         {
            int status;
            if( !d->process )
               d->connected = CON_PLAYING;

            if( ( waitpid( d->process, &status, WNOHANG ) ) == -1 )
            {
               d->connected = CON_PLAYING;
               d->process = 0;
               fcntl( d->descriptor, F_SETFL, FNDELAY );
            }
            else if( status > 0 )
            {
               CHAR_DATA *ch;
               d->connected = CON_PLAYING;
               d->process = 0;
               fcntl( d->descriptor, F_SETFL, FNDELAY );
               ch = d->original ? d->original : d->character;
               if( ch )
                  ch_printf( ch, "Process exited with status code %d.\n\r", status );
            }
            if( d->connected == CON_FORKED )
               continue;
         }
#endif

         d->idle++;  /* make it so a descriptor can idle out */
         if( FD_ISSET( d->descriptor, &exc_set ) )
         {
            FD_CLR( d->descriptor, &in_set );
            FD_CLR( d->descriptor, &out_set );
            if( d->character && d->connected >= CON_PLAYING )
               save_char_obj( d->character );
            d->outtop = 0;
            close_socket( d, TRUE );
            continue;
         }
         else if( ( !d->character && d->idle > 360 )  /* 2 mins */
                  || ( d->connected != CON_PLAYING && d->idle > 2400 )  /* 10 mins */
                  || ( ( d->idle > 14400 ) && ( d->character->level < LEVEL_IMMORTAL ) )  /* 1hr */
                  || ( ( d->idle > 32000 ) && ( d->character->level >= LEVEL_IMMORTAL ) ) )
            /*
             * imms idle off after 32000 to prevent rollover crashes 
             */
         {
            write_to_descriptor( d, "Idle timeout... disconnecting.\n\r", 0 );
            update_connhistory( d, CONNTYPE_IDLE );
            d->outtop = 0;
            close_socket( d, TRUE );
            continue;
         }
         else
         {
            d->fcommand = FALSE;

            if( FD_ISSET( d->descriptor, &in_set ) )
            {
               d->idle = 0;
               if( d->character )
                  d->character->timer = 0;
               if( !read_from_descriptor( d ) )
               {
                  FD_CLR( d->descriptor, &out_set );
                  if( d->character && d->connected >= CON_PLAYING )
                     save_char_obj( d->character );
                  d->outtop = 0;
                  update_connhistory( d, CONNTYPE_LINKDEAD );
                  close_socket( d, FALSE );
                  continue;
               }
            }

            /*
             * check for input from the dns 
             */
            if( ( d->connected == CON_PLAYING || d->character != NULL ) && d->ifd != -1 && FD_ISSET( d->ifd, &in_set ) )
               process_dns( d );

            if( d->character && d->character->wait > 0 )
            {
               --d->character->wait;
               continue;
            }

            read_from_buffer( d );
            if( d->incomm[0] != '\0' )
            {
               d->fcommand = TRUE;
               if( d->character && !IS_AFFECTED( d->character, AFF_SPAMGUARD ) )
                  stop_idling( d->character );

               mudstrlcpy( cmdline, d->incomm, MIL );
               d->incomm[0] = '\0';

               if( d->pagepoint )
                  set_pager_input( d, cmdline );
               else
                  switch ( d->connected )
                  {
                     default:
                        nanny( d, cmdline );
                        break;
                     case CON_PLAYING:
                        if( d->original )
                           d->original->pcdata->cmd_recurse = 0;
                        else
                           d->character->pcdata->cmd_recurse = 0;
                        interpret( d->character, cmdline );
                        break;
                     case CON_EDITING:
                        edit_buffer( d->character, cmdline );
                        break;
                  }
            }
         }
         if( d == last_descriptor )
            break;
      }  /* End of descriptor input loop */

#if !defined(__CYGWIN__)
#ifdef MULTIPORT
      mud_recv_message(  );
#endif
#endif
#ifdef I3
      I3_loop(  );
#endif
#ifdef IMC
      imc_loop(  );
#endif

      /*
       * Autonomous game motion.
       */
      update_handler(  );

      /*
       * Output.
       */
      for( d = first_descriptor; d; d = d_next )
      {
         d_next = d->next;

         if( ( d->fcommand || d->outtop > 0 || d->pagetop > 0 ) && FD_ISSET( d->descriptor, &out_set ) )
         {
            if( d->pagepoint )
            {
               if( !pager_output( d ) )
               {
                  if( d->character && d->connected >= CON_PLAYING )
                     save_char_obj( d->character );
                  d->outtop = 0;
                  close_socket( d, FALSE );
               }
            }
            else if( !flush_buffer( d, TRUE ) )
            {
               if( d->character && d->connected >= CON_PLAYING )
                  save_char_obj( d->character );
               d->outtop = 0;
               close_socket( d, FALSE );
            }
         }
         if( d == last_descriptor )
            break;
      }  /* End of descriptor output loop */

      /*
       * Synchronize to a clock.
       * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
       * Careful here of signed versus unsigned arithmetic.
       */
      {
         struct timeval now_time;
         long secDelta;
         long usecDelta;

         gettimeofday( &now_time, NULL );
         usecDelta = ( ( int )last_time.tv_usec ) - ( ( int )now_time.tv_usec ) + 1000000 / sysdata.pulsepersec;
         secDelta = ( ( int )last_time.tv_sec ) - ( ( int )now_time.tv_sec );
         while( usecDelta < 0 )
         {
            usecDelta += 1000000;
            secDelta -= 1;
         }

         while( usecDelta >= 1000000 )
         {
            usecDelta -= 1000000;
            secDelta += 1;
         }

         if( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
         {
            struct timeval stall_time;

            stall_time.tv_usec = usecDelta;
            stall_time.tv_sec = secDelta;
            if( select( 0, NULL, NULL, NULL, &stall_time ) < 0 && errno != EINTR )
            {
               perror( "game_loop: select: stall" );
               exit( 1 );
            }
         }
      }

      gettimeofday( &last_time, NULL );
      current_time = ( time_t ) last_time.tv_sec;

      /*
       * Dunno if it needs to be reset, but I'll do it anyway. End of the loop here. 
       */
      set_alarm( 0 );

      /*
       * This will be the very last thing done here, because if you can't make it through
       * * one lousy loop without crashing a second time.....
       */
      sigsegv = FALSE;
   }
   /*
    * End of main game loop 
    */

   return;  /* Returns back to 'main', and will result in mud shutdown */
}

/*
 * Clean all memory on exit to help find leaks
 * Yeah I know, one big ugly function -Druid
 * Added to AFKMud by Samson on 5-8-03.
 */
void cleanup_memory( void )
{
   int hash, loopa;
#ifdef OLD_CRYPT
   char *cryptstr;
#endif
   CHAR_DATA *character;
   OBJ_DATA *object;
   DESCRIPTOR_DATA *desc, *desc_next;
   DNS_DATA *dns, *dns_next;

#ifdef IMC
   fprintf( stdout, "%s", "IMC2 Data.\n" );
   free_imcdata( TRUE );
   imc_delete_info(  );
#endif
#ifdef I3
   fprintf( stdout, "%s", "I3 Data.\n" );
   free_i3data( TRUE );
   destroy_I3_mud( this_i3mud );
#endif

   fprintf( stdout, "%s", "Quote List.\n" );
   free_quotes(  );

   fprintf( stdout, "%s", "Random Environment Data.\n" );
   free_envs(  );

   fprintf( stdout, "%s", "MXP Object Commands.\n" );
   free_mxpobj_cmds(  );

   fprintf( stdout, "%s", "Auction Sale Data.\n" );
   free_sales(  );

   fprintf( stdout, "%s", "Project Data.\n" );
   free_projects(  );

   fprintf( stdout, "%s", "Ban Data.\n" );
   free_bans(  );

   fprintf( stdout, "%s", "Auth List.\n" );
   free_all_auths(  );

   fprintf( stdout, "%s", "Morph Data.\n" );
   free_morphs(  );

   fprintf( stdout, "%s", "Rune Data.\n" );
   free_runedata(  );

   fprintf( stdout, "%s", "Connection History Data.\n" );
   free_connhistory( 0 );

   fprintf( stdout, "%s", "Slay Table.\n" );
   free_slays(  );

   fprintf( stdout, "%s", "Holidays.\n" );
   free_holidays(  );

   fprintf( stdout, "%s", "Specfun List.\n" );
   free_specfuns(  );

   fprintf( stdout, "%s", "Wizinfo Data.\n" );
   clear_wizinfo(  );

   fprintf( stdout, "%s", "Skyship landings.\n" );
   free_landings(  );

   fprintf( stdout, "%s", "Ships.\n" );
   free_ships(  );

   fprintf( stdout, "%s", "Overland Landmarks.\n" );
   free_landmarks(  );

   fprintf( stdout, "%s", "Overland Exits.\n" );
   free_mapexits(  );

   fprintf( stdout, "%s", "Mixtures and Liquids.\n" );
   free_liquiddata(  );

   fprintf( stdout, "%s", "DNS Cache data.\n" );
   for( dns = first_cache; dns; dns = dns_next )
   {
      dns_next = dns->next;
      free_dns( dns );
   }

   fprintf( stdout, "%s", "Local Channels.\n" );
   free_mudchannels(  );

   /*
    * Commands 
    */
   fprintf( stdout, "%s", "Commands.\n" );
   free_commands(  );

#ifdef MULTIPORT
   /*
    * Shell Commands 
    */
   fprintf( stdout, "%s", "Shell Commands.\n" );
   free_shellcommands(  );
#endif

   /*
    * Deities 
    */
   fprintf( stdout, "%s", "Deities.\n" );
   free_deities(  );

   /*
    * Clans 
    */
   fprintf( stdout, "%s", "Clans.\n" );
   free_clans(  );

   /*
    * socials 
    */
   fprintf( stdout, "%s", "Socials.\n" );
   free_socials(  );

   /*
    * Watches 
    */
   fprintf( stdout, "%s", "Watches.\n" );
   free_watchlist(  );

   /*
    * Helps 
    */
   fprintf( stdout, "%s", "Helps.\n" );
   free_helps(  );

   /*
    * Languages 
    */
   fprintf( stdout, "%s", "Languages.\n" );
   free_tongues(  );

   /*
    * Boards 
    */
   fprintf( stdout, "%s", "Boards.\n" );
   free_boards(  );

   /*
    * Whack supermob 
    */
   fprintf( stdout, "%s", "Whacking supermob.\n" );
   if( supermob )
   {
      char_from_room( supermob );
      UNLINK( supermob, first_char, last_char, next, prev );
      free_char( supermob );
   }

   /*
    * Free Objects 
    */
   clean_obj_queue(  );
   fprintf( stdout, "%s", "Objects.\n" );
   while( ( object = last_object ) != NULL )
      extract_obj( object );
   clean_obj_queue(  );

   /*
    * Free Characters 
    */
   clean_char_queue(  );
   fprintf( stdout, "%s", "Characters.\n" );
   while( ( character = last_char ) != NULL )
      extract_char( character, TRUE );
   clean_char_queue(  );

   /*
    * Descriptors 
    */
   fprintf( stdout, "%s", "Descriptors.\n" );
   for( desc = first_descriptor; desc; desc = desc_next )
   {
      desc_next = desc->next;
      UNLINK( desc, first_descriptor, last_descriptor, next, prev );
      free_desc( desc );
   }

   /*
    * Races 
    */
   fprintf( stdout, "%s", "Races.\n" );
   for( hash = 0; hash < MAX_RACE; hash++ )
   {
      for( loopa = 0; loopa < MAX_WHERE_NAME; loopa++ )
         STRFREE( race_table[hash]->where_name[loopa] );
      DISPOSE( race_table[hash] );
   }

   /*
    * Classes 
    */
   fprintf( stdout, "%s", "Classes.\n" );
   for( hash = 0; hash < MAX_CLASS; hash++ )
   {
      STRFREE( class_table[hash]->who_name );
      DISPOSE( class_table[hash] );
   }

   /*
    * Teleport lists 
    */
   fprintf( stdout, "%s", "Teleport Data.\n" );
   free_teleports(  );

   /*
    * Areas - this includes killing off the hash tables and such 
    */
   fprintf( stdout, "%s", "Area Data Tables.\n" );
   close_all_areas(  );

   /*
    * Get rid of auction pointer  MUST BE AFTER OBJECTS DESTROYED 
    */
   fprintf( stdout, "%s", "Auction.\n" );
   DISPOSE( auction );

   /*
    * System Data 
    */
   fprintf( stdout, "%s", "System data.\n" );
   DISPOSE( sysdata.time_of_max );
   DISPOSE( sysdata.mud_name );
   DISPOSE( sysdata.admin_email );
   DISPOSE( sysdata.password );
   DISPOSE( sysdata.telnet );
   DISPOSE( sysdata.http );

   /*
    * Title table 
    */
   fprintf( stdout, "%s", "Title table.\n" );
   for( hash = 0; hash < MAX_CLASS; hash++ )
   {
      for( loopa = 0; loopa < MAX_LEVEL + 1; loopa++ )
      {
         STRFREE( title_table[hash][loopa][0] );
         STRFREE( title_table[hash][loopa][1] );
      }
   }

   /*
    * Skills 
    */
   fprintf( stdout, "%s", "Skills and Herbs.\n" );
   free_skills(  );

   /*
    * Prog Act lists 
    */
   fprintf( stdout, "%s", "Mudprog act lists.\n" );
   free_prog_actlists(  );

   fprintf( stdout, "%s", "Abit/Qbit Data.\n" );
   free_questbits(  );

   fprintf( stdout, "%s", "Events.\n" );
   free_events(  );

   /*
    * Some freaking globals 
    */
   fprintf( stdout, "%s", "Globals.\n" );
   DISPOSE( ranged_target_name );

#ifdef OLD_CRYPT
   fprintf( stdout, "%s", "Disposing of crypt.\n" );
   cryptstr = crypt( "cleaning", "$1$Cleanup" );
   free( cryptstr );
#endif

   fprintf( stdout, "%s", "Checking string hash for leftovers.\n" );
   {
      for( hash = 0; hash < 1024; hash++ )
         hash_dump( hash );
   }

   /*
    * Last but not least, close the libdl - Samson 
    */
   dlclose( sysdata.dlHandle );
   fprintf( stdout, "%s", "Cleanup complete, exiting.\n" );
   return;
}  /* cleanup memory */

int main( int argc, char **argv )
{
   struct timeval now_time;
   int temp = -1, temp2 = -1;
   bool fCopyOver = FALSE;

   DONT_UPPER = FALSE;
   num_descriptors = 0;
   num_logins = 0;
   first_descriptor = NULL;
   last_descriptor = NULL;
   sysdata.NO_NAME_RESOLVING = TRUE;
   sysdata.WAIT_FOR_AUTH = TRUE;
   mudstrlcpy( lastplayercmd, "No commands issued yet", MIL * 2 );

   /*
    * Init time.
    */
   tzset(  );
   gettimeofday( &now_time, NULL );
   current_time = ( time_t ) now_time.tv_sec;
   mudstrlcpy( str_boot_time, c_time( current_time, -1 ), MIL );  /* Records when the mud was last rebooted */

   new_pfile_time_t = current_time + 86400;

   /*
    * Get the port number.
    */
   port = 9500;
   if( argc > 1 )
   {
      if( !is_number( argv[1] ) )
      {
         fprintf( stderr, "Usage: %s [port #]\n", argv[0] );
         exit( 1 );
      }
      else if( ( port = atoi( argv[1] ) ) <= 1024 )
      {
         fprintf( stderr, "%s", "Port number must be above 1024.\n" );
         exit( 1 );
      }

      if( argv[2] && argv[2][0] )
      {
         fCopyOver = TRUE;
         control = atoi( argv[3] );
#ifdef I3
         I3_socket = atoi( argv[4] );
#endif
#ifdef WEBSVR
         temp = atoi( argv[5] );
#endif
#ifdef IMC
         temp2 = atoi( argv[6] );
#endif
      }
      else
         fCopyOver = FALSE;
   }

   /*
    * Initialize all startup functions of the mud. 
    */
   init_mud( fCopyOver, port, temp, temp2 );

   /*
    * Set various signal traps, waiting until after completing all bootup operations
    * * before doing so because crashes during bootup should not be intercepted. Samson 3-11-04
    */
   signal( SIGPIPE, SIG_IGN );
   signal( SIGALRM, caught_alarm );
   signal( SIGTERM, SigTerm );   /* Catch kill signals */
   signal( SIGUSR1, SigUser1 );  /* Catch user defined signals */
   signal( SIGUSR2, SigUser2 );
#ifdef MULTIPORT
   signal( SIGCHLD, SigChld );
#endif

   /*
    * If this setting is active, intercept SIGSEGV and keep the mud running.
    * * Doing so sets a flag variable which if TRUE will cause SegVio to abort()
    * * If game_loop is restarted and makes it through once without crashing again,
    * * then the flag is unset and SIGSEGV will continue to be intercepted. Samson 3-11-04
    */
   if( sysdata.crashhandler == TRUE )
      signal( SIGSEGV, SegVio );

   /*
    * Sick isn't it? The whole game being run inside of one little statement..... :P 
    */
   game_loop(  );

   /*
    * Clean up the loose ends. 
    */
   close_mud(  );

   /*
    * That's all, folks.
    */
   log_string( "Normal termination of game." );
   log_string( "Cleaning up Memory.&d" );
   cleanup_memory(  );
   exit( 0 );
}

#define NAME(ch)        ( IS_NPC(ch) ? ch->short_descr : ch->name )

char *MORPHNAME( CHAR_DATA *ch )
{
   if( ch->morph && ch->morph->morph && ch->morph->morph->short_desc != NULL )
      return ch->morph->morph->short_desc;
   else
      return NAME(ch);
}

char *act_string( const char *format, CHAR_DATA * to, CHAR_DATA * ch, void *arg1, void *arg2, int flags )
{
   static char *const he_she[] = { "it", "he", "she", "it" };
   static char *const him_her[] = { "it", "him", "her", "it" };
   static char *const his_her[] = { "its", "his", "her", "its" };
   static char buf[MSL];
   char fname[MIL], temp[MSL];
   char *point = buf;
   const char *str = format;
   const char *i;
   CHAR_DATA *vch = ( CHAR_DATA * ) arg2;
   OBJ_DATA *obj1 = ( OBJ_DATA * ) arg1;
   OBJ_DATA *obj2 = ( OBJ_DATA * ) arg2;

   if( !str )
   {
      bug( "%s", "act_string: NULL str!" );
      return "";
   }

   if( str[0] == '$' )
      DONT_UPPER = FALSE;

   while( *str != '\0' )
   {
      if( *str != '$' )
      {
         *point++ = *str++;
         continue;
      }
      ++str;
      if( !arg2 && *str >= 'A' && *str <= 'Z' )
      {
         bug( "Act: missing arg2 for code %c:", *str );
         bug( "Act: Missing arg2 came from %s", ch->name );
         if( IS_NPC( ch ) )
            bug( "Act: NPC vnum: %d", ch->pIndexData->vnum );
         bug( "%s", format );
         i = " <@@@> ";
      }
      else
      {
         switch ( *str )
         {
            default:
               bug( "Act: bad code %c.", *str );
               bug( "Act: Bad code came from %s", ch->name );
               i = " <@@@> ";
               break;

#ifdef I3
            case '$':
               i = "$";
               break;
#endif

            case 't':
               if( arg1 != NULL )
                  i = ( char * )arg1;
               else
               {
                  bug( "%s: bad $t.", __FUNCTION__ );
                  i = " <@@@> ";
               }
               break;

            case 'T':
               if( arg2 != NULL )
                  i = ( char * )arg2;
               else
               {
                  bug( "%s: bad $T.", __FUNCTION__ );
                  i = " <@@@> ";
               }
               break;

            case 'n':
               if( ch->morph == NULL )
                  i = ( to ? PERS( ch, to, FALSE ) : NAME( ch ) );
               else if( !IS_SET( flags, STRING_IMM ) )
                  i = ( to ? MORPHPERS( ch, to, FALSE ) : MORPHNAME( ch ) );
               else
               {
                  snprintf( temp, MSL, "(%s) %s", ( to ? PERS( ch, to, FALSE ) : NAME( ch ) ),
                            ( to ? MORPHPERS( ch, to, FALSE ) : MORPHNAME( ch ) ) );
                  i = temp;
               }
               break;

            case 'N':
               if( vch->morph == NULL )
                  i = ( to ? PERS( vch, to, FALSE ) : NAME( vch ) );
               else if( !IS_SET( flags, STRING_IMM ) )
                  i = ( to ? MORPHPERS( vch, to, FALSE ) : MORPHNAME( vch ) );
               else
               {
                  snprintf( temp, MSL, "(%s) %s", ( to ? PERS( vch, to, FALSE ) : NAME( vch ) ),
                            ( to ? MORPHPERS( vch, to, FALSE ) : MORPHNAME( vch ) ) );
                  i = temp;
               }
               break;

            case 'e':
               if( ch->sex > SEX_HERMAPHRODYTE || ch->sex < 0 )
               {
                  bug( "act_string: player %s has sex set at %d!", ch->name, ch->sex );
                  i = "it";
               }
               else
                  i = he_she[URANGE( 0, ch->sex, 2 )];
               break;

            case 'E':
               if( vch->sex > SEX_HERMAPHRODYTE || vch->sex < 0 )
               {
                  bug( "act_string: player %s has sex set at %d!", vch->name, vch->sex );
                  i = "it";
               }
               else
                  i = he_she[URANGE( 0, vch->sex, 2 )];
               break;

            case 'm':
               if( ch->sex > SEX_HERMAPHRODYTE || ch->sex < 0 )
               {
                  bug( "act_string: player %s has sex set at %d!", ch->name, ch->sex );
                  i = "it";
               }
               else
                  i = him_her[URANGE( 0, ch->sex, 2 )];
               break;

            case 'M':
               if( vch->sex > SEX_HERMAPHRODYTE || vch->sex < 0 )
               {
                  bug( "act_string: player %s has sex set at %d!", vch->name, vch->sex );
                  i = "it";
               }
               else
                  i = him_her[URANGE( 0, vch->sex, 2 )];
               break;

            case 's':
               if( ch->sex > SEX_HERMAPHRODYTE || ch->sex < 0 )
               {
                  bug( "act_string: player %s has sex set at %d!", ch->name, ch->sex );
                  i = "its";
               }
               else
                  i = his_her[URANGE( 0, ch->sex, 2 )];
               break;

            case 'S':
               if( vch->sex > SEX_HERMAPHRODYTE || vch->sex < 0 )
               {
                  bug( "act_string: player %s has sex set at %d!", vch->name, vch->sex );
                  i = "its";
               }
               else
                  i = his_her[URANGE( 0, vch->sex, 2 )];
               break;

            case 'q':
               i = ( to == ch ) ? "" : "s";
               break;

            case 'Q':
               i = ( to == ch ) ? "your" : his_her[URANGE( 0, ch->sex, 2 )];
               break;

            case 'p':
               i = ( !obj1 ? "<BUG>" : ( !to || can_see_obj( to, obj1, FALSE ) ? obj_short( obj1 ) : "something" ) );
               break;

            case 'P':
               i = ( !obj2 ? "<BUG>" : ( !to || can_see_obj( to, obj2, FALSE ) ? obj_short( obj2 ) : "something" ) );
               break;

            case 'd':
               if( !arg2 || ( ( char * )arg2 )[0] == '\0' )
                  i = "door";
               else
               {
                  one_argument( ( char * )arg2, fname );
                  i = fname;
               }
               break;
         }
      }
      ++str;
      while( ( *point = *i ) != '\0' )
         ++point, ++i;
   }
   mudstrlcpy( point, "\n\r", MSL - ( point - buf ) );
   if( !DONT_UPPER )
      buf[0] = UPPER( buf[0] );
   return buf;
}

#undef NAME

void act_printf( short AType, CHAR_DATA * ch, void *arg1, void *arg2, int type, const char *str, ... )
{
   va_list arg;
   char format[MSL * 2];

   /*
    * Discard null and zero-length messages.
    */
   if( !str || str[0] == '\0' )
      return;

   va_start( arg, str );
   vsnprintf( format, MSL * 2, str, arg );
   va_end( arg );

   act( AType, format, ch, arg1, arg2, type );
}

void act( short AType, const char *format, CHAR_DATA * ch, void *arg1, void *arg2, int type )
{
   char *txt;
   CHAR_DATA *to;
   CHAR_DATA *third = ( CHAR_DATA * ) arg1;
   CHAR_DATA *vch = ( CHAR_DATA * ) arg2;

   /*
    * Discard null and zero-length messages.
    */
   if( !format || format[0] == '\0' )
      return;

   if( !ch )
   {
      bug( "Act: null ch. (%s)", format );
      return;
   }

   if( !ch->in_room )
      to = NULL;
   else if( type == TO_CHAR )
      to = ch;
   else if( type == TO_THIRD )
      to = third;
   else
      to = ch->in_room->first_person;

   /*
    * ACT_SECRETIVE handling
    */
   if( IS_ACT_FLAG( ch, ACT_SECRETIVE ) && type != TO_CHAR )
      return;

   if( type == TO_VICT )
   {
      if( !vch )
      {
         bug( "%s", "Act: null vch with TO_VICT." );
         bug( "%s (%s)", ch->name, format );
         return;
      }
      if( !vch->in_room )
      {
         bug( "%s", "Act: vch in NULL room!" );
         bug( "%s -> %s (%s)", ch->name, vch->name, format );
         return;
      }

      if( is_ignoring( ch, vch ) )
      {
         /*
          * continue unless speaker is an immortal 
          */
         if( !IS_IMMORTAL( vch ) || ch->level > vch->level )
            return;
         else
            ch_printf( ch, "&[ignore]You attempt to ignore %s, but are unable to do so.\n\r", vch->name );
      }
      to = vch;
   }

   if( MOBtrigger && type != TO_CHAR && type != TO_VICT && to )
   {
      OBJ_DATA *to_obj;

      txt = act_string( format, NULL, ch, arg1, arg2, STRING_IMM );
      if( HAS_PROG( to->in_room, ACT_PROG ) )
         rprog_act_trigger( txt, to->in_room, ch, ( OBJ_DATA * ) arg1, ( void * )arg2 );
      for( to_obj = to->in_room->first_content; to_obj; to_obj = to_obj->next_content )
         if( HAS_PROG( to_obj->pIndexData, ACT_PROG ) )
            oprog_act_trigger( txt, to_obj, ch, ( OBJ_DATA * ) arg1, ( void * )arg2 );
   }

   /*
    * Anyone feel like telling me the point of looping through the whole
    * room when we're only sending to one char anyways..? -- Alty 
    */
   /*
    * Because, silly, now we can use this sweet little bit of code to make
    * sure that messages to people on the maps go where they need to :P - Samson 
    */
   for( ; to; to = ( type == TO_CHAR || type == TO_VICT || type == TO_THIRD ) ? NULL : to->next_in_room )
   {
      if( ( !to->desc && ( IS_NPC( to ) && !HAS_PROG( to->pIndexData, ACT_PROG ) ) ) || !IS_AWAKE( to ) )
         continue;

      /*
       * OasisOLC II check - Tagith 
       */
      if( to->desc && is_inolc( to->desc ) )
         continue;

      if( type == TO_CHAR )
      {
         if( to != ch )
            continue;

         if( !is_same_map( ch, to ) )
            continue;
      }

      if( type == TO_THIRD && to != third )
         continue;

      if( type == TO_VICT && ( to != vch || to == ch ) )
         continue;

      if( type == TO_ROOM )
      {
         if( to == ch )
            continue;

         if( is_ignoring( ch, to ) )
            continue;

         if( !is_same_map( ch, to ) )
            continue;
      }

      if( type == TO_NOTVICT )
      {
         if( to == ch || to == vch )
            continue;

         if( is_ignoring( ch, to ) )
            continue;

         if( vch != NULL && is_ignoring( vch, to ) )
            continue;

         if( !is_same_map( ch, to ) )
            continue;
      }

      if( type == TO_CANSEE )
      {
         if( to == ch )
            continue;

         if( IS_IMMORTAL( ch ) && IS_PLR_FLAG( ch, PLR_WIZINVIS ) )
         {
            if( to->level < ch->pcdata->wizinvis )
               continue;
         }

         if( is_ignoring( ch, to ) )
            continue;

         if( !is_same_map( ch, to ) )
            continue;
      }

      if( IS_IMMORTAL( to ) )
         txt = act_string( format, to, ch, arg1, arg2, STRING_IMM );
      else
         txt = act_string( format, to, ch, arg1, arg2, STRING_NONE );

      if( to->desc )
      {
         set_char_color( AType, to );
         send_to_char( txt, to );
      }
      if( MOBtrigger )
      {
         /*
          * Note: use original string, not string with ANSI. -- Alty 
          */
         mprog_act_trigger( txt, to, ch, ( OBJ_DATA * ) arg1, ( void * )arg2 );
      }
   }
   MOBtrigger = TRUE;
   return;
}
