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
 *                          Dynamic Channel System                          *
 ****************************************************************************/

#include <string.h>
#include <ctype.h>
#include "mud.h"
#include "channels.h"

char *mxp_chan_str( CHAR_DATA * ch, const char *verb );
char *mxp_chan_str_close( CHAR_DATA * ch, const char *verb );
char *translate( int percent, const char *in, const char *name );
char *mini_c_time( time_t curtime, int tz );
#if !defined(__CYGWIN__)
#ifdef MULTIPORT
void mud_message( CHAR_DATA * ch, MUD_CHANNEL * channel, char *arg );
#endif
#endif

MUD_CHANNEL *first_channel;
MUD_CHANNEL *last_channel;

char *const chan_types[] = {
   "Global", "Zone", "Guild", "Council", "PK", "Log"
};

char *const chan_flags[] = {
   "keephistory", "interport"
};

int get_chantypes( char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( chan_types ) / sizeof( chan_types[0] ); x++ )
      if( !str_cmp( name, chan_types[x] ) )
         return x;
   return -1;
}

int get_chanflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( chan_flags ) / sizeof( chan_flags[0] ) ); x++ )
      if( !str_cmp( flag, chan_flags[x] ) )
         return x;
   return -1;
}

void read_channel( MUD_CHANNEL * channel, FILE * fp )
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

         case 'C':
            KEY( "ChanName", channel->name, fread_string_nohash( fp ) );
            KEY( "ChanColorname", channel->colorname, fread_string( fp ) );
            KEY( "ChanLevel", channel->level, fread_number( fp ) );
            KEY( "ChanType", channel->type, fread_number( fp ) );
            KEY( "ChanFlags", channel->flags, fread_number( fp ) );
            /*
             * Legacy conversion for old channel files 
             */
            if( !str_cmp( word, "ChanHistory" ) )
            {
               int temph = fread_number( fp );
               if( temph == TRUE )
                  SET_CHANFLAG( channel, CHAN_KEEPHISTORY );
               fMatch = TRUE;
               break;
            }
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !channel->colorname || channel->colorname[0] == '\0' )
                  channel->colorname = STRALLOC( "chat" );
               return;
            }
            break;
      }
      if( !fMatch )
         bug( "read_channel: no match: %s", word );
   }
}

void load_mudchannels( void )
{
   FILE *fp;
   MUD_CHANNEL *channel;

   first_channel = NULL;
   last_channel = NULL;

   log_string( "Loading mud channels..." );

   if( ( fp = fopen( CHANNEL_FILE, "r" ) ) == NULL )
   {
      bug( "%s", "No channel file found." );
      return;
   }

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
         bug( "%s", "load_channels: # not found." );
         break;
      }

      word = fread_word( fp );
      if( !str_cmp( word, "CHANNEL" ) )
      {
         CREATE( channel, MUD_CHANNEL, 1 );
         read_channel( channel, fp );

         LINK( channel, first_channel, last_channel, next, prev );
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "load_channels: bad section: %s.", word );
         continue;
      }
   }
   FCLOSE( fp );
   return;
}

void save_mudchannels( void )
{
   FILE *fp;
   MUD_CHANNEL *channel;

   if( ( fp = fopen( CHANNEL_FILE, "w" ) ) == NULL )
   {
      bug( "%s", "Couldn't write to channel file." );
      return;
   }

   for( channel = first_channel; channel; channel = channel->next )
   {
      if( channel->name )
      {
         fprintf( fp, "%s", "#CHANNEL\n" );
         fprintf( fp, "ChanName    %s~\n", channel->name );
         fprintf( fp, "ChanColorname %s~\n", channel->colorname );
         fprintf( fp, "ChanLevel   %d\n", channel->level );
         fprintf( fp, "ChanType    %d\n", channel->type );
         fprintf( fp, "ChanFlags %d\n", channel->flags );
         fprintf( fp, "%s", "End\n\n" );
      }
   }
   fprintf( fp, "%s", "#END\n" );
   FCLOSE( fp );
}

MUD_CHANNEL *find_channel( char *name )
{
   MUD_CHANNEL *channel = NULL;

   for( channel = first_channel; channel; channel = channel->next )
   {
      if( !str_prefix( name, channel->name ) )
         return channel;
   }
   return NULL;
}

CMDF do_makechannel( CHAR_DATA * ch, char *argument )
{
   MUD_CHANNEL *channel;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "&GSyntax: makechannel <name>\n\r", ch );
      return;
   }

   if( ( channel = find_channel( argument ) ) )
   {
      send_to_char( "&RA channel with that name already exists.\n\r", ch );
      return;
   }

   smash_tilde( argument );
   CREATE( channel, MUD_CHANNEL, 1 );
   channel->name = str_dup( argument );
   channel->colorname = STRALLOC( "chat" );
   channel->level = LEVEL_IMMORTAL;
   channel->type = CHAN_GLOBAL;
   channel->flags = 0;
   LINK( channel, first_channel, last_channel, next, prev );
   ch_printf( ch, "&YNew channel &G%s &Ycreated.\n\r", argument );
   save_mudchannels(  );
   return;
}

CMDF do_setchannel( CHAR_DATA * ch, char *argument )
{
   MUD_CHANNEL *channel;
   char arg[MIL], arg2[MIL];

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "&GSyntax: setchannel <channel> <field> <value>\n\r\n\r", ch );
      send_to_char( "&YField may be one of the following:\n\r", ch );
      send_to_char( "name level type flags color\n\r", ch );
      return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, arg );

   if( !( channel = find_channel( arg ) ) )
   {
      send_to_char( "&RNo channel by that name exists.\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg2 );

   if( !arg || arg2[0] == '\0' )
   {
      do_setchannel( ch, "" );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      ch_printf( ch, "&YChannel &G%s &Yrenamed to &G%s\n\r", channel->name, argument );
      DISPOSE( channel->name );
      channel->name = str_dup( argument );
      save_mudchannels(  );
      return;
   }

   if( !str_cmp( arg2, "color" ) )
   {
      ch_printf( ch, "&YChannel &G%s &Ycolor changed to &G%s\n\r", channel->name, argument );
      STRFREE( channel->colorname );
      channel->colorname = STRALLOC( argument );
      save_mudchannels(  );
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      int level;

      if( !is_number( argument ) )
      {
         send_to_char( "&RLevel must be numerical.\n\r", ch );
         return;
      }

      level = atoi( argument );

      if( level < 1 || level > MAX_LEVEL )
      {
         ch_printf( ch, "&RInvalid level. Acceptable range is 1 to %d.\n\r", MAX_LEVEL );
         return;
      }

      channel->level = level;
      ch_printf( ch, "&YChannel &G%s &Ylevel changed to &G%d\n\r", channel->name, level );
      save_mudchannels(  );
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      int type = get_chantypes( argument );

      if( type == -1 )
      {
         send_to_char( "&RInvalid channel type.\n\r", ch );
         return;
      }

      channel->type = type;
      ch_printf( ch, "&YChannel &G%s &Ytype changed to &G%s\n\r", channel->name, argument );
      save_mudchannels(  );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      char arg3[MIL];
      int value;

      if( !argument || argument[0] == '\0' )
      {
         do_setchannel( ch, "" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_chanflag( arg3 );
         if( value < 0 || value > 31 )
            ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
         else
         {
            if( IS_CHANFLAG( channel, 1 << value ) )
               REMOVE_CHANFLAG( channel, 1 << value );
            else
               SET_CHANFLAG( channel, 1 << value );
         }
      }
      send_to_char( "Channel flags set.\n\r", ch );
      save_mudchannels(  );
      return;
   }

   do_setchannel( ch, "" );
   return;
}

void delete_channel( MUD_CHANNEL * channel )
{
   int loopa;

   UNLINK( channel, first_channel, last_channel, next, prev );
   for( loopa = 0; loopa < MAX_CHANHISTORY; loopa++ )
   {
      DISPOSE( channel->history[loopa][0] );
      DISPOSE( channel->history[loopa][1] );
   }
   DISPOSE( channel->name );
   STRFREE( channel->colorname );
   DISPOSE( channel );
   return;
}

void free_mudchannels( void )
{
   MUD_CHANNEL *channel, *channel_next;

   for( channel = first_channel; channel; channel = channel_next )
   {
      channel_next = channel->next;
      delete_channel( channel );
   }
   return;
}

CMDF do_destroychannel( CHAR_DATA * ch, char *argument )
{
   MUD_CHANNEL *channel;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "&GSyntax: destroychannel <name>\n\r", ch );
      return;
   }

   if( !( channel = find_channel( argument ) ) )
   {
      send_to_char( "&RNo channel with that name exists.\n\r", ch );
      return;
   }
   delete_channel( channel );
   ch_printf( ch, "&YChannel &G%s &Ydestroyed.\n\r", argument );
   save_mudchannels(  );
   return;
}

CMDF do_showchannels( CHAR_DATA * ch, char *argument )
{
   MUD_CHANNEL *channel;

   send_to_char( "&WName               &YLevel &cColor     &BType       &GFlags\n\r", ch );
   send_to_char( "&W----------------------------------------------------------\n\r", ch );
   for( channel = first_channel; channel; channel = channel->next )
      ch_printf( ch, "&W%-18s &Y%-4d  &c%-10s &B%-10s &G%s\n\r", capitalize( channel->name ),
                 channel->level, channel->colorname, chan_types[channel->type], flag_string( channel->flags, chan_flags ) );
   return;
}

/* Stuff borrowed from I3/MUD-Net code to handle channel listening */

/*  changetarg: extract a single argument (with given max length) from
 *  argument to arg; if arg==NULL, just skip an arg, don't copy it out
 */
const char *getarg( const char *argument, char *arg, int length )
{
   int len = 0;

   if( !argument || argument[0] == '\0' )
   {
      if( arg )
         arg[0] = '\0';

      return argument;
   }

   while( *argument && isspace( *argument ) )
      argument++;

   if( arg )
      while( *argument && !isspace( *argument ) && len < length - 1 )
         *arg++ = *argument++, len++;
   else
      while( *argument && !isspace( *argument ) )
         argument++;

   while( *argument && !isspace( *argument ) )
      argument++;

   while( *argument && isspace( *argument ) )
      argument++;

   if( arg )
      *arg = '\0';

   return argument;
}

/* Check for a name in a list */
int hasname( const char *list, const char *name )
{
   const char *p;
   char arg[MIL];

   if( !list )
      return ( 0 );

   p = getarg( list, arg, MIL );
   while( arg[0] )
   {
      if( !strcasecmp( name, arg ) )
         return 1;
      p = getarg( p, arg, MIL );
   }

   return 0;
}

/* Add a name to a list */
void addname( char **list, const char *name )
{
   char buf[MSL];

   if( hasname( *list, name ) )
      return;

   if( *list && *list[0] != '\0' )
      snprintf( buf, MSL, "%s %s", *list, name );
   else
      mudstrlcpy( buf, name, MSL );

   STRFREE( *list );
   *list = STRALLOC( buf );
}

/* Remove a name from a list */
void removename( char **list, const char *name )
{
   char buf[MSL];
   char arg[MIL];
   const char *p;

   buf[0] = '\0';
   p = getarg( *list, arg, MIL );
   while( arg[0] )
   {
      if( strcasecmp( arg, name ) )
      {
         if( buf[0] )
            mudstrlcat( buf, " ", MSL );
         mudstrlcat( buf, arg, MSL );
      }
      p = getarg( p, arg, MIL );
   }

   STRFREE( *list );
   *list = STRALLOC( buf );
}

CMDF do_listen( CHAR_DATA * ch, char *argument )
{
   MUD_CHANNEL *channel;

   if( IS_NPC( ch ) )
   {
      send_to_char( "NPCs cannot change channels.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "&GSyntax: listen <channel>\n\r", ch );
      send_to_char( "&GSyntax: listen all\n\r", ch );
      send_to_char( "&GSyntax: listen none\n\r", ch );
      send_to_char( "&GFor a list of channels, type &Wchannels\n\r", ch );
      send_to_char( "&YYou are listening to the following local mud channels:\n\r\n\r", ch );
      ch_printf( ch, "&W%s\n\r", ch->pcdata->chan_listen );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      for( channel = first_channel; channel; channel = channel->next )
      {
         if( ch->level >= channel->level && !hasname( ch->pcdata->chan_listen, channel->name ) )
            addname( &ch->pcdata->chan_listen, channel->name );
      }
      send_to_char( "&YYou are now listening to all available channels.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "none" ) )
   {
      for( channel = first_channel; channel; channel = channel->next )
      {
         if( hasname( ch->pcdata->chan_listen, channel->name ) )
            removename( &ch->pcdata->chan_listen, channel->name );
      }
      send_to_char( "&YYou no longer listen to any available channels.\n\r", ch );
      return;
   }

   if( hasname( ch->pcdata->chan_listen, argument ) )
   {
      removename( &ch->pcdata->chan_listen, argument );
      ch_printf( ch, "&YYou no longer listen to &W%s\n\r", argument );
   }
   else
   {
      if( !( channel = find_channel( argument ) ) )
      {
         send_to_char( "No such channel.\n\r", ch );
         return;
      }
      if( channel->level > ch->level )
      {
         send_to_char( "That channel is above your level.\n\r", ch );
         return;
      }
      addname( &ch->pcdata->chan_listen, channel->name );
      ch_printf( ch, "&YYou now listen to &W%s\n\r", channel->name );
   }
   return;
}

/* Revised channel display by Zarius */
CMDF do_channels( CHAR_DATA * ch, char *argument )
{
   MUD_CHANNEL *channel;

   if( IS_NPC( ch ) )
   {
      send_to_char( "NPCs cannot list channels.\n\r", ch );
      return;
   }

   send_to_char( "&YThe following channels are available:\n\r", ch );
   send_to_char( "To toggle a channel, use the &Wlisten &Ycommand.\n\r\n\r", ch );

   send_to_char( "&WChannel        On/Off&D\n\r", ch );
   send_to_char( "&B-----------------------&D\n\r", ch );
   for( channel = first_channel; channel; channel = channel->next )
   {
      if( ch->level >= channel->level )
      {
         ch_printf( ch, "&w%-17s%s&D\n\r", capitalize( channel->name ),
                    ( hasname( ch->pcdata->chan_listen, channel->name ) ) ? "&GOn" : "&ROff" );
      }
   }
   send_to_char( "\n\r", ch );
   return;
}

void invert( char *arg1, char *arg2 )
{
   int i = 0;
   int len = strlen( arg1 ) - 1;

   while( i <= len )
   {
      *( arg2 + i ) = *( arg1 + ( len - i ) );
      i++;
   }
   *( arg2 + i ) = '\0';
}

/* Provided by Remcon to stop crashes with channel history */
char *add_percent( char *str )
{
   static char newstr[MSL];
   int i, j;

   for( i = j = 0; str[i] != '\0'; i++ )
   {
      if( str[i] == '%' )
         newstr[j++] = '%';
      newstr[j++] = str[i];
   }
   newstr[j] = '\0';
   return newstr;
}

/* Duplicate of to_channel from act_comm.c modified for dynamic channels */
void send_tochannel( CHAR_DATA * ch, MUD_CHANNEL * channel, char *argument )
{
   char buf[MSL], word[MIL];
   char *arg, *socbuf_char = NULL, *socbuf_vict = NULL, *socbuf_other = NULL;
   CHAR_DATA *victim = NULL;
   CHAR_DATA *vch;
   OBJ_DATA *obj; /* Burgundy Amulet */
   SOCIALTYPE *social = NULL;
   int position, x;
   bool emote = FALSE;
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }

   if( IS_NPC( ch ) && channel->type == CHAN_GUILD )
   {
      send_to_char( "Mobs can't be in clans/guilds.\n\r", ch );
      return;
   }

   if( !IS_PKILL( ch ) && channel->type == CHAN_PK )
   {
      if( !IS_IMMORTAL( ch ) )
      {
         send_to_char( "Peacefuls have no need to use wartalk.\n\r", ch );
         return;
      }
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SILENCE ) )
   {
      send_to_char( "The room absorbs your words!\n\r", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_SILENCE ) )
   {
      send_to_char( "You are unable to utter a sound!\n\r", ch );
      return;
   }

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      if( ch->master )
         send_to_char( "I don't think so...\n\r", ch->master );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      char *name;

      if( !IS_CHANFLAG( channel, CHAN_KEEPHISTORY ) )
      {
         ch_printf( ch, "%s what?\n\r", capitalize( channel->name ) );
         return;
      }

      ch_printf( ch, "&cThe last %d %s messages:\n\r", MAX_CHANHISTORY, channel->name );
      for( x = 0; x < MAX_CHANHISTORY; x++ )
      {
         if( channel->history[x][0] != NULL )
         {
            switch ( channel->hlevel[x] )
            {
               case 0:
                  name = channel->history[x][0];
                  break;
               case 1:
                  if( IS_AFFECTED( ch, AFF_DETECT_INVIS ) || IS_PLR_FLAG( ch, PLR_HOLYLIGHT ) )
                     name = channel->history[x][0];
                  else
                     name = "Someone";
                  break;
               case 2:
                  if( ch->level >= channel->hinvis[x] )
                     name = channel->history[x][0];
                  else
                     name = "Someone";
                  break;
               default:
                  name = "Someone";
            }
            ch_printf( ch, channel->history[x][1], mini_c_time( channel->htime[x], ch->pcdata->timezone ), name );
         }
         else
            break;
      }
      return;
   }

   if( IS_PLR_FLAG( ch, PLR_SILENCE ) )
   {
      ch_printf( ch, "You can't %s.\n\r", channel->name );
      return;
   }

   /*
    * Inverts the speech of anyone carrying the burgundy amulet 
    */
   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->pIndexData->vnum == 1405 ) /* The amulet itself */
      {
         char ibuf[MSL];

         invert( argument, ibuf );
         mudstrlcpy( argument, ibuf, MSL );
         break;
      }
   }

   arg = argument;
   arg = one_argument( arg, word );

   if( word[0] == '@' && ( social = find_social( word + 1 ) ) != NULL )
   {
      if( arg && *arg )
      {
         char name[MIL];

         one_argument( arg, name );

         if( ( victim = get_char_world( ch, name ) ) )
            arg = one_argument( arg, name );

         if( !victim )
         {
            socbuf_char = social->char_no_arg;
            socbuf_vict = social->others_no_arg;
            socbuf_other = social->others_no_arg;
            if( !socbuf_char && !socbuf_other )
               social = NULL;
         }
         else if( victim == ch )
         {
            socbuf_char = social->char_auto;
            socbuf_vict = social->others_auto;
            socbuf_other = social->others_auto;
            if( !socbuf_char && !socbuf_other )
               social = NULL;
         }
         else
         {
            socbuf_char = social->char_found;
            socbuf_vict = social->vict_found;
            socbuf_other = social->others_found;
            if( !socbuf_char && !socbuf_other && !socbuf_vict )
               social = NULL;
         }
      }
      else
      {
         socbuf_char = social->char_no_arg;
         socbuf_vict = social->others_no_arg;
         socbuf_other = social->others_no_arg;
         if( !socbuf_char && !socbuf_other )
            social = NULL;
      }
   }

   if( word[0] == ',' )
      emote = TRUE;

   if( social )
   {
      act_printf( AT_PLAIN, ch, argument, victim, TO_CHAR, "&W[&[%s]%s%s%s&W] &[%s]%s",
                  channel->colorname, mxp_chan_str( ch, channel->name ), capitalize( channel->name ), mxp_chan_str_close( ch,
                                                                                                                          channel->
                                                                                                                          name ),
                  channel->colorname, socbuf_char );
   }
   else if( emote )
   {
      ch_printf( ch, "&W[&[%s]%s%s%s&W] &[%s]%s %s\n\r",
                 channel->colorname, mxp_chan_str( ch, channel->name ), capitalize( channel->name ), mxp_chan_str_close( ch,
                                                                                                                         channel->
                                                                                                                         name ),
                 channel->colorname, ch->name, argument + 1 );
      argument = argument + 1;
   }
   else
      ch_printf( ch, "&[%s]You %s%s%s '%s'\n\r",
                 channel->colorname, mxp_chan_str( ch, channel->name ), channel->name, mxp_chan_str_close( ch,
                                                                                                           channel->name ),
                 argument );

   if( IS_ROOM_FLAG( ch->in_room, ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "%s: %s (%s)", IS_NPC( ch ) ? ch->short_descr : ch->name, argument, channel->name );

   /*
    * Channel history. Records the last MAX_CHANHISTORY messages to channels which keep historys 
    */
   if( IS_CHANFLAG( channel, CHAN_KEEPHISTORY ) )
   {
      for( x = 0; x < MAX_CHANHISTORY; x++ )
      {
         int type;

         type = 0;
         if( IS_AFFECTED( ch, AFF_INVISIBLE ) )
            type = 1;
         if( IS_PLR_FLAG( ch, PLR_WIZINVIS ) )
            type = 2;

         if( channel->history[x][0] == NULL )
         {
            if( IS_NPC( ch ) )
               channel->history[x][0] = str_dup( ch->short_descr );
            else
               channel->history[x][0] = str_dup( ch->name );

            argument = add_percent( argument );
            strdup_printf( &channel->history[x][1], "   &R[%%s] &G%%s%s %s\n\r", emote ? "" : ":", argument );
            channel->htime[x] = current_time;
            channel->hlevel[x] = type;
            if( type == 2 )
               channel->hinvis[x] = ch->pcdata->wizinvis;
            else
               channel->hinvis[x] = 0;
            break;
         }

         if( x == MAX_CHANHISTORY - 1 )
         {
            int y;

            for( y = 1; y < MAX_CHANHISTORY; y++ )
            {
               int z = y - 1;

               if( channel->history[z][0] != NULL )
               {
                  DISPOSE( channel->history[z][0] );
                  DISPOSE( channel->history[z][1] );
                  channel->history[z][0] = str_dup( channel->history[y][0] );
                  channel->history[z][1] = str_dup( channel->history[y][1] );
                  channel->hlevel[z] = channel->hlevel[y];
                  channel->hinvis[z] = channel->hinvis[y];
                  channel->htime[z] = channel->htime[y];
               }
            }
            DISPOSE( channel->history[x][0] );
            DISPOSE( channel->history[x][1] );
            if( IS_NPC( ch ) )
               channel->history[x][0] = str_dup( ch->short_descr );
            else
               channel->history[x][0] = str_dup( ch->name );

            argument = add_percent( argument );
            strdup_printf( &channel->history[x][1], "   &R[%%s] &G%%s%s %s\n\r", emote ? "" : ":", argument );
            channel->hlevel[x] = type;
            channel->htime[x] = current_time;
            if( type == 2 )
               channel->hinvis[x] = ch->pcdata->wizinvis;
            else
               channel->hinvis[x] = 0;
         }
      }
   }

   for( vch = first_char; vch; vch = vch->next )
   {
      /*
       * Hackish solution to stop that damned "someone chat" bug - Matarael 17.3.2002 
       */
      bool mapped = FALSE;
      int origmap = -1, origx = -1, origy = -1;

      if( IS_NPC( vch ) || vch == ch || !vch->desc )
         continue;

      if( vch->desc->connected == CON_PLAYING && hasname( vch->pcdata->chan_listen, channel->name ) )
      {
         char *sbuf = argument;
         char lbuf[MIL + 4];  /* invis level string + buf */

         if( vch->level < channel->level )
            continue;

         if( IS_ROOM_FLAG( vch->in_room, ROOM_SILENCE ) )
            continue;

         if( channel->type == CHAN_ZONE && vch->in_room->area != ch->in_room->area )
            continue;

         if( channel->type == CHAN_PK && !IS_PKILL( vch ) && !IS_IMMORTAL( vch ) )
            continue;

         if( channel->type == CHAN_GUILD )
         {
            if( IS_NPC( vch ) )
               continue;
            if( vch->pcdata->clan != ch->pcdata->clan )
               continue;
         }

         if( channel->type == CHAN_COUNCIL )
         {
            if( IS_NPC( vch ) )
               continue;
            if( vch->pcdata->council != ch->pcdata->council )
               continue;
         }

         position = vch->position;
         vch->position = POS_STANDING;

         if( IS_PLR_FLAG( ch, PLR_WIZINVIS ) && can_see( vch, ch, FALSE ) && IS_IMMORTAL( vch ) )
            snprintf( lbuf, MIL + 4, "%s(%d) ", channel->colorname,
                      ( !IS_NPC( ch ) ) ? ch->pcdata->wizinvis : ch->mobinvis );
         else
            lbuf[0] = '\0';

         if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
         {
            int speakswell = UMIN( knows_language( vch, ch->speaking, ch ), knows_language( ch, ch->speaking, vch ) );

            if( speakswell < 85 )
               sbuf = translate( speakswell, argument, lang_names[speaking] );
         }

         /*
          * Check to see if target is ignoring the sender 
          */
         if( is_ignoring( vch, ch ) )
         {
            /*
             * If the sender is an imm then they cannot be ignored 
             */
            if( !IS_IMMORTAL( ch ) || vch->level > ch->level )
            {
               /*
                * Off to oblivion! 
                */
               continue;
            }
         }

         MOBtrigger = FALSE;

         /*
          * Hackish solution to stop that damned "someone chat" bug - Matarael 17.3.2002 
          */
         if( IS_PLR_FLAG( ch, PLR_ONMAP ) )
         {
            mapped = TRUE;
            origx = ch->x;
            origy = ch->y;
            origmap = ch->map;
         }
         fix_maps( vch, ch );

         if( !social && !emote )
         {
            snprintf( buf, MSL, "&[%s]$n %s%s%ss '$t&[%s]'",
                      channel->colorname, mxp_chan_str( vch, channel->name ), channel->name,
                      mxp_chan_str_close( vch, channel->name ), channel->colorname );
            mudstrlcat( lbuf, buf, MIL + 4 );
            act( AT_PLAIN, lbuf, ch, sbuf, vch, TO_VICT );
         }
         if( emote )
         {
            snprintf( buf, MSL, "&W[&[%s]%s%s%s&W] &[%s]$n $t",
                      channel->colorname, mxp_chan_str( vch, channel->name ), capitalize( channel->name ),
                      mxp_chan_str_close( vch, channel->name ), channel->colorname );
            mudstrlcat( lbuf, buf, MIL + 4 );
            act( AT_PLAIN, lbuf, ch, sbuf, vch, TO_VICT );
         }
         if( social )
         {
            if( vch == victim )
            {
               act_printf( AT_PLAIN, ch, NULL, vch, TO_VICT, "&W[&[%s]%s%s%s&W] &[%s]%s",
                           channel->colorname, mxp_chan_str( vch, channel->name ), capitalize( channel->name ),
                           mxp_chan_str_close( vch, channel->name ), channel->colorname, socbuf_vict );
            }
            else
            {
               act_printf( AT_PLAIN, ch, vch, victim, TO_THIRD, "&W[&[%s]%s%s%s&W] &[%s]%s", channel->colorname,
                           mxp_chan_str( vch, channel->name ), capitalize( channel->name ), mxp_chan_str_close( vch,
                                                                                                                channel->
                                                                                                                name ),
                           channel->colorname, socbuf_other );
            }
         }
         vch->position = position;
         /*
          * Hackish solution to stop that damned "someone chat" bug - Matarael 17.3.2002 
          */
         if( mapped )
         {
            ch->map = origmap;
            ch->x = origx;
            ch->y = origy;
            SET_PLR_FLAG( ch, PLR_ONMAP );
         }
         else
         {
            REMOVE_PLR_FLAG( ch, PLR_ONMAP );
            ch->map = -1;
            ch->x = -1;
            ch->y = -1;
         }
      }
   }
   return;
}

void to_channel( const char *argument, char *xchannel, int level )
{
   MUD_CHANNEL *channel;
   char buf[MSL];
   DESCRIPTOR_DATA *d;

   if( !first_descriptor || !argument || argument[0] == '\0' )
      return;

   if( !( channel = find_channel( xchannel ) ) )
      return;

   if( channel->type != CHAN_LOG )
      return;

   snprintf( buf, MSL, "%s: %s\n\r", capitalize( channel->name ), argument );

   for( d = first_descriptor; d; d = d->next )
   {
      CHAR_DATA *vch;

      vch = d->original ? d->original : d->character;

      if( !vch )
         continue;

      if( d->original )
         continue;

      /*
       * This could be coming in higher than the normal level, so check first 
       */
      if( vch->level < level )
         continue;

      if( d->connected == CON_PLAYING && vch->level >= channel->level && hasname( vch->pcdata->chan_listen, channel->name ) )
      {
         set_char_color( AT_LOG, vch );
         send_to_char( buf, vch );
      }
   }
   return;
}

bool local_channel_hook( CHAR_DATA * ch, char *command, char *argument )
{
   MUD_CHANNEL *channel;

   if( !( channel = find_channel( command ) ) )
      return FALSE;

   if( ch->level < channel->level )
      return FALSE;

   /*
    * Logs are meant to be seen, not talked on 
    */
   if( channel->type == CHAN_LOG )
      return FALSE;

   if( !IS_NPC( ch ) && !hasname( ch->pcdata->chan_listen, channel->name ) )
   {
      ch_printf( ch, "&RYou are not listening to the &G%s &Rchannel.\n\r", channel->name );
      return TRUE;
   }

#if !defined(__CYGWIN__)
#ifdef MULTIPORT
   if( IS_CHANFLAG( channel, CHAN_INTERPORT ) )
      mud_message( ch, channel, argument );
#endif
#endif
   send_tochannel( ch, channel, argument );
   return TRUE;
}
