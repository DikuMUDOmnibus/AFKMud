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
 *                        Wizard/god command module                         *
 ****************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fnmatch.h>
#include "mud.h"
#include "calendar.h"
#include "clans.h"
#include "deity.h"
#include "event.h"
#include "finger.h"
#ifdef I3
#include "i3.h"
#endif
#ifdef IMC
#include "imc.h"
#endif
#include "liquids.h"
#include "msp.h"
#include "mud_prog.h"
#include "mxp.h"
#include "overland.h"
#include "pfiles.h"
#include "shops.h"

/* External functions */
#ifdef MULTIPORT
void shellcommands( CHAR_DATA * ch, int curr_lvl );
#endif
void build_wizinfo( void );
void note_attach( CHAR_DATA * ch );
int get_langflag( char *flag );
int get_color( char *argument );
int get_logflag( char *flag );
void save_sysdata( SYSTEM_DATA sys );
void write_race_file( int ra );
int calc_thac0( CHAR_DATA * ch, CHAR_DATA * victim, int dist );   /* For mstat */
void remove_member( char *clan_name, char *name ); /* For do_destroy */
CMDF do_oldwhere( CHAR_DATA * ch, char *argument );
void rent_adjust_pfile( char *argument );
CMDF do_help( CHAR_DATA * ch, char *argument );
LIQ_TABLE *get_liq_vnum( int vnum );
void save_clan( CLAN_DATA * clan );
void delete_clan( CHAR_DATA * ch, CLAN_DATA * clan );
bool in_arena( CHAR_DATA * ch );
void skill_notfound( CHAR_DATA * ch, char *argument );
void check_stored_objects( CHAR_DATA * ch, int cvnum );
int recall( CHAR_DATA * ch, int target );
void save_socials( void );
void save_commands( void );
void affect_modify( CHAR_DATA * ch, AFFECT_DATA * paf, bool fAdd );
void ClassSpecificStuff( CHAR_DATA * ch );
void load_commands( void );
void remove_from_auth( char *name );
void calc_season( void );
int get_saveflag( char *flag );
void delete_obj( OBJ_INDEX_DATA * obj );
void delete_mob( MOB_INDEX_DATA * mob );
void delete_room( ROOM_INDEX_DATA * room );
void removename( char **list, const char *name );
void addname( char **list, const char *name );
void ostat_plus( CHAR_DATA * ch, OBJ_DATA * obj, bool olc );
char *sha256_crypt( const char *pwd );

/*
 * Global variables.
 */
#ifdef MULTIPORT
extern bool compilelock;
#endif
extern const char *liquid_types[];
extern bool bootlock;
extern int reboot_counter;
#ifdef WEBSVR
extern int web_socket;
#endif

CMDF do_testmatch( CHAR_DATA * ch, char *argument )
{
   char arg1[MSL];

   argument = one_argument( argument, arg1 );

   if( !fnmatch( arg1, argument, 0 ) )
   {
      send_to_char( "Match!\n\r", ch );
      return;
   }
   send_to_char( "No Match!\n\r", ch );
   return;
}

CMDF do_lvlcheck( CHAR_DATA * ch, char *argument )
{
   int z, lev1 = 0, lev2 = 0, lev3 = 0;

   if( !is_number( argument ) )
   {
      send_to_char( "Must be numeric\n\r", ch );
      return;
   }

   z = atoi( argument );

   lev1 = exp_level( z - 1 );
   lev2 = exp_level( z );
   lev3 = exp_level( z + 1 );
   ch_printf( ch, "Level input: %d. Exp -1: %d. Exp: %d. Exp +1: %d.\n\r", z, lev1, lev2, lev3 );
}

#ifndef WEBSVR
/* dummy command for when the Webserver code is compiled out to keep it from killing the command entry. */
CMDF do_web( CHAR_DATA * ch, char *argument )
{
   send_to_char( "Huh?\n\r", ch );
   return;
}

CMDF do_webroom( CHAR_DATA * ch, char *argument )
{
   send_to_char( "Huh?\n\r", ch );
   return;
}
#endif

/* Password resetting command, added by Samson 2-11-98
   Code courtesy of John Strange - Triad Mud */
/* Ugraded to use SHA-256 Encryption - Samson 1-22-06 : Code by Druid */
CMDF do_newpassword( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim;
   char *pwdnew;

   if( IS_NPC( ch ) )
      return;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( ( ch->pcdata->pwd != '\0' ) && ( arg1[0] == '\0' || arg2[0] == '\0' ) )
   {
      send_to_char( "Syntax: newpass <char> <newpassword>.\n\r", ch );
      return;
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      ch_printf( ch, "%s isn't here, they have to be here to reset passwords.\n\r", arg1 );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "You cannot change the password of NPCs!\n\r", ch );
      return;
   }

   /*
    * Immortal level check added to code by Samson 2-11-98 
    */
   if( ch->level < LEVEL_ADMIN )
   {
      if( victim->level >= LEVEL_IMMORTAL )
      {
         send_to_char( "You can't change that person's password!\n\r", ch );
         return;
      }
   }
   else
   {
      if( victim->level >= ch->level )
      {
         ch_printf( ch, "%s would not appreciate that :P\n\r", victim->name );
         return;
      }
   }

   if( strlen( arg2 ) < 5 )
   {
      send_to_char( "New password must be at least five characters long.\n\r", ch );
      return;
   }

   /*
    * SHA-256 Encryption & Password Fix - Druid 
    */
   if( strlen( arg2 ) > 16 )
   {
      send_to_char( "New password cannot exceed 16 characters in length.\n\r", ch );
      return;
   }

   if( arg2[0] == '!' )
   {
      send_to_char( "New password cannot begin with the '!' character.\n\r", ch );
      return;
   }

   pwdnew = sha256_crypt( arg2 );   /* SHA-256 Encryption */
   DISPOSE( victim->pcdata->pwd );
   victim->pcdata->pwd = str_dup( pwdnew );
   save_char_obj( victim );
   ch_printf( ch, "&R%s's password has been changed to: %s\n\r&w", victim->name, arg2 );
   ch_printf( victim, "&R%s has changed your password to: %s\n\r&w", ch->name, arg2 );
   return;
}

/* Immortal highfive, added by Samson 2-15-98 Inspired by the version used on Crystal Shard */
/* Updated 2-10-02 to parse through act() instead of echo_to_all for the high immortal condition */
CMDF do_highfive( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( IS_NPC( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      act( AT_SOCIAL, "You jump in the air and highfive everyone in the room!", ch, NULL, NULL, TO_CHAR );
      act( AT_SOCIAL, "$n jumps in the air and highfives everyone in the room!", ch, NULL, ch, TO_ROOM );
      return;
   }

   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "Nobody by that name is here.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      act( AT_SOCIAL, "Sometimes you really amaze yourself!", ch, NULL, NULL, TO_CHAR );
      act( AT_SOCIAL, "$n gives $mself a highfive!", ch, NULL, ch, TO_ROOM );
      return;
   }

   if( IS_NPC( victim ) )
   {
      act( AT_SOCIAL, "You jump up and highfive $N!", ch, NULL, victim, TO_CHAR );
      act( AT_SOCIAL, "$n jumps up and gives you a highfive!", ch, NULL, victim, TO_VICT );
      act( AT_SOCIAL, "$n jumps up and gives $N a highfive!", ch, NULL, victim, TO_NOTVICT );
      return;
   }

   if( victim->level > LEVEL_TRUEIMM && ch->level > LEVEL_TRUEIMM )
   {
      CHAR_DATA *vch;

      for( vch = first_char; vch; vch = vch->next )
      {
         if( IS_NPC( vch ) )
            continue;

         if( vch == ch )
            act( AT_IMMORT, "The whole world rumbles as you highfive $N!", ch, NULL, victim, TO_CHAR );
         else if( vch == victim )
            act( AT_IMMORT, "The whole world rumbles as $n highfives you!", ch, NULL, victim, TO_VICT );
         else
            act( AT_IMMORT, "The whole world rumbles as $n and $N highfive!", ch, vch, victim, TO_THIRD );
      }
      return;
   }
   else
   {
      act( AT_SOCIAL, "You jump up and highfive $N!", ch, NULL, victim, TO_CHAR );
      act( AT_SOCIAL, "$n jumps up and gives you a highfive!", ch, NULL, victim, TO_VICT );
      act( AT_SOCIAL, "$n jumps up and gives $N a highfive!", ch, NULL, victim, TO_NOTVICT );
      return;
   }
}

/* Function modified from original form - Samson 3-23-98 */
/* Updated to Sadiq's wizhelp snippet - Thanks Sadiq! */
/* Accepts level argument to display an individual level - Added by Samson 2-20-00 */
CMDF do_wizhelp( CHAR_DATA * ch, char *argument )
{
   CMDTYPE *cmd;
   int col = 0, hash, curr_lvl;

   set_pager_color( AT_PLAIN, ch );

   if( !argument || argument[0] == '\0' )
   {
      for( curr_lvl = LEVEL_IMMORTAL; curr_lvl <= ch->level; curr_lvl++ )
      {
         send_to_pager( "\n\r\n\r", ch );
         col = 1;
         pager_printf( ch, "[LEVEL %-2d]\n\r", curr_lvl );
         for( hash = 0; hash < 126; hash++ )
            for( cmd = command_hash[hash]; cmd; cmd = cmd->next )
               if( ( cmd->level == curr_lvl ) && cmd->level <= ch->level )
               {
                  pager_printf( ch, "%-12s", cmd->name );
                  if( ++col % 6 == 0 )
                     send_to_pager( "\n\r", ch );
               }
#ifdef MULTIPORT
         shellcommands( ch, curr_lvl );
#endif
      }
      if( col % 6 != 0 )
         send_to_pager( "\n\r", ch );
      return;
   }

   if( !is_number( argument ) )
   {
      send_to_char( "Syntax: wizhelp [level]\n\r", ch );
      return;
   }

   curr_lvl = atoi( argument );

   if( curr_lvl < LEVEL_IMMORTAL || curr_lvl > MAX_LEVEL )
   {
      ch_printf( ch, "Valid levels are between %d and %d.\n\r", LEVEL_IMMORTAL, MAX_LEVEL );
      return;
   }

   send_to_pager( "\n\r\n\r", ch );
   col = 1;
   pager_printf( ch, "[LEVEL %-2d]\n\r", curr_lvl );
   for( hash = 0; hash < 126; hash++ )
      for( cmd = command_hash[hash]; cmd; cmd = cmd->next )
         if( cmd->level == curr_lvl )
         {
            pager_printf( ch, "%-12s", cmd->name );
            if( ++col % 6 == 0 )
               send_to_pager( "\n\r", ch );
         }
   if( col % 6 != 0 )
      send_to_pager( "\n\r", ch );
#ifdef MULTIPORT
   shellcommands( ch, curr_lvl );
#endif
   return;
}

CMDF do_bamfin( CHAR_DATA * ch, char *argument )
{
   if( !IS_NPC( ch ) )
   {
      smash_tilde( argument );
      DISPOSE( ch->pcdata->bamfin );
      ch->pcdata->bamfin = str_dup( argument );
      send_to_char( "&YBamfin set.\n\r", ch );
   }
   return;
}

CMDF do_bamfout( CHAR_DATA * ch, char *argument )
{
   if( !IS_NPC( ch ) )
   {
      smash_tilde( argument );
      DISPOSE( ch->pcdata->bamfout );
      ch->pcdata->bamfout = str_dup( argument );
      send_to_char( "&YBamfout set.\n\r", ch );
   }
   return;
}

CMDF do_rank( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "&[immortal]Usage:  rank <string>.\n\r", ch );
      send_to_char( "   or:  rank none.\n\r", ch );
      return;
   }
   smash_tilde( argument );
   STRFREE( ch->pcdata->rank );
   if( str_cmp( argument, "none" ) )
      ch->pcdata->rank = STRALLOC( argument );
   send_to_char( "&[immortal]Ok.\n\r", ch );
   return;
}

CMDF do_retire( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Retire whom?\n\r", ch );
      return;
   }
   if( ( victim = get_char_world( ch, argument ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }
   if( victim->level < LEVEL_SAVIOR )
   {
      ch_printf( ch, "The minimum level for retirement is %d.\n\r", LEVEL_SAVIOR );
      return;
   }
   if( IS_RETIRED( victim ) )
   {
      REMOVE_PCFLAG( victim, PCFLAG_RETIRED );
      ch_printf( ch, "%s returns from retirement.\n\r", victim->name );
      ch_printf( victim, "%s brings you back from retirement.\n\r", ch->name );
   }
   else
   {
      SET_PCFLAG( victim, PCFLAG_RETIRED );
      ch_printf( ch, "%s is now a retired immortal.\n\r", victim->name );
      ch_printf( victim, "Courtesy of %s, you are now a retired immortal.\n\r", ch->name );
   }
   return;
}

CMDF do_delay( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   char arg[MIL];
   int delay;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   if( !arg || arg[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax:  delay <victim> <# of rounds>\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, arg ) ) )
   {
      send_to_char( "No such character online.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Mobiles are unaffected by lag.\n\r", ch );
      return;
   }
   if( !IS_NPC( victim ) && get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You haven't the power to succeed against them.\n\r", ch );
      return;
   }
   if( !str_cmp( argument, "none" ) )
   {
      send_to_char( "All character delay removed.\n\r", ch );
      victim->wait = 0;
      return;
   }
   delay = atoi( argument );
   if( delay < 1 )
   {
      send_to_char( "Pointless.  Try a positive number.\n\r", ch );
      return;
   }
   if( delay > 999 )
      send_to_char( "You cruel bastard.  Just kill them.\n\r", ch );

   WAIT_STATE( victim, delay * sysdata.pulseviolence );
   ch_printf( ch, "You've delayed %s for %d rounds.\n\r", victim->name, delay );
   return;
}

CMDF do_deny( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Deny whom?\n\r", ch );
      return;
   }
   if( ( victim = get_char_world( ch, argument ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }
   SET_PLR_FLAG( victim, PLR_DENY );
   set_char_color( AT_IMMORT, victim );
   send_to_char( "You are denied access!\n\r", victim );
   ch_printf( ch, "You have denied access to %s.\n\r", victim->name );
   if( victim->fighting )
      stop_fighting( victim, TRUE );   /* Blodkai, 97 */
   save_char_obj( victim );
   if( victim->desc )
      close_socket( victim->desc, FALSE );
   return;
}

CMDF do_disconnect( CHAR_DATA * ch, char *argument )
{
   int desc;
   DESCRIPTOR_DATA *d;
   CHAR_DATA *victim;
   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "You must specify a player or descriptor to disconnect.\n\r", ch );
      return;
   }

   if( is_number( argument ) )
      desc = atoi( argument );
   else if( ( victim = get_char_world( ch, argument ) ) )
   {
      if( victim->desc == NULL )
      {
         ch_printf( ch, "%s does not have a desriptor.\n\r", victim->name );
         return;
      }
      else
         desc = victim->desc->descriptor; /* Seems weird... but this seems faster? --X */
   }
   else
   {
      ch_printf( ch, "Disconnect: '%s' was not found!\n\r", argument );
      return;
   }

   for( d = first_descriptor; d; d = d->next )
   {
      if( d->descriptor == desc )
      {
         victim = d->original ? d->original : d->character;

         if( ( victim && get_trust( victim ) >= get_trust( ch ) ) || ( victim && get_trust( victim ) >= get_trust( ch ) ) )
         {
            send_to_char( "You cannot disconnect that person.\n\r", ch );
            log_printf( "%s tried to disconnect %s but failed.", ch->name, victim->name ? victim->name : "Someone" );
            return;
         }
         if( victim )
            log_printf( "%s has disconnected %s", ch->name, victim->name ? victim->name : "Someone" );
         else
            log_printf( "%s has disconnected desc #%d", ch->name, desc );
         close_socket( d, FALSE );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
   }
   bug( "do_disconnect: desc '%d' not found!", desc );
   send_to_char( "Descriptor not found!\n\r", ch );
   return;
}

void echo_all_printf( short AT_COLOR, short tar, char *Str, ... )
{
   va_list arg;
   char argument[MSL];

   if( !Str || Str[0] == '\0' )
      return;

   va_start( arg, Str );
   vsnprintf( argument, MSL, Str, arg );
   va_end( arg );

   echo_to_all( AT_COLOR, argument, tar );
   return;
}

void echo_to_all( short AT_COLOR, char *argument, short tar )
{
   DESCRIPTOR_DATA *d;

   if( !argument || argument[0] == '\0' )
      return;

   for( d = first_descriptor; d; d = d->next )
   {
      /*
       * Added showing echoes to players who are editing, so they won't
       * * miss out on important info like upcoming reboots. --Narn 
       */
      /*
       * CON_PLAYING = 0, and anything else greater than 0 is a player who is actually in the game.
       * * They will need to see anything deemed important enough to use echo_to_all, such as
       * * power failures etc.
       */
      if( d->connected >= CON_PLAYING )
      {
         /*
          * This one is kinda useless except for switched.. 
          */
         if( tar == ECHOTAR_PC && IS_NPC( d->character ) )
            continue;
         else if( tar == ECHOTAR_IMM && !IS_IMMORTAL( d->character ) )
            continue;

         if( MXP_ON( d->character ) )
            send_to_char( MXP_TAG_SECURE, d->character );

         set_char_color( AT_COLOR, d->character );
         send_to_char( argument, d->character );
         send_to_char( "\n\r", d->character );
      }
   }
   return;
}

CMDF do_echo( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   short color;
   int target;
   char *parg;

   set_char_color( AT_IMMORT, ch );

   if( IS_PLR_FLAG( ch, PLR_NO_EMOTE ) )
   {
      send_to_char( "You can't do that right now.\n\r", ch );
      return;
   }
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Echo what?\n\r", ch );
      return;
   }

   if( ( color = get_color( argument ) ) )
      argument = one_argument( argument, arg );
   parg = argument;
   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "PC" ) || !str_cmp( arg, "player" ) )
      target = ECHOTAR_PC;
   else if( !str_cmp( arg, "imm" ) )
      target = ECHOTAR_IMM;
   else
   {
      target = ECHOTAR_ALL;
      argument = parg;
   }
   if( !color && ( color = get_color( argument ) ) )
      argument = one_argument( argument, arg );
   if( !color )
      color = AT_IMMORT;
   one_argument( argument, arg );
   echo_to_all( color, argument, target );
}

/* Stupid little function for the Voice of God - Samson 4-30-99 */
CMDF do_voice( CHAR_DATA * ch, char *argument )
{
   echo_all_printf( AT_IMMORT, ECHOTAR_ALL, "The Voice of God says: %s", argument );
   return;
}

void echo_to_room( short AT_COLOR, ROOM_INDEX_DATA * room, char *argument )
{
   CHAR_DATA *vic;

   for( vic = room->first_person; vic; vic = vic->next_in_room )
   {
      if( MXP_ON( vic ) )
         send_to_char( MXP_TAG_SECURE, vic );

      set_char_color( AT_COLOR, vic );
      send_to_char( argument, vic );
      send_to_char( "\n\r", vic );
   }
}

CMDF do_recho( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   short color;

   set_char_color( AT_IMMORT, ch );

   if( IS_PLR_FLAG( ch, PLR_NO_EMOTE ) )
   {
      send_to_char( "You can't do that right now.\n\r", ch );
      return;
   }
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Recho what?\n\r", ch );
      return;
   }

   one_argument( argument, arg );

   if( ( color = get_color( argument ) ) )
   {
      argument = one_argument( argument, arg );
      echo_to_room( color, ch->in_room, argument );
   }
   else
      echo_to_room( AT_IMMORT, ch->in_room, argument );
}

ROOM_INDEX_DATA *find_location( CHAR_DATA * ch, char *arg )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj;

   if( is_number( arg ) )
      return get_room_index( atoi( arg ) );

   if( !str_cmp( arg, "pk" ) )   /* "Goto pk", "at pk", etc */
      return get_room_index( last_pkroom );

   if( ( victim = get_char_world( ch, arg ) ) != NULL )
      return victim->in_room;

   if( ( obj = get_obj_world( ch, arg ) ) != NULL )
      return obj->in_room;

   return NULL;
}

CMDF do_transfer( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   ROOM_INDEX_DATA *location, *original;
   DESCRIPTOR_DATA *d;
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Transfer whom (and where)?\n\r", ch );
      return;
   }
   if( !str_cmp( arg1, "all" ) )
   {
      for( d = first_descriptor; d; d = d->next )
      {
         if( d->connected == CON_PLAYING && d->character != ch && d->character->in_room
             && d->newstate != 2 && can_see( ch, d->character, TRUE ) )
         {
            char buf[MIL];
            snprintf( buf, MIL, "%s %s", d->character->name, argument );
            do_transfer( ch, buf );
         }
      }
      return;
   }

   /*
    * Thanks to Grodyn for the optional location parameter.
    */

   if( !argument || argument[0] == '\0' )
      location = ch->in_room;
   else
   {
      send_to_char( "You can only transfer to your occupied room.\n\r", ch );
      send_to_char( "If you need to transfer someone to a remote location, use the at command there to transfer them.\n\r",
                    ch );
      return;
   }

   if( ( victim = get_char_world( ch, arg1 ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      ch_printf( ch, "Are you feeling alright today %s?\n\r", ch->name );
      return;
   }

   if( ch->level < victim->level )
   {
      send_to_char( "A mystical force prevents your actions.\n\r", ch );
      return;
   }

   if( !victim->in_room )
   {
      send_to_char( "They have no physical location!\n\r", ch );
      return;
   }

   if( victim->fighting )
      stop_fighting( victim, TRUE );

   act( AT_MAGIC, "A swirling vortex arrives to pick up $n!", victim, NULL, NULL, TO_ROOM );
   original = victim->in_room;

   leave_map( victim, ch, location );

   act( AT_MAGIC, "A swirling vortex arrives, carrying $n!", victim, NULL, NULL, TO_ROOM );
   if( ch != victim )
      act( AT_IMMORT, "$n has sent a swirling vortex to transport you.", ch, NULL, victim, TO_VICT );
   send_to_char( "Ok.\n\r", ch );

   if( !IS_IMMORTAL( victim ) && !IS_NPC( victim ) && !in_hard_range( victim, location->area ) )
      send_to_char( "Warning: the player's level is not within the area's level range.\n\r", ch );

   return;
}

/*  Added atmob and atobj to reduce lag associated with at
 *  --Shaddai
 */
void atmob( CHAR_DATA * ch, CHAR_DATA * wch, char *argument )
{
   ROOM_INDEX_DATA *location, *original;
   short origmap, origx, origy;

   set_char_color( AT_IMMORT, ch );
   location = wch->in_room;

   if( is_ignoring( wch, ch ) )
   {
      send_to_char( "No such location.\n\r", ch );
      return;
   }

   if( room_is_private( location ) )
   {
      if( ch->level < sysdata.level_override_private )
      {
         send_to_char( "That room is private right now.\n\r", ch );
         return;
      }
      else
      {
         send_to_char( "Overriding private flag!\n\r", ch );
      }
   }

   if( IS_ROOM_FLAG( location, ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      send_to_char( "Go away! That room has been sealed for privacy!\n\r", ch );
      return;
   }

   origmap = ch->map;
   origx = ch->x;
   origy = ch->y;

   /*
    * Bunch of checks to make sure the "ator" is temporarily on the same grid as
    * * the "atee" - Samson
    */

   if( IS_ROOM_FLAG( location, ROOM_MAP ) && !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      SET_PLR_FLAG( ch, PLR_ONMAP );
      ch->map = wch->map;
      ch->x = wch->x;
      ch->y = wch->y;
   }
   else if( IS_ROOM_FLAG( location, ROOM_MAP ) && IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      ch->map = wch->map;
      ch->x = wch->x;
      ch->y = wch->y;
   }
   else if( !IS_ROOM_FLAG( location, ROOM_MAP ) && IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      REMOVE_PLR_FLAG( ch, PLR_ONMAP );
      ch->map = -1;
      ch->x = -1;
      ch->y = -1;
   }

   set_char_color( AT_PLAIN, ch );
   original = ch->in_room;
   char_from_room( ch );
   if( !char_to_room( ch, location ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   interpret( ch, argument );

   if( IS_PLR_FLAG( ch, PLR_ONMAP ) && !IS_ROOM_FLAG( original, ROOM_MAP ) )
      REMOVE_PLR_FLAG( ch, PLR_ONMAP );
   else if( !IS_PLR_FLAG( ch, PLR_ONMAP ) && IS_ROOM_FLAG( original, ROOM_MAP ) )
      SET_PLR_FLAG( ch, PLR_ONMAP );

   ch->map = origmap;
   ch->x = origx;
   ch->y = origy;

   if( !char_died( ch ) )
   {
      char_from_room( ch );
      if( !char_to_room( ch, original ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }
   return;
}

void atobj( CHAR_DATA * ch, OBJ_DATA * obj, char *argument )
{
   ROOM_INDEX_DATA *location, *original;
   short origmap, origx, origy;

   set_char_color( AT_IMMORT, ch );
   location = obj->in_room;

   if( room_is_private( location ) )
   {
      if( ch->level < sysdata.level_override_private )
      {
         send_to_char( "That room is private right now.\n\r", ch );
         return;
      }
      else
      {
         send_to_char( "Overriding private flag!\n\r", ch );
      }
   }

   if( IS_ROOM_FLAG( location, ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      send_to_char( "Go away! That room has been sealed for privacy!\n\r", ch );
      return;
   }

   origmap = ch->map;
   origx = ch->x;
   origy = ch->y;

   /*
    * Bunch of checks to make sure the imm is on the same grid as the object - Samson 
    */
   if( IS_ROOM_FLAG( location, ROOM_MAP ) && !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      SET_PLR_FLAG( ch, PLR_ONMAP );
      ch->map = obj->map;
      ch->x = obj->x;
      ch->y = obj->y;
   }
   else if( IS_ROOM_FLAG( location, ROOM_MAP ) && IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      ch->map = obj->map;
      ch->x = obj->x;
      ch->y = obj->y;
   }
   else if( !IS_ROOM_FLAG( location, ROOM_MAP ) && IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      REMOVE_PLR_FLAG( ch, PLR_ONMAP );
      ch->map = -1;
      ch->x = -1;
      ch->y = -1;
   }

   set_char_color( AT_PLAIN, ch );
   original = ch->in_room;
   char_from_room( ch );
   if( !char_to_room( ch, location ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   interpret( ch, argument );

   if( IS_PLR_FLAG( ch, PLR_ONMAP ) && !IS_ROOM_FLAG( original, ROOM_MAP ) )
      REMOVE_PLR_FLAG( ch, PLR_ONMAP );
   else if( !IS_PLR_FLAG( ch, PLR_ONMAP ) && IS_ROOM_FLAG( original, ROOM_MAP ) )
      SET_PLR_FLAG( ch, PLR_ONMAP );

   ch->map = origmap;
   ch->x = origx;
   ch->y = origy;

   if( !char_died( ch ) )
   {
      char_from_room( ch );
      if( !char_to_room( ch, original ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }
   return;
}

/* Smaug 1.02a at command restored by Samson 8-14-98 */
CMDF do_at( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   ROOM_INDEX_DATA *location, *original;
   CHAR_DATA *wch;
   OBJ_DATA *obj;
   short origmap, origx, origy;

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "At where what?\n\r", ch );
      return;
   }

   if( !is_number( arg ) )
   {
      if( ( wch = get_char_world( ch, arg ) ) != NULL && wch->in_room != NULL )
      {
         atmob( ch, wch, argument );
         return;
      }

      if( ( obj = get_obj_world( ch, arg ) ) != NULL && obj->in_room != NULL )
      {
         atobj( ch, obj, argument );
         return;
      }

      send_to_char( "No such mob or object.\n\r", ch );
      return;
   }

   if( ( location = find_location( ch, arg ) ) == NULL )
   {
      send_to_char( "No such location.\n\r", ch );
      return;
   }

   if( room_is_private( location ) )
   {
      if( ch->level < sysdata.level_override_private )
      {
         send_to_char( "That room is private right now.\n\r", ch );
         return;
      }
      else
      {
         send_to_char( "Overriding private flag!\n\r", ch );
      }
   }

   if( IS_ROOM_FLAG( location, ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      send_to_char( "Go away! That room has been sealed for privacy!\n\r", ch );
      return;
   }

   origmap = ch->map;
   origx = ch->x;
   origy = ch->y;

   /*
    * Since we're this far down, it's a given that the location isn't on a map since
    * a vnum had to be specified to get here. Therefore you want to be off map, and
    * at coords of -1, -1 to avoid problems - Samson 
    */
   REMOVE_PLR_FLAG( ch, PLR_ONMAP );
   ch->map = -1;
   ch->x = -1;
   ch->y = -1;

   original = ch->in_room;
   char_from_room( ch );
   if( !char_to_room( ch, location ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   interpret( ch, argument );

   /*
    * And even if you weren't on a map to begin with, this will still work fine 
    */
   if( IS_PLR_FLAG( ch, PLR_ONMAP ) && !IS_ROOM_FLAG( original, ROOM_MAP ) )
      REMOVE_PLR_FLAG( ch, PLR_ONMAP );
   else if( !IS_PLR_FLAG( ch, PLR_ONMAP ) && IS_ROOM_FLAG( original, ROOM_MAP ) )
      SET_PLR_FLAG( ch, PLR_ONMAP );

   ch->map = origmap;
   ch->x = origx;
   ch->y = origy;

   /*
    * See if 'ch' still exists before continuing!
    * Handles 'at XXXX quit' case.
    */
   for( wch = first_char; wch; wch = wch->next )
   {
      if( wch == ch )
      {
         char_from_room( ch );
         if( !char_to_room( ch, original ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         break;
      }
   }
   return;
}

CMDF do_rat( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   ROOM_INDEX_DATA *location, *original;
   int Start, End, vnum;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !arg1 || arg1[0] == '\0' || !arg2 || arg2[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax: rat <start> <end> <command>\n\r", ch );
      return;
   }

   Start = atoi( arg1 );
   End = atoi( arg2 );
   if( Start < 1 || End < Start || Start > End || Start == End || End > sysdata.maxvnum )
   {
      send_to_char( "Invalid range.\n\r", ch );
      return;
   }
   if( !str_cmp( argument, "quit" ) )
   {
      send_to_char( "I don't think so!\n\r", ch );
      return;
   }

   original = ch->in_room;
   for( vnum = Start; vnum <= End; vnum++ )
   {
      if( !( location = get_room_index( vnum ) ) )
         continue;

      char_from_room( ch );
      if( !char_to_room( ch, location ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      interpret( ch, argument );
   }

   char_from_room( ch );
   if( !char_to_room( ch, original ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   send_to_char( "Done.\n\r", ch );
   return;
}

/* Rewritten by Whir to be viewer friendly - 8/26/98 */
CMDF do_rstat( CHAR_DATA * ch, char *argument )
{
   ROOM_INDEX_DATA *location;
   OBJ_DATA *obj;
   CHAR_DATA *rch;
   EXIT_DATA *pexit;
   int cnt;
   static char *dir_text[] = { "n", "e", "s", "w", "u", "d", "ne", "nw", "se", "sw", "?" };

   if( !str_cmp( argument, "exits" ) )
   {
      location = ch->in_room;

      ch_printf( ch, "&wExits for room '&G%s.&w' vnum &G%d&w\n\r", location->name, location->vnum );

      cnt = 0;
      for( pexit = location->first_exit; pexit; pexit = pexit->next )
      {
         ch_printf( ch, "%2d) &G%2s&w to &G%-5d&w.  Key: &G%d  &wKeywords: '&G%s&w'  &wFlags: &G%s&w.\n\r",
                    ++cnt, dir_text[pexit->vdir], pexit->to_room ? pexit->to_room->vnum : 0,
                    pexit->key, pexit->keyword, ext_flag_string( &pexit->exit_info, ex_flags ) );

         ch_printf( ch, "Description: &G%s&wExit links back to vnum: &G%d  &wExit's RoomVnum: &G%d&w\n\r\n\r",
                    ( pexit->exitdesc && pexit->exitdesc[0] != '\0' ) ? pexit->exitdesc : "(NONE)\n\r",
                    pexit->rexit ? pexit->rexit->vnum : 0, pexit->rvnum );
      }
      return;
   }

   location = ( argument[0] == '\0' ) ? ch->in_room : find_location( ch, argument );

   if( !location )
   {
      send_to_char( "No such location.\n\r", ch );
      return;
   }

   if( ch->in_room != location && room_is_private( location ) )
   {
      if( get_trust( ch ) < LEVEL_GREATER )
      {
         send_to_char( "That room is private right now.\n\r", ch );
         return;
      }
      else
         send_to_char( "Overriding private flag!\n\r", ch );
   }

   ch_printf( ch, "&wName: &G%s\n\r&wArea: &G%s  ", location->name, location->area ? location->area->name : "None????" );
   ch_printf( ch, "&wFilename: &G%s\n\r", location->area ? location->area->filename : "None????" );
   ch_printf( ch, "&wVnum: &G%d&w  Light: &G%d&w  TeleDelay: &G%d&w  TeleVnum: &G%d&w  Tunnel: &G%d&w\n\r",
              location->vnum, location->light, location->tele_delay, location->tele_vnum, location->tunnel );

   ch_printf( ch, "Room flags: &G%s&w\n\r", ext_flag_string( &location->room_flags, r_flags ) );
   ch_printf( ch, "Sector type: &G%s&w\n\r", sect_types[location->sector_type] );

   ch_printf( ch, "Description:&w\n\r%s&w", location->roomdesc );

   /*
    * NiteDesc rstat added by Dracones 
    */
   if( location->nitedesc && location->nitedesc[0] != '\0' )
      ch_printf( ch, "NiteDesc:&w\n\r%s&w", location->nitedesc );

   if( location->first_extradesc )
   {
      EXTRA_DESCR_DATA *ed;

      send_to_char( "Extra description keywords:\n\r&G", ch );
      for( ed = location->first_extradesc; ed; ed = ed->next )
      {
         send_to_char( ed->keyword, ch );

         if( ed->next )
            send_to_char( " ", ch );
      }
      send_to_char( "&w'.\n\r\n\r", ch );
   }

   if( xIS_EMPTY( ch->in_room->progtypes ) )
      send_to_char( "Roomprogs: &GNone&w\n\r", ch );
   else
   {
      MPROG_DATA *mprg;

      send_to_char( "Roomprogs: &G", ch );

      for( mprg = ch->in_room->mudprogs; mprg; mprg = mprg->next )
         ch_printf( ch, "%s ", mprog_type_to_name( mprg->type ) );
      send_to_char( "&w\n\r", ch );
   }

   send_to_char( "Characters/Mobiles in the room:\n\r", ch );
   for( rch = location->first_person; rch; rch = rch->next_in_room )
   {
      if( can_see( ch, rch, FALSE ) )
         ch_printf( ch, "(&G%d&w)&G%s&w\n\r", ( IS_NPC( rch ) ? rch->pIndexData->vnum : 0 ), rch->name );
   }

   send_to_char( "\n\rObjects in the room:\n\r", ch );
   for( obj = location->first_content; obj; obj = obj->next_content )
      ch_printf( ch, "(&G%d&w)&G%s&w\n\r", obj->pIndexData->vnum, obj->name );

   send_to_char( "\n\r", ch );

   if( location->first_exit )
      send_to_char( "------------------- EXITS -------------------\n\r", ch );

   cnt = 0;
   for( pexit = location->first_exit; pexit; pexit = pexit->next )
   {
      ch_printf( ch, "%2d) &G%-2s &wto &G%-5d  &wKey: &G%-5d  &wKeywords: &G%s&w  Flags: &G%s&w.\n\r",
                 ++cnt, dir_text[pexit->vdir], pexit->to_room ? pexit->to_room->vnum : 0,
                 pexit->key, ( pexit->keyword && pexit->keyword[0] != '\0' ) ? pexit->keyword : "(none)",
                 ext_flag_string( &pexit->exit_info, ex_flags ) );
   }
   return;
}

/* Rewritten by Whir to be viewer friendly - 8/26/98 */
CMDF do_ostat( CHAR_DATA * ch, char *argument )
{
   char *suf;
   AFFECT_DATA *paf;
   OBJ_DATA *obj;
   short day;

   if( ( obj = get_obj_world( ch, argument ) ) == NULL )
   {
      send_to_char( "That object doesn't exist!\n\r", ch );
      return;
   }

   day = obj->day + 1;

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

   ch_printf( ch, "&w|Name   : &G%s&w\n\r", obj->name );

   ch_printf( ch, "|Short  : &G%s&w\n\r|Long   : &G%s&w\n\r", obj->short_descr,
              ( obj->objdesc && obj->objdesc[0] != '\0' ) ? obj->objdesc : "" );

   if( obj->action_desc && obj->action_desc[0] != '\0' )
      ch_printf( ch, "|Action : &G%s&w\n\r", obj->action_desc );

   ch_printf( ch, "|Area   : %s\n\r", obj->pIndexData->area ? obj->pIndexData->area->name : "(NONE)" );

   ch_printf( ch, "|Vnum   : &G%5d&w |Type  :     &G%9s&w |Count     : &G%3.3d&w |Gcount: &G%3.3d&w\n\r",
              obj->pIndexData->vnum, item_type_name( obj ), obj->pIndexData->count, obj->count );

   ch_printf( ch,
              "|Number : &G%2.2d&w/&G%2.2d&w |Weight:     &G%4.4d&w/&G%4.4d&w |Wear_loc  : &G%3.2d&w |Layers: &G%d&w\n\r", 1,
              get_obj_number( obj ), obj->weight, get_obj_weight( obj ), obj->wear_loc, obj->pIndexData->layers );

   ch_printf( ch, "|Cost   : &G%5d&w |Rent* : &G%6d&w |Rent  : &G%6d&w |Timer     : &G%d&w |Ego   : &G%d&w\n\r",
              obj->cost, obj->pIndexData->rent, obj->rent, obj->timer, item_ego( obj ) );

   ch_printf( ch, "|In room: &G%5d&w |In obj: &G%s&w |Level :  &G%5d&w |Limit: &G%5d&w\n\r",
              obj->in_room == NULL ? 0 : obj->in_room->vnum,
              obj->in_obj == NULL ? "(NONE)" : obj->in_obj->short_descr, obj->level, obj->pIndexData->limit );

   ch_printf( ch, "|On map       : &G%s&w\n\r", IS_OBJ_FLAG( obj, ITEM_ONMAP ) ? map_names[obj->map] : "(NONE)" );

   ch_printf( ch, "|Object Coords: &G%d %d&w\n\r", obj->x, obj->y );

   ch_printf( ch, "|Wear flags   : &G%s&w\n\r", flag_string( obj->wear_flags, w_flags ) );

   ch_printf( ch, "|Extra flags  : &G%s&w\n\r", ext_flag_string( &obj->extra_flags, o_flags ) );

   ch_printf( ch, "|Carried by   : &G%s&w\n\r", obj->carried_by == NULL ? "(NONE)" : obj->carried_by->name );
   ch_printf( ch, "|Prizeowner   : &G%s&w\n\r", obj->owner == NULL ? "(NONE)" : obj->owner );
   ch_printf( ch, "|Seller       : &G%s&w\n\r", obj->seller == NULL ? "(NONE)" : obj->seller );
   ch_printf( ch, "|Buyer        : &G%s&w\n\r", obj->buyer == NULL ? "(NONE)" : obj->buyer );
   ch_printf( ch, "|Current bid  : &G%d&w\n\r", obj->bid );

   if( obj->year == 0 )
      send_to_char( "|Scheduled donation date: &G(NONE)&w\n\r", ch );
   else
      ch_printf( ch, "|Scheduled donation date: &G %d%s day in the Month of %s, in the year %d.&w", day, suf,
                 month_name[obj->month], obj->year );

   ch_printf( ch, "|Index Values : &G%2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d&w\n\r",
              obj->pIndexData->value[0], obj->pIndexData->value[1], obj->pIndexData->value[2], obj->pIndexData->value[3],
              obj->pIndexData->value[4], obj->pIndexData->value[5], obj->pIndexData->value[6], obj->pIndexData->value[7],
              obj->pIndexData->value[8], obj->pIndexData->value[9], obj->pIndexData->value[10] );

   ch_printf( ch, "|Object Values: &G%2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d&w\n\r",
              obj->value[0], obj->value[1], obj->value[2], obj->value[3],
              obj->value[4], obj->value[5], obj->value[6], obj->value[7], obj->value[8], obj->value[9], obj->value[10] );

   if( obj->pIndexData->first_extradesc )
   {
      EXTRA_DESCR_DATA *ed;

      send_to_char( "|Primary description keywords:   '", ch );
      for( ed = obj->pIndexData->first_extradesc; ed; ed = ed->next )
      {
         send_to_char( ed->keyword, ch );
         if( ed->next )
            send_to_char( " ", ch );
      }
      send_to_char( "'.\n\r", ch );
   }
   if( obj->first_extradesc )
   {
      EXTRA_DESCR_DATA *ed;

      send_to_char( "|Secondary description keywords: '", ch );
      for( ed = obj->first_extradesc; ed; ed = ed->next )
      {
         send_to_char( ed->keyword, ch );
         if( ed->next )
            send_to_char( " ", ch );
      }
      send_to_char( "'.\n\r", ch );
   }

   if( xIS_EMPTY( obj->pIndexData->progtypes ) )
      send_to_char( "|Objprogs     : &GNone&w\n\r", ch );
   else
   {
      MPROG_DATA *mprg;

      send_to_char( "|Objprogs     : &G", ch );

      for( mprg = obj->pIndexData->mudprogs; mprg; mprg = mprg->next )
         ch_printf( ch, "%s ", mprog_type_to_name( mprg->type ) );
      send_to_char( "&w\n\r", ch );
   }

   /*
    * Rather useful and cool function provided by Druid to decipher those values 
    */
   send_to_char( "\n\r", ch );
   ostat_plus( ch, obj, FALSE );
   send_to_char( "\n\r", ch );

   for( paf = obj->first_affect; paf; paf = paf->next )
      showaffect( ch, paf );

   for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
      showaffect( ch, paf );

   return;
}

CMDF do_moblog( CHAR_DATA * ch, char *argument )
{
   set_char_color( AT_LOG, ch );
   send_to_char( "\n\r[Date_|_Time]  Current moblog:\n\r", ch );
   show_file( ch, MOBLOG_FILE );
   return;
}

/* do_mstat rewritten by Whir to be view-friendly 8/18/98 */
CMDF do_mstat( CHAR_DATA * ch, char *argument )
{
   AFFECT_DATA *paf;
   CHAR_DATA *victim;
   SKILLTYPE *skill;
   int iLang = 0;
   char lbuf[256];

   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( !IS_NPC( victim ) )
   {
      ch_printf( ch, "&w|Name  : &G%10s &w|Clan  : &G%10s &w|PKill : &G%10s &w|Room  : &G%d&w\n\r",
                 victim->name, ( victim->pcdata->clan == NULL ) ? "(NONE)" : victim->pcdata->clan->name,
                 IS_PCFLAG( victim, PCFLAG_DEADLY ) ? "Yes" : "No", victim->in_room->vnum );

      ch_printf( ch, "|Level : &G%10d &w|Trust : &G%10d &w|Sex   : &G%10s &w|Gold  : &G%d&w\n\r",
                 victim->level, victim->trust, npc_sex[victim->sex], victim->gold );

      ch_printf( ch, "|STR   : &G%10d &w|HPs   : &G%10d &w|MaxHPs: &G%10d &w\n\r",
                 get_curr_str( victim ), victim->hit, victim->max_hit );

      ch_printf( ch, "|INT   : &G%10d &w|Mana  : &G%10d &w|MaxMan: &G%10d &w|Pos   : &G%s&w\n\r",
                 get_curr_int( victim ), victim->mana, victim->max_mana, npc_position[victim->position] );

      ch_printf( ch, "|WIS   : &G%10d &w|Move  : &G%10d &w|MaxMov: &G%10d &w|Wimpy : &G%d&w\n\r",
                 get_curr_wis( victim ), victim->move, victim->max_move, victim->wimpy );

      ch_printf( ch, "|DEX   : &G%10d &w|AC    : &G%10d &w|Align : &G%10d &w|Favor : &G%d&w\n\r",
                 get_curr_dex( victim ), GET_AC( victim ), victim->alignment, victim->pcdata->favor );

      ch_printf( ch, "|CON   : &G%10d &w|+Hit  : &G%10d &w|+Dam  : &G%10d &w|Pracs : &G%d&w\n\r",
                 get_curr_con( victim ), GET_HITROLL( victim ), GET_DAMROLL( victim ), victim->pcdata->practice );

      ch_printf( ch, "|CHA   : &G%10d &w|BseAge: &G%10d &w|Agemod: &G%10d &w|\n\r",
                 get_curr_cha( victim ), victim->pcdata->age, victim->pcdata->age_bonus );

      if( victim->pcdata->condition[COND_THIRST] == -1 )
         mudstrlcpy( lbuf, "|Thirst: &G    Immune &w", 256 );
      else
         snprintf( lbuf, 256, "|Thirst: &G%10d &w", victim->pcdata->condition[COND_THIRST] );
      if( victim->pcdata->condition[COND_FULL] == -1 )
         mudstrlcat( lbuf, "|Hunger: &G    Immune &w", 256 );
      else
         snprintf( lbuf + strlen( lbuf ), 256 - strlen( lbuf ), "|Hunger: &G%10d &w", victim->pcdata->condition[COND_FULL] );
      ch_printf( ch, "|LCK   : &G%10d &w%s|Drunk : &G%d &w\n\r",
                 get_curr_lck( victim ), lbuf, victim->pcdata->condition[COND_DRUNK] );

      ch_printf( ch, "|Class :&G%11s &w|Mental: &G%10d &w|#Attks: &G%10f &w|Interface: &G%s &w\n\r",
                 capitalize( get_class( victim ) ), victim->mental_state,
                 victim->numattacks, interfaces[victim->pcdata->interface] );

      ch_printf( ch, "|Race  : &G%10s &w|Barehand: &G%d&wd&G%d&w+&G%d&w\n\r",
                 capitalize( get_race( victim ) ), victim->barenumdie, victim->baresizedie, GET_DAMROLL( victim ) );

      ch_printf( ch, "|Deity :&G%11s &w|Authed:&G%11s &w|SF    :&G%11d &w|                   \n\r",
                 ( victim->pcdata->deity == NULL ) ? "(NONE)" : victim->pcdata->deity->name,
                 ( victim->pcdata->authed_by
                   && victim->pcdata->authed_by[0] != '\0' ) ? victim->pcdata->authed_by : "Unknown", victim->spellfail );

      ch_printf( ch, "|Map   : &G%10s &w|Coords: &G%d %d&w\n\r",
                 IS_PLR_FLAG( victim, PLR_ONMAP ) ? map_names[victim->map] : "(NONE)", victim->x, victim->y );

      ch_printf( ch, "|Master: &G%10s &w|Leader: &G%s&w\n\r", victim->master ? victim->master->name : "(NONE)",
                 victim->leader ? victim->leader->name : "(NONE)" );

      ch_printf( ch, "|Saves : ---------- | ----------------- | ----------------- | -----------------\n\r" );

      ch_printf( ch, "|Poison: &G%10d &w|Para  : &G%10d &w|Wands : &G%10d &w|Spell : &G%d&w\n\r",
                 victim->saving_poison_death, victim->saving_para_petri, victim->saving_wand, victim->saving_spell_staff );

      ch_printf( ch, "|Death : &G%10d &w|Petri : &G%10d &w|Breath: &G%10d &w|Staves: &G%d&w\n\r",
                 victim->saving_poison_death, victim->saving_para_petri, victim->saving_breath, victim->saving_spell_staff );

      if( victim->desc )
      {
         ch_printf( ch, "|Player's Terminal Program: &G%s&w\n\r", victim->desc->client );
         ch_printf( ch, "|Player's Terminal Support: &GMCCP[%s]  &GMSP[%s]  &GMXP[%s]&w\n\r",
                    victim->desc->can_compress ? "&wX&G" : " ",
                    victim->desc->msp_detected ? "&wX&G" : " ", victim->desc->mxp_detected ? "&wX&G" : " " );

         ch_printf( ch, "|Terminal Support In Use  : &GMCCP[%s]  &GMSP[%s]  &GMXP[%s]&w\n\r",
                    victim->desc->can_compress ? "&wX&G" : " ",
                    MSP_ON( victim ) ? "&wX&G" : " ", MXP_ON( victim ) ? "&wX&G" : " " );
      }

      ch_printf( ch, "|Player Flags: &G%s&w\n\r", !&victim->act ? "(NONE)" : ext_flag_string( &victim->act, plr_flags ) );
      ch_printf( ch, "|PC Flags    : &G%s&w\n\r",
                 !victim->pcdata->flags ? "(NONE)" : flag_string( victim->pcdata->flags, pc_flags ) );

      send_to_char( "|Languages: ", ch );
      for( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; ++iLang )
         if( knows_language( victim, lang_array[iLang], victim ) )
         {
            if( lang_array[iLang] & victim->speaking )
               set_char_color( AT_SCORE3, ch );
            send_to_char( lang_names[iLang], ch );
            send_to_char( " ", ch );
            set_char_color( AT_SCORE, ch );
         }
      send_to_char( "\n\r", ch );

      ch_printf( ch, "&w|Affected By : &G%s&w\n\r",
                 !&victim->affected_by ? "(NONE)" : affect_bit_name( &victim->affected_by ) );

      ch_printf( ch, "|Bestowments : &G%s&w\n\r", !victim->pcdata->bestowments ? "(NONE)" : victim->pcdata->bestowments );

      ch_printf( ch, "|Resistances : &G%s&w\n\r",
                 !&victim->resistant ? "(NONE)" : ext_flag_string( &victim->resistant, ris_flags ) );

      ch_printf( ch, "|Immunities  : &G%s&w\n\r",
                 !&victim->immune ? "(NONE)" : ext_flag_string( &victim->immune, ris_flags ) );

      ch_printf( ch, "|Suscepts    : &G%s&w\n\r",
                 !&victim->susceptible ? "(NONE)" : ext_flag_string( &victim->susceptible, ris_flags ) );

      ch_printf( ch, "|Absorbs     : &G%s&w\n\r",
                 !&victim->absorb ? "(NONE)" : ext_flag_string( &victim->absorb, ris_flags ) );

      for( paf = victim->first_affect; paf; paf = paf->next )
         if( ( skill = get_skilltype( paf->type ) ) != NULL )
         {
            char loc[MIL];

            if( paf->location == APPLY_AFFECT || paf->location == APPLY_EXT_AFFECT )
            {
               mudstrlcpy( loc, a_flags[paf->modifier], MIL );
            }
            else if( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE || paf->location == APPLY_ABSORB || paf->location == APPLY_SUSCEPTIBLE )
            {
               mudstrlcpy( loc, ris_flags[paf->modifier], MIL );
            }
            else
               snprintf( loc, MIL, "%d", paf->modifier );
            ch_printf( ch, "|%s: '&G%s&w' modifies &G%s&w by &G%s&w for &G%d&w rounds with bits &G%s&w.\n\r",
                       skill_tname[skill->type], skill->name, a_types[paf->location],
                       loc, paf->duration, a_flags[paf->bit] );
         }
      return;
   }
   else
   {
      ch_printf( ch, "&w|Name  : &G%-50s &w|Room  : &G%d&w\n\r", victim->name, victim->in_room->vnum );
      ch_printf( ch, "|Area  : &G%-50s &w\n\r", victim->pIndexData->area ? victim->pIndexData->area->name : "(NONE)" );

      ch_printf( ch, "|Level : &G%10d &w|Vnum  : &G%10d &w|Sex   : &G%10s &w|Gold  : &G%d&w\n\r",
                 victim->level, victim->pIndexData->vnum, npc_sex[victim->sex], victim->gold );

      ch_printf( ch, "|STR   : &G%10d &w|HPs   : &G%10d &w|MaxHPs: &G%10d &w|Exp   : &G%d&w\n\r",
                 get_curr_str( victim ), victim->hit, victim->max_hit, victim->exp );

      ch_printf( ch, "|INT   : &G%10d &w|Mana  : &G%10d &w|MaxMan: &G%10d &w|Pos   : &G%s&w\n\r",
                 get_curr_int( victim ), victim->mana, victim->max_mana, npc_position[victim->position] );

      ch_printf( ch, "|WIS   : &G%10d &w|Move  : &G%10d &w|MaxMov: &G%10d &w|H.D.  : &G%d&wd&G%d&w+&G%d&w\n\r",
                 get_curr_wis( victim ), victim->move, victim->max_move, victim->pIndexData->hitnodice,
                 victim->pIndexData->hitsizedice, victim->pIndexData->hitplus );

      ch_printf( ch, "|DEX   : &G%10d &w|AC    : &G%10d &w|Align : &G%10d &w|D.D.  : &G%d&wd&G%d&w+&G%d&w\n\r",
                 get_curr_dex( victim ), GET_AC( victim ), victim->alignment, victim->pIndexData->damnodice,
                 victim->pIndexData->damsizedice, victim->pIndexData->damplus );

      ch_printf( ch, "|CON   : &G%10d &w|Thac0 : &G%10d &w|+Hit  : &G%10d &w|+Dam  : &G%d&w\n\r",
                 get_curr_con( victim ), calc_thac0( victim, NULL, 0 ), GET_HITROLL( victim ), GET_DAMROLL( victim ) );

      ch_printf( ch, "|CHA   : &G%10d &w|Count : &G%10d &w|Timer : &G%10d&w\n\r",
                 get_curr_cha( victim ), victim->pIndexData->count, victim->timer );

      ch_printf( ch, "|LCK   : &G%10d &w|#Attks: &G%10f &w|Thac0*: &G%10d &w|Exp*  : &G%d&w\n\r",
                 get_curr_lck( victim ), victim->numattacks, victim->mobthac0, victim->pIndexData->exp );

      ch_printf( ch, "|Class : &G%10s &w|Master: &G%s&w\n\r",
                 capitalize( npc_class[victim->Class] ), victim->master ? victim->master->name : "(NONE)" );

      ch_printf( ch, "|Race  : &G%10s &w|Leader: &G%s&w\n\r",
                 capitalize( npc_race[victim->race] ), victim->leader ? victim->leader->name : "(NONE)" );

      ch_printf( ch, "|Map   : &G%10s &w|Coords: &G%d %d    &w|Native Sector: &G%s&w\n\r",
                 IS_ACT_FLAG( victim, ACT_ONMAP ) ? map_names[victim->map] : "(NONE)",
                 victim->x, victim->y, victim->sector < 0 ? "Not set yet" : sect_types[victim->sector] );

      ch_printf( ch, "|Saves : ---------- | ----------------- | ----------------- | -----------------\n\r" );

      ch_printf( ch, "|Poison: &G%10d &w|Para  : &G%10d &w|Wands : &G%10d &w|Spell : &G%d&w\n\r",
                 victim->saving_poison_death, victim->saving_para_petri, victim->saving_wand, victim->saving_spell_staff );

      ch_printf( ch, "|Death : &G%10d &w|Petri : &G%10d &w|Breath: &G%10d &w|Staves: &G%d&w\n\r",
                 victim->saving_poison_death, victim->saving_para_petri, victim->saving_breath, victim->saving_spell_staff );

      ch_printf( ch, "|Short : &G%s&w\n\r",
                 ( victim->short_descr && victim->short_descr[0] != '\0' ) ? victim->short_descr : "(NONE)" );

      ch_printf( ch, "|Long  : &G%s&w\n\r",
                 ( victim->long_descr && victim->long_descr[0] != '\0' ) ? victim->long_descr : "(NONE)" );

      send_to_char( "|Languages: ", ch );
      for( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; ++iLang )
         if( knows_language( victim, lang_array[iLang], victim ) || ( IS_NPC( ch ) && victim->speaks == 0 ) )
         {
            if( lang_array[iLang] & victim->speaking )
               set_char_color( AT_SCORE3, ch );
            send_to_char( lang_names[iLang], ch );
            send_to_char( " ", ch );
            set_char_color( AT_SCORE, ch );
         }
      send_to_char( "\n\r", ch );

      ch_printf( ch, "&w|Act Flags   : &G%s&w\n\r", ext_flag_string( &victim->act, act_flags ) );

      ch_printf( ch, "|Affected By : &G%s&w\n\r",
                 !&victim->affected_by ? "(NONE)" : affect_bit_name( &victim->affected_by ) );

      ch_printf( ch, "|Mob Spec Fun: &G%s&w\n\r", victim->spec_funname ? victim->spec_funname : "(NONE)" );

      ch_printf( ch, "|Body Parts  : &G%s&w\n\r", flag_string( victim->xflags, part_flags ) );

      ch_printf( ch, "|Resistances : &G%s&w\n\r",
                 !&victim->resistant ? "(NONE)" : ext_flag_string( &victim->resistant, ris_flags ) );

      ch_printf( ch, "|Immunities  : &G%s&w\n\r",
                 !&victim->immune ? "(NONE)" : ext_flag_string( &victim->immune, ris_flags ) );

      ch_printf( ch, "|Suscepts    : &G%s&w\n\r",
                 !&victim->susceptible ? "(NONE)" : ext_flag_string( &victim->susceptible, ris_flags ) );

      ch_printf( ch, "|Absorbs     : &G%s&w\n\r",
                 !&victim->absorb ? "(NONE)" : ext_flag_string( &victim->absorb, ris_flags ) );

      ch_printf( ch, "|Attacks     : &G%s&w\n\r", ext_flag_string( &victim->attacks, attack_flags ) );

      ch_printf( ch, "|Defenses    : &G%s&w\n\r", ext_flag_string( &victim->defenses, defense_flags ) );

      if( xIS_EMPTY( victim->pIndexData->progtypes ) )
         send_to_char( "|Mobprogs    : &GNone&w\n\r", ch );
      else
      {
         MPROG_DATA *mprg;

         send_to_char( "|Mobprogs    : &G", ch );

         for( mprg = victim->pIndexData->mudprogs; mprg; mprg = mprg->next )
            ch_printf( ch, "%s ", mprog_type_to_name( mprg->type ) );
         send_to_char( "&w\n\r", ch );
      }

      for( paf = victim->first_affect; paf; paf = paf->next )
         if( ( skill = get_skilltype( paf->type ) ) != NULL )
         {
            char loc[MIL];

            if( paf->location == APPLY_AFFECT || paf->location == APPLY_EXT_AFFECT )
            {
               mudstrlcpy( loc, a_flags[paf->modifier], MIL );
            }
            else if( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE || paf->location == APPLY_ABSORB || paf->location == APPLY_SUSCEPTIBLE )
            {
               mudstrlcpy( loc, ris_flags[paf->modifier], MIL );
            }
            else
               snprintf( loc, MIL, "%d", paf->modifier );
            ch_printf( ch, "|%s: '&G%s&w' modifies &G%s&w by &G%s&w for &G%d&w rounds with bits &G%s&w.\n\r",
                       skill_tname[skill->type], skill->name, a_types[paf->location],
                       loc, paf->duration, a_flags[paf->bit] );
         }
      return;
   }
}

/* Shard-like consolidated stat command - Samson 3-21-98 */
CMDF do_stat( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   char arg[MIL];

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Syntax:\n\r", ch );
      if( ch->level >= LEVEL_DEMI )
         send_to_char( "stat mob <vnum or keyword>\n\r", ch );
      if( ch->level >= LEVEL_SAVIOR )
         send_to_char( "stat obj <vnum or keyword>\n\r", ch );
      send_to_char( "stat room <vnum>\n\r", ch );
      if( ch->level >= LEVEL_DEMI )
         send_to_char( "stat player <name>", ch );
      return;
   }

   if( ( !str_cmp( arg, "mob" ) || !str_cmp( arg, "m" ) ) && ch->level >= LEVEL_DEMI )
   {
      if( ( victim = get_char_world( ch, argument ) ) == NULL )
      {
         send_to_char( "No such mob is loaded.\n\r", ch );
         return;
      }

      if( !IS_NPC( victim ) )
      {
         ch_printf( ch, "%s is not a mob!\n\r", victim->name );
         return;
      }

      do_mstat( ch, argument );
      return;
   }

   if( ( !str_cmp( arg, "player" ) || !str_cmp( arg, "p" ) ) && ch->level >= LEVEL_DEMI )
   {
      if( ( victim = get_char_world( ch, argument ) ) == NULL )
      {
         send_to_char( "No such player is online.\n\r", ch );
         return;
      }

      if( IS_NPC( victim ) )
      {
         ch_printf( ch, "%s is not a player!\n\r", argument );
         return;
      }
      do_mstat( ch, argument );
      return;
   }

   if( ( !str_cmp( arg, "obj" ) || !str_cmp( arg, "o" ) ) && ch->level >= LEVEL_SAVIOR )
   {
      do_ostat( ch, argument );
      return;
   }

   if( !str_cmp( arg, "room" ) || !str_cmp( arg, "r" ) )
   {
      do_rstat( ch, argument );
      return;
   }
   /*
    * echo syntax 
    */
   do_stat( ch, "" );
}

CMDF do_mfind( CHAR_DATA * ch, char *argument )
{
   MOB_INDEX_DATA *pMobIndex;
   int hash, nMatch;
   bool fAll;

   set_pager_color( AT_PLAIN, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Mfind whom?\n\r", ch );
      return;
   }

   fAll = !str_cmp( argument, "all" );
   nMatch = 0;

   for( hash = 0; hash < MAX_KEY_HASH; hash++ )
      for( pMobIndex = mob_index_hash[hash]; pMobIndex; pMobIndex = pMobIndex->next )
         if( fAll || nifty_is_name( argument, pMobIndex->player_name ) )
         {
            nMatch++;
            pager_printf( ch, "[%5d] %s\n\r", pMobIndex->vnum, capitalize( pMobIndex->short_descr ) );
         }

   if( nMatch )
      pager_printf( ch, "Number of matches: %d\n", nMatch );
   else
      send_to_char( "Nothing like that in hell, earth, or heaven.\n\r", ch );
   return;
}

CMDF do_rfind( CHAR_DATA * ch, char *argument )
{
   ROOM_INDEX_DATA *pRoom;
   int hash, nMatch;
   bool fAll;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Rfind what?\n\r", ch );
      return;
   }

   set_pager_color( AT_PLAIN, ch );
   fAll = !str_cmp( argument, "all" );
   nMatch = 0;

   for( hash = 0; hash < MAX_KEY_HASH; hash++ )
      for( pRoom = room_index_hash[hash]; pRoom; pRoom = pRoom->next )
         if( fAll || nifty_is_name_prefix( argument, pRoom->name ) )
         {
            nMatch++;
            pager_printf( ch, "[%5d] %s\n\r", pRoom->vnum, pRoom->name );
         }

   if( nMatch )
      pager_printf( ch, "Number of matches: %d\n", nMatch );
   else
      send_to_char( "Nowhere like that in hell, earth, or heaven.\n\r", ch );

   return;
}

CMDF do_ofind( CHAR_DATA * ch, char *argument )
{
   OBJ_INDEX_DATA *pObjIndex;
   int hash, nMatch;
   bool fAll;

   set_pager_color( AT_PLAIN, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Ofind what?\n\r", ch );
      return;
   }

   fAll = !str_cmp( argument, "all" );
   nMatch = 0;

   for( hash = 0; hash < MAX_KEY_HASH; hash++ )
      for( pObjIndex = obj_index_hash[hash]; pObjIndex; pObjIndex = pObjIndex->next )
         if( fAll || nifty_is_name( argument, pObjIndex->name ) )
         {
            nMatch++;
            pager_printf( ch, "[%5d] %s\n\r", pObjIndex->vnum, capitalize( pObjIndex->short_descr ) );
         }

   if( nMatch )
      pager_printf( ch, "Number of matches: %d\n", nMatch );
   else
      send_to_char( "Nothing like that in hell, earth, or heaven.\n\r", ch );
   return;
}

/*****
 * Oftype: Object find Type
 * Find object matching a certain type
 *****/
void find_oftype( CHAR_DATA * ch, char *argument )
{
   OBJ_INDEX_DATA *pObjIndex;
   int hash, nMatch, type;

   set_pager_color( AT_PLAIN, ch );

   nMatch = 0;

   /*
    * My God, now isn't this MUCH nicer than what was here before? - Samson 9-18-03 
    */
   type = get_otype( argument );
   if( type < 0 )
   {
      ch_printf( ch, "%s is an invalid item type.\n\r", argument );
      return;
   }

   for( hash = 0; hash < MAX_KEY_HASH; hash++ )
      for( pObjIndex = obj_index_hash[hash]; pObjIndex; pObjIndex = pObjIndex->next )
         if( type == pObjIndex->item_type )
         {
            nMatch++;
            pager_printf( ch, "[%5d] %s\n\r", pObjIndex->vnum, capitalize( pObjIndex->short_descr ) );
         }

   if( nMatch )
      pager_printf( ch, "Number of matches: %d\n", nMatch );
   else
      send_to_char( "Sorry, no matching item types found.\n\r", ch );
   return;
}

/* Consolidated find command 3-21-98 (SLAY DWIP) */
CMDF do_find( CHAR_DATA * ch, char *argument )
{
   char arg[MIL], arg2[MIL];

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( arg[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Syntax:\n\r", ch );
      if( ch->level >= LEVEL_DEMI )
         send_to_char( "find mob <keyword>\n\r", ch );
      send_to_char( "find obj <keyword>\n\r", ch );
      send_to_char( "find obj type <item type>\n\r", ch );
      return;
   }

   if( ( !str_cmp( arg, "mob" ) || !str_cmp( arg, "m" ) ) && ch->level >= LEVEL_DEMI )
   {
      do_mfind( ch, arg2 );
      return;
   }

   if( !str_cmp( arg, "obj" ) || !str_cmp( arg, "o" ) )
   {
      if( str_cmp( arg2, "type" ) )
         do_ofind( ch, arg2 );
      else
      {
         if( !argument || argument[0] == '\0' )
            do_find( ch, "" );
         else
            find_oftype( ch, argument );
      }
      return;
   }
   /*
    * echo syntax 
    */
   do_find( ch, "" );
}

CMDF do_mwhere( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   bool found;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Mwhere whom?\n\r", ch );
      return;
   }

   found = FALSE;
   for( victim = first_char; victim; victim = victim->next )
   {
      if( IS_NPC( victim ) && victim->in_room && nifty_is_name( argument, victim->name ) )
      {
         found = TRUE;
         if( IS_ACT_FLAG( victim, ACT_ONMAP ) )
            pager_printf( ch, "&Y[&W%5d&Y] &G%-28s &YOverland: &C%s %d %d\n\r",
                          victim->pIndexData->vnum, victim->short_descr, map_names[victim->map], victim->x, victim->y );
         else
            pager_printf( ch, "&Y[&W%5d&Y] &G%-28s &Y[&W%5d&Y] &C%s\n\r",
                          victim->pIndexData->vnum, victim->short_descr, victim->in_room->vnum, victim->in_room->name );
      }
   }
   if( !found )
      act( AT_PLAIN, "You didn't find any $T.", ch, NULL, argument, TO_CHAR );
   return;
}

/* Added 'show' argument for lowbie imms without ostat -- Blodkai */
/* Made show the default action :) Shaddai */
/* Trimmed size, added vict info, put lipstick on the pig -- Blod */
CMDF do_bodybag( CHAR_DATA * ch, char *argument )
{
   char buf2[MSL], buf3[MSL], arg1[MIL], arg2[MIL];
   CHAR_DATA *owner;
   OBJ_DATA *obj;
   bool found = FALSE, bag = FALSE;

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "&PSyntax:  bodybag <character> | bodybag <character> yes/bag/now\n\r", ch );
      return;
   }

   mudstrlcpy( buf3, " ", MSL );
   snprintf( buf2, MSL, "the corpse of %s", arg1 );
   argument = one_argument( argument, arg2 );

   if( arg2[0] != '\0' && ( str_cmp( arg2, "yes" ) && str_cmp( arg2, "bag" ) && str_cmp( arg2, "now" ) ) )
   {
      send_to_char( "\n\r&PSyntax:  bodybag <character> | bodybag <character> yes/bag/now\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "yes" ) || !str_cmp( arg2, "bag" ) || !str_cmp( arg2, "now" ) )
      bag = TRUE;

   pager_printf( ch, "\n\r&P%s remains of %s ... ", bag ? "Retrieving" : "Searching for", capitalize( arg1 ) );

   for( obj = first_object; obj; obj = obj->next )
   {
      if( obj->in_room && !str_cmp( buf2, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
      {
         send_to_pager( "\n\r", ch );
         found = TRUE;
         pager_printf( ch, "&P%s:  %s%-12.12s   &PIn:  &w%-22.22s  &P[&w%5d&P]   &PTimer:  %s%2d",
                       bag ? "Bagging" : "Corpse",
                       bag ? "&R" : "&w", capitalize( arg1 ), obj->in_room->area->name, obj->in_room->vnum,
                       obj->timer < 1 ? "&w" : obj->timer < 5 ? "&R" : obj->timer < 10 ? "&Y" : "&w", obj->timer );
         if( bag )
         {
            obj_from_room( obj );
            obj = obj_to_char( obj, ch );
            obj->timer = -1;
            save_char_obj( ch );
         }
      }
   }
   if( !found )
   {
      send_to_pager( "&Pno corpse was found.\n\r", ch );
      return;
   }
   send_to_pager( "\n\r", ch );
   for( owner = first_char; owner; owner = owner->next )
   {
      if( IS_NPC( owner ) )
         continue;
      if( can_see( ch, owner, TRUE ) && !str_cmp( arg1, owner->name ) )
         break;
   }
   if( owner == NULL )
   {
      pager_printf( ch, "&P%s is not currently online.\n\r", capitalize( arg1 ) );
      return;
   }
   if( owner->pcdata->deity )
      pager_printf( ch, "&P%s (%d) has %d favor with %s (needed to supplicate: %d)\n\r",
                    owner->name, owner->level, owner->pcdata->favor, owner->pcdata->deity->name,
                    owner->pcdata->deity->scorpse );
   else
      pager_printf( ch, "&P%s (%d) has no deity.\n\r", owner->name, owner->level );
   return;
}

/* New owhere by Altrag, 03/14/96 */
CMDF do_owhere( CHAR_DATA * ch, char *argument )
{
   char buf[MSL], arg[MIL], arg1[MIL];
   OBJ_DATA *obj;
   bool found;
   int icnt = 0;

   argument = one_argument( argument, arg );
   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Owhere what?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   if( arg1[0] != '\0' && !str_prefix( arg1, "nesthunt" ) )
   {
      if( !( obj = get_obj_world( ch, arg ) ) )
      {
         send_to_char( "Nesthunt for what object?\n\r", ch );
         return;
      }
      for( ; obj->in_obj; obj = obj->in_obj )
      {
         pager_printf( ch, "&Y[&W%5d&Y] &G%-28s &Cin object &Y[&W%5d&Y] &C%s\n\r",
                       obj->pIndexData->vnum, obj_short( obj ), obj->in_obj->pIndexData->vnum, obj->in_obj->short_descr );
         ++icnt;
      }
      snprintf( buf, MSL, "&Y[&W%5d&Y] &G%-28s ", obj->pIndexData->vnum, obj_short( obj ) );
      if( obj->carried_by )
         snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Cinvent   &Y[&W%5d&Y] &C%s\n\r",
                   ( IS_NPC( obj->carried_by ) ? obj->carried_by->pIndexData->vnum : 0 ), PERS( obj->carried_by, ch,
                                                                                                TRUE ) );
      else if( obj->in_room )
      {
         if( IS_OBJ_FLAG( obj, ITEM_ONMAP ) )
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Coverland &Y[&W%s&Y] &C%d %d\n\r",
                      map_names[obj->map], obj->x, obj->y );
         else
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Croom     &Y[&W%5d&Y] &C%s\n\r",
                      obj->in_room->vnum, obj->in_room->name );
      }
      else if( obj->in_obj )
      {
         bug( "%s", "do_owhere: obj->in_obj after NULL!" );
         mudstrlcat( buf, "object??\n\r", MSL );
      }
      else
      {
         bug( "%s", "do_owhere: object doesnt have location!" );
         mudstrlcat( buf, "nowhere??\n\r", MSL );
      }
      send_to_pager( buf, ch );
      ++icnt;
      pager_printf( ch, "Nested %d levels deep.\n\r", icnt );
      return;
   }

   found = FALSE;
   for( obj = first_object; obj; obj = obj->next )
   {
      if( !nifty_is_name( arg, obj->name ) )
         continue;
      found = TRUE;

      snprintf( buf, MSL, "&Y(&W%3d&Y) [&W%5d&Y] &G%-28s ", ++icnt, obj->pIndexData->vnum, obj_short( obj ) );
      if( obj->carried_by )
         snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Cinvent   &Y[&W%5d&Y] &C%s\n\r",
                   ( IS_NPC( obj->carried_by ) ? obj->carried_by->pIndexData->vnum : 0 ), PERS( obj->carried_by, ch,
                                                                                                TRUE ) );
      else if( obj->in_room )
      {
         if( IS_OBJ_FLAG( obj, ITEM_ONMAP ) )
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Coverland &Y[&W%s&Y] &C%d %d\n\r",
                      map_names[obj->map], obj->x, obj->y );
         else
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Croom     &Y[&W%5d&Y] &C%s\n\r",
                      obj->in_room->vnum, obj->in_room->name );
      }
      else if( obj->in_obj )
         snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Cobject &Y[&W%5d&Y] &C%s\n\r",
                   obj->in_obj->pIndexData->vnum, obj_short( obj->in_obj ) );
      else
      {
         bug( "%s", "do_owhere: object doesnt have location!" );
         mudstrlcat( buf, "nowhere??\n\r", MSL );
      }
      send_to_pager( buf, ch );
   }
   if( !found )
      act( AT_PLAIN, "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
   else
      pager_printf( ch, "%d matches.\n\r", icnt );
   return;
}

/* Locates where players are in the game, added by Samson on 1-2-98.
   Courtesy of D. Diaz 7th Circle Mud. Used by do_wherecon */
CMDF do_pwhere( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   DESCRIPTOR_DATA *d;
   bool found;

   if( !argument || argument[0] == '\0' )
   {
      send_to_pager( "&[people]Players you can see online:\n\r", ch );
      found = FALSE;
      for( d = first_descriptor; d; d = d->next )
         if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
             && ( victim = d->character ) != NULL && !IS_NPC( victim ) && victim->in_room
             && can_see( ch, victim, TRUE ) && !is_ignoring( victim, ch ) )
         {
            found = TRUE;
            if( IS_PLR_FLAG( victim, PLR_ONMAP ) )
               pager_printf( ch, "&G%-28s &Y[&WOverland&Y] &C%s %d %d\n\r",
                             victim->name, map_names[victim->map], victim->x, victim->y );
            else
               pager_printf( ch, "&G%-28s &Y[&W%5d&Y]&C %s\n\r",
                             victim->name, victim->in_room->vnum, victim->in_room->name );
         }
      if( !found )
         send_to_char( "None.\n\r", ch );
   }
   else
   {
      send_to_pager( "You search high and low and find:\n\r", ch );
      found = FALSE;
      for( victim = first_char; victim; victim = victim->next )
         if( victim->in_room && can_see( ch, victim, TRUE ) && is_name( argument, victim->name ) )
         {
            found = TRUE;
            pager_printf( ch, "&Y%-28s &G[&W%5d&G]&C %s\n\r",
                          PERS( victim, ch, TRUE ), victim->in_room->vnum, victim->in_room->name );
            break;
         }
      if( !found )
         send_to_char( "Nobody by that name.\n\r", ch );
   }
   return;
}

/* Consolidated Where command - Samson 3-21-98 */
CMDF do_where( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];

   argument = one_argument( argument, arg );

   if( ch->level < LEVEL_SAVIOR )
   {
      do_oldwhere( ch, argument );
      return;
   }

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Syntax:\n\r", ch );
      if( ch->level >= LEVEL_DEMI )
         send_to_char( "where mob <keyword or vnum>\n\r", ch );
      send_to_char( "where obj <keyword or vnum>\n\r", ch );
      if( ch->level >= LEVEL_DEMI )
         send_to_char( "where player <name>\n\r", ch );
      return;
   }

   if( ( !str_cmp( arg, "mob" ) || !str_cmp( arg, "m" ) ) && ch->level >= LEVEL_DEMI )
   {
      CHAR_DATA *victim;
      MOB_INDEX_DATA *pMobIndex;
      int vnum;
      int mobcnt = 0;
      bool found = FALSE;

      set_char_color( AT_PLAIN, ch );

      if( !is_number( argument ) )
      {
         do_mwhere( ch, argument );
         return;
      }

      vnum = atoi( argument );

      if( ( pMobIndex = get_mob_index( vnum ) ) == NULL )
      {
         send_to_char( "No mobile has that vnum.\n\r", ch );
         return;
      }

      for( victim = first_char; victim; victim = victim->next )
      {
         if( IS_NPC( victim ) && victim->in_room && victim->pIndexData->vnum == vnum )
         {
            found = TRUE;
            mobcnt++;
            pager_printf( ch, "[%5d] %-28s [%5d] %s\n\r",
                          victim->pIndexData->vnum, victim->short_descr, victim->in_room->vnum, victim->in_room->name );
         }
      }

      if( !found )
         pager_printf( ch, "No copies of vnum %d are loaded.\n\r", vnum );
      else
         pager_printf( ch, "%d matches for vnum %d are loaded.\n\r", mobcnt, vnum );
      return;
   }

   if( ( !str_cmp( arg, "player" ) || !str_cmp( arg, "p" ) ) && ch->level >= LEVEL_DEMI )
   {
      do_pwhere( ch, argument );
      return;
   }

   if( !str_cmp( arg, "obj" ) || !str_cmp( arg, "o" ) )
   {
      char buf[MSL];
      OBJ_DATA *obj;
      OBJ_INDEX_DATA *pObjIndex;
      int vnum;
      int icnt = 0;
      bool found = FALSE;

      set_char_color( AT_PLAIN, ch );

      if( !is_number( argument ) )
      {
         do_owhere( ch, argument );
         return;
      }

      vnum = atoi( argument );

      if( ( pObjIndex = get_obj_index( vnum ) ) == NULL )
      {
         send_to_char( "No object has that vnum.\n\r", ch );
         return;
      }

      for( obj = first_object; obj; obj = obj->next )
      {
         if( obj->pIndexData->vnum != vnum )
            continue;
         found = TRUE;

         snprintf( buf, MSL, "(%3d) [%5d] %-28s in ", ++icnt, obj->pIndexData->vnum, obj_short( obj ) );
         if( obj->carried_by )
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "invent [%5d] %s\n\r",
                      ( IS_NPC( obj->carried_by ) ? obj->carried_by->pIndexData->vnum : 0 ), PERS( obj->carried_by, ch,
                                                                                                   FALSE ) );
         else if( obj->in_room )
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "room   [%5d] %s\n\r", obj->in_room->vnum,
                      obj->in_room->name );
         else if( obj->in_obj )
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "object [%5d] %s\n\r", obj->in_obj->pIndexData->vnum,
                      obj_short( obj->in_obj ) );
         else
         {
            bug( "do_where: object '%s' doesn't have location!", obj->short_descr );
            mudstrlcat( buf, "nowhere??\n\r", MSL );
         }
         send_to_pager( buf, ch );
      }

      if( !found )
         pager_printf( ch, "No copies of vnum %d are loaded.\n\r", vnum );
      else
         pager_printf( ch, "%d matches for vnum %d are loaded.\n\r", icnt, vnum );

      pager_printf( ch, "Checking player files for stored copies of vnum %d....\n\r", vnum );

      check_stored_objects( ch, vnum );
      return;
   }

   /*
    * echo syntax 
    */
   do_where( ch, "" );
}

CMDF do_reboo( CHAR_DATA * ch, char *argument )
{
   send_to_char( "&YIf you want to REBOOT, spell it out.\n\r", ch );
   return;
}

/* Added security check for online compiler - Samson 4-8-98 */
CMDF do_reboot( CHAR_DATA * ch, char *argument )
{
#ifdef MULTIPORT
   if( compilelock )
   {
      send_to_char
         ( "&RSorry, the mud cannot be rebooted during a compiler operation.\n\rPlease wait for the compiler to finish.\n\r",
           ch );
      return;
   }
#endif

   set_char_color( AT_IMMORT, ch );

   if( argument[0] == '\0' )
   {
      if( bootlock )
      {
         send_to_char( "Reboot will be cancelled at the next event poll.\n\r", ch );
         reboot_counter = -5;
         bootlock = FALSE;
         sysdata.DENY_NEW_PLAYERS = FALSE;
         return;
      }
      sysdata.DENY_NEW_PLAYERS = TRUE;
      reboot_counter = sysdata.rebootcount;

      echo_to_all( AT_RED, "Reboot countdown started.", ECHOTAR_ALL );
      echo_all_printf( AT_YELLOW, ECHOTAR_ALL, "Game reboot in %d minutes.", reboot_counter );
      bootlock = TRUE;
      add_event( 60, ev_reboot_count, NULL );
      return;
   }

   if( str_cmp( argument, "mud now" ) && str_cmp( argument, "nosave" ) && str_cmp( argument, "and sort skill table" ) )
   {
      send_to_char( "Syntax: reboot mud now\n\r", ch );
      send_to_char( "Syntax: reboot nosave\n\r", ch );
      send_to_char( "Syntax: reboot and sort skill table\n\r", ch );
      send_to_char( "\n\rOr type 'reboot' with no argument to start a countdown.\n\r", ch );
      return;
   }

   echo_to_all( AT_IMMORT, "Manual Game Reboot", ECHOTAR_ALL );

   if( !str_cmp( argument, "and sort skill table" ) )
   {
      sort_skill_table(  );
      save_skill_table(  );
   }

   /*
    * Save all characters before booting. 
    */
   if( !str_cmp( argument, "nosave" ) )
      DONTSAVE = TRUE;

   mud_down = TRUE;
   return;
}

CMDF do_shutdow( CHAR_DATA * ch, char *argument )
{
   send_to_char( "&YIf you want to SHUTDOWN, spell it out.\n\r", ch );
   return;
}

/* Function modified from original form, added check for online compiler - Samson 4-8-98 */
CMDF do_shutdown( CHAR_DATA * ch, char *argument )
{
#ifdef MULTIPORT
   if( compilelock )
   {
      send_to_char
         ( "&RSorry, the mud cannot be shutdown during a compiler operation.\n\rPlease wait for the compiler to finish.\n\r",
           ch );
      return;
   }
#endif

   set_char_color( AT_IMMORT, ch );

   if( str_cmp( argument, "mud now" ) && str_cmp( argument, "nosave" ) )
   {
      send_to_char( "Syntax:  'shutdown mud now' or 'shutdown nosave'\n\r", ch );
      return;
   }

   echo_to_all( AT_IMMORT, "Manual Game Shutdown.", ECHOTAR_ALL );
   shutdown_mud( "Manual shutdown" );

   /*
    * Save all characters before booting. 
    */
   if( !str_cmp( argument, "nosave" ) )
      DONTSAVE = TRUE;

   mud_down = TRUE;
   return;
}

CMDF do_snoop( CHAR_DATA * ch, char *argument )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "&GSnooping the following people:\n\r\n\r", ch );
      for( d = first_descriptor; d; d = d->next )
         if( d->snoop_by == ch->desc )
            ch_printf( ch, "%s ", d->original ? d->original->name : d->character->name );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( !victim->desc )
   {
      send_to_char( "No descriptor to snoop.\n\r", ch );
      return;
   }
   if( victim == ch )
   {
      send_to_char( "Cancelling all snoops.\n\r", ch );
      for( d = first_descriptor; d; d = d->next )
         if( d->snoop_by == ch->desc )
            d->snoop_by = NULL;
      return;
   }
   if( victim->desc->snoop_by )
   {
      send_to_char( "Busy already.\n\r", ch );
      return;
   }

   /*
    * Minimum snoop level... a secret mset value
    * makes the snooper think that the victim is already being snooped
    * ( Right. Secret. Except it's in publically available source code. DUH! )
    */
   if( get_trust( victim ) >= get_trust( ch ) || ( victim->pcdata && victim->pcdata->min_snoop > get_trust( ch ) ) )
   {
      send_to_char( "Busy already.\n\r", ch );
      return;
   }

   if( ch->desc )
   {
      for( d = ch->desc->snoop_by; d; d = d->snoop_by )
         if( d->character == victim || d->original == victim )
         {
            send_to_char( "No snoop loops.\n\r", ch );
            return;
         }
   }

   victim->desc->snoop_by = ch->desc;
   send_to_char( "Ok.\n\r", ch );
   return;
}

CMDF do_switch( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Switch into whom?\n\r", ch );
      return;
   }
   if( !ch->desc )
      return;
   if( ch->desc->original )
   {
      send_to_char( "You are already switched.\n\r", ch );
      return;
   }
   if( ( victim = get_char_world( ch, argument ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( victim == ch )
   {
      send_to_char( "Be serious.\n\r", ch );
      return;
   }
   if( victim->desc )
   {
      send_to_char( "Character in use.\n\r", ch );
      return;
   }
   if( !IS_NPC( victim ) && !IS_IMP( ch ) )
   {
      send_to_char( "You cannot switch into a player!\n\r", ch );
      return;
   }
   if( victim->switched )
   {
      send_to_char( "You can't switch into a player that is switched!\n\r", ch );
      return;
   }
   if( IS_PLR_FLAG( victim, PLR_FREEZE ) )
   {
      send_to_char( "You shouldn't switch into a player that is frozen!\n\r", ch );
      return;
   }

   ch->desc->character = victim;
   ch->desc->original = ch;
   victim->desc = ch->desc;
   ch->desc = NULL;
   ch->switched = victim;
   send_to_char( "Ok.\n\r", victim );
   return;
}

CMDF do_return( CHAR_DATA * ch, char *argument )
{
   if( !IS_NPC( ch ) && get_trust( ch ) < LEVEL_IMMORTAL )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }
   set_char_color( AT_IMMORT, ch );

   if( !ch->desc )
      return;
   if( !ch->desc->original )
   {
      send_to_char( "You aren't switched.\n\r", ch );
      return;
   }

   send_to_char( "You return to your original body.\n\r", ch );

   ch->desc->character = ch->desc->original;
   ch->desc->original = NULL;
   ch->desc->character->desc = ch->desc;
   ch->desc->character->switched = NULL;
   ch->desc = NULL;
   return;
}

CMDF do_minvoke( CHAR_DATA * ch, char *argument )
{
   MOB_INDEX_DATA *pMobIndex;
   CHAR_DATA *victim;
   int vnum;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax: load m <vnum/keyword>\n\r", ch );
      return;
   }
   if( !is_number( argument ) )
   {
      char arg2[MIL];
      int hash, cnt;
      int count = number_argument( argument, arg2 );

      vnum = -1;
      for( hash = cnt = 0; hash < MAX_KEY_HASH; hash++ )
         for( pMobIndex = mob_index_hash[hash]; pMobIndex; pMobIndex = pMobIndex->next )
            if( nifty_is_name( arg2, pMobIndex->player_name ) && ++cnt == count )
            {
               vnum = pMobIndex->vnum;
               break;
            }
      if( vnum == -1 )
      {
         send_to_char( "No such mobile exists.\n\r", ch );
         return;
      }
   }
   else
      vnum = atoi( argument );

   if( get_trust( ch ) < LEVEL_DEMI )
   {
      AREA_DATA *pArea;

      if( IS_NPC( ch ) )
      {
         send_to_char( "Huh?\n\r", ch );
         return;
      }
      if( !( pArea = ch->pcdata->area ) )
      {
         send_to_char( "You must have an assigned area to invoke this mobile.\n\r", ch );
         return;
      }
      if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
      {
         send_to_char( "That number is not in your allocated range.\n\r", ch );
         return;
      }
   }
   if( ( pMobIndex = get_mob_index( vnum ) ) == NULL )
   {
      send_to_char( "No mobile has that vnum.\n\r", ch );
      return;
   }

   victim = create_mobile( pMobIndex );
   if( !char_to_room( victim, ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   /*
    * If you load one on the map, make sure it gets placed properly - Samson 8-21-99 
    */
   fix_maps( ch, victim );
   victim->sector = get_terrain( ch->map, ch->x, ch->y );

   act( AT_IMMORT, "$n peers into the ether, and plucks out $N!", ch, NULL, victim, TO_ROOM );
   /*
    * How about seeing what we're invoking for a change. -Blodkai
    */
   ch_printf( ch, "&YYou peer into the ether.... and pluck out %s!\n\r", pMobIndex->short_descr );
   ch_printf( ch, "(&W#%d &Y- &W%s &Y- &Wlvl %d&Y)\n\r", pMobIndex->vnum, pMobIndex->player_name, victim->level );
   return;
}

CMDF do_oinvoke( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   OBJ_INDEX_DATA *pObjIndex;
   OBJ_DATA *obj;
   int vnum, level;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Syntax: load obj <vnum/keyword> <level>\n\r", ch );
      return;
   }
   if( !argument || argument[0] == '\0' )
      level = 1;
   else
   {
      if( !is_number( argument ) )
      {
         send_to_char( "Syntax: load obj <vnum/keyword> <level>\n\r", ch );
         return;
      }
      level = atoi( argument );
      if( level < 0 || level > get_trust( ch ) )
      {
         send_to_char( "Limited to your trust level.\n\r", ch );
         return;
      }
   }

   if( !is_number( arg1 ) )
   {
      char arg[MIL];
      int hash, cnt, count = number_argument( arg1, arg );

      vnum = -1;
      for( hash = cnt = 0; hash < MAX_KEY_HASH; hash++ )
         for( pObjIndex = obj_index_hash[hash]; pObjIndex; pObjIndex = pObjIndex->next )
            if( nifty_is_name( arg, pObjIndex->name ) && ++cnt == count )
            {
               vnum = pObjIndex->vnum;
               break;
            }

      if( vnum == -1 )
      {
         send_to_char( "No such object exists.\n\r", ch );
         return;
      }
   }
   else
      vnum = atoi( arg1 );

   if( ( pObjIndex = get_obj_index( vnum ) ) == NULL )
   {
      send_to_char( "No object has that vnum.\n\r", ch );
      return;
   }

   if( !( obj = create_object( pObjIndex, level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }

#ifdef MULTIPORT
   if( obj->rent >= sysdata.minrent && port == MAINPORT )
   {
      if( !IS_IMP( ch ) )
      {
         send_to_char( "Loading of rare items is restricted to KLs and above on this port.\n\r", ch );
         return;
      }
      else
      {
         ch_printf( ch, "WARNING: This item has rent exceeding %d! Destroy this item when finished!\n\r", sysdata.minrent );
         log_printf( "do_load: %s has loaded a copy of vnum %d.", ch->name, pObjIndex->vnum );
      }
   }
#else
   if( obj->rent >= sysdata.minrent )
   {
      ch_printf( ch, "WARNING: This item has rent exceeding %d! Destroy this item when finished!\n\r", sysdata.minrent );
      log_printf( "do_load: %s has loaded a copy of vnum %d.", ch->name, pObjIndex->vnum );
   }
#endif

   if( CAN_WEAR( obj, ITEM_TAKE ) )
   {
      obj = obj_to_char( obj, ch );
      act( AT_IMMORT, "$n waves $s hand, and $p appears in $s inventory!", ch, obj, NULL, TO_ROOM );
      act( AT_IMMORT, "You wave your hand, and $p appears in your inventory!", ch, obj, NULL, TO_CHAR );
   }
   else
   {
      obj = obj_to_room( obj, ch->in_room, ch );
      act( AT_IMMORT, "$n waves $s hand, and $p appears in the room!", ch, obj, NULL, TO_ROOM );
      act( AT_IMMORT, "You wave your hand, and $p appears in the room!", ch, obj, NULL, TO_CHAR );
   }

   /*
    * This is bad, means stats were exceeding rent specs 
    */
   if( obj->rent == -2 )
      send_to_char( "&YWARNING: This object exceeds allowable rent specs.\n\r", ch );

   /*
    * I invoked what? --Blodkai 
    */
   ch_printf( ch, "&Y(&W#%d &Y- &W%s &Y- &Wlvl %d&Y)\n\r", pObjIndex->vnum, pObjIndex->name, obj->level );
   return;
}

/* Shard-like load command spliced off of ROT codebase - Samson 3-21-98 */
CMDF do_load( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Syntax:\n\r", ch );
      if( ch->level >= LEVEL_DEMI )
         send_to_char( "load mob <vnum or keyword>\n\r", ch );
      send_to_char( "load obj <vnum or keyword>\n\r", ch );
      return;
   }

   if( ( !str_cmp( arg, "mob" ) || !str_cmp( arg, "m" ) ) && ch->level >= LEVEL_DEMI )
   {
      do_minvoke( ch, argument );
      return;
   }

   if( !str_cmp( arg, "obj" ) || !str_cmp( arg, "o" ) )
   {
      do_oinvoke( ch, argument );
      return;
   }
   /*
    * echo syntax 
    */
   do_load( ch, "" );
}

/* Function modified from original form - Samson, unknown date */
CMDF do_purge( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim, *tch;
   OBJ_DATA *obj, *obj_next;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      /*
       * 'purge' 
       */
      CHAR_DATA *vnext;

      for( victim = ch->in_room->first_person; victim; victim = vnext )
      {
         vnext = victim->next_in_room;

         /*
          * GACK! Why did this get removed?? 
          */
         if( !IS_NPC( victim ) )
            continue;

         for( tch = ch->in_room->first_person; tch; tch = tch->next_in_room )
            if( !IS_NPC( tch ) && tch->pcdata->dest_buf == victim )
               break;

         if( tch && !IS_NPC( tch ) && tch->pcdata->dest_buf == victim )
            continue;

         if( ( !str_cmp( victim->short_descr, "The Negative Magnetic Space Wedgy" )
               || !str_cmp( victim->short_descr, "Krusty" )
               || !str_cmp( victim->short_descr, "Satan" )
               || !str_cmp( victim->short_descr, "Mini-Cam" ) ) && str_cmp( ch->name, "Samson" ) )
         {
            send_to_char( "Did you REALLY think the Great Lord and Master would allow that?\n\r", ch );
            continue;
         }

         /*
          * This will work in normal rooms too since they should always be -1,-1,-1 outside of the maps. 
          */
         if( is_same_map( ch, victim ) )
            extract_char( victim, TRUE );
      }

      for( obj = ch->in_room->first_content; obj; obj = obj_next )
      {
         obj_next = obj->next_content;

         for( tch = ch->in_room->first_person; tch; tch = tch->next_in_room )
            if( !IS_NPC( tch ) && tch->pcdata->dest_buf == obj )
               break;
         if( tch && !IS_NPC( tch ) && tch->pcdata->dest_buf == obj )
            continue;

         /*
          * If target is on a map, make sure your at the right coordinates - Samson 
          */
         if( ch->map == obj->map && ch->x == obj->x && ch->y == obj->y )
            extract_obj( obj );
      }

      act( AT_IMMORT, "$n makes a complex series of gestures.... suddenly things seem a lot cleaner!", ch, NULL, NULL,
           TO_ROOM );
      act( AT_IMMORT, "You make a complex series of gestures.... suddenly things seem a lot cleaner!", ch, NULL, NULL,
           TO_CHAR );
      return;
   }
   victim = NULL;
   obj = NULL;

   /*
    * fixed to get things in room first -- i.e., purge portal (obj),
    * * no more purging mobs with that keyword in another room first
    * * -- Tri 
    */
   if( ( victim = get_char_room( ch, argument ) ) == NULL && ( obj = get_obj_here( ch, argument ) ) == NULL )
   {
      send_to_char( "That isn't here.\n\r", ch );
      return;
   }

   /*
    * Single object purge in room for high level purge - Scryn 8/12
    */
   if( obj )
   {
      for( tch = ch->in_room->first_person; tch; tch = tch->next_in_room )
         if( !IS_NPC( tch ) && tch->pcdata->dest_buf == obj )
         {
            send_to_char( "You cannot purge something being edited.\n\r", ch );
            return;
         }

      separate_obj( obj );

      act( AT_IMMORT, "$n snaps $s finger, and $p vanishes from sight!", ch, obj, NULL, TO_ROOM );
      act( AT_IMMORT, "You snap your fingers, and $p vanishes from sight!", ch, obj, NULL, TO_CHAR );
      extract_obj( obj );
      return;
   }

   if( !IS_NPC( victim ) )
   {
      send_to_char( "Not on PC's.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You cannot purge yourself!\n\r", ch );
      return;
   }

   for( tch = ch->in_room->first_person; tch; tch = tch->next_in_room )
      if( !IS_NPC( tch ) && tch->pcdata->dest_buf == victim )
      {
         send_to_char( "You cannot purge something being edited.\n\r", ch );
         return;
      }

   if( ( !str_cmp( victim->short_descr, "The Negative Magnetic Space Wedgy" )
         || !str_cmp( victim->short_descr, "Krusty" )
         || !str_cmp( victim->short_descr, "Satan" )
         || !str_cmp( victim->short_descr, "Mini-Cam" ) ) && str_cmp( ch->name, "Samson" ) )
   {
      send_to_char( "Did you REALLY think the Great Lord and Master would allow that?\n\r", ch );
      return;
   }
   act( AT_IMMORT, "$n snaps $s fingers, and $N vanishes from sight!", ch, NULL, victim, TO_NOTVICT );
   act( AT_IMMORT, "You snap your fingers, and $N vanishes from sight!", ch, NULL, victim, TO_CHAR );
   extract_char( victim, TRUE );
   return;
}

/*
 * This could have other applications too.. move if needed. -- Altrag
 */
void close_area( AREA_DATA * pArea )
{
   CHAR_DATA *ech, *ech_next;
   OBJ_DATA *eobj, *eobj_next;
   ROOM_INDEX_DATA *rid, *rid_next;
   OBJ_INDEX_DATA *oid, *oid_next;
   MOB_INDEX_DATA *mid, *mid_next;
   NEIGHBOR_DATA *neighbor, *neighbor_next;
   int icnt;

   for( ech = first_char; ech; ech = ech_next )
   {
      ech_next = ech->next;

      if( ech->fighting )
         stop_fighting( ech, TRUE );
      if( IS_NPC( ech ) )
      {
         /*
          * if mob is in area, or part of area. 
          */
         if( URANGE( pArea->low_vnum, ech->pIndexData->vnum, pArea->hi_vnum ) == ech->pIndexData->vnum
             || ( ech->in_room && ech->in_room->area == pArea ) )
            extract_char( ech, TRUE );
         continue;
      }

      if( ech->in_room && ech->in_room->area == pArea )
         recall( ech, -1 );

      if( !IS_NPC( ech ) && ech->pcdata->area == pArea )
      {
         ech->pcdata->area = NULL;
         ech->pcdata->low_vnum = 0;
         ech->pcdata->hi_vnum = 0;
         save_char_obj( ech );
      }
   }
   for( eobj = first_object; eobj; eobj = eobj_next )
   {
      eobj_next = eobj->next;
      /*
       * if obj is in area, or part of area. 
       */
      if( URANGE( pArea->low_vnum, eobj->pIndexData->vnum, pArea->hi_vnum ) == eobj->pIndexData->vnum
          || ( eobj->in_room && eobj->in_room->area == pArea ) )
         extract_obj( eobj );
   }
   for( icnt = 0; icnt < MAX_KEY_HASH; icnt++ )
   {
      for( rid = room_index_hash[icnt]; rid; rid = rid_next )
      {
         rid_next = rid->next;

         if( rid->area != pArea )
            continue;
         delete_room( rid );
      }
      pArea->first_room = pArea->last_room = NULL;

      for( mid = mob_index_hash[icnt]; mid; mid = mid_next )
      {
         mid_next = mid->next;

         if( mid->vnum < pArea->low_vnum || mid->vnum > pArea->hi_vnum )
            continue;
         delete_mob( mid );
      }

      for( oid = obj_index_hash[icnt]; oid; oid = oid_next )
      {
         oid_next = oid->next;

         if( oid->vnum < pArea->low_vnum || oid->vnum > pArea->hi_vnum )
            continue;
         delete_obj( oid );
      }
   }
   if( pArea->weather )
   {
      for( neighbor = pArea->weather->first_neighbor; neighbor; neighbor = neighbor_next )
      {
         neighbor_next = neighbor->next;
         UNLINK( neighbor, pArea->weather->first_neighbor, pArea->weather->last_neighbor, next, prev );
         STRFREE( neighbor->name );
         DISPOSE( neighbor );
      }
      DISPOSE( pArea->weather );
   }
   DISPOSE( pArea->name );
   DISPOSE( pArea->filename );
   DISPOSE( pArea->resetmsg );
   STRFREE( pArea->author );
   if( pArea->version > 1 && pArea->version != 1000 )
   {
      UNLINK( pArea, first_area_nsort, last_area_nsort, next_sort_name, prev_sort_name );
      UNLINK( pArea, first_area_vsort, last_area_vsort, next_sort, prev_sort );
   }
   UNLINK( pArea, first_area, last_area, next, prev );
   DISPOSE( pArea );
}

void close_all_areas( void )
{
   AREA_DATA *area, *area_next;

   for( area = first_area; area; area = area_next )
   {
      area_next = area->next;
      close_area( area );
   }
   return;
}

/* Function modified from original form - Samson 7-29-98 */
CMDF do_balzhur( CHAR_DATA * ch, char *argument )
{
   char buf[MSL], buf2[MSL];
   char *name;
   CHAR_DATA *victim;
   AREA_DATA *pArea;
   int sn;

   set_char_color( AT_BLOOD, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Who is deserving of such a fate?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't currently playing.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "This will do little good on mobiles.\n\r", ch );
      return;
   }
   if( victim->level >= ch->level )
   {
      send_to_char( "I wouldn't even think of that if I were you...\n\r", ch );
      return;
   }

   victim->level = 2;
   victim->trust = 0;
   check_switch( victim, TRUE );
   send_to_char( "&WYou summon the Negative Magnetic Space Wedgy to wreak your wrath!\n\r", ch );
   send_to_char( "The Wedgy sneers at you evilly, then vanishes in a puff of smoke.\n\r", ch );
   send_to_char( "&[immortal]You hear an ungodly sound in the distance that makes your blood run cold!\n\r", victim );
   echo_all_printf( AT_IMMORT, ECHOTAR_ALL, "The Wedgy screams, 'You are MINE %s!!!'", victim->name );
   victim->exp = 2000;
   victim->max_hit = 10;
   victim->max_mana = 100;
   victim->max_move = 150;
   victim->gold = 0;
   victim->pcdata->balance = 0;
   for( sn = 0; sn < top_sn; sn++ )
      victim->pcdata->learned[sn] = 0;
   victim->pcdata->practice = 0;
   victim->hit = victim->max_hit;
   victim->mana = victim->max_mana;
   victim->move = victim->max_move;
   name = capitalize( victim->name );
   snprintf( buf, MSL, "%s%s", GOD_DIR, name );

   if( !remove( buf ) )
      send_to_char( "&RPlayer's immortal data destroyed.\n\r", ch );
   else if( errno != ENOENT )
   {
      ch_printf( ch, "&RUnknown error #%d - %s (immortal data).  Report to Samson\n\r", errno, strerror( errno ) );
      snprintf( buf2, MSL, "%s balzhuring %s", ch->name, buf );
      perror( buf2 );
   }
   snprintf( buf2, MSL, "%s.are", name );
   for( pArea = first_area; pArea; pArea = pArea->next )
      if( !str_cmp( pArea->filename, buf2 ) )
      {
         snprintf( buf, MSL, "%s%s", BUILD_DIR, buf2 );
         fold_area( pArea, buf, FALSE );
         close_area( pArea );
         snprintf( buf2, MSL, "%s.bak", buf );
         if( !rename( buf, buf2 ) )
            send_to_char( "&RPlayer's area data destroyed.  Area saved as backup.\n\r", ch );
         else if( errno != ENOENT )
         {
            ch_printf( ch, "&RUnknown error #%d - %s (area data).  Report to  Samson.\n\r", errno, strerror( errno ) );
            snprintf( buf2, MSL, "%s destroying %s", ch->name, buf );
            perror( buf2 );
         }
         break;
      }

   advance_level( victim );
   make_wizlist(  );
   interpret( victim, "help M_BALZHUR_" );
   send_to_char( "&WYou awake after a long period of time...\n\r", victim );
   while( victim->first_carrying )
      extract_obj( victim->first_carrying );
   return;
}

/* Function modified from original form on unknown date - Samson */
CMDF do_advance( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   CHAR_DATA *victim;
   int level, iLevel;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' || !argument || argument[0] == '\0' || !is_number( argument ) )
   {
      send_to_char( "Syntax:  advance <character> <level>\n\r", ch );
      return;
   }
   if( ( victim = get_char_world( ch, arg1 ) ) == NULL )
   {
      send_to_char( "That character is not in the game.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "You cannot advance a mobile.\n\r", ch );
      return;
   }
   /*
    * You can demote yourself but not someone else at your own trust.-- Narn
    */
   /*
    * Guys, you have any idea how STUPID this sounds? -- Samson 
    */
   if( ch == victim )
   {
      send_to_char( "Sorry, adjusting your own level is forbidden.\n\r", ch );
      return;
   }

   if( ch->level <= victim->level && ch != victim )
   {
      send_to_char( "You can't do that.\n\r", ch );
      return;
   }

   if( ( level = atoi( argument ) ) < 1 || level > LEVEL_AVATAR )
   {
      ch_printf( ch, "Level range is 1 to %d.\n\r", LEVEL_AVATAR );
      return;
   }

   if( level > get_trust( ch ) )
   {
      send_to_char( "You cannot advance someone beyond your own trust level.\n\r", ch );
      return;
   }

   /*
    * Check added to keep advance command from going beyond Avatar level - Samson 
    */
   if( victim->level >= LEVEL_AVATAR )
   {
      send_to_char( "If your trying to raise an immortal's level, use the promote command.\n\r", ch );
      send_to_char( "If your trying to lower an immortal's level, use the demote command.\n\r", ch );
      ch_printf( ch, "If your trying to immortalize %s, use the immortalize command.\n\r", victim->name );
      return;
   }

   if( level == victim->level )
   {
      ch_printf( ch, "What would be the point? %s is already level %d.\n\r", victim->name, level );
      return;
   }

   /*
    * Lower level:
    * *   Reset to level 1.
    * *   Then raise again.
    * *   Currently, an imp can lower another imp.
    * *   -- Swiftest
    * *   Can't lower imms >= your trust (other than self) per Narn's change.
    * *   Few minor text changes as well.  -- Blod
    */
   if( level < victim->level )
   {
      int sn;

      set_char_color( AT_IMMORT, victim );
      if( level < victim->level )
      {
         int tmp = victim->level;

         victim->level = level;
         check_switch( victim, FALSE );
         victim->level = tmp;

         ch_printf( ch, "Demoting %s from level %d to level %d!\r\n", victim->name, victim->level, level );
         send_to_char( "Cursed and forsaken!  The gods have lowered your level...\r\n", victim );
      }
      else
      {
         ch_printf( ch, "%s is already level %d.  Re-advancing...\n\r", victim->name, level );
         send_to_char( "Deja vu!  Your mind reels as you re-live your past levels!\n\r", victim );
      }
      victim->level = 1;
      victim->exp = exp_level( 1 );
      victim->max_hit = 20;
      victim->max_mana = 100;
      victim->max_move = 150;
      for( sn = 0; sn < top_sn; sn++ )
         victim->pcdata->learned[sn] = 0;
      victim->pcdata->practice = 0;
      victim->hit = victim->max_hit;
      victim->mana = victim->max_mana;
      victim->move = victim->max_move;
      advance_level( victim );
      /*
       * Rank fix added by Narn. 
       */
      STRFREE( victim->pcdata->rank );
      /*
       * Stuff added to make sure character's wizinvis level doesn't stay
       * higher than actual level, take wizinvis away from advance < 50 
       */
      if( IS_PLR_FLAG( victim, PLR_WIZINVIS ) )
         victim->pcdata->wizinvis = victim->trust;
      if( IS_PLR_FLAG( victim, PLR_WIZINVIS ) && ( victim->level <= LEVEL_AVATAR ) )
      {
         REMOVE_PLR_FLAG( victim, PLR_WIZINVIS );
         victim->pcdata->wizinvis = 0;
      }
   }
   else
   {
      ch_printf( ch, "Raising %s from level %d to level %d!\n\r", victim->name, victim->level, level );
      send_to_char( "The gods feel fit to raise your level!\n\r", victim );
   }
   for( iLevel = victim->level; iLevel < level; iLevel++ )
   {
      if( level < LEVEL_IMMORTAL )
         send_to_char( "You raise a level!!\n\r", victim );
      victim->level += 1;
      advance_level( victim );
   }
   victim->exp = exp_level( victim->level );
   victim->trust = 0;
   return;
}

/*
 * "Fix" a character's stats					-Thoric
 */
void fix_char( CHAR_DATA * ch )
{
   AFFECT_DATA *aff;
   OBJ_DATA *obj;

   de_equip_char( ch );

   for( aff = ch->first_affect; aff; aff = aff->next )
      affect_modify( ch, aff, FALSE );

   xCLEAR_BITS( ch->affected_by );
   xSET_BITS( ch->affected_by, race_table[ch->race]->affected );
   ch->mental_state = -10;
   ch->hit = UMAX( 1, ch->hit );
   ch->mana = UMAX( 1, ch->mana );
   ch->move = UMAX( 1, ch->move );
   ch->armor = 100;
   ch->mod_str = 0;
   ch->mod_dex = 0;
   ch->mod_wis = 0;
   ch->mod_int = 0;
   ch->mod_con = 0;
   ch->mod_cha = 0;
   ch->mod_lck = 0;
   ch->damroll = 0;
   ch->hitroll = 0;
   ch->alignment = URANGE( -1000, ch->alignment, 1000 );
   ch->saving_breath = 0;
   ch->saving_wand = 0;
   ch->saving_para_petri = 0;
   ch->saving_spell_staff = 0;
   ch->saving_poison_death = 0;

   ch->carry_weight = 0;
   ch->carry_number = 0;

   ClassSpecificStuff( ch );  /* Brought over from DOTD code - Samson 4-6-99 */

   for( aff = ch->first_affect; aff; aff = aff->next )
      affect_modify( ch, aff, TRUE );

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->wear_loc == WEAR_NONE )
         ch->carry_number += get_obj_number( obj );
      if( !xIS_SET( obj->extra_flags, ITEM_MAGIC ) )
         ch->carry_weight += get_obj_weight( obj );
   }

   re_equip_char( ch );
}

CMDF do_fixchar( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: fixchar <playername>\n\r", ch );
      return;
   }

   victim = get_char_world( ch, argument );
   if( !victim )
   {
      send_to_char( "They're not here.\n\r", ch );
      return;
   }
   fix_char( victim );
   send_to_char( "Done.\n\r", ch );
}

/* Function modified from original form - Samson */
/* Misc mods made on 1-18-98 - Samson */
/* Eliminated clan_type GUILD - Samson 11-30-98 */
CMDF do_immortalize( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   int sn;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax:  immortalize <char>\n\r", ch );
      return;
   }

   if( ( victim = get_char_world( ch, argument ) ) == NULL )
   {
      send_to_char( "That player is not here.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   /*
    * Added this check, not sure why the code didn't already have it. Samson 1-18-98 
    */
   if( victim->level >= LEVEL_IMMORTAL )
   {
      ch_printf( ch, "Don't be silly, %s is already immortal.\n\r", victim->name );
      return;
   }

   if( victim->level != LEVEL_AVATAR )
   {
      send_to_char( "This player is not yet worthy of immortality.\n\r", ch );
      return;
   }

   send_to_char( "Immortalizing a player...\n\r", ch );
   act( AT_IMMORT, "$n begins to chant softly... then raises $s arms to the sky...", ch, NULL, NULL, TO_ROOM );
   send_to_char( "&WYou suddenly feel very strange...\n\r\n\r", victim );
   interpret( victim, "help M_GODLVL1_" );
   send_to_char( "&WYou awake... all your possessions are gone.\n\r", victim );
   while( victim->first_carrying )
      extract_obj( victim->first_carrying );
   victim->level = LEVEL_IMMORTAL;
   advance_level( victim );

   /*
    * Remove clan/guild/order and update accordingly 
    */
   if( victim->pcdata->clan )
   {
      if( victim->speaking & LANG_CLAN )
         victim->speaking = LANG_COMMON;
      REMOVE_BIT( victim->speaks, LANG_CLAN );
      --victim->pcdata->clan->members;
      if( !str_cmp( victim->name, victim->pcdata->clan->leader ) )
         STRFREE( victim->pcdata->clan->leader );
      if( !str_cmp( victim->name, victim->pcdata->clan->number1 ) )
         STRFREE( victim->pcdata->clan->number1 );
      if( !str_cmp( victim->name, victim->pcdata->clan->number2 ) )
         STRFREE( victim->pcdata->clan->number2 );
      victim->pcdata->clan = NULL;
      STRFREE( victim->pcdata->clan_name );
   }
   victim->exp = exp_level( victim->level );
   victim->trust = 0;

   /*
    * Automatic settings for imms added by Samson 1-18-98 
    */

   fix_char( victim );
   for( sn = 0; sn < top_sn; sn++ )
      victim->pcdata->learned[sn] = 100;

   SET_PLR_FLAG( victim, PLR_HOLYLIGHT );
   SET_PCFLAG( victim, PCFLAG_PASSDOOR );
   STRFREE( victim->pcdata->rank );
   save_char_obj( victim );
   make_wizlist(  );
   build_wizinfo(  );
   if( victim->alignment > 350 )
      echo_to_all( AT_IMMORT, "The forces of order grow stronger as another mortal ascends to the heavens!", ECHOTAR_ALL );
   if( victim->alignment <= 350 && victim->alignment >= -350 )
      echo_to_all( AT_IMMORT, "The forces of balance grow stronger as another mortal ascends to the heavens!", ECHOTAR_ALL );
   if( victim->alignment < -350 )
      echo_to_all( AT_IMMORT, "The forces of chaos grow stronger as another mortal ascends to the heavens!", ECHOTAR_ALL );
   /*
    * End added automatic settings 
    */
   return;
}

CMDF do_trust( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   CHAR_DATA *victim;
   int level;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' || !argument || argument[0] == '\0' || !is_number( argument ) )
   {
      send_to_char( "Syntax:  trust <char> <level>.\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      send_to_char( "That player is not online.\n\r", ch );
      return;
   }
   if( ( level = atoi( argument ) ) < 0 || level > MAX_LEVEL )
   {
      ch_printf( ch, "Level must be 0 (reset) or 1 to %d.\n\r", MAX_LEVEL );
      return;
   }
   if( level > get_trust( ch ) )
   {
      send_to_char( "Limited to your own trust.\n\r", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You can't do that.\n\r", ch );
      return;
   }

   victim->trust = level;
   send_to_char( "Ok.\n\r", ch );
   return;
}

/* Command to silently sneak something into someone's inventory - for immortals only */
CMDF do_stash( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *obj;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !str_cmp( arg2, "to" ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Stash what on whom?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( is_number( arg1 ) )
   {
      /*
       * 'give NNNN coins victim' 
       */
      int amount;

      amount = atoi( arg1 );
      if( amount <= 0 || ( str_cmp( arg2, "coins" ) && str_cmp( arg2, "coin" ) ) )
      {
         send_to_char( "Sorry, you can't do that.\n\r", ch );
         return;
      }

      argument = one_argument( argument, arg2 );
      if( !str_cmp( arg2, "to" ) && argument[0] != '\0' )
         argument = one_argument( argument, arg2 );
      if( arg2[0] == '\0' )
      {
         send_to_char( "Stash what on whom?\n\r", ch );
         return;
      }

      if( ( victim = get_char_world( ch, arg2 ) ) == NULL )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }

      if( IS_NPC( victim ) && !IS_IMP( ch ) )
      {
         send_to_char( "Don't stash things on mobs!\n\r", ch );
         return;
      }

      if( !IS_IMP( ch ) )
      {
         if( ch->gold < amount )
         {
            send_to_char( "Very generous of you, but you haven't got that much gold.\n\r", ch );
            return;
         }
      }

      if( !IS_IMP( ch ) )
         ch->gold -= amount;

      victim->gold += amount;

      act( AT_ACTION, "You stash some coins on $N.", ch, NULL, victim, TO_CHAR );

      if( IS_SAVE_FLAG( SV_GIVE ) && !char_died( ch ) )
         save_char_obj( ch );

      if( IS_SAVE_FLAG( SV_RECEIVE ) && !char_died( victim ) )
         save_char_obj( victim );

      return;
   }

   if( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
   {
      send_to_char( "You do not have that item.\n\r", ch );
      return;
   }

   if( obj->wear_loc != WEAR_NONE )
   {
      send_to_char( "You must remove it first.\n\r", ch );
      return;
   }

   if( ( victim = get_char_world( ch, arg2 ) ) == NULL )
   {
      send_to_char( "They aren't in the game.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Don't stash things on mobs!\n\r", ch );
      return;
   }

   if( victim->carry_number + ( get_obj_number( obj ) / obj->count ) > can_carry_n( victim ) )
   {
      act( AT_PLAIN, "$N has $S hands full.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim->carry_weight + ( get_obj_weight( obj ) / obj->count ) > can_carry_w( victim ) )
   {
      act( AT_PLAIN, "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) && !can_take_proto( victim ) )
   {
      act( AT_PLAIN, "You cannot give that to $N!", ch, NULL, victim, TO_CHAR );
      return;
   }

   separate_obj( obj );
   obj_from_char( obj );

   act( AT_ACTION, "You stash $p on $N.", ch, obj, victim, TO_CHAR );
   obj = obj_to_char( obj, victim );

   if( IS_SAVE_FLAG( SV_GIVE ) && !char_died( ch ) )
      save_char_obj( ch );

   if( IS_SAVE_FLAG( SV_RECEIVE ) && !char_died( victim ) )
      save_char_obj( victim );

   return;
}

/* Summer 1997 --Blod */
CMDF do_scatter( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   ROOM_INDEX_DATA *pRoomIndex, *from;
   int schance;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Scatter whom?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( victim == ch )
   {
      send_to_char( "It's called teleport. Try it.\n\r", ch );
      return;
   }
   if( !IS_NPC( victim ) && get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You haven't the power to succeed against them.\n\r", ch );
      return;
   }

   schance = number_range( 1, 2 );

   from = victim->in_room;

   if( schance == 1 || IS_IMMORTAL( victim ) )
   {
      int map, x, y;
      short sector;

      for( ;; )
      {
         map = number_range( 0, MAP_MAX - 1 );
         x = number_range( 0, MAX_X - 1 );
         y = number_range( 0, MAX_Y - 1 );

         sector = get_terrain( map, x, y );
         if( sector == -1 )
            continue;
         if( sect_show[sector].canpass )
            break;
      }
      act( AT_MAGIC, "With the sweep of an arm, $n flings $N to the astral winds.", ch, NULL, victim, TO_NOTVICT );
      act( AT_MAGIC, "With the sweep of an arm, $n flings you to the astral winds.", ch, NULL, victim, TO_VICT );
      act( AT_MAGIC, "With the sweep of an arm, you fling $N to the astral winds.", ch, NULL, victim, TO_CHAR );
      enter_map( victim, NULL, x, y, map );
      collect_followers( victim, from, victim->in_room );
      victim->position = POS_STANDING;
      act( AT_MAGIC, "$n is deposited in a heap by the astral winds.", victim, NULL, NULL, TO_ROOM );
   }
   else
   {
      for( ;; )
      {
         pRoomIndex = get_room_index( number_range( 0, sysdata.maxvnum ) );
         if( pRoomIndex )
            if( !IS_ROOM_FLAG( pRoomIndex, ROOM_PRIVATE )
                && !IS_ROOM_FLAG( pRoomIndex, ROOM_SOLITARY )
                && !IS_ROOM_FLAG( pRoomIndex, ROOM_PROTOTYPE ) && !IS_ROOM_FLAG( pRoomIndex, ROOM_ISOLATED ) )
               break;
      }
      if( victim->fighting )
         stop_fighting( victim, TRUE );
      act( AT_MAGIC, "With the sweep of an arm, $n flings $N to the astral winds.", ch, NULL, victim, TO_NOTVICT );
      act( AT_MAGIC, "With the sweep of an arm, $n flings you to the astral winds.", ch, NULL, victim, TO_VICT );
      act( AT_MAGIC, "With the sweep of an arm, you fling $N to the astral winds.", ch, NULL, victim, TO_CHAR );
      leave_map( victim, NULL, pRoomIndex );
      victim->position = POS_RESTING;
      act( AT_MAGIC, "$n is deposited in a heap by the astral winds.", victim, NULL, NULL, TO_ROOM );
   }
   return;
}

CMDF do_strew( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *obj_next, *obj_lose;
   ROOM_INDEX_DATA *pRoomIndex;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "Strew who, what?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      send_to_char( "It would work better if they were here.\n\r", ch );
      return;
   }
   if( victim == ch )
   {
      send_to_char( "Try taking it out on someone else first.\n\r", ch );
      return;
   }
   if( !IS_NPC( victim ) && get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You haven't the power to succeed against them.\n\r", ch );
      return;
   }
   if( !str_cmp( argument, "coins" ) )
   {
      if( victim->gold < 1 )
      {
         send_to_char( "Drat, this one's got no gold to start with.\n\r", ch );
         return;
      }
      victim->gold = 0;
      act( AT_MAGIC, "$n gestures and an unearthly gale sends $N's coins flying!", ch, NULL, victim, TO_NOTVICT );
      act( AT_MAGIC, "You gesture and an unearthly gale sends $N's coins flying!", ch, NULL, victim, TO_CHAR );
      act( AT_MAGIC, "As $n gestures, an unearthly gale sends your currency flying!", ch, NULL, victim, TO_VICT );
      return;
   }
   for( ;; )
   {
      pRoomIndex = get_room_index( number_range( 0, sysdata.maxvnum ) );
      if( pRoomIndex )
         if( !IS_ROOM_FLAG( pRoomIndex, ROOM_PRIVATE )
             && !IS_ROOM_FLAG( pRoomIndex, ROOM_SOLITARY )
             && !IS_ROOM_FLAG( pRoomIndex, ROOM_PROTOTYPE )
             && !IS_ROOM_FLAG( pRoomIndex, ROOM_ISOLATED ) && !IS_ROOM_FLAG( pRoomIndex, ROOM_MAP ) )
            break;
   }
   if( !str_cmp( argument, "inventory" ) )
   {
      act( AT_MAGIC, "$n speaks a single word, sending $N's possessions flying!", ch, NULL, victim, TO_NOTVICT );
      act( AT_MAGIC, "You speak a single word, sending $N's possessions flying!", ch, NULL, victim, TO_CHAR );
      act( AT_MAGIC, "$n speaks a single word, sending your possessions flying!", ch, NULL, victim, TO_VICT );
      for( obj_lose = victim->first_carrying; obj_lose; obj_lose = obj_next )
      {
         obj_next = obj_lose->next_content;
         obj_from_char( obj_lose );
         obj_to_room( obj_lose, pRoomIndex, NULL );
         pager_printf( ch, "\t&w%s sent to %d\n\r", capitalize( obj_lose->short_descr ), pRoomIndex->vnum );
      }
      return;
   }
   send_to_char( "Strew their coins or inventory?\n\r", ch );
   return;
}

CMDF do_strip( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj_next, *obj_lose;
   int count = 0;

   set_char_color( AT_OBJECT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Strip who?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They're not here.\n\r", ch );
      return;
   }
   if( victim == ch )
   {
      send_to_char( "Kinky.\n\r", ch );
      return;
   }
   if( !IS_NPC( victim ) && get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You haven't the power to succeed against them.\n\r", ch );
      return;
   }
   act( AT_OBJECT, "Searching $N ...", ch, NULL, victim, TO_CHAR );
   for( obj_lose = victim->first_carrying; obj_lose; obj_lose = obj_next )
   {
      obj_next = obj_lose->next_content;
      obj_from_char( obj_lose );
      obj_to_char( obj_lose, ch );
      pager_printf( ch, "  &G... %s (&g%s) &Gtaken.\n\r", capitalize( obj_lose->short_descr ), obj_lose->name );
      count++;
   }
   if( !count )
      send_to_pager( "&GNothing found to take.\n\r", ch );
   return;
}

#define RESTORE_INTERVAL 21600
CMDF do_restore( CHAR_DATA * ch, char *argument )
{
   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Restore whom?\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      CHAR_DATA *vch, *vch_next;

      if( !ch->pcdata )
         return;

      if( get_trust( ch ) < LEVEL_SUB_IMPLEM )
      {
         if( IS_NPC( ch ) )
         {
            send_to_char( "You can't do that.\n\r", ch );
            return;
         }
         else
         {
            /*
             * Check if the player did a restore all within the last 18 hours. 
             */
            if( !IS_IMP( ch ) && current_time - last_restore_all_time < RESTORE_INTERVAL )
            {
               send_to_char( "Sorry, you can't do a restore all yet.\n\r", ch );
               interpret( ch, "restoretime" );
               return;
            }
         }
      }
      last_restore_all_time = current_time;
      ch->pcdata->restore_time = current_time;
      save_char_obj( ch );
      send_to_char( "Ok.\n\r", ch );
      for( vch = first_char; vch; vch = vch_next )
      {
         vch_next = vch->next;

         if( !IS_NPC( vch ) && !IS_IMMORTAL( vch ) && !CAN_PKILL( vch ) && !in_arena( vch ) )
         {
            vch->hit = vch->max_hit;
            vch->mana = vch->max_mana;
            vch->move = vch->max_move;
            if( vch->pcdata->condition[COND_FULL] != -1 )
               vch->pcdata->condition[COND_FULL] = sysdata.maxcondval;
            if( vch->pcdata->condition[COND_THIRST] != -1 )
               vch->pcdata->condition[COND_THIRST] = sysdata.maxcondval;
            vch->pcdata->condition[COND_DRUNK] = 0;
            vch->mental_state = 0;
            update_pos( vch );
            act( AT_IMMORT, "$n has restored you.", ch, NULL, vch, TO_VICT );
         }
      }
   }
   else
   {
      CHAR_DATA *victim;

      if( !( victim = get_char_world( ch, argument ) ) )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }

      if( get_trust( ch ) < LEVEL_LESSER && victim != ch && !( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) ) )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }

      victim->hit = victim->max_hit;
      victim->mana = victim->max_mana;
      victim->move = victim->max_move;
      if( !IS_NPC( victim ) )
      {
         if( victim->pcdata->condition[COND_FULL] != -1 )
            victim->pcdata->condition[COND_FULL] = sysdata.maxcondval;
         if( victim->pcdata->condition[COND_THIRST] != -1 )
            victim->pcdata->condition[COND_THIRST] = sysdata.maxcondval;
         victim->pcdata->condition[COND_DRUNK] = 0;
         victim->mental_state = 0;
      }
      update_pos( victim );
      if( ch != victim )
         act( AT_IMMORT, "$n has restored you.", ch, NULL, victim, TO_VICT );
      send_to_char( "Ok.\n\r", ch );
      return;
   }
}

CMDF do_restoretime( CHAR_DATA * ch, char *argument )
{
   long int time_passed;
   int hour, minute;

   set_char_color( AT_IMMORT, ch );

   if( !last_restore_all_time )
      send_to_char( "There has been no restore all since reboot.\n\r", ch );
   else
   {
      time_passed = current_time - last_restore_all_time;
      hour = ( int )( time_passed / 3600 );
      minute = ( int )( ( time_passed - ( hour * 3600 ) ) / 60 );
      ch_printf( ch, "The  last restore all was %d hours and %d minutes ago.\n\r", hour, minute );
   }

   if( !ch->pcdata )
      return;

   if( !ch->pcdata->restore_time )
   {
      send_to_char( "You have never done a restore all.\n\r", ch );
      return;
   }

   time_passed = current_time - ch->pcdata->restore_time;
   hour = ( int )( time_passed / 3600 );
   minute = ( int )( ( time_passed - ( hour * 3600 ) ) / 60 );
   ch_printf( ch, "Your last restore all was %d hours and %d minutes ago.\n\r", hour, minute );
   return;
}

CMDF do_freeze( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Freeze whom?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed, and they saw...\n\r", ch );
      ch_printf( victim, "%s is attempting to freeze you.\n\r", ch->name );
      return;
   }

   if ( victim->desc && victim->desc->original && get_trust(victim->desc->original) >= get_trust(ch) )
   {
      send_to_char( "For some inexplicable reason, you failed.\r\n", ch );
      return;
   }

   if( IS_PLR_FLAG( victim, PLR_FREEZE ) )
   {
      REMOVE_PLR_FLAG( victim, PLR_FREEZE );
      send_to_char( "&CYour frozen form suddenly thaws.\n\r", victim );
      ch_printf( ch, "&C%s is now unfrozen.\n\r", victim->name );
   }
   else
   {
      if ( victim->switched )
      {
         do_return( victim->switched, "" );
         set_char_color( AT_LBLUE, victim );
      }
      SET_PLR_FLAG( victim, PLR_FREEZE );
      send_to_char( "&CA godly force turns your body to ice!\n\r", victim );
      ch_printf( ch, "&CYou have frozen %s.\n\r", victim->name );
   }
   save_char_obj( victim );
   return;
}

CMDF do_log( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Log whom?\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      if( fLogAll )
      {
         fLogAll = FALSE;
         send_to_char( "Log ALL off.\n\r", ch );
      }
      else
      {
         fLogAll = TRUE;
         send_to_char( "Log ALL on.\n\r", ch );
      }
      return;
   }

   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   /*
    * No level check, gods can log anyone.
    */
   if( IS_PLR_FLAG( victim, PLR_LOG ) )
   {
      REMOVE_PLR_FLAG( victim, PLR_LOG );
      ch_printf( ch, "LOG removed from %s.\n\r", victim->name );
   }
   else
   {
      SET_PLR_FLAG( victim, PLR_LOG );
      ch_printf( ch, "LOG applied to %s.\n\r", victim->name );
   }
   return;
}

CMDF do_litterbug( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Set litterbug flag on whom?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }
   if( IS_PLR_FLAG( victim, PLR_LITTERBUG ) )
   {
      REMOVE_PLR_FLAG( victim, PLR_LITTERBUG );
      send_to_char( "You can drop items again.\n\r", victim );
      ch_printf( ch, "LITTERBUG removed from %s.\n\r", victim->name );
   }
   else
   {
      SET_PLR_FLAG( victim, PLR_LITTERBUG );
      send_to_char( "A strange force prevents you from dropping any more items!\n\r", victim );
      ch_printf( ch, "LITTERBUG set on %s.\n\r", victim->name );
   }
   return;
}

CMDF do_noemote( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Noemote whom?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }
   if( IS_PLR_FLAG( victim, PLR_NO_EMOTE ) )
   {
      REMOVE_PLR_FLAG( victim, PLR_NO_EMOTE );
      send_to_char( "You can emote again.\n\r", victim );
      ch_printf( ch, "NOEMOTE removed from %s.\n\r", victim->name );
   }
   else
   {
      SET_PLR_FLAG( victim, PLR_NO_EMOTE );
      send_to_char( "You can't emote!\n\r", victim );
      ch_printf( ch, "NOEMOTE applied to %s.\n\r", victim->name );
   }
   return;
}

CMDF do_notell( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Notell whom?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }
   if( IS_PLR_FLAG( victim, PLR_NO_TELL ) )
   {
      REMOVE_PLR_FLAG( victim, PLR_NO_TELL );
      send_to_char( "You can send tells again.\n\r", victim );
      ch_printf( ch, "NOTELL removed from %s.\n\r", victim->name );
   }
   else
   {
      SET_PLR_FLAG( victim, PLR_NO_TELL );
      send_to_char( "You can't send tells!\n\r", victim );
      ch_printf( ch, "NOTELL applied to %s.\n\r", victim->name );
   }
   return;
}

CMDF do_notitle( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   char buf[MSL];

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Notitle whom?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }
   if( IS_PCFLAG( victim, PCFLAG_NOTITLE ) )
   {
      REMOVE_PCFLAG( victim, PCFLAG_NOTITLE );
      send_to_char( "You can set your own title again.\n\r", victim );
      ch_printf( ch, "NOTITLE removed from %s.\n\r", victim->name );
   }
   else
   {
      SET_PCFLAG( victim, PCFLAG_NOTITLE );
      snprintf( buf, MSL, "the %s", title_table[victim->Class][victim->level][victim->sex == SEX_FEMALE ? 1 : 0] );
      set_title( victim, buf );
      send_to_char( "You can't set your own title!\n\r", victim );
      ch_printf( ch, "NOTITLE set on %s.\n\r", victim->name );
   }
   return;
}

CMDF do_silence( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Silence whom?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }
   if( IS_PLR_FLAG( victim, PLR_SILENCE ) )
      send_to_char( "Player already silenced, use unsilence to remove.\n\r", ch );
   else
   {
      SET_PLR_FLAG( victim, PLR_SILENCE );
      send_to_char( "You can't use channels!\n\r", victim );
      ch_printf( ch, "You SILENCE %s.\n\r", victim->name );
   }
   return;
}

/* Much better than toggling this with do_silence, yech --Blodkai */
CMDF do_unsilence( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Unsilence whom?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }
   if( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }
   if( IS_PLR_FLAG( victim, PLR_SILENCE ) )
   {
      REMOVE_PLR_FLAG( victim, PLR_SILENCE );
      send_to_char( "You can use channels again.\n\r", victim );
      ch_printf( ch, "SILENCE removed from %s.\n\r", victim->name );
   }
   else
   {
      send_to_char( "That player is not silenced.\n\r", ch );
   }
   return;
}

CMDF do_peace( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *rch;

   act( AT_IMMORT, "$n booms, 'PEACE!'", ch, NULL, NULL, TO_ROOM );
   act( AT_IMMORT, "You boom, 'PEACE!'", ch, NULL, NULL, TO_CHAR );
   for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
   {
      if( rch->fighting )
      {
         stop_fighting( rch, TRUE );
         interpret( rch, "sit" );
      }

      /*
       * Added by Narn, Nov 28/95 
       */
      stop_hating( rch );
      stop_hunting( rch );
      stop_fearing( rch );
   }
   send_to_char( "&YOk.\n\r", ch );
   return;
}

CMDF do_lockdown( CHAR_DATA * ch, char *argument )
{
   sysdata.LOCKDOWN = !sysdata.LOCKDOWN;

   save_sysdata( sysdata );

   if( sysdata.LOCKDOWN )
      echo_to_all( AT_RED, "Total lockdown activated.", ECHOTAR_IMM );
   else
      echo_to_all( AT_RED, "Total lockdown deactivated.", ECHOTAR_IMM );
   return;
}

CMDF do_implock( CHAR_DATA * ch, char *argument )
{
   sysdata.IMPLOCK = !sysdata.IMPLOCK;

   save_sysdata( sysdata );

   if( sysdata.IMPLOCK )
      echo_to_all( AT_RED, "Implock activated.", ECHOTAR_IMM );
   else
      echo_to_all( AT_RED, "Implock deactivated.", ECHOTAR_IMM );
   return;
}

CMDF do_wizlock( CHAR_DATA * ch, char *argument )
{
   sysdata.WIZLOCK = !sysdata.WIZLOCK;

   save_sysdata( sysdata );

   if( sysdata.WIZLOCK )
      echo_to_all( AT_RED, "Wizlock activated.", ECHOTAR_IMM );
   else
      echo_to_all( AT_RED, "Wizlock deactivated.", ECHOTAR_IMM );
   return;
}

CMDF do_noresolve( CHAR_DATA * ch, char *argument )
{
   sysdata.NO_NAME_RESOLVING = !sysdata.NO_NAME_RESOLVING;

   save_sysdata( sysdata );

   if( sysdata.NO_NAME_RESOLVING )
      send_to_char( "&YName resolving disabled.\n\r", ch );
   else
      send_to_char( "&YName resolving enabled.\n\r", ch );
   return;
}

/* Output of command reformmated by Samson 2-8-98, and again on 4-7-98 */
CMDF do_users( CHAR_DATA * ch, char *argument )
{
   DESCRIPTOR_DATA *d;
   int count;
   char *st;

   set_pager_color( AT_PLAIN, ch );

   count = 0;
   send_to_pager( "Desc|     Constate      |Idle|    Player    | HostIP                   \n\r", ch );
   send_to_pager( "----+-------------------+----+--------------+--------------------------\n\r", ch );
#ifdef I3
   if( I3_is_connected(  ) )
      pager_printf( ch, " %3d| %-17s |%4d| %-12s | %s \n\r", I3_socket, "Connected", 0, "Intermud-3", I3_ROUTER_NAME );
#endif
#ifdef IMC
   if( this_imcmud->state == IMC_ONLINE )
      pager_printf( ch, " %3d| %-17s |%4d| %-12s | %s \n\r",
                    this_imcmud->desc, "Connected", 0, "IMC2", this_imcmud->network );
#endif
#ifdef WEBSVR
   if( web_socket > 0 )
      pager_printf( ch, " %3d| %-17s |%4d| %-12s | %s \n\r", web_socket, "Connected", 0, "Web", "Internal Webserver" );
#endif
   for( d = first_descriptor; d; d = d->next )
   {
      switch ( d->connected )
      {
         case CON_PLAYING:
            st = "Playing";
            break;
         case CON_GET_NAME:
            st = "Get name";
            break;
         case CON_GET_OLD_PASSWORD:
            st = "Get password";
            break;
         case CON_CONFIRM_NEW_NAME:
            st = "Confirm name";
            break;
         case CON_GET_NEW_PASSWORD:
            st = "New password";
            break;
         case CON_CONFIRM_NEW_PASSWORD:
            st = "Confirm password";
            break;
         case CON_GET_NEW_SEX:
            st = "Get sex";
            break;
         case CON_READ_MOTD:
            st = "Reading MOTD";
            break;
         case CON_EDITING:
            st = "In line editor";
            break;
         case CON_PRESS_ENTER:
            st = "Press enter";
            break;
         case CON_PRIZENAME:
            st = "Sindhae prizename";
            break;
         case CON_CONFIRMPRIZENAME:
            st = "Confirming prize";
            break;
         case CON_PRIZEKEY:
            st = "Sindhae keywords";
            break;
         case CON_CONFIRMPRIZEKEY:
            st = "Confirming keywords";
            break;
         case CON_ROLL_STATS:
            st = "Rolling stats";
            break;
         case CON_RAISE_STAT:
            st = "Raising stats";
            break;
         case CON_DELETE:
            st = "Confirm delete";
            break;
         case CON_FORKED:
            st = "Command shell";
            break;
         case CON_GET_PORT_PASSWORD:
            st = "Access code";
            break;
         case CON_COPYOVER_RECOVER:
            st = "Hotboot";
            break;
         case CON_PLOADED:
            st = "Ploaded";
            break;
         case CON_MEDIT:
         case CON_OEDIT:
         case CON_REDIT:
            st = "Using OLC";
            break;
         default:
            st = "Invalid!!!!";
            break;
      }

      if( !argument || argument[0] == '\0' )
      {
         if( IS_IMP( ch ) || ( d->character && can_see( ch, d->character, TRUE ) && !is_ignoring( d->character, ch ) ) )
         {
            count++;
            pager_printf( ch, " %3d| %-17s |%4d| %-12s | %s \n\r", d->descriptor, st, d->idle / 4,
                          d->original ? d->original->name : d->character ? d->character->name : "(None!)", d->host );
         }
      }
      else
      {
         if( ( get_trust( ch ) >= LEVEL_SUPREME || ( d->character && can_see( ch, d->character, TRUE ) ) )
             && ( !str_prefix( argument, d->host ) || ( d->character && !str_prefix( argument, d->character->name ) ) ) )
         {
            count++;
            pager_printf( ch, " %3d| %2d|%4d| %-12s | %s \n\r", d->descriptor, d->connected, d->idle / 4,
                          d->original ? d->original->name : d->character ? d->character->name : "(None!)", d->host );
         }
      }
   }
   pager_printf( ch, "%d user%s.\n\r", count, count == 1 ? "" : "s" );
   return;
}

/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
/* Samson 4-7-98, added protection against forced delete */
CMDF do_force( CHAR_DATA * ch, char *argument )
{
   CMDTYPE *cmd = NULL;
   char arg[MIL], arg2[MIL], command[MSL];
   bool mobsonly;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );
   if( !arg || arg[0] == '\0' || !arg2 || arg2[0] == '\0' )
   {
      send_to_char( "Force whom to do what?\n\r", ch );
      return;
   }

   mobsonly = ch->level < sysdata.level_forcepc;
   cmd = find_command( arg2 );
   snprintf( command, MSL, "%s %s", arg2, argument );

   if( !str_cmp( arg, "all" ) )
   {
      CHAR_DATA *vch, *vch_next;

      if( mobsonly )
      {
         send_to_char( "You are not of sufficient level to force players yet.\n\r", ch );
         return;
      }

      if( cmd && IS_CMD_FLAG( cmd, CMD_NOFORCE ) )
      {
         ch_printf( ch, "You cannot force anyone to %s\n\r", cmd->name );
         log_printf( "%s attempted to force all to %s - command is flagged noforce", ch->name, cmd->name );
         return;
      }

      for( vch = first_char; vch; vch = vch_next )
      {
         vch_next = vch->next;

         if( !IS_NPC( vch ) && get_trust( vch ) < get_trust( ch ) )
         {
            act( AT_IMMORT, "$n forces you to '$t'.", ch, command, vch, TO_VICT );
            interpret( vch, command );
         }
      }
   }
   else
   {
      CHAR_DATA *victim;

      if( !( victim = get_char_world( ch, arg ) ) )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }

      if( victim == ch )
      {
         send_to_char( "Aye aye, right away!\n\r", ch );
         return;
      }

      if( ( get_trust( victim ) >= get_trust( ch ) ) || ( mobsonly && !IS_NPC( victim ) ) )
      {
         send_to_char( "Do it yourself!\n\r", ch );
         return;
      }

      if( cmd && IS_CMD_FLAG( cmd, CMD_NOFORCE ) )
      {
         ch_printf( ch, "You cannot force anyone to %s\n\r", cmd->name );
         log_printf( "%s attempted to force %s to %s - command is flagged noforce", ch->name, victim->name, cmd->name );
         return;
      }

      if( ch->level < LEVEL_GOD && IS_NPC( victim ) && !str_prefix( "mp", argument ) )
      {
         send_to_char( "You can't force a mob to do that!\n\r", ch );
         return;
      }
      act( AT_IMMORT, "$n forces you to '$t'.", ch, command, victim, TO_VICT );
      interpret( victim, command );
   }
   send_to_char( "Ok.\n\r", ch );
   return;
}

CMDF do_invis( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   short level;

   set_char_color( AT_IMMORT, ch );

   /*
    * if ( IS_NPC(ch)) return; 
    */

   argument = one_argument( argument, arg );
   if( arg && arg[0] != '\0' )
   {
      if( !is_number( arg ) )
      {
         send_to_char( "Usage: invis | invis <level>\n\r", ch );
         return;
      }
      level = atoi( arg );
      if( level < 2 || level > get_trust( ch ) )
      {
         send_to_char( "Invalid level.\n\r", ch );
         return;
      }

      if( !IS_NPC( ch ) )
      {
         ch->pcdata->wizinvis = level;
         ch_printf( ch, "Wizinvis level set to %d.\n\r", level );
      }

      else
      {
         ch->mobinvis = level;
         ch_printf( ch, "Mobinvis level set to %d.\n\r", level );
      }
      return;
   }

   if( !IS_NPC( ch ) )
   {
      if( ch->pcdata->wizinvis < 2 )
         ch->pcdata->wizinvis = ch->level;
   }
   else
   {
      if( ch->mobinvis < 2 )
         ch->mobinvis = ch->level;
   }

   if( !IS_NPC( ch ) )
   {
      save_char_obj( ch );
      if( IS_PLR_FLAG( ch, PLR_WIZINVIS ) )
      {
         REMOVE_PLR_FLAG( ch, PLR_WIZINVIS );
         act( AT_IMMORT, "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
         send_to_char( "You slowly fade back into existence.\n\r", ch );
      }
      else
      {
         act( AT_IMMORT, "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
         send_to_char( "You slowly vanish into thin air.\n\r", ch );
         SET_PLR_FLAG( ch, PLR_WIZINVIS );
      }
   }
   else
   {
      if( IS_ACT_FLAG( ch, ACT_MOBINVIS ) )
      {
         REMOVE_ACT_FLAG( ch, ACT_MOBINVIS );
         act( AT_IMMORT, "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
         send_to_char( "You slowly fade back into existence.\n\r", ch );
      }
      else
      {
         act( AT_IMMORT, "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
         send_to_char( "You slowly vanish into thin air.\n\r", ch );
         SET_ACT_FLAG( ch, ACT_MOBINVIS );
      }
   }
   return;
}

CMDF do_holylight( CHAR_DATA * ch, char *argument )
{
   set_char_color( AT_IMMORT, ch );

   if( IS_NPC( ch ) )
      return;

   xTOGGLE_BIT( ch->act, PLR_HOLYLIGHT );

   if( IS_PLR_FLAG( ch, PLR_HOLYLIGHT ) )
      send_to_char( "Holy light mode on.\n\r", ch );
   else
      send_to_char( "Holy light mode off.\n\r", ch );
   return;
}

/*
 * Load up a player file
 */
CMDF do_loadup( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *temp;
   char fname[256];
   struct stat fst;
   bool loaded;
   DESCRIPTOR_DATA *d;
   int old_room_vnum;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: loadup <playername>\n\r", ch );
      return;
   }
   for( temp = first_char; temp; temp = temp->next )
   {
      if( IS_NPC( temp ) )
         continue;
      if( can_see( ch, temp, TRUE ) && !str_cmp( argument, temp->name ) )
         break;
   }
   if( temp != NULL )
   {
      send_to_char( "They are already playing.\n\r", ch );
      return;
   }
   argument[0] = UPPER( argument[0] );
   snprintf( fname, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ) );

   /*
    * Bug fix here provided by Senir to stop /dev/null crash 
    */
   if( stat( fname, &fst ) == -1 || !check_parse_name( capitalize( argument ), FALSE ) )
   {
      send_to_char( "&YNo such player exists.\n\r", ch );
      return;
   }

   if( stat( fname, &fst ) != -1 )
   {
      CREATE( d, DESCRIPTOR_DATA, 1 );
      d->next = NULL;
      d->prev = NULL;
      d->connected = CON_PLOADED;
      d->outsize = 2000;
      CREATE( d->outbuf, char, d->outsize );

      loaded = load_char_obj( d, argument, FALSE, FALSE );
      LINK( d->character, first_char, last_char, next, prev );
      old_room_vnum = d->character->in_room->vnum;
      if( !char_to_room( d->character, ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      if( get_trust( d->character ) >= get_trust( ch ) )
      {
         interpret( d->character, "say How dare you load my Pfile!" );
         cmdf( d->character, "dino %s", ch->name );
         ch_printf( ch, "I think you'd better leave %s alone!\n\r", argument );
         d->character->desc = NULL;
         interpret( d->character, "quit auto" );
         return;
      }
      d->character->desc = NULL;
      d->character = NULL;
      DISPOSE( d->outbuf );
      DISPOSE( d );
      ch_printf( ch, "&R%s loaded from room %d.\n\r", capitalize( argument ), old_room_vnum );
      act_printf( AT_IMMORT, ch, NULL, NULL, TO_ROOM, "%s appears from nowhere, eyes glazed over.", capitalize( argument ) );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   /*
    * else no player file 
    */
   send_to_char( "No such player.\n\r", ch );
   return;
}

/*
 * Extract area names from "input" string and place result in "output" string
 * e.g. "aset joe.are sedit susan.are cset" --> "joe.are susan.are"
 * - Gorog
 */
 /*
  * Rewrite by Xorith. 12/1/03
  */
char *extract_area_names( CHAR_DATA * ch )
{
   char tarea[MSL];
   char *tbuf = NULL;
   static char area_names[MSL];

   if( !ch || IS_NPC( ch ) )
      return NULL;

   if( !ch->pcdata->bestowments || ch->pcdata->bestowments[0] == '\0' )
      return NULL;

   tarea[0] = '\0';
   area_names[0] = '\0';
   tbuf = ch->pcdata->bestowments;
   tbuf = one_argument( tbuf, tarea );
   if( !tarea || tarea[0] == '\0' )
      return NULL;
   while( 1 )
   {
      if( strstr( tarea, ".are" ) )
      {
         if( !area_names || area_names[0] == '\0' )
            mudstrlcpy( area_names, tarea, MSL );
         else
            snprintf( area_names + strlen( area_names ), MSL, " %s", tarea );
      }
      if( !tbuf || tbuf[0] == '\0' )
         break;
      tbuf = one_argument( tbuf, tarea );
   }
   return area_names;
}

char *extract_command_names( CHAR_DATA * ch )
{
   char tcomm[MSL];
   char *tbuf = NULL;
   static char comm_names[MSL];

   if( !ch || IS_NPC( ch ) )
      return NULL;

   if( !ch->pcdata->bestowments || ch->pcdata->bestowments[0] == '\0' )
      return NULL;

   tcomm[0] = '\0';
   comm_names[0] = '\0';

   tbuf = ch->pcdata->bestowments;
   tbuf = one_argument( tbuf, tcomm );
   if( !tcomm || tcomm[0] == '\0' )
      return NULL;
   while( 1 )
   {
      if( !strstr( tcomm, ".are" ) )
      {
         if( !comm_names || comm_names[0] == '\0' )
            mudstrlcpy( comm_names, tcomm, MSL );
         else
            snprintf( comm_names + strlen( comm_names ), MSL, " %s", tcomm );
      }
      if( !tbuf || tbuf[0] == '\0' )
         break;
      tbuf = one_argument( tbuf, tcomm );

   }
   return comm_names;
}

/*
 * Remove area names from "input" string and place result in "output" string
 * e.g. "aset joe.are sedit susan.are cset" --> "aset sedit cset"
 * - Gorog
 */
void remove_area_names( CHAR_DATA * ch )
{
   char tarea[MSL];
   char *buf = NULL;

   if( !ch || IS_NPC( ch ) )
      return;

   if( !ch->pcdata->bestowments || ch->pcdata->bestowments[0] == '\0' )
      return;

   tarea[0] = '\0';
   buf = ch->pcdata->bestowments;
   buf = one_argument( buf, tarea );
   if( !tarea || tarea[0] == '\0' )
      return;
   while( 1 )
   {
      if( strstr( tarea, ".are" ) )
         removename( &ch->pcdata->bestowments, tarea );
      if( !buf || buf[0] == '\0' )
         break;
      buf = one_argument( buf, tarea );
   }
   return;
}

/*
 * Allows members of the Area Council to add Area names to the bestow field.
 * Area names mus end with ".are" so that no commands can be bestowed.
 */
/* Function modified from original form - Samson */
/* Function revamped by Xorith on 12/1/03 */
CMDF do_bestowarea( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   char *buf;
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );

#ifdef MULTIPORT
   if( port != BUILDPORT && !IS_IMP( ch ) )
   {
      send_to_char( "Only an implementor may bestow an area on this port.\n\r", ch );
      return;
   }
#endif

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax:\n\r"
                    "bestowarea <victim> <filename>.are\n\r"
                    "bestowarea <victim> none             removes bestowed areas\n\r"
                    "bestowarea <victim> remove <filename>.are    removes a specific area\n\r"
                    "bestowarea <victim> list             lists bestowed areas\n\r"
                    "bestowarea <victim>                  lists bestowed areas\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( victim = get_char_world( ch, arg ) ) )
   {
      ch_printf( ch, "Player '%s' is not online.\n\r", arg );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Mobiles may not have areas bestowed upon them.\n\r", ch );
      return;
   }

   if( victim == ch || get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You aren't powerful enough...\n\r", ch );
      return;
   }

   if( get_trust( victim ) < LEVEL_IMMORTAL )
   {
      ch_printf( ch, "%s is not an immortal and may not have an area bestowed.\n\r", victim->name );
      return;
   }

   if( !argument || argument[0] == '\0' || !str_cmp( argument, "list" ) )
   {
      if( !victim->pcdata->bestowments || victim->pcdata->bestowments[0] == '\0' )
      {
         ch_printf( ch, "%s does not have any areas bestowed upon them.\n\r", victim->name );
         return;
      }
      if( !( buf = extract_area_names( victim ) ) || buf[0] == '\0' )
         ch_printf( ch, "%s does not have any areas bestowed upon them.\n\r", victim->name );
      else
         ch_printf( ch, "%s's bestowed areas: %s\n\r", victim->name, buf );
      return;
   }

   argument = one_argument( argument, arg );

   if( argument && argument[0] != '\0' && !str_cmp( arg, "remove" ) )
   {
      while( 1 )
      {
         argument = one_argument( argument, arg );
         if( !hasname( victim->pcdata->bestowments, arg ) )
         {
            ch_printf( ch, "%s does not have an area named %s bestowed.\n\r", victim->name, arg );
            return;
         }
         removename( &victim->pcdata->bestowments, arg );
         ch_printf( ch, "Removed area %s from %s.\n\r", arg, victim->name );
         save_char_obj( victim );
         if( !argument || argument[0] == '\0' )
            break;
      }
      return;
   }

   if( !str_cmp( arg, "none" ) )
   {
      if( !victim->pcdata->bestowments || !strstr( victim->pcdata->bestowments, ".are" ) )
      {
         ch_printf( ch, "%s has no areas bestowed!\n\r", victim->name );
         return;
      }
      remove_area_names( victim );
      save_char_obj( victim );
      ch_printf( ch, "All of %s's bestowed areas have been removed.\n\r", victim->name );
      return;
   }

   while( 1 )
   {
      if( !strstr( arg, ".are" ) )
      {
         ch_printf( ch, "'%s' is not a valid area to bestow.\n\r", arg );
         send_to_char( "You can only bestow an area name\n\r", ch );
         send_to_char( "E.G. bestow joe sam.are\n\r", ch );
         return;
      }

      if( hasname( victim->pcdata->bestowments, arg ) )
      {
         ch_printf( ch, "%s already has the area %s bestowed.\n\r", victim->name, arg );
         return;
      }

      smash_tilde( arg );
      addname( &victim->pcdata->bestowments, arg );
      ch_printf( victim, "%s has bestowed on you the area: %s\n\r", ch->name, arg );
      ch_printf( ch, "%s has been bestowed: %s\n\r", victim->name, arg );
      save_char_obj( victim );
      if( !argument || argument[0] == '\0' )
         break;
      else
         argument = one_argument( argument, arg );
   }
   return;
}

CMDF do_bestow( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   char *buf = NULL;
   CHAR_DATA *victim;
   CMDTYPE *cmd;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax:\n\r"
                    "bestow <victim> <command>           adds a command to the list\n\r"
                    "bestow <victim> none                removes bestowed areas\n\r"
                    "bestow <victim> remove <command>    removes a specific area\n\r"
                    "bestow <victim> list                lists bestowed areas\n\r"
                    "bestow <victim>                     lists bestowed areas\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( victim = get_char_world( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "You can't give special abilities to a mob!\n\r", ch );
      return;
   }
   if( victim == ch || get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You aren't powerful enough...\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' || !str_cmp( argument, "list" ) )
   {
      if( !victim->pcdata->bestowments || victim->pcdata->bestowments[0] == '\0' )
      {
         ch_printf( ch, "%s has no bestowed commands.\n\r", victim->name );
         return;
      }

      if( !( buf = extract_command_names( victim ) ) || buf[0] == '\0' )
      {
         ch_printf( ch, "%s has no bestowed commands.\n\r", victim->name );
         return;
      }
      ch_printf( ch, "Current bestowed commands on %s: %s.\n\r", victim->name, buf );
      return;
   }

   argument = one_argument( argument, arg );

   if( argument && argument[0] != '\0' && !str_cmp( arg, "remove" ) )
   {
      while( 1 )
      {
         argument = one_argument( argument, arg );
         if( !hasname( victim->pcdata->bestowments, arg ) )
         {
            ch_printf( ch, "%s does not have a command named %s bestowed.\n\r", victim->name, arg );
            return;
         }
         removename( &victim->pcdata->bestowments, arg );
         ch_printf( ch, "Removed command %s from %s.\n\r", arg, victim->name );
         save_char_obj( victim );
         if( !argument || argument[0] == '\0' )
            break;
      }
      return;
   }

   if( !str_cmp( arg, "none" ) )
   {
      if( !victim->pcdata->bestowments || victim->pcdata->bestowments[0] == '\0' ||
          !( buf = extract_command_names( victim ) ) || buf[0] == '\0' )
      {
         ch_printf( ch, "%s has no commands bestowed!\n\r", victim->name );
         return;
      }
      buf = NULL;
      if( strstr( victim->pcdata->bestowments, ".are" ) )
         buf = extract_area_names( victim );
      STRFREE( victim->pcdata->bestowments );
      if( buf && buf[0] != '\0' )
         victim->pcdata->bestowments = STRALLOC( buf );
      else
         victim->pcdata->bestowments = STRALLOC( "" );
      ch_printf( ch, "Command bestowments removed from %s.\n\r", victim->name );
      ch_printf( victim, "%s has removed your bestowed commands.\n\r", ch->name );
      check_switch( victim, FALSE );
      save_char_obj( victim );
      return;
   }

   while( 1 )
   {
      if( strstr( arg, ".are" ) )
      {
         ch_printf( ch, "'%s' is not a valid command to bestow.\n\r", arg );
         send_to_char( "You cannot bestow an area with 'bestow'. Use 'bestowarea'.\n\r", ch );
         return;
      }

      if( hasname( victim->pcdata->bestowments, arg ) )
      {
         ch_printf( ch, "%s already has '%s' bestowed.\n\r", victim->name, arg );
         return;
      }

      if( !( cmd = find_command( arg ) ) )
      {
         ch_printf( ch, "'%s' is not a valid command.\n\r", arg );
         return;
      }

      if( cmd->level > get_trust( ch ) )
      {
         ch_printf( ch, "The command '%s' is beyond you, thus you cannot bestow it.\n\r", arg );
         return;
      }

      smash_tilde( arg );
      addname( &victim->pcdata->bestowments, arg );
      ch_printf( victim, "%s has bestowed on you the command: %s\n\r", ch->name, arg );
      ch_printf( ch, "%s has been bestowed: %s\n\r", victim->name, arg );
      save_char_obj( victim );
      if( !argument || argument[0] == '\0' )
         break;
      else
         argument = one_argument( argument, arg );
   }
   return;
}

/* Immortal demotion command, modified from do_advance by Samson 1-18-98 */
CMDF do_demote( CHAR_DATA * ch, char *argument )
{
   char buf[256], buf2[256];
   CHAR_DATA *victim;
   char *die;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax: demote <char>.\n\r", ch );
      return;
   }

   if( ( victim = get_char_world( ch, argument ) ) == NULL )
   {
      send_to_char( "That player is not here.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Oh, please, come now. Don't mess with the NPCs.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Why on earth would you want to do that?\n\r", ch );
      return;
   }

   if( victim->level >= ch->level )
   {
      send_to_char( "You cannot demote someone who is at or above your level.\n\r", ch );
      return;
   }

   if( victim->level < LEVEL_IMMORTAL )
   {
      ch_printf( ch, "%s is not immortal, and cannot be demoted. Use the advance command.\n\r", victim->name );
      return;
   }

   send_to_char( "Demoting an immortal!\n\r", ch );
   send_to_char( "You have been demoted!\n\r", victim );

   victim->level -= 1;

   /*
    * Rank fix added by Narn. 
    */
   STRFREE( victim->pcdata->rank );

   advance_level( victim );
   victim->exp = exp_level( victim->level );
   victim->trust = 0;

   /*
    * Stuff added to make sure players wizinvis level doesnt stay higher 
    * * than their actual level and to take wizinvis away from advance below 100
    */
   if( IS_PLR_FLAG( victim, PLR_WIZINVIS ) )
      victim->pcdata->wizinvis = victim->level;

   if( IS_PLR_FLAG( victim, PLR_WIZINVIS ) && ( victim->level <= LEVEL_AVATAR ) )
   {
      REMOVE_PLR_FLAG( victim, PLR_WIZINVIS );
      victim->pcdata->wizinvis = 0;
   }

   if( victim->level == LEVEL_AVATAR )
   {
      REMOVE_PLR_FLAG( victim, PLR_HOLYLIGHT );
      die = victim->name;
      snprintf( buf, 256, "%s%s", GOD_DIR, capitalize( die ) );
      if( !remove( buf ) )
         send_to_char( "Player's immortal data destroyed.\n\r", ch );
      snprintf( buf2, 256, "%s.are", capitalize( die ) );
      send_to_char( "You have been thrown from the heavens by the Gods!\n\rYou are no longer immortal!\n\r", victim );
      REMOVE_PCFLAG( victim, PCFLAG_PASSDOOR );
      victim->pcdata->realm = 0;
   }
   snprintf( buf, 256, "%s none", victim->name );
   do_bestow( ch, buf );
   save_char_obj( victim );
   make_wizlist(  );
   build_wizinfo(  );
#ifdef I3
   if( this_i3mud && victim->level < this_i3mud->adminlevel )
      victim->pcdata->i3chardata->i3perm = I3PERM_IMM;
   if( this_i3mud && victim->level < this_i3mud->immlevel )
      victim->pcdata->i3chardata->i3perm = I3PERM_MORT;
#endif
#ifdef IMC
   if( this_imcmud && victim->level < this_imcmud->adminlevel )
      victim->pcdata->imcchardata->imcperm = IMCPERM_IMM;
   if( this_imcmud && victim->level < this_imcmud->immlevel )
      victim->pcdata->imcchardata->imcperm = IMCPERM_MORT;
#endif
   return;
}

/* Immortal promotion command, modified from do_advance by Samson 1-18-98 */
CMDF do_promote( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   int level;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax: promote <char>.\n\r", ch );
      return;
   }

   if( ( victim = get_char_world( ch, argument ) ) == NULL )
   {
      send_to_char( "That player is not here.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Nice try :P\n\r", ch );
      return;
   }

   if( victim->level >= ch->level )
   {
      send_to_char( "You cannot promote someone at or above your own level.\n\r", ch );
      return;
   }

   if( victim->level < LEVEL_AVATAR )
   {
      ch_printf( ch, "%s is not immortal, and cannot be promoted. Use the advance command.\n\r", victim->name );
      return;
   }

   if( victim->level == LEVEL_AVATAR )
   {
      ch_printf( ch, "You must use the immortalize command to make %s an immortal.\n\r", victim->name );
      return;
   }

   if( victim->level == LEVEL_IMMORTAL )
   {
      if( !victim->pcdata->email || !str_cmp( victim->pcdata->email, "" ) )
      {
         ch_printf( ch, "Cannot promote %s, their email information is missing.\n\r", victim->name );
         return;
      }
      if( victim->pcdata->realm == 0 )
      {
         ch_printf( ch, "Cannot promote %s, no realm has been assigned.\n\r", victim->name );
         return;
      }
   }

   level = victim->level + 1;
   if( level > MAX_LEVEL )
   {
      ch_printf( ch, "Cannot promote %s above the max level of %d.\n\r", victim->name, MAX_LEVEL );
      return;
   }

   act( AT_IMMORT, "You make some arcane gestures with your hands, then point a finger at $N!", ch, NULL, victim, TO_CHAR );
   act( AT_IMMORT, "$n makes some arcane gestures with $s hands, then points $s finger at you!", ch, NULL, victim, TO_VICT );
   act( AT_IMMORT, "$n makes some arcane gestures with $s hands, then points $s finger at $N!", ch, NULL, victim,
        TO_NOTVICT );
   set_char_color( AT_WHITE, victim );
   send_to_char( "&WYou suddenly feel very strange...\n\r\n\r", victim );

   switch ( level )
   {
      default:
         send_to_char( "You have been promoted!\n\r", victim );
         break;

      case LEVEL_ACOLYTE:
         do_help( victim, "M_GODLVL1_" );
         break;
      case LEVEL_CREATOR:
         do_help( victim, "M_GODLVL2_" );
         break;
      case LEVEL_SAVIOR:
         do_help( victim, "M_GODLVL3_" );
         break;
      case LEVEL_DEMI:
         do_help( victim, "M_GODLVL4_" );
         break;
      case LEVEL_TRUEIMM:
         do_help( victim, "M_GODLVL5_" );
         break;
      case LEVEL_LESSER:
         do_help( victim, "M_GODLVL6_" );
         break;
      case LEVEL_GOD:
         do_help( victim, "M_GODLVL7_" );
         break;
      case LEVEL_GREATER:
         do_help( victim, "M_GODLVL8_" );
         break;
      case LEVEL_ASCENDANT:
         do_help( victim, "M_GODLVL9_" );
         break;
      case LEVEL_SUB_IMPLEM:
         do_help( victim, "M_GODLVL10_" );
         break;
      case LEVEL_IMPLEMENTOR:
         do_help( victim, "M_GODLVL11_" );
         break;
      case LEVEL_KL:
         do_help( victim, "M_GODLVL12_" );
         break;
      case LEVEL_ADMIN:
         do_help( victim, "M_GODLVL13_" );
         break;
      case LEVEL_SUPREME:
         do_help( victim, "M_GODLVL15_" );
   }

   victim->level += 1;
   advance_level( victim );
   victim->exp = exp_level( victim->level );
   victim->trust = 0;
   STRFREE( victim->pcdata->rank );
   victim->pcdata->rank = STRALLOC( realm_string[victim->pcdata->realm] );
   save_char_obj( victim );
   make_wizlist(  );
   build_wizinfo(  );
   if( victim->alignment > 350 )
      echo_all_printf( AT_WHITE, ECHOTAR_ALL, "A bright white flash arcs across the sky as %s gains in power!",
                       victim->name );
   if( victim->alignment >= -350 && victim->alignment <= 350 )
      echo_all_printf( AT_GREY, ECHOTAR_ALL, "A dull grey flash arcs across the sky as %s gains in power!", victim->name );
   if( victim->alignment < -350 )
      echo_all_printf( AT_DGREY, ECHOTAR_ALL, "An eerie black flash arcs across the sky as %s gains in power!",
                       victim->name );
   return;
}

/* Online high level immortal command for displaying what the encryption
 * of a name/password would be, taking in 2 arguments - the name and the
 * password - can still only change the password if you have access to 
 * pfiles and the correct password
 */
/*
 * Roger Libiez (Samson, Alsherok) caught this
 * I forgot that this function even existed... *whoops*
 * Anyway, it is rewritten with bounds checking
 */
CMDF do_form_password( CHAR_DATA * ch, char *argument )
{
   char *pwcheck;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: formpass <password>\n\r", ch );
      return;
   }

   if( strlen( argument ) > 16 )
   {
      send_to_char( "Usage: formpass <password>\n\r", ch );
      send_to_char( "New password cannot exceed 16 characters in length.\n\r", ch );
      return;
   }

   if( strlen( argument ) < 5 )
   {
      send_to_char( "Usage: formpass <password>\n\r", ch );
      send_to_char( "New password must be at least 5 characters in length.\n\r", ch );
      return;
   }

   if( argument[0] == '!' )
   {
      send_to_char( "Usage: formpass <password>\n\r", ch );
      send_to_char( "New password cannot begin with the '!' character.\n\r", ch );
      return;
   }

   pwcheck = sha256_crypt( argument );
   ch_printf( ch, "%s results in the encrypted string: %s\n\r", argument, pwcheck );
   return;
}

/*
 * Purge a player file.  No more player.  -- Altrag
 */
CMDF do_destro( CHAR_DATA * ch, char *argument )
{
   send_to_char( "&RIf you want to destroy a character, spell it out!\n\r", ch );
   return;
}

/* Modified to update wizlist when imms delete. Samson - 4-16-98 */
CMDF do_destroy( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   char buf[256], buf2[256];
   char *name;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Destroy what player file?\n\r", ch );
      return;
   }

   if( !check_parse_name( capitalize( argument ), FALSE ) )
   {
      send_to_char( "That's not a valid player file name!\n\r", ch );
      return;
   }

   for( victim = first_char; victim; victim = victim->next )
      if( !IS_NPC( victim ) && !str_cmp( victim->name, argument ) )
         break;

   remove_from_auth( argument );

   if( !victim )
   {
      DESCRIPTOR_DATA *d;

      /*
       * Make sure they aren't halfway logged in. 
       */
      for( d = first_descriptor; d; d = d->next )
         if( ( victim = d->character ) && !IS_NPC( victim ) && !str_cmp( victim->name, argument ) )
            break;
      if( d )
         close_socket( d, TRUE );
   }
   else
   {
      int x, y;

      if( victim->pcdata->clan )
      {
         CLAN_DATA *clan = victim->pcdata->clan;

         if( !str_cmp( victim->name, clan->leader ) )
            STRFREE( clan->leader );
         if( !str_cmp( victim->name, clan->number1 ) )
            STRFREE( clan->number1 );
         if( !str_cmp( victim->name, clan->number2 ) )
            STRFREE( clan->number2 );
         remove_member( clan->name, victim->name );
         --clan->members;
         save_clan( clan );
         if( clan->members < 1 )
            delete_clan( ch, clan );
      }

      if( victim->pcdata->deity )
      {
         DEITY_DATA *deity = victim->pcdata->deity;

         --deity->worshippers;
         if( deity->worshippers < 1 )
            deity->worshippers = 0;
         save_deity( deity );
      }

      quitting_char = victim;
      save_char_obj( victim );
      saving_char = NULL;
      extract_char( victim, TRUE );
      for( x = 0; x < MAX_WEAR; x++ )
         for( y = 0; y < MAX_LAYERS; y++ )
            save_equipment[x][y] = NULL;
   }
   name = capitalize( argument );
   snprintf( buf, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), name );
   if( !remove( buf ) )
   {
      AREA_DATA *pArea;

      ch_printf( ch, "&RPlayer %s destroyed.\n\r", name );
      num_pfiles -= 1;

      snprintf( buf, 256, "%s%s", GOD_DIR, name );
      if( !remove( buf ) )
      {
         send_to_char( "Player's immortal data destroyed.\n\r", ch );
         make_wizlist(  );
         build_wizinfo(  );
      }
      else if( errno != ENOENT )
      {
         ch_printf( ch, "Unknown error #%d - %s (immortal data).  Report to Samson.\n\r", errno, strerror( errno ) );
         snprintf( buf2, 256, "%s destroying %s", ch->name, buf );
         perror( buf2 );
      }
      snprintf( buf2, 256, "%s.are", name );
      for( pArea = first_area; pArea; pArea = pArea->next )
         if( !str_cmp( pArea->filename, buf2 ) )
         {
            snprintf( buf, 256, "%s%s", BUILD_DIR, buf2 );
            fold_area( pArea, buf, FALSE );
            close_area( pArea );
            snprintf( buf2, 256, "%s.bak", buf );
            if( !rename( buf, buf2 ) )
               send_to_char( "&RPlayer's area data destroyed.  Area saved as backup.\n\r", ch );
            else if( errno != ENOENT )
            {
               ch_printf( ch, "&RUnknown error #%d - %s (area data).  Report to Samson.\n\r", errno, strerror( errno ) );
               snprintf( buf2, 256, "%s destroying %s", ch->name, buf );
               perror( buf2 );
            }
            break;
         }
   }
   else if( errno == ENOENT )
      send_to_char( "Player does not exist.\n\r", ch );
   else
   {
      ch_printf( ch, "&RUnknown error #%d - %s.  Report to Samson.\n\r", errno, strerror( errno ) );
      snprintf( buf, 256, "%s destroying %s", ch->name, argument );
      perror( buf );
   }
   return;
}

/* Super-AT command:
FOR ALL <action>
FOR MORTALS <action>
FOR GODS <action>
FOR MOBS <action>
FOR EVERYWHERE <action>

Executes action several times, either on ALL players (not including yourself),
MORTALS (including trusted characters), GODS (characters with level higher than
L_HERO), MOBS (Not recommended) or every room (not recommended either!)

If you insert a # in the action, it will be replaced by the name of the target.

If # is a part of the action, the action will be executed for every target
in game. If there is no #, the action will be executed for every room containg
at least one target, but only once per room. # cannot be used with FOR EVERY-
WHERE. # can be anywhere in the action.

Example: 

FOR ALL SMILE -> you will only smile once in a room with 2 players.
FOR ALL TWIDDLE # -> In a room with A and B, you will twiddle A then B.

Destroying the characters this command acts upon MAY cause it to fail. Try to
avoid something like FOR MOBS PURGE (although it actually works at my MUD).

FOR MOBS TRANS 3054 (transfer ALL the mobs to Midgaard temple) does NOT work
though :)

The command works by transporting the character to each of the rooms with 
target in them. Private rooms are not violated.

*/

/* Expand the name of a character into a string that identifies THAT
   character within a room. E.g. the second 'guard' -> 2. guard
*/
const char *name_expand( CHAR_DATA * ch )
{
   int count = 1;
   CHAR_DATA *rch;
   char name[MIL];   /*  HOPEFULLY no mob has a name longer than THAT */

   static char outbuf[MIL];

   if( !IS_NPC( ch ) )
      return ch->name;

   one_argument( ch->name, name );  /* copy the first word into name */

   if( !name[0] ) /* weird mob .. no keywords */
      return "";

   /*
    * ->people changed to ->first_person -- TRI 
    */
   for( rch = ch->in_room->first_person; rch && ( rch != ch ); rch = rch->next_in_room )
      if( is_name( name, rch->name ) )
         count++;

   snprintf( outbuf, MIL, "%d.%s", count, name );
   return outbuf;
}

CMDF do_for( CHAR_DATA * ch, char *argument )
{
   char range[MIL], buf[MSL];
   bool fGods = FALSE, fMortals = FALSE, fMobs = FALSE, fEverywhere = FALSE, found;
   ROOM_INDEX_DATA *room, *old_room;
   CHAR_DATA *p, *p_prev;  /* p_next to p_prev -- TRI */
   int i;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, range );
   if( !range || range[0] == '\0' || !argument || argument[0] == '\0' ) /* invalid usage? */
   {
      interpret( ch, "help for" );
      return;
   }

   if( !str_prefix( "quit", argument ) )
   {
      send_to_char( "Are you trying to crash the MUD or something?\n\r", ch );
      return;
   }

   if( !str_cmp( range, "all" ) )
   {
      fMortals = TRUE;
      fGods = TRUE;
   }
   else if( !str_cmp( range, "gods" ) )
      fGods = TRUE;
   else if( !str_cmp( range, "mortals" ) )
      fMortals = TRUE;
   else if( !str_cmp( range, "mobs" ) )
      fMobs = TRUE;
   else if( !str_cmp( range, "everywhere" ) )
      fEverywhere = TRUE;
   else
      interpret( ch, "help for" );  /* show syntax */

   /*
    * do not allow # to make it easier 
    */
   if( fEverywhere && strchr( argument, '#' ) )
   {
      send_to_char( "Cannot use FOR EVERYWHERE with the # thingie.\n\r", ch );
      return;
   }

   set_char_color( AT_PLAIN, ch );
   if( strchr( argument, '#' ) ) /* replace # ? */
   {
      /*
       * char_list - last_char, p_next - gch_prev -- TRI 
       */
      for( p = last_char; p; p = p_prev )
      {
         p_prev = p->prev; /* TRI */
         /*
          * p_next = p->next; 
          *//*
          * In case someone DOES try to AT MOBS SLAY # 
          */
         found = FALSE;

         if( !( p->in_room ) || room_is_private( p->in_room ) || ( p == ch ) )
            continue;

         if( IS_NPC( p ) && fMobs )
            found = TRUE;
         else if( !IS_NPC( p ) && p->level >= LEVEL_IMMORTAL && fGods )
            found = TRUE;
         else if( !IS_NPC( p ) && p->level < LEVEL_IMMORTAL && fMortals )
            found = TRUE;

         /*
          * It looks ugly to me.. but it works :) 
          */
         if( found ) /* p is 'appropriate' */
         {
            char *pSource = argument;  /* head of buffer to be parsed */
            char *pDest = buf;   /* parse into this */

            while( *pSource )
            {
               if( *pSource == '#' )   /* Replace # with name of target */
               {
                  const char *namebuf = name_expand( p );

                  if( namebuf )  /* in case there is no mob name ?? */
                     while( *namebuf ) /* copy name over */
                        *( pDest++ ) = *( namebuf++ );
                  pSource++;
               }
               else
                  *( pDest++ ) = *( pSource++ );
            }  /* while */
            *pDest = '\0'; /* Terminate */

            /*
             * Execute 
             */
            old_room = ch->in_room;
            char_from_room( ch );
            if( !char_to_room( ch, p->in_room ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            interpret( ch, buf );
            char_from_room( ch );
            if( !char_to_room( ch, old_room ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         }  /* if found */
      }  /* for every char */
   }
   else  /* just for every room with the appropriate people in it */
   {
      for( i = 0; i < MAX_KEY_HASH; i++ ) /* run through all the buckets */
         for( room = room_index_hash[i]; room; room = room->next )
         {
            found = FALSE;

            /*
             * Anyone in here at all? 
             */
            if( fEverywhere ) /* Everywhere executes always */
               found = TRUE;
            else if( !room->first_person )   /* Skip it if room is empty */
               continue;
            /*
             * ->people changed to first_person -- TRI 
             */

            /*
             * Check if there is anyone here of the requried type 
             */
            /*
             * Stop as soon as a match is found or there are no more ppl in room 
             */
            /*
             * ->people to ->first_person -- TRI 
             */
            for( p = room->first_person; p && !found; p = p->next_in_room )
            {
               if( p == ch )  /* do not execute on oneself */
                  continue;

               if( IS_NPC( p ) && fMobs )
                  found = TRUE;
               else if( !IS_NPC( p ) && ( p->level >= LEVEL_IMMORTAL ) && fGods )
                  found = TRUE;
               else if( !IS_NPC( p ) && ( p->level <= LEVEL_IMMORTAL ) && fMortals )
                  found = TRUE;
            }  /* for everyone inside the room */

            if( found && !room_is_private( room ) )   /* Any of the required type here AND room not private? */
            {
               /*
                * This may be ineffective. Consider moving character out of old_room
                * once at beginning of command then moving back at the end.
                * This however, is more safe?
                */

               old_room = ch->in_room;
               char_from_room( ch );
               if( !char_to_room( ch, room ) )
                  log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
               interpret( ch, argument );
               char_from_room( ch );
               if( !char_to_room( ch, old_room ) )
                  log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            }  /* if found */
         }  /* for every room in a bucket */
   }  /* if strchr */
}  /* do_for */

/* Modified to treat Hell's vnum as a variable, not a constant "6" - Samson */
CMDF do_hell( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   char arg[MIL];
   short htime;
   bool h_d = FALSE;
   struct tm *tms;

   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Hell who, and for how long?\n\r", ch );
      return;
   }
   if( !( victim = get_char_world( ch, arg ) ) || IS_NPC( victim ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_IMMORTAL( victim ) )
   {
      send_to_char( "There is no point in helling an immortal.\n\r", ch );
      return;
   }
   if( victim->pcdata->release_date != 0 )
   {
      ch_printf( ch, "They are already in hell until %24.24s, by %s.\n\r",
                 c_time( victim->pcdata->release_date, ch->pcdata->timezone ), victim->pcdata->helled_by );
      return;
   }
   argument = one_argument( argument, arg );
   if( !arg || arg[0] == '\0' || !is_number( arg ) )
   {
      send_to_char( "Hell them for how long?\n\r", ch );
      return;
   }

   htime = atoi( arg );
   if( htime <= 0 )
   {
      send_to_char( "You cannot hell for zero or negative time.\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( !arg || arg[0] == '\0' || !str_cmp( arg, "hours" ) )
      h_d = TRUE;
   else if( str_cmp( arg, "days" ) )
   {
      send_to_char( "Is that value in hours or days?\n\r", ch );
      return;
   }
   else if( htime > 30 )
   {
      send_to_char( "You may not hell a person for more than 30 days at a time.\n\r", ch );
      return;
   }
   tms = localtime( &current_time );

   if( h_d )
      tms->tm_hour += htime;
   else
      tms->tm_mday += htime;
   victim->pcdata->release_date = mktime( tms );
   victim->pcdata->helled_by = STRALLOC( ch->name );
   ch_printf( ch, "%s will be released from hell at %24.24s.\n\r", victim->name,
              c_time( victim->pcdata->release_date, ch->pcdata->timezone ) );
   act( AT_MAGIC, "$n disappears in a cloud of hellish light.", victim, NULL, ch, TO_NOTVICT );
   char_from_room( victim );
   if( !char_to_room( victim, get_room_index( ROOM_VNUM_HELL ) ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   act( AT_MAGIC, "$n appears in a could of hellish light.", victim, NULL, ch, TO_NOTVICT );
   interpret( victim, "look" );
   ch_printf( victim, "The immortals are not pleased with your actions.\n\r"
              "You shall remain in hell for %d %s%s.\n\r", htime, ( h_d ? "hour" : "day" ), ( htime == 1 ? "" : "s" ) );
   save_char_obj( victim );   /* used to save ch, fixed by Thoric 09/17/96 */
   return;
}

/* Modified to treat Hell's vnum as a variable, not a constant "6" - Samson */
CMDF do_unhell( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   ROOM_INDEX_DATA *location;

   set_char_color( AT_IMMORT, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Unhell whom..?\n\r", ch );
      return;
   }
   location = ch->in_room;
   victim = get_char_world( ch, argument );
   if( !victim || IS_NPC( victim ) )
   {
      send_to_char( "No such player character present.\n\r", ch );
      return;
   }
   if( victim->in_room->vnum != ROOM_VNUM_HELL )
   {
      send_to_char( "No one like that is in hell.\n\r", ch );
      return;
   }

   if( victim->pcdata->clan )
      location = get_room_index( victim->pcdata->clan->recall );
   else
      location = get_room_index( ROOM_VNUM_TEMPLE );
   if( !location )
      location = get_room_index( ROOM_VNUM_LIMBO );
   MOBtrigger = FALSE;
   act( AT_MAGIC, "$n disappears in a cloud of godly light.", victim, NULL, ch, TO_NOTVICT );
   char_from_room( victim );
   if( !char_to_room( victim, location ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   send_to_char( "The gods have smiled on you and released you from hell early!\n\r", victim );
   interpret( victim, "look" );
   if( victim != ch )
      send_to_char( "They have been released.\n\r", ch );
   if( victim->pcdata->helled_by )
   {
      if( str_cmp( ch->name, victim->pcdata->helled_by ) )
         ch_printf( ch, "(You should probably write a note to %s, explaining the early release.)\n\r",
                    victim->pcdata->helled_by );
      STRFREE( victim->pcdata->helled_by );
   }
   MOBtrigger = FALSE;
   act( AT_MAGIC, "$n appears in a cloud of godly light.", victim, NULL, ch, TO_NOTVICT );
   victim->pcdata->release_date = 0;
   save_char_obj( victim );
   return;
}

/* Vnum search command by Swordbearer */
CMDF do_vsearch( CHAR_DATA * ch, char *argument )
{
   bool found = FALSE;
   OBJ_DATA *obj, *in_obj;
   int obj_counter = 1, argi;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax:  vsearch <vnum>.\n\r", ch );
      return;
   }

   argi = atoi( argument );
   if( argi < 1 || argi > sysdata.maxvnum )
   {
      send_to_char( "Vnum out of range.\n\r", ch );
      return;
   }
   for( obj = first_object; obj != NULL; obj = obj->next )
   {
      if( !can_see_obj( ch, obj, TRUE ) || !( argi == obj->pIndexData->vnum ) )
         continue;

      found = TRUE;
      for( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj );

      if( in_obj->carried_by != NULL )
         pager_printf( ch, "&Y[&W%2d&Y] &GLevel %d %s carried by %s.\n\r",
                       obj_counter, obj->level, obj_short( obj ), PERS( in_obj->carried_by, ch, TRUE ) );
      else
         pager_printf( ch, "&Y[&W%2d&Y] [&W%-5d&Y] &G%s in %s.\n\r", obj_counter,
                       ( ( in_obj->in_room ) ? in_obj->in_room->vnum : 0 ),
                       obj_short( obj ), ( in_obj->in_room == NULL ) ? "somewhere" : in_obj->in_room->name );

      obj_counter++;
   }
   if( !found )
      send_to_char( "Nothing like that in hell, earth, or heaven.\n\r", ch );
   return;
}

/* 
 * Simple function to let any imm make any player instantly sober.
 * Saw no need for level restrictions on this.
 * Written by Narn, Apr/96 
 */
CMDF do_sober( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   set_char_color( AT_IMMORT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Sober up who?\n\r", ch );
      return;
   }

   smash_tilde( argument );
   if( ( victim = get_char_world( ch, argument ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on mobs.\n\r", ch );
      return;
   }

   victim->pcdata->condition[COND_DRUNK] = 0;
   send_to_char( "Ok.\n\r", ch );
   send_to_char( "You feel sober again.\n\r", victim );
   return;
}

/*
 * Free a social structure					-Thoric
 */
void free_social( SOCIALTYPE * social )
{
   DISPOSE( social->name );
   DISPOSE( social->char_no_arg );
   DISPOSE( social->others_no_arg );
   DISPOSE( social->char_found );
   DISPOSE( social->others_found );
   DISPOSE( social->vict_found );
   DISPOSE( social->char_auto );
   DISPOSE( social->others_auto );
   DISPOSE( social->obj_self );
   DISPOSE( social->obj_others );
   DISPOSE( social );
}

void free_socials( void )
{
   SOCIALTYPE *social, *social_next;
   int hash;

   for( hash = 0; hash < 27; hash++ )
   {
      for( social = social_index[hash]; social; social = social_next )
      {
         social_next = social->next;
         free_social( social );
      }
   }
   return;
}

/*
 * Remove a social from it's hash index				-Thoric
 */
void unlink_social( SOCIALTYPE * social )
{
   SOCIALTYPE *tmp, *tmp_next;
   int hash;

   if( !social )
   {
      bug( "%s", "Unlink_social: NULL social" );
      return;
   }

   if( social->name[0] < 'a' || social->name[0] > 'z' )
      hash = 0;
   else
      hash = ( social->name[0] - 'a' ) + 1;

   if( social == ( tmp = social_index[hash] ) )
   {
      social_index[hash] = tmp->next;
      return;
   }
   for( ; tmp; tmp = tmp_next )
   {
      tmp_next = tmp->next;
      if( social == tmp_next )
      {
         tmp->next = tmp_next->next;
         return;
      }
   }
}

/*
 * Add a social to the social index table			-Thoric
 * Hashed and insert sorted
 */
void add_social( SOCIALTYPE * social )
{
   int hash, x;
   SOCIALTYPE *tmp, *prev;

   if( !social )
   {
      bug( "%s", "Add_social: NULL social" );
      return;
   }

   if( !social->name )
   {
      bug( "%s", "Add_social: NULL social->name" );
      return;
   }

   if( !social->char_no_arg )
   {
      bug( "Add_social: NULL social->char_no_arg on social %s", social->name );
      return;
   }

   /*
    * make sure the name is all lowercase 
    */
   for( x = 0; social->name[x] != '\0'; x++ )
      social->name[x] = LOWER( social->name[x] );

   if( social->name[0] < 'a' || social->name[0] > 'z' )
      hash = 0;
   else
      hash = ( social->name[0] - 'a' ) + 1;

   if( ( prev = tmp = social_index[hash] ) == NULL )
   {
      social->next = social_index[hash];
      social_index[hash] = social;
      return;
   }

   for( ; tmp; tmp = tmp->next )
   {
      if( !str_cmp( social->name, tmp->name ) )
      {
         bug( "Add_social: trying to add duplicate name to bucket %d", hash );
         free_social( social );
         return;
      }
      else if( x < 0 )
      {
         if( tmp == social_index[hash] )
         {
            social->next = social_index[hash];
            social_index[hash] = social;
            return;
         }
         prev->next = social;
         social->next = tmp;
         return;
      }
      prev = tmp;
   }

   /*
    * add to end 
    */
   prev->next = social;
   social->next = NULL;
   return;
}

/*
 * Social editor/displayer/save/delete				-Thoric
 */
/* Modified to have level checks for actual levels, not trust - Samson */
CMDF do_sedit( CHAR_DATA * ch, char *argument )
{
   SOCIALTYPE *social;
   char arg1[MIL], arg2[MIL];

   set_char_color( AT_SOCIAL, ch );

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Syntax: sedit <social> [field] [data]\n\r", ch );
      send_to_char( "Syntax: sedit <social> create\n\r", ch );
      if( ch->level > LEVEL_GOD )
         send_to_char( "Syntax: sedit <social> delete\n\r", ch );
      send_to_char( "Syntax: sedit <save>\n\r", ch );
      if( ch->level > LEVEL_GREATER )
         send_to_char( "Syntax: sedit <social> name <newname>\n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( "  cnoarg onoarg cfound ofound vfound cauto oauto objself objothers\n\r", ch );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      save_socials(  );
      send_to_char( "Social file updated.\n\r", ch );
      return;
   }

   social = find_social( arg1 );
   if( !str_cmp( arg2, "create" ) )
   {
      if( social )
      {
         send_to_char( "That social already exists!\n\r", ch );
         return;
      }
      CREATE( social, SOCIALTYPE, 1 );
      social->name = str_dup( arg1 );
      strdup_printf( &social->char_no_arg, "You %s.", arg1 );
      add_social( social );
      send_to_char( "Social added.\n\r", ch );
      return;
   }

   if( !social )
   {
      send_to_char( "Social not found.\n\r", ch );
      return;
   }

   if( !arg2 || arg2[0] == '\0' || !str_cmp( arg2, "show" ) )
   {
      ch_printf( ch, "Social   : %s\n\r\n\rCNoArg   : %s\n\r", social->name, social->char_no_arg );
      ch_printf( ch, "ONoArg   : %s\n\rCFound   : %s\n\rOFound   : %s\n\r",
                 social->others_no_arg ? social->others_no_arg : "(not set)",
                 social->char_found ? social->char_found : "(not set)",
                 social->others_found ? social->others_found : "(not set)" );
      ch_printf( ch, "VFound   : %s\n\rCAuto    : %s\n\rOAuto    : %s\n\r",
                 social->vict_found ? social->vict_found : "(not set)",
                 social->char_auto ? social->char_auto : "(not set)",
                 social->others_auto ? social->others_auto : "(not set)" );
      ch_printf( ch, "ObjSelf  : %s\n\rObjOthers: %s\n\r",
                 social->obj_self ? social->obj_self : "(not set)", social->obj_others ? social->obj_others : "(not set)" );
      return;
   }
   if( ch->level > LEVEL_GOD && !str_cmp( arg2, "delete" ) )
   {
      unlink_social( social );
      free_social( social );
      send_to_char( "Deleted.\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "cnoarg" ) )
   {
      if( !argument || argument[0] == '\0' || !str_cmp( argument, "clear" ) )
      {
         send_to_char( "You cannot clear this field. It must have a message.\n\r", ch );
         return;
      }
      DISPOSE( social->char_no_arg );
      social->char_no_arg = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "onoarg" ) )
   {
      DISPOSE( social->others_no_arg );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->others_no_arg = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "cfound" ) )
   {
      DISPOSE( social->char_found );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->char_found = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "ofound" ) )
   {
      DISPOSE( social->others_found );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->others_found = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "vfound" ) )
   {
      DISPOSE( social->vict_found );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->vict_found = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "cauto" ) )
   {
      DISPOSE( social->char_auto );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->char_auto = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "oauto" ) )
   {
      DISPOSE( social->others_auto );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->others_auto = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "objself" ) )
   {
      DISPOSE( social->obj_self );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->obj_self = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "objothers" ) )
   {
      DISPOSE( social->obj_others );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->obj_others = str_dup( argument );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   if( get_trust( ch ) > LEVEL_GREATER && !str_cmp( arg2, "name" ) )
   {
      bool relocate;
      SOCIALTYPE *checksocial;

      one_argument( argument, arg1 );
      if( arg1[0] == '\0' )
      {
         send_to_char( "Cannot clear name field!\n\r", ch );
         return;
      }
      if( ( checksocial = find_social( arg1 ) ) != NULL )
      {
         ch_printf( ch, "There is already a social named %s.\n\r", arg1 );
         return;
      }
      if( arg1[0] != social->name[0] )
      {
         unlink_social( social );
         relocate = TRUE;
      }
      else
         relocate = FALSE;
      if( social->name )
         DISPOSE( social->name );
      social->name = str_dup( arg1 );
      if( relocate )
         add_social( social );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   /*
    * display usage message 
    */
   do_sedit( ch, "" );
}

void update_calendar( void )
{
   sysdata.daysperyear = sysdata.dayspermonth * sysdata.monthsperyear;
   sysdata.hoursunrise = sysdata.hoursperday / 4;
   sysdata.hourdaybegin = sysdata.hoursunrise + 1;
   sysdata.hournoon = sysdata.hoursperday / 2;
   sysdata.hoursunset = ( ( sysdata.hoursperday / 4 ) * 3 );
   sysdata.hournightbegin = sysdata.hoursunset + 1;
   sysdata.hourmidnight = sysdata.hoursperday;
   calc_season(  );
   return;
}

void update_timers( void )
{
   sysdata.pulsetick = sysdata.secpertick * sysdata.pulsepersec;
   sysdata.pulseviolence = 3 * sysdata.pulsepersec;
   sysdata.pulsespell = sysdata.pulseviolence;
   sysdata.pulsemobile = 4 * sysdata.pulsepersec;
   sysdata.pulsecalendar = 4 * sysdata.pulsetick;
   sysdata.pulseenvironment = 15 * sysdata.pulsepersec;
   sysdata.pulseskyship = sysdata.pulsemobile;
   return;
}

/* Redone cset command, with more in-game changable parameters - Samson 2-19-02 */
CMDF do_cset( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   int value = -1;

   if( !argument || argument[0] == '\0' )
   {
      send_to_pager( "&WThe following options may be set:\n\r\n\r", ch );
      send_to_pager( "&GSite Parameters\n\r", ch );
      send_to_pager( "---------------\n\r", ch );
      pager_printf( ch, "&BMudname&c: %s &BEmail&c: %s &BPassword&c: [not shown]\n\r",
                    sysdata.mud_name, sysdata.admin_email );
      pager_printf( ch, "&BHTTP&c: %s &BTelnet&c: %s\n\r\n\r", show_tilde( sysdata.http ), sysdata.telnet );

      send_to_pager( "&GGame Toggles\n\r", ch );
      send_to_pager( "------------\n\r", ch );
      pager_printf( ch, "&BRent&c: %s &BNameauth&c: %s &BImmhost Checking&c: %s &BTestmode&c: %s\n\r",
                    sysdata.RENT ? "Enabled" : "Disabled", sysdata.WAIT_FOR_AUTH ? "Enabled" : "Disabled",
                    sysdata.check_imm_host ? "Enabled" : "Disabled", sysdata.TESTINGMODE ? "On" : "Off" );
      if( sysdata.RENT )
         pager_printf( ch, "   &BMinimum Rent&c: %d\n\r", sysdata.minrent );
      pager_printf( ch, "&BPet Save&c: %s &BCrashhandler&c: %s\n\r",
                    sysdata.save_pets ? "On" : "Off", sysdata.crashhandler ? "Enabled" : "Disabled" );
      pager_printf( ch, "&BPfile Pruning&c: %s\n\r", sysdata.CLEANPFILES ? "Enabled" : "Disabled" );
      if( sysdata.CLEANPFILES )
      {
         pager_printf( ch, "   &BNewbie Purge &c: %d days\n\r", sysdata.newbie_purge );
         pager_printf( ch, "   &BRegular Purge&c: %d days\n\r", sysdata.regular_purge );
      }
#ifdef WEBSVR
      pager_printf( ch, "&BWebserver&c: %s\n\r", sysdata.webrunning ? "On" : "Off" );
      pager_printf( ch, "   &BLog hits&c: %s &BBoot with mud&c: %s\n\r",
                    sysdata.webcounter ? "Yes" : "No", sysdata.webtoggle ? "Yes" : "No" );
#endif

      send_to_pager
         ( "\n\r&GGame Settings - Note: Altering these can have drastic affects on the game. Use with caution.\n\r", ch );
      send_to_pager( "-------------\n\r", ch );
      pager_printf( ch, "&BMaxVnum&c: %d &BOverland Radius&c: %d &BReboot Count&c: %d &BAuction Seconds&c: %d\n\r",
                    sysdata.maxvnum, sysdata.mapsize, sysdata.rebootcount, sysdata.auctionseconds );
      pager_printf( ch,
                    "&BMin Guild Level&c: %d &BMax Condition Value&c: %d &BMax Ignores&c: %d &BMax Item Impact&c: %d &BInit Weapon Condition&c: %d\n\r",
                    sysdata.minguildlevel, sysdata.maxcondval, sysdata.maxign, sysdata.maximpact, sysdata.initcond );
      pager_printf( ch,
                    "&BForce Players&c: %d &BPrivate Override&c: %d &BGet Notake&c: %d &BAutosave Freq&c: %d &BMax Holidays&c: %d\n\r",
                    sysdata.level_forcepc, sysdata.level_override_private, sysdata.level_getobjnotake,
                    sysdata.save_frequency, sysdata.maxholiday );
      pager_printf( ch, "&BProto Mod&c: %d &B &BMset Player&c: %d &BBestow Diff&c: %d &BBuild Level&c: %d\n\r",
                    sysdata.level_modify_proto, sysdata.level_mset_player, sysdata.bestow_dif, sysdata.build_level );
      pager_printf( ch, "&BRead all mail&c: %d &BTake all mail&c: %d &BRead mail free&c: %d &BWrite mail free&c: %d\n\r",
                    sysdata.read_all_mail, sysdata.take_others_mail, sysdata.read_mail_free, sysdata.write_mail_free );
      pager_printf( ch,
                    "&BHours per day&c: %d &BDays per week&c: %d &BDays per month&c: %d &BMonths per year&c: %d &RDays per year&W: %d\n\r",
                    sysdata.hoursperday, sysdata.daysperweek, sysdata.dayspermonth, sysdata.monthsperyear,
                    sysdata.daysperyear );
      pager_printf( ch, "&BSave Flags&c: %s\n\r", flag_string( sysdata.save_flags, save_flag ) );
      pager_printf( ch, "\n\r&BSeconds per tick&c: %d   &BPulse per second&c: %d\n\r",
                    sysdata.secpertick, sysdata.pulsepersec );
      pager_printf( ch, "   &RPULSE_TICK&W: %d &RPULSE_VIOLENCE&W: %d &RPULSE_SPELL&W: %d &RPULSE_MOBILE&W: %d\n\r",
                    sysdata.pulsetick, sysdata.pulseviolence, sysdata.pulseviolence, sysdata.pulsemobile );
      pager_printf( ch, "   &RPULSE_CALENDAR&W: %d &RPULSE_ENVIRONMENT&W: %d &RPULSE_SKYSHIP&W: %d\n\r",
                    sysdata.pulsecalendar, sysdata.pulseenvironment, sysdata.pulseskyship );
      return;
   }

   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "help" ) )
   {
      interpret( ch, "help cset" );
      return;
   }

   set_char_color( AT_IMMORT, ch );

   if( !str_cmp( arg, "mudname" ) )
   {
      DISPOSE( sysdata.mud_name );
      if( !argument || argument[0] == '\0' )
      {
         sysdata.mud_name = str_dup( "(Name not set)" );
         send_to_char( "Mud name cleared.\n\r", ch );
      }
      else
      {
         smash_tilde( argument );
         sysdata.mud_name = str_dup( argument );
         ch_printf( ch, "Mud name set to: %s\n\r", argument );
      }
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "password" ) )
   {
      char *pwdnew;

      if( strlen( argument ) < 5 )
      {
         send_to_char( "New password must be at least five characters long.\n\r", ch );
         return;
      }

      if( argument[0] == '!' )
      {
         send_to_char( "New password cannot begin with the '!' character.\n\r", ch );
         return;
      }

      pwdnew = sha256_crypt( argument );  /* SHA-256 Encryption */
      DISPOSE( sysdata.password );
      sysdata.password = str_dup( pwdnew );
      send_to_char( "Mud password changed.\n\r", ch );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "email" ) )
   {
      DISPOSE( sysdata.admin_email );
      if( !argument || argument[0] == '\0' )
      {
         sysdata.admin_email = str_dup( "Not Set" );
         send_to_char( "Email address cleared.\n\r", ch );
      }
      else
      {
         smash_tilde( argument );
         sysdata.admin_email = str_dup( argument );
         ch_printf( ch, "Email address set to %s\n\r", argument );
      }
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "http" ) )
   {
      DISPOSE( sysdata.http );
      if( !argument || argument[0] == '\0' )
      {
         sysdata.http = str_dup( "No page set" );
         send_to_char( "HTTP address cleared.\n\r", ch );
      }
      else
      {
         hide_tilde( argument );
         sysdata.http = str_dup( argument );
         ch_printf( ch, "HTTP address set to %s\n\r", show_tilde( sysdata.http ) );
      }
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "telnet" ) )
   {
      DISPOSE( sysdata.telnet );
      if( !argument || argument[0] == '\0' )
      {
         sysdata.telnet = str_dup( "Not Set" );
         send_to_char( "Telnet address cleared.\n\r", ch );
      }
      else
      {
         smash_tilde( argument );
         sysdata.telnet = str_dup( argument );
         ch_printf( ch, "Telnet address set to %s\n\r", argument );
      }
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "rent" ) )
   {
      sysdata.RENT = !sysdata.RENT;

      if( sysdata.RENT )
         send_to_char( "Rent system enabled.\n\r", ch );
      else
         send_to_char( "Rent system disabled.\n\r", ch );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "nameauth" ) )
   {
      sysdata.WAIT_FOR_AUTH = !sysdata.WAIT_FOR_AUTH;

      if( sysdata.WAIT_FOR_AUTH )
         send_to_char( "Name Authorization system enabled.\n\r", ch );
      else
         send_to_char( "Name Authorization system disabled.\n\r", ch );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "immhost-checking" ) )
   {
      sysdata.check_imm_host = !sysdata.check_imm_host;

      if( sysdata.check_imm_host )
         send_to_char( "Immhost Checking enabled.\n\r", ch );
      else
         send_to_char( "Immhost Checking disabled.\n\r", ch );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "testmode" ) )
   {
      sysdata.TESTINGMODE = !sysdata.TESTINGMODE;

      if( sysdata.TESTINGMODE )
         send_to_char( "Server Testmode enabled.\n\r", ch );
      else
         send_to_char( "Server Testmode disabled.\n\r", ch );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "pfile-pruning" ) )
   {
      sysdata.CLEANPFILES = !sysdata.CLEANPFILES;

      if( sysdata.CLEANPFILES )
         send_to_char( "Pfile Pruning enabled.\n\r", ch );
      else
         send_to_char( "Pfile Pruning disabled.\n\r", ch );
      save_sysdata( sysdata );
      return;
   }

#ifdef WEBSVR
   if( !str_cmp( arg, "webserver" ) )
   {
      if( !sysdata.webrunning )
         interpret( ch, "web start" );
      else
         interpret( ch, "web stop" );
      return;
   }

   if( !str_cmp( arg, "log-hits" ) )
   {
      sysdata.webcounter = !sysdata.webcounter;

      if( sysdata.webcounter )
         send_to_char( "Webserver hits will now be logged.\n\r", ch );
      else
         send_to_char( "Webserver hits will no longer be logged.\n\r", ch );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "boot-with-mud" ) )
   {
      sysdata.webtoggle = !sysdata.webtoggle;

      if( sysdata.webtoggle )
         send_to_char( "Webserver will now boot at mud startup.\n\r", ch );
      else
         send_to_char( "Webserver will no longer boot at mud startup.\n\r", ch );
      save_sysdata( sysdata );
      return;
   }
#endif

   if( !str_cmp( arg, "pet-save" ) )
   {
      sysdata.save_pets = !sysdata.save_pets;

      if( sysdata.save_pets )
         send_to_char( "Pet saving enabled.\n\r", ch );
      else
         send_to_char( "Pet saving disabled.\n\r", ch );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "crashhandler" ) )
   {
      sysdata.crashhandler = !sysdata.crashhandler;

      if( sysdata.crashhandler )
         send_to_char( "Crash handling will be enabled at next reboot.\n\r", ch );
      else
         send_to_char( "Crash handling will be disabled at next reboot.\n\r", ch );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "save-flag" ) )
   {
      int x = get_saveflag( argument );

      if( x == -1 )
      {
         send_to_char( "Not a save flag.\n\r", ch );
         return;
      }
      TOGGLE_BIT( sysdata.save_flags, 1 << x );
      ch_printf( ch, "%s flag toggled.\n\r", argument );
      save_sysdata( sysdata );
      return;
   }

   /*
    * Everything below this point requires an argument, kick them if it's not there. 
    */
   if( !argument || argument[0] == '\0' )
   {
      do_cset( ch, "help" );
      return;
   }

   /*
    * Everything below here requires numerical arguments, kick them again. 
    */
   if( !is_number( argument ) )
   {
      send_to_char( "&RError: Argument must be an integer value.\n\r", ch );
      return;
   }

   value = atoi( argument );

   /*
    * Typical size range of integers 
    */
   if( value < 0 || value > 2000000000 )
   {
      send_to_char( "&RError: Invalid integer value. Range is 0 to 2000000000.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "reboot-count" ) )
   {
      sysdata.rebootcount = value;
      ch_printf( ch, "Reboot time counter set to %d minutes.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "auction-seconds" ) )
   {
      sysdata.auctionseconds = value;
      ch_printf( ch, "Auction timer set to %d seconds.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "newbie-purge" ) )
   {
      if( value > 32767 )
      {
         send_to_char( "&RError: Cannot set Newbie Purge above 32767.\n\r", ch );
         return;
      }
      sysdata.newbie_purge = value;
      ch_printf( ch, "Newbie Purge set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "regular-purge" ) )
   {
      if( value > 32767 )
      {
         send_to_char( "&RError: Cannot set Regular Purge above 32767.\n\r", ch );
         return;
      }
      sysdata.regular_purge = value;
      ch_printf( ch, "Regular Purge set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "minimum-rent" ) )
   {
      sysdata.minrent = value;
      ch_printf( ch, "Minimum Rent set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "maxvnum" ) )
   {
      AREA_DATA *area;
      char buf[MSL];
      int vnum = 0;

      for( area = first_area; area; area = area->next )
      {
         if( area->hi_vnum > vnum )
         {
            mudstrlcpy( buf, area->name, MSL );
            vnum = area->hi_vnum;
         }
      }

      if( value <= vnum )
      {
         ch_printf( ch, "&RError: Cannot set MaxVnum to %d, existing areas extend to %d.\n\r", value, vnum );
         return;
      }

      if( value - vnum < 1000 )
      {
         ch_printf( ch, "Warning: Setting MaxVnum to %d leaves you with less than 1000 vnums beyond the highest area.\n\r",
                    value );
         ch_printf( ch, "Highest area %s ends with vnum %d.\n\r", buf, vnum );
      }

      sysdata.maxvnum = value;
      ch_printf( ch, "MaxVnum changed to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "overland-radius" ) )
   {
      if( value > 14 )
      {
         send_to_char( "&RError: Cannot set Overland Radius larger than 14 due to screen size restrictions.\n\r", ch );
         return;
      }
      sysdata.mapsize = value;
      ch_printf( ch, "Overland Radius set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "min-guild-level" ) )
   {
      if( value > LEVEL_AVATAR )
      {
         ch_printf( ch, "&RError: Cannot set Min Guild Level above level %d.\n\r", LEVEL_AVATAR );
         return;
      }
      sysdata.minguildlevel = value;
      ch_printf( ch, "Min Guild Level set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "max-condition-value" ) )
   {
      sysdata.maxcondval = value;
      ch_printf( ch, "Max Condition Value set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "max-ignores" ) )
   {
      sysdata.maxign = value;
      ch_printf( ch, "Max Ignores set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "max-item-impact" ) )
   {
      sysdata.maximpact = value;
      ch_printf( ch, "Max Item Impact set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "init-weapon-condition" ) )
   {
      sysdata.initcond = value;
      ch_printf( ch, "Init Weapon Condition set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "force-players" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch_printf( ch, "&RError: Cannot set Force Players above level %d, or below level %d.\n\r", MAX_LEVEL,
                    LEVEL_IMMORTAL );
         return;
      }
      sysdata.level_forcepc = value;
      ch_printf( ch, "Force Players set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "private-override" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch_printf( ch, "&RError: Cannot set Private Override above level %d, or below level %d.\n\r", MAX_LEVEL,
                    LEVEL_IMMORTAL );
         return;
      }
      sysdata.level_override_private = value;
      ch_printf( ch, "Private Override set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "get-notake" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch_printf( ch, "&RError: Cannot set Get Notake above level %d, or below level %d.\n\r", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata.level_getobjnotake = value;
      ch_printf( ch, "Get Notake set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "autosave-freq" ) )
   {
      if( value > 32767 )
      {
         send_to_char( "&RError: Cannot set Autosave Freq above 32767.\n\r", ch );
         return;
      }
      sysdata.save_frequency = value;
      ch_printf( ch, "Autosave Freq set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "max-holidays" ) )
   {
      sysdata.maxholiday = value;
      ch_printf( ch, "Max Holiday set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "proto-mod" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch_printf( ch, "&RError: Cannot set Proto Mod above level %d, or below level %d.\n\r", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata.level_modify_proto = value;
      ch_printf( ch, "Proto Mod set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "mset-player" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch_printf( ch, "&RError: Cannot set Mset Player above level %d, or below level %d.\n\r", MAX_LEVEL,
                    LEVEL_IMMORTAL );
         return;
      }
      sysdata.level_mset_player = value;
      ch_printf( ch, "Mset Player set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "bestow-diff" ) )
   {
      if( value > MAX_LEVEL )
      {
         ch_printf( ch, "&RError: Cannot set Bestow Diff above %d.\n\r", MAX_LEVEL );
         return;
      }
      sysdata.bestow_dif = value;
      ch_printf( ch, "Bestow Diff set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "build-level" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch_printf( ch, "&RError: Cannot set Build Level above level %d, or below level %d.\n\r", MAX_LEVEL,
                    LEVEL_IMMORTAL );
         return;
      }
      sysdata.build_level = value;
      ch_printf( ch, "Build Level set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "read-all-mail" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch_printf( ch, "&RError: Cannot set Read all mail above level %d, or below level %d.\n\r", MAX_LEVEL,
                    LEVEL_IMMORTAL );
         return;
      }
      sysdata.read_all_mail = value;
      ch_printf( ch, "Read all mail set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "take-all-mail" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch_printf( ch, "&RError: Cannot set Take all mail above level %d, or below level %d.\n\r", MAX_LEVEL,
                    LEVEL_IMMORTAL );
         return;
      }
      sysdata.take_others_mail = value;
      ch_printf( ch, "Take all mail set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "read-mail-free" ) )
   {
      if( value > MAX_LEVEL )
      {
         ch_printf( ch, "&RError: Cannot set Read mail free above level %d.\n\r", MAX_LEVEL );
         return;
      }
      sysdata.read_mail_free = value;
      ch_printf( ch, "Read mail free set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "write-mail-free" ) )
   {
      if( value > MAX_LEVEL )
      {
         ch_printf( ch, "&RError: Cannot set Write mail free above level %d.\n\r", MAX_LEVEL );
         return;
      }
      sysdata.write_mail_free = value;
      ch_printf( ch, "Write mail free set to %d.\n\r", value );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "hours-per-day" ) )
   {
      sysdata.hoursperday = value;
      ch_printf( ch, "Hours per day set to %d.\n\r", value );
      update_calendar(  );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "days-per-week" ) )
   {
      sysdata.daysperweek = value;
      ch_printf( ch, "Days per week set to %d.\n\r", value );
      update_calendar(  );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "days-per-month" ) )
   {
      sysdata.dayspermonth = value;
      ch_printf( ch, "Days per month set to %d.\n\r", value );
      update_calendar(  );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "months-per-year" ) )
   {
      sysdata.monthsperyear = value;
      ch_printf( ch, "Months per year set to %d.\n\r", value );
      update_calendar(  );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "seconds-per-tick" ) )
   {
      sysdata.secpertick = value;
      ch_printf( ch, "Seconds per tick set to %d.\n\r", value );
      update_timers(  );
      save_sysdata( sysdata );
      return;
   }

   if( !str_cmp( arg, "pulse-per-second" ) )
   {
      sysdata.pulsepersec = value;
      ch_printf( ch, "Pulse per second set to %d.\n\r", value );
      update_timers(  );
      save_sysdata( sysdata );
      return;
   }

   do_cset( ch, "help" );
   return;
}

/*
 * Free a command structure					-Thoric
 */
void free_command( CMDTYPE * command )
{
   DISPOSE( command->name );
   DISPOSE( command->fun_name );
   DISPOSE( command );
}

void free_commands( void )
{
   CMDTYPE *command, *cmd_next;
   int hash;

   for( hash = 0; hash < 126; hash++ )
   {
      for( command = command_hash[hash]; command; command = cmd_next )
      {
         cmd_next = command->next;
         command->next = NULL;
         command->do_fun = NULL;
         free_command( command );
      }
   }
   return;
}

/*
 * Remove a command from it's hash index			-Thoric
 */
void unlink_command( CMDTYPE * command )
{
   CMDTYPE *tmp, *tmp_next;
   int hash;

   if( !command )
   {
      bug( "%s", "Unlink_command NULL command" );
      return;
   }

   hash = command->name[0] % 126;

   if( command == ( tmp = command_hash[hash] ) )
   {
      command_hash[hash] = tmp->next;
      return;
   }
   for( ; tmp; tmp = tmp_next )
   {
      tmp_next = tmp->next;
      if( command == tmp_next )
      {
         tmp->next = tmp_next->next;
         return;
      }
   }
}

/*
 * Add a command to the command hash table			-Thoric
 */
void add_command( CMDTYPE * command )
{
   int hash, x;
   CMDTYPE *tmp, *prev;

   if( !command )
   {
      bug( "%s", "Add_command: NULL command" );
      return;
   }

   if( !command->name )
   {
      bug( "%s", "Add_command: NULL command->name" );
      return;
   }

   if( !command->do_fun )
   {
      bug( "Add_command: NULL command->do_fun for command %s", command->name );
      return;
   }

   /*
    * make sure the name is all lowercase 
    */
   for( x = 0; command->name[x] != '\0'; x++ )
      command->name[x] = LOWER( command->name[x] );

   hash = command->name[0] % 126;

   if( ( prev = tmp = command_hash[hash] ) == NULL )
   {
      command->next = command_hash[hash];
      command_hash[hash] = command;
      return;
   }

   /*
    * add to the END of the list 
    */
   for( ; tmp; tmp = tmp->next )
      if( !tmp->next )
      {
         tmp->next = command;
         command->next = NULL;
      }
   return;
}

/*
 * Command editor/displayer/save/delete				-Thoric
 * Added support for interpret flags                            -Shaddai
 */
/* Function modified to support new command lookup table - Samson 4-2-98 */
CMDF do_cedit( CHAR_DATA * ch, char *argument )
{
   CMDTYPE *command;
   char arg1[MIL], arg2[MIL];

   set_char_color( AT_IMMORT, ch );

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Syntax: cedit save cmdtable\n\r", ch );
      if( get_trust( ch ) > LEVEL_SUB_IMPLEM )
      {
         send_to_char( "Syntax: cedit <command> create [code]\n\r", ch );
         send_to_char( "Syntax: cedit <command> delete\n\r", ch );
         send_to_char( "Syntax: cedit <command> show\n\r", ch );
         send_to_char( "Syntax: cedit <command> raise\n\r", ch );
         send_to_char( "Syntax: cedit <command> lower\n\r", ch );
         send_to_char( "Syntax: cedit <command> list\n\r", ch );
         send_to_char( "Syntax: cedit <command> [field]\n\r", ch );
         send_to_char( "\n\rField being one of:\n\r", ch );
         send_to_char( "  level position log code flags\n\r", ch );
      }
      return;
   }

   if( ch->level > LEVEL_GREATER && !str_cmp( arg1, "save" ) && !str_cmp( arg2, "cmdtable" ) )
   {
      save_commands(  );
      send_to_char( "Saved.\n\r", ch );
      return;
   }

   command = find_command( arg1 );
   if( get_trust( ch ) > LEVEL_SUB_IMPLEM && !str_cmp( arg2, "create" ) )
   {
      if( command )
      {
         send_to_char( "That command already exists!\n\r", ch );
         return;
      }
      CREATE( command, CMDTYPE, 1 );
      command->name = str_dup( arg1 );
      command->level = get_trust( ch );
      if( *argument )
         one_argument( argument, arg2 );
      else
         snprintf( arg2, MIL, "do_%s", arg1 );
      command->do_fun = skill_function( arg2 );
      command->fun_name = str_dup( arg2 );
      add_command( command );
      send_to_char( "Command added.\n\r", ch );
      if( command->do_fun == skill_notfound )
         ch_printf( ch, "Code %s not found.  Set to no code.\n\r", arg2 );
      return;
   }

   if( !command )
   {
      send_to_char( "Command not found.\n\r", ch );
      return;
   }
   else if( command->level > get_trust( ch ) )
   {
      send_to_char( "You cannot touch this command.\n\r", ch );
      return;
   }

   if( arg2[0] == '\0' || !str_cmp( arg2, "show" ) )
   {
      ch_printf( ch, "Command:  %s\n\rLevel:    %d\n\rPosition: %s\n\rLog:      %s\n\rFunc Name:     %s\n\rFlags:  %s\n\r",
                 command->name, command->level, npc_position[command->position], log_flag[command->log],
                 command->fun_name, flag_string( command->flags, cmd_flags ) );
      return;
   }

   if( get_trust( ch ) <= LEVEL_SUB_IMPLEM )
   {
      do_cedit( ch, "" );
      return;
   }

   if( !str_cmp( arg2, "raise" ) )
   {
      CMDTYPE *tmp, *tmp_next;
      int hash = command->name[0] % 126;

      if( ( tmp = command_hash[hash] ) == command )
      {
         send_to_char( "That command is already at the top.\n\r", ch );
         return;
      }
      if( tmp->next == command )
      {
         command_hash[hash] = command;
         tmp_next = tmp->next;
         tmp->next = command->next;
         command->next = tmp;
         ch_printf( ch, "Moved %s above %s.\n\r", command->name, command->next->name );
         return;
      }
      for( ; tmp; tmp = tmp->next )
      {
         tmp_next = tmp->next;
         if( tmp_next->next == command )
         {
            tmp->next = command;
            tmp_next->next = command->next;
            command->next = tmp_next;
            ch_printf( ch, "Moved %s above %s.\n\r", command->name, command->next->name );
            return;
         }
      }
      send_to_char( "ERROR -- Not Found!\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "lower" ) )
   {
      CMDTYPE *tmp, *tmp_next;
      int hash = command->name[0] % 126;

      if( command->next == NULL )
      {
         send_to_char( "That command is already at the bottom.\n\r", ch );
         return;
      }
      tmp = command_hash[hash];
      if( tmp == command )
      {
         tmp_next = tmp->next;
         command_hash[hash] = command->next;
         command->next = tmp_next->next;
         tmp_next->next = command;

         ch_printf( ch, "Moved %s below %s.\n\r", command->name, tmp_next->name );
         return;
      }
      for( ; tmp; tmp = tmp->next )
      {
         if( tmp->next == command )
         {
            tmp_next = command->next;
            tmp->next = tmp_next;
            command->next = tmp_next->next;
            tmp_next->next = command;

            ch_printf( ch, "Moved %s below %s.\n\r", command->name, tmp_next->name );
            return;
         }
      }
      send_to_char( "ERROR -- Not Found!\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "list" ) )
   {
      CMDTYPE *tmp;
      int hash = command->name[0] % 126;

      pager_printf( ch, "Priority placement for [%s]:\n\r", command->name );
      for( tmp = command_hash[hash]; tmp; tmp = tmp->next )
      {
         if( tmp == command )
            set_pager_color( AT_GREEN, ch );
         else
            set_pager_color( AT_PLAIN, ch );
         pager_printf( ch, "  %s\n\r", tmp->name );
      }
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      unlink_command( command );
      free_command( command );
      send_to_char( "Command deleted.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "code" ) )
   {
      DO_FUN *fun = skill_function( argument );

      if( fun == skill_notfound )
      {
         send_to_char( "Code not found.\n\r", ch );
         return;
      }
      command->do_fun = fun;
      DISPOSE( command->fun_name );
      command->fun_name = str_dup( argument );
      send_to_char( "Command code updated.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      int level = atoi( argument );

      if( ( level < 0 || level > get_trust( ch ) ) )
      {
         send_to_char( "Level out of range.\r\n", ch );
         return;
      }

      if ( level > command->level && command->do_fun == do_switch )
      {
         command->level = level;
         check_switches(FALSE);
      }
      else
         command->level = level;
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "log" ) )
   {
      int clog = get_logflag( argument );

      if( clog < 0 || clog > LOG_ALL )
      {
         send_to_char( "Log out of range.\n\r", ch );
         return;
      }
      command->log = clog;
      send_to_char( "Command log setting updated.\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "position" ) )
   {
      int pos;

      pos = get_npc_position( argument );

      if( pos < 0 || pos > POS_MAX )
      {
         send_to_char( "Invalid position.\n\r", ch );
         return;
      }
      command->position = pos;
      send_to_char( "Command position updated.\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "flags" ) )
   {
      int flag;
      if( is_number( argument ) )
         flag = atoi( argument );
      else
         flag = get_cmdflag( argument );
      if( flag < 0 || flag >= 32 )
      {
         if( is_number( argument ) )
            send_to_char( "Invalid flag: range is from 0 to 31.\n\r", ch );
         else
            ch_printf( ch, "Unknown flag %s.\n", argument );
         return;
      }

      TOGGLE_BIT( command->flags, 1 << flag );
      send_to_char( "Command flags updated.\n\r", ch );
      return;
   }
   if( !str_cmp( arg2, "name" ) )
   {
      bool relocate;
      CMDTYPE *checkcmd;

      one_argument( argument, arg1 );
      if( arg1[0] == '\0' )
      {
         send_to_char( "Cannot clear name field!\n\r", ch );
         return;
      }
      if( ( checkcmd = find_command( arg1 ) ) != NULL )
      {
         ch_printf( ch, "There is already a command named %s.\n\r", arg1 );
         return;
      }
      if( arg1[0] != command->name[0] )
      {
         unlink_command( command );
         relocate = TRUE;
      }
      else
         relocate = FALSE;
      if( command->name )
         DISPOSE( command->name );
      command->name = str_dup( arg1 );
      if( relocate )
         add_command( command );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   /*
    * display usage message 
    */
   do_cedit( ch, "" );
}

CMDF do_restrict( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   short level;
   CMDTYPE *cmd;
   bool found;

   found = FALSE;
   set_char_color( AT_IMMORT, ch );

   argument = one_argument( argument, arg );
   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Restrict which command?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
      level = get_trust( ch );
   else
   {
      if( !is_number( argument ) )
      {
         send_to_char( "Level must be numeric.\n\r", ch );
         return;
      }
      level = atoi( argument );
   }
   level = UMAX( UMIN( get_trust( ch ), level ), 0 );

   cmd = find_command( arg );
   if( !cmd )
   {
      send_to_char( "No command by that name.\n\r", ch );
      return;
   }
   if( cmd->level > get_trust( ch ) )
   {
      send_to_char( "You may not restrict that command.\n\r", ch );
      return;
   }

   if( !str_prefix( argument, "show" ) )
   {
      cmdf( ch, "%s show", cmd->name );
      return;
   }
   cmd->level = level;
   ch_printf( ch, "You restrict %s to level %d\n\r", cmd->name, level );
   log_printf( "%s restricting %s to level %d", ch->name, cmd->name, level );
   return;
}

/*
 * Display Class information					-Thoric
 */
CMDF do_showclass( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   CLASS_TYPE *Class;
   int cl, low, hi;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Syntax: showclass <Class> [level range]\n\r", ch );
      return;
   }
   if( is_number( arg1 ) && ( cl = atoi( arg1 ) ) >= 0 && cl < MAX_CLASS )
      Class = class_table[cl];
   else
   {
      Class = NULL;
      for( cl = 0; cl < MAX_CLASS && class_table[cl]; cl++ )
         if( !str_cmp( class_table[cl]->who_name, arg1 ) )
         {
            Class = class_table[cl];
            break;
         }
   }
   if( !Class )
   {
      send_to_char( "No such Class.\n\r", ch );
      return;
   }
   pager_printf( ch, "&wCLASS: &W%s\n\r", Class->who_name );
   pager_printf( ch, "&wStarting Weapon: &W%-5d      &wStarting Armor: &W%-5d\n\r", Class->weapon, Class->armor );
   pager_printf( ch, "&wStarting Legwear: &W%-5d     &wStarting Headwear: &W%-5d\n\r", Class->legwear, Class->headwear );
   pager_printf( ch, "&wStarting Armwear: &W%-5d     &wStarting Footwear: &W%-5d\n\r", Class->armwear, Class->footwear );
   pager_printf( ch, "&wStarting Shield: &W%-5d      &wStarting Held Item: &W%-5d\n\r", Class->shield, Class->held );
   pager_printf( ch, "&wMax Skill Adept: &W%-3d      &wBaseThac0 : &W%-5d     &wThac0Gain: &W%f\n\r",
                 Class->skill_adept, Class->base_thac0, Class->thac0_gain );
   pager_printf( ch, "&wHp Min/Hp Max  : &W%-2d/%-2d           &wMana  : &W%-3s\n\r",
                 Class->hp_min, Class->hp_max, Class->fMana ? "yes" : "no " );
   pager_printf( ch, "&wAffected by:  &W%s\n\r", affect_bit_name( &Class->affected ) );
   pager_printf( ch, "&wResistant to: &W%s\n\r", ext_flag_string( &Class->resist, ris_flags ) );
   pager_printf( ch, "&wSusceptible to: &W%s\n\r", ext_flag_string( &Class->suscept, ris_flags ) );

   if( arg2 && arg2[0] != '\0' )
   {
      int x, y, cnt;

      low = UMAX( 0, atoi( arg2 ) );
      hi = URANGE( low, atoi( argument ), MAX_LEVEL );
      for( x = low; x <= hi; x++ )
      {
         pager_printf( ch, "&wLevel: &W%d     &wExperience required: &W%ld\n\r", x, exp_level( x ) );
         pager_printf( ch, "&wMale: &W%-30s &wFemale: &W%s\n\r", title_table[cl][x][0], title_table[cl][x][1] );
         cnt = 0;
         for( y = gsn_first_spell; y < gsn_top_sn; y++ )
            if( skill_table[y]->skill_level[cl] == x )
            {
               pager_printf( ch, "  &[skill]%-7s %-19s%3d     ", skill_tname[skill_table[y]->type],
                             skill_table[y]->name, skill_table[y]->skill_adept[cl] );
               if( ++cnt % 2 == 0 )
                  send_to_pager( "\n\r", ch );
            }
         if( cnt % 2 != 0 )
            send_to_pager( "\n\r", ch );
         send_to_pager( "\n\r", ch );
      }
   }
}

/*
 * Create a new Class online.				    	-Shaddai
 */
bool create_new_class( int Class, char *argument )
{
   int i;

   if( Class >= MAX_CLASS || class_table[Class] == NULL )
      return FALSE;
   STRFREE( class_table[Class]->who_name );
   if( argument && argument[0] != '\0' )
      argument[0] = UPPER( argument[0] );
   class_table[Class]->who_name = STRALLOC( argument );
   xCLEAR_BITS( class_table[Class]->affected );
   class_table[Class]->attr_prime = 0;
   xCLEAR_BITS( class_table[Class]->resist );
   xCLEAR_BITS( class_table[Class]->suscept );
   class_table[Class]->weapon = 0;
   class_table[Class]->skill_adept = 0;
   class_table[Class]->base_thac0 = 0;
   class_table[Class]->thac0_gain = 0;
   class_table[Class]->hp_min = 0;
   class_table[Class]->hp_max = 0;
   class_table[Class]->fMana = FALSE;
   for( i = 0; i < MAX_LEVEL; i++ )
   {
      title_table[Class][i][0] = STRALLOC( "Not set." );
      title_table[Class][i][1] = STRALLOC( "Not set." );
   }
   return TRUE;
}

/*
 * Edit Class information					-Thoric
 */
CMDF do_setclass( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   FILE *fpList;
   char classlist[256];
   CLASS_TYPE *Class;
   int cl, value, i;

   set_char_color( AT_IMMORT, ch );

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1[0] == '\0' )
   {
      send_to_char( "Syntax: setclass <Class> <field> <value>\n\r", ch );
      send_to_char( "Syntax: setclass <Class> create\n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( "  name prime weapon armor legwear headwear\n\r", ch );
      send_to_char( "  armwear footwear shield held basethac0 thac0gain\n\r", ch );
      send_to_char( "  hpmin hpmax mana mtitle ftitle\n\r", ch );
      send_to_char( "  affected resist suscept skill\n\r", ch );
      return;
   }
   if( is_number( arg1 ) && ( cl = atoi( arg1 ) ) >= 0 && cl < MAX_CLASS )
      Class = class_table[cl];
   else
   {
      Class = NULL;
      for( cl = 0; cl < MAX_CLASS && class_table[cl]; cl++ )
      {
         if( !class_table[cl]->who_name )
            continue;
         if( !str_cmp( class_table[cl]->who_name, arg1 ) )
         {
            Class = class_table[cl];
            break;
         }
      }
   }
   if( !str_cmp( arg2, "create" ) && Class )
   {
      send_to_char( "That Class already exists!\n\r", ch );
      return;
   }

   if( !Class && str_cmp( arg2, "create" ) )
   {
      send_to_char( "No such Class.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "create" ) )
   {
      char filename[256];

      if( MAX_PC_CLASS >= MAX_CLASS )
      {
         send_to_char( "You need to up MAX_CLASS in mud and make clean.\r\n", ch );
         return;
      }

      snprintf( filename, sizeof( filename ), "%s.class", arg1 );
      if( !is_valid_filename( ch, CLASS_DIR, filename ) )
         return;

      if( !( create_new_class( MAX_PC_CLASS, arg1 ) ) )
      {
         send_to_char( "Couldn't create a new class.\r\n", ch );
         return;
      }
      write_class_file( MAX_PC_CLASS );
      MAX_PC_CLASS++;

      snprintf( classlist, 256, "%s%s", CLASS_DIR, CLASS_LIST );
      if( !( fpList = fopen( classlist, "w" ) ) )
      {
         bug( "%s", "Can't open class list for writing." );
         return;
      }

      for( i = 0; i < MAX_PC_CLASS; i++ )
         fprintf( fpList, "%s.class\n", class_table[i]->who_name );

      fprintf( fpList, "%s", "$\n" );
      FCLOSE( fpList );
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !argument )
   {
      send_to_char( "You must specify an argument.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "skill" ) )
   {
      SKILLTYPE *skill;
      int sn, level, adept;

      argument = one_argument( argument, arg2 );
      if( ( sn = skill_lookup( arg2 ) ) > 0 )
      {
         skill = get_skilltype( sn );
         argument = one_argument( argument, arg2 );
         level = atoi( arg2 );
         argument = one_argument( argument, arg2 );
         adept = atoi( arg2 );
         skill->skill_level[cl] = level;
         skill->skill_adept[cl] = adept;
         write_class_file( cl );
         ch_printf( ch, "Skill \"%s\" added at level %d and %d%%.\n\r", skill->name, level, adept );
      }
      else
         ch_printf( ch, "No such skill as %s.\n\r", arg2 );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      char buf[256];
      struct class_type *ccheck = NULL;

      one_argument( argument, arg1 );
      if( !arg1 || arg1[0] == '\0' )
      {
         send_to_char( "You can't set a class name to nothing.\r\n", ch );
         return;
      }

      snprintf( buf, sizeof( buf ), "%s.class", arg1 );
      if( !is_valid_filename( ch, CLASS_DIR, buf ) )
         return;

      for( i = 0; i < MAX_PC_CLASS && class_table[i]; i++ )
      {
         if( !class_table[i]->who_name )
            continue;

         if( !str_cmp( class_table[i]->who_name, arg1 ) )
         {
            ccheck = class_table[i];
            break;
         }
      }
      if( ccheck != NULL )
      {
         ch_printf( ch, "Already a class called %s.\r\n", arg1 );
         return;
      }

      snprintf( buf, sizeof( buf ), "%s%s.class", CLASS_DIR, Class->who_name );
      unlink( buf );
      if( Class->who_name )
         STRFREE( Class->who_name );
      Class->who_name = STRALLOC( capitalize( argument ) );
      ch_printf( ch, "class renamed to %s.\r\n", arg1 );
      write_class_file( cl );

      snprintf( classlist, 256, "%s%s", CLASS_DIR, CLASS_LIST );
      if( !( fpList = fopen( classlist, "w" ) ) )
      {
         bug( "%s", "Can't open class list for writing." );
         return;
      }

      for( i = 0; i < MAX_PC_CLASS; i++ )
         fprintf( fpList, "%s%s.class\n", CLASS_DIR, class_table[i]->who_name );

      fprintf( fpList, "%s", "$\n" );
      FCLOSE( fpList );
      return;
   }

   if( !str_cmp( arg2, "affected" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: setclass <Class> affected <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg2 );
         value = get_aflag( arg2 );
         if( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\n\r", arg2 );
         else
            xTOGGLE_BIT( Class->affected, value );
      }
      send_to_char( "Done.\n\r", ch );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "resist" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: setclass <Class> resist <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg2 );
         value = get_risflag( arg2 );
         if( value < 0 || value > 31 )
            ch_printf( ch, "Unknown flag: %s\n\r", arg2 );
         else
            xTOGGLE_BIT( Class->resist, value );
      }
      send_to_char( "Done.\n\r", ch );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "suscept" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: setclass <Class> suscept <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg2 );
         value = get_risflag( arg2 );
         if( value < 0 || value > 31 )
            ch_printf( ch, "Unknown flag: %s\n\r", arg2 );
         else
            xTOGGLE_BIT( Class->suscept, value );
      }
      send_to_char( "Done.\n\r", ch );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "prime" ) )
   {
      int x = get_atype( argument );

      if( x < APPLY_NONE || ( x > APPLY_CON && x != APPLY_LCK && x != APPLY_CHA ) )
         send_to_char( "Invalid prime attribute!\r\n", ch );
      else
      {
         Class->attr_prime = x;
         send_to_char( "Prime attribute set.\r\n", ch );
         write_class_file( cl );
      }
      return;
   }
   if( !str_cmp( arg2, "weapon" ) )
   {
      Class->weapon = atoi( argument );
      send_to_char( "Starting weapon set.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "armor" ) )
   {
      Class->armor = atoi( argument );
      send_to_char( "Starting armor set.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "legwear" ) )
   {
      Class->legwear = atoi( argument );
      send_to_char( "Starting legwear set.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "headwear" ) )
   {
      Class->headwear = atoi( argument );
      send_to_char( "Starting headwear set.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "armwear" ) )
   {
      Class->armwear = atoi( argument );
      send_to_char( "Starting armwear set.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "footwear" ) )
   {
      Class->footwear = atoi( argument );
      send_to_char( "Starting footwear set.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "shield" ) )
   {
      Class->shield = atoi( argument );
      send_to_char( "Starting shield set.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "held" ) )
   {
      Class->held = atoi( argument );
      send_to_char( "Starting held item set.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "basethac0" ) )
   {
      Class->base_thac0 = atoi( argument );
      send_to_char( "Base Thac0 set.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "thac0gain" ) )
   {
      Class->thac0_gain = atof( argument );
      send_to_char( "Thac0gain set.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "hpmin" ) )
   {
      Class->hp_min = atoi( argument );
      send_to_char( "Min HP gain set.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "hpmax" ) )
   {
      Class->hp_max = atoi( argument );
      send_to_char( "Max HP gain set.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "mana" ) )
   {
      if( UPPER( argument[0] ) == 'Y' )
         Class->fMana = TRUE;
      else
         Class->fMana = FALSE;
      send_to_char( "Mana flag toggled.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "mtitle" ) )
   {
      char arg3[MIL];
      int x;

      argument = one_argument( argument, arg3 );
      if( arg3[0] == '\0' || argument[0] == '\0' )
      {
         send_to_char( "Syntax: setclass <Class> mtitle <level> <title>\n\r", ch );
         return;
      }
      if( ( x = atoi( arg3 ) ) < 0 || x > MAX_LEVEL )
      {
         send_to_char( "Invalid level.\n\r", ch );
         return;
      }
      STRFREE( title_table[cl][x][SEX_MALE] );
      title_table[cl][x][SEX_MALE] = STRALLOC( argument );
      send_to_char( "Done.\n\r", ch );
      write_class_file( cl );
      return;
   }
   if( !str_cmp( arg2, "ftitle" ) )
   {
      char arg3[MIL], arg4[MIL];
      int x;

      argument = one_argument( argument, arg3 );
      argument = one_argument( argument, arg4 );
      if( arg3[0] == '\0' || argument[0] == '\0' )
      {
         send_to_char( "Syntax: setclass <Class> ftitle <level> <title>\n\r", ch );
         return;
      }
      if( ( x = atoi( arg4 ) ) < 0 || x > MAX_LEVEL )
      {
         send_to_char( "Invalid level.\n\r", ch );
         return;
      }
      STRFREE( title_table[cl][x][SEX_FEMALE] );
      /*
       * Bug fix below -Shaddai
       */
      title_table[cl][x][SEX_FEMALE] = STRALLOC( argument );
      send_to_char( "Done\n\r", ch );
      write_class_file( cl );
      return;
   }
   do_setclass( ch, "" );
}

/*
 * Create an instance of a new race.			-Shaddai
 */
bool create_new_race( int race, char *argument )
{
   int i = 0;
   if( race >= MAX_RACE || race_table[race] == NULL )
      return FALSE;
   for( i = 0; i < MAX_WHERE_NAME; i++ )
      race_table[race]->where_name[i] = where_name[i];
   if( argument[0] != '\0' )
      argument[0] = UPPER( argument[0] );
   snprintf( race_table[race]->race_name, 16, "%s", argument );
   race_table[race]->class_restriction = 0;
   race_table[race]->str_plus = 0;
   race_table[race]->dex_plus = 0;
   race_table[race]->wis_plus = 0;
   race_table[race]->int_plus = 0;
   race_table[race]->con_plus = 0;
   race_table[race]->cha_plus = 0;
   race_table[race]->lck_plus = 0;
   race_table[race]->hit = 0;
   race_table[race]->mana = 0;
   xCLEAR_BITS( race_table[race]->affected );
   xCLEAR_BITS( race_table[race]->resist );
   xCLEAR_BITS( race_table[race]->suscept );
   race_table[race]->language = 0;
   race_table[race]->alignment = 0;
   race_table[race]->minalign = 0;
   race_table[race]->maxalign = 0;
   race_table[race]->ac_plus = 0;
   race_table[race]->exp_multiplier = 0;
   xCLEAR_BITS( race_table[race]->attacks );
   xCLEAR_BITS( race_table[race]->defenses );
   race_table[race]->height = 0;
   race_table[race]->weight = 0;
   race_table[race]->hunger_mod = 0;
   race_table[race]->thirst_mod = 0;
   race_table[race]->mana_regen = 0;
   race_table[race]->hp_regen = 0;
   return TRUE;
}

/* Modified by Samson to allow setting language by name - 8-6-98 */
CMDF do_setrace( CHAR_DATA * ch, char *argument )
{
   RACE_TYPE *race;
   char arg1[MIL], arg2[MIL], arg3[MIL];
   FILE *fpList;
   char racelist[256];
   int value, v2, ra, i;

   set_char_color( AT_IMMORT, ch );

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Syntax: setrace <race> <field> <value>\n\r", ch );
      send_to_char( "Syntax: setrace <race> create	     \n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( "  name classes strplus dexplus wisplus\n\r", ch );
      send_to_char( "  intplus conplus chaplus lckplus hit\n\r", ch );
      send_to_char( "  mana affected resist suscept language\n\r", ch );
      send_to_char( "  attack defense alignment acplus \n\r", ch );
      send_to_char( "  minalign maxalign height weight      \n\r", ch );
      send_to_char( "  hungermod thirstmod expmultiplier    \n\r", ch );
      send_to_char( "  saving_poison_death saving_wand      \n\r", ch );
      send_to_char( "  saving_para_petri saving_breath      \n\r", ch );
      send_to_char( "  saving_spell_staff                   \n\r", ch );
      send_to_char( "  mana_regen hp_regen                  \n\r", ch );
      return;
   }
   if( is_number( arg1 ) && ( ra = atoi( arg1 ) ) >= 0 && ra < MAX_RACE )
      race = race_table[ra];
   else
   {
      race = NULL;
      for( ra = 0; ra < MAX_RACE && race_table[ra]; ra++ )
      {
         if( !race_table[ra]->race_name )
            continue;

         if( !str_cmp( race_table[ra]->race_name, arg1 ) )
         {
            race = race_table[ra];
            break;
         }
      }
   }
   if( !str_cmp( arg2, "create" ) && race )
   {
      send_to_char( "That race already exists!\n\r", ch );
      return;
   }
   else if( !race && str_cmp( arg2, "create" ) )
   {
      send_to_char( "No such race.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "create" ) )
   {
      char filename[256];

      if( MAX_PC_RACE >= MAX_RACE )
      {
         send_to_char( "You need to up MAX_RACE in mud.h and make clean.\r\n", ch );
         return;
      }

      snprintf( filename, sizeof( filename ), "%s.race", arg1 );
      if( !is_valid_filename( ch, RACE_DIR, filename ) )
         return;

      if( ( create_new_race( MAX_PC_RACE, arg1 ) ) == FALSE )
      {
         send_to_char( "Couldn't create a new race.\r\n", ch );
         return;
      }
      write_race_file( MAX_PC_RACE );
      MAX_PC_RACE++;

      snprintf( racelist, 256, "%s%s", RACE_DIR, RACE_LIST );
      if( !( fpList = fopen( racelist, "w" ) ) )
      {
         bug( "%s", "Error opening racelist." );
         return;
      }
      for( i = 0; i < MAX_PC_RACE; i++ )
         fprintf( fpList, "%s.race\n", race_table[i]->race_name );
      fprintf( fpList, "%s", "$\n" );
      fclose( fpList );
      fpList = NULL;
      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( !argument )
   {
      send_to_char( "You must specify an argument.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      char buf[256];
      RACE_TYPE *rcheck = NULL;

      one_argument( argument, arg1 );

      if( !arg1 || arg1[0] == '\0' )
      {
         send_to_char( "You can't set a race name to nothing.\r\n", ch );
         return;
      }

      snprintf( buf, sizeof( buf ), "%s.race", arg1 );
      if( !is_valid_filename( ch, RACE_DIR, buf ) )
         return;

      for( i = 0; i < MAX_PC_RACE && race_table[i]; i++ )
      {
         if( !race_table[i]->race_name )
            continue;

         if( !str_cmp( race_table[i]->race_name, arg1 ) )
         {
            rcheck = race_table[i];
            break;
         }
      }
      if( rcheck != NULL )
      {
         ch_printf( ch, "Already a race called %s.\r\n", arg1 );
         return;
      }

      snprintf( buf, sizeof( buf ), "%s%s.race", RACE_DIR, race->race_name );
      unlink( buf );

      snprintf( race->race_name, 16, "%s", capitalize( argument ) );
      write_race_file( ra );

      snprintf( racelist, 256, "%s%s", RACE_DIR, RACE_LIST );
      if( !( fpList = fopen( racelist, "w" ) ) )
      {
         bug( "%s", "Error opening racelist." );
         return;
      }
      for( i = 0; i < MAX_PC_RACE; i++ )
         fprintf( fpList, "%s.race\n", race_table[i]->race_name );
      fprintf( fpList, "%s", "$\n" );
      FCLOSE( fpList );
      send_to_char( "Race name set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "strplus" ) )
   {
      race->str_plus = ( short ) atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "dexplus" ) )
   {
      race->dex_plus = ( short ) atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "wisplus" ) )
   {
      race->wis_plus = ( short ) atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "intplus" ) )
   {
      race->int_plus = ( short ) atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "conplus" ) )
   {
      race->con_plus = ( short ) atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "chaplus" ) )
   {
      race->cha_plus = ( short ) atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "lckplus" ) )
   {
      race->lck_plus = ( short ) atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "hit" ) )
   {
      race->hit = ( short ) atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "mana" ) )
   {
      race->mana = ( short ) atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "affected" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: setrace <race> affected <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_aflag( arg3 );
         if( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( race->affected, value );
      }
      write_race_file( ra );
      send_to_char( "Racial affects set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "resist" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: setrace <race> resist <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value > 31 )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( race->resist, value );
      }
      write_race_file( ra );
      send_to_char( "Racial resistances set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "suscept" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: setrace <race> suscept <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value > 31 )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( race->suscept, value );
      }
      write_race_file( ra );
      send_to_char( "Racial susceptabilities set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "language" ) )
   {
      argument = one_argument( argument, arg3 );
      value = get_langflag( arg3 );
      if( value == LANG_UNKNOWN )
         ch_printf( ch, "Unknown language: %s\n\r", arg3 );
      else
      {
         if( !( value &= VALID_LANGS ) )
            ch_printf( ch, "Player races may not speak %s.\n\r", arg3 );
      }

      v2 = get_langnum( arg3 );
      if( v2 == -1 )
         ch_printf( ch, "Unknown language: %s\n\r", arg3 );
      else
         TOGGLE_BIT( race->language, 1 << v2 );

      write_race_file( ra );
      send_to_char( "Racial language set.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "classes" ) )
   {
      for( i = 0; i < MAX_CLASS; i++ )
      {
         if( !str_cmp( argument, class_table[i]->who_name ) )
         {
            TOGGLE_BIT( race->class_restriction, 1 << i );  /* k, that's boggling */
            write_race_file( ra );
            send_to_char( "Classes set.\n\r", ch );
            return;
         }
      }
      send_to_char( "No such class.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "acplus" ) )
   {
      race->ac_plus = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "alignment" ) )
   {
      race->alignment = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   /*
    * not implemented 
    */
   if( !str_cmp( arg2, "defense" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: setrace <race> defense <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_defenseflag( arg3 );
         if( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( race->defenses, value );
      }
      write_race_file( ra );
      return;
   }

   /*
    * not implemented 
    */
   if( !str_cmp( arg2, "attack" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Usage: setrace <race> attack <flag> [flag]...\n\r", ch );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_attackflag( arg3 );
         if( value < 0 || value > MAX_BITS )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
            xTOGGLE_BIT( race->attacks, value );
      }
      write_race_file( ra );
      return;
   }

   if( !str_cmp( arg2, "minalign" ) )
   {
      race->minalign = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "maxalign" ) )
   {
      race->maxalign = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "height" ) )
   {
      race->height = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "weight" ) )
   {
      race->weight = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "thirstmod" ) )
   {
      race->thirst_mod = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "hungermod" ) )
   {
      race->hunger_mod = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "maxalign" ) )
   {
      race->maxalign = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "expmultiplier" ) )
   {
      race->exp_multiplier = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "saving_poison_death" ) )
   {
      race->saving_poison_death = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "saving_wand" ) )
   {
      race->saving_wand = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "saving_para_petri" ) )
   {
      race->saving_para_petri = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "saving_breath" ) )
   {
      race->saving_breath = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "saving_spell_staff" ) )
   {
      race->saving_spell_staff = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   /*
    * unimplemented stuff follows 
    */
   if( !str_cmp( arg2, "mana_regen" ) )
   {
      race->mana_regen = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "hp_regen" ) )
   {
      race->hp_regen = atoi( argument );
      write_race_file( ra );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   do_setrace( ch, "" );
}

/* Modified by Samson to display language spoken by race - 8-6-98 */
CMDF do_showrace( CHAR_DATA * ch, char *argument )
{
   struct race_type *race;
   int ra, i, ct;

   set_pager_color( AT_PLAIN, ch );

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax: showrace  \n\r", ch );
      /*
       * Show the races code addition by Blackmane 
       */
      /*
       * fixed printout by Miki 
       */
      ct = 0;
      for( i = 0; i < MAX_RACE; i++ )
      {
         ++ct;
         pager_printf( ch, "%2d> %-11s", i, race_table[i]->race_name );
         if( ct % 5 == 0 )
            send_to_pager( "\n\r", ch );
      }
      send_to_pager( "\n\r", ch );
      return;
   }
   if( is_number( argument ) && ( ra = atoi( argument ) ) >= 0 && ra < MAX_RACE )
      race = race_table[ra];
   else
   {
      race = NULL;
      for( ra = 0; ra < MAX_RACE && race_table[ra]; ra++ )
         if( !str_cmp( race_table[ra]->race_name, argument ) )
         {
            race = race_table[ra];
            break;
         }
   }
   if( !race )
   {
      send_to_char( "No such race.\n\r", ch );
      return;
   }

   ch_printf( ch, "RACE: %s\n\r", race->race_name );
   ct = 0;
   send_to_char( "Disallowed Classes: ", ch );
   for( i = 0; i < MAX_CLASS; i++ )
   {
      if( IS_SET( race->class_restriction, 1 << i ) )
      {
         ct++;
         ch_printf( ch, "%s ", class_table[i]->who_name );
         if( ct % 6 == 0 )
            send_to_char( "\n\r", ch );
      }
   }
   if( ( ct % 6 != 0 ) || ( ct == 0 ) )
      send_to_char( "\n\r", ch );

   ct = 0;
   send_to_char( "Allowed Classes: ", ch );
   for( i = 0; i < MAX_CLASS; i++ )
   {
      if( !IS_SET( race->class_restriction, 1 << i ) )
      {
         ct++;
         ch_printf( ch, "%s ", class_table[i]->who_name );
         if( ct % 6 == 0 )
            send_to_char( "\n\r", ch );
      }
   }
   if( ( ct % 6 != 0 ) || ( ct == 0 ) )
      send_to_char( "\n\r", ch );

   ch_printf( ch, "Str Plus: %-3d\tDex Plus: %-3d\tWis Plus: %-3d\tInt Plus: %-3d\t\n\r",
              race->str_plus, race->dex_plus, race->wis_plus, race->int_plus );

   ch_printf( ch, "Con Plus: %-3d\tCha Plus: %-3d\tLck Plus: %-3d\n\r", race->con_plus, race->cha_plus, race->lck_plus );

   ch_printf( ch, "Hit Pts:  %-3d\tMana: %-3d\tAlign: %-4d\tAC: %-d\n\r",
              race->hit, race->mana, race->alignment, race->ac_plus );

   ch_printf( ch, "Min Align: %d\tMax Align: %-d\t\tXP Mult: %-d%%\n\r",
              race->minalign, race->maxalign, race->exp_multiplier );

   ch_printf( ch, "Height: %3d in.\t\tWeight: %4d lbs.\tHungerMod: %d\tThirstMod: %d\n\r",
              race->height, race->weight, race->hunger_mod, race->thirst_mod );

   ch_printf( ch, "Spoken Language: %s", flag_string( race->language, lang_names ) );
   send_to_char( "\n\r", ch );

   send_to_char( "Affected by: ", ch );
   send_to_char( affect_bit_name( &race->affected ), ch );
   send_to_char( "\n\r", ch );

   send_to_char( "Resistant to: ", ch );
   send_to_char( ext_flag_string( &race->resist, ris_flags ), ch );
   send_to_char( "\n\r", ch );

   send_to_char( "Susceptible to: ", ch );
   send_to_char( ext_flag_string( &race->suscept, ris_flags ), ch );
   send_to_char( "\n\r", ch );

   ch_printf( ch, "Saves: (P/D) %d (W) %d (P/P) %d (B) %d (S/S) %d\n\r",
              race->saving_poison_death, race->saving_wand, race->saving_para_petri,
              race->saving_breath, race->saving_spell_staff );

   send_to_char( "Innate Attacks: ", ch );
   send_to_char( ext_flag_string( &race->attacks, attack_flags ), ch );
   send_to_char( "\n\r", ch );

   send_to_char( "Innate Defenses: ", ch );
   send_to_char( ext_flag_string( &race->defenses, defense_flags ), ch );
   send_to_char( "\n\r", ch );
   return;
}

/* Simple, small way to make keeping track of small mods easier - Blod */
CMDF do_fixed( CHAR_DATA * ch, char *argument )
{
   struct tm *t = localtime( &current_time );

   set_char_color( AT_OBJECT, ch );
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "\n\rUsage:  'fixed list' or 'fixed <message>'", ch );
      if( get_trust( ch ) >= LEVEL_ASCENDANT )
         send_to_char( " or 'fixed clear now'\n\r", ch );
      else
         send_to_char( "\n\r", ch );
      return;
   }
   if( !str_cmp( argument, "clear now" ) && get_trust( ch ) >= LEVEL_ASCENDANT )
   {
      FILE *fp = fopen( FIXED_FILE, "w" );
      if( fp )
         FCLOSE( fp );
      send_to_char( "Fixed file cleared.\n\r", ch );
      return;
   }
   if( !str_cmp( argument, "list" ) )
   {
      send_to_char( "\n\r&g[&GDate  &g|  &GVnum&g]\n\r", ch );
      show_file( ch, FIXED_FILE );
   }
   else
   {
      append_to_file( FIXED_FILE, "&g|&G%-2.2d/%-2.2d &g| &G%5d&g|  %s:  &G%s",
                      t->tm_mon + 1, t->tm_mday, ch->in_room ? ch->in_room->vnum : 0,
                      IS_NPC( ch ) ? ch->short_descr : ch->name, argument );
      send_to_char( "Thanks, your modification has been logged.\n\r", ch );
   }
   return;
}

/*
 * Command to display the weather status of all the areas
 * Last Modified: July 21, 1997
 * Fireblade
 *
 * EGAD! This thing was one UGLY function before! This is much nicer to look at, yes?
 * Samson 9-18-03
 */
CMDF do_showweather( CHAR_DATA * ch, char *argument )
{
   AREA_DATA *pArea;

   if( !ch )
   {
      bug( "%s", "do_showweather: NULL char data" );
      return;
   }

   ch_printf( ch, "&B%-40s %-8s %-8s %-8s\n\r", "Area Name:", "Temp:", "Precip:", "Wind:" );

   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      if( !argument || argument[0] == '\0' || nifty_is_name_prefix( argument, pArea->name ) )
      {
         ch_printf( ch, "&B%-40s &W%3d &B(&C%3d&B) &W%3d &B(&C%3d&B) &W%3d &B(&C%3d&B)\n\r",
                    pArea->name, pArea->weather->temp, pArea->weather->temp_vector, pArea->weather->precip,
                    pArea->weather->precip_vector, pArea->weather->wind, pArea->weather->wind_vector );
      }
   }
   return;
}

 /*
  * Added by Adjani, 12-23-02. Plan to add more to this, but for now it works. 
  */
CMDF do_forgefind( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *smith;
   bool found;

   found = FALSE;
   send_to_pager( "\n\r", ch );
   for( smith = first_char; smith; smith = smith->next )
   {
      if( IS_ACT_FLAG( smith, ACT_SMITH ) || IS_ACT_FLAG( smith, ACT_GUILDFORGE ) )
      {
         found = TRUE;
         pager_printf( ch, "Forge: *%5d* %-30s at *%5d*, %s.\n\r",
                       smith->pIndexData->vnum, smith->short_descr, smith->in_room->vnum, smith->in_room->name );
      }
   }

   if( !found )
      send_to_char( "You didn't find any forges.\n\r", ch );
   return;
}
