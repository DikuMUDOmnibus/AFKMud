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
 *                          Spell handling module                           *
 ****************************************************************************/

#include <string.h>
#include <ctype.h>
#if defined(__CYGWIN__) || defined(__OpenBSD__) || defined(__FreeBSD__)
#include <sys/time.h>
#endif
#include "mud.h"
#include "clans.h"
#include "fight.h"
#include "liquids.h"
#include "overland.h"
#include "polymorph.h"

int astral_target;   /* Added for Astral Walk spell - Samson */

SPELLF spell_null( int sn, int level, CHAR_DATA * ch, void *vo );
void remove_visit( CHAR_DATA * ch, ROOM_INDEX_DATA * room );   /* For farsight */
LIQ_TABLE *get_liq_vnum( int vnum );
void adjust_favor( CHAR_DATA * ch, int field, int mod );
bool is_safe( CHAR_DATA * ch, CHAR_DATA * victim );
bool check_illegal_pk( CHAR_DATA * ch, CHAR_DATA * victim );
bool in_arena( CHAR_DATA * ch );
void start_hunting( CHAR_DATA * ch, CHAR_DATA * victim );
void start_hating( CHAR_DATA * ch, CHAR_DATA * victim );
void start_timer( struct timeval *starttime );
time_t end_timer( struct timeval *endtime );
char *wear_bit_name( int wear_flags );
int recall( CHAR_DATA * ch, int target );
bool circle_follow( CHAR_DATA * ch, CHAR_DATA * victim );
void add_follower( CHAR_DATA * ch, CHAR_DATA * master );
void stop_follower( CHAR_DATA * ch );
int interpolate( int level, int value_00, int value_32 );
MORPH_DATA *find_morph( CHAR_DATA * ch, char *target, bool is_cast );
void raw_kill( CHAR_DATA * ch, CHAR_DATA * victim );
void affect_modify( CHAR_DATA * ch, AFFECT_DATA * paf, bool fAdd );
ch_ret check_room_for_traps( CHAR_DATA * ch, int flag );
ROOM_INDEX_DATA *recall_room( CHAR_DATA * ch );
bool beacon_check( CHAR_DATA * ch, ROOM_INDEX_DATA * beacon );

int EqWBits( CHAR_DATA * ch, int bit )
{
   OBJ_DATA *obj;

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
      if( obj->wear_loc != WEAR_NONE && IS_OBJ_FLAG( obj, bit ) )
         return 1;

   return 0;
}

/*
 * Is immune to a damage type
 */
bool is_immune( CHAR_DATA *ch, short damtype )
{
   switch( damtype )
   {
      case SD_FIRE:           return( IS_IMMUNE( ch, RIS_FIRE ) );
      case SD_COLD:           return( IS_IMMUNE( ch, RIS_COLD ) );
      case SD_ELECTRICITY:    return( IS_IMMUNE( ch, RIS_ELECTRICITY ) );
      case SD_ENERGY:         return( IS_IMMUNE( ch, RIS_ENERGY ) );
      case SD_ACID:           return( IS_IMMUNE( ch, RIS_ACID ) );
      case SD_POISON:         return( IS_IMMUNE( ch, RIS_POISON ) );
      case SD_DRAIN:          return( IS_IMMUNE( ch, RIS_DRAIN ) );
   }
   return FALSE;
}

/*
 * Lookup an herb by name.
 */
int herb_lookup( const char *name )
{
   int sn;

   for( sn = 0; sn < top_herb; sn++ )
   {
      if( !herb_table[sn] || !herb_table[sn]->name )
         return -1;
      if( LOWER( name[0] ) == LOWER( herb_table[sn]->name[0] ) && !str_prefix( name, herb_table[sn]->name ) )
         return sn;
   }
   return -1;
}

/*
 * Perform a binary search on a section of the skill table	-Thoric
 * Each different section of the skill table is sorted alphabetically
 *
 * Check for prefix matches
 */
int bsearch_skill_prefix( const char *name, int first, int top )
{
   int sn;

   for( ;; )
   {
      sn = ( first + top ) >> 1;

      if( !IS_VALID_SN( sn ) )
         return -1;

      if( LOWER( name[0] ) == LOWER( skill_table[sn]->name[0] ) && !str_prefix( name, skill_table[sn]->name ) )
         return sn;
      if( first >= top )
         return -1;
      if( strcmp( name, skill_table[sn]->name ) < 1 )
         top = sn - 1;
      else
         first = sn + 1;
   }
}

/*
 * Perform a binary search on a section of the skill table	-Thoric
 * Each different section of the skill table is sorted alphabetically
 *
 * Check for exact matches only
 */
int bsearch_skill_exact( const char *name, int first, int top )
{
   int sn;

   for( ;; )
   {
      sn = ( first + top ) >> 1;

      if( !IS_VALID_SN( sn ) )
         return -1;
      if( !str_cmp( name, skill_table[sn]->name ) )
         return sn;
      if( first >= top )
         return -1;
      if( strcmp( name, skill_table[sn]->name ) < 1 )
         top = sn - 1;
      else
         first = sn + 1;
   }
}

/*
 * Perform a binary search on a section of the skill table	-Thoric
 * Each different section of the skill table is sorted alphabetically
 *
 * Check exact match first, then a prefix match
 */
int bsearch_skill( const char *name, int first, int top )
{
   int sn = bsearch_skill_exact( name, first, top );

   return ( sn == -1 ) ? bsearch_skill_prefix( name, first, top ) : sn;
}

/*
 * Lookup a skill by name.
 */
int skill_lookup( const char *name )
{
   int sn;

   if( ( sn = bsearch_skill_exact( name, gsn_first_spell, gsn_first_skill - 1 ) ) == -1 )
      if( ( sn = bsearch_skill_exact( name, gsn_first_skill, gsn_first_weapon - 1 ) ) == -1 )
         if( ( sn = bsearch_skill_exact( name, gsn_first_weapon, gsn_first_tongue - 1 ) ) == -1 )
            if( ( sn = bsearch_skill_exact( name, gsn_first_tongue, gsn_first_ability - 1 ) ) == -1 )
               if( ( sn = bsearch_skill_exact( name, gsn_first_ability, gsn_first_lore - 1 ) ) == -1 )
                  if( ( sn = bsearch_skill_exact( name, gsn_first_lore, gsn_top_sn - 1 ) ) == -1 )
                     if( ( sn = bsearch_skill_prefix( name, gsn_first_spell, gsn_first_skill - 1 ) ) == -1 )
                        if( ( sn = bsearch_skill_prefix( name, gsn_first_skill, gsn_first_weapon - 1 ) ) == -1 )
                           if( ( sn = bsearch_skill_prefix( name, gsn_first_weapon, gsn_first_tongue - 1 ) ) == -1 )
                              if( ( sn = bsearch_skill_prefix( name, gsn_first_tongue, gsn_first_ability - 1 ) ) == -1 )
                                 if( ( sn = bsearch_skill_prefix( name, gsn_first_ability, gsn_first_lore - 1 ) ) == -1 )
                                    if( ( sn = bsearch_skill_prefix( name, gsn_first_lore, gsn_top_sn - 1 ) ) == -1
                                        && gsn_top_sn < top_sn )
                                    {
                                       for( sn = gsn_top_sn; sn < top_sn; sn++ )
                                       {
                                          if( !skill_table[sn] || !skill_table[sn]->name )
                                             return -1;
                                          if( LOWER( name[0] ) == LOWER( skill_table[sn]->name[0] )
                                              && !str_prefix( name, skill_table[sn]->name ) )
                                             return sn;
                                       }
                                       return -1;
                                    }
   return sn;
}

/*
 * Return a skilltype pointer based on sn			-Thoric
 * Returns NULL if bad, unused or personal sn.
 */
SKILLTYPE *get_skilltype( int sn )
{
   if( sn >= TYPE_PERSONAL )
      return NULL;
   if( sn >= TYPE_HERB )
      return IS_VALID_HERB( sn - TYPE_HERB ) ? herb_table[sn - TYPE_HERB] : NULL;
   if( sn >= TYPE_HIT )
      return NULL;
   return IS_VALID_SN( sn ) ? skill_table[sn] : NULL;
}

/*
 * Perform a binary search on a section of the skill table
 * Each different section of the skill table is sorted alphabetically
 * Only match skills player knows				-Thoric
 */
int ch_bsearch_skill_prefix( CHAR_DATA * ch, const char *name, int first, int top )
{
   int sn;

   for( ;; )
   {
      sn = ( first + top ) >> 1;

      if( LOWER( name[0] ) == LOWER( skill_table[sn]->name[0] ) && !str_prefix( name, skill_table[sn]->name )
          && ch->pcdata->learned[sn] > 0 && ch->level >= skill_table[sn]->skill_level[ch->Class] )
         return sn;
      if( first >= top )
         return -1;
      if( strcmp( name, skill_table[sn]->name ) < 1 )
         top = sn - 1;
      else
         first = sn + 1;
   }
}

int ch_bsearch_skill_exact( CHAR_DATA * ch, const char *name, int first, int top )
{
   int sn;

   for( ;; )
   {
      sn = ( first + top ) >> 1;

      if( !str_cmp( name, skill_table[sn]->name )
          && ch->pcdata->learned[sn] > 0 && ch->level >= skill_table[sn]->skill_level[ch->Class] )
         return sn;
      if( first >= top )
         return -1;
      if( strcmp( name, skill_table[sn]->name ) < 1 )
         top = sn - 1;
      else
         first = sn + 1;
   }
}

int ch_bsearch_skill( CHAR_DATA * ch, const char *name, int first, int top )
{
   int sn = ch_bsearch_skill_exact( ch, name, first, top );

   return ( sn == -1 ) ? ch_bsearch_skill_prefix( ch, name, first, top ) : sn;
}

int find_spell( CHAR_DATA * ch, const char *name, bool know )
{
   if( IS_NPC( ch ) || !know )
      return bsearch_skill( name, gsn_first_spell, gsn_first_skill - 1 );
   else
      return ch_bsearch_skill( ch, name, gsn_first_spell, gsn_first_skill - 1 );
}

/*
 * Lookup a skill by slot number.
 * Used for object loading.
 */
int slot_lookup( int slot )
{
   int sn;

   if( slot <= 0 )
      return -1;

   for( sn = 0; sn < top_sn; sn++ )
      if( slot == skill_table[sn]->slot )
         return sn;

   if( fBootDb )
      bug( "%s: bad slot %d.", __FUNCTION__, slot );

   return -1;
}

/*
 * Fancy message handling for a successful casting		-Thoric
 */
void successful_casting( SKILLTYPE * skill, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj )
{
   short chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
   short chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
   short chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

   if( skill->target != TAR_CHAR_OFFENSIVE )
   {
      chit = chitroom;
      chitme = chitroom;
   }

   if( ch && ch != victim )
   {
      if( skill->hit_char && skill->hit_char[0] != '\0' )
         act( chit, skill->hit_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL )
         act( chit, "Ok.", ch, NULL, NULL, TO_CHAR );
   }
   if( ch && skill->hit_room && skill->hit_room[0] != '\0' )
      act( chitroom, skill->hit_room, ch, obj, victim, TO_NOTVICT );
   if( ch && victim && skill->hit_vict && skill->hit_vict[0] != '\0' )
   {
      if( ch != victim )
         act( chitme, skill->hit_vict, ch, obj, victim, TO_VICT );
      else
         act( chitme, skill->hit_vict, ch, obj, victim, TO_CHAR );
   }
   else if( ch && ch == victim && skill->type == SKILL_SPELL )
      act( chitme, "Ok.", ch, NULL, NULL, TO_CHAR );
   else if( ch && ch == victim && skill->type == SKILL_SKILL )
   {
      if( skill->hit_char && ( skill->hit_char[0] != '\0' ) )
         act( chit, skill->hit_char, ch, obj, victim, TO_CHAR );
      else
         act( chit, "Ok.", ch, NULL, NULL, TO_CHAR );
   }
}

/*
 * Fancy message handling for a failed casting			-Thoric
 */
void failed_casting( SKILLTYPE * skill, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj )
{
   short chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
   short chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
   short chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

   if( skill->target != TAR_CHAR_OFFENSIVE )
   {
      chit = chitroom;
      chitme = chitroom;
   }

   if( ch && ch != victim )
   {
      if( skill->miss_char && skill->miss_char[0] != '\0' )
         act( chit, skill->miss_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL )
         act( chit, "You failed.", ch, NULL, NULL, TO_CHAR );
   }
   if( ch && skill->miss_room && skill->miss_room[0] != '\0' && str_cmp( skill->miss_room, "supress" ) )
      act( chitroom, skill->miss_room, ch, obj, victim, TO_NOTVICT );

   if( ch && victim && skill->miss_vict && skill->miss_vict[0] != '\0' )
   {
      if( ch != victim )
         act( chitme, skill->miss_vict, ch, obj, victim, TO_VICT );
      else
         act( chitme, skill->miss_vict, ch, obj, victim, TO_CHAR );
   }
   else if( ch && ch == victim )
   {
      if( skill->miss_char && skill->miss_char[0] != '\0' )
         act( chitme, skill->miss_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL )
         act( chitme, "You failed.", ch, NULL, NULL, TO_CHAR );
   }
}

/*
 * Fancy message handling for being immune to something		-Thoric
 */
void immune_casting( SKILLTYPE * skill, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj )
{
   short chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
   short chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
   short chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

   if( skill->target != TAR_CHAR_OFFENSIVE )
   {
      chit = chitroom;
      chitme = chitroom;
   }

   if( ch && ch != victim )
   {
      if( skill->imm_char && skill->imm_char[0] != '\0' )
         act( chit, skill->imm_char, ch, obj, victim, TO_CHAR );
      else if( skill->miss_char && skill->miss_char[0] != '\0' )
         act( chit, skill->miss_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL )
         act( chit, "That appears to have no effect.", ch, NULL, NULL, TO_CHAR );
   }
   if( ch && skill->imm_room && skill->imm_room[0] != '\0' )
      act( chitroom, skill->imm_room, ch, obj, victim, TO_NOTVICT );
   else if( ch && skill->miss_room && skill->miss_room[0] != '\0' )
      act( chitroom, skill->miss_room, ch, obj, victim, TO_NOTVICT );
   if( ch && victim && skill->imm_vict && skill->imm_vict[0] != '\0' )
   {
      if( ch != victim )
         act( chitme, skill->imm_vict, ch, obj, victim, TO_VICT );
      else
         act( chitme, skill->imm_vict, ch, obj, victim, TO_CHAR );
   }
   else if( ch && victim && skill->miss_vict && skill->miss_vict[0] != '\0' )
   {
      if( ch != victim )
         act( chitme, skill->miss_vict, ch, obj, victim, TO_VICT );
      else
         act( chitme, skill->miss_vict, ch, obj, victim, TO_CHAR );
   }
   else if( ch && ch == victim )
   {
      if( skill->imm_char && skill->imm_char[0] != '\0' )
         act( chit, skill->imm_char, ch, obj, victim, TO_CHAR );
      else if( skill->miss_char && skill->miss_char[0] != '\0' )
         act( chit, skill->miss_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL )
         act( chit, "That appears to have no affect.", ch, NULL, NULL, TO_CHAR );
   }
}

/*
 * Utter mystical words for an sn.
 */
void say_spell( CHAR_DATA * ch, int sn )
{
   char buf[MSL], buf2[MSL];
   CHAR_DATA *rch;
   char *pName;
   int iSyl, length;
   SKILLTYPE *skill = get_skilltype( sn );

   struct syl_type
   {
      char *old;
      char *snew;
   };

   static const struct syl_type syl_table[] = {
      {" ", " "},
      {"ar", "abra"},
      {"au", "kada"},
      {"bless", "fido"},
      {"blind", "nose"},
      {"bur", "mosa"},
      {"cu", "judi"},
      {"de", "oculo"},
      {"en", "unso"},
      {"light", "dies"},
      {"lo", "hi"},
      {"mor", "zak"},
      {"move", "sido"},
      {"ness", "lacri"},
      {"ning", "illa"},
      {"per", "duda"},
      {"polymorph", "iaddahs"},
      {"ra", "gru"},
      {"re", "candus"},
      {"son", "sabru"},
      {"tect", "infra"},
      {"tri", "cula"},
      {"ven", "nofo"},
      {"a", "a"}, {"b", "b"}, {"c", "q"}, {"d", "e"},
      {"e", "z"}, {"f", "y"}, {"g", "o"}, {"h", "p"},
      {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"},
      {"m", "w"}, {"n", "i"}, {"o", "a"}, {"p", "s"},
      {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"},
      {"u", "j"}, {"v", "z"}, {"w", "x"}, {"x", "n"},
      {"y", "l"}, {"z", "k"},
      {"", ""}
   };

   buf[0] = '\0';
   for( pName = skill->name; *pName != '\0'; pName += length )
   {
      for( iSyl = 0; ( length = strlen( syl_table[iSyl].old ) ) != 0; iSyl++ )
      {
         if( !str_prefix( syl_table[iSyl].old, pName ) )
         {
            mudstrlcat( buf, syl_table[iSyl].snew, MSL );
            break;
         }
      }
      if( length == 0 )
         length = 1;
   }

   if( ch->Class == CLASS_BARD )
   {
      mudstrlcpy( buf2, "$n plays a song.", MSL );
      snprintf( buf, MSL, "$n plays the song, '%s'.", skill->name );
   }

   else
   {
      snprintf( buf2, MSL, "$n utters the words, '%s'.", buf );
      snprintf( buf, MSL, "$n utters the words, '%s'.", skill->name );
   }

   for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
   {
      if( rch != ch )
      {
         if( is_same_map( ch, rch ) )
            act( AT_MAGIC, ch->Class == rch->Class ? buf : buf2, ch, NULL, rch, TO_VICT );
      }
   }
   return;
}

/*
 * Make adjustments to saving throw based in RIS		-Thoric
 */
int ris_save( CHAR_DATA * ch, int rchance, int ris )
{
   short modifier;

   modifier = 10;
   if( IS_IMMUNE( ch, ris ) || IS_ABSORB( ch, ris ) )
      return 1000;
   if( IS_RESIS( ch, ris ) )
      modifier -= 2;
   if( IS_SUSCEP( ch, ris ) )
   {
      if( IS_NPC( ch ) && IS_IMMUNE( ch, ris ) )
         modifier += 0;
      else
         modifier += 2;
   }
   if( modifier <= 0 )
      return 1000;
   if( modifier == 10 )
      return rchance;
   return ( rchance * modifier ) / 10;
}

/*								    -Thoric
 * Fancy dice expression parsing complete with order of operations,
 * simple exponent support, dice support as well as a few extra
 * variables: L = level, H = hp, M = mana, V = move, S = str, X = dex
 *            I = int, W = wis, C = con, A = cha, U = luck, A = age
 *
 * Used for spell dice parsing, ie: 3d8+L-6
 *
 */
int rd_parse( CHAR_DATA * ch, int level, char *pexp )
{
   int lop = 0, gop = 0, eop = 0;
   unsigned int x, len = 0;
   char operation;
   char *sexp[2];
   int total = 0;

   /*
    * take care of nulls coming in 
    */
   if( !pexp || !strlen( pexp ) )
      return 0;

   /*
    * get rid of brackets if they surround the entire expresion
    */
   if( ( *pexp == '(' ) && pexp[strlen( pexp ) - 1] == ')' )
   {
      pexp[strlen( pexp ) - 1] = '\0';
      ++pexp;
   }

   /*
    * check if the expresion is just a number 
    */
   len = strlen( pexp );
   if( len == 1 && isalpha( pexp[0] ) )
   {
      switch ( pexp[0] )
      {
         case 'L':
         case 'l':
            return level;
         case 'H':
         case 'h':
            return ch->hit;
         case 'M':
         case 'm':
            return ch->mana;
         case 'V':
         case 'v':
            return ch->move;
         case 'S':
         case 's':
            return get_curr_str( ch );
         case 'I':
         case 'i':
            return get_curr_int( ch );
         case 'W':
         case 'w':
            return get_curr_wis( ch );
         case 'X':
         case 'x':
            return get_curr_dex( ch );
         case 'C':
         case 'c':
            return get_curr_con( ch );
         case 'A':
         case 'a':
            return get_curr_cha( ch );
         case 'U':
         case 'u':
            return get_curr_lck( ch );
         case 'Y':
         case 'y':
            return get_age( ch );
      }
   }

   for( x = 0; x < len; ++x )
      if( !isdigit( pexp[x] ) && !isspace( pexp[x] ) )
         break;
   if( x == len )
      return atoi( pexp );

   /*
    * break it into 2 parts 
    */
   for( x = 0; x < strlen( pexp ); ++x )
      switch ( pexp[x] )
      {
         case '^':
            if( !total )
               eop = x;
            break;
         case '-':
         case '+':
            if( !total )
               lop = x;
            break;
         case '*':
         case '/':
         case '%':
         case 'd':
         case 'D':
         case '<':
         case '>':
         case '{':
         case '}':
         case '=':
            if( !total )
               gop = x;
            break;
         case '(':
            ++total;
            break;
         case ')':
            --total;
            break;
      }
   if( lop )
      x = lop;
   else if( gop )
      x = gop;
   else
      x = eop;
   operation = pexp[x];
   pexp[x] = '\0';
   sexp[0] = pexp;
   sexp[1] = ( char * )( pexp + x + 1 );

   /*
    * work it out 
    */
   total = rd_parse( ch, level, sexp[0] );

   switch ( operation )
   {
      case '-':
         total -= rd_parse( ch, level, sexp[1] );
         break;
      case '+':
         total += rd_parse( ch, level, sexp[1] );
         break;
      case '*':
         total *= rd_parse( ch, level, sexp[1] );
         break;
      case '/':
         total /= rd_parse( ch, level, sexp[1] );
         break;
      case '%':
         total %= rd_parse( ch, level, sexp[1] );
         break;
      case 'd':
      case 'D':
         total = dice( total, rd_parse( ch, level, sexp[1] ) );
         break;
      case '<':
         total = ( total < rd_parse( ch, level, sexp[1] ) );
         break;
      case '>':
         total = ( total > rd_parse( ch, level, sexp[1] ) );
         break;
      case '=':
         total = ( total == rd_parse( ch, level, sexp[1] ) );
         break;
      case '{':
         total = UMIN( total, rd_parse( ch, level, sexp[1] ) );
         break;
      case '}':
         total = UMAX( total, rd_parse( ch, level, sexp[1] ) );
         break;

      case '^':
      {
         unsigned int y = rd_parse( ch, level, sexp[1] ), z = total;

         for( x = 1; x < y; ++x, z *= total );
         total = z;
         break;
      }
   }

   return total;
}

/* wrapper function so as not to destroy exp */
int dice_parse( CHAR_DATA * ch, int level, char *xexp )
{
   char buf[MIL];

   mudstrlcpy( buf, xexp, MIL );
   return rd_parse( ch, level, buf );
}

/*
 * Compute a saving throw.
 * Negative apply's make saving throw better.
 */
bool saves_poison_death( int level, CHAR_DATA * victim )
{
   int save;

   save = LEVEL_AVATAR + ( victim->level - level - victim->saving_poison_death ) * 5;
   save = URANGE( 5, save, 95 );
   return chance( victim, save );
}

bool saves_wands( int level, CHAR_DATA * victim )
{
   int save;

   if( IS_IMMUNE( victim, RIS_MAGIC ) )
      return TRUE;

   save = LEVEL_AVATAR + ( victim->level - level - victim->saving_wand ) * 5;
   save = URANGE( 5, save, 95 );
   return chance( victim, save );
}

bool saves_para_petri( int level, CHAR_DATA * victim )
{
   int save;

   save = LEVEL_AVATAR + ( victim->level - level - victim->saving_para_petri ) * 5;
   save = URANGE( 5, save, 95 );
   return chance( victim, save );
}

bool saves_breath( int level, CHAR_DATA * victim )
{
   int save;

   save = LEVEL_AVATAR + ( victim->level - level - victim->saving_breath ) * 5;
   save = URANGE( 5, save, 95 );
   return chance( victim, save );
}

bool saves_spell_staff( int level, CHAR_DATA * victim )
{
   int save;

   if( IS_IMMUNE( victim, RIS_MAGIC ) )
      return TRUE;

   if( IS_NPC( victim ) && level > 10 )
      level -= 5;
   save = LEVEL_AVATAR + ( victim->level - level - victim->saving_spell_staff ) * 5;
   save = URANGE( 5, save, 95 );
   return chance( victim, save );
}

/*
 * Process the spell's required components, if any		-Thoric
 * -----------------------------------------------
 * T###		check for item of type ###
 * V#####	check for item of vnum #####
 * Kword	check for item with keyword 'word'
 * G#####	check if player has ##### amount of gold
 * H####	check if player has #### amount of hitpoints
 *
 * Special operators:
 * ! spell fails if player has this
 * + don't consume this component
 * @ decrease component's value[0], and extract if it reaches 0
 * # decrease component's value[1], and extract if it reaches 0
 * $ decrease component's value[2], and extract if it reaches 0
 * % decrease component's value[3], and extract if it reaches 0
 * ^ decrease component's value[4], and extract if it reaches 0
 * & decrease component's value[5], and extract if it reaches 0
 */
bool process_spell_components( CHAR_DATA * ch, int sn )
{
   SKILLTYPE *skill = get_skilltype( sn );
   char *comp = skill->components;
   char *check;
   char arg[MIL];
   bool consume, fail, found;
   int val, value;
   OBJ_DATA *obj;

   /*
    * if no components necessary, then everything is cool 
    */
   if( !comp || comp[0] == '\0' )
      return TRUE;

   while( comp[0] != '\0' )
   {
      comp = one_argument( comp, arg );
      consume = TRUE;
      fail = found = FALSE;
      val = -1;
      switch ( arg[1] )
      {
         default:
            check = arg + 1;
            break;
         case '!':
            check = arg + 2;
            fail = TRUE;
            break;
         case '+':
            check = arg + 2;
            consume = FALSE;
            break;
         case '@':
            check = arg + 2;
            val = 0;
            break;
         case '#':
            check = arg + 2;
            val = 1;
            break;
         case '$':
            check = arg + 2;
            val = 2;
            break;
         case '%':
            check = arg + 2;
            val = 3;
            break;
         case '^':
            check = arg + 2;
            val = 4;
            break;
         case '&':
            check = arg + 2;
            val = 5;
            break;
            /*
             * reserve '*', '(' and ')' for v6, v7 and v8   
             */
      }
      value = atoi( check );
      obj = NULL;
      switch ( UPPER( arg[0] ) )
      {
         case 'T':
            for( obj = ch->first_carrying; obj; obj = obj->next_content )
               if( obj->item_type == value )
               {
                  if( fail )
                  {
                     send_to_char( "Something disrupts the casting of this spell...\n\r", ch );
                     return FALSE;
                  }
                  found = TRUE;
                  break;
               }
            break;
         case 'V':
            for( obj = ch->first_carrying; obj; obj = obj->next_content )
               if( obj->pIndexData->vnum == value )
               {
                  if( fail )
                  {
                     send_to_char( "Something disrupts the casting of this spell...\n\r", ch );
                     return FALSE;
                  }
                  found = TRUE;
                  break;
               }
            break;
         case 'K':
            for( obj = ch->first_carrying; obj; obj = obj->next_content )
               if( nifty_is_name( check, obj->name ) )
               {
                  if( fail )
                  {
                     send_to_char( "Something disrupts the casting of this spell...\n\r", ch );
                     return FALSE;
                  }
                  found = TRUE;
                  break;
               }
            break;
         case 'G':
            if( ch->gold >= value )
            {
               if( fail )
               {
                  send_to_char( "Something disrupts the casting of this spell...\n\r", ch );
                  return FALSE;
               }
               else
               {
                  if( consume )
                  {
                     send_to_char( "&[gold]You feel a little lighter...\n\r", ch );
                     ch->gold -= value;
                  }
                  continue;
               }
            }
            break;
         case 'H':
            if( ch->hit >= value )
            {
               if( fail )
               {
                  send_to_char( "Something disrupts the casting of this spell...\n\r", ch );
                  return FALSE;
               }
               else
               {
                  if( consume )
                  {
                     send_to_char( "&[blood]You feel a little weaker...\n\r", ch );
                     ch->hit -= value;
                     update_pos( ch );
                  }
                  continue;
               }
            }
            break;
      }
      /*
       * having this component would make the spell fail... if we get
       * here, then the caster didn't have that component 
       */
      if( fail )
         continue;
      if( !found )
      {
         send_to_char( "Something is missing...\n\r", ch );
         return FALSE;
      }
      if( obj )
      {
         if( val >= 0 && val < 6 )
         {
            separate_obj( obj );
            if( obj->value[val] <= 0 )
            {
               act( AT_MAGIC, "$p disappears in a puff of smoke!", ch, obj, NULL, TO_CHAR );
               act( AT_MAGIC, "$p disappears in a puff of smoke!", ch, obj, NULL, TO_ROOM );
               extract_obj( obj );
               return FALSE;
            }
            else if( --obj->value[val] == 0 )
            {
               act( AT_MAGIC, "$p glows briefly, then disappears in a puff of smoke!", ch, obj, NULL, TO_CHAR );
               act( AT_MAGIC, "$p glows briefly, then disappears in a puff of smoke!", ch, obj, NULL, TO_ROOM );
               extract_obj( obj );
            }
            else
               act( AT_MAGIC, "$p glows briefly and a whisp of smoke rises from it.", ch, obj, NULL, TO_CHAR );
         }
         else if( consume )
         {
            separate_obj( obj );
            act( AT_MAGIC, "$p glows brightly, then disappears in a puff of smoke!", ch, obj, NULL, TO_CHAR );
            act( AT_MAGIC, "$p glows brightly, then disappears in a puff of smoke!", ch, obj, NULL, TO_ROOM );
            extract_obj( obj );
         }
         else
         {
            int count = obj->count;

            obj->count = 1;
            act( AT_MAGIC, "$p glows briefly.", ch, obj, NULL, TO_CHAR );
            obj->count = count;
         }
      }
   }
   return TRUE;
}

int pAbort;

/*
 * Locate targets.
 */
/* Turn off annoying message and just abort if needed */
bool silence_locate_targets;

void *locate_targets( CHAR_DATA * ch, char *arg, int sn, CHAR_DATA ** victim, OBJ_DATA ** obj )
{
   SKILLTYPE *skill = get_skilltype( sn );
   void *vo = NULL;

   *victim = NULL;
   *obj = NULL;

   switch ( skill->target )
   {
      default:
         bug( "Do_cast: bad target for sn %d.", sn );
         return &pAbort;

      case TAR_IGNORE:
         break;

      case TAR_CHAR_OFFENSIVE:
      {
         if( arg[0] == '\0' )
         {
            if( ( *victim = who_fighting( ch ) ) == NULL )
            {
               if( !silence_locate_targets )
                  send_to_char( "Cast the spell on whom?\n\r", ch );
               return &pAbort;
            }
         }
         else
         {
            if( ( *victim = get_char_room( ch, arg ) ) == NULL )
            {
               if( !silence_locate_targets )
                  send_to_char( "They aren't here.\n\r", ch );
               return &pAbort;
            }
         }
      }

         if( is_safe( ch, *victim ) )
            return &pAbort;

         if( ch == *victim )
         {
            if( SPELL_FLAG( get_skilltype( sn ), SF_NOSELF ) )
            {
               if( !silence_locate_targets )
                  send_to_char( "You can't cast this on yourself!\n\r", ch );
               return &pAbort;
            }
            if( !silence_locate_targets )
               send_to_char( "Cast this on yourself?  Okay...\n\r", ch );
         }

         if( !IS_NPC( ch ) )
         {
            if( !IS_NPC( *victim ) )
            {
               if( get_timer( ch, TIMER_PKILLED ) > 0 )
               {
                  if( !silence_locate_targets )
                     send_to_char( "You have been killed in the last 5 minutes.\n\r", ch );
                  return &pAbort;
               }

               if( get_timer( *victim, TIMER_PKILLED ) > 0 )
               {
                  if( !silence_locate_targets )
                     send_to_char( "This player has been killed in the last 5 minutes.\n\r", ch );
                  return &pAbort;
               }
               if( !CAN_PKILL( ch ) || !CAN_PKILL( *victim ) )
               {
                  if( !silence_locate_targets )
                     send_to_char( "You cannot attack another player.\n\r", ch );
                  return &pAbort;
               }
               if( *victim != ch )
               {
                  if( !silence_locate_targets )
                     send_to_char( "You really shouldn't do this to another player...\n\r", ch );
                  else if( who_fighting( *victim ) != ch )
                  {
                     /*
                      * Only auto-attack those that are hitting you. 
                      */
                     return &pAbort;
                  }
               }
            }

            if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == *victim )
            {
               if( !silence_locate_targets )
                  send_to_char( "You can't do that on your own follower.\n\r", ch );
               return &pAbort;
            }
         }

         if( check_illegal_pk( ch, *victim ) )
         {
            send_to_char( "You cannot cast that on another player!\n\r", ch );
            return &pAbort;
         }

         vo = ( void * )*victim;
         break;

      case TAR_CHAR_DEFENSIVE:
      {
         if( arg[0] == '\0' )
            *victim = ch;
         else
         {
            if( ( *victim = get_char_room( ch, arg ) ) == NULL )
            {
               if( !silence_locate_targets )
                  send_to_char( "They aren't here.\n\r", ch );
               return &pAbort;
            }
         }
      }

         if( ch == *victim && SPELL_FLAG( get_skilltype( sn ), SF_NOSELF ) )
         {
            if( !silence_locate_targets )
               send_to_char( "You can't cast this on yourself!\n\r", ch );
            return &pAbort;
         }

         vo = ( void * )*victim;
         break;

      case TAR_CHAR_SELF:
         if( arg[0] != '\0' && !nifty_is_name( arg, ch->name ) )
         {
            if( !silence_locate_targets )
               send_to_char( "You cannot cast this spell on another.\n\r", ch );
            return &pAbort;
         }

         vo = ( void * )ch;
         break;

      case TAR_OBJ_INV:
      {
         if( arg[0] == '\0' )
         {
            if( !silence_locate_targets )
               send_to_char( "What should the spell be cast upon?\n\r", ch );
            return &pAbort;
         }

         if( ( *obj = get_obj_carry( ch, arg ) ) == NULL )
         {
            if( !silence_locate_targets )
               send_to_char( "You are not carrying that.\n\r", ch );
            return &pAbort;
         }
      }

         vo = ( void * )*obj;
         break;
   }

   return vo;
}


/*
 * The kludgy global is for spells who want more stuff from command line.
 */
char *target_name;
char *ranged_target_name = NULL;

/*
 * Cast a spell.  Multi-caster and component support by Thoric
 */
CMDF do_cast( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   char arg2[MIL];
   static char staticbuf[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   void *vo = NULL;
   int mana;
   int max = 0;
   int sn;
   ch_ret retcode;
   bool dont_wait = FALSE;
   SKILLTYPE *skill = NULL;
   struct timeval time_used;

   retcode = rNONE;

   switch ( ch->substate )
   {
      default:
         /*
          * no ordering charmed mobs to cast spells 
          */

         if( !CAN_CAST( ch ) )
         {
            send_to_char( "I suppose you think you're a mage?\n\r", ch );
            return;
         }

         if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't seem to do that right now...\n\r", ch );
            return;
         }
         if( IS_ROOM_FLAG( ch->in_room, ROOM_NO_MAGIC ) )
         {
            send_to_char( "&[magic]Your magical energies were disperssed mysteriously.\n\r", ch );
            return;
         }

         target_name = one_argument( argument, arg1 );
         one_argument( target_name, arg2 );
         DISPOSE( ranged_target_name );
         ranged_target_name = str_dup( target_name );
         if( ch->morph != NULL )
         {
            if( !( ch->morph->cast_allowed ) )
            {
               send_to_char( "This morph isn't permitted to cast.\n\r", ch );
               return;
            }
         }
         if( ch->Class == CLASS_BARD && arg1[0] == '\0' )
         {
            send_to_char( "What do you wish to play?\n\r", ch );
            return;
         }

         else if( arg1[0] == '\0' )
         {
            send_to_char( "Cast which what where?\n\r", ch );
            return;
         }

         /*
          * Regular mortal spell casting 
          */
         if( get_trust( ch ) < LEVEL_GOD )
         {
            if( ( sn = find_spell( ch, arg1, TRUE ) ) < 0
                || ( !IS_NPC( ch ) && ch->level < skill_table[sn]->skill_level[ch->Class] ) )
            {
               send_to_char( "You can't cast that!\n\r", ch );
               return;
            }
            if( ( skill = get_skilltype( sn ) ) == NULL )
            {
               send_to_char( "You can't do that right now...\n\r", ch );
               return;
            }
            if( IS_AFFECTED( ch, AFF_SILENCE ) )   /* Silence spell prevents casting */
            {
               send_to_char( "&[magic]You are magically silenced, you cannot utter a sound!\n\r", ch );
               return;
            }
            if( IS_AFFECTED( ch, AFF_BASH ) )   /* Being bashed prevents casting too */
            {
               send_to_char( "&[magic]You've been stunned by a blow to the head. Wait for it to wear off.\n\r", ch );
               return;
            }
         }
         else
            /*
             * Godly "spell builder" spell casting with debugging messages
             */
         {
            if( ( sn = skill_lookup( arg1 ) ) < 0 )
            {
               send_to_char( "We didn't create that yet...\n\r", ch );
               return;
            }
            if( sn >= MAX_SKILL )
            {
               send_to_char( "Hmm... that might hurt.\n\r", ch );
               return;
            }
            if( ( skill = get_skilltype( sn ) ) == NULL )
            {
               send_to_char( "Something is severely wrong with that one...\n\r", ch );
               return;
            }
            if( skill->type != SKILL_SPELL )
            {
               send_to_char( "That isn't a spell.\n\r", ch );
               return;
            }
            if( !skill->spell_fun )
            {
               send_to_char( "We didn't finish that one yet...\n\r", ch );
               return;
            }
         }

         /*
          * Something else removed by Merc         -Thoric
          */
         /*
          * Band-aid alert!  !IS_NPC check -- Blod 
          */
         /*
          * Removed !IS_NPC check -- Tarl 
          */
         if( ch->position < skill->minimum_position )
         {
            switch ( ch->position )
            {
               default:
                  send_to_char( "You can't concentrate enough.\n\r", ch );
                  break;
               case POS_SITTING:
                  send_to_char( "You can't summon enough energy sitting down.\n\r", ch );
                  break;
               case POS_RESTING:
                  send_to_char( "You're too relaxed to cast that spell.\n\r", ch );
                  break;
               case POS_FIGHTING:
                  if( skill->minimum_position <= POS_EVASIVE )
                  {
                     send_to_char( "This fighting style is too demanding for that!\n\r", ch );
                  }
                  else
                  {
                     send_to_char( "No way!  You are still fighting!\n\r", ch );
                  }
                  break;
               case POS_DEFENSIVE:
                  if( skill->minimum_position <= POS_EVASIVE )
                  {
                     send_to_char( "This fighting style is too demanding for that!\n\r", ch );
                  }
                  else
                  {
                     send_to_char( "No way!  You are still fighting!\n\r", ch );
                  }
                  break;
               case POS_AGGRESSIVE:
                  if( skill->minimum_position <= POS_EVASIVE )
                  {
                     send_to_char( "This fighting style is too demanding for that!\n\r", ch );
                  }
                  else
                  {
                     send_to_char( "No way!  You are still fighting!\n\r", ch );
                  }
                  break;
               case POS_BERSERK:
                  if( skill->minimum_position <= POS_EVASIVE )
                  {
                     send_to_char( "This fighting style is too demanding for that!\n\r", ch );
                  }
                  else
                  {
                     send_to_char( "No way!  You are still fighting!\n\r", ch );
                  }
                  break;
               case POS_EVASIVE:
                  send_to_char( "No way!  You are still fighting!\n\r", ch );
                  break;
               case POS_SLEEPING:
                  send_to_char( "You dream about great feats of magic.\n\r", ch );
                  break;
            }
            return;
         }

         if( skill->spell_fun == spell_null )
         {
            send_to_char( "That's not a spell!\n\r", ch );
            return;
         }

         if( !skill->spell_fun )
         {
            send_to_char( "You cannot cast that... yet.\n\r", ch );
            return;
         }

         if( !IS_NPC( ch ) && !IS_IMMORTAL( ch ) && skill->guild != CLASS_NONE
             && ( !ch->pcdata->clan || skill->guild != ch->pcdata->clan->Class ) )
         {
            send_to_char( "That is only available to members of a certain guild.\n\r", ch );
            return;
         }

         /*
          * Mystaric, 980908 - Added checks for spell sector type 
          */
         if( !ch->in_room || ( skill->spell_sector && !IS_SET( skill->spell_sector, ( 1 << ch->in_room->sector_type ) ) ) )
         {
            send_to_char( "You can not cast that here.\n\r", ch );
            return;
         }

         mana = IS_NPC( ch ) ? 0 : UMAX( skill->min_mana, 100 / ( 2 + ch->level - skill->skill_level[ch->Class] ) );

         /*
          * Locate targets.
          */
         vo = locate_targets( ch, arg2, sn, &victim, &obj );
         if( vo == &pAbort )
            return;

         if( !IS_NPC( ch ) && victim && !IS_NPC( victim )
             && CAN_PKILL( victim ) && !CAN_PKILL( ch ) && !in_arena( ch ) && !in_arena( victim ) )
         {
            send_to_char( "&[magic]The gods will not permit you to cast spells on that character.\n\r", ch );
            return;
         }

         if( !IS_NPC( ch ) && ch->mana < mana )
         {
            send_to_char( "You don't have enough mana.\n\r", ch );
            return;
         }

         if( skill->participants <= 1 )
            break;

         /*
          * multi-participant spells         -Thoric 
          */
         add_timer( ch, TIMER_DO_FUN, UMIN( skill->beats / 10, 3 ), do_cast, 1 );
         act( AT_MAGIC, "You begin to chant...", ch, NULL, NULL, TO_CHAR );
         act( AT_MAGIC, "$n begins to chant...", ch, NULL, NULL, TO_ROOM );
         strdup_printf( &ch->alloc_ptr, "%s %s", arg2, target_name );
         ch->tempnum = sn;
         return;
      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         if( IS_VALID_SN( ( sn = ch->tempnum ) ) )
         {
            if( ( skill = get_skilltype( sn ) ) == NULL )
            {
               send_to_char( "Something went wrong...\n\r", ch );
               bug( "do_cast: SUB_TIMER_DO_ABORT: bad sn %d", sn );
               return;
            }
            mana = IS_NPC( ch ) ? 0 : UMAX( skill->min_mana, 100 / ( 2 + ch->level - skill->skill_level[ch->Class] ) );

            if( ch->level < LEVEL_IMMORTAL ) /* so imms dont lose mana */
               ch->mana -= mana / 3;
         }
         set_char_color( AT_MAGIC, ch );
         send_to_char( "You stop chanting...\n\r", ch );
         /*
          * should add chance of backfire here 
          */
         return;
      case 1:
         sn = ch->tempnum;
         if( ( skill = get_skilltype( sn ) ) == NULL )
         {
            send_to_char( "Something went wrong...\n\r", ch );
            bug( "do_cast: substate 1: bad sn %d", sn );
            return;
         }
         if( !ch->alloc_ptr || !IS_VALID_SN( sn ) || skill->type != SKILL_SPELL )
         {
            send_to_char( "Something cancels out the spell!\n\r", ch );
            bug( "do_cast: ch->alloc_ptr NULL or bad sn (%d)", sn );
            return;
         }
         mana = IS_NPC( ch ) ? 0 : UMAX( skill->min_mana, 100 / ( 2 + ch->level - skill->skill_level[ch->Class] ) );
         mudstrlcpy( staticbuf, ch->alloc_ptr, MIL );
         target_name = one_argument( staticbuf, arg2 );
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         if( skill->participants > 1 )
         {
            int cnt = 1;
            CHAR_DATA *tmp;
            TIMER *t;

            for( tmp = ch->in_room->first_person; tmp; tmp = tmp->next_in_room )
               if( tmp != ch && ( t = get_timerptr( tmp, TIMER_DO_FUN ) ) != NULL
                   && t->count >= 1 && t->do_fun == do_cast && tmp->tempnum == sn && tmp->alloc_ptr
                   && !str_cmp( tmp->alloc_ptr, staticbuf ) )
                  ++cnt;
            if( cnt >= skill->participants )
            {
               for( tmp = ch->in_room->first_person; tmp; tmp = tmp->next_in_room )
                  if( tmp != ch && ( t = get_timerptr( tmp, TIMER_DO_FUN ) ) != NULL
                      && t->count >= 1 && t->do_fun == do_cast && tmp->tempnum == sn && tmp->alloc_ptr
                      && !str_cmp( tmp->alloc_ptr, staticbuf ) )
                  {
                     extract_timer( tmp, t );
                     act( AT_MAGIC, "Channeling your energy into $n, you help cast the spell!", ch, NULL, tmp, TO_VICT );
                     act( AT_MAGIC, "$N channels $S energy into you!", ch, NULL, tmp, TO_CHAR );
                     act( AT_MAGIC, "$N channels $S energy into $n!", ch, NULL, tmp, TO_NOTVICT );
                     tmp->mana -= mana;
                     tmp->substate = SUB_NONE;
                     tmp->tempnum = -1;
                     DISPOSE( tmp->alloc_ptr );
                  }
               dont_wait = TRUE;
               send_to_char( "You concentrate all the energy into a burst of mystical words!\n\r", ch );
               vo = locate_targets( ch, arg2, sn, &victim, &obj );
               if( vo == &pAbort )
                  return;
            }
            else
            {
               send_to_char( "&[magic]There was not enough power for the spell to succeed...\n\r", ch );

               if( ch->level < LEVEL_IMMORTAL ) /* so imms dont lose mana */
                  ch->mana -= mana / 2;
               learn_from_failure( ch, sn );
               return;
            }
         }
   }

   /*
    * uttering those magic words unless casting "ventriloquate" 
    */
   if( str_cmp( skill->name, "ventriloquate" ) )
      say_spell( ch, sn );

   if( !dont_wait )
      WAIT_STATE( ch, skill->beats );

   /*
    * Getting ready to cast... check for spell components  -Thoric
    */
   if( !process_spell_components( ch, sn ) )
   {
      if( ch->level < LEVEL_IMMORTAL ) /* so imms dont lose mana */
         ch->mana -= mana / 2;
      learn_from_failure( ch, sn );
      return;
   }

   if( IS_IMMORTAL( ch ) || IS_NPC( ch ) )
      max = 1;
   else
   {
      max = ch->spellfail + ( skill->difficulty * 5 ) + ( GET_COND( ch, COND_DRUNK ) * 2 );
      switch ( ch->Class )
      {
         default:
            break;
         case CLASS_MAGE:
            if( EqWBits( ch, ITEM_ANTI_MAGE ) )
               max += 10;
            break;
         case CLASS_CLERIC:
            if( EqWBits( ch, ITEM_ANTI_CLERIC ) )
               max += 10;
            break;
         case CLASS_DRUID:
            if( EqWBits( ch, ITEM_ANTI_DRUID ) )
               max += 10;
            break;
         case CLASS_PALADIN:
            if( EqWBits( ch, ITEM_ANTI_PALADIN ) )
               max += 10;
            break;
         case CLASS_ANTIPALADIN:
            if( EqWBits( ch, ITEM_ANTI_APAL ) )
               max += 10;
            break;
         case CLASS_BARD:
            if( EqWBits( ch, ITEM_ANTI_BARD ) )
               max += 10;
            break;
         case CLASS_RANGER:
            if( EqWBits( ch, ITEM_ANTI_RANGER ) )
               max += 10;
            break;
         case CLASS_NECROMANCER:
            if( EqWBits( ch, ITEM_ANTI_NECRO ) )
               max += 10;
            break;
      }
   }

   if( ch->pcdata && number_range( 1, max ) > ch->pcdata->learned[sn] )
   {
      /*
       * Some more interesting loss of concentration messages  -Thoric 
       */
      switch ( number_bits( 2 ) )
      {
         case 0: /* too busy */
            if( ch->fighting )
               send_to_char( "This round of battle is too hectic to concentrate properly.\n\r", ch );
            else
               send_to_char( "You lost your concentration.\n\r", ch );
            break;
         case 1: /* irritation */
            if( number_bits( 2 ) == 0 )
            {
               switch ( number_bits( 2 ) )
               {
                  case 0:
                     send_to_char( "A tickle in your nose prevents you from keeping your concentration.\n\r", ch );
                     break;
                  case 1:
                     send_to_char( "An itch on your leg keeps you from properly casting your spell.\n\r", ch );
                     break;
                  case 2:
                     send_to_char( "Something in your throat prevents you from uttering the proper phrase.\n\r", ch );
                     break;
                  case 3:
                     send_to_char( "A twitch in your eye disrupts your concentration for a moment.\n\r", ch );
                     break;
               }
            }
            else
               send_to_char( "Something distracts you, and you lose your concentration.\n\r", ch );
            break;
         case 2: /* not enough time */
            if( ch->fighting )
               send_to_char( "There wasn't enough time this round to complete the casting.\n\r", ch );
            else
               send_to_char( "You lost your concentration.\n\r", ch );
            break;
         case 3:
            send_to_char( "You get a mental block mid-way through the casting.\n\r", ch );
            break;
      }
      if( ch->level < LEVEL_IMMORTAL ) /* so imms dont lose mana */
         ch->mana -= mana / 2;
      learn_from_failure( ch, sn );
      return;
   }
   else
   {
      ch->mana -= mana;

      /*
       * check for immunity to magic if victim is known...
       * and it is a TAR_CHAR_DEFENSIVE/SELF spell
       * otherwise spells will have to check themselves
       */
      if( ( ( skill->target == TAR_CHAR_DEFENSIVE || skill->target == TAR_CHAR_SELF )
            && victim && IS_IMMUNE( victim, RIS_MAGIC ) ) )
      {
         immune_casting( skill, ch, victim, NULL );
         retcode = rSPELL_FAILED;
      }
      else
      {
         start_timer( &time_used );
         retcode = ( *skill->spell_fun ) ( sn, ch->level, ch, vo );
         end_timer( &time_used );
      }
   }

   if( retcode == rCHAR_DIED || retcode == rERROR || char_died( ch ) )
      return;

   /*
    * learning 
    */
   if( retcode == rSPELL_FAILED )
      learn_from_failure( ch, sn );

   /*
    * favor adjustments 
    */
   if( victim && victim != ch && !IS_NPC( victim ) && skill->target == TAR_CHAR_DEFENSIVE )
      adjust_favor( ch, 7, 1 );

   if( victim && victim != ch && !IS_NPC( ch ) && skill->target == TAR_CHAR_DEFENSIVE )
      adjust_favor( victim, 13, 1 );

   if( victim && victim != ch && !IS_NPC( ch ) && skill->target == TAR_CHAR_OFFENSIVE )
      adjust_favor( ch, 4, 1 );

   /*
    * Fixed up a weird mess here, and added double safeguards -Thoric
    */
   if( skill->target == TAR_CHAR_OFFENSIVE && victim && !char_died( victim ) && victim != ch )
   {
      CHAR_DATA *vch, *vch_next;

      for( vch = ch->in_room->first_person; vch; vch = vch_next )
      {
         vch_next = vch->next_in_room;

         if( vch == victim )
         {
            if( vch->master != ch && !vch->fighting )
               retcode = multi_hit( vch, ch, TYPE_UNDEFINED );
            break;
         }
      }
   }
   return;
}

/* Wrapper function for Bards - Samson 10-26-98 */
CMDF do_play( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj, *obj_next;
   bool found;

   if( ch->Class != CLASS_BARD )
   {
      send_to_char( "Only a bard can play songs!\n\r", ch );
      return;
   }

   found = FALSE;

   for( obj = ch->first_carrying; obj != NULL; obj = obj_next )
   {
      obj_next = obj->next_content;

      if( obj->item_type == ITEM_INSTRUMENT && obj->wear_loc == WEAR_HOLD )
      {
         found = TRUE;
         break;
      }
   }

   if( !found )
   {
      send_to_char( "You are not holding an instrument!\n\r", ch );
      return;
   }

   do_cast( ch, argument );
   return;
}

/*
 * Cast spells at targets using a magical object.
 */
ch_ret obj_cast_spell( int sn, int level, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj )
{
   void *vo;
   ch_ret retcode = rNONE;
   int levdiff = ch->level - level;
   SKILLTYPE *skill = get_skilltype( sn );
   struct timeval time_used;

   if( sn == -1 )
      return retcode;

   if( !skill || !skill->spell_fun )
   {
      bug( "Obj_cast_spell: bad sn %d.", sn );
      return rERROR;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_NO_MAGIC ) )
   {
      send_to_char( "&[magic]The magic from the spell is dispersed...\n\r", ch );
      return rNONE;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SAFE ) && skill->target == TAR_CHAR_OFFENSIVE )
   {
      send_to_char( "&[magic]This is a safe room...\n\r", ch );
      return rNONE;
   }

   /*
    * Basically this was added to cut down on level 5 players using level
    * 40 scrolls in battle too often ;)     -Thoric
    */
   if( ( skill->target == TAR_CHAR_OFFENSIVE || number_bits( 7 ) == 1 ) /* 1/128 chance if non-offensive */
       && skill->type != SKILL_HERB && !chance( ch, 95 + levdiff ) )
   {
      switch ( number_bits( 2 ) )
      {
         case 0:
            failed_casting( skill, ch, victim, NULL );
            break;
         case 1:
            act( AT_MAGIC, "The $t spell backfires!", ch, skill->name, victim, TO_CHAR );
            if( victim )
               act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_VICT );
            act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_NOTVICT );
            return damage( ch, ch, number_range( 1, level ), TYPE_UNDEFINED );
         case 2:
            failed_casting( skill, ch, victim, NULL );
            break;
         case 3:
            act( AT_MAGIC, "The $t spell backfires!", ch, skill->name, victim, TO_CHAR );
            if( victim )
               act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_VICT );
            act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_NOTVICT );
            return damage( ch, ch, number_range( 1, level ), TYPE_UNDEFINED );
      }
      return rNONE;
   }

   target_name = "";
   switch ( skill->target )
   {
      default:
         bug( "Obj_cast_spell: bad target for sn %d.", sn );
         return rERROR;

      case TAR_IGNORE:
         vo = NULL;
         if( victim )
            target_name = victim->name;
         else if( obj )
            target_name = obj->name;
         break;

      case TAR_CHAR_OFFENSIVE:
         if( victim != ch )
         {
            if( !victim )
               victim = who_fighting( ch );
            if( !victim || ( !IS_NPC( victim ) && !in_arena( victim ) ) )
            {
               send_to_char( "You can't do that.\n\r", ch );
               return rNONE;
            }
         }
         if( ch != victim && is_safe( ch, victim ) )
            return rNONE;
         vo = ( void * )victim;
         break;

      case TAR_CHAR_DEFENSIVE:
         if( victim == NULL )
            victim = ch;
         vo = ( void * )victim;
         if( skill->type != SKILL_HERB && IS_IMMUNE( victim, RIS_MAGIC ) )
         {
            immune_casting( skill, ch, victim, NULL );
            return rNONE;
         }
         break;

      case TAR_CHAR_SELF:
         vo = ( void * )ch;
         if( skill->type != SKILL_HERB && IS_IMMUNE( ch, RIS_MAGIC ) )
         {
            immune_casting( skill, ch, victim, NULL );
            return rNONE;
         }
         break;

      case TAR_OBJ_INV:
         if( obj == NULL )
         {
            send_to_char( "You can't do that.\n\r", ch );
            return rNONE;
         }
         vo = ( void * )obj;
         break;
   }
   start_timer( &time_used );
   retcode = ( *skill->spell_fun ) ( sn, level, ch, vo );
   end_timer( &time_used );

   if( retcode == rSPELL_FAILED )
      retcode = rNONE;

   if( retcode == rCHAR_DIED || retcode == rERROR )
      return retcode;

   if( char_died( ch ) )
      return rCHAR_DIED;

   if( skill->target == TAR_CHAR_OFFENSIVE && victim != ch && !char_died( victim ) )
   {
      CHAR_DATA *vch;
      CHAR_DATA *vch_next;

      for( vch = ch->in_room->first_person; vch; vch = vch_next )
      {
         vch_next = vch->next_in_room;
         if( victim == vch && !vch->fighting && vch->master != ch )
         {
            retcode = multi_hit( vch, ch, TYPE_UNDEFINED );
            break;
         }
      }
   }

   return retcode;
}

/*
 * Spell functions.
 */

/* Stock spells start here */

/* Do not delete - used by the liquidate skill! */
SPELLF spell_midas_touch( int sn, int level, CHAR_DATA * ch, void *vo )
{
   int val;
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;

   if( IS_OBJ_FLAG( obj, ITEM_NODROP ) || IS_OBJ_FLAG( obj, ITEM_SINDHAE ) )
   {
      send_to_char( "You can't seem to let go of it.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) && get_trust( ch ) < LEVEL_IMMORTAL )
      /*
       * was victim instead of ch!  Thanks Nick Gammon 
       */
   {
      send_to_char( "That item is not for mortal hands to touch!\n\r", ch );
      return rSPELL_FAILED;   /* Thoric */
   }

   if( obj->rent >= sysdata.minrent )
   {
      send_to_char( "Rare items cannot be liquidated in this manner!\n\r", ch );
      return rNONE;
   }

   if( !CAN_WEAR( obj, ITEM_TAKE ) || ( obj->item_type == ITEM_CORPSE_NPC ) || ( obj->item_type == ITEM_CORPSE_PC ) )
   {
      send_to_char( "You cannot seem to turn this item to gold!\n\r", ch );
      return rNONE;
   }

   separate_obj( obj ); /* nice, alty :) */

   val = obj->cost / 2;
   val = UMAX( 0, val );
   ch->gold += val;

   if( obj_extracted( obj ) )
      return rNONE;

   extract_obj( obj );

   send_to_char( "You transmogrify the item to gold!\n\r", ch );
   return rNONE;
}

SPELLF spell_cure_poison( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   SKILLTYPE *skill = get_skilltype( sn );

   if( IS_IMMUNE( victim, RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( is_affected( victim, gsn_poison ) )
   {
      affect_strip( victim, gsn_poison );
      set_char_color( AT_MAGIC, victim );
      send_to_char( "A warm feeling runs through your body.\n\r", victim );
      victim->mental_state = URANGE( -100, victim->mental_state, -10 );
      if( ch != victim )
      {
         act( AT_MAGIC, "A flush of health washes over $N.", ch, NULL, victim, TO_NOTVICT );
         act( AT_MAGIC, "You lift the poison from $N's body.", ch, NULL, victim, TO_CHAR );
      }
      return rNONE;
   }
   else
   {
      set_char_color( AT_MAGIC, ch );
      if( ch != victim )
         send_to_char( "You work your cure, but it has no apparent effect.\n\r", ch );
      else
         send_to_char( "You don't seem to be poisoned.\n\r", ch );
      return rSPELL_FAILED;
   }
}

SPELLF spell_cure_blindness( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   SKILLTYPE *skill = get_skilltype( sn );

   set_char_color( AT_MAGIC, ch );
   if( IS_IMMUNE( victim, RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( !is_affected( victim, gsn_blindness ) )
   {
      if( ch != victim )
         send_to_char( "You work your cure, but it has no apparent effect.\n\r", ch );
      else
         send_to_char( "You don't seem to be blind.\n\r", ch );
      return rSPELL_FAILED;
   }
   affect_strip( victim, gsn_blindness );
   set_char_color( AT_MAGIC, victim );
   send_to_char( "Your vision returns!\n\r", victim );
   if( ch != victim )
      send_to_char( "You work your cure, restoring vision.\n\r", ch );
   return rNONE;
}

/* Must keep */
SPELLF spell_call_lightning( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *vch, *vch_next;
   int dam;
   bool ch_died;
   ch_ret retcode = rNONE;

   if( ( !IS_OUTSIDE( ch ) || INDOOR_SECTOR( ch->in_room->sector_type ) )
       && !IS_PLR_FLAG( ch, PLR_ONMAP ) && !IS_ACT_FLAG( ch, ACT_ONMAP ) )
   {
      send_to_char( "You must be outdoors to cast this spell.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( ch->in_room->area->weather->precip <= 0 )
   {
      send_to_char( "You need bad weather.\n\r", ch );
      return rSPELL_FAILED;
   }

   dam = dice( level, 6 );

   set_char_color( AT_MAGIC, ch );
   send_to_char( "God's lightning strikes your foes!\n\r", ch );
   act( AT_MAGIC, "$n calls God's lightning to strike $s foes!", ch, NULL, NULL, TO_ROOM );

   ch_died = FALSE;
   for( vch = ch->in_room->first_person; vch; vch = vch_next )
   {
      vch_next = vch->next_in_room;

      if( IS_PLR_FLAG( vch, PLR_WIZINVIS ) && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
         continue;

      if( !is_same_map( ch, vch ) )
         continue;

      if( vch != ch && ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) ) )
         retcode = damage( ch, vch, saves_spell_staff( level, vch ) ? dam / 2 : dam, sn );
      if( retcode == rCHAR_DIED || char_died( ch ) )
      {
         ch_died = TRUE;
         continue;
      }

      if( !ch_died && IS_OUTSIDE( vch ) && IS_AWAKE( vch ) )
      {
         if( number_bits( 3 ) == 0 )
            send_to_char( "&BLightning flashes in the sky.\n\r", vch );
      }
   }
   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}

/* Do not remove */
SPELLF spell_change_sex( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   AFFECT_DATA af;
   SKILLTYPE *skill = get_skilltype( sn );

   if( IS_IMMUNE( victim, RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   if( is_affected( victim, sn ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   af.type = sn;
   af.duration = ( int )( 10 * level * DUR_CONV );
   af.location = APPLY_SEX;
   do
   {
      af.modifier = number_range( 0, SEX_MAX ) - victim->sex;
   }
   while( af.modifier == 0 );
   af.bit = 0;
   affect_to_char( victim, &af );
   successful_casting( skill, ch, victim, NULL );
   return rNONE;
}

bool can_charm( CHAR_DATA * ch )
{
   if( IS_NPC( ch ) || IS_IMMORTAL( ch ) )
      return TRUE;
   if( ( ( get_curr_cha( ch ) / 5 ) + 1 ) > ch->pcdata->charmies )
      return TRUE;
   return FALSE;
}

SPELLF spell_charm_person( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   int schance;
   SKILLTYPE *skill = get_skilltype( sn );

   if( victim == ch )
   {
      send_to_char( "You like yourself even better!\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_IMMUNE( victim, RIS_MAGIC ) || IS_IMMUNE( victim, RIS_CHARM ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( !IS_NPC( victim ) && !IS_NPC( ch ) )
   {
      send_to_char( "I don't think so...\n\r", ch );
      send_to_char( "You feel charmed...\n\r", victim );
      return rSPELL_FAILED;
   }

   schance = ris_save( victim, level, RIS_CHARM );

   if( IS_AFFECTED( victim, AFF_CHARM ) || schance == 1000 || IS_AFFECTED( ch, AFF_CHARM )
       || level < victim->level || circle_follow( victim, ch ) || !can_charm( ch ) || saves_spell_staff( schance, victim ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( victim->master )
      stop_follower( victim );

   bind_follower( victim, ch, sn, ( int )( number_fuzzy( ( int )( ( level + 1 ) / 5 ) + 1 ) * DUR_CONV ) );
   successful_casting( skill, ch, victim, NULL );

   if( IS_NPC( victim ) )
   {
      start_hating( victim, ch );
      start_hunting( victim, ch );
   }
   return rNONE;
}

SPELLF spell_control_weather( int sn, int level, CHAR_DATA * ch, void *vo )
{
   SKILLTYPE *skill = get_skilltype( sn );
   WEATHER_DATA *weath;
   int change;
   weath = ch->in_room->area->weather;

   change = number_range( -rand_factor, rand_factor ) + ( ch->level * 3 ) / ( 2 * max_vector );

   if( !str_cmp( target_name, "warmer" ) )
      weath->temp_vector += change;
   else if( !str_cmp( target_name, "colder" ) )
      weath->temp_vector -= change;
   else if( !str_cmp( target_name, "wetter" ) )
      weath->precip_vector += change;
   else if( !str_cmp( target_name, "drier" ) )
      weath->precip_vector -= change;
   else if( !str_cmp( target_name, "windier" ) )
      weath->wind_vector += change;
   else if( !str_cmp( target_name, "calmer" ) )
      weath->wind_vector -= change;
   else
   {
      send_to_char( "Do you want it to get warmer, colder, wetter, drier, windier, or calmer?\n\r", ch );
      return rSPELL_FAILED;
   }

   weath->temp_vector = URANGE( -max_vector, weath->temp_vector, max_vector );
   weath->precip_vector = URANGE( -max_vector, weath->precip_vector, max_vector );
   weath->wind_vector = URANGE( -max_vector, weath->wind_vector, max_vector );

   weather_update(  );
   successful_casting( skill, ch, NULL, NULL );
   return rNONE;
}

SPELLF spell_create_water( int sn, int level, CHAR_DATA * ch, void *vo )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;
   LIQ_TABLE *liq = NULL;
   WEATHER_DATA *weath;
   int water;

   if( obj->item_type != ITEM_DRINK_CON )
   {
      send_to_char( "It is unable to hold water.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( ( liq = get_liq_vnum( obj->value[2] ) ) == NULL )
   {
      bug( "create_water: bad liquid number %d", obj->value[2] );
      send_to_char( "There's already another liquid in the container.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( str_cmp( liq->name, "water" ) && obj->value[1] != 0 )
   {
      send_to_char( "There's already another liquid in the container.\n\r", ch );
      return rSPELL_FAILED;
   }

   weath = ch->in_room->area->weather;

   water = UMIN( level * ( weath->precip >= 0 ? 4 : 2 ), obj->value[0] - obj->value[1] );

   if( water > 0 )
   {
      separate_obj( obj );
      obj->value[2] = liq->vnum;
      obj->value[1] += water;

      /*
       * Don't overfill the container! 
       */
      if( obj->value[1] > obj->value[0] )
         obj->value[1] = obj->value[0];

      if( !is_name( "water", obj->name ) )
         stralloc_printf( &obj->name, "%s water", obj->name );

      act( AT_MAGIC, "$p is filled.", ch, obj, NULL, TO_CHAR );
   }
   return rNONE;
}

SPELLF spell_detect_poison( int sn, int level, CHAR_DATA * ch, void *vo )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;

   set_char_color( AT_MAGIC, ch );
   if( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD || obj->item_type == ITEM_COOK )
   {
      if( obj->item_type == ITEM_COOK && obj->value[2] == 0 )
         send_to_char( "It looks undercooked.\n\r", ch );
      else if( obj->value[3] != 0 )
         send_to_char( "You smell poisonous fumes.\n\r", ch );
      else
         send_to_char( "It looks very delicious.\n\r", ch );
   }
   else
      send_to_char( "It doesn't look poisoned.\n\r", ch );
   return rNONE;
}

/* MUST BE KEPT!!!! Used by defense types for mobiles!!! */
SPELLF spell_dispel_evil( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   int dam;
   SKILLTYPE *skill = get_skilltype( sn );

   if( !IS_NPC( ch ) && IS_EVIL( ch ) )
      victim = ch;

   if( IS_GOOD( victim ) )
   {
      act( AT_MAGIC, "The Gods protect $N.", ch, NULL, victim, TO_ROOM );
      return rSPELL_FAILED;
   }

   if( IS_NEUTRAL( victim ) )
   {
      act( AT_MAGIC, "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
      return rSPELL_FAILED;
   }

   if( IS_IMMUNE( victim, RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   dam = dice( level, 8 );

   if( ch->Class == CLASS_PALADIN )
      dam = dice( level, 6 );

   if( saves_spell_staff( level, victim ) )
      return rSPELL_FAILED;
   return damage( ch, victim, dam, sn );
}

/* Redone by Samson. In a desparate attempt to get this thing to behave
 * properly and such. It's MUCH more of a pain in the ass than it might seem.
 */
SPELLF spell_dispel_magic( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   AFFECT_DATA *paf, *paf_next;
   SKILLTYPE *skill = NULL;

   set_char_color( AT_MAGIC, ch );

   if( victim != ch )
   {  /* Edited by Tarl 27 Mar 02 to correct */
      if( saves_spell_staff( level, victim ) )
      {
         set_char_color( AT_MAGIC, ch );
         send_to_char( "You weave arcane gestures, but the spell does nothing.\n\r", ch );
         return rSPELL_FAILED;
      }
   }

   /*
    * Remove ALL affects generated by spells, and kill the AFF_X bit for it as well 
    */
   for( paf = victim->first_affect; paf != NULL; paf = paf_next )
   {
      paf_next = paf->next;

      if( ( skill = get_skilltype( paf->type ) ) != NULL )
      {
         if( skill->type == SKILL_SPELL )
         {
            if( skill->msg_off )
               ch_printf( victim, "%s\n\r", skill->msg_off );
            xREMOVE_BIT( victim->affected_by, paf->bit );
            affect_remove( victim, paf );
         }
      }
   }

   /*
    * Now we get to do the hard part - step thru and look for things to dispel when it's not set by a spell. 
    */

   if( IS_AFFECTED( victim, AFF_SANCTUARY ) )
      REMOVE_AFFECTED( victim, AFF_SANCTUARY );

   if( IS_AFFECTED( victim, AFF_FAERIE_FIRE ) )
      REMOVE_AFFECTED( victim, AFF_FAERIE_FIRE );

   if( IS_AFFECTED( victim, AFF_CURSE ) )
      REMOVE_AFFECTED( victim, AFF_CURSE );

   if( IS_AFFECTED( victim, AFF_PROTECT ) )
      REMOVE_AFFECTED( victim, AFF_PROTECT );

   if( IS_AFFECTED( victim, AFF_SLEEP ) )
      REMOVE_AFFECTED( victim, AFF_SLEEP );

   if( IS_AFFECTED( victim, AFF_CHARM ) )
      REMOVE_AFFECTED( victim, AFF_CHARM );

   if( IS_AFFECTED( victim, AFF_ACIDMIST ) )
      REMOVE_AFFECTED( victim, AFF_ACIDMIST );

   if( IS_AFFECTED( victim, AFF_TRUESIGHT ) )
      REMOVE_AFFECTED( victim, AFF_TRUESIGHT );

   if( IS_AFFECTED( victim, AFF_FIRESHIELD ) )
      REMOVE_AFFECTED( victim, AFF_FIRESHIELD );

   if( IS_AFFECTED( victim, AFF_SHOCKSHIELD ) )
      REMOVE_AFFECTED( victim, AFF_SHOCKSHIELD );

   if( IS_AFFECTED( victim, AFF_VENOMSHIELD ) )
      REMOVE_AFFECTED( victim, AFF_VENOMSHIELD );

   if( IS_AFFECTED( victim, AFF_ICESHIELD ) )
      REMOVE_AFFECTED( victim, AFF_ICESHIELD );

   if( IS_AFFECTED( victim, AFF_BLADEBARRIER ) )
      REMOVE_AFFECTED( victim, AFF_BLADEBARRIER );

   if( IS_AFFECTED( victim, AFF_SILENCE ) )
      REMOVE_AFFECTED( victim, AFF_SILENCE );

   if( IS_AFFECTED( victim, AFF_HASTE ) )
      REMOVE_AFFECTED( victim, AFF_HASTE );

   if( IS_AFFECTED( victim, AFF_SLOW ) )
      REMOVE_AFFECTED( victim, AFF_SLOW );

   set_char_color( AT_MAGIC, ch );
   ch_printf( ch, "You weave arcane gestures, and %s's spells are negated!\n\r",
              IS_NPC( victim ) ? victim->short_descr : victim->name );

   /*
    * Have to reset victim's racial and eq affects etc 
    */
   if( !IS_NPC( victim ) )
      update_aris( victim );

   if( IS_NPC( victim ) )
   {
      if( !ch->fighting || ( ch->fighting && ch->fighting->who != victim ) )
         multi_hit( ch, victim, TYPE_UNDEFINED );
   }
   return rNONE;
}

SPELLF spell_polymorph( int sn, int level, CHAR_DATA * ch, void *vo )
{
   MORPH_DATA *morph;
   SKILLTYPE *skill = get_skilltype( sn );

   morph = find_morph( ch, target_name, TRUE );
   if( !morph )
   {
      send_to_char( "You can't morph into anything like that!\n\r", ch );
      return rSPELL_FAILED;
   }

   if( !do_morph_char( ch, morph ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }
   return rNONE;
}

SPELLF spell_disruption( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   int dam;

   dam = dice( level, 8 );

   if( ch->Class == CLASS_PALADIN );
   dam = dice( level, 6 );

   if( !IsUndead( victim ) )
   {
      send_to_char( "Your spell fizzles into the ether.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( saves_spell_staff( level, victim ) )
      dam = 0;
   return damage( ch, victim, dam, sn );
}

SPELLF spell_enchant_weapon( int sn, int level, CHAR_DATA * ch, void *vo )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;
   AFFECT_DATA *paf;

   if( obj->item_type != ITEM_WEAPON || IS_OBJ_FLAG( obj, ITEM_MAGIC ) || obj->first_affect )
   {
      act( AT_MAGIC, "Your magic twists and winds around $p but cannot take hold.", ch, obj, NULL, TO_CHAR );
      act( AT_MAGIC, "$n's magic twists and winds around $p but cannot take hold.", ch, obj, NULL, TO_NOTVICT );
      return rSPELL_FAILED;
   }

   /*
    * Bug fix here. -- Alty 
    */
   separate_obj( obj );
   CREATE( paf, AFFECT_DATA, 1 );
   paf->type = -1;
   paf->duration = -1;
   paf->location = APPLY_HITROLL;
   paf->modifier = level / 15;
   paf->bit = 0;
   LINK( paf, obj->first_affect, obj->last_affect, next, prev );

   CREATE( paf, AFFECT_DATA, 1 );
   paf->type = -1;
   paf->duration = -1;
   paf->location = APPLY_DAMROLL;
   paf->modifier = level / 15;
   paf->bit = 0;
   LINK( paf, obj->first_affect, obj->last_affect, next, prev );

   SET_OBJ_FLAG( obj, ITEM_MAGIC );

   if( IS_GOOD( ch ) )
   {
      SET_OBJ_FLAG( obj, ITEM_ANTI_EVIL );
      act( AT_BLUE, "$p gleams with flecks of blue energy.", ch, obj, NULL, TO_ROOM );
      act( AT_BLUE, "$p gleams with flecks of blue energy.", ch, obj, NULL, TO_CHAR );
   }
   else if( IS_EVIL( ch ) )
   {
      SET_OBJ_FLAG( obj, ITEM_ANTI_GOOD );
      act( AT_BLOOD, "A crimson stain flows slowly over $p.", ch, obj, NULL, TO_CHAR );
      act( AT_BLOOD, "A crimson stain flows slowly over $p.", ch, obj, NULL, TO_ROOM );
   }
   else
   {
      SET_OBJ_FLAG( obj, ITEM_ANTI_EVIL );
      SET_OBJ_FLAG( obj, ITEM_ANTI_GOOD );
      act( AT_YELLOW, "$p glows with a disquieting light.", ch, obj, NULL, TO_ROOM );
      act( AT_YELLOW, "$p glows with a disquieting light.", ch, obj, NULL, TO_CHAR );
   }
   return rNONE;
}

/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
/* If this thing ever gets looked at in the future, the exp drain calculation needs fixing! */
SPELLF spell_energy_drain( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   int dam;
   int schance;
   SKILLTYPE *skill = get_skilltype( sn );

   if( IS_IMMUNE( victim, RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   schance = ris_save( victim, victim->level, RIS_DRAIN );
   if( schance == 1000 || saves_spell_staff( schance, victim ) )
   {
      failed_casting( skill, ch, victim, NULL );   /* SB */
      return rSPELL_FAILED;
   }

   ch->alignment = UMAX( -1000, ch->alignment - 200 );
   if( victim->level <= 1 )
      dam = GET_HIT( victim ) * 12;
   else
   {
      gain_exp( victim, 0 - number_range( level / 2, 3 * level / 2 ) );
      dam = 1;
   }

   if( GET_HIT( ch ) > GET_MAX_HIT( ch ) )
      GET_HIT( ch ) = GET_MAX_HIT( ch );
   return damage( ch, victim, dam, sn );
}

SPELLF spell_faerie_fog( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *ich;

   act( AT_MAGIC, "$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM );
   act( AT_MAGIC, "You conjure a cloud of purple smoke.", ch, NULL, NULL, TO_CHAR );

   for( ich = ch->in_room->first_person; ich; ich = ich->next_in_room )
   {
      if( IS_PLR_FLAG( ich, PLR_WIZINVIS ) )
         continue;

      if( !is_same_map( ch, ich ) )
         continue;

      if( ich == ch || saves_spell_staff( level, ich ) )
         continue;

      affect_strip( ich, gsn_invis );
      affect_strip( ich, gsn_mass_invis );
      affect_strip( ich, gsn_sneak );
      REMOVE_AFFECTED( ich, AFF_HIDE );
      REMOVE_AFFECTED( ich, AFF_INVISIBLE );
      REMOVE_AFFECTED( ich, AFF_SNEAK );
      act( AT_MAGIC, "$n is revealed!", ich, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You are revealed!", ich, NULL, NULL, TO_CHAR );
   }
   return rNONE;
}

SPELLF spell_gate( int sn, int level, CHAR_DATA * ch, void *vo )
{
   MOB_INDEX_DATA *temp;
   CHAR_DATA *mob;

   if( ( temp = get_mob_index( MOB_VNUM_GATE ) ) == NULL )
   {
      bug( "Spell_gate: Servitor daemon vnum %d doesn't exist.", MOB_VNUM_GATE );
      return rSPELL_FAILED;
   }

   if( !can_charm( ch ) )
   {
      send_to_char( "You already have too many followers to support!\n\r", ch );
      return rSPELL_FAILED;
   }

   mob = create_mobile( temp );
   if( !char_to_room( mob, ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   fix_maps( ch, mob );
   act( AT_MAGIC, "You bring forth $N from another plane!", ch, NULL, mob, TO_CHAR );
   act( AT_MAGIC, "$n brings forth $N from another plane!", ch, NULL, mob, TO_ROOM );
   bind_follower( mob, ch, sn, ( int )( number_fuzzy( ( int )( ( level + 1 ) / 5 ) + 1 ) * DUR_CONV ) );
   return rNONE;
}

/* Modified by Scryn to work on mobs/players/objs */
/* Made it show short descrs instead of keywords, seeing as you need
   to know the keyword anyways, we may as well make it look nice -- Alty */
/* Function modified from original form */
/* Output of information reformatted to look better - Samson 2-8-98 */
SPELLF spell_identify( int sn, int level, CHAR_DATA * ch, void *vo )
{
   OBJ_DATA *obj;
   CHAR_DATA *victim;
   AFFECT_DATA *paf;
   SKILLTYPE *sktmp;
   SKILLTYPE *skill = get_skilltype( sn );
   char *name;

   if( target_name[0] == '\0' )
   {
      send_to_char( "What should the spell be cast upon?\n\r", ch );
      return rSPELL_FAILED;
   }

   if( ( obj = get_obj_carry( ch, target_name ) ) != NULL )
   {
      set_char_color( AT_LBLUE, ch );
      ch_printf( ch, "Object: %s\n\r", obj->short_descr );
      if( ch->level >= LEVEL_IMMORTAL )
         ch_printf( ch, "Vnum: %d\n\r", obj->pIndexData->vnum );
      ch_printf( ch, "Keywords: %s\n\r", obj->name );
      ch_printf( ch, "Type: %s\n\r", item_type_name( obj ) );
      ch_printf( ch, "Wear Flags : %s\n\r", wear_bit_name( obj->wear_flags ) );
      ch_printf( ch, "Layers     : %d\n\r", obj->pIndexData->layers );
      ch_printf( ch, "Extra Flags: %s\n\r", extra_bit_name( &obj->extra_flags ) );
      ch_printf( ch, "Magic Flags: %s\n\r", magic_bit_name( obj->magic_flags ) );
      ch_printf( ch, "Weight: %d  Value: %d  Rent Cost: %d\n\r", obj->weight, obj->cost, obj->rent );
      set_char_color( AT_MAGIC, ch );


      switch ( obj->item_type )
      {
         case ITEM_CONTAINER:
            ch_printf( ch, "%s appears to be %s.\n\r", capitalize( obj->short_descr ),
                       obj->value[0] < 76 ? "of a small capacity" :
                       obj->value[0] < 150 ? "of a small to medium capacity" :
                       obj->value[0] < 300 ? "of a medium capacity" :
                       obj->value[0] < 550 ? "of a medium to large capacity" :
                       obj->value[0] < 751 ? "of a large capacity" : "of a giant capacity" );
            break;

         case ITEM_PILL:
         case ITEM_SCROLL:
         case ITEM_POTION:
            ch_printf( ch, "Level %d spells of:", obj->value[0] );

            if( obj->value[1] >= 0 && ( sktmp = get_skilltype( obj->value[1] ) ) != NULL )
            {
               send_to_char( " '", ch );
               send_to_char( sktmp->name, ch );
               send_to_char( "'", ch );
            }

            if( obj->value[2] >= 0 && ( sktmp = get_skilltype( obj->value[2] ) ) != NULL )
            {
               send_to_char( " '", ch );
               send_to_char( sktmp->name, ch );
               send_to_char( "'", ch );
            }

            if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) != NULL )
            {
               send_to_char( " '", ch );
               send_to_char( sktmp->name, ch );
               send_to_char( "'", ch );
            }

            send_to_char( ".\n\r", ch );
            break;

         case ITEM_SALVE:
            ch_printf( ch, "Has %d of %d applications of level %d", obj->value[2], obj->value[1], obj->value[0] );
            if( obj->value[4] >= 0 && ( sktmp = get_skilltype( obj->value[4] ) ) != NULL )
            {
               send_to_char( " '", ch );
               send_to_char( sktmp->name, ch );
               send_to_char( "'", ch );
            }
            if( obj->value[5] >= 0 && ( sktmp = get_skilltype( obj->value[5] ) ) != NULL )
            {
               send_to_char( " '", ch );
               send_to_char( sktmp->name, ch );
               send_to_char( "'", ch );
            }
            send_to_char( ".\n\r", ch );
            break;

         case ITEM_WAND:
         case ITEM_STAFF:
            ch_printf( ch, "Has %d of %d charges of level %d", obj->value[2], obj->value[1], obj->value[0] );

            if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) != NULL )
            {
               send_to_char( " '", ch );
               send_to_char( sktmp->name, ch );
               send_to_char( "'", ch );
            }

            send_to_char( ".\n\r", ch );
            break;

         case ITEM_WEAPON:
            ch_printf( ch, "Damage is %d to %d (average %d)%s\n\r",
                       obj->value[1], obj->value[2], ( obj->value[1] + obj->value[2] ) / 2,
                       IS_OBJ_FLAG( obj, ITEM_POISONED ) ? ", and is poisonous." : "." );
            ch_printf( ch, "Skill needed: %s\n\r", weapon_skills[obj->value[4]] );
            ch_printf( ch, "Damage type:  %s\n\r", attack_table[obj->value[3]] );
            ch_printf( ch, "Current condition: %s\n\r", condtxt( obj->value[6], obj->value[0] ) );
            if( obj->value[7] > 0 )
               ch_printf( ch, "Available sockets: %d\n\r", obj->value[7] );
            if( obj->socket[0] && str_cmp( obj->socket[0], "None" ) )
               ch_printf( ch, "Socket 1: %s Rune\n\r", obj->socket[0] );
            if( obj->socket[1] && str_cmp( obj->socket[1], "None" ) )
               ch_printf( ch, "Socket 2: %s Rune\n\r", obj->socket[1] );
            if( obj->socket[2] && str_cmp( obj->socket[2], "None" ) )
               ch_printf( ch, "Socket 3: %s Rune\n\r", obj->socket[2] );
            break;

         case ITEM_MISSILE_WEAPON:
            ch_printf( ch, "Bonus damage added to projectiles is %d to %d (average %d).\n\r",
                       obj->value[1], obj->value[2], ( obj->value[1] + obj->value[2] ) / 2 );
            ch_printf( ch, "Skill needed:      %s\n\r", weapon_skills[obj->value[4]] );
            ch_printf( ch, "Projectiles fired: %s\n\r", projectiles[obj->value[5]] );
            ch_printf( ch, "Current condition: %s\n\r", condtxt( obj->value[6], obj->value[0] ) );
            if( obj->value[7] > 0 )
               ch_printf( ch, "Available sockets: %d\n\r", obj->value[7] );
            if( obj->socket[0] && str_cmp( obj->socket[0], "None" ) )
               ch_printf( ch, "Socket 1: %s Rune\n\r", obj->socket[0] );
            if( obj->socket[1] && str_cmp( obj->socket[1], "None" ) )
               ch_printf( ch, "Socket 2: %s Rune\n\r", obj->socket[1] );
            if( obj->socket[2] && str_cmp( obj->socket[2], "None" ) )
               ch_printf( ch, "Socket 3: %s Rune\n\r", obj->socket[2] );
            break;

         case ITEM_PROJECTILE:
            ch_printf( ch, "Damage is %d to %d (average %d)%s\n\r",
                       obj->value[1], obj->value[2], ( obj->value[1] + obj->value[2] ) / 2,
                       IS_OBJ_FLAG( obj, ITEM_POISONED ) ? ", and is poisonous." : "." );
            ch_printf( ch, "Damage type: %s\n\r", attack_table[obj->value[3]] );
            ch_printf( ch, "Projectile type: %s\n\r", projectiles[obj->value[4]] );
            ch_printf( ch, "Current condition: %s\n\r", condtxt( obj->value[5], obj->value[0] ) );
            break;

         case ITEM_ARMOR:
            ch_printf( ch, "Current AC value: %d\n\r", obj->value[1] );
            ch_printf( ch, "Current condition: %s\n\r", condtxt( obj->value[1], obj->value[0] ) );
            if( obj->value[2] > 0 )
               ch_printf( ch, "Available sockets: %d\n\r", obj->value[2] );
            if( obj->socket[0] && str_cmp( obj->socket[0], "None" ) )
               ch_printf( ch, "Socket 1: %s Rune\n\r", obj->socket[0] );
            if( obj->socket[1] && str_cmp( obj->socket[1], "None" ) )
               ch_printf( ch, "Socket 2: %s Rune\n\r", obj->socket[1] );
            if( obj->socket[2] && str_cmp( obj->socket[2], "None" ) )
               ch_printf( ch, "Socket 3: %s Rune\n\r", obj->socket[2] );
            break;
      }

      for( paf = obj->first_affect; paf; paf = paf->next )
         showaffect( ch, paf );

      for( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
         showaffect( ch, paf );

      return rNONE;
   }

   else if( ( victim = get_char_room( ch, target_name ) ) != NULL )
   {

      if( IS_IMMUNE( victim, RIS_MAGIC ) )
      {
         immune_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      /*
       * If they are morphed or a NPC use the appropriate short_desc otherwise
       * * use their name -- Shaddai
       */

      if( victim->morph && victim->morph->morph )
         name = capitalize( victim->morph->morph->short_desc );
      else if( IS_NPC( victim ) )
         name = capitalize( victim->short_descr );
      else
         name = victim->name;

      ch_printf( ch, "%s appears to be between level %d and %d.\n\r", name, victim->level - ( victim->level % 5 ),
                 victim->level - ( victim->level % 5 ) + 5 );

      if( IS_NPC( victim ) && victim->morph )
         ch_printf( ch, "%s appears to truly be %s.\n\r", name, ( ch->level > victim->level + 10 )
                    ? victim->name : "someone else" );

      ch_printf( ch, "%s looks like %s, and follows the ways of the %s.\n\r", name, aoran( get_race( victim ) ),
                 get_class( victim ) );

      if( ( chance( ch, 50 ) && ch->level >= victim->level + 10 ) || IS_IMMORTAL( ch ) )
      {
         ch_printf( ch, "%s appears to be affected by: ", name );

         if( !victim->first_affect )
         {
            send_to_char( "nothing.\n\r", ch );
            return rNONE;
         }

         for( paf = victim->first_affect; paf; paf = paf->next )
         {
            if( victim->first_affect != victim->last_affect )
            {
               if( paf != victim->last_affect && ( sktmp = get_skilltype( paf->type ) ) != NULL )
                  ch_printf( ch, "%s, ", sktmp->name );

               if( paf == victim->last_affect && ( sktmp = get_skilltype( paf->type ) ) != NULL )
               {
                  ch_printf( ch, "and %s.\n\r", sktmp->name );
                  return rNONE;
               }
            }
            else
            {
               if( ( sktmp = get_skilltype( paf->type ) ) != NULL )
                  ch_printf( ch, "%s.\n\r", sktmp->name );
               else
                  send_to_char( "\n\r", ch );
               return rNONE;
            }
         }
      }
   }

   else
   {
      ch_printf( ch, "You can't find %s!\n\r", target_name );
      return rSPELL_FAILED;
   }
   return rNONE;
}

SPELLF spell_invis( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim;
   SKILLTYPE *skill = get_skilltype( sn );

   /*
    * Modifications on 1/2/96 to work on player/object - Scryn 
    */
   if( target_name[0] == '\0' )
      victim = ch;
   else
      victim = get_char_room( ch, target_name );

   if( victim && is_same_map( ch, victim ) )
   {
      AFFECT_DATA af;

      if( IS_IMMUNE( victim, RIS_MAGIC ) )
      {
         immune_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( IS_AFFECTED( victim, AFF_INVISIBLE ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      act( AT_MAGIC, "$n fades out of existence.", victim, NULL, NULL, TO_ROOM );
      af.type = sn;
      af.duration = ( int )( ( ( level / 4 ) + 12 ) * DUR_CONV );
      af.location = APPLY_NONE;
      af.modifier = 0;
      af.bit = AFF_INVISIBLE;
      affect_to_char( victim, &af );
      act( AT_MAGIC, "You fade out of existence.", victim, NULL, NULL, TO_CHAR );
      return rNONE;
   }
   else
   {
      OBJ_DATA *obj;

      obj = get_obj_carry( ch, target_name );

      if( obj )
      {
         separate_obj( obj ); /* Fix multi-invis bug --Blod */
         if( IS_OBJ_FLAG( obj, ITEM_INVIS ) || chance( ch, 40 + level / 10 ) )
         {
            failed_casting( skill, ch, NULL, NULL );
            return rSPELL_FAILED;
         }
         SET_OBJ_FLAG( obj, ITEM_INVIS );
         act( AT_MAGIC, "$p fades out of existence.", ch, obj, NULL, TO_CHAR );
         return rNONE;
      }
   }
   ch_printf( ch, "You can't find %s!\n\r", target_name );
   return rSPELL_FAILED;
}

SPELLF spell_know_alignment( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   char *msg;
   int ap;
   SKILLTYPE *skill = get_skilltype( sn );

   if( !victim )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( IS_IMMUNE( victim, RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   ap = victim->alignment;

   if( ap > 700 )
      msg = "$N has an aura as white as the driven snow.";
   else if( ap > 350 )
      msg = "$N is of excellent moral character.";
   else if( ap > 100 )
      msg = "$N is often kind and thoughtful.";
   else if( ap > -100 )
      msg = "$N doesn't have a firm moral commitment.";
   else if( ap > -350 )
      msg = "$N lies to $S friends.";
   else if( ap > -700 )
      msg = "$N would just as soon kill you as look at you.";
   else
      msg = "I'd rather just not say anything at all about $N.";

   act( AT_MAGIC, msg, ch, NULL, victim, TO_CHAR );
   return rNONE;
}

SPELLF spell_locate_object( int sn, int level, CHAR_DATA * ch, void *vo )
{
   OBJ_DATA *obj, *in_obj;
   int cnt, found = 0;

   for( obj = first_object; obj; obj = obj->next )
   {
      if( !can_see_obj( ch, obj, TRUE ) || !nifty_is_name( target_name, obj->name ) )
         continue;
      if( ( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) || IS_OBJ_FLAG( obj, ITEM_NOLOCATE ) ) && !IS_IMMORTAL( ch ) )
         continue;

      found++;

      for( cnt = 0, in_obj = obj; in_obj->in_obj && cnt < 100; in_obj = in_obj->in_obj, ++cnt )
         ;
      if( cnt >= MAX_NEST )
      {
         bug( "spell_locate_obj: object [%d] %s is nested more than %d times!",
              obj->pIndexData->vnum, obj->short_descr, MAX_NEST );
         continue;
      }

      set_char_color( AT_MAGIC, ch );
      if( in_obj->carried_by )
      {
         if( IS_IMMORTAL( in_obj->carried_by ) && !IS_NPC( in_obj->carried_by )
             && ( ch->level < in_obj->carried_by->pcdata->wizinvis ) && IS_PLR_FLAG( in_obj->carried_by, PLR_WIZINVIS ) )
         {
            found--;
            continue;
         }
         pager_printf( ch, "%s carried by %s.\n\r", obj_short( obj ), PERS( in_obj->carried_by, ch, FALSE ) );
      }
      else
         pager_printf( ch, "%s in %s.\n\r", obj_short( obj ),
                       in_obj->in_room == NULL ? "somewhere" : in_obj->in_room->name );
   }

   if( !found )
   {
      send_to_char( "Nothing like that exists.\n\r", ch );
      return rSPELL_FAILED;
   }
   return rNONE;
}

SPELLF spell_remove_trap( int sn, int level, CHAR_DATA * ch, void *vo )
{
   OBJ_DATA *obj;
   OBJ_DATA *trap;
   bool found;
   int retcode;
   SKILLTYPE *skill = get_skilltype( sn );

   if( !target_name || target_name[0] == '\0' )
   {
      send_to_char( "Remove trap on what?\n\r", ch );
      return rSPELL_FAILED;
   }

   found = FALSE;

   if( !ch->in_room->first_content )
   {
      send_to_char( "You can't find that here.\n\r", ch );
      return rNONE;
   }

   for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
      if( can_see_obj( ch, obj, FALSE ) && nifty_is_name( target_name, obj->name ) )
      {
         found = TRUE;
         break;
      }

   if( !found )
   {
      send_to_char( "You can't find that here.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( ( trap = get_trap( obj ) ) == NULL )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }


   if( !chance( ch, 70 + get_curr_wis( ch ) ) )
   {
      send_to_char( "Ooops!\n\r", ch );
      retcode = spring_trap( ch, trap );
      if( retcode == rNONE )
         retcode = rSPELL_FAILED;
      return retcode;
   }

   extract_obj( trap );

   successful_casting( skill, ch, NULL, NULL );
   return rNONE;
}

SPELLF spell_sleep( int sn, int level, CHAR_DATA * ch, void *vo )
{
   AFFECT_DATA af;
   int retcode;
   int schance;
   int tmp;
   CHAR_DATA *victim;
   SKILLTYPE *skill = get_skilltype( sn );

   if( ( victim = get_char_room( ch, target_name ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( !IS_NPC( victim ) && victim->fighting )
   {
      send_to_char( "You cannot sleep a fighting player.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( is_safe( ch, victim ) )
      return rSPELL_FAILED;

   if( IS_IMMUNE( victim, RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( SPELL_FLAG( skill, SF_PKSENSITIVE ) && !IS_NPC( ch ) && !IS_NPC( victim ) )
      tmp = level / 2;
   else
      tmp = level;

   if( IS_AFFECTED( victim, AFF_SLEEP ) || ( schance = ris_save( victim, tmp, RIS_SLEEP ) ) == 1000
       || level < victim->level || ( victim != ch && IS_ROOM_FLAG( victim->in_room, ROOM_SAFE ) )
       || saves_spell_staff( schance, victim ) )
   {
      failed_casting( skill, ch, victim, NULL );
      if( ch == victim )
         return rSPELL_FAILED;
      if( !victim->fighting )
      {
         retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
         if( retcode == rNONE )
            retcode = rSPELL_FAILED;
         return retcode;
      }
   }
   af.type = sn;
   af.duration = ( int )( ( 4 + level ) * DUR_CONV );
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bit = AFF_SLEEP;
   affect_join( victim, &af );

   if( IS_AWAKE( victim ) )
   {
      act( AT_MAGIC, "You feel very sleepy ..... zzzzzz.", victim, NULL, NULL, TO_CHAR );
      act( AT_MAGIC, "$n goes to sleep.", victim, NULL, NULL, TO_ROOM );
      victim->position = POS_SLEEPING;
   }
   if( IS_NPC( victim ) )
      start_hating( victim, ch );

   return rNONE;
}

/* Modified by Samson - Uses proper room and area flags to check for failure now */
/* Modified again to block NPC summons - Samson */
SPELLF spell_summon( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim;

   if( ( victim = get_char_world( ch, target_name ) ) == NULL || !victim->in_room )
   {
      send_to_char( "Nobody matching that name is on.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( victim == ch )
   {
      send_to_char( "You can't be serious??\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Summoning NPCs is not permitted.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_NO_SUMMON ) || IS_ROOM_FLAG( victim->in_room, ROOM_NO_SUMMON )
       || IS_AREA_FLAG( victim->in_room->area, AFLAG_NOSUMMON ) || IS_AREA_FLAG( ch->in_room->area, AFLAG_NOSUMMON ) )
   {
      send_to_char( "Arcane magic blocks the spell.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_IMMORTAL( victim ) && !IS_IMMORTAL( ch ) )
   {
      send_to_char( "Summoning immortals is not permitted.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_PCFLAG( victim, PCFLAG_NOSUMMON ) && !IS_NPC( ch ) )
   {
      send_to_char( "That person does not permit summonings.\n\r", ch );
      return rSPELL_FAILED;
   }

   /*
    * Check to see if victim has been here at least once - Samson 7-11-00 
    */
   if( !has_visited( victim, ch->in_room->area ) )
   {
      ch_printf( ch, "%s has not yet formed a magical bond with this area!\n\r", victim->name );
      return rSPELL_FAILED;
   }

   if( victim->fighting )
   {
      send_to_char( "You cannot obtain an accurate fix, they are moving around too much!\n\r", ch );
      return rSPELL_FAILED;
   }

   if( !IS_NPC( ch ) && !IS_IMMORTAL( ch ) )
   {
      act( AT_MAGIC, "You feel a wave of nausea overcome you...", ch, NULL, NULL, TO_CHAR );
      act( AT_MAGIC, "$n collapses, stunned!", ch, NULL, NULL, TO_ROOM );
      ch->position = POS_STUNNED;
   }

   act( AT_MAGIC, "$n disappears suddenly.", victim, NULL, NULL, TO_ROOM );

   leave_map( victim, ch, ch->in_room );

   act( AT_MAGIC, "$n arrives suddenly.", victim, NULL, NULL, TO_ROOM );
   act( AT_MAGIC, "$N has summoned you!", victim, NULL, ch, TO_CHAR );

   return rNONE;
}

/*
 * Travel via the astral plains to quickly travel to desired location
 *	-Thoric
 *
 * Uses SMAUG spell messages is available to allow use as a SMAUG spell
 */
/* Modified by Samson, arrives in Astral Plane area at randomly chosen location */
SPELLF spell_astral_walk( int sn, int level, CHAR_DATA * ch, void *vo )
{
   ROOM_INDEX_DATA *location, *original;
   CHAR_DATA *vch, *vch_next;

   if( ( location = get_room_index( astral_target ) ) == NULL
       || IS_ROOM_FLAG( ch->in_room, ROOM_NO_ASTRAL ) || IS_AREA_FLAG( ch->in_room->area, AFLAG_NOASTRAL ) )
   {
      send_to_char( "&[magic]A mysterious force prevents you from opening a gateway.\n\r", ch );
      return rSPELL_FAILED;
   }

   act( AT_MAGIC, "$n opens a gateway in the air, and steps through onto another plane!", ch, NULL, NULL, TO_ROOM );

   original = ch->in_room;
   leave_map( ch, NULL, location );

   act( AT_MAGIC, "You open a gateway onto another plane!", ch, NULL, NULL, TO_CHAR );
   act( AT_MAGIC, "$n appears from a gateway in thin air!", ch, NULL, NULL, TO_ROOM );

   for( vch = original->first_person; vch; vch = vch_next )
   {
      vch_next = vch->next_in_room;

      if( !is_same_group( vch, ch ) )
         continue;

      act( AT_MAGIC, "The gateway remains open long enough for you to follow $s through.", ch, NULL, vch, TO_VICT );
      leave_map( vch, ch, location );
   }
   astral_target = number_range( 4350, 4449 );
   return rNONE;
}

/* Modified by Samson to check for proper room and area flags */
SPELLF spell_teleport( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   ROOM_INDEX_DATA *pRoomIndex, *from;
   SKILLTYPE *skill = get_skilltype( sn );
   int schance;

   if( !victim->in_room )
   {
      send_to_char( "No such person is here.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_ROOM_FLAG( victim->in_room, ROOM_NOTELEPORT ) || IS_AREA_FLAG( victim->in_room->area, AFLAG_NOTELEPORT ) )
   {
      send_to_char( "Arcane magic disrupts the spell.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( ( !IS_NPC( ch ) && victim->fighting )
       || ( victim != ch && ( saves_spell_staff( level, victim ) || saves_wands( level, victim ) ) ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   schance = number_range( 1, 2 );

   if( ch->on )
   {
      ch->on = NULL;
      ch->position = POS_STANDING;
   }
   if( ch->position != POS_STANDING )
      ch->position = POS_STANDING;

   from = victim->in_room;

   if( schance == 1 )
   {
      short map, x, y, sector;

      for( ;; )
      {
         map = number_range( 0, MAP_MAX - 1 );
         x = number_range( 0, MAX_X - 1 );
         y = number_range( 0, MAX_Y - 1 );

         sector = get_terrain( map, x, y );
         if( sector == -1 )
            continue;
         if( sect_show[sector].canpass )
            break;
      }
      act( AT_MAGIC, "$n slowly fades out of view.", victim, NULL, NULL, TO_ROOM );
      enter_map( victim, NULL, x, y, map );
      collect_followers( victim, from, victim->in_room );
      if( !IS_NPC( victim ) )
         act( AT_MAGIC, "$n slowly fades into view.", victim, NULL, NULL, TO_ROOM );
   }
   else
   {
      for( ;; )
      {
         pRoomIndex = get_room_index( number_range( 0, sysdata.maxvnum ) );
         if( pRoomIndex )
            if( !IS_ROOM_FLAG( pRoomIndex, ROOM_NOTELEPORT )
                && !IS_AREA_FLAG( pRoomIndex->area, AFLAG_NOTELEPORT )
                && !IS_ROOM_FLAG( pRoomIndex, ROOM_PROTOTYPE ) && in_hard_range( victim, pRoomIndex->area ) )
               break;
      }
      act( AT_MAGIC, "$n slowly fades out of view.", victim, NULL, NULL, TO_ROOM );
      leave_map( victim, NULL, pRoomIndex );
      if( !IS_NPC( victim ) )
         act( AT_MAGIC, "$n slowly fades into view.", victim, NULL, NULL, TO_ROOM );
   }
   return rNONE;
}

SPELLF spell_ventriloquate( int sn, int level, CHAR_DATA * ch, void *vo )
{
   char buf1[MSL], buf2[MSL], speaker[MIL];
   CHAR_DATA *vch;

   target_name = one_argument( target_name, speaker );

   snprintf( buf1, MSL, "%s says '%s'.\n\r", speaker, target_name );
   snprintf( buf2, MSL, "Someone makes %s say '%s'.\n\r", speaker, target_name );
   buf1[0] = UPPER( buf1[0] );

   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
   {
      if( !is_name( speaker, vch->name ) && is_same_map( ch, vch ) )
         ch_printf( vch, "&[say]%s\n\r", saves_spell_staff( level, vch ) ? buf2 : buf1 );
   }
   return rNONE;
}

/* MUST BE KEPT!!!! Used by attack types for mobiles!!! */
SPELLF spell_weaken( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   AFFECT_DATA af;
   SKILLTYPE *skill = get_skilltype( sn );

   set_char_color( AT_MAGIC, ch );
   if( IS_IMMUNE( victim, RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   if( is_affected( victim, sn ) || saves_wands( level, victim ) )
   {
      send_to_char( "Your magic fails to take hold.\n\r", ch );
      return rSPELL_FAILED;
   }
   af.type = sn;
   af.duration = ( int )( level / 2 * DUR_CONV );
   af.location = APPLY_STR;
   af.modifier = -2;
   af.bit = 0;
   affect_to_char( victim, &af );
   send_to_char( "&[magic]Your muscles seem to atrophy!\n\r", victim );
   if( ch != victim )
   {
      if( ( ( ( !IS_NPC( victim ) && class_table[victim->Class]->attr_prime == APPLY_STR ) || IS_NPC( victim ) )
            && get_curr_str( victim ) < 25 ) || get_curr_str( victim ) < 20 )
      {
         act( AT_MAGIC, "$N labors weakly as your spell atrophies $S muscles.", ch, NULL, victim, TO_CHAR );
         act( AT_MAGIC, "$N labors weakly as $n's spell atrophies $S muscles.", ch, NULL, victim, TO_NOTVICT );
      }
      else
      {
         act( AT_MAGIC, "You induce a mild atrophy in $N's muscles.", ch, NULL, victim, TO_CHAR );
         act( AT_MAGIC, "$n induces a mild atrophy in $N's muscles.", ch, NULL, victim, TO_NOTVICT );
      }
   }
   return rNONE;
}

/*
 * A spell as it should be				-Thoric
 */
SPELLF spell_word_of_recall( int sn, int level, CHAR_DATA * ch, void *vo )
{
   char arg3[MIL];
   int call = -1;
   int target = -1;

   target_name = one_argument( target_name, arg3 );

   if( arg3[0] == '\0' )
      call = recall( ch, -1 );

   else
   {
      target = atoi( arg3 );

      if( target < 0 || target >= MAX_BEACONS )
      {
         send_to_char( "Beacon value is out of range.\n\r", ch );
         return rSPELL_FAILED;
      }
      call = recall( ch, target );
   }

   if( call == 1 )
      return rNONE;
   else
      return rSPELL_FAILED;
}

/*
 * NPC spells.
 */
SPELLF spell_acid_breath( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   OBJ_DATA *obj_lose;
   OBJ_DATA *obj_next;
   int dam;
   int hpch;

   if( chance( ch, 2 * level ) && !saves_breath( level, victim ) )
   {
      for( obj_lose = victim->first_carrying; obj_lose; obj_lose = obj_next )
      {
         int iWear;

         obj_next = obj_lose->next_content;

         if( number_bits( 2 ) != 0 )
            continue;

         switch ( obj_lose->item_type )
         {
            case ITEM_ARMOR:
               if( obj_lose->value[0] > 0 )
               {
                  separate_obj( obj_lose );
                  act( AT_DAMAGE, "$p is pitted and etched!", victim, obj_lose, NULL, TO_CHAR );
                  if( ( iWear = obj_lose->wear_loc ) != WEAR_NONE )
                     victim->armor += apply_ac( obj_lose, iWear );
                  obj_lose->value[0] -= 1;
                  obj_lose->cost = 0;
                  if( iWear != WEAR_NONE )
                     victim->armor -= apply_ac( obj_lose, iWear );
               }
               break;

            case ITEM_CONTAINER:
               separate_obj( obj_lose );
               act( AT_DAMAGE, "$p fumes and dissolves!", victim, obj_lose, NULL, TO_CHAR );
               act( AT_OBJECT, "The contents of $p held by $N spill onto the ground.", victim, obj_lose, victim, TO_ROOM );
               act( AT_OBJECT, "The contents of $p spill out onto the ground!", victim, obj_lose, NULL, TO_CHAR );
               empty_obj( obj_lose, NULL, victim->in_room );
               extract_obj( obj_lose );
               break;
         }
      }
   }

   hpch = UMAX( 10, ch->hit );
   dam = number_range( hpch / 16 + 1, hpch / 8 );
   if( saves_breath( level, victim ) )
      dam /= 2;
   return damage( ch, victim, dam, sn );
}

SPELLF spell_fire_breath( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   OBJ_DATA *obj_lose;
   OBJ_DATA *obj_next;
   int dam;
   int hpch;

   if( chance( ch, 2 * level ) && !saves_breath( level, victim ) )
   {
      for( obj_lose = victim->first_carrying; obj_lose; obj_lose = obj_next )
      {
         char *msg;

         obj_next = obj_lose->next_content;
         if( number_bits( 2 ) != 0 )
            continue;

         switch ( obj_lose->item_type )
         {
            default:
               continue;
            case ITEM_CONTAINER:
               msg = "$p ignites and burns!";
               break;
            case ITEM_POTION:
               msg = "$p bubbles and boils!";
               break;
            case ITEM_SCROLL:
               msg = "$p crackles and burns!";
               break;
            case ITEM_STAFF:
               msg = "$p smokes and chars!";
               break;
            case ITEM_WAND:
               msg = "$p sparks and sputters!";
               break;
            case ITEM_COOK:
            case ITEM_FOOD:
               msg = "$p blackens and crisps!";
               break;
            case ITEM_PILL:
               msg = "$p melts and drips!";
               break;
         }

         separate_obj( obj_lose );
         act( AT_DAMAGE, msg, victim, obj_lose, NULL, TO_CHAR );
         if( obj_lose->item_type == ITEM_CONTAINER )
         {
            act( AT_OBJECT, "The contents of $p held by $N spill onto the ground.", victim, obj_lose, victim, TO_ROOM );
            act( AT_OBJECT, "The contents of $p spill out onto the ground!", victim, obj_lose, NULL, TO_CHAR );
            empty_obj( obj_lose, NULL, victim->in_room );
         }
         extract_obj( obj_lose );
      }
   }

   hpch = UMAX( 10, ch->hit );
   dam = number_range( hpch / 16 + 1, hpch / 8 );
   if( saves_breath( level, victim ) )
      dam /= 2;
   return damage( ch, victim, dam, sn );
}

SPELLF spell_frost_breath( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   OBJ_DATA *obj_lose;
   OBJ_DATA *obj_next;
   int dam;
   int hpch;

   if( chance( ch, 2 * level ) && !saves_breath( level, victim ) )
   {
      for( obj_lose = victim->first_carrying; obj_lose; obj_lose = obj_next )
      {
         char *msg;

         obj_next = obj_lose->next_content;
         if( number_bits( 2 ) != 0 )
            continue;

         switch ( obj_lose->item_type )
         {
            default:
               continue;
            case ITEM_CONTAINER:
            case ITEM_DRINK_CON:
            case ITEM_POTION:
               msg = "$p freezes and shatters!";
               break;
         }

         separate_obj( obj_lose );
         act( AT_DAMAGE, msg, victim, obj_lose, NULL, TO_CHAR );
         if( obj_lose->item_type == ITEM_CONTAINER )
         {
            act( AT_OBJECT, "The contents of $p held by $N spill onto the ground.", victim, obj_lose, victim, TO_ROOM );
            act( AT_OBJECT, "The contents of $p spill out onto the ground!", victim, obj_lose, NULL, TO_CHAR );
            empty_obj( obj_lose, NULL, victim->in_room );
         }
         extract_obj( obj_lose );
      }
   }

   hpch = UMAX( 10, ch->hit );
   dam = number_range( hpch / 16 + 1, hpch / 8 );
   if( saves_breath( level, victim ) )
      dam /= 2;
   return damage( ch, victim, dam, sn );
}

SPELLF spell_gas_breath( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *vch, *vch_next;
   int dam, hpch;
   bool ch_died;

   ch_died = FALSE;

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SAFE ) )
   {
      send_to_char( "&[magic]You fail to breathe.\n\r", ch );
      return rNONE;
   }

   for( vch = ch->in_room->first_person; vch; vch = vch_next )
   {
      vch_next = vch->next_in_room;
      if( IS_PLR_FLAG( vch, PLR_WIZINVIS ) && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
         continue;

      if( IS_PLR_FLAG( vch, PLR_ONMAP ) || IS_ACT_FLAG( vch, ACT_ONMAP ) )
      {
         if( vch->map != ch->map || vch->x != ch->x || vch->y != ch->y )
            continue;
      }

      if( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) )
      {
         hpch = UMAX( 10, ch->hit );
         dam = number_range( hpch / 16 + 1, hpch / 8 );
         if( saves_breath( level, vch ) )
            dam /= 2;
         if( damage( ch, vch, dam, sn ) == rCHAR_DIED || char_died( ch ) )
            ch_died = TRUE;
      }
   }
   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}

SPELLF spell_lightning_breath( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   int dam, hpch;

   hpch = UMAX( 10, ch->hit );
   dam = number_range( hpch / 16 + 1, hpch / 8 );
   if( saves_breath( level, victim ) )
      dam /= 2;
   return damage( ch, victim, dam, sn );
}

SPELLF spell_null( int sn, int level, CHAR_DATA * ch, void *vo )
{
   send_to_char( "That's not a spell!\n\r", ch );
   return rNONE;
}

/* don't remove, may look redundant, but is important */
SPELLF spell_notfound( int sn, int level, CHAR_DATA * ch, void *vo )
{
   send_to_char( "That's not a spell!\n\r", ch );
   return rNONE;
}

/*
 *   Haus' Spell Additions
 *
 */

/* to do: portal           (like mpcreatepassage)
 *        sharpness        (makes weapon of caster's level)
 *        repair           (repairs armor)
 *        blood burn       (offensive)  * name: net book of spells *
 *        spirit scream    (offensive)  * name: net book of spells *
 *        something about saltpeter or brimstone
 */

/* Working on DM's transport eq suggestion - Scryn 8/13 */
SPELLF spell_transport( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim;
   char arg3[MSL];
   OBJ_DATA *obj;
   SKILLTYPE *skill = get_skilltype( sn );

   target_name = one_argument( target_name, arg3 );

   if( ( victim = get_char_world( ch, target_name ) ) == NULL
       || victim == ch
       || IS_ROOM_FLAG( victim->in_room, ROOM_PRIVATE )
       || IS_ROOM_FLAG( victim->in_room, ROOM_SOLITARY )
       || IS_ROOM_FLAG( victim->in_room, ROOM_NOTELEPORT )
       || IS_AREA_FLAG( victim->in_room->area, AFLAG_NOTELEPORT )
       || IS_ROOM_FLAG( ch->in_room, ROOM_NOTELEPORT )
       || IS_AREA_FLAG( ch->in_room->area, AFLAG_NOTELEPORT )
       || IS_ROOM_FLAG( victim->in_room, ROOM_PROTOTYPE )
       || IS_ACT_FLAG( victim, ACT_PROTOTYPE ) || ( IS_NPC( victim ) && saves_spell_staff( level, victim ) ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( victim->in_room == ch->in_room && is_same_map( victim, ch ) )
   {
      send_to_char( "They are right beside you!", ch );
      return rSPELL_FAILED;
   }

   if( ( obj = get_obj_carry( ch, arg3 ) ) == NULL
       || ( victim->carry_weight + get_obj_weight( obj ) ) > can_carry_w( victim ) || IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   separate_obj( obj ); /* altrag shoots, haus alley-oops! */

   if( IS_OBJ_FLAG( obj, ITEM_NODROP ) || IS_OBJ_FLAG( obj, ITEM_SINDHAE ) )
   {
      send_to_char( "You can't seem to let go of it.\n\r", ch );
      return rSPELL_FAILED;   /* nice catch, caine */
   }

   if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) && victim->level < LEVEL_IMMORTAL )
   {
      send_to_char( "That item is not for mortal hands to touch!\n\r", ch );
      return rSPELL_FAILED;   /* Thoric */
   }

   act( AT_MAGIC, "$p slowly dematerializes...", ch, obj, NULL, TO_CHAR );
   act( AT_MAGIC, "$p slowly dematerializes from $n's hands..", ch, obj, NULL, TO_ROOM );
   obj_from_char( obj );
   obj_to_char( obj, victim );
   act( AT_MAGIC, "$p from $n appears in your hands!", ch, obj, victim, TO_VICT );
   act( AT_MAGIC, "$p appears in $n's hands!", victim, obj, NULL, TO_ROOM );
   save_char_obj( ch );
   save_char_obj( victim );
   return rNONE;
}

/*
 * Syntax portal (mob/char) 
 * opens a 2-way EX_PORTAL from caster's room to room inhabited by  
 *  mob or character won't mess with existing exits
 *
 * do_mp_open_passage, combined with spell_astral
 */
SPELLF spell_portal( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim;
   ROOM_INDEX_DATA *targetRoom, *fromRoom;
   int targetRoomVnum;
   OBJ_DATA *portalObj;
   EXIT_DATA *pexit;
   SKILLTYPE *skill = get_skilltype( sn );

   /*
    * No go if all kinds of things aren't just right, including the caster
    * and victim are not both pkill or both peaceful. -- Narn
    */
   if( ( victim = get_char_world( ch, target_name ) ) == NULL
       || !victim->in_room || IS_ROOM_FLAG( victim->in_room, ROOM_PROTOTYPE ) || IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
   {
      send_to_char( "Nobody matching that target name is around.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( victim == ch )
   {
      send_to_char( "What?? Make a portal to yourself?\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_PLR_FLAG( victim, PLR_ONMAP )
       || IS_ACT_FLAG( ch, ACT_ONMAP ) || IS_ACT_FLAG( victim, ACT_ONMAP ) )
   {
      send_to_char( "Portals cannot be created to or from overland maps.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( victim->in_room == ch->in_room )
   {
      send_to_char( "They are right beside you!", ch );
      return rSPELL_FAILED;
   }

   if( IS_ROOM_FLAG( victim->in_room, ROOM_NO_PORTAL ) || IS_AREA_FLAG( victim->in_room->area, AFLAG_NOPORTAL )
       || IS_ROOM_FLAG( ch->in_room, ROOM_NO_PORTAL ) || IS_AREA_FLAG( ch->in_room->area, AFLAG_NOPORTAL ) )
   {
      send_to_char( "Arcane magic blocks the formation of the portal.\n\r", ch );
      return rSPELL_FAILED;
   }

   /*
    * Check to see if ch has actually been there at least once - Samson 7-11-00 
    */
   if( !has_visited( ch, victim->in_room->area ) )
   {
      send_to_char( "You have not formed a magical bond with that area yet!\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_NPC( victim ) && saves_spell_staff( level, victim ) )
   {
      send_to_char( "Your target resisted the attempt to create the portal.\n\r", ch );
      send_to_char( "Your powers have thwarted an attempted portal...\n\r", victim );
      return rSPELL_FAILED;
   }

   targetRoomVnum = victim->in_room->vnum;
   fromRoom = ch->in_room;
   targetRoom = victim->in_room;

   /*
    * Check if there already is a portal in either room. 
    */
   for( pexit = fromRoom->first_exit; pexit; pexit = pexit->next )
   {
      if( IS_EXIT_FLAG( pexit, EX_PORTAL ) )
      {
         send_to_char( "There is already a portal in this room.\n\r", ch );
         return rSPELL_FAILED;
      }

      if( pexit->vdir == DIR_PORTAL )
      {
         send_to_char( "You may not create a portal in this room.\n\r", ch );
         return rSPELL_FAILED;
      }
   }

   for( pexit = targetRoom->first_exit; pexit; pexit = pexit->next )
      if( pexit->vdir == DIR_PORTAL )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

   pexit = make_exit( fromRoom, targetRoom, DIR_PORTAL );
   pexit->keyword = STRALLOC( "portal" );
   pexit->exitdesc = STRALLOC( "You gaze into the shimmering portal...\n\r" );
   pexit->key = -1;
   SET_EXIT_FLAG( pexit, EX_PORTAL );
   SET_EXIT_FLAG( pexit, EX_xENTER );
   SET_EXIT_FLAG( pexit, EX_HIDDEN );
   SET_EXIT_FLAG( pexit, EX_xLOOK );
   pexit->vnum = targetRoomVnum;

   if( !( portalObj = create_object( get_obj_index( OBJ_VNUM_PORTAL ), 0 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return rSPELL_FAILED;
   }
   portalObj->timer = 3;
   stralloc_printf( &portalObj->short_descr, "a portal created by %s", ch->name );

   /*
    * support for new casting messages 
    */
   if( !skill->hit_char || skill->hit_char[0] == '\0' )
      send_to_char( "&[magic]You utter an incantation, and a portal forms in front of you!\n\r", ch );
   else
      act( AT_MAGIC, skill->hit_char, ch, NULL, victim, TO_CHAR );
   if( !skill->hit_room || skill->hit_room[0] == '\0' )
      act( AT_MAGIC, "$n utters an incantation, and a portal forms in front of you!", ch, NULL, NULL, TO_ROOM );
   else
      act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_ROOM );
   if( !skill->hit_vict || skill->hit_vict[0] == '\0' )
      act( AT_MAGIC, "A shimmering portal forms in front of you!", victim, NULL, NULL, TO_ROOM );
   else
      act( AT_MAGIC, skill->hit_vict, victim, NULL, victim, TO_ROOM );
   portalObj = obj_to_room( portalObj, ch->in_room, ch );

   pexit = make_exit( targetRoom, fromRoom, DIR_PORTAL );
   pexit->keyword = STRALLOC( "portal" );
   pexit->exitdesc = STRALLOC( "You gaze into the shimmering portal...\n\r" );
   pexit->key = -1;
   SET_EXIT_FLAG( pexit, EX_PORTAL );
   SET_EXIT_FLAG( pexit, EX_xENTER );
   SET_EXIT_FLAG( pexit, EX_HIDDEN );
   pexit->vnum = targetRoomVnum;

   portalObj = create_object( get_obj_index( OBJ_VNUM_PORTAL ), 0 );
   portalObj->timer = 3;
   stralloc_printf( &portalObj->short_descr, "a portal created by %s", ch->name );
   portalObj = obj_to_room( portalObj, targetRoom, victim );
   return rNONE;
}

SPELLF spell_farsight( int sn, int level, CHAR_DATA * ch, void *vo )
{
   ROOM_INDEX_DATA *location, *original;
   CHAR_DATA *victim;
   SKILLTYPE *skill = get_skilltype( sn );
   int origmap, origx, origy;
   bool visited;

   /*
    * The spell fails if the victim isn't playing, the victim is the caster,
    * the target room has private, solitary, noastral, death or proto flags,
    * the caster's room is norecall, the victim is too high in level, the 
    * victim is a proto mob, the victim makes the saving throw or the pkill 
    * flag on the caster is not the same as on the victim.  Got it?
    */
   /*
    * Sure do, now bite me, I'm changing it anyway guys :P - Samson 
    */
   if( ( victim = get_char_world( ch, target_name ) ) == NULL
       || victim == ch || !victim->in_room || IS_ROOM_FLAG( victim->in_room, ROOM_PROTOTYPE )
       || IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
   {
      send_to_char( "Nobody matching that target name is around.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_ROOM_FLAG( victim->in_room, ROOM_NOSCRY ) || IS_ROOM_FLAG( ch->in_room, ROOM_NOSCRY )
       || IS_AREA_FLAG( victim->in_room->area, AFLAG_NOSCRY ) || IS_AREA_FLAG( ch->in_room->area, AFLAG_NOSCRY ) )
   {
      send_to_char( "Arcane magic blocks your vision.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_NPC( victim ) && saves_spell_staff( level, victim ) )
   {
      send_to_char( "Your target resisted the spell.\n\r", ch );
      send_to_char( "You feel as though someone is watching you....\n\r", victim );
      return rSPELL_FAILED;
   }

   location = victim->in_room;
   if( !location )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   successful_casting( skill, ch, victim, NULL );

   original = ch->in_room;
   origmap = ch->map;
   origx = ch->x;
   origy = ch->y;

   /*
    * Bunch of checks to make sure the caster is on the same grid as the target - Samson 
    */
   if( IS_ROOM_FLAG( location, ROOM_MAP ) && !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      SET_PLR_FLAG( ch, PLR_ONMAP );
      ch->map = victim->map;
      ch->x = victim->x;
      ch->y = victim->y;
   }
   else if( IS_ROOM_FLAG( location, ROOM_MAP ) && IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      ch->map = victim->map;
      ch->x = victim->x;
      ch->y = victim->y;
   }
   else if( !IS_ROOM_FLAG( location, ROOM_MAP ) && IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      REMOVE_PLR_FLAG( ch, PLR_ONMAP );
      ch->map = -1;
      ch->x = -1;
      ch->y = -1;
   }

   visited = has_visited( ch, location->area );
   char_from_room( ch );
   if( !char_to_room( ch, location ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   interpret( ch, "look" );
   char_from_room( ch );
   if( !char_to_room( ch, original ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   if( !visited )
      remove_visit( ch, location );

   if( IS_PLR_FLAG( ch, PLR_ONMAP ) && !IS_ROOM_FLAG( original, ROOM_MAP ) )
      REMOVE_PLR_FLAG( ch, PLR_ONMAP );
   else if( !IS_PLR_FLAG( ch, PLR_ONMAP ) && IS_ROOM_FLAG( original, ROOM_MAP ) )
      SET_PLR_FLAG( ch, PLR_ONMAP );

   ch->map = origmap;
   ch->x = origx;
   ch->y = origy;
   return rNONE;
}

SPELLF spell_recharge( int sn, int level, CHAR_DATA * ch, void *vo )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;

   if( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND )
   {
      separate_obj( obj );
      if( obj->value[2] == obj->value[1] || obj->value[1] > ( obj->pIndexData->value[1] * 4 ) )
      {
         act( AT_FIRE, "$p bursts into flames, injuring you!", ch, obj, NULL, TO_CHAR );
         act( AT_FIRE, "$p bursts into flames, charring $n!", ch, obj, NULL, TO_ROOM );
         extract_obj( obj );
         if( damage( ch, ch, obj->level * 2, TYPE_UNDEFINED ) == rCHAR_DIED || char_died( ch ) )
            return rCHAR_DIED;
         else
            return rSPELL_FAILED;
      }

      if( chance( ch, 2 ) )
      {
         act( AT_YELLOW, "$p glows with a blinding magical luminescence.", ch, obj, NULL, TO_CHAR );
         obj->value[1] *= 2;
         obj->value[2] = obj->value[1];
         return rNONE;
      }
      else if( chance( ch, 5 ) )
      {
         act( AT_YELLOW, "$p glows brightly for a few seconds...", ch, obj, NULL, TO_CHAR );
         obj->value[2] = obj->value[1];
         return rNONE;
      }
      else if( chance( ch, 10 ) )
      {
         act( AT_WHITE, "$p disintegrates into a void.", ch, obj, NULL, TO_CHAR );
         act( AT_WHITE, "$n's attempt at recharging fails, and $p disintegrates.", ch, obj, NULL, TO_ROOM );
         extract_obj( obj );
         return rSPELL_FAILED;
      }
      else if( chance( ch, 50 - ( ch->level / 2 ) ) )
      {
         send_to_char( "Nothing happens.\n\r", ch );
         return rSPELL_FAILED;
      }
      else
      {
         act( AT_MAGIC, "$p feels warm to the touch.", ch, obj, NULL, TO_CHAR );
         --obj->value[1];
         obj->value[2] = obj->value[1];
         return rNONE;
      }
   }
   else
   {
      send_to_char( "You can't recharge that!\n\r", ch );
      return rSPELL_FAILED;
   }
}

/*
 * Idea from AD&D 2nd edition player's handbook (c)1989 TSR Hobbies Inc.
 * -Thoric
 */
SPELLF spell_plant_pass( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim;
   SKILLTYPE *skill = get_skilltype( sn );

   if( ( victim = get_char_world( ch, target_name ) ) == NULL
       || victim == ch
       || !victim->in_room
       || IS_ROOM_FLAG( victim->in_room, ROOM_PRIVATE )
       || IS_ROOM_FLAG( victim->in_room, ROOM_SOLITARY )
       || IS_ROOM_FLAG( victim->in_room, ROOM_NO_ASTRAL )
       || IS_ROOM_FLAG( victim->in_room, ROOM_DEATH )
       || IS_ROOM_FLAG( victim->in_room, ROOM_PROTOTYPE )
       || ( victim->in_room->sector_type != SECT_FOREST && victim->in_room->sector_type != SECT_FIELD )
       || ( ch->in_room->sector_type != SECT_FOREST && ch->in_room->sector_type != SECT_FIELD )
       || IS_ROOM_FLAG( ch->in_room, ROOM_NO_RECALL )
       || victim->level >= level + 15
       || ( CAN_PKILL( victim ) && !IS_NPC( ch ) && !IS_PKILL( ch ) )
       || IS_ACT_FLAG( victim, ACT_PROTOTYPE )
       || ( IS_NPC( victim ) && saves_spell_staff( level, victim ) )
       || !in_hard_range( ch, victim->in_room->area )
       || ( IS_AREA_FLAG( victim->in_room->area, AFLAG_NOPKILL ) && IS_PKILL( ch ) ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   /*
    * Check to see if ch has visited the target area - Samson 7-11-00 
    */
   if( !has_visited( ch, victim->in_room->area ) )
   {
      send_to_char( "You have not yet formed a magical bond with that area!\n\r", ch );
      return rSPELL_FAILED;
   }

   if( ch->in_room->sector_type == SECT_FOREST )
      act( AT_MAGIC, "$n melds into a nearby tree!", ch, NULL, NULL, TO_ROOM );
   else
      act( AT_MAGIC, "$n melds into the grass!", ch, NULL, NULL, TO_ROOM );

   leave_map( ch, victim, victim->in_room );

   if( ch->in_room->sector_type == SECT_FOREST )
      act( AT_MAGIC, "$n appears from behind a nearby tree!", ch, NULL, NULL, TO_ROOM );
   else
      act( AT_MAGIC, "$n grows up from the grass!", ch, NULL, NULL, TO_ROOM );

   return rNONE;
}

/* Scryn 2/2/96 */
SPELLF spell_remove_invis( int sn, int level, CHAR_DATA * ch, void *vo )
{
   OBJ_DATA *obj;
   SKILLTYPE *skill = get_skilltype( sn );

   if( target_name[0] == '\0' )
   {
      send_to_char( "What should the spell be cast upon?\n\r", ch );
      return rSPELL_FAILED;
   }

   obj = get_obj_carry( ch, target_name );

   if( obj )
   {
      if( !IS_OBJ_FLAG( obj, ITEM_INVIS ) )
      {
         send_to_char( "Its not invisible!\n\r", ch );
         return rSPELL_FAILED;
      }
      REMOVE_OBJ_FLAG( obj, ITEM_INVIS );
      act( AT_MAGIC, "$p becomes visible again.", ch, obj, NULL, TO_CHAR );

      send_to_char( "Ok.\n\r", ch );
      return rNONE;
   }
   else
   {
      CHAR_DATA *victim;

      victim = get_char_room( ch, target_name );

      if( victim )
      {
         if( !can_see( ch, victim, FALSE ) )
         {
            ch_printf( ch, "You don't see %s!\n\r", target_name );
            return rSPELL_FAILED;
         }

         if( !IS_AFFECTED( victim, AFF_INVISIBLE ) )
         {
            send_to_char( "They are not invisible!\n\r", ch );
            return rSPELL_FAILED;
         }

         if( is_safe( ch, victim ) )
         {
            failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
         }

         if( IS_IMMUNE( victim, RIS_MAGIC ) )
         {
            immune_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
         }
         if( !IS_NPC( victim ) )
         {
            if( chance( ch, 50 ) && ch->level + 10 < victim->level )
            {
               failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
            else
            {
               if( check_illegal_pk( ch, victim ) )
               {
                  send_to_char( "You cannot cast that on another player!\n\r", ch );
                  return rSPELL_FAILED;
               }
            }
         }
         else
         {
            if( chance( ch, 50 ) && ch->level + 15 < victim->level )
            {
               failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
         }

         affect_strip( victim, gsn_invis );
         affect_strip( victim, gsn_mass_invis );
         REMOVE_AFFECTED( victim, AFF_INVISIBLE );
         successful_casting( skill, ch, victim, NULL );
         return rNONE;
      }
      ch_printf( ch, "You can't find %s!\n\r", target_name );
      return rSPELL_FAILED;
   }
}

/* New Animate Dead by Whir 8/29/98
 * Original by Scryn and Altrag
 */
SPELLF spell_animate_dead( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *mob;
   OBJ_DATA *corpse;
   OBJ_DATA *corpse_next;
   OBJ_DATA *obj;
   OBJ_DATA *obj_next;
   bool found;
   MOB_INDEX_DATA *pMobIndex;
   char arg[MIL];
   SKILLTYPE *skill = get_skilltype( sn );   /* 4370 */
   char *corpse_name = NULL;
   int sindex = -1;

   found = FALSE;

   target_name = one_argument( target_name, arg );

   for( corpse = ch->in_room->first_content; corpse; corpse = corpse_next )
   {
      corpse_next = corpse->next_content;

      if( corpse->item_type == ITEM_CORPSE_NPC && corpse->cost != -5
          && corpse->x == ch->x && corpse->y == ch->y && corpse->map == ch->map )
      {
         found = TRUE;
         break;
      }
   }

   if( !found )
   {
      send_to_char( "You can't find a suitable corpse.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( ( pMobIndex = get_mob_index( corpse->value[4] ) ) == NULL )
   {
      bug( "%s", "Can't find mob for value[4] of corpse, spell_animate_dead" );
      return rSPELL_FAILED;
   }

   if( !can_charm( ch ) )
   {
      send_to_char( "You already have too many followers to support!\n\r", ch );
      return rSPELL_FAILED;
   }

   if( ch->mana - ( pMobIndex->level * 4 ) < 0 )
   {
      send_to_char( "You don't have enough mana to raise this corpse.\n\r", ch );
      return rSPELL_FAILED;
   }
   else
      ch->mana -= ( pMobIndex->level * 4 );  /* 4400 */

   if( IS_IMMORTAL( ch ) || chance( ch, 75 ) )
   {
      if( !str_cmp( arg, "skeleton" ) )
      {
         sindex = MOB_VNUM_ANIMATED_SKELETON;
         corpse_name = "skeleton";
      }

      else if( !str_cmp( arg, "zombie" ) )
      {
         sindex = MOB_VNUM_ANIMATED_ZOMBIE;
         corpse_name = "zombie";
      }

      else if( !str_cmp( arg, "ghoul" ) )
      {
         sindex = MOB_VNUM_ANIMATED_GHOUL;
         corpse_name = "ghoul";
      }

      else if( !str_cmp( arg, "crypt thing" ) )
      {
         sindex = MOB_VNUM_ANIMATED_CRYPT_THING;
         corpse_name = "crypt thing";
      }

      else if( !str_cmp( arg, "mummy" ) )
      {
         sindex = MOB_VNUM_ANIMATED_MUMMY;
         corpse_name = "mummy";
      }

      else if( !str_cmp( arg, "ghost" ) )
      {
         sindex = MOB_VNUM_ANIMATED_GHOST;
         corpse_name = "ghost";
      }

      else if( !str_cmp( arg, "death knight" ) )
      {
         sindex = MOB_VNUM_ANIMATED_DEATH_KNIGHT;
         corpse_name = "death knight";
      }

      else if( !str_cmp( arg, "dracolich" ) )
      {
         sindex = MOB_VNUM_ANIMATED_DRACOLICH;
         corpse_name = "dracolich";
      }

      if( sindex == -1 )
      {
         send_to_char( "Turn it into WHAT?????\n\r", ch );
         return rSPELL_FAILED;
      }

      mob = create_mobile( get_mob_index( sindex ) );

      /*
       * Bugfix by Tarl so only dragons become dracoliches. 29 July 2002 
       */
      if( corpse_name == "dracolich" )
      {
         if( corpse->value[6] != RACE_DRAGON )
         {
            send_to_char( "Only dead Dragon's can become Dracoliches.\n\r", ch );
            if( !char_to_room( mob, get_room_index( ROOM_VNUM_POLY ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            return rSPELL_FAILED;
         }
      }

      if( corpse->level < mob->level )
      {
         ch_printf( ch, "The spirit of this corpse is not powerful enough to become a %s.\n\r", corpse_name );
         if( !char_to_room( mob, get_room_index( ROOM_VNUM_POLY ) ) )   /* Send to here to prevent bugs */
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return rSPELL_FAILED;
      }

      if( ch->level < mob->level )
      {
         ch_printf( ch, "You are not powerful enough to animate a %s yet.\n\r", arg );
         if( !char_to_room( mob, get_room_index( ROOM_VNUM_POLY ) ) )   /* Send to here to prevent bugs */
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return rSPELL_FAILED;
      }

      if( !char_to_room( mob, ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      act( AT_MAGIC, "$n makes $T rise from the grave!", ch, NULL, pMobIndex->short_descr, TO_ROOM );
      act( AT_MAGIC, "You make $T rise from the grave!", ch, NULL, pMobIndex->short_descr, TO_CHAR );

      stralloc_printf( &mob->name, "%s %s", corpse_name, pMobIndex->player_name );
      stralloc_printf( &mob->short_descr, "The %s", corpse_name );
      stralloc_printf( &mob->long_descr, "A %s struggles with the horror of its undeath.\n\r", corpse_name );
      bind_follower( mob, ch, sn, ( int )( number_fuzzy( ( int )( ( level + 1 ) / 4 ) + 1 ) * DUR_CONV ) );

      if( corpse->first_content )
         for( obj = corpse->first_content; obj; obj = obj_next )
         {
            obj_next = obj->next_content;
            obj_from_obj( obj );
            obj_to_room( obj, corpse->in_room, mob );
         }

      separate_obj( corpse );
      extract_obj( corpse );
      return rNONE;
   }
   else
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }
}

/* Ignores pickproofs, but can't unlock containers. -- Altrag 17/2/96 */
/* Oh ye of little faith Altrag.... it does now :) */
SPELLF spell_knock( int sn, int level, CHAR_DATA * ch, void *vo )
{
   EXIT_DATA *pexit;
   OBJ_DATA *obj;

   set_char_color( AT_MAGIC, ch );
   /*
    * shouldn't know why it didn't work, and shouldn't work on pickproof
    * exits.  -Thoric
    */
   /*
    * agreed, it shouldn't work on pickproof anything - but explaining why
    * * a spell failed won't hurt you any Thoric :)
    */

   /*
    * Checks exit keywords first - Samson 
    */
   if( ( pexit = find_door( ch, target_name, TRUE ) ) )
   {
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         ch_printf( ch, "The %s is already open.\n\r", pexit->keyword );
         return rSPELL_FAILED;
      }
      if( !IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         ch_printf( ch, "The %s is not locked.\n\r", pexit->keyword );
         return rSPELL_FAILED;
      }
      if( IS_EXIT_FLAG( pexit, EX_PICKPROOF ) )
      {
         send_to_char( "The lock is too strong to be forced open.\n\r", ch );
         return rSPELL_FAILED;
      }
      REMOVE_EXIT_FLAG( pexit, EX_LOCKED );
      send_to_char( "*Click*\n\r", ch );
      if( pexit->rexit && pexit->rexit->to_room == ch->in_room )
         REMOVE_EXIT_FLAG( pexit->rexit, EX_LOCKED );
      check_room_for_traps( ch, TRAP_UNLOCK | trap_door[pexit->vdir] );
      return rNONE;
   }

   /*
    * Then checks objects in the room to see if there's a match - Samson 
    */
   if( ( obj = get_obj_here( ch, target_name ) ) )
   {
      if( obj->item_type != ITEM_CONTAINER )
      {
         ch_printf( ch, "%s is not even a container!\n\r", obj->short_descr );
         return rSPELL_FAILED;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         send_to_char( "It's not closed.\n\r", ch );
         return rSPELL_FAILED;
      }
      if( !IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         send_to_char( "It's already unlocked.\n\r", ch );
         return rSPELL_FAILED;
      }
      if( IS_SET( obj->value[1], CONT_PICKPROOF ) )
      {
         send_to_char( "The lock is too strong to be forced open.\n\r", ch );
         return rSPELL_FAILED;
      }
      REMOVE_BIT( obj->value[1], CONT_LOCKED );
      send_to_char( "*Click*\n\r", ch );
      act( AT_MAGIC, "$n magically unlocks $p.", ch, obj, NULL, TO_ROOM );
      /*
       * Need to figure out how to check for traps here 
       */
      return rNONE;
   }
   send_to_char( "What were you trying to knock?\n\r", ch );
   return rSPELL_FAILED;
}

/* Tells to sleepers in are. -- Altrag 17/2/96 */
SPELLF spell_dream( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim;
   char arg[MIL];

   target_name = one_argument( target_name, arg );
   set_char_color( AT_MAGIC, ch );

   if( !( victim = get_char_world( ch, arg ) ) || victim->in_room->area != ch->in_room->area )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( victim->position != POS_SLEEPING )
   {
      send_to_char( "They aren't asleep.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( !target_name )
   {
      send_to_char( "What do you want them to dream about?\n\r", ch );
      return rSPELL_FAILED;
   }
   ch_printf( victim, "&[tell]You have dreams about %s telling you '%s'.\n\r", PERS( ch, victim, FALSE ), target_name );
   successful_casting( get_skilltype( sn ), ch, victim, NULL );
   return rNONE;
}

/* MUST BE KEPT!!! Used by attack types for mobiles!! */
SPELLF spell_spiral_blast( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   int dam;
   bool ch_died;

   ch_died = FALSE;

   if( ( victim = get_char_room( ch, target_name ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( is_safe( ch, victim ) )
      return rSPELL_FAILED;

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SAFE ) )
   {
      send_to_char( "&[magic]You fail to breathe.\n\r", ch );
      return rNONE;
   }

   act( AT_MAGIC, "Swirling colours radiate from $n, encompassing $N.", ch, ch, victim, TO_ROOM );
   act( AT_MAGIC, "Swirling colours radiate from you, encompassing $N.", ch, ch, victim, TO_CHAR );

   dam = dice( level, 9 );
   if( saves_breath( level, victim ) )
      dam /= 2;
   if( damage( ch, victim, dam, sn ) == rCHAR_DIED || char_died( ch ) )
      ch_died = TRUE;

   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}

   /*******************************************************
	 * Everything after this point is part of SMAUG SPELLS *
	 *******************************************************/

/*
 * saving throw check						-Thoric
 */
bool check_save( int sn, int level, CHAR_DATA * ch, CHAR_DATA * victim )
{
   SKILLTYPE *skill = get_skilltype( sn );
   bool saved = FALSE;

   if( SPELL_FLAG( skill, SF_PKSENSITIVE ) && !IS_NPC( ch ) && !IS_NPC( victim ) )
      level /= 2;

   if( skill->saves )
      switch ( skill->saves )
      {
         case SS_POISON_DEATH:
            saved = saves_poison_death( level, victim );
            break;
         case SS_ROD_WANDS:
            saved = saves_wands( level, victim );
            break;
         case SS_PARA_PETRI:
            saved = saves_para_petri( level, victim );
            break;
         case SS_BREATH:
            saved = saves_breath( level, victim );
            break;
         case SS_SPELL_STAFF:
            saved = saves_spell_staff( level, victim );
            break;
      }
   return saved;
}

SPELLF spell_affectchar( int sn, int level, CHAR_DATA * ch, void *vo )
{
   AFFECT_DATA af;
   SMAUG_AFF *saf;
   SKILLTYPE *skill = get_skilltype( sn );
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   int schance;
   bool affected = FALSE, first = TRUE;
   ch_ret retcode = rNONE;

   if( SPELL_FLAG( skill, SF_RECASTABLE ) )
      affect_strip( victim, sn );
   for( saf = skill->affects; saf; saf = saf->next )
   {
      if( saf->location >= REVERSE_APPLY )
      {
         if( !SPELL_FLAG( skill, SF_ACCUMULATIVE ) )
         {
            if( first == TRUE )
            {
               if( SPELL_FLAG( skill, SF_RECASTABLE ) )
                  affect_strip( ch, sn );
               if( is_affected( ch, sn ) )
                  affected = TRUE;
            }
            first = FALSE;
            if( affected == TRUE )
               continue;
         }
         victim = ch;
      }
      else
         victim = ( CHAR_DATA * ) vo;

      /*
       * Check if char has this bitvector already 
       */
      af.bit = saf->bitvector;
      if( saf->bitvector >= 0 && IS_AFFECTED( victim, saf->bitvector ) && !SPELL_FLAG( skill, SF_ACCUMULATIVE ) )
         continue;
      /*
       * necessary for affect_strip to work properly...
       */
      switch ( saf->bitvector )
      {
         default:
            af.type = sn;
            break;
         case AFF_POISON:
            af.type = gsn_poison;
            schance = ris_save( victim, level, RIS_POISON );
            if( schance == 1000 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            if( saves_poison_death( schance, victim ) )
            {
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            victim->mental_state = URANGE( 30, victim->mental_state + 2, 100 );
            break;
         case AFF_BLIND:
            af.type = gsn_blindness;
            break;
         case AFF_CURSE:
            af.type = gsn_curse;
            break;
         case AFF_INVISIBLE:
            af.type = gsn_invis;
            break;
         case AFF_SLEEP:
            af.type = gsn_sleep;
            schance = ris_save( victim, level, RIS_SLEEP );
            if( schance == 1000 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            break;
         case AFF_CHARM:
            af.type = gsn_charm_person;
            schance = ris_save( victim, level, RIS_CHARM );
            if( schance == 1000 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            break;
         case AFF_PARALYSIS:
            af.type = gsn_paralyze;
            schance = ris_save( victim, level, RIS_PARALYSIS );
            if( schance == 1000 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            break;
      }
      {
         int tmp = dice_parse( ch, level, saf->duration );
         af.duration = UMIN( tmp, 32700 );
      }
      if( saf->location == APPLY_AFFECT
       || saf->location == APPLY_EXT_AFFECT )
      {
         af.modifier = saf->bitvector;
      }
      else if( saf->location == APPLY_RESISTANT
       || saf->location == APPLY_IMMUNE
       || saf->location == APPLY_ABSORB
       || saf->location == APPLY_SUSCEPTIBLE )
      {
         af.modifier = get_risflag( saf->modifier );
      }
      else
         af.modifier = dice_parse( ch, level, saf->modifier );
      af.location = saf->location % REVERSE_APPLY;
      if( af.duration == 0 )
      {
         switch ( af.location )
         {
            case APPLY_HIT:
               victim->hit = URANGE( 0, victim->hit + af.modifier, victim->max_hit );
               update_pos( victim );
               if( IS_NPC( victim ) && victim->hit <= 0 )
                  damage( ch, victim, 5, TYPE_UNDEFINED );
               break;
            case APPLY_MANA:
               victim->mana = URANGE( 0, victim->mana + af.modifier, victim->max_mana );
               update_pos( victim );
               break;
            case APPLY_MOVE:
               victim->move = URANGE( 0, victim->move + af.modifier, victim->max_move );
               update_pos( victim );
               break;
            default:
               affect_modify( victim, &af, TRUE );
               break;
         }
      }
      else if( SPELL_FLAG( skill, SF_ACCUMULATIVE ) )
         affect_join( victim, &af );
      else
         affect_to_char( victim, &af );
   }
   update_pos( victim );
   return retcode;
}

/*
 * Generic offensive spell damage attack			-Thoric
 */
SPELLF spell_attack( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   SKILLTYPE *skill = get_skilltype( sn );
   bool saved = check_save( sn, level, ch, victim );
   int dam;
   ch_ret retcode = rNONE;

   if( saved && SPELL_SAVE( skill ) == SE_NEGATE )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   if( skill->dice )
      dam = UMAX( 0, dice_parse( ch, level, skill->dice ) );
   else
      dam = dice( 1, level / 2 );
   if( saved )
   {
      switch ( SPELL_SAVE( skill ) )
      {
         case SE_3QTRDAM:
            dam = ( dam * 3 ) / 4;
            break;
         case SE_HALFDAM:
            dam >>= 1;
            break;
         case SE_QUARTERDAM:
            dam >>= 2;
            break;
         case SE_EIGHTHDAM:
            dam >>= 3;
            break;

         case SE_ABSORB:  /* victim absorbs spell for hp's */
            act( AT_MAGIC, "$N absorbs your $t!", ch, skill->noun_damage, victim, TO_CHAR );
            act( AT_MAGIC, "You absorb $N's $t!", victim, skill->noun_damage, ch, TO_CHAR );
            act( AT_MAGIC, "$N absorbs $n's $t!", ch, skill->noun_damage, victim, TO_NOTVICT );
            victim->hit = URANGE( 0, victim->hit + dam, victim->max_hit );
            update_pos( victim );
            if( skill->affects )
               retcode = spell_affectchar( sn, level, ch, victim );
            return retcode;

         case SE_REFLECT: /* reflect the spell to the caster */
            return spell_attack( sn, level, victim, ch );
      }
   }
   retcode = damage( ch, victim, dam, sn );
   if( retcode == rNONE && skill->affects && !char_died( ch ) && !char_died( victim )
       && ( !is_affected( victim, sn ) || SPELL_FLAG( skill, SF_ACCUMULATIVE ) || SPELL_FLAG( skill, SF_RECASTABLE ) ) )
      retcode = spell_affectchar( sn, level, ch, victim );
   return retcode;
}

/*
 * Generic area attack						-Thoric
 */
SPELLF spell_area_attack( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *vch, *vch_next;
   SKILLTYPE *skill = get_skilltype( sn );
   bool saved;
   bool affects;
   int dam;
   bool ch_died = FALSE;
   ch_ret retcode = rNONE;

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SAFE ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }

   affects = ( skill->affects ? TRUE : FALSE );
   if( skill->hit_char && skill->hit_char[0] != '\0' )
      act( AT_MAGIC, skill->hit_char, ch, NULL, NULL, TO_CHAR );
   if( skill->hit_room && skill->hit_room[0] != '\0' )
      act( AT_MAGIC, skill->hit_room, ch, NULL, NULL, TO_ROOM );

   for( vch = ch->in_room->first_person; vch; vch = vch_next )
   {
      vch_next = vch->next_in_room;

      if( IS_PLR_FLAG( vch, PLR_WIZINVIS ) && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
         continue;

      /*
       * Verify they're in the same spot 
       */
      if( vch == ch || !is_same_map( vch, ch ) )
         continue;

      if( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) )
      {
         saved = check_save( sn, level, ch, vch );
         if( saved && SPELL_SAVE( skill ) == SE_NEGATE )
         {
            failed_casting( skill, ch, vch, NULL );
            continue;
         }
         else if( skill->dice )
            dam = dice_parse( ch, level, skill->dice );
         else
            dam = dice( 1, level / 2 );
         if( saved )
         {
            switch ( SPELL_SAVE( skill ) )
            {
               case SE_3QTRDAM:
                  dam = ( dam * 3 ) / 4;
                  break;
               case SE_HALFDAM:
                  dam >>= 1;
                  break;
               case SE_QUARTERDAM:
                  dam >>= 2;
                  break;
               case SE_EIGHTHDAM:
                  dam >>= 3;
                  break;

               case SE_ABSORB:  /* victim absorbs spell for hp's */
                  act( AT_MAGIC, "$N absorbs your $t!", ch, skill->noun_damage, vch, TO_CHAR );
                  act( AT_MAGIC, "You absorb $N's $t!", vch, skill->noun_damage, ch, TO_CHAR );
                  act( AT_MAGIC, "$N absorbs $n's $t!", ch, skill->noun_damage, vch, TO_NOTVICT );
                  vch->hit = URANGE( 0, vch->hit + dam, vch->max_hit );
                  update_pos( vch );
                  continue;

               case SE_REFLECT: /* reflect the spell to the caster */
                  retcode = spell_attack( sn, level, vch, ch );
                  if( char_died( ch ) )
                  {
                     ch_died = TRUE;
                     break;
                  }
                  continue;
            }
         }
         if( IS_ROOM_FLAG( ch->in_room, ROOM_CAVE ) && vch == ch )
            dam = dam / 2;
         if( IS_ROOM_FLAG( ch->in_room, ROOM_CAVERN ) && vch == ch )
            dam = 0;

         retcode = damage( ch, vch, dam, sn );
      }
      if( retcode == rNONE && affects && !char_died( ch ) && !char_died( vch )
          && ( !is_affected( vch, sn ) || SPELL_FLAG( skill, SF_ACCUMULATIVE ) || SPELL_FLAG( skill, SF_RECASTABLE ) ) )
         retcode = spell_affectchar( sn, level, ch, vch );
      if( retcode == rCHAR_DIED || char_died( ch ) )
      {
         ch_died = TRUE;
         break;
      }
   }
   return retcode;
}

/*
 * Generic spell affect						-Thoric
 */
SPELLF spell_affect( int sn, int level, CHAR_DATA * ch, void *vo )
{
   SMAUG_AFF *saf;
   SKILLTYPE *skill = get_skilltype( sn );
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   bool groupsp;
   bool areasp;
   bool hitchar = FALSE, hitroom = FALSE, hitvict = FALSE;
   ch_ret retcode;

   if( !skill->affects )
   {
      bug( "spell_affect has no affects sn %d", sn );
      return rNONE;
   }
   if( SPELL_FLAG( skill, SF_GROUPSPELL ) )
      groupsp = TRUE;
   else
      groupsp = FALSE;

   if( SPELL_FLAG( skill, SF_AREA ) )
      areasp = TRUE;
   else
      areasp = FALSE;
   if( !groupsp && !areasp )
   {
      /*
       * Can't find a victim 
       */
      if( !victim )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( ( skill->type != SKILL_HERB && IS_IMMUNE( victim, RIS_MAGIC ) ) || is_immune( victim, SPELL_DAMAGE( skill ) ) )
      {
         immune_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      /*
       * Spell is already on this guy 
       */
      if( is_affected( victim, sn ) && !SPELL_FLAG( skill, SF_ACCUMULATIVE ) && !SPELL_FLAG( skill, SF_RECASTABLE ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( ( saf = skill->affects ) && !saf->next && saf->location == APPLY_STRIPSN
          && !is_affected( victim, dice_parse( ch, level, saf->modifier ) ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( check_save( sn, level, ch, victim ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }
   }
   else
   {
      if( skill->hit_char && skill->hit_char[0] != '\0' )
      {
         if( strstr( skill->hit_char, "$N" ) )
            hitchar = TRUE;
         else
            act( AT_MAGIC, skill->hit_char, ch, NULL, NULL, TO_CHAR );
      }
      if( skill->hit_room && skill->hit_room[0] != '\0' )
      {
         if( strstr( skill->hit_room, "$N" ) )
            hitroom = TRUE;
         else
            act( AT_MAGIC, skill->hit_room, ch, NULL, NULL, TO_ROOM );
      }
      if( skill->hit_vict && skill->hit_vict[0] != '\0' )
         hitvict = TRUE;
      if( victim )
         victim = victim->in_room->first_person;
      else
         victim = ch->in_room->first_person;
   }
   if( !victim )
   {
      bug( "spell_affect: could not find victim: sn %d", sn );
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   for( ; victim; victim = victim->next_in_room )
   {
      if( groupsp || areasp )
      {
         if( ( groupsp && !is_same_group( victim, ch ) ) || IS_IMMUNE( victim, RIS_MAGIC )
             || is_immune( victim, SPELL_DAMAGE( skill ) ) || check_save( sn, level, ch, victim )
             || ( !SPELL_FLAG( skill, SF_RECASTABLE ) && is_affected( victim, sn ) ) )
            continue;

         if( hitvict && ch != victim )
         {
            act( AT_MAGIC, skill->hit_vict, ch, NULL, victim, TO_VICT );
            if( hitroom )
            {
               act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_NOTVICT );
               act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_CHAR );
            }
         }
         else if( hitroom )
            act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_ROOM );
         if( ch == victim )
         {
            if( hitvict )
               act( AT_MAGIC, skill->hit_vict, ch, NULL, ch, TO_CHAR );
            else if( hitchar )
               act( AT_MAGIC, skill->hit_char, ch, NULL, ch, TO_CHAR );
         }
         else if( hitchar )
            act( AT_MAGIC, skill->hit_char, ch, NULL, victim, TO_CHAR );
      }
      retcode = spell_affectchar( sn, level, ch, victim );
      if( !groupsp && !areasp )
      {
         if( retcode == rVICT_IMMUNE )
            immune_casting( skill, ch, victim, NULL );
         else
            successful_casting( skill, ch, victim, NULL );
         break;
      }
   }
   return rNONE;
}

/*
 * Generic inventory object spell				-Thoric
 */
SPELLF spell_obj_inv( int sn, int level, CHAR_DATA * ch, void *vo )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;
   LIQ_TABLE *liq = NULL;
   SKILLTYPE *skill = get_skilltype( sn );

   if( !obj )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rNONE;
   }

   switch ( SPELL_ACTION( skill ) )
   {
      default:
      case SA_NONE:
         return rNONE;

      case SA_CREATE:
         if( SPELL_FLAG( skill, SF_WATER ) ) /* create water */
         {
            int water;
            WEATHER_DATA *weath = ch->in_room->area->weather;

            if( obj->item_type != ITEM_DRINK_CON )
            {
               send_to_char( "It is unable to hold water.\n\r", ch );
               return rSPELL_FAILED;
            }

            if( ( liq = get_liq_vnum( obj->value[2] ) ) == NULL )
            {
               bug( "create_water: bad liquid number %d", obj->value[2] );
               send_to_char( "There's already another liquid in the container.\n\r", ch );
               return rSPELL_FAILED;
            }

            if( str_cmp( liq->name, "water" ) && obj->value[1] != 0 )
            {
               send_to_char( "There's already another liquid in the container.\n\r", ch );
               return rSPELL_FAILED;
            }

            water = UMIN( ( skill->dice ? dice_parse( ch, level, skill->dice ) : level )
                          * ( weath->precip >= 0 ? 2 : 1 ), obj->value[0] - obj->value[1] );

            if( water > 0 )
            {
               separate_obj( obj );
               obj->value[2] = liq->vnum;
               obj->value[1] += water;

               /*
                * Don't overfill the container! 
                */
               if( obj->value[1] > obj->value[0] )
                  obj->value[1] = obj->value[0];

               if( !is_name( "water", obj->name ) )
                  stralloc_printf( &obj->name, "%s water", obj->name );
            }
            successful_casting( skill, ch, NULL, obj );
            return rNONE;
         }
         if( SPELL_DAMAGE( skill ) == SD_FIRE ) /* burn object */
         {
            /*
             * return rNONE; 
             */
         }
         if( SPELL_DAMAGE( skill ) == SD_POISON || SPELL_CLASS( skill ) == SC_DEATH )  /* poison object */
         {
            switch ( obj->item_type )
            {
               default:
                  failed_casting( skill, ch, NULL, obj );
                  break;
               case ITEM_COOK:
               case ITEM_FOOD:
               case ITEM_DRINK_CON:
                  separate_obj( obj );
                  obj->value[3] = 1;
                  successful_casting( skill, ch, NULL, obj );
                  break;
            }
            return rNONE;
         }
         if( SPELL_CLASS( skill ) == SC_LIFE /* purify food/water */
             && ( obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_COOK ) )
         {
            switch ( obj->item_type )
            {
               default:
                  failed_casting( skill, ch, NULL, obj );
                  break;
               case ITEM_COOK:
               case ITEM_FOOD:
               case ITEM_DRINK_CON:
                  separate_obj( obj );
                  obj->value[3] = 0;
                  successful_casting( skill, ch, NULL, obj );
                  break;
            }
            return rNONE;
         }

         if( SPELL_CLASS( skill ) != SC_NONE )
         {
            failed_casting( skill, ch, NULL, obj );
            return rNONE;
         }
         switch ( SPELL_POWER( skill ) )  /* clone object */
         {
               OBJ_DATA *clone;

            default:
            case SP_NONE:
               if( ch->level - obj->level < 10 || obj->cost > ch->level * get_curr_int( ch ) * get_curr_wis( ch ) )
               {
                  failed_casting( skill, ch, NULL, obj );
                  return rNONE;
               }
               break;
            case SP_MINOR:
               if( ch->level - obj->level < 20 || obj->cost > ch->level * get_curr_int( ch ) / 5 )
               {
                  failed_casting( skill, ch, NULL, obj );
                  return rNONE;
               }
               break;
            case SP_GREATER:
               if( ch->level - obj->level < 5 || obj->cost > ch->level * 10 * get_curr_int( ch ) * get_curr_wis( ch ) )
               {
                  failed_casting( skill, ch, NULL, obj );
                  return rNONE;
               }
               break;
            case SP_MAJOR:
               if( ch->level - obj->level < 0 || obj->cost > ch->level * 50 * get_curr_int( ch ) * get_curr_wis( ch ) )
               {
                  failed_casting( skill, ch, NULL, obj );
                  return rNONE;
               }
               clone = clone_object( obj );
               clone->timer = skill->dice ? dice_parse( ch, level, skill->dice ) : 0;
               obj_to_char( clone, ch );
               successful_casting( skill, ch, NULL, obj );
               break;
         }
         return rNONE;

      case SA_DESTROY:
      case SA_RESIST:
      case SA_SUSCEPT:
      case SA_DIVINATE:
         if( SPELL_DAMAGE( skill ) == SD_POISON )  /* detect poison */
         {
            if( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD || obj->item_type == ITEM_COOK )
            {
               if( obj->item_type == ITEM_COOK && obj->value[2] == 0 )
                  send_to_char( "It looks undercooked.\n\r", ch );
               else if( obj->value[3] != 0 )
                  send_to_char( "You smell poisonous fumes.\n\r", ch );
               else
                  send_to_char( "It looks very delicious.\n\r", ch );
            }
            else
               send_to_char( "It doesn't look poisoned.\n\r", ch );
            return rNONE;
         }
         return rNONE;
      case SA_OBSCURE: /* make obj invis */
         if( IS_OBJ_FLAG( obj, ITEM_INVIS ) || chance( ch, skill->dice ? dice_parse( ch, level, skill->dice ) : 20 ) )
         {
            failed_casting( skill, ch, NULL, NULL );
            return rSPELL_FAILED;
         }
         successful_casting( skill, ch, NULL, obj );
         SET_OBJ_FLAG( obj, ITEM_INVIS );
         return rNONE;

      case SA_CHANGE:
         return rNONE;
   }
}

/*
 * Generic object creating spell				-Thoric
 */
SPELLF spell_create_obj( int sn, int level, CHAR_DATA * ch, void *vo )
{
   SKILLTYPE *skill = get_skilltype( sn );
   int lvl, vnum = skill->value;
   OBJ_DATA *obj;

   switch ( SPELL_POWER( skill ) )
   {
      default:
      case SP_NONE:
         lvl = 10;
         break;
      case SP_MINOR:
         lvl = 0;
         break;
      case SP_GREATER:
         lvl = level / 2;
         break;
      case SP_MAJOR:
         lvl = level;
         break;
   }

   if( !( obj = create_object( get_obj_index( vnum ), lvl ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      failed_casting( skill, ch, NULL, NULL );
      return rNONE;
   }
   obj->timer = skill->dice ? dice_parse( ch, level, skill->dice ) : 0;
   successful_casting( skill, ch, NULL, obj );
   if( CAN_WEAR( obj, ITEM_TAKE ) )
      obj_to_char( obj, ch );
   else
      obj_to_room( obj, ch->in_room, ch );
   return rNONE;
}

/*
 * Generic mob creating spell					-Thoric
 */
SPELLF spell_create_mob( int sn, int level, CHAR_DATA * ch, void *vo )
{
   SKILLTYPE *skill = get_skilltype( sn );
   int lvl;
   int vnum = skill->value;
   CHAR_DATA *mob;
   MOB_INDEX_DATA *mi;
   AFFECT_DATA af;

   /*
    * set maximum mob level 
    */
   switch ( SPELL_POWER( skill ) )
   {
      default:
      case SP_NONE:
         lvl = 20;
         break;
      case SP_MINOR:
         lvl = 5;
         break;
      case SP_GREATER:
         lvl = level / 2;
         break;
      case SP_MAJOR:
         lvl = level;
         break;
   }

   /*
    * Add predetermined mobiles here
    */
   if( vnum == 0 )
   {
      if( !str_cmp( target_name, "cityguard" ) )
         vnum = MOB_VNUM_CITYGUARD;
   }

   if( ( mi = get_mob_index( vnum ) ) == NULL || ( mob = create_mobile( mi ) ) == NULL )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rNONE;
   }
   mob->level = UMIN( lvl, skill->dice ? dice_parse( ch, level, skill->dice ) : mob->level );
   mob->armor = interpolate( mob->level, 100, -100 );

   mob->max_hit = mob->level * 8 + number_range( mob->level * mob->level / 4, mob->level * mob->level );
   mob->hit = mob->max_hit;
   mob->gold = 0;
   successful_casting( skill, ch, mob, NULL );
   if( !char_to_room( mob, ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   add_follower( mob, ch );
   af.type = sn;
   af.duration = ( int )( number_fuzzy( ( int )( ( level + 1 ) / 3 ) + 1 ) * DUR_CONV );
   af.location = 0;
   af.modifier = 0;
   af.bit = AFF_CHARM;
   affect_to_char( mob, &af );
   return rNONE;
}

ch_ret ranged_attack( CHAR_DATA *, char *, OBJ_DATA *, OBJ_DATA *, short, short );

/*
 * Generic handler for new "SMAUG" spells			-Thoric
 */
SPELLF spell_smaug( int sn, int level, CHAR_DATA * ch, void *vo )
{
   struct skill_type *skill = get_skilltype( sn );

   /*
    * Put this check in to prevent crashes from this getting a bad skill 
    */
   if( !skill )
   {
      bug( "spell_smaug: Called with a null skill for sn %d", sn );
      return rERROR;
   }

   switch ( skill->target )
   {
      case TAR_IGNORE:
         if( SPELL_FLAG( skill, SF_NOMOUNT ) )
         {
            send_to_char( "You can't do that while mounted.\n\r", ch );
            return rNONE;
         }

         /*
          * offensive area spell 
          */
         if( SPELL_FLAG( skill, SF_AREA )
             && ( ( SPELL_ACTION( skill ) == SA_DESTROY && SPELL_CLASS( skill ) == SC_LIFE )
                  || ( SPELL_ACTION( skill ) == SA_CREATE && SPELL_CLASS( skill ) == SC_DEATH ) ) )
            return spell_area_attack( sn, level, ch, vo );

         if( SPELL_ACTION( skill ) == SA_CREATE )
         {
            if( SPELL_FLAG( skill, SF_OBJECT ) )   /* create object */
               return spell_create_obj( sn, level, ch, vo );
            if( SPELL_CLASS( skill ) == SC_LIFE )  /* create mob */
               return spell_create_mob( sn, level, ch, vo );
         }

         /*
          * affect a distant player 
          */
         if( SPELL_FLAG( skill, SF_DISTANT ) && SPELL_FLAG( skill, SF_CHARACTER ) )
            return spell_affect( sn, level, ch, get_char_world( ch, target_name ) );

         /*
          * affect a player in this room (should have been TAR_CHAR_XXX) 
          */
         if( SPELL_FLAG( skill, SF_CHARACTER ) )
            return spell_affect( sn, level, ch, get_char_room( ch, target_name ) );

         if( skill->range > 0 && ( ( SPELL_ACTION( skill ) == SA_DESTROY && SPELL_CLASS( skill ) == SC_LIFE )
                                   || ( SPELL_ACTION( skill ) == SA_CREATE && SPELL_CLASS( skill ) == SC_DEATH ) ) )
            return ranged_attack( ch, ranged_target_name, NULL, NULL, sn, skill->range );

         /*
          * will fail, or be an area/group affect 
          */
         return spell_affect( sn, level, ch, vo );

      case TAR_CHAR_OFFENSIVE:
         if( SPELL_FLAG( skill, SF_NOMOUNT ) )
         {
            send_to_char( "You can't do that while mounted.\n\r", ch );
            return rNONE;
         }

         /*
          * a regular damage inflicting spell attack 
          */
         if( ( SPELL_ACTION( skill ) == SA_DESTROY && SPELL_CLASS( skill ) == SC_LIFE )
             || ( SPELL_ACTION( skill ) == SA_CREATE && SPELL_CLASS( skill ) == SC_DEATH ) )
            return spell_attack( sn, level, ch, vo );

         /*
          * a nasty spell affect 
          */
         return spell_affect( sn, level, ch, vo );

      case TAR_CHAR_DEFENSIVE:
      case TAR_CHAR_SELF:
         if( SPELL_FLAG( skill, SF_NOFIGHT ) && ch->position > POS_SITTING && ch->position < POS_STANDING )
         {
            send_to_char( "You can't concentrate enough for that!\n\r", ch );
            return rNONE;
         }

         if( SPELL_FLAG( skill, SF_NOMOUNT ) )
         {
            send_to_char( "You can't do that while mounted.\n\r", ch );
            return rNONE;
         }

         if( vo && SPELL_ACTION( skill ) == SA_DESTROY )
         {
            CHAR_DATA *victim = ( CHAR_DATA * ) vo;

            /*
             * cure poison 
             */
            if( SPELL_DAMAGE( skill ) == SD_POISON )
            {
               if( is_affected( victim, gsn_poison ) )
               {
                  affect_strip( victim, gsn_poison );
                  victim->mental_state = URANGE( -100, victim->mental_state, -10 );
                  successful_casting( skill, ch, victim, NULL );
                  return rNONE;
               }
               failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
            /*
             * cure blindness 
             */
            if( SPELL_CLASS( skill ) == SC_ILLUSION )
            {
               if( is_affected( victim, gsn_blindness ) )
               {
                  affect_strip( victim, gsn_blindness );
                  successful_casting( skill, ch, victim, NULL );
                  return rNONE;
               }
               failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
         }
         return spell_affect( sn, level, ch, vo );

      case TAR_OBJ_INV:
         if( SPELL_FLAG( skill, SF_NOMOUNT ) )
         {
            send_to_char( "You can't do that while mounted.\n\r", ch );
            return rNONE;
         }
         return spell_obj_inv( sn, level, ch, vo );
   }
   return rNONE;
}

/* Everything from here down has been added by Alsherok */

SPELLF spell_treespeak( int sn, int level, CHAR_DATA * ch, void *vo )
{
   AFFECT_DATA af;

   af.type = skill_lookup( "treetalk" );
   af.duration = 93; /* One hour long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_TREETALK;
   affect_to_char( ch, &af );

   send_to_char( "&[magic]You enter an altered state and are now speaking the language of trees.\n\r", ch );
   return rNONE;
}

SPELLF spell_tree_transport( int sn, int level, CHAR_DATA * ch, void *vo )
{
   ROOM_INDEX_DATA *target;
   OBJ_DATA *tree, *obj;
   bool found = FALSE;

   for( tree = ch->in_room->first_content; tree; tree = tree->next_content )
   {
      if( tree->item_type == ITEM_TREE )
      {
         if( ch->x == tree->x && ch->y == tree->y && ch->map == tree->map )
         {
            found = TRUE;
            break;
         }
      }
   }

   if( !found )
   {
      send_to_char( "You need to have a tree nearby.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( !( obj = get_obj_world( ch, target_name ) ) )
   {
      send_to_char( "No trees exist in the world with that name.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( obj->item_type != ITEM_TREE )
   {
      send_to_char( "That's not a tree!\n\r", ch );
      return rSPELL_FAILED;
   }

   if( !obj->in_room )
   {
      send_to_char( "That tree is nowhere to be found.\n\r", ch );
      return rSPELL_FAILED;
   }

   target = obj->in_room;

   if( !has_visited( ch, target->area ) )
   {
      send_to_char( "You have not visited that area yet!\n\r", ch );
      return rSPELL_FAILED;
   }

   act( AT_MAGIC, "$n touches $p, and slowly vanishes within!", ch, tree, NULL, TO_ROOM );
   act( AT_MAGIC, "You touch $p, and join your forms.", ch, tree, NULL, TO_CHAR );

   /*
    * Need to explicitly set coordinates and map information with objects 
    */
   leave_map( ch, NULL, target );
   ch->map = obj->map;
   ch->x = obj->x;
   ch->y = obj->y;

   act( AT_MAGIC, "$p rustles slightly, and $n magically steps from within!", ch, obj, NULL, TO_ROOM );
   act( AT_MAGIC, "You are instantly transported to $p!", ch, obj, NULL, TO_CHAR );
   return rNONE;
}

SPELLF spell_group_towngate( int sn, int level, CHAR_DATA * ch, void *vo )
{
   ROOM_INDEX_DATA *room = NULL, *original;
   CHAR_DATA *rch;
   int groupcount, groupvisit;

   if( target_name[0] == '\0' )
   {
      send_to_char( "Where do you wish to go??\n\r", ch );
      send_to_char( "bywater, maldoth, palainth, greyhaven, dragongate, venetorium or graecia ?\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_AREA_FLAG( ch->in_room->area, AFLAG_NOPORTAL ) || IS_ROOM_FLAG( ch->in_room, ROOM_NO_PORTAL ) )
   {
      send_to_char( "Arcane magic blocks the formation of your gate.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( !str_cmp( target_name, "bywater" ) )
      room = get_room_index( 7035 );
   /*
    * Updated Vnum for bywater -- Tarl 16 July 2002 
    */
   if( !str_cmp( target_name, "maldoth" ) )
      room = get_room_index( 10058 );

   if( !str_cmp( target_name, "palainth" ) )
      room = get_room_index( 5150 );

   if( !str_cmp( target_name, "greyhaven" ) )
      room = get_room_index( 4118 );

   if( !str_cmp( target_name, "dragongate" ) )
      room = get_room_index( 19339 );

   if( !str_cmp( target_name, "venetorium" ) )
      room = get_room_index( 5562 );

   if( !str_cmp( target_name, "graecia" ) )
      room = get_room_index( 13806 );

   if( !room )
   {
      send_to_char( "Where do you wish to go??\n\r", ch );
      send_to_char( "bywater, maldoth, palainth, greyhaven, dragongate, venetorium or graecia ?\n\r", ch );
      return rSPELL_FAILED;
   }

   groupcount = 0;
   groupvisit = 0;

   for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
   {
      if( is_same_group( rch, ch ) && !IS_NPC( rch ) )
         groupcount++;
   }

   for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
   {
      if( is_same_group( rch, ch ) && has_visited( rch, room->area ) && !IS_NPC( rch ) )
         groupvisit++;
   }

   original = ch->in_room;

   if( groupcount == groupvisit )
   {
      CHAR_DATA *rch_next;

      leave_map( ch, NULL, room );  /* This will work, regardless. Trust me. */

      for( rch = original->first_person; rch; rch = rch_next )
      {
         rch_next = rch->next_in_room;

         if( is_same_group( rch, ch ) )
            leave_map( rch, ch, room );
      }
      return rNONE;
   }
   else
   {
      ch_printf( ch, "You have not yet all visited %s!\n\r", room->area->name );
      return rSPELL_FAILED;
   }
}

/* Might and magic towngate spell added 12 Mar 01 by Samson/Tarl. */
SPELLF spell_towngate( int sn, int level, CHAR_DATA * ch, void *vo )
{
   ROOM_INDEX_DATA *room = NULL;

   if( target_name[0] == '\0' )
   {
      send_to_char( "Where do you wish to go??\n\r", ch );
      send_to_char( "bywater, maldoth, palainth, greyhaven, dragongate, venetorium or graecia ?\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_AREA_FLAG( ch->in_room->area, AFLAG_NOPORTAL ) || IS_ROOM_FLAG( ch->in_room, ROOM_NO_PORTAL ) )
   {
      send_to_char( "Arcane magic blocks the formation of your gate.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( !str_cmp( target_name, "bywater" ) )
      room = get_room_index( 7035 );
   /*
    * Updated Vnum for bywater -- Tarl 16 July 2002 
    */

   if( !str_cmp( target_name, "maldoth" ) )
      room = get_room_index( 10058 );

   if( !str_cmp( target_name, "palainth" ) )
      room = get_room_index( 5150 );

   if( !str_cmp( target_name, "greyhaven" ) )
      room = get_room_index( 4118 );

   if( !str_cmp( target_name, "dragongate" ) )
      room = get_room_index( 19339 );

   if( !str_cmp( target_name, "venetorium" ) )
      room = get_room_index( 5562 );

   if( !str_cmp( target_name, "graecia" ) )
      room = get_room_index( 13806 );

   if( !room )
   {
      send_to_char( "Where do you wish to go??\n\r", ch );
      send_to_char( "bywater, maldoth, palainth, greyhaven, dragongate, venetorium or graecia ?\n\r", ch );
      return rSPELL_FAILED;
   }

   if( !has_visited( ch, room->area ) )
   {
      ch_printf( ch, "You have not yet visited %s!\n\r", room->area->name );
      return rSPELL_FAILED;
   }

   leave_map( ch, NULL, room );
   return rNONE;
}

SPELLF spell_enlightenment( int sn, int level, CHAR_DATA * ch, void *vo )
{
   AFFECT_DATA af;

   if( IS_AFFECTED( ch, AFF_ENLIGHTEN ) )
   {
      act( AT_MAGIC, "You may only be enlightened once a day.", ch, NULL, NULL, TO_CHAR );
      return rSPELL_FAILED;
   }
   if( ch->hit > ch->max_hit * 2 )
   {
      act( AT_MAGIC, "You cannot acheive enlightenment at this time.", ch, NULL, NULL, TO_CHAR );
      return rSPELL_FAILED;
   }

   af.type = skill_lookup( "enlightenment" );
   af.duration = 2616;  /* One day long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_ENLIGHTEN;
   affect_to_char( ch, &af );

   af.type = skill_lookup( "enlightenment" );
   af.duration = ch->level;
   af.modifier = ch->max_hit;
   af.location = APPLY_HIT;
   af.bit = AFF_ENLIGHTEN;
   affect_to_char( ch, &af );

   /*
    * Safety net just in case 
    */
   if( ch->hit > ch->max_hit )
      ch->hit = ch->max_hit;

   act( AT_MAGIC, "You have acheived enlightenment!", ch, NULL, NULL, TO_CHAR );
   return rNONE;
}

SPELLF spell_rejuv( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim;

   if( target_name[0] == '\0' )
      victim = ch;
   else
      victim = get_char_room( ch, target_name );

   if( IS_NPC( victim ) )
   {
      act( AT_MAGIC, "Rejuvenating a monster would be folly.", ch, NULL, NULL, TO_CHAR );
      return rSPELL_FAILED;
   }

   if( IS_IMMORTAL( victim ) )
   {
      act( AT_MAGIC, "$N has no need of rejuvenation.", ch, NULL, victim, TO_CHAR );
      return rSPELL_FAILED;
   }

   if( !is_same_group( ch, victim ) )
   {
      act( AT_MAGIC, "You must be grouped with your target.", ch, NULL, NULL, TO_CHAR );
      return rSPELL_FAILED;
   }

   act( AT_MAGIC, "$n lays $s hands upon you and chants.", ch, NULL, victim, TO_VICT );
   act( AT_MAGIC, "You lay your hands upon $N and chant.", ch, NULL, victim, TO_CHAR );
   act( AT_MAGIC, "$n lays $s hands upon $N and chants.", ch, NULL, victim, TO_NOTVICT );

   if( number_range( 1, 100 ) != 42 || IS_IMMORTAL( ch ) )
   {  /* All is well */
      if( get_age( victim ) > 20 )
      {
         act( AT_MAGIC, "You feel younger!", ch, NULL, victim, TO_CHAR );
         act( AT_MAGIC, "$n looks younger!", victim, NULL, NULL, TO_ROOM );
         victim->pcdata->age_bonus -= 1;
         ch->alignment += 50;
         if( ch->alignment > 1000 )
            ch->alignment = 1000;
      }
      else
         act( AT_MAGIC, "You don't feel any younger.\n\r", ch, NULL, victim, TO_CHAR );
   }
   else
      switch ( number_range( 1, 6 ) )
      {
         case 1:
         case 2:
         case 3: /* Death */
            if( number_range( 1, 3 ) == 1 )
            {  /* 1/3 chance for caster to get it */
               victim = ch;
            }
            act( AT_MAGIC, "Your heart gives out from the strain.", ch, NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n's heart gives out from the strain.", victim, NULL, NULL, TO_ROOM );
            raw_kill( ch, victim ); /* Ooops. The target died :) */
            log_printf( "%s killed during rejuvenation by %s", victim->name, ch->name );
            break;
         case 4:
         case 5: /* Age */
            if( number_range( 1, 3 ) == 1 )
            {
               victim = ch;
            }
            act( AT_MAGIC, "You age horribly!", ch, NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n ages horribly!", victim, NULL, NULL, TO_ROOM );
            victim->pcdata->age_bonus += 10;
            break;
         case 6: /* Drain *
                   * if (number_range(1, 3) == 1)
                   * {
                   * spell_energy_drain(60, ch, ch, 0);
                   * }
                   * else
                   * {
                   * spell_energy_drain(60, ch, victim, 0);
                   * } */
            break;
      }
   return rNONE;
}

SPELLF spell_slow( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim;
   SKILLTYPE *skill = get_skilltype( sn );

   if( target_name[0] == '\0' )
      victim = ch;
   else
      victim = get_char_room( ch, target_name );

   if( victim && is_same_map( ch, victim ) )
   {
      AFFECT_DATA af;

      if( IS_IMMUNE( victim, RIS_MAGIC ) )
      {
         immune_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( IS_AFFECTED( victim, AFF_SLOW ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      act( AT_MAGIC, "$n has been slowed to a crawl!", victim, NULL, NULL, TO_ROOM );
      af.type = sn;
      af.duration = level;
      af.location = APPLY_NONE;
      af.modifier = 0;
      af.bit = AFF_SLOW;
      affect_to_char( victim, &af );
      act( AT_MAGIC, "You have been slowed to a crawl!", victim, NULL, NULL, TO_CHAR );
      return rNONE;
   }
   ch_printf( ch, "You can't find %s!\n\r", target_name );
   return rSPELL_FAILED;
}

SPELLF spell_haste( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim;
   SKILLTYPE *skill = get_skilltype( sn );

   if( target_name[0] == '\0' )
      victim = ch;
   else
      victim = get_char_room( ch, target_name );

   if( victim && is_same_map( ch, victim ) )
   {
      AFFECT_DATA af;

      if( IS_IMMUNE( victim, RIS_MAGIC ) )
      {
         immune_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( IS_AFFECTED( victim, AFF_HASTE ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      act( AT_MAGIC, "$n is endowed with the speed of Quicklings!", victim, NULL, NULL, TO_ROOM );
      af.type = sn;
      af.duration = level;
      af.location = APPLY_NONE;
      af.modifier = 0;
      af.bit = AFF_HASTE;
      affect_to_char( victim, &af );
      act( AT_MAGIC, "You are endowed with the speed of Quicklings!", victim, NULL, NULL, TO_CHAR );
      if( !IS_NPC( victim ) )
         victim->pcdata->age_bonus += 1;
      return rNONE;
   }
   ch_printf( ch, "You can't find %s!\n\r", target_name );
   return rSPELL_FAILED;
}

SPELLF spell_warsteed( int sn, int level, CHAR_DATA * ch, void *vo )
{
   MOB_INDEX_DATA *temp;
   CHAR_DATA *mob;

   if( ( !IS_OUTSIDE( ch ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      send_to_char( "You must be outdoors to cast this spell.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( ch->Class == CLASS_PALADIN )
   {
      if( ( temp = get_mob_index( MOB_VNUM_WARMOUNTTHREE ) ) == NULL )
      {
         bug( "Spell_warsteed: Paladin Warmount vnum %d doesn't exist.", MOB_VNUM_WARMOUNTTHREE );
         return rSPELL_FAILED;
      }
   }
   else
   {
      if( ( temp = get_mob_index( MOB_VNUM_WARMOUNTFOUR ) ) == NULL )
      {
         bug( "Spell_warmount: Antipaladin warmount vnum %d doesn't exist.", MOB_VNUM_WARMOUNTFOUR );
         return rSPELL_FAILED;
      }
   }

   if( !can_charm( ch ) )
   {
      send_to_char( "You already have too many followers to support!\n\r", ch );
      return rSPELL_FAILED;
   }

   mob = create_mobile( temp );
   if( !char_to_room( mob, ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   fix_maps( ch, mob );
   send_to_char( "&[magic]You summon a spectacular flying steed into being!\n\r", ch );
   bind_follower( mob, ch, sn, ch->level * 10 );
   return rNONE;
}

SPELLF spell_warmount( int sn, int level, CHAR_DATA * ch, void *vo )
{
   MOB_INDEX_DATA *temp;
   CHAR_DATA *mob;

   if( ( !IS_OUTSIDE( ch ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      send_to_char( "You must be outdoors to cast this spell.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( ch->Class == CLASS_PALADIN )
   {
      if( ( temp = get_mob_index( MOB_VNUM_WARMOUNT ) ) == NULL )
      {
         bug( "Spell_warmount: Paladin Warmount vnum %d doesn't exist.", MOB_VNUM_WARMOUNT );
         return rSPELL_FAILED;
      }
   }
   else
   {
      if( ( temp = get_mob_index( MOB_VNUM_WARMOUNTTWO ) ) == NULL )
      {
         bug( "Spell_warmount: Antipaladin warmount vnum %d doesn't exist.", MOB_VNUM_WARMOUNTTWO );
         return rSPELL_FAILED;
      }
   }

   if( !can_charm( ch ) )
   {
      send_to_char( "You already have too many followers to support!\n\r", ch );
      return rSPELL_FAILED;
   }

   mob = create_mobile( temp );
   if( !char_to_room( mob, ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   fix_maps( ch, mob );
   send_to_char( "&[magic]You summon a spectacular steed into being!\n\r", ch );
   bind_follower( mob, ch, sn, ch->level * 10 );
   return rNONE;
}

SPELLF spell_fireseed( int sn, int level, CHAR_DATA * ch, void *vo )
{
   OBJ_DATA *obj;
   OBJ_INDEX_DATA *objcheck;
   short seedcount = 4;

   objcheck = get_obj_index( OBJ_VNUM_FIRESEED );
   if( objcheck != NULL )
   {
      while( seedcount > 0 )
      {
         obj = create_object( get_obj_index( OBJ_VNUM_FIRESEED ), 0 );
         obj_to_char( obj, ch );
         --seedcount;
      }
      send_to_char( "&[magic]With a wave of your hands, some fireseeds appear in your inventory.\n\r", ch );
      return rNONE;
   }
   else
   {
      bug( "spell_fireseed: Fireseed object %d not found.", OBJ_VNUM_FIRESEED );
      return rSPELL_FAILED;
   }
}

SPELLF spell_despair( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *vch, *vch_next;
   bool despair = false;

   /*
    * Add check for proper bard instrument in future 
    */
   for( vch = ch->in_room->first_person; vch; vch = vch_next )
   {
      vch_next = vch->next_in_room;

      if( IS_NPC( vch ) )
         continue;

      if( !is_same_map( ch, vch ) )
         continue;
      else
      {
         if( saves_spell_staff( level, vch ) )
            continue;
         else
         {
            interpret( vch, "flee" );
            despair = true;
         }
      }
   }
   if( despair )
      send_to_char( "&[magic]Your magic strikes fear into the hearts of the occupants!\n\r", ch );
   else
      send_to_char( "&[magic]You weave your magic, but there was no noticable affect.\n\r", ch );
   return rNONE;
}

SPELLF spell_enrage( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *vch;
   bool anger = FALSE;

   /*
    * Add check for proper bard instrument in future 
    */

   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
   {
      if( !IS_NPC( vch ) )
         continue;

      if( !is_same_map( ch, vch ) )
         continue;
      else
      {
         if( !IS_ACT_FLAG( vch, ACT_AGGRESSIVE ) )
         {
            SET_ACT_FLAG( vch, ACT_AGGRESSIVE );
            anger = TRUE;
         }
      }
   }

   if( anger )
      send_to_char( "&[magic]The occupants of the room become highly enraged!\n\r", ch );
   else
      send_to_char( "&[magic]You weave your magic, but there was no noticable affect.\n\r", ch );

   return rNONE;
}

SPELLF spell_calm( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *vch;
   bool soothe = FALSE;

   /*
    * Add check for proper bard instrument in future 
    */

   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
   {
      if( !IS_NPC( vch ) )
         continue;

      if( !is_same_map( ch, vch ) )
         continue;
      else
      {
         if( IS_ACT_FLAG( vch, ACT_AGGRESSIVE ) )
         {
            REMOVE_ACT_FLAG( vch, ACT_AGGRESSIVE );
            soothe = TRUE;
         }
         if( IS_ACT_FLAG( vch, ACT_META_AGGR ) )
         {
            REMOVE_ACT_FLAG( vch, ACT_META_AGGR );
            soothe = TRUE;
         }
      }
   }

   if( soothe )
      send_to_char( "&[magic]A soothing calm settles upon the occupants in the room.\n\r", ch );
   else
      send_to_char( "&[magic]You weave your magic, but there was no noticable affect.\n\r", ch );

   return rNONE;
}

SPELLF spell_gust_of_wind( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *vch;
   int dam = dice( 8, 5 );
   bool ch_died;
   ch_ret retcode = rNONE;

   if( ( !IS_OUTSIDE( ch ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      send_to_char( "You must be outdoors to cast this spell.\n\r", ch );
      return rSPELL_FAILED;
   }

   ch_died = FALSE;
   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
   {
      if( !IS_NPC( vch ) )
         continue;

      if( !is_same_map( ch, vch ) )
         continue;
      else
      {
         vch->position = POS_SITTING;
         retcode = damage( ch, vch, saves_spell_staff( level, vch ) ? dam / 2 : dam, sn );
         if( retcode == rCHAR_DIED || char_died( ch ) )
            ch_died = TRUE;
      }
   }
   send_to_char( "&[magic]You arouse a stiff gust of wind, knocking everyone to the floor.\n\r", ch );

   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}

SPELLF spell_sunray( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *vch;
   AFFECT_DATA af;
   int dam = 0, mobcount = 0;
   bool ch_died;
   ch_ret retcode = rNONE;

   if( ( !IS_OUTSIDE( ch ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      send_to_char( "You must be outdoors to cast this spell.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( time_info.sunlight == SUN_SET || time_info.sunlight == SUN_DARK )
   {
      send_to_char( "This spell cannot be cast without daylight.\n\r", ch );
      return rSPELL_FAILED;
   }

   ch_died = FALSE;
   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
   {
      if( !IS_NPC( vch ) )
         continue;

      if( !is_same_map( ch, vch ) )
         continue;
      else
      {
         if( IsUndead( vch ) )
         {
            dam = dice( 8, 10 );
            retcode = damage( ch, vch, saves_spell_staff( level, vch ) ? dam / 2 : dam, sn );
            if( retcode == rCHAR_DIED || char_died( ch ) )
               ch_died = TRUE;
            send_to_char( "&[magic]The burning rays of the sun sear your flesh!\n\r", vch );
         }

         if( IS_AFFECTED( vch, AFF_BLIND ) )
            continue;

         af.type = gsn_blindness;
         af.location = APPLY_HITROLL;
         af.modifier = -8;
         af.duration = ( int )( ( 1 + ( level / 3 ) ) * DUR_CONV );
         af.bit = AFF_BLIND;
         affect_to_char( vch, &af );
         send_to_char( "&[magic]You are blinded!\n\r", vch );
         mobcount++;
      }
   }
   if( mobcount > 0 )
      send_to_char( "&[magic]You focus the rays of the sun, blinding everyone!\n\r", ch );
   else
      send_to_char( "&[magic]You focus the rays of the sun, but little else.\n\r", ch );

   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}

SPELLF spell_creeping_doom( int sn, int level, CHAR_DATA * ch, void *vo )
{
   MOB_INDEX_DATA *temp;
   CHAR_DATA *mob;

   if( ( !IS_OUTSIDE( ch ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !IS_PLR_FLAG( ch, PLR_ONMAP ) )
   {
      send_to_char( "You must be outdoors to cast this spell.\n\r", ch );
      return rSPELL_FAILED;
   }
   if( ( temp = get_mob_index( MOB_VNUM_CREEPINGDOOM ) ) == NULL )
   {
      bug( "Spell_creeping_doom: Creeping Doom vnum %d doesn't exist.", MOB_VNUM_CREEPINGDOOM );
      return rSPELL_FAILED;
   }

   mob = create_mobile( temp );
   if( !char_to_room( mob, ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   fix_maps( ch, mob );

   send_to_char( "&[magic]You summon a vile swarm of insects into being!\n\r", ch );
   return rNONE;
}

SPELLF spell_heroes_feast( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *gch;
   int heal;

   heal = dice( 1, 4 ) + 4;

   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( is_same_group( gch, ch ) )
      {
         if( IS_NPC( gch ) )
            continue;
         if( !is_same_map( ch, gch ) )
            continue;
         else
         {
            gch->move = gch->max_move;
            gch->hit += heal;
            if( gch->hit > gch->max_hit )
               gch->hit = gch->max_hit;
            gch->pcdata->condition[COND_FULL] = sysdata.maxcondval;
            gch->pcdata->condition[COND_THIRST] = sysdata.maxcondval;
            gch->pcdata->condition[COND_DRUNK] = 0;
            send_to_char( "You partake of a magnificent feast!\n\r", gch );
         }
      }
   }
   return rNONE;
}

SPELLF spell_remove_paralysis( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   SKILLTYPE *skill = get_skilltype( sn );

   if( IS_IMMUNE( victim, RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( !is_affected( victim, gsn_paralyze ) )
   {
      if( ch != victim )
         send_to_char( "You work your cure, but it has no apparent effect.\n\r", ch );
      else
         send_to_char( "You don't seem to be paralyzed.\n\r", ch );
      return rSPELL_FAILED;
   }
   affect_strip( victim, gsn_paralyze );
   send_to_char( "&[magic]You can move again!\n\r", victim );
   if( ch != victim )
      send_to_char( "&[magic]You work your cure, removing the paralysis.\n\r", ch );
   return rNONE;
}

SPELLF spell_remove_silence( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   SKILLTYPE *skill = get_skilltype( sn );

   if( IS_IMMUNE( victim, RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( !is_affected( victim, gsn_silence ) )
   {
      if( ch != victim )
         send_to_char( "You work your cure, but it has no apparent effect.\n\r", ch );
      else
         send_to_char( "You don't seem to be silenced.\n\r", ch );
      return rSPELL_FAILED;
   }
   affect_strip( victim, gsn_silence );
   send_to_char( "&[magic]Your voice returns!\n\r", victim );
   if( ch != victim )
      send_to_char( "&[magic]You work your cure, removing the silence.\n\r", ch );
   return rNONE;
}

SPELLF spell_enchant_armor( int sn, int level, CHAR_DATA * ch, void *vo )
{
   OBJ_DATA *obj = ( OBJ_DATA * ) vo;
   AFFECT_DATA *paf;

   if( obj->item_type != ITEM_ARMOR || IS_OBJ_FLAG( obj, ITEM_MAGIC ) || obj->first_affect )
   {
      act( AT_MAGIC, "Your magic twists and winds around $p but cannot take hold.", ch, obj, NULL, TO_CHAR );
      act( AT_MAGIC, "$n's magic twists and winds around $p but cannot take hold.", ch, obj, NULL, TO_NOTVICT );
      return rSPELL_FAILED;
   }

   /*
    * Bug fix here. -- Alty 
    */
   separate_obj( obj );
   CREATE( paf, AFFECT_DATA, 1 );
   paf->type = -1;
   paf->duration = -1;
   paf->location = APPLY_AC;
/* Modified by Tarl (11 Mar 01) to adjust the bonus based on level. */
   switch ( level / 10 )
   {
      case 1:
         paf->modifier = -1;
         break;
      case 2:
         paf->modifier = -2;
         break;
      case 3:
         paf->modifier = -3;
         break;
      case 4:
         paf->modifier = -4;
         break;
      case 5:
         paf->modifier = -5;
         break;
      case 6:
         paf->modifier = -6;
         break;
      case 7:
         paf->modifier = -7;
         break;
      case 8:
         paf->modifier = -8;
         break;
      case 9:
         paf->modifier = -9;
         break;
      case 10:
         paf->modifier = -10;
         break;
      default:
         paf->modifier = -10;
   }
   paf->bit = 0;
   LINK( paf, obj->first_affect, obj->last_affect, next, prev );

   SET_OBJ_FLAG( obj, ITEM_MAGIC );

   if( IS_GOOD( ch ) )
   {
      SET_OBJ_FLAG( obj, ITEM_ANTI_EVIL );
      act( AT_BLUE, "$p glows blue.", ch, obj, NULL, TO_CHAR );
   }
   else if( IS_EVIL( ch ) )
   {
      SET_OBJ_FLAG( obj, ITEM_ANTI_GOOD );
      act( AT_RED, "$p glows red.", ch, obj, NULL, TO_CHAR );
   }
   else
   {
      SET_OBJ_FLAG( obj, ITEM_ANTI_EVIL );
      SET_OBJ_FLAG( obj, ITEM_ANTI_GOOD );
      act( AT_YELLOW, "$p glows yellow.", ch, obj, NULL, TO_CHAR );
   }

   send_to_char( "Ok.\n\r", ch );
   return rNONE;
}

SPELLF spell_remove_curse( int sn, int level, CHAR_DATA * ch, void *vo )
{
   OBJ_DATA *obj;
   CHAR_DATA *victim = ( CHAR_DATA * ) vo;
   SKILLTYPE *skill = get_skilltype( sn );

   if( IS_IMMUNE( victim, RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( is_affected( victim, gsn_curse ) )
   {
      affect_strip( victim, gsn_curse );
      send_to_char( "&[magic]The weight of your curse is lifted.\n\r", victim );
      if( ch != victim )
      {
         act( AT_MAGIC, "You dispel the curses afflicting $N.", ch, NULL, victim, TO_CHAR );
         act( AT_MAGIC, "$n's dispels the curses afflicting $N.", ch, NULL, victim, TO_NOTVICT );
      }
   }
   else if( victim->first_carrying )
   {
      for( obj = victim->first_carrying; obj; obj = obj->next_content )
         if( !obj->in_obj && ( IS_OBJ_FLAG( obj, ITEM_NOREMOVE ) || IS_OBJ_FLAG( obj, ITEM_NODROP ) ) )
         {
            if( IS_OBJ_FLAG( obj, ITEM_SINDHAE ) )
               continue;
            if( IS_OBJ_FLAG( obj, ITEM_NOREMOVE ) )
               REMOVE_OBJ_FLAG( obj, ITEM_NOREMOVE );
            if( IS_OBJ_FLAG( obj, ITEM_NODROP ) )
               REMOVE_OBJ_FLAG( obj, ITEM_NODROP );
            send_to_char( "&[magic]You feel a burden released.\n\r", victim );
            if( ch != victim )
            {
               act( AT_MAGIC, "You dispel the curses afflicting $N.", ch, NULL, victim, TO_CHAR );
               act( AT_MAGIC, "$n's dispels the curses afflicting $N.", ch, NULL, victim, TO_NOTVICT );
            }
            return rNONE;
         }
   }
   return rNONE;
}

/* A simple beacon spell, written by Quzah (quzah@geocities.com) Enjoy. */
/* Hacked to death by Samson 2-8-99 */
/* NOTE: Spell will work on overland, but results would be less than desireable.
 * Until it gets fixed, it's probably best to set the overland zones with nobeacon flags.
 */
SPELLF spell_beacon( int sn, int level, CHAR_DATA * ch, void *vo )
{
   int a;

   if( ch->in_room == NULL )
   {
      return rNONE;
   }

   if( IS_AREA_FLAG( ch->in_room->area, AFLAG_NORECALL ) || IS_AREA_FLAG( ch->in_room->area, AFLAG_NOBEACON ) )
   {
      send_to_char( "&[magic]Arcane magic disperses the spell, you cannot place a beacon in this area.\n\r", ch );
      return rSPELL_FAILED;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_NO_RECALL ) || IS_ROOM_FLAG( ch->in_room, ROOM_NOBEACON ) )
   {
      send_to_char( "&[magic]Arcane magic disperses the spell, you cannot place a beacon in this room.\n\r", ch );
      return rSPELL_FAILED;
   }

   /*
    * Set beacons with this spell, up to 5 
    */

   for( a = 0; a < MAX_BEACONS; a++ )
   {
      if( ch->pcdata->beacon[a] == 0 || ch->pcdata->beacon[a] == ch->in_room->vnum )
         break;
   }

   if( a >= MAX_BEACONS )
   {
      send_to_char( "&[magic]No more beacon space is available. You will need to clear one.\n\r", ch );
      return rSPELL_FAILED;
   }

   ch->pcdata->beacon[a] = ch->in_room->vnum;
   send_to_char( "&[magic]A magical beacon forms, tuned to your ether pattern.\n\r", ch );
   return rNONE;
}

/* Lists beacons set by the beacon spell - Samson 2-7-99 */
CMDF do_beacon( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   ROOM_INDEX_DATA *pRoomIndex = NULL;
   int a;

   if( IS_NPC( ch ) )
      return;

   argument = one_argument( argument, arg );
   /*
    * Edited by Tarl to include Area name. 24 Mar 02 
    */
   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "To clear a set beacon: beacon clear #\n\r\n\r", ch );
      send_to_char( " ## | Location name                           | Area\n\r", ch );
      send_to_char( "----+-----------------------------------------+-------------\n\r", ch );

      for( a = 0; a < MAX_BEACONS; a++ )
      {
         pRoomIndex = get_room_index( ch->pcdata->beacon[a] );
         if( pRoomIndex != NULL )
            ch_printf( ch, " %2d | %-39s | %s\n\r", a, pRoomIndex->name, pRoomIndex->area->name );
         else
            ch_printf( ch, " %2d | Not Set\n\r", a );
         pRoomIndex = NULL;
      }
      return;
   }

   if( !str_cmp( arg, "clear" ) )
   {
      a = atoi( argument );

      if( a < 0 || a >= MAX_BEACONS )
      {
         send_to_char( "Beacon value is out of range.\n\r", ch );
         return;
      }

      pRoomIndex = get_room_index( ch->pcdata->beacon[a] );

      if( !pRoomIndex )
      {
         ch->pcdata->beacon[a] = 0;
         ch_printf( ch, "Beacon %d was already empty.\n\r", a );
         return;
      }

      ch_printf( ch, "You sever your ether ties to %s.\n\r", pRoomIndex->name );
      ch->pcdata->beacon[a] = 0;
      return;
   }
   do_beacon( ch, "" );
   return;
}

/* New continent and plane based recall, moved from skills.c - Samson 3-28-98 */
int recall( CHAR_DATA * ch, int target )
{
   ROOM_INDEX_DATA *location, *beacon;
   CHAR_DATA *opponent;

   location = NULL;
   beacon = NULL;

   if( ( opponent = who_fighting( ch ) ) != NULL )
   {
      send_to_char( "You cannot recall while fighting!\n\r", ch );
      return -1;
   }

   if( !IS_NPC( ch ) && target != -1 )
   {
      if( ch->pcdata->beacon[target] == 0 )
      {
         send_to_char( "You have no beacon set in that slot!\n\r", ch );
         return -1;
      }

      beacon = get_room_index( ch->pcdata->beacon[target] );

      if( !beacon )
      {
         send_to_char( "The beacon no longer exists.....\n\r", ch );
         ch->pcdata->beacon[target] = 0;
         return -1;
      }
      if( !beacon_check( ch, beacon ) )
         return -1;
      location = beacon;
   }

   if( !location )
      location = recall_room( ch );

   if( !location )
   {
      bug( "%s", "No recall room found!" );
      send_to_char( "You cannot recall on this plane!\n\r", ch );
      return -1;
   }

   if( ch->in_room == location )
   {
      send_to_char( "Is there a point to recalling when your already there?\n\r", ch );
      return -1;
   }

   if( IS_AREA_FLAG( ch->in_room->area, AFLAG_NORECALL ) )
   {
      send_to_char( "A mystical force blocks any retreat from this area.\n\r", ch );
      return -1;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_NO_RECALL ) )
   {
      send_to_char( "A mystical force in the room blocks your attempt.\n\r", ch );
      return -1;
   }

   act( AT_MAGIC, "$n disappears in a swirl of smoke.", ch, NULL, NULL, TO_ROOM );

   leave_map( ch, NULL, location );

   act( AT_MAGIC, "$n appears from a puff of smoke!", ch, NULL, NULL, TO_ROOM );
   return 1;
}

/* Simplified recall spell for use ONLY on items - Samson 3-4-99 */
SPELLF spell_recall( int sn, int level, CHAR_DATA * ch, void *vo )
{
   int call = -1;

   call = recall( ch, -1 );

   if( call == 1 )
      return rNONE;
   else
      return rSPELL_FAILED;
}

SPELLF spell_chain_lightning( int sn, int level, CHAR_DATA * ch, void *vo )
{
   CHAR_DATA *vch_next, *vch, *victim = ( CHAR_DATA * ) vo;
   bool ch_died = FALSE;
   ch_ret retcode = rNONE;
   int dam = dice( level, 6 );

   if( victim )
      damage( ch, victim, dam, sn );

   for( vch = first_char; vch; vch = vch_next )
   {
      vch_next = vch->next;

      if( !vch->in_room || victim == vch )
         continue;

      if( vch->in_room == ch->in_room )
      {
         if( !is_same_map( ch, vch ) )
            continue;

         if( !IS_NPC( vch ) && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
            continue;

         dam = dice( UMAX( 1, level - 1 ), 6 );

         if( vch != ch && ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) ) )
            retcode = damage( ch, vch, dam, sn );

         if( retcode == rCHAR_DIED || char_died( ch ) )
         {
            ch_died = TRUE;
            continue;
         }

         if( char_died( vch ) )
            continue;
      }

      if( !ch_died && vch->in_room->area == ch->in_room->area && vch->in_room != ch->in_room )
         send_to_char( "&[magic]You hear a loud thunderclap...\n\r", vch );
   }

   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}
