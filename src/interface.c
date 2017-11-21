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
 *                        Terminal Interface Module                         *
 ****************************************************************************/

/* Interface code, commands affected by it have been moved here to keep track of them better */

#include "mud.h"
#include "clans.h"
#include "mxp.h"

void afk_score( CHAR_DATA * ch, char *argument );
void afk_attrib( CHAR_DATA * ch, char *argument );
void afk_prac_output( CHAR_DATA * ch, CHAR_DATA * mob );
void dale_score( CHAR_DATA * ch, char *argument );
void dale_attrib( CHAR_DATA * ch, char *argument );
void dale_prac_output( CHAR_DATA * ch, CHAR_DATA * is_at_gm );
void dale_group( CHAR_DATA * ch, char *argument );
void smaug_score( CHAR_DATA * ch, char *argument );
void smaug_prac_output( CHAR_DATA * ch, CHAR_DATA * is_at_gm );
void smaug_group( CHAR_DATA * ch, char *argument );
bool check_pets( CHAR_DATA * ch, MOB_INDEX_DATA * pet );

extern char *const realm_string[];
extern int num_logins;

/* Rank buffer code for use in I3 ( and elsewhere if we see fit I suppose ) */
/* buffer size is 200 because I3 and IMC2 will be expecting it to be so, DON'T CHANGE THIS */
char *rankbuffer( CHAR_DATA * ch )
{
   static char rbuf[200];

   if( IS_IMMORTAL( ch ) )
   {
      snprintf( rbuf, 200, "&Y%s", realm_string[ch->pcdata->realm] );

      if( ch->pcdata->rank && ch->pcdata->rank[0] != '\0' )
         snprintf( rbuf, 200, "&Y%s", ch->pcdata->rank );
   }
   else
   {
      snprintf( rbuf, 200, "&B%s", class_table[ch->Class]->who_name );

      if( ch->pcdata->rank && ch->pcdata->rank[0] != '\0' )
         snprintf( rbuf, 200, "&B%s", ch->pcdata->rank );

      if( ch->pcdata->clan )
      {
         if( ch->pcdata->clan->leadrank && ch->pcdata->clan->leadrank[0] != '\0'
             && ch->pcdata->clan->leader && !str_cmp( ch->name, ch->pcdata->clan->leader ) )
            snprintf( rbuf, 200, "&B%s", ch->pcdata->clan->leadrank );

         if( ch->pcdata->clan->onerank && ch->pcdata->clan->onerank[0] != '\0'
             && ch->pcdata->clan->number1 && !str_cmp( ch->name, ch->pcdata->clan->number1 ) )
            snprintf( rbuf, 200, "&B%s", ch->pcdata->clan->onerank );

         if( ch->pcdata->clan->tworank && ch->pcdata->clan->tworank[0] != '\0'
             && ch->pcdata->clan->number2 && !str_cmp( ch->name, ch->pcdata->clan->number2 ) )
            snprintf( rbuf, 200, "&B%s", ch->pcdata->clan->tworank );
      }
   }
   return rbuf;
}

CMDF do_score( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      dale_score( ch, "" );
      return;
   }

   switch ( GET_INTF( ch ) )
   {
      case INT_DALE:
         dale_score( ch, argument );
         break;
      case INT_SMAUG:
         smaug_score( ch, argument );
         break;
      default:
         afk_score( ch, argument );
         break;
   }
}

CMDF do_attrib( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      dale_score( ch, "" );
      return;
   }

   switch ( GET_INTF( ch ) )
   {
      case INT_SMAUG:
         smaug_score( ch, argument );
         break;
      case INT_DALE:
         dale_attrib( ch, argument );
         break;
      default:
         afk_attrib( ch, argument );
         break;
   }
}

/* Derived directly from the i3who code, which is a hybrid mix of Smaug, RM, and Dale who. */
int afk_who( CHAR_DATA * ch, char *argument )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *person;
   FILE *webwho = NULL;
   char s1[16], s2[16], s3[16], s4[16], s5[16], s6[16], s7[16];
   char rank[200], clan_name[MIL], buf[MSL], outbuf[MSL], stats[MIL], invis_str[50];
   int pcount = 0, amount, xx = 0, yy = 0;

   /*
    * Make sure the CH is before opening the file, don't need some yahoo trying to bug things out this way 
    */
   if( !str_cmp( argument, "www" ) && !ch )
   {
      if( !( webwho = fopen( WEBWHO_FILE, "w" ) ) )
      {
         bug( "%s", "afk_who: Unable to open webwho file for writing!" );
         return 0;
      }
   }

   if( ch )
   {
      snprintf( s1, 16, "%s", color_str( AT_WHO, ch ) );
      snprintf( s2, 16, "%s", color_str( AT_WHO2, ch ) );
      snprintf( s3, 16, "%s", color_str( AT_WHO3, ch ) );
      snprintf( s4, 16, "%s", color_str( AT_WHO4, ch ) );
      snprintf( s5, 16, "%s", color_str( AT_WHO5, ch ) );
      snprintf( s6, 16, "%s", color_str( AT_WHO6, ch ) );
      snprintf( s7, 16, "%s", color_str( AT_WHO7, ch ) );
   }

   outbuf[0] = '\0';

   if( !ch )
   {
      snprintf( buf, MSL, "&R-=[ &WPlayers on %s &R]=-", sysdata.mud_name );
      snprintf( outbuf, MSL, "%s", color_align( buf, 80, ALIGN_CENTER ) );
   }

   if( webwho )
   {
      mudstrlcat( outbuf, "<br />", MSL );
      fprintf( webwho, "%s", outbuf );
   }

   if( !ch )
   {
      outbuf[0] = '\0';

      snprintf( buf, MSL, "&Y-=[ &Wtelnet://%s:%d &Y]=-", sysdata.telnet, port );
      amount = 78 - color_strlen( buf );  /* Determine amount to put in front of line */

      if( amount < 1 )
         amount = 1;

      amount = amount / 2;

      for( xx = 0; xx < amount; xx++ )
         mudstrlcat( outbuf, " ", MSL );

      mudstrlcat( outbuf, buf, MSL );

      if( webwho )
      {
         mudstrlcat( outbuf, "<br />", MSL );
         fprintf( webwho, "%s", outbuf );
      }
   }
   xx = 0;
   for( d = first_descriptor; d; d = d->next )
   {
      person = d->original ? d->original : d->character;

      if( person && d->connected >= CON_PLAYING )
      {
         if( person->level >= LEVEL_IMMORTAL )
            continue;

         if( ch )
         {
            if( !can_see( ch, person, TRUE ) || is_ignoring( person, ch ) )
               continue;
         }
         if( ch && xx == 0 )
            pager_printf( ch,
                          "\n\r%s--------------------------------=[ %sPlayers %s]=---------------------------------\n\r\n\r",
                          s7, s6, s7 );
         else if( webwho && xx == 0 )
            fprintf( webwho, "%s",
                     "<br />&B--------------------------------=[ &WPlayers &B]=---------------------------------<br /><br />" );

         pcount++;

         snprintf( rank, 200, "%s", rankbuffer( person ) );
         snprintf( outbuf, MSL, "%s", color_align( rank, 20, ALIGN_CENTER ) );

         if( ch )
            send_to_pager( outbuf, ch );
         if( webwho )
            fprintf( webwho, "%s", outbuf );

         if( ch )
            snprintf( stats, MIL, "%s[", s3 );
         else
            mudstrlcpy( stats, "&z[", MIL );
         if( IS_PLR_FLAG( person, PLR_AFK ) )
            mudstrlcat( stats, "AFK", MIL );
         else
            mudstrlcat( stats, "---", MIL );
         if( CAN_PKILL( person ) )
            mudstrlcat( stats, "PK]", MIL );
         else
            mudstrlcat( stats, "--]", MIL );
         if( ch )
            mudstrlcat( stats, s3, MIL );
         else
            mudstrlcat( stats, "&G", MIL );

         if( person->pcdata->clan )
         {
            if( ch )
               snprintf( clan_name, MIL, " %s[", s5 );
            else
               mudstrlcpy( clan_name, " &c[", MIL );

            mudstrlcat( clan_name, person->pcdata->clan->name, MIL );
            if( ch )
               mudstrlcat( clan_name, s5, MIL );
            else
               mudstrlcat( clan_name, "&c", MIL );
            mudstrlcat( clan_name, "]", MIL );
         }
         else
            clan_name[0] = '\0';

         if( ch )
         {
            if( MXP_ON( ch ) && person->pcdata->homepage && person->pcdata->homepage[0] != '\0' )
            {
               pager_printf( ch, "%s %s" MXP_TAG_SECURE "<a href='%s'>%s</a>" MXP_TAG_LOCKED "%s%s\n\r",
                             stats, s4, person->pcdata->homepage, person->name, person->pcdata->title, clan_name );
            }
            else
               pager_printf( ch, "%s %s%s%s%s\n\r", stats, s4, person->name, person->pcdata->title, clan_name );
         }
         if( webwho )
         {
            if( person->pcdata->homepage && person->pcdata->homepage[0] != '\0' )
               fprintf( webwho, "%s <a href=\"%s\" target=\"_blank\">%s</a>%s%s<br />", stats, person->pcdata->homepage,
                        person->name, person->pcdata->title, clan_name );
            else
               fprintf( webwho, "%s &G%s%s%s<br />", stats, person->name, person->pcdata->title, clan_name );
         }
         xx++;
      }
   }

   yy = 0;
   for( d = first_descriptor; d; d = d->next )
   {
      person = d->original ? d->original : d->character;

      if( person && d->connected >= CON_PLAYING )
      {
         if( person->level < LEVEL_IMMORTAL )
            continue;

         if( ch )
         {
            if( !can_see( ch, person, TRUE ) || is_ignoring( person, ch ) )
               continue;
         }
         else
         {
            if( IS_PLR_FLAG( person, PLR_WIZINVIS ) )
               continue;
         }
         if( ch && yy == 0 )
         {
            pager_printf( ch,
                          "\n\r%s-------------------------------=[ %sImmortals %s]=--------------------------------\n\r\n\r",
                          s1, s6, s1 );
         }
         else if( webwho && yy == 0 )
            fprintf( webwho, "%s",
                     "<br />&R-------------------------------=[ &WImmortals &R]=--------------------------------<br /><br />" );

         pcount++;

         snprintf( rank, 200, "%s", rankbuffer( person ) );
         snprintf( outbuf, MSL, "%s", color_align( rank, 20, ALIGN_CENTER ) );
         if( ch )
            send_to_pager( outbuf, ch );
         if( webwho )
            fprintf( webwho, "%s", outbuf );

         if( ch )
            snprintf( stats, MIL, "%s[", s3 );
         else
            mudstrlcpy( stats, "&z[", MIL );
         if( IS_PLR_FLAG( person, PLR_AFK ) )
            mudstrlcat( stats, "AFK", MIL );
         else
            mudstrlcat( stats, "---", MIL );

         if( CAN_PKILL( person ) )
            mudstrlcat( stats, "PK]", MIL );
         else
            mudstrlcat( stats, "--]", MIL );
         if( ch )
            mudstrlcat( stats, s3, MIL );
         else
            mudstrlcat( stats, "&G", MIL );
         /*
          * Modified by Tarl 24 April 02 to display an invis level on the AFKMud interface. 
          */
         if( IS_PLR_FLAG( person, PLR_WIZINVIS ) )
         {
            snprintf( invis_str, 50, " (%d)", person->pcdata->wizinvis );
            mudstrlcat( stats, invis_str, MIL );
         }

         if( person->pcdata->clan )
         {
            if( ch )
               snprintf( clan_name, MIL, " %s[", s5 );
            else
               mudstrlcpy( clan_name, " &c[", MIL );

            mudstrlcat( clan_name, person->pcdata->clan->name, MIL );
            if( ch )
               mudstrlcat( clan_name, s5, MIL );
            else
               mudstrlcat( clan_name, "&c", MIL );
            mudstrlcat( clan_name, "]", MIL );
         }
         else
            clan_name[0] = '\0';

         if( ch )
         {
            if( MXP_ON( ch ) && person->pcdata->homepage && person->pcdata->homepage[0] != '\0' )
            {
               pager_printf( ch, "%s %s" MXP_TAG_SECURE "<a href='%s'>%s</a>" MXP_TAG_LOCKED "%s%s\n\r",
                             stats, s4, person->pcdata->homepage, person->name, person->pcdata->title, clan_name );
            }
            else
               pager_printf( ch, "%s %s%s%s%s\n\r", stats, s4, person->name, person->pcdata->title, clan_name );
         }
         if( webwho )
         {
            if( person->pcdata->homepage && person->pcdata->homepage[0] != '\0' )
               fprintf( webwho, "%s <a href=\"%s\" target=\"_blank\">%s</a>%s%s<br />", stats, person->pcdata->homepage,
                        person->name, person->pcdata->title, clan_name );
            else
               fprintf( webwho, "%s %s%s%s<br />", stats, person->name, person->pcdata->title, clan_name );
         }
         yy++;
      }
   }

   if( webwho )
   {
      fprintf( webwho, "<br />&Y[&W%d Player%s&Y] ", pcount, pcount == 1 ? "" : "s" );
      fprintf( webwho, "&Y[&WHomepage: <a href=\"%s\" target=\"_blank\">%s</a>&Y] [&W%d Max Since Reboot&Y]<br />",
               sysdata.http, sysdata.http, sysdata.maxplayers );
      fprintf( webwho, "&Y[&W%d login%s since last reboot on %s&Y]<br />", num_logins, num_logins == 1 ? "" : "s",
               str_boot_time );
   }

   if( webwho )
      FCLOSE( webwho );

   return pcount;
}

CMDF do_who( CHAR_DATA * ch, char *argument )
{
   char buf[MSL], buf2[MSL], outbuf[MSL];
   char s1[16], s2[16], s3[16];
   int amount = 0, xx = 0, pcount = 0;

   if( !ch )
   {
      afk_who( NULL, "www" );
      return;
   }

   snprintf( s1, 16, "%s", color_str( AT_WHO, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_WHO6, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_WHO2, ch ) );

   outbuf[0] = '\0';
   snprintf( buf, MSL, "%s-=[ %s%s %s]=-", s1, s2, sysdata.mud_name, s1 );
   snprintf( buf2, MSL, "&R-=[ &W%s &R]=-", sysdata.mud_name );
   amount = 78 - color_strlen( buf2 ); /* Determine amount to put in front of line */

   if( amount < 1 )
      amount = 1;

   amount = amount / 2;

   for( xx = 0; xx < amount; xx++ )
      mudstrlcat( outbuf, " ", MSL );

   mudstrlcat( outbuf, buf, MSL );
   pager_printf( ch, "%s\n\r", outbuf );

   pcount = afk_who( ch, argument );

   pager_printf( ch, "\n\r%s[%s%d Player%s%s] ", s3, s2, pcount, pcount == 1 ? "" : "s", s3 );
   if( MXP_ON( ch ) )
   {
      pager_printf( ch,
                    "%s[%sHomepage: " MXP_TAG_SECURE "<a href='%s' hint='%s Homepage'>%s</a>" MXP_TAG_LOCKED
                    "%s] [%s%3d Max Since Reboot%s]\n\r", s3, s2, sysdata.http, sysdata.mud_name, sysdata.http, s3, s2,
                    sysdata.maxplayers, s3 );
   }
   else
   {
      pager_printf( ch, "%s[%sHomepage: %s%s] [%s%d Max since reboot%s]\n\r",
                    s3, s2, sysdata.http, s3, s2, sysdata.maxplayers, s3 );
   }
   pager_printf( ch, "%s[%s%d login%s since last reboot on %s%s]\n\r", s3, s2,
                 num_logins, num_logins == 1 ? "" : "s", str_boot_time, s3 );
}

/*
 * Place any skill types you don't want them to be able to practice
 * normally in this list.  Separate each with a space.
 * (Uses an is_name check). -- Altrag
 */
#define CANT_PRAC "Tongue"
void race_practice( CHAR_DATA * ch, CHAR_DATA * mob, int sn )
{
   double adept;
   int race = mob->race;

   if( ch->race != race )
   {
      act( AT_TELL, "$n tells you 'I cannot teach those of your race.'", mob, NULL, ch, TO_VICT );
      return;
   }

   if( ( mob->level < ( skill_table[sn]->race_level[race] ) ) && skill_table[sn]->race_level[race] > 0 )
   {
      act( AT_TELL, "$n tells you 'You cannot learn that from me, you must find another...'", mob, NULL, ch, TO_VICT );
      return;
   }

   if( ch->level < skill_table[sn]->race_level[race] )
   {
      act( AT_TELL, "$n tells you 'You're not ready to learn that yet...'", mob, NULL, ch, TO_VICT );
      return;
   }

   if( is_name( skill_tname[skill_table[sn]->type], CANT_PRAC ) )
   {
      act( AT_TELL, "$n tells you 'I do not know how to teach that.'", mob, NULL, ch, TO_VICT );
      return;
   }

   adept = skill_table[sn]->race_adept[race] * 0.3 + ( get_curr_int( ch ) * 2 );

   if( ch->pcdata->learned[sn] >= adept )
   {
      act_printf( AT_TELL, mob, NULL, ch, TO_VICT, "$n tells you, 'I've taught you everything I can about %s.'",
                  skill_table[sn]->name );
      act( AT_TELL, "$n tells you, 'You'll have to practice it on your own now...'", mob, NULL, ch, TO_VICT );
   }
   else
   {
      ch->pcdata->practice--;
      ch->pcdata->learned[sn] += ( 4 + int_app[get_curr_int( ch )].learn );
      act( AT_ACTION, "You practice $T.", ch, NULL, skill_table[sn]->name, TO_CHAR );
      act( AT_ACTION, "$n practices $T.", ch, NULL, skill_table[sn]->name, TO_ROOM );
      if( ch->pcdata->learned[sn] >= adept )
      {
         ch->pcdata->learned[sn] = ( short ) adept;
         act( AT_TELL, "$n tells you. 'You'll have to practice it on your own now...'", mob, NULL, ch, TO_VICT );
      }
   }
   return;
}

/* Function modified from original form - Samson */
CMDF do_practice( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   int sn = 0;
   CHAR_DATA *mob = NULL;
   double adept;
   int Class;

   if( IS_NPC( ch ) )
      return;

   for( mob = ch->in_room->first_person; mob; mob = mob->next_in_room )
      if( IS_NPC( mob ) && ( IS_ACT_FLAG( mob, ACT_PRACTICE ) || IS_ACT_FLAG( mob, ACT_TEACHER ) ) )
         break;

   if( argument[0] == '\0' )
   {
      switch ( GET_INTF( ch ) )
      {
         default:
            afk_prac_output( ch, mob );
            break;
         case INT_SMAUG:
            smaug_prac_output( ch, mob );
            break;
         case INT_DALE:
            dale_prac_output( ch, mob );
            break;
      }
   }
   else
   {
      if( !IS_AWAKE( ch ) )
      {
         send_to_char( "In your dreams, or what?\n\r", ch );
         return;
      }

      if( !mob )
      {
         send_to_char( "You can't do that here.\n\r", ch );
         return;
      }

      if( ch->pcdata->practice < 1 )
      {
         act( AT_TELL, "$n tells you 'You must earn some more practice sessions.'", mob, NULL, ch, TO_VICT );
         return;
      }

      if( ( sn = skill_lookup( argument ) ) == -1 )
      {
         act_printf( AT_TELL, mob, NULL, ch, TO_VICT,
                     "$n tells you 'I've never heard of %s. Are you sure you know what you want?'", argument );
         return;
      }

      if( IS_ACT_FLAG( mob, ACT_TEACHER ) )
      {
         race_practice( ch, mob, sn );
         return;
      }

      Class = mob->Class;

      if( ch->Class != Class )
      {
         act( AT_TELL, "$n tells you 'I cannot teach those of your Class.'", mob, NULL, ch, TO_VICT );
         return;
      }

      if( skill_table[sn]->skill_level[Class] > LEVEL_AVATAR )
      {
         act( AT_TELL, "$n tells you 'Only an immortal of your Class may learn that.'", mob, NULL, ch, TO_VICT );
         return;
      }

      if( ( mob->level < ( skill_table[sn]->skill_level[Class] ) ) && skill_table[sn]->skill_level[Class] > 0 )
      {
         act( AT_TELL, "$n tells you 'You cannot learn that from me, you must find another...'", mob, NULL, ch, TO_VICT );
         return;
      }

      if( ch->level < skill_table[sn]->skill_level[Class] )
      {
         act( AT_TELL, "$n tells you 'You're not ready to learn that yet...'", mob, NULL, ch, TO_VICT );
         return;
      }

      if( is_name( skill_tname[skill_table[sn]->type], CANT_PRAC ) )
      {
         act( AT_TELL, "$n tells you 'I do not know how to teach that.'", mob, NULL, ch, TO_VICT );
         return;
      }

      /*
       * Skill requires a special teacher
       */
      if( skill_table[sn]->teachers && skill_table[sn]->teachers[0] != '\0' )
      {
         snprintf( buf, MSL, "%d", mob->pIndexData->vnum );
         if( !is_name( buf, skill_table[sn]->teachers ) )
         {
            act( AT_TELL, "$n tells you, 'You must find a specialist to learn that!'", mob, NULL, ch, TO_VICT );
            return;
         }
      }

      /*
       * Guild checks - right now, cant practice guild skills - done on 
       * induct/outcast
       */
      /*
       * if( !IS_NPC(ch) && !IS_GUILDED(ch) && skill_table[sn]->guild != CLASS_NONE )
       * {
       * act( AT_TELL, "$n tells you 'Only guild members can use that..'", mob, NULL, ch, TO_VICT );
       * return;
       * }
       * 
       * if( !IS_NPC(ch) && skill_table[sn]->guild != CLASS_NONE && ch->pcdata->clan->Class != skill_table[sn]->guild )
       * {
       * act( AT_TELL, "$n tells you 'That I can not teach to your guild.'", mob, NULL, ch, TO_VICT );
       * return;
       * }
       * 
       * if( !IS_NPC(ch) && skill_table[sn]->guild != CLASS_NONE )
       * {
       * act( AT_TELL, "$n tells you 'That is only for members of guilds...'", mob, NULL, ch, TO_VICT );
       * return;
       * }
       */

      adept = class_table[ch->Class]->skill_adept * 0.3 + ( get_curr_int( ch ) * 2 );

      if( ch->pcdata->learned[sn] >= adept )
      {
         act_printf( AT_TELL, mob, NULL, ch, TO_VICT,
                     "$n tells you, 'I've taught you everything I can about %s.'", skill_table[sn]->name );
         act( AT_TELL, "$n tells you, 'You'll have to practice it on your own now...'", mob, NULL, ch, TO_VICT );
      }
      else
      {
         ch->pcdata->practice--;
         ch->pcdata->learned[sn] += ( 4 + int_app[get_curr_int( ch )].learn );
         act( AT_ACTION, "You practice $T.", ch, NULL, skill_table[sn]->name, TO_CHAR );
         act( AT_ACTION, "$n practices $T.", ch, NULL, skill_table[sn]->name, TO_ROOM );
         if( ch->pcdata->learned[sn] >= adept )
         {
            ch->pcdata->learned[sn] = ( short ) adept;
            act( AT_TELL, "$n tells you. 'You'll have to practice it on your own now...'", mob, NULL, ch, TO_VICT );
         }
      }
   }
   return;
}

CMDF do_group( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim = NULL;

   if( !argument || argument[0] == '\0' )
   {
      switch ( GET_INTF( ch ) )
      {
         case INT_SMAUG:
            smaug_group( ch, argument );
            break;
         default:
            dale_group( ch, argument );
            break;
      }
      return;
   }

   if( !str_cmp( argument, "disband" ) )
   {
      CHAR_DATA *gch;
      int count = 0;

      if( ch->leader || ch->master )
      {
         send_to_char( "You cannot disband a group if you're following someone.\n\r", ch );
         return;
      }

      for( gch = first_char; gch; gch = gch->next )
      {
         if( is_same_group( ch, gch ) && ( ch != gch ) )
         {
            gch->leader = NULL;
            if( ( IS_NPC( gch ) && !check_pets( ch, gch->pIndexData ) ) || !IS_NPC( gch ) )
               gch->master = NULL;
            count++;
            send_to_char( "Your group is disbanded.\n\r", gch );
         }
      }
      if( count == 0 )
         send_to_char( "You have no group members to disband.\n\r", ch );
      else
         send_to_char( "You disband your group.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      CHAR_DATA *rch;
      int count = 0;

      for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
      {
         if( ch != rch && !IS_NPC( rch ) && can_see( ch, rch, FALSE ) && rch->master == ch
             && !ch->master && !ch->leader && !is_same_group( rch, ch ) && IS_PKILL( ch ) == IS_PKILL( rch ) )
         {
            rch->leader = ch;
            count++;
         }
      }
      if( count == 0 )
         send_to_char( "You have no eligible group members.\n\r", ch );
      else
      {
         act( AT_ACTION, "$n groups $s followers.", ch, NULL, victim, TO_ROOM );
         send_to_char( "You group your followers.\n\r", ch );
      }
      return;
   }
   if( !( victim = get_char_room( ch, argument ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( ch->master || ( ch->leader && ch->leader != ch ) )
   {
      send_to_char( "But you are following someone else!\n\r", ch );
      return;
   }

   if( victim->master != ch && ch != victim )
   {
      act( AT_PLAIN, "$N isn't following you.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( is_same_group( victim, ch ) && ch != victim )
   {
      victim->leader = NULL;
      act( AT_ACTION, "$n removes $N from $s group.", ch, NULL, victim, TO_NOTVICT );
      act( AT_ACTION, "$n removes you from $s group.", ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "You remove $N from your group.", ch, NULL, victim, TO_CHAR );
      return;
   }

   victim->leader = ch;
   act( AT_ACTION, "$N joins $n's group.", ch, NULL, victim, TO_NOTVICT );
   act( AT_ACTION, "You join $n's group.", ch, NULL, victim, TO_VICT );
   act( AT_ACTION, "$N joins your group.", ch, NULL, victim, TO_CHAR );
   return;
}

CMDF do_interface( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Creatures such as yourself can't see the world in any other way!\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "afk" ) || !str_cmp( argument, "afkmud" ) )
   {
      ch->pcdata->interface = INT_AFKMUD;
      send_to_char( "Interface set to AFKMud!\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "dale" ) )
   {
      ch->pcdata->interface = INT_DALE;
      send_to_char( "Interface set to Dale/SillyMUD!\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "smaug" ) )
   {
      ch->pcdata->interface = INT_SMAUG;
      send_to_char( "Interface set to Smaug!\n\r", ch );
      return;
   }

   send_to_char( "That's not a valid interface.\n\r", ch );
   send_to_char( "Interface can be: Afkmud, Dale, or Smaug\n\r", ch );
}
