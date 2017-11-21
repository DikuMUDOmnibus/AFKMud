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
 *  The MUDprograms are heavily based on the original MOBprogram code that  *
 *                       was written by N'Atas-ha.                          *
 ****************************************************************************/

#include <string.h>
#include "mud.h"
#include "bits.h"
#include "deity.h"
#include "mud_prog.h"
#include "overland.h"
#include "polymorph.h"

extern int top_affect;

void raw_kill( CHAR_DATA * ch, CHAR_DATA * victim );
double ris_damage( CHAR_DATA * ch, double dam, int ris );
void damage_obj( OBJ_DATA * obj );
int recall( CHAR_DATA * ch, int target );
int get_langflag( char *flag );
int get_trigflag( char *flag );
DEITY_DATA *get_deity( char *name );
MORPH_DATA *get_morph( char *arg );
MORPH_DATA *get_morph_vnum( int arg );
BIT_DATA *get_qbit( CHAR_DATA * ch, int number );
void remove_qbit( CHAR_DATA * ch, int number );
void start_hunting( CHAR_DATA * ch, CHAR_DATA * victim );
void start_hating( CHAR_DATA * ch, CHAR_DATA * victim );
void start_fearing( CHAR_DATA * ch, CHAR_DATA * victim );
char *attribtext( int attribute );
void add_to_auth( CHAR_DATA * ch );
void addname( char **list, const char *name );
void music_to_char( const char *fname, int volume, CHAR_DATA * ch, bool toroom );

CMDF do_mpmset( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL], arg3[MIL];
   CHAR_DATA *victim;
   int value, v2;
   int minattr, maxattr;

   /*
    * A desc means switched.. too many loopholes if we allow that.. 
    */
   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) || ch->desc )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   smash_tilde( argument );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   mudstrlcpy( arg3, argument, MIL );

   if( !arg1 || arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "MpMset: no args" );
      return;
   }

   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      progbugf( ch, "%s", "MpMset: no victim" );
      return;
   }

   if( IS_IMMORTAL( victim ) )
   {
      send_to_char( "You can't do that!\n\r", ch );
      return;
   }

   if( IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
   {
      progbugf( ch, "%s", "MpMset: victim is proto" );
      return;
   }

   if( IS_NPC( victim ) )
   {
      minattr = 1;
      maxattr = 25;
   }
   else
   {
      minattr = 3;
      maxattr = 18;
   }

   value = is_number( arg3 ) ? atoi( arg3 ) : -1;
   if( atoi( arg3 ) < -1 && value == -1 )
      value = atoi( arg3 );

   if( !str_cmp( arg2, "str" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbugf( ch, "%s", "MpMset: Invalid str" );
         return;
      }
      victim->perm_str = value;
      return;
   }

   if( !str_cmp( arg2, "int" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbugf( ch, "%s", "MpMset: Invalid int" );
         return;
      }
      victim->perm_int = value;
      return;
   }

   if( !str_cmp( arg2, "wis" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbugf( ch, "%s", "MpMset: Invalid wis" );
         return;
      }
      victim->perm_wis = value;
      return;
   }

   if( !str_cmp( arg2, "dex" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbugf( ch, "%s", "MpMset: Invalid dex" );
         return;
      }
      victim->perm_dex = value;
      return;
   }

   if( !str_cmp( arg2, "con" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbugf( ch, "%s", "MpMset: Invalid con" );
         return;
      }
      victim->perm_con = value;
      return;
   }

   if( !str_cmp( arg2, "cha" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbugf( ch, "%s", "MpMset: Invalid cha" );
         return;
      }
      victim->perm_cha = value;
      return;
   }

   if( !str_cmp( arg2, "lck" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbugf( ch, "%s", "MpMset: Invalid lck" );
         return;
      }
      victim->perm_lck = value;
      return;
   }

   if( !str_cmp( arg2, "sav1" ) )
   {
      if( value < -30 || value > 30 )
      {
         progbugf( ch, "%s", "MpMset: Invalid sav1" );
         return;
      }
      victim->saving_poison_death = value;
      return;
   }

   if( !str_cmp( arg2, "sav2" ) )
   {
      if( value < -30 || value > 30 )
      {
         progbugf( ch, "%s", "MpMset: Invalid sav2" );
         return;
      }
      victim->saving_wand = value;
      return;
   }

   if( !str_cmp( arg2, "sav3" ) )
   {
      if( value < -30 || value > 30 )
      {
         progbugf( ch, "%s", "MpMset: Invalid sav3" );
         return;
      }
      victim->saving_para_petri = value;
      return;
   }

   if( !str_cmp( arg2, "sav4" ) )
   {
      if( value < -30 || value > 30 )
      {
         progbugf( ch, "%s", "MpMset: Invalid sav4" );
         return;
      }
      victim->saving_breath = value;
      return;
   }

   if( !str_cmp( arg2, "sav5" ) )
   {
      if( value < -30 || value > 30 )
      {
         progbugf( ch, "%s", "MpMset: Invalid sav5" );
         return;
      }
      victim->saving_spell_staff = value;
      return;
   }

   /*
    * Modified to allow mobs to set by name instead of number - Samson 7-30-98 
    */
   if( !str_cmp( arg2, "sex" ) )
   {
      switch ( arg3[0] )
      {
         case 'm':
         case 'M':
            value = SEX_MALE;
            break;
         case 'f':
         case 'F':
            value = SEX_FEMALE;
            break;
         case 'n':
         case 'N':
            value = SEX_NEUTRAL;
            break;
         case 'h':
         case 'H':
            value = SEX_HERMAPHRODYTE;
            break;
         default:
            progbugf( ch, "%s", "MpMset: Attempting to set invalid sex!" );
            return;
      }
      victim->sex = value;
      return;
   }

   /*
    * Modified to allow mudprogs to set PC Class/race during new creation process 
    */
   /*
    * Samson - 7-30-98 
    */
   if( !str_cmp( arg2, "Class" ) )
   {
      value = get_npc_class( arg3 );
      if( value < 0 )
         value = atoi( arg3 );
      if( !IS_NPC( victim ) )
      {
         if( value < 0 || value >= MAX_CLASS )
         {
            progbugf( ch, "%s", "MpMset: Attempting to set invalid player Class!" );
            return;
         }
         victim->Class = value;
         return;
      }
      if( IS_NPC( victim ) )
      {
         if( value < 0 || value >= MAX_NPC_CLASS )
         {
            progbugf( ch, "%s", "MpMset: Invalid npc Class" );
            return;
         }
         victim->Class = value;
         return;
      }
   }

   if( !str_cmp( arg2, "race" ) )
   {
      value = get_npc_race( arg3 );
      if( value < 0 )
         value = atoi( arg3 );
      if( !IS_NPC( victim ) )
      {
         if( value < 0 || value >= MAX_RACE )
         {
            progbugf( ch, "%s", "MpMset: Attempting to set invalid player race!" );
            return;
         }
         victim->race = value;
         return;
      }
      if( IS_NPC( victim ) )
      {
         if( value < 0 || value >= MAX_NPC_RACE )
         {
            progbugf( ch, "%s", "MpMset: Invalid npc race" );
            return;
         }
         victim->race = value;
         return;
      }
   }

   if( !str_cmp( arg2, "armor" ) )
   {
      if( value < -300 || value > 300 )
      {
         send_to_char( "AC range is -300 to 300.\n\r", ch );
         return;
      }
      victim->armor = value;
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc level" );
         return;
      }

      if( value < 0 || value > LEVEL_AVATAR + 5 )
      {
         progbugf( ch, "%s", "MpMset: Invalid npc level" );
         return;
      }
      victim->level = value;
      return;
   }

   if( !str_cmp( arg2, "numattacks" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc numattacks" );
         return;
      }

      if( value < 0 || value > 20 )
      {
         progbugf( ch, "%s", "MpMset: Invalid npc numattacks" );
         return;
      }
      victim->numattacks = ( float )( value );
      return;
   }

   if( !str_cmp( arg2, "gold" ) )
   {
      victim->gold = value;
      return;
   }

   if( !str_cmp( arg2, "hitroll" ) )
   {
      victim->hitroll = URANGE( 0, value, 85 );
      return;
   }

   if( !str_cmp( arg2, "damroll" ) )
   {
      victim->damroll = URANGE( 0, value, 65 );
      return;
   }

   if( !str_cmp( arg2, "hp" ) )
   {
      if( value < 1 || value > 32700 )
      {
         progbugf( ch, "%s", "MpMset: Invalid hp" );
         return;
      }
      victim->max_hit = value;
      return;
   }

   if( !str_cmp( arg2, "mana" ) )
   {
      if( value < 0 || value > 30000 )
      {
         progbugf( ch, "%s", "MpMset: Invalid mana" );
         return;
      }
      victim->max_mana = value;
      return;
   }

   if( !str_cmp( arg2, "move" ) )
   {
      if( value < 0 || value > 30000 )
      {
         progbugf( ch, "%s", "MpMset: Invalid move" );
         return;
      }
      victim->max_move = value;
      return;
   }

   if( !str_cmp( arg2, "practice" ) )
   {
      if( IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set npc practice" );
         return;
      }
      if( value < 0 || value > 500 )
      {
         progbugf( ch, "%s", "MpMset: Invalid practice" );
         return;
      }
      victim->pcdata->practice = value;
      return;
   }

   if( !str_cmp( arg2, "align" ) )
   {
      if( value < -1000 || value > 1000 )
      {
         progbugf( ch, "%s", "MpMset: Invalid align" );
         return;
      }
      victim->alignment = value;
      return;
   }

   if( !str_cmp( arg2, "favor" ) )
   {
      if( IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set npc favor" );
         return;
      }

      if( value < -2500 || value > 2500 )
      {
         progbugf( ch, "%s", "MpMset: Invalid pc favor" );
         return;
      }

      victim->pcdata->favor = value;
      return;
   }

   if( !str_cmp( arg2, "mentalstate" ) )
   {
      if( value < -100 || value > 100 )
      {
         progbugf( ch, "%s", "MpMset: Invalid mentalstate" );
         return;
      }
      victim->mental_state = value;
      return;
   }

   if( !str_cmp( arg2, "thirst" ) )
   {
      if( IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set npc thirst" );
         return;
      }

      if( value < 0 || value > 100 )
      {
         progbugf( ch, "%s", "MpMset: Invalid pc thirst" );
         return;
      }

      victim->pcdata->condition[COND_THIRST] = value;
      return;
   }

   if( !str_cmp( arg2, "drunk" ) )
   {
      if( IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set npc drunk" );
         return;
      }

      if( value < 0 || value > 100 )
      {
         progbugf( ch, "%s", "MpMset: Invalid pc drunk" );
         return;
      }

      victim->pcdata->condition[COND_DRUNK] = value;
      return;
   }

   if( !str_cmp( arg2, "full" ) )
   {
      if( IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set npc full" );
         return;
      }

      if( value < 0 || value > 100 )
      {
         progbugf( ch, "%s", "MpMset: Invalid pc full" );
         return;
      }

      victim->pcdata->condition[COND_FULL] = value;
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc name" );
         return;
      }

      STRFREE( victim->name );
      victim->name = STRALLOC( arg3 );
      return;
   }

   if( !str_cmp( arg2, "deity" ) )
   {
      DEITY_DATA *deity;

      if( IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set npc deity" );
         return;
      }

      if( !arg3 || arg3[0] == '\0' )
      {
         STRFREE( victim->pcdata->deity_name );
         victim->pcdata->deity = NULL;
         return;
      }

      deity = get_deity( arg3 );
      if( !deity )
      {
         progbugf( ch, "%s", "MpMset: Invalid deity" );
         return;
      }
      STRFREE( victim->pcdata->deity_name );
      victim->pcdata->deity_name = QUICKLINK( deity->name );
      victim->pcdata->deity = deity;
      return;
   }

   if( !str_cmp( arg2, "short" ) )
   {
      STRFREE( victim->short_descr );
      victim->short_descr = STRALLOC( arg3 );
      return;
   }

   if( !str_cmp( arg2, "long" ) )
   {
      stralloc_printf( &victim->long_descr, "%s\n\r", arg3 );
      return;
   }

   if( !str_cmp( arg2, "title" ) )
   {
      if( IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set npc title" );
         return;
      }

      set_title( victim, arg3 );
      return;
   }

   if( !str_cmp( arg2, "spec" ) || !str_cmp( arg2, "spec_fun" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc spec" );
         return;
      }

      if( !str_cmp( arg3, "none" ) )
      {
         victim->spec_fun = NULL;
         STRFREE( victim->spec_funname );
         return;
      }

      if( ( victim->spec_fun = m_spec_lookup( arg3 ) ) == NULL )
      {
         progbugf( ch, "%s", "MpMset: Invalid spec" );
         return;
      }
      STRFREE( victim->spec_funname );
      victim->spec_funname = STRALLOC( arg3 );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc flags" );
         return;
      }

      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpMset: no flags" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_actflag( arg3 );
         if( value < 0 || value > MAX_BITS )
            progbugf( ch, "MpMset: Invalid flag: %s", arg3 );
         else
         {
            if( value == ACT_PROTOTYPE )
               progbugf( ch, "%s", "MpMset: can't set prototype flag" );
            else if( value == ACT_IS_NPC )
               progbugf( ch, "%s", "MpMset: can't remove npc flag" );
            else
               xTOGGLE_BIT( victim->act, value );
         }
      }
      return;
   }

   if( !str_cmp( arg2, "affected" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't modify pc affected" );
         return;
      }

      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpMset: no affected" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_aflag( arg3 );
         if( value < 0 || value > MAX_BITS )
            progbugf( ch, "MpMset: Invalid affected: %s", arg3 );
         else
            xTOGGLE_BIT( victim->affected_by, value );
      }
      return;
   }

   /*
    * save some more finger-leather for setting RIS stuff
    * Why there's can_modify checks here AND in the called function, Ill
    * never know, so I removed them.. -- Alty
    */
   if( !str_cmp( arg2, "r" ) )
   {
      funcf( ch, do_mpmset, "%s resistant %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "i" ) )
   {
      funcf( ch, do_mpmset, "%s immune %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "s" ) )
   {
      funcf( ch, do_mpmset, "%s susceptible %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "ri" ) )
   {
      funcf( ch, do_mpmset, "%s resistant %s", arg1, arg3 );
      funcf( ch, do_mpmset, "%s immune %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "rs" ) )
   {
      funcf( ch, do_mpmset, "%s resistant %s", arg1, arg3 );
      funcf( ch, do_mpmset, "%s susceptible %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "is" ) )
   {
      funcf( ch, do_mpmset, "%s immune %s", arg1, arg3 );
      funcf( ch, do_mpmset, "%s susceptible %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "ris" ) )
   {
      funcf( ch, do_mpmset, "%s resistant %s", arg1, arg3 );
      funcf( ch, do_mpmset, "%s immune %s", arg1, arg3 );
      funcf( ch, do_mpmset, "%s susceptible %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "resistant" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc resistant" );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpMset: no resistant" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            progbugf( ch, "MpMset: Invalid resistant: %s", arg3 );
         else
            xTOGGLE_BIT( victim->resistant, value );
      }
      return;
   }

   if( !str_cmp( arg2, "immune" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc immune" );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpMset: no immune" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            progbugf( ch, "MpMset: Invalid immune: %s", arg3 );
         else
            xTOGGLE_BIT( victim->immune, value );
      }
      return;
   }

   if( !str_cmp( arg2, "susceptible" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc susceptible" );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpMset: no susceptible" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            progbugf( ch, "MpMset: Invalid susceptible: %s", arg3 );
         else
            xTOGGLE_BIT( victim->susceptible, value );
      }
      return;
   }

   if( !str_cmp( arg2, "absorb" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc absorb" );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpMset: no absorb" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            progbugf( ch, "MpMset: Invalid absorb: %s", arg3 );
         else
            xTOGGLE_BIT( victim->absorb, value );
      }
      return;
   }

   if( !str_cmp( arg2, "part" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc part" );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpMset: no part" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_partflag( arg3 );
         if( value < 0 || value > 31 )
            progbugf( ch, "MpMset: Invalid part: %s", arg3 );
         else
            TOGGLE_BIT( victim->xflags, 1 << value );
      }
      return;
   }

   if( !str_cmp( arg2, "attack" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc attack" );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpMset: no attack" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_attackflag( arg3 );
         if( value < 0 )
            progbugf( ch, "MpMset: Invalid attack: %s", arg3 );
         else
            xTOGGLE_BIT( victim->attacks, value );
      }
      return;
   }

   if( !str_cmp( arg2, "defense" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc defense" );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpMset: no defense" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_defenseflag( arg3 );
         if( value < 0 || value > MAX_BITS )
            progbugf( ch, "MpMset: Invalid defense: %s", arg3 );
         else
            xTOGGLE_BIT( victim->defenses, value );
      }
      return;
   }

   if( !str_cmp( arg2, "pos" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc pos" );
         return;
      }
      if( value < 0 || value > POS_STANDING )
      {
         progbugf( ch, "%s", "MpMset: Invalid pos" );
         return;
      }
      victim->position = value;
      return;
   }

   if( !str_cmp( arg2, "defpos" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc defpos" );
         return;
      }
      if( value < 0 || value > POS_STANDING )
      {
         progbugf( ch, "%s", "MpMset: Invalid defpos" );
         return;
      }
      victim->defposition = value;
      return;
   }

   if( !str_cmp( arg2, "speaks" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpMset: no speaks" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_langflag( arg3 );
         v2 = get_langnum( arg3 );
         if( value == LANG_UNKNOWN )
            progbugf( ch, "MpMset: Invalid speaks: %s", arg3 );
         else if( !IS_NPC( victim ) )
         {
            if( !( value &= VALID_LANGS ) )
            {
               progbugf( ch, "MpMset: Invalid player language: %s", arg3 );
               continue;
            }
            if( v2 == -1 )
               progbugf( ch, "MpMset: Unknown language: %s", arg3 );
            else
               TOGGLE_BIT( victim->speaks, 1 << v2 );
         }
         else
         {
            if( v2 == -1 )
               progbugf( ch, "MpMset: Unknown language: %s", arg3 );
            else
               TOGGLE_BIT( victim->speaks, 1 << v2 );
         }
      }
      if( !IS_NPC( victim ) )
      {
         REMOVE_BIT( victim->speaks, race_table[victim->race]->language );
         if( !knows_language( victim, victim->speaking, victim ) )
            victim->speaking = race_table[victim->race]->language;
      }
      return;
   }

   if( !str_cmp( arg2, "speaking" ) )
   {
      if( !IS_NPC( victim ) )
      {
         progbugf( ch, "%s", "MpMset: can't set pc speaking" );
         return;
      }
      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpMset: no speaking" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_langflag( arg3 );
         if( value == LANG_UNKNOWN )
            progbugf( ch, "MpMset: Invalid speaking: %s", arg3 );
         else
         {
            v2 = get_langnum( arg3 );
            if( v2 == -1 )
               progbugf( ch, "MpMset: Unknown language: %s", arg3 );
            else
               TOGGLE_BIT( victim->speaks, 1 << v2 );
         }
      }
      return;
   }

   progbugf( ch, "MpMset: Invalid field: %s", arg2 );
   return;
}

CMDF do_mposet( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL], arg3[MIL];
   OBJ_DATA *obj;
   int value, tmp;

   /*
    * A desc means switched.. too many loopholes if we allow that.. 
    */
   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) || ch->desc )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   smash_tilde( argument );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   mudstrlcpy( arg3, argument, MIL );

   if( !*arg1 )
   {
      progbugf( ch, "%s", "MpOset: no args" );
      return;
   }

   if( ( obj = get_obj_here( ch, arg1 ) ) == NULL )
   {
      progbugf( ch, "%s", "MpOset: no object" );
      return;
   }

   if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
   {
      progbugf( ch, "%s", "MpOset: can't set prototype items" );
      return;
   }
   separate_obj( obj );
   value = atoi( arg3 );

   if( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
   {
      obj->value[0] = value;
      return;
   }

   if( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
   {
      obj->value[1] = value;
      return;
   }

   if( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
   {
      obj->value[2] = value;
      return;
   }

   if( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
   {
      obj->value[3] = value;
      return;
   }

   if( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
   {
      obj->value[4] = value;
      return;
   }

   if( !str_cmp( arg2, "value5" ) || !str_cmp( arg2, "v5" ) )
   {
      if( obj->item_type == ITEM_CORPSE_PC )
      {
         progbugf( ch, "%s", "MpOset: Attempting to alter skeleton value for corpse" );
         return;
      }

      obj->value[5] = value;
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpOset: no type" );
         return;
      }
      value = get_otype( argument );
      if( value < 1 )
      {
         progbugf( ch, "%s", "MpOset: Invalid type" );
         return;
      }
      obj->item_type = value;
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpOset: no flags" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_oflag( arg3 );
         if( value < 0 || value > MAX_BITS )
            progbugf( ch, "MpOset: Invalid flag: %s", arg3 );
         else
         {
            if( value == ITEM_PROTOTYPE )
               progbugf( ch, "%s", "MpOset: can't set prototype flag" );
            else
               xTOGGLE_BIT( obj->extra_flags, value );
         }
      }
      return;
   }

   if( !str_cmp( arg2, "wear" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpOset: no wear" );
         return;
      }
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_wflag( arg3 );
         if( value < 0 || value > 31 )
            progbugf( ch, "MpOset: Invalid wear: %s", arg3 );
         else
            TOGGLE_BIT( obj->wear_flags, 1 << value );
      }

      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      obj->level = value;
      return;
   }

   if( !str_cmp( arg2, "weight" ) )
   {
      obj->weight = value;
      return;
   }

   if( !str_cmp( arg2, "cost" ) )
   {
      obj->cost = value;
      return;
   }

   if( !str_cmp( arg2, "timer" ) )
   {
      obj->timer = value;
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      STRFREE( obj->name );
      obj->name = STRALLOC( arg3 );
      return;
   }

   if( !str_cmp( arg2, "short" ) )
   {
      STRFREE( obj->short_descr );
      obj->short_descr = STRALLOC( arg3 );
      if( obj == supermob_obj )
      {
         STRFREE( supermob->short_descr );
         supermob->short_descr = QUICKLINK( obj->short_descr );
      }
      /*
       * Feature added by Narn, Apr/96 
       * * If the item is not proto, add the word 'rename' to the keywords
       * * if it is not already there.
       */
      if( str_infix( "mprename", obj->name ) )
         stralloc_printf( &obj->name, "%s %s", obj->name, "mprename" );
      return;
   }

   if( !str_cmp( arg2, "long" ) )
   {
      STRFREE( obj->objdesc );
      obj->objdesc = STRALLOC( arg3 );
      return;
   }

   if( !str_cmp( arg2, "actiondesc" ) )
   {
      if( strstr( arg3, "%n" ) || strstr( arg3, "%d" ) || strstr( arg3, "%l" ) )
      {
         progbugf( ch, "%s", "MpOset: Illegal actiondesc" );
         return;
      }
      STRFREE( obj->action_desc );
      obj->action_desc = STRALLOC( arg3 );
      return;
   }

   if( !str_cmp( arg2, "affect" ) )
   {
      AFFECT_DATA *paf;
      short loc;
      int bitv;

      argument = one_argument( argument, arg2 );
      if( arg2[0] == '\0' || !argument || argument[0] == 0 )
      {
         progbugf( ch, "%s", "MpOset: Bad affect syntax" );
         return;
      }
      loc = get_atype( arg2 );
      if( loc < 1 )
      {
         progbugf( ch, "%s", "MpOset: Invalid affect field" );
         return;
      }
      if( loc >= APPLY_AFFECT && loc < APPLY_WEAPONSPELL )
      {
         bitv = 0;
         while( argument[0] != '\0' )
         {
            argument = one_argument( argument, arg3 );
            if( loc == APPLY_AFFECT )
               value = get_aflag( arg3 );
            else
               value = get_risflag( arg3 );
            if( value < 0 || value > 31 )
               progbugf( ch, "MpOset: bad affect flag: %s", arg3 );
            else
               SET_BIT( bitv, 1 << value );
         }
         if( !bitv )
            return;
         value = bitv;
      }
      else
      {
         argument = one_argument( argument, arg3 );
         value = atoi( arg3 );
      }
      CREATE( paf, AFFECT_DATA, 1 );
      paf->type = -1;
      paf->duration = -1;
      paf->location = loc;
      paf->modifier = value;
      paf->bit = 0;
      paf->next = NULL;
      LINK( paf, obj->first_affect, obj->last_affect, next, prev );
      ++top_affect;
      return;
   }

   if( !str_cmp( arg2, "rmaffect" ) )
   {
      AFFECT_DATA *paf;
      short loc, count;

      if( !argument || argument[0] == '\0' )
      {
         progbugf( ch, "%s", "MpOset: no rmaffect" );
         return;
      }
      loc = atoi( argument );
      if( loc < 1 )
      {
         progbugf( ch, "%s", "MpOset: Invalid rmaffect" );
         return;
      }

      count = 0;

      for( paf = obj->first_affect; paf; paf = paf->next )
      {
         if( ++count == loc )
         {
            UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
            DISPOSE( paf );
            send_to_char( "Removed.\n\r", ch );
            --top_affect;
            return;
         }
      }
      progbugf( ch, "%s", "MpOset: rmaffect not found" );
      return;
   }

   /*
    * save some finger-leather
    */
   if( !str_cmp( arg2, "ris" ) )
   {
      funcf( ch, do_mposet, "%s affect resistant %s", arg1, arg3 );
      funcf( ch, do_mposet, "%s affect immune %s", arg1, arg3 );
      funcf( ch, do_mposet, "%s affect susceptible %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "r" ) )
   {
      funcf( ch, do_mpmset, "%s affect resistant %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "i" ) )
   {
      funcf( ch, do_mposet, "%s affect immune %s", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "s" ) )
   {
      funcf( ch, do_mposet, "%s affect susceptible %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "ri" ) )
   {
      funcf( ch, do_mposet, "%s affect resistant %s", arg1, arg3 );
      funcf( ch, do_mposet, "%s affect immune %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "rs" ) )
   {
      funcf( ch, do_mposet, "%s affect resistant %s", arg1, arg3 );
      funcf( ch, do_mposet, "%s affect susceptible %s", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "is" ) )
   {
      funcf( ch, do_mposet, "%s affect immune %s", arg1, arg3 );
      funcf( ch, do_mposet, "%s affect susceptible %s", arg1, arg3 );
      return;
   }

   /*
    * Make it easier to set special object values by name than number
    *                  -Thoric
    */
   tmp = -1;
   switch ( obj->item_type )
   {
      case ITEM_WEAPON:
         if( !str_cmp( arg2, "weapontype" ) )
         {
            unsigned int x;

            value = -1;
            for( x = 0; x < sizeof( attack_table ) / sizeof( attack_table[0] ); x++ )
               if( !str_cmp( arg3, attack_table[x] ) )
                  value = x;
            if( value < 0 )
            {
               progbugf( ch, "%s", "MpOset: Invalid weapon type" );
               return;
            }
            tmp = 3;
            break;
         }
         if( !str_cmp( arg2, "condition" ) )
            tmp = 0;
         break;
      case ITEM_ARMOR:
         if( !str_cmp( arg2, "condition" ) )
            tmp = 3;
         if( !str_cmp( arg2, "ac" ) )
            tmp = 1;
         break;
      case ITEM_SALVE:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "maxdoses" ) )
            tmp = 1;
         if( !str_cmp( arg2, "doses" ) )
            tmp = 2;
         if( !str_cmp( arg2, "delay" ) )
            tmp = 3;
         if( !str_cmp( arg2, "spell1" ) )
            tmp = 4;
         if( !str_cmp( arg2, "spell2" ) )
            tmp = 5;
         if( tmp >= 4 && tmp <= 5 )
            value = skill_lookup( arg3 );
         break;
      case ITEM_SCROLL:
      case ITEM_POTION:
      case ITEM_PILL:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "spell1" ) )
            tmp = 1;
         if( !str_cmp( arg2, "spell2" ) )
            tmp = 2;
         if( !str_cmp( arg2, "spell3" ) )
            tmp = 3;
         if( tmp >= 1 && tmp <= 3 )
            value = skill_lookup( arg3 );
         break;
      case ITEM_STAFF:
      case ITEM_WAND:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "spell" ) )
         {
            tmp = 3;
            value = skill_lookup( arg3 );
         }
         if( !str_cmp( arg2, "maxcharges" ) )
            tmp = 1;
         if( !str_cmp( arg2, "charges" ) )
            tmp = 2;
         break;
      case ITEM_CONTAINER:
         if( !str_cmp( arg2, "capacity" ) )
            tmp = 0;
         if( !str_cmp( arg2, "cflags" ) )
            tmp = 1;
         if( !str_cmp( arg2, "key" ) )
            tmp = 2;
         break;
      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
      case ITEM_BUTTON:
         if( !str_cmp( arg2, "tflags" ) )
         {
            tmp = 0;
            value = get_trigflag( arg3 );
         }
         break;
   }
   if( tmp >= 0 && tmp <= 3 )
   {
      obj->value[tmp] = value;
      return;
   }

   progbugf( ch, "MpOset: Invalid field: %s", arg2 );
   return;
}

char *mprog_type_to_name( int type )
{
   switch ( type )
   {
      case IN_FILE_PROG:
         return "in_file_prog";
      case ACT_PROG:
         return "act_prog";
      case SPEECH_PROG:
         return "speech_prog";
      case SPEECH_AND_PROG:
         return "speech_and_prog";
      case RAND_PROG:
         return "rand_prog";
      case FIGHT_PROG:
         return "fight_prog";
      case HITPRCNT_PROG:
         return "hitprcnt_prog";
      case DEATH_PROG:
         return "death_prog";
      case ENTRY_PROG:
         return "entry_prog";
      case GREET_PROG:
         return "greet_prog";
      case ALL_GREET_PROG:
         return "all_greet_prog";
      case GIVE_PROG:
         return "give_prog";
      case BRIBE_PROG:
         return "bribe_prog";
      case HOUR_PROG:
         return "hour_prog";
      case TIME_PROG:
         return "time_prog";
      case MONTH_PROG:
         return "month_prog";
      case WEAR_PROG:
         return "wear_prog";
      case REMOVE_PROG:
         return "remove_prog";
      case SAC_PROG:
         return "sac_prog";
      case LOOK_PROG:
         return "look_prog";
      case EXA_PROG:
         return "exa_prog";
      case ZAP_PROG:
         return "zap_prog";
      case GET_PROG:
         return "get_prog";
      case DROP_PROG:
         return "drop_prog";
      case REPAIR_PROG:
         return "repair_prog";
      case DAMAGE_PROG:
         return "damage_prog";
      case PULL_PROG:
         return "pull_prog";
      case PUSH_PROG:
         return "push_prog";
      case SCRIPT_PROG:
         return "script_prog";
      case SLEEP_PROG:
         return "sleep_prog";
      case REST_PROG:
         return "rest_prog";
      case LEAVE_PROG:
         return "leave_prog";
      case USE_PROG:
         return "use_prog";
      case KEYWORD_PROG:
         return "keyword_prog";
      default:
         return "ERROR_PROG";
   }
}

/* A trivial rehack of do_mstat.  This doesnt show all the data, but just
 * enough to identify the mob and give its basic condition.  It does however,
 * show the MUDprograms which are set.
 */
CMDF do_mpstat( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   MPROG_DATA *mprg;
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "MProg stat whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( !IS_NPC( victim ) )
   {
      send_to_char( "Only Mobiles can have MobPrograms!\n\r", ch );
      return;
   }

   if( xIS_EMPTY( victim->pIndexData->progtypes ) )
   {
      send_to_char( "That Mobile has no Programs set.\n\r", ch );
      return;
   }

   ch_printf( ch, "Name: %s.  Vnum: %d.\n\r", victim->name, victim->pIndexData->vnum );

   ch_printf( ch, "Short description: %s.\n\rLong  description: %s",
              victim->short_descr, victim->long_descr[0] != '\0' ? victim->long_descr : "(none).\n\r" );

   ch_printf( ch, "Hp: %d/%d.  Mana: %d/%d.  Move: %d/%d. \n\r",
              victim->hit, victim->max_hit, victim->mana, victim->max_mana, victim->move, victim->max_move );

   ch_printf( ch,
              "Lv: %d.  Class: %d.  Align: %d.  AC: %d.  Gold: %d.  Exp: %d.\n\r",
              victim->level, victim->Class, victim->alignment, GET_AC( victim ), victim->gold, victim->exp );

   for( mprg = victim->pIndexData->mudprogs; mprg; mprg = mprg->next )
      ch_printf( ch, "%s>%s %s\n\r%s\n\r", ( mprg->fileprog ? "(FILEPROG) " : "" ),
         mprog_type_to_name( mprg->type ), mprg->arglist, mprg->comlist );
   return;
}

/* Opstat - Scryn 8/12*/
CMDF do_opstat( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   MPROG_DATA *mprg;
   OBJ_DATA *obj;

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "OProg stat what?\n\r", ch );
      return;
   }

   if( ( obj = get_obj_world( ch, arg ) ) == NULL )
   {
      send_to_char( "You cannot find that.\n\r", ch );
      return;
   }

   if( xIS_EMPTY( obj->pIndexData->progtypes ) )
   {
      send_to_char( "That object has no programs set.\n\r", ch );
      return;
   }

   ch_printf( ch, "Name: %s.  Vnum: %d.\n\r", obj->name, obj->pIndexData->vnum );

   ch_printf( ch, "Short description: %s.\n\r", obj->short_descr );

   for( mprg = obj->pIndexData->mudprogs; mprg; mprg = mprg->next )
      ch_printf( ch, ">%s %s\n\r%s\n\r", mprog_type_to_name( mprg->type ), mprg->arglist, mprg->comlist );

   return;

}

/* Rpstat - Scryn 8/12 */
CMDF do_rpstat( CHAR_DATA * ch, char *argument )
{
   MPROG_DATA *mprg;

   if( xIS_EMPTY( ch->in_room->progtypes ) )
   {
      send_to_char( "This room has no programs set.\n\r", ch );
      return;
   }

   ch_printf( ch, "Name: %s.  Vnum: %d.\n\r", ch->in_room->name, ch->in_room->vnum );

   for( mprg = ch->in_room->mudprogs; mprg; mprg = mprg->next )
      ch_printf( ch, ">%s %s\n\r%s\n\r", mprog_type_to_name( mprg->type ), mprg->arglist, mprg->comlist );
   return;
}

/* Woowoo - Blodkai, November 1997 */
CMDF do_mpasupress( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   char arg2[MIL];
   CHAR_DATA *victim;
   int rnds;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpasupress:  invalid (nonexistent?) argument" );
      return;
   }
   if( arg2[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpasupress:  invalid (nonexistent?) argument" );
      return;
   }
   if( ( victim = get_char_room( ch, arg1 ) ) == NULL )
   {
      progbugf( ch, "%s", "Mpasupress:  victim not present" );
      return;
   }
   rnds = atoi( arg2 );
   if( rnds < 0 || rnds > 32000 )
   {
      progbugf( ch, "%s", "Mpsupress:  invalid (nonexistent?) argument" );
      return;
   }
   add_timer( victim, TIMER_ASUPRESSED, rnds, NULL, 0 );
   return;
}

/* lets the mobile kill any player or mobile without murder*/
CMDF do_mpkill( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;


   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }
   if( !ch )
   {
      bug( "%s", "Nonexistent ch in do_mpkill!" );
      return;
   }

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      progbugf( ch, "%s", "MpKill - no argument" );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      progbugf( ch, "MpKill - Victim %s not in room", arg );
      return;
   }

   if( victim == ch )
   {
      progbugf( ch, "%s", "MpKill - Bad victim (self) to attack" );
      return;
   }

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      progbugf( ch, "%s", "MpKill - Already fighting" );
      return;
   }
   multi_hit( ch, victim, TYPE_UNDEFINED );
   return;
}


/* lets the mobile destroy an object in its inventory
   it can also destroy a worn object and it can destroy
   items using all.xxxxx or just plain all of them */

CMDF do_mpjunk( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   OBJ_DATA *obj;
   OBJ_DATA *obj_next;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }


   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpjunk - No argument" );
      return;
   }

   if( str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
   {
      if( ( obj = get_obj_wear( ch, arg ) ) != NULL )
      {
         unequip_char( ch, obj );
         extract_obj( obj );
         return;
      }
      if( ( obj = get_obj_carry( ch, arg ) ) == NULL )
         return;
      extract_obj( obj );
   }
   else
      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         if( arg[3] == '\0' || is_name( &arg[4], obj->name ) )
         {
            if( obj->wear_loc != WEAR_NONE )
               unequip_char( ch, obj );
            extract_obj( obj );
         }
      }

   return;

}

/*
 * This function examines a text string to see if the first "word" is a
 * color indicator (e.g. _red, _whi_, _blu).  -  Gorog
 */
int get_color( char *argument )  /* get color code from command string */
{
   char color[MIL];
   char *cptr;
   static char const *color_list = "_bla_red_dgr_bro_dbl_pur_cya_cha_dch_ora_gre_yel_blu_pin_lbl_whi";
   static char const *blink_list = "*bla*red*dgr*bro*dbl*pur*cya*cha*dch*ora*gre*yel*blu*pin*lbl*whi";

   one_argument( argument, color );
   if( color[0] != '_' && color[0] != '*' )
      return 0;
   if( ( cptr = strstr( color_list, color ) ) )
      return ( cptr - color_list ) / 4;
   if( ( cptr = strstr( blink_list, color ) ) )
      return ( cptr - blink_list ) / 4 + AT_BLINK;
   return 0;
}

/* Prints the argument to all the rooms around the mobile */
CMDF do_mpasound( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   ROOM_INDEX_DATA *was_in_room;
   EXIT_DATA *pexit;
   short color;
   EXT_BV actflags;

   if( !ch )
   {
      bug( "%s", "Nonexistent ch in do_mpasound!" );
      return;
   }
   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpasound - No argument" );
      return;
   }
   actflags = ch->act;
   REMOVE_ACT_FLAG( ch, ACT_SECRETIVE );
   /*
    * DONT_UPPER prevents argument[0] from being captilized. --Shaddai 
    */
   DONT_UPPER = TRUE;
   if( ( color = get_color( argument ) ) )
      argument = one_argument( argument, arg1 );
   was_in_room = ch->in_room;
   for( pexit = was_in_room->first_exit; pexit; pexit = pexit->next )
   {
      if( pexit->to_room && pexit->to_room != was_in_room )
      {
         ch->in_room = pexit->to_room;
         MOBtrigger = FALSE;
         if( color )
            act( color, argument, ch, NULL, NULL, TO_ROOM );
         else
            act( AT_SAY, argument, ch, NULL, NULL, TO_ROOM );
      }
   }
   DONT_UPPER = FALSE;  /* Always set it back to FALSE */
   ch->act = actflags;
   ch->in_room = was_in_room;
   return;
}

/* prints the message to all in the room other than the mob and victim */
CMDF do_mpechoaround( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   EXT_BV actflags;
   short color;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpechoaround - No argument" );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbugf( ch, "Mpechoaround - victim %s does not exist", arg );
      return;
   }

   actflags = ch->act;
   REMOVE_ACT_FLAG( ch, ACT_SECRETIVE );

   if( ( color = get_color( argument ) ) )
   {
      argument = one_argument( argument, arg );
      act( color, argument, ch, NULL, victim, TO_NOTVICT );
   }
   else
      act( AT_ACTION, argument, ch, NULL, victim, TO_NOTVICT );

   ch->act = actflags;
}

/* prints message only to victim */
CMDF do_mpechoat( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   EXT_BV actflags;
   short color;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpechoat - No argument" );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbugf( ch, "Mpechoat - victim %s does not exist", arg );
      return;
   }

   actflags = ch->act;
   REMOVE_ACT_FLAG( ch, ACT_SECRETIVE );

   DONT_UPPER = TRUE;
   if( argument[0] == '\0' )
      act( AT_ACTION, " ", ch, NULL, victim, TO_VICT );
   else if( ( color = get_color( argument ) ) )
   {
      argument = one_argument( argument, arg );
      act( color, argument, ch, NULL, victim, TO_VICT );
   }
   else
      act( AT_ACTION, argument, ch, NULL, victim, TO_VICT );

   DONT_UPPER = FALSE;

   ch->act = actflags;
}

/* prints message to room at large. */
CMDF do_mpecho( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   short color;
   EXT_BV actflags;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   actflags = ch->act;
   REMOVE_ACT_FLAG( ch, ACT_SECRETIVE );

   DONT_UPPER = TRUE;
   if( argument[0] == '\0' )
      act( AT_ACTION, " ", ch, NULL, NULL, TO_ROOM );
   else if( ( color = get_color( argument ) ) )
   {
      argument = one_argument( argument, arg1 );
      act( color, argument, ch, NULL, NULL, TO_ROOM );
   }
   else
      act( AT_ACTION, argument, ch, NULL, NULL, TO_ROOM );
   DONT_UPPER = FALSE;
   ch->act = actflags;
}

/* Lets the mobile load an item or mobile. All itemsare loaded into inventory.
 * You can specify a level with the load object portion as well. 
 */
CMDF do_mpmload( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   MOB_INDEX_DATA *pMobIndex;
   CHAR_DATA *victim;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   one_argument( argument, arg );

   if( arg[0] == '\0' || !is_number( arg ) )
   {
      progbugf( ch, "Mpmload - Bad vnum %s as arg", arg ? arg : "NULL" );
      return;
   }

   if( !( pMobIndex = get_mob_index( atoi( arg ) ) ) )
   {
      progbugf( ch, "Mpmload - Bad mob vnum %s", arg );
      return;
   }

   victim = create_mobile( pMobIndex );
   if( !char_to_room( victim, ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   return;
}

CMDF do_mpoload( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   OBJ_DATA *obj;
   int level, timer = 0;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }


   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || !is_number( arg1 ) )
   {
      progbugf( ch, "%s", "Mpoload - Bad syntax" );
      return;
   }

   if( arg2[0] == '\0' )
      level = get_trust( ch );
   else
   {
      /*
       * New feature from Alander.
       */
      if( !is_number( arg2 ) )
      {
         progbugf( ch, "Mpoload - Bad level syntax: %s", arg2 );
         return;
      }
      level = atoi( arg2 );
      if( level < 0 || level > get_trust( ch ) )
      {
         progbugf( ch, "Mpoload - Bad level %d", level );
         return;
      }

      /*
       * New feature from Thoric.
       */
      timer = atoi( argument );
      if( timer < 0 )
      {
         progbugf( ch, "Mpoload - Bad timer %d", timer );
         return;
      }
   }

   if( !( obj = create_object( get_obj_index( atoi( arg1 ) ), level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      progbugf( ch, "Mpoload - Bad vnum arg %s", arg1 );
      return;
   }

   obj->timer = timer;
   if( CAN_WEAR( obj, ITEM_TAKE ) )
      obj_to_char( obj, ch );
   else
      obj_to_room( obj, ch->in_room, ch );
   return;
}

/* Just a hack of do_pardon from act_wiz.c -- Blodkai, 6/15/97 */
CMDF do_mppardon( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   char arg2[MIL];
   CHAR_DATA *victim;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      progbugf( ch, "%s", "Mppardon:  missing argument" );
      return;
   }
   if( ( victim = get_char_room( ch, arg1 ) ) == NULL )
   {
      progbugf( ch, "Mppardon: offender %s not present", arg1 );
      return;
   }
   if( IS_NPC( victim ) )
   {
      progbugf( ch, "Mppardon:  trying to pardon NPC %s", victim->short_descr );
      return;
   }
   if( !str_cmp( arg2, "litterbug" ) )
   {
      if( IS_PLR_FLAG( victim, PLR_LITTERBUG ) )
      {
         REMOVE_PLR_FLAG( victim, PLR_LITTERBUG );
         send_to_char( "Your crime of littering has been pardoned./n/r", victim );
      }
      return;
   }
   progbugf( ch, "%s", "Mppardon: Invalid argument" );
   return;
}

/* lets the mobile purge all objects and other npcs in the room,
   or purge a specified object or mob in the room.  It can purge
   itself, but this had best be the last command in the MUDprogram
   otherwise ugly stuff will happen */
CMDF do_mppurge( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *obj;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      /*
       * 'purge' 
       */
      CHAR_DATA *vnext;

      if( IS_ACT_FLAG( ch, ACT_ONMAP ) )
      {
         progbugf( ch, "%s", "mppurge: Room purge called from overland map" );
         return;
      }

      for( victim = ch->in_room->first_person; victim; victim = vnext )
      {
         vnext = victim->next_in_room;
         if( IS_NPC( victim ) && victim != ch )
            extract_char( victim, TRUE );
      }
      while( ch->in_room->first_content )
         extract_obj( ch->in_room->first_content );

      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      if( ( obj = get_obj_here( ch, arg ) ) != NULL )
      {
         extract_obj( obj );
      }
      else
         progbugf( ch, "%s", "Mppurge - Bad argument" );
      return;
   }

   if( !IS_NPC( victim ) )
   {
      progbugf( ch, "Mppurge - Trying to purge a PC %s", victim->name );
      return;
   }

   if( victim == ch )
   {
      progbugf( ch, "%s", "Mppurge - Trying to purge oneself" );
      return;
   }

   if( IS_NPC( victim ) && victim->pIndexData->vnum == MOB_VNUM_SUPERMOB )
   {
      progbugf( ch, "%s", "Mppurge: trying to purge supermob", ch );
      return;
   }
   extract_char( victim, TRUE );
   return;
}

/* Allow mobiles to go wizinvis with programs -- SB */
CMDF do_mpinvis( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   short level;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }


   argument = one_argument( argument, arg );
   if( arg && arg[0] != '\0' )
   {
      if( !is_number( arg ) )
      {
         progbugf( ch, "%s", "Mpinvis - Non numeric argument " );
         return;
      }
      level = atoi( arg );
      if( level < 2 || level > LEVEL_IMMORTAL ) /* Updated hardcode level check - Samson */
      {
         progbugf( ch, "MPinvis - Invalid level %d", level );
         return;
      }

      ch->mobinvis = level;
      return;
   }

   if( ch->mobinvis < 2 )
      ch->mobinvis = ch->level;

   if( IS_ACT_FLAG( ch, ACT_MOBINVIS ) )
   {
      REMOVE_ACT_FLAG( ch, ACT_MOBINVIS );
      act( AT_IMMORT, "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
   }
   else
   {
      SET_ACT_FLAG( ch, ACT_MOBINVIS );
      act( AT_IMMORT, "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
   }
   return;
}

/* lets the mobile goto any location it wishes that is not private */
/* Mounted chars follow their mobiles now - Blod, 11/97 */
CMDF do_mpgoto( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   ROOM_INDEX_DATA *location;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   one_argument( argument, arg );
   if( arg[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpgoto - No argument" );
      return;
   }

   if( ( location = find_location( ch, arg ) ) == NULL )
   {
      progbugf( ch, "Mpgoto - No such location %s", arg );
      return;
   }

   if( ch->fighting )
      stop_fighting( ch, TRUE );

   leave_map( ch, NULL, location );

   return;
}

/* lets the mobile do a command at another location. Very useful */
CMDF do_mpat( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   ROOM_INDEX_DATA *location;
   ROOM_INDEX_DATA *original;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }


   argument = one_argument( argument, arg );

   if( arg[0] == '\0' || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpat - Bad argument" );
      return;
   }

   if( ( location = find_location( ch, arg ) ) == NULL )
   {
      progbugf( ch, "Mpat - No such location %s", arg );
      return;
   }

   original = ch->in_room;
   char_from_room( ch );
   if( !char_to_room( ch, location ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   interpret( ch, argument );

   if( !char_died( ch ) )
   {
      char_from_room( ch );
      if( !char_to_room( ch, original ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }

   return;
}

/* allow a mobile to advance a player's level... very dangerous */
CMDF do_mpadvance( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   int level;
   int iLevel;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpadvance - Bad syntax" );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbugf( ch, "Mpadvance - Victim %s not there", arg );
      return;
   }

   if( IS_NPC( victim ) )
   {
      progbugf( ch, "Mpadvance - Victim %s is NPC" );
      return;
   }

   if( victim->level >= LEVEL_AVATAR )
      return;

   level = victim->level + 1;

   if( victim->level > ch->level )
   {
      act( AT_TELL, "$n tells you, 'Sorry... you must seek someone more powerful than I.'", ch, NULL, victim, TO_VICT );
      return;
   }

   switch ( level )
   {
      default:
         send_to_char( "You feel more powerful!\n\r", victim );
         break;
   }

   for( iLevel = victim->level; iLevel < level; iLevel++ )
   {
      if( level < LEVEL_IMMORTAL )
         send_to_char( "You raise a level!!  ", victim );
      victim->level += 1;
      advance_level( victim );
   }
   /*
    * Modified by Samson 4-30-99 
    */
   victim->exp = exp_level( victim->level );
   victim->trust = 0;
   return;
}

/* lets the mobile transfer people.  the all argument transfers
   everyone in the current room to the specified location 
   the area argument transfers everyone in the current area to the
   specified location */
CMDF do_mptransfer( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   ROOM_INDEX_DATA *location;
   CHAR_DATA *victim, *nextinroom, *immortal;
   DESCRIPTOR_DATA *d;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !arg1 || arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mptransfer - Bad syntax" );
      return;
   }

   /*
    * Put in the variable nextinroom to make this work right. -Narn 
    */
   if( !str_cmp( arg1, "all" ) )
   {
      for( victim = ch->in_room->first_person; victim; victim = nextinroom )
      {
         nextinroom = victim->next_in_room;
         if( IS_IMMORTAL( ch ) && ch->master != NULL )
            continue;

         if( victim != ch && can_see( ch, victim, TRUE ) )
            funcf( ch, do_mptransfer, "%s %s", victim->name, arg2 );
      }
      return;
   }

   /*
    * This will only transfer PC's in the area not Mobs --Shaddai 
    */
   if( !str_cmp( arg1, "area" ) )
   {
      for( d = first_descriptor; d; d = d->next )
      {
         if( !d->character || ( d->connected != CON_PLAYING && d->connected != CON_EDITING ) || !can_see( ch, d->character, FALSE ) || ch->in_room->area != d->character->in_room->area || d->character->level == 1 )   /* new auth */
            continue;
         funcf( ch, do_mptransfer, "%s %s", d->character->name, arg2 );
      }
      return;
   }

   /*
    * Thanks to Grodyn for the optional location parameter.
    */
   if( !arg2 || arg2[0] == '\0' )
      location = ch->in_room;
   else
   {
      if( !( location = find_location( ch, arg2 ) ) )
      {
         progbugf( ch, "Mptransfer - no such location: to: %s from: %d", arg2, ch->in_room->vnum );
         return;
      }

      if( room_is_private( location ) )
      {
         progbugf( ch, "Mptransfer - Private room %d", location->vnum );
         return;
      }
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      progbugf( ch, "Mptransfer - No such person %s", arg1 );
      return;
   }

   if( !victim->in_room )
   {
      progbugf( ch, "Mptransfer - Victim %s in Limbo", victim->name );
      return;
   }

   if( !IS_NPC( victim ) && victim->pcdata->release_date != 0 )
      progbugf( ch, "Mptransfer - helled character (%s)", victim->name );

   /*
    * Krusty/Wedgy protect 
    */
   if( IS_NPC( victim ) &&
       ( !str_cmp( victim->short_descr, "The Negative Magnetic Space Wedgy" )
         || !str_cmp( victim->short_descr, "Krusty" ) ) )
   {
      if( location->area != victim->pIndexData->area )
      {
         if( !str_cmp( victim->short_descr, "The Negative Magnetic Space Wedgy" ) )
            interpret( victim, "yell That was a most pathetic attempt to displace me, infidel." );
         else
         {
            if( str_cmp( location->area->name, "The Great Void" ) )
               interpret( victim, "yell AAARRGGHH! Quit trying to move me around dammit!" );
         }
      }
      return;
   }

   /*
    * If victim not in area's level range, do not transfer 
    */
   if( !in_hard_range( victim, location->area ) && !IS_ROOM_FLAG( location, ROOM_PROTOTYPE ) )
      return;

   if( victim->fighting )
      stop_fighting( victim, TRUE );

   /*
    * hey... if an immortal's following someone, they should go with a mortal
    * * when they're mptrans'd, don't you think?
    * *  -- TRI
    */
   for( immortal = victim->in_room->first_person; immortal; immortal = nextinroom )
   {
      nextinroom = immortal->next_in_room;
      if( IS_NPC( immortal ) || get_trust( immortal ) < LEVEL_IMMORTAL || immortal->master != victim )
         continue;
      if( immortal->fighting )
         stop_fighting( immortal, TRUE );
      leave_map( immortal, ch, location );
   }

   /*
    * Alert, cheesy hack sighted on scanners sir! 
    */
   victim->tempnum = 3210;
   leave_map( victim, ch, location );
   victim->tempnum = 0;

   act( AT_MAGIC, "A swirling vortex arrives, carrying $n!", victim, NULL, NULL, TO_ROOM );
   return;
}

/* lets the mobile force someone to do something.  must be mortal level
   and the all argument only affects those in the room with the mobile */
CMDF do_mpforce( CHAR_DATA * ch, char *argument )
{
   CMDTYPE *cmd = NULL;
   char arg[MIL], arg2[MIL];

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( !arg2 || arg2[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpforce - Bad syntax: Missing command" );
      return;
   }

   cmd = find_command( arg2 );


   if( !str_cmp( arg, "all" ) )
   {
      CHAR_DATA *vch, *vch_next;

      for( vch = ch->in_room->first_person; vch; vch = vch_next )
      {
         vch_next = vch->next_in_room;

         if( get_trust( vch ) < get_trust( ch ) && can_see( ch, vch, FALSE ) )
         {
            if( cmd && IS_CMD_FLAG( cmd, CMD_NOFORCE ) )
            {
               progbugf( ch, "Mpforce: Attempted to force all to %s - command is flagged noforce", arg2 );
               return;
            }
            interpret( vch, argument );
         }
      }
   }

   else
   {
      CHAR_DATA *victim;
      char buf[MSL];

      if( !( victim = get_char_room( ch, arg ) ) )
      {
         progbugf( ch, "Mpforce - No such victim %s", arg );
         return;
      }

      if( victim == ch )
      {
         progbugf( ch, "%s", "Mpforce - Forcing oneself" );
         return;
      }

      if( !IS_NPC( victim ) && ( !victim->desc ) && IS_IMMORTAL( victim ) )
      {
         progbugf( ch, "Mpforce - Attempting to force link dead immortal %s", victim->name );
         return;
      }

      /*
       * Commands with a CMD_NOFORCE flag will not be allowed to be mpforced Samson 3-3-04 
       */
      if( cmd && IS_CMD_FLAG( cmd, CMD_NOFORCE ) )
      {
         progbugf( ch, "Mpforce: Attempted to force %s to %s - command is flagged noforce", victim->name, cmd->name );
         return;
      }
      snprintf( buf, MSL, "%s ", arg2 );
      if( argument && argument[0] != '\0' )
         mudstrlcat( buf, argument, MSL );
      interpret( victim, buf );
   }
   return;
}

/*
 * mpbodybag for mobs to do cr's  --Shaddai
 */
CMDF do_mpbodybag( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   char arg[MSL], buf2[MSL], buf3[MSL];

   if( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpbodybag - called w/o enough argument(s)" );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbugf( ch, "Mpbodybag: victim %s not in room", arg );
      return;
   }
   if( IS_NPC( victim ) )
   {
      progbugf( ch, "%s", "Mpbodybag: bodybagging a npc corpse" );
      return;
   }
   mudstrlcpy( buf3, " ", MSL );
   snprintf( buf2, MSL, "the corpse of %s", arg );
   for( obj = first_object; obj; obj = obj->next )
   {
      if( obj->in_room && !str_cmp( buf2, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
      {
         obj_from_room( obj );
         obj = obj_to_char( obj, ch );
         obj->timer = -1;
      }
   }
   /*
    * Maybe should just make the command logged... Shrug I am not sure
    * * --Shaddai
    */
   progbugf( ch, "Mpbodybag: Grabbed %s", buf2 );
   return;
}

/*
 * mpmorph and mpunmorph for morphing people with mobs. --Shaddai
 */

CMDF do_mpmorph( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   MORPH_DATA *morph;
   char arg1[MIL];
   char arg2[MIL];

   if( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpmorph - called w/o enough argument(s)" );
      return;
   }

   if( ( victim = get_char_room( ch, arg1 ) ) == NULL )
   {
      progbugf( ch, "Mpmorph: victim %s not in room", arg1 );
      return;
   }


   if( !is_number( arg2 ) )
      morph = get_morph( arg2 );
   else
      morph = get_morph_vnum( atoi( arg2 ) );
   if( !morph )
   {
      progbugf( ch, "Mpmorph - unknown morph %s", arg2 );
      return;
   }
   if( victim->morph )
   {
      progbugf( ch, "Mpmorph - victim %s already morphed", victim->name );
      return;
   }
   do_morph_char( victim, morph );
   return;
}

CMDF do_mpunmorph( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   char arg[MSL];

   if( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpmorph - called w/o an argument" );
      return;
   }

   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbugf( ch, "Mpunmorph: victim %s not in room", arg );
      return;
   }
   if( !victim->morph )
   {
      progbugf( ch, "Mpunmorph: victim %s not morphed", victim->name );
      return;
   }
   do_unmorph_char( victim );
   return;
}

CMDF do_mpechozone( CHAR_DATA * ch, char *argument )  /* Blod, late 97 */
{
   char arg1[MIL];
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;
   short color;
   EXT_BV actflags;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   actflags = ch->act;
   REMOVE_ACT_FLAG( ch, ACT_SECRETIVE );
   if( ( color = get_color( argument ) ) )
      argument = one_argument( argument, arg1 );
   DONT_UPPER = TRUE;
   for( vch = first_char; vch; vch = vch_next )
   {
      vch_next = vch->next;
      if( vch->in_room->area == ch->in_room->area && !IS_NPC( vch ) && IS_AWAKE( vch ) )
      {
         if( argument[0] == '\0' )
            act( AT_ACTION, " ", vch, NULL, NULL, TO_CHAR );
         else if( color )
            act( color, argument, vch, NULL, NULL, TO_CHAR );
         else
            act( AT_ACTION, argument, vch, NULL, NULL, TO_CHAR );
      }
   }
   DONT_UPPER = FALSE;
   ch->act = actflags;
}

/*
 *  Haus' toys follow:
 */

/*
 * syntax:  mppractice victim spell_name max%
 *
 */
CMDF do_mp_practice( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   char arg2[MIL];
   char arg3[MIL];
   CHAR_DATA *victim;
   int sn, max, tmp, adept;
   char *skillname;

   if( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   if( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
   {
      progbugf( ch, "%s", "Mppractice - Bad syntax" );
      return;
   }

   if( ( victim = get_char_room( ch, arg1 ) ) == NULL )
   {
      progbugf( ch, "Mppractice: Invalid student %s not in room", arg1 );
      return;
   }

   if( ( sn = skill_lookup( arg2 ) ) < 0 )
   {
      progbugf( ch, "Mppractice: Invalid spell/skill name %s", arg2 );
      return;
   }

   if( IS_NPC( victim ) )
   {
      progbugf( ch, "%s", "Mppractice: Can't train a mob" );
      return;
   }

   skillname = skill_table[sn]->name;

   max = atoi( arg3 );
   if( ( max < 0 ) || ( max > 100 ) )
   {
      progbugf( ch, "mp_practice: Invalid maxpercent: %d", max );
      return;
   }

   if( victim->level < skill_table[sn]->skill_level[victim->Class] )
   {
      act_printf( AT_TELL, ch, NULL, victim, TO_VICT,
                  "$n attempts to tutor you in %s, but it's beyond your comprehension.", skillname );
      return;
   }

   /*
    * adept is how high the player can learn it 
    */
   /*
    * adept = class_table[ch->Class]->skill_adept; 
    */
   adept = GET_ADEPT( victim, sn );

   if( ( victim->pcdata->learned[sn] >= adept ) || ( victim->pcdata->learned[sn] >= max ) )
   {
      act_printf( AT_TELL, ch, NULL, victim, TO_VICT,
                  "$n shows some knowledge of %s, but yours is clearly superior.", skillname );
      return;
   }

   /*
    * past here, victim learns something 
    */
   tmp = UMIN( victim->pcdata->learned[sn] + int_app[get_curr_int( victim )].learn, max );
   act( AT_ACTION, "$N demonstrates $t to you. You feel more learned in this subject.", victim, skill_table[sn]->name, ch,
        TO_CHAR );

   victim->pcdata->learned[sn] = max;

   if( victim->pcdata->learned[sn] >= adept )
   {
      victim->pcdata->learned[sn] = adept;
      act( AT_TELL, "$n tells you, 'You have learned all I know on this subject...'", ch, NULL, victim, TO_VICT );
   }
   return;
}

CMDF do_mpstrew( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *obj_next;
   OBJ_DATA *obj_lose;
   ROOM_INDEX_DATA *pRoomIndex;
   int rvnum, vnum;

   set_char_color( AT_IMMORT, ch );

   if( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );

   if( arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpstrew: invalid (nonexistent?) argument" );
      return;
   }
   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      progbugf( ch, "Mpstrew: victim %s not in room", arg1 );
      return;
   }
   if( !argument || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpstrew: No command arguments" );
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
      return;
   }

   if( !str_cmp( argument, "inventory" ) )
   {

      for( ;; )
      {
         rvnum = number_range( 1, sysdata.maxvnum );
         pRoomIndex = get_room_index( rvnum );
         if( pRoomIndex && !IS_ROOM_FLAG( pRoomIndex, ROOM_MAP ) )
            break;
      }

      for( obj_lose = victim->first_carrying; obj_lose; obj_lose = obj_next )
      {
         obj_next = obj_lose->next_content;
         obj_from_char( obj_lose );
         obj_to_room( obj_lose, pRoomIndex, NULL );
         pager_printf( ch, "\t&w%s sent to %d\n\r", capitalize( obj_lose->short_descr ), pRoomIndex->vnum );
      }
      return;
   }

   vnum = atoi( argument );

   for( obj_lose = victim->first_carrying; obj_lose; obj_lose = obj_next )
   {
      obj_next = obj_lose->next_content;

      if( obj_lose->pIndexData->vnum == vnum )
      {
         obj_from_char( obj_lose );
         extract_obj( obj_lose );
      }
   }
   return;
}

CMDF do_mpscatter( CHAR_DATA * ch, char *argument )
{
   char arg1[MSL];
   CHAR_DATA *victim;
   ROOM_INDEX_DATA *pRoomIndex;
   int schance;

   if( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );

   if( !arg1 || arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpscatter: invalid (nonexistent?) argument" );
      return;
   }

   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      progbugf( ch, "Mpscatter: victim %s not in room", arg1 );
      return;
   }

   if( victim->level == LEVEL_SUPREME )
   {
      progbugf( ch, "Mpscatter: victim %s level too high", victim->name );
      return;
   }

   schance = number_range( 1, 2 );

   if( schance == 1 )
   {
      int map, x, y;
      short sector;

      for( ;; )
      {
         map = ( number_range( 1, 3 ) - 1 );
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
                && !IS_ROOM_FLAG( pRoomIndex, ROOM_SOLITARY ) && !IS_ROOM_FLAG( pRoomIndex, ROOM_PROTOTYPE ) )
               break;
      }
      if( victim->fighting )
         stop_fighting( victim, TRUE );
      act( AT_MAGIC, "With the sweep of an arm, $n flings $N to the astral winds.", ch, NULL, victim, TO_NOTVICT );
      act( AT_MAGIC, "With the sweep of an arm, $n flings you to the astral winds.", ch, NULL, victim, TO_VICT );
      act( AT_MAGIC, "With the sweep of an arm, you fling $N to the astral winds.", ch, NULL, victim, TO_CHAR );
      char_from_room( victim );
      if( !char_to_room( victim, pRoomIndex ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      victim->position = POS_RESTING;
      act( AT_MAGIC, "$n is deposited in a heap by the astral winds.", victim, NULL, NULL, TO_ROOM );
      interpret( victim, "look" );
   }
   return;
}

/*
 * syntax: mpslay (character)
 */
CMDF do_mp_slay( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   CHAR_DATA *victim;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpslay: invalid (nonexistent?) argument" );
      return;
   }

   if( ( victim = get_char_room( ch, arg1 ) ) == NULL )
   {
      progbugf( ch, "Mpslay: victim %s not in room", arg1 );
      return;
   }

   if( victim == ch )
   {
      progbugf( ch, "%s", "Mpslay: trying to slay self" );
      return;
   }

   if( IS_NPC( victim ) && victim->pIndexData->vnum == MOB_VNUM_SUPERMOB )
   {
      progbugf( ch, "%s", "Mpslay: trying to slay supermob" );
      return;
   }

   if( victim->level < LEVEL_SUPREME )
   {
      act( AT_IMMORT, "You slay $M in cold blood!", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n slays you in cold blood!", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n slays $N in cold blood!", ch, NULL, victim, TO_NOTVICT );
      raw_kill( ch, victim );
      stop_fighting( ch, FALSE );
      stop_hating( ch );
      stop_fearing( ch );
      stop_hunting( ch );
   }
   else
   {
      act( AT_IMMORT, "You attempt to slay $M, but the All Mighty protects $M!", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n attempts to slay you. The Almighty is snickering in the corner.", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n attempts to slay $N, but the All Mighty protects $M!", ch, NULL, victim, TO_NOTVICT );
   }
   return;
}

/*
 * Inflict damage from a mudprogram
 *
 *  note: should be careful about using victim afterwards
 */
ch_ret simple_damage( CHAR_DATA * ch, CHAR_DATA * victim, double dam, int dt )
{
   short dameq;
   bool npcvict;
   OBJ_DATA *damobj;
   ch_ret retcode;


   retcode = rNONE;

   if( !ch )
   {
      bug( "%s", "simple_damage: null ch!" );
      return rERROR;
   }
   if( !victim )
   {
      progbugf( ch, "%s", "simple_damage: null victim!" );
      return rVICT_DIED;
   }

   if( victim->position == POS_DEAD )
   {
      return rVICT_DIED;
   }

   npcvict = IS_NPC( victim );

   if( dam )
   {
      if( IS_FIRE( dt ) )
         dam = ris_damage( victim, dam, RIS_FIRE );
      else if( IS_COLD( dt ) )
         dam = ris_damage( victim, dam, RIS_COLD );
      else if( IS_ACID( dt ) )
         dam = ris_damage( victim, dam, RIS_ACID );
      else if( IS_ELECTRICITY( dt ) )
         dam = ris_damage( victim, dam, RIS_ELECTRICITY );
      else if( IS_ENERGY( dt ) )
         dam = ris_damage( victim, dam, RIS_ENERGY );
      else if( dt == gsn_poison )
         dam = ris_damage( victim, dam, RIS_POISON );
      else if( dt == ( TYPE_HIT + 7 ) || dt == ( TYPE_HIT + 8 ) )
         dam = ris_damage( victim, dam, RIS_BLUNT );
      else if( dt == ( TYPE_HIT + 2 ) || dt == ( TYPE_HIT + 11 ) )
         dam = ris_damage( victim, dam, RIS_PIERCE );
      else if( dt == ( TYPE_HIT + 1 ) || dt == ( TYPE_HIT + 3 ) )
         dam = ris_damage( victim, dam, RIS_SLASH );
      if( dam < 0 )
         dam = 0;
   }

   if( victim != ch )
   {
      /*
       * Damage modifiers.
       */
      if( IS_AFFECTED( victim, AFF_SANCTUARY ) )
         dam /= 2;

      if( IS_AFFECTED( victim, AFF_PROTECT ) && IS_EVIL( ch ) )
         dam -= ( int )( dam / 4 );

      if( dam < 0 )
         dam = 0;

      /*
       * dam_message( ch, victim, dam, dt ); 
       */
   }

   /*
    * Check for EQ damage.... ;)
    */

   if( dam > 10 )
   {
      /*
       * get a random body eq part 
       */
      dameq = number_range( WEAR_LIGHT, WEAR_EYES );
      damobj = get_eq_char( victim, dameq );
      if( damobj )
      {
         if( dam > get_obj_resistance( damobj ) )
            damage_obj( damobj );
      }
   }

   /*
    * Hurt the victim.
    * Inform the victim of his new state.
    */
   victim->hit -= ( short ) dam;
   if( !IS_NPC( victim ) && victim->level >= LEVEL_IMMORTAL && victim->hit < 1 )
      victim->hit = 1;

   if( !npcvict && get_trust( victim ) >= LEVEL_IMMORTAL && get_trust( ch ) >= LEVEL_IMMORTAL && victim->hit < 1 )
      victim->hit = 1;
   update_pos( victim );

   switch ( victim->position )
   {
      case POS_MORTAL:
         act( AT_DYING, "$n is mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_ROOM );
         act( AT_DANGER, "You are mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_CHAR );
         break;

      case POS_INCAP:
         act( AT_DYING, "$n is incapacitated and will slowly die, if not aided.", victim, NULL, NULL, TO_ROOM );
         act( AT_DANGER, "You are incapacitated and will slowly die, if not aided.", victim, NULL, NULL, TO_CHAR );
         break;

      case POS_STUNNED:
         if( !IS_AFFECTED( victim, AFF_PARALYSIS ) )
         {
            act( AT_ACTION, "$n is stunned, but will probably recover.", victim, NULL, NULL, TO_ROOM );
            act( AT_HURT, "You are stunned, but will probably recover.", victim, NULL, NULL, TO_CHAR );
         }
         break;

      case POS_DEAD:
         act( AT_DEAD, "$n is DEAD!!", victim, 0, 0, TO_ROOM );
         act( AT_DEAD, "You have been KILLED!!\n\r", victim, 0, 0, TO_CHAR );
         break;

      default:
         if( dam > victim->max_hit / 4 )
            act( AT_HURT, "That really did HURT!", victim, 0, 0, TO_CHAR );
         if( victim->hit < victim->max_hit / 4 )
            act( AT_DANGER, "You wish that your wounds would stop BLEEDING so much!", victim, 0, 0, TO_CHAR );
         break;
   }

   /*
    * Payoff for killing things.
    */
   if( victim->position == POS_DEAD )
   {
      if( !npcvict )
      {
         log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "%s (%d) killed by %s at %d",
                          victim->name, victim->level, ( IS_NPC( ch ) ? ch->short_descr : ch->name ),
                          victim->in_room->vnum );

         /*
          * Dying penalty:
          * 1/2 way back to previous level.
          */
         if( victim->exp > exp_level( victim->level ) )
            gain_exp( victim, ( exp_level( victim->level ) - victim->exp ) / 2 );
      }
      raw_kill( ch, victim );
      victim = NULL;

      return rVICT_DIED;
   }

   if( victim == ch )
      return rNONE;

   /*
    * Take care of link dead people.
    */
   if( !npcvict && !victim->desc )
   {
      if( number_range( 0, victim->wait ) == 0 )
      {
         recall( victim, -1 );
         return rNONE;
      }
   }

   /*
    * Wimp out?
    */
   if( npcvict && dam > 0 )
   {
      if( ( IS_ACT_FLAG( victim, ACT_WIMPY ) && number_bits( 1 ) == 0 && victim->hit < victim->max_hit / 2 )
          || ( IS_AFFECTED( victim, AFF_CHARM ) && victim->master && victim->master->in_room != victim->in_room ) )
      {
         start_fearing( victim, ch );
         stop_hunting( victim );
         interpret( victim, "flee" );
      }
   }

   if( !npcvict && victim->hit > 0 && victim->hit <= victim->wimpy && victim->wait == 0 )
      interpret( victim, "flee" );
   else if( !npcvict && IS_PLR_FLAG( victim, PLR_FLEE ) )
      interpret( victim, "flee" );

   tail_chain(  );
   return rNONE;
}

/*
 * syntax: mpdamage (character) (#hps)
 */
CMDF do_mp_damage( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim, *nextinroom;
   double dam;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !arg1 || arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpdamage: missing argument1" );
      return;
   }
   /*
    * Am I asking for trouble here or what?  But I need it. -- Blodkai 
    */
   if( !str_cmp( arg1, "all" ) )
   {
      for( victim = ch->in_room->first_person; victim; victim = nextinroom )
      {
         nextinroom = victim->next_in_room;
         if( victim != ch && can_see( ch, victim, FALSE ) ) /* Could go either way */
            funcf( ch, do_mp_damage, "'%s' %s", victim->name, arg2 );
      }
      return;
   }
   if( !arg2 || arg2[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpdamage: missing argument2" );
      return;
   }
   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      progbugf( ch, "Mpdamage: victim %s not in room", arg1 );
      return;
   }
   if( victim == ch )
   {
      progbugf( ch, "%s", "Mpdamage: trying to damage self" );
      return;
   }
   dam = atoi( arg2 );
   if( ( dam < 0 ) || ( dam > 32000 ) )
   {
      progbugf( ch, "Mpdamage: invalid (nonexistent?) argument %d", dam );
      return;
   }
   /*
    * this is kinda begging for trouble        
    */
   /*
    * Note from Thoric to whoever put this in...
    * Wouldn't it be better to call damage(ch, ch, dam, dt)?
    * I hate redundant code
    */
   if( simple_damage( ch, victim, dam, TYPE_UNDEFINED ) == rVICT_DIED )
   {
      stop_fighting( ch, FALSE );
      stop_hating( ch );
      stop_fearing( ch );
      stop_hunting( ch );
   }
   return;
}

CMDF do_mp_log( CHAR_DATA * ch, char *argument )
{
   struct tm *t = localtime( &current_time );

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "Mp_log:  non-existent entry" );
      return;
   }
   append_to_file( MOBLOG_FILE, "&p%-2.2d/%-2.2d | %-2.2d:%-2.2d  &P%s:  &p%s",
                   t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, ch->short_descr, argument );
   return;
}

/*
 * syntax: mprestore (character) (#hps)                Gorog
 */
CMDF do_mp_restore( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   char arg2[MIL];
   CHAR_DATA *victim;
   int hp;

   if( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mprestore: invalid argument1" );
      return;
   }

   if( arg2[0] == '\0' )
   {
      progbugf( ch, "%s", "Mprestore: invalid argument2" );
      return;
   }

   if( ( victim = get_char_room( ch, arg1 ) ) == NULL )
   {
      progbugf( ch, "Mprestore: victim %s not in room", arg1 );
      return;
   }

   hp = atoi( arg2 );

   if( ( hp < 0 ) || ( hp > 32000 ) )
   {
      progbugf( ch, "Mprestore: invalid hp amount %d", hp );
      return;
   }
   hp += victim->hit;
   victim->hit = ( hp > 32000 || hp < 0 || hp > victim->max_hit ) ? victim->max_hit : hp;
}

/*
 * Syntax mpfavor target number
 * Raise a player's favor in progs.
 */
CMDF do_mpfavor( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   char arg2[MIL];
   CHAR_DATA *victim;
   int favor;
   char *tmp;
   bool plus = FALSE, minus = FALSE;

   if( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
      return;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpfavor: invalid argument1" );
      return;
   }

   if( arg2[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpfavor: invalid argument2" );
      return;
   }

   tmp = arg2;
   if( tmp[0] == '+' )
   {
      plus = TRUE;
      tmp++;
      if( tmp[0] == '\0' )
      {
         progbugf( ch, "%s", "Mpfavor: invalid argument2" );
         return;
      }
   }
   else if( tmp[0] == '-' )
   {
      minus = TRUE;
      tmp++;
      if( tmp[0] == '\0' )
      {
         progbugf( ch, "%s", "Mpfavor: invalid argument2" );
         return;
      }
   }

   if( !( victim = get_char_room( ch, arg1 ) ) )
   {
      progbugf( ch, "Mpfavor: victim %s not in room", arg1 );
      return;
   }

   favor = atoi( tmp );
   if( plus )
      victim->pcdata->favor = URANGE( -2500, victim->pcdata->favor + favor, 2500 );
   else if( minus )
      victim->pcdata->favor = URANGE( -2500, victim->pcdata->favor - favor, 2500 );
   else
      victim->pcdata->favor = URANGE( -2500, favor, 2500 );
}

/*
 * Syntax mp_open_passage x y z
 *
 * opens a 1-way passage from room x to room y in direction z
 *
 *  won't mess with existing exits
 */
CMDF do_mp_open_passage( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL], arg3[MIL];
   ROOM_INDEX_DATA *targetRoom, *fromRoom;
   int targetRoomVnum, fromRoomVnum, exit_num = 0;
   EXIT_DATA *pexit;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   if( !arg1 || arg1[0] == '\0' )
   {
      progbug( "MpOpenPassage - Missing arg1.", ch );
      return;
   }
   if( !arg2 || arg2[0] == '\0' )
   {
      progbug( "MpOpenPassage - Missing arg2.", ch );
      return;
   }
   if( !arg3 || arg3[0] == '\0' )
   {
      progbug( "MpOpenPassage - Missing arg3", ch );
      return;
   }
   if( !is_number( arg1 ) )
   {
      progbug( "MpOpenPassage - arg1 isn't a number.", ch );
      return;
   }
   fromRoomVnum = atoi( arg1 );
   if( !( fromRoom = get_room_index( fromRoomVnum ) ) )
   {
      progbug( "MpOpenPassage - arg1 isn't an existing room.", ch );
      return;
   }
   if( !is_number( arg2 ) )
   {
      progbug( "MpOpenPassage - arg2 isn't a number.", ch );
      return;
   }
   targetRoomVnum = atoi( arg2 );
   if( !( targetRoom = get_room_index( targetRoomVnum ) ) )
   {
      progbug( "MpOpenPassage - arg2 isn't an existing room.", ch );
      return;
   }
   if( !is_number( arg3 ) )
   {
      if( ( exit_num = get_dirnum( arg3 ) ) < 0 )
      {
         progbug( "MpOpenPassage - arg3 isn't a valid direction name.", ch );
         return;
      }
   }
   else if( is_number( arg3 ) )
      exit_num = atoi( arg3 );

   if( ( exit_num < 0 ) || ( exit_num > MAX_DIR ) )
   {
      progbugf( ch, "MpOpenPassage - arg3 isn't a valid direction use a number from 0 - %d.", MAX_DIR );
      return;
   }
   if( ( pexit = get_exit( fromRoom, exit_num ) ) != NULL )
   {
      if( !IS_EXIT_FLAG( pexit, EX_PASSAGE ) )
         return;
      progbugf( ch, "MpOpenPassage - Exit %d already exists.", exit_num );
      return;
   }
   pexit = make_exit( fromRoom, targetRoom, exit_num );
   pexit->key = -1;
   xCLEAR_BITS( pexit->exit_info );
   SET_EXIT_FLAG( pexit, EX_PASSAGE );
   return;
}


/*
 * Syntax mp_fillin x
 * Simply closes the door
 */
CMDF do_mp_fill_in( CHAR_DATA * ch, char *argument )
{
   EXIT_DATA *pexit;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !( pexit = find_door( ch, argument, TRUE ) ) )
   {
      progbugf( ch, "MpFillIn - Exit %s does not exist", argument );
      return;
   }
   SET_EXIT_FLAG( pexit, EX_CLOSED );
   return;
}

/*
 * Syntax mp_close_passage x y 
 *
 * closes a passage in room x leading in direction y
 *
 * the exit must have EX_PASSAGE set
 */
CMDF do_mp_close_passage( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   ROOM_INDEX_DATA *fromRoom;
   int fromRoomVnum, exit_num = 0;
   EXIT_DATA *pexit;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !arg1 || arg1[0] == '\0' )
   {
      progbug( "MpClosePassage - Missing arg1.", ch );
      return;
   }
   if( !arg2 || arg2[0] == '\0' )
   {
      progbug( "MpClosePassage - Missing arg2.", ch );
      return;
   }
   if( !is_number( arg1 ) )
   {
      progbug( "MpClosePassage - arg1 isn't a number.", ch );
      return;
   }
   fromRoomVnum = atoi( arg1 );
   if( !( fromRoom = get_room_index( fromRoomVnum ) ) )
   {
      progbug( "MpClosePassage - arg1 isn't an existing room.", ch );
      return;
   }
   if( !is_number( arg2 ) )
   {
      if( ( exit_num = get_dirnum( arg2 ) ) < 0 )
      {
         progbug( "MpOpenPassage - arg3 isn't a valid direction name.", ch );
         return;
      }
   }
   else if( is_number( arg2 ) )
      exit_num = atoi( arg2 );
   if( ( exit_num < 0 ) || ( exit_num > MAX_DIR ) )
   {
      progbugf( ch, "MpClosePassage - arg2 isn't a valid direction use a number from 0 - %d.", MAX_DIR );
      return;
   }
   if( !( pexit = get_exit( fromRoom, exit_num ) ) )
      return;  /* already closed, ignore...  so rand_progs close without spam */
   if( !IS_EXIT_FLAG( pexit, EX_PASSAGE ) )
   {
      progbug( "MpClosePassage - Exit not a passage.", ch );
      return;
   }
   extract_exit( fromRoom, pexit );
   return;
}

/*
 * Does nothing.  Used for scripts.
 */
/* don't get any funny ideas about making this take an arg and pause to count it off! */
CMDF do_mpnothing( CHAR_DATA * ch, char *argument )
{
   if( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   return;
}

/*
 *   Sends a message to sleeping character.  Should be fun
 *    with room sleep_progs
 */
CMDF do_mpdream( CHAR_DATA * ch, char *argument )
{
   char arg1[MSL];
   CHAR_DATA *vict;

   if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );

   if( ( vict = get_char_world( ch, arg1 ) ) == NULL )
   {
      progbugf( ch, "Mpdream: No such character %s", arg1 );
      return;
   }

   if( vict->position <= POS_SLEEPING )
   {
      send_to_char( argument, vict );
      send_to_char( "\n\r", vict );
   }
   return;
}

CMDF do_mpdelay( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   int delay;

   if( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( !*arg )
   {
      progbugf( ch, "%s", "Mpdelay: no duration specified" );
      return;
   }
   if( !( victim = get_char_room( ch, arg ) ) )
   {
      progbugf( ch, "Mpdelay: target %s not in room", arg );
      return;
   }
   if( IS_IMMORTAL( victim ) )
   {
      progbugf( ch, "Mpdelay: target %s is immortal", victim->name );
      return;
   }
   argument = one_argument( argument, arg );
   if( !*arg || !is_number( arg ) )
   {
      progbugf( ch, "%s", "Mpdelay: invalid (nonexistant?) argument" );
      return;
   }
   delay = atoi( arg );
   if( delay < 1 || delay > 30 )
   {
      progbugf( ch, "Mpdelay:  argument %d out of range (1 to 30)", delay );
      return;
   }
   WAIT_STATE( victim, delay * sysdata.pulseviolence );
   send_to_char( "Mpdelay applied.\n\r", ch );
   return;
}

CMDF do_mppeace( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *rch;
   CHAR_DATA *victim;

   if( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) )
      return;

   argument = one_argument( argument, arg );
   if( !*arg )
   {
      progbugf( ch, "%s", "Mppeace: invalid (nonexistent?) argument" );
      return;
   }
   if( !str_cmp( arg, "all" ) )
   {
      for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
      {
         if( rch->fighting )
         {
            stop_fighting( rch, TRUE );
            interpret( rch, "sit" );
         }
         stop_hating( rch );
         stop_hunting( rch );
         stop_fearing( rch );
      }
      return;
   }
   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      progbugf( ch, "Mppeace: target %s not in room", arg );
      return;
   }
   if( victim->fighting )
      stop_fighting( victim, TRUE );
   stop_hating( ch );
   stop_hunting( ch );
   stop_fearing( ch );
   stop_hating( victim );
   stop_hunting( victim );
   stop_fearing( victim );
   return;
}

CMDF do_mpsindhae( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   char prizebuf[MSL];
   CHAR_DATA *victim;
   OBJ_INDEX_DATA *pObjIndex, *prizeindex = NULL;
   OBJ_DATA *prize, *temp;
   char *Class;
   int hash, tokencount, tokenstart, x;
   bool found;

   argument = one_argument( argument, arg );

   victim = get_char_room( ch, arg );

   if( victim == NULL )
   {
      progbugf( ch, "%s", "mpsindhae: No target for redemption!" );
      return;
   }

   if( !victim->in_room )
   {
      progbugf( ch, "mpsindhae: Redemption target %s in NULL room! Transplanting to Limbo.", victim->name );
      if( !char_to_room( victim, get_room_index( ROOM_VNUM_LIMBO ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }

   if( IS_NPC( victim ) )
   {
      progbugf( ch, "mpsindhae: NPC %s triggered the program, bouncing his butt to Bywater!", victim->short_descr );
      char_from_room( victim );
      if( !char_to_room( victim, get_room_index( ROOM_VNUM_TEMPLE ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }

   tokencount = 0;
   tokenstart = 5;

   if( !str_cmp( argument, "bronze" ) )
      tokenstart = 5;

   if( !str_cmp( argument, "silver" ) )
      tokenstart = 14;

   if( !str_cmp( argument, "gold" ) )
      tokenstart = 23;

   if( !str_cmp( argument, "platinum" ) )
      tokenstart = 32;

   if( get_qbit( victim, tokenstart ) != NULL )
      tokencount++;
   if( get_qbit( victim, tokenstart + 1 ) != NULL )
      tokencount++;
   if( get_qbit( victim, tokenstart + 2 ) != NULL )
      tokencount++;
   if( get_qbit( victim, tokenstart + 3 ) != NULL )
      tokencount++;
   if( get_qbit( victim, tokenstart + 4 ) != NULL )
      tokencount++;
   if( get_qbit( victim, tokenstart + 5 ) != NULL )
      tokencount++;
   if( get_qbit( victim, tokenstart + 6 ) != NULL )
      tokencount++;
   if( get_qbit( victim, tokenstart + 7 ) != NULL )
      tokencount++;
   if( get_qbit( victim, tokenstart + 8 ) != NULL )
      tokencount++;

   if( tokencount < 9 )
   {
      char_from_room( victim );
      if( !char_to_room( victim, get_room_index( ROOM_VNUM_REDEEM ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      interpret( victim, "look" );
      ch_printf( victim, "&BYou have not killed all 9 %s creatures yet.\n\r", argument );
      send_to_char( "You may not redeem your prize until you do.\n\r", victim );
      return;
   }

   snprintf( prizebuf, MSL, "%s-", argument );
   Class = npc_class[victim->Class];
   mudstrlcat( prizebuf, Class, MSL );

   found = FALSE;

   for( hash = 0; hash < MAX_KEY_HASH; hash++ )
   {
      for( pObjIndex = obj_index_hash[hash]; pObjIndex; pObjIndex = pObjIndex->next )
      {
         if( nifty_is_name( prizebuf, pObjIndex->name ) )
         {
            found = TRUE;
            prizeindex = pObjIndex;
            break;
         }
      }
   }

   if( !found || !prizeindex )
   {
      progbugf( ch, "mpsindhae: Unable to resolve prize index for %s", prizebuf );
      char_from_room( victim );
      if( !char_to_room( victim, get_room_index( ROOM_VNUM_REDEEM ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      interpret( victim, "look" );
      send_to_char( "&RAn internal error occured, please ask an immortal for assistance.\n\r", victim );
      return;
   }

   for( temp = first_object; temp; temp = temp->next )
   {
      if( temp->pIndexData->vnum == prizeindex->vnum && !str_cmp( temp->owner, victim->name ) )
      {
         progbugf( ch, "mpsindhae: victim already has %s prize", argument );
         char_from_room( victim );
         if( !char_to_room( victim, get_room_index( ROOM_VNUM_REDEEM ) ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         interpret( victim, "look" );
         ch_printf( victim, "&YYou already have a %s %s prize, you may not collect another yet.\n\r", argument, Class );
         return;
      }
   }

   if( !( prize = create_object( prizeindex, victim->level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }
   prize = obj_to_char( prize, victim );

   for( x = tokenstart; x < tokenstart + 9; x++ )
      remove_qbit( victim, x );

   log_printf( "%s is beginning redemption for %s %s Sindhae prize.", victim->name, argument, Class );

   ch_printf( victim, "&[magic]%s appears from the mists of the void.\n\r\n\r", prize->short_descr );

   SET_OBJ_FLAG( prize, ITEM_PERSONAL );
   STRFREE( prize->owner );
   prize->owner = STRALLOC( victim->name );

   send_to_char( "&GYou will now be asked to name your prize.\n\r", victim );
   send_to_char( "When the command prompt appears, enter the name you want your prize to have.\n\r", victim );
   send_to_char( "This will be the name other players will see when they look at you.\n\r", victim );
   send_to_char( "As always, if you get stuck, type 'help' at the command prompt.\n\r\n\r", victim );
   ch_printf( victim, "&RYou are editing %s.\n\r", prize->short_descr );
   write_to_buffer( victim->desc, "[SINDHAE] Prizename: ", 0 );
   victim->desc->connected = CON_PRIZENAME;
   victim->pcdata->spare_ptr = prize;
   return;
}

CMDF do_mpgwraggedd( CHAR_DATA * ch, char *argument )
{
   ROOM_INDEX_DATA *pRoomIndex = NULL;
   CHAR_DATA *victim;
   char arg[MIL];
   short target;

   argument = one_argument( argument, arg );

   victim = get_char_room( ch, arg );

   if( victim == NULL )
   {
      progbugf( ch, "%s", "mpgwraggedd: Teleport victim is NULL!" );
      return;
   }

   target = number_range( 1, 4 );

   switch ( target )
   {
      case 1:
         pRoomIndex = get_room_index( 2805 );
         break;
      case 2:
         pRoomIndex = get_room_index( 2790 );
         break;
      case 3:
         pRoomIndex = get_room_index( 552 );
         break;
      case 4:
         pRoomIndex = get_room_index( 7950 );
         break;
   }

   if( pRoomIndex == NULL )
   {
      progbugf( ch, "mpgwraggedd: Teleport failed, target room %d doesn't exist!", target );
      return;
   }

   char_from_room( victim );
   if( !char_to_room( victim, pRoomIndex ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   return;
}

/* Copy a PC, creating a doppleganger - Samson 10-11-99 */
CHAR_DATA *make_doppleganger( CHAR_DATA * ch )
{
   CHAR_DATA *mob;
   MOB_INDEX_DATA *pMobIndex;

   pMobIndex = get_mob_index( MOB_DOPPLEGANGER );

   if( !pMobIndex )
   {
      bug( "%s", "make_doppleganger: Doppleganger mob not found!" );
      return NULL;
   }

   CREATE( mob, CHAR_DATA, 1 );
   clear_char( mob );
   mob->pIndexData = pMobIndex;

   stralloc_printf( &mob->name, "%s doppleganger", ch->name );
   stralloc_printf( &mob->short_descr, "%s", ch->name );
   stralloc_printf( &mob->long_descr, "%s%s is here before you.", ch->name, ch->pcdata->title );

   if( ch->chardesc && ch->chardesc[0] != '\0' )
      mob->chardesc = QUICKLINK( ch->chardesc );
   else
      mob->chardesc = STRALLOC( "Boring generic something." );
   mob->race = ch->race;
   mob->Class = ch->Class;
   STRFREE( mob->spec_funname );

   switch ( mob->Class )
   {
      case CLASS_MAGE:
         mob->spec_fun = m_spec_lookup( "spec_cast_mage" );
         mob->spec_funname = STRALLOC( "spec_cast_mage" );
         break;

      case CLASS_CLERIC:
         mob->spec_fun = m_spec_lookup( "spec_cast_cleric" );
         mob->spec_funname = STRALLOC( "spec_cast_cleric" );
         break;

      case CLASS_WARRIOR:
         mob->spec_fun = m_spec_lookup( "spec_warrior" );
         mob->spec_funname = STRALLOC( "spec_warrior" );
         break;

      case CLASS_ROGUE:
         mob->spec_fun = m_spec_lookup( "spec_thief" );
         mob->spec_funname = STRALLOC( "spec_thief" );
         break;

      case CLASS_RANGER:
         mob->spec_fun = m_spec_lookup( "spec_ranger" );
         mob->spec_funname = STRALLOC( "spec_ranger" );
         break;

      case CLASS_PALADIN:
         mob->spec_fun = m_spec_lookup( "spec_paladin" );
         mob->spec_funname = STRALLOC( "spec_paladin" );
         break;

      case CLASS_DRUID:
         mob->spec_fun = m_spec_lookup( "spec_druid" );
         mob->spec_funname = STRALLOC( "spec_druid" );
         break;

      case CLASS_ANTIPALADIN:
         mob->spec_fun = m_spec_lookup( "spec_antipaladin" );
         mob->spec_funname = STRALLOC( "spec_antipaladin" );
         break;

      case CLASS_BARD:
         mob->spec_fun = m_spec_lookup( "spec_bard" );
         mob->spec_funname = STRALLOC( "spec_bard" );
         break;

      default:
         mob->spec_fun = NULL;
         break;
   }

   mob->mpscriptpos = 0;
   mob->level = ch->level;
   mob->act = pMobIndex->act;
   SET_ACT_FLAG( mob, ACT_AGGRESSIVE );
   mob->map = ch->map;
   mob->x = ch->x;
   mob->y = ch->y;

   if( IS_ACT_FLAG( mob, ACT_MOBINVIS ) )
      mob->mobinvis = mob->level;

   mob->affected_by = ch->affected_by;

   SET_AFFECTED( mob, AFF_DETECT_INVIS );
   SET_AFFECTED( mob, AFF_DETECT_HIDDEN );
   SET_AFFECTED( mob, AFF_TRUESIGHT );
   SET_AFFECTED( mob, AFF_INFRARED );

   mob->alignment = ch->alignment;
   mob->sex = ch->sex;
   mob->armor = GET_AC( ch );
   mob->max_hit = ch->max_hit;
   mob->hit = mob->max_hit;
   mob->gold = 0;
   mob->exp = 0;
   mob->position = POS_STANDING;
   mob->defposition = POS_STANDING;
   mob->barenumdie = ch->barenumdie;
   mob->baresizedie = ch->baresizedie;
   mob->mobthac0 = ch->mobthac0;
   mob->hitplus = GET_HITROLL( ch );
   mob->damplus = GET_DAMROLL( ch );
   mob->perm_str = get_curr_str( ch );
   mob->perm_wis = get_curr_wis( ch );
   mob->perm_int = get_curr_int( ch );
   mob->perm_dex = get_curr_dex( ch );
   mob->perm_con = get_curr_con( ch );
   mob->perm_cha = get_curr_cha( ch );
   mob->perm_lck = get_curr_lck( ch );
   mob->hitroll = ch->hitroll;
   mob->damroll = ch->damroll;
   mob->saving_poison_death = ch->saving_poison_death;
   mob->saving_wand = ch->saving_wand;
   mob->saving_para_petri = ch->saving_para_petri;
   mob->saving_breath = ch->saving_breath;
   mob->saving_spell_staff = ch->saving_spell_staff;
   mob->height = ch->height;
   mob->weight = ch->weight;
   mob->resistant = ch->resistant;
   mob->immune = ch->immune;
   mob->susceptible = ch->susceptible;
   mob->absorb = ch->absorb;
   mob->attacks = ch->attacks;
   mob->defenses = ch->defenses;
   mob->numattacks = ch->numattacks;
   mob->speaks = ch->speaks;
   mob->speaking = ch->speaking;
   mob->xflags = race_bodyparts( ch );
   xCLEAR_BITS( mob->no_affected_by );
   mob->no_resistant = ch->no_resistant;
   mob->no_immune = ch->no_immune;
   mob->no_susceptible = ch->no_susceptible;

   /*
    * Insert in list.
    */
   LINK( mob, first_char, last_char, next, prev );
   pMobIndex->count++;
   nummobsloaded++;

   return ( mob );
}

/* Equip the doppleganger with everything the PC has - Samson 10-11-99 */
void equip_doppleganger( CHAR_DATA * ch, CHAR_DATA * mob )
{
   OBJ_DATA *obj, *obj_next, *newobj;

   for( obj = ch->first_carrying; obj != NULL; obj = obj_next )
   {
      obj_next = obj->next_content;
      if( obj->wear_loc != WEAR_NONE )
      {
         newobj = clone_object( obj );
         SET_OBJ_FLAG( newobj, ITEM_DEATHROT );
         obj_to_char( newobj, mob );
      }
   }

   return;
}

CMDF do_mpdoppleganger( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *mob, *host;
   ROOM_INDEX_DATA *pRoomIndex;

   argument = one_argument( argument, arg );

   host = get_char_room( ch, arg );

   if( !host )
   {
      progbugf( ch, "%s", "mpdoppleganger: Attempting to copy NULL host!" );
      return;
   }

   if( host->in_room != ch->in_room )
   {
      progbugf( ch, "mpdoppleganger: Cannot copy host PC %s, not in same room!", host->name );
      return;
   }

   pRoomIndex = host->in_room;

   mob = make_doppleganger( host );

   if( !mob )
   {
      progbugf( ch, "mpdoppleganger: Failure to create doppleganer out of %s!", host->name );
      return;
   }
   if( !char_to_room( mob, pRoomIndex ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   equip_doppleganger( host, mob );
}

/* Mob prog used by Lord Arthmoor - Samson 6-1-99 */
CMDF do_mparthmoor( CHAR_DATA * ch, char *argument )
{
   OBJ_INDEX_DATA *pObjIndex;
   OBJ_DATA *obj;
   MOB_INDEX_DATA *pMobIndex;
   CHAR_DATA *mob;
   ROOM_INDEX_DATA *pRoomIndex;
   int i, j;

   if( !ch )
   {
      progbugf( ch, "%s", "mparthmoor: Lord Arthmoor is missing!" );
      return;
   }

   if( !ch->in_room )
   {
      progbugf( ch, "%s", "mparthmoor: Guard summon called from NULL room!" );
      return;
   }

   if( !( pMobIndex = get_mob_index( 25108 ) ) )
   {
      progbugf( ch, "%s", "mparthmoor: Sentry guard 25108 not found!!" );
      return;
   }

   if( !( pRoomIndex = get_room_index( 25296 ) ) )
   {
      progbugf( ch, "%s", "mparthmoor: Guard summoning room 25296 not found!!" );
      return;
   }

   /*
    * Create 5 guards, and equip each with their standard gear - Samson 
    */
   for( i = 1; i < 6; i++ )
   {
      mob = create_mobile( pMobIndex );
      if( !char_to_room( mob, pRoomIndex ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      for( j = 25117; j < 25124; j++ )
      {
         if( ( pObjIndex = get_obj_index( j ) ) == NULL )
         {
            progbugf( ch, "mparthmoor: Object %d not available!", j );
            continue;
         }
         obj = create_object( pObjIndex, mob->level );
         obj_to_char( obj, mob );
      }
      interpret( mob, "wear all" );
      interpret( mob, "north" );
   }
   return;
}

/*
 * "Roll" players stats based on the character name		-Thoric
 */
/* Rewritten by Whir. Thanks to Vor/Casteele for help 2-1-98 */
/* Racial bonus calculations moved to this function and removed from comm.c - Samson 2-2-98 */
/* Updated to AD&D standards by Samson 9-5-98 */
/* Changed to use internal random number generator instead of OS dependant random() function - Samson 9-5-98 */
void name_stamp_stats( CHAR_DATA * ch )
{
   ch->perm_str = 6 + dice( 2, 6 );
   ch->perm_dex = 6 + dice( 2, 6 );
   ch->perm_wis = 6 + dice( 2, 6 );
   ch->perm_int = 6 + dice( 2, 6 );
   ch->perm_con = 6 + dice( 2, 6 );
   ch->perm_cha = 6 + dice( 2, 6 );
   ch->perm_lck = 6 + dice( 2, 6 );

   ch->perm_str += race_table[ch->race]->str_plus;
   ch->perm_int += race_table[ch->race]->int_plus;
   ch->perm_wis += race_table[ch->race]->wis_plus;
   ch->perm_dex += race_table[ch->race]->dex_plus;
   ch->perm_con += race_table[ch->race]->con_plus;
   ch->perm_cha += race_table[ch->race]->cha_plus;
   ch->perm_lck += race_table[ch->race]->lck_plus;
}

/* Sets up newbie default values for new creation system - Samson 1-2-99 */
void setup_newbie( CHAR_DATA * ch, bool NEWLOGIN )
{
   OBJ_DATA *obj;
   OBJ_INDEX_DATA *objcheck;
   RACE_TYPE *race;

   int iLang;

   ch->Class = CLASS_WARRIOR; /* Default for new PC - Samson 8-4-98 */
   ch->race = RACE_HUMAN;  /* Default for new PC - Samson 8-4-98 */
   ch->pcdata->clan = NULL;

   race = race_table[ch->race];

   /*
    * Set to zero as default values - Samson 9-5-98 
    */
   ch->affected_by = race->affected;
   ch->attacks = race->attacks;
   ch->defenses = race->defenses;
   ch->saving_poison_death = 0;
   ch->saving_wand = 0;
   ch->saving_para_petri = 0;
   ch->saving_breath = 0;
   ch->saving_spell_staff = 0;

   ch->alignment = 0;   /* Oops, forgot to set this. Causes trouble for restarts :) */

   ch->height = number_range( ( int )( race->height * .8 ), ( int )( race->height * 1.2 ) );
   ch->weight = number_range( ( int )( race->weight * .8 ), ( int )( race->weight * 1.2 ) );

   if( ( iLang = skill_lookup( "common" ) ) < 0 )
      bug( "%s", "setup_newbie: cannot find common language." );
   else
      ch->pcdata->learned[iLang] = 100;

   name_stamp_stats( ch ); /* Initialize first stat roll for new PC - Samson */

   ch->level = 1;
   ch->exp = 0;

   /*
    * Set player birthday to current mud day, -17 years - Samson 10-25-99 
    */
   ch->pcdata->day = time_info.day;
   ch->pcdata->month = time_info.month;
   ch->pcdata->year = time_info.year - 17;
   ch->pcdata->age = 17;
   ch->pcdata->age_bonus = 0;

   /*
    * Set recall point to the Abecedarium - Samson 5-13-01 
    */
   ch->pcdata->alsherok = 2914;  /* Red Dragon Inn */

   ch->max_hit = 10;
   ch->hit = ch->max_hit;

   ch->max_mana = 100;
   ch->mana = ch->max_mana;

   ch->max_move = 150;
   ch->move = ch->max_move;

   set_title( ch, "the Newbie" );

   /*
    * Added by Narn. Start new characters with autoexit and autgold already turned on. Very few people don't use those. 
    */
   SET_PLR_FLAG( ch, PLR_AUTOGOLD );
   SET_PLR_FLAG( ch, PLR_AUTOEXIT );

   /*
    * Added by Brittany, Nov 24/96.  The object is the adventurer's guide to Alsherok, part of newbie.are. 
    */
   /*
    * Modified by Samson to use variable so object can be moved to a new zone if needed - 9-5-98 
    */
   objcheck = get_obj_index( OBJ_VNUM_NEWBIE_GUIDE );
   if( objcheck != NULL )
   {
      if( !( obj = create_object( get_obj_index( OBJ_VNUM_NEWBIE_GUIDE ), 0 ) ) )
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      else
         obj_to_char( obj, ch );
   }
   else
      bug( "setup_newbie: Newbie Guide object %d not found.", OBJ_VNUM_NEWBIE_GUIDE );

   objcheck = get_obj_index( OBJ_VNUM_SCHOOL_BANNER );
   if( objcheck != NULL )
   {
      if( !( obj = create_object( get_obj_index( OBJ_VNUM_SCHOOL_BANNER ), 0 ) ) )
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      else
      {
         obj_to_char( obj, ch );
         equip_char( ch, obj, WEAR_LIGHT );
      }
   }
   else
      bug( "setup_newbie: Newbie light object %d not found.", OBJ_VNUM_SCHOOL_BANNER );

   if( !NEWLOGIN )
      return;

   if( !sysdata.WAIT_FOR_AUTH )  /* Altered by Samson 4-12-98 */
   {
      if( !char_to_room( ch, get_room_index( ROOM_NOAUTH_START ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }
   else
   {
      if( !char_to_room( ch, get_room_index( ROOM_AUTH_START ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      SET_PCFLAG( ch, PCFLAG_UNAUTHED );
      add_to_auth( ch );   /* new auth */
   }
   music_to_char( "creation.mid", 100, ch, FALSE );
   addname( &ch->pcdata->chan_listen, "chat" );
   return;
}

/* Alignment setting for Class during creation - Samson 4-17-98 */
void class_create_check( CHAR_DATA * ch )
{
   switch ( ch->Class )
   {
      default:   /* Any other Class not listed below */
         ch->alignment = 0;
         break;

      case CLASS_ANTIPALADIN:   /* Antipaladin */
         ch->alignment = -1000;
         break;

      case CLASS_PALADIN: /* Paladin */
         ch->alignment = 1000;
         break;

      case CLASS_RANGER:
         ch->alignment = 350;
         break;
   }
   return;
}

/* Strip PC & unlearn racial language when asking to restart creation - Samson 10-12-98 */
CMDF do_mpredo( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj, *obj_next;
   RACE_TYPE *race;
   int sn;

   if( !IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( ( victim = get_char_world( ch, argument ) ) == NULL )
   {
      progbugf( ch, "%s", "Mpredo - No such person" );
      return;
   }

   if( !victim->desc )
   {
      progbugf( ch, "Mpredo - Victim %s has no descriptor!", victim->name );
      return;
   }

   if( !victim->in_room )
   {
      progbugf( ch, "Mpredo - Victim %s in Limbo", victim->name );
      return;
   }

   if( IS_NPC( victim ) )
   {
      progbugf( ch, "Mpredo - Victim %s is an NPC", victim->short_descr );
      return;
   }

   race = race_table[victim->race];

   log_printf( "%s is restarting creation from end room.\n\r", victim->name );

   for( obj = victim->first_carrying; obj != NULL; obj = obj_next )
   {
      obj_next = obj->next_content;
      if( obj->wear_loc != WEAR_NONE )
         unequip_char( victim, obj );
   }

   for( obj = victim->first_carrying; obj; obj = obj_next )
   {
      obj_next = obj->next_content;
      separate_obj( obj );
      obj_from_char( obj );
      if( !obj_next )
         obj_next = victim->first_carrying;
      extract_obj( obj );
   }

   for( sn = 0; sn < top_sn; sn++ )
      victim->pcdata->learned[sn] = 0;

   setup_newbie( victim, FALSE );
   return;
}

/* Final pre-entry setup for new characters - Samson 8-6-98 */
CMDF do_mpraceset( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   char arg1[MIL], buf[MSL];
   RACE_TYPE *race;
   CLASS_TYPE *Class;
   char *classname;
   OBJ_DATA *obj;
   int iLang;

   if( !IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );

   if( !arg1 || arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpraceset - Bad syntax. No argument!" );
      return;
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      progbugf( ch, "Mpraceset - No such person %s", arg1 );
      return;
   }

   if( !victim->desc )
   {
      progbugf( ch, "Mpraceset - Victim %s has no descriptor!", victim->name );
      return;
   }

   if( !victim->in_room )
   {
      progbugf( ch, "Mpraceset - Victim %s in Limbo", victim->name );
      return;
   }

   if( IS_NPC( victim ) )
   {
      progbugf( ch, "Mpraceset - Victim %s is an NPC", victim->name );
      return;
   }

   race = race_table[victim->race];
   Class = class_table[victim->Class];
   classname = Class->who_name;

   victim->affected_by = race->affected;
   victim->armor += race->ac_plus;
   victim->attacks = race->attacks;
   victim->defenses = race->defenses;
   victim->saving_poison_death = race->saving_poison_death;
   victim->saving_wand = race->saving_wand;
   victim->saving_para_petri = race->saving_para_petri;
   victim->saving_breath = race->saving_breath;
   victim->saving_spell_staff = race->saving_spell_staff;

   victim->height = number_range( ( int )( race->height * .75 ), ( int )( race->height * 1.25 ) );
   victim->weight = number_range( ( int )( race->weight * .75 ), ( int )( race->weight * 1.25 ) );

   for( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ )
      if( lang_array[iLang] == race->language )
         break;

   if( lang_array[iLang] == LANG_UNKNOWN )
      progbugf( ch, "%s", "Mpraceset: invalid racial language." );
   else
   {
      if( ( iLang = skill_lookup( lang_names[iLang] ) ) < 0 )
         progbugf( ch, "%s", "Mpraceset: cannot find racial language." );
      else
         victim->pcdata->learned[iLang] = 100;
   }

   class_create_check( victim ); /* Checks Class for proper alignment on creation - Samson 4-17-98 */

   victim->max_hit = 15 + con_app[get_curr_con( victim )].hitp + number_range( Class->hp_min, Class->hp_max ) + race->hit;
   victim->hit = victim->max_hit;

   victim->max_mana = 100 + race->mana;
   victim->mana = victim->max_mana;

   snprintf( buf, MSL, "the %s", title_table[victim->Class][victim->level][victim->sex == SEX_FEMALE ? 1 : 0] );
   set_title( victim, buf );

   if( Class->weapon != -1 )
   {
      if( !( obj = create_object( get_obj_index( Class->weapon ), 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         bug( "Class weapon %d not found for Class %s.", Class->weapon, classname );
      }
      else
      {
         obj_to_char( obj, victim );
         equip_char( victim, obj, WEAR_WIELD );
      }
   }

   if( Class->armor != -1 )
   {
      if( !( obj = create_object( get_obj_index( Class->armor ), 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         bug( "Class armor %d not found for Class %s.", Class->armor, classname );
      }
      else
      {
         obj_to_char( obj, victim );
         equip_char( victim, obj, WEAR_BODY );
      }
   }

   if( Class->legwear != -1 )
   {
      if( !( obj = create_object( get_obj_index( Class->legwear ), 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         bug( "Class legwear %d not found for Class %s.", Class->legwear, classname );
      }
      else
      {
         obj_to_char( obj, victim );
         equip_char( victim, obj, WEAR_LEGS );
      }
   }

   if( Class->headwear != -1 )
   {
      if( !( obj = create_object( get_obj_index( Class->headwear ), 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         bug( "Class headwear %d not found for Class %s.", Class->headwear, classname );
      }
      else
      {
         obj_to_char( obj, victim );
         equip_char( victim, obj, WEAR_HEAD );
      }
   }

   if( Class->armwear != -1 )
   {
      if( !( obj = create_object( get_obj_index( Class->armwear ), 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         bug( "Class armwear %d not found for Class %s.", Class->armwear, classname );
      }
      else
      {
         obj_to_char( obj, victim );
         equip_char( victim, obj, WEAR_ARMS );
      }
   }

   if( Class->footwear != -1 )
   {
      if( !( obj = create_object( get_obj_index( Class->footwear ), 1 ) ) && Class->footwear )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         bug( "Class footwear %d not found for Class %s.", Class->footwear, classname );
      }
      else
      {
         obj_to_char( obj, victim );
         equip_char( victim, obj, WEAR_FEET );
      }
   }

   if( Class->shield != -1 )
   {
      if( !( obj = create_object( get_obj_index( Class->shield ), 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         bug( "Class shield %d not found for Class %s.", Class->shield, classname );
      }
      else
      {
         obj_to_char( obj, victim );
         equip_char( victim, obj, WEAR_SHIELD );
      }
   }

   if( Class->held != -1 )
   {
      if( !( obj = create_object( get_obj_index( Class->held ), 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         bug( "Class held %d not found for Class %s.", Class->held, classname );
      }
      else
      {
         obj_to_char( obj, victim );
         equip_char( victim, obj, WEAR_HOLD );
      }
   }
   return;
}

/* Stat rerolling function for new PC creation system - Samson 8-4-98 */
CMDF do_mpstatreroll( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( !IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpstatreroll - Bad syntax. No argument!" );
      return;
   }

   if( !( victim = get_char_world( ch, argument ) ) )
   {
      progbugf( ch, "Mpstatreroll - No such person %s", argument );
      return;
   }

   if( !victim->desc )
   {
      progbugf( ch, "Mpstatreroll - Victim %s has no descriptor!", victim->name );
      return;
   }

   if( !victim->in_room )
   {
      progbugf( ch, "Mpstatreroll - Victim %s in Limbo", victim->name );
      return;
   }

   write_to_buffer( victim->desc, "You may roll as often as you like.\n\r", 0 );

   name_stamp_stats( victim );

   buffer_printf( victim->desc, "\n\rStr: %s\n\r", attribtext( victim->perm_str ) );
   buffer_printf( victim->desc, "Int: %s\n\r", attribtext( victim->perm_int ) );
   buffer_printf( victim->desc, "Wis: %s\n\r", attribtext( victim->perm_wis ) );
   buffer_printf( victim->desc, "Dex: %s\n\r", attribtext( victim->perm_dex ) );
   buffer_printf( victim->desc, "Con: %s\n\r", attribtext( victim->perm_con ) );
   buffer_printf( victim->desc, "Cha: %s\n\r", attribtext( victim->perm_cha ) );
   buffer_printf( victim->desc, "Lck: %s\n\r", attribtext( victim->perm_lck ) );
   write_to_buffer( victim->desc, "\n\rKeep these stats? (Y/N)", 0 );
   victim->desc->connected = CON_ROLL_STATS;
   return;
}

/* Copy of mptransfer with a do_look attatched - Samson 4-14-98 */
CMDF do_mptrlook( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   ROOM_INDEX_DATA *location;
   CHAR_DATA *victim, *nextinroom;

   if( IS_AFFECTED( ch, AFF_CHARM ) )
      return;

   if( !IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !arg1 || arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mptrlook - Bad syntax" );
      return;
   }

   /*
    * Put in the variable nextinroom to make this work right. -Narn 
    */
   if( !str_cmp( arg1, "all" ) )
   {
      for( victim = ch->in_room->first_person; victim; victim = nextinroom )
      {
         nextinroom = victim->next_in_room;
         if( victim != ch && victim->level > 1 && can_see( ch, victim, TRUE ) )
            funcf( ch, do_mptrlook, "%s %s", victim->name, arg2 );
      }
      return;
   }

   /*
    * Thanks to Grodyn for the optional location parameter.
    */
   if( arg2 && arg2[0] == '\0' )
      location = ch->in_room;
   else
   {
      if( !( location = find_location( ch, arg2 ) ) )
      {
         progbugf( ch, "Mptrlook - No such location %s", arg2 );
         return;
      }

      if( room_is_private( location ) )
      {
         progbugf( ch, "Mptrlook - Private room %d", location->vnum );
         return;
      }
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      progbugf( ch, "Mptrlook - No such person %s", arg1 );
      return;
   }

   if( !victim->in_room )
   {
      progbugf( ch, "Mptrlook - Victim %s in Limbo", victim->name );
      return;
   }

   if( !IS_NPC( victim ) && victim->pcdata->release_date != 0 )
      progbugf( ch, "Mptrlook - helled character (%s)", victim->name );

   /*
    * Krusty/Wedgy protect 
    */
   if( IS_NPC( victim ) &&
       ( !str_cmp( victim->short_descr, "The Negative Magnetic Space Wedgy" )
         || !str_cmp( victim->short_descr, "Krusty" ) ) )
   {
      if( location->area != victim->pIndexData->area )
      {
         if( !str_cmp( victim->short_descr, "The Negative Magnetic Space Wedgy" ) )
            interpret( victim, "yell That was a most pathetic attempt to displace me, infidel." );
         else
         {
            if( str_cmp( location->area->name, "The Great Void" ) )
               interpret( victim, "yell AAARRGGHH! Quit trying to move me around dammit!" );
         }
      }
      return;
   }

   /*
    * If victim not in area's level range, do not transfer 
    */
   if( !in_hard_range( victim, location->area ) && !IS_ROOM_FLAG( location, ROOM_PROTOTYPE ) )
      return;

   if( victim->fighting )
      stop_fighting( victim, TRUE );

   leave_map( victim, ch, location );
   act( AT_MAGIC, "A swirling vortex arrives, carrying $n!", victim, NULL, NULL, TO_ROOM );
   return;
}

/* New mob hate, hunt, and fear code courtesy Rjael of Saltwind MUD Installed by Samson 4-14-98 */
CMDF do_mphate( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim, *master, *mob;
   int vnum;

   if( !IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mphate - Bad syntax, bad victim" );
      return;
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      progbugf( ch, "Mphate - No such person %s" );
      return;
   }
   else if( IS_NPC( victim ) )
   {
      if( IS_AFFECTED( victim, AFF_CHARM ) && ( master = victim->master ) )
      {
         if( !( victim = get_char_world( ch, master->name ) ) )
         {
            progbugf( ch, "Mphate - NULL NPC Master for %s", victim->name );
            return;
         }
      }
      else
      {
         progbugf( ch, "Mphate - NPC victim %s", victim->short_descr );
         return;
      }
   }

   if( arg2[0] == '\0' )
   {
      progbugf( ch, "%s", "Mphate - bad syntax, no aggressor" );
      return;
   }
   else
   {
      if( is_number( arg2 ) )
      {
         vnum = atoi( arg2 );
         if( vnum < 1 || vnum > sysdata.maxvnum )
         {
            progbugf( ch, "Mphate -- aggressor vnum %d out of range", vnum );
            return;
         }
      }
      else
      {
         progbugf( ch, "%s", "Mphate -- aggressor no vnum" );
         return;
      }
   }
   for( mob = first_char; mob; mob = mob->next )
   {
      if( !IS_NPC( mob ) || !mob->in_room || !mob->pIndexData->vnum )
         continue;

      if( vnum == mob->pIndexData->vnum )
         start_hating( mob, victim );
   }
}

CMDF do_mphunt( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim, *master, *mob;
   int vnum;

   if( !IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mphunt - Bad syntax, bad victim" );
      return;
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      progbugf( ch, "Mphunt - No such person %s", arg1 );
      return;
   }
   else if( IS_NPC( victim ) )
   {
      if( IS_AFFECTED( victim, AFF_CHARM ) && ( master = victim->master ) )
      {
         if( !( victim = get_char_world( ch, master->name ) ) )
         {
            progbugf( ch, "Mphunt - NULL NPC Master for %s", victim->name );
            return;
         }
      }
      else
      {
         progbugf( ch, "Mphunt - NPC victim %s", victim->short_descr );
         return;
      }
   }

   if( arg2[0] == '\0' )
   {
      progbugf( ch, "%s", "Mphunt - bad syntax, no aggressor" );
      return;
   }
   else
   {
      if( is_number( arg2 ) )
      {
         vnum = atoi( arg2 );
         if( vnum < 1 || vnum > sysdata.maxvnum )
         {
            progbugf( ch, "Mphunt -- aggressor vnum %d out of range", vnum );
            return;
         }
      }
      else
      {
         progbugf( ch, "%s", "Mphunt -- aggressor no vnum" );
         return;
      }
   }
   for( mob = first_char; mob; mob = mob->next )
   {
      if( !IS_NPC( mob ) || !mob->in_room || !mob->pIndexData->vnum )
         continue;

      if( vnum == mob->pIndexData->vnum )
         start_hunting( mob, victim );
   }
}

CMDF do_mpfear( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim, *master, *mob;
   int vnum;

   if( !IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpfear - Bad syntax, bad victim" );
      return;
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      progbugf( ch, "Mpfear - No such person %s", arg1 );
      return;
   }
   else if( IS_NPC( victim ) )
   {
      if( IS_AFFECTED( victim, AFF_CHARM ) && ( master = victim->master ) )
      {
         if( !( victim = get_char_world( ch, master->name ) ) )
         {
            progbugf( ch, "Mpfear - NULL NPC Master for %s", victim->name );
            return;
         }
      }
      else
      {
         progbugf( ch, "Mpfear - NPC victim %s", victim->short_descr );
         return;
      }
   }

   if( arg2[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpfear - bad syntax, no aggressor" );
      return;
   }
   else
   {
      if( is_number( arg2 ) )
      {
         vnum = atoi( arg2 );
         if( vnum < 1 || vnum > sysdata.maxvnum )
         {
            progbugf( ch, "Mpfear -- aggressor vnum %d out of range", vnum );
            return;
         }
      }
      else
      {
         progbugf( ch, "%s", "Mpfear -- aggressor no vnum" );
         return;
      }
   }
   for( mob = first_char; mob; mob = mob->next )
   {
      if( !IS_NPC( mob ) || !mob->in_room || !mob->pIndexData->vnum )
         continue;

      if( vnum == mob->pIndexData->vnum )
         start_fearing( mob, victim );
   }
}
