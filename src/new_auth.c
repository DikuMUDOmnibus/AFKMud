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
 *                      New Name Authorization module                       *
 ****************************************************************************/

/*
 *  New name authorization system
 *  Author: Rantic (supfly@geocities.com)
 *  of FrozenMUD (empire.digiunix.net 4000)
 *
 *  Permission to use and distribute this code is granted provided
 *  this header is retained and unaltered, and the distribution
 *  package contains all the original files unmodified.
 *  If you modify this code and use/distribute modified versions
 *  you must give credit to the original author(s).
 */

#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "mud.h"
#include "new_auth.h"

CHAR_DATA *get_waiting_desc( CHAR_DATA * ch, char *name );
CMDF do_reserve( CHAR_DATA * ch, char *argument );
CMDF do_destroy( CHAR_DATA * ch, char *argument );

AUTH_LIST *first_auth_name;
AUTH_LIST *last_auth_name;

void name_generator( char *argument )
{
   int start_counter = 0;
   int middle_counter = 0;
   int end_counter = 0;
   char start_string[100][10];
   char middle_string[100][10];
   char end_string[100][10];
   char tempstring[151];
   struct timeval starttime;
   time_t t;
   char name[300];
   FILE *infile;

   tempstring[0] = '\0';

   infile = fopen( NAMEGEN_FILE, "r" );
   if( infile == NULL )
   {
      log_string( "Can't find NAMEGEN file." );
      return;
   }

   fgets( tempstring, 150, infile );
   tempstring[strlen( tempstring ) - 1] = '\0';
   while( str_cmp( tempstring, "[start]" ) != 0 )
   {
      fgets( tempstring, 150, infile );
      tempstring[strlen( tempstring ) - 1] = '\0'; /* remove linefeed          */
   }

   while( str_cmp( tempstring, "[middle]" ) != 0 )
   {
      fgets( tempstring, 150, infile );
      tempstring[strlen( tempstring ) - 1] = '\0'; /* remove linefeed          */
      if( tempstring[0] != '/' )
         mudstrlcpy( start_string[start_counter++], tempstring, 100 );
   }
   while( str_cmp( tempstring, "[end]" ) != 0 )
   {
      fgets( tempstring, 150, infile );
      tempstring[strlen( tempstring ) - 1] = '\0'; /* remove linefeed          */
      if( tempstring[0] != '/' )
         mudstrlcpy( middle_string[middle_counter++], tempstring, 100 );
   }
   while( str_cmp( tempstring, "[finish]" ) != 0 )
   {
      fgets( tempstring, 150, infile );
      tempstring[strlen( tempstring ) - 1] = '\0'; /* remove linefeed          */
      if( tempstring[0] != '/' )
         mudstrlcpy( end_string[end_counter++], tempstring, 100 );
   }
   FCLOSE( infile );
   gettimeofday( &starttime, NULL );
   srand( ( unsigned )time( &t ) + starttime.tv_usec );
   start_counter--;
   middle_counter--;
   end_counter--;

   mudstrlcpy( name, start_string[rand(  ) % start_counter], 300 );  /* get a start                  */
   mudstrlcat( name, middle_string[rand(  ) % middle_counter], 300 );   /* get a middle                 */
   mudstrlcat( name, end_string[rand(  ) % end_counter], 300 );   /* get an ending                */
   mudstrlcat( argument, name, 300 );
   return;
}

CMDF do_name_generator( CHAR_DATA * ch, char *argument )
{
   char name[300];

   name[0] = '\0';

   name_generator( name );
   send_to_char( name, ch );
   return;
}

/* Added by Tarl 5 Dec 02 to allow picking names from a file. Used for the namegen
   code in reset.c */
void pick_name( char *argument, char *filename )
{
   struct timeval starttime;
   time_t t;
   char name[200];
   char tempstring[151];
   int counter = 0;
   FILE *infile;
   char names[200][20];

   tempstring[0] = '\0';

   infile = fopen( filename, "r" );
   if( infile == NULL )
   {
      log_printf( "Can't find %s", filename );
      return;
   }

   fgets( tempstring, 150, infile );
   tempstring[strlen( tempstring ) - 1] = '\0';
   while( str_cmp( tempstring, "[start]" ) != 0 )
   {
      fgets( tempstring, 100, infile );
      tempstring[strlen( tempstring ) - 1] = '\0'; /* remove linefeed */
   }
   while( str_cmp( tempstring, "[finish]" ) != 0 )
   {
      fgets( tempstring, 100, infile );
      tempstring[strlen( tempstring ) - 1] = '\0';
      if( tempstring[0] != '/' )
         mudstrlcpy( names[counter++], tempstring, 200 );
   }
   FCLOSE( infile );
   gettimeofday( &starttime, NULL );
   srand( ( unsigned )time( &t ) + starttime.tv_usec );
   counter--;

   mudstrlcpy( name, names[rand(  ) % counter], 200 );
   mudstrlcat( argument, name, 200 );
   return;
}

bool exists_player( char *name )
{
   struct stat fst;
   char buf[256];
   CHAR_DATA *victim = NULL;

   /*
    * Stands to reason that if there ain't a name to look at, they damn well don't exist! 
    */
   if( !name || !str_cmp( name, "" ) )
      return FALSE;

   snprintf( buf, 256, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ) );

   if( stat( buf, &fst ) != -1 )
      return TRUE;

   else if( ( victim = get_char_world( supermob, name ) ) != NULL )
      return TRUE;

   return FALSE;
}

void free_auth_entry( AUTH_LIST * auth )
{
   UNLINK( auth, first_auth_name, last_auth_name, next, prev );
   STRFREE( auth->authed_by );
   STRFREE( auth->change_by );
   STRFREE( auth->name );
   DISPOSE( auth );
}

void free_all_auths( void )
{
   AUTH_LIST *auth, *auth_next;

   for( auth = first_auth_name; auth; auth = auth_next )
   {
      auth_next = auth->next;
      free_auth_entry( auth );
   }
   return;
}

void clean_auth_list( void )
{
   AUTH_LIST *auth, *nauth;

   for( auth = first_auth_name; auth; auth = nauth )
   {
      nauth = auth->next;

      if( !exists_player( auth->name ) )
         free_auth_entry( auth );
      else
      {
         time_t tdiff = 0;
         time_t curr_time = time( 0 );
         struct stat fst;
         char file[256], name[MSL];
         int MAX_AUTH_WAIT = 7;

         mudstrlcpy( name, auth->name, MSL );
         snprintf( file, 256, "%s%c/%s", PLAYER_DIR, LOWER( auth->name[0] ), capitalize( auth->name ) );

         if( stat( file, &fst ) != -1 )
            tdiff = ( curr_time - fst.st_mtime ) / 86400;
         else
            bug( "File %s does not exist!", file );

         if( tdiff > MAX_AUTH_WAIT )
         {
            if( unlink( file ) == -1 )
               perror( "Unlink: do_auth: \"clean\"" );
            else
               log_printf( "%s deleted for inactivity: %ld days", file, ( long int )tdiff );
         }
      }
   }
}

void write_auth_file( FILE * fpout, AUTH_LIST * list )
{
   fprintf( fpout, "Name		%s~\n", list->name );
   fprintf( fpout, "State		%d\n", list->state );
   if( list->authed_by )
      fprintf( fpout, "AuthedBy       %s~\n", list->authed_by );
   if( list->change_by )
      fprintf( fpout, "Change		%s~\n", list->change_by );
   fprintf( fpout, "%s", "End\n\n" );
}

void fread_auth( FILE * fp )
{
   AUTH_LIST *new_auth;
   bool fMatch;
   const char *word;

   CREATE( new_auth, AUTH_LIST, 1 );

   new_auth->authed_by = NULL;
   new_auth->change_by = NULL;

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
            KEY( "AuthedBy", new_auth->authed_by, fread_string( fp ) );
            break;

         case 'C':
            KEY( "Change", new_auth->change_by, fread_string( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               LINK( new_auth, first_auth_name, last_auth_name, next, prev );
               return;
            }
            break;

         case 'N':
            KEY( "Name", new_auth->name, fread_string( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "State" ) )
            {
               new_auth->state = fread_number( fp );
               if( new_auth->state == AUTH_ONLINE || new_auth->state == AUTH_LINK_DEAD )
                  /*
                   * Crash proofing. Can't be online when  booting up. Would suck for do_auth 
                   */
                  new_auth->state = AUTH_OFFLINE;
               fMatch = TRUE;
               break;
            }
            break;
      }
      if( !fMatch )
         bug( "Fread_auth: no match: %s", word );
   }
}

void save_auth_list( void )
{
   FILE *fpout;
   AUTH_LIST *list;

   if( ( fpout = fopen( AUTH_FILE, "w" ) ) == NULL )
   {
      bug( "%s", "Cannot open auth.dat for writing." );
      perror( AUTH_FILE );
      return;
   }

   for( list = first_auth_name; list; list = list->next )
   {
      fprintf( fpout, "%s", "#AUTH\n" );
      write_auth_file( fpout, list );
   }

   fprintf( fpout, "%s", "#END\n" );
   FCLOSE( fpout );
}

void clear_auth_list( void )
{
   AUTH_LIST *auth, *nauth;

   for( auth = first_auth_name; auth; auth = nauth )
   {
      nauth = auth->next;

      if( !exists_player( auth->name ) )
         free_auth_entry( auth );
   }
   save_auth_list(  );
}

void load_auth_list( void )
{
   FILE *fp;
   int x;

   first_auth_name = last_auth_name = NULL;

   if( ( fp = fopen( AUTH_FILE, "r" ) ) != NULL )
   {
      x = 0;
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
            bug( "%s", "Load_auth_list: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "AUTH" ) )
         {
            fread_auth( fp );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s", "load_auth_list: bad section." );
            continue;
         }
      }
      FCLOSE( fp );
   }
   else
   {
      bug( "%s", "Cannot open auth.dat" );
      return;
   }
   clear_auth_list(  );
}

int get_auth_state( CHAR_DATA * ch )
{
   AUTH_LIST *namestate;
   int state;

   state = AUTH_AUTHED;

   for( namestate = first_auth_name; namestate; namestate = namestate->next )
   {
      if( !str_cmp( namestate->name, ch->name ) )
      {
         state = namestate->state;
         break;
      }
   }
   return state;
}

AUTH_LIST *get_auth_name( char *name )
{
   AUTH_LIST *mname;

   if( last_auth_name && last_auth_name->next != NULL )
      bug( "Last_auth_name->next != NULL: %s", last_auth_name->next->name );

   for( mname = first_auth_name; mname; mname = mname->next )
   {
      if( !str_cmp( mname->name, name ) ) /* If the name is already in the list, break */
         break;
   }
   return mname;
}

void add_to_auth( CHAR_DATA * ch )
{
   AUTH_LIST *new_name;

   new_name = get_auth_name( ch->name );
   if( new_name != NULL )
      return;
   else
   {
      CREATE( new_name, AUTH_LIST, 1 );
      new_name->name = STRALLOC( ch->name );
      new_name->state = AUTH_ONLINE;   /* Just entered the game */
      LINK( new_name, first_auth_name, last_auth_name, next, prev );
      save_auth_list(  );
   }
}

void remove_from_auth( char *name )
{
   AUTH_LIST *old_name;

   old_name = get_auth_name( name );
   if( old_name == NULL )  /* Its not old */
      return;
   else
   {
      free_auth_entry( old_name );
      save_auth_list(  );
   }
}

void check_auth_state( CHAR_DATA * ch )
{
   AUTH_LIST *old_auth;
   CMDTYPE *command;
   int level = LEVEL_IMMORTAL;

   command = find_command( "authorize" );
   if( !command )
      level = LEVEL_IMMORTAL;
   else
      level = command->level;

   old_auth = get_auth_name( ch->name );
   if( old_auth == NULL )
      return;

   if( old_auth->state == AUTH_OFFLINE || old_auth->state == AUTH_LINK_DEAD )
   {
      old_auth->state = AUTH_ONLINE;
      save_auth_list(  );
   }
   else if( old_auth->state == AUTH_CHANGE_NAME )
   {
      ch_printf( ch,
                 "&R\n\rThe MUD Administrators have found the name %s\n\r"
                 "to be unacceptable. You must choose a new one.\n\r"
                 "The name you choose must be medieval and original.\n\r"
                 "No titles, descriptive words, or names close to any existing\n\r"
                 "Immortal's name. See 'help name'.\n\r", ch->name );
   }
   else if( old_auth->state == AUTH_AUTHED )
   {
      STRFREE( ch->pcdata->authed_by );
      if( old_auth->authed_by )
      {
         ch->pcdata->authed_by = QUICKLINK( old_auth->authed_by );
         STRFREE( old_auth->authed_by );
      }
      else
         ch->pcdata->authed_by = STRALLOC( "The Code" );

      ch_printf( ch,
                 "\n\r&GThe MUD Administrators have accepted the name %s.\n\rYou are now free to roam %s.\n\r", ch->name,
                 sysdata.mud_name );
      REMOVE_PCFLAG( ch, PCFLAG_UNAUTHED );
      remove_from_auth( ch->name );
      return;
   }
   return;
}

/* 
 * Check if the name prefix uniquely identifies a char descriptor
 */
CHAR_DATA *get_waiting_desc( CHAR_DATA * ch, char *name )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *ret_char = NULL;
   static unsigned int number_of_hits;

   number_of_hits = 0;
   for( d = first_descriptor; d; d = d->next )
   {
      if( d->character && ( !str_prefix( name, d->character->name ) ) && IS_WAITING_FOR_AUTH( d->character ) )
      {
         if( ++number_of_hits > 1 )
         {
            ch_printf( ch, "%s does not uniquely identify a char.\n\r", name );
            return NULL;
         }
         ret_char = d->character;   /* return current char on exit */
      }
   }
   if( number_of_hits == 1 )
      return ret_char;
   else
   {
      send_to_char( "No one like that waiting for authorization.\n\r", ch );
      return NULL;
   }
}

/* new auth */
CMDF do_authorize( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   CHAR_DATA *victim = NULL;
   AUTH_LIST *auth;
   CMDTYPE *command;
   int level = LEVEL_IMMORTAL;
   bool offline, authed, changename, pending;

   offline = authed = changename = pending = FALSE;
   auth = NULL;

   /*
    * Checks level of authorize command, for log messages. - Samson 10-18-98 
    */
   command = find_command( "authorize" );
   if( !command )
      level = LEVEL_IMMORTAL;
   else
      level = command->level;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "To approve a waiting character: auth <name>\n\r", ch );
      send_to_char( "To deny a waiting character:    auth <name> reject\n\r", ch );
      send_to_char( "To ask a waiting character to change names: auth <name> change\n\r", ch );
      send_to_char( "To have the code verify the list: auth fixlist\n\r", ch );
      send_to_char( "To have the code purge inactive entries: auth clean\n\r", ch );

      send_to_char( "\n\r&[divider]--- Characters awaiting approval ---\n\r", ch );

      for( auth = first_auth_name; auth; auth = auth->next )
      {
         if( auth->state == AUTH_CHANGE_NAME )
            changename = TRUE;
         else if( auth->state == AUTH_AUTHED )
            authed = TRUE;

         if( auth->name != NULL && auth->state < AUTH_CHANGE_NAME )
            pending = TRUE;
      }
      if( pending )
      {
         for( auth = first_auth_name; auth; auth = auth->next )
         {
            if( auth->state < AUTH_CHANGE_NAME )
            {
               switch ( auth->state )
               {
                  default:
                     ch_printf( ch, "\t%s\t\tUnknown?\n\r", auth->name );
                     break;
                  case AUTH_LINK_DEAD:
                     ch_printf( ch, "\t%s\t\tLink Dead\n\r", auth->name );
                     break;
                  case AUTH_ONLINE:
                     ch_printf( ch, "\t%s\t\tOnline\n\r", auth->name );
                     break;
                  case AUTH_OFFLINE:
                     ch_printf( ch, "\t%s\t\tOffline\n\r", auth->name );
                     break;
               }
            }
         }
      }
      else
         send_to_char( "\tNone\n\r", ch );

      if( authed )
      {
         send_to_char( "\n\r&[divider]Authorized Characters:\n\r", ch );
         send_to_char( "---------------------------------------------\n\r", ch );
         for( auth = first_auth_name; auth; auth = auth->next )
         {
            if( auth->state == AUTH_AUTHED )
               ch_printf( ch, "Name: %s\t Approved by: %s\n\r", auth->name, auth->authed_by );
         }
      }
      if( changename )
      {
         send_to_char( "\n\r&[divider]Change Name:\n\r", ch );
         send_to_char( "---------------------------------------------\n\r", ch );
         for( auth = first_auth_name; auth; auth = auth->next )
         {
            if( auth->state == AUTH_CHANGE_NAME )
               ch_printf( ch, "Name: %s\t Change requested by: %s\n\r", auth->name, auth->change_by );
         }
      }
      return;
   }

   if( !str_cmp( arg1, "fixlist" ) )
   {
      send_to_pager( "Checking authorization list...\n\r", ch );
      clear_auth_list(  );
      send_to_pager( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg1, "clean" ) )
   {
      send_to_pager( "Cleaning authorization list...\n\r", ch );
      clean_auth_list(  );
      send_to_pager( "Checking authorization list...\n\r", ch );
      clear_auth_list(  );
      send_to_pager( "Done.\n\r", ch );
      return;
   }

   auth = get_auth_name( arg1 );
   if( auth != NULL )
   {
      if( auth->state == AUTH_OFFLINE || auth->state == AUTH_LINK_DEAD )
      {
         offline = TRUE;
         if( !argument || argument[0] == '\0' || !str_cmp( argument, "accept" ) || !str_cmp( argument, "yes" ) )
         {
            auth->state = AUTH_AUTHED;
            auth->authed_by = QUICKLINK( ch->name );
            save_auth_list(  );
            log_printf_plus( LOG_AUTH, level, "%s: authorized", auth->name );
            ch_printf( ch, "You have authorized %s.\n\r", auth->name );
            return;
         }
         else if( !str_cmp( argument, "reject" ) )
         {
            log_printf_plus( LOG_AUTH, level, "%s: denied authorization", auth->name );
            ch_printf( ch, "You have denied %s.\n\r", auth->name );
            /*
             * Addition so that denied names get added to reserved list - Samson 10-18-98 
             */
            funcf( ch, do_reserve, "%s add", auth->name );
            do_destroy( ch, auth->name );
            return;
         }
         else if( !str_cmp( argument, "change" ) )
         {
            auth->state = AUTH_CHANGE_NAME;
            auth->change_by = QUICKLINK( ch->name );
            save_auth_list(  );
            log_printf_plus( LOG_AUTH, level, "%s: name denied", auth->name );
            ch_printf( ch, "You requested %s change names.\n\r", auth->name );
            /*
             * Addition so that requested name changes get added to reserved list - Samson 10-18-98 
             */
            funcf( ch, do_reserve, "%s add", auth->name );
            return;
         }
         else
         {
            send_to_char( "Invalid argument.\n\r", ch );
            return;
         }
      }
      else
      {
         victim = get_waiting_desc( ch, arg1 );
         if( victim == NULL )
            return;

         set_char_color( AT_IMMORT, victim );
         if( !argument || argument[0] == '\0' || !str_cmp( argument, "accept" ) || !str_cmp( argument, "yes" ) )
         {
            STRFREE( victim->pcdata->authed_by );
            victim->pcdata->authed_by = QUICKLINK( ch->name );
            log_printf_plus( LOG_AUTH, level, "%s: authorized", victim->name );

            ch_printf( ch, "You have authorized %s.\n\r", victim->name );

            ch_printf( victim,
                       "\n\r&GThe MUD Administrators have accepted the name %s.\n\r"
                       "You are now free to roam the %s.\n\r", victim->name, sysdata.mud_name );
            REMOVE_PCFLAG( victim, PCFLAG_UNAUTHED );
            remove_from_auth( victim->name );
            return;
         }
         else if( !str_cmp( argument, "reject" ) )
         {
            send_to_char( "&RYou have been denied access.\n\r", victim );
            log_printf_plus( LOG_AUTH, level, "%s: denied authorization", victim->name );
            ch_printf( ch, "You have denied %s.\n\r", victim->name );
            remove_from_auth( victim->name );
            /*
             * Addition to add denied names to reserved list - Samson 10-18-98 
             */
            funcf( ch, do_reserve, "%s add", victim->name );
            do_destroy( ch, victim->name );
            return;
         }
         else if( !str_cmp( argument, "change" ) )
         {
            auth->state = AUTH_CHANGE_NAME;
            auth->change_by = QUICKLINK( ch->name );
            save_auth_list(  );
            log_printf_plus( LOG_AUTH, level, "%s: name denied", victim->name );
            ch_printf( victim,
                       "&R\n\rThe MUD Administrators have found the name %s to be unacceptable.\n\r"
                       "You may choose a new name when you reach the end of this area.\n\r"
                       "The name you choose must be medieval and original.\n\r"
                       "No titles, descriptive words, or names close to any existing\n\r"
                       "Immortal's name. See 'help name'.\n\r", victim->name );
            ch_printf( ch, "You requested %s change names.\n\r", victim->name );
            /*
             * Addition to put denied name on reserved list - Samson 10-18-98 
             */
            funcf( ch, do_reserve, "%s add", victim->name );
            return;
         }
         else
         {
            send_to_char( "Invalid argument.\n\r", ch );
            return;
         }
      }
   }
   else
   {
      send_to_char( "No such player pending authorization.\n\r", ch );
      return;
   }
}

/* new auth */
CMDF do_name( CHAR_DATA * ch, char *argument )
{
   char fname[256];
   struct stat fst;
   CHAR_DATA *tmp;
   AUTH_LIST *auth_name;

   auth_name = NULL;
   auth_name = get_auth_name( ch->name );
   if( auth_name == NULL )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument[0] = UPPER( argument[0] );

   if( !check_parse_name( argument, TRUE ) )
   {
      send_to_char( "Illegal name, try another.\n\r", ch );
      return;
   }

   if( !str_cmp( ch->name, argument ) )
   {
      send_to_char( "That's already your name!\n\r", ch );
      return;
   }

   for( tmp = first_char; tmp; tmp = tmp->next )
   {
      if( !str_cmp( argument, tmp->name ) )
         break;
   }

   if( tmp )
   {
      send_to_char( "That name is already taken. Please choose another.\n\r", ch );
      return;
   }

   snprintf( fname, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ) );
   if( stat( fname, &fst ) != -1 )
   {
      send_to_char( "That name is already taken. Please choose another.\n\r", ch );
      return;
   }
   snprintf( fname, 256, "%s%c/%s", PLAYER_DIR, tolower( ch->name[0] ), capitalize( ch->name ) );
   unlink( fname );  /* cronel, for auth */

   STRFREE( ch->name );
   ch->name = STRALLOC( argument );
   STRFREE( ch->pcdata->filename );
   ch->pcdata->filename = STRALLOC( argument );
   send_to_char( "Your name has been changed and is being submitted for approval.\n\r", ch );
   STRFREE( auth_name->name );
   auth_name->name = STRALLOC( argument );
   auth_name->state = AUTH_ONLINE;
   STRFREE( auth_name->change_by );
   save_auth_list(  );
   return;
}

/* changed for new auth */
CMDF do_mpapplyb( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   CMDTYPE *command;
   int level = LEVEL_IMMORTAL;

   /*
    * Checks to see level of authorize command.
    * Makes no sense to see the auth channel if you can't auth. - Samson 12-28-98 
    */
   command = find_command( "authorize" );

   if( !command )
      level = LEVEL_IMMORTAL;
   else
      level = command->level;

   if( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpapplyb - bad syntax" );
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      progbugf( ch, "Mpapplyb - no such player %s in room.", argument );
      return;
   }

   if( !victim->desc )
   {
      progbugf( ch, "Mpapplyb - linkdead target %s.", victim->name );
      return;
   }

   if( victim->fighting )
      stop_fighting( victim, TRUE );
   if( NOT_AUTHED( victim ) )
   {
      log_printf_plus( LOG_AUTH, level, "%s [%s] New player entering the game.\n\r", victim->name, victim->desc->host );
      ch_printf( victim, "\n\rYou are now entering the game...\n\r"
                 "However, your character has not been authorized yet and can not\n\r"
                 "advance past level 5 until then. Your character will be saved,\n\r"
                 "but not allowed to fully indulge in %s.\n\r", sysdata.mud_name );
   }
   return;
}

/* changed for new auth */
void auth_update( void )
{
   AUTH_LIST *auth;
   char buf[MIL], abuf[MSL];
   CMDTYPE *command;
   DESCRIPTOR_DATA *d;
   int level = LEVEL_IMMORTAL;
   bool found_imm = FALSE; /* Is at least 1 immortal on? */
   bool found_hit = FALSE; /* was at least one found? */

   command = find_command( "authorize" );
   if( !command )
      level = LEVEL_IMMORTAL;
   else
      level = command->level;

   mudstrlcpy( abuf, "--- Characters awaiting approval ---\n\r", MSL );
   for( auth = first_auth_name; auth; auth = auth->next )
   {
      if( auth != NULL && auth->state < AUTH_CHANGE_NAME )
      {
         found_hit = TRUE;
         snprintf( buf, MIL, "Name: %s      Status: %s\n\r", auth->name,
                   ( auth->state == AUTH_ONLINE ) ? "Online" : "Offline" );
         mudstrlcat( abuf, buf, MSL );
      }
   }

   if( found_hit )
   {
      for( d = first_descriptor; d; d = d->next )
         if( d->connected == CON_PLAYING && d->character && IS_IMMORTAL( d->character ) && d->character->level >= level )
            found_imm = TRUE;

      if( found_imm )
         log_string_plus( abuf, LOG_AUTH, level );
   }
}

/* Modified to require an "add" or "remove" argument in addition to name - Samson 10-18-98 */
/* Gutted to append to an external file now rather than load the pile into memory at boot - Samson 11-21-03 */
CMDF do_reserve( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "To add a name: reserve <name> add\n\r", ch );
      send_to_char( "To remove a name: Someone with shell access has to do this now.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "add" ) )
   {
      append_to_file( RESERVED_LIST, "%s", arg );
      send_to_char( "Name reserved.\n\r", ch );
      return;
   }
   send_to_char( "Invalid argument.\n\r", ch );
}
