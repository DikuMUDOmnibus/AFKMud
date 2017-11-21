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
 *                   Character saving and loading module                    *
 ****************************************************************************/

#include <string.h>  /* All because of that damn memcpy statement! */
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include "mud.h"
#include "alias.h"
#include "bits.h"
#include "boards.h"
#include "channels.h"
#include "clans.h"
#include "deity.h"
#include "finger.h"

extern FILE *fpArea;

/*
 * Externals
 */
BOARD_DATA *get_board( CHAR_DATA * ch, const char *board_name );
void fwrite_comments( CHAR_DATA * ch, FILE * fp );
void fread_comment( CHAR_DATA * ch, FILE * fp );
void fread_old_comment( CHAR_DATA * ch, FILE * fp );
void removename( char **list, const char *name );
MUD_CHANNEL *find_channel( char *name );
#ifdef I3
void i3init_char( CHAR_DATA * ch );
void i3save_char( CHAR_DATA * ch, FILE * fp );
bool i3load_char( CHAR_DATA * ch, FILE * fp, const char *word );
#endif
#ifdef IMC
void imc_initchar( CHAR_DATA * ch );
bool imc_loadchar( CHAR_DATA * ch, FILE * fp, const char *word );
void imc_savechar( CHAR_DATA * ch, FILE * fp );
#endif
BIT_DATA *find_qbit( int number );
CLAN_DATA *get_clan( char *name );
COUNCIL_DATA *get_council( char *name );
DEITY_DATA *get_deity( char *name );
void fwrite_morph_data( CHAR_DATA * ch, FILE * fp );
void fread_morph_data( CHAR_DATA * ch, FILE * fp );
void load_zonedata( CHAR_DATA * ch, FILE * fp );
void save_zonedata( CHAR_DATA * ch, FILE * fp );
void save_ignores( CHAR_DATA * ch, FILE * fp );
void load_ignores( CHAR_DATA * ch, FILE * fp );
void ClassSpecificStuff( CHAR_DATA * ch );
char *default_fprompt( CHAR_DATA * ch );
char *default_prompt( CHAR_DATA * ch );

/*
 * Increment with every major format change.
 */
#define SAVEVERSION 22
/* Updated to version 4 after addition of alias code - Samson 3-23-98 */
/* Updated to version 5 after installation of color code - Samson */
/* Updated to version 6 for rare item tracking support - Samson */
/* DOTD pfiles saved as version 7 */
/* Updated to version 8 for text based data saving - Samson */
/* Updated to version 9 for new exp tables - Samson 4-30-99 */
/* Updated to version 10 after weapon code updates - Samson 1-15-00 */
/* Updated to version 11 for mv + 50 boost - Samson 4-25-00 */
/* Updated to version 12 for mana recalcs - Samson 1-19-01 */
/* Updated to version 13 to force activation of MSP/MXP for old players - Samson 8-21-01 */
/* Updated to version 14 to force activation of MXP Prompt line - Samson 2-27-02 */
/* Updated to version 15 for new exp system - Samson 12-15-02 */
/* Updated to 16 to award stat gains for old characters - Samson 12-16-02 */
/* Updated to 17 for yet another try at an xp system that doesn't suck - Samson 12-22-02 */
/* Updated to 18 for the reorganized format - Samson 5-16-04 */
/* 19 skipped */
/* Updated to 20: Starting version for official support of AFKMud 2.0 pfiles */
/* Updated to 21 because Samson was stupid and acted hastily before finalizing the bitset conversions 7-8-04 */
/* Updated to 22 for sha256 password conversion */

/*
 * Array to keep track of equipment temporarily.		-Thoric
 */
OBJ_DATA *save_equipment[MAX_WEAR][8];
OBJ_DATA *mob_save_equipment[MAX_WEAR][8];
CHAR_DATA *quitting_char, *loading_char, *saving_char;

int file_ver = SAVEVERSION;

/*
 * Array of containers read for proper re-nesting of objects.
 */
static OBJ_DATA *rgObjNest[MAX_NEST];

/*
 * Un-equip character before saving to ensure proper	-Thoric
 * stats are saved in case of changes to or removal of EQ
 */
void de_equip_char( CHAR_DATA * ch )
{
   OBJ_DATA *obj;
   int x, y;

   for( x = 0; x < MAX_WEAR; x++ )
   {
      for( y = 0; y < MAX_LAYERS; y++ )
      {
         if( IS_NPC( ch ) )
            mob_save_equipment[x][y] = NULL;
         else
            save_equipment[x][y] = NULL;
      }
   }
   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->wear_loc > -1 && obj->wear_loc < MAX_WEAR )
      {
         if( char_ego( ch ) >= item_ego( obj ) )
         {
            for( x = 0; x < MAX_LAYERS; x++ )
            {
               if( IS_NPC( ch ) )
               {
                  if( !mob_save_equipment[obj->wear_loc][x] )
                  {
                     mob_save_equipment[obj->wear_loc][x] = obj;
                     break;
                  }
               }
               else
               {
                  if( !save_equipment[obj->wear_loc][x] )
                  {
                     save_equipment[obj->wear_loc][x] = obj;
                     break;
                  }
               }
            }
            if( x == MAX_LAYERS )
            {
               bug( "%s had on more than %d layers of clothing in one location (%d): %s",
                    ch->name, MAX_LAYERS, obj->wear_loc, obj->name );
            }
         }
         unequip_char( ch, obj );
      }
   }
}

/*
 * Re-equip character					-Thoric
 */
void re_equip_char( CHAR_DATA * ch )
{
   int x, y;

   for( x = 0; x < MAX_WEAR; x++ )
      for( y = 0; y < MAX_LAYERS; y++ )
         if( IS_NPC( ch ) )
         {
            if( mob_save_equipment[x][y] != NULL )
            {
               if( quitting_char != ch )
                  equip_char( ch, mob_save_equipment[x][y], x );
               mob_save_equipment[x][y] = NULL;
            }
            else
               break;
         }
         else
         {
            if( save_equipment[x][y] != NULL )
            {
               if( quitting_char != ch )
                  equip_char( ch, save_equipment[x][y], x );
               save_equipment[x][y] = NULL;
            }
            else
               break;
         }
}

/*
 * Write the char.
 */
void fwrite_char( CHAR_DATA * ch, FILE * fp )
{
   AFFECT_DATA *paf;
   ALIAS_DATA *pal;
   int sn;
   short pos;
   SKILLTYPE *skill = NULL;

   if( IS_NPC( ch ) )
   {
      bug( "%s: NPC save called!", __FUNCTION__ );
      return;
   }

   fprintf( fp, "%s", "#PLAYER\n" );
   fprintf( fp, "Version      %d\n", SAVEVERSION );
   fprintf( fp, "Name         %s~\n", ch->name );
   fprintf( fp, "Password     %s~\n", ch->pcdata->pwd );
   if( ch->chardesc && ch->chardesc[0] != '\0' )
      fprintf( fp, "Description  %s~\n", strip_cr( ch->chardesc ) );
   fprintf( fp, "Sex          %s~\n", npc_sex[ch->sex] );
   fprintf( fp, "Race         %s~\n", npc_race[ch->race] );
   fprintf( fp, "Class        %s~\n", npc_class[ch->Class] );
   if( ch->pcdata->title && ch->pcdata->title[0] != '\0' )
      fprintf( fp, "Title        %s~\n", ch->pcdata->title );
   if( ch->pcdata->rank && ch->pcdata->rank[0] != '\0' )
      fprintf( fp, "Rank         %s~\n", ch->pcdata->rank );
   if( ch->pcdata->bestowments && ch->pcdata->bestowments[0] != '\0' )
      fprintf( fp, "Bestowments  %s~\n", ch->pcdata->bestowments );
   if( ch->pcdata->homepage && ch->pcdata->homepage[0] != '\0' )
      fprintf( fp, "Homepage     %s~\n", ch->pcdata->homepage );
   if( ch->pcdata->email && ch->pcdata->email[0] != '\0' )  /* Samson 4-19-98 */
      fprintf( fp, "Email        %s~\n", ch->pcdata->email );
   fprintf( fp, "Site         %s\n", ch->pcdata->lasthost );
   if( ch->pcdata->icq > 0 )  /* Samson 1-4-99 */
      fprintf( fp, "ICQ          %d\n", ch->pcdata->icq );
   if( ch->pcdata->bio && ch->pcdata->bio[0] != '\0' )
      fprintf( fp, "Bio          %s~\n", strip_cr( ch->pcdata->bio ) );
   if( ch->pcdata->authed_by && ch->pcdata->authed_by[0] != '\0' )
      fprintf( fp, "AuthedBy     %s~\n", ch->pcdata->authed_by );
   if( ch->pcdata->prompt && ch->pcdata->prompt[0] != '\0' )
      fprintf( fp, "Prompt       %s~\n", ch->pcdata->prompt );
   if( ch->pcdata->fprompt && ch->pcdata->fprompt[0] != '\0' )
      fprintf( fp, "FPrompt      %s~\n", ch->pcdata->fprompt );
   if( ch->pcdata->deity_name && ch->pcdata->deity_name[0] != '\0' )
      fprintf( fp, "Deity        %s~\n", ch->pcdata->deity_name );
   if( ch->pcdata->clan_name && ch->pcdata->clan_name[0] != '\0' )
      fprintf( fp, "Clan         %s~\n", ch->pcdata->clan_name );
   if( !xIS_EMPTY( ch->act ) )
      fprintf( fp, "ActFlags     %s~\n", ext_flag_string( &ch->act, plr_flags ) );
   if( !xIS_EMPTY( ch->affected_by ) )
      fprintf( fp, "AffectFlags  %s~\n", ext_flag_string( &ch->affected_by, a_flags ) );
   if( ch->pcdata->flags > 0 )
      fprintf( fp, "PCFlags      %s~\n", flag_string( ch->pcdata->flags, pc_flags ) );
   if( ch->pcdata->chan_listen && ch->pcdata->chan_listen[0] != '\0' )
      fprintf( fp, "Channels     %s~\n", ch->pcdata->chan_listen );
   if( ch->pcdata->release_date )
      fprintf( fp, "Helled       %ld %s~\n", ch->pcdata->release_date, ch->pcdata->helled_by );
   fprintf( fp, "Status       %d %d %d %d %d %d %d\n",
            ch->level, ch->gold, ch->exp, ch->height, ch->weight, ch->spellfail, ch->mental_state );
   fprintf( fp, "Status2      %d %d %d %d %d %d %d %d\n",
            ch->style, ch->pcdata->practice, ch->alignment, ch->pcdata->favor, ch->hitroll, ch->damroll, ch->armor,
            ch->wimpy );
   fprintf( fp, "Configs      %d %d %d %d %d %d %d %d\n", ch->pcdata->pagerlen, -1, ch->speaks, ch->speaking,
            ch->pcdata->timezone, ch->wait, ch->pcdata->interface, ( ch->in_room == get_room_index( ROOM_VNUM_LIMBO )
                                                                     && ch->was_in_room ) ? ch->was_in_room->vnum : ch->
            in_room->vnum );

   /*
    * MOTD times - Samson 12-31-00 
    */
   fprintf( fp, "Motd         %ld %ld\n", ( long int )ch->pcdata->motd, ( long int )ch->pcdata->imotd );

   fprintf( fp, "Age          %d %d %d %d %ld\n",
            ch->pcdata->age_bonus, ch->pcdata->day, ch->pcdata->month, ch->pcdata->year,
            ch->pcdata->played + ( current_time - ch->pcdata->logon ) );

   fprintf( fp, "HpManaMove   %d %d %d %d %d %d\n", ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move );
   fprintf( fp, "Regens       %d %d %d\n", ch->hit_regen, ch->mana_regen, ch->move_regen );

   if( !xIS_EMPTY( ch->no_affected_by ) )
      fprintf( fp, "NoAffectedBy %s\n", print_bitvector( &ch->no_affected_by ) );

   pos = ch->position;
   if( pos > POS_SITTING && pos < POS_STANDING )
      pos = POS_STANDING;
   fprintf( fp, "Position     %s~\n", npc_position[pos] );
   /*
    * Overland Map - Samson 7-31-99 
    */
   fprintf( fp, "Coordinates  %d %d %d\n", ch->x, ch->y, ch->map );
   fprintf( fp, "SavingThrows %d %d %d %d %d\n",
            ch->saving_poison_death, ch->saving_wand, ch->saving_para_petri, ch->saving_breath, ch->saving_spell_staff );
   fprintf( fp, "RentData     %d %d %d %d %d\n",
            ch->pcdata->balance, ch->pcdata->rent, ch->pcdata->norares, ch->pcdata->autorent, ch->pcdata->camp );
   /*
    * Recall code update to recall to last inn rented at - Samson 12-20-00 
    */
   fprintf( fp, "RentRooms    %d %d %d\n", ch->pcdata->alsherok, ch->pcdata->eletar, ch->pcdata->alatia );
   fprintf( fp, "KillInfo     %d %d %d %d %d\n",
            ch->pcdata->pkills, ch->pcdata->pdeaths, ch->pcdata->mkills, ch->pcdata->mdeaths, ch->pcdata->illegal_pk );

   if( !xIS_EMPTY( ch->resistant ) )
      fprintf( fp, "Resistant    %s~\n", ext_flag_string( &ch->resistant, ris_flags ) );
   if( !xIS_EMPTY( ch->no_resistant ) )
      fprintf( fp, "Nores        %s~\n", ext_flag_string( &ch->no_resistant, ris_flags ) );
   if( !xIS_EMPTY( ch->susceptible ) )
      fprintf( fp, "Susceptible  %s~\n", ext_flag_string( &ch->susceptible, ris_flags ) );
   if( !xIS_EMPTY( ch->no_susceptible ) )
      fprintf( fp, "Nosusc       %s~\n", ext_flag_string( &ch->no_susceptible, ris_flags ) );
   if( !xIS_EMPTY( ch->immune ) )
      fprintf( fp, "Immune       %s~\n", ext_flag_string( &ch->immune, ris_flags ) );
   if( !xIS_EMPTY( ch->no_immune ) )
      fprintf( fp, "Noimm        %s~\n", ext_flag_string( &ch->no_immune, ris_flags ) );
   if( !xIS_EMPTY( ch->absorb ) )
      fprintf( fp, "Absorb       %s~\n", ext_flag_string( &ch->absorb, ris_flags ) );

   if( get_timer( ch, TIMER_PKILLED ) && ( get_timer( ch, TIMER_PKILLED ) > 0 ) )
      fprintf( fp, "PTimer       %d\n", get_timer( ch, TIMER_PKILLED ) );

   fprintf( fp, "AttrPerm     %d %d %d %d %d %d %d\n",
            ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con, ch->perm_cha, ch->perm_lck );

   fprintf( fp, "AttrMod      %d %d %d %d %d %d %d\n",
            ch->mod_str, ch->mod_int, ch->mod_wis, ch->mod_dex, ch->mod_con, ch->mod_cha, ch->mod_lck );

   fprintf( fp, "Condition    %d %d %d %d\n",
            ch->pcdata->condition[0], ch->pcdata->condition[1], ch->pcdata->condition[2], ch->pcdata->condition[3] );

   if( IS_IMMORTAL( ch ) )
   {
      if( ch->pcdata->council )
         fprintf( fp, "Council      %s~\n", ch->pcdata->council_name );
      if( ch->pcdata->bamfin && ch->pcdata->bamfin[0] != '\0' )
         fprintf( fp, "Bamfin       %s~\n", ch->pcdata->bamfin );
      if( ch->pcdata->bamfout && ch->pcdata->bamfout[0] != '\0' )
         fprintf( fp, "Bamfout      %s~\n", ch->pcdata->bamfout );
      fprintf( fp, "ImmData      %d %ld %d %d %d %d\n",
               ch->trust, ch->pcdata->restore_time, ch->pcdata->wizinvis,
               ch->pcdata->low_vnum, ch->pcdata->hi_vnum, ch->pcdata->realm );
   }

   for( sn = 1; sn < top_sn; sn++ )
   {
      if( skill_table[sn]->name && ch->pcdata->learned[sn] > 0 )
      {
         switch ( skill_table[sn]->type )
         {
            default:
               fprintf( fp, "Skill        %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_SPELL:
               fprintf( fp, "Spell        %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_WEAPON:
               fprintf( fp, "Weapon       %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_TONGUE:
               fprintf( fp, "Tongue       %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_RACIAL:
               fprintf( fp, "Ability      %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_LORE:
               fprintf( fp, "Lore         %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
         }
      }
   }

   for( paf = ch->first_affect; paf; paf = paf->next )
   {
      if( paf->type >= 0 && !( skill = get_skilltype( paf->type ) ) )
         continue;

      if( paf->type >= 0 && paf->type < TYPE_PERSONAL )
         fprintf( fp, "AffectData   '%s' %3d %3d %3d %d\n",
                  skill->name, paf->duration, paf->modifier, paf->location, paf->bit );
      else
      {
         if( paf->location == APPLY_AFFECT )
            fprintf( fp, "Affect %s '%s' %d %d %d\n",
                     a_types[paf->location], a_flags[paf->modifier], paf->type, paf->duration, paf->bit );
         else if( paf->location == APPLY_WEAPONSPELL
                  || paf->location == APPLY_WEARSPELL
                  || paf->location == APPLY_REMOVESPELL
                  || paf->location == APPLY_STRIPSN
                  || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
            fprintf( fp, "Affect %s '%s' %d %d %d\n", a_types[paf->location],
                     IS_VALID_SN( paf->modifier ) ? skill_table[paf->modifier]->name : "UNKNOWN",
                     paf->type, paf->duration, paf->bit );
         else if( paf->location == APPLY_RESISTANT
                  || paf->location == APPLY_IMMUNE || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
            fprintf( fp, "Affect %s %s~ %d %d %d\n",
                     a_types[paf->location], ext_flag_string( &paf->rismod, ris_flags ), paf->type, paf->duration,
                     paf->bit );
         else
            fprintf( fp, "Affect %s %d %d %d %d\n",
                     a_types[paf->location], paf->modifier, paf->type, paf->duration, paf->bit );
      }
   }

   /*
    * Save color values - Samson 9-29-98 
    */
   {
      int x;
      fprintf( fp, "MaxColors    %d\n", MAX_COLORS );
      fprintf( fp, "%s", "Colors       " );
      for( x = 0; x < MAX_COLORS; x++ )
         fprintf( fp, "%d ", ch->pcdata->colors[x] );
      fprintf( fp, "%s", "\n" );
   }

   /*
    * Save recall beacons - Samson 2-7-99 
    */
   {
      int x;
      fprintf( fp, "MaxBeacons   %d\n", MAX_BEACONS );
      fprintf( fp, "%s", "Beacons      " );
      for( x = 0; x < MAX_BEACONS; x++ )
         fprintf( fp, "%d ", ch->pcdata->beacon[x] );
      fprintf( fp, "%s", "\n" );
   }

   for( pal = ch->pcdata->first_alias; pal; pal = pal->next )
   {
      if( !pal->name || !pal->cmd || !*pal->name || !*pal->cmd )
         continue;
      fprintf( fp, "Alias        %s~ %s~\n", pal->name, pal->cmd );
   }

   if( ch->pcdata->first_boarddata )
   {
      BOARD_CHARDATA *chboard, *chboard_next;

      for( chboard = ch->pcdata->first_boarddata; chboard; chboard = chboard_next )
      {
         chboard_next = chboard->next;

         /*
          * Ugh.. is it worth saving that extra board_chardata field on pcdata? 
          */
         /*
          * No, Xorith, it wasn't. So I changed it - Samson 10-15-03 
          */
         if( !chboard->board_name )
         {
            UNLINK( chboard, ch->pcdata->first_boarddata, ch->pcdata->last_boarddata, next, prev );
            STRFREE( chboard->board_name );
            DISPOSE( chboard );
            continue;
         }
         fprintf( fp, "Board_Data   %s~ %ld %d\n", chboard->board_name, ( long int )chboard->last_read, chboard->alert );
      }
   }

   save_zonedata( ch, fp );
   save_ignores( ch, fp );

   if( ch->pcdata->first_qbit )
   {
      BIT_DATA *bit;

      for( bit = ch->pcdata->first_qbit; bit; bit = bit->next )
         fprintf( fp, "Qbit        %d %s~\n", bit->number, bit->desc );
   }
#ifdef I3
   i3save_char( ch, fp );
#endif
#ifdef IMC
   imc_savechar( ch, fp );
#endif
   fprintf( fp, "%s", "End\n\n" );
   return;
}

/*
 * Write an object and its contents.
 */
void fwrite_obj( CHAR_DATA * ch, OBJ_DATA * obj, CLAN_DATA * clan, FILE * fp, int iNest, short os_type, bool hotboot )
{
   EXTRA_DESCR_DATA *ed;
   AFFECT_DATA *paf;
   short wear, wear_loc, x;

   if( iNest >= MAX_NEST )
   {
      bug( "%s: iNest hit MAX_NEST %d", __FUNCTION__, iNest );
      return;
   }

   if( !obj )
   {
      bug( "%s: NULL obj", __FUNCTION__ );
      return;
   }

   /*
    * Slick recursion to write lists backwards,
    *   so loading them will load in forwards order.
    */
   if( obj->prev_content && os_type != OS_CORPSE )
      if( os_type == OS_CARRY )
         fwrite_obj( ch, obj->prev_content, clan, fp, iNest, OS_CARRY, hotboot );

   /*
    * Castrate storage characters.
    * Catch deleted objects                                    -Thoric
    * Do NOT save prototype items!          -Thoric
    * But bypass this if it's a hotboot because you want it to stay on the ground. - Samson
    */
   if( !hotboot )
   {
      if( ( obj->item_type == ITEM_KEY && !IS_OBJ_FLAG( obj, ITEM_CLANOBJECT ) )
          || obj_extracted( obj ) || IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) || obj->rent == -1 )
         return;
   }

   /*
    * Catch rent items going into auction houses - Samson 6-23-99 
    */
   if( ch )
   {
      if( ( IS_ACT_FLAG( ch, ACT_AUCTION ) ) && obj->rent >= sysdata.minrent )
         return;
   }

   /*
    * Catch rent items going into clan storerooms - Samson 2-3-01 
    */
   if( clan && ch && ch->in_room )
   {
      if( obj->rent >= sysdata.minrent && ch->in_room->vnum == clan->storeroom )
         return;
   }

   /*
    * DO NOT save corpses lying on the ground as a hotboot item, they already saved elsewhere! - Samson 
    */
   if( hotboot && obj->item_type == ITEM_CORPSE_PC )
      return;

   /*
    * Corpse saving. -- Altrag 
    */
   fprintf( fp, "%s", ( os_type == OS_CORPSE ? "#CORPSE\n" : "#OBJECT\n" ) );
   fprintf( fp, "Version      %d\n", SAVEVERSION );
   if( iNest )
      fprintf( fp, "Nest         %d\n", iNest );
   if( obj->count > 1 )
      fprintf( fp, "Count        %d\n", obj->count );
   if( obj->name && ( !obj->pIndexData->name  || str_cmp( obj->name, obj->pIndexData->name ) ) )
      fprintf( fp, "Name         %s~\n", obj->name );
   if( obj->short_descr && ( !obj->pIndexData->short_descr || str_cmp( obj->short_descr, obj->pIndexData->short_descr ) ) )
      fprintf( fp, "ShortDescr   %s~\n", obj->short_descr );
   if( obj->objdesc && ( !obj->pIndexData->objdesc || str_cmp( obj->objdesc, obj->pIndexData->objdesc ) ) )
      fprintf( fp, "Description  %s~\n", obj->objdesc );
   if( obj->action_desc && ( !obj->pIndexData->action_desc || str_cmp( obj->action_desc, obj->pIndexData->action_desc ) ) )
      fprintf( fp, "ActionDesc   %s~\n", obj->action_desc );
   fprintf( fp, "Ovnum         %d\n", obj->pIndexData->vnum );
   fprintf( fp, "Rent         %d\n", obj->rent );
   if( ( os_type == OS_CORPSE || hotboot ) && obj->in_room )
   {
      fprintf( fp, "Room         %d\n", obj->in_room->vnum );
      fprintf( fp, "Rvnum	   %d\n", obj->room_vnum );
   }
   if( !xSAME_BITS( obj->extra_flags, obj->pIndexData->extra_flags ) )
      fprintf( fp, "ExtraFlags   %s~\n", ext_flag_string( &obj->extra_flags, o_flags ) );
   if( obj->magic_flags != obj->pIndexData->magic_flags )
      fprintf( fp, "MagicFlags   %s~\n", flag_string( obj->magic_flags, mag_flags ) );
   if( obj->wear_flags != obj->pIndexData->wear_flags )
      fprintf( fp, "WearFlags    %s~\n", flag_string( obj->wear_flags, w_flags ) );
   wear_loc = -1;
   for( wear = 0; wear < MAX_WEAR; wear++ )
      for( x = 0; x < MAX_LAYERS; x++ )
         if( ch )
         {
            if( IS_NPC( ch ) )
            {
               if( obj == mob_save_equipment[wear][x] )
               {
                  wear_loc = wear;
                  break;
               }
               else if( !mob_save_equipment[wear][x] )
                  break;
            }
            else
            {
               if( obj == save_equipment[wear][x] )
               {
                  wear_loc = wear;
                  break;
               }
               else if( !save_equipment[wear][x] )
                  break;
            }
         }
   if( wear_loc != -1 )
      fprintf( fp, "WearLoc      %d\n", wear_loc );
   if( obj->item_type != obj->pIndexData->item_type )
      fprintf( fp, "ItemType     %d\n", obj->item_type );
   if( obj->weight != obj->pIndexData->weight )
      fprintf( fp, "Weight       %d\n", obj->weight );
   if( obj->level )
      fprintf( fp, "Level        %d\n", obj->level );
   if( obj->timer )
      fprintf( fp, "Timer        %d\n", obj->timer );
   if( obj->cost != obj->pIndexData->cost )
      fprintf( fp, "Cost         %d\n", obj->cost );
   if( obj->seller != NULL )
      fprintf( fp, "Seller	%s~\n", obj->seller );
   if( obj->buyer != NULL )
      fprintf( fp, "Buyer   %s~\n", obj->buyer );
   if( obj->bid != 0 )
      fprintf( fp, "Bid	%d\n", obj->bid );
   if( obj->owner != NULL )
      fprintf( fp, "Owner	%s~\n", obj->owner );
   fprintf( fp, "Oday		%d\n", obj->day );
   fprintf( fp, "Omonth	%d\n", obj->month );
   fprintf( fp, "Oyear		%d\n", obj->year );
   fprintf( fp, "Coords	%d %d %d\n", obj->x, obj->y, obj->map );
   fprintf( fp, "Values      %d %d %d %d %d %d %d %d %d %d %d\n",
            obj->value[0], obj->value[1], obj->value[2], obj->value[3],
            obj->value[4], obj->value[5], obj->value[6], obj->value[7], obj->value[8], obj->value[9], obj->value[10] );
   fprintf( fp, "Sockets     %s %s %s\n",
            obj->socket[0] ? obj->socket[0] : "None",
            obj->socket[1] ? obj->socket[1] : "None", obj->socket[2] ? obj->socket[2] : "None" );

   switch ( obj->item_type )
   {
      case ITEM_PILL:  /* was down there with staff and wand, wrongly - Scryn */
      case ITEM_POTION:
      case ITEM_SCROLL:
         if( IS_VALID_SN( obj->value[1] ) )
            fprintf( fp, "Spell 1      '%s'\n", skill_table[obj->value[1]]->name );
         if( IS_VALID_SN( obj->value[2] ) )
            fprintf( fp, "Spell 2      '%s'\n", skill_table[obj->value[2]]->name );
         if( IS_VALID_SN( obj->value[3] ) )
            fprintf( fp, "Spell 3      '%s'\n", skill_table[obj->value[3]]->name );
         break;

      case ITEM_STAFF:
      case ITEM_WAND:
         if( IS_VALID_SN( obj->value[3] ) )
            fprintf( fp, "Spell 3      '%s'\n", skill_table[obj->value[3]]->name );
         break;

      case ITEM_SALVE:
         if( IS_VALID_SN( obj->value[4] ) )
            fprintf( fp, "Spell 4      '%s'\n", skill_table[obj->value[4]]->name );
         if( IS_VALID_SN( obj->value[5] ) )
            fprintf( fp, "Spell 5      '%s'\n", skill_table[obj->value[5]]->name );
         break;
   }

   for( paf = obj->first_affect; paf; paf = paf->next )
   {
      /*
       * Save extra object affects - Thoric
       */
      if( paf->type < 0 || paf->type >= top_sn )
      {
         fprintf( fp, "Affect       %d %d %d %d %d\n",
                  paf->type, paf->duration,
                  ( ( paf->location == APPLY_WEAPONSPELL
                      || paf->location == APPLY_WEARSPELL
                      || paf->location == APPLY_REMOVESPELL
                      || paf->location == APPLY_STRIPSN
                      || paf->location == APPLY_RECURRINGSPELL )
                    && IS_VALID_SN( paf->modifier ) )
                  ? skill_table[paf->modifier]->slot : paf->modifier, paf->location, paf->bit );
      }
      else
         fprintf( fp, "AffectData   '%s' %d %d %d %d\n",
                  skill_table[paf->type]->name, paf->duration,
                  ( ( paf->location == APPLY_WEAPONSPELL
                      || paf->location == APPLY_WEARSPELL
                      || paf->location == APPLY_REMOVESPELL
                      || paf->location == APPLY_STRIPSN
                      || paf->location == APPLY_RECURRINGSPELL )
                    && IS_VALID_SN( paf->modifier ) )
                  ? skill_table[paf->modifier]->slot : paf->modifier, paf->location, paf->bit );
   }

   for( ed = obj->first_extradesc; ed; ed = ed->next )
   {
      if( ed->extradesc && ed->extradesc[0] != '\0' )
         fprintf( fp, "ExtraDescr   %s~ %s~\n", ed->keyword, ed->extradesc );
      else
         fprintf( fp, "ExtraDescr   %s~ ~\n", ed->keyword );
   }

   fprintf( fp, "%s", "End\n\n" );

   if( obj->first_content )
      fwrite_obj( ch, obj->last_content, clan, fp, iNest + 1, OS_CARRY, hotboot );

   return;
}

/*
 * This will write one mobile structure pointed to be fp --Shaddai
 *   Edited by Tarl 5 May 2002 to allow pets to save items.
*/
void fwrite_mobile( CHAR_DATA * mob, FILE * fp, bool shopmob )
{
   AFFECT_DATA *paf;
   SKILLTYPE *skill = NULL;
   OBJ_DATA *obj, *obj_next;

   if( !IS_NPC( mob ) || !fp )
      return;

   fprintf( fp, "%s", shopmob ? "#SHOP\n" : "#MOBILE\n" );
   fprintf( fp, "Vnum    %d\n", mob->pIndexData->vnum );
   fprintf( fp, "Level   %d\n", mob->level );
   if( mob->in_room )
      fprintf( fp, "Room      %d\n", mob->in_room->vnum );
   else
      fprintf( fp, "Room      %d\n", ROOM_VNUM_ALTAR );
   fprintf( fp, "Coordinates  %d %d %d\n", mob->x, mob->y, mob->map );
   if( mob->name && mob->pIndexData->player_name && str_cmp( mob->name, mob->pIndexData->player_name ) )
      fprintf( fp, "Name     %s~\n", mob->name );
   if( mob->short_descr && mob->pIndexData->short_descr && str_cmp( mob->short_descr, mob->pIndexData->short_descr ) )
      fprintf( fp, "Short	%s~\n", mob->short_descr );
   if( mob->long_descr && mob->pIndexData->long_descr && str_cmp( mob->long_descr, mob->pIndexData->long_descr ) )
      fprintf( fp, "Long	%s~\n", mob->long_descr );
   if( mob->chardesc && mob->pIndexData->chardesc && str_cmp( mob->chardesc, mob->pIndexData->chardesc ) )
      fprintf( fp, "Description %s~\n", mob->chardesc );
   fprintf( fp, "Position        %d\n", mob->position );
   if( IS_ACT_FLAG( mob, ACT_MOUNTED ) )
      REMOVE_ACT_FLAG( mob, ACT_MOUNTED );
   fprintf( fp, "Flags           %s\n", print_bitvector( &mob->act ) );
   if( !xIS_EMPTY( mob->affected_by ) )
      fprintf( fp, "AffectedBy   %s\n", print_bitvector( &mob->affected_by ) );

   for( paf = mob->first_affect; paf; paf = paf->next )
   {
      if( paf->type >= 0 && !( skill = get_skilltype( paf->type ) ) )
         continue;

      if( paf->type >= 0 && paf->type < TYPE_PERSONAL )
         fprintf( fp, "AffectData   '%s' %3d %3d %3d %d\n",
                  skill->name, paf->duration, paf->modifier, paf->location, paf->bit );
      else
         fprintf( fp, "Affect       %3d %3d %3d %3d %d\n",
                  paf->type, paf->duration, paf->modifier, paf->location, paf->bit );
   }
   fprintf( fp, "HpManaMove   %d %d %d %d %d %d\n", mob->hit, mob->max_hit, mob->mana, mob->max_mana, mob->move,
            mob->max_move );
   fprintf( fp, "Exp          %d\n", mob->exp );
   obj = mob->first_carrying;
   if( obj != NULL )
   {
      for( obj = mob->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         if( obj->rent >= sysdata.minrent )
            extract_obj( obj );
      }
   }

   if( shopmob )
      fprintf( fp, "%s", "EndVendor\n\n" );

   de_equip_char( mob );
   if( mob->first_carrying )
      fwrite_obj( mob, mob->last_carrying, NULL, fp, 0, OS_CARRY, mob->master->pcdata->hotboot );
   re_equip_char( mob );

   if( !shopmob )
      fprintf( fp, "%s", "EndMobile\n\n" );
   return;
}

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes, some of the infrastructure is provided.
 */
void save_char_obj( CHAR_DATA * ch )
{
   char strsave[256], strback[256];
   FILE *fp;

   if( !ch )
   {
      bug( "%s", "Save_char_obj: null ch!" );
      return;
   }

   if( IS_NPC( ch ) )
      return;

   saving_char = ch;

   if( ch->desc && ch->desc->original )
      ch = ch->desc->original;

   de_equip_char( ch );

   ch->pcdata->save_time = current_time;
   snprintf( strsave, 256, "%s%c/%s", PLAYER_DIR, tolower( ch->pcdata->filename[0] ), capitalize( ch->pcdata->filename ) );

   /*
    * Save immortal stats, level & vnums for wizlist    -Thoric
    * and do_vnums command
    *
    * Also save the player flags so we the wizlist builder can see
    * who is a guest and who is retired.
    */
   if( ch->level >= LEVEL_IMMORTAL )
   {
      snprintf( strback, 256, "%s%s", GOD_DIR, capitalize( ch->pcdata->filename ) );

      if( !( fp = fopen( strback, "w" ) ) )
      {
         perror( strsave );
         bug( "%s", "Save_god_level: fopen" );
      }
      else
      {
         fprintf( fp, "Level        %d\n", ch->level );
         fprintf( fp, "Pcflags      %d\n", ch->pcdata->flags );
         if( ch->pcdata->homepage && ch->pcdata->homepage[0] != '\0' )
            fprintf( fp, "Homepage    %s~\n", ch->pcdata->homepage );
         fprintf( fp, "Realm	   %d\n", ch->pcdata->realm );
         if( ch->pcdata->low_vnum && ch->pcdata->hi_vnum )
            fprintf( fp, "VnumRange    %d %d\n", ch->pcdata->low_vnum, ch->pcdata->hi_vnum );
         if( ch->pcdata->email && ch->pcdata->email[0] != '\0' )
            fprintf( fp, "Email        %s~\n", ch->pcdata->email );
         if( ch->pcdata->icq > 0 )
            fprintf( fp, "ICQ          %d\n", ch->pcdata->icq );
         fprintf( fp, "%s", "End\n" );
         FCLOSE( fp );
      }
   }

   if( !( fp = fopen( strsave, "w" ) ) )
   {
      bug( "%s", "Save_char_obj: fopen" );
      perror( strsave );
   }
   else
   {
      fwrite_char( ch, fp );
      if( ch->morph )
         fwrite_morph_data( ch, fp );
      if( ch->first_carrying )
         fwrite_obj( ch, ch->last_carrying, NULL, fp, 0, OS_CARRY, ch->pcdata->hotboot );

      if( sysdata.save_pets && ch->first_pet )
      {
         CHAR_DATA *pet;

         for( pet = ch->first_pet; pet; pet = pet->next_pet )
            fwrite_mobile( pet, fp, false );
      }

      if( ch->pcdata->first_comment )
         fwrite_comments( ch, fp );
      fprintf( fp, "%s", "#END\n" );
      FCLOSE( fp );
   }

   ClassSpecificStuff( ch );  /* Brought over from DOTD code - Samson 4-6-99 */

   re_equip_char( ch );

   quitting_char = NULL;
   saving_char = NULL;
   return;
}

short find_old_age( CHAR_DATA * ch )
{
   short age;

   if( IS_NPC( ch ) )
      return -1;

   age = ch->pcdata->played / 86400;   /* Calculate realtime number of days played */

   age = age / 7; /* Calculates rough estimate on number of mud years played */

   age += 17;  /* Add 17 years, new characters begin at 17. */

   ch->pcdata->day = ( number_range( 1, sysdata.dayspermonth ) - 1 );   /* Assign random day of birth */
   ch->pcdata->month = ( number_range( 1, sysdata.monthsperyear ) - 1 );   /* Assign random month of birth */
   ch->pcdata->year = time_info.year - age;  /* Assign birth year based on calculations above */

   return age;
}

/*
 * Read in a char.
 */
void fread_char( CHAR_DATA * ch, FILE * fp, bool preload, bool copyover )
{
   char *line;
   const char *word;
   int x1, x2, x3, x4, x5, x6, x7, x8;
   short killcnt;
   bool fMatch;
   int max_colors = 0;  /* Color code */
   int max_beacons = 0; /* Beacon spell */

   file_ver = 0;
   killcnt = 0;

   /*
    * Setup color values in case player has none set - Samson 
    */
   memcpy( &ch->pcdata->colors, &default_set, sizeof( default_set ) );

   for( ;; )
   {
      word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'A':
            if( !str_cmp( word, "Ability" ) )
            {
               char *ability;
               int sn, value;

               value = fread_number( fp );
               ability = fread_word( fp );
               sn = bsearch_skill_exact( ability, gsn_first_ability, gsn_first_lore - 1 );
               if( sn < 0 )
                  log_printf( "Fread_char: unknown ability: %s", ability );
               else
               {
                  ch->pcdata->learned[sn] = value;
                  if( ch->level < LEVEL_IMMORTAL )
                  {
                     if( skill_table[sn]->race_level[ch->race] >= LEVEL_IMMORTAL )
                     {
                        ch->pcdata->learned[sn] = 0;
                        ch->pcdata->practice++;
                     }
                  }
               }
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "Absorb" ) )
            {
               char *absorb = NULL;
               char flag[MIL];
               int value;

               if( file_ver < 21 )
                  ch->absorb = fread_bitvector( fp );
               else
               {
                  absorb = fread_flagstring( fp );

                  while( absorb[0] != '\0' )
                  {
                     absorb = one_argument( absorb, flag );
                     value = get_risflag( flag );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "Unknown RIS flag (A): %s", flag );
                     else
                        SET_ABSORB( ch, value );
                  }
               }
               fMatch = TRUE;
               break;
            }
            /*
             * ACT: OLD FIELD 
             */
            KEY( "Act", ch->act, fread_bitvector( fp ) );
            if( !str_cmp( word, "ActFlags" ) )
            {
               char *actflags = NULL;
               char flag[MIL];
               int value;

               actflags = fread_flagstring( fp );
               while( actflags[0] != '\0' )
               {
                  actflags = one_argument( actflags, flag );
                  if( file_ver == 18 )
                     value = get_actflag( flag );
                  else
                     value = get_plrflag( flag );
                  if( value < 0 || value > MAX_BITS )
                     bug( "Unknown actflag: %s\n\r", flag );
                  else
                     SET_PLR_FLAG( ch, value );
               }
               fMatch = TRUE;
               break;
            }
            /*
             * AFFECTEDBY: OLD FIELD 
             */
            KEY( "AffectedBy", ch->affected_by, fread_bitvector( fp ) );
            if( !str_cmp( word, "AffectFlags" ) )
            {
               char *affflags = NULL;
               char flag[MIL];
               int value;

               affflags = fread_flagstring( fp );
               while( affflags[0] != '\0' )
               {
                  affflags = one_argument( affflags, flag );
                  value = get_aflag( flag );
                  if( value < 0 || value > MAX_BITS )
                     bug( "Unknown affect flag: %s\n\r", flag );
                  else
                     SET_AFFECTED( ch, value );
               }
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "Age" ) )
            {
               time_t xx5 = 0;
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = 0;
               sscanf( line, "%d %d %d %d %ld", &x1, &x2, &x3, &x4, &xx5 );
               ch->pcdata->age_bonus = x1;
               ch->pcdata->day = x2;
               ch->pcdata->month = x3;
               ch->pcdata->year = x4;
               ch->pcdata->played = xx5;
               fMatch = TRUE;
               break;
            }
            /*
             * AGEMOD/ALIGNMENT/ARMOR: OLD FIELDS 
             */
            KEY( "Agemod", ch->pcdata->age_bonus, fread_number( fp ) );
            KEY( "Alignment", ch->alignment, fread_number( fp ) );
            KEY( "Armor", ch->armor, fread_number( fp ) );

            if( !str_cmp( word, "Affect" ) || !str_cmp( word, "AffectData" ) )
            {
               AFFECT_DATA *paf;

               CREATE( paf, AFFECT_DATA, 1 );
               paf->type = -1;
               paf->duration = -1;
               paf->bit = 0;
               paf->modifier = 0;
               xCLEAR_BITS( paf->rismod );

               if( !str_cmp( word, "Affect" ) )
               {
                  char *loc = NULL;
                  char *aff = NULL;
                  char *risa = NULL;
                  char flag[MIL];
                  int value;

                  loc = fread_word( fp );
                  value = get_atype( loc );
                  if( value < 0 || value >= MAX_APPLY_TYPE )
                     bug( "%s: Invalid apply type: %s", __FUNCTION__, loc );
                  else
                     paf->location = value;

                  if( paf->location == APPLY_WEAPONSPELL
                      || paf->location == APPLY_WEARSPELL
                      || paf->location == APPLY_REMOVESPELL
                      || paf->location == APPLY_STRIPSN
                      || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
                     paf->modifier = skill_lookup( fread_word( fp ) );
                  else if( paf->location == APPLY_AFFECT )
                  {
                     aff = fread_word( fp );
                     value = get_aflag( aff );
                     if( value < 0 || value >= MAX_AFFECTED_BY )
                        bug( "%s: Unsupportable value for affect flag: %s", __FUNCTION__, aff );
                     else
                        paf->modifier = value;
                  }
                  else if( paf->location == APPLY_RESISTANT
                           || paf->location == APPLY_IMMUNE
                           || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
                  {
                     risa = fread_flagstring( fp );

                     while( risa[0] != '\0' )
                     {
                        risa = one_argument( risa, flag );
                        value = get_risflag( flag );
                        if( value < 0 || value >= MAX_RIS_FLAG )
                           bug( "%s: Unsupportable value for RISA flag: %s", __FUNCTION__, flag );
                        else
                           xSET_BIT( paf->rismod, value );
                     }
                  }
                  else
                     paf->modifier = fread_number( fp );
                  paf->type = fread_short( fp );
                  paf->duration = fread_number( fp );
                  paf->bit = fread_number( fp );
                  if( paf->bit >= MAX_AFFECTED_BY )
                  {
                     DISPOSE( paf );
                     fMatch = true;
                     break;
                  }
               }
               else
               {
                  int sn;
                  char *sname = fread_word( fp );

                  if( ( sn = skill_lookup( sname ) ) < 0 )
                  {
                     if( ( sn = herb_lookup( sname ) ) < 0 )
                        log_printf( "%s: unknown skill.", __FUNCTION__ );
                     else
                        sn += TYPE_HERB;
                  }
                  paf->type = sn;
                  paf->duration = fread_number( fp );
                  paf->modifier = fread_number( fp );
                  paf->location = fread_number( fp );
                  if( paf->location == APPLY_WEAPONSPELL
                      || paf->location == APPLY_WEARSPELL
                      || paf->location == APPLY_REMOVESPELL
                      || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL )
                     paf->modifier = slot_lookup( paf->modifier );
                  paf->bit = fread_number( fp );
                  if( paf->bit >= MAX_AFFECTED_BY )
                  {
                     DISPOSE( paf );
                     fMatch = true;
                     break;
                  }
               }
               LINK( paf, ch->first_affect, ch->last_affect, next, prev );
               fMatch = true;
               break;
            }

            if( !str_cmp( word, "AttrMod" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = 13;
               sscanf( line, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
               ch->mod_str = x1;
               ch->mod_int = x2;
               ch->mod_wis = x3;
               ch->mod_dex = x4;
               ch->mod_con = x5;
               ch->mod_cha = x6;
               ch->mod_lck = x7;
               if( !x7 )
                  ch->mod_lck = 0;
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Alias" ) )
            {
               ALIAS_DATA *pal;

               CREATE( pal, ALIAS_DATA, 1 );
               pal->name = fread_string( fp );
               pal->cmd = fread_string( fp );
               LINK( pal, ch->pcdata->first_alias, ch->pcdata->last_alias, next, prev );
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "AttrPerm" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
               sscanf( line, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
               ch->perm_str = x1;
               ch->perm_int = x2;
               ch->perm_wis = x3;
               ch->perm_dex = x4;
               ch->perm_con = x5;
               ch->perm_cha = x6;
               ch->perm_lck = x7;
               if( !x7 || x7 == 0 )
                  ch->perm_lck = 13;
               fMatch = TRUE;
               break;
            }
            KEY( "AuthedBy", ch->pcdata->authed_by, fread_string( fp ) );
            break;

         case 'B':
            /*
             * BALANCE: OLD FIELD 
             */
            KEY( "Balance", ch->pcdata->balance, fread_number( fp ) );
            KEY( "Bamfin", ch->pcdata->bamfin, fread_string_nohash( fp ) );
            KEY( "Bamfout", ch->pcdata->bamfout, fread_string_nohash( fp ) );

            /*
             * Load beacons - Samson 9-29-98 
             */
            if( !str_cmp( word, "Beacons" ) )
            {
               int x;

               for( x = 0; x < max_beacons; x++ )
                  ch->pcdata->beacon[x] = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            /*
             * BDAY/BMONTH/BYEAR: OLD FIELDS 
             */
            KEY( "Bday", ch->pcdata->day, fread_number( fp ) );
            KEY( "Bestowments", ch->pcdata->bestowments, fread_string_nohash( fp ) );
            KEY( "Bio", ch->pcdata->bio, fread_string_nohash( fp ) );
            KEY( "Bmonth", ch->pcdata->month, fread_number( fp ) );
            KEY( "Byear", ch->pcdata->year, fread_number( fp ) );
            if( !str_cmp( word, "Board_Data" ) )
            {
               BOARD_CHARDATA *pboard;
               BOARD_DATA *board;

               word = fread_flagstring( fp );
               if( !( board = get_board( NULL, word ) ) )
               {
                  log_printf( "Player %s has board %s which apparently doesn't exist?", ch->name, word );
                  ch_printf( ch, "Warning: the board %s no longer exsists.\n\r", word );
                  fread_to_eol( fp );
                  fMatch = TRUE;
                  break;
               }
               CREATE( pboard, BOARD_CHARDATA, 1 );
               pboard->board_name = QUICKLINK( board->name );
               pboard->last_read = fread_long( fp );
               pboard->alert = fread_number( fp );
               LINK( pboard, ch->pcdata->first_boarddata, ch->pcdata->last_boarddata, next, prev );
               fMatch = TRUE;
               break;
            }
            break;

         case 'C':
            KEY( "Channels", ch->pcdata->chan_listen, fread_string( fp ) );
            if( !str_cmp( word, "Clan" ) )
            {
               ch->pcdata->clan_name = fread_string( fp );

               if( !preload && ch->pcdata->clan_name[0] != '\0'
                   && !( ch->pcdata->clan = get_clan( ch->pcdata->clan_name ) ) )
               {
                  ch_printf( ch,
                             "Warning: the organization %s no longer exists, and therefore you no longer\n\rbelong to that organization.\n\r",
                             ch->pcdata->clan_name );
                  STRFREE( ch->pcdata->clan_name );
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Class" ) )
            {
               int Class;

               Class = get_npc_class( fread_flagstring( fp ) );

               if( Class < 0 || Class >= MAX_NPC_CLASS )
               {
                  bug( "fread_char: Player %s has invalid Class! Defaulting to warrior.", ch->name );
                  Class = CLASS_WARRIOR;
               }
               ch->Class = Class;
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Condition" ) )
            {
               line = fread_line( fp );
               sscanf( line, "%d %d %d %d", &x1, &x2, &x3, &x4 );
               ch->pcdata->condition[0] = x1;
               ch->pcdata->condition[1] = x2;
               ch->pcdata->condition[2] = x3;
               ch->pcdata->condition[3] = x4;
               fMatch = TRUE;
               break;
            }

            /*
             * Load color values - Samson 9-29-98 
             */
            if( !str_cmp( word, "Colors" ) )
            {
               int x;

               for( x = 0; x < max_colors; x++ )
                  ch->pcdata->colors[x] = fread_number( fp );
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Configs" ) )
            {
               ROOM_INDEX_DATA *temp;

               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = 0;
               sscanf( line, "%d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8 );
               ch->pcdata->pagerlen = x1;
               ch->speaks = x3;
               ch->speaking = x4;
               ch->pcdata->timezone = x5;
               ch->wait = x6;
               ch->pcdata->interface = x7;

               temp = get_room_index( x8 );

               if( !temp )
                  temp = get_room_index( ROOM_VNUM_TEMPLE );

               if( !temp )
                  temp = get_room_index( ROOM_VNUM_LIMBO );

               /*
                * Um, yeah. If this happens you're shit out of luck! 
                */
               if( !temp )
               {
                  bug( "%s", "FATAL: No valid fallback rooms. Program terminating!" );
                  exit( 1 );
               }
               /*
                * And you're going to crash if the above check failed, because you're an idiot if you remove this Vnum 
                */
               if( IS_ROOM_FLAG( temp, ROOM_ISOLATED ) )
                  ch->in_room = get_room_index( ROOM_VNUM_TEMPLE );
               else
                  ch->in_room = temp;
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Coordinates" ) )
            {
               ch->x = fread_number( fp );
               ch->y = fread_number( fp );
               ch->map = fread_number( fp );

               if( !IS_PLR_FLAG( ch, PLR_ONMAP ) )
               {
                  ch->x = -1;
                  ch->y = -1;
                  ch->map = -1;
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Council" ) )
            {
               ch->pcdata->council_name = fread_string( fp );
               if( !preload && ch->pcdata->council_name[0] != '\0'
                   && !( ch->pcdata->council = get_council( ch->pcdata->council_name ) ) )
               {
                  ch_printf( ch,
                             "Warning: the council %s no longer exists, and herefore you no longer\n\rbelong to a council.\n\r",
                             ch->pcdata->council_name );
                  STRFREE( ch->pcdata->council_name );
               }
               fMatch = TRUE;
               break;
            }
            break;

         case 'D':
            /*
             * DAMROLL: OLD FIELD 
             */
            KEY( "Damroll", ch->damroll, fread_number( fp ) );
            if( !str_cmp( word, "Deity" ) )
            {
               ch->pcdata->deity_name = fread_string( fp );

               if( !preload && ch->pcdata->deity_name[0] != '\0'
                   && !( ch->pcdata->deity = get_deity( ch->pcdata->deity_name ) ) )
               {
                  ch_printf( ch, "Warning: the deity %s no longer exists.\n\r", ch->pcdata->deity_name );
                  STRFREE( ch->pcdata->deity_name );
                  ch->pcdata->favor = 0;
               }
               fMatch = TRUE;
               break;
            }
            KEY( "Description", ch->chardesc, fread_string( fp ) );
            break;

            /*
             * 'E' was moved to after 'S' 
             */
         case 'F':
            /*
             * FAVOR/FLAGS: OLD FIELDS 
             */
            KEY( "Favor", ch->pcdata->favor, fread_number( fp ) );
            KEY( "Flags", ch->pcdata->flags, fread_number( fp ) );
            if( !str_cmp( word, "FPrompt" ) )
            {
               STRFREE( ch->pcdata->fprompt );
               ch->pcdata->fprompt = fread_string( fp );
               fMatch = TRUE;
               break;
            }
            break;

         case 'G':
            /*
             * GOLD: OLD FIELD 
             */
            KEY( "Gold", ch->gold, fread_number( fp ) );
            break;

         case 'H':
            if( !str_cmp( word, "Helled" ) )
            {
               ch->pcdata->release_date = fread_long( fp );
               ch->pcdata->helled_by = fread_string( fp );
               fMatch = TRUE;
               break;
            }
            /*
             * HITROLL: OLD FIELD 
             */
            KEY( "Hitroll", ch->hitroll, fread_number( fp ) );
            KEY( "Homepage", ch->pcdata->homepage, fread_string_nohash( fp ) );

            if( !str_cmp( word, "HpManaMove" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = 0;
               sscanf( line, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );
               ch->hit = x1;
               ch->max_hit = x2;
               ch->mana = x3;
               ch->max_mana = x4;
               ch->move = x5;
               ch->max_move = x6;
               fMatch = TRUE;
               break;
            }
            break;

         case 'I':
            KEY( "ICQ", ch->pcdata->icq, fread_number( fp ) );
            if( !str_cmp( word, "ImmData" ) )
            {
               line = fread_line( fp );
               time_t xx2 = 0;
               x1 = x3 = x4 = x5 = x6 = 0;
               sscanf( line, "%d %ld %d %d %d %d", &x1, &xx2, &x3, &x4, &x5, &x6 );
               ch->trust = x1;
               ch->pcdata->restore_time = xx2;
               ch->pcdata->wizinvis = x3;
               ch->pcdata->low_vnum = x4;
               ch->pcdata->hi_vnum = x5;
               ch->pcdata->realm = x6;
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "Ignored" ) )
            {
               load_ignores( ch, fp );
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Immune" ) )
            {
               char *immune = NULL;
               char flag[MIL];
               int value;

               if( file_ver < 21 )
                  ch->immune = fread_bitvector( fp );
               else
               {
                  immune = fread_flagstring( fp );

                  while( immune[0] != '\0' )
                  {
                     immune = one_argument( immune, flag );
                     value = get_risflag( flag );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "Unknown RIS flag (A): %s", flag );
                     else
                        SET_IMMUNE( ch, value );
                  }
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Interface" ) )
            {
               int temp;

               temp = fread_number( fp );

               if( temp < 0 || temp > INT_AFKMUD )
                  temp = INT_AFKMUD;

               ch->pcdata->interface = temp;

               fMatch = TRUE;
               break;
            }
#ifdef I3
            if( ( fMatch = i3load_char( ch, fp, word ) ) )
               break;
#endif
#ifdef IMC
            if( ( fMatch = imc_loadchar( ch, fp, word ) ) )
               break;
#endif
            break;

         case 'K':
            if( !str_cmp( word, "KillInfo" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = 0;
               sscanf( line, "%d %d %d %d %d", &x1, &x2, &x3, &x4, &x5 );
               ch->pcdata->pkills = x1;
               ch->pcdata->pdeaths = x2;
               ch->pcdata->mkills = x3;
               ch->pcdata->mdeaths = x4;
               ch->pcdata->illegal_pk = x5;
               fMatch = TRUE;
               break;
            }
            break;

         case 'L':
            /*
             * LEVEL/LANGUAGES: OLD FIELDS 
             */
            KEY( "Level", ch->level, fread_number( fp ) );
            if( !str_cmp( word, "Languages" ) )
            {
               ch->speaks = fread_number( fp );
               ch->speaking = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "Lore" ) )
            {
               char *lore;
               int sn, value;

               value = fread_number( fp );
               lore = fread_word( fp );
               sn = bsearch_skill_exact( lore, gsn_first_lore, gsn_top_sn - 1 );
               if( sn < 0 )
                  log_printf( "Fread_char: unknown lore: %s", lore );
               else
               {
                  ch->pcdata->learned[sn] = value;
                  if( ch->level < LEVEL_IMMORTAL )
                  {
                     if( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL )
                     {
                        ch->pcdata->learned[sn] = 0;
                        ch->pcdata->practice++;
                     }
                  }
               }
               fMatch = TRUE;
               break;
            }
            break;

         case 'M':
            KEY( "MaxBeacons", max_beacons, fread_number( fp ) );
            KEY( "MaxColors", max_colors, fread_number( fp ) );
            if( !str_cmp( word, "Motd" ) )
            {
               line = fread_line( fp );
               time_t xx1 = 0, xx2 = 0;
               sscanf( line, "%ld %ld", &xx1, &xx2 );
               ch->pcdata->motd = xx1;
               ch->pcdata->imotd = xx2;
               fMatch = TRUE;
               break;
            }
            break;

         case 'N':
            KEY( "Name", ch->name, fread_string( fp ) );
            if( !str_cmp( word, "Nores" ) )
            {
               char *nores = NULL;
               char flag[MIL];
               int value;

               if( file_ver < 21 )
                  ch->no_resistant = fread_bitvector( fp );
               else
               {
                  nores = fread_flagstring( fp );

                  while( nores[0] != '\0' )
                  {
                     nores = one_argument( nores, flag );
                     value = get_risflag( flag );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "Unknown RIS flag (A): %s", flag );
                     else
                        xSET_BIT( ch->no_resistant, value );
                  }
               }
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "Nosusc" ) )
            {
               char *nosusc = NULL;
               char flag[MIL];
               int value;

               if( file_ver < 21 )
                  ch->no_susceptible = fread_bitvector( fp );
               else
               {
                  nosusc = fread_flagstring( fp );

                  while( nosusc[0] != '\0' )
                  {
                     nosusc = one_argument( nosusc, flag );
                     value = get_risflag( flag );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "Unknown RIS flag (A): %s", flag );
                     else
                        xSET_BIT( ch->no_susceptible, value );
                  }
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Noimm" ) )
            {
               char *noimm = NULL;
               char flag[MIL];
               int value;

               if( file_ver < 21 )
                  ch->no_immune = fread_bitvector( fp );
               else
               {
                  noimm = fread_flagstring( fp );

                  while( noimm[0] != '\0' )
                  {
                     noimm = one_argument( noimm, flag );
                     value = get_risflag( flag );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "Unknown RIS flag (A): %s", flag );
                     else
                        xSET_BIT( ch->no_immune, value );
                  }
               }
               fMatch = TRUE;
               break;
            }
            /*
             * THE NEXT 3: OLD FIELDS 
             */
            KEY( "NoImmune", ch->no_immune, fread_bitvector( fp ) );
            KEY( "NoResistant", ch->no_resistant, fread_bitvector( fp ) );
            KEY( "NoSusceptible", ch->no_susceptible, fread_bitvector( fp ) );
            if( !str_cmp( word, "NoAffectedBy" ) )
            {
               EXT_BV aflags;
               char *affflags = NULL;
               char flag[MIL];
               int value;

               if( file_ver < 21 )
               {
                  aflags = fread_bitvector( fp );
                  affflags = ext_flag_string( &aflags, a_flags );

                  while( affflags[0] != '\0' )
                  {
                     affflags = one_argument( affflags, flag );
                     value = get_aflag( flag );
                     if( value < 0 || value >= MAX_AFFECTED_BY )
                        bug( "Unknown affect flag: %s\n\r", flag );
                     else
                        xSET_BIT( ch->no_affected_by, value );
                  }
               }
               else
               {
                  affflags = fread_flagstring( fp );
                  while( affflags[0] != '\0' )
                  {
                     affflags = one_argument( affflags, flag );
                     value = get_aflag( flag );
                     if( value < 0 || value >= MAX_AFFECTED_BY )
                        bug( "Unknown affect flag: %s\n\r", flag );
                     else
                        xSET_BIT( ch->no_affected_by, value );
                  }
               }
               fMatch = TRUE;
               break;
            }
            break;

         case 'P':
            if( !str_cmp( word, "Password" ) )
            {
               ch->pcdata->pwd = fread_string_nohash( fp );
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "PCFlags" ) )
            {
               char *pcflags = NULL;
               char flag[MIL];
               int value;

               pcflags = fread_flagstring( fp );
               while( pcflags[0] != '\0' )
               {
                  pcflags = one_argument( pcflags, flag );
                  value = get_pcflag( flag );
                  if( value < 0 || value >= MAX_PCFLAG )
                     bug( "Unknown PC flag: %s\n\r", flag );
                  else
                     SET_PCFLAG( ch, 1 << value );
               }
               fMatch = TRUE;
               break;
            }

            /*
             * NEXT 2: OLD FIELDS 
             */
            KEY( "Pagerlen", ch->pcdata->pagerlen, fread_number( fp ) );
            KEY( "Played", ch->pcdata->played, fread_long( fp ) );
            if( !str_cmp( word, "Position" ) )
            {
               int position;

               position = get_npc_position( fread_flagstring( fp ) );

               if( position < 0 || position >= POS_MAX )
               {
                  bug( "fread_char: Player %s has invalid position! Defaulting to standing.", ch->name );
                  position = POS_STANDING;
               }
               ch->position = position;
               fMatch = TRUE;
               break;
            }
            /*
             * PRACTICE: OLD FIELD 
             */
            KEY( "Practice", ch->pcdata->practice, fread_number( fp ) );
            if( !str_cmp( word, "Prompt" ) )
            {
               STRFREE( ch->pcdata->prompt );
               ch->pcdata->prompt = fread_string( fp );
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "PTimer" ) )
            {
               add_timer( ch, TIMER_PKILLED, fread_number( fp ), NULL, 0 );
               fMatch = TRUE;
               break;
            }
            break;

         case 'Q':
            if( !str_cmp( word, "Qbit" ) )
            {
               BIT_DATA *bit;
               BIT_DATA *desc;

               CREATE( bit, BIT_DATA, 1 );

               bit->number = fread_number( fp );
               if( !( desc = find_qbit( bit->number ) ) )
                  mudstrlcpy( bit->desc, fread_flagstring( fp ), MSL );
               else
               {
                  mudstrlcpy( bit->desc, desc->desc, MSL );
                  fread_flagstring( fp );
               }
               LINK( bit, ch->pcdata->first_qbit, ch->pcdata->last_qbit, next, prev );
               fMatch = TRUE;
               break;
            }
            break;

         case 'R':
            if( !str_cmp( word, "Race" ) )
            {
               int race;

               race = get_npc_race( fread_flagstring( fp ) );

               if( race < 0 || race >= MAX_NPC_RACE )
               {
                  bug( "fread_char: Player %s has invalid race! Defaulting to human.", ch->name );
                  race = RACE_HUMAN;
               }
               ch->race = race;
               fMatch = TRUE;
               break;
            }
            KEY( "Rank", ch->pcdata->rank, fread_string( fp ) );
            /*
             * REALM: OLD FIELD 
             */
            KEY( "Realm", ch->pcdata->realm, fread_number( fp ) );

            if( !str_cmp( word, "RentData" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = 0;
               sscanf( line, "%d %d %d %d %d", &x1, &x2, &x3, &x4, &x5 );
               ch->pcdata->balance = x1;
               ch->pcdata->rent = x2;
               ch->pcdata->norares = x3;
               ch->pcdata->autorent = x4;
               ch->pcdata->camp = x5;
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "RentRooms" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = 0;
               sscanf( line, "%d %d %d", &x1, &x2, &x3 );
               ch->pcdata->alsherok = x1;
               ch->pcdata->eletar = x2;
               ch->pcdata->alatia = x3;
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Resistant" ) )
            {
               char *res = NULL;
               char flag[MIL];
               int value;

               if( file_ver < 21 )
                  ch->resistant = fread_bitvector( fp );
               else
               {
                  res = fread_flagstring( fp );

                  while( res[0] != '\0' )
                  {
                     res = one_argument( res, flag );
                     value = get_risflag( flag );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "Unknown RIS flag (A): %s", flag );
                     else
                        SET_RESIS( ch, value );
                  }
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Regens" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = 0;
               sscanf( line, "%d %d %d", &x1, &x2, &x3 );
               ch->hit_regen = x1;
               ch->mana_regen = x2;
               ch->move_regen = x3;
               fMatch = TRUE;
               break;
            }

            /*
             * RISA: OLD FIELD 
             */
            if( !str_cmp( word, "RISA" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
               sscanf( line, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
               ch->resistant.bits[0] = x1;
               ch->no_resistant.bits[0] = x2;
               ch->immune.bits[0] = x3;
               ch->no_immune.bits[0] = x4;
               ch->susceptible.bits[0] = x5;
               ch->no_susceptible.bits[0] = x6;
               ch->absorb.bits[0] = x7;
               fMatch = TRUE;
               break;
            }

            /*
             * ROOM: OLD FIELD 
             */
            if( !str_cmp( word, "Room" ) )
            {
               ROOM_INDEX_DATA *temp = get_room_index( fread_number( fp ) );

               if( !temp )
                  temp = get_room_index( ROOM_VNUM_TEMPLE );

               if( !temp )
                  temp = get_room_index( ROOM_VNUM_LIMBO );

               /*
                * Um, yeah. If this happens you're shit out of luck! 
                */
               if( !temp )
               {
                  bug( "%s", "FATAL: No valid fallback rooms. Program terminating!" );
                  exit( 1 );
               }
               /*
                * And you're going to crash if the above check failed, because you're an idiot if you remove this Vnum 
                */
               if( IS_ROOM_FLAG( temp, ROOM_ISOLATED ) )
                  ch->in_room = get_room_index( ROOM_VNUM_TEMPLE );
               else
                  ch->in_room = temp;

               fMatch = TRUE;
               break;
            }

            /*
             * ROOMRANGE: OLD FIELD 
             */
            if( !str_cmp( word, "RoomRange" ) )
            {
               ch->pcdata->low_vnum = fread_number( fp );
               ch->pcdata->hi_vnum = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            break;

         case 'S':
            if( !str_cmp( word, "Sex" ) )
            {
               int sex;

               sex = get_npc_sex( fread_flagstring( fp ) );

               if( sex < 0 || sex >= SEX_MAX )
               {
                  bug( "fread_char: Player %s has invalid sex! Defaulting to Neuter.", ch->name );
                  sex = SEX_NEUTRAL;
               }
               ch->sex = sex;
               fMatch = TRUE;
               break;
            }

            /*
             * STYLE: OLD FIELD 
             */
            KEY( "Style", ch->style, fread_number( fp ) );

            if( !str_cmp( word, "Susceptible" ) )
            {
               char *susc = NULL;
               char flag[MIL];
               int value;

               if( file_ver < 21 )
                  ch->susceptible = fread_bitvector( fp );
               else
               {
                  susc = fread_flagstring( fp );

                  while( susc[0] != '\0' )
                  {
                     susc = one_argument( susc, flag );
                     value = get_risflag( flag );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "Unknown RIS flag (A): %s", flag );
                     else
                        SET_SUSCEP( ch, value );
                  }
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "SavingThrows" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = 0;
               sscanf( line, "%d %d %d %d %d", &x1, &x2, &x3, &x4, &x5 );
               ch->saving_poison_death = x1;
               ch->saving_wand = x2;
               ch->saving_para_petri = x3;
               ch->saving_breath = x4;
               ch->saving_spell_staff = x5;
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Site" ) )
            {
               if( !copyover )
                  ch_printf( ch, "Last connected from: %s\n\r", fread_word( fp ) );
               else
                  fread_to_eol( fp );
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Skill" ) )
            {
               char *skill;
               int sn, value;

               value = fread_number( fp );
               skill = fread_word( fp );
               sn = bsearch_skill_exact( skill, gsn_first_skill, gsn_first_weapon - 1 );
               if( sn < 0 )
                  log_printf( "Fread_char: unknown skill: %s", skill );
               else
               {
                  ch->pcdata->learned[sn] = value;
                  /*
                   * Take care of people who have stuff they shouldn't
                   * * Assumes Class and level were loaded before. -- Altrag
                   * * Assumes practices are loaded first too now. -- Altrag
                   */
                  if( ch->level < LEVEL_IMMORTAL )
                  {
                     if( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL )
                     {
                        ch->pcdata->learned[sn] = 0;
                        ch->pcdata->practice++;
                     }
                  }
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Spell" ) )
            {
               char *spell;
               int sn, value;

               value = fread_number( fp );
               spell = fread_word( fp );
               sn = bsearch_skill_exact( spell, gsn_first_spell, gsn_first_skill - 1 );
               if( sn < 0 )
                  log_printf( "Fread_char: unknown spell: %s", spell );
               else
               {
                  ch->pcdata->learned[sn] = value;
                  if( ch->level < LEVEL_IMMORTAL )
                  {
                     if( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL )
                     {
                        ch->pcdata->learned[sn] = 0;
                        ch->pcdata->practice++;
                     }
                  }
               }
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "Status" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
               sscanf( line, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
               ch->level = x1;
               ch->gold = x2;
               ch->exp = x3;
               ch->height = x4;
               ch->weight = x5;
               ch->spellfail = x6;
               ch->mental_state = x7;
               fMatch = TRUE;
               if( preload )
                  return;
               else
                  break;

            }

            if( !str_cmp( word, "Status2" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = 0;
               sscanf( line, "%d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8 );
               ch->style = x1;
               ch->pcdata->practice = x2;
               ch->alignment = x3;
               ch->pcdata->favor = x4;
               ch->hitroll = x5;
               ch->damroll = x6;
               ch->armor = x7;
               ch->wimpy = x8;
               fMatch = TRUE;
               break;
            }
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( preload )
                  return;

               /*
                * Let no character be trusted higher than one below maxlevel -- Narn 
                */
               ch->trust = UMIN( ch->trust, MAX_LEVEL - 1 );

               if( ch->pcdata->played < 0 )
                  ch->pcdata->played = 0;

               if( ch->pcdata->chan_listen != NULL )
               {
                  MUD_CHANNEL *channel = NULL;
                  char *channels = ch->pcdata->chan_listen;
                  char arg[MIL];

                  while( channels[0] != '\0' )
                  {
                     channels = one_argument( channels, arg );

                     if( !( channel = find_channel( arg ) ) )
                        removename( &ch->pcdata->chan_listen, arg );
                  }
               }
               /*
                * Provide at least the one channel 
                */
               else
                  ch->pcdata->chan_listen = STRALLOC( "chat" );

               ch->pcdata->editor = NULL;

               /*
                * no good for newbies at all 
                */
               if( !IS_IMMORTAL( ch ) && ( !ch->speaking || ch->speaking < 0 ) )
                  ch->speaking = LANG_COMMON;
               if( IS_IMMORTAL( ch ) )
               {
                  ch->speaks = ~0;
                  if( ch->speaking == 0 )
                     ch->speaking = ~0;
               }

               /*
                * this disallows chars from being 6', 180lbs, but easier than a flag 
                */
               if( ch->height == 72 || ch->height == 0 )
                  ch->height =
                     number_range( ( int )( race_table[ch->race]->height * 0.75 ),
                                   ( int )( race_table[ch->race]->height * 1.25 ) );
               if( ch->weight == 180 || ch->weight == 0 )
                  ch->weight =
                     number_range( ( int )( race_table[ch->race]->weight * 0.75 ),
                                   ( int )( race_table[ch->race]->weight * 1.25 ) );

               REMOVE_PLR_FLAG( ch, PLR_MAPEDIT ); /* In case they saved while editing */

               if( ch->pcdata->year == 0 )
                  ch->pcdata->age = find_old_age( ch );
               else
                  ch->pcdata->age = calculate_age( ch );

               if( ch->pcdata->month > sysdata.monthsperyear - 1 )
                  ch->pcdata->month = sysdata.monthsperyear - 1;  /* Catches the bad month values */

               if( ch->pcdata->day > sysdata.dayspermonth - 1 )
                  ch->pcdata->day = sysdata.dayspermonth - 1;  /* Cathes the bad day values */

               if( !ch->pcdata->deity_name || ch->pcdata->deity_name[0] == '\0' )
                  ch->pcdata->favor = 0;
               return;
            }
            KEY( "Email", ch->pcdata->email, fread_string_nohash( fp ) );

            /*
             * EXP: OLD FIELD 
             */
            if( !str_cmp( word, "Exp" ) )
            {
               if( file_ver < 17 )
               {
                  fread_number( fp );
                  ch->tempnum = ( int )( ch->level / ( LEVEL_AVATAR * 0.20 ) );
                  ch->exp = exp_level( ch->level + 1 ) - 1;
                  fMatch = TRUE;
                  break;
               }
               else
               {
                  ch->exp = fread_number( fp );
                  fMatch = TRUE;
                  break;
               }
            }
            break;

         case 'T':
            if( !str_cmp( word, "Tongue" ) )
            {
               char *tongue;
               int sn, value;

               value = fread_number( fp );
               tongue = fread_word( fp );
               sn = bsearch_skill_exact( tongue, gsn_first_tongue, gsn_first_ability - 1 );
               if( sn < 0 )
                  log_printf( "Fread_char: unknown tongue: %s", tongue );
               else
               {
                  ch->pcdata->learned[sn] = value;
                  if( ch->level < LEVEL_IMMORTAL )
                  {
                     if( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL )
                     {
                        ch->pcdata->learned[sn] = 0;
                        ch->pcdata->practice++;
                     }
                  }
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Title" ) )
            {
               ch->pcdata->title = fread_string( fp );
               if( ch->pcdata->title != NULL && ( isalpha( ch->pcdata->title[0] ) || isdigit( ch->pcdata->title[0] ) ) )
                  stralloc_printf( &ch->pcdata->title, " %s", ch->pcdata->title );

               fMatch = TRUE;
               break;
            }
            break;

         case 'V':
            if( !str_cmp( word, "Version" ) )
            {
               file_ver = fread_number( fp );
               ch->pcdata->version = file_ver;
               fMatch = TRUE;
               break;
            }
            break;

         case 'W':
            if( !str_cmp( word, "Weapon" ) )
            {
               char *weapon;
               int sn, value;

               value = fread_number( fp );
               weapon = fread_word( fp );
               sn = bsearch_skill_exact( weapon, gsn_first_weapon, gsn_first_tongue - 1 );
               if( sn < 0 )
                  log_printf( "Fread_char: unknown weapon skill: %s", weapon );
               else
               {
                  ch->pcdata->learned[sn] = value;
                  if( ch->level < LEVEL_IMMORTAL )
                  {
                     if( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL )
                     {
                        ch->pcdata->learned[sn] = 0;
                        ch->pcdata->practice++;
                     }
                  }
               }
               fMatch = TRUE;
               break;
            }
            break;

         case 'Z':
            if( !str_cmp( word, "Zone" ) )
            {
               load_zonedata( ch, fp );
               fMatch = TRUE;
               break;
            }
            break;
      }
      if( !fMatch )
         log_printf( "Fread_char: no match: %s", word );
   }
}

void fread_obj( CHAR_DATA * ch, FILE * fp, short os_type )
{
   OBJ_DATA *obj;
   const char *word;
   int iNest, obj_file_ver;
   bool fMatch, fNest, fVnum;
   ROOM_INDEX_DATA *room = NULL;

   if( ch )
   {
      room = ch->in_room;
      if( ch->tempnum == -9999 )
         file_ver = 0;
   }

   /*
    * Jesus Christ, how the hell did Smaug get away without versioning the object format for so long!!! 
    */
   obj_file_ver = 0;

   CREATE( obj, OBJ_DATA, 1 );
   obj->count = 1;
   obj->wear_loc = -1;
   obj->weight = 1;
   obj->map = -1;
   obj->x = -1;
   obj->y = -1;

   fNest = TRUE;  /* Requiring a Nest 0 is a waste */
   fVnum = TRUE;
   iNest = 0;

   for( ;; )
   {
      word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'A':
            KEY( "ActionDesc", obj->action_desc, fread_string( fp ) );
            if( !str_cmp( word, "Affect" ) || !str_cmp( word, "AffectData" ) )
            {
               AFFECT_DATA *paf;
               int pafmod;

               CREATE( paf, AFFECT_DATA, 1 );
               if( !str_cmp( word, "Affect" ) )
                  paf->type = fread_number( fp );
               else
               {
                  int sn;

                  sn = skill_lookup( fread_word( fp ) );
                  if( sn < 0 )
                     bug( "%s: Vnum %d - unknown skill: %s", __FUNCTION__, obj->pIndexData->vnum, word );
                  else
                     paf->type = sn;
               }
               paf->duration = fread_number( fp );
               pafmod = fread_number( fp );
               paf->location = fread_number( fp );
               paf->bit = fread_number( fp );
               if( paf->location == APPLY_WEAPONSPELL
                   || paf->location == APPLY_WEARSPELL
                   || paf->location == APPLY_STRIPSN
                   || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_RECURRINGSPELL )
                  paf->modifier = slot_lookup( pafmod );
               else
                  paf->modifier = pafmod;
               LINK( paf, obj->first_affect, obj->last_affect, next, prev );
               fMatch = true;
               break;
            }
            break;

         case 'B':
            KEY( "Bid", obj->bid, fread_number( fp ) );  /* Samson 6-20-99 */
            KEY( "Buyer", obj->buyer, fread_string( fp ) ); /* Samson 6-20-99 */
            break;

         case 'C':
            if( !str_cmp( word, "Coords" ) )
            {
               obj->x = fread_short( fp );
               obj->y = fread_short( fp );
               obj->map = fread_short( fp );
               fMatch = TRUE;
               break;
            }
            KEY( "Cost", obj->cost, fread_number( fp ) );
            KEY( "Count", obj->count, fread_number( fp ) );
            break;

         case 'D':
            KEY( "Description", obj->objdesc, fread_string( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "ExtraFlags" ) )
            {
               if( obj_file_ver < 20 )
                  obj->extra_flags = fread_bitvector( fp );
               else
               {
                  char *eflags = NULL;
                  char flag[MIL];
                  int value;

                  eflags = fread_flagstring( fp );

                  while( eflags[0] != '\0' )
                  {
                     eflags = one_argument( eflags, flag );
                     value = get_oflag( flag );
                     if( value < 0 || value >= MAX_ITEM_FLAG )
                        bug( "Unknown extra flag: %s", flag );
                     else
                        SET_OBJ_FLAG( obj, value );
                  }
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "ExtraDescr" ) )
            {
               EXTRA_DESCR_DATA *ed;

               CREATE( ed, EXTRA_DESCR_DATA, 1 );
               ed->keyword = fread_string( fp );
               ed->extradesc = fread_string( fp );
               LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
               fMatch = TRUE;
            }

            if( !str_cmp( word, "End" ) )
            {
               if( !fNest || !fVnum )
               {
                  if( obj->name )
                     bug( "Fread_obj: %s incomplete object.", obj->name );
                  else
                     bug( "%s", "Fread_obj: incomplete object." );
                  STRFREE( obj->name );
                  STRFREE( obj->objdesc );
                  STRFREE( obj->short_descr );
                  DISPOSE( obj );
                  return;
               }
               else
               {
                  short wear_loc = obj->wear_loc;

                  if( !obj->name && obj->pIndexData->name != NULL )
                     obj->name = QUICKLINK( obj->pIndexData->name );
                  if( !obj->objdesc && obj->pIndexData->objdesc != NULL )
                     obj->objdesc = QUICKLINK( obj->pIndexData->objdesc );
                  if( !obj->short_descr && obj->pIndexData->short_descr != NULL )
                     obj->short_descr = QUICKLINK( obj->pIndexData->short_descr );
                  if( !obj->action_desc && obj->pIndexData->action_desc != NULL )
                     obj->action_desc = QUICKLINK( obj->pIndexData->action_desc );
                  if( IS_OBJ_FLAG( obj, ITEM_PERSONAL ) && !obj->owner && ch )
                     obj->owner = STRALLOC( ch->name );
                  LINK( obj, first_object, last_object, next, prev );

                  /*
                   * Don't fix it if it matches the vnum for random treasure 
                   */
                  if( obj->item_type == ITEM_WEAPON && obj->pIndexData->vnum != OBJ_VNUM_TREASURE )
                     obj->value[4] = obj->pIndexData->value[4];

                  /*
                   * Altered count method for rare items - Samson 11-5-98 
                   */
                  obj->pIndexData->count += obj->count;

                  if( obj->rent >= sysdata.minrent )
                     obj->pIndexData->count -= obj->count;

                  if( fNest )
                     rgObjNest[iNest] = obj;
                  numobjsloaded += obj->count;
                  ++physicalobjects;
                  if( file_ver > 1 || obj->wear_loc < -1 || obj->wear_loc >= MAX_WEAR )
                     obj->wear_loc = -1;
                  /*
                   * Corpse saving. -- Altrag 
                   */
                  if( os_type == OS_CORPSE )
                  {
                     if( !room )
                     {
                        bug( "%s", "Fread_obj: Corpse without room" );
                        room = get_room_index( ROOM_VNUM_LIMBO );
                     }
                     /*
                      * Give the corpse a timer if there isn't one 
                      */

                     if( obj->timer < 1 )
                        obj->timer = 80;
                     obj_to_room( obj, room, NULL );
                  }
                  else if( iNest == 0 || rgObjNest[iNest] == NULL )
                  {
                     int slot = -1;
                     bool reslot = FALSE;

                     if( file_ver > 1 && wear_loc > -1 && wear_loc < MAX_WEAR )
                     {
                        int x;

                        for( x = 0; x < MAX_LAYERS; x++ )
                           if( IS_NPC( ch ) )
                           {
                              if( !mob_save_equipment[wear_loc][x] )
                              {
                                 mob_save_equipment[wear_loc][x] = obj;
                                 slot = x;
                                 reslot = TRUE;
                                 break;
                              }
                           }
                           else
                           {
                              if( !save_equipment[wear_loc][x] )
                              {
                                 save_equipment[wear_loc][x] = obj;
                                 slot = x;
                                 reslot = TRUE;
                                 break;
                              }
                           }
                        if( x == MAX_LAYERS )
                           bug( "Fread_obj: too many layers %d: vnum %d mob %d",
                                wear_loc, obj->pIndexData->vnum, IS_NPC( ch ) ? ch->pIndexData->vnum : 0 );
                     }
                     obj_to_char( obj, ch );
                     if( reslot && slot != -1 )
                     {
                        if( IS_NPC( ch ) )
                           mob_save_equipment[wear_loc][slot] = obj;
                        else
                           save_equipment[wear_loc][slot] = obj;
                     }
                  }
                  else
                  {
                     if( rgObjNest[iNest - 1] )
                     {
                        separate_obj( rgObjNest[iNest - 1] );
                        obj = obj_to_obj( obj, rgObjNest[iNest - 1] );
                     }
                     else
                        bug( "Fread_obj: nest layer missing %d", iNest - 1 );
                  }
                  if( fNest )
                     rgObjNest[iNest] = obj;
                  return;
               }
            }
            break;

         case 'I':
            KEY( "ItemType", obj->item_type, fread_number( fp ) );
            break;

         case 'M':
            if( !str_cmp( word, "MagicFlags" ) )
            {
               if( obj_file_ver < 20 )
                  obj->magic_flags = fread_number( fp );
               else
               {
                  char *mflags = NULL;
                  char flag[MIL];
                  int value;

                  mflags = fread_flagstring( fp );

                  while( mflags[0] != '\0' )
                  {
                     mflags = one_argument( mflags, flag );
                     value = get_magflag( flag );
                     if( value < 0 || value >= MAX_MFLAG )
                        bug( "Unknown magic flag: %s", flag );
                     else
                        SET_MAGIC_FLAG( obj, value );
                  }
               }
               fMatch = TRUE;
               break;
            }
            break;

         case 'L':
            KEY( "Level", obj->level, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", obj->name, fread_string( fp ) );

            if( !str_cmp( word, "Nest" ) )
            {
               iNest = fread_number( fp );
               if( iNest < 0 || iNest >= MAX_NEST )
               {
                  bug( "Fread_obj: bad nest %d.", iNest );
                  iNest = 0;
                  fNest = FALSE;
               }
               fMatch = TRUE;
            }
            break;

         case 'O':
            KEY( "Oday", obj->day, fread_number( fp ) );
            KEY( "Omonth", obj->month, fread_number( fp ) );
            KEY( "Oyear", obj->year, fread_number( fp ) );
            KEY( "Owner", obj->owner, fread_string( fp ) );
            if( !str_cmp( word, "Ovnum" ) )
            {
               int vnum;

               vnum = fread_number( fp );
               if( !( obj->pIndexData = get_obj_index( vnum ) ) )
                  fVnum = FALSE;
               else
               {
                  fVnum = TRUE;
                  obj->cost = obj->pIndexData->cost;
                  obj->rent = obj->pIndexData->rent;
                  obj->weight = obj->pIndexData->weight;
                  obj->item_type = obj->pIndexData->item_type;
                  obj->wear_flags = obj->pIndexData->wear_flags;
                  obj->extra_flags = obj->pIndexData->extra_flags;
               }
               fMatch = TRUE;
               break;
            }
            break;

         case 'R':
            KEY( "Rent", obj->rent, fread_number( fp ) );   /* Samson 5-8-99 */
            KEY( "Room", room, get_room_index( fread_number( fp ) ) );
            KEY( "Rvnum", obj->room_vnum, fread_number( fp ) );   /* hotboot tracker */

         case 'S':
            KEY( "Seller", obj->seller, fread_string( fp ) );  /* Samson 6-20-99 */
            KEY( "ShortDescr", obj->short_descr, fread_string( fp ) );

            if( !str_cmp( word, "Spell" ) )
            {
               int iValue, sn;

               iValue = fread_number( fp );
               sn = skill_lookup( fread_word( fp ) );
               if( iValue < 0 || iValue > 10 )
                  bug( "Fread_obj: bad iValue %d.", iValue );
               /*
                * Bug fixed here to change corrupted spell values to -1 to stop spamming logs - Samson 7-5-03 
                */
               else if( sn < 0 )
               {
                  bug( "Fread_obj: Vnum %d - unknown skill: %s", obj->pIndexData->vnum, word );
                  obj->value[iValue] = -1;
               }
               else
                  obj->value[iValue] = sn;
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Sockets" ) )
            {
               obj->socket[0] = STRALLOC( fread_word( fp ) );
               obj->socket[1] = STRALLOC( fread_word( fp ) );
               obj->socket[2] = STRALLOC( fread_word( fp ) );
               fMatch = TRUE;
               break;
            }
            break;

         case 'T':
            KEY( "Timer", obj->timer, fread_number( fp ) );
            break;

         case 'V':
            KEY( "Version", obj_file_ver, fread_number( fp ) );
            if( !str_cmp( word, "Values" ) )
            {
               int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11;
               char *ln = fread_line( fp );

               x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = 0;
               sscanf( ln, "%d %d %d %d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8, &x9, &x10, &x11 );

               if( x7 == 0 && ( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_MISSILE_WEAPON ) )
                  x7 = x1;

               if( x6 == 0 && obj->item_type == ITEM_PROJECTILE )
                  x6 = x1;

               obj->value[0] = x1;
               obj->value[1] = x2;
               obj->value[2] = x3;
               obj->value[3] = x4;
               obj->value[4] = x5;
               obj->value[5] = x6;
               obj->value[6] = x7;
               obj->value[7] = x8;
               obj->value[8] = x9;
               obj->value[9] = x10;
               obj->value[10] = x11;

               /*
                * Ugh, the price one pays for forgetting - had to keep corpses from doing this 
                */
               if( file_ver < 10 && fVnum == TRUE && os_type != OS_CORPSE )
               {
                  obj->value[0] = obj->pIndexData->value[0];
                  obj->value[1] = obj->pIndexData->value[1];
                  obj->value[2] = obj->pIndexData->value[2];
                  obj->value[3] = obj->pIndexData->value[3];
                  obj->value[4] = obj->pIndexData->value[4];
                  obj->value[5] = obj->pIndexData->value[5];

                  if( obj->item_type == ITEM_WEAPON )
                     obj->value[2] = obj->pIndexData->value[1] * obj->pIndexData->value[2];
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Vnum" ) )
            {
               int vnum;

               vnum = fread_number( fp );
               if( !( obj->pIndexData = get_obj_index( vnum ) ) )
                  fVnum = FALSE;
               else
               {
                  fVnum = TRUE;
                  obj->cost = obj->pIndexData->cost;
                  obj->rent = obj->pIndexData->rent;  /* Samson 5-8-99 */
                  obj->weight = obj->pIndexData->weight;
                  obj->item_type = obj->pIndexData->item_type;
                  obj->wear_flags = obj->pIndexData->wear_flags;
                  obj->extra_flags = obj->pIndexData->extra_flags;
               }
               fMatch = TRUE;
               break;
            }
            break;

         case 'W':
            if( !str_cmp( word, "WearFlags" ) )
            {
               if( obj_file_ver < 20 )
                  obj->wear_flags = fread_number( fp );
               else
               {
                  char *wflags = NULL;
                  char flag[MIL];
                  int value;

                  wflags = fread_flagstring( fp );

                  while( wflags[0] != '\0' )
                  {
                     wflags = one_argument( wflags, flag );
                     value = get_wflag( flag );
                     if( value < 0 || value >= MAX_WEAR_FLAG )
                        bug( "Unknown wear flag: %s", flag );
                     else
                        SET_WEAR_FLAG( obj, 1 << value );
                  }
               }
               fMatch = TRUE;
               break;
            }
            KEY( "WearLoc", obj->wear_loc, fread_number( fp ) );
            KEY( "Weight", obj->weight, fread_number( fp ) );
            break;

      }

      if( !fMatch )
      {
         EXTRA_DESCR_DATA *ed;
         AFFECT_DATA *paf;

         bug( "Fread_obj: no match: %s", word );
         fread_to_eol( fp );
         STRFREE( obj->name );
         STRFREE( obj->objdesc );
         STRFREE( obj->short_descr );
         while( ( ed = obj->first_extradesc ) != NULL )
         {
            STRFREE( ed->keyword );
            STRFREE( ed->extradesc );
            UNLINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
            DISPOSE( ed );
         }
         while( ( paf = obj->first_affect ) != NULL )
         {
            UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
            DISPOSE( paf );
         }
         DISPOSE( obj );
         return;
      }
   }
}

/*
 * This will read one mobile structure pointer to by fp --Shaddai
 *   Edited by Tarl 5 May 2002 to allow pets to load equipment.
 */
CHAR_DATA *fread_mobile( FILE * fp, bool shopmob )
{
   CHAR_DATA *mob = NULL;
   const char *word;
   bool fMatch;
   int inroom = 0, i, x;
   ROOM_INDEX_DATA *pRoomIndex = NULL;
   MOB_INDEX_DATA *pMobIndex = NULL;

   if( !shopmob )
      word = feof( fp ) ? "EndMobile" : fread_word( fp );
   else
      word = feof( fp ) ? "EndVendor" : fread_word( fp );

   if( word[0] == '\0' )
   {
      bug( "%s: EOF encountered reading file!", __FUNCTION__ );
      if( !shopmob )
         word = "EndMobile";
      else
         word = "EndVendor";
   }

   if( !str_cmp( word, "Vnum" ) )
   {
      int vnum;

      vnum = fread_number( fp );
      pMobIndex = get_mob_index( vnum );
      if( !pMobIndex )
      {
         for( ;; )
         {
            if( !shopmob )
               word = feof( fp ) ? "EndMobile" : fread_word( fp );
            else
               word = feof( fp ) ? "EndVendor" : fread_word( fp );

            if( word[0] == '\0' )
            {
               bug( "%s: EOF encountered reading file!", __FUNCTION__ );
               if( !shopmob )
                  word = "EndMobile";
               else
                  word = "EndVendor";
            }

            /*
             * So we don't get so many bug messages when something messes up
             * * --Shaddai
             */
            if( !str_cmp( word, "EndMobile" ) || !str_cmp( word, "EndVendor" ) )
               break;
         }
         bug( "%s: No index data for vnum %d", __FUNCTION__, vnum );
         return NULL;
      }
      mob = create_mobile( pMobIndex );
   }
   else
   {
      for( ;; )
      {
         if( !shopmob )
            word = feof( fp ) ? "EndMobile" : fread_word( fp );
         else
            word = feof( fp ) ? "EndVendor" : fread_word( fp );

         if( word[0] == '\0' )
         {
            bug( "%s: EOF encountered reading file!", __FUNCTION__ );
            if( !shopmob )
               word = "EndMobile";
            else
               word = "EndVendor";
         }

         /*
          * So we don't get so many bug messages when something messes up
          * * --Shaddai
          */
         if( !str_cmp( word, "EndMobile" ) || !str_cmp( word, "EndVendor" ) )
            break;
      }
      extract_char( mob, TRUE );
      bug( "%s: Vnum not found", __FUNCTION__ );
      return NULL;
   }

   for( ;; )
   {
      if( !shopmob )
         word = feof( fp ) ? "EndMobile" : fread_word( fp );
      else
         word = feof( fp ) ? "EndVendor" : fread_word( fp );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         if( !shopmob )
            word = "EndMobile";
         else
            word = "EndVendor";
      }

      fMatch = FALSE;
      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#OBJECT" ) )
            {
               fread_obj( mob, fp, OS_CARRY );
               fMatch = true;
               break;
            }
            break;

         case 'A':
            if( !str_cmp( word, "Affect" ) || !str_cmp( word, "AffectData" ) )
            {
               AFFECT_DATA *paf;

               CREATE( paf, AFFECT_DATA, 1 );
               if( !str_cmp( word, "Affect" ) )
                  paf->type = fread_number( fp );
               else
               {
                  int sn;
                  char *sname = fread_word( fp );

                  if( ( sn = skill_lookup( sname ) ) < 0 )
                  {
                     if( ( sn = herb_lookup( sname ) ) < 0 )
                        bug( "%s", "Fread_mobile: unknown skill." );
                     else
                        sn += TYPE_HERB;
                  }
                  paf->type = sn;
               }

               paf->duration = fread_number( fp );
               paf->modifier = fread_number( fp );
               paf->location = fread_number( fp );
               if( paf->location == APPLY_WEAPONSPELL
                   || paf->location == APPLY_WEARSPELL
                   || paf->location == APPLY_REMOVESPELL
                   || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL )
                  paf->modifier = slot_lookup( paf->modifier );
               paf->bit = fread_number( fp );
               LINK( paf, mob->first_affect, mob->last_affect, next, prev );
               fMatch = true;
               break;
            }
            KEY( "AffectedBy", mob->affected_by, fread_bitvector( fp ) );
            break;

         case 'C':
            if( !str_cmp( word, "Coordinates" ) )
            {
               mob->x = fread_number( fp );
               mob->y = fread_number( fp );
               mob->map = fread_number( fp );

               fMatch = TRUE;
               break;
            }
            break;

         case 'D':
            if( !str_cmp( word, "Description" ) )
            {
               STRFREE( mob->chardesc );
               mob->chardesc = fread_string( fp );
               fMatch = TRUE;
               break;
            }
            break;

         case 'E':
            if( !str_cmp( word, "EndMobile" ) || !str_cmp( word, "EndVendor" ) )
            {
               if( inroom == 0 )
                  inroom = ROOM_VNUM_TEMPLE;
               pRoomIndex = get_room_index( inroom );
               if( !pRoomIndex )
                  pRoomIndex = get_room_index( ROOM_VNUM_LIMBO );
               if( !char_to_room( mob, pRoomIndex ) )
                  log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

               for( i = 0; i < MAX_WEAR; i++ )
                  for( x = 0; x < MAX_LAYERS; x++ )
                     if( mob_save_equipment[i][x] )
                     {
                        equip_char( mob, mob_save_equipment[i][x], i );
                        mob_save_equipment[i][x] = NULL;
                     }
               return mob;
            }
            if( !str_cmp( word, "Exp" ) )
            {
               mob->exp = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            break;

         case 'F':
            KEY( "Flags", mob->act, fread_bitvector( fp ) );
            break;

         case 'H':
            if( !str_cmp( word, "HpManaMove" ) )
            {
               mob->hit = fread_number( fp );
               mob->max_hit = fread_number( fp );
               mob->mana = fread_number( fp );
               mob->max_mana = fread_number( fp );
               mob->move = fread_number( fp );
               mob->max_move = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            break;

         case 'L':
            KEY( "Level", mob->level, fread_number( fp ) );
            if( !str_cmp( word, "Long" ) )
            {
               STRFREE( mob->long_descr );
               mob->long_descr = fread_string( fp );
               fMatch = TRUE;
               break;
            }
            break;

         case 'N':
            if( !str_cmp( word, "Name" ) )
            {
               STRFREE( mob->name );
               mob->name = fread_string( fp );
               fMatch = TRUE;
               break;
            }
            break;

         case 'P':
            KEY( "Position", mob->position, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Room", inroom, fread_number( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "Short" ) )
            {
               STRFREE( mob->short_descr );
               mob->short_descr = fread_string( fp );
               fMatch = TRUE;
               break;
            }
            break;
      }
      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj( DESCRIPTOR_DATA * d, char *name, bool preload, bool copyover )
{
   char strsave[256];
   CHAR_DATA *ch;
   FILE *fp;
   bool found;
   struct stat fst;
   int i, x;

   CREATE( ch, CHAR_DATA, 1 );
   for( x = 0; x < MAX_WEAR; x++ )
      for( i = 0; i < MAX_LAYERS; i++ )
      {
         save_equipment[x][i] = NULL;
         mob_save_equipment[x][i] = NULL;
      }
   clear_char( ch );
   loading_char = ch;

   CREATE( ch->pcdata, PC_DATA, 1 );
   d->character = ch;
   ch->desc = d;
   ch->pcdata->filename = STRALLOC( name );
   ch->name = NULL;
   xCLEAR_BITS( ch->act );
   SET_PLR_FLAG( ch, PLR_COMBINE );
   SET_PLR_FLAG( ch, PLR_PROMPT );
   ch->perm_str = 13;
   ch->perm_int = 13;
   ch->perm_wis = 13;
   ch->perm_dex = 13;
   ch->perm_con = 13;
   ch->perm_cha = 13;
   ch->perm_lck = 13;
   xCLEAR_BITS( ch->no_resistant );
   xCLEAR_BITS( ch->no_susceptible );
   xCLEAR_BITS( ch->no_immune );
   ch->was_in_room = NULL;
   xCLEAR_BITS( ch->no_affected_by );
   ch->pcdata->condition[COND_THIRST] = ( int )( sysdata.maxcondval * .75 );
   ch->pcdata->condition[COND_FULL] = ( int )( sysdata.maxcondval * .75 );
   ch->pcdata->condition[COND_DRUNK] = 0;
   ch->pcdata->wizinvis = 0;
   ch->pcdata->charmies = 0;
   ch->mental_state = -10;
   ch->mobinvis = 0;
   for( i = 0; i < MAX_SKILL; i++ )
      ch->pcdata->learned[i] = 0;
   ch->pcdata->release_date = 0;
   ch->pcdata->helled_by = NULL;
   ch->saving_poison_death = 0;
   ch->saving_wand = 0;
   ch->saving_para_petri = 0;
   ch->saving_breath = 0;
   ch->saving_spell_staff = 0;
   ch->style = STYLE_FIGHTING;
   ch->pcdata->first_comment = NULL;   /* comments */
   ch->pcdata->last_comment = NULL; /* comments */
   ch->pcdata->pagerlen = 24;
   ch->pcdata->first_ignored = NULL;   /* Ignore list */
   ch->pcdata->last_ignored = NULL;
   ch->morph = NULL;
   ch->pcdata->practice = 0;
   ch->pcdata->prompt = STRALLOC( default_prompt( ch ) );
   ch->pcdata->fprompt = STRALLOC( default_fprompt( ch ) );
   ch->pcdata->afkbuf = NULL; /* Initialize AFK reason buffer - Samson 9-2-98 */
   ch->pcdata->email = NULL;  /* Initialize email address - Samson 1-4-99 */
   ch->pcdata->homepage = NULL;  /* Initialize homepage - Samson 1-4-99 */
   ch->pcdata->icq = 0; /* Initialize icq# - Samson 1-4-99 */
   ch->pcdata->realm = 0;  /* Initialize realm - Samson 6-6-99 */
   ch->pcdata->secedit = SECT_OCEAN;   /* Initialize Map OLC sector - Samson 8-1-99 */
   ch->pcdata->day = 0; /* Default Birthday - Samson 10-25-99 */
   ch->pcdata->month = 0;
   ch->pcdata->year = 0;
   ch->pcdata->age_bonus = 0; /* Default aging factor - Samson 10-25-99 */
   ch->pcdata->rent = 0;   /* Initialize default rent value - Samson 1-24-00 */
   ch->pcdata->norares = FALSE;  /* Default value for toggle flag - Samson 1-24-00 */
   ch->pcdata->autorent = FALSE;
   ch->first_pet = NULL;   /* Initialize pet stuff - Samson 4-22-00 */
   ch->last_pet = NULL;
   ch->pcdata->first_alias = NULL;  /* Initialize alias list - Samson 7-11-00 */
   ch->pcdata->last_alias = NULL;
   ch->pcdata->alsherok = ROOM_VNUM_TEMPLE;  /* Initialize the 3 rent recall rooms to default - Samson 12-20-00 */
   ch->pcdata->eletar = ROOM_VNUM_ELETAR_RECALL;
   ch->pcdata->alatia = ROOM_VNUM_ALATIA_RECALL;
   ch->pcdata->imotd = 0;  /* Defaults for MOTD handler - this way they'll see it at least once */
   ch->pcdata->motd = 0;
   ch->pcdata->spam = 0;
   ch->pcdata->hotboot = FALSE;  /* Never changed except when PC is saved during hotboot/sigterm save */
   ch->pcdata->interface = INT_AFKMUD;
   ch->pcdata->lasthost = str_dup( "Unknown-Host" );
   ch->pcdata->chan_listen = NULL;
   ch->pcdata->logon = current_time;
   ch->pcdata->timezone = -1;
   for( i = 0; i < MAX_SAYHISTORY; i++ )
      ch->pcdata->say_history[i] = NULL;
#ifdef I3
   i3init_char( ch );
#endif
#ifdef IMC
   imc_initchar( ch );
#endif

   found = FALSE;
   snprintf( strsave, 256, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ) );
   if( stat( strsave, &fst ) != -1 && d->connected != CON_PLOADED )
   {
      if( preload )
         log_printf_plus( LOG_COMM, LEVEL_KL, "Preloading player data for: %s", ch->pcdata->filename );
      else
         log_printf_plus( LOG_COMM, LEVEL_KL, "Loading player data for %s (%dK)", ch->pcdata->filename,
                          ( int )fst.st_size / 1024 );
   }
   /*
    * else no player file 
    */

   if( ( fp = fopen( strsave, "r" ) ) != NULL )
   {
      int iNest;

      for( iNest = 0; iNest < MAX_NEST; iNest++ )
         rgObjNest[iNest] = NULL;

      found = TRUE;
      /*
       * Cheat so that bug will show line #'s -- Altrag 
       */
      fpArea = fp;
      mudstrlcpy( strArea, strsave, MIL );
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
            bug( "Load_char_obj: # not found. %s", name );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "PLAYER" ) )
         {
            fread_char( ch, fp, preload, copyover );
            if( preload )
               break;
         }
         else if( !str_cmp( word, "OBJECT" ) )  /* Objects  */
            fread_obj( ch, fp, OS_CARRY );
         else if( !str_cmp( word, "MorphData" ) )  /* Morphs */
            fread_morph_data( ch, fp );
         else if( !str_cmp( word, "COMMENT2" ) )
            fread_comment( ch, fp );   /* Comments */
         else if( !str_cmp( word, "COMMENT" ) )
            fread_old_comment( ch, fp );  /* Older Comments */
         else if( !str_cmp( word, "MOBILE" ) )
         {
            CHAR_DATA *mob = NULL;
            mob = fread_mobile( fp, false );
            if( mob )
            {
               bind_follower( mob, ch, -1, -2 );
               if( mob->in_room && ch->in_room )
               {
                  char_from_room( mob );
                  if( !char_to_room( mob, ch->in_room ) )
                     log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
               }
            }
         }
         else if( !str_cmp( word, "END" ) )  /* Done */
            break;
         else
         {
            bug( "Load_char_obj: bad section: %s", name );
            break;
         }
      }
      FCLOSE( fp );
      fpArea = NULL;
      mudstrlcpy( strArea, "$", MIL );
   }

   if( !found )
   {
      if( d )
      {
         if( d->mxp_detected )
            SET_PLR_FLAG( ch, PLR_MXP );

         if( d->msp_detected )
            SET_PLR_FLAG( ch, PLR_MSP );
      }
      ch->name = STRALLOC( name );
      ch->pcdata->editor = NULL;
      ch->pcdata->clan = NULL;
      ch->pcdata->council = NULL;
      ch->pcdata->deity = NULL;
      ch->first_pet = NULL;
      ch->last_pet = NULL;
      ch->pcdata->icq = 0; /* Samson 1-4-99 */
      ch->pcdata->low_vnum = 0;
      ch->pcdata->hi_vnum = 0;
      ch->pcdata->wizinvis = 0;
      ch->pcdata->balance = 0;   /* Initialize bank balance to zero - Samson */
      ch->pcdata->camp = 0;   /* Default to player hasn't camped - Samson 9-19-98 */
   }
   else
   {
      if( !ch->name )
         ch->name = STRALLOC( name );

      if( IS_PLR_FLAG( ch, PLR_FLEE ) )
         REMOVE_PLR_FLAG( ch, PLR_FLEE );

      if( IS_IMMORTAL( ch ) )
      {
         if( ch->pcdata->wizinvis < 2 )
            ch->pcdata->wizinvis = ch->level;
         assign_area( ch );
      }
      if( file_ver > 1 )
      {
         for( i = 0; i < MAX_WEAR; i++ )
            for( x = 0; x < MAX_LAYERS; x++ )
               if( save_equipment[i][x] )
               {
                  equip_char( ch, save_equipment[i][x], i );
                  save_equipment[i][x] = NULL;
               }
               else
                  break;
      }
      ClassSpecificStuff( ch );  /* Brought over from DOTD code - Samson 4-6-99 */
   }

   /*
    * Rebuild affected_by and RIS to catch errors - FB 
    */
   update_aris( ch );
   loading_char = NULL;
   return found;
}

/*
 * Added support for removeing so we could take out the write_corpses
 * so we could take it out of the save_char_obj function. --Shaddai
 */
void write_corpses( CHAR_DATA * ch, char *name, OBJ_DATA * objrem )
{
   OBJ_DATA *corpse;
   FILE *fp = NULL;

   /*
    * Name and ch support so that we dont have to have a char to save their
    * corpses.. (ie: decayed corpses while offline) 
    */
   if( ch && IS_NPC( ch ) )
   {
      bug( "%s", "Write_corpses: writing NPC corpse." );
      return;
   }
   if( ch )
      name = ch->name;
   /*
    * Go by vnum, less chance of screwups. -- Altrag 
    */
   for( corpse = first_object; corpse; corpse = corpse->next )
      if( corpse->pIndexData->vnum == OBJ_VNUM_CORPSE_PC && corpse->in_room != NULL
          && !str_cmp( corpse->short_descr + 14, name ) && objrem != corpse )
      {
         if( !fp )
         {
            char buf[256];

            snprintf( buf, 256, "%s%s", CORPSE_DIR, capitalize( name ) );
            if( !( fp = fopen( buf, "w" ) ) )
            {
               bug( "%s", "Write_corpses: Cannot open file." );
               perror( buf );
               return;
            }
         }
         fwrite_obj( ch, corpse, NULL, fp, 0, OS_CORPSE, FALSE );
      }
   if( fp )
   {
      fprintf( fp, "%s", "#END\n\n" );
      FCLOSE( fp );
   }
   else
   {
      char buf[256];

      snprintf( buf, 256, "%s%s", CORPSE_DIR, capitalize( name ) );
      remove( buf );
   }
   return;
}

void load_corpses( void )
{
   DIR *dp;
   struct dirent *de;

   if( !( dp = opendir( CORPSE_DIR ) ) )
   {
      bug( "Load_corpses: can't open %s", CORPSE_DIR );
      perror( CORPSE_DIR );
      return;
   }

   falling = 1;   /* Arbitrary, must be >0 though. */
   while( ( de = readdir( dp ) ) != NULL )
   {
      if( de->d_name[0] != '.' )
      {
         if( !str_cmp( de->d_name, "CVS" ) )
            continue;
         snprintf( strArea, MIL, "%s%s", CORPSE_DIR, de->d_name );
         fprintf( stderr, "Corpse -> %s\n", strArea );
         if( !( fpArea = fopen( strArea, "r" ) ) )
         {
            perror( strArea );
            continue;
         }
         for( ;; )
         {
            char letter;
            char *word;

            letter = fread_letter( fpArea );
            if( letter == '*' )
            {
               fread_to_eol( fpArea );
               continue;
            }
            if( letter != '#' )
            {
               bug( "%s", "Load_corpses: # not found." );
               break;
            }
            word = fread_word( fpArea );
            if( !str_cmp( word, "CORPSE" ) )
               fread_obj( NULL, fpArea, OS_CORPSE );
            else if( !str_cmp( word, "OBJECT" ) )
               fread_obj( NULL, fpArea, OS_CARRY );
            else if( !str_cmp( word, "END" ) )
               break;
            else
            {
               bug( "Load_corpses: bad section: %s", word );
               break;
            }
         }
         FCLOSE( fpArea );
      }
   }
   mudstrlcpy( strArea, "$", MIL );
   closedir( dp );
   falling = 0;
   return;
}

CMDF do_save( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
      return;

   WAIT_STATE( ch, 2 ); /* For big muds with save-happy players, like RoD */
   update_aris( ch );   /* update char affects and RIS */
   save_char_obj( ch );
   saving_char = NULL;
   send_to_char( "Saved...\n\r", ch );
   return;
}
