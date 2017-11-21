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
 *                        DaleMud Interface Module                          *
 ****************************************************************************/

/******************************************************
            Desolation of the Dragon MUD II
      (C) 1997, 1998  Jesse DeFer and Heath Leach
 http://dotd.mudservices.com  dotd@dotd.mudservices.com 
 ******************************************************/

#include "mud.h"
#include "calendar.h"
#include "clans.h"
#include "deity.h"
#include "msp.h"
#include "mxp.h"

void paint( short AType, CHAR_DATA * ch, const char *fmt, ... );
char *attribtext( int attribute );

void dale_score( CHAR_DATA * ch, char *argument )
{
   char buf[MSL], s1[16], s2[16], s3[16], s4[16];
   int iLang;

   snprintf( s1, 16, "%s", color_str( AT_SCORE, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_SCORE2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_SCORE3, ch ) );
   snprintf( s4, 16, "%s", color_str( AT_SCORE4, ch ) );

   if( !IS_NPC( ch ) )
   {
      char *suf;
      short day;
      day = ch->pcdata->day + 1;

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

      ch_printf( ch, "\n\r%sYou are %s%d%s years old.\n\r", s1, s2, get_age( ch ), s1 );

      if( time_info.day == ch->pcdata->day && time_info.month == ch->pcdata->month )
         ch_printf( ch, "%sToday is your birthday!%s\n\r", s2, s1 );
      else
         ch_printf( ch, "%sYour birthday is: %sDay of %s, %d%s day in the Month of %s, in the year %d.%s\n\r",
                    s1, s2, day_name[ch->pcdata->day % 13], day, suf, month_name[ch->pcdata->month], ch->pcdata->year, s1 );
   }

   ch_printf( ch, "%sYou belong to the %s%s%s race.\n\r", s1, s3, get_race( ch ), s1 );

   ch_printf( ch, "%sYou have %s%d%s(%s%d%s) hit, %s%d%s(%s%d%s) mana, %s%d%s(%s%d%s) movement points.\n\r",
              s1, s2, ch->hit, s1, s4, ch->max_hit, s1, s2, ch->mana, s1, s4, ch->max_mana, s1, s2, ch->move, s1, s4,
              ch->max_move, s1 );

   if( !IS_NPC( ch ) )
   {
      ch_printf( ch, "%sYou have %s%d%s practices left.\n\r", s1, s2, ch->pcdata->practice, s1 );

      ch_printf( ch, "%sYou have died %s%d%s times and killed %s%d%s times.\n\r",
                 s1, s2, ch->pcdata->mdeaths, s1, s2, ch->pcdata->mkills, s1 );
   }

   ch_printf( ch, "%sYour alignment is: %s%d%s.\n\r", s1, s3, ch->alignment, s1 );

   ch_printf( ch, "%sYou are a level %s%d %s%s\n\r", s1, s2, ch->level, capitalize( npc_class[ch->Class] ), s1 );

   ch_printf( ch, "%sYou have scored %s%d%s exp.\n\r", s1, s2, GET_EXP( ch ), s1 );

   if( !IS_NPC( ch ) )
   {
      ch_printf( ch, "%sYou have %s%d%s gold on hand and %s%d%s in the bank.\n\r",
                 s1, s2, ch->gold, s1, s2, ch->pcdata->balance, s1 );

      if( ch->pcdata->deity )
      {
         ch_printf( ch, "%sYou are devoted to %s%s%s, and have %s%d%s favor.\n\r",
                    s1, s2, ch->pcdata->deity->name, s1, s2, ch->pcdata->favor, s1 );
      }
   }

   if( !IS_NPC( ch ) )
   {
      ch_printf( ch, "%sThis ranks you as: %s%s%s%s\n\r", s1, s3, ch->name, ch->pcdata->title, s1 );
      ch_printf( ch, "%sYou have been playing for: %s%ld%s hours.\n\r", s1, s2, ( long int )GET_TIME_PLAYED( ch ), s1 );
   }

   if( !IS_IMMORTAL( ch ) )
      ch_printf( ch, "%sExp to next level: %s%ld%s\n\r", s1, s2, exp_level( ch->level + 1 ) - ch->exp, s1 );

   if( get_trust( ch ) != ch->level )
      ch_printf( ch, "%sYou are trusted at level: %s%d%s\n\r", s1, s2, get_trust( ch ), s1 );

   mudstrlcpy( buf, npc_position[ch->position], MSL );

   ch_printf( ch, "%sYou are %s%s%s.\n\r", s1, s3, buf, s1 );

   if( !IS_NPC( ch ) && ch->pcdata->condition[COND_DRUNK] > 10 )
      send_to_char( "You are drunk.\n\r", ch );
   if( !IS_NPC( ch ) && ch->pcdata->condition[COND_THIRST] == 0 )
      send_to_char( "You are in danger of dehydrating.\n\r", ch );
   if( !IS_NPC( ch ) && ch->pcdata->condition[COND_FULL] == 0 )
      send_to_char( "You are starving to death.\n\r", ch );
   if( ch->position != POS_SLEEPING )
      switch ( ch->mental_state / 10 )
      {
         default:
            send_to_char( "You're completely messed up!\n\r", ch );
            break;
         case -10:
            send_to_char( "You're barely conscious.\n\r", ch );
            break;
         case -9:
            send_to_char( "You can barely keep your eyes open.\n\r", ch );
            break;
         case -8:
            send_to_char( "You're extremely drowsy.\n\r", ch );
            break;
         case -7:
            send_to_char( "You feel very unmotivated.\n\r", ch );
            break;
         case -6:
            send_to_char( "You feel sedated.\n\r", ch );
            break;
         case -5:
            send_to_char( "You feel sleepy.\n\r", ch );
            break;
         case -4:
            send_to_char( "You feel tired.\n\r", ch );
            break;
         case -3:
            send_to_char( "You could use a rest.\n\r", ch );
            break;
         case -2:
            send_to_char( "You feel a little under the weather.\n\r", ch );
            break;
         case -1:
            send_to_char( "You feel fine.\n\r", ch );
            break;
         case 0:
            send_to_char( "You feel great.\n\r", ch );
            break;
         case 1:
            send_to_char( "You feel energetic.\n\r", ch );
            break;
         case 2:
            send_to_char( "Your mind is racing.\n\r", ch );
            break;
         case 3:
            send_to_char( "You can't think straight.\n\r", ch );
            break;
         case 4:
            send_to_char( "Your mind is going 100 miles an hour.\n\r", ch );
            break;
         case 5:
            send_to_char( "You're high as a kite.\n\r", ch );
            break;
         case 6:
            send_to_char( "Your mind and body are slipping apart.\n\r", ch );
            break;
         case 7:
            send_to_char( "Reality is slipping away.\n\r", ch );
            break;
         case 8:
            send_to_char( "You have no idea what is real, and what is not.\n\r", ch );
            break;
         case 9:
            send_to_char( "You feel immortal.\n\r", ch );
            break;
         case 10:
            send_to_char( "You are a Supreme Entity.\n\r", ch );
            break;
      }
   else if( ch->mental_state > 45 )
      send_to_char( "Your sleep is filled with strange and vivid dreams.\n\r", ch );
   else if( ch->mental_state > 25 )
      send_to_char( "Your sleep is uneasy.\n\r", ch );
   else if( ch->mental_state < -35 )
      send_to_char( "You are deep in a much needed sleep.\n\r", ch );
   else if( ch->mental_state < -25 )
      send_to_char( "You are in deep slumber.\n\r", ch );

   send_to_char( "\n\rLanguages: ", ch );
   for( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ )
      if( knows_language( ch, lang_array[iLang], ch ) || ( IS_NPC( ch ) && ch->speaks == 0 ) )
      {
         if( lang_array[iLang] & ch->speaking || ( IS_NPC( ch ) && !ch->speaking ) )
            set_char_color( AT_RED, ch );
         send_to_char( lang_names[iLang], ch );
         send_to_char( " ", ch );
         set_char_color( AT_SCORE4, ch );
      }
   send_to_char( "\n\r\n\r", ch );

   set_char_color( AT_SCORE, ch );

   if( ch->desc )
   {
      ch_printf( ch, "%sTerminal Support Detected: %sMCCP[%s%s%s] MSP[%s%s%s] MXP[%s%s%s]\n\r", s1, s3,
                 s4, ch->desc->can_compress ? "X" : " ", s3,
                 s4, ch->desc->msp_detected ? "X" : " ", s3, s4, ch->desc->mxp_detected ? "X" : " ", s3 );

      ch_printf( ch, "%sTerminal Support In Use  : %sMCCP[%s%s%s] MSP[%s%s%s] MXP[%s%s%s]\n\r\n\r", s1, s3,
                 s4, ch->desc->can_compress ? "X" : " ", s3,
                 s4, MSP_ON( ch ) ? "X" : " ", s3, s4, MXP_ON( ch ) ? "X" : " ", s3 );
   }

   if( IS_IMMORTAL( ch ) )
   {
      send_to_char( "----------------------------------------------------------------------------\n\r", ch );

      ch_printf( ch, "%sIMMORTAL DATA:  Wizinvis [%s]  Wizlevel (%d)\n\r", s1,
                 IS_PLR_FLAG( ch, PLR_WIZINVIS ) ? "X" : " ", ch->pcdata->wizinvis );

      ch_printf( ch, "%sBamfin:  %s%s%s\n\r", s1, s3, ( ch->pcdata->bamfin && ch->pcdata->bamfin[0] != '\0' )
                 ? ch->pcdata->bamfin : "appears in a swirling mist.", s1 );

      ch_printf( ch, "%sBamfout: %s%s%s\n\r", s1, s3, ( ch->pcdata->bamfout && ch->pcdata->bamfout[0] != '\0' )
                 ? ch->pcdata->bamfout : "leaves in a swirling mist.", s1 );

      /*
       * Area Loaded info - Scryn 8/11
       */
      if( ch->pcdata->area )
      {
         ch_printf( ch, "%sVnums: %s%-5.5d - %-5.5d%s\n\r",
                    s1, s2, ch->pcdata->area->low_vnum, ch->pcdata->area->hi_vnum, s1 );
      }

      if( ch->pcdata->bestowments && ch->pcdata->bestowments[0] != '\0' )
         ch_printf( ch, "%sYou are bestowed with the command(s): %s.\n\r", s1, ch->pcdata->bestowments );
   }

   return;
}

void dale_attrib( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   AFFECT_DATA *paf;

   if( IS_NPC( ch ) )
      mudstrlcpy( buf, " ", MSL );
   else
      snprintf( buf, MSL, " %s%d%s years old, ", color_str( AT_SCORE2, ch ), get_age( ch ), color_str( AT_SCORE, ch ) );
   ch_printf( ch, "\n\r%sYou are%s%s%d%s inches, and %s%d%s lbs.\n\r"
              "You are carrying %s%d%s lbs of equipment.\n\r"
              "Your Armor Class is %s%d%s.\n\r",
              color_str( AT_SCORE, ch ), buf, color_str( AT_SCORE2, ch ),
              ch->height, color_str( AT_SCORE, ch ), color_str( AT_SCORE2, ch ),
              ch->weight, color_str( AT_SCORE, ch ), color_str( AT_SCORE2, ch ),
              ch->carry_weight, color_str( AT_SCORE, ch ), color_str( AT_SCORE3, ch ),
              GET_AC( ch ), color_str( AT_SCORE, ch ) );

   if( ch->level < 10 )
   {
      paint( AT_SCORE, ch, "You have:\n\r" );
      paint( AT_SCORE, ch, "Str: " );
      paint( AT_SCORE2, ch, "%s\n\r", attribtext( get_curr_str( ch ) ) );
      paint( AT_SCORE, ch, "Int: " );
      paint( AT_SCORE2, ch, "%s\n\r", attribtext( get_curr_int( ch ) ) );
      paint( AT_SCORE, ch, "Wis: " );
      paint( AT_SCORE2, ch, "%s\n\r", attribtext( get_curr_wis( ch ) ) );
      paint( AT_SCORE, ch, "Dex: " );
      paint( AT_SCORE2, ch, "%s\n\r", attribtext( get_curr_dex( ch ) ) );
      paint( AT_SCORE, ch, "Con: " );
      paint( AT_SCORE2, ch, "%s\n\r", attribtext( get_curr_con( ch ) ) );
      paint( AT_SCORE, ch, "Cha: " );
      paint( AT_SCORE2, ch, "%s\n\r", attribtext( get_curr_cha( ch ) ) );
      paint( AT_SCORE, ch, "Lck: " );
      paint( AT_SCORE2, ch, "%s\n\r", attribtext( get_curr_lck( ch ) ) );
   }
   else
   {
      paint( AT_SCORE, ch, "You have " );
      paint( AT_SCORE2, ch, "%d ", get_curr_str( ch ) );
      paint( AT_SCORE, ch, "STR, " );
      paint( AT_SCORE2, ch, "%d ", get_curr_int( ch ) );
      paint( AT_SCORE, ch, "INT, " );
      paint( AT_SCORE2, ch, "%d ", get_curr_wis( ch ) );
      paint( AT_SCORE, ch, "WIS, " );
      paint( AT_SCORE2, ch, "%d ", get_curr_dex( ch ) );
      paint( AT_SCORE, ch, "DEX, " );
      paint( AT_SCORE2, ch, "%d ", get_curr_con( ch ) );
      paint( AT_SCORE, ch, "CON, " );
      paint( AT_SCORE2, ch, "%d ", get_curr_cha( ch ) );
      paint( AT_SCORE, ch, "CHA, " );
      paint( AT_SCORE2, ch, "%d ", get_curr_lck( ch ) );
      paint( AT_SCORE, ch, "LCK\n\r" );
   }

   paint( AT_SCORE, ch, "%s", "Your hit and damage bonuses are " );
   paint( AT_SCORE3, ch, "%d ", GET_HITROLL( ch ) );
   paint( AT_SCORE, ch, "%s", "and " );
   paint( AT_SCORE3, ch, "%d ", GET_DAMROLL( ch ) );
   paint( AT_SCORE, ch, "%s", "respectively.\n\r" );

   send_to_char( "\n\rAffecting Spells:\n\r", ch );
   send_to_char( "-----------------\n\r", ch );
   if( ch->first_affect )
   {
      SKILLTYPE *sktmp;

      for( paf = ch->first_affect; paf; paf = paf->next )
      {
         if( ( sktmp = get_skilltype( paf->type ) ) == NULL )
            continue;
         snprintf( buf, MSL, "'%s%s%s'", color_str( AT_SCORE3, ch ), sktmp->name, color_str( AT_SCORE, ch ) );
         ch_printf( ch, "%s%s : %-20s (%s%d hours%s)\n\r",
                    color_str( AT_SCORE, ch ), skill_tname[sktmp->type], buf, color_str( AT_SCORE2, ch ),
                    paf->duration / ( int )DUR_CONV, color_str( AT_SCORE, ch ) );
      }
   }
}

void dale_group( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *gch;
   CHAR_DATA *leader;

   leader = ch->leader ? ch->leader : ch;
   ch_printf( ch, "%s%s's%s group:\n\r", color_str( AT_SCORE3, ch ), leader->name, color_str( AT_SCORE, ch ) );

   for( gch = first_char; gch; gch = gch->next )
   {
      if( is_same_group( gch, ch ) )
      {
         ch_printf( ch, "%s%-50s%s HP:%s%2.0f%s%% %4s:%s%2.0f%s%% MV:%s%2.0f%s%%\n\r",
                    color_str( AT_SCORE3, ch ), capitalize( gch->name ), color_str( AT_SCORE, ch ),
                    color_str( AT_SCORE2, ch ), ( ( float )GET_HIT( gch ) / GET_MAX_HIT( gch ) ) * 100 + 0.5,
                    color_str( AT_SCORE, ch ), "MANA", color_str( AT_SCORE2, ch ),
                    ( ( float )GET_MANA( gch ) / GET_MAX_MANA( gch ) ) * 100 + 0.5,
                    color_str( AT_SCORE, ch ), color_str( AT_SCORE2, ch ),
                    ( ( float )GET_MOVE( gch ) / GET_MAX_MOVE( gch ) ) * 100 + 0.5, color_str( AT_SCORE, ch ) );
      }
   }
}

char *how_good( int percent )
{
   static char buf[256];

   if( percent < 0 )
      mudstrlcpy( buf, "impossible - tell an immortal!", 256 );
   else if( percent == 0 )
      mudstrlcpy( buf, "not learned", 256 );
   else if( percent <= 10 )
      mudstrlcpy( buf, "awful", 256 );
   else if( percent <= 20 )
      mudstrlcpy( buf, "terrible", 256 );
   else if( percent <= 30 )
      mudstrlcpy( buf, "bad", 256 );
   else if( percent <= 40 )
      mudstrlcpy( buf, "poor", 256 );
   else if( percent <= 55 )
      mudstrlcpy( buf, "average", 256 );
   else if( percent <= 60 )
      mudstrlcpy( buf, "tolerable", 256 );
   else if( percent <= 70 )
      mudstrlcpy( buf, "fair", 256 );
   else if( percent <= 80 )
      mudstrlcpy( buf, "good", 256 );
   else if( percent <= 85 )
      mudstrlcpy( buf, "very good", 256 );
   else if( percent <= 90 )
      mudstrlcpy( buf, "excellent", 256 );
   else
      mudstrlcpy( buf, "Superb", 256 );

   return ( buf );
}

void dale_prac_output( CHAR_DATA * ch, CHAR_DATA * is_at_gm )
{
   int col, sn, is_ok;
   short lasttype, cnt;
   char buf[MIL];

   col = cnt = 0;
   lasttype = SKILL_SPELL;
   if( is_at_gm )
      snprintf( buf, MIL, "%d", is_at_gm->pIndexData->vnum );

   if( is_at_gm )
   {
      if( IS_ACT_FLAG( is_at_gm, ACT_PRACTICE ) )
      {
         if( is_at_gm->Class != ch->Class )
         {
            act( AT_TELL, "$n tells you 'I cannot teach those of your class.'", is_at_gm, NULL, ch, TO_VICT );
            return;
         }
      }
      else
      {
         if( is_at_gm->race != ch->race )
         {
            act( AT_TELL, "$n tells you 'I cannot teach those of your race.'", is_at_gm, NULL, ch, TO_VICT );
            return;
         }
      }
   }

   set_pager_color( AT_MAGIC, ch );
   for( sn = 0; sn < top_sn; sn++ )
   {
      if( !skill_table[sn]->name )
         continue;

      if( is_at_gm )
      {
         if( IS_ACT_FLAG( is_at_gm, ACT_PRACTICE ) )
            if( skill_table[sn]->skill_level[is_at_gm->Class] > is_at_gm->level )
               continue;
         if( IS_ACT_FLAG( is_at_gm, ACT_TEACHER ) )
            if( skill_table[sn]->race_level[is_at_gm->race] > is_at_gm->level )
               continue;
      }
      else
      {
         is_ok = FALSE;

         if( ch->level >= skill_table[sn]->skill_level[ch->Class] )
            is_ok = TRUE;
         if( ch->level >= skill_table[sn]->race_level[ch->race] )
            is_ok = TRUE;

         if( !is_ok )
            continue;
      }

      if( ch->pcdata->learned[sn] == 0 && SPELL_FLAG( skill_table[sn], SF_SECRETSKILL ) )
         continue;

      if( !IS_IMMORTAL( ch ) && ( skill_table[sn]->guild != CLASS_NONE && ( !IS_GUILDED( ch ) ) ) )
         continue;

      if( ch->pcdata->learned[sn] ||
          ( is_at_gm && is_at_gm->level >= skill_table[sn]->skill_level[is_at_gm->Class] )
          || ( is_at_gm && is_at_gm->level >= skill_table[sn]->race_level[is_at_gm->race] ) )
      {
         ++cnt;

         /*
          * Modified by Whir to look pretty - 1/30/2003 
          */
         pager_printf( ch, "%s%22s%s(%11s)", color_str( AT_SKILL, ch ), skill_table[sn]->name,
                       ( ch->level < skill_table[sn]->skill_level[ch->Class]
                         && ch->level < skill_table[sn]->race_level[ch->race] ) ? "&W*&D" : " ",
                       how_good( ch->pcdata->learned[sn] ) );

         if( ++col % 2 == 0 )
            send_to_pager( "\n\r", ch );
      }
   }

   if( col % 2 != 0 )
      send_to_pager( "\n\r", ch );

   send_to_pager( "&W*&D Indicates a skill you have not acheived the required level for yet.\n\r", ch );
   pager_printf( ch, "%sYou have %d practice sessions left.\n\r", color_str( AT_SKILL, ch ), ch->pcdata->practice );
}
