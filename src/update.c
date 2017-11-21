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
 *                          Regular update module                           *
 ****************************************************************************/

#include <sys/time.h>
#include "mud.h"
#include "auction.h"
#include "clans.h"
#include "deity.h"
#ifdef I3
#include "i3.h"
#endif
#ifdef IMC
#include "imc.h"
#endif
#include "mud_prog.h"
#include "new_auth.h"
#include "polymorph.h"

/*
 * Global Variables
 */
CHAR_DATA *gch_prev;
OBJ_DATA *gobj_prev;
CHAR_DATA *timechar;
extern int top_exit;
extern int cur_qobjs;
extern int cur_qchars;
extern char lastplayercmd[MIL * 2];
static bool update_month_trigger;

extern bool bootlock;
extern struct act_prog_data *mob_act_list;
extern time_t board_expire_time_t;

void save_timedata( void );
void calc_season( void );  /* Samson - See calendar.c */
void room_act_update( void );
void obj_act_update( void );
void mpsleep_update( void );
void check_pfiles( time_t reset );
void check_boards( time_t board_exp );
void sound_to_char( const char *fname, int volume, CHAR_DATA * ch, bool toroom );
void prune_dns( void );
bool is_fearing( CHAR_DATA * ch, CHAR_DATA * victim );
void raw_kill( CHAR_DATA * ch, CHAR_DATA * victim );
bool is_hating( CHAR_DATA * ch, CHAR_DATA * victim );
void check_attacker( CHAR_DATA * ch, CHAR_DATA * victim );
int get_terrain( short map, short x, short y );
bool map_wander( CHAR_DATA * ch, short map, short x, short y, short sector );
void clean_auctions( void );
void set_supermob( OBJ_DATA * obj );
void write_corpses( CHAR_DATA * ch, char *name, OBJ_DATA * objrem );
bool check_social( CHAR_DATA * ch, char *command, char *argument );
int get_auth_state( CHAR_DATA * ch );
void auth_update( void );
void skyship_update( void );
void environment_update( void );
void run_events( time_t newtime );
void ClassSpecificStuff( CHAR_DATA * ch );
bool will_fall( CHAR_DATA * ch, int fall );
void make_corpse( CHAR_DATA * ch, CHAR_DATA * killer );
void mprog_wordlist_check( char *arg, CHAR_DATA * mob, CHAR_DATA * actor, OBJ_DATA * object, void *vo, int type );
void mprog_random_trigger( CHAR_DATA * mob );
void mprog_script_trigger( CHAR_DATA * mob );
void mprog_hour_trigger( CHAR_DATA * mob );
void mprog_time_trigger( CHAR_DATA * mob );
void mprog_month_trigger( CHAR_DATA * mob );
void hunt_victim( CHAR_DATA * ch );
void clean_char_queue( void );
void clean_obj_queue( void );
void show_stats( CHAR_DATA * ch, DESCRIPTOR_DATA * d );
ch_ret pullcheck( CHAR_DATA * ch, int pulse );
void area_update( void );
void teleport( CHAR_DATA * ch, int room, int flags );

char *corpse_descs[] = {
   "A skeleton of %s lies here in a pile.",
   "The corpse of %s is in the last stages of decay.",
   "The corpse of %s is crawling with vermin.",
   "The corpse of %s fills the air with a foul stench.",
   "The corpse of %s is buzzing with flies.",
   "The corpse of %s lies here."
};

/*
 * Advancement stuff.
 */
void advance_level( CHAR_DATA * ch )
{
   char buf[MSL];
   int add_hp, add_mana, add_prac, manamod = 0, manahighdie, manaroll;

   snprintf( buf, MSL, "the %s", title_table[ch->Class][ch->level][ch->sex == SEX_FEMALE ? 1 : 0] );
   set_title( ch, buf );

   /*
    * Updated mana gaining to give pure mage and pure cleric more per level 
    */
   /*
    * Any changes here should be reflected in save.c where hp/mana/movement is loaded 
    */
   if( CAN_CAST( ch ) )
   {
      switch ( ch->Class )
      {
         case CLASS_MAGE:
         case CLASS_CLERIC:
            manamod = 20;
            break;
         case CLASS_DRUID:
         case CLASS_NECROMANCER:
            manamod = 17;
            break;
         default:
            manamod = 13;
            break;
      }
   }
   else  /* For non-casting classes, cause alot of racials still require mana */
      manamod = 9;

   /*
    * Samson 10-10-98 
    */
   manahighdie = ( get_curr_int( ch ) + get_curr_wis( ch ) ) / 6;
   if( manahighdie < 2 )
      manahighdie = 2;

   add_hp =
      con_app[get_curr_con( ch )].hitp + number_range( class_table[ch->Class]->hp_min, class_table[ch->Class]->hp_max );

   manaroll = dice( 1, manahighdie ) + 1; /* Samson 10-10-98 */

   add_mana = ( manaroll * manamod ) / 10;

   add_prac = 4 + wis_app[get_curr_wis( ch )].practice;

   add_hp = UMAX( 1, add_hp );

   add_mana = UMAX( 1, add_mana );

   ch->max_hit += add_hp;
   ch->max_mana += add_mana;
   ch->pcdata->practice += add_prac;

   REMOVE_PLR_FLAG( ch, PLR_BOUGHT_PET );

   STRFREE( ch->pcdata->rank );
   ch->pcdata->rank = STRALLOC( class_table[ch->Class]->who_name );

   if( ch->level == LEVEL_AVATAR )
   {
      echo_all_printf( AT_IMMORT, ECHOTAR_ALL, "%s has just achieved Avatarhood!", ch->name );
      STRFREE( ch->pcdata->rank );
      ch->pcdata->rank = STRALLOC( "Avatar" );
      interpret( ch, "help M_ADVHERO_" );
   }
   if( ch->level < LEVEL_IMMORTAL )
      ch_printf( ch, "&WYour gain is: %d hp, %d mana, %d prac.\n\r", add_hp, add_mana, add_prac );

   ClassSpecificStuff( ch );  /* Brought over from DOTD code - Samson 4-6-99 */

   save_char_obj( ch );
   return;
}

CMDF do_levelup( CHAR_DATA * ch, char *argument )
{
   if( ch->exp < exp_level( ch->level + 1 ) )
   {
      send_to_char( "&RYou don't have enough experience to level yet, go forth and adventure!\n\r", ch );
      return;
   }

   ch->level++;
   advance_level( ch );
   log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "%s has advanced to level %d!", ch->name, ch->level );
   cmdf( ch, "gtell I just reached level %d!", ch->level );
#ifdef I3
   if( this_i3mud && ch->level == this_i3mud->minlevel )
   {
      ch->pcdata->i3chardata->i3perm = I3PERM_MORT;
      send_to_char( "You have now reached the minimum allowed level to make use of the Intermud-3 network.\n\r", ch );
      send_to_char( "If you are interested in checking it out, please see 'help intermud3' and its associated topics.\n\r",
                    ch );
      send_to_char
         ( "If you are not interested, then continue to enjoy the game. You will not be reminded of this again.\n\r", ch );
   }
#endif
#ifdef IMC
   if( this_imcmud && ch->level == this_imcmud->minlevel )
   {
      ch->pcdata->imcchardata->imcperm = IMCPERM_MORT;
      send_to_char( "You have now reached the minimum allowed level to make use of the IMC2 network.\n\r", ch );
      send_to_char( "If you are interested in checking it out, please see 'help imc2' and its associated topics.\n\r", ch );
      send_to_char
         ( "If you are not interested, then continue to enjoy the game. You will not be reminded of this again.\n\r", ch );
   }
#endif
   if( ch->level == 4 )
      send_to_char
         ( "&YYou can now be affected by hunger and thirst.\n\rIt is adviseable for you to purchase food and water soon.\n\r",
           ch );

   if( number_range( 1, 2 ) == 1 )
      sound_to_char( "level.mid", 100, ch, FALSE );
   else
      sound_to_char( "level2.mid", 100, ch, FALSE );
   if( ch->level % 20 == 0 )
   {
      show_stats( ch, ch->desc );
      ch->tempnum = 1;
      ch->desc->connected = CON_RAISE_STAT;
   }
   return;
}

/* Function modified from original form - Samson 3-7-98 */
void gain_exp( CHAR_DATA * ch, double gain )
{
   double modgain;

   if( IS_NPC( ch ) || ch->level >= LEVEL_AVATAR )
      return;

   modgain = gain;

   /*
    * per-race experience multipliers 
    */
   modgain *= ( race_table[ch->race]->exp_multiplier / 100.0 );

   /*
    * xp cap to prevent any one event from giving enuf xp to 
    */
   /*
    * gain more than one level - FB 
    */
   modgain = UMIN( modgain, exp_level( ch->level + 2 ) - exp_level( ch->level + 1 ) );

   ch->exp = ( int )( UMAX( 0, ch->exp + modgain ) );

   if( NOT_AUTHED( ch ) && ch->exp >= exp_level( 10 ) )  /* new auth */
   {
      send_to_char( "You can not ascend to a higher level until your name is authorized.\n\r", ch );
      ch->exp = ( exp_level( 10 ) - 1 );
      return;
   }

   if( ch->level < LEVEL_AVATAR && ch->exp >= exp_level( ch->level + 1 ) )
   {
      send_to_char( "&RYou have acheived enough experience to level! Use the levelup command to do so.&w\n\r", ch );
      if( ch->exp >= exp_level( ch->level + 2 ) )
      {
         ch->exp = exp_level( ch->level + 2 ) - 1;
         send_to_char( "&RYou cannot gain any further experience until you level up.&w\n\r", ch );
      }
   }
   return;
}

/*
 * Regeneration stuff.
 */
/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf( int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6 )
{
   if( age < 15 )
      return ( p0 ); /* < 15   */
   else if( age <= 29 )
      return ( int )( p1 + ( ( ( age - 15 ) * ( p2 - p1 ) ) / 15 ) );   /* 15..29 */
   else if( age <= 44 )
      return ( int )( p2 + ( ( ( age - 30 ) * ( p3 - p2 ) ) / 15 ) );   /* 30..44 */
   else if( age <= 59 )
      return ( int )( p3 + ( ( ( age - 45 ) * ( p4 - p3 ) ) / 15 ) );   /* 45..59 */
   else if( age <= 79 )
      return ( int )( p4 + ( ( ( age - 60 ) * ( p5 - p4 ) ) / 20 ) );   /* 60..79 */
   else
      return ( p6 ); /* >= 80 */
}

/* manapoint gain pr. game hour */
int mana_gain( CHAR_DATA * ch )
{
   int gain;

   if( IS_NPC( ch ) )
      gain = 8;
   else
      gain = graf( get_age( ch ), 3, 9, 12, 16, 20, 16, 2 );

   switch ( GET_POS( ch ) )
   {
      case POS_SLEEPING:
         gain += gain;
         break;
      case POS_RESTING:
         gain += ( gain >> 1 );  /* Divide by 2 */
         break;
      case POS_SITTING:
         gain += ( gain >> 2 );  /* Divide by 4 */
         break;
   }

   gain += ch->mana_regen;

   gain += gain;

   gain += wis_app[get_curr_wis( ch )].practice * 2;

   if( IS_AFFECTED( ch, AFF_POISON ) )
      gain >>= 2;

   if( !IS_NPC( ch ) )
      if( ( GET_COND( ch, COND_FULL ) == 0 ) || ( GET_COND( ch, COND_THIRST ) == 0 ) )
         gain >>= 2;

   if( !IS_NPC( ch ) )
   {
      if( GET_COND( ch, COND_DRUNK ) > 10 )
         gain += ( gain >> 1 );
      else if( GET_COND( ch, COND_DRUNK ) > 0 )
         gain += ( gain >> 2 );
   }

   if( ch->Class != CLASS_MAGE || ch->Class != CLASS_NECROMANCER || ch->Class != CLASS_CLERIC || ch->Class != CLASS_DRUID )
      gain -= 2;

   return ( gain );
}

int hit_gain( CHAR_DATA * ch )
{

   int gain, dam;

   if( IS_NPC( ch ) )
      gain = 8;
   else
   {
      if( ch->position > POS_SITTING && ch->position < POS_STANDING )
         gain = 0;
      else
         gain = graf( get_age( ch ), 17, 20, 23, 26, 23, 18, 12 );
   }

   switch ( GET_POS( ch ) )
   {
      case POS_SLEEPING:
         gain += 15;
         break;
      case POS_RESTING:
         gain += 10;
         break;
      case POS_SITTING:
         gain += 5;
         break;
   }

   gain += con_app[get_curr_con( ch )].hitp / 2;

   if( IS_AFFECTED( ch, AFF_POISON ) )
   {
      gain = 0;
      dam = number_range( 10, 32 );
      if( GET_RACE( ch ) == RACE_HALFLING )
         dam = number_range( 1, 20 );
      damage( ch, ch, dam, gsn_poison );
      return 0;
   }

   gain += ch->hit_regen;

   if( !IS_NPC( ch ) )
   {
      if( ( GET_COND( ch, COND_FULL ) == 0 ) || ( GET_COND( ch, COND_THIRST ) == 0 ) )
         gain >>= 4;
      if( GET_COND( ch, COND_DRUNK ) > 10 )
         gain += ( gain >> 1 );
      else if( GET_COND( ch, COND_DRUNK ) > 0 )
         gain += ( gain >> 2 );
   }

   if( ch->Class != CLASS_WARRIOR || ch->Class != CLASS_PALADIN || ch->Class != CLASS_ANTIPALADIN
       || ch->Class != CLASS_RANGER )
   {
      gain -= 2;
      if( gain < 0 && !ch->fighting )
         damage( ch, ch, gain * -1, skill_lookup( "unknown" ) );
   }

   return ( gain );
}

int move_gain( CHAR_DATA * ch )
/* move gain pr. game hour */
{
   int gain;

   if( IS_NPC( ch ) )
   {
      gain = 22;
      if( IsRideable( ch ) )
         gain += gain / 2;

      /*
       * Neat and fast 
       */
   }
   else
   {
      if( ch->position < POS_BERSERK || ch->position > POS_EVASIVE )
         gain = graf( get_age( ch ), 15, 21, 25, 28, 20, 10, 3 );
      else
         gain = 0;
   }

   switch ( GET_POS( ch ) )
   {
      case POS_SLEEPING:
         gain += ( gain >> 2 );  /* Divide by 4 */
         break;
      case POS_RESTING:
         gain += ( gain >> 3 );  /* Divide by 8 */
         break;
      case POS_SITTING:
         gain += ( gain >> 4 );  /* Divide by 16 */
         break;
   }

   gain += ch->move_regen;

   if( IS_AFFECTED( ch, AFF_POISON ) )
      gain >>= 5;

   if( !IS_NPC( ch ) )
      if( ( GET_COND( ch, COND_FULL ) == 0 ) || ( GET_COND( ch, COND_THIRST ) == 0 ) )
         gain >>= 3;

   if( ch->Class != CLASS_ROGUE || ch->Class != CLASS_BARD || ch->Class != CLASS_MONK )
      gain -= 2;

   return ( gain );
}

void gain_condition( CHAR_DATA * ch, int iCond, int value )
{
   int condition;
   ch_ret retcode = rNONE;

   if( value == 0 || IS_NPC( ch ) || ch->level >= LEVEL_IMMORTAL )
      return;

   /*
    * Ghosts are immune to all these 
    */
   if( IS_PLR_FLAG( ch, PLR_GHOST ) )
      return;

   condition = ch->pcdata->condition[iCond];

   /*
    * Immune to hunger/thirst 
    */
   if( condition == -1 )
      return;

   ch->pcdata->condition[iCond] = URANGE( 0, condition + value, sysdata.maxcondval );

   if( ch->pcdata->condition[iCond] == 0 )
   {
      switch ( iCond )
      {
         case COND_FULL:
            if( ch->level < LEVEL_IMMORTAL )
            {
               send_to_char( "&[hungry]You are STARVING!\n\r", ch );
               act( AT_HUNGRY, "$n is starved half to death!", ch, NULL, NULL, TO_ROOM );
               if( number_bits( 1 ) == 0 )
                  worsen_mental_state( ch, 1 );
            }
            break;

         case COND_THIRST:
            if( ch->level < LEVEL_IMMORTAL )
            {
               send_to_char( "&[thirsty]You are DYING of THIRST!\n\r", ch );
               act( AT_THIRSTY, "$n is dying of thirst!", ch, NULL, NULL, TO_ROOM );
               if( number_bits( 1 ) == 0 )
                  worsen_mental_state( ch, 1 );
            }
            break;

         case COND_DRUNK:
            if( condition != 0 )
               send_to_char( "&[sober]You are sober.\n\r", ch );
            retcode = rNONE;
            break;
         default:
            bug( "Gain_condition: invalid condition type %d", iCond );
            retcode = rNONE;
            break;
      }
   }

   if( retcode != rNONE )
      return;

   if( ch->pcdata->condition[iCond] == 1 )
   {
      switch ( iCond )
      {
         case COND_FULL:
            if( ch->level < LEVEL_IMMORTAL )
            {
               send_to_char( "&[hungry]You are really hungry.\n\r", ch );
               act( AT_HUNGRY, "You can hear $n's stomach growling.", ch, NULL, NULL, TO_ROOM );
               if( number_bits( 1 ) == 0 )
                  worsen_mental_state( ch, 1 );
            }
            break;

         case COND_THIRST:
            if( ch->level < LEVEL_IMMORTAL )
            {
               send_to_char( "&[thirsty]You are really thirsty.\n\r", ch );
               act( AT_THIRSTY, "$n looks a little parched.", ch, NULL, NULL, TO_ROOM );
               if( number_bits( 1 ) == 0 )
                  worsen_mental_state( ch, 1 );
            }
            break;

         case COND_DRUNK:
            if( condition != 0 )
               send_to_char( "&[sober]You are feeling a little less light headed.\n\r", ch );
            break;
      }
   }

   if( ch->pcdata->condition[iCond] == 2 )
   {
      switch ( iCond )
      {
         case COND_FULL:
            if( ch->level < LEVEL_IMMORTAL )
               send_to_char( "&[hungry]You are hungry.\n\r", ch );
            break;

         case COND_THIRST:
            if( ch->level < LEVEL_IMMORTAL )
               send_to_char( "&[thirsty]You are thirsty.\n\r", ch );
            break;
      }
   }

   if( ch->pcdata->condition[iCond] == 3 )
   {
      switch ( iCond )
      {
         case COND_FULL:
            if( ch->level < LEVEL_IMMORTAL )
               send_to_char( "&[hungry]You are a mite peckish.\n\r", ch );
            break;

         case COND_THIRST:
            if( ch->level < LEVEL_IMMORTAL )
               send_to_char( "&[thirsty]You could use a sip of something refreshing.\n\r", ch );
            break;
      }
   }
   return;
}

/*
 * drunk randoms	- Tricops
 * (Made part of mobile_update	-Thoric)
 */
void drunk_randoms( CHAR_DATA * ch )
{
   CHAR_DATA *rvch = NULL;
   CHAR_DATA *vch;
   short drunk;
   short position;

   if( IS_NPC( ch ) || ch->pcdata->condition[COND_DRUNK] <= 0 )
      return;

   if( number_percent(  ) < 30 )
      return;

   drunk = ch->pcdata->condition[COND_DRUNK];
   position = ch->position;
   ch->position = POS_STANDING;

   if( number_percent(  ) < ( 2 * drunk / 20 ) )
      check_social( ch, "burp", "" );
   else if( number_percent(  ) < ( 2 * drunk / 20 ) )
      check_social( ch, "hiccup", "" );
   else if( number_percent(  ) < ( 2 * drunk / 20 ) )
      check_social( ch, "drool", "" );
   else if( number_percent(  ) < ( 2 * drunk / 20 ) )
      check_social( ch, "fart", "" );
   else if( drunk > ( 10 + ( get_curr_con( ch ) / 5 ) ) && number_percent(  ) < ( 2 * drunk / 18 ) )
   {
      for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
         if( number_percent(  ) < 10 )
            rvch = vch;
      if( rvch )
         cmdf( ch, "puke %s", rvch->name );
      else
         interpret( ch, "puke" );
   }
   ch->position = position;
   return;
}

/*
 * Random hallucinations for those suffering from an overly high mentalstate
 * (Hats off to Albert Hoffman's "problem child")	-Thoric
 */
void hallucinations( CHAR_DATA * ch )
{
   if( ch->mental_state >= 30 && number_bits( 5 - ( ch->mental_state >= 50 ) - ( ch->mental_state >= 75 ) ) == 0 )
   {
      char *t;

      switch ( number_range( 1, UMIN( 21, ( ch->mental_state + 5 ) / 5 ) ) )
      {
         default:
         case 1:
            t = "You feel very restless... you can't sit still.\n\r";
            break;
         case 2:
            t = "You're tingling all over.\n\r";
            break;
         case 3:
            t = "Your skin is crawling.\n\r";
            break;
         case 4:
            t = "You suddenly feel that something is terribly wrong.\n\r";
            break;
         case 5:
            t = "Those damn little fairies keep laughing at you!\n\r";
            break;
         case 6:
            t = "You can hear your mother crying...\n\r";
            break;
         case 7:
            t = "Have you been here before, or not?  You're not sure...\n\r";
            break;
         case 8:
            t = "Painful childhood memories flash through your mind.\n\r";
            break;
         case 9:
            t = "You hear someone call your name in the distance...\n\r";
            break;
         case 10:
            t = "Your head is pulsating... you can't think straight.\n\r";
            break;
         case 11:
            t = "The ground... seems to be squirming...\n\r";
            break;
         case 12:
            t = "You're not quite sure what is real anymore.\n\r";
            break;
         case 13:
            t = "It's all a dream... or is it?\n\r";
            break;
         case 14:
            t = "You hear your grandchildren praying for you to watch over them.\n\r";
            break;
         case 15:
            t = "They're coming to get you... coming to take you away...\n\r";
            break;
         case 16:
            t = "You begin to feel all powerful!\n\r";
            break;
         case 17:
            t = "You're light as air... the heavens are yours for the taking.\n\r";
            break;
         case 18:
            t = "Your whole life flashes by... and your future...\n\r";
            break;
         case 19:
            t = "You are everywhere and everything... you know all and are all!\n\r";
            break;
         case 20:
            t = "You feel immortal!\n\r";
            break;
         case 21:
            t = "Ahh... the power of a Supreme Entity... what to do...\n\r";
            break;
      }
      send_to_char( t, ch );
   }
   return;
}

/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Mud cpu time.
 */
void mobile_update( void )
{
   CHAR_DATA *ch;
   EXIT_DATA *pexit;
   int door;
   ch_ret retcode;

   retcode = rNONE;

   /*
    * Examine all mobs. 
    */
   for( ch = last_char; ch; ch = gch_prev )
   {
      if( ch == first_char && ch->prev )
      {
         bug( "%s", "mobile_update: first_char->prev != NULL... fixed" );
         ch->prev = NULL;
      }
      gch_prev = ch->prev;

      if( gch_prev && gch_prev->next != ch )
      {
         bug( "FATAL: Mobile_update: %s->prev->next doesn't point to ch.", ch->name );
         bug( "%s", "Short-cutting here" );
         gch_prev = NULL;
         ch->prev = NULL;
         echo_to_all( AT_RED, "WARNING: Automated code message - CRASH LIKELY", ECHOTAR_ALL );
      }

      /*
       * Mobs with no assigned area should not be updated - Samson 6-11-04 
       */
      if( IS_NPC( ch ) && ch->pIndexData->area == NULL )
         continue;

      /*
       * NPCs belonging to prototype areas should be doing nothing 
       */
      if( IS_NPC( ch ) && IS_AREA_FLAG( ch->pIndexData->area, AFLAG_PROTOTYPE ) )
         continue;

      if( !IS_NPC( ch ) )
      {
         if( !IS_PLR_FLAG( ch, PLR_GHOST ) )
         {
            drunk_randoms( ch );
            hallucinations( ch );
            continue;
         }
      }

      /*
       * Removed || IS_AFFECTED(ch, AFF_CHARM) from the line below, so that pets with progs would be nice.
       * * Tarl 5 May 2002
       */
      if( !ch->in_room || IS_AFFECTED( ch, AFF_PARALYSIS ) )
         continue;

      /*
       * Clean up 'animated corpses' that are not charmed' - Scryn 
       */
      /*
       * Also clean up woodland call mobs and summoned warmounts etc - Samson 5-3-00 
       */
      switch ( ch->pIndexData->vnum )
      {
         case 5:
         case 20:
         case 21:
         case 22:
         case 23:
         case 24:
         case 25:
         case 26:
         case 27:
            if( !IS_AFFECTED( ch, AFF_CHARM ) )
            {
               if( ch->in_room->first_person )
                  act( AT_MAGIC, "$n returns to the dust from whence $e came.", ch, NULL, NULL, TO_ROOM );
               if( IS_NPC( ch ) )   /* Guard against purging switched? */
                  extract_char( ch, TRUE );
               continue;
            }

         case 11001:
         case 11002:
         case 11003:
         case 11004:
         case 11005:
         case 11006:
            if( !IS_AFFECTED( ch, AFF_CHARM ) )
            {
               if( ch->in_room->first_person )
                  act( AT_MAGIC, "$n dashes back into the brush.", ch, NULL, NULL, TO_ROOM );
               if( IS_NPC( ch ) )
                  extract_char( ch, TRUE );
               continue;
            }

         case 65:
         case 11007:
         case 11008:
         case 11009:
            if( !IS_AFFECTED( ch, AFF_CHARM ) )
            {
               if( ch->master && ch->master->mount == ch )
               {
                  ch->master->position = POS_SITTING;
                  ch_printf( ch->master, "%s bucks you off, and then gallops away into the wilds!\n\r", ch->short_descr );
               }
               else
               {
                  if( ch->in_room->first_person )
                     act( AT_MAGIC, "$n suddenly bolts and gallops away.", ch, NULL, NULL, TO_ROOM );
               }
               if( IS_NPC( ch ) )
                  extract_char( ch, TRUE );
               continue;
            }
         case 11029:
         case 30000:
         case 30021:
         case 30030:
         case 30041:
         case 30046:
         case 30047:
         case 30048:
            if( !IS_AFFECTED( ch, AFF_CHARM ) )
            {
               if( ch->in_room->first_person )
                  act( AT_MAGIC, "The magic binding $n to this plane dissipates and $e vanishes.", ch, NULL, NULL, TO_ROOM );
               if( IS_NPC( ch ) )
                  extract_char( ch, TRUE );
               continue;
            }
      }

      if( ch->timer > 0 )
      {
         --ch->timer;

         if( ch->timer == 0 )
         {
            make_corpse( ch, ch );
            extract_char( ch, TRUE );
            continue;
         }
      }

      if( IS_ACT_FLAG( ch, ACT_PET ) && !IS_AFFECTED( ch, AFF_CHARM ) && ch->master )
         unbind_follower( ch, ch->master );

      if( !IS_ACT_FLAG( ch, ACT_SENTINEL ) && !ch->fighting && ch->hunting )
      {
         WAIT_STATE( ch, 2 * sysdata.pulseviolence );
         hunt_victim( ch );
         continue;
      }

      /*
       * Examine call for special procedure 
       */
      if( ch->spec_fun )
      {
         if( ( *ch->spec_fun ) ( ch ) )
            continue;
         if( char_died( ch ) )
            continue;
      }

      /*
       * Check for mudprogram script on mob 
       */
      if( HAS_PROG( ch->pIndexData, SCRIPT_PROG ) )
      {
         mprog_script_trigger( ch );
         continue;
      }

      /*
       * That's all for sleeping / busy monster 
       */
      if( ch->position != POS_STANDING )
         continue;

      if( IS_ACT_FLAG( ch, ACT_MOUNTED ) )
      {
         if( IS_ACT_FLAG( ch, ACT_AGGRESSIVE ) || IS_ACT_FLAG( ch, ACT_META_AGGR ) )
            interpret( ch, "emote makes threatening gestures." );
         continue;
      }

      if( !ch->in_room->area )
      {
         bug( "Room %d for mob %s is not associated with an area?", ch->in_room->vnum, ch->name );
         if( ch->was_in_room )
            bug( "Was in room %d", ch->was_in_room->vnum );
         extract_char( ch, TRUE );
         continue;
      }

      /*
       * MOBprogram random trigger 
       */
      if( ch->in_room->area->nplayer > 0 )
      {
         mprog_random_trigger( ch );
         if( char_died( ch ) )
            continue;
         if( ch->position < POS_STANDING )
            continue;
      }

      /*
       * MOBprogram hour trigger: do something for an hour 
       */
      mprog_hour_trigger( ch );

      if( char_died( ch ) )
         continue;

      rprog_hour_trigger( ch );
      if( char_died( ch ) )
         continue;

      if( ch->position < POS_STANDING )
         continue;

      /*
       * Scavenge 
       */
      if( IS_ACT_FLAG( ch, ACT_SCAVENGER ) && ch->in_room->first_content && number_bits( 2 ) == 0 )
      {
         OBJ_DATA *obj;
         OBJ_DATA *obj_best;
         int max;

         max = 1;
         obj_best = NULL;
         for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
         {
            if ( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) && !IS_ACT_FLAG( ch, ACT_PROTOTYPE ) )
               continue;
            if( CAN_WEAR( obj, ITEM_TAKE ) && obj->cost > max && !IS_OBJ_FLAG( obj, ITEM_BURIED ) )
            {
               obj_best = obj;
               max = obj->cost;
            }
         }

         if( obj_best )
         {
            obj_from_room( obj_best );
            obj_to_char( obj_best, ch );
            act( AT_ACTION, "$n gets $p.", ch, obj_best, NULL, TO_ROOM );
         }
      }

      /*
       * Map wanderers - Samson 7-29-00 
       */
      if( IS_ACT_FLAG( ch, ACT_ONMAP ) )
      {
         short sector = get_terrain( ch->map, ch->x, ch->y );
         short map = ch->map;
         short x = ch->x;
         short y = ch->y;
         short dir = number_bits( 5 );

         if( dir < DIR_SOMEWHERE && dir != DIR_UP && dir != DIR_DOWN )
         {
            switch ( dir )
            {
               case DIR_NORTH:
                  if( map_wander( ch, map, x, y - 1, sector ) )
                     move_char( ch, NULL, 0, DIR_NORTH, FALSE );
                  break;
               case DIR_NORTHEAST:
                  if( map_wander( ch, map, x + 1, y - 1, sector ) )
                     move_char( ch, NULL, 0, DIR_NORTHEAST, FALSE );
                  break;
               case DIR_EAST:
                  if( map_wander( ch, map, x + 1, y, sector ) )
                     move_char( ch, NULL, 0, DIR_EAST, FALSE );
                  break;
               case DIR_SOUTHEAST:
                  if( map_wander( ch, map, x + 1, y + 1, sector ) )
                     move_char( ch, NULL, 0, DIR_SOUTHEAST, FALSE );
                  break;
               case DIR_SOUTH:
                  if( map_wander( ch, map, x, y + 1, sector ) )
                     move_char( ch, NULL, 0, DIR_SOUTH, FALSE );
                  break;
               case DIR_SOUTHWEST:
                  if( map_wander( ch, map, x - 1, y + 1, sector ) )
                     move_char( ch, NULL, 0, DIR_SOUTHWEST, FALSE );
                  break;
               case DIR_WEST:
                  if( map_wander( ch, map, x - 1, y, sector ) )
                     move_char( ch, NULL, 0, DIR_WEST, FALSE );
                  break;
               case DIR_NORTHWEST:
                  if( map_wander( ch, map, x - 1, y - 1, sector ) )
                     move_char( ch, NULL, 0, DIR_NORTHWEST, FALSE );
                  break;
            }
         }
         if( char_died( ch ) )
            continue;
      }

      /*
       * Wander 
       */
      /*
       * Update hunt_victim also if any changes are made here 
       */
      if( !IS_ACT_FLAG( ch, ACT_SENTINEL )
          && !IS_ACT_FLAG( ch, ACT_PROTOTYPE )
          && ( door = number_bits( 5 ) ) <= 9 && ( pexit = get_exit( ch->in_room, door ) ) != NULL && pexit->to_room
          /*
           * && !IS_EXIT_FLAG( pexit, EX_CLOSED ) - Test to see if mobs will open doors like this. 
           */
          && !IS_EXIT_FLAG( pexit, EX_WINDOW )
          && !IS_EXIT_FLAG( pexit, EX_NOMOB )
          /*
           * Keep em from wandering through my walls, Marcus 
           */
          && !IS_EXIT_FLAG( pexit, EX_FORTIFIED )
          && !IS_EXIT_FLAG( pexit, EX_HEAVY )
          && !IS_EXIT_FLAG( pexit, EX_MEDIUM )
          && !IS_EXIT_FLAG( pexit, EX_LIGHT )
          && !IS_EXIT_FLAG( pexit, EX_CRUMBLING )
          && !IS_ROOM_FLAG( pexit->to_room, ROOM_NO_MOB )
          && !IS_ROOM_FLAG( pexit->to_room, ROOM_DEATH )
          && ( !IS_ACT_FLAG( ch, ACT_STAY_AREA ) || pexit->to_room->area == ch->in_room->area ) )
      {
         if( pexit->to_room->sector_type == SECT_WATER_NOSWIM && !IS_AFFECTED( ch, AFF_AQUA_BREATH ) )
            continue;

         if( pexit->to_room->sector_type == SECT_RIVER && !IS_AFFECTED( ch, AFF_AQUA_BREATH ) )
            continue;

         if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && !IS_ROOM_FLAG( pexit->to_room, ROOM_NO_MOB ) )
            cmdf( ch, "open %s", pexit->keyword );
         retcode = move_char( ch, pexit, 0, pexit->vdir, FALSE );

         /*
          * If ch changes position due to it's or some other mob's movement via MOBProgs, continue - Kahn 
          */
         if( char_died( ch ) )
            continue;

         if( retcode != rNONE || IS_ACT_FLAG( ch, ACT_SENTINEL ) || ch->position < POS_STANDING )
            continue;
      }

      /*
       * Flee 
       */
      if( ch->hit < ch->max_hit / 2
          && ( door = number_bits( 4 ) ) <= 9
          && ( pexit = get_exit( ch->in_room, door ) ) != NULL
          && pexit->to_room && !IS_EXIT_FLAG( pexit, EX_CLOSED ) && !IS_EXIT_FLAG( pexit, EX_NOMOB )
          && !IS_EXIT_FLAG( pexit, EX_WINDOW )
          /*
           * Keep em from wandering through my walls, Marcus 
           */
          && !IS_EXIT_FLAG( pexit, EX_FORTIFIED )
          && !IS_EXIT_FLAG( pexit, EX_HEAVY )
          && !IS_EXIT_FLAG( pexit, EX_MEDIUM )
          && !IS_EXIT_FLAG( pexit, EX_LIGHT )
          && !IS_EXIT_FLAG( pexit, EX_CRUMBLING ) && !IS_ROOM_FLAG( pexit->to_room, ROOM_NO_MOB )
          && !IS_ROOM_FLAG( pexit->to_room, ROOM_DEATH ) )
      {
         CHAR_DATA *rch;
         bool found;

         if( pexit->to_room->sector_type == SECT_WATER_NOSWIM && !IS_AFFECTED( ch, AFF_AQUA_BREATH ) )
            continue;

         if( pexit->to_room->sector_type == SECT_RIVER && !IS_AFFECTED( ch, AFF_AQUA_BREATH ) )
            continue;

         found = FALSE;
         for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
         {
            if( is_fearing( ch, rch ) )
            {
               switch ( number_bits( 2 ) )
               {
                  case 0:
                     cmdf( ch, "yell Get away from me, %s!", rch->name );
                     break;
                  case 1:
                     cmdf( ch, "yell Leave me be, %s!", rch->name );
                     break;
                  case 2:
                     cmdf( ch, "yell %s is trying to kill me!  Help!", rch->name );
                     break;
                  case 3:
                     cmdf( ch, "yell Someone save me from %s!", rch->name );
                     break;
               }
               found = TRUE;
               break;
            }
         }
         if( found )
            retcode = move_char( ch, pexit, 0, pexit->vdir, FALSE );
      }
   }
   return;
}

/* Anything that should be updating based on time should go here - like hunger/thirst for one */
void char_calendar_update( void )
{
   CHAR_DATA *ch;

   for( ch = last_char; ch; ch = gch_prev )
   {
      if( ch == first_char && ch->prev )
      {
         bug( "%s", "char_calendar_update: first_char->prev != NULL... fixed" );
         ch->prev = NULL;
      }
      gch_prev = ch->prev;
      if( gch_prev && gch_prev->next != ch )
      {
         bug( "%s", "char_calendar_update: ch->prev->next != ch" );
         return;
      }

      if( !IS_NPC( ch ) && !IS_IMMORTAL( ch ) )
      {
         gain_condition( ch, COND_DRUNK, -1 );

         /*
          * Newbies won't starve now - Samson 10-2-98 
          */
         if( ch->in_room && ch->level > 3 )
            gain_condition( ch, COND_FULL, -1 + race_table[ch->race]->hunger_mod );

         /*
          * Newbies won't dehydrate now - Samson 10-2-98 
          */
         if( ch->in_room && ch->level > 3 )
         {
            int sector;

            if( IS_PLR_FLAG( ch, PLR_ONMAP ) )
               sector = get_terrain( ch->map, ch->x, ch->y );
            else
               sector = ch->in_room->sector_type;

            switch ( sector )
            {
               default:
                  gain_condition( ch, COND_THIRST, -1 + race_table[ch->race]->thirst_mod );
                  break;
               case SECT_DESERT:
                  gain_condition( ch, COND_THIRST, -3 + race_table[ch->race]->thirst_mod );
                  break;
               case SECT_UNDERWATER:
               case SECT_OCEANFLOOR:
                  if( number_bits( 1 ) == 0 )
                     gain_condition( ch, COND_THIRST, -1 + race_table[ch->race]->thirst_mod );
                  break;
            }
         }
      }
   }
}

/*
 * Update all chars, including mobs.
 * This function is performance sensitive.
 */
void char_update( void )
{
   CHAR_DATA *ch;
   CHAR_DATA *ch_save;
   short save_count = 0;

   ch_save = NULL;
   for( ch = last_char; ch; ch = gch_prev )
   {
      if( ch == first_char && ch->prev )
      {
         bug( "%s", "char_update: first_char->prev != NULL... fixed" );
         ch->prev = NULL;
      }
      gch_prev = ch->prev;
      if( gch_prev && gch_prev->next != ch )
      {
         bug( "%s", "char_update: ch->prev->next != ch" );
         return;
      }

      /*
       *  Do a room_prog rand check right off the bat
       *   if ch disappears (rprog might wax npc's), continue
       */
      if( !IS_NPC( ch ) )
         rprog_random_trigger( ch );
      if( char_died( ch ) )
         continue;

      if( IS_NPC( ch ) )
         mprog_time_trigger( ch );
      if( char_died( ch ) )
         continue;

      rprog_time_trigger( ch );
      if( char_died( ch ) )
         continue;

      if( IS_NPC( ch ) && update_month_trigger == TRUE )
         mprog_month_trigger( ch );
      if( char_died( ch ) )
         continue;

      if( IS_NPC( ch ) && update_month_trigger == TRUE )
         rprog_month_trigger( ch );
      if( char_died( ch ) )
         continue;

      /*
       * See if player should be auto-saved.
       */
      if( !IS_NPC( ch ) && ( !ch->desc || ch->desc->connected == CON_PLAYING )
          && current_time - ch->pcdata->save_time > ( sysdata.save_frequency * 60 ) )
         ch_save = ch;
      else
         ch_save = NULL;

      if( ch->position >= POS_STUNNED )
      {
         if( ch->hit < ch->max_hit )
         {
            ch->hit += hit_gain( ch );

            if( ch->hit > ch->max_hit )
               ch->hit = ch->max_hit;
         }

         if( ch->mana < ch->max_mana )
         {
            ch->mana += mana_gain( ch );

            if( ch->mana > ch->max_mana )
               ch->mana = ch->max_mana;
         }

         if( ch->move < ch->max_move )
         {
            ch->move += move_gain( ch );

            if( ch->move > ch->max_move )
               ch->move = ch->max_move;
         }
      }

      if( ch->position < POS_STUNNED && ch->hit <= -10 )
         raw_kill( ch, ch );

      if( ch->position == POS_STUNNED )
         update_pos( ch );

      /*
       * Morph timer expires 
       */

      if( ch->morph )
      {
         if( ch->morph->timer > 0 )
         {
            ch->morph->timer--;
            if( ch->morph->timer == 0 )
               do_unmorph_char( ch );
         }
      }

      if( !IS_NPC( ch ) && ch->level < LEVEL_IMMORTAL )
      {
         OBJ_DATA *obj;

         if( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL && obj->item_type == ITEM_LIGHT && obj->value[2] > 0 )
         {
            if( --obj->value[2] == 0 && ch->in_room )
            {
               ch->in_room->light -= obj->count;
               if( ch->in_room->light < 0 )
                  ch->in_room->light = 0;
               act( AT_ACTION, "$p goes out.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "$p goes out.", ch, obj, NULL, TO_CHAR );
               extract_obj( obj );
            }
         }

         if( ++ch->timer >= 12 )
         {
            if( ch->timer == 12 && ch->in_room )
            {
               if( ch->fighting )
                  stop_fighting( ch, TRUE );
               act( AT_ACTION, "$n enters a state of suspended animation.", ch, NULL, NULL, TO_ROOM );
               send_to_char( "You have entered a state of suspended animation.\n\r", ch );
               SET_PLR_FLAG( ch, PLR_IDLING );  /* Samson 5-8-99 */
               if( IS_SAVE_FLAG( SV_IDLE ) )
                  save_char_obj( ch );
            }
         }

         if( ch->timer > 24 )
            interpret( ch, "quit auto" );
         else if( ch == ch_save && IS_SAVE_FLAG( SV_AUTO ) && ++save_count < 10 )
            /*
             * save max of 15 per tick 
             */
         {
            save_char_obj( ch );
            send_to_char( "You have been AutoSaved.\n\r", ch );
         }

         if( ch->pcdata->condition[COND_DRUNK] > 8 )
            worsen_mental_state( ch, ch->pcdata->condition[COND_DRUNK] / 8 );
         if( ch->pcdata->condition[COND_FULL] > 1 )
         {
            switch ( ch->position )
            {
               case POS_SLEEPING:
                  better_mental_state( ch, 4 );
                  break;
               case POS_RESTING:
                  better_mental_state( ch, 3 );
                  break;
               case POS_SITTING:
               case POS_MOUNTED:
                  better_mental_state( ch, 2 );
                  break;
               case POS_STANDING:
                  better_mental_state( ch, 1 );
                  break;
               case POS_FIGHTING:
               case POS_EVASIVE:
               case POS_DEFENSIVE:
               case POS_AGGRESSIVE:
               case POS_BERSERK:
                  if( number_bits( 2 ) == 0 )
                     better_mental_state( ch, 1 );
                  break;
            }
         }
         if( ch->pcdata->condition[COND_THIRST] > 1 )
         {
            switch ( ch->position )
            {
               case POS_SLEEPING:
                  better_mental_state( ch, 5 );
                  break;
               case POS_RESTING:
                  better_mental_state( ch, 3 );
                  break;
               case POS_SITTING:
               case POS_MOUNTED:
                  better_mental_state( ch, 2 );
                  break;
               case POS_STANDING:
                  better_mental_state( ch, 1 );
                  break;
               case POS_FIGHTING:
               case POS_EVASIVE:
               case POS_DEFENSIVE:
               case POS_AGGRESSIVE:
               case POS_BERSERK:
                  if( number_bits( 2 ) == 0 )
                     better_mental_state( ch, 1 );
                  break;
            }
         }
      }
      if( !IS_NPC( ch ) && !IS_IMMORTAL( ch ) && ch->pcdata->release_date > 0 && ch->pcdata->release_date <= current_time )
      {
         ROOM_INDEX_DATA *location;
         if( ch->pcdata->clan )
            location = get_room_index( ch->pcdata->clan->recall );
         else if( ch->pcdata->deity )
            location = get_room_index( ch->pcdata->deity->recallroom );
         else
            location = get_room_index( ROOM_VNUM_TEMPLE );
         if( !location )
            location = get_room_index( ROOM_VNUM_LIMBO );
         if( !location )
            location = ch->in_room;
         MOBtrigger = FALSE;
         char_from_room( ch );
         if( !char_to_room( ch, location ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         send_to_char( "The gods have released you from hell as your sentance is up!\n\r", ch );
         interpret( ch, "look" );
         STRFREE( ch->pcdata->helled_by );
         ch->pcdata->helled_by = NULL;
         ch->pcdata->release_date = 0;
         save_char_obj( ch );
      }

      if( !IS_NPC( ch ) )
         calculate_age( ch );

      if( !char_died( ch ) )
      {
         OBJ_DATA *arrow = NULL;
         int dam = 0;

         if( ( arrow = get_eq_char( ch, WEAR_LODGE_RIB ) ) != NULL )
         {
            dam = number_range( ( 2 * arrow->value[1] ), ( 2 * arrow->value[2] ) );
            act( AT_CARNAGE, "$n suffers damage from $p stuck in $s rib.", ch, arrow, NULL, TO_ROOM );
            act( AT_CARNAGE, "You suffer damage from $p stuck in your rib.", ch, arrow, NULL, TO_CHAR );
            damage( ch, ch, dam, TYPE_UNDEFINED );
         }
         if( char_died( ch ) )
            continue;

         if( ( arrow = get_eq_char( ch, WEAR_LODGE_LEG ) ) != NULL )
         {
            dam = number_range( arrow->value[1], arrow->value[2] );
            act( AT_CARNAGE, "$n suffers damage from $p stuck in $s leg.", ch, arrow, NULL, TO_ROOM );
            act( AT_CARNAGE, "You suffer damage from $p stuck in your leg.", ch, arrow, NULL, TO_CHAR );
            damage( ch, ch, dam, TYPE_UNDEFINED );
         }
         if( char_died( ch ) )
            continue;

         if( ( arrow = get_eq_char( ch, WEAR_LODGE_ARM ) ) != NULL )
         {
            dam = number_range( arrow->value[1], arrow->value[2] );
            act( AT_CARNAGE, "$n suffers damage from $p stuck in $s arm.", ch, arrow, NULL, TO_ROOM );
            act( AT_CARNAGE, "You suffer damage from $p stuck in your arm.", ch, arrow, NULL, TO_CHAR );
            damage( ch, ch, dam, TYPE_UNDEFINED );
         }
         if( char_died( ch ) )
            continue;
      }

      if( !char_died( ch ) )
      {
         /*
          * Careful with the damages here,
          *   MUST NOT refer to ch after damage taken, without checking
          *   return code and/or char_died as it may be lethal damage.
          */
         if( IS_AFFECTED( ch, AFF_POISON ) )
         {
            act( AT_POISON, "$n shivers and suffers.", ch, NULL, NULL, TO_ROOM );
            act( AT_POISON, "You shiver and suffer.", ch, NULL, NULL, TO_CHAR );
            ch->mental_state = URANGE( 20, ch->mental_state + ( IS_NPC( ch ) ? 2 : 4 ), 100 );
            damage( ch, ch, 30, gsn_poison );
         }
         else if( ch->position == POS_INCAP )
            damage( ch, ch, 2, TYPE_UNDEFINED );
         else if( ch->position == POS_MORTAL )
            damage( ch, ch, 4, TYPE_UNDEFINED );
         if( char_died( ch ) )
            continue;

         /*
          * Recurring spell affect
          */
         if( IS_AFFECTED( ch, AFF_RECURRINGSPELL ) )
         {
            AFFECT_DATA *paf, *paf_next;
            SKILLTYPE *skill;
            bool found = FALSE, died = FALSE;

            for( paf = ch->first_affect; paf; paf = paf_next )
            {
               paf_next = paf->next;
               if( paf->location == APPLY_RECURRINGSPELL )
               {
                  found = TRUE;
                  if( IS_VALID_SN( paf->modifier ) && ( skill = skill_table[paf->modifier] ) != NULL
                      && skill->type == SKILL_SPELL )
                  {
                     if( ( *skill->spell_fun ) ( paf->modifier, ch->level, ch, ch ) == rCHAR_DIED || char_died( ch ) )
                     {
                        died = TRUE;
                        break;
                     }
                  }
               }
            }
            if( died )
               continue;
            if( !found )
               REMOVE_AFFECTED( ch, AFF_RECURRINGSPELL );
         }

         if( ch->mental_state >= 30 )
            switch ( ( ch->mental_state + 5 ) / 10 )
            {
               case 3:
                  send_to_char( "You feel feverish.\n\r", ch );
                  act( AT_ACTION, "$n looks kind of out of it.", ch, NULL, NULL, TO_ROOM );
                  break;
               case 4:
                  send_to_char( "You do not feel well at all.\n\r", ch );
                  act( AT_ACTION, "$n doesn't look too good.", ch, NULL, NULL, TO_ROOM );
                  break;
               case 5:
                  send_to_char( "You need help!\n\r", ch );
                  act( AT_ACTION, "$n looks like $e could use your help.", ch, NULL, NULL, TO_ROOM );
                  break;
               case 6:
                  send_to_char( "Seekest thou a cleric.\n\r", ch );
                  act( AT_ACTION, "Someone should fetch a healer for $n.", ch, NULL, NULL, TO_ROOM );
                  break;
               case 7:
                  send_to_char( "You feel reality slipping away...\n\r", ch );
                  act( AT_ACTION, "$n doesn't appear to be aware of what's going on.", ch, NULL, NULL, TO_ROOM );
                  break;
               case 8:
                  send_to_char( "You begin to understand... everything.\n\r", ch );
                  act( AT_ACTION, "$n starts ranting like a madman!", ch, NULL, NULL, TO_ROOM );
                  break;
               case 9:
                  send_to_char( "You are ONE with the universe.\n\r", ch );
                  act( AT_ACTION, "$n is ranting on about 'the answer', 'ONE' and other mumbo-jumbo...", ch, NULL, NULL,
                       TO_ROOM );
                  break;
               case 10:
                  send_to_char( "You feel the end is near.\n\r", ch );
                  act( AT_ACTION, "$n is muttering and ranting in tongues...", ch, NULL, NULL, TO_ROOM );
                  break;
            }
         if( ch->mental_state <= -30 )
            switch ( ( abs( ch->mental_state ) + 5 ) / 10 )
            {
               case 10:
                  if( ch->position > POS_SLEEPING )
                  {
                     if( ( ch->position == POS_STANDING || ch->position < POS_BERSERK )
                         && number_percent(  ) + 10 < abs( ch->mental_state ) )
                        interpret( ch, "sleep" );
                     else
                        send_to_char( "You're barely conscious.\n\r", ch );
                  }
                  break;
               case 9:
                  if( ch->position > POS_SLEEPING )
                  {
                     if( ( ch->position == POS_STANDING || ch->position < POS_BERSERK )
                         && ( number_percent(  ) + 20 ) < abs( ch->mental_state ) )
                        interpret( ch, "sleep" );
                     else
                        send_to_char( "You can barely keep your eyes open.\n\r", ch );
                  }
                  break;
               case 8:
                  if( ch->position > POS_SLEEPING )
                  {
                     if( ch->position < POS_SITTING && ( number_percent(  ) + 30 ) < abs( ch->mental_state ) )
                        interpret( ch, "sleep" );
                     else
                        send_to_char( "You're extremely drowsy.\n\r", ch );
                  }
                  break;
               case 7:
                  if( ch->position > POS_RESTING )
                     send_to_char( "You feel very unmotivated.\n\r", ch );
                  break;
               case 6:
                  if( ch->position > POS_RESTING )
                     send_to_char( "You feel sedated.\n\r", ch );
                  break;
               case 5:
                  if( ch->position > POS_RESTING )
                     send_to_char( "You feel sleepy.\n\r", ch );
                  break;
               case 4:
                  if( ch->position > POS_RESTING )
                     send_to_char( "You feel tired.\n\r", ch );
                  break;
               case 3:
                  if( ch->position > POS_RESTING )
                     send_to_char( "You could use a rest.\n\r", ch );
                  break;
            }
         if( ch->timer > 24 )
            interpret( ch, "quit auto" );
         else if( ch == ch_save && IS_SAVE_FLAG( SV_AUTO ) && ++save_count < 10 )   /* save max of 10 per tick */
            save_char_obj( ch );
      }
   }

   return;
}

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update( void )
{
   OBJ_DATA *obj;
   short AT_TEMP;

   for( obj = last_object; obj; obj = gobj_prev )
   {
      CHAR_DATA *rch;
      char *message;

      if( obj == first_object && obj->prev )
      {
         bug( "%s", "obj_update: first_object->prev != NULL... fixed" );
         obj->prev = NULL;
      }
      gobj_prev = obj->prev;

      if( gobj_prev && gobj_prev->next != obj )
      {
         bug( "%s", "obj_update: obj->prev->next != obj" );
         return;
      }

      if( obj->carried_by )
      {
         oprog_random_trigger( obj );
         if( update_month_trigger == TRUE )
            oprog_month_trigger( obj );
      }
      else if( obj->in_room && obj->in_room->area->nplayer > 0 )
      {
         oprog_random_trigger( obj );
         if( update_month_trigger == TRUE )
            oprog_month_trigger( obj );
      }

      if( obj_extracted( obj ) )
         continue;

      if( obj->item_type == ITEM_PIPE )
      {
         if( IS_SET( obj->value[3], PIPE_LIT ) )
         {
            if( --obj->value[1] <= 0 )
            {
               obj->value[1] = 0;
               REMOVE_BIT( obj->value[3], PIPE_LIT );
            }
            else if( IS_SET( obj->value[3], PIPE_HOT ) )
               REMOVE_BIT( obj->value[3], PIPE_HOT );
            else
            {
               if( IS_SET( obj->value[3], PIPE_GOINGOUT ) )
               {
                  REMOVE_BIT( obj->value[3], PIPE_LIT );
                  REMOVE_BIT( obj->value[3], PIPE_GOINGOUT );
               }
               else
                  SET_BIT( obj->value[3], PIPE_GOINGOUT );
            }
            if( !IS_SET( obj->value[3], PIPE_LIT ) )
               SET_BIT( obj->value[3], PIPE_FULLOFASH );
         }
         else
            REMOVE_BIT( obj->value[3], PIPE_HOT );
      }

      /*
       * Separated these because the stock method had a bug, and it wasn't working with the skeletons - Samson 
       */
      if( obj->item_type == ITEM_CORPSE_PC && obj->value[5] == 0 )
      {
         char name[MSL];
         char *bufptr;
         int frac = obj->timer / 8;
         bufptr = one_argument( obj->short_descr, name );
         bufptr = one_argument( bufptr, name );
         bufptr = one_argument( bufptr, name );

         separate_obj( obj );

         frac = URANGE( 1, frac, 5 );

         obj->value[3] = frac;   /* Advances decay stage for resurrection spell */
         stralloc_printf( &obj->objdesc, corpse_descs[frac], bufptr );

         /*
          * Why not update the description? 
          */
         write_corpses( NULL, bufptr, NULL );
      }

      /*
       * Make sure skeletons get saved as their timers descrease 
       */
      if( obj->item_type == ITEM_CORPSE_PC && obj->value[5] == 1 )
         write_corpses( NULL, obj->short_descr + 14, NULL );

      if( obj->item_type == ITEM_CORPSE_NPC && obj->timer <= 5 )
      {
         char name[MSL];
         char *bufptr;
         bufptr = one_argument( obj->short_descr, name );
         bufptr = one_argument( bufptr, name );
         bufptr = one_argument( bufptr, name );

         separate_obj( obj );
         stralloc_printf( &obj->objdesc, corpse_descs[obj->timer], bufptr );
      }

      /*
       * don't let inventory decay 
       */
      if( IS_OBJ_FLAG( obj, ITEM_INVENTORY ) )
         continue;

      /*
       * groundrot items only decay on the ground 
       */
      if( IS_OBJ_FLAG( obj, ITEM_GROUNDROT ) && !obj->in_room )
         continue;

      if( ( obj->timer <= 0 || --obj->timer > 0 ) )
         continue;

      /*
       * if we get this far, object's timer has expired. 
       */

      AT_TEMP = AT_PLAIN;
      switch ( obj->item_type )
      {
         default:
            message = "$p mysteriously vanishes.";
            AT_TEMP = AT_PLAIN;
            break;

         case ITEM_CONTAINER:
            message = "$p falls apart, tattered from age.";
            AT_TEMP = AT_OBJECT;
            break;

         case ITEM_PORTAL:
            message = "$p unravels and winks from existence.";
            remove_portal( obj );
            obj->item_type = ITEM_TRASH;  /* so extract_obj  */
            AT_TEMP = AT_MAGIC;  /* doesn't remove_portal */
            break;

         case ITEM_FOUNTAIN:
            message = "$p dries up.";
            AT_TEMP = AT_BLUE;
            break;

         case ITEM_CORPSE_PC:
            if( obj->value[5] == 0 )
               message = "The flesh from $p decays away leaving just a skeleton behind.";
            else
               message = "$p decays into dust and blows away.";
            AT_TEMP = AT_OBJECT;
            break;

         case ITEM_CORPSE_NPC:
            message = "$p decays into dust and blows away.";
            AT_TEMP = AT_OBJECT;
            break;

         case ITEM_COOK:
         case ITEM_FOOD:
            message = "$p is devoured by a swarm of maggots.";
            AT_TEMP = AT_HUNGRY;
            break;

         case ITEM_BLOOD:
            message = "$p slowly seeps into the ground.";
            AT_TEMP = AT_BLOOD;
            break;

         case ITEM_BLOODSTAIN:
            message = "$p dries up into flakes and blows away.";
            AT_TEMP = AT_BLOOD;
            break;

         case ITEM_SCRAPS:
            message = "$p crumble and decay into nothing.";
            AT_TEMP = AT_OBJECT;
            break;

         case ITEM_FIRE:
            message = "$p burns out.";
            AT_TEMP = AT_FIRE;
            break;

         case ITEM_TRASH:
            message = "$p mysteriously vanishes.";
            AT_TEMP = AT_PLAIN;
            if( obj->in_room )
            {
               if( obj->pIndexData->vnum == OBJ_VNUM_FIREPIT )
               {
                  message = "$p scatters away in the breeze.";
                  AT_TEMP = AT_OBJECT;
               }
            }
      }

      if( obj->carried_by )
         act( AT_TEMP, message, obj->carried_by, obj, NULL, TO_CHAR );

      else if( obj->in_room && ( rch = obj->in_room->first_person ) != NULL && !IS_OBJ_FLAG( obj, ITEM_BURIED ) )
      {
         act( AT_TEMP, message, rch, obj, NULL, TO_ROOM );
         act( AT_TEMP, message, rch, obj, NULL, TO_CHAR );
      }

      /*
       * Player skeletons surrender any objects on them to the room when the corpse decays away. - Samson 8-13-98 
       */
      if( obj->item_type == ITEM_CORPSE_PC )
      {
         if( obj->value[5] == 0 )
         {
            char name[MSL], name2[MSL];
            char *bufptr;

            bufptr = one_argument( obj->short_descr, name );
            bufptr = one_argument( bufptr, name );
            bufptr = one_argument( bufptr, name );
            mudstrlcpy( name2, bufptr, MSL );   /* Dunno why, but it's corrupting if I don't do this - Samson */

            obj->timer = 250; /* Corpse is now a skeleton */
            obj->value[3] = 0;
            obj->value[5] = 1;

            stralloc_printf( &obj->name, "corpse skeleton %s", name2 );
            stralloc_printf( &obj->short_descr, "A skeleton of %s", name2 );
            stralloc_printf( &obj->objdesc, corpse_descs[0], name2 );
            write_corpses( NULL, obj->short_descr + 14, NULL );
         }
         else
         {
            if( obj->carried_by )
               empty_obj( obj, NULL, obj->carried_by->in_room );
            else if( obj->in_room )
               empty_obj( obj, NULL, obj->in_room );
         }
      }

      if( obj->item_type == ITEM_FIRE )
      {
         OBJ_DATA *firepit;

         if( !( firepit = create_object( get_obj_index( OBJ_VNUM_FIREPIT ), 0 ) ) )
            log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         else
         {
            firepit->map = obj->map;
            firepit->x = obj->x;
            firepit->y = obj->y;
            set_supermob( obj );
            obj_to_room( firepit, obj->in_room, supermob );
            release_supermob(  );
            firepit->timer = 40;
         }
      }
      if( obj->timer < 1 )
         extract_obj( obj );
   }
   return;
}

/*
 * Function to check important stuff happening to a player
 * This function should take about 5% of mud cpu time
 */
void char_check( void )
{
   CHAR_DATA *ch, *ch_next;
   OBJ_DATA *obj;
   TIMER *timer, *timer_next;
   ch_ret retcode;
   static int cnt = 0;
   static int pulse = 0;

   pulse = ( pulse + 1 ) % 100;

   /*
    * This little counter can be used to handle periodic events 
    */
   cnt = ( cnt + 1 ) % sysdata.secpertick;

   for( ch = first_char; ch; ch = ch_next )
   {
      ch_next = ch->next;
      will_fall( ch, 0 );

      if( char_died( ch ) )
         continue;

      /*
       * This is where ACT_RUNNING mobs used to go, but we eliminated those. NPCs had nothing else to do here. 
       */
      if( IS_NPC( ch ) )
         continue;

      else
      {
         for( timer = ch->first_timer; timer; timer = timer_next )
         {
            timer_next = timer->next;
            if( --timer->count <= 0 )
            {
               if( timer->type == TIMER_ASUPRESSED )
               {
                  if( timer->value == -1 )
                  {
                     timer->count = 1000;
                     continue;
                  }
               }

               if( timer->type == TIMER_DO_FUN )
               {
                  int tempsub;

                  tempsub = ch->substate;
                  ch->substate = timer->value;
                  ( timer->do_fun ) ( ch, "" );
                  if( char_died( ch ) )
                     break;
                  ch->substate = tempsub;
               }
               extract_timer( ch, timer );
            }
         }

         /*
          * check for exits moving players around 
          */
         if( ( retcode = pullcheck( ch, pulse ) ) == rCHAR_DIED || char_died( ch ) )
            continue;

         if( ch->mount && ch->in_room != ch->mount->in_room )
         {
            REMOVE_ACT_FLAG( ch->mount, ACT_MOUNTED );
            ch->mount = NULL;
            ch->position = POS_STANDING;
            send_to_char( "No longer upon your mount, you fall to the ground...\n\rOUCH!\n\r", ch );
         }

         if( ( ch->in_room && ch->in_room->sector_type == SECT_UNDERWATER )
             || ( ch->in_room && ch->in_room->sector_type == SECT_OCEANFLOOR ) )
         {
            /*
             * Immortal timer test message added by Samson, works only in room 1450 
             */
            if( ( ch->level >= LEVEL_IMMORTAL ) && ( ch->in_room->vnum == 1450 ) )
               send_to_char( "You're underwater, or on the oceanfloor.", ch );

            if( !IS_AFFECTED( ch, AFF_AQUA_BREATH ) )
            {
               /*
                * Immortal timer test message added by Samson, works only in room 1450 
                */
               if( ( ch->level >= LEVEL_IMMORTAL ) && ( ch->in_room->vnum == 1450 ) )
                  send_to_char( "No aqua breath, you'd be drowning this fast if mortal.", ch );
               if( ch->level < LEVEL_IMMORTAL )
               {
                  int dam;

                  /*
                   * Changed level of damage at Brittany's request. -- Narn 
                   */
                  dam = number_range( ch->max_hit / 100, ch->max_hit / 50 );
                  dam = UMAX( 1, dam );
                  if( number_bits( 3 ) == 0 )
                     send_to_char( "You cough and choke as you try to breathe water!\n\r", ch );
                  damage( ch, ch, dam, TYPE_UNDEFINED );
               }
            }
         }

         if( char_died( ch ) )
            continue;

         if( ch->in_room && ( ch->in_room->sector_type == SECT_RIVER ) )
         {
            /*
             * Immortal timer test message added by Samson, works only in room 1450 
             */
            if( ( ch->level >= LEVEL_IMMORTAL ) && ( ch->in_room->vnum == 1450 ) )
               send_to_char( "You're in a river, which hopefully will be flowing soon.\n\r", ch );

            if( !IS_AFFECTED( ch, AFF_FLYING ) && !IS_AFFECTED( ch, AFF_AQUA_BREATH ) && !IS_AFFECTED( ch, AFF_FLOATING ) )
            {
               for( obj = ch->first_carrying; obj; obj = obj->next_content )
                  if( obj->item_type == ITEM_BOAT )
                     break;

               if( !obj )
               {
                  /*
                   * Immortal timer test message added by Samson, works only in room 1450 
                   */
                  if( ( ch->level >= LEVEL_IMMORTAL ) && ( ch->in_room->vnum == 1450 ) )
                     send_to_char( "If mortal, now you'd be drowning this fast.\n\r", ch );

                  if( ch->level < LEVEL_IMMORTAL )
                  {
                     int mov;
                     int dam;

                     if( ch->move > 0 )
                     {
                        mov = number_range( ch->max_move / 20, ch->max_move / 5 );
                        mov = UMAX( 1, mov );

                        if( ch->move - mov < 0 )
                           ch->move = 0;
                        else
                           ch->move -= mov;

                        if( number_bits( 3 ) == 0 )
                           send_to_char( "You struggle to remain afloat in the current.\n\r", ch );
                     }
                     else
                     {
                        dam = number_range( ch->max_hit / 20, ch->max_hit / 5 );
                        dam = UMAX( 1, dam );

                        if( number_bits( 3 ) == 0 )
                           send_to_char( "Struggling with exhaustion, you choke on a mouthful of water.\n\r", ch );
                        damage( ch, ch, dam, TYPE_UNDEFINED );
                     }
                  }
               }
            }

         }

         if( ch->in_room && ch->in_room->sector_type == SECT_WATER_SWIM )
         {
            /*
             * Immortal timer test message added by Samson, works only in room 1450 
             */
            if( ( ch->level >= LEVEL_IMMORTAL ) && ( ch->in_room->vnum == 1450 ) )
               send_to_char( "You're in shallow water.\n\r", ch );

            if( !IS_AFFECTED( ch, AFF_FLYING ) && !IS_AFFECTED( ch, AFF_FLOATING ) && !IS_AFFECTED( ch, AFF_AQUA_BREATH )
                && !ch->mount )
            {
               for( obj = ch->first_carrying; obj; obj = obj->next_content )
                  if( obj->item_type == ITEM_BOAT )
                     break;

               if( !obj )
               {
                  /*
                   * Immortal timer test message added by Samson, works only in room 1450 
                   */
                  if( ( ch->level >= LEVEL_IMMORTAL ) && ( ch->in_room->vnum == 1450 ) )
                     send_to_char( "One of these days I suppose swim skill would be nice.\n\r", ch );

                  if( ch->level < LEVEL_IMMORTAL )
                  {
                     int mov;
                     int dam;

                     if( ch->move > 0 )
                     {
                        mov = number_range( ch->max_move / 20, ch->max_move / 5 );
                        mov = UMAX( 1, mov );

                        if( !IS_NPC( ch ) && number_percent(  ) < ch->pcdata->learned[gsn_swim] )
                           ;
                        else
                        {
                           if( ch->move - mov < 0 )
                              ch->move = 0;
                           else
                              ch->move -= mov;

                           if( number_bits( 3 ) == 0 )
                           {
                              send_to_char( "You struggle to remain afloat.\n\r", ch );
                              if( !IS_NPC( ch ) && ch->pcdata->learned[gsn_swim] > 0 )
                                 learn_from_failure( ch, gsn_swim );
                           }
                        }
                     }
                     else
                     {
                        dam = number_range( ch->max_hit / 20, ch->max_hit / 5 );
                        dam = UMAX( 1, dam );

                        if( number_bits( 3 ) == 0 )
                           send_to_char( "Struggling with exhaustion, you choke on a mouthful of water.\n\r", ch );
                        damage( ch, ch, dam, TYPE_UNDEFINED );
                        if( !IS_NPC( ch ) && ch->pcdata->learned[gsn_swim] > 0 )
                           learn_from_failure( ch, gsn_swim );
                     }
                  }
               }
            }
         }

         if( ch->in_room && ch->in_room->sector_type == SECT_WATER_NOSWIM )
         {
            /*
             * Immortal timer test message added by Samson, works only in room 1450 
             */
            if( ( ch->level >= LEVEL_IMMORTAL ) && ( ch->in_room->vnum == 1450 ) )
               send_to_char( "You're in deep water.\n\r", ch );

            if( !IS_AFFECTED( ch, AFF_FLYING ) && !IS_AFFECTED( ch, AFF_FLOATING ) && !IS_AFFECTED( ch, AFF_AQUA_BREATH )
                && !ch->mount )
            {
               for( obj = ch->first_carrying; obj; obj = obj->next_content )
                  if( obj->item_type == ITEM_BOAT )
                     break;

               if( !obj )
               {
                  /*
                   * Immortal timer test message added by Samson, works only in room 1450 
                   */
                  if( ( ch->level >= LEVEL_IMMORTAL ) && ( ch->in_room->vnum == 1450 ) )
                     send_to_char( "You'd be drowning this fast if mortal.\n\r", ch );

                  if( ch->level < LEVEL_IMMORTAL )
                  {
                     int mov;
                     int dam;

                     if( ch->move > 0 )
                     {
                        mov = number_range( ch->max_move / 20, ch->max_move / 5 );
                        mov = UMAX( 1, mov );

                        if( ch->move - mov < 0 )
                           ch->move = 0;
                        else
                           ch->move -= mov;

                        if( number_bits( 3 ) == 0 )
                           send_to_char( "You struggle to remain afloat in the deep water.\n\r", ch );
                     }
                     else
                     {
                        dam = number_range( ch->max_hit / 20, ch->max_hit / 5 );
                        dam = UMAX( 1, dam );

                        if( number_bits( 3 ) == 0 )
                           send_to_char( "Struggling with exhaustion, you choke on a mouthful of water.\n\r", ch );
                        damage( ch, ch, dam, TYPE_UNDEFINED );
                     }
                  }
               }
            }
         }
      }
   }
}

/*
 * Aggress.
 *
 * for each descriptor
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function should take 5% to 10% of ALL mud cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 */
void aggr_update( void )
{
   DESCRIPTOR_DATA *d, *dnext;
   CHAR_DATA *wch;
   CHAR_DATA *ch;
   CHAR_DATA *ch_next;
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;
   CHAR_DATA *victim;
   struct act_prog_data *apdtmp;

   /*
    * check mobprog act queue 
    */
   while( ( apdtmp = mob_act_list ) != NULL )
   {
      wch = ( CHAR_DATA * ) mob_act_list->vo;
      if( !char_died( wch ) && wch->mpactnum > 0 )
      {
         MPROG_ACT_LIST *tmp_act;

         while( ( tmp_act = wch->mpact ) != NULL )
         {
            if( tmp_act->obj && obj_extracted( tmp_act->obj ) )
               tmp_act->obj = NULL;
            if( tmp_act->ch && !char_died( tmp_act->ch ) )
               mprog_wordlist_check( tmp_act->buf, wch, tmp_act->ch, tmp_act->obj, tmp_act->vo, ACT_PROG );
            wch->mpact = tmp_act->next;
            DISPOSE( tmp_act->buf );
            DISPOSE( tmp_act );
         }
         wch->mpactnum = 0;
         wch->mpact = NULL;
      }
      mob_act_list = apdtmp->next;
      DISPOSE( apdtmp );
   }

   /*
    * Just check descriptors here for victims to aggressive mobs
    * We can check for linkdead victims in char_check   -Thoric
    */
   for( d = first_descriptor; d; d = dnext )
   {
      dnext = d->next;
      if( ( d->connected != CON_PLAYING && d->connected != CON_EDITING ) || ( wch = d->character ) == NULL )
         continue;

      if( char_died( wch ) || IS_NPC( wch ) || wch->level >= LEVEL_IMMORTAL || !wch->in_room
          || IS_PLR_FLAG( wch, PLR_IDLING ) )
         /*
          * Protect Idle/Linkdead players - Samson 5-8-99 
          */
         continue;

      for( ch = wch->in_room->first_person; ch; ch = ch_next )
      {
         int count;

         ch_next = ch->next_in_room;

         if( !IS_NPC( ch ) || ch->fighting || IS_AFFECTED( ch, AFF_CHARM ) || !IS_AWAKE( ch )
             || ( IS_ACT_FLAG( ch, ACT_WIMPY ) && IS_AWAKE( wch ) ) || !can_see( ch, wch, FALSE ) )
            continue;

         if( is_hating( ch, wch ) )
         {
            found_prey( ch, wch );
            continue;
         }

         if( ( !IS_ACT_FLAG( ch, ACT_AGGRESSIVE ) && !IS_ACT_FLAG( ch, ACT_META_AGGR ) )
             || IS_ACT_FLAG( ch, ACT_MOUNTED ) || IS_ROOM_FLAG( ch->in_room, ROOM_SAFE ) )
            continue;

         /*
          * Ok we have a 'wch' player character and a 'ch' npc aggressor.
          * Now make the aggressor fight a RANDOM pc victim in the room,
          *   giving each 'vch' an equal chance of selection.
          *
          * Depending on flags set, the mob may attack another mob
          */
         count = 0;
         victim = NULL;
         for( vch = wch->in_room->first_person; vch; vch = vch_next )
         {
            vch_next = vch->next_in_room;

            if( ( !IS_NPC( vch ) || IS_ACT_FLAG( ch, ACT_META_AGGR ) || IS_ACT_FLAG( vch, ACT_ANNOYING ) )
                && vch->level < LEVEL_IMMORTAL && ( !IS_ACT_FLAG( ch, ACT_WIMPY ) || !IS_AWAKE( vch ) )
                && can_see( ch, vch, FALSE ) )
            {
               if( number_range( 0, count ) == 0 )
                  victim = vch;
               count++;
            }
         }

         if( !victim )
         {
            bug( "%s: null victim. Aggro: %s", __FUNCTION__, ch->name );
            log_string( "Breaking aggr_update loop and transferring aggressor to Limbo." );
            char_from_room( ch );
            if( !char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            break;
         }

         /*
          * Skyships are immune to attack 
          */
         if( IS_PLR_FLAG( victim, PLR_BOARDED ) )
            break;

         /*
          * backstabbing mobs (Thoric) 
          */
         if( IS_ATTACK( ch, ATCK_BACKSTAB ) )
         {
            OBJ_DATA *obj;

            if( !ch->mount && ( obj = get_eq_char( ch, WEAR_WIELD ) ) != NULL && ( obj->value[4] == WEP_DAGGER )
                && !victim->fighting && victim->hit >= victim->max_hit )
            {
               check_attacker( ch, victim );
               WAIT_STATE( ch, skill_table[gsn_backstab]->beats );
               if( !IS_AWAKE( victim ) || number_percent(  ) + 5 < ch->level )
               {
                  global_retcode = multi_hit( ch, victim, gsn_backstab );
                  continue;
               }
               else
               {
                  global_retcode = damage( ch, victim, 0, gsn_backstab );
                  continue;
               }
            }
         }
         global_retcode = multi_hit( ch, victim, TYPE_UNDEFINED );
      }
   }
   return;
}

void tele_update( void )
{
   TELEPORT_DATA *tele, *tele_next;

   if( !first_teleport )
      return;

   for( tele = first_teleport; tele; tele = tele_next )
   {
      tele_next = tele->next;
      if( --tele->timer <= 0 )
      {
         if( tele->room->first_person )
         {
            if( IS_ROOM_FLAG( tele->room, ROOM_TELESHOWDESC ) )
               teleport( tele->room->first_person, tele->room->tele_vnum, TELE_SHOWDESC | TELE_TRANSALL );
            else
               teleport( tele->room->first_person, tele->room->tele_vnum, TELE_TRANSALL );
         }
         UNLINK( tele, first_teleport, last_teleport, next, prev );
         DISPOSE( tele );
      }
   }
}

void affect_update( CHAR_DATA * ch )
{
   AFFECT_DATA *paf, *paf_next;
   SKILLTYPE *skill;

   for( paf = ch->first_affect; paf; paf = paf_next )
   {
      paf_next = paf->next;
      if( paf->duration > 0 )
         paf->duration--;
      else if( paf->duration < 0 )
         ;
      else
      {
         if( !paf_next || paf_next->type != paf->type || paf_next->duration > 0 )
         {
            skill = get_skilltype( paf->type );
            if( paf->type > 0 && skill && skill->msg_off )
               ch_printf( ch, "&[wearoff]%s\n\r", skill->msg_off );
         }
         affect_remove( ch, paf );
      }
   }
}

void spell_update( void )
{
   CHAR_DATA *ch;
   CHAR_DATA *lst_ch;

   lst_ch = NULL;
   for( ch = last_char; ch; lst_ch = ch, ch = gch_prev )
   {
      if( ch == first_char && ch->prev )
      {
         bug( "%s", "ERROR: spell_update: first_char->prev != NULL, fixing..." );
         ch->prev = NULL;
      }

      gch_prev = ch->prev;

      if( gch_prev && gch_prev->next != ch )
      {
         bug( "FATAL: spell_update: %s->prev->next doesn't point to ch.", ch->name );
         bug( "%s", "Short-cutting here" );
         ch->prev = NULL;
         gch_prev = NULL;
         echo_to_all( AT_RED, "WARNING: Automated code message - CRASH LIKELY", ECHOTAR_ALL );
      }

      /*
       * See if we got a pointer to someone who recently died...
       * if so, either the pointer is bad... or it's a player who
       * "died", and is back at the healer...
       * Since he/she's in the char_list, it's likely to be the later...
       * and should not already be in another fight already
       */
      if( char_died( ch ) )
         continue;

      /*
       * See if we got a pointer to some bad looking data...
       */
      if( !ch->in_room || !ch->name )
      {
         log_string( "spell_update: bad ch record!  (Shortcutting.)" );
         log_printf( "ch: %ld  ch->in_room: %ld  ch->prev: %ld  ch->next: %ld",
                     ( long )ch, ( long )ch->in_room, ( long )ch->prev, ( long )ch->next );
         log_string( lastplayercmd );
         if( lst_ch )
            log_printf( "lst_ch: %ld  lst_ch->prev: %ld  lst_ch->next: %ld",
                        ( long )lst_ch, ( long )lst_ch->prev, ( long )lst_ch->next );
         else
            log_string( "lst_ch: NULL" );
         gch_prev = NULL;
         continue;
      }

      if( char_died( ch ) )
         continue;

      /*
       * We need spells that have shorter durations than an hour.
       * So a melee round sounds good to me... -Thoric
       */
      affect_update( ch );
   }
   return;
}

/*
 * Function to update weather vectors according to climate
 * settings, random effects, and neighboring areas.
 * Last modified: July 18, 1997
 * - Fireblade
 */
void adjust_vectors( WEATHER_DATA * weather )
{
   NEIGHBOR_DATA *neigh;
   double dT, dP, dW;

   if( !weather )
   {
      bug( "%s", "adjust_vectors: NULL weather data." );
      return;
   }

   dT = 0;
   dP = 0;
   dW = 0;

   /*
    * Add in random effects 
    */
   dT += number_range( -rand_factor, rand_factor );
   dP += number_range( -rand_factor, rand_factor );
   dW += number_range( -rand_factor, rand_factor );

   /*
    * Add in climate effects
    */
   dT += climate_factor * ( ( ( weather->climate_temp - 2 ) * weath_unit ) - ( weather->temp ) ) / weath_unit;
   dP += climate_factor * ( ( ( weather->climate_precip - 2 ) * weath_unit ) - ( weather->precip ) ) / weath_unit;
   dW += climate_factor * ( ( ( weather->climate_wind - 2 ) * weath_unit ) - ( weather->wind ) ) / weath_unit;


   /*
    * Add in effects from neighboring areas 
    */
   for( neigh = weather->first_neighbor; neigh; neigh = neigh->next )
   {
      /*
       * see if we have the area cache'd already 
       */
      if( !neigh->address )
      {
         /*
          * try and find address for area 
          */
         neigh->address = get_area( neigh->name );

         /*
          * if couldn't find area ditch the neigh 
          */
         if( !neigh->address )
         {
            NEIGHBOR_DATA *temp;
            bug( "%s", "adjust_weather: invalid area name" );
            temp = neigh->prev;
            UNLINK( neigh, weather->first_neighbor, weather->last_neighbor, next, prev );
            STRFREE( neigh->name );
            DISPOSE( neigh );
            neigh = temp;
            continue;
         }
      }

      dT += ( neigh->address->weather->temp - weather->temp ) / neigh_factor;
      dP += ( neigh->address->weather->precip - weather->precip ) / neigh_factor;
      dW += ( neigh->address->weather->wind - weather->wind ) / neigh_factor;
   }

   /*
    * now apply the effects to the vectors 
    */
   weather->temp_vector += ( int )dT;
   weather->precip_vector += ( int )dP;
   weather->wind_vector += ( int )dW;

   /*
    * Make sure they are within the right range 
    */
   weather->temp_vector = URANGE( -max_vector, weather->temp_vector, max_vector );
   weather->precip_vector = URANGE( -max_vector, weather->precip_vector, max_vector );
   weather->wind_vector = URANGE( -max_vector, weather->wind_vector, max_vector );

   return;
}

/*
 * get weather echo messages according to area weather...
 * stores echo message in weath_data.... must be called before
 * the vectors are adjusted
 * Last Modified: August 10, 1997
 * Fireblade
 */
void get_weather_echo( WEATHER_DATA * weath )
{
   int n;
   int temp, precip, wind;
   int dT, dP, dW;
   int tindex, pindex, windex;

   /*
    * set echo to be nothing 
    */
   weath->echo = NULL;
   weath->echo_color = AT_GREY;

   /*
    * get the random number 
    */
   n = number_bits( 2 );

   /*
    * variables for convenience 
    */
   temp = weath->temp;
   precip = weath->precip;
   wind = weath->wind;

   dT = weath->temp_vector;
   dP = weath->precip_vector;
   dW = weath->wind_vector;

   tindex = ( temp + 3 * weath_unit - 1 ) / weath_unit;
   pindex = ( precip + 3 * weath_unit - 1 ) / weath_unit;
   windex = ( wind + 3 * weath_unit - 1 ) / weath_unit;

   /*
    * get the echo string... mainly based on precip 
    */
   switch ( pindex )
   {
      case 0:
         if( precip - dP > -2 * weath_unit )
         {
            char *echo_strings[4] = {
               "The clouds disappear.\n\r",
               "The clouds disappear.\n\r",
               "The sky begins to break through the clouds.\n\r",
               "The clouds are slowly evaporating.\n\r"
            };

            weath->echo = echo_strings[n];
            weath->echo_color = AT_WHITE;
         }
         break;

      case 1:
         if( precip - dP <= -2 * weath_unit )
         {
            char *echo_strings[4] = {
               "The sky is getting cloudy.\n\r",
               "The sky is getting cloudy.\n\r",
               "Light clouds cast a haze over the sky.\n\r",
               "Billows of clouds spread through the sky.\n\r"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_GREY;
         }
         break;

      case 2:
         if( precip - dP > 0 )
         {
            if( tindex > 1 )
            {
               char *echo_strings[4] = {
                  "The rain stops.\n\r",
                  "The rain stops.\n\r",
                  "The rainstorm tapers off.\n\r",
                  "The rain's intensity breaks.\n\r"
               };
               weath->echo = echo_strings[n];
               weath->echo_color = AT_CYAN;
            }
            else
            {
               char *echo_strings[4] = {
                  "The snow stops.\n\r",
                  "The snow stops.\n\r",
                  "The snow showers taper off.\n\r",
                  "The snow flakes disappear from the sky.\n\r"
               };
               weath->echo = echo_strings[n];
               weath->echo_color = AT_WHITE;
            }
         }
         break;

      case 3:
         if( precip - dP <= 0 )
         {
            if( tindex > 1 )
            {
               char *echo_strings[4] = {
                  "It starts to rain.\n\r",
                  "It starts to rain.\n\r",
                  "A droplet of rain falls upon you.\n\r",
                  "The rain begins to patter.\n\r"
               };
               weath->echo = echo_strings[n];
               weath->echo_color = AT_CYAN;
            }
            else
            {
               char *echo_strings[4] = {
                  "It starts to snow.\n\r",
                  "It starts to snow.\n\r",
                  "Crystal flakes begin to fall from the sky.\n\r",
                  "Snow flakes drift down from the clouds.\n\r"
               };
               weath->echo = echo_strings[n];
               weath->echo_color = AT_WHITE;
            }
         }
         else if( tindex < 2 && temp - dT > -weath_unit )
         {
            char *echo_strings[4] = {
               "The temperature drops and the rain becomes a light snow.\n\r",
               "The temperature drops and the rain becomes a light snow.\n\r",
               "Flurries form as the rain freezes.\n\r",
               "Large snow flakes begin to fall with the rain.\n\r"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_WHITE;
         }
         else if( tindex > 1 && temp - dT <= -weath_unit )
         {
            char *echo_strings[4] = {
               "The snow flurries are gradually replaced by pockets of rain.\n\r",
               "The snow flurries are gradually replaced by pockets of rain.\n\r",
               "The falling snow turns to a cold drizzle.\n\r",
               "The snow turns to rain as the air warms.\n\r"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_CYAN;
         }
         break;

      case 4:
         if( precip - dP > 2 * weath_unit )
         {
            if( tindex > 1 )
            {
               char *echo_strings[4] = {
                  "The lightning has stopped.\n\r",
                  "The lightning has stopped.\n\r",
                  "The sky settles, and the thunder surrenders.\n\r",
                  "The lightning bursts fade as the storm weakens.\n\r"
               };
               weath->echo = echo_strings[n];
               weath->echo_color = AT_GREY;
            }
         }
         else if( tindex < 2 && temp - dT > -weath_unit )
         {
            char *echo_strings[4] = {
               "The cold rain turns to snow.\n\r",
               "The cold rain turns to snow.\n\r",
               "Snow flakes begin to fall amidst the rain.\n\r",
               "The driving rain begins to freeze.\n\r"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_WHITE;
         }
         else if( tindex > 1 && temp - dT <= -weath_unit )
         {
            char *echo_strings[4] = {
               "The snow becomes a freezing rain.\n\r",
               "The snow becomes a freezing rain.\n\r",
               "A cold rain beats down on you as the snow begins to melt.\n\r",
               "The snow is slowly replaced by a heavy rain.\n\r"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_CYAN;
         }
         break;

      case 5:
         if( precip - dP <= 2 * weath_unit )
         {
            if( tindex > 1 )
            {
               char *echo_strings[4] = {
                  "Lightning flashes in the sky.\n\r",
                  "Lightning flashes in the sky.\n\r",
                  "A flash of lightning splits the sky.\n\r",
                  "The sky flashes, and the ground trembles with thunder.\n\r"
               };
               weath->echo = echo_strings[n];
               weath->echo_color = AT_YELLOW;
            }
         }
         else if( tindex > 1 && temp - dT <= -weath_unit )
         {
            char *echo_strings[4] = {
               "The sky rumbles with thunder as the snow changes to rain.\n\r",
               "The sky rumbles with thunder as the snow changes to rain.\n\r",
               "The falling turns to freezing rain amidst flashes of lightning.\n\r",
               "The falling snow begins to melt as thunder crashes overhead.\n\r"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_WHITE;
         }
         else if( tindex < 2 && temp - dT > -weath_unit )
         {
            char *echo_strings[4] = {
               "The lightning stops as the rainstorm becomes a blinding blizzard.\n\r",
               "The lightning stops as the rainstorm becomes a blinding blizzard.\n\r",
               "The thunder dies off as the pounding rain turns to heavy snow.\n\r",
               "The cold rain turns to snow and the lightning stops.\n\r"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_CYAN;
         }
         break;

      default:
         bug( "%s", "echo_weather: invalid precip index" );
         weath->precip = 0;
         break;
   }

   return;
}

/*
 * get echo messages according to time changes...
 * some echoes depend upon the weather so an echo must be
 * found for each area
 * Last Modified: August 10, 1997
 * Fireblade
 */
void get_time_echo( WEATHER_DATA * weath )
{
   int n;
   int pindex;

   n = number_bits( 2 );
   pindex = ( weath->precip + 3 * weath_unit - 1 ) / weath_unit;
   weath->echo = NULL;
   weath->echo_color = AT_GREY;

   if( time_info.hour == sysdata.hourdaybegin )
   {
      char *echo_strings[4] = {
         "The day has begun.\n\r",
         "The day has begun.\n\r",
         "The sky slowly begins to glow.\n\r",
         "The sun slowly embarks upon a new day.\n\r"
      };
      time_info.sunlight = SUN_RISE;
      weath->echo = echo_strings[n];
      weath->echo_color = AT_YELLOW;
   }
   if( time_info.hour == sysdata.hoursunrise )
   {
      char *echo_strings[4] = {
         "The sun rises in the east.\n\r",
         "The sun rises in the east.\n\r",
         "The hazy sun rises over the horizon.\n\r",
         "Day breaks as the sun lifts into the sky.\n\r"
      };
      time_info.sunlight = SUN_LIGHT;
      weath->echo = echo_strings[n];
      weath->echo_color = AT_ORANGE;
   }
   if( time_info.hour == sysdata.hournoon )
   {
      if( pindex > 0 )
      {
         weath->echo = "It's noon.\n\r";
      }
      else
      {
         char *echo_strings[2] = {
            "The intensity of the sun heralds the noon hour.\n\r",
            "The sun's bright rays beat down upon your shoulders.\n\r"
         };
         weath->echo = echo_strings[n % 2];
      }
      time_info.sunlight = SUN_LIGHT;
      weath->echo_color = AT_WHITE;
   }
   if( time_info.hour == sysdata.hoursunset )
   {
      char *echo_strings[4] = {
         "The sun slowly disappears in the west.\n\r",
         "The reddish sun sets past the horizon.\n\r",
         "The sky turns a reddish orange as the sun ends its journey.\n\r",
         "The sun's radiance dims as it sinks in the sky.\n\r"
      };
      time_info.sunlight = SUN_SET;
      weath->echo = echo_strings[n];
      weath->echo_color = AT_RED;
   }
   if( time_info.hour == sysdata.hournightbegin )
   {
      if( pindex > 0 )
      {
         char *echo_strings[2] = {
            "The night begins.\n\r",
            "Twilight descends around you.\n\r"
         };
         weath->echo = echo_strings[n % 2];
      }
      else
      {
         char *echo_strings[2] = {
            "The moon's gentle glow diffuses through the night sky.\n\r",
            "The night sky gleams with glittering starlight.\n\r"
         };
         weath->echo = echo_strings[n % 2];
      }
      time_info.sunlight = SUN_DARK;
      weath->echo_color = AT_DBLUE;
   }
   return;
}

/*
 * function updates weather for each area
 * Last Modified: July 31, 1997
 * Fireblade
 */
void weather_update(  )
{
   AREA_DATA *pArea;
   DESCRIPTOR_DATA *d;
   int limit;

   limit = 3 * weath_unit;

   for( pArea = first_area; pArea; pArea = pArea->next )
   {
      /*
       * Apply vectors to fields 
       */
      pArea->weather->temp += pArea->weather->temp_vector;
      pArea->weather->precip += pArea->weather->precip_vector;
      pArea->weather->wind += pArea->weather->wind_vector;

      /*
       * Make sure they are within the proper range 
       */
      pArea->weather->temp = URANGE( -limit, pArea->weather->temp, limit );
      pArea->weather->precip = URANGE( -limit, pArea->weather->precip, limit );
      pArea->weather->wind = URANGE( -limit, pArea->weather->wind, limit );

      /*
       * get an appropriate echo for the area 
       */
      get_weather_echo( pArea->weather );
   }

   for( pArea = first_area; pArea; pArea = pArea->next )
      adjust_vectors( pArea->weather );

   /*
    * display the echo strings to the appropriate players 
    */
   for( d = first_descriptor; d; d = d->next )
   {
      WEATHER_DATA *weath;

      if( d->connected == CON_PLAYING && IS_OUTSIDE( d->character ) &&
          !INDOOR_SECTOR( d->character->in_room->sector_type ) && IS_AWAKE( d->character ) )
         /*
          * Changed to use INDOOR_SECTOR macro - Samson 9-27-98 
          */
      {
         weath = d->character->in_room->area->weather;
         if( !weath->echo )
            continue;
         set_char_color( weath->echo_color, d->character );
         send_to_char( weath->echo, d->character );
      }
   }
   return;
}

/*
 * update the time
 */
void time_update( void )
{
   AREA_DATA *pArea;
   DESCRIPTOR_DATA *d;
   WEATHER_DATA *weath;

   ++time_info.hour;

   if( time_info.hour == 1 )
      update_month_trigger = FALSE;

   if( time_info.hour == sysdata.hourdaybegin || time_info.hour == sysdata.hoursunrise
       || time_info.hour == sysdata.hournoon || time_info.hour == sysdata.hoursunset
       || time_info.hour == sysdata.hournightbegin )
   {
      for( pArea = first_area; pArea; pArea = pArea->next )
         get_time_echo( pArea->weather );

      for( d = first_descriptor; d; d = d->next )
      {
         if( d->connected == CON_PLAYING && IS_OUTSIDE( d->character ) && IS_AWAKE( d->character ) )
         {
            weath = d->character->in_room->area->weather;
            if( !weath->echo )
               continue;
            set_char_color( weath->echo_color, d->character );
            send_to_char( weath->echo, d->character );
         }
      }
   }
   if( time_info.hour == sysdata.hourmidnight )
   {
      time_info.hour = 0;
      time_info.day++;
      /*
       * Sweep old crap from auction houses on daily basis - Samson 11-1-99 
       */
      clean_auctions(  );
   }

   if( time_info.day >= sysdata.dayspermonth )
   {
      time_info.day = 0;
      time_info.month++;
      update_month_trigger = TRUE;
   }

   if( time_info.month >= sysdata.monthsperyear )
   {
      time_info.month = 0;
      time_info.year++;
   }
   calc_season(  );  /* Samson 5-6-99 */
   /*
    * Save game world time - Samson 1-21-99 
    */
   save_timedata(  );
   return;
}

void subtract_times( struct timeval *endtime, struct timeval *starttime )
{
   endtime->tv_sec -= starttime->tv_sec;
   endtime->tv_usec -= starttime->tv_usec;
   while( endtime->tv_usec < 0 )
   {
      endtime->tv_usec += 1000000;
      endtime->tv_sec--;
   }
   return;
}

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */
void update_handler( void )
{
   static int pulse_environment;
   static int pulse_mobile;
   static int pulse_skyship;
   static int pulse_violence;
   static int pulse_spell;
   static int pulse_point;
   static int pulse_second;
   static int pulse_time;
   struct timeval sttime;
   struct timeval entime;

   if( timechar )
   {
      set_char_color( AT_PLAIN, timechar );
      send_to_char( "Starting update timer.\n\r", timechar );
      gettimeofday( &sttime, NULL );
   }

   if( --pulse_mobile <= 0 )
   {
      pulse_mobile = sysdata.pulsemobile;
      mobile_update(  );
   }

   if( --pulse_skyship <= 0 )
   {
      pulse_skyship = sysdata.pulseskyship;
      skyship_update(  );
   }

   if( --pulse_environment <= 0 )
   {
      pulse_environment = sysdata.pulseenvironment;
      environment_update(  );
   }

   /*
    * If this damn thing hadn't been used as a base value in about 50 other places..... 
    */
   if( --pulse_violence <= 0 )
      pulse_violence = sysdata.pulseviolence;

   if( --pulse_spell <= 0 )
   {
      pulse_spell = sysdata.pulsespell;
      spell_update(  );
   }

   if( --pulse_time <= 0 )
   {
      pulse_time = sysdata.pulsecalendar;

      time_update(  );
      weather_update(  );
      char_calendar_update(  );
   }

   if( --pulse_point <= 0 )
   {
      pulse_point = number_range( ( int )( sysdata.pulsetick * 0.65 ), ( int )( sysdata.pulsetick * 1.35 ) );

      auth_update(  );  /* Gorog */
      char_update(  );
      obj_update(  );
   }

   if( --pulse_second <= 0 )
   {
      pulse_second = sysdata.pulsepersec;
      char_check(  );
      check_pfiles( 0 );
      check_boards( board_expire_time_t );
      prune_dns(  );
   }

   mpsleep_update(  );  /* Check for sleeping mud progs  -rkb */
   tele_update(  );
   aggr_update(  );
   obj_act_update(  );
   room_act_update(  );
   clean_obj_queue(  ); /* dispose of extracted objects */
   clean_char_queue(  );   /* dispose of dead mobs/quitting chars */
   run_events( current_time );
   if( timechar )
   {
      gettimeofday( &entime, NULL );
      set_char_color( AT_PLAIN, timechar );
      send_to_char( "Update timing complete.\n\r", timechar );
      subtract_times( &entime, &sttime );
      ch_printf( timechar, "Timing took %ld.%ld seconds.\n\r", entime.tv_sec, entime.tv_usec );
      timechar = NULL;
   }
   tail_chain(  );
   return;
}

void remove_portal( OBJ_DATA * portal )
{
   ROOM_INDEX_DATA *fromRoom, *toRoom;
   EXIT_DATA *pexit;
   bool found;

   if( !portal )
   {
      bug( "%s", "remove_portal: portal is NULL" );
      return;
   }

   fromRoom = portal->in_room;
   found = FALSE;
   if( !fromRoom )
   {
      bug( "%s", "remove_portal: portal->in_room is NULL" );
      return;
   }

   for( pexit = fromRoom->first_exit; pexit; pexit = pexit->next )
      if( IS_EXIT_FLAG( pexit, EX_PORTAL ) )
      {
         found = TRUE;
         break;
      }

   if( !found )
   {
      bug( "remove_portal: portal not found in room %d!", fromRoom->vnum );
      return;
   }

   if( pexit->vdir != DIR_PORTAL )
      bug( "remove_portal: exit in dir %d != DIR_PORTAL", pexit->vdir );

   if( ( toRoom = pexit->to_room ) == NULL )
      bug( "%s", "remove_portal: toRoom is NULL" );

   extract_exit( fromRoom, pexit );

   return;
}
