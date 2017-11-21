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
 *                         AFKMud Interface Module                          *
 ****************************************************************************/

#include "mud.h"
#include "calendar.h"
#include "clans.h"
#include "deity.h"
#include "msp.h"
#include "mxp.h"

char *how_good( int percent );
char *attribtext( int attribute );

void afk_score( CHAR_DATA * ch, char *argument )
{
   char s1[16], s2[16], s3[16], s4[16], s5[16];

   snprintf( s1, 16, "%s", color_str( AT_SCORE, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_SCORE2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_SCORE3, ch ) );
   snprintf( s4, 16, "%s", color_str( AT_SCORE4, ch ) );
   snprintf( s5, 16, "%s", color_str( AT_SCORE5, ch ) );

   ch_printf( ch, "%sScore for %s%s\n\r", s1, ch->name, ch->pcdata->title );
   ch_printf( ch, "%s--------------------------------------------------------------------------------\n\r", s5 );

   ch_printf( ch, "%sLevel: %s%-15d %sHitPoints: %s%5d%s/%s%5d      %sPager    %s(%s%s%s) %s%4d\n\r",
              s2, s3, ch->level, s2, s3, ch->hit, s1, s4, ch->max_hit, s2, s1,
              s3, IS_PCFLAG( ch, PCFLAG_PAGERON ) ? "X" : " ", s1, s4, ch->pcdata->pagerlen );

   ch_printf( ch, "%sRace : %s%-15.15s %sMana     : %s%5d%s/%s%5d      %sAutoexit %s(%s%s%s)\n\r",
              s2, s3, capitalize( npc_race[ch->race] ), s2, s3, ch->mana, s1, s4, ch->max_mana, s2, s1,
              s3, IS_PLR_FLAG( ch, PLR_AUTOEXIT ) ? "X" : " ", s1 );

   ch_printf( ch, "%sClass: %s%-15.15s %sMovement : %s%5d%s/%s%5d      %sAutoloot %s(%s%s%s)\n\r",
              s2, s3, capitalize( npc_class[ch->Class] ), s2, s3, ch->move, s1, s4, ch->max_move, s2, s1,
              s3, IS_PLR_FLAG( ch, PLR_AUTOLOOT ) ? "X" : " ", s1 );

   ch_printf( ch, "%sAlign: %s%-15d %sTo Hit   : %s%s%-9d       %sAutosac  %s(%s%s%s)\n\r",
              s2, s3, ch->alignment, s2, s3, GET_HITROLL( ch ) > 0 ? "+" : "", GET_HITROLL( ch ), s2, s1,
              s3, IS_PLR_FLAG( ch, PLR_AUTOSAC ) ? "X" : " ", s1 );

   if( ch->level < 10 )
   {
      ch_printf( ch, "%sSTR  : %s%-15.15s %sTo Dam   : %s%s%-9d       %sSmartsac %s(%s%s%s)\n\r",
                 s2, s3, attribtext( get_curr_str( ch ) ), s2, s3, GET_DAMROLL( ch ) > 0 ? "+" : "", GET_DAMROLL( ch ), s2,
                 s1, s3, IS_PLR_FLAG( ch, PLR_SMARTSAC ) ? "X" : " ", s1 );

      ch_printf( ch, "%sINT  : %s%-15.15s %sAC       : %s%s%d\n\r",
                 s2, s3, attribtext( get_curr_int( ch ) ), s2, s3, GET_AC( ch ) > 0 ? "+" : "", GET_AC( ch ) );

      ch_printf( ch, "%sWIS  : %s%-15.15s %sWimpy    : %s%d\n\r",
                 s2, s3, attribtext( get_curr_wis( ch ) ), s2, s3, ch->wimpy );

      ch_printf( ch, "%sDEX  : %s%-15.15s %sExp      : %s%d\n\r",
                 s2, s3, attribtext( get_curr_dex( ch ) ), s2, s3, ch->exp );

      ch_printf( ch, "%sCON  : %s%-15.15s %sGold     : %s%d\n\r",
                 s2, s3, attribtext( get_curr_con( ch ) ), s2, s3, ch->gold );

      ch_printf( ch, "%sCHA  : %s%-15.15s %sWeight   : %s%d%s/%s%d\n\r",
                 s2, s3, attribtext( get_curr_cha( ch ) ), s2, s3, ch->carry_weight, s1, s4, can_carry_w( ch ) );

      ch_printf( ch, "%sLCK  : %s%-15.15s %sItems    : %s%d%s/%s%d\n\r",
                 s2, s3, attribtext( get_curr_lck( ch ) ), s2, s3, ch->carry_number, s1, s4, can_carry_n( ch ) );
   }
   else
   {
      ch_printf( ch, "%sSTR  : %s%-2d%s/%s%-12d %sTo Dam   : %s%s%-9d       %sSmartsac %s(%s%s%s)\n\r",
                 s2, s3, get_curr_str( ch ), s1, s4, ch->perm_str, s2, s3, GET_DAMROLL( ch ) > 0 ? "+" : " ",
                 GET_DAMROLL( ch ), s2, s1, s3, IS_PLR_FLAG( ch, PLR_SMARTSAC ) ? "X" : " ", s1 );

      ch_printf( ch, "%sINT  : %s%-2d%s/%s%-12d %sAC       : %s%s%d\n\r",
                 s2, s3, get_curr_int( ch ), s1, s4, ch->perm_int, s2, s3, GET_AC( ch ) > 0 ? "+" : "", GET_AC( ch ) );

      ch_printf( ch, "%sWIS  : %s%-2d%s/%s%-12d %sWimpy    : %s%d\n\r",
                 s2, s3, get_curr_wis( ch ), s1, s4, ch->perm_wis, s2, s3, ch->wimpy );

      ch_printf( ch, "%sDEX  : %s%-2d%s/%s%-12d %sExp      : %s%d\n\r",
                 s2, s3, get_curr_dex( ch ), s1, s4, ch->perm_dex, s2, s3, ch->exp );

      ch_printf( ch, "%sCON  : %s%-2d%s/%s%-12d %sGold     : %s%d\n\r",
                 s2, s3, get_curr_con( ch ), s1, s4, ch->perm_con, s2, s3, ch->gold );

      ch_printf( ch, "%sCHA  : %s%-2d%s/%s%-12d %sWeight   : %s%d%s/%s%d\n\r",
                 s2, s3, get_curr_cha( ch ), s1, s4, ch->perm_cha, s2, s3, ch->carry_weight, s1, s4, can_carry_w( ch ) );

      ch_printf( ch, "%sLCK  : %s%-2d%s/%s%-12d %sItems    : %s%d%s/%s%d\n\r",
                 s2, s3, get_curr_lck( ch ), s1, s4, ch->perm_lck, s2, s3, ch->carry_number, s1, s4, can_carry_n( ch ) );
   }

   ch_printf( ch, "%sPracs: %s%-15d %sFavor    : %s%d\n\r\n\r", s2, s3, ch->pcdata->practice, s2, s3, ch->pcdata->favor );

   ch_printf( ch, "%sYou are %s.\n\r", s2, npc_position[ch->position] );

   if( ch->pcdata->condition[COND_DRUNK] > 10 )
      ch_printf( ch, "%sYou are drunk.\n\r", s2 );

   if( ch->pcdata->condition[COND_THIRST] == 0 )
      ch_printf( ch, "%sYou are extremely thirsty.\n\r", s2 );

   if( ch->pcdata->condition[COND_FULL] == 0 )
      ch_printf( ch, "%sYou are starving.\n\r", s2 );

   set_char_color( AT_SCORE2, ch );
   if( ch->position != POS_SLEEPING )
   {
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
   }
   else
   {
      if( ch->mental_state > 45 )
         send_to_char( "Your sleep is filled with strange and vivid dreams.\n\r", ch );
      else if( ch->mental_state > 25 )
         send_to_char( "Your sleep is uneasy.\n\r", ch );
      else if( ch->mental_state < -35 )
         send_to_char( "You are deep in a much needed sleep.\n\r", ch );
      else if( ch->mental_state < -25 )
         send_to_char( "You are in deep slumber.\n\r", ch );
   }

   if( ch->desc )
   {
      ch_printf( ch, "\n\r%sTerminal Support Detected: MCCP%s[%s%s%s] %sMSP%s[%s%s%s] %sMXP%s[%s%s%s]\n\r", s2,
                 s1, s3, ch->desc->can_compress ? "X" : " ", s1, s2,
                 s1, s3, ch->desc->msp_detected ? "X" : " ", s1, s2, s1, s3, ch->desc->mxp_detected ? "X" : " ", s1 );

      ch_printf( ch, "%sTerminal Support In Use  : MCCP%s[%s%s%s] %sMSP%s[%s%s%s] %sMXP%s[%s%s%s]\n\r", s2,
                 s1, s3, ch->desc->can_compress ? "X" : " ", s1, s2,
                 s1, s3, MSP_ON( ch ) ? "X" : " ", s1, s2, s1, s3, MXP_ON( ch ) ? "X" : " ", s1 );
   }

   if( ch->pcdata->bestowments && ch->pcdata->bestowments[0] != '\0' )
      ch_printf( ch, "\n\r%sYou are bestowed with the command(s): %s%s.\n\r", s2, s3, ch->pcdata->bestowments );

   if( IS_IMMORTAL( ch ) )
   {
      ch_printf( ch, "\n\r%s--------------------------------------------------------------------------------\n\r", s5 );

      ch_printf( ch, "%sIMMORTAL DATA:  Wizinvis %s(%s%s%s)  %sLevel %s(%s%d%s)\n\r",
                 s2, s1, s3, IS_PLR_FLAG( ch, PLR_WIZINVIS ) ? "X" : " ", s1, s2, s1, s3, ch->pcdata->wizinvis, s1 );

      ch_printf( ch, "%sBamfin:  %s%s\n\r", s2, s1,
                 ( ch->pcdata->bamfin
                   && ch->pcdata->bamfin[0] != '\0' ) ? ch->pcdata->bamfin : "appears in a swirling mist." );

      ch_printf( ch, "%sBamfout: %s%s\n\r", s2, s1,
                 ( ch->pcdata->bamfout
                   && ch->pcdata->bamfout[0] != '\0' ) ? ch->pcdata->bamfout : "leaves in a swirling mist." );

      /*
       * Area Loaded info - Scryn 8/11
       */
      if( ch->pcdata->area )
      {
         ch_printf( ch, "%sVnums: %s%-5.5d - %-5.5d\n\r", s2, s1, ch->pcdata->area->low_vnum, ch->pcdata->area->hi_vnum );
      }
   }

   if( ch->first_affect )
   {
      SKILLTYPE *sktmp;
      AFFECT_DATA *paf;
      char buf[MSL];

      ch_printf( ch, "\n\r%s--------------------------------------------------------------------------------\n\r", s5 );
      ch_printf( ch, "%sAffect Data:\n\r\n\r", s2 );

      for( paf = ch->first_affect; paf; paf = paf->next )
      {
         if( !( sktmp = get_skilltype( paf->type ) ) )
            continue;

         snprintf( buf, MSL, "%s'%s%s%s'", s1, s3, sktmp->name, s1 );

         ch_printf( ch, "%s%s : %-20s %s(%s%d hours%s)\n\r",
                    s2, skill_tname[sktmp->type], buf, s1, s4, paf->duration / ( int )DUR_CONV, s1 );
      }
   }
   return;
}

void afk_attrib( CHAR_DATA * ch, char *argument )
{
   char *suf;
   char s1[16], s2[16], s3[16], s4[16], s5[16];
   int iLang;
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

   snprintf( s1, 16, "%s", color_str( AT_SCORE, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_SCORE2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_SCORE3, ch ) );
   snprintf( s4, 16, "%s", color_str( AT_SCORE4, ch ) );
   snprintf( s5, 16, "%s", color_str( AT_SCORE5, ch ) );

   ch_printf( ch, "%sYou are %s%d %syears old.\n\r", s2, s3, get_age( ch ), s2 );

   ch_printf( ch, "%sYou are %s%d%s inches tall, and weigh %s%d%s lbs.\n\r", s2, s3, ch->height, s2, s3, ch->weight, s2 );

   if( time_info.day == ch->pcdata->day && time_info.month == ch->pcdata->month )
      ch_printf( ch, "%sToday is your birthday!\n\r", s2 );
   else
      ch_printf( ch, "%sYour birthday is: %sDay of %s, %d%s day in the Month of %s, in the year %d.\n\r",
                 s2, s1, day_name[ch->pcdata->day % 13], day, suf, month_name[ch->pcdata->month], ch->pcdata->year );

   ch_printf( ch, "%sYou have played for %s%ld %shours.\n\r", s2, s3, ( long int )GET_TIME_PLAYED( ch ), s1 );

   if( ch->pcdata->deity )
      ch_printf( ch, "%sYou have devoted to %s%s.\n\r", s2, s3, ch->pcdata->deity->name );

   ch_printf( ch, "\n\r%sLanguages: ", s2 );

   for( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ )
   {
      if( knows_language( ch, lang_array[iLang], ch ) )
      {
         if( lang_array[iLang] & ch->speaking )
            send_to_char( s3, ch );
         send_to_char( lang_names[iLang], ch );
         send_to_char( " ", ch );
         send_to_char( s1, ch );
      }
   }
   send_to_char( "\n\r\n\r", ch );

   ch_printf( ch, "%sLogin: %s%s\n\r", s2, s3, c_time( ch->pcdata->logon, ch->pcdata->timezone ) );
   ch_printf( ch, "%sSaved: %s%s\n\r", s2, s3,
              ch->pcdata->save_time ? c_time( ch->pcdata->save_time, ch->pcdata->timezone ) : "no save this session" );
   ch_printf( ch, "%sTime : %s%s\r", s2, s3, c_time( current_time, ch->pcdata->timezone ) );

   ch_printf( ch, "\n\r%sMKills : %s%d\n\r", s2, s3, ch->pcdata->mkills );
   ch_printf( ch, "%sMDeaths: %s%d\n\r\n\r", s2, s3, ch->pcdata->mdeaths );

   if( ch->pcdata->clan && ch->pcdata->clan->clan_type == CLAN_ORDER )
   {
      ch_printf( ch, "%sGuild         : %s%s\n\r", s2, s3, ch->pcdata->clan->name );
      ch_printf( ch, "%sGuild MDeaths : %s%-6d      %sGuild MKills: %s%d\n\r\n\r",
                 s2, s3, ch->pcdata->clan->mdeaths, s2, s3, ch->pcdata->clan->mkills );
   }

   if( CAN_PKILL( ch ) )
   {
      ch_printf( ch, "%sPKills        : %s%-6d      %sClan        : %s%s\n\r",
                 s2, s3, ch->pcdata->pkills, s2, s3, ch->pcdata->clan ? ch->pcdata->clan->name : "None" );
      ch_printf( ch, "%sIllegal PKills: %s%-6d      %sClan PKills : %s%d\n\r",
                 s2, s3, ch->pcdata->illegal_pk, s2, s3, ( ch->pcdata->clan ? ch->pcdata->clan->pkills[0] : -1 ) );
      ch_printf( ch, "%sPDeaths       : %s%-6d      %sClan PDeaths: %s%d\n\r",
                 s2, s3, ch->pcdata->pdeaths, s2, s3, ( ch->pcdata->clan ? ch->pcdata->clan->pdeaths[0] : -1 ) );
   }
   return;
}

void afk_prac_output( CHAR_DATA * ch, CHAR_DATA * is_at_gm )
{
   int col, sn, is_ok;
   short lasttype, cnt;
   char s1[16], s2[16], s3[16], s4[16];

   col = cnt = 0;
   lasttype = SKILL_SPELL;
   set_pager_color( AT_MAGIC, ch );

   snprintf( s1, 16, "%s", color_str( AT_PRAC, ch ) );
   snprintf( s2, 16, "%s", color_str( AT_PRAC2, ch ) );
   snprintf( s3, 16, "%s", color_str( AT_PRAC3, ch ) );
   snprintf( s4, 16, "%s", color_str( AT_PRAC4, ch ) );

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

   for( sn = 0; sn < top_sn; sn++ )
   {
      if( !skill_table[sn]->name )
         break;

      if( !str_cmp( skill_table[sn]->name, "reserved" ) && ( IS_IMMORTAL( ch ) || CAN_CAST( ch ) ) )
      {
         if( col % 2 != 0 )
            send_to_pager( "\n\r", ch );
         pager_printf( ch, "%s--------------------------------[%sSpells%s]---------------------------------\n\r", s4, s2,
                       s4 );
         col = 0;
      }
      if( skill_table[sn]->type != lasttype )
      {
         if( !cnt )
            send_to_pager( "                                (none)\n\r", ch );
         else if( col % 2 != 0 )
            send_to_pager( "\n\r", ch );
         pager_printf( ch, "%s--------------------------------[%s%ss%s]---------------------------------\n\r",
                       s4, s2, skill_tname[skill_table[sn]->type], s4 );
         col = cnt = 0;
      }
      lasttype = skill_table[sn]->type;

      if( !IS_IMMORTAL( ch ) && ( skill_table[sn]->guild != CLASS_NONE && ( !IS_GUILDED( ch ) ) ) )
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

      if( ch->pcdata->learned[sn] ||
          ( is_at_gm && is_at_gm->level >= skill_table[sn]->skill_level[is_at_gm->Class] )
          || ( is_at_gm && is_at_gm->level >= skill_table[sn]->race_level[is_at_gm->race] ) )
      {
         ++cnt;

         /*
          * Edited by Whir to look pretty, still want to improve it though - 1/30/03 
          */
         pager_printf( ch, "%s%22s%s%s(%s%11s%s)", s2, skill_table[sn]->name,
                       ( ch->level < skill_table[sn]->skill_level[ch->Class]
                         && ch->level < skill_table[sn]->race_level[ch->race] ) ? "&W*" : " ", s1, s3,
                       how_good( ch->pcdata->learned[sn] ), s1 );

         if( ++col % 2 == 0 )
            send_to_pager( "\n\r", ch );
      }
   }

   if( col % 2 != 0 )
      send_to_pager( "\n\r", ch );

   send_to_pager( "&W*&D Indicates a skill you have not acheived the required level for yet.\n\r", ch );
   pager_printf( ch, "%sYou have %s%d %spractice sessions left.\n\r", s1, s4, ch->pcdata->practice, s1 );
   return;
}
