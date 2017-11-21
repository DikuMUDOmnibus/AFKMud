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
 *                        Smaug Interface Module                            *
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

char *attribtext( int attribute );

void smaug_score( CHAR_DATA * ch, char *argument )
{
   char buf[MSL], def_color[16];
   char *suf;
   AFFECT_DATA *paf;
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

   set_char_color( AT_SCORE4, ch );

   mudstrlcpy( def_color, color_str( AT_SCORE4, ch ), 16 );

   mudstrlcpy( buf, npc_position[ch->position], MSL );

   ch_printf( ch, "\n\rScore for %s%s%s.\n\r", ch->name, ch->pcdata->title, def_color );
   if( get_trust( ch ) != ch->level )
      ch_printf( ch, "You are trusted at level %d.\n\r", get_trust( ch ) );

   send_to_char( "----------------------------------------------------------------------------\n\r", ch );

   ch_printf( ch, "LEVEL: %-3.3d         Race : %-15.15s Played: %ld hours\n\r",
              ch->level, capitalize( get_race( ch ) ), ( long int )GET_TIME_PLAYED( ch ) );

   ch_printf( ch, "AGE  : %-6d      Class: %-15.15s Log In: %s\n\r",
              get_age( ch ), capitalize( npc_class[ch->Class] ), c_time( ch->pcdata->logon, ch->pcdata->timezone ) );

   if( ch->level < 10 )
   {
      ch_printf( ch, "STR  : %-10.10s      ToHit: %2d              Saved : %s\n\r",
                 attribtext( get_curr_str( ch ) ), GET_HITROLL( ch ),
                 ch->pcdata->save_time ? c_time( ch->pcdata->save_time, ch->pcdata->timezone ) : "no save this session\n" );

      ch_printf( ch, "INT  : %-10.10s      ToDam: %2d              Time  :  %s\n\r",
                 attribtext( get_curr_int( ch ) ), GET_DAMROLL( ch ), c_time( current_time, ch->pcdata->timezone ) );

      ch_printf( ch, "WIS  : %-10.10s      Armor: %d \n\r", attribtext( get_curr_wis( ch ) ), GET_AC( ch ) );

      ch_printf( ch, "DEX  : %-10.10s      Align: %d \n\r", attribtext( get_curr_dex( ch ) ), ch->alignment );

      ch_printf( ch, "CON  : %-10.10s      Pos'n: %-14.14s  Weight: %5.5d (max %7.7d)\n\r",
                 attribtext( get_curr_con( ch ) ), buf, ch->carry_weight, can_carry_w( ch ) );

      ch_printf( ch, "CHA  : %-10.10s      Wimpy: %2.2d              Items : %5.5d (max %5.5d)\n\r",
                 attribtext( get_curr_cha( ch ) ), ch->wimpy, ch->carry_number, can_carry_n( ch ) );

      ch_printf( ch, "LCK  : %-10.10s \n\r", attribtext( get_curr_lck( ch ) ) );
   }
   else
   {
      ch_printf( ch, "STR  : %2.2d(%2.2d)      ToHit: %2d              Saved : %s\n\r",
                 get_curr_str( ch ), ch->perm_str, GET_HITROLL( ch ),
                 ch->pcdata->save_time ? c_time( ch->pcdata->save_time, ch->pcdata->timezone ) : "no save this session" );

      ch_printf( ch, "INT  : %2.2d(%2.2d)      ToDam: %2d              Time  :  %s\n\r",
                 get_curr_int( ch ), ch->perm_int, GET_DAMROLL( ch ), c_time( current_time, ch->pcdata->timezone ) );

      ch_printf( ch, "WIS  : %2.2d(%2.2d)      Armor: %d \n\r", get_curr_wis( ch ), ch->perm_wis, GET_AC( ch ) );

      ch_printf( ch, "DEX  : %2.2d(%2.2d)      Align: %d \n\r", get_curr_dex( ch ), ch->perm_dex, ch->alignment );

      ch_printf( ch, "CON  : %2.2d(%2.2d)      Pos'n: %-14.14s  Weight: %5.5d (max %7.7d)\n\r",
                 get_curr_con( ch ), ch->perm_con, buf, ch->carry_weight, can_carry_w( ch ) );

      ch_printf( ch, "CHA  : %2.2d(%2.2d)      Wimpy: %2.2d              Items : %5.5d (max %5.5d)\n\r",
                 get_curr_cha( ch ), ch->perm_cha, ch->wimpy, ch->carry_number, can_carry_n( ch ) );

      ch_printf( ch, "LCK  : %2.2d(%2.2d) \n\r", get_curr_lck( ch ), ch->perm_lck );
   }

   ch_printf( ch, "PRACT: %3.3d         Hitpoints: %-5d of %5d   Pager: (%c) %3d    AutoExit(%c)\n\r",
              ch->pcdata->practice, GET_HIT( ch ), GET_MAX_HIT( ch ), IS_PCFLAG( ch, PCFLAG_PAGERON ) ? 'X' : ' ',
              ch->pcdata->pagerlen, IS_PLR_FLAG( ch, PLR_AUTOEXIT ) ? 'X' : ' ' );

   ch_printf( ch, "XP   : %-10d       Mana: %-5d of %5d   MKills:  %-5.5d    AutoLoot(%c)\n\r",
              GET_EXP( ch ), GET_MANA( ch ), GET_MAX_MANA( ch ), ch->pcdata->mkills, IS_PLR_FLAG( ch,
                                                                                                  PLR_AUTOLOOT ) ? 'X' :
              ' ' );

   ch_printf( ch, "GOLD : %-10d       Move: %-5d of %5d   Mdeaths: %-5.5d    AutoSac (%c)\n\r",
              ch->gold, GET_MOVE( ch ), GET_MAX_MOVE( ch ), ch->pcdata->mdeaths, IS_PLR_FLAG( ch,
                                                                                              PLR_AUTOSAC ) ? 'X' : ' ' );

   if( !IS_NPC( ch ) && ch->pcdata->condition[COND_DRUNK] > 10 )
      send_to_char( "You are drunk.\n\r", ch );
   if( !IS_NPC( ch ) && ch->pcdata->condition[COND_THIRST] == 0 )
      send_to_char( "You are in danger of dehydrating.\n\r", ch );
   if( !IS_NPC( ch ) && ch->pcdata->condition[COND_FULL] == 0 )
      send_to_char( "You are starving to death.\n\r", ch );
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
   else if( ch->mental_state > 45 )
      send_to_char( "Your sleep is filled with strange and vivid dreams.\n\r", ch );
   else if( ch->mental_state > 25 )
      send_to_char( "Your sleep is uneasy.\n\r", ch );
   else if( ch->mental_state < -35 )
      send_to_char( "You are deep in a much needed sleep.\n\r", ch );
   else if( ch->mental_state < -25 )
      send_to_char( "You are in deep slumber.\n\r", ch );

   if( time_info.day == ch->pcdata->day && time_info.month == ch->pcdata->month )
      send_to_char( "Today is your birthday!\n\r", ch );
   else
      ch_printf( ch, "Your birthday is: Day of %s, %d%s day in the Month of %s, in the year %d.\n\r",
                 day_name[ch->pcdata->day % 13], day, suf, month_name[ch->pcdata->month], ch->pcdata->year );

   send_to_char( "Languages: ", ch );
   for( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ )
      if( knows_language( ch, lang_array[iLang], ch ) || ( IS_NPC( ch ) && ch->speaks == 0 ) )
      {
         if( lang_array[iLang] & ch->speaking || ( IS_NPC( ch ) && !ch->speaking ) )
            set_char_color( AT_RED, ch );
         send_to_char( lang_names[iLang], ch );
         send_to_char( " ", ch );
         set_char_color( AT_SCORE4, ch );
      }
   send_to_char( "\n\r", ch );

   if( ch->pcdata->bestowments && ch->pcdata->bestowments[0] != '\0' )
      ch_printf( ch, "You are bestowed with the command(s): %s.\n\r\n\r", ch->pcdata->bestowments );

   if( ch->desc )
   {
      ch_printf( ch, "Terminal Support Detected: MCCP[%s] MSP[%s] MXP[%s]\n\r",
                 ch->desc->can_compress ? "X" : " ",
                 ch->desc->msp_detected ? "X" : " ", ch->desc->mxp_detected ? "X" : " " );

      ch_printf( ch, "Terminal Support In Use  : MCCP[%s] MSP[%s] MXP[%s]\n\r\n\r",
                 ch->desc->can_compress ? "X" : " ", MSP_ON( ch ) ? "X" : " ", MXP_ON( ch ) ? "X" : " " );
   }

   if( CAN_PKILL( ch ) )
   {
      send_to_char( "----------------------------------------------------------------------------\n\r", ch );
      ch_printf( ch, "PKILL DATA:  Pkills (%3.3d)     Illegal Pkills (%3.3d)     Pdeaths (%3.3d)\n\r",
                 ch->pcdata->pkills, ch->pcdata->illegal_pk, ch->pcdata->pdeaths );
   }
   if( ch->pcdata->deity )
   {
      send_to_char( "----------------------------------------------------------------------------\n\r", ch );
      ch_printf( ch, "Deity:  %-20s  Favor: %d\n\r", ch->pcdata->deity->name, ch->pcdata->favor );
   }
   if( ch->pcdata->clan && ch->pcdata->clan->clan_type == CLAN_ORDER )
   {
      send_to_char( "----------------------------------------------------------------------------\n\r", ch );
      ch_printf( ch, "Guild:  %-20s  Guild Mkills:  %-6d   Guild MDeaths:  %-6d\n\r",
                 ch->pcdata->clan->name, ch->pcdata->clan->mkills, ch->pcdata->clan->mdeaths );
   }
   if( IS_IMMORTAL( ch ) )
   {
      send_to_char( "----------------------------------------------------------------------------\n\r", ch );

      ch_printf( ch, "IMMORTAL DATA:  Wizinvis [%s]  Wizlevel (%d)\n\r",
                 IS_PLR_FLAG( ch, PLR_WIZINVIS ) ? "X" : " ", ch->pcdata->wizinvis );

      ch_printf( ch, "Bamfin:  %s\n\r", ( ch->pcdata->bamfin && ch->pcdata->bamfin[0] != '\0' )
                 ? ch->pcdata->bamfin : "appears in a swirling mist." );
      ch_printf( ch, "Bamfout: %s\n\r", ( ch->pcdata->bamfout && ch->pcdata->bamfout[0] != '\0' )
                 ? ch->pcdata->bamfout : "leaves in a swirling mist." );

      /*
       * Area Loaded info - Scryn 8/11
       */
      if( ch->pcdata->area )
         ch_printf( ch, "Vnums: %-5.5d - %-5.5d\n\r", ch->pcdata->area->low_vnum, ch->pcdata->area->hi_vnum );
   }
   if( ch->first_affect )
   {
      int i;
      SKILLTYPE *sktmp;

      i = 0;
      send_to_char( "----------------------------------------------------------------------------\n\r", ch );
      send_to_char( "AFFECT DATA:                            ", ch );
      for( paf = ch->first_affect; paf; paf = paf->next )
      {
         if( ( sktmp = get_skilltype( paf->type ) ) == NULL )
            continue;
         if( ch->level < 20 )
         {
            ch_printf( ch, "[%-34.34s]    ", sktmp->name );
            if( i == 0 )
               i = 1;
            if( ( ++i % 3 ) == 0 )
               send_to_char( "\n\r", ch );
         }
         if( ch->level >= 20 )
         {
            if( paf->modifier == 0 )
               ch_printf( ch, "[%-24.24s;%5d rds]    ", sktmp->name, paf->duration );
            else if( paf->modifier > 999 )
               ch_printf( ch, "[%-15.15s; %5d rds]    ", sktmp->name, paf->duration );
            else
               ch_printf( ch, "[%-11.11s; %+-3.3d; %5d rds]    ", sktmp->name, paf->modifier, paf->duration );
            if( i == 0 )
               i = 1;
            if( ( ++i % 2 ) == 0 )
               send_to_char( "\n\r", ch );
         }
      }
   }
   send_to_char( "\n\r", ch );
   return;
}

void smaug_group( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *gch;
   CHAR_DATA *leader;

   leader = ch->leader ? ch->leader : ch;
   ch_printf( ch, "%s's group:\n\r", leader->name );

   for( gch = first_char; gch; gch = gch->next )
   {
      if( is_same_group( gch, ch ) )
      {
         ch_printf( ch, "%-16s %4d/%4d hp %4d/%4d %s %4d/%4d mv %5d xp\n\r", capitalize( PERS( gch, ch, TRUE ) ),
                    gch->hit, gch->max_hit, gch->mana, gch->max_mana, "mana", gch->move, gch->max_move, gch->exp );
      }
   }
}

void smaug_prac_output( CHAR_DATA * ch, CHAR_DATA * is_at_gm )
{
   int col, sn, is_ok;
   short lasttype, cnt;

   col = cnt = 0;
   lasttype = SKILL_SPELL;
   set_pager_color( AT_MAGIC, ch );

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
         if( col % 3 != 0 )
            send_to_pager( "\n\r", ch );
         send_to_pager( "--------------------------------[Spells]---------------------------------\n\r", ch );
         col = 0;
      }
      if( skill_table[sn]->type != lasttype )
      {
         if( !cnt )
            send_to_pager( "                                (none)\n\r", ch );
         else if( col % 3 != 0 )
            send_to_pager( "\n\r", ch );
         pager_printf( ch, "--------------------------------[%ss]---------------------------------\n\r",
                       skill_tname[skill_table[sn]->type] );
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
          * Modified by Whir to look pretty - 1/30/2003 
          */
         pager_printf( ch, "%26s%s %3d%%  ", skill_table[sn]->name,
                       ( ch->level < skill_table[sn]->skill_level[ch->Class]
                         && ch->level < skill_table[sn]->race_level[ch->race] ) ? "&W*&D" : " ", ch->pcdata->learned[sn] );

         if( ++col % 2 == 0 )
            send_to_pager( "\n\r", ch );
      }
   }

   if( col % 3 != 0 )
      send_to_pager( "\n\r", ch );

   send_to_pager( "&W*&D Indicates a skill you have not acheived the required level for yet.\n\r", ch );
   pager_printf( ch, "You have %d practice sessions left.\n\r", ch->pcdata->practice );
}
