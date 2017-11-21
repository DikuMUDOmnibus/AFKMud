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
 *                      Command interpretation module                       *
 ****************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include "mud.h"
#include "alias.h"
#include "clans.h"

/*
 * Externals
 */
bool mprog_keyword_trigger( char *txt, CHAR_DATA * actor );
void subtract_times( struct timeval *endtime, struct timeval *starttime );
bool check_ability( CHAR_DATA * ch, char *command, char *argument );
bool check_skill( CHAR_DATA * ch, char *command, char *argument );
#ifdef MULTIPORT
bool shell_hook( CHAR_DATA * ch, char *command, char *argument );
#endif
#ifdef I3
bool I3_command_hook( CHAR_DATA * ch, char *command, char *argument );
#endif
#ifdef IMC
bool imc_command_hook( CHAR_DATA * ch, char *command, char *argument );
#endif
bool local_channel_hook( CHAR_DATA * ch, char *command, char *argument );
bool valid_watch( char *logline );
void write_watch_files( CHAR_DATA * ch, CMDTYPE * cmd, char *logline );
void check_watch_cmd( CHAR_DATA * ch, CMDTYPE * cmd, char *logline );

/*
 * Log-all switch.
 */
bool fLogAll = FALSE;

CMDTYPE *command_hash[126];   /* hash table for cmd_table */
SOCIALTYPE *social_index[27]; /* hash table for socials   */
extern char lastplayercmd[MIL * 2];

void cmdf( CHAR_DATA * ch, char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   interpret( ch, buf );
}

/* Be damn sure the function you pass here is valid, or Bad Things(tm) will happen. */
void funcf( CHAR_DATA * ch, DO_FUN * cmd, char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   if( !cmd )
   {
      bug( "%s", "Bad function passed to funcf!" );
      return;
   }

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   ( cmd ) ( ch, buf );
}

/******************************************************
      Desolation of the Dragon MUD II - Alias Code
      (C) 1997, 1998  Jesse DeFer and Heath Leach
 http://dotd.mudservices.com  dotd@dotd.mudservices.com 
 ******************************************************/

ALIAS_DATA *find_alias( CHAR_DATA * ch, char *argument )
{
   ALIAS_DATA *pal;

   if( !ch || !ch->pcdata )
      return ( NULL );

   for( pal = ch->pcdata->first_alias; pal; pal = pal->next )
      if( !str_prefix( argument, pal->name ) )
         return ( pal );

   return ( NULL );
}

void free_alias( CHAR_DATA * ch, ALIAS_DATA * pal )
{
   STRFREE( pal->name );
   STRFREE( pal->cmd );
   UNLINK( pal, ch->pcdata->first_alias, ch->pcdata->last_alias, next, prev );
   DISPOSE( pal );
   return;
}

CMDF do_alias( CHAR_DATA * ch, char *argument )
{
   ALIAS_DATA *pal = NULL;
   char arg[MIL];
   char *p;

   if( IS_NPC( ch ) )
      return;

   for( p = argument; *p != '\0'; p++ )
   {
      if( *p == '~' )
      {
         send_to_char( "Command not acceptable, cannot use the ~ character.\n\r", ch );
         return;
      }
   }

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      if( !ch->pcdata->first_alias )
      {
         send_to_char( "You have no aliases defined!\n\r", ch );
         return;
      }
      pager_printf( ch, "%-20s What it does\n\r", "Alias" );
      for( pal = ch->pcdata->first_alias; pal; pal = pal->next )
         pager_printf( ch, "%-20s %s\n\r", pal->name, pal->cmd );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      if( ( pal = find_alias( ch, arg ) ) != NULL )
      {
         free_alias( ch, pal );
         send_to_char( "Deleted Alias.\n\r", ch );
      }
      else
         send_to_char( "That alias does not exist.\n\r", ch );
      return;
   }

   if( ( pal = find_alias( ch, arg ) ) == NULL )
   {
      CREATE( pal, ALIAS_DATA, 1 );
      pal->name = STRALLOC( arg );
      pal->cmd = STRALLOC( argument );
      LINK( pal, ch->pcdata->first_alias, ch->pcdata->last_alias, next, prev );
      send_to_char( "Created Alias.\n\r", ch );
   }
   else
   {
      STRFREE( pal->cmd );
      pal->cmd = STRALLOC( argument );
      send_to_char( "Modified Alias.\n\r", ch );
   }
}

void free_aliases( CHAR_DATA * ch )
{
   ALIAS_DATA *pal, *next_pal;

   if( !ch || !ch->pcdata )
      return;

   for( pal = ch->pcdata->first_alias; pal; pal = next_pal )
   {
      next_pal = pal->next;
      free_alias( ch, pal );
   }
}

bool check_alias( CHAR_DATA * ch, char *command, char *argument )
{
   char arg[MIL];
   ALIAS_DATA *alias;

   if( !( alias = find_alias( ch, command ) ) )
      return FALSE;

   if( !alias->cmd || !*alias->cmd )
      return FALSE;

   snprintf( arg, MIL, "%s", alias->cmd );

   if( ch->pcdata->cmd_recurse == -1 || ++ch->pcdata->cmd_recurse > 50 )
   {
      if( ch->pcdata->cmd_recurse != -1 )
      {
         send_to_char( "Unable to further process command, recurses too much.\n\r", ch );
         ch->pcdata->cmd_recurse = -1;
      }
      return FALSE;
   }

   if( argument && argument[0] != '\0' )
   {
      mudstrlcat( arg, " ", MIL );
      mudstrlcat( arg, argument, MIL );
   }
   interpret( ch, arg );
   return TRUE;
}

/*
 * Character not in position for command?
 */
bool check_pos( CHAR_DATA * ch, short position )
{

   if( IS_NPC( ch ) && ch->position > 3 ) /*Band-aid alert?  -- Blod */
      return TRUE;

   if( ch->position < position )
   {
      switch ( ch->position )
      {
         case POS_DEAD:
            send_to_char( "A little difficult to do when you are DEAD...\n\r", ch );
            break;

         case POS_MORTAL:
         case POS_INCAP:
            send_to_char( "You are hurt far too bad for that.\n\r", ch );
            break;

         case POS_STUNNED:
            send_to_char( "You are too stunned to do that.\n\r", ch );
            break;

         case POS_SLEEPING:
            send_to_char( "In your dreams, or what?\n\r", ch );
            break;

         case POS_RESTING:
            send_to_char( "Nah... You feel too relaxed...\n\r", ch );
            break;

         case POS_SITTING:
            send_to_char( "You can't do that sitting down.\n\r", ch );
            break;

         case POS_FIGHTING:
            if( position <= POS_EVASIVE )
            {
               send_to_char( "This fighting style is too demanding for that!\n\r", ch );
            }
            else
            {
               send_to_char( "No way!  You are still fighting!\n\r", ch );
            }
            break;
         case POS_DEFENSIVE:
            if( position <= POS_EVASIVE )
            {
               send_to_char( "This fighting style is too demanding for that!\n\r", ch );
            }
            else
            {
               send_to_char( "No way!  You are still fighting!\n\r", ch );
            }
            break;
         case POS_AGGRESSIVE:
            if( position <= POS_EVASIVE )
            {
               send_to_char( "This fighting style is too demanding for that!\n\r", ch );
            }
            else
            {
               send_to_char( "No way!  You are still fighting!\n\r", ch );
            }
            break;
         case POS_BERSERK:
            if( position <= POS_EVASIVE )
            {
               send_to_char( "This fighting style is too demanding for that!\n\r", ch );
            }
            else
            {
               send_to_char( "No way!  You are still fighting!\n\r", ch );
            }
            break;
         case POS_EVASIVE:
            send_to_char( "No way!  You are still fighting!\n\r", ch );
            break;

      }
      return FALSE;
   }
   return TRUE;
}

bool check_social( CHAR_DATA * ch, char *command, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *obj; /* Object socials */
   SOCIALTYPE *social;
   CHAR_DATA *remfirst, *remlast, *remtemp;  /* for ignore cmnd */

   if( ( social = find_social( command ) ) == NULL )
      return FALSE;

   if( IS_PLR_FLAG( ch, PLR_NO_EMOTE ) )
   {
      send_to_char( "You are anti-social!\n\r", ch );
      return TRUE;
   }

   switch ( ch->position )
   {
      case POS_DEAD:
         send_to_char( "Lie still; you are DEAD.\n\r", ch );
         return TRUE;

      case POS_INCAP:
      case POS_MORTAL:
         send_to_char( "You are hurt far too bad for that.\n\r", ch );
         return TRUE;

      case POS_STUNNED:
         send_to_char( "You are too stunned to do that.\n\r", ch );
         return TRUE;

      case POS_SLEEPING:
         /*
          * I just know this is the path to a 12" 'if' statement.  :(
          * But two players asked for it already!  -- Furey
          */
         if( !str_cmp( social->name, "snore" ) )
            break;
         send_to_char( "In your dreams, or what?\n\r", ch );
         return TRUE;
   }

   remfirst = NULL;
   remlast = NULL;
   remtemp = NULL;

   /*
    * Search room for chars ignoring social sender and 
    */
   /*
    * remove them from the room until social has been completed 
    */
   /*
    * Hmmmm. This all seems a bit on the dangerous side to me - Samson 
    */
   for( victim = ch->in_room->first_person; victim; victim = victim->next_in_room )
   {
      if( is_ignoring( victim, ch ) )
      {
         if( !IS_IMMORTAL( ch ) || victim->level > ch->level )
         {
            UNLINK( victim, victim->in_room->first_person, victim->in_room->last_person, next_in_room, prev_in_room );
            LINK( victim, remfirst, remlast, next_in_room, prev_in_room );
         }
         else
            ch_printf( victim, "&[ignore]You attempt to ignore %s, but are unable to do so.\n\r", ch->name );
      }
   }

   one_argument( argument, arg );
   victim = NULL;
   if( !arg || arg[0] == '\0' )
   {
      act( AT_SOCIAL, social->others_no_arg, ch, NULL, victim, TO_ROOM );
      act( AT_SOCIAL, social->char_no_arg, ch, NULL, victim, TO_CHAR );

      /*
       * Replace the chars in the ignoring list to the room 
       */
      /*
       * note that the ordering of the players in the room might change 
       */
      for( victim = remfirst; victim; victim = remtemp )
      {
         remtemp = victim->next_in_room;
         if( !char_to_room( victim, ch->in_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      }
      return TRUE;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      /*
       * If they aren't in the room, they may be in the list of people ignoring... 
       */
      for( victim = remfirst; victim; victim = victim->next_in_room )
      {
         if( nifty_is_name( victim->name, arg ) || nifty_is_name_prefix( arg, victim->name ) )
         {
            send_to_char( "They aren't here.\n\r", ch );
            break;
         }
      }

      if( ( ( obj = get_obj_list( ch, arg, ch->in_room->first_content ) )
            || ( obj = get_obj_list( ch, arg, ch->first_carrying ) ) ) && !victim )
      {
         if( social->obj_self && social->obj_others )
         {
            act( AT_SOCIAL, social->obj_self, ch, NULL, obj, TO_CHAR );
            act( AT_SOCIAL, social->obj_others, ch, NULL, obj, TO_ROOM );
         }

         /*
          * Replace the chars in the ignoring list to the room 
          */
         /*
          * note that the ordering of the players in the room might change 
          */
         for( victim = remfirst; victim; victim = remtemp )
         {
            remtemp = victim->next_in_room;
            if( !char_to_room( victim, ch->in_room ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         }
         return TRUE;
      }

      if( !victim )
         send_to_char( "They aren't here.\n\r", ch );

      /*
       * Replace the chars in the ignoring list to the room 
       */
      /*
       * note that the ordering of the players in the room might change 
       */
      for( victim = remfirst; victim; victim = remtemp )
      {
         remtemp = victim->next_in_room;
         if( !char_to_room( victim, ch->in_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      }
      return TRUE;
   }

   if( victim == ch )
   {
      act( AT_SOCIAL, social->others_auto, ch, NULL, victim, TO_ROOM );
      act( AT_SOCIAL, social->char_auto, ch, NULL, victim, TO_CHAR );

      /*
       * Replace the chars in the ignoring list to the room 
       */
      /*
       * note that the ordering of the players in the room might change 
       */
      for( victim = remfirst; victim; victim = remtemp )
      {
         remtemp = victim->next_in_room;
         if( !char_to_room( victim, ch->in_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      }
      return TRUE;
   }

   act( AT_SOCIAL, social->others_found, ch, NULL, victim, TO_NOTVICT );
   act( AT_SOCIAL, social->char_found, ch, NULL, victim, TO_CHAR );
   act( AT_SOCIAL, social->vict_found, ch, NULL, victim, TO_VICT );

   if( !IS_NPC( ch ) && IS_NPC( victim ) && !IS_AFFECTED( victim, AFF_CHARM )
       && IS_AWAKE( victim ) && !HAS_PROG( victim->pIndexData, ACT_PROG ) )
   {
      switch ( number_bits( 4 ) )
      {
         case 0:
            if( IS_EVIL( ch ) || IS_NEUTRAL( ch ) )
            {
               act( AT_ACTION, "$n slaps $N.", victim, NULL, ch, TO_NOTVICT );
               act( AT_ACTION, "You slap $N.", victim, NULL, ch, TO_CHAR );
               act( AT_ACTION, "$n slaps you.", victim, NULL, ch, TO_VICT );
            }
            else
            {
               act( AT_ACTION, "$n acts like $N doesn't even exist.", victim, NULL, ch, TO_NOTVICT );
               act( AT_ACTION, "You just ignore $N.", victim, NULL, ch, TO_CHAR );
               act( AT_ACTION, "$n appears to be ignoring you.", victim, NULL, ch, TO_VICT );
            }
            break;

         case 1:
         case 2:
         case 3:
         case 4:
         case 5:
         case 6:
         case 7:
         case 8:
            act( AT_SOCIAL, social->others_found, victim, NULL, ch, TO_NOTVICT );
            act( AT_SOCIAL, social->char_found, victim, NULL, ch, TO_CHAR );
            act( AT_SOCIAL, social->vict_found, victim, NULL, ch, TO_VICT );
            break;

         case 9:
         case 10:
         case 11:
         case 12:
            act( AT_ACTION, "$n slaps $N.", victim, NULL, ch, TO_NOTVICT );
            act( AT_ACTION, "You slap $N.", victim, NULL, ch, TO_CHAR );
            act( AT_ACTION, "$n slaps you.", victim, NULL, ch, TO_VICT );
            break;
      }
   }

   /*
    * Replace the chars in the ignoring list to the room 
    */
   /*
    * note that the ordering of the players in the room might change 
    */
   for( victim = remfirst; victim; victim = remtemp )
   {
      remtemp = victim->next_in_room;
      if( !char_to_room( victim, ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }
   return TRUE;
}

/*
 *  This function checks the command against the command flags to make
 *  sure they can use the command online.  This allows the commands to be
 *  edited online to allow or disallow certain situations.  May be an idea
 *  to rework this so we can edit the message sent back online, as well as
 *  maybe a crude parsing language so we can add in new checks online without
 *  haveing to hard-code them in.     -- Shaddai   August 25, 1997
 */

/* Needed a global here */
char cmd_flag_buf[MSL];

char *check_cmd_flags( CHAR_DATA * ch, CMDTYPE * cmd )
{
   if( IS_AFFECTED( ch, AFF_POSSESS ) && IS_CMD_FLAG( cmd, CMD_POSSESS ) )
      snprintf( cmd_flag_buf, MSL, "You can't %s while you are possessing someone!\n\r", cmd->name );
   else if( ch->morph != NULL && IS_CMD_FLAG( cmd, CMD_POLYMORPHED ) )
      snprintf( cmd_flag_buf, MSL, "You can't %s while you are polymorphed!\n\r", cmd->name );
   else
      cmd_flag_buf[0] = '\0';
   return cmd_flag_buf;
}

void start_timer( struct timeval *starttime )
{
   if( !starttime )
   {
      bug( "%s", "Start_timer: NULL stime." );
      return;
   }
   gettimeofday( starttime, NULL );
   return;
}

time_t end_timer( struct timeval * starttime )
{
   struct timeval etime;

   /*
    * Mark etime before checking stime, so that we get a better reading.. 
    */
   gettimeofday( &etime, NULL );
   if( !starttime || ( !starttime->tv_sec && !starttime->tv_usec ) )
   {
      bug( "%s", "End_timer: bad starttime." );
      return 0;
   }
   subtract_times( &etime, starttime );
   /*
    * stime becomes time used 
    */
   *starttime = etime;
   return ( etime.tv_sec * 1000000 ) + etime.tv_usec;
}

/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
/* Function modified from original form - Samson 3-23-98, 3-26-98 */
void interpret( CHAR_DATA * ch, char *argument )
{
   char command[MIL], logline[MIL];
   char *buf;
   TIMER *timer = NULL;
   CMDTYPE *cmd = NULL;
   int trust, loglvl;
   bool found;
   struct timeval time_used;
   long tmptime;

   if( !ch )
   {
      bug( "%s", "interpret: null ch!" );
      return;
   }

   if( !ch->in_room )
   {
      bug( "interpret: %s null in_room!", ch->name );
      return;
   }

   found = FALSE;
   if( ch->substate == SUB_REPEATCMD )
   {
      DO_FUN *fun;

      if( !( fun = ch->last_cmd ) )
      {
         ch->substate = SUB_NONE;
         bug( "interpret: %s SUB_REPEATCMD with NULL last_cmd", ch->name );
         return;
      }
      else
      {
         int x;

         /*
          * yes... we lose out on the hashing speediness here...
          * but the only REPEATCMDS are wizcommands (currently)
          */
         for( x = 0; x < 126; x++ )
         {
            for( cmd = command_hash[x]; cmd; cmd = cmd->next )
               if( cmd->do_fun == fun )
               {
                  found = TRUE;
                  break;
               }
            if( found )
               break;
         }
         if( !found )
         {
            cmd = NULL;
            bug( "%s", "interpret: SUB_REPEATCMD: last_cmd invalid" );
            return;
         }
         snprintf( logline, MIL, "(%s) %s", cmd->name, argument );
      }
   }

   if( !cmd )
   {
      /*
       * Changed the order of these ifchecks to prevent crashing. 
       */
      if( !argument || argument[0] == '\0' || !str_cmp( argument, "" ) )
      {
         bug( "%s", "interpret: null argument!" );
         return;
      }

      /*
       * Strip leading spaces.
       */
      while( isspace( *argument ) )
         argument++;
      if( argument[0] == '\0' )
         return;

      /*
       * Implement freeze command.
       */
      if( IS_PLR_FLAG( ch, PLR_FREEZE ) )
      {
         send_to_char( "You're totally frozen!\n\r", ch );
         return;
      }

      /*
       * Spamguard autofreeze - Samson 3-18-01 
       */
      if( !IS_NPC( ch ) && IS_AFFECTED( ch, AFF_SPAMGUARD ) )
      {
         send_to_char( "&RYou've been autofrozen by the spamguard!!\n\r", ch );
         send_to_char( "&YTriggering the spamfreeze more than once will only worsen your situation.\n\r", ch );
         return;
      }

      if( ( mprog_keyword_trigger( argument, ch ) ) == TRUE )
      {
         /*
          * Added because keywords are checked before commands 
          */
         if( IS_PLR_FLAG( ch, PLR_AFK ) )
         {
            REMOVE_PLR_FLAG( ch, PLR_AFK );
            REMOVE_PLR_FLAG( ch, PLR_IDLING );
            DISPOSE( ch->pcdata->afkbuf );
            act( AT_GREY, "$n is no longer afk.", ch, NULL, NULL, TO_CANSEE );
            send_to_char( "You are no longer afk.\n\r", ch );
         }
         return;
      }

      /*
       * Grab the command word.
       * Special parsing so ' can be a command, also no spaces needed after punctuation.
       */
      mudstrlcpy( logline, argument, MIL );
      if( !isalpha( argument[0] ) && !isdigit( argument[0] ) )
      {
         command[0] = argument[0];
         command[1] = '\0';
         argument++;
         while( isspace( *argument ) )
            argument++;
      }
      else
         argument = one_argument( argument, command );

      /*
       * Look for command in command table.
       * Check for council powers and/or bestowments
       */
      trust = get_trust( ch );
      for( cmd = command_hash[LOWER( command[0] ) % 126]; cmd; cmd = cmd->next )
         if( !str_prefix( command, cmd->name )
             && ( cmd->level <= trust ||
                  ( !IS_NPC( ch ) && ch->pcdata->council && is_name( cmd->name, ch->pcdata->council->powers )
                    && cmd->level <= ( trust + MAX_CPD ) ) || ( !IS_NPC( ch ) && ch->pcdata->bestowments
                                                                && ch->pcdata->bestowments[0] != '\0'
                                                                && hasname( ch->pcdata->bestowments, cmd->name )
                                                                && cmd->level <= ( trust + sysdata.bestow_dif ) ) ) )
         {
            found = TRUE;
            break;
         }

      /*
       * Turn off afk bit when any command performed.
       */
      if( IS_PLR_FLAG( ch, PLR_AFK ) && ( str_cmp( command, "AFK" ) ) )
      {
         REMOVE_PLR_FLAG( ch, PLR_AFK );
         REMOVE_PLR_FLAG( ch, PLR_IDLING );
         DISPOSE( ch->pcdata->afkbuf );
         act( AT_GREY, "$n is no longer afk.", ch, NULL, NULL, TO_CANSEE );
         send_to_char( "You are no longer afk.\n\r", ch );
      }

      /*
       * Check command for action flag, undo hide if it needs to be - Samson 7-7-00 
       */
      if( found && IS_CMD_FLAG( cmd, CMD_ACTION ) )
      {
         affect_strip( ch, gsn_hide );
         REMOVE_AFFECTED( ch, AFF_HIDE );
      }
   }

   /*
    * Log and snoop.
    */
   snprintf( lastplayercmd, MIL * 2, "%s used command: %s", ch->name, logline );

   if( found && cmd->log == LOG_NEVER )
      mudstrlcpy( logline, "XXXXXXXX XXXXXXXX XXXXXXXX", MIL );

   loglvl = found ? cmd->log : LOG_NORMAL;

   /*
    * Write input line to watch files if applicable
    */
   if( !IS_NPC( ch ) && ch->desc && valid_watch( logline ) )
   {
      if( found && IS_CMD_FLAG( cmd, CMD_WATCH ) )
         write_watch_files( ch, cmd, logline );
      else if( IS_PCFLAG( ch, PCFLAG_WATCH ) )
         write_watch_files( ch, NULL, logline );
   }

   /*
    * Cannot perform commands that aren't ghost approved 
    */
   if( found && !IS_CMD_FLAG( cmd, CMD_GHOST ) && IS_PLR_FLAG( ch, PLR_GHOST ) )
   {
      send_to_char( "&YBut you're a GHOST! You can't do that until you've been resurrected!\n\r", ch );
      return;
   }

   if( found && !IS_NPC( ch ) && IS_CMD_FLAG( cmd, CMD_MUDPROG ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( IS_PLR_FLAG( ch, PLR_LOG ) || fLogAll || loglvl == LOG_BUILD || loglvl == LOG_HIGH || loglvl == LOG_ALWAYS )
   {
      char lbuf[MSL];

      /*
       * Added by Narn to show who is switched into a mob that executes
       * a logged command.  Check for descriptor in case force is used. 
       */
      if( ch->desc && ch->desc->original )
         snprintf( lbuf, MSL, "Log %s (%s): %s", ch->name, ch->desc->original->name, logline );
      else
         snprintf( lbuf, MSL, "Log %s: %s", ch->name, logline );

      check_watch_cmd( ch, cmd, logline );

      /*
       * Make it so a 'log all' will send most output to the log
       * file only, and not spam the log channel to death   -Thoric
       */
      if( fLogAll && loglvl == LOG_NORMAL && IS_PLR_FLAG( ch, PLR_LOG ) )
         loglvl = LOG_ALL;
      log_string_plus( lbuf, loglvl, ch->level );
   }

   if( ch->desc && ch->desc->snoop_by )
   {
      buffer_printf( ch->desc->snoop_by, "%s", ch->name );
      write_to_buffer( ch->desc->snoop_by, "% ", 2 );
      write_to_buffer( ch->desc->snoop_by, logline, 0 );
      write_to_buffer( ch->desc->snoop_by, "\n\r", 2 );
   }

   /*
    * check for a timer delayed command (search, dig, detrap, etc) 
    */
   if( ( timer = get_timerptr( ch, TIMER_DO_FUN ) ) != NULL )
   {
      int tempsub;

      tempsub = ch->substate;
      ch->substate = SUB_TIMER_DO_ABORT;
      ( timer->do_fun ) ( ch, "" );
      if( char_died( ch ) )
         return;
      if( ch->substate != SUB_TIMER_CANT_ABORT )
      {
         ch->substate = tempsub;
         extract_timer( ch, timer );
      }
      else
      {
         ch->substate = tempsub;
         return;
      }
   }

   /*
    * Look for command in skill and socials table.
    */
   if( !found )
   {
#ifdef MULTIPORT
      if( !shell_hook( ch, command, argument ) && !local_channel_hook( ch, command, argument )
#else
      if( !local_channel_hook( ch, command, argument )
#endif
          && !check_skill( ch, command, argument )
          && !check_ability( ch, command, argument )
          && !check_alias( ch, command, argument ) && !check_social( ch, command, argument )
#ifdef I3
          && !I3_command_hook( ch, command, argument )
#endif
#ifdef IMC
          && !imc_command_hook( ch, command, argument )
#endif
          )
      {
         EXIT_DATA *pexit;

         /*
          * check for an auto-matic exit command 
          */
         if( ( pexit = find_door( ch, command, TRUE ) ) != NULL && IS_EXIT_FLAG( pexit, EX_xAUTO ) )
         {
            if( IS_EXIT_FLAG( pexit, EX_CLOSED )
                && ( !IS_AFFECTED( ch, AFF_PASS_DOOR ) || IS_EXIT_FLAG( pexit, EX_NOPASSDOOR ) ) )
            {
               if( !IS_EXIT_FLAG( pexit, EX_SECRET ) )
                  act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
               else
                  send_to_char( "You cannot do that here.\n\r", ch );
               return;
            }
            if( ( IS_EXIT_FLAG( pexit, EX_FORTIFIED ) || IS_EXIT_FLAG( pexit, EX_HEAVY )
                  || IS_EXIT_FLAG( pexit, EX_MEDIUM ) || IS_EXIT_FLAG( pexit, EX_LIGHT )
                  || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) ) )
            {
               send_to_char( "You cannot do that here.\n\r", ch );
               return;
            }
            move_char( ch, pexit, 0, pexit->vdir, FALSE );
            return;
         }
         send_to_char( "Huh?\n\r", ch );
      }
      return;
   }

   /*
    * Character not in position for command?
    */
   if( !check_pos( ch, cmd->position ) )
      return;

   /*
    * So we can check commands for things like Posses and Polymorph
    * *  But still keep the online editing ability.  -- Shaddai
    * *  Send back the message to print out, so we have the option
    * *  this function might be usefull elsewhere.  Also using the
    * *  send_to_char_color so we can colorize the strings if need be. --Shaddai
    */
   buf = check_cmd_flags( ch, cmd );

   if( buf[0] != '\0' )
   {
      send_to_char( buf, ch );
      return;
   }

   /*
    * Dispatch the command.
    */
   ch->prev_cmd = ch->last_cmd;  /* haus, for automapping */
   ch->last_cmd = cmd->do_fun;   /* Usurped in hopes of using it for the spamguard instead */
   if( !str_cmp( cmd->name, "slay" ) )
   {
      char *tmpargument = argument;
      char tmparg[MIL];

      tmpargument = one_argument( tmpargument, tmparg );

      if( !str_cmp( tmparg, "Samson" ) )
      {
         send_to_char( "The forces of the universe prevent you from taking this action....\n\r", ch );
         return;
      }
   }
   start_timer( &time_used );
   ( *cmd->do_fun ) ( ch, argument );
   end_timer( &time_used );

   tmptime = UMIN( time_used.tv_sec, 19 ) * 1000000 + time_used.tv_usec;

   /*
    * laggy command notice: command took longer than 3.0 seconds 
    */
   if( tmptime > 3000000 )
   {
      log_printf_plus( LOG_NORMAL, ch->level, "[*****] LAG: %s: %s %s (R:%d S:%ld.%06ld)", ch->name,
                       cmd->name, ( cmd->log == LOG_NEVER ? "XXX" : argument ),
                       ch->in_room ? ch->in_room->vnum : 0, time_used.tv_sec, time_used.tv_usec );
   }
   mudstrlcpy( lastplayercmd, "No commands pending", MIL * 2 );
   tail_chain(  );
}

CMDTYPE *find_command( char *command )
{
   CMDTYPE *cmd;
   int hash;

   hash = LOWER( command[0] ) % 126;

   for( cmd = command_hash[hash]; cmd; cmd = cmd->next )
      if( !str_prefix( command, cmd->name ) )
         return cmd;

   return NULL;
}

SOCIALTYPE *find_social( char *command )
{
   SOCIALTYPE *social;
   int hash;

   if( command[0] < 'a' || command[0] > 'z' )
      hash = 0;
   else
      hash = ( command[0] - 'a' ) + 1;

   for( social = social_index[hash]; social; social = social->next )
      if( !str_prefix( command, social->name ) )
         return social;

   return NULL;
}

/*
 * Return TRUE if an argument is completely numeric.
 */
bool is_number( const char *arg )
{
   bool first = TRUE;

   if( *arg == '\0' )
      return FALSE;

   for( ; *arg != '\0'; arg++ )
   {
      if( first && *arg == '-' )
      {
         first = FALSE;
         continue;
      }

      if( !isdigit( *arg ) )
         return FALSE;
      first = FALSE;
   }
   return TRUE;
}

/*
 * Given a string like 14.foo, return 14 and 'foo'
 */
int number_argument( char *argument, char *arg )
{
   char *pdot;
   int number;

   for( pdot = argument; *pdot != '\0'; pdot++ )
   {
      if( *pdot == '.' )
      {
         *pdot = '\0';
         number = atoi( argument );
         *pdot = '.';
         strcpy( arg, pdot + 1 );   /* Leave this one alone! BAD THINGS(TM) will happen if you don't! */
         return number;
      }
   }
   strcpy( arg, argument );   /* Leave this one alone! BAD THINGS(TM) will happen if you don't! */
   return 1;
}

/* Scan a whole argument for a single word - return TRUE if found - Samson 7-24-00 */
/* Code by Orion Elder */
bool arg_cmp( char *haystack, char *needle )
{
   char argument[MSL];

   for( ;; )
   {
      haystack = one_argument( haystack, argument );

      if( !argument || argument[0] == '\0' )
         return FALSE;
      else if( !str_cmp( argument, needle ) )
         return TRUE;
      else
         continue;
   }
}

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes. No longer mangles case either. That used to be annoying.
 */
char *one_argument( char *argument, char *arg_first )
{
   char cEnd;
   int count;

   count = 0;

   while( isspace( *argument ) )
      argument++;

   cEnd = ' ';
   if( *argument == '\'' || *argument == '"' )
      cEnd = *argument++;

   while( *argument != '\0' || ++count >= 255 )
   {
      if( *argument == cEnd )
      {
         argument++;
         break;
      }
      *arg_first = ( *argument );
      arg_first++;
      argument++;
   }
   *arg_first = '\0';

   while( isspace( *argument ) )
      argument++;

   return argument;
}

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes.  Delimiters = { ' ', '-' }
 * No longer mangles case either. That used to be annoying.
 */
char *one_argument2( char *argument, char *arg_first )
{
   char cEnd;
   short count;

   count = 0;

   if( !argument || argument[0] == '\0' )
   {
      arg_first[0] = '\0';
      return argument;
   }

   while( isspace( *argument ) )
      argument++;

   cEnd = ' ';
   if( *argument == '\'' || *argument == '"' )
      cEnd = *argument++;

   while( *argument != '\0' || ++count >= 255 )
   {
      if( *argument == cEnd || *argument == '-' )
      {
         argument++;
         break;
      }
      *arg_first = ( *argument );
      arg_first++;
      argument++;
   }
   *arg_first = '\0';

   while( isspace( *argument ) )
      argument++;

   return argument;
}

CMDF do_timecmd( CHAR_DATA * ch, char *argument )
{
   struct timeval starttime;
   struct timeval endtime;
   static bool timing;
   char arg[MIL];

   send_to_char( "Timing\n\r", ch );
   if( timing )
      return;

   one_argument( argument, arg );
   if( !*arg )
   {
      send_to_char( "No command to time.\n\r", ch );
      return;
   }
   if( !str_cmp( arg, "update" ) )
   {
      if( timechar )
         send_to_char( "Another person is already timing updates.\n\r", ch );
      else
      {
         timechar = ch;
         send_to_char( "Setting up to record next update loop.\n\r", ch );
      }
      return;
   }
   send_to_char( "&[plain]Starting timer.\n\r", ch );
   timing = TRUE;
   gettimeofday( &starttime, NULL );
   interpret( ch, argument );
   gettimeofday( &endtime, NULL );
   timing = FALSE;
   send_to_char( "&[plain]Timing complete.\n\r", ch );
   subtract_times( &endtime, &starttime );
   ch_printf( ch, "Timing took %ld.%06ld seconds.\n\r", endtime.tv_sec, endtime.tv_usec );
   return;
}
