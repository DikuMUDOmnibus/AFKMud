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
 *                         Player skills module                             *
 ****************************************************************************/

#include <string.h>
#include <limits.h>
#if defined(__FreeBSD__) || defined(__CYGWIN__) || defined(__OpenBSD__)
#include <sys/time.h>
#endif
#include "mud.h"
#include "fight.h"
#include "overland.h"
#include "polymorph.h"

void skill_notfound( CHAR_DATA * ch, char *argument );
SPELLF spell_smaug( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_notfound( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_null( int sn, int level, CHAR_DATA * ch, void *vo );
void show_char_to_char( CHAR_DATA * list, CHAR_DATA * ch );
int ris_save( CHAR_DATA * ch, int rchance, int ris );
bool validate_spec_fun( char *name );
void adjust_favor( CHAR_DATA * ch, int field, int mod );
bool is_safe( CHAR_DATA * ch, CHAR_DATA * victim );
bool check_illegal_pk( CHAR_DATA * ch, CHAR_DATA * victim );
bool legal_loot( CHAR_DATA * ch, CHAR_DATA * victim );
void set_fighting( CHAR_DATA * ch, CHAR_DATA * victim );
void failed_casting( struct skill_type *skill, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj );
void start_timer( struct timeval *starttime );
time_t end_timer( struct timeval *endtime );
void check_mount_objs( CHAR_DATA * ch, bool fell );
int get_door( char *arg );
void check_killer( CHAR_DATA * ch, CHAR_DATA * victim );
void raw_kill( CHAR_DATA * ch, CHAR_DATA * victim );
void death_cry( CHAR_DATA * ch );
void group_gain( CHAR_DATA * ch, CHAR_DATA * victim );
ch_ret one_hit( CHAR_DATA * ch, CHAR_DATA * victim, int dt );
ch_ret check_room_for_traps( CHAR_DATA * ch, int flag );

const char *att_kick_kill_ch[] = {
   "Your kick caves $N's chest in, which kills $M.",
   "Your kick destroys $N's arm and caves in one side of $S rib cage.",
   "Your kick smashes through $N's leg and into $S pelvis, killing $M.",
   "Your kick shatters $N's skull.",
   "Your kick at $N's snout shatters $S jaw, killing $M.",
   "You kick $N in the rump with such force that $E keels over dead.",
   "You kick $N in the belly, mangling several ribs and killing $M instantly.",
   "$N's scales cave in as your mighty kick kills $N.",
   "Your kick rips bark asunder and leaves fly everywhere, killing the $N.",
   "Bits of $N are sent flying as you kick him to pieces.",
   "You punt $N across the room, $E lands in a heap of broken flesh.",
   "You kick $N in the groin, $E dies screaming an octave higher.",
   "",   /* GHOST */
   "Feathers fly about as you blast $N to pieces with your kick.",
   "Your kick splits $N to pieces, rotting flesh flies everywhere.",
   "Your kick topples $N over, killing it.",
   "Your foot shatters cartilage, sending bits of $N everywhere.",
   "You launch a mighty kick at $N's gills, killing it.",
   "Your kick at $N sends $M to the grave.",
   "."
};

const char *att_kick_kill_victim[] = {
   "$n crushes you beneath $s foot, killing you.",
   "$n destroys your arm and half your ribs.  You die.",
   "$n neatly splits your groin in two, you collapse and die instantly.",
   "$n splits your head in two, killing you instantly.",
   "$n forces your jaw into the lower part of your brain.",
   "$n kicks you from behind, snapping your spine and killing you.",
   "$n kicks your stomach and you into the great land beyond!!",
   "Your scales are no defense against $n's mighty kick.",
   "$n rips you apart with a massive kick, you die in a flutter of leaves.",
   "You are torn to little pieces as $n splits you with $s kick.",
   "$n's kick sends you flying, you die before you land.",
   "Puny little $n manages to land a killing blow to your groin, OUCH!",
   "",   /* GHOST */
   "Your feathers fly about as $n pulverizes you with a massive kick.",
   "$n's kick rips your rotten body into shreds, and your various pieces die.",
   "$n kicks you so hard, you fall over and die.",
   "$n shatters your exoskeleton, you die.",
   "$n kicks you in the gills!  You cannot breath..... you die!.",
   "$n sends you to the grave with a mighty kick.",
   "."
};

const char *att_kick_kill_room[] = {
   "$n strikes $N in chest, shattering the ribs beneath it.",
   "$n kicks $N in the side, destroying $S arm and ribs.",
   "$n nails $N in the groin, the pain killing $M.",
   "$n shatters $N's head, reducing $M to a twitching heap!",
   "$n blasts $N in the snout, destroying bones and causing death.",
   "$n kills $N with a massive kick to the rear.",
   "$n sends $N to the grave with a massive blow to the stomach!",
   "$n ignores $N's scales and kills $M with a mighty kick.",
   "$n sends bark and leaves flying as $e splits $N in two.",
   "$n blasts $N to pieces with a ferocious kick.",
   "$n sends $N flying, $E lands with a LOUD THUD, making no other noise.",
   "$N falls to the ground and dies clutching $S crotch due to $n's kick.",
   "",   /* GHOST */
   "$N disappears into a cloud of feathers as $n kicks $M to death.",
   "$n blasts $N's rotten body into pieces with a powerful kick.",
   "$n kicks $N so hard, it falls over and dies.",
   "$n blasts $N's exoskeleton to little fragments.",
   "$n kicks $N in the gills, killing it.",
   "$n sends $N to the grave with a mighty kick.",
   "."
};

const char *att_kick_miss_ch[] = {
   "$N steps back, and your kick misses $M.",
   "$N deftly blocks your kick with $S forearm.",
   "$N dodges, and you miss your kick at $S legs.",
   "$N ducks, and your foot flies a mile high.",
   "$N steps back and grins evilly as your foot flys by $S face.",
   "$N laughs at your feeble attempt to kick $M from behind.",
   "Your kick at $N's belly makes it laugh.",
   "$N chuckles as your kick bounces off $S tough scales.",
   "You kick $N in the side, denting your foot.",
   "Your sloppy kick is easily avoided by $N.",
   "You misjudge $N's height and kick well above $S head.",
   "You stub your toe against $N's shin as you try to kick $M.",
   "Your kick passes through $N!!", /* Ghost */
   "$N nimbly flitters away from your kick.",
   "$N sidesteps your kick and sneers at you.",
   "Your kick bounces off $N's leathery hide.",
   "Your kick bounces off $N's tough exoskeleton.",
   "$N deflects your kick with a fin.",
   "$N avoids your paltry attempt at a kick.",
   "."
};

const char *att_kick_miss_victim[] = {
   "$n misses you with $s clumsy kick at your chest.",
   "You block $n's feeble kick with your arm.",
   "You dodge $n's feeble leg sweep.",
   "You duck under $n's lame kick.",
   "You step back and grin as $n misses your face with a kick.",
   "$n attempts a feeble kick from behind, which you neatly avoid.",
   "You laugh at $n's feeble attempt to kick you in the stomach.",
   "$n kicks you, but your scales are much too tough for that wimp.",
   "You laugh as $n dents $s foot on your bark.",
   "You easily avoid a sloppy kick from $n.",
   "$n's kick parts your hair but does little else.",
   "$n's light kick to your shin bearly gets your attention.",
   "$n passes through you with $s puny kick.",
   "You nimbly flitter away from $n's kick.",
   "You sneer as you sidestep $n's kick.",
   "$n's kick bounces off your tough hide.",
   "$n tries to kick you, but your too tough.",
   "$n tried to kick you, but you deflected it with a fin.",
   "You avoid $n's feeble attempt to kick you.",
   "."
};

const char *att_kick_miss_room[] = {
   "$n misses $N with a clumsy kick.",
   "$N blocks $n's kick with $S arm.",
   "$N easily dodges $n's feeble leg sweep.",
   "$N easily ducks under $n's lame kick.",
   "$N steps back and grins evilly at $n's feeble kick to $S face misses.",
   "$n launches a kick at $N's behind, but fails miserably.",
   "$N laughs at $n's attempt to kick $M in the stomach.",
   "$n tries to kick $N, but $s foot bounces off of $N's scales.",
   "$n hurts his foot trying to kick $N.",
   "$N avoids a lame kick launched by $n.",
   "$n misses a kick at $N due to $S small size.",
   "$n misses a kick at $N's groin, stubbing $s toe in the process.",
   "$n's foot goes right through $N!!!!",
   "$N flitters away from $n's kick.",
   "$N sneers at $n while sidestepping $s kick.",
   "$N's tough hide deflects $n's kick.",
   "$n hurts $s foot on $N's tough exterior.",
   "$n tries to kick $N, but is thwarted by a fin.",
   "$N avoids $n's feeble kick.",
   "."
};

const char *att_kick_hit_ch[] = {
   "Your kick crashes into $N's chest.",
   "Your kick hits $N in the side.",
   "You hit $N in the thigh with a hefty sweep.",
   "You hit $N in the face, sending $M reeling.",
   "You plant your foot firmly in $N's snout, smashing it to one side.",
   "You nail $N from behind, sending him reeling.",
   "You kick $N in the stomach, winding $M.",
   "You find a soft spot in $N's scales and launch a solid kick there.",
   "Your kick hits $N, sending small branches and leaves everywhere.",
   "Your kick contacts with $N, dislodging little pieces of $M.",
   "Your kick hits $N right in the stomach, $N is rendered breathless.",
   "You stomp on $N's foot. After all, thats about all you can do to a giant.",
   "",   /* GHOST */
   "Your kick  sends $N reeling through the air.",
   "You kick $N and feel rotten bones crunch from the blow.",
   "You smash $N with a hefty roundhouse kick.",
   "You kick $N, cracking it's exoskeleton.",
   "Your mighty kick rearranges $N's scales.",
   "You leap off the ground and crash into $N with a powerful kick.",
   "."
};

const char *att_kick_hit_victim[] = {
   "$n's kick crashes into your chest.",
   "$n's kick hits you in your side.",
   "$n's sweep catches you in the side and you almost stumble.",
   "$n hits you in the face, gee, what pretty colors...",
   "$n kicks you in the snout, smashing it up against your face.",
   "$n blasts you in the rear, ouch!",
   "Your breath rushes from you as $n kicks you in the stomach.",
   "$n finds a soft spot on your scales and kicks you, ouch!",
   "$n kicks you hard, sending leaves flying everywhere!",
   "$n kicks you in the side, dislodging small parts of you.",
   "You suddenly see $n's foot in your chest.",
   "$n lands a kick hard on your foot making you jump around in pain.",
   "",   /* GHOST */
   "$n kicks you, and you go reeling through the air.",
   "$n kicks you and your bones crumble.",
   "$n hits you in the flank with a hefty roundhouse kick.",
   "$n ruins some of your scales with a well placed kick.",
   "$n leaps off of the grand and crashes into you with $s kick.",
   "."
};

const char *att_kick_hit_room[] = {
   "$n hits $N with a mighty kick to $S chest.",
   "$n whacks $N in the side with a sound kick.",
   "$n almost sweeps $N off of $S feet with a well placed leg sweep.",
   "$N's eyes roll as $n plants a foot in $S face.",
   "$N's snout is smashed as $n relocates it with $s foot.",
   "$n hits $N with an impressive kick from behind.",
   "$N gasps as $n kick $N in the stomach.",
   "$n finds a soft spot in $N's scales and launches a solid kick there.",
   "$n kicks $N.  Leaves fly everywhere!!",
   "$n hits $N with a mighty kick, $N loses parts of $Mself.",
   "$n kicks $N in the stomach, $N is rendered breathless.",
   "$n kicks $N in the foot, $N hops around in pain.",
   "",   /* GHOST */
   "$n sends $N reeling through the air with a mighty kick.",
   "$n kicks $N causing parts of $N to cave in!",
   "$n kicks $N in the side with a hefty roundhouse kick.",
   "$n kicks $N, cracking exo-skelelton.",
   "$n kicks $N hard, sending scales flying!",
   "$n leaps up and nails $N with a mighty kick.",
   "."
};

char *const spell_flag[] = { "water", "earth", "air", "astral", "area", "distant", "reverse",
   "noself", "nobind", "accumulative", "recastable", "noscribe",
   "nobrew", "group", "object", "character", "secretskill", "pksensitive",
   "stoponfail", "nofight", "nodispel", "randomtarget", "r2", "r3", "r4",
   "r5", "r6", "r7", "r8", "r9", "r10", "r11"
};

const char *spell_saves[] = { "none", "Poison/Death", "Wands", "Paralysis/Petrification", "Breath", "Spells" };

const char *spell_save_effect[] =
   { "none", "Negation", "1/8 Damage", "1/4 Damage", "Half Damage", "3/4 Damage", "Reflection", "Absorbtion" };

char *const spell_damage[] = { "none", "fire", "cold", "electricity", "energy", "acid", "poison", "drain" };

char *const spell_action[] = { "none", "create", "destroy", "resist", "suscept", "divinate", "obscure", "change" };

char *const spell_power[] = { "none", "minor", "greater", "major" };

char *const spell_class[] = { "none", "lunar", "solar", "travel", "summon", "life", "death", "illusion" };

char *const target_type[] = { "ignore", "offensive", "defensive", "self", "objinv" };

void free_skill( SKILLTYPE * skill )
{
   SMAUG_AFF *aff, *aff_next;

   if( skill->affects )
   {
      for( aff = skill->affects; aff; aff = aff_next )
      {
         aff_next = aff->next;

         DISPOSE( aff->duration );
         DISPOSE( aff->modifier );
         DISPOSE( aff );
      }
   }
   DISPOSE( skill->name );
   STRFREE( skill->author );
   DISPOSE( skill->noun_damage );
   DISPOSE( skill->msg_off );
   DISPOSE( skill->hit_char );
   DISPOSE( skill->hit_vict );
   DISPOSE( skill->hit_room );
   DISPOSE( skill->hit_dest );
   DISPOSE( skill->miss_char );
   DISPOSE( skill->miss_vict );
   DISPOSE( skill->miss_room );
   DISPOSE( skill->die_char );
   DISPOSE( skill->die_vict );
   DISPOSE( skill->die_room );
   DISPOSE( skill->imm_char );
   DISPOSE( skill->imm_vict );
   DISPOSE( skill->imm_room );
   DISPOSE( skill->dice );
   DISPOSE( skill->components );
   DISPOSE( skill->teachers );
   skill->spell_fun = NULL;
   skill->skill_fun = NULL;
   DISPOSE( skill->spell_fun_name );
   DISPOSE( skill->skill_fun_name );
   DISPOSE( skill );

   return;
}

void free_skills( void )
{
   SKILLTYPE *skill;
   int hash = 0;

   for( hash = 0; hash < top_sn; hash++ )
   {
      skill = skill_table[hash];
      free_skill( skill );
   }

   for( hash = 0; hash < top_herb; hash++ )
   {
      skill = herb_table[hash];
      free_skill( skill );
   }
   return;
}

int IsGiant( CHAR_DATA * ch )
{
   if( !ch )
      return ( FALSE );

   switch ( GET_RACE( ch ) )
   {
      case RACE_GIANT:
      case RACE_GIANT_HILL:
      case RACE_GIANT_FROST:
      case RACE_GIANT_FIRE:
      case RACE_GIANT_CLOUD:
      case RACE_GIANT_STORM:
      case RACE_GIANT_STONE:
      case RACE_HALF_GIANT:
      case RACE_TYTAN:
      case RACE_GOD:
      case RACE_GIANT_SKELETON:
      case RACE_TROLL:
         return ( TRUE );
      default:
         return ( FALSE );
   }
}

/*
 * Dummy function
 */
void skill_notfound( CHAR_DATA * ch, char *argument )
{
   send_to_char( "Huh?\n\r", ch );
   return;
}

/*
 * New skill computation, based on character's current learned value
 * as well as character's intelligence + wisdom. Samson 3-13-98.
 *
 * Modified 5-28-99 to conform with Crystal Shard advancement code,
 * int + wis factor removed from consideration. Difficulty rating removed
 * from consideration - Samson
 */
void learn_racials( CHAR_DATA * ch, int sn )
{
   int adept, gain, sklvl;

   sklvl = skill_table[sn]->race_level[ch->race];
   if( sklvl == 0 )
      sklvl = ch->level;

   if( IS_NPC( ch ) || ch->pcdata->learned[sn] == 0 || ch->level < skill_table[sn]->race_level[ch->race] )
      return;

   if( number_percent(  ) > ch->pcdata->learned[sn] / 2 )
   {
      adept = skill_table[sn]->race_adept[ch->race];

      if( ch->pcdata->learned[sn] < adept )
      {
         ch->pcdata->learned[sn] += 1;

         if( !skill_table[sn]->skill_fun && str_cmp( skill_table[sn]->spell_fun_name, "spell_null" ) )
            send_to_char( "You learn from your mistake.\n\r", ch );

         if( ch->pcdata->learned[sn] == adept ) /* fully learned! */
         {
            gain = sklvl * 1000;
            if( ch->Class == CLASS_MAGE )
               gain = gain * 2;
            ch_printf( ch, "&WYou are now an adept of %s! You gain %d bonus experience!\n\r", skill_table[sn]->name, gain );
            gain_exp( ch, gain );
         }
      }
   }
   return;
}

void learn_from_failure( CHAR_DATA * ch, int sn )
{
   int adept, gain, lchance, sklvl;

   lchance = number_range( 1, 2 );

   if( lchance == 2 )   /* Time to cut into the speed of advancement a bit */
      return;

   if( skill_table[sn]->type == SKILL_RACIAL )
   {
      learn_racials( ch, sn );
      return;
   }

   /*
    * Edited by Tarl, 7th June 2003 to make weapon skills increase 1 in 20 hits or so 
    */
   if( skill_table[sn]->type == SKILL_WEAPON )
   {
      lchance = number_range( 1, 10 );
      if( lchance != 5 )
      {
         return;
      }

   }

   sklvl = skill_table[sn]->skill_level[ch->Class];
   if( sklvl == 0 )
      sklvl = 1;

   if( IS_NPC( ch ) || ch->pcdata->learned[sn] == 0 || ch->level < skill_table[sn]->skill_level[ch->Class] )
      return;

   if( number_percent(  ) > ch->pcdata->learned[sn] / 2 )
   {
      adept = GET_ADEPT( ch, sn );
      if( ch->pcdata->learned[sn] < adept )
      {
         ch->pcdata->learned[sn] += 1;

         if( !skill_table[sn]->skill_fun && str_cmp( skill_table[sn]->spell_fun_name, "spell_null" ) )
            send_to_char( "You learn from your mistake.\n\r", ch );

         if( ch->pcdata->learned[sn] == adept ) /* fully learned! */
         {
            gain = sklvl * 1000;
            if( ch->Class == CLASS_MAGE )
               gain = gain * 2;
            ch_printf( ch, "&WYou are now an adept of %s! You gain %d bonus experience!\n\r", skill_table[sn]->name, gain );
            gain_exp( ch, gain );
         }
      }
   }
   return;
}

/* New command to view a player's skills - Samson 4-13-98 */
CMDF do_viewskills( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   int sn, col;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "&zSyntax: skills <player>.\n\r", ch );
      return;
   }

   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "No such person in the game.\n\r", ch );
      return;
   }

   col = 0;

   if( !IS_NPC( victim ) )
   {
      set_char_color( AT_SKILL, ch );
      for( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ )
      {
         if( skill_table[sn]->name == NULL )
            break;
         if( victim->pcdata->learned[sn] == 0 )
            continue;

         ch_printf( ch, "%20s %3d%% ", skill_table[sn]->name, victim->pcdata->learned[sn] );

         if( ++col % 3 == 0 )
            send_to_char( "\n\r", ch );
      }
   }
   return;
}

int get_ssave( char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_saves ) / sizeof( spell_saves[0] ); x++ )
      if( !str_cmp( name, spell_saves[x] ) )
         return x;
   return -1;
}

int get_starget( char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( target_type ) / sizeof( target_type[0] ); x++ )
      if( !str_cmp( name, target_type[x] ) )
         return x;
   return -1;
}

int get_sflag( char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_flag ) / sizeof( spell_flag[0] ); x++ )
      if( !str_cmp( name, spell_flag[x] ) )
         return x;
   return -1;
}

int get_sdamage( char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_damage ) / sizeof( spell_damage[0] ); x++ )
      if( !str_cmp( name, spell_damage[x] ) )
         return x;
   return -1;
}

int get_saction( char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_action ) / sizeof( spell_action[0] ); x++ )
      if( !str_cmp( name, spell_action[x] ) )
         return x;
   return -1;
}

int get_ssave_effect( char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_save_effect ) / sizeof( spell_save_effect[0] ); x++ )
      if( !str_cmp( name, spell_save_effect[x] ) )
         return x;
   return -1;
}

int get_spower( char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_power ) / sizeof( spell_power[0] ); x++ )
      if( !str_cmp( name, spell_power[x] ) )
         return x;
   return -1;
}

int get_sclass( char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_class ) / sizeof( spell_class[0] ); x++ )
      if( !str_cmp( name, spell_class[x] ) )
         return x;
   return -1;
}

bool is_legal_kill( CHAR_DATA * ch, CHAR_DATA * vch )
{
   if( IS_NPC( ch ) || IS_NPC( vch ) )
      return TRUE;
   if( !IS_PKILL( ch ) || !IS_PKILL( vch ) )
      return FALSE;
   if( ch->pcdata->clan && ch->pcdata->clan == vch->pcdata->clan )
      return FALSE;
   return TRUE;
}

/*  New check to see if you can use skills to support morphs --Shaddai */
bool can_use_skill( CHAR_DATA * ch, int percent, int gsn )
{
   bool check = FALSE;
   if( IS_NPC( ch ) && percent < 85 )
      check = TRUE;
   else if( !IS_NPC( ch ) && percent < LEARNED( ch, gsn ) )
      check = TRUE;
   else if( ch->morph && ch->morph->morph && ch->morph->morph->skills &&
            ch->morph->morph->skills[0] != '\0' && is_name( skill_table[gsn]->name, ch->morph->morph->skills )
            && percent < 85 )
      check = TRUE;
   if( ch->morph && ch->morph->morph && ch->morph->morph->no_skills &&
       ch->morph->morph->no_skills[0] != '\0' && is_name( skill_table[gsn]->name, ch->morph->morph->no_skills ) )
      check = FALSE;
   return check;
}

bool check_ability( CHAR_DATA * ch, char *command, char *argument )
{
   int sn;
   int first = gsn_first_ability;
   int top = gsn_top_sn - 1;
   int mana;
   struct timeval time_used;

   /*
    * bsearch for the ability 
    */
   for( ;; )
   {
      sn = ( first + top ) >> 1;

      if( LOWER( command[0] ) == LOWER( skill_table[sn]->name[0] ) && !str_prefix( command, skill_table[sn]->name )
          && ( skill_table[sn]->skill_fun || skill_table[sn]->spell_fun != spell_null ) && ( can_use_skill( ch, 0, sn ) ) )
         break;

      if( first >= top )
         return FALSE;
      if( strcmp( command, skill_table[sn]->name ) < 1 )
         top = sn - 1;
      else
         first = sn + 1;
   }

   if( !check_pos( ch, skill_table[sn]->minimum_position ) )
      return TRUE;

   if( IS_NPC( ch ) && ( IS_AFFECTED( ch, AFF_CHARM ) || IS_AFFECTED( ch, AFF_POSSESS ) ) )
   {
      send_to_char( "For some reason, you seem unable to perform that...\n\r", ch );
      act( AT_GREY, "$n wanders around aimlessly.", ch, NULL, NULL, TO_ROOM );
      return TRUE;
   }

   /*
    * check if mana is required 
    */
   if( skill_table[sn]->min_mana )
   {
      mana = IS_NPC( ch ) ? 0 : UMAX( skill_table[sn]->min_mana,
                                      100 / ( 2 + ch->level - skill_table[sn]->race_level[ch->race] ) );

      if( !IS_NPC( ch ) && ch->mana < mana )
      {
         send_to_char( "You don't have enough mana.\n\r", ch );
         return TRUE;
      }
   }
   else
      mana = 0;

   /*
    * Is this a real do-fun, or a really a spell?
    */
   if( !skill_table[sn]->skill_fun )
   {
      ch_ret retcode = rNONE;
      void *vo = NULL;
      CHAR_DATA *victim = NULL;
      OBJ_DATA *obj = NULL;

      target_name = "";

      switch ( skill_table[sn]->target )
      {
         default:
            bug( "Check_ability: bad target for sn %d.", sn );
            send_to_char( "Something went wrong...\n\r", ch );
            return TRUE;

         case TAR_IGNORE:
            vo = NULL;
            if( !argument || argument[0] == '\0' )
            {
               if( ( victim = who_fighting( ch ) ) != NULL )
                  target_name = victim->name;
            }
            else
               target_name = argument;
            break;

         case TAR_CHAR_OFFENSIVE:
         {
            if( ( !argument || argument[0] == '\0' ) && !( victim = who_fighting( ch ) ) )
            {
               ch_printf( ch, "Confusion overcomes you as your '%s' has no target.\n\r", skill_table[sn]->name );
               return TRUE;
            }
            else if( argument[0] != '\0' && !( victim = get_char_room( ch, argument ) ) )
            {
               send_to_char( "They aren't here.\n\r", ch );
               return TRUE;
            }
         }
            if( is_safe( ch, victim ) )
               return TRUE;

            if( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) )
            {
               send_to_char( "You can't target yourself!\n\r", ch );
               return TRUE;
            }

            if( !IS_NPC( ch ) )
            {
               if( !IS_NPC( victim ) )
               {
                  if( get_timer( ch, TIMER_PKILLED ) > 0 )
                  {
                     send_to_char( "You have been killed in the last 5 minutes.\n\r", ch );
                     return TRUE;
                  }

                  if( get_timer( victim, TIMER_PKILLED ) > 0 )
                  {
                     send_to_char( "This player has been killed in the last 5 minutes.\n\r", ch );
                     return TRUE;
                  }

                  if( victim != ch )
                     send_to_char( "You really shouldn't do this to another player...\n\r", ch );
               }

               if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
               {
                  send_to_char( "You can't do that on your own follower.\n\r", ch );
                  return TRUE;
               }
            }

            if( check_illegal_pk( ch, victim ) )
            {
               send_to_char( "You can't do that to another player!\n\r", ch );
               return TRUE;
            }
            vo = ( void * )victim;
            break;

         case TAR_CHAR_DEFENSIVE:
         {
            if( argument[0] != '\0' && !( victim = get_char_room( ch, argument ) ) )
            {
               send_to_char( "They aren't here.\n\r", ch );
               return TRUE;
            }
            if( !victim )
               victim = ch;
         }

            if( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) )
            {
               send_to_char( "You can't target yourself!\n\r", ch );
               return TRUE;
            }
            vo = ( void * )victim;
            break;

         case TAR_CHAR_SELF:
            victim = ch;
            vo = ( void * )ch;
            break;

         case TAR_OBJ_INV:
         {
            if( !( obj = get_obj_carry( ch, argument ) ) )
            {
               send_to_char( "You can't find that.\n\r", ch );
               return TRUE;
            }
         }
            vo = ( void * )obj;
            break;
      }

      /*
       * waitstate 
       */
      WAIT_STATE( ch, skill_table[sn]->beats );
      /*
       * check for failure 
       */
      if( ( number_percent(  ) + skill_table[sn]->difficulty * 5 ) > ( IS_NPC( ch ) ? 75 : LEARNED( ch, sn ) ) )
      {
         failed_casting( skill_table[sn], ch, victim, obj );
         learn_from_failure( ch, sn );
         if( mana )
            ch->mana -= mana / 2;
         return TRUE;
      }
      if( mana )
         ch->mana -= mana;

      start_timer( &time_used );
      retcode = ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, vo );
      end_timer( &time_used );

      if( retcode == rCHAR_DIED || retcode == rERROR )
         return TRUE;

      if( char_died( ch ) )
         return TRUE;

      if( retcode == rSPELL_FAILED )
      {
         learn_from_failure( ch, sn );
         retcode = rNONE;
      }

      if( skill_table[sn]->target == TAR_CHAR_OFFENSIVE && victim != ch && !char_died( victim ) )
      {
         CHAR_DATA *vch;
         CHAR_DATA *vch_next;

         for( vch = ch->in_room->first_person; vch; vch = vch_next )
         {
            vch_next = vch->next_in_room;
            if( victim == vch && !victim->fighting && victim->master != ch )
            {
               retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
               break;
            }
         }
      }
      return TRUE;
   }

   if( mana )
      ch->mana -= mana;

   ch->prev_cmd = ch->last_cmd;  /* haus, for automapping */
   ch->last_cmd = skill_table[sn]->skill_fun;
   start_timer( &time_used );
   ( *skill_table[sn]->skill_fun ) ( ch, argument );
   end_timer( &time_used );

   tail_chain(  );
   return TRUE;
}

/*
 * Perform a binary search on a section of the skill table
 * Each different section of the skill table is sorted alphabetically
 * Only match skills player knows				-Thoric
 */
bool check_skill( CHAR_DATA * ch, char *command, char *argument )
{
   int sn;
   int first = gsn_first_skill;
   int top = gsn_first_weapon - 1;
   int mana;
   struct timeval time_used;

   /*
    * bsearch for the skill 
    */
   for( ;; )
   {
      sn = ( first + top ) >> 1;

      if( LOWER( command[0] ) == LOWER( skill_table[sn]->name[0] ) && !str_prefix( command, skill_table[sn]->name )
          && ( skill_table[sn]->skill_fun || skill_table[sn]->spell_fun != spell_null ) && ( can_use_skill( ch, 0, sn ) ) )
         break;

      if( first >= top )
         return FALSE;
      if( strcmp( command, skill_table[sn]->name ) < 1 )
         top = sn - 1;
      else
         first = sn + 1;
   }

   if( !check_pos( ch, skill_table[sn]->minimum_position ) )
      return TRUE;

   if( IS_NPC( ch ) && ( IS_AFFECTED( ch, AFF_CHARM ) || IS_AFFECTED( ch, AFF_POSSESS ) ) )
   {
      send_to_char( "For some reason, you seem unable to perform that...\n\r", ch );
      act( AT_GREY, "$n wanders around aimlessly.", ch, NULL, NULL, TO_ROOM );
      return TRUE;
   }

   /*
    * check if mana is required 
    */
   if( skill_table[sn]->min_mana )
   {
      mana = IS_NPC( ch ) ? 0 : UMAX( skill_table[sn]->min_mana,
                                      100 / ( 2 + ch->level - skill_table[sn]->skill_level[ch->Class] ) );

      if( !IS_NPC( ch ) && ch->mana < mana )
      {
         send_to_char( "You don't have enough mana.\n\r", ch );
         return TRUE;
      }
   }
   else
      mana = 0;

   /*
    * Is this a real do-fun, or a really a spell?
    */
   if( !skill_table[sn]->skill_fun )
   {
      ch_ret retcode = rNONE;
      void *vo = NULL;
      CHAR_DATA *victim = NULL;
      OBJ_DATA *obj = NULL;

      target_name = "";

      switch ( skill_table[sn]->target )
      {
         default:
            bug( "Check_skill: bad target for sn %d.", sn );
            send_to_char( "Something went wrong...\n\r", ch );
            return TRUE;

         case TAR_IGNORE:
            vo = NULL;
            if( !argument || argument[0] == '\0' )
            {
               if( ( victim = who_fighting( ch ) ) != NULL )
                  target_name = victim->name;
            }
            else
               target_name = argument;
            break;

         case TAR_CHAR_OFFENSIVE:
         {
            if( argument[0] == '\0' && !( victim = who_fighting( ch ) ) )
            {
               ch_printf( ch, "Confusion overcomes you as your '%s' has no target.\n\r", skill_table[sn]->name );
               return TRUE;
            }
            else if( argument[0] != '\0' && !( victim = get_char_room( ch, argument ) ) )
            {
               send_to_char( "They aren't here.\n\r", ch );
               return TRUE;
            }
         }
            if( is_safe( ch, victim ) )
               return TRUE;

            if( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) )
            {
               send_to_char( "You can't target yourself!\n\r", ch );
               return TRUE;
            }

            if( !IS_NPC( ch ) )
            {
               if( !IS_NPC( victim ) )
               {
                  if( get_timer( ch, TIMER_PKILLED ) > 0 )
                  {
                     send_to_char( "You have been killed in the last 5 minutes.\n\r", ch );
                     return TRUE;
                  }

                  if( get_timer( victim, TIMER_PKILLED ) > 0 )
                  {
                     send_to_char( "This player has been killed in the last 5 minutes.\n\r", ch );
                     return TRUE;
                  }

                  if( victim != ch )
                     send_to_char( "You really shouldn't do this to another player...\n\r", ch );
               }

               if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
               {
                  send_to_char( "You can't do that on your own follower.\n\r", ch );
                  return TRUE;
               }
            }

            if( check_illegal_pk( ch, victim ) )
            {
               send_to_char( "You can't do that to another player!\n\r", ch );
               return TRUE;
            }
            vo = ( void * )victim;
            break;

         case TAR_CHAR_DEFENSIVE:
         {
            if( argument[0] != '\0' && ( victim = get_char_room( ch, argument ) ) == NULL )
            {
               send_to_char( "They aren't here.\n\r", ch );
               return TRUE;
            }
            if( !victim )
               victim = ch;
         }
            if( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) )
            {
               send_to_char( "You can't target yourself!\n\r", ch );
               return TRUE;
            }
            vo = ( void * )victim;
            break;

         case TAR_CHAR_SELF:
            victim = ch;
            vo = ( void * )ch;
            break;

         case TAR_OBJ_INV:
         {
            if( !( obj = get_obj_carry( ch, argument ) ) )
            {
               send_to_char( "You can't find that.\n\r", ch );
               return TRUE;
            }
         }
            vo = ( void * )obj;
            break;
      }

      /*
       * waitstate 
       */
      WAIT_STATE( ch, skill_table[sn]->beats );
      /*
       * check for failure 
       */
      if( ( number_percent(  ) + skill_table[sn]->difficulty * 5 ) > ( IS_NPC( ch ) ? 75 : LEARNED( ch, sn ) ) )
      {
         failed_casting( skill_table[sn], ch, victim, obj );
         learn_from_failure( ch, sn );
         if( mana )
            ch->mana -= mana / 2;
         return TRUE;
      }
      if( mana )
         ch->mana -= mana;

      start_timer( &time_used );
      retcode = ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, vo );
      end_timer( &time_used );

      if( retcode == rCHAR_DIED || retcode == rERROR )
         return TRUE;

      if( char_died( ch ) )
         return TRUE;

      if( retcode == rSPELL_FAILED )
      {
         learn_from_failure( ch, sn );
         retcode = rNONE;
      }

      if( skill_table[sn]->target == TAR_CHAR_OFFENSIVE && victim != ch && !char_died( victim ) )
      {
         CHAR_DATA *vch;
         CHAR_DATA *vch_next;

         for( vch = ch->in_room->first_person; vch; vch = vch_next )
         {
            vch_next = vch->next_in_room;
            if( victim == vch && !victim->fighting && victim->master != ch )
            {
               retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
               break;
            }
         }
      }
      return TRUE;
   }

   if( mana )
      ch->mana -= mana;

   ch->prev_cmd = ch->last_cmd;  /* haus, for automapping */
   ch->last_cmd = skill_table[sn]->skill_fun;
   start_timer( &time_used );
   ( *skill_table[sn]->skill_fun ) ( ch, argument );
   end_timer( &time_used );

   tail_chain(  );
   return TRUE;
}

/*
 * Lookup a skills information
 * High god command
 */
CMDF do_slookup( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   int sn;
   int iClass, iRace;
   SKILLTYPE *skill = NULL;

   if( argument[0] == '\0' )
   {
      send_to_char( "Slookup what?\n\r"
                    "  slookup all\n\r  slookup null\n\r  slookup smaug\n\r"
                    "  slookup herbs\n\r  slookup tongues\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      for( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ )
         pager_printf( ch, "Sn: %4d Slot: %4d Skill/spell: '%-20s' Damtype: %s\n\r",
                       sn, skill_table[sn]->slot, skill_table[sn]->name, spell_damage[SPELL_DAMAGE( skill_table[sn] )] );
   }
   else if( !str_cmp( argument, "null" ) )
   {
      int num = 0;

      for( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ )
         if( ( skill_table[sn]->skill_fun == skill_notfound || skill_table[sn]->skill_fun == NULL )
             && ( skill_table[sn]->spell_fun == spell_notfound || skill_table[sn]->spell_fun == spell_null )
             && skill_table[sn]->type != SKILL_TONGUE )
         {
            pager_printf( ch, "Sn: %3d Slot: %3d Name: '%-24s' Damtype: %s\n\r",
                          sn, skill_table[sn]->slot, skill_table[sn]->name, spell_damage[SPELL_DAMAGE( skill_table[sn] )] );
            num++;
         }
      pager_printf( ch, "%d matches found.\n\r", num );
   }
   else if( !str_cmp( argument, "tongues" ) )
   {
      int num = 0;

      for( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ )
         if( skill_table[sn]->type == SKILL_TONGUE )
         {
            pager_printf( ch, "Sn: %3d Slot: %3d Name: '%-24s'\n\r", sn, skill_table[sn]->slot, skill_table[sn]->name );
            num++;
         }
      pager_printf( ch, "%d matches found.\n\r", num );
   }
   else if( !str_cmp( argument, "smaug" ) )
   {
      int num = 0;

      for( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ )
         if( skill_table[sn]->spell_fun == spell_smaug )
         {
            pager_printf( ch, "Sn: %3d Slot: %3d Name: '%-24s' Damtype: %s\n\r",
                          sn, skill_table[sn]->slot, skill_table[sn]->name, spell_damage[SPELL_DAMAGE( skill_table[sn] )] );
            num++;
         }
      pager_printf( ch, "%d matches found.\n\r", num );
   }
   else if( !str_cmp( argument, "herbs" ) )
   {
      for( sn = 0; sn < top_herb && herb_table[sn] && herb_table[sn]->name; sn++ )
         pager_printf( ch, "%d) %s\n\r", sn, herb_table[sn]->name );
   }
   else
   {
      SMAUG_AFF *aff;
      int cnt = 0;

      if( argument[0] == 'h' && is_number( argument + 1 ) )
      {
         sn = atoi( argument + 1 );
         if( !IS_VALID_HERB( sn ) )
         {
            send_to_char( "Invalid herb.\n\r", ch );
            return;
         }
         skill = herb_table[sn];
      }
      else if( is_number( argument ) )
      {
         sn = atoi( argument );
         if( ( skill = get_skilltype( sn ) ) == NULL )
         {
            send_to_char( "Invalid sn.\n\r", ch );
            return;
         }
         sn %= 1000;
      }
      else if( ( sn = skill_lookup( argument ) ) >= 0 )
         skill = skill_table[sn];
      else if( ( sn = herb_lookup( argument ) ) >= 0 )
         skill = herb_table[sn];
      else
      {
         send_to_char( "No such skill, spell, proficiency or tongue.\n\r", ch );
         return;
      }
      if( !skill )
      {
         send_to_char( "Not created yet.\n\r", ch );
         return;
      }

      ch_printf( ch, "Sn: %4d Slot: %4d %s: '%-20s'\n\r", sn, skill->slot, skill_tname[skill->type], skill->name );
      if( skill->author )
         ch_printf( ch, "Author: %s\n\r", skill->author );
      if( skill->info )
         ch_printf( ch, "DamType: %s  ActType: %s   ClassType: %s   PowerType: %s\n\r",
                    spell_damage[SPELL_DAMAGE( skill )], spell_action[SPELL_ACTION( skill )],
                    spell_class[SPELL_CLASS( skill )], spell_power[SPELL_POWER( skill )] );
      if( skill->flags )
      {
         int x;

         mudstrlcpy( buf, "Flags:", MSL );
         for( x = 0; x < 32; ++x )
            if( SPELL_FLAG( skill, 1 << x ) )
            {
               mudstrlcat( buf, " ", MSL );
               mudstrlcat( buf, spell_flag[x], MSL );
            }
         mudstrlcat( buf, "\n\r", MSL );
         send_to_char( buf, ch );
      }
      ch_printf( ch, "Saves: %s  SaveEffect: %s\n\r",
                 spell_saves[( int )skill->saves], spell_save_effect[SPELL_SAVE( skill )] );

      if( skill->difficulty != '\0' )
         ch_printf( ch, "Difficulty: %d\n\r", ( int )skill->difficulty );

      ch_printf( ch, "Type: %s  Target: %s  Minpos: %s  Mana: %d  Beats: %d  Range: %d\n\r",
                 skill_tname[skill->type], target_type[URANGE( TAR_IGNORE, skill->target, TAR_OBJ_INV )],
                 npc_position[skill->minimum_position], skill->min_mana, skill->beats, skill->range );

      ch_printf( ch, "Flags: %d  Guild: %d  Value: %d  Info: %d\n\r",
                 skill->flags, skill->guild, skill->value, skill->info );

      ch_printf( ch, "Rent: %d  Code: %s\n\r", skill->rent,
                 skill->skill_fun ? skill->skill_fun_name : skill->spell_fun_name );

      ch_printf( ch, "Dammsg: %s\n\rWearoff: %s\n", skill->noun_damage, skill->msg_off ? skill->msg_off : "(none set)" );

      if( skill->dice && skill->dice[0] != '\0' )
         ch_printf( ch, "Dice: %s\n\r", skill->dice );

      if( skill->teachers && skill->teachers[0] != '\0' )
         ch_printf( ch, "Teachers: %s\n\r", skill->teachers );

      if( skill->components && skill->components[0] != '\0' )
         ch_printf( ch, "Components: %s\n\r", skill->components );

      if( skill->participants )
         ch_printf( ch, "Participants: %d\n\r", ( int )skill->participants );

      for( aff = skill->affects; aff; aff = aff->next )
      {
         if( aff == skill->affects )
            send_to_char( "\n\r", ch );

         snprintf( buf, MSL, "Affect %d", ++cnt );
         if( aff->location )
         {
            mudstrlcat( buf, " modifies ", MSL );
            mudstrlcat( buf, a_types[aff->location % REVERSE_APPLY], MSL );
            mudstrlcat( buf, " by '", MSL );
            mudstrlcat( buf, aff->modifier, MSL );
            if( aff->bitvector != -1 )
               mudstrlcat( buf, "' and", MSL );
            else
               mudstrlcat( buf, "'", MSL );
         }
         if( aff->bitvector != -1 )
         {
            mudstrlcat( buf, " applies ", MSL );
            mudstrlcat( buf, a_flags[aff->bitvector], MSL );
         }
         if( aff->duration[0] != '\0' && aff->duration[0] != '0' )
         {
            mudstrlcat( buf, " for '", MSL );
            mudstrlcat( buf, aff->duration, MSL );
            mudstrlcat( buf, "' rounds", MSL );
         }
         if( aff->location >= REVERSE_APPLY )
            mudstrlcat( buf, " (affects caster only)", MSL );
         mudstrlcat( buf, "\n\r", MSL );
         send_to_char( buf, ch );
         if( !aff->next )
            send_to_char( "\n\r", ch );
      }
      if( skill->hit_char && skill->hit_char[0] != '\0' )
         ch_printf( ch, "Hitchar   : %s\n\r", skill->hit_char );

      if( skill->hit_vict && skill->hit_vict[0] != '\0' )
         ch_printf( ch, "Hitvict   : %s\n\r", skill->hit_vict );

      if( skill->hit_room && skill->hit_room[0] != '\0' )
         ch_printf( ch, "Hitroom   : %s\n\r", skill->hit_room );

      if( skill->hit_dest && skill->hit_dest[0] != '\0' )
         ch_printf( ch, "Hitdest   : %s\n\r", skill->hit_dest );

      if( skill->miss_char && skill->miss_char[0] != '\0' )
         ch_printf( ch, "Misschar  : %s\n\r", skill->miss_char );

      if( skill->miss_vict && skill->miss_vict[0] != '\0' )
         ch_printf( ch, "Missvict  : %s\n\r", skill->miss_vict );

      if( skill->miss_room && skill->miss_room[0] != '\0' )
         ch_printf( ch, "Missroom  : %s\n\r", skill->miss_room );

      if( skill->die_char && skill->die_char[0] != '\0' )
         ch_printf( ch, "Diechar   : %s\n\r", skill->die_char );

      if( skill->die_vict && skill->die_vict[0] != '\0' )
         ch_printf( ch, "Dievict   : %s\n\r", skill->die_vict );

      if( skill->die_room && skill->die_room[0] != '\0' )
         ch_printf( ch, "Dieroom   : %s\n\r", skill->die_room );

      if( skill->imm_char && skill->imm_char[0] != '\0' )
         ch_printf( ch, "Immchar   : %s\n\r", skill->imm_char );

      if( skill->imm_vict && skill->imm_vict[0] != '\0' )
         ch_printf( ch, "Immvict   : %s\n\r", skill->imm_vict );

      if( skill->imm_room && skill->imm_room[0] != '\0' )
         ch_printf( ch, "Immroom   : %s\n\r", skill->imm_room );

      if( skill->type != SKILL_HERB )
      {
         if( skill->type != SKILL_RACIAL )
         {
            send_to_char( "--------------------------[CLASS USE]--------------------------\n\r", ch );
            for( iClass = 0; iClass < MAX_PC_CLASS; iClass++ )
            {
               mudstrlcpy( buf, class_table[iClass]->who_name, MSL );
               snprintf( buf + 3, MSL - 3, " ) lvl: %3d max: %2d%%", skill->skill_level[iClass],
                         skill->skill_adept[iClass] );
               if( iClass % 3 == 2 )
                  mudstrlcat( buf, "\n\r", MSL );
               else
                  mudstrlcat( buf, "  ", MSL );
               send_to_char( buf, ch );
            }
         }
         else
         {
            send_to_char( "\n\r--------------------------[RACE USE]--------------------------\n\r", ch );
            for( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
            {
               snprintf( buf, MSL, "%8.8s ) lvl: %3d max: %2d%%",
                         race_table[iRace]->race_name, skill->race_level[iRace], skill->race_adept[iRace] );
               if( !str_cmp( race_table[iRace]->race_name, "unused" ) )
                  mudstrlcpy( buf, "                           ", MSL );
               if( ( iRace > 0 ) && ( iRace % 2 == 1 ) )
                  mudstrlcat( buf, "\n\r", MSL );
               else
                  mudstrlcat( buf, "  ", MSL );
               send_to_char( buf, ch );
            }
         }
      }
      send_to_char( "\n\r", ch );
   }
   return;
}

/*
 * Set a skill's attributes or what skills a player has.
 * High god command, with support for creating skills/spells/herbs/etc
 */
CMDF do_sset( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim;
   int value, sn, i;
   bool fAll;

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || arg2[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Syntax: sset <victim> <skill> <value>\n\r", ch );
      send_to_char( "or:     sset <victim> all     <value>\n\r", ch );
      if( IS_IMP( ch ) )
      {
         send_to_char( "or:     sset save skill table\n\r", ch );
         send_to_char( "or:     sset save herb table\n\r", ch );
         send_to_char( "or:     sset create skill 'new skill'\n\r", ch );
         send_to_char( "or:     sset create herb 'new herb'\n\r", ch );
         send_to_char( "or:     sset create ability 'new ability'\n\r", ch );
         send_to_char( "or:     sset create lore 'new lore'\n\r", ch );
      }
      if( ch->level > LEVEL_GREATER )
      {
         send_to_char( "or:     sset <sn>     <field> <value>\n\r", ch );
         send_to_char( "\n\rField being one of:\n\r", ch );
         send_to_char( "  name code target minpos slot mana beats dammsg wearoff guild minlevel\n\r", ch );
         send_to_char( "  type damtype acttype classtype powertype seffect flags dice value difficulty\n\r", ch );
         send_to_char( "  affect rmaffect level adept hit miss die imm (char/vict/room)\n\r", ch );
         send_to_char( "  components teachers racelevel raceadept rent\n\r", ch );
         send_to_char( "Affect having the fields: <location> <modfifier> [duration] [bitvector]\n\r", ch );
         send_to_char( "(See AFFECTTYPES for location, and AFFECTED_BY for bitvector)\n\r", ch );
      }
      send_to_char( "Skill being any skill or spell.\n\r", ch );
      return;
   }

   if( IS_IMP( ch ) && !str_cmp( arg1, "save" ) && !str_cmp( argument, "table" ) )
   {
      if( !str_cmp( arg2, "skill" ) )
      {
         send_to_char( "Saving skill table...\n\r", ch );
         save_skill_table(  );
         save_classes(  );
         save_races(  );
         return;
      }
      if( !str_cmp( arg2, "herb" ) )
      {
         send_to_char( "Saving herb table...\n\r", ch );
         save_herb_table(  );
         return;
      }
   }
   if( IS_IMP( ch ) && !str_cmp( arg1, "create" )
       && ( !str_cmp( arg2, "skill" ) || !str_cmp( arg2, "herb" ) || !str_cmp( arg2, "ability" ) ) )
   {
      struct skill_type *skill;
      short type = SKILL_UNKNOWN;

      if( !str_cmp( arg2, "herb" ) )
      {
         type = SKILL_HERB;
         if( top_herb >= MAX_HERB )
         {
            ch_printf( ch, "The current top herb is %d, which is the maximum. "
                       "To add more herbs,\n\rMAX_HERB will have to be raised in mud.h, and the mud recompiled.\n\r",
                       top_herb );
            return;
         }
      }
      else if( top_sn >= MAX_SKILL )
      {
         ch_printf( ch, "The current top sn is %d, which is the maximum. "
                    "To add more skills,\n\rMAX_SKILL will have to be raised in mud.h, and the mud recompiled.\n\r",
                    top_sn );
         return;
      }
      CREATE( skill, struct skill_type, 1 );
      skill->slot = 0;
      if( type == SKILL_HERB )
      {
         int max, x;

         herb_table[top_herb++] = skill;
         for( max = x = 0; x < top_herb - 1; x++ )
            if( herb_table[x] && herb_table[x]->slot > max )
               max = herb_table[x]->slot;
         skill->slot = max + 1;
      }
      else
         skill_table[top_sn++] = skill;
      skill->min_mana = 0;
      skill->name = str_dup( argument );
      skill->spell_fun = spell_smaug;
      skill->guild = -1;
      skill->type = type;
      skill->author = QUICKLINK( ch->name );
      if( !str_cmp( arg2, "ability" ) )
         skill->type = SKILL_RACIAL;
      if( !str_cmp( arg2, "lore" ) )
         skill->type = SKILL_LORE;

      for( i = 0; i < MAX_PC_CLASS; i++ )
      {
         skill->skill_level[i] = LEVEL_IMMORTAL;
         skill->skill_adept[i] = 95;
      }
      for( i = 0; i < MAX_PC_RACE; i++ )
      {
         skill->race_level[i] = LEVEL_IMMORTAL;
         skill->race_adept[i] = 95;
      }

      send_to_char( "Done.\n\r", ch );
      return;
   }

   if( arg1[0] == 'h' )
      sn = atoi( arg1 + 1 );
   else
      sn = atoi( arg1 );
   if( get_trust( ch ) > LEVEL_GREATER
       && ( ( arg1[0] == 'h' && is_number( arg1 + 1 ) && ( sn = atoi( arg1 + 1 ) ) >= 0 )
            || ( is_number( arg1 ) && ( sn = atoi( arg1 ) ) >= 0 ) ) )
   {
      struct skill_type *skill;

      if( arg1[0] == 'h' )
      {
         if( sn >= top_herb )
         {
            send_to_char( "Herb number out of range.\n\r", ch );
            return;
         }
         skill = herb_table[sn];
      }
      else
      {
         if( ( skill = get_skilltype( sn ) ) == NULL )
         {
            send_to_char( "Skill number out of range.\n\r", ch );
            return;
         }
         sn %= 1000;
      }

      if( !str_cmp( arg2, "difficulty" ) )
      {
         skill->difficulty = atoi( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "rent" ) )
      {
         skill->rent = atoi( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "participants" ) )
      {
         skill->participants = atoi( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "damtype" ) )
      {
         int x = get_sdamage( argument );

         if( x == -1 )
            send_to_char( "Not a spell damage type.\n\r", ch );
         else
         {
            SET_SDAM( skill, x );
            send_to_char( "Ok.\n\r", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "acttype" ) )
      {
         int x = get_saction( argument );

         if( x == -1 )
            send_to_char( "Not a spell action type.\n\r", ch );
         else
         {
            SET_SACT( skill, x );
            send_to_char( "Ok.\n\r", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "classtype" ) )
      {
         int x = get_sclass( argument );

         if( x == -1 )
            send_to_char( "Not a spell Class type.\n\r", ch );
         else
         {
            SET_SCLA( skill, x );
            send_to_char( "Ok.\n\r", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "powertype" ) )
      {
         int x = get_spower( argument );

         if( x == -1 )
            send_to_char( "Not a spell power type.\n\r", ch );
         else
         {
            SET_SPOW( skill, x );
            send_to_char( "Ok.\n\r", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "seffect" ) )
      {
         int x = get_ssave_effect( argument );

         if( x == -1 )
            send_to_char( "Not a spell save effect type.\n\r", ch );
         else
         {
            SET_SSAV( skill, x );
            send_to_char( "Ok.\n\r", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "flags" ) )
      {
         char arg3[MIL];
         int x;

         while( argument[0] != '\0' )
         {
            argument = one_argument( argument, arg3 );
            x = get_sflag( arg3 );
            if( x < 0 || x > 31 )
               ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
            else
               TOGGLE_BIT( skill->flags, 1 << x );
         }

         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "saves" ) )
      {
         int x = get_ssave( argument );

         if( x == -1 )
            send_to_char( "Not a saving type.\n\r", ch );
         else
         {
            skill->saves = x;
            send_to_char( "Ok.\n\r", ch );
         }
         return;
      }

      if( !str_cmp( arg2, "code" ) )
      {
         DO_FUN *skillfun;
         SPELL_FUN *spellfun;

         if( validate_spec_fun( argument ) )
         {
            send_to_char( "Cannot use a spec_fun for skills or spells.\n\r", ch );
            return;
         }
         else if( !str_prefix( "do_", argument ) && ( skillfun = skill_function( argument ) ) != skill_notfound )
         {
            skill->skill_fun = skillfun;
            skill->spell_fun = NULL;
            DISPOSE( skill->skill_fun_name );
            skill->skill_fun_name = str_dup( argument );
         }
         else if( ( spellfun = spell_function( argument ) ) != spell_notfound )
         {
            skill->spell_fun = spellfun;
            skill->skill_fun = NULL;
            DISPOSE( skill->skill_fun_name );
            skill->spell_fun_name = str_dup( argument );
         }
         else
         {
            send_to_char( "Not a spell or skill.\n\r", ch );
            return;
         }
         send_to_char( "Ok.\n\r", ch );
         return;
      }

      if( !str_cmp( arg2, "target" ) )
      {
         int x = get_starget( argument );

         if( x == -1 )
            send_to_char( "Not a valid target type.\n\r", ch );
         else
         {
            skill->target = x;
            send_to_char( "Ok.\n\r", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "minpos" ) )
      {
         int pos;

         pos = get_npc_position( argument );

         if( pos < 0 || pos > POS_MAX )
         {
            send_to_char( "Invalid position.\n\r", ch );
            return;
         }

         skill->minimum_position = pos;
         send_to_char( "Skill minposition set.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "minlevel" ) )
      {
         skill->min_level = URANGE( 1, atoi( argument ), MAX_LEVEL );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "slot" ) )
      {
         skill->slot = URANGE( 0, atoi( argument ), 30000 );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "mana" ) )
      {
         skill->min_mana = URANGE( 0, atoi( argument ), 2000 );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "beats" ) )
      {
         skill->beats = URANGE( 0, atoi( argument ), 120 );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "range" ) )
      {
         skill->range = URANGE( 0, atoi( argument ), 20 );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "guild" ) )
      {
         skill->guild = atoi( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "value" ) )
      {
         skill->value = atoi( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "type" ) )
      {
         skill->type = get_skill( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "rmaffect" ) )
      {
         SMAUG_AFF *aff = skill->affects;
         SMAUG_AFF *aff_next;
         int num = atoi( argument );
         int cnt = 1;

         if( !aff )
         {
            send_to_char( "This spell has no special affects to remove.\n\r", ch );
            return;
         }
         if( num == 1 )
         {
            skill->affects = aff->next;
            DISPOSE( aff->duration );
            DISPOSE( aff->modifier );
            DISPOSE( aff );
            send_to_char( "Removed.\n\r", ch );
            return;
         }
         for( ; aff; aff = aff->next )
         {
            if( ++cnt == num && ( aff_next = aff->next ) != NULL )
            {
               aff->next = aff_next->next;
               DISPOSE( aff_next->duration );
               DISPOSE( aff_next->modifier );
               DISPOSE( aff_next );
               send_to_char( "Removed.\n\r", ch );
               return;
            }
         }
         send_to_char( "Not found.\n\r", ch );
         return;
      }
      /*
       * affect <location> <modifier> <duration> <bitvector>
       */
      if( !str_cmp( arg2, "affect" ) )
      {
         char location[MIL];
         char modifier[MIL];
         char duration[MIL];
/*	    char bitvector[MIL];	*/
         int loc, bit, tmpbit;
         SMAUG_AFF *aff;

         argument = one_argument( argument, location );
         argument = one_argument( argument, modifier );
         argument = one_argument( argument, duration );

         if( location[0] == '!' )
            loc = get_atype( location + 1 ) + REVERSE_APPLY;
         else
            loc = get_atype( location );
         if( ( loc % REVERSE_APPLY ) < 0 || ( loc % REVERSE_APPLY ) >= MAX_APPLY_TYPE )
         {
            send_to_char( "Unknown affect location.  See AFFECTTYPES.\n\r", ch );
            return;
         }
         bit = -1;
         if( argument[0] != '\0' )
         {
            if( ( tmpbit = get_aflag( argument ) ) == -1 )
               ch_printf( ch, "Unknown bitvector: %s.  See AFFECTED_BY\n\r", argument );
            else
               bit = tmpbit;
         }
         CREATE( aff, SMAUG_AFF, 1 );
         if( !str_cmp( duration, "0" ) )
            duration[0] = '\0';
         if( !str_cmp( modifier, "0" ) )
            modifier[0] = '\0';
         aff->duration = str_dup( duration );
         aff->location = loc;
         if( loc == APPLY_AFFECT || loc == APPLY_EXT_AFFECT )
         {
            int modval = get_aflag( modifier );

            /* Sanitize the flag input for the modifier if needed -- Samson */
            if( modval < 0 )
               modval = 0;
            /* Spells/skills affect people, yes? People can only have EXT_BV affects, yes? */
            if( loc == APPLY_AFFECT )
               aff->location = APPLY_EXT_AFFECT;
            mudstrlcpy( modifier, a_flags[modval], MIL );
         }
         if( loc == APPLY_RESISTANT || loc == APPLY_IMMUNE || loc == APPLY_ABSORB || loc == APPLY_SUSCEPTIBLE )
         {
            int modval = get_risflag( modifier );

            /* Sanitize the flag input for the modifier if needed -- Samson */
            if( modval < 0 )
               modval = 0;
            mudstrlcpy( modifier, ris_flags[modval], MIL );
         }
         aff->modifier = str_dup( modifier );
         aff->bitvector = bit;
         aff->next = skill->affects;
         skill->affects = aff;
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      /*
       * Modified to read Class by name - Samson 9-24-98 
       */
      if( !str_cmp( arg2, "level" ) )
      {
         char arg3[MIL];
         int Class;

         argument = one_argument( argument, arg3 );
         Class = get_pc_class( arg3 );

         if( Class >= MAX_PC_CLASS || Class < 0 )
            ch_printf( ch, "%s is not a valid Class.\n\r", arg3 );
         else
            skill->skill_level[Class] = URANGE( 0, atoi( argument ), MAX_LEVEL );
         return;
      }
      /*
       * Modified to read race by name - Samson 9-24-98 
       */
      if( !str_cmp( arg2, "racelevel" ) )
      {
         char arg3[MIL];
         int race;

         argument = one_argument( argument, arg3 );
         race = get_pc_race( arg3 );

         if( race >= MAX_PC_RACE || race < 0 )
            ch_printf( ch, "%s is not a valid race.\n\r", arg3 );
         else
            skill->race_level[race] = URANGE( 0, atoi( argument ), MAX_LEVEL );
         return;
      }
      /*
       * Modified to read Class by name - Samson 9-24-98 
       */
      if( !str_cmp( arg2, "adept" ) )
      {
         char arg3[MIL];
         int Class;

         argument = one_argument( argument, arg3 );
         Class = get_pc_class( arg3 );

         if( Class >= MAX_PC_CLASS || Class < 0 )
            ch_printf( ch, "%s is not a valid Class.\n\r", arg3 );
         else
            skill->skill_adept[Class] = URANGE( 0, atoi( argument ), 100 );
         return;
      }
      /*
       * Modified to read race by name - Samson 9-24-98 
       */
      if( !str_cmp( arg2, "raceadept" ) )
      {
         char arg3[MIL];
         int race;

         argument = one_argument( argument, arg3 );
         race = get_pc_race( arg3 );

         if( race >= MAX_PC_RACE || race < 0 )
            ch_printf( ch, "%s is not a valid race.\n\r", arg3 );
         else
            skill->race_adept[race] = URANGE( 0, atoi( argument ), 100 );
         return;
      }

      if( !str_cmp( arg2, "name" ) )
      {
         DISPOSE( skill->name );
         skill->name = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "author" ) )
      {
         if( !IS_IMP( ch ) )
         {
            send_to_char( "Sorry, only admins can change this.\n\r", ch );
            return;
         }
         STRFREE( skill->author );
         skill->author = STRALLOC( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "dammsg" ) )
      {
         DISPOSE( skill->noun_damage );
         if( str_cmp( argument, "clear" ) )
            skill->noun_damage = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "wearoff" ) )
      {
         DISPOSE( skill->msg_off );
         if( str_cmp( argument, "clear" ) )
            skill->msg_off = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "hitchar" ) )
      {
         DISPOSE( skill->hit_char );
         if( str_cmp( argument, "clear" ) )
            skill->hit_char = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "hitvict" ) )
      {
         DISPOSE( skill->hit_vict );
         if( str_cmp( argument, "clear" ) )
            skill->hit_vict = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "hitroom" ) )
      {
         DISPOSE( skill->hit_room );
         if( str_cmp( argument, "clear" ) )
            skill->hit_room = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "hitdest" ) )
      {
         DISPOSE( skill->hit_dest );
         if( str_cmp( argument, "clear" ) )
            skill->hit_dest = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "misschar" ) )
      {
         DISPOSE( skill->miss_char );
         if( str_cmp( argument, "clear" ) )
            skill->miss_char = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "missvict" ) )
      {
         DISPOSE( skill->miss_vict );
         if( str_cmp( argument, "clear" ) )
            skill->miss_vict = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "missroom" ) )
      {
         DISPOSE( skill->miss_room );
         if( str_cmp( argument, "clear" ) )
            skill->miss_room = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "diechar" ) )
      {
         DISPOSE( skill->die_char );
         if( str_cmp( argument, "clear" ) )
            skill->die_char = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "dievict" ) )
      {
         DISPOSE( skill->die_vict );
         if( str_cmp( argument, "clear" ) )
            skill->die_vict = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "dieroom" ) )
      {
         DISPOSE( skill->die_room );
         if( str_cmp( argument, "clear" ) )
            skill->die_room = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "immchar" ) )
      {
         DISPOSE( skill->imm_char );
         if( str_cmp( argument, "clear" ) )
            skill->imm_char = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "immvict" ) )
      {
         DISPOSE( skill->imm_vict );
         if( str_cmp( argument, "clear" ) )
            skill->imm_vict = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "immroom" ) )
      {
         DISPOSE( skill->imm_room );
         if( str_cmp( argument, "clear" ) )
            skill->imm_room = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "dice" ) )
      {
         DISPOSE( skill->dice );
         if( str_cmp( argument, "clear" ) )
            skill->dice = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "components" ) )
      {
         DISPOSE( skill->components );
         if( str_cmp( argument, "clear" ) )
            skill->components = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      if( !str_cmp( arg2, "teachers" ) )
      {
         DISPOSE( skill->teachers );
         if( str_cmp( argument, "clear" ) )
            skill->teachers = str_dup( argument );
         send_to_char( "Ok.\n\r", ch );
         return;
      }
      do_sset( ch, "" );
      return;
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      if( ( sn = skill_lookup( arg1 ) ) >= 0 )
         funcf( ch, do_sset, "%d %s %s", sn, arg2, argument );
      else
         send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   fAll = !str_cmp( arg2, "all" );
   sn = 0;
   if( !fAll && ( sn = skill_lookup( arg2 ) ) < 0 )
   {
      send_to_char( "No such skill or spell.\n\r", ch );
      return;
   }

   /*
    * Snarf the value.
    */
   if( !is_number( argument ) )
   {
      send_to_char( "Value must be numeric.\n\r", ch );
      return;
   }

   value = atoi( argument );
   if( value < 0 || value > 100 )
   {
      send_to_char( "Value range is 0 to 100.\n\r", ch );
      return;
   }

   if( fAll )
   {
      for( sn = 0; sn < top_sn; sn++ )
      {
         /*
          * Fix by Narn to prevent ssetting skills the player shouldn't have. 
          */
         if( victim->level >= skill_table[sn]->skill_level[victim->Class]
             || victim->level >= skill_table[sn]->race_level[victim->race] )
         {
            if( value == 100 && !IS_IMMORTAL( victim ) )
               victim->pcdata->learned[sn] = GET_ADEPT( victim, sn );
            else
               victim->pcdata->learned[sn] = value;
         }
      }
   }
   else
      victim->pcdata->learned[sn] = value;

   return;
}

CMDF do_gouge( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   short dam;
   int schance;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   if( !can_use_skill( ch, 0, gsn_gouge ) )
   {
      send_to_char( "You do not yet know of this skill.\n\r", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't get close enough while mounted.\n\r", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\n\r", ch );
      return;
   }

   schance = ( ( get_curr_dex( victim ) - get_curr_dex( ch ) ) * 10 ) + 10;
   if( !IS_NPC( ch ) && !IS_NPC( victim ) )
      schance += sysdata.gouge_plr_vs_plr;
   if( victim->fighting && victim->fighting->who != ch )
      schance += sysdata.gouge_nontank;
   if( can_use_skill( ch, ( number_percent(  ) + schance ), gsn_gouge ) )
   {
      dam = number_range( 5, ch->level );
      global_retcode = damage( ch, victim, dam, gsn_gouge );
      if( global_retcode == rNONE )
      {
         if( !IS_AFFECTED( victim, AFF_BLIND ) )
         {
            af.type = gsn_blindness;
            af.location = APPLY_HITROLL;
            af.modifier = -6;
            if( !IS_NPC( victim ) && !IS_NPC( ch ) )
               af.duration = ( ch->level + 10 ) / get_curr_con( victim );
            else
               af.duration = 3 + ( ch->level / 15 );
            af.bit = AFF_BLIND;
            affect_to_char( victim, &af );
            act( AT_SKILL, "You can't see a thing!", victim, NULL, NULL, TO_CHAR );
         }
         WAIT_STATE( ch, sysdata.pulseviolence );
         if( !IS_NPC( ch ) && !IS_NPC( victim ) )
         {
            if( number_bits( 1 ) == 0 )
            {
               ch_printf( ch, "%s looks momentarily dazed.\n\r", victim->name );
               send_to_char( "You are momentarily dazed ...\n\r", victim );
               WAIT_STATE( victim, sysdata.pulseviolence );
            }
         }
         else
            WAIT_STATE( victim, sysdata.pulseviolence );
         /*
          * Taken out by request - put back in by Thoric
          * * This is how it was designed.  You'd be a tad stunned
          * * if someone gouged you in the eye.
          * * Mildly modified by Blodkai, Feb 1998 at request of
          * * of pkill Conclave (peaceful use remains the same)
          */
      }
      else if( global_retcode == rVICT_DIED )
      {
         act( AT_BLOOD, "Your fingers plunge into your victim's brain, causing immediate death!", ch, NULL, NULL, TO_CHAR );
      }
   }
   else
   {
      WAIT_STATE( ch, skill_table[gsn_gouge]->beats );
      global_retcode = damage( ch, victim, 0, gsn_gouge );
      learn_from_failure( ch, gsn_gouge );
   }

   return;
}

CMDF do_detrap( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   OBJ_DATA *obj;
   OBJ_DATA *trap;
   int percent;
   bool found = FALSE;

   switch ( ch->substate )
   {
      default:
         if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\n\r", ch );
            return;
         }
         argument = one_argument( argument, arg );
         if( !can_use_skill( ch, 0, gsn_detrap ) )
         {
            send_to_char( "You do not yet know of this skill.\n\r", ch );
            return;
         }
         if( arg[0] == '\0' )
         {
            send_to_char( "Detrap what?\n\r", ch );
            return;
         }
         if( ms_find_obj( ch ) )
            return;
         found = FALSE;
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\n\r", ch );
            return;
         }
         if( !ch->in_room->first_content )
         {
            send_to_char( "You can't find that here.\n\r", ch );
            return;
         }
         for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
         {
            if( can_see_obj( ch, obj, FALSE ) && nifty_is_name( arg, obj->name ) )
            {
               found = TRUE;
               break;
            }
         }
         if( !found )
         {
            send_to_char( "You can't find that here.\n\r", ch );
            return;
         }
         act( AT_ACTION, "You carefully begin your attempt to remove a trap from $p...", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n carefully attempts to remove a trap from $p...", ch, obj, NULL, TO_ROOM );
         ch->alloc_ptr = str_dup( obj->name );
         add_timer( ch, TIMER_DO_FUN, 3, do_detrap, 1 );
/*	    WAIT_STATE( ch, skill_table[gsn_detrap]->beats ); */
         return;
      case 1:
         if( !ch->alloc_ptr )
         {
            send_to_char( "Your detrapping was interrupted!\n\r", ch );
            bug( "%s", "do_detrap: ch->alloc_ptr NULL!" );
            return;
         }
         mudstrlcpy( arg, ch->alloc_ptr, MIL );
         DISPOSE( ch->alloc_ptr );
         ch->alloc_ptr = NULL;
         ch->substate = SUB_NONE;
         break;
      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         send_to_char( "You carefully stop what you were doing.\n\r", ch );
         return;
   }

   if( !ch->in_room->first_content )
   {
      send_to_char( "You can't find that here.\n\r", ch );
      return;
   }
   for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
   {
      if( can_see_obj( ch, obj, FALSE ) && nifty_is_name( arg, obj->name ) )
      {
         found = TRUE;
         break;
      }
   }
   if( !found )
   {
      send_to_char( "You can't find that here.\n\r", ch );
      return;
   }
   if( ( trap = get_trap( obj ) ) == NULL )
   {
      send_to_char( "You find no trap on that.\n\r", ch );
      return;
   }

   percent = number_percent(  ) - ( ch->level / 15 ) - ( get_curr_lck( ch ) - 16 );

   separate_obj( obj );
   if( !can_use_skill( ch, percent, gsn_detrap ) )
   {
      send_to_char( "Ooops!\n\r", ch );
      spring_trap( ch, trap );
      learn_from_failure( ch, gsn_detrap );
      return;
   }

   extract_obj( trap );

   send_to_char( "You successfully remove a trap.\n\r", ch );
   return;
}

CMDF do_dig( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   OBJ_DATA *obj;
   OBJ_DATA *startobj;
   bool found, shovel;
   EXIT_DATA *pexit;

   switch ( ch->substate )
   {
      default:
         if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\n\r", ch );
            return;
         }
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\n\r", ch );
            return;
         }
         one_argument( argument, arg );
         if( arg[0] != '\0' )
         {
            if( ( pexit = find_door( ch, arg, TRUE ) ) == NULL && get_dir( arg ) == -1 )
            {
               send_to_char( "What direction is that?\n\r", ch );
               return;
            }
            if( pexit )
            {
               if( !IS_EXIT_FLAG( pexit, EX_DIG ) && !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
               {
                  send_to_char( "There is no need to dig out that exit.\n\r", ch );
                  return;
               }
            }
         }
         else
         {
            int sector;

            if( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_ACT_FLAG( ch, ACT_ONMAP ) )
               sector = map_sector[ch->map][ch->x][ch->y];
            else
               sector = ch->in_room->sector_type;

            switch ( sector )
            {
               case SECT_CITY:
                  send_to_char( "The road is too hard to dig through.\n\r", ch );
                  return;
               case SECT_INDOORS:
                  send_to_char( "The floor is too hard to dig through.\n\r", ch );
                  return;
               case SECT_WATER_SWIM:
               case SECT_WATER_NOSWIM:
               case SECT_UNDERWATER:
                  send_to_char( "You cannot dig here.\n\r", ch );
                  return;
               case SECT_AIR:
                  send_to_char( "What?  In the air?!\n\r", ch );
                  return;
            }
         }
         add_timer( ch, TIMER_DO_FUN, UMIN( skill_table[gsn_dig]->beats / 10, 3 ), do_dig, 1 );
         ch->alloc_ptr = str_dup( arg );
         send_to_char( "You begin digging...\n\r", ch );
         act( AT_PLAIN, "$n begins digging...", ch, NULL, NULL, TO_ROOM );
         return;

      case 1:
         if( !ch->alloc_ptr )
         {
            send_to_char( "Your digging was interrupted!\n\r", ch );
            act( AT_PLAIN, "$n's digging was interrupted!", ch, NULL, NULL, TO_ROOM );
            bug( "%s", "do_dig: alloc_ptr NULL" );
            return;
         }
         mudstrlcpy( arg, ch->alloc_ptr, MIL );
         DISPOSE( ch->alloc_ptr );
         break;

      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         send_to_char( "You stop digging...\n\r", ch );
         act( AT_PLAIN, "$n stops digging...", ch, NULL, NULL, TO_ROOM );
         return;
   }

   ch->substate = SUB_NONE;

   /*
    * not having a shovel makes it harder to succeed 
    */
   shovel = FALSE;
   for( obj = ch->first_carrying; obj; obj = obj->next_content )
      if( obj->item_type == ITEM_SHOVEL )
      {
         shovel = TRUE;
         break;
      }

   /*
    * dig out an EX_DIG exit... 
    */
   if( arg[0] != '\0' )
   {
      if( ( pexit = find_door( ch, arg, TRUE ) ) != NULL
          && IS_EXIT_FLAG( pexit, EX_DIG ) && IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         /*
          * 4 times harder to dig open a passage without a shovel 
          */
         if( can_use_skill( ch, ( number_percent(  ) * ( shovel ? 1 : 4 ) ), gsn_dig ) )
         {
            REMOVE_EXIT_FLAG( pexit, EX_CLOSED );
            send_to_char( "You dig open a passageway!\n\r", ch );
            act( AT_PLAIN, "$n digs open a passageway!", ch, NULL, NULL, TO_ROOM );
            return;
         }
      }
      learn_from_failure( ch, gsn_dig );
      send_to_char( "Your dig did not discover any exit...\n\r", ch );
      act( AT_PLAIN, "$n's dig did not discover any exit...", ch, NULL, NULL, TO_ROOM );
      return;
   }

   startobj = ch->in_room->first_content;
   found = FALSE;

   for( obj = startobj; obj; obj = obj->next_content )
   {
      /*
       * twice as hard to find something without a shovel 
       */
      if( IS_OBJ_FLAG( obj, ITEM_BURIED ) && ( can_use_skill( ch, ( number_percent(  ) * ( shovel ? 1 : 2 ) ), gsn_dig ) ) )
      {
         found = TRUE;
         break;
      }
   }

   if( !found )
   {
      send_to_char( "Your dig uncovered nothing.\n\r", ch );
      act( AT_PLAIN, "$n's dig uncovered nothing.", ch, NULL, NULL, TO_ROOM );
      learn_from_failure( ch, gsn_dig );
      return;
   }

   separate_obj( obj );
   REMOVE_OBJ_FLAG( obj, ITEM_BURIED );
   act( AT_SKILL, "Your dig uncovered $p!", ch, obj, NULL, TO_CHAR );
   act( AT_SKILL, "$n's dig uncovered $p!", ch, obj, NULL, TO_ROOM );
   if( obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC )
      adjust_favor( ch, 14, 1 );

   return;
}

CMDF do_search( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   OBJ_DATA *obj;
   OBJ_DATA *container;
   OBJ_DATA *startobj;
   int percent, door;

   door = -1;
   switch ( ch->substate )
   {
      default:
         if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\n\r", ch );
            return;
         }
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\n\r", ch );
            return;
         }
         argument = one_argument( argument, arg );
         if( arg[0] != '\0' && ( door = get_door( arg ) ) == -1 )
         {
            container = get_obj_here( ch, arg );
            if( !container )
            {
               send_to_char( "You can't find that here.\n\r", ch );
               return;
            }
            if( container->item_type != ITEM_CONTAINER )
            {
               send_to_char( "You can't search in that!\n\r", ch );
               return;
            }
            if( IS_SET( container->value[1], CONT_CLOSED ) )
            {
               send_to_char( "It is closed.\n\r", ch );
               return;
            }
         }
         add_timer( ch, TIMER_DO_FUN, UMIN( skill_table[gsn_search]->beats / 10, 3 ), do_search, 1 );
         send_to_char( "You begin your search...\n\r", ch );
         act( AT_MAGIC, "$n begins searching the room.....", ch, NULL, NULL, TO_ROOM );
         ch->alloc_ptr = str_dup( arg );
         return;

      case 1:
         if( !ch->alloc_ptr )
         {
            send_to_char( "Your search was interrupted!\n\r", ch );
            bug( "%s", "do_search: alloc_ptr NULL" );
            return;
         }
         mudstrlcpy( arg, ch->alloc_ptr, MIL );
         DISPOSE( ch->alloc_ptr );
         break;
      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         send_to_char( "You stop your search...\n\r", ch );
         return;
   }
   ch->substate = SUB_NONE;
   if( arg[0] == '\0' )
      startobj = ch->in_room->first_content;
   else
   {
      if( ( door = get_door( arg ) ) != -1 )
         startobj = NULL;
      else
      {
         container = get_obj_here( ch, arg );
         if( !container )
         {
            send_to_char( "You can't find that here.\n\r", ch );
            return;
         }
         startobj = container->first_content;
      }
   }

   if( ( !startobj && door == -1 ) || IS_NPC( ch ) )
   {
      send_to_char( "You find nothing.\n\r", ch );
      act( AT_MAGIC, "$n found nothing in $s search.", ch, NULL, NULL, TO_ROOM );
      learn_from_failure( ch, gsn_search );
      return;
   }

   percent = number_percent(  ) + number_percent(  ) - ( ch->level / 10 );

   if( door != -1 )
   {
      EXIT_DATA *pexit;

      if( ( pexit = get_exit( ch->in_room, door ) ) != NULL && IS_EXIT_FLAG( pexit, EX_SECRET )
          && IS_EXIT_FLAG( pexit, EX_xSEARCHABLE ) && can_use_skill( ch, percent, gsn_search ) )
      {
         act( AT_SKILL, "Your search reveals the $d!", ch, NULL, pexit->keyword, TO_CHAR );
         act( AT_SKILL, "$n finds the $d!", ch, NULL, pexit->keyword, TO_ROOM );
         REMOVE_EXIT_FLAG( pexit, EX_SECRET );
         return;
      }
   }
   else
      for( obj = startobj; obj; obj = obj->next_content )
      {
         if( IS_OBJ_FLAG( obj, ITEM_HIDDEN ) && can_use_skill( ch, percent, gsn_search ) )
         {
            separate_obj( obj );
            REMOVE_OBJ_FLAG( obj, ITEM_HIDDEN );
            act( AT_SKILL, "Your search reveals $p!", ch, obj, NULL, TO_CHAR );
            act( AT_SKILL, "$n finds $p!", ch, obj, NULL, TO_ROOM );
            return;
         }
      }

   send_to_char( "You find nothing.\n\r", ch );
   act( AT_MAGIC, "$n found nothing in $s search.", ch, NULL, NULL, TO_ROOM );
   learn_from_failure( ch, gsn_search );
   return;
}

CMDF do_steal( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim, *mst;
   OBJ_DATA *obj;
   int percent;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\n\r", ch );
      return;
   }

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Steal what from whom?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( victim = get_char_room( ch, arg2 ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "That's pointless.\n\r", ch );
      return;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SAFE ) )
   {
      send_to_char( "&[magic]A magical force interrupts you.\n\r", ch );
      return;
   }

   if( !IS_NPC( victim ) )
   {
      if( !CAN_PKILL( ch ) || !CAN_PKILL( victim ) )
      {
         send_to_char( "&[immortal]The gods forbid stealing from non-PK players.\n\r", ch );
         return;
      }
   }

   if( IS_ACT_FLAG( victim, ACT_PACIFIST ) ) /* Gorog */
   {
      send_to_char( "They are a pacifist - Shame on you!\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_steal]->beats );
   percent = number_percent(  ) + ( IS_AWAKE( victim ) ? 10 : -50 )
      - ( get_curr_lck( ch ) - 15 ) + ( get_curr_lck( victim ) - 13 );

   /*
    * Changed the level check, made it 10 levels instead of five and made the 
    * victim not attack in the case of a too high level difference.  This is 
    * to allow mobprogs where the mob steals eq without having to put level 
    * checks into the progs.  Also gave the mobs a 10% chance of failure.
    */
   if( ch->level + 10 < victim->level )
   {
      send_to_char( "You really don't want to try that!\n\r", ch );
      return;
   }

   if( victim->position == POS_FIGHTING || !can_use_skill( ch, percent, gsn_steal ) )
   {
      /*
       * Failure.
       */
      send_to_char( "Oops...\n\r", ch );
      act( AT_ACTION, "$n tried to steal from you!\n\r", ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$n tried to steal from $N.\n\r", ch, NULL, victim, TO_NOTVICT );

      cmdf( victim, "yell %s is a bloody thief!", ch->name );

      learn_from_failure( ch, gsn_steal );
      if( !IS_NPC( ch ) )
      {
         if( legal_loot( ch, victim ) )
            global_retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
         else
         {
            if( IS_NPC( ch ) )
            {
               if( ( mst = ch->master ) == NULL )
                  return;
            }
            else
               mst = ch;
            if( IS_NPC( mst ) )
               return;
         }
      }
      return;
   }

   if( !str_cmp( arg1, "coin" ) || !str_cmp( arg1, "coins" ) || !str_cmp( arg1, "gold" ) )
   {
      int amount;

      amount = ( int )( victim->gold * number_range( 1, 10 ) / 100 );
      if( amount <= 0 )
      {
         send_to_char( "You couldn't get any gold.\n\r", ch );
         learn_from_failure( ch, gsn_steal );
         return;
      }

      ch->gold += amount;
      victim->gold -= amount;
      ch_printf( ch, "Aha!  You got %d gold coins.\n\r", amount );
      return;
   }

   if( ( obj = get_obj_carry( victim, arg1 ) ) == NULL )
   {
      send_to_char( "You can't seem to find it.\n\r", ch );
      learn_from_failure( ch, gsn_steal );
      return;
   }

   if( IS_OBJ_FLAG( obj, ITEM_LOYAL ) )
   {
      send_to_char( "The item refuses to part with its owner!\n\r", ch );
      return;
   }

   if( !can_drop_obj( ch, obj ) || IS_OBJ_FLAG( obj, ITEM_INVENTORY )
       || IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) || item_ego( obj ) > char_ego( ch ) )
   {
      send_to_char( "You can't manage to pry it away.\n\r", ch );
      learn_from_failure( ch, gsn_steal );
      return;
   }

   if( ch->carry_number + ( get_obj_number( obj ) / obj->count ) > can_carry_n( ch ) )
   {
      send_to_char( "You have your hands full.\n\r", ch );
      learn_from_failure( ch, gsn_steal );
      return;
   }

   if( ch->carry_weight + ( get_obj_weight( obj ) / obj->count ) > can_carry_w( ch ) )
   {
      send_to_char( "You can't carry that much weight.\n\r", ch );
      learn_from_failure( ch, gsn_steal );
      return;
   }

   separate_obj( obj );
   obj_from_char( obj );
   obj_to_char( obj, ch );
   send_to_char( "Ok.\n\r", ch );
   adjust_favor( ch, 9, 1 );
   return;
}

/* Heavily modified with Shard stuff, including the critical pierce - Samson 5-22-99 */
CMDF do_backstab( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   AFFECT_DATA af;
   int percent, base;
   int awake_saw = 50, to_saw = 0, sleep_paralysis = 70, awake_paralysis = 90;
   bool no_para = FALSE;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't do that right now.\n\r", ch );
      return;
   }

   /*
    * Someone is gonna die if I come back later and find this removed again!!! 
    */
   if( ch->Class != CLASS_ROGUE && !IS_IMMORTAL( ch ) )
   {
      send_to_char( "What do you think you are? A Rogue???\n\r", ch );
      return;
   }

   one_argument( argument, arg );

   if( ch->mount )
   {
      send_to_char( "You can't get close enough while mounted.\n\r", ch );
      return;
   }

   if( arg[0] == '\0' )
   {
      send_to_char( "Backstab whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "How can you sneak up on yourself?\n\r", ch );
      return;
   }

   if( is_safe( ch, victim ) )
      return;

   if( victim->hit < victim->max_hit )
   {
      ch_printf( ch, "%s is hurt and suspicious, you'll never get close enough.\r\n",
         ( IS_NPC( victim ) ? victim->short_descr : victim->name ) );
      return;
   }

   /*
    * Added stabbing weapon. -Narn 
    */
   if( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL )
   {
      send_to_char( "You need to wield a piercing or stabbing weapon.\n\r", ch );
      return;
   }

   if( obj->value[4] != WEP_DAGGER )
   {
      if( ( obj->value[4] == WEP_SWORD && obj->value[3] != DAM_PIERCE ) || obj->value[4] != WEP_SWORD )
      {
         send_to_char( "You need to wield a piercing or stabbing weapon.\n\r", ch );
         return;
      }
   }

   if( victim->fighting )
      base = 0;
   else
      base = 4;

/* ==4/3/95== Gives the backstab a chance to paralysis it's victim.
 * This chance is increased and is mainly dependent upon
 * the fact that the thief is sneaking.  There is also a
 * chance that the mob will not even notice thief has missed
 * the backstab.  A sleeping mob has better chance of being
 * paralysised and has no chance for missing backstab.
 * Chance to paralysis and not be seen is modified by thieves
 * dexterity - Open [Former Shard coder]
 */

   af.type = gsn_paralyze;
   af.duration = -1;
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_PARALYSIS;

   if( IS_AFFECTED( ch, AFF_SNEAK ) )
      to_saw += number_range( 1, 100 );
   if( get_curr_dex( ch ) > get_curr_dex( victim ) )
      to_saw += get_curr_dex( ch );

   percent = number_percent(  ) - ( get_curr_lck( ch ) - 14 ) + ( get_curr_lck( victim ) - 13 );

   if( check_illegal_pk( ch, victim ) )
   {
      send_to_char( "You cannot do that to another player!\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_backstab]->beats );

   if( ( IS_NPC( ch ) || percent > ch->pcdata->learned[gsn_backstab] ) && IS_AWAKE( victim ) )
   {
      /*
       * Failed backstab 
       */
      if( awake_saw > to_saw )
      {
         /*
          * Victim saw attempt at backstab 
          */
         act( AT_SKILL, "$n nearly slices off $s finger trying to backstab $N!", ch, NULL, victim, TO_NOTVICT );
         act( AT_SKILL, "$n nearly slices off $s finger trying to backstab you!", ch, NULL, victim, TO_VICT );
         act( AT_SKILL, "You nearly slice off your finger trying to backstab $N!", ch, NULL, victim, TO_CHAR );
         global_retcode = damage( ch, victim, 0, gsn_backstab );
      }
      else
      {
         /*
          * victim did not see failed attempt 
          */
         act( AT_SKILL, "$N didn't seem to notice your futile attempt to backstab!", ch, NULL, victim, TO_CHAR );
         act( AT_SKILL, "$N didn't seem to notice $n's failed backstab attempt!", ch, NULL, victim, TO_NOTVICT );
      }
      learn_from_failure( ch, gsn_backstab );

   }
   else
   {  /* Successful Backstab */
      if( !IS_AFFECTED( victim, AFF_PARALYSIS ) )
      {
         if( IS_IMMUNE( victim, RIS_HOLD ) )
            no_para = TRUE;
         if( IS_RESIS( victim, RIS_HOLD ) )
         {
            if( saves_spell_staff( ch->level, victim ) )
               no_para = TRUE;
         }
      }

      if( IS_AWAKE( victim ) )
      {
         if( to_saw > awake_paralysis && !no_para )
         {  /* Para victim */
            act( AT_SKILL, "$N is frozen by a critical pierce to the spine!", ch, NULL, victim, TO_CHAR );
            act( AT_SKILL, "$n got you in the spine! You are paralyzed!!", ch, NULL, victim, TO_VICT );
            act( AT_SKILL, "$n paralyzed $N with a critical pierce to the spine!", ch, NULL, victim, TO_NOTVICT );
            affect_to_char( victim, &af );
         }

         act( AT_SKILL, "$n sneaks up behind $N, stabbing $M in the back!", ch, NULL, victim, TO_NOTVICT );
         act( AT_SKILL, "$n sneaks up behind you, stabbing you in the back!", ch, NULL, victim, TO_VICT );
         act( AT_SKILL, "You sneak up behind $N, stabbing $M in the back!", ch, NULL, victim, TO_CHAR );
         ch->hitroll += base;
         global_retcode = multi_hit( ch, victim, gsn_backstab );
         ch->hitroll -= base;
         adjust_favor( ch, 10, 1 );
      }
      else
      {
         if( to_saw > sleep_paralysis && !no_para )
         {  /* Para victim */
            act( AT_SKILL, "$N is frozen by a critical pierce to the spine!", ch, NULL, victim, TO_NOTVICT );
            act( AT_SKILL, "$n got you in the spine! You are paralyzed!!", ch, NULL, victim, TO_CHAR );
            affect_to_char( victim, &af );
         }

         act( AT_SKILL, "$n sneaks up behind $N, stabbing $M in the back!", ch, NULL, victim, TO_NOTVICT );
         act( AT_SKILL, "$n sneaks up behind you, stabbing you in the back!", ch, NULL, victim, TO_VICT );
         act( AT_SKILL, "You sneak up behind $N, stabbing $M in the back!", ch, NULL, victim, TO_CHAR );
         ch->hitroll += base;
         global_retcode = multi_hit( ch, victim, gsn_backstab );
         ch->hitroll -= base;
         adjust_favor( ch, 10, 1 );
      }
   }
   return;
}

CMDF do_rescue( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   CHAR_DATA *fch;
   int percent;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_BERSERK ) )
   {
      send_to_char( "You aren't thinking clearly...\n\r", ch );
      return;
   }

   one_argument( argument, arg );
   if( arg[0] == '\0' )
   {
      send_to_char( "Rescue whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "How about fleeing instead?\n\r", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\n\r", ch );
      return;
   }

   if( !IS_NPC( ch ) && IS_NPC( victim ) )
   {
      send_to_char( "They don't need your help!\n\r", ch );
      return;
   }

   if( !ch->fighting )
   {
      send_to_char( "Too late...\n\r", ch );
      return;
   }

   if( ( fch = who_fighting( victim ) ) == NULL )
   {
      send_to_char( "They are not fighting right now.\n\r", ch );
      return;
   }

   if( who_fighting( victim ) == ch )
   {
      send_to_char( "Just running away would be better...\n\r", ch );
      return;
   }

   if( IS_AFFECTED( victim, AFF_BERSERK ) )
   {
      send_to_char( "Stepping in front of a berserker would not be an intelligent decision.\n\r", ch );
      return;
   }

   percent = number_percent(  ) - ( get_curr_lck( ch ) - 14 ) - ( get_curr_lck( victim ) - 16 );

   WAIT_STATE( ch, skill_table[gsn_rescue]->beats );
   if( !can_use_skill( ch, percent, gsn_rescue ) )
   {
      send_to_char( "You fail the rescue.\n\r", ch );
      act( AT_SKILL, "$n tries to rescue you!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "$n tries to rescue $N!", ch, NULL, victim, TO_NOTVICT );
      learn_from_failure( ch, gsn_rescue );
      return;
   }

   act( AT_SKILL, "You rescue $N!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n rescues you!", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "$n moves in front of $N!", ch, NULL, victim, TO_NOTVICT );

   adjust_favor( ch, 8, 1 );
   stop_fighting( fch, FALSE );
   stop_fighting( victim, FALSE );
   if( ch->fighting )
      stop_fighting( ch, FALSE );

   set_fighting( ch, fch );
   set_fighting( fch, ch );
   return;
}

void kick_messages( CHAR_DATA * ch, CHAR_DATA * victim, int dam, ch_ret rcode )
{
   int i;

   switch ( GET_RACE( victim ) )
   {
      case RACE_HUMAN:
      case RACE_HIGH_ELF:
      case RACE_DWARF:
      case RACE_DROW:
      case RACE_ORC:
      case RACE_LYCANTH:
      case RACE_TROLL:
      case RACE_DEMON:
      case RACE_DEVIL:
      case RACE_MFLAYER:
      case RACE_ASTRAL:
      case RACE_PATRYN:
      case RACE_SARTAN:
      case RACE_DRAAGDIM:
      case RACE_GOLEM:
      case RACE_TROGMAN:
      case RACE_IGUANADON:
      case RACE_HALF_ELF:
      case RACE_HALF_OGRE:
      case RACE_HALF_ORC:
      case RACE_HALF_GIANT:
         i = number_range( 0, 3 );
         break;
      case RACE_PREDATOR:
      case RACE_HERBIV:
      case RACE_LABRAT:
         i = number_range( 4, 6 );
         break;
      case RACE_REPTILE:
      case RACE_DRAGON:
      case RACE_DRAGON_RED:
      case RACE_DRAGON_BLACK:
      case RACE_DRAGON_GREEN:
      case RACE_DRAGON_WHITE:
      case RACE_DRAGON_BLUE:
      case RACE_DRAGON_SILVER:
      case RACE_DRAGON_GOLD:
      case RACE_DRAGON_BRONZE:
      case RACE_DRAGON_COPPER:
      case RACE_DRAGON_BRASS:
         i = number_range( 4, 7 );
         break;
      case RACE_TREE:
         i = 8;
         break;
      case RACE_PARASITE:
      case RACE_SLIME:
      case RACE_VEGGIE:
      case RACE_VEGMAN:
         i = 9;
         break;
      case RACE_ROO:
      case RACE_GNOME:
      case RACE_HALFLING:
      case RACE_GOBLIN:
      case RACE_SMURF:
      case RACE_ENFAN:
         i = 10;
         break;
      case RACE_GIANT:
      case RACE_GIANT_HILL:
      case RACE_GIANT_FROST:
      case RACE_GIANT_FIRE:
      case RACE_GIANT_CLOUD:
      case RACE_GIANT_STORM:
      case RACE_GIANT_STONE:
      case RACE_TYTAN:
      case RACE_GOD:
         i = 11;
         break;
      case RACE_GHOST:
         i = 12;
         break;
      case RACE_BIRD:
      case RACE_SKEXIE:
         i = 13;
         break;
      case RACE_UNDEAD:
      case RACE_UNDEAD_VAMPIRE:
      case RACE_UNDEAD_LICH:
      case RACE_UNDEAD_WIGHT:
      case RACE_UNDEAD_GHAST:
      case RACE_UNDEAD_SPECTRE:
      case RACE_UNDEAD_ZOMBIE:
      case RACE_UNDEAD_SKELETON:
      case RACE_UNDEAD_GHOUL:
         i = 14;
         break;
      case RACE_DINOSAUR:
         i = 15;
         break;
      case RACE_INSECT:
      case RACE_ARACHNID:
         i = 16;
         break;
      case RACE_FISH:
         i = 17;
         break;
      default:
         i = 1;
   };

   if( ( !dam ) || ( rcode == rVICT_IMMUNE ) )
   {
      act( AT_GREY, att_kick_miss_ch[i], ch, NULL, victim, TO_CHAR );
      act( AT_GREY, att_kick_miss_victim[i], ch, NULL, victim, TO_VICT );
      act( AT_GREY, att_kick_miss_room[i], ch, NULL, victim, TO_NOTVICT );
   }
   else if( rcode == rVICT_DIED )
   {
      act( AT_GREY, att_kick_kill_ch[i], ch, NULL, victim, TO_CHAR );
      /*
       * act(AT_GREY, att_kick_kill_victim[i], ch, NULL, victim, TO_VICT); 
       */
      act( AT_GREY, att_kick_kill_room[i], ch, NULL, victim, TO_NOTVICT );
   }
   else
   {
      act( AT_GREY, att_kick_hit_ch[i], ch, NULL, victim, TO_CHAR );
      act( AT_GREY, att_kick_hit_victim[i], ch, NULL, victim, TO_VICT );
      act( AT_GREY, att_kick_hit_room[i], ch, NULL, victim, TO_NOTVICT );
   }
}

CMDF do_kick( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_kick]->skill_level[ch->Class]
       && ch->level < skill_table[gsn_kick]->race_level[ch->race] )
   {
      send_to_char( "You better leave the martial arts to fighters.\n\r", ch );
      return;
   }

   if( !( victim = who_fighting( ch ) ) )
      if( !( victim = get_char_room( ch, argument ) ) )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }

   if( !is_legal_kill( ch, victim ) || is_safe( ch, victim ) )
   {
      send_to_char( "You can't do that!\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_kick]->beats );
   if( ( IS_NPC( ch ) || number_percent(  ) < ch->pcdata->learned[gsn_kick] ) && GET_RACE( victim ) != RACE_GHOST )
   {
      global_retcode = damage( ch, victim, ch->Class == CLASS_MONK ? ch->level : ( ch->level / 2 ), gsn_kick );
      kick_messages( ch, victim, 1, global_retcode );
   }
   else
   {
      learn_from_failure( ch, gsn_kick );
      global_retcode = damage( ch, victim, 0, gsn_kick );
      kick_messages( ch, victim, 0, global_retcode );
   }
   return;
}

CMDF do_bite( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_bite]->skill_level[ch->Class] )
   {
      send_to_char( "That isn't quite one of your natural skills.\n\r", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_bite]->beats );
   if( can_use_skill( ch, number_percent(  ), gsn_bite ) )
   {
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_bite );
   }
   else
   {
      learn_from_failure( ch, gsn_bite );
      global_retcode = damage( ch, victim, 0, gsn_bite );
   }
   return;
}

CMDF do_claw( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_claw]->skill_level[ch->Class] )
   {
      send_to_char( "That isn't quite one of your natural skills.\n\r", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_claw]->beats );
   if( can_use_skill( ch, number_percent(  ), gsn_claw ) )
   {
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_claw );
   }
   else
   {
      learn_from_failure( ch, gsn_claw );
      global_retcode = damage( ch, victim, 0, gsn_claw );
   }
   return;
}

CMDF do_sting( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_sting]->skill_level[ch->Class] )
   {
      send_to_char( "That isn't quite one of your natural skills.\n\r", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_sting]->beats );
   if( can_use_skill( ch, number_percent(  ), gsn_sting ) )
   {
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_sting );
   }
   else
   {
      learn_from_failure( ch, gsn_sting );
      global_retcode = damage( ch, victim, 0, gsn_sting );
   }
   return;
}

CMDF do_tail( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_tail]->skill_level[ch->Class] )
   {
      send_to_char( "That isn't quite one of your natural skills.\n\r", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_tail]->beats );
   if( can_use_skill( ch, number_percent(  ), gsn_tail ) )
   {
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_tail );
   }
   else
   {
      learn_from_failure( ch, gsn_tail );
      global_retcode = damage( ch, victim, 0, gsn_tail );
   }
   return;
}

CMDF do_bash( CHAR_DATA * ch, char *argument )
{
   AFFECT_DATA af;
   CHAR_DATA *victim;
   int schance;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_bash]->skill_level[ch->Class] )
   {
      send_to_char( "You better leave the martial arts to fighters.\n\r", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
      if( !( victim = get_char_room( ch, argument ) ) )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }

   if( IS_ACT_FLAG( victim, ACT_HUGE ) || IS_AFFECTED( victim, AFF_GROWTH ) )
      if( !IsGiant( ch ) && !IS_AFFECTED( ch, AFF_GROWTH ) )
      {
         ch_printf( ch, "%s is MUCH too large to bash!\n\r", PERS( victim, ch, FALSE ) );
         return;
      }

   schance = ( ( get_curr_dex( victim ) + get_curr_str( victim ) + victim->level )
               - ( get_curr_dex( ch ) + get_curr_str( ch ) + ch->level ) );
   if( victim->fighting && victim->fighting->who != ch )
      schance += 25;
   WAIT_STATE( ch, skill_table[gsn_bash]->beats );
   if( IS_NPC( ch ) || ( number_percent(  ) + schance ) < ch->pcdata->learned[gsn_bash] )
   {
      WAIT_STATE( ch, 2 * sysdata.pulseviolence );
      WAIT_STATE( victim, 2 * sysdata.pulseviolence );
      victim->position = POS_SITTING;
      act( AT_SKILL, "$N smashes into you, and knocks you to the ground!", victim, NULL, ch, TO_CHAR );
      act( AT_SKILL, "You smash into $N, and knock $M to the ground!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n smashes into $N, and knocks $M to the ground!", ch, NULL, victim, TO_NOTVICT );
      global_retcode = damage( ch, victim, number_range( 1, 2 ), gsn_bash );
      victim->position = POS_SITTING;

      /*
       * A cheap hack to force the issue of not being able to cast spells 
       */
      af.type = gsn_bash;
      af.duration = 4;  /* 4 battle rounds, hopefully */
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bit = AFF_BASH;
      affect_to_char( victim, &af );
   }
   else
   {
      WAIT_STATE( ch, 2 * sysdata.pulseviolence );
      learn_from_failure( ch, gsn_bash );
      ch->position = POS_SITTING;
      act( AT_SKILL, "$N tries to smash into you, but falls to the ground!", victim, NULL, ch, TO_CHAR );
      act( AT_SKILL, "You smash into $N, and bounce off $M to the ground!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n smashes into $N, and bounces off $M to the ground!", ch, NULL, victim, TO_NOTVICT );
      global_retcode = damage( ch, victim, 0, gsn_bash );
   }
   return;
}

CMDF do_stun( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   int schance;
   bool fail;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_stun]->skill_level[ch->Class] )
   {
      send_to_char( "You better leave the martial arts to fighters.\n\r", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\n\r", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->move < ch->max_move / 10 )
   {
      send_to_char( "&[skill]You are far too tired to do that.\n\r", ch );
      return;  /* missing return fixed March 11/96 */
   }

   WAIT_STATE( ch, skill_table[gsn_stun]->beats );
   fail = FALSE;
   schance = ris_save( victim, ch->level, RIS_PARALYSIS );
   if( schance == 1000 )
      fail = TRUE;
   else
      fail = saves_para_petri( schance, victim );

   schance =
      ( ( ( get_curr_dex( victim ) + get_curr_str( victim ) ) - ( get_curr_dex( ch ) + get_curr_str( ch ) ) ) * 10 ) + 10;
   /*
    * harder for player to stun another player 
    */
   if( !IS_NPC( ch ) && !IS_NPC( victim ) )
      schance += sysdata.stun_plr_vs_plr;
   else
      schance += sysdata.stun_regular;
   if( !fail && can_use_skill( ch, ( number_percent(  ) + schance ), gsn_stun ) )
   {
      /*
       * DO *NOT* CHANGE!    -Thoric    
       */
      if( !IS_NPC( ch ) )
         ch->move -= ch->max_move / 10;
      WAIT_STATE( ch, 2 * sysdata.pulseviolence );
      WAIT_STATE( victim, sysdata.pulseviolence );
      act( AT_SKILL, "$N smashes into you, leaving you stunned!", victim, NULL, ch, TO_CHAR );
      act( AT_SKILL, "You smash into $N, leaving $M stunned!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n smashes into $N, leaving $M stunned!", ch, NULL, victim, TO_NOTVICT );
      if( !IS_AFFECTED( victim, AFF_PARALYSIS ) )
      {
         af.type = gsn_stun;
         af.location = APPLY_AC;
         af.modifier = 20;
         af.duration = 3;
         af.bit = AFF_PARALYSIS;
         affect_to_char( victim, &af );
         update_pos( victim );
      }
   }
   else
   {
      WAIT_STATE( ch, 2 * sysdata.pulseviolence );
      if( !IS_NPC( ch ) )
         ch->move -= ch->max_move / 15;
      learn_from_failure( ch, gsn_stun );
      act( AT_SKILL, "$n charges at you screaming, but you dodge out of the way.", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You try to stun $N, but $E dodges out of the way.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n charges screaming at $N, but keeps going right on past.", ch, NULL, victim, TO_NOTVICT );
   }
   return;
}

bool check_grip( CHAR_DATA * ch, CHAR_DATA * victim )
{
   int schance;

   if( !IS_AWAKE( victim ) )
      return FALSE;

   if( !IS_DEFENSE( victim, DFND_GRIP ) )
      return FALSE;

   if( IS_NPC( victim ) )
      schance = UMIN( 60, 2 * victim->level );
   else
      schance = ( int )( LEARNED( victim, gsn_grip ) / 2 );

   /*
    * Consider luck as a factor 
    */
   schance += ( 2 * ( get_curr_lck( victim ) - 13 ) );

   if( number_percent(  ) >= schance + victim->level - ch->level )
   {
      learn_from_failure( victim, gsn_grip );
      return FALSE;
   }
   act( AT_SKILL, "You evade $n's attempt to disarm you.", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "$N holds $S weapon strongly, and is not disarmed.", ch, NULL, victim, TO_CHAR );
   return TRUE;
}

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 * Check for loyalty flag (weapon disarms to inventory) for pkillers -Blodkai
 */
void disarm( CHAR_DATA * ch, CHAR_DATA * victim )
{
   OBJ_DATA *obj, *tmpobj;

   if( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
      return;

   if( ( tmpobj = get_eq_char( victim, WEAR_DUAL_WIELD ) ) != NULL && number_bits( 1 ) == 0 )
      obj = tmpobj;

   if( get_eq_char( ch, WEAR_WIELD ) == NULL && number_bits( 1 ) == 0 )
   {
      learn_from_failure( ch, gsn_disarm );
      return;
   }

   if( IS_NPC( ch ) && !can_see_obj( ch, obj, FALSE ) && number_bits( 1 ) == 0 )
   {
      learn_from_failure( ch, gsn_disarm );
      return;
   }

   if( check_grip( ch, victim ) )
   {
      learn_from_failure( ch, gsn_disarm );
      return;
   }

   act( AT_SKILL, "$n DISARMS you!", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "You disarm $N!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n disarms $N!", ch, NULL, victim, TO_NOTVICT );

   if( obj == get_eq_char( victim, WEAR_WIELD ) && ( tmpobj = get_eq_char( victim, WEAR_DUAL_WIELD ) ) != NULL )
      tmpobj->wear_loc = WEAR_WIELD;

   obj_from_char( obj );
   if( IS_NPC( victim ) || ( IS_OBJ_FLAG( obj, ITEM_LOYAL ) && IS_PKILL( victim ) && !IS_NPC( ch ) ) )
      obj_to_char( obj, victim );
   else
      obj_to_room( obj, victim->in_room, victim );

   return;
}

CMDF do_disarm( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   int percent;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_disarm]->skill_level[ch->Class] )
   {
      send_to_char( "You don't know how to disarm opponents.\n\r", ch );
      return;
   }

   if( get_eq_char( ch, WEAR_WIELD ) == NULL && ch->Class != CLASS_MONK )
   {
      send_to_char( "You must wield a weapon to disarm.\n\r", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\n\r", ch );
      return;
   }

   if( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
   {
      send_to_char( "Your opponent is not wielding a weapon.\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_disarm]->beats );
   percent = number_percent(  ) + victim->level - ch->level - ( get_curr_lck( ch ) - 15 ) + ( get_curr_lck( victim ) - 15 );
   if( !can_see_obj( ch, obj, FALSE ) )
      percent += 10;
   if( can_use_skill( ch, ( percent * 3 / 2 ), gsn_disarm ) )
      disarm( ch, victim );
   else
   {
      send_to_char( "You failed.\n\r", ch );
      learn_from_failure( ch, gsn_disarm );
   }
   return;
}

/*
 * Trip a creature.
 * Caller must check for successful attack.
 */
void trip( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( IS_AFFECTED( victim, AFF_FLYING ) || IS_AFFECTED( victim, AFF_FLOATING ) )
      return;
   if( victim->mount )
   {
      if( IS_AFFECTED( victim->mount, AFF_FLYING ) || IS_AFFECTED( victim->mount, AFF_FLOATING ) )
         return;
      act( AT_SKILL, "$n trips your mount and you fall off!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You trip $N's mount and $N falls off!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n trips $N's mount and $N falls off!", ch, NULL, victim, TO_NOTVICT );
      REMOVE_ACT_FLAG( victim->mount, ACT_MOUNTED );
      victim->mount = NULL;
      WAIT_STATE( ch, 2 * sysdata.pulseviolence );
      WAIT_STATE( victim, 2 * sysdata.pulseviolence );
      victim->position = POS_RESTING;
      return;
   }
   if( victim->wait == 0 )
   {
      act( AT_SKILL, "$n trips you and you go down!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You trip $N and $N goes down!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n trips $N and $N goes down!", ch, NULL, victim, TO_NOTVICT );

      WAIT_STATE( ch, 2 * sysdata.pulseviolence );
      WAIT_STATE( victim, 2 * sysdata.pulseviolence );
      victim->position = POS_RESTING;
   }
   return;
}

CMDF do_pick( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *gch;
   OBJ_DATA *obj;
   EXIT_DATA *pexit;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Pick what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_pick_lock]->beats );

   /*
    * look for guards 
    */
   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( IS_NPC( gch ) && IS_AWAKE( gch ) && ch->level + 5 < gch->level )
      {
         act( AT_PLAIN, "$N is standing too close to the lock.", ch, NULL, gch, TO_CHAR );
         return;
      }
   }

   if( !can_use_skill( ch, number_percent(  ), gsn_pick_lock ) )
   {
      send_to_char( "You failed.\n\r", ch );
      learn_from_failure( ch, gsn_pick_lock );
      return;
   }

   if( ( pexit = find_door( ch, arg, TRUE ) ) != NULL )
   {
      /*
       * 'pick door' 
       */
      EXIT_DATA *pexit_rev;

      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         send_to_char( "It's not closed.\n\r", ch );
         return;
      }
      if( pexit->key < 0 )
      {
         send_to_char( "It can't be picked.\n\r", ch );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         send_to_char( "It's already unlocked.\n\r", ch );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_PICKPROOF ) )
      {
         send_to_char( "You failed.\n\r", ch );
         learn_from_failure( ch, gsn_pick_lock );
         check_room_for_traps( ch, TRAP_PICK | trap_door[pexit->vdir] );
         return;
      }

      REMOVE_EXIT_FLAG( pexit, EX_LOCKED );
      send_to_char( "*Click*\n\r", ch );
      act( AT_ACTION, "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
      adjust_favor( ch, 9, 1 );
      /*
       * pick the other side 
       */
      if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
      {
         REMOVE_EXIT_FLAG( pexit_rev, EX_LOCKED );
      }
      check_room_for_traps( ch, TRAP_PICK | trap_door[pexit->vdir] );
      return;
   }

   if( ( obj = get_obj_here( ch, arg ) ) != NULL )
   {
      /*
       * 'pick object' 
       */
      if( obj->item_type != ITEM_CONTAINER )
      {
         send_to_char( "That's not a container.\n\r", ch );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         send_to_char( "It's not closed.\n\r", ch );
         return;
      }
      if( obj->value[2] < 0 )
      {
         send_to_char( "It can't be unlocked.\n\r", ch );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         send_to_char( "It's already unlocked.\n\r", ch );
         return;
      }
      if( IS_SET( obj->value[1], CONT_PICKPROOF ) )
      {
         send_to_char( "You failed.\n\r", ch );
         learn_from_failure( ch, gsn_pick_lock );
         check_for_trap( ch, obj, TRAP_PICK );
         return;
      }

      separate_obj( obj );
      REMOVE_BIT( obj->value[1], CONT_LOCKED );
      send_to_char( "*Click*\n\r", ch );
      act( AT_ACTION, "$n picks $p.", ch, obj, NULL, TO_ROOM );
      adjust_favor( ch, 9, 1 );
      check_for_trap( ch, obj, TRAP_PICK );
      return;
   }

   ch_printf( ch, "You see no %s here.\n\r", arg );
   return;
}

/*
 * Contributed by Alander.
 */
CMDF do_visible( CHAR_DATA * ch, char *argument )
{
   affect_strip( ch, gsn_invis );
   affect_strip( ch, gsn_mass_invis );
   affect_strip( ch, gsn_sneak );
   REMOVE_AFFECTED( ch, AFF_HIDE );
   REMOVE_AFFECTED( ch, AFF_INVISIBLE );
   REMOVE_AFFECTED( ch, AFF_SNEAK );

   /*
    * Remove immortal wizinvis - Samson 11-28-98 
    */
   if( IS_PLR_FLAG( ch, PLR_WIZINVIS ) )
   {
      REMOVE_PLR_FLAG( ch, PLR_WIZINVIS );
      act( AT_IMMORT, "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
      send_to_char( "&[immortal]You slowly fade back into existence.\n\r", ch );
   }
   send_to_char( "Ok.\n\r", ch );
   return;
}

CMDF do_aid( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   int percent;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   one_argument( argument, arg );
   if( arg[0] == '\0' )
   {
      send_to_char( "Aid whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( IS_NPC( victim ) )  /* Gorog */
   {
      send_to_char( "Not on mobs.\n\r", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Aid yourself?\n\r", ch );
      return;
   }

   if( victim->position > POS_STUNNED )
   {
      act( AT_PLAIN, "$N doesn't need your help.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim->hit <= -6 )
   {
      act( AT_PLAIN, "$N's condition is beyond your aiding ability.", ch, NULL, victim, TO_CHAR );
      return;
   }

   percent = number_percent(  ) - ( get_curr_lck( ch ) - 13 );
   WAIT_STATE( ch, skill_table[gsn_aid]->beats );
   if( !can_use_skill( ch, percent, gsn_aid ) )
   {
      send_to_char( "You fail.\n\r", ch );
      learn_from_failure( ch, gsn_aid );
      return;
   }

   act( AT_SKILL, "You aid $N!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n aids $N!", ch, NULL, victim, TO_NOTVICT );
   adjust_favor( ch, 8, 1 );
   if( victim->hit < 1 )
      victim->hit = 1;

   update_pos( victim );
   act( AT_SKILL, "$n aids you!", ch, NULL, victim, TO_VICT );
   return;
}

int mount_ego_check( CHAR_DATA * ch, CHAR_DATA * horse )
{
   int ride_ego, drag_ego, align, check;

   if( IS_NPC( ch ) )
      return -5;

   if( IsDragon( horse ) )
   {
      drag_ego = horse->level * 2;
      if( IS_ACT_FLAG( horse, ACT_AGGRESSIVE ) || IS_ACT_FLAG( horse, ACT_META_AGGR ) )
         drag_ego += horse->level;

      ride_ego = ch->pcdata->learned[gsn_mount] / 10 + ch->level / 2;

      if( is_affected( ch, gsn_dragon_ride ) )
         ride_ego += ( ( get_curr_int( ch ) + get_curr_wis( ch ) ) / 2 );
      align = GET_ALIGN( ch ) - GET_ALIGN( horse );
      if( align < 0 )
         align = -align;
      align /= 100;
      align -= 5;
      drag_ego += align;
      if( GET_HIT( horse ) > 0 )
         drag_ego -= GET_MAX_HIT( horse ) / GET_HIT( horse );
      else
         drag_ego = 0;
      if( GET_HIT( ch ) > 0 )
         ride_ego -= GET_MAX_HIT( ch ) / GET_HIT( ch );
      else
         ride_ego = 0;

      check = drag_ego + number_range( 1, 10 ) - ( ride_ego + number_range( 1, 10 ) );
      return ( check );
   }
   else
   {
      drag_ego = horse->level;

      if( drag_ego > 15 )
         drag_ego *= 2;

      ride_ego = ch->pcdata->learned[gsn_mount] / 10 + ch->level;

      if( is_affected( ch, gsn_dragon_ride ) )
         ride_ego += ( get_curr_int( ch ) + get_curr_wis( ch ) );
      check = drag_ego + number_range( 1, 5 ) - ( ride_ego + number_range( 1, 10 ) );
      if( IS_ACT_FLAG( horse, ACT_PET ) && horse->master == ch )
         check = -1;
      return ( check );
   }
}

CMDF do_mount( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   short learned;
   int check;

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_mount]->skill_level[ch->Class] )
   {
      send_to_char( "I don't think that would be a good idea...\n\r", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You're already mounted!\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, argument ) ) == NULL )
   {
      send_to_char( "You can't find that here.\n\r", ch );
      return;
   }

   if( !IS_ACT_FLAG( victim, ACT_MOUNTABLE ) )
   {
      send_to_char( "You can't mount that!\n\r", ch );
      return;
   }

   if( IS_ACT_FLAG( victim, ACT_MOUNTED ) )
   {
      send_to_char( "That mount already has a rider.\n\r", ch );
      return;
   }

   if( victim->position < POS_STANDING )
   {
      send_to_char( "Your mount must be standing.\n\r", ch );
      return;
   }

   if( victim->position == POS_FIGHTING || victim->fighting )
   {
      send_to_char( "Your mount is moving around too much.\n\r", ch );
      return;
   }

   if( victim->my_rider && victim->my_rider != ch )   /* prevents dragon theft */
   {
      send_to_char( "This is someone else's dragon.\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_mount]->beats );

   check = mount_ego_check( ch, victim );
   if( check > 5 )
   {
      act( AT_SKILL, "$N snarls and attacks!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "As $n tries to mount $N, $N attacks $n!", ch, NULL, victim, TO_NOTVICT );
      cmdf( victim, "kill %s", GET_NAME( ch ) );
      return;
   }
   else if( check > -1 )
   {
      act( AT_SKILL, "$N moves out of the way and you fall on your butt.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "as $n tries to mount $N, $N moves out of the way", ch, NULL, victim, TO_NOTVICT );
      GET_POS( ch ) = POS_SITTING;
      return;
   }

   if( !IS_NPC( ch ) )
      learned = ch->pcdata->learned[gsn_mount];
   else
      learned = 0;

   if( is_affected( ch, gsn_dragon_ride ) )
      learned += ch->level;

   if( IS_NPC( ch ) || number_percent(  ) < learned )
   {
      SET_ACT_FLAG( victim, ACT_MOUNTED );
      ch->mount = victim;
      act( AT_SKILL, "You mount $N.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n skillfully mounts $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n mounts you.", ch, NULL, victim, TO_VICT );
      ch->position = POS_MOUNTED;
   }
   else
   {
      act( AT_SKILL, "You unsuccessfully try to mount $N.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n unsuccessfully attempts to mount $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n tries to mount you.", ch, NULL, victim, TO_VICT );
      learn_from_failure( ch, gsn_mount );
   }
   return;
}

CMDF do_dismount( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   bool fell = FALSE;

   if( ( victim = ch->mount ) == NULL )
   {
      send_to_char( "You're not mounted.\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_mount]->beats );
   if( IS_NPC( ch ) || number_percent(  ) < ch->pcdata->learned[gsn_mount] )
   {
      act( AT_SKILL, "You dismount $N.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n skillfully dismounts $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n dismounts you.  Whew!", ch, NULL, victim, TO_VICT );
      REMOVE_ACT_FLAG( victim, ACT_MOUNTED );
      ch->mount = NULL;
      ch->position = POS_STANDING;
   }
   else
   {
      act( AT_SKILL, "You fall off while dismounting $N.  Ouch!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n falls off of $N while dismounting.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n falls off your back.", ch, NULL, victim, TO_VICT );
      learn_from_failure( ch, gsn_mount );
      REMOVE_ACT_FLAG( victim, ACT_MOUNTED );
      ch->mount = NULL;
      ch->position = POS_SITTING;
      fell = TRUE;
      global_retcode = damage( ch, ch, 1, TYPE_UNDEFINED );
   }
   check_mount_objs( ch, fell ); /* Check for ITEM_MUSTMOUNT stuff */
   return;
}

/*
 * Check for parry.
 */
bool check_parry( CHAR_DATA * ch, CHAR_DATA * victim )
{
   int chances;

   if( !IS_AWAKE( victim ) )
      return FALSE;

   if( !IS_DEFENSE( victim, DFND_PARRY ) )
      return FALSE;

   if( IS_NPC( victim ) )
   {
      /*
       * Tuan was here.  :) 
       */
      chances = UMIN( 60, 2 * victim->level );
   }
   else
   {
      if( get_eq_char( victim, WEAR_WIELD ) == NULL )
         return FALSE;
      chances = ( int )( LEARNED( victim, gsn_parry ) / sysdata.parry_mod );
   }

   /*
    * Put in the call to chance() to allow penalties for misaligned clannies. 
    */
   if( chances != 0 && victim->morph )
      chances += victim->morph->parry;

   if( !chance( victim, chances + victim->level - ch->level ) )
   {
      learn_from_failure( victim, gsn_parry );
      return FALSE;
   }

   /*
    * Modified by Tarl 24 April 02 to reduce combat spam with GAG flag. 
    */
   /*
    * BAD TARL! You forgot yur IS_NPC checks here :) 
    */
   /*
    * Macro cleanup made that unnecessary on 9-18-03 - Samson 
    */
   if( !IS_PCFLAG( victim, PCFLAG_GAG ) )
      act( AT_SKILL, "You parry $n's attack.", ch, NULL, victim, TO_VICT );

   if( !IS_NPC( ch ) && !( IS_SET( ch->pcdata->flags, PCFLAG_GAG ) ) )
      act( AT_SKILL, "$N parries your attack.", ch, NULL, victim, TO_CHAR );

   act( AT_SKILL, "$N parries $n's attack.", ch, NULL, victim, TO_NOTVICT );

   return TRUE;
}

/*
 * Check for dodge.
 */
bool check_dodge( CHAR_DATA * ch, CHAR_DATA * victim )
{
   int chances;

   if( !IS_AWAKE( victim ) )
      return FALSE;

   if( !IS_DEFENSE( victim, DFND_DODGE ) )
      return FALSE;

   if( IS_NPC( victim ) )
      chances = UMIN( 60, 2 * victim->level );
   else
      chances = ( int )( LEARNED( victim, gsn_dodge ) / sysdata.dodge_mod );

   if( chances != 0 && victim->morph != NULL )
      chances += victim->morph->dodge;

   /*
    * Consider luck as a factor 
    */
   if( !chance( victim, chances + victim->level - ch->level ) )
   {
      learn_from_failure( victim, gsn_dodge );
      return FALSE;
   }
   /*
    * Modified by Tarl 24 April 02 to reduce combat spam with GAG flag. 
    */
   /*
    * And yes, I forgot the NPC checks here at first, too. :P 
    */
   /*
    * And the macro cleanup made this one moot as well - Samson 
    */
   if( !IS_PCFLAG( victim, PCFLAG_GAG ) )
      act( AT_SKILL, "You dodge $n's attack.", ch, NULL, victim, TO_VICT );

   if( !IS_NPC( ch ) && ( !( IS_SET( ch->pcdata->flags, PCFLAG_GAG ) ) ) )
      act( AT_SKILL, "$N dodges your attack.", ch, NULL, victim, TO_CHAR );

   act( AT_SKILL, "$N dodges $n's attack.", ch, NULL, victim, TO_NOTVICT );

   return TRUE;
}

bool check_tumble( CHAR_DATA * ch, CHAR_DATA * victim )
{
   int chances;

   if( ( victim->Class != CLASS_ROGUE && victim->Class != CLASS_MONK ) || !IS_AWAKE( victim ) )
      return FALSE;
   if( !IS_NPC( victim ) && !victim->pcdata->learned[gsn_tumble] > 0 )
      return FALSE;
   if( IS_NPC( victim ) )
      chances = UMIN( 60, 2 * victim->level );
   else
      chances = ( int )( LEARNED( victim, gsn_tumble ) / sysdata.tumble_mod + ( get_curr_dex( victim ) - 13 ) );
   if( chances != 0 && victim->morph )
      chances += victim->morph->tumble;
   if( !chance( victim, chances + victim->level - ch->level ) )
   {
      learn_from_failure( victim, gsn_tumble );
      return FALSE;
   }
   act( AT_SKILL, "You tumble away from $n's attack.", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "$N tumbles away from your attack.", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$N tumbles away from $n's attack.", ch, NULL, victim, TO_NOTVICT );
   return TRUE;
}

CMDF do_poison_weapon( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj, *pobj, *wobj;
   int percent;

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_poison_weapon]->skill_level[ch->Class] )
   {
      send_to_char( "What do you think you are, a thief?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "What are you trying to poison?\n\r", ch );
      return;
   }
   if( ch->fighting )
   {
      send_to_char( "While you're fighting?  Nice try.\n\r", ch );
      return;
   }
   if( ms_find_obj( ch ) )
      return;

   if( !( obj = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You do not have that weapon.\n\r", ch );
      return;
   }
   if( obj->item_type != ITEM_WEAPON )
   {
      send_to_char( "That item is not a weapon.\n\r", ch );
      return;
   }
   if( IS_OBJ_FLAG( obj, ITEM_POISONED ) )
   {
      send_to_char( "That weapon is already poisoned.\n\r", ch );
      return;
   }
   if( IS_OBJ_FLAG( obj, ITEM_CLANOBJECT ) )
   {
      send_to_char( "It doesn't appear to be fashioned of a poisonable material.\n\r", ch );
      return;
   }
   /*
    * Now we have a valid weapon...check to see if we have the powder. 
    */
   for( pobj = ch->first_carrying; pobj; pobj = pobj->next_content )
   {
      if( pobj->pIndexData->vnum == OBJ_VNUM_BLACK_POWDER )
         break;
   }
   if( !pobj )
   {
      send_to_char( "You do not have the black poison powder.\n\r", ch );
      return;
   }
   /*
    * Okay, we have the powder...do we have water? 
    */
   for( wobj = ch->first_carrying; wobj; wobj = wobj->next_content )
   {
      if( wobj->item_type == ITEM_DRINK_CON && wobj->value[1] > 0 && wobj->value[2] == 0 )
         break;
   }
   if( !wobj )
   {
      send_to_char( "You have no water to mix with the powder.\n\r", ch );
      return;
   }
   /*
    * Great, we have the ingredients...but is the thief smart enough? 
    */
   /*
    * Modified by Tarl 24 April 02 Lowered the wisdom requirement from 16 to 14. 
    */
   if( !IS_NPC( ch ) && get_curr_wis( ch ) < 14 )
   {
      send_to_char( "You can't quite remember what to do...\n\r", ch );
      return;
   }
   /*
    * And does the thief have steady enough hands? 
    */
   if( !IS_NPC( ch ) && ( ( get_curr_dex( ch ) < 17 ) || ch->pcdata->condition[COND_DRUNK] > 0 ) )
   {
      send_to_char( "Your hands aren't steady enough to properly mix the poison.\n\r", ch );
      return;
   }
   WAIT_STATE( ch, skill_table[gsn_poison_weapon]->beats );

   percent = ( number_percent(  ) - ( get_curr_lck( ch ) - 14 ) );

   /*
    * Check the skill percentage 
    */
   separate_obj( pobj );
   separate_obj( wobj );
   if( !can_use_skill( ch, percent, gsn_poison_weapon ) )
   {
      send_to_char( "&RYou failed and spill some on yourself.  Ouch!&w\n\r", ch );
      damage( ch, ch, ch->level, gsn_poison_weapon );
      act( AT_RED, "$n spills the poison all over!", ch, NULL, NULL, TO_ROOM );
      extract_obj( pobj );
      extract_obj( wobj );
      learn_from_failure( ch, gsn_poison_weapon );
      return;
   }
   separate_obj( obj );
   /*
    * Well, I'm tired of waiting.  Are you? 
    */
   act( AT_RED, "You mix $p in $P, creating a deadly poison!", ch, pobj, wobj, TO_CHAR );
   act( AT_RED, "$n mixes $p in $P, creating a deadly poison!", ch, pobj, wobj, TO_ROOM );
   act( AT_GREEN, "You pour the poison over $p, which glistens wickedly!", ch, obj, NULL, TO_CHAR );
   act( AT_GREEN, "$n pours the poison over $p, which glistens wickedly!", ch, obj, NULL, TO_ROOM );
   SET_OBJ_FLAG( obj, ITEM_POISONED );
   obj->cost *= 2;
   /*
    * Set an object timer.  Don't want proliferation of poisoned weapons 
    */
   obj->timer = UMIN( obj->level, ch->level );

   if( IS_OBJ_FLAG( obj, ITEM_BLESS ) )
      obj->timer *= 2;

   if( IS_OBJ_FLAG( obj, ITEM_MAGIC ) )
      obj->timer *= 2;

   /*
    * WHAT?  All of that, just for that one bit?  How lame. ;) 
    */
   act( AT_BLUE, "The remainder of the poison eats through $p.", ch, wobj, NULL, TO_CHAR );
   act( AT_BLUE, "The remainder of the poison eats through $p.", ch, wobj, NULL, TO_ROOM );
   extract_obj( pobj );
   extract_obj( wobj );
   return;
}

CMDF do_scribe( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *scroll;
   int sn;
   int mana;

   if( IS_NPC( ch ) )
      return;

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_scribe]->skill_level[ch->Class] )
   {
      send_to_char( "A skill such as this requires more magical ability than that of your Class.\n\r", ch );
      return;
   }

   if( argument[0] == '\0' || !str_cmp( argument, "" ) )
   {
      send_to_char( "Scribe what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( sn = find_spell( ch, argument, TRUE ) ) < 0 )
   {
      send_to_char( "You have not learned that spell.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "word of recall" ) )
      sn = find_spell( ch, "recall", FALSE );

   if( skill_table[sn]->spell_fun == spell_null )
   {
      send_to_char( "That's not a spell!\n\r", ch );
      return;
   }

   if( SPELL_FLAG( skill_table[sn], SF_NOSCRIBE ) )
   {
      send_to_char( "You cannot scribe that spell.\n\r", ch );
      return;
   }

   mana = IS_NPC( ch ) ? 0 : UMAX( skill_table[sn]->min_mana,
                                   100 / ( 2 + ch->level - skill_table[sn]->skill_level[ch->Class] ) );

   mana *= 5;

   if( !IS_NPC( ch ) && ch->mana < mana )
   {
      send_to_char( "You don't have enough mana.\n\r", ch );
      return;
   }

   if( ( scroll = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
   {
      send_to_char( "You must be holding a blank scroll to scribe it.\n\r", ch );
      return;
   }

   if( scroll->pIndexData->vnum != OBJ_VNUM_SCROLL_SCRIBING )
   {
      send_to_char( "You must be holding a blank scroll to scribe it.\n\r", ch );
      return;
   }

   if( ( scroll->value[1] != -1 ) && ( scroll->pIndexData->vnum == OBJ_VNUM_SCROLL_SCRIBING ) )
   {
      send_to_char( "That scroll has already been inscribed.\n\r", ch );
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      learn_from_failure( ch, gsn_scribe );
      ch->mana -= ( mana / 2 );
      return;
   }

   if( !can_use_skill( ch, number_percent(  ), gsn_scribe ) )
   {
      send_to_char( "&[magic]You failed.\n\r", ch );
      learn_from_failure( ch, gsn_scribe );
      ch->mana -= ( mana / 2 );
      return;
   }

   scroll->value[1] = sn;
   scroll->value[0] = ch->level;
   stralloc_printf( &scroll->short_descr, "%s scroll", skill_table[sn]->name );
   stralloc_printf( &scroll->objdesc, "A glowing scroll inscribed '%s' lies in the dust.", skill_table[sn]->name );
   stralloc_printf( &scroll->name, "scroll scribing %s", skill_table[sn]->name );

   act( AT_MAGIC, "$n magically scribes $p.", ch, scroll, NULL, TO_ROOM );
   act( AT_MAGIC, "You magically scribe $p.", ch, scroll, NULL, TO_CHAR );

   ch->mana -= mana;
}

CMDF do_brew( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *potion;
   OBJ_DATA *fire;
   int sn;
   int mana;
   bool found;

   if( IS_NPC( ch ) )
      return;

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_brew]->skill_level[ch->Class] )
   {
      send_to_char( "A skill such as this requires more magical ability than that of your Class.\n\r", ch );
      return;
   }

   if( argument[0] == '\0' || !str_cmp( argument, "" ) )
   {
      send_to_char( "Brew what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( sn = find_spell( ch, argument, TRUE ) ) < 0 )
   {
      send_to_char( "You have not learned that spell.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "word of recall" ) )
      sn = find_spell( ch, "recall", FALSE );

   if( skill_table[sn]->spell_fun == spell_null )
   {
      send_to_char( "That's not a spell!\n\r", ch );
      return;
   }

   if( SPELL_FLAG( skill_table[sn], SF_NOBREW ) )
   {
      send_to_char( "You cannot brew that spell.\n\r", ch );
      return;
   }

   mana = IS_NPC( ch ) ? 0 : UMAX( skill_table[sn]->min_mana,
                                   100 / ( 2 + ch->level - skill_table[sn]->skill_level[ch->Class] ) );

   mana *= 4;

   if( !IS_NPC( ch ) && ch->mana < mana )
   {
      send_to_char( "You don't have enough mana.\n\r", ch );
      return;
   }

   found = FALSE;

   for( fire = ch->in_room->first_content; fire; fire = fire->next_content )
   {
      if( fire->item_type == ITEM_FIRE )
      {
         found = TRUE;
         break;
      }
   }

   if( !found )
   {
      send_to_char( "There must be a fire in the room to brew a potion.\n\r", ch );
      return;
   }

   if( ( potion = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
   {
      send_to_char( "You must be holding an empty flask to brew a potion.\n\r", ch );
      return;
   }

   if( potion->pIndexData->vnum != OBJ_VNUM_FLASK_BREWING )
   {
      send_to_char( "You must be holding an empty flask to brew a potion.\n\r", ch );
      return;
   }

   if( ( potion->value[1] != -1 ) && ( potion->pIndexData->vnum == OBJ_VNUM_FLASK_BREWING ) )
   {
      send_to_char( "That's not an empty flask.\n\r", ch );
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      learn_from_failure( ch, gsn_brew );
      ch->mana -= ( mana / 2 );
      return;
   }

   if( !can_use_skill( ch, number_percent(  ), gsn_brew ) )
   {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "&[magic]You failed.\n\r", ch );
      learn_from_failure( ch, gsn_brew );
      ch->mana -= ( mana / 2 );
      return;
   }

   potion->value[1] = sn;
   potion->value[0] = ch->level;
   stralloc_printf( &potion->short_descr, "%s potion", skill_table[sn]->name );
   stralloc_printf( &potion->objdesc, "A strange potion labelled '%s' sizzles in a glass flask.", skill_table[sn]->name );
   stralloc_printf( &potion->name, "flask potion %s", skill_table[sn]->name );

   act( AT_MAGIC, "$n brews up $p.", ch, potion, NULL, TO_ROOM );
   act( AT_MAGIC, "You brew up $p.", ch, potion, NULL, TO_CHAR );

   ch->mana -= mana;
}

CMDF do_circle( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   int percent;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   one_argument( argument, arg );

   if( ch->mount )
   {
      send_to_char( "You can't circle while mounted.\n\r", ch );
      return;
   }

   if( arg[0] == '\0' )
   {
      send_to_char( "Circle around whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "How can you sneak up on yourself?\n\r", ch );
      return;
   }

   if( is_safe( ch, victim ) )
      return;

   if( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL )
   {
      send_to_char( "You need to wield a piercing or stabbing weapon.\n\r", ch );
      return;
   }

   if( obj->value[4] != WEP_DAGGER )
   {
      if( ( obj->value[4] == WEP_SWORD && obj->value[3] != DAM_PIERCE ) || obj->value[4] != WEP_SWORD )
      {
         send_to_char( "You need to wield a piercing or stabbing weapon.\n\r", ch );
         return;
      }
   }

   if( !ch->fighting )
   {
      send_to_char( "You can't circle when you aren't fighting.\n\r", ch );
      return;
   }

   if( !victim->fighting )
   {
      send_to_char( "You can't circle around a person who is not fighting.\n\r", ch );
      return;
   }

   if( victim->num_fighting < 2 )
   {
      act( AT_PLAIN, "You can't circle around them without a distraction.", ch, NULL, victim, TO_CHAR );
      return;
   }

   percent = number_percent(  ) - ( get_curr_lck( ch ) - 16 ) + ( get_curr_lck( victim ) - 13 );

   if( check_illegal_pk( ch, victim ) )
   {
      send_to_char( "You can't do that to another player!\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_circle]->beats );
   if( can_use_skill( ch, percent, gsn_circle ) )
   {
      WAIT_STATE( ch, 2 * sysdata.pulseviolence );
      act( AT_SKILL, "$n sneaks up behind $N, stabbing $M in the back!", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n sneaks up behind you, stabbing you in the back!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You sneak up behind $N, stabbing $M in the back!", ch, NULL, victim, TO_CHAR );
      global_retcode = multi_hit( ch, victim, gsn_circle );
      adjust_favor( ch, 10, 1 );
   }
   else
   {
      learn_from_failure( ch, gsn_circle );
      WAIT_STATE( ch, 2 * sysdata.pulseviolence );
      act( AT_SKILL, "$n nearly slices off $s finger trying to backstab $N!", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n nearly slices off $s finger trying to backstab you!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You nearly slice off your finger trying to backstab $N!", ch, NULL, victim, TO_CHAR );
      global_retcode = damage( ch, victim, 0, gsn_circle );
   }
   return;
}

/* Berserk and HitAll. -- Altrag */
CMDF do_berserk( CHAR_DATA * ch, char *argument )
{
   short percent;
   AFFECT_DATA af;

   if( !ch->fighting )
   {
      send_to_char( "But you aren't fighting!\n\r", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_BERSERK ) )
   {
      send_to_char( "Your rage is already at its peak!\n\r", ch );
      return;
   }

   percent = LEARNED( ch, gsn_berserk );
   WAIT_STATE( ch, skill_table[gsn_berserk]->beats );
   if( !chance( ch, percent ) )
   {
      send_to_char( "You couldn't build up enough rage.\n\r", ch );
      learn_from_failure( ch, gsn_berserk );
      return;
   }
   af.type = gsn_berserk;
   /*
    * Hmmm.. 10-20 combat rounds at level 50.. good enough for most mobs,
    * and if not they can always go berserk again.. shrug.. maybe even
    * too high. -- Altrag 
    */
   af.duration = number_range( ch->level / 5, ch->level * 2 / 5 );
   /*
    * Hmm.. you get stronger when yer really enraged.. mind over matter
    * type thing.. 
    */
   af.location = APPLY_STR;
   af.modifier = 1;
   af.bit = AFF_BERSERK;
   affect_to_char( ch, &af );
   send_to_char( "You start to lose control..\n\r", ch );
   return;
}

CMDF do_hitall( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;
   short nvict = 0;
   short nhit = 0;
   short percent;

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SAFE ) )
   {
      send_to_char( "&BA godly force prevents you.\n\r", ch );
      return;
   }

   if( !ch->in_room->first_person )
   {
      send_to_char( "There's no one else here!\n\r", ch );
      return;
   }
   percent = LEARNED( ch, gsn_hitall );
   for( vch = ch->in_room->first_person; vch; vch = vch_next )
   {
      vch_next = vch->next_in_room;
      if( is_same_group( ch, vch ) || !is_legal_kill( ch, vch ) || !can_see( ch, vch, FALSE ) || is_safe( ch, vch ) )
         continue;
      if( ++nvict > ch->level / 5 )
         break;
      if( chance( ch, percent ) )
      {
         nhit++;
         global_retcode = one_hit( ch, vch, gsn_hitall );
      }
      else
         global_retcode = damage( ch, vch, 0, gsn_hitall );
      /*
       * Fireshield, etc. could kill ch too.. :>.. -- Altrag 
       */
      if( global_retcode == rCHAR_DIED || char_died( ch ) )
         return;
   }
   if( !nvict )
   {
      send_to_char( "There's no one else here!\n\r", ch );
      return;
   }
   ch->move = UMAX( 0, ch->move - nvict * 3 + nhit );
   if( !nhit )
      learn_from_failure( ch, gsn_hitall );
   return;
}

static char *dir_desc[] = {
   "to the north",
   "to the east",
   "to the south",
   "to the west",
   "upwards",
   "downwards",
   "to the northeast",
   "to the northwest",
   "to the southeast",
   "to the southwest",
   "through the portal"
};

static char *rng_desc[] = {
   "right here",
   "immediately",
   "nearby",
   "a ways",
   "a good ways",
   "far",
   "far off",
   "very far",
   "very far off",
   "in the distance"
};

static void scanroom( CHAR_DATA * ch, ROOM_INDEX_DATA * room, int dir, int maxdist, int dist )
{
   CHAR_DATA *tch;
   EXIT_DATA *ex;

   for( tch = room->first_person; tch; tch = tch->next_in_room )
   {
      if( can_see( ch, tch, FALSE ) && !is_ignoring( tch, ch ) )
         ch_printf( ch, "%-30s : %s %s\n\r", IS_NPC( tch ) ? tch->short_descr : tch->name,
                    rng_desc[dist], dist == 0 ? "" : dir_desc[dir] );
   }
   for( ex = room->first_exit; ex; ex = ex->next )
      if( ex->vdir == dir )
         break;

   if( !ex || ex->vdir != dir || ex->vdir == DIR_SOMEWHERE || maxdist - 1 == 0
       || IS_EXIT_FLAG( ex, EX_CLOSED ) || IS_EXIT_FLAG( ex, EX_DIG ) || IS_EXIT_FLAG( ex, EX_FORTIFIED )
       || IS_EXIT_FLAG( ex, EX_HEAVY ) || IS_EXIT_FLAG( ex, EX_MEDIUM ) || IS_EXIT_FLAG( ex, EX_LIGHT )
       || IS_EXIT_FLAG( ex, EX_CRUMBLING ) || IS_EXIT_FLAG( ex, EX_OVERLAND ) )
      return;

   scanroom( ch, ex->to_room, dir, maxdist - 1, dist + 1 );
}

void map_scan( CHAR_DATA * ch );

/* Scan no longer accepts a direction argument */
CMDF do_scan( CHAR_DATA * ch, char *argument )
{
   int maxdist = 1;
   EXIT_DATA *ex;

   maxdist = ch->level / 10;

   maxdist = URANGE( 1, maxdist, 9 );

   if( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_ACT_FLAG( ch, ACT_ONMAP ) )
   {
      map_scan( ch );
      return;
   }

   scanroom( ch, ch->in_room, -1, 1, 0 );

   for( ex = ch->in_room->first_exit; ex; ex = ex->next )
   {
      if( IS_EXIT_FLAG( ex, EX_DIG ) || IS_EXIT_FLAG( ex, EX_CLOSED )
          || IS_EXIT_FLAG( ex, EX_FORTIFIED ) || IS_EXIT_FLAG( ex, EX_HEAVY )
          || IS_EXIT_FLAG( ex, EX_MEDIUM ) || IS_EXIT_FLAG( ex, EX_LIGHT )
          || IS_EXIT_FLAG( ex, EX_CRUMBLING ) || IS_EXIT_FLAG( ex, EX_OVERLAND ) )
         continue;

      if( ex->vdir == DIR_SOMEWHERE && !IS_IMMORTAL( ch ) )
         continue;

      scanroom( ch, ex->to_room, ex->vdir, maxdist, 1 );
   }
}

CMDF do_slice( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *corpse, *obj, *slice;
   bool found;
   MOB_INDEX_DATA *pMobIndex;
   found = FALSE;

   if( !IS_NPC( ch ) && !IS_IMMORTAL( ch ) && ch->level < skill_table[gsn_slice]->skill_level[ch->Class] )
   {
      send_to_char( "You are not learned in this skill.\n\r", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "From what do you wish to slice meat?\n\r", ch );
      return;
   }

   if( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL
       || ( obj->value[3] != DAM_SLASH && obj->value[3] != DAM_HACK && obj->value[3] != DAM_PIERCE
            && obj->value[3] != DAM_STAB ) )
   {
      send_to_char( "You need to wield a sharp weapon.\n\r", ch );
      return;
   }

   if( ( corpse = get_obj_here( ch, argument ) ) == NULL )
   {
      send_to_char( "You can't find that here.\n\r", ch );
      return;
   }

   if( corpse->item_type != ITEM_CORPSE_NPC || corpse->timer < 5 || corpse->value[3] < 75 )
   {
      send_to_char( "That is not a suitable source of meat.\n\r", ch );
      return;
   }

   if( ( pMobIndex = get_mob_index( corpse->value[4] ) ) == NULL )
   {
      send_to_char( "Error - report to immortals\n\r", ch );
      bug( "%s", "Can not find mob for value[4] of corpse, do_slice" );
      return;
   }

   if( !can_use_skill( ch, number_percent(  ), gsn_slice ) && !IS_IMMORTAL( ch ) )
   {
      send_to_char( "You fail to slice the meat properly.\n\r", ch );
      learn_from_failure( ch, gsn_slice );   /* Just in case they die :> */
      if( number_percent(  ) + ( get_curr_dex( ch ) - 13 ) < 10 )
      {
         act( AT_BLOOD, "You cut yourself!", ch, NULL, NULL, TO_CHAR );
         damage( ch, ch, ch->level, gsn_slice );
      }
      return;
   }

   if( !( slice = create_object( get_obj_index( OBJ_VNUM_SLICE ), 0 ) ) )
   {
      send_to_char( "Error - report to immortals\n\r", ch );
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }

   stralloc_printf( &slice->name, "meat fresh slice %s", pMobIndex->player_name );
   stralloc_printf( &slice->short_descr, "a slice of raw meat from %s", pMobIndex->short_descr );
   stralloc_printf( &slice->objdesc, "A slice of raw meat from %s lies on the ground.", pMobIndex->short_descr );

   act( AT_BLOOD, "$n cuts a slice of meat from $p.", ch, corpse, NULL, TO_ROOM );
   act( AT_BLOOD, "You cut a slice of meat from $p.", ch, corpse, NULL, TO_CHAR );

   obj_to_char( slice, ch );
   corpse->value[3] -= 25;
   return;
}

/*------------------------------------------------------------ 
 *  Fighting Styles - haus
 */
CMDF do_style( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
/*  char buf[MIL];
    int percent; */

   if( IS_NPC( ch ) )
      return;

   one_argument( argument, arg );
   if( arg[0] == '\0' )
   {
      ch_printf( ch, "&wAdopt which fighting style?  (current:  %s&w)\n\r",
                 ch->style == STYLE_BERSERK ? "&Rberserk" :
                 ch->style == STYLE_AGGRESSIVE ? "&Raggressive" :
                 ch->style == STYLE_DEFENSIVE ? "&Ydefensive" : ch->style == STYLE_EVASIVE ? "&Yevasive" : "standard" );
      return;
   }

   if( !str_prefix( arg, "evasive" ) )
   {
      if( ch->level < skill_table[gsn_style_evasive]->skill_level[ch->Class] )
      {
         send_to_char( "You have not yet learned enough to fight evasively.\n\r", ch );
         return;
      }
      WAIT_STATE( ch, skill_table[gsn_style_evasive]->beats );
      if( number_percent(  ) < LEARNED( ch, gsn_style_evasive ) )
      {
         /*
          * success 
          */
         if( ch->fighting )
         {
            ch->position = POS_EVASIVE;
            if( IS_PKILL( ch ) )
               act( AT_ACTION, "$n falls back into an evasive stance.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_EVASIVE;
         send_to_char( "You adopt an evasive fighting style.\n\r", ch );
         return;
      }
      else
      {
         /*
          * failure 
          */
         send_to_char( "You nearly trip in a lame attempt to adopt an evasive fighting style.\n\r", ch );
         learn_from_failure( ch, gsn_style_evasive );
         return;
      }
   }
   else if( !str_prefix( arg, "defensive" ) )
   {
      if( ch->level < skill_table[gsn_style_defensive]->skill_level[ch->Class] )
      {
         send_to_char( "You have not yet learned enough to fight defensively.\n\r", ch );
         return;
      }
      WAIT_STATE( ch, skill_table[gsn_style_defensive]->beats );
      if( number_percent(  ) < LEARNED( ch, gsn_style_defensive ) )
      {
         /*
          * success 
          */
         if( ch->fighting )
         {
            ch->position = POS_DEFENSIVE;
            if( IS_PKILL( ch ) )
               act( AT_ACTION, "$n moves into a defensive posture.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_DEFENSIVE;
         send_to_char( "You adopt a defensive fighting style.\n\r", ch );
         return;
      }
      else
      {
         /*
          * failure 
          */
         send_to_char( "You nearly trip in a lame attempt to adopt a defensive fighting style.\n\r", ch );
         learn_from_failure( ch, gsn_style_defensive );
         return;
      }
   }
   else if( !str_prefix( arg, "standard" ) )
   {
      if( ch->level < skill_table[gsn_style_standard]->skill_level[ch->Class] )
      {
         send_to_char( "You have not yet learned enough to fight in the standard style.\n\r", ch );
         return;
      }
      WAIT_STATE( ch, skill_table[gsn_style_standard]->beats );
      if( number_percent(  ) < LEARNED( ch, gsn_style_standard ) )
      {
         /*
          * success 
          */
         if( ch->fighting )
         {
            ch->position = POS_FIGHTING;
            if( IS_PKILL( ch ) )
               act( AT_ACTION, "$n switches to a standard fighting style.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_FIGHTING;
         send_to_char( "You adopt a standard fighting style.\n\r", ch );
         return;
      }
      else
      {
         /*
          * failure 
          */
         send_to_char( "You nearly trip in a lame attempt to adopt a standard fighting style.\n\r", ch );
         learn_from_failure( ch, gsn_style_standard );
         return;
      }
   }
   else if( !str_prefix( arg, "aggressive" ) )
   {
      if( ch->level < skill_table[gsn_style_aggressive]->skill_level[ch->Class] )
      {
         send_to_char( "You have not yet learned enough to fight aggressively.\n\r", ch );
         return;
      }
      WAIT_STATE( ch, skill_table[gsn_style_aggressive]->beats );
      if( number_percent(  ) < LEARNED( ch, gsn_style_aggressive ) )
      {
         /*
          * success 
          */
         if( ch->fighting )
         {
            ch->position = POS_AGGRESSIVE;
            if( IS_PKILL( ch ) )
               act( AT_ACTION, "$n assumes an aggressive stance.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_AGGRESSIVE;
         send_to_char( "You adopt an aggressive fighting style.\n\r", ch );
         return;
      }
      else
      {
         /*
          * failure 
          */
         send_to_char( "You nearly trip in a lame attempt to adopt an aggressive fighting style.\n\r", ch );
         learn_from_failure( ch, gsn_style_aggressive );
         return;
      }
   }
   else if( !str_prefix( arg, "berserk" ) )
   {
      if( ch->level < skill_table[gsn_style_berserk]->skill_level[ch->Class] )
      {
         send_to_char( "You have not yet learned enough to fight as a berserker.\n\r", ch );
         return;
      }
      WAIT_STATE( ch, skill_table[gsn_style_berserk]->beats );
      if( number_percent(  ) < LEARNED( ch, gsn_style_berserk ) )
      {
         /*
          * success 
          */
         if( ch->fighting )
         {
            ch->position = POS_BERSERK;
            if( IS_PKILL( ch ) )
               act( AT_ACTION, "$n enters a wildly aggressive style.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_BERSERK;
         send_to_char( "You adopt a berserk fighting style.\n\r", ch );
         return;
      }
      else
      {
         /*
          * failure 
          */
         send_to_char( "You nearly trip in a lame attempt to adopt a berserk fighting style.\n\r", ch );
         learn_from_failure( ch, gsn_style_berserk );
         return;
      }
   }

   send_to_char( "Adopt which fighting style?\n\r", ch );

   return;
}

/*
 * Cook was coded by Blackmane and heavily modified by Shaddai
 */
CMDF do_cook( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *food, *fire;
   char arg[MIL];

   one_argument( argument, arg );
   if( IS_NPC( ch ) || ch->level < skill_table[gsn_cook]->skill_level[ch->Class] )
   {
      send_to_char( "That skill is beyond your understanding.\n\r", ch );
      return;
   }
   if( arg[0] == '\0' )
   {
      send_to_char( "Cook what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( food = get_obj_carry( ch, arg ) ) == NULL )
   {
      send_to_char( "You do not have that item.\n\r", ch );
      return;
   }
   if( food->item_type != ITEM_COOK )
   {
      send_to_char( "How can you cook that?\n\r", ch );
      return;
   }
   if( food->value[2] > 2 )
   {
      send_to_char( "That is already burnt to a crisp.\n\r", ch );
      return;
   }
   for( fire = ch->in_room->first_content; fire; fire = fire->next_content )
   {
      if( fire->item_type == ITEM_FIRE )
         break;
   }
   if( !fire )
   {
      send_to_char( "There is no fire here!\n\r", ch );
      return;
   }

   separate_obj( food );   /* Yeah, so you don't end up burning all your meat */

   if( number_percent(  ) > LEARNED( ch, gsn_cook ) )
   {
      food->timer = food->timer / 2;
      food->value[0] = 0;
      food->value[2] = 3;
      act( AT_MAGIC, "$p catches on fire burning it to a crisp!\n\r", ch, food, NULL, TO_CHAR );
      act( AT_MAGIC, "$n catches $p on fire burning it to a crisp.", ch, food, NULL, TO_ROOM );
      stralloc_printf( &food->short_descr, "a burnt %s", food->pIndexData->name );
      stralloc_printf( &food->objdesc, "A burnt %s.", food->pIndexData->name );
      learn_from_failure( ch, gsn_cook );
      return;
   }

   if( number_percent(  ) > 85 )
   {
      food->timer = food->timer * 3;
      food->value[2] += 2;
      act( AT_MAGIC, "$n overcooks $p.", ch, food, NULL, TO_ROOM );
      act( AT_MAGIC, "You overcook $p.", ch, food, NULL, TO_CHAR );
      stralloc_printf( &food->short_descr, "an overcooked %s", food->pIndexData->name );
      stralloc_printf( &food->objdesc, "An overcooked %s.", food->pIndexData->name );
   }
   else
   {
      food->timer = food->timer * 4;
      food->value[0] *= 2;
      act( AT_MAGIC, "$n roasts $p.", ch, food, NULL, TO_ROOM );
      act( AT_MAGIC, "You roast $p.", ch, food, NULL, TO_CHAR );
      stralloc_printf( &food->short_descr, "a roasted %s", food->pIndexData->name );
      stralloc_printf( &food->objdesc, "A roasted %s.", food->pIndexData->name );
      food->value[2]++;
   }
   return;
}

/* Everything from here down has been added by Alsherok */

/* Centaur backheel skill, clone of kick, but needs to be separate for GSN reasons */
CMDF do_backheel( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_backheel]->race_level[ch->race] )
   {
      send_to_char( "You better leave the martial arts to fighters.\n\r", ch );
      return;
   }

   if( !( victim = who_fighting( ch ) ) )
   {
      if( !( victim = get_char_room( ch, argument ) ) )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }
   }

   if( !is_legal_kill( ch, victim ) || is_safe( ch, victim ) )
   {
      send_to_char( "You can't do that!\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_backheel]->beats );
   if( ( IS_NPC( ch ) || number_percent(  ) < ch->pcdata->learned[gsn_backheel] ) && victim->race != RACE_GHOST )
   {
      global_retcode = damage( ch, victim, ch->Class == CLASS_MONK ? ch->level : ( ch->level / 2 ), gsn_backheel );
      kick_messages( ch, victim, 1, global_retcode );
   }
   else
   {
      learn_from_failure( ch, gsn_backheel );
      global_retcode = damage( ch, victim, 0, gsn_backheel );
      kick_messages( ch, victim, 0, global_retcode );
   }
   return;
}

CMDF do_tinker( CHAR_DATA * ch, char *argument )
{
   OBJ_INDEX_DATA *pobj = NULL;
   OBJ_DATA *obj = NULL;
   MOB_INDEX_DATA *pmob = NULL;
   CHAR_DATA *mob = NULL;
   int cost = 10000;

   set_char_color( AT_SKILL, ch );

   if( argument[0] == '\0' || !argument )
   {
      send_to_char( "What do you wish to construct?\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "flamethrower" ) )
      pobj = get_obj_index( 11039 );

   if( !str_cmp( argument, "ladder" ) )
      pobj = get_obj_index( 11040 );

   if( !str_cmp( argument, "digger" ) )
      pobj = get_obj_index( 11041 );

   if( !str_cmp( argument, "lockpick" ) )
      pobj = get_obj_index( 11042 );

   if( !str_cmp( argument, "breather" ) )
      pobj = get_obj_index( 11043 );

   if( !str_cmp( argument, "flyer" ) )
   {
      pmob = get_mob_index( 11010 );
      cost = 20000;
   }

   if( !pobj && !pmob )
   {
      ch_printf( ch, "You cannot construct a %s.\n\r", argument );
      return;
   }

   if( ch->gold < cost )
   {
      send_to_char( "You don't have enough gold for the parts!\n\r", ch );
      return;
   }

   ch->gold -= cost;

   WAIT_STATE( ch, skill_table[gsn_tinker]->beats );
   if( number_percent(  ) > ch->pcdata->learned[gsn_tinker] )
   {
      if( ch->pcdata->learned[gsn_tinker] > 0 )
      {
         send_to_char( "You fiddle around for awhile, but only cost yourself money.\n\r", ch );
         learn_from_failure( ch, gsn_tinker );
      }
      return;
   }

   if( pobj )
   {
      obj = create_object( pobj, 1 );
      obj = obj_to_char( obj, ch );

      ch_printf( ch, "You tinker around awhile and construct %s!\n\r", obj->short_descr );
      return;
   }
   if( pmob )
   {
      mob = create_mobile( pmob );
      if( !char_to_room( mob, ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      ch_printf( ch, "You tinker around awhile and construct a %s!\n\r", mob->short_descr );
      return;
   }
   bug( "%s", "do_tinker: Somehow reached the end of the function!!!" );
   send_to_char( "Oops. Something mighty odd just happened. The imms have been informed.\n\r", ch );
   send_to_char( "Reimbursing the gold you lost...\n\r", ch );
   ch->gold += cost;

   return;
}

CMDF do_deathsong( CHAR_DATA * ch, char *argument )
{
   AFFECT_DATA af;

   set_char_color( AT_SKILL, ch );

   if( IS_AFFECTED( ch, AFF_DEATHSONG ) )
   {
      send_to_char( "You can only use the song once per day.\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_deathsong]->beats );

   af.type = gsn_deathsong;
   af.duration = 2616;  /* One day long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_DEATHSONG;
   affect_to_char( ch, &af );

   if( number_percent(  ) < ch->pcdata->learned[gsn_deathsong] )
   {
      af.type = gsn_deathsong;
      af.duration = ch->level;
      af.modifier = 20;
      af.location = APPLY_PARRY;
      af.bit = AFF_DEATHSONG;
      affect_to_char( ch, &af );

      af.type = gsn_deathsong;
      af.duration = ch->level;
      af.modifier = ch->level / 50;
      af.location = APPLY_DAMROLL;
      af.bit = AFF_DEATHSONG;
      affect_to_char( ch, &af );

      send_to_char( "Your song increases your combat abilities.\n\r", ch );
   }
   else
   {
      if( ch->pcdata->learned[gsn_deathsong] > 0 )
      {
         send_to_char( "Something distracts you and you fumble the words.\n\r", ch );
         learn_from_failure( ch, gsn_deathsong );
      }
   }

   return;
}

CMDF do_tenacity( CHAR_DATA * ch, char *argument )
{
   AFFECT_DATA af;

   set_char_color( AT_SKILL, ch );

   if( IS_AFFECTED( ch, AFF_TENACITY ) )
   {
      send_to_char( "You are already as tenacious as can be!\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_tenacity]->beats );

   if( number_percent(  ) < ch->pcdata->learned[gsn_tenacity] )
   {
      af.type = gsn_tenacity;
      af.duration = ch->level;
      af.modifier = 2;
      af.location = APPLY_HITROLL;
      af.bit = AFF_TENACITY;
      affect_to_char( ch, &af );

      send_to_char( "You psych yourself up into a tenacious frenzy.\n\r", ch );
   }
   else
   {
      if( ch->pcdata->learned[gsn_tenacity] > 0 )
      {
         send_to_char( "You just can't seem to psych yourself up for some reason.\n\r", ch );
         learn_from_failure( ch, gsn_tenacity );
      }
   }

   return;
}

CMDF do_reverie( CHAR_DATA * ch, char *argument )
{
   AFFECT_DATA af;

   set_char_color( AT_SKILL, ch );

   if( IS_AFFECTED( ch, AFF_REVERIE ) )
   {
      send_to_char( "You may only use reverie once per day.\n\r", ch );
      return;
   }

   af.type = gsn_reverie;
   af.duration = 2616;  /* One day long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_REVERIE;
   affect_to_char( ch, &af );

   if( number_percent(  ) < ch->pcdata->learned[gsn_reverie] )
   {
      af.type = gsn_reverie;
      af.duration = ( int )( 2 * DUR_CONV );
      af.modifier = 80;
      af.location = APPLY_HIT_REGEN;
      af.bit = AFF_REVERIE;
      affect_to_char( ch, &af );

      af.type = gsn_reverie;
      af.duration = ( int )( 2 * DUR_CONV );
      af.modifier = 80;
      af.location = APPLY_MANA_REGEN;
      af.bit = AFF_REVERIE;
      affect_to_char( ch, &af );

      send_to_char( "Your song speeds up your regeneration.\n\r", ch );
   }
   else
   {
      if( ch->pcdata->learned[gsn_reverie] > 0 )
      {
         send_to_char( "Something distracts you, causing you to fumble the words.\n\r", ch );
         learn_from_failure( ch, gsn_reverie );
      }
   }

   return;
}

CMDF do_bladesong( CHAR_DATA * ch, char *argument )
{
   AFFECT_DATA af;

   set_char_color( AT_MAGIC, ch );

   if( IS_AFFECTED( ch, AFF_BLADESONG ) )
   {
      send_to_char( "You may only use bladesong once per day.\n\r", ch );
      return;
   }

   af.type = gsn_bladesong;
   af.duration = 2616;  /* One day long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_BLADESONG;
   affect_to_char( ch, &af );

   if( number_percent(  ) < ch->pcdata->learned[gsn_bladesong] )
   {
      af.type = gsn_bladesong;
      af.duration = ( ch->level / 2 );
      af.modifier = 2;
      af.location = APPLY_HITROLL;
      af.bit = AFF_BLADESONG;
      affect_to_char( ch, &af );

      af.type = gsn_bladesong;
      af.duration = ( ch->level / 2 );
      af.modifier = 2;
      af.location = APPLY_DAMROLL;
      af.bit = AFF_BLADESONG;
      affect_to_char( ch, &af );

      af.type = gsn_bladesong;
      af.duration = ( ch->level / 2 );
      af.modifier = 20;
      af.location = APPLY_DODGE;
      af.bit = AFF_BLADESONG;
      affect_to_char( ch, &af );

      send_to_char( "Your song inspires you to heightened ability in combat.\n\r", ch );
   }
   else
   {
      if( ch->pcdata->learned[gsn_bladesong] > 0 )
      {
         send_to_char( "Something distracts you, causing you to fumble the words.\n\r", ch );
         learn_from_failure( ch, gsn_bladesong );
      }
   }

   return;
}

CMDF do_elvensong( CHAR_DATA * ch, char *argument )
{
   AFFECT_DATA af;

   set_char_color( AT_MAGIC, ch );

   if( IS_AFFECTED( ch, AFF_ELVENSONG ) )
   {
      send_to_char( "You may only use Elven song once per day.\n\r", ch );
      return;
   }

   af.type = gsn_elvensong;
   af.duration = 2616;  /* One day long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_ELVENSONG;
   affect_to_char( ch, &af );

   if( number_percent(  ) < ch->pcdata->learned[gsn_elvensong] )
   {
      send_to_char( "Your soothing song regenerates your health and energy.\n\r", ch );

      ch->hit += 40;
      ch->mana += 40;

      if( ch->hit >= ch->max_hit )
         ch->hit = ch->max_hit;
      if( ch->mana >= ch->max_mana )
         ch->mana = ch->max_mana;
   }
   else
   {
      if( ch->pcdata->learned[gsn_elvensong] > 0 )
      {
         send_to_char( "Something distracts you, causing you to fumble the words.\n\r", ch );
         learn_from_failure( ch, gsn_elvensong );
      }
   }
   return;
}

/* Assassinate skill, added by Samson on unknown date. Code courtesy of unknown author from Smaug mailing list. */
CMDF do_assassinate( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   short percent;

   /*
    * Someone is gonna die if I come back later and find this removed again!!! 
    */
   if( ch->Class != CLASS_ROGUE && !IS_IMMORTAL( ch ) )
   {
      send_to_char( "What do you think you are? A Rogue???\n\r", ch );
      return;
   }

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't do that right now.\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   if( arg1[0] == '\0' )
   {
      send_to_char( "Who do you want to assassinate?\n\r", ch );
      return;
   }

   victim = get_char_room( ch, arg1 );
   if( victim == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( IS_IMMORTAL( victim ) && victim->level > ch->level )
   {
      send_to_char( "I don't think so...\n\r", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't get close enough while mounted.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "How can you sneak up on yourself?\n\r", ch );
      return;
   }

   if( is_safe( ch, victim ) )
      return;

   /*
    * Added stabbing weapon. -Narn 
    */
   if( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL )
   {
      send_to_char( "You need to wield a piercing or stabbing weapon.\n\r", ch );
      return;
   }

   if( obj->value[4] != WEP_DAGGER )
   {
      if( ( obj->value[4] == WEP_SWORD && obj->value[3] != DAM_PIERCE ) || obj->value[4] != WEP_SWORD )
      {
         send_to_char( "You need to wield a piercing or stabbing weapon.\n\r", ch );
         return;
      }
   }

   if( victim->fighting )
   {
      send_to_char( "You can't assassinate someone who is in combat.\n\r", ch );
      return;
   }

   /*
    * Can assassinate a char even if it's hurt as long as it's sleeping. -Tsunami 
    */
   if( victim->hit < victim->max_hit && IS_AWAKE( victim ) )
   {
      act( AT_PLAIN, "$N is hurt and suspicious ... you can't sneak up.", ch, NULL, victim, TO_CHAR );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_assassinate]->beats );
   percent = number_percent(  ) + UMAX( 0, ( victim->level - ch->level ) * 2 );
   if( IS_NPC( ch ) || percent < ch->pcdata->learned[gsn_assassinate] )
   {
      act( AT_ACTION, "You slip quietly up behind $N, plunging your", ch, NULL, victim, TO_CHAR );
      act( AT_ACTION, "$p deep into $s back!", ch, obj, victim, TO_CHAR );
      act( AT_ACTION, "Piercing a vital organ, $N falls to the ground, dead.", ch, NULL, victim, TO_CHAR );
      act( AT_ACTION, "$n slips behind you, and before you can react, plunges", ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$s $p deep into your back!", ch, obj, victim, TO_VICT );
      act( AT_ACTION, "You slip quietly off to your death as $e pierces a vital organ.....", ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$n slips quietly up behind $N, plunging $s", ch, NULL, victim, TO_NOTVICT );
      act( AT_ACTION, "$p deep into $S back!", ch, obj, victim, TO_NOTVICT );
      act( AT_ACTION, "$N falls to the ground, dead, after $n pierces a vital organ.....", ch, NULL, victim, TO_NOTVICT );
      check_killer( ch, victim );
      group_gain( ch, victim );
      raw_kill( ch, victim );
   }
   else
   {
      act( AT_MAGIC, "You fail to assassinate $N.", ch, NULL, victim, TO_CHAR );
      act( AT_MAGIC, "$n tried to assassinate you, but luckily $e failed...", ch, NULL, victim, TO_VICT );
      act( AT_MAGIC, "$n tries to assassinate $N but fails.", ch, NULL, victim, TO_NOTVICT );
      learn_from_failure( ch, gsn_assassinate );
      global_retcode = one_hit( ch, victim, TYPE_UNDEFINED );
   }
   return;
}

/* Adapted from Dalemud by Sten */
/* Installed by Samson 10-22-98 - Monk skill */
CMDF do_feign( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\n\r", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_feign]->skill_level[ch->Class] )
   {
      send_to_char( "You better leave the martial arts to fighters.\n\r", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "What?! and fall off your mount?!\n\r", ch );
      return;
   }

   if( !( victim = who_fighting( ch ) ) )
   {
      send_to_char( "You aren't fighting anyone.\n\r", ch );
      return;
   }

   send_to_char( "You try to fake your own demise\n\r", ch );
   death_cry( ch );

   act( AT_SKILL, "$n is dead! R.I.P.", ch, NULL, victim, TO_NOTVICT );

   WAIT_STATE( ch, skill_table[gsn_feign]->beats );
   if( can_use_skill( ch, number_percent(  ), gsn_feign ) )
   {
      stop_fighting( victim, TRUE );
      ch->position = POS_SLEEPING;
      SET_AFFECTED( ch, AFF_HIDE );
      WAIT_STATE( ch, 2 * sysdata.pulseviolence );
   }
   else
   {
      ch->position = POS_SLEEPING;
      WAIT_STATE( ch, 3 * sysdata.pulseviolence );
      learn_from_failure( ch, gsn_feign );
   }
   return;
}

CMDF do_forage( CHAR_DATA * ch, char *argument )
{
   OBJ_INDEX_DATA *herb;
   OBJ_DATA *obj;
   int vnum = 11027;
   int sector;
   short range;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use this skill.\n\r", ch );
      return;
   }

   if( ( !IS_OUTSIDE( ch ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      send_to_char( "You must be outdoors to use this skill.\n\r", ch );
      return;
   }

   if( ch->move < 10 )
   {
      send_to_char( "You do not have enough movement left to forage.\n\r", ch );
      return;
   }

   if( IS_PLR_FLAG( ch, PLR_ONMAP ) )
      sector = map_sector[ch->map][ch->x][ch->y];
   else
      sector = ch->in_room->sector_type;

   switch ( sector )
   {
      case SECT_UNDERWATER:
      case SECT_OCEANFLOOR:
         send_to_char( "You can't spend that kind of time underwater!\n\r", ch );
         return;
      case SECT_RIVER:
         send_to_char( "The river's current is too strong to stay in one spot!\n\r", ch );
         return;
      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:
      case SECT_OCEAN:
         send_to_char( "The water is too deep to see anything here!\n\r", ch );
         return;
      case SECT_AIR:
         send_to_char( "Yeah, sure, forage in thin air???\n\r", ch );
         return;
      case SECT_CITY:
         send_to_char( "This spot is far too well traveled to find anything useful.\n\r", ch );
         return;
      case SECT_ICE:
         send_to_char( "Nothing but ice here buddy.\n\r", ch );
         return;
      case SECT_LAVA:
         send_to_char( "What? You want to barbecue yourself?\n\r", ch );
         return;
      default:
         break;
   }

   range = number_range( 0, 10 );
   vnum += range;

   herb = get_obj_index( vnum );

   if( herb == NULL )
   {
      bug( "do_forage: Cannot locate item for vnum %d", vnum );
      send_to_char( "Oops. Slight bug here. The immortals have been notified.\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_forage]->beats );
   ch->move -= 10;
   if( ch->move < 1 )
      ch->move = 0;

   if( number_percent(  ) < ch->pcdata->learned[gsn_forage] )
   {
      obj = create_object( herb, 1 );
      obj = obj_to_char( obj, ch );
      send_to_char( "After an intense search of the area, your efforts have\n\r", ch );
      ch_printf( ch, "yielded you %s!\n\r", obj->short_descr );
      return;
   }
   else
   {
      send_to_char( "Your search of the area reveals nothing useful.\n\r", ch );
      learn_from_failure( ch, gsn_forage );
   }
   return;
}

CMDF do_woodcall( CHAR_DATA * ch, char *argument )
{
   MOB_INDEX_DATA *call;
   CHAR_DATA *mob;
   int vnum = 11001;
   int sector;
   short range;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use this skill.\n\r", ch );
      return;
   }

   if( ( !IS_OUTSIDE( ch ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      send_to_char( "You must be outdoors to use this skill.\n\r", ch );
      return;
   }

   if( ch->move < 30 )
   {
      send_to_char( "You do not have enough movement left to call forth the animal.\n\r", ch );
      return;
   }

   if( IS_PLR_FLAG( ch, PLR_ONMAP ) )
      sector = map_sector[ch->map][ch->x][ch->y];
   else
      sector = ch->in_room->sector_type;

   switch ( sector )
   {
      case SECT_UNDERWATER:
      case SECT_OCEANFLOOR:
         send_to_char( "You can't call out underwater!\n\r", ch );
         return;
      case SECT_RIVER:
         send_to_char( "The river is too swift for that!\n\r", ch );
         return;
      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:
      case SECT_OCEAN:
         send_to_char( "The water is too deep to call anything here!\n\r", ch );
         return;
      case SECT_AIR:
         send_to_char( "Yeah, sure, in thin air???\n\r", ch );
         return;
      case SECT_CITY:
         send_to_char( "This spot is far too well traveled to attract anything.\n\r", ch );
         return;
      case SECT_ICE:
         send_to_char( "Nothing but ice here buddy.\n\r", ch );
         return;
      case SECT_LAVA:
         send_to_char( "What? You want to barbecue yourself?\n\r", ch );
         return;
      default:
         break;
   }

   range = number_range( 0, 5 );
   vnum += range;

   call = get_mob_index( vnum );

   if( call == NULL )
   {
      bug( "do_woodcall: Cannot locate mob for vnum %d", vnum );
      send_to_char( "Oops. Slight bug here. The immortals have been notified.\n\r", ch );
      return;
   }

   if( !can_charm( ch ) )
   {
      send_to_char( "You already have too many followers to support!\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_woodcall]->beats );
   ch->move -= 30;
   if( ch->move < 1 )
      ch->move = 0;

   if( number_percent(  ) < ch->pcdata->learned[gsn_woodcall] )
   {
      mob = create_mobile( call );
      if( !char_to_room( mob, ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      ch_printf( ch, "&[skill]Your calls attract %s to your side!\n\r", mob->short_descr );
      bind_follower( mob, ch, gsn_woodcall, ch->level * 10 );
      return;
   }
   else
   {
      send_to_char( "Your calls fall on deaf ears, nothing comes forth.\n\r", ch );
      learn_from_failure( ch, gsn_woodcall );
   }
   return;
}

CMDF do_mining( CHAR_DATA * ch, char *argument )
{
   OBJ_INDEX_DATA *ore;
   OBJ_DATA *obj;
   int vnum = 11300;
   short range;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use this skill.\n\r", ch );
      return;
   }

   if( !IS_PLR_FLAG( ch, PLR_ONMAP ) && ch->in_room->sector_type != SECT_MOUNTAIN
       && ch->in_room->sector_type != SECT_UNDERGROUND )
   {
      send_to_char( "You must be in the mountains, or underground to do mining.\n\r", ch );
      return;
   }

   if( IS_PLR_FLAG( ch, PLR_ONMAP ) && map_sector[ch->map][ch->x][ch->y] != SECT_MOUNTAIN )
   {
      send_to_char( "You must be in the mountains to do mining.\n\r", ch );
      return;
   }

   if( ch->move < 40 )
   {
      send_to_char( "You do not have enough movement left to mine.\n\r", ch );
      return;
   }

   range = number_range( 1, 50 );

   if( range < 14 )
      vnum = 11300;
   if( range > 13 && range < 27 )
      vnum = 11301;
   if( range > 26 && range < 41 )
      vnum = 11302;
   if( range > 40 && range < 47 )
      vnum = 11303;
   if( range > 46 )
      vnum = 11304;

   ore = get_obj_index( vnum );

   if( ore == NULL )
   {
      bug( "do_mining: Cannot locate item for vnum %d", vnum );
      send_to_char( "Oops. Slight bug here. The immortals have been notified.\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_mining]->beats );
   ch->move -= 40;
   if( ch->move < 1 )
      ch->move = 0;

   if( number_percent(  ) < ch->pcdata->learned[gsn_mining] )
   {
      obj = create_object( ore, 1 );
      obj = obj_to_char( obj, ch );
      ch_printf( ch, "After some intense mining, you unearth %s!\n\r", obj->short_descr );
      return;
   }
   else
   {
      send_to_char( "Your search of the area reveals nothing useful.\n\r", ch );
      learn_from_failure( ch, gsn_mining );
   }
   return;
}

CMDF do_quiv( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   char arg[MIL];

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs can't use the fabled quivering palm.\n\r", ch );
      return;
   }

   if( ch->Class != CLASS_MONK && !IS_IMMORTAL( ch ) )
   {
      send_to_char( "Yeah, I bet you thought you were a monk.\n\r", ch );
      return;
   }

   if( ch->level < skill_table[gsn_quiv]->skill_level[ch->Class] )
   {
      send_to_char( "You are not yet powerful enough to try that......\n\r", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_QUIV ) )
   {
      send_to_char( "You may only use this attack once per day.\n\r", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "Your mount prevents you from getting close enough.\n\r", ch );
      return;
   }

   one_argument( argument, arg );

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "Who do you wish to use the fabled quivering palm on?\n\r", ch );
      return;
   }

   if( ch == victim )
   {
      send_to_char( "Use quivering palm on yourself? That's ludicrous.\n\r", ch );
      return;
   }

   if( is_safe( ch, victim ) )
      return;

   if( ch->level < victim->level && number_percent(  ) > 15 )
   {
      act( AT_SKILL, "$N's experience prevents your attempt.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( ch->max_hit * 2 < victim->max_hit && number_percent(  ) > 10 )
   {
      act( AT_PLAIN, "$N's might prevents your attempt.", ch, NULL, victim, TO_CHAR );
      return;
   }

   send_to_char( "You begin to work on the vibrations.\n\r", ch );

   af.type = gsn_quiv;
   af.duration = 2616;  /* One day long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_QUIV;

   if( number_percent(  ) < ch->pcdata->learned[gsn_quiv] )
   {
      act( AT_SKILL, "Your hand vibrates intensly as it strikes $N dead!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "The last thing you see before dying is $n's blurred palm!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "$n's palm blurs from sight, and suddenly $N drops dead!", ch, NULL, victim, TO_NOTVICT );
      group_gain( ch, victim );
      raw_kill( ch, victim );
      affect_to_char( ch, &af );
   }
   else
   {
      send_to_char( "Your vibrations fade ineffectively.\n\r", ch );
      learn_from_failure( ch, gsn_quiv );
   }
   return;
}

/* Charge code written by Sadiq - April 28, 1998    *
 * e-mail to MudPrince@aol.com                      */
CMDF do_charge( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *wand;
   int sn;
   int mana;
   int charge;

   if( IS_NPC( ch ) )
      return;

   if( ch->level < skill_table[gsn_charge]->skill_level[ch->Class] )
   {
      send_to_char( "A skill such as this is presently beyond your comprehension.\n\r", ch );
      return;
   }

   if( argument[0] == '\0' || !str_cmp( argument, "" ) )
   {
      send_to_char( "Bind what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( sn = find_spell( ch, argument, TRUE ) ) < 0 )
   {
      send_to_char( "There is no such spell.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "word of recall" ) )
      sn = find_spell( ch, "recall", FALSE );

   if( skill_table[sn]->type != SKILL_SPELL )
   {
      send_to_char( "That's not a spell!\n\r", ch );
      return;
   }

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
   {
      send_to_char( "You have not yet learned that spell.\n\r", ch );
      return;
   }

   if( SPELL_FLAG( skill_table[sn], SF_NOCHARGE ) )
   {
      send_to_char( "You cannot bind that spell.\n\r", ch );
      return;
   }

   mana = UMAX( skill_table[sn]->min_mana, 100 / ( 2 + ch->level - skill_table[sn]->skill_level[ch->Class] ) );

   mana *= 3;

   if( ch->mana < mana )
   {
      send_to_char( "You don't have enough mana.\n\r", ch );
      return;
   }

   if( ( wand = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
   {
      send_to_char( "You must be holding a suitable wand to bind it.\n\r", ch );
      return;
   }

   if( wand->pIndexData->vnum != OBJ_VNUM_WAND_CHARGING )
   {
      send_to_char( "You must be holding a suitable wand to bind it.\n\r", ch );
      return;
   }

   if( wand->value[3] != -1 && wand->pIndexData->vnum == OBJ_VNUM_WAND_CHARGING )
   {
      send_to_char( "That wand has already been bound.\n\r", ch );
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      send_to_char( "The spell fizzles and dies due to a lack of the proper components.\n\r", ch );
      ch->mana -= ( mana / 2 );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_charge]->beats );
   if( !IS_NPC( ch ) && number_percent(  ) > ch->pcdata->learned[gsn_charge] )
   {
      send_to_char( "&[magic]Your spell fails and the distortions in the magic destroy the wand!\n\r", ch );
      learn_from_failure( ch, gsn_charge );
      extract_obj( wand );
      ch->mana -= ( mana / 2 );
      return;
   }

   charge = get_curr_int( ch ) + dice( 1, 5 );

   wand->level = ch->level;
   wand->value[0] = ch->level / 2;
   wand->value[1] = charge;
   wand->value[2] = charge;
   wand->value[3] = sn;
   stralloc_printf( &wand->short_descr, "wand of %s", skill_table[sn]->name );
   stralloc_printf( &wand->objdesc, "A polished wand of '%s' has been left here.", skill_table[sn]->name );
   stralloc_printf( &wand->name, "wand %s", skill_table[sn]->name );
   act( AT_MAGIC, "$n magically charges $p.", ch, wand, NULL, TO_ROOM );
   act( AT_MAGIC, "You magically charge $p.", ch, wand, NULL, TO_CHAR );

   ch->mana -= mana;

   return;
}

CMDF do_tan( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *corpse = NULL, *hide = NULL;
   char itemname[MIL], itemtype[MIL], hidetype[MIL];
   int percent = 0, acapply = 0, acbonus = 0, lev = 0;

   if( IS_NPC( ch ) && !IS_ACT_FLAG( ch, ACT_POLYSELF ) )
      return;

   if( ch->mount )
   {
      send_to_char( "Not from this mount you cannot!\n\r", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_tan]->skill_level[ch->Class] )
   {
      send_to_char( "What do you think you are, A tanner?\n\r", ch );
      return;
   }

   argument = one_argument( argument, itemname );
   argument = one_argument( argument, itemtype );

   if( itemname[0] == '\0' )
   {
      send_to_char( "You may make the following items out of a corpse:\n\r", ch );
      send_to_char
         ( "Jacket, shield, boots, gloves, leggings, sleeves, helmet, bag, belt, cloak, whip,\n\rquiver, waterskin, and collar\n\r",
           ch );
      return;
   }

   if( itemtype[0] == '\0' )
   {
      send_to_char( "I see that, but what do you wanna make?\n\r", ch );
      return;
   }

   if( !( corpse = get_obj_here( ch, itemname ) ) )
   {
      send_to_char( "Where did that carcass go?\n\r", ch );
      return;
   }

   if( corpse->item_type != ITEM_CORPSE_PC && corpse->item_type != ITEM_CORPSE_NPC )
   {
      send_to_char( "That is not a corpse, you cannot tan it.\n\r", ch );
      return;
   }

   if( corpse->value[5] == 1 )
   {
      send_to_char( "There isn't any flesh left on that corpse to tan anything with!\n\r", ch );
      return;
   }

   separate_obj( corpse ); /* No need to destroy ALL corpses of this type */
   percent = number_percent(  );

   if( percent > LEARNED( ch, gsn_tan ) )
   {
      act( AT_PLAIN, "You hack at $p but manage to only destroy the hide.", ch, corpse, NULL, TO_CHAR );
      act( AT_PLAIN, "$n tries to skins $p for its hide, but destroys it.", ch, corpse, NULL, TO_ROOM );
      learn_from_failure( ch, gsn_tan );

      /*
       * Tanning won't destroy what the corpse was carrying - Samson 11-20-99 
       */
      if( corpse->carried_by )
         empty_obj( corpse, NULL, corpse->carried_by->in_room );
      else if( corpse->in_room )
         empty_obj( corpse, NULL, corpse->in_room );

      extract_obj( corpse );
      return;
   }

   lev = corpse->level; /* Why not? The corpse creation code already provided us with this. */

   acbonus += lev / 10;
   acapply += lev / 10;

   if( ch->Class == CLASS_RANGER )
      acbonus += 1;

   acbonus = URANGE( 0, acbonus, 20 );
   acapply = URANGE( 0, acapply, 20 );

   if( !str_cmp( itemtype, "shield" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_SHIELD ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      acapply++;
      acbonus++;
      mudstrlcpy( hidetype, "A shield", MIL );
   }
   else if( !str_cmp( itemtype, "jacket" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_JACKET ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      acapply += 5;
      acbonus += 2;
      mudstrlcpy( hidetype, "A jacket", MIL );
   }
   else if( !str_cmp( itemtype, "boots" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_BOOTS ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      acapply--;
      if( acapply < 0 )
         acapply = 0;
      acbonus--;
      if( acbonus < 0 )
         acbonus = 0;
      mudstrlcpy( hidetype, "Boots", MIL );
   }
   else if( !str_cmp( itemtype, "gloves" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_GLOVES ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      acapply--;
      if( acapply < 0 )
         acapply = 0;
      acbonus--;
      if( acbonus < 0 )
         acbonus = 0;
      mudstrlcpy( hidetype, "Gloves", MIL );
   }
   else if( !str_cmp( itemtype, "leggings" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_LEGGINGS ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      acapply++;
      acbonus++;
      mudstrlcpy( hidetype, "Leggings", MIL );
   }
   else if( !str_cmp( itemtype, "sleeves" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_SLEEVES ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      acapply++;
      acbonus++;
      mudstrlcpy( hidetype, "Sleeves", MIL );
   }
   else if( !str_cmp( itemtype, "helmet" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_HELMET ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      acapply--;
      if( acapply < 0 )
         acapply = 0;
      acbonus--;
      if( acbonus < 0 )
         acbonus = 0;
      mudstrlcpy( hidetype, "A helmet", MIL );
   }
   else if( !str_cmp( itemtype, "bag" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_BAG ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      mudstrlcpy( hidetype, "A bag", MIL );
   }
   else if( !str_cmp( itemtype, "belt" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_BELT ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      acapply--;
      if( acapply < 0 )
         acapply = 0;
      acbonus--;
      if( acbonus < 0 )
         acbonus = 0;
      mudstrlcpy( hidetype, "A belt", MIL );
   }
   else if( !str_cmp( itemtype, "cloak" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_CLOAK ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      acapply--;
      if( acapply < 0 )
         acapply = 0;
      acbonus--;
      if( acbonus < 0 )
         acbonus = 0;
      mudstrlcpy( hidetype, "A cloak", MIL );
   }
   else if( !str_cmp( itemtype, "quiver" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_QUIVER ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      mudstrlcpy( hidetype, "A quiver", MIL );
   }
   else if( !str_cmp( itemtype, "waterskin" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_WATERSKIN ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      mudstrlcpy( hidetype, "A waterskin", MIL );
   }
   else if( !str_cmp( itemtype, "collar" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_COLLAR ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      acapply--;
      if( acapply < 0 )
         acapply = 0;
      acbonus--;
      if( acbonus < 0 )
         acbonus = 0;
      mudstrlcpy( hidetype, "A collar", MIL );
   }
   else if( !str_cmp( itemtype, "whip" ) )
   {
      if( !( hide = create_object( get_obj_index( OBJ_VNUM_TAN_WHIP ), 0 ) ) )
      {
         send_to_char( "Ooops. Bug. The immortals have been notified.\n\r", ch );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      acapply++;
      mudstrlcpy( hidetype, "A whip", MIL );
   }
   else
   {
      send_to_char( "Illegal type of equipment!\n\r", ch );
      return;
   }
   if( !hide )
   {
      bug( "%s", "Tan objects missing." );
      send_to_char( "You messed up the hide and it's useless.\n\r", ch );
      return;
   }
   obj_to_char( hide, ch );

   stralloc_printf( &hide->name, "%s", hidetype );
   stralloc_printf( &hide->short_descr, "%s made from the hide of %s", hidetype, corpse->short_descr + 14 );
   stralloc_printf( &hide->objdesc, "%s made from the hide of %s lies here.", hidetype, corpse->short_descr + 14 );

   if( hide->item_type == ITEM_ARMOR )
   {
      hide->value[0] = acapply;
      hide->value[1] = acapply;
   }
   else if( hide->item_type == ITEM_WEAPON )
   {
      hide->value[0] = acapply;
      hide->value[1] = ( acapply / 10 ) + 1;
      hide->value[2] = acapply * 2;
   }
   else if( hide->item_type == ITEM_CONTAINER )
      hide->value[0] = acapply + 25;
   else if( hide->item_type == ITEM_DRINK_CON )
      hide->value[0] = acapply + 10;

   act_printf( AT_PLAIN, ch, corpse, NULL, TO_CHAR, "You carve %s from $p.", hide->name );
   act_printf( AT_PLAIN, ch, corpse, NULL, TO_ROOM, "$n carves %s from $p.", hide->name );

   /*
    * Tanning won't destroy what the corpse was carrying - Samson 11-20-99 
    */
   if( corpse->carried_by )
      empty_obj( corpse, NULL, corpse->carried_by->in_room );
   else if( corpse->in_room )
      empty_obj( corpse, NULL, corpse->in_room );

   extract_obj( corpse );

   return;
}
