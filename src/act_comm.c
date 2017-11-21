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
 *                      Player communication module                         *
 ****************************************************************************/

#include <string.h>
#include <ctype.h>
#include "mud.h"
#include "boards.h"
#include "language.h"

void invert( char *arg1, char *arg2 ); /* For do_say */
char *mxp_chan_str( CHAR_DATA * ch, const char *verb );
char *mxp_chan_str_close( CHAR_DATA * ch, const char *verb );
void mprog_targetted_speech_trigger( char *txt, CHAR_DATA * actor, CHAR_DATA * victim );
void mprog_speech_trigger( char *txt, CHAR_DATA * mob );
void mprog_and_speech_trigger( char *txt, CHAR_DATA * mob );
BOARD_DATA *find_board( CHAR_DATA * ch );
BOARD_DATA *get_board( CHAR_DATA * ch, const char *argument );
char *mini_c_time( time_t curtime, int tz );

LANG_DATA *get_lang( const char *name )
{
   LANG_DATA *lng;

   for( lng = first_lang; lng; lng = lng->next )
      if( !str_cmp( lng->name, name ) )
         return lng;
   return NULL;
}

/* percent = percent knowing the language. */
char *translate( int percent, const char *in, const char *name )
{
   LCNV_DATA *cnv;
   static char buf[MSL];
   char buf2[MSL];
   const char *pbuf;
   char *pbuf2 = buf2;
   LANG_DATA *lng;

   if( percent > 99 || !str_cmp( name, "common" ) )
   {
      mudstrlcpy( buf, in, MSL );
      return buf;
   }

   /*
    * If we don't know this language... use "default" 
    */
   if( !( lng = get_lang( name ) ) )
   {
      if( !( lng = get_lang( "default" ) ) )
      {
         mudstrlcpy( buf, in, MSL );
         return buf;
      }
   }

   for( pbuf = in; *pbuf; )
   {
      for( cnv = lng->first_precnv; cnv; cnv = cnv->next )
      {
         if( !str_prefix( cnv->old, pbuf ) )
         {
            if( percent && ( rand(  ) % 100 ) < percent )
            {
               strncpy( pbuf2, pbuf, cnv->olen );
               pbuf2[cnv->olen] = '\0';
               pbuf2 += cnv->olen;
            }
            else
            {
               mudstrlcpy( pbuf2, cnv->lnew, MSL - ( pbuf2 - buf2 ) );
               pbuf2 += cnv->nlen;
            }
            pbuf += cnv->olen;
            break;
         }
      }
      if( !cnv )
      {
         if( isalpha( *pbuf ) && ( !percent || ( rand(  ) % 100 ) > percent ) )
         {
            *pbuf2 = lng->alphabet[LOWER( *pbuf ) - 'a'];
            if( isupper( *pbuf ) )
               *pbuf2 = UPPER( *pbuf2 );
         }
         else
            *pbuf2 = *pbuf;
         pbuf++;
         pbuf2++;
      }
   }
   *pbuf2 = '\0';
   for( pbuf = buf2, pbuf2 = buf; *pbuf; )
   {
      for( cnv = lng->first_cnv; cnv; cnv = cnv->next )
         if( !str_prefix( cnv->old, pbuf ) )
         {
            mudstrlcpy( pbuf2, cnv->lnew, MSL - ( pbuf2 - buf2 ) );
            pbuf += cnv->olen;
            pbuf2 += cnv->nlen;
            break;
         }
      if( !cnv )
         *( pbuf2++ ) = *( pbuf++ );
   }
   *pbuf2 = '\0';
   return buf;
}

char *drunk_speech( const char *argument, CHAR_DATA * ch )
{
   const char *arg = argument;
   static char buf[MIL * 2];
   char buf1[MIL * 2];
   short drunk;
   char *txt;
   char *txt1;

   if( IS_NPC( ch ) || !ch->pcdata )
   {
      mudstrlcpy( buf, argument, MIL * 2 );
      return buf;
   }

   drunk = ch->pcdata->condition[COND_DRUNK];

   if( drunk <= 0 )
   {
      mudstrlcpy( buf, argument, MIL * 2 );
      return buf;
   }

   buf[0] = '\0';
   buf1[0] = '\0';

   if( !argument )
   {
      bug( "%s", "Drunk_speech: NULL argument" );
      return "";
   }

   txt = buf;
   txt1 = buf1;

   while( *arg != '\0' )
   {
      if( toupper( *arg ) == 'T' )
      {
         if( number_percent(  ) < ( drunk * 2 ) )  /* add 'h' after an 'T' */
         {
            *txt++ = *arg;
            *txt++ = 'h';
         }
         else
            *txt++ = *arg;
      }
      else if( toupper( *arg ) == 'X' )
      {
         if( number_percent(  ) < ( drunk * 2 / 2 ) )
            *txt++ = 'c', *txt++ = 's', *txt++ = 'h';
         else
            *txt++ = *arg;
      }
      else if( number_percent(  ) < ( drunk * 2 / 5 ) )  /* slurred letters */
      {
         short slurn = number_range( 1, 2 );
         short currslur = 0;

         while( currslur < slurn )
            *txt++ = *arg, currslur++;
      }
      else
         *txt++ = *arg;

      arg++;
   }

   *txt = '\0';

   txt = buf;

   while( *txt != '\0' )   /* Let's mess with the string's caps */
   {
      if( number_percent(  ) < ( 2 * drunk / 2.5 ) )
      {
         if( isupper( *txt ) )
            *txt1 = tolower( *txt );
         else if( islower( *txt ) )
            *txt1 = toupper( *txt );
         else
            *txt1 = *txt;
      }
      else
         *txt1 = *txt;

      txt1++, txt++;
   }

   *txt1 = '\0';
   txt1 = buf1;
   txt = buf;

   while( *txt1 != '\0' )  /* Let's make them stutter */
   {
      if( *txt1 == ' ' )   /* If there's a space, then there's gotta be a */
      {  /* along there somewhere soon */

         while( *txt1 == ' ' )   /* Don't stutter on spaces */
            *txt++ = *txt1++;

         if( ( number_percent(  ) < ( 2 * drunk / 4 ) ) && *txt1 != '\0' )
         {
            short offset = number_range( 0, 2 );
            short pos = 0;

            while( *txt1 != '\0' && pos < offset )
               *txt++ = *txt1++, pos++;

            if( *txt1 == ' ' )   /* Make sure not to stutter a space after */
            {  /* the initial offset into the word */
               *txt++ = *txt1++;
               continue;
            }

            pos = 0;
            offset = number_range( 2, 4 );
            while( *txt1 != '\0' && pos < offset )
            {
               *txt++ = *txt1;
               pos++;
               if( *txt1 == ' ' || pos == offset ) /* Make sure we don't stick */
               {  /* A hyphen right before a space */
                  txt1--;
                  break;
               }
               *txt++ = '-';
            }
            if( *txt1 != '\0' )
               txt1++;
         }
      }
      else
         *txt++ = *txt1++;
   }

   *txt = '\0';

   return buf;
}

/*
 * Written by Kratas (moon@deathmoon.com)
 * Modified by Samson to be used in place of the ASK channel.
 */
CMDF do_ask( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   EXT_BV actflags;
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "Ask who what?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL || ( IS_NPC( victim ) && victim->in_room != ch->in_room ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SILENCE ) )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You ask yourself the question. Did it help?\r\n", ch );
      return;
   }

   actflags = ch->act;
   if( IS_NPC( ch ) )
      REMOVE_ACT_FLAG( ch, ACT_SECRETIVE );

   char *sbuf = argument;

   /*
    * Check to see if a player on a map is at the same coords as the recipient 
    * don't need to verify the PLR_ONMAP flags here, it's a room occupants check 
    */
   if( !is_same_map( ch, victim ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   /*
    * Check to see if character is ignoring speaker 
    */
   if( is_ignoring( victim, ch ) )
   {
      if( !IS_IMMORTAL( ch ) || get_trust( victim ) > get_trust( ch ) )
         return;
      else
         ch_printf( victim, "&[ignore]You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
   }

   if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
   {
      int speakswell = UMIN( knows_language( victim, ch->speaking, ch ), knows_language( ch, ch->speaking, victim ) );
      if( speakswell < 75 )
         sbuf = translate( speakswell, argument, lang_names[speaking] );
   }
   sbuf = drunk_speech( sbuf, ch );

   ch->act = actflags;
   MOBtrigger = FALSE;

   act( AT_SAY, "You ask $N '$t'", ch, drunk_speech( argument, ch ), victim, TO_CHAR );
   act( AT_SAY, "$n asks you '$t'", ch, drunk_speech( argument, ch ), victim, TO_VICT );

   if( IS_ROOM_FLAG( ch->in_room, ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "%s: %s", IS_NPC( ch ) ? ch->short_descr : ch->name, argument );

   mprog_targetted_speech_trigger( argument, ch, victim );
   return;
}

void update_sayhistory( CHAR_DATA * vch, CHAR_DATA * ch, const char *msg )
{
   char new_msg[MSL];
   int x;

   snprintf( new_msg, MSL, "%ld %s%s said '%s'", current_time,
             ( vch == ch ? color_str( AT_SAY, ch ) : color_str( AT_SAY, vch ) ),
             vch == ch ? "You" : PERS( ch, vch, FALSE ), msg );

   for( x = 0; x < MAX_SAYHISTORY; x++ )
   {
      if( vch->pcdata->say_history[x] == '\0' )
      {
         vch->pcdata->say_history[x] = str_dup( new_msg );
         break;
      }

      if( x == MAX_SAYHISTORY - 1 )
      {
         int i;

         for( i = 1; i < MAX_SAYHISTORY; i++ )
         {
            DISPOSE( vch->pcdata->say_history[i - 1] );
            vch->pcdata->say_history[i - 1] = str_dup( vch->pcdata->say_history[i] );
         }
         DISPOSE( vch->pcdata->say_history[x] );
         vch->pcdata->say_history[x] = str_dup( new_msg );
      }
   }
   return;
}

CMDF do_say( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *vch;
   OBJ_DATA *obj;
   EXT_BV actflags;
   int speaking = -1, lang, x;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }

   if( !argument || argument[0] == '\0' )
   {
      if( IS_NPC( ch ) )
      {
         send_to_char( "Say what?", ch );
         return;
      }

      ch_printf( ch, "&cThe last %d things you heard said:\n\r", MAX_SAYHISTORY );

      for( x = 0; x < MAX_SAYHISTORY; x++ )
      {
         char histbuf[MSL];
         if( ch->pcdata->say_history[x] == NULL )
            break;
         one_argument( ch->pcdata->say_history[x], histbuf );
         ch_printf( ch, "&R[%s]%s\n\r", mini_c_time( atoi( histbuf ), ch->pcdata->timezone ),
                    ch->pcdata->say_history[x] + strlen( histbuf ) );
      }
      return;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SILENCE ) )
   {
      send_to_char( "You can't do that here.\n\r", ch );
      return;
   }

   actflags = ch->act;
   if( IS_NPC( ch ) )
      REMOVE_PLR_FLAG( ch, ACT_SECRETIVE );

   /*
    * Inverts the speech of anyone carrying the burgundy amulet 
    */
   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->pIndexData->vnum == 1405 ) /* The amulet itself */
      {
         char buf[MSL];

         invert( argument, buf );
         mudstrlcpy( argument, buf, MSL );
      }
   }

   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
   {
      char *sbuf = argument;

      if( vch == ch )
         continue;

      /*
       * Check to see if a player on a map is at the same coords as the recipient 
       * don't need to verify the PLR_ONMAP flags here, it's a room occupants check 
       */
      if( !is_same_map( ch, vch ) )
         continue;

      /*
       * Check to see if character is ignoring speaker 
       */
      if( is_ignoring( vch, ch ) )
      {
         /*
          * continue unless speaker is an immortal 
          */
         if( !IS_IMMORTAL( ch ) || vch->level > ch->level )
            continue;
         else
            ch_printf( vch, "&[ignore]You attempt to ignore %s, but are unable to do so.\n\r", ch->name );
      }
      if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
      {
         int speakswell = UMIN( knows_language( vch, ch->speaking, ch ), knows_language( ch, ch->speaking, vch ) );

         if( speakswell < 75 )
            sbuf = translate( speakswell, argument, lang_names[speaking] );
      }
      sbuf = drunk_speech( sbuf, ch );
      MOBtrigger = FALSE;
      act_printf( AT_SAY, ch, sbuf, vch, TO_VICT,
                  "$n %s%s%ss '$t'", mxp_chan_str( vch, "say" ), "say", mxp_chan_str_close( vch, "say" ) );
      if( !IS_NPC( vch ) )
         update_sayhistory( vch, ch, sbuf );
   }
   ch->act = actflags;
   MOBtrigger = FALSE;

   act_printf( AT_SAY, ch, NULL, drunk_speech( argument, ch ), TO_CHAR,
               "You %s%s%s '$T'", mxp_chan_str( ch, "say" ), "say", mxp_chan_str_close( ch, "say" ) );
   if( !IS_NPC( ch ) )
      update_sayhistory( ch, ch, drunk_speech( argument, ch ) );

   if( IS_ROOM_FLAG( ch->in_room, ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "%s: %s", IS_NPC( ch ) ? ch->short_descr : ch->name, argument );

   mprog_speech_trigger( argument, ch );
   mprog_and_speech_trigger( argument, ch );
   if( char_died( ch ) )
      return;
   oprog_speech_trigger( argument, ch );
   oprog_and_speech_trigger( argument, ch );
   if( char_died( ch ) )
      return;
   rprog_speech_trigger( argument, ch );
   rprog_and_speech_trigger( argument, ch );
   return;
}

CMDF do_whisper( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   int position;
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Whisper to whom what?\n\r", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( ch == victim )
   {
      send_to_char( "You have a nice little chat with yourself.\n\r", ch );
      return;
   }

   if( !IS_NPC( victim ) && ( victim->switched ) && !IS_AFFECTED( victim->switched, AFF_POSSESS ) )
   {
      send_to_char( "That player is switched.\n\r", ch );
      return;
   }
   else if( !IS_NPC( victim ) && ( !victim->desc ) )
   {
      send_to_char( "That player is link-dead.\n\r", ch );
      return;
   }
   if( IS_PLR_FLAG( victim, PLR_AFK ) )
   {
      send_to_char( "That player is afk.\n\r", ch );
      return;
   }
   if( IS_PLR_FLAG( victim, PLR_SILENCE ) )
      send_to_char( "That player is silenced.  They will receive your message but can not respond.\n\r", ch );

   if( IS_AFFECTED( victim, AFF_SILENCE ) )
      send_to_char( "That player has been magically muted!\n\r", ch );

   if( victim->desc && victim->desc->connected == CON_EDITING && get_trust( ch ) < LEVEL_GOD )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer.  Please try again in a few minutes.", ch, 0, victim, TO_CHAR );
      return;
   }

   /*
    * Check to see if target of tell is ignoring the sender 
    */
   if( is_ignoring( victim, ch ) )
   {
      /*
       * If the sender is an imm then they cannot be ignored 
       */
      if( !IS_IMMORTAL( ch ) || get_trust( victim ) > get_trust( ch ) )
         return;
      else
         ch_printf( victim, "&[ignore]You attempt to ignore %s, but are unable to do so.\n\r", ch->name );
   }

   MOBtrigger = FALSE;
   act( AT_WHISPER, "You whisper to $N '$t'", ch, argument, victim, TO_CHAR );
   position = victim->position;
   victim->position = POS_STANDING;
   if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
   {
      int speakswell = UMIN( knows_language( victim, ch->speaking, ch ), knows_language( ch, ch->speaking, victim ) );

      if( speakswell < 85 )
         act( AT_WHISPER, "$n whispers to you '$t'", ch,
              translate( speakswell, argument, lang_names[speaking] ), victim, TO_VICT );
      else
         act( AT_WHISPER, "$n whispers to you '$t'", ch, argument, victim, TO_VICT );
   }
   else
      act( AT_WHISPER, "$n whispers to you '$t'", ch, argument, victim, TO_VICT );

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SILENCE ) )
      act( AT_WHISPER, "$n whispers something to $N.", ch, argument, victim, TO_NOTVICT );

   victim->position = position;
   if( IS_ROOM_FLAG( ch->in_room, ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "%s: %s (whisper to) %s.", IS_NPC( ch ) ? ch->short_descr : ch->name, argument,
                      IS_NPC( victim ) ? victim->short_descr : victim->name );

   if( IS_NPC( victim ) )
   {
      mprog_speech_trigger( argument, ch );
      mprog_and_speech_trigger( argument, ch );
   }
   return;
}

/* Beep command courtesy of Altrag */
/* Installed by Samson on unknown date, allows user to beep other users */
CMDF do_beep( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( ch->pcdata->release_date != 0 )
   {
      send_to_char( "Nope, no beeping from hell.\n\r", ch );
      return;
   }

   if( !argument || !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "Beep who?\n\r", ch );
      return;
   }

   /*
    * NPC check added by Samson 2-15-98 
    */
   if( IS_NPC( victim ) || is_ignoring( victim, ch ) )
   {
      send_to_char( "Beep who?\n\r", ch );
      return;
   }

   /*
    * PCFLAG_NOBEEP check added by Samson 2-15-98 
    */
   if( IS_PCFLAG( victim, PCFLAG_NOBEEP ) )
   {
      ch_printf( ch, "%s is not accepting beeps at this time.\n\r", victim->name );
      return;
   }

   ch_printf( victim, "%s is beeping you!\a\n\r", PERS( ch, victim, TRUE ) );
   ch_printf( ch, "You beep %s.\n\r", PERS( victim, ch, TRUE ) );
   return;
}

void update_tellhistory( CHAR_DATA * vch, CHAR_DATA * ch, const char *msg, bool self )
{
   CHAR_DATA *tch;
   char new_msg[MSL];
   int x;

   if( self )
   {
      snprintf( new_msg, MSL, "%ld %sYou told %s '%s'",
                current_time, color_str( AT_TELL, ch ), PERS( vch, ch, FALSE ), msg );
      tch = ch;
   }
   else
   {
      snprintf( new_msg, MSL, "%ld %s%s told you '%s'",
                current_time, color_str( AT_TELL, vch ), PERS( ch, vch, FALSE ), msg );
      tch = vch;
   }

   if( IS_NPC( tch ) )
      return;

   for( x = 0; x < MAX_TELLHISTORY; x++ )
   {
      if( !tch->pcdata->tell_history[x] || tch->pcdata->tell_history[x] == '\0' )
      {
         tch->pcdata->tell_history[x] = str_dup( new_msg );
         break;
      }

      if( x == MAX_TELLHISTORY - 1 )
      {
         int i;

         for( i = 1; i < MAX_TELLHISTORY; i++ )
         {
            DISPOSE( tch->pcdata->tell_history[i - 1] );
            tch->pcdata->tell_history[i - 1] = str_dup( tch->pcdata->tell_history[i] );
         }
         DISPOSE( tch->pcdata->tell_history[x] );
         tch->pcdata->tell_history[x] = str_dup( new_msg );
      }
   }
   return;
}

CMDF do_tell( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   int position;
   CHAR_DATA *switched_victim = NULL;
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SILENCE ) )
   {
      send_to_char( "You can't do that here.\n\r", ch );
      return;
   }

   if( IS_PLR_FLAG( ch, PLR_SILENCE ) || IS_PLR_FLAG( ch, PLR_NO_TELL ) || IS_AFFECTED( ch, AFF_SILENCE ) )
   {
      send_to_char( "You can't do that.\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' || argument[0] == '\0' )
   {
      int x;

      if( IS_NPC( ch ) )
      {
         send_to_char( "Tell who what?", ch );
         return;
      }

      ch_printf( ch, "&cThe last %d things you were told:\n\r", MAX_TELLHISTORY );

      for( x = 0; x < MAX_TELLHISTORY; x++ )
      {
         char histbuf[MSL];
         if( ch->pcdata->tell_history[x] == NULL )
            break;
         one_argument( ch->pcdata->tell_history[x], histbuf );
         ch_printf( ch, "&R[%s]%s\n\r", mini_c_time( atoi( histbuf ), ch->pcdata->timezone ),
                    ch->pcdata->tell_history[x] + strlen( histbuf ) );
      }
      return;
   }

   if( !( victim = get_char_world( ch, arg ) ) || ( IS_NPC( victim ) && victim->in_room != ch->in_room ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( ch == victim )
   {
      send_to_char( "You have a nice little chat with yourself.\n\r", ch );
      return;
   }

   if( !IS_NPC( victim ) && ( victim->switched ) && ( get_trust( ch ) > LEVEL_AVATAR )
       && !IS_AFFECTED( victim->switched, AFF_POSSESS ) )
   {
      send_to_char( "That player is switched.\n\r", ch );
      return;
   }

   else if( !IS_NPC( victim ) && ( victim->switched ) && IS_AFFECTED( victim->switched, AFF_POSSESS ) )
      switched_victim = victim->switched;

   else if( !IS_NPC( victim ) && ( !victim->desc ) )
   {
      send_to_char( "That player is link-dead.\n\r", ch );
      return;
   }

   if( IS_PLR_FLAG( victim, PLR_AFK ) )
   {
      send_to_char( "That player is afk.\n\r", ch );
      return;
   }

   if( IS_PCFLAG( victim, PCFLAG_NOTELL ) && !IS_IMMORTAL( ch ) )
      /*
       * Immortal check added to let imms tell players at all times, Adjani, 12-02-2002
       */
   {
      act( AT_PLAIN, "$E has $S tells turned off.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( IS_PLR_FLAG( victim, PLR_SILENCE ) )
      send_to_char( "That player is silenced.  They will receive your message but can not respond.\n\r", ch );

   if( !IS_NPC( victim ) && IS_AFFECTED( victim, AFF_SILENCE ) )
      send_to_char( "That player has been magically muted!\n\r", ch );

   if( ( !IS_IMMORTAL( ch ) && !IS_AWAKE( victim ) )
       || ( !IS_NPC( victim ) && IS_ROOM_FLAG( victim->in_room, ROOM_SILENCE ) ) )
   {
      act( AT_PLAIN, "$E can't hear you.", ch, 0, victim, TO_CHAR );
      return;
   }

   if( victim->desc  /* make sure desc exists first  -Thoric */
       && victim->desc->connected == CON_EDITING && get_trust( ch ) < LEVEL_GOD )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer.  Please try again in a few minutes.", ch, 0, victim, TO_CHAR );
      return;
   }

   /*
    * Check to see if target of tell is ignoring the sender 
    */
   if( is_ignoring( victim, ch ) )
   {
      /*
       * If the sender is an imm then they cannot be ignored 
       */
      if( !IS_IMMORTAL( ch ) || victim->level > ch->level )
      {
         /*
          * Drop the command into oblivion, why tell the other guy you're ignoring them? 
          */
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }
      else
         ch_printf( victim, "&[ignore]You attempt to ignore %s, but are unable to do so.\n\r", ch->name );
   }

   if( switched_victim )
      victim = switched_victim;

   MOBtrigger = FALSE;  /* BUGFIX - do_tell: Tells were triggering act progs */

   act( AT_TELL, "You tell $N '$t'", ch, argument, victim, TO_CHAR );
   update_tellhistory( victim, ch, argument, TRUE );
   position = victim->position;
   victim->position = POS_STANDING;
   if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
   {
      int speakswell = UMIN( knows_language( victim, ch->speaking, ch ), knows_language( ch, ch->speaking, victim ) );

      if( speakswell < 85 )
      {
         act( AT_TELL, "$n tells you '$t'", ch, translate( speakswell, argument, lang_names[speaking] ), victim, TO_VICT );
         update_tellhistory( victim, ch, translate( speakswell, argument, lang_names[speaking] ), FALSE );
      }
      else
      {
         act( AT_TELL, "$n tells you '$t'", ch, argument, victim, TO_VICT );
         update_tellhistory( victim, ch, argument, FALSE );
      }
   }
   else
   {
      act( AT_TELL, "$n tells you '$t'", ch, argument, victim, TO_VICT );
      update_tellhistory( victim, ch, argument, FALSE );
   }

   MOBtrigger = TRUE;   /* BUGFIX - do_tell: Tells were triggering act progs */

   victim->position = position;
   victim->reply = ch;

   if( IS_ROOM_FLAG( ch->in_room, ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "%s: %s (tell to) %s.", IS_NPC( ch ) ? ch->short_descr : ch->name,
                      argument, IS_NPC( victim ) ? victim->short_descr : victim->name );

   if( IS_NPC( victim ) )
   {
      mprog_speech_trigger( argument, ch );
      mprog_and_speech_trigger( argument, ch );
   }
   return;
}

CMDF do_reply( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( !( find_board( ch ) ) )
   {
      char arg[MIL]; /* Placed this here since it's only used here -- X */
      if( ( is_number( one_argument( argument, arg ) ) ) && ( get_board( ch, arg ) ) )
      {
         cmdf( ch, "write %s", argument );
         return;
      }
   }
   else if( is_number( argument ) && argument[0] != '\0' )
   {
      cmdf( ch, "write %s", argument );
      return;
   }

   if( !( victim = ch->reply ) )
   {
      send_to_char( "Either you have nothing to reply to, or that person has left.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      ch_printf( ch, "And what would you like to say in reply to %s?\n\r", victim->name );
      return;
   }

   /*
    * This is a bit shorter than what was here before, no? Accomplished the same bloody thing too. -- Xorith 
    */
   cmdf( ch, "tell %s %s", victim->name, argument );
   return;
}

CMDF do_emote( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   char *plast;
   CHAR_DATA *vch;
   EXT_BV actflags;
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }

   /*
    * Per Alcane's notice, emote no longer works in silent rooms - Samson 1-14-00 
    */
   if( IS_ROOM_FLAG( ch->in_room, ROOM_SILENCE ) )
   {
      send_to_char( "The room is magically silenced! You cannot express emotions!\n\r", ch );
      return;
   }

   if( IS_PLR_FLAG( ch, PLR_NO_EMOTE ) )
   {
      send_to_char( "You can't show your emotions.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Emote what?\n\r", ch );
      return;
   }

   actflags = ch->act;
   if( IS_NPC( ch ) )
      REMOVE_ACT_FLAG( ch, ACT_SECRETIVE );
   for( plast = argument; *plast != '\0'; plast++ )
      ;

   mudstrlcpy( buf, argument, MSL );
   if( isalpha( plast[-1] ) )
      mudstrlcat( buf, ".", MSL );
   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
   {
      char *sbuf = buf;

      /*
       * Check to see if a player on a map is at the same coords as the recipient 
       * don't need to verify the PLR_ONMAP flags here, it's a room occupants check 
       */
      if( !is_same_map( ch, vch ) )
         continue;

      /*
       * Check to see if character is ignoring emoter 
       */
      if( is_ignoring( vch, ch ) )
      {
         /*
          * continue unless emoter is an immortal 
          */
         if( !IS_IMMORTAL( ch ) || vch->level > ch->level )
            continue;
         else
            ch_printf( vch, "&[ignore]You attempt to ignore %s, but are unable to do so.\n\r", ch->name );
      }
      if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
      {
         int speakswell = UMIN( knows_language( vch, ch->speaking, ch ), knows_language( ch, ch->speaking, vch ) );

         if( speakswell < 85 )
            sbuf = translate( speakswell, argument, lang_names[speaking] );
      }
      MOBtrigger = FALSE;
      act( AT_SOCIAL, "$n $t", ch, sbuf, vch, ( vch == ch ? TO_CHAR : TO_VICT ) );
   }
   ch->act = actflags;
   if( IS_ROOM_FLAG( ch->in_room, ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "%s %s (emote)", IS_NPC( ch ) ? ch->short_descr : ch->name, argument );
   return;
}

/* 0 = bug 1 = idea 2 = typo */
void tybuid( CHAR_DATA * ch, char *argument, int type )
{
   struct tm *t = localtime( &current_time );
   static char *const tybuid_name[] = { "bug", "idea", "typo" };
   static char *const tybuid_file[] = { PBUG_FILE, IDEA_FILE, TYPO_FILE };

   set_char_color( AT_PLAIN, ch );

   if( !argument || argument[0] == '\0' )
   {
      if( type == 1 )
         send_to_char( "\n\rUsage:  'idea <message>'\n\r", ch );
      else
         ch_printf( ch, "Usage:  '%s <message>'  (your location is automatically recorded)\n\r", tybuid_name[type] );
      if( get_trust( ch ) >= LEVEL_ASCENDANT )
         ch_printf( ch, "  '%s list' or '%s clear now'\n\r", tybuid_name[type], tybuid_name[type] );
      return;
   }

   if( !str_cmp( argument, "clear now" ) && get_trust( ch ) >= LEVEL_ASCENDANT )
   {
      FILE *fp;
      if( !( fp = fopen( tybuid_file[type], "w" ) ) )
      {
         bug( "%s: unable to stat %s file '%s'!", __FUNCTION__, tybuid_name[type], tybuid_file[type] );
         return;
      }
      FCLOSE( fp );
      ch_printf( ch, "The %s file has been cleared.\n\r", tybuid_name[type] );
      return;
   }

   if( !str_cmp( argument, "list" ) && get_trust( ch ) >= LEVEL_ASCENDANT )
   {
      show_file( ch, tybuid_file[type] );
      return;
   }

   append_file( ch, tybuid_file[type], "(%-2.2d/%-2.2d):  %s", t->tm_mon + 1, t->tm_mday, argument );
   ch_printf( ch, "Thank you! Your %s has been recorded.\n\r", tybuid_name[type] );
   return;
}

CMDF do_bug( CHAR_DATA * ch, char *argument )
{
   /*
    * 0 = bug 
    */
   tybuid( ch, argument, 0 );
   return;
}

CMDF do_ide( CHAR_DATA * ch, char *argument )
{
   send_to_char( "&YIf you want to send an idea, type 'idea <message>'.\n\r", ch );
   send_to_char( "If you want to identify an object, use the identify spell.\n\r", ch );
   return;
}

CMDF do_idea( CHAR_DATA * ch, char *argument )
{
   /*
    * 1 = idea 
    */
   tybuid( ch, argument, 1 );
   return;
}

CMDF do_typo( CHAR_DATA * ch, char *argument )
{
   /*
    * 2 = typo 
    */
   tybuid( ch, argument, 2 );
   return;
}

/*
 * Something from original DikuMUD that Merc yanked out.
 * Used to prevent following loops, which can cause problems if people
 * follow in a loop through an exit leading back into the same room
 * (Which exists in many maze areas)			-Thoric
 */
bool circle_follow( CHAR_DATA * ch, CHAR_DATA * victim )
{
   CHAR_DATA *tmp;

   for( tmp = victim; tmp; tmp = tmp->master )
      if( tmp == ch )
         return TRUE;
   return FALSE;
}

CMDF do_dismiss( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Dismiss whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, argument ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( ( IS_AFFECTED( victim, AFF_CHARM ) ) && ( IS_NPC( victim ) ) && ( victim->master == ch ) )
   {
      unbind_follower( victim, ch );
      stop_hating( victim );
      stop_hunting( victim );
      stop_fearing( victim );
      act( AT_ACTION, "$n dismisses $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_ACTION, "You dismiss $N.", ch, NULL, victim, TO_CHAR );
   }
   else
      send_to_char( "You cannot dismiss them.\n\r", ch );
   return;
}

void add_follower( CHAR_DATA * ch, CHAR_DATA * master )
{
   if( ch->master )
   {
      bug( "%s", "Add_follower: non-null master." );
      return;
   }

   ch->master = master;
   ch->leader = NULL;

   if( can_see( master, ch, FALSE ) )
      act( AT_ACTION, "$n now follows you.", ch, NULL, master, TO_VICT );
   act( AT_ACTION, "You now follow $N.", ch, NULL, master, TO_CHAR );

   return;
}

void stop_follower( CHAR_DATA * ch )
{
   if( !ch->master )
   {
      bug( "Stop_follower: %s has null master.", ch->name );
      return;
   }

   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      REMOVE_AFFECTED( ch, AFF_CHARM );
      affect_strip( ch, gsn_charm_person );
   }

   if( can_see( ch->master, ch, FALSE ) )
      if( !( !IS_NPC( ch->master ) && IS_IMMORTAL( ch ) && !IS_IMMORTAL( ch->master ) ) )
         act( AT_ACTION, "$n stops following you.", ch, NULL, ch->master, TO_VICT );
   act( AT_ACTION, "You stop following $N.", ch, NULL, ch->master, TO_CHAR );

   ch->master = NULL;
   ch->leader = NULL;
   return;
}

/* Function modified from original form by Samson 3-3-98 */
CMDF do_follow( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Follow whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, argument ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master )
   {
      act( AT_PLAIN, "But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR );
      return;
   }

   if( victim == ch )
   {
      if( !ch->master )
      {
         send_to_char( "You already follow yourself.\n\r", ch );
         return;
      }
      stop_follower( ch );
      return;
   }

   if( circle_follow( ch, victim ) )
   {
      send_to_char( "Following in loops is not allowed... sorry.\n\r", ch );
      return;
   }

   if( ch->master )
      stop_follower( ch );

   add_follower( ch, victim );
   return;
}

void die_follower( CHAR_DATA * ch )
{
   CHAR_DATA *fch;

   if( ch->master )
      unbind_follower( ch, ch->master );

   ch->leader = NULL;

   for( fch = first_char; fch; fch = fch->next )
   {
      if( fch->master == ch )
         unbind_follower( fch, ch );

      if( fch->leader == ch )
         fch->leader = fch;
   }
   return;
}

CMDF do_order( CHAR_DATA * ch, char *argument )
{
   char arg[MIL], argbuf[MIL];
   CHAR_DATA *victim, *och, *och_next;
   bool found, fAll;

   mudstrlcpy( argbuf, argument, MIL );
   argument = one_argument( argument, arg );

   if( arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Order whom to do what?\n\r", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You feel like taking, not giving, orders.\n\r", ch );
      return;
   }

   if( !str_cmp( arg, "all" ) )
   {
      fAll = TRUE;
      victim = NULL;
   }
   else
   {
      fAll = FALSE;
      if( ( victim = get_char_room( ch, arg ) ) == NULL )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }

      if( victim == ch )
      {
         send_to_char( "Aye aye, right away!\n\r", ch );
         return;
      }

      if( !IS_AFFECTED( victim, AFF_CHARM ) || victim->master != ch )
      {
         do_say( victim, "Do it yourself!" );
         return;
      }
   }

   found = FALSE;
   for( och = ch->in_room->first_person; och; och = och_next )
   {
      och_next = och->next_in_room;

      if( IS_AFFECTED( och, AFF_CHARM ) && och->master == ch && ( fAll || och == victim ) )
      {
         found = TRUE;
         act( AT_ACTION, "$n orders you to '$t'.", ch, argument, och, TO_VICT );
         interpret( och, argument );
      }
   }

   if( found )
   {
      send_to_char( "Ok.\n\r", ch );
      WAIT_STATE( ch, 12 );
   }
   else
      send_to_char( "You have no followers here.\n\r", ch );
   return;
}

/*
 * 'Split' originally by Gnort, God of Chaos.
 */
CMDF do_split( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *gch;
   int members, amount, share, extra;

   if( !argument || argument[0] == '\0' || !is_number( argument ) )
   {
      send_to_char( "Split how much?\n\r", ch );
      return;
   }

   amount = atoi( argument );

   if( amount < 0 )
   {
      send_to_char( "Your group wouldn't like that.\n\r", ch );
      return;
   }

   if( amount == 0 )
   {
      send_to_char( "You hand out zero coins, but no one notices.\n\r", ch );
      return;
   }

   if( ch->gold < amount )
   {
      send_to_char( "You don't have that much gold.\n\r", ch );
      return;
   }

   members = 0;
   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( is_same_group( gch, ch ) )
         members++;
   }

   if( IS_PLR_FLAG( ch, PLR_AUTOGOLD ) && members < 2 )
      return;

   if( members < 2 )
   {
      send_to_char( "Just keep it all.\n\r", ch );
      return;
   }

   share = amount / members;
   extra = amount % members;

   if( share == 0 )
   {
      send_to_char( "Don't even bother, cheapskate.\n\r", ch );
      return;
   }

   ch->gold -= amount;
   ch->gold += share + extra;

   ch_printf( ch, "&[gold]You split %d gold coins. Your share is %d gold coins.\n\r", amount, share + extra );

   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( gch != ch && is_same_group( gch, ch ) )
      {
         act_printf( AT_GOLD, ch, NULL, gch, TO_VICT, "$n splits %d gold coins. Your share is %d gold coins.", amount,
                     share );
         gch->gold += share;
      }
   }
   return;
}

CMDF do_gtell( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *gch;
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Tell your group what?\n\r", ch );
      return;
   }

   if( IS_PLR_FLAG( ch, PLR_NO_TELL ) )
   {
      send_to_char( "Your message didn't get through!\n\r", ch );
      return;
   }

   /*
    * Note use of send_to_char, so gtell works on sleepers.
    */
   for( gch = first_char; gch; gch = gch->next )
   {
      if( is_same_group( gch, ch ) )
      {
         set_char_color( AT_GTELL, gch );
         if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
         {
            int speakswell = UMIN( knows_language( gch, ch->speaking, ch ), knows_language( ch, ch->speaking, gch ) );

            if( speakswell < 85 )
            {
               if( gch == ch )
                  ch_printf( gch, "You tell the group '%s'\n\r", translate( speakswell, argument, lang_names[speaking] ) );
               else
                  ch_printf( gch, "%s tells the group '%s'\n\r", ch->name,
                             translate( speakswell, argument, lang_names[speaking] ) );
            }
            else
            {
               if( gch == ch )
                  ch_printf( gch, "You tell the group '%s'\n\r", argument );
               else
                  ch_printf( gch, "%s tells the group '%s'\n\r", ch->name, argument );
            }
         }
         else
         {
            if( gch == ch )
               ch_printf( gch, "You tell the group '%s'\n\r", argument );
            else
               ch_printf( gch, "%s tells the group '%s'\n\r", ch->name, argument );
         }
      }
   }
   return;
}

/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( CHAR_DATA * ach, CHAR_DATA * bch )
{
   if( ach->leader )
      ach = ach->leader;
   if( bch->leader )
      bch = bch->leader;
   return ach == bch;
}

/*
 * Language support functions. -- Altrag
 * 07/01/96
 *
 * Modified to return how well the language is known 04/04/98 - Thoric
 * Currently returns 100% for known languages... but should really return
 * a number based on player's wisdom (maybe 50+((25-wisdom)*2) ?)
 */
int knows_language( CHAR_DATA * ch, int language, CHAR_DATA * cch )
{
   short sn;

   if( !IS_NPC( ch ) && IS_IMMORTAL( ch ) )
      return 100;

   if( IS_NPC( ch ) && !ch->speaks )   /* No langs = knows all for npcs */
      return 100;

   if( IS_NPC( ch ) && IS_SET( ch->speaks, ( language & ~LANG_CLAN ) ) )
      return 100;

   /*
    * everyone KNOWS common tongue 
    */
   if( IS_SET( language, LANG_COMMON ) )
      return 100;

   if( language & LANG_CLAN )
   {
      /*
       * Clan = common for mobs.. snicker.. -- Altrag 
       */
      if( IS_NPC( ch ) || IS_NPC( cch ) )
         return 100;

      if( ch->pcdata->clan == cch->pcdata->clan && ch->pcdata->clan != NULL )
         return 100;
   }

   if( !IS_NPC( ch ) )
   {
      int lang;

      /*
       * Racial languages for PCs 
       */
      if( IS_SET( race_table[ch->race]->language, language ) )
         return 100;

      for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
         if( IS_SET( language, lang_array[lang] ) && IS_SET( ch->speaks, lang_array[lang] ) )
         {
            if( ( sn = skill_lookup( lang_names[lang] ) ) != -1 )
               return ch->pcdata->learned[sn];
         }
   }
   return 0;
}

int const lang_array[] = {
   LANG_COMMON, LANG_ELVEN, LANG_DWARVEN, LANG_PIXIE,
   LANG_OGRE, LANG_ORCISH, LANG_TROLLISH, LANG_RODENT,
   LANG_INSECTOID, LANG_MAMMAL, LANG_REPTILE,
   LANG_DRAGON, LANG_SPIRITUAL, LANG_MAGICAL,
   LANG_GOBLIN, LANG_GOD, LANG_ANCIENT, LANG_HALFLING,
   LANG_CLAN, LANG_GITH, LANG_MINOTAUR, LANG_CENTAUR, LANG_GNOME, LANG_SAHUAGIN, LANG_UNKNOWN
};

char *const lang_names[] = {
   "common", "elvish", "dwarven", "pixie", "ogre",
   "orcish", "trollese", "rodent", "insectoid",
   "mammal", "reptile", "dragon", "spiritual",
   "magical", "goblin", "god", "ancient",
   "halfling", "clan", "gith", "minotaur", "centaur", "gnomish", "sahuagin", ""
};

CMDF do_speak( CHAR_DATA * ch, char *argument )
{
   int langs;

   if( !str_cmp( argument, "all" ) && IS_IMMORTAL( ch ) )
   {
      ch->speaking = ~LANG_CLAN;
      send_to_char( "&[say]Now speaking all languages.\n\r", ch );
      return;
   }
   for( langs = 0; lang_array[langs] != LANG_UNKNOWN; langs++ )
      if( !str_prefix( argument, lang_names[langs] ) )
         if( knows_language( ch, lang_array[langs], ch ) )
         {
            if( lang_array[langs] == LANG_CLAN && ( IS_NPC( ch ) || !ch->pcdata->clan ) )
               continue;
            ch->speaking = lang_array[langs];
            ch_printf( ch, "&[say]You now speak %s.\n\r", lang_names[langs] );
            return;
         }
   send_to_char( "&[say]You do not know that language.\n\r", ch );
}

CMDF do_languages( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   int lang;

   argument = one_argument( argument, arg );
   if( arg[0] != '\0' && !str_prefix( arg, "learn" ) && !IS_IMMORTAL( ch ) && !IS_NPC( ch ) )
   {
      CHAR_DATA *sch;
      int sn, prct, prac;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Learn which language?\n\r", ch );
         return;
      }
      for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      {
         if( lang_array[lang] == LANG_CLAN )
            continue;
         if( !str_prefix( argument, lang_names[lang] ) )
            break;
      }
      if( lang_array[lang] == LANG_UNKNOWN )
      {
         send_to_char( "That is not a language.\n\r", ch );
         return;
      }
      if( !( VALID_LANGS & lang_array[lang] ) )
      {
         send_to_char( "You may not learn that language.\n\r", ch );
         return;
      }
      if( ( sn = skill_lookup( lang_names[lang] ) ) < 0 )
      {
         send_to_char( "That is not a language.\n\r", ch );
         return;
      }
      if( race_table[ch->race]->language & lang_array[lang]
          || lang_array[lang] == LANG_COMMON || ch->pcdata->learned[sn] >= 99 )
      {
         act( AT_PLAIN, "You are already fluent in $t.", ch, lang_names[lang], NULL, TO_CHAR );
         return;
      }
      /*
       * Bug fix - Samson 12-25-98 
       */
      for( sch = ch->in_room->first_person; sch; sch = sch->next_in_room )
         if( IS_ACT_FLAG( sch, ACT_SCHOLAR ) && knows_language( sch, ch->speaking, ch )
             && knows_language( sch, lang_array[lang], sch ) && ( !sch->speaking
                                                                  || knows_language( ch, sch->speaking, sch ) ) )
            break;
      if( !sch )
      {
         send_to_char( "There is no one who can teach that language here.\n\r", ch );
         return;
      }
      /*
       * 0..16 cha = 2 pracs, 17..25 = 1 prac. -- Altrag 
       */
      prac = 2 - ( get_curr_cha( ch ) / 17 );
      if( ch->pcdata->practice < prac )
      {
         act( AT_TELL, "$n tells you 'You do not have enough practices.'", sch, NULL, ch, TO_VICT );
         return;
      }
      ch->pcdata->practice -= prac;
      /*
       * Max 12% (5 + 4 + 3) at 24+ int and 21+ wis. -- Altrag 
       */
      prct = 5 + ( get_curr_int( ch ) / 6 ) + ( get_curr_wis( ch ) / 7 );
      ch->pcdata->learned[sn] += prct;
      ch->pcdata->learned[sn] = UMIN( ch->pcdata->learned[sn], 99 );
      SET_BIT( ch->speaks, lang_array[lang] );
      if( ch->pcdata->learned[sn] == prct )
         act( AT_PLAIN, "You begin lessons in $t.", ch, lang_names[lang], NULL, TO_CHAR );
      else if( ch->pcdata->learned[sn] < 60 )
         act( AT_PLAIN, "You continue lessons in $t.", ch, lang_names[lang], NULL, TO_CHAR );
      else if( ch->pcdata->learned[sn] < 60 + prct )
         act( AT_PLAIN, "You feel you can start communicating in $t.", ch, lang_names[lang], NULL, TO_CHAR );
      else if( ch->pcdata->learned[sn] < 99 )
         act( AT_PLAIN, "You become more fluent in $t.", ch, lang_names[lang], NULL, TO_CHAR );
      else
         act( AT_PLAIN, "You now speak perfect $t.", ch, lang_names[lang], NULL, TO_CHAR );
      return;
   }
   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( knows_language( ch, lang_array[lang], ch ) )
      {
         if( ch->speaking & lang_array[lang] || ( IS_NPC( ch ) && !ch->speaking ) )
            set_char_color( AT_SAY, ch );
         else
            set_char_color( AT_PLAIN, ch );
         send_to_char( lang_names[lang], ch );
         send_to_char( "\n\r", ch );
      }
   send_to_char( "\n\r", ch );
   return;
}
