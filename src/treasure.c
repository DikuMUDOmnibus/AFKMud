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
 *                       Rune/Gem socketing module                          *
 *                 Inspired by the system used in Diablo 2                  *
 *             Also contains the random treasure creation code              *
 ****************************************************************************/

#include <string.h>
#include "mud.h"
#include "clans.h"
#include "treasure.h"

void save_clan( CLAN_DATA * clan );

int ncommon, nrare, nurare;
extern int top_affect;

RUNE_DATA *first_rune;
RUNE_DATA *last_rune;
RWORD_DATA *first_rword;
RWORD_DATA *last_rword;

/* AGH! *BONK BONK BONK* Why didn't any of us think of THIS before!? So much NICER!
 * This table is also used in generating weapons.
 * If you edit this table, adjust TMAT_MAX in treasure.h by the number of entries you adjusted.
 */
const struct armorgenM armor_materials[] = {
   /*
    * Material 
    *//*
    * Material Name 
    *//*
    * Weight Mod 
    *//*
    * AC Mod 
    *//*
    * WD Mod 
    *//*
    * Cost Mod 
    *//*
    * Mob Level 
    */

   {0, "Not Defined", 0, 0, 0, 0, 0},
   {1, "Iron ", 1.25, 0, 0, 0.9, 1},
   {2, "Steel ", 1, 0, 0, 1, 1}, /* Steel is the baseline value */
   {3, "Bronze ", 1, -1, -1, 1.1, 1},
   {4, "Dwarven Steel ", 1, 0, 1, 1.4, 10},
   {5, "Silver ", 1, -2, -2, 2, 10},
   {6, "Gold ", 2, -4, -4, 4, 15},
   {7, "Elven Steel ", 0.5, 0, 0, 5, 10},
   {8, "Mithril ", 0.75, 2, 1, 5, 20},
   {9, "Titanium ", 0.25, 2, 1, 5, 25},
   {10, "Adamantine ", 0.5, 4, 2, 7, 30},
   {11, "Blackmite ", 1.2, 5, 3, 7, 40},
   {12, "Stone ", 3, 3, -1, 2, 5},
   {13, "", 1, 0, 0, 1, 1} /* Generic non-descript material cloned from steel */
};

/* If you edit this table, adjust TATP_MAX in treasure.h by the number of entries you adjusted. */
const struct armorgenT armor_type[] = {
   /*
    * Type 
    *//*
    * Name 
    *//*
    * Base Weight 
    *//*
    * Base AC 
    *//*
    * Base Cost 
    *//*
    * Flags 
    */

   {0, "Not Defined", 0, 0, 0, ""},
   {1, "Padded", 2, 1, 200, "organic"},
   {2, "Leather", 3, 2, 500, "antimage antinecro antimonk organic"},
   {3, "Hide", 5, 4, 500, "antimage antinecro antimonk organic"},
   {4, "Studded Leather", 5, 3, 700, "antimage antinecro antimonk organic"},
   {5, "Chainmail", 8, 5, 750, "antidruid antimage antinecro antimonk metal"},
   {6, "Splintmail", 13, 6, 800, "antidruid antimage antinecro antimonk antibard anticleric antirogue metal"},
   {7, "Ringmail", 10, 3, 1000, "antidruid antimage antinecro antimonk metal"},
   {8, "Scalemail", 13, 4, 1200, "antidruid antimage antinecro antimonk metal"},
   {9, "Banded Mail", 12, 6, 2000, "antidruid antimage antinecro antimonk antibard anticleric antirogue metal"},
   {10, "Platemail", 14, 8, 6000, "antidruid antimage antinecro antimonk antibard anticleric antirogue metal"},
   {11, "Field Plate", 10, 9, 20000, "antidruid antimage antinecro antimonk antibard anticleric antirogue metal"},
   {12, "Full Plate", 12, 10, 40000, "antidruid antimage antinecro antimonk antibard anticleric antirogue metal"},
   {13, "Buckler", 3, 3, 200, ""},
   {14, "Small Shield", 5, 7, 500, ""},
   {15, "Medium Shield", 5, 10, 1000, ""},
   {16, "Body Shield", 10, 15, 1500, ""}
};

/* If you edit this table, adjust TWTP_MAX in treasure.h by the number of entries you adjusted. */
const struct weaponT weapon_type[] = {
   /*
    * Type 
    *//*
    * Name 
    *//*
    * Base damage 
    *//*
    * Weight 
    *//*
    * Cost 
    *//*
    * Skill 
    *//*
    * Damage Type 
    *//*
    * Flags 
    */

   {0, "Not Defined", 0, 0, 0, 0, 0, ""},
   {1, "Dagger", 4, 4, 200, WEP_DAGGER, DAM_PIERCE, "anticleric antimonk metal"},
   {2, "Claw", 4, 4, 300, WEP_TALON, DAM_SLASH, "anticleric antimonk metal"},
   {3, "Shortsword", 5, 6, 500, WEP_SWORD, DAM_PIERCE, "anticleric antimonk antimage antinecro antidruid metal"},
   {4, "Longsword", 6, 10, 800, WEP_SWORD, DAM_SLASH, "anticleric antimonk antimage antinecro antidruid metal"},
   {5, "Claymore", 10, 20, 1600, WEP_SWORD, DAM_SLASH,
    "anticleric antimonk antimage antinecro antidruid antibard antirogue twohand metal"},
   {6, "Mace", 6, 12, 700, WEP_MACE, DAM_CRUSH, "antimonk antimage antinecro antirogue metal"},
   {7, "Maul", 10, 24, 1500, WEP_MACE, DAM_CRUSH, "antimonk antimage antinecro antirogue twohand metal"},
   {8, "Staff", 5, 8, 600, WEP_STAFF, DAM_CRUSH, "twohand"},
   {9, "Axe", 7, 14, 800, WEP_AXE, DAM_HACK, "anticleric antimonk antimage antinecro antidruid antirogue metal"},
   {10, "War Axe", 11, 26, 1600, WEP_AXE, DAM_HACK,
    "anticleric antimonk antimage antinecro antidruid antirogue twohand metal"},
   {11, "Spear", 5, 10, 600, WEP_SPEAR, DAM_THRUST, "antimage anticleric antinecro twohand"},
   {12, "Pike", 9, 15, 900, WEP_SPEAR, DAM_THRUST, "anticleric antimonk antimage antinecro antidruid antirogue twohand"}
};

/* Adjust TQUAL_MAX in treasure.h when editing this table */
char *const weapon_quality[] = {
   "Not Defined", "Average", "Good", "Superb", "Legendary"
};

char *const rarity[] = {
   "Common", "Rare", "Ultrarare"
};

char *const gems1[12] = {
   "Banded Agate", "Eye Agate", "Moss Agate", "Azurite", "Blue Quartz", "Hematite",
   "Lapus Lazuli", "Malachite", "Obsidian", "Rhodochrosite", "Tiger Eye", "Freshwater Pearl"
};

char *const gems2[16] = {
   "Bloodstone", "Carnelian", "Chalcedony", "Chrysoprase", "Citrine", "Iolite", "Jasper",
   "Moonstone", "Peridot", "Quartz", "Sadonyx", "Rose Quartz", "Smokey Quartz", "Star Rose Quartz",
   "Zircon", "Black Zircon"
};

char *const gems3[16] = {
   "Amber", "Amethyst", "Chrysoberyl", "Coral", "Red Garnet", "Brown-Green Garnet", "Jade", "Jet",
   "White Pearl", "Golden Pearl", "Pink Pearl", "Silver Pearl", "Red Spinel", "Red-Brown Spinel",
   "Deep Green Spinel", "Tourmaline"
};

char *const gems4[6] = {
   "Alexandrite", "Aquamarine", "Violet Garnet", "Black Pearl", "Deep Blue Spinel", "Golden Yellow Topaz"
};

char *const gems5[7] = {
   "Emerald", "White Opal", "Black Opal", "Fire Opal", "Blue Sapphire", "Tomb Jade", "Water Opal"
};

char *const gems6[11] = {
   "Bright Green Emerald", "Blue-White Diamond", "Pink Diamond", "Brown Diamond", "Blue Diamond",
   "Jacinth", "Black Sapphire", "Ruby", "Star Ruby", "Blue Star Sapphire", "Black Star Sapphire"
};

void free_runedata( void )
{
   RUNE_DATA *rune, *rune_next;
   RWORD_DATA *rword, *rword_next;

   for( rune = first_rune; rune; rune = rune_next )
   {
      rune_next = rune->next;
      UNLINK( rune, first_rune, last_rune, next, prev );
      STRFREE( rune->name );
      DISPOSE( rune );
   }

   for( rword = first_rword; rword; rword = rword_next )
   {
      rword_next = rword->next;
      UNLINK( rword, first_rword, last_rword, next, prev );
      DISPOSE( rword->name );
      STRFREE( rword->rune1 );
      STRFREE( rword->rune2 );
      STRFREE( rword->rune3 );
      DISPOSE( rword );
   }
   return;
}

int get_rarity( char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( rarity ) / sizeof( rarity[0] ); x++ )
      if( !str_cmp( name, rarity[x] ) )
         return x;
   return -1;
}

void read_runeword( RWORD_DATA * rword, FILE * fp )
{
   const char *word;
   bool fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'N':
            KEY( "Name", rword->name, fread_string_nohash( fp ) );
            break;

         case 'R':
            KEY( "Rune1", rword->rune1, fread_string( fp ) );
            KEY( "Rune2", rword->rune2, fread_string( fp ) );
            KEY( "Rune3", rword->rune3, fread_string( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "Stat1" ) )
            {
               rword->stat1[0] = fread_number( fp );
               rword->stat1[1] = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "Stat2" ) )
            {
               rword->stat2[0] = fread_number( fp );
               rword->stat2[1] = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "Stat3" ) )
            {
               rword->stat3[0] = fread_number( fp );
               rword->stat3[1] = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "Stat4" ) )
            {
               rword->stat4[0] = fread_number( fp );
               rword->stat4[1] = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            break;

         case 'T':
            KEY( "Type", rword->type, fread_number( fp ) );
            break;

      }
      if( !fMatch )
         bug( "read_runeword: no match: %s", word );
   }
}

void load_runewords( void )
{
   FILE *fp;
   RWORD_DATA *rword, *rword_prev;

   first_rword = NULL;
   last_rword = NULL;

   log_string( "Loading runewords..." );

   if( !( fp = fopen( RUNEWORD_FILE, "r" ) ) )
   {
      log_string( "No runeword file found." );
      return;
   }

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
         bug( "%s", "load_runewords: # not found." );
         break;
      }

      word = fread_word( fp );
      if( !str_cmp( word, "RWORD" ) )
      {
         CREATE( rword, RWORD_DATA, 1 );
         rword->rune1 = NULL;
         rword->rune2 = NULL;
         rword->rune3 = NULL;
         rword->stat1[0] = 0;
         rword->stat2[0] = 0;
         rword->stat3[0] = 0;
         rword->stat4[0] = 0;
         read_runeword( rword, fp );

         for( rword_prev = first_rword; rword_prev; rword_prev = rword_prev->next )
            if( strcasecmp( rword_prev->name, rword->name ) >= 0 )
               break;

         if( !rword_prev )
            LINK( rword, first_rword, last_rword, next, prev );
         else
            INSERT( rword, rword_prev, first_rword, next, prev );

         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "load_runewords: bad section: %s.", word );
         continue;
      }
   }
   FCLOSE( fp );
   return;
}

void read_rune( RUNE_DATA * rune, FILE * fp )
{
   const char *word;
   bool fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'N':
            KEY( "Name", rune->name, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Rarity", rune->rarity, fread_number( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "Stat1" ) )
            {
               rune->stat1[0] = fread_number( fp );
               rune->stat1[1] = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "Stat2" ) )
            {
               rune->stat2[0] = fread_number( fp );
               rune->stat2[1] = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            break;
      }
      if( !fMatch )
         bug( "read_rune: no match: %s", word );
   }
}

void load_runes( void )
{
   FILE *fp;
   RUNE_DATA *rune, *rune_prev;

   ncommon = 0, nrare = 0, nurare = 0;

   first_rune = NULL;
   last_rune = NULL;

   log_string( "Loading runes..." );

   if( !( fp = fopen( RUNE_FILE, "r" ) ) )
   {
      log_string( "No rune file found." );
      return;
   }

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
         bug( "%s", "load_runes: # not found." );
         break;
      }

      word = fread_word( fp );
      if( !str_cmp( word, "RUNE" ) )
      {
         CREATE( rune, RUNE_DATA, 1 );
         read_rune( rune, fp );

         for( rune_prev = first_rune; rune_prev; rune_prev = rune_prev->next )
            if( strcasecmp( rune_prev->name, rune->name ) >= 0 )
               break;

         if( !rune_prev )
            LINK( rune, first_rune, last_rune, next, prev );
         else
            INSERT( rune, rune_prev, first_rune, next, prev );

         switch ( rune->rarity )
         {
            case RUNE_COMMON:
               ncommon += 1;
               break;
            case RUNE_RARE:
               nrare += 1;
               break;
            case RUNE_ULTRARARE:
               nurare += 1;
               break;
            default:
               break;
         }
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "load_runes: bad section: %s.", word );
         continue;
      }
   }
   FCLOSE( fp );
   load_runewords(  );
   return;
}

void save_runes( void )
{
   FILE *fp;
   RUNE_DATA *rune;

   if( ( fp = fopen( RUNE_FILE, "w" ) ) == NULL )
   {
      log_string( "Couldn't write to rune file." );
      return;
   }

   for( rune = first_rune; rune; rune = rune->next )
   {
      fprintf( fp, "%s", "#RUNE\n" );
      fprintf( fp, "Name     %s~\n", rune->name );
      fprintf( fp, "Rarity   %d\n", rune->rarity );
      fprintf( fp, "Stat1    %d %d\n", rune->stat1[0], rune->stat1[1] );
      fprintf( fp, "Stat2    %d %d\n", rune->stat2[0], rune->stat2[1] );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   FCLOSE( fp );
}

RUNE_DATA *check_rune( char *name )
{
   RUNE_DATA *rune = NULL;

   for( rune = first_rune; rune; rune = rune->next )
   {
      if( !str_cmp( rune->name, name ) )
         return rune;
   }
   return NULL;
}

CMDF do_makerune( CHAR_DATA * ch, char *argument )
{
   RUNE_DATA *rune = NULL, *rune_prev;

   if( IS_NPC( ch ) )
   {
      send_to_char( "NPCs can't use this command.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax: makerune <name>\n\r", ch );
      return;
   }

   smash_tilde( argument );
   if( ( rune = check_rune( argument ) ) != NULL )
   {
      ch_printf( ch, "A rune called %s already exists. Choose another name.\n\r", argument );
      return;
   }

   CREATE( rune, RUNE_DATA, 1 );
   rune->name = STRALLOC( argument );
   rune->rarity = RUNE_COMMON;

   for( rune_prev = first_rune; rune_prev; rune_prev = rune_prev->next )
      if( strcasecmp( rune_prev->name, rune->name ) >= 0 )
         break;

   if( !rune_prev )
      LINK( rune, first_rune, last_rune, next, prev );
   else
      INSERT( rune, rune_prev, first_rune, next, prev );

   ch_printf( ch, "New rune %s has been created.\n\r", argument );
   save_runes(  );
   return;
}

CMDF do_destroyrune( CHAR_DATA * ch, char *argument )
{
   RUNE_DATA *rune = NULL;

   if( IS_NPC( ch ) )
   {
      send_to_char( "NPCs can't use this command.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax: destroyrune <name>\n\r", ch );
      return;
   }

   if( ( rune = check_rune( argument ) ) == NULL )
   {
      ch_printf( ch, "No rune called %s exists.\n\r", argument );
      return;
   }

   STRFREE( rune->name );
   UNLINK( rune, first_rune, last_rune, next, prev );
   DISPOSE( rune );
   save_runes(  );

   ch_printf( ch, "Rune %s has been destroyed.\n\r", argument );
   return;
}

CMDF do_setrune( CHAR_DATA * ch, char *argument )
{
   RUNE_DATA *rune;
   char arg[MIL], arg2[MIL], arg3[MIL];

   if( IS_NPC( ch ) )
   {
      send_to_char( "NPCs can't use this command.\n\r", ch );
      return;
   }
   smash_tilde( argument );
   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Syntax: setrune <rune_name> <field> <value> [second value]\n\r", ch );
      send_to_char( "Field can be one of the following:\n\r", ch );
      send_to_char( "name rarity stat1 stat2\n\r", ch );
      return;
   }

   if( !( rune = check_rune( arg ) ) )
   {
      ch_printf( ch, "No rune named %s exists.\n\r", arg );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      RUNE_DATA *newrune;

      if( ( newrune = check_rune( arg3 ) ) != NULL )
      {
         ch_printf( ch, "A rune named %s already exists. Choose a new name.\n\r", arg3 );
         return;
      }
      stralloc_printf( &rune->name, "%s", arg3 );
      save_runes(  );
      ch_printf( ch, "Rune %s has been renamed as %s\n\r", arg, arg3 );
      return;
   }

   if( !str_cmp( arg2, "rarity" ) )
   {
      int value = get_rarity( arg3 );

      if( value < 0 || value > RUNE_ULTRARARE )
      {
         ch_printf( ch, "%s is an invalid rarity.\n\r", arg3 );
         return;
      }
      rune->rarity = value;
      save_runes(  );
      ch_printf( ch, "%s rune is now %s rarity.\n\r", rune->name, rarity[value] );
      return;
   }

   if( !str_cmp( arg2, "stat1" ) )
   {
      int value = get_atype( arg3 );

      if( value < 0 )
      {
         ch_printf( ch, "%s is an invalid stat to apply.\n\r", arg3 );
         return;
      }
      if( !str_cmp( arg3, "affected" ) )
      {
         int val2 = get_aflag( argument );

         if( val2 < 0 )
         {
            ch_printf( ch, "%s is an invalid affect.\n\r", argument );
            return;
         }
         rune->stat1[0] = value;
         rune->stat1[1] = val2;
         save_runes(  );
         ch_printf( ch, "%s rune now confers: %s %s\n\r", rune->name, arg3, argument );
         return;
      }
      if( !is_number( argument ) )
      {
         send_to_char( "Apply modifier must be numerical.\n\r", ch );
         return;
      }
      rune->stat1[0] = value;
      rune->stat1[1] = atoi( argument );
      save_runes(  );
      ch_printf( ch, "%s rune now confers: %s %s\n\r", rune->name, arg3, argument );
      return;
   }

   if( !str_cmp( arg2, "stat2" ) )
   {
      int value = get_atype( arg3 );

      if( value < 0 )
      {
         ch_printf( ch, "%s is an invalid stat to apply.\n\r", arg3 );
         return;
      }
      if( !str_cmp( arg3, "affected" ) )
      {
         int val2 = get_aflag( argument );

         if( val2 < 0 )
         {
            ch_printf( ch, "%s is an invalid affect.\n\r", argument );
            return;
         }
         rune->stat2[0] = value;
         rune->stat2[1] = val2;
         save_runes(  );
         ch_printf( ch, "%s rune now confers: %s %s\n\r", rune->name, arg3, argument );
         return;
      }
      if( !is_number( argument ) )
      {
         send_to_char( "Apply modifier must be numerical.\n\r", ch );
         return;
      }
      rune->stat2[0] = value;
      rune->stat2[1] = atoi( argument );
      save_runes(  );
      ch_printf( ch, "%s rune now confers: %s %s\n\r", rune->name, arg3, argument );
      return;
   }

   return;
}

/* Ok Tarl, you've convinced me this is needed :) */
CMDF do_loadrune( CHAR_DATA * ch, char *argument )
{
   RUNE_DATA *rune;
   OBJ_DATA *obj;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Load which rune? Use showrunes to display the list.\n\r", ch );
      return;
   }

   if( ( rune = check_rune( argument ) ) == NULL )
   {
      ch_printf( ch, "%s does not exist.\n\r", argument );
      return;
   }

   if( !( obj = create_object( get_obj_index( OBJ_VNUM_RUNE ), ch->level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      send_to_char( "&RGeneric rune item is MISSING! Report to Samson.\n\r", ch );
      return;
   }
   stralloc_printf( &obj->name, "%s rune", rune->name );
   stralloc_printf( &obj->short_descr, "%s Rune", rune->name );
   stralloc_printf( &obj->objdesc, "A magical %s Rune lies here pulsating.", rune->name );
   obj->value[0] = rune->stat1[0];
   obj->value[1] = rune->stat1[1];
   obj->value[2] = rune->stat2[0];
   obj->value[3] = rune->stat2[1];
   obj_to_char( obj, ch );
   ch_printf( ch, "You now have a %s Rune.\n\r", rune->name );
   return;
}

/* Edited by Tarl 2 April 02 for alphabetical display */
/* Modified again by Samson for the same purpose only done differently :) */
CMDF do_showrunes( CHAR_DATA * ch, char *argument )
{
   RUNE_DATA *rune;
   int total = 0;

   send_to_pager( "Currently created runes:\n\r\n\r", ch );
   ch_printf( ch, "%-6.6s %-10.10s %-15.15s %-6.6s %-15.15s %-6.6s\n\r",
              "Rune", "Rarity", "Stat1", "Mod1", "Stat2", "Mod2" );

   if( !argument || argument[0] == '\0' )
   {
      for( rune = first_rune; rune; rune = rune->next )
      {
         ch_printf( ch, "%-6.6s %-10.10s %-15.15s %-6d %-15.15s %-6d\n\r", rune->name, rarity[rune->rarity],
                    a_types[rune->stat1[0]], rune->stat1[1], a_types[rune->stat2[0]], rune->stat2[1] );
         total++;
      }
      ch_printf( ch, "%d total runes displayed.\n\r", total );
   }
   else
   {
      for( rune = first_rune; rune; rune = rune->next )
      {
         if( !str_prefix( argument, rune->name ) )
         {
            ch_printf( ch, "%-6.6s %-10.10s %-15.15s %-6d %-15.15s %-6d\n\r", rune->name, rarity[rune->rarity],
                       a_types[rune->stat1[0]], rune->stat1[1], a_types[rune->stat2[0]], rune->stat2[1] );
            total++;
         }
      }
      ch_printf( ch, "%d total runes displayed.\n\r", total );
   }
   return;
}

CMDF do_runewords( CHAR_DATA * ch, char *argument )
{
   RWORD_DATA *rword;
   int total = 0;

   send_to_pager( "Currently created runewords:\n\r\n\r", ch );
   ch_printf( ch, "%-10.10s %-6.6s %-12.12s %-6.6s %-12.12s %-6.6s %-12.12s %-6.6s %-12.12s %-6.6s\n\r",
              "Word", "Type", "Stat1", "Mod1", "Stat2", "Mod2", "Stat3", "Mod3", "Stat4", "Mod4" );

   if( !argument || argument[0] == '\0' )
   {
      for( rword = first_rword; rword; rword = rword->next )
      {
         ch_printf( ch, "%-17.17s %-6.6s %-12.12s %-6d %-12.12s %-6d %-12.12s %-6d %-12.12s %-6d\n\r",
                    rword->name, rword->type == 0 ? "Armor" : "Weapon",
                    a_types[rword->stat1[0]], rword->stat1[1], a_types[rword->stat2[0]], rword->stat2[1],
                    a_types[rword->stat3[0]], rword->stat3[1], a_types[rword->stat4[0]], rword->stat4[1] );
         total++;
      }
      ch_printf( ch, "%d total runewords displayed.\n\r", total );
   }
   else
   {
      for( rword = first_rword; rword; rword = rword->next )
      {
         if( !str_prefix( argument, rword->name ) )
         {
            ch_printf( ch, "%-10.10s %-6.6s %-12.12s %-6d %-12.12s %-6d %-12.12s %-6d %-12.12s %-6d\n\r",
                       rword->name, rword->type == 0 ? "Armor" : "Weapon",
                       a_types[rword->stat1[0]], rword->stat1[1], a_types[rword->stat2[0]], rword->stat2[1],
                       a_types[rword->stat3[0]], rword->stat3[1], a_types[rword->stat4[0]], rword->stat4[1] );
            total++;
         }
      }
      ch_printf( ch, "%d total runewords displayed.\n\r", total );
   }
   return;
}

OBJ_DATA *generate_rune( CHAR_DATA * ch, int level )
{
   OBJ_DATA *newrune;
   RUNE_DATA *rune;
   int wrune = 0, cap = 0, rare = 0, pick = 0, ccount = 0, rcount = 0, ucount = 0;
   bool found = FALSE;

   if( ch->level < 20 )
      cap = 80;
   else if( ch->level < 40 )
      cap = 98;
   else
      cap = 100;

   wrune = number_range( 1, cap );

   if( wrune <= 88 )
      rare = RUNE_COMMON;
   else if( wrune <= 98 )
      rare = RUNE_RARE;
   else
      rare = RUNE_ULTRARARE;

   switch ( rare )
   {
      case RUNE_COMMON:
         pick = number_range( 1, ncommon );
         break;
      case RUNE_RARE:
         pick = number_range( 1, nrare );
         break;
      case RUNE_ULTRARARE:
         pick = number_range( 1, nurare );
         break;
      default:
         pick = 1;
         break;
   }

   for( rune = first_rune; rune; rune = rune->next )
   {
      switch ( rune->rarity )
      {
         case RUNE_COMMON:
            ccount += 1;
            if( ccount == pick && rare == rune->rarity )
               found = TRUE;
            break;
         case RUNE_RARE:
            rcount += 1;
            if( rcount == pick && rare == rune->rarity )
               found = TRUE;
            break;
         case RUNE_ULTRARARE:
            ucount += 1;
            if( ucount == pick && rare == rune->rarity )
               found = TRUE;
            break;
      }
      if( found )
         break;
   }

   if( !rune )
   {
      bug( "%s", "generate_rune: Rune data not found? Something bad happened!" );
      return NULL;
   }

   if( !( newrune = create_object( get_obj_index( OBJ_VNUM_RUNE ), level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return NULL;
   }

   stralloc_printf( &newrune->name, "%s rune", rune->name );
   stralloc_printf( &newrune->short_descr, "%s Rune", rune->name );
   stralloc_printf( &newrune->objdesc, "A magical %s Rune lies here pulsating.", rune->name );
   newrune->value[0] = rune->stat1[0];
   newrune->value[1] = rune->stat1[1];
   newrune->value[2] = rune->stat2[0];
   newrune->value[3] = rune->stat2[1];

   return newrune;
}

OBJ_DATA *generate_gem( int level )
{
   OBJ_DATA *gem;
   char *gname;
   int gemname, cost;
   int gemtable = number_range( 1, 100 );

   if( !( gem = create_object( get_obj_index( OBJ_VNUM_TREASURE ), level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return NULL;
   }

   if( gemtable <= 25 )
   {
      gemname = number_range( 0, 11 );
      gname = gems1[gemname];
      cost = 10;
   }
   else if( gemtable <= 50 )
   {
      gemname = number_range( 0, 15 );
      gname = gems2[gemname];
      cost = 50;
   }
   else if( gemtable <= 70 )
   {
      gemname = number_range( 0, 15 );
      gname = gems3[gemname];
      cost = 100;
   }
   else if( gemtable <= 90 )
   {
      gemname = number_range( 0, 5 );
      gname = gems4[gemname];
      cost = 500;
   }
   else if( gemtable <= 99 )
   {
      gemname = number_range( 0, 6 );
      gname = gems5[gemname];
      cost = 1000;
   }
   else
   {
      gemname = number_range( 0, 10 );
      gname = gems6[gemname];
      cost = 5000;
   }
   gem->item_type = ITEM_TREASURE;
   stralloc_printf( &gem->name, "gem %s", gname );
   stralloc_printf( &gem->short_descr, "%s", gname );
   stralloc_printf( &gem->objdesc, "A chunk of %s lies here gleaming.", gname );
   gem->cost = cost;
   return gem;
}

void weapongen( OBJ_DATA * obj )
{
   AFFECT_DATA *paf, *paf_next;
   char *eflags = NULL;
   char flag[MIL];
   int v8, v9, v10, value;
   bool protoflag = FALSE;

   if( obj->item_type != ITEM_WEAPON || obj->value[8] == 0 || obj->value[9] == 0 || obj->value[10] == 0 )
   {
      bug( "Improperly set item passed to weapongen: %s", obj->name );
      return;
   }

   if( obj->value[8] >= TWTP_MAX )
   {
      bug( "Improper weapon type passed to weapongen for %s", obj->name );
      obj->value[8] = 0;
      return;
   }

   if( obj->value[9] >= TMAT_MAX )
   {
      bug( "Improper material passed to weapongen for %s", obj->name );
      obj->value[9] = 0;
      return;
   }

   if( obj->value[10] >= TQUAL_MAX )
   {
      bug( "Improper quality passed to weapongen for %s", obj->name );
      obj->value[10] = 0;
      return;
   }

   if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
      protoflag = TRUE;

   v8 = obj->value[8];
   v9 = obj->value[9];
   v10 = obj->value[10];
   obj->weight = ( int )( weapon_type[v8].weight * armor_materials[v9].weight );
   obj->value[0] = obj->value[6] = sysdata.initcond;
   obj->value[1] = v10 + armor_materials[v9].wd;
   obj->value[2] = ( v10 * weapon_type[v8].wd ) + armor_materials[v9].wd;
   obj->value[3] = weapon_type[v8].damage;
   obj->value[4] = weapon_type[v8].skill;
   obj->cost = ( int )( weapon_type[v8].cost * armor_materials[v9].cost );

   eflags = weapon_type[v8].flags;

   while( eflags[0] != '\0' )
   {
      eflags = one_argument( eflags, flag );
      value = get_oflag( flag );
      if( value < 0 || value > MAX_BITS )
         bug( "weapongen: Unknown object extraflag: %s\n\r", flag );
      else
         SET_OBJ_FLAG( obj, value );
   }

   for( paf = obj->first_affect; paf; paf = paf_next )
   {
      paf_next = paf->next;
      UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
      DISPOSE( paf );
      --top_affect;
      continue;
   }

   if( protoflag )
   {
      for( paf = obj->pIndexData->first_affect; paf; paf = paf_next )
      {
         paf_next = paf->next;
         UNLINK( paf, obj->pIndexData->first_affect, obj->pIndexData->last_affect, next, prev );
         DISPOSE( paf );
         continue;
      }
   }

   /*
    * And now to adjust the index for the new settings, if needed. 
    */
   if( protoflag )
   {
      obj->pIndexData->weight = obj->weight;
      obj->pIndexData->value[0] = obj->value[0];
      obj->pIndexData->value[1] = obj->value[1];
      obj->pIndexData->value[2] = obj->value[2];
      obj->pIndexData->value[3] = obj->value[3];
      obj->pIndexData->value[4] = obj->value[4];
      obj->pIndexData->cost = obj->cost;
      obj->pIndexData->extra_flags = obj->extra_flags;
      SET_OBJ_FLAG( obj, ITEM_PROTOTYPE );
   }

   if( obj->value[9] == 11 )  /* Blackmite save vs spell/stave affect */
   {
      CREATE( paf, AFFECT_DATA, 1 );
      paf->type = -1;
      paf->duration = -1;
      paf->location = 24;  /* Save vs Spell */
      paf->modifier = -2;
      paf->bit = 0;
      if( protoflag )
         LINK( paf, obj->pIndexData->first_affect, obj->pIndexData->last_affect, next, prev );
      else
      {
         LINK( paf, obj->first_affect, obj->last_affect, next, prev );
         ++top_affect;
      }
   }
   return;
}

void armorgen( OBJ_DATA * obj )
{
   AFFECT_DATA *paf, *paf_next;
   char *eflags = NULL;
   char flag[MIL];
   int v3, v4, value;
   bool protoflag = FALSE;

   if( obj->item_type != ITEM_ARMOR || obj->value[3] == 0 || obj->value[4] == 0 )
   {
      bug( "Improperly set item passed to armorgen: %s", obj->name );
      return;
   }

   if( obj->value[3] >= TATP_MAX )
   {
      bug( "Improper armor type passed to armorgen for %s", obj->name );
      obj->value[3] = 0;
      return;
   }

   if( obj->value[4] >= TMAT_MAX )
   {
      bug( "Improper material passed to armorgen for %s", obj->name );
      obj->value[4] = 0;
      return;
   }

   if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
      protoflag = TRUE;

   /*
    * Nice ugly block of Class-anti removals and of course metal/organic 
    */
   REMOVE_OBJ_FLAG( obj, ITEM_ANTI_CLERIC );
   REMOVE_OBJ_FLAG( obj, ITEM_ANTI_MAGE );
   REMOVE_OBJ_FLAG( obj, ITEM_ANTI_ROGUE );
   REMOVE_OBJ_FLAG( obj, ITEM_ANTI_WARRIOR );
   REMOVE_OBJ_FLAG( obj, ITEM_ANTI_BARD );
   REMOVE_OBJ_FLAG( obj, ITEM_ANTI_DRUID );
   REMOVE_OBJ_FLAG( obj, ITEM_ANTI_MONK );
   REMOVE_OBJ_FLAG( obj, ITEM_ANTI_RANGER );
   REMOVE_OBJ_FLAG( obj, ITEM_ANTI_PALADIN );
   REMOVE_OBJ_FLAG( obj, ITEM_ANTI_NECRO );
   REMOVE_OBJ_FLAG( obj, ITEM_ANTI_APAL );
   REMOVE_OBJ_FLAG( obj, ITEM_METAL );
   REMOVE_OBJ_FLAG( obj, ITEM_ORGANIC );

   v3 = obj->value[3];
   v4 = obj->value[4];
   obj->weight = ( int )( armor_type[v3].weight * armor_materials[v4].weight );
   obj->value[0] = obj->value[1] = armor_type[v3].ac + armor_materials[v4].ac;
   obj->cost = ( int )( armor_type[v3].cost * armor_materials[v4].cost );

   eflags = armor_type[v3].flags;

   while( eflags[0] != '\0' )
   {
      eflags = one_argument( eflags, flag );
      value = get_oflag( flag );
      if( value < 0 || value > MAX_BITS )
         bug( "armorgen: Unknown object extraflag: %s\n\r", flag );
      else
         SET_OBJ_FLAG( obj, value );
   }

   for( paf = obj->first_affect; paf; paf = paf_next )
   {
      paf_next = paf->next;
      UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
      DISPOSE( paf );
      --top_affect;
      continue;
   }

   if( protoflag )
   {
      for( paf = obj->pIndexData->first_affect; paf; paf = paf_next )
      {
         paf_next = paf->next;
         UNLINK( paf, obj->pIndexData->first_affect, obj->pIndexData->last_affect, next, prev );
         DISPOSE( paf );
         continue;
      }
   }

   /*
    * And now to adjust the index for the new settings, if needed. 
    */
   if( protoflag )
   {
      obj->pIndexData->weight = obj->weight;
      obj->pIndexData->value[0] = obj->value[0];
      obj->pIndexData->value[1] = obj->value[1];
      obj->pIndexData->cost = obj->cost;
      obj->pIndexData->extra_flags = obj->extra_flags;
      SET_OBJ_FLAG( obj, ITEM_PROTOTYPE );
   }

   if( obj->value[4] == 11 )  /* Blackmite save vs spell/stave affect */
   {
      CREATE( paf, AFFECT_DATA, 1 );
      paf->type = -1;
      paf->duration = -1;
      paf->location = 24;  /* Save vs Spell */
      paf->modifier = -2;
      paf->bit = 0;
      if( protoflag )
         LINK( paf, obj->pIndexData->first_affect, obj->pIndexData->last_affect, next, prev );
      else
      {
         LINK( paf, obj->first_affect, obj->last_affect, next, prev );
         ++top_affect;
      }
   }
   return;
}

OBJ_DATA *generate_item( AREA_DATA * area, int level )
{
   OBJ_DATA *newitem = NULL;
   int pick;

   pick = number_range( 1, 100 );
   if( !( newitem = create_object( get_obj_index( OBJ_VNUM_TREASURE ), level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return NULL;
   }

   /*
    * Make a random scroll 
    */
   if( pick <= area->tg_scroll )
   {
      char *name;
      char *desc;
      int value = 0;
      int pick2 = number_range( 1, 10 );

      switch ( pick2 )
      {
         case 1:
            value = skill_lookup( "cure light" );
            name = "scroll cure light";
            desc = "Scroll of Cure Light Wounds";
            break;
         case 2:
            value = skill_lookup( "recall" );
            name = "scroll recall";
            desc = "Scroll of Recall";
            break;
         case 3:
            value = skill_lookup( "cure serious" );
            name = "scroll cure serious";
            desc = "Scroll of Cure Serious Wounds";
            break;
         case 4:
            value = skill_lookup( "identify" );
            name = "scroll identify";
            desc = "Scroll of Identify";
            break;
         case 5:
            value = skill_lookup( "bless" );
            name = "scroll bless";
            desc = "Scroll of Bless";
            break;
         case 6:
            value = skill_lookup( "armor" );
            name = "scroll armor";
            desc = "Scroll of Armor";
            break;
         case 7:
            value = skill_lookup( "cure poison" );
            name = "scroll cure poison";
            desc = "Scroll of Cure Poison";
            break;
         case 8:
            value = skill_lookup( "cure blindness" );
            name = "scroll cure blind";
            desc = "Scroll of Cure Blindness";
            break;
         case 9:
            value = skill_lookup( "aqua breath" );
            name = "scroll aqua breath";
            desc = "Scroll of Aqua Breath";
            break;
         case 10:
            value = skill_lookup( "refresh" );
            name = "scroll refresh";
            desc = "Scroll of Refresh";
            break;
         default:
            value = -1;
            name = "scroll blank";
            desc = "A blank scroll";
            break;
      }
      newitem->item_type = ITEM_SCROLL;
      newitem->value[0] = level;
      newitem->value[1] = value;
      newitem->value[2] = -1;
      newitem->value[3] = -1;
      newitem->cost = 200;
      newitem->rent = 0;
      stralloc_printf( &newitem->name, "%s", name );
      stralloc_printf( &newitem->short_descr, "%s", desc );
      stralloc_printf( &newitem->objdesc, "%s", "A parchment scroll lies here on the ground." );
      return ( newitem );
   }
   /*
    * Make a random potion 
    */
   else if( pick <= area->tg_potion )
   {
      char *name;
      char *desc;
      int value = 0;
      int pick2 = number_range( 1, 10 );

      switch ( pick2 )
      {
         case 1:
            value = skill_lookup( "cure light" );
            name = "potion cure light";
            desc = "Potion of Cure Light Wounds";
            break;
         case 2:
            value = skill_lookup( "bless" );
            name = "potion bless";
            desc = "Potion of Bless";
            break;
         case 3:
            value = skill_lookup( "armor" );
            name = "potion armor";
            desc = "Potion of Armor";
            break;
         case 4:
            value = skill_lookup( "cure blindness" );
            name = "potion cure blind";
            desc = "Potion of Cure Blindness";
            break;
         case 5:
            value = skill_lookup( "cure poison" );
            name = "potion cure poison";
            desc = "Potion of Cure Poison";
            break;
         case 6:
            value = skill_lookup( "cure serious" );
            name = "potion cure serious";
            desc = "Potion of Cure Serious Wounds";
            break;
         case 7:
            value = skill_lookup( "aqua breath" );
            name = "potion aqua breath";
            desc = "Potion of Aqua Breath";
            break;
         case 8:
            value = skill_lookup( "detect evil" );
            name = "potion detect evil";
            desc = "Potion of Detect Evil";
            break;
         case 9:
            value = skill_lookup( "protection from evil" );
            name = "potion protect evil";
            desc = "Potion of Protection From Evil";
            break;
         case 10:
            value = skill_lookup( "refresh" );
            name = "potion refresh";
            desc = "Potion of Refresh";
            break;
         default:
            value = -1;
            name = "flask emtpy";
            desc = "An empty flask";
            break;
      }
      newitem->item_type = ITEM_POTION;
      newitem->value[0] = level;
      newitem->value[1] = value;
      newitem->value[2] = -1;
      newitem->value[3] = -1;
      newitem->cost = 200;
      newitem->rent = 0;
      stralloc_printf( &newitem->name, "%s", name );
      stralloc_printf( &newitem->short_descr, "%s", desc );
      stralloc_printf( &newitem->objdesc, "%s", "A glass potion flask lies here on the ground." );
      return ( newitem );
   }
   else if( pick <= area->tg_wand )
   {
      char *name;
      char *desc;
      int value = 0;
      int pick2 = number_range( 1, ( level / 10 ) );

      switch ( pick2 )
      {
         case 1:
            value = skill_lookup( "magic missile" );
            name = "wand magic missile";
            desc = "Wand of Magic Missiles";
            break;
         case 2:
            value = skill_lookup( "acid blast" );
            name = "wand acid blast";
            desc = "Wand of Acid Blast";
            break;
         case 3:
            value = skill_lookup( "magnetic thrust" );
            name = "wand magnetic thrust";
            desc = "Wand of Magnetic Thrust";
            break;
         case 4:
            value = skill_lookup( "lightning bolt" );
            name = "wand lightning bolt";
            desc = "Wand of Lightning";
            break;
         case 5:
            value = skill_lookup( "fireball" );
            name = "wand fireball";
            desc = "Wand of Fireball";
            break;
         case 6:
            value = skill_lookup( "cone of cold" );
            name = "wand cone cold";
            desc = "Cone of Cold Wand";
            break;
         case 7:
            value = skill_lookup( "scorching surge" );
            name = "wand scorching surge";
            desc = "Wand of Scorching Surge";
            break;
         case 8:
            value = skill_lookup( "quantum spike" );
            name = "wand quantum spike";
            desc = "Wand of Quantum Spike";
            break;
         case 9:
            value = skill_lookup( "meteor swarm" );
            name = "wand meteor swarm";
            desc = "Wand of Meteors";
            break;
         case 10:
            value = skill_lookup( "spiral blast" );
            name = "wand spiral blast";
            desc = "Wand of Spirals";
            break;
         default:
            value = -1;
            name = "wand blank";
            desc = "A blank spell wand";
            break;
      }
      newitem->item_type = ITEM_WAND;
      SET_WEAR_FLAG( newitem, ITEM_HOLD );
      newitem->value[0] = level;
      newitem->value[1] = dice( 2, 10 );
      newitem->value[2] = newitem->value[1];
      newitem->value[3] = value;
      newitem->cost = 200;
      newitem->rent = 0;
      stralloc_printf( &newitem->name, "%s", name );
      stralloc_printf( &newitem->short_descr, "%s", desc );
      stralloc_printf( &newitem->objdesc, "%s", "A glowing wand lies here on the ground." );
      return ( newitem );
   }
   else if( pick <= area->tg_armor )
   {
      int value = 0, x, mval;

      if( level < 20 )
         value = number_range( 0, 1 );
      else if( level > 21 && level < 60 )
         value = number_range( 0, 2 );
      else
         value = number_range( 0, 3 );

      newitem->item_type = ITEM_ARMOR;
      newitem->value[2] = value;
      newitem->value[3] = number_range( 1, 12 );   /* No shields generated for this */
      /*
       * Hey look, if it takes more than 50000 iterations to find something.... 
       */
      for( x = 0; x < 50000; x++ )
      {
         mval = number_range( 1, 12 );
         if( armor_materials[mval].mlevel <= level )
         {
            newitem->value[4] = mval;
            break;
         }
      }
      if( newitem->value[3] < 5 )
         newitem->value[4] = 13; /* Sets the generic material value if an organic armor is created */
      armorgen( newitem );
      SET_OBJ_FLAG( newitem, ITEM_MAGIC );
      SET_WEAR_FLAG( newitem, ITEM_WEAR_BODY );
      if( value == 1 )
         newitem->rent = 0;
      else
         newitem->rent = sysdata.minrent - 1;
      stralloc_printf( &newitem->name, "%s%s", armor_materials[newitem->value[4]].name, armor_type[newitem->value[3]].name );
      if( newitem->value[2] > 0 )
         stralloc_printf( &newitem->short_descr, "Socketed %s%s Chestpiece",
                          armor_materials[newitem->value[4]].name, armor_type[newitem->value[3]].name );
      else
         stralloc_printf( &newitem->short_descr, "%s%s Chestpiece",
                          armor_materials[newitem->value[4]].name, armor_type[newitem->value[3]].name );
      if( newitem->value[2] > 0 )
         stralloc_printf( &newitem->objdesc, "A socketed %s%s chestpiece lies here in a heap.",
                          armor_materials[newitem->value[4]].name, armor_type[newitem->value[3]].name );
      else
         stralloc_printf( &newitem->objdesc, "A %s%s chestpiece lies here in a heap.",
                          armor_materials[newitem->value[4]].name, armor_type[newitem->value[3]].name );
      return ( newitem );
   }
   else
   {
      int x, mval, value;

      newitem->item_type = ITEM_WEAPON;
      newitem->value[8] = number_range( 1, TWTP_MAX - 1 );
      /*
       * Hey look, if it takes more than 50000 iterations to find something.... 
       */
      for( x = 0; x < 50000; x++ )
      {
         mval = number_range( 1, TMAT_MAX - 1 );
         if( armor_materials[mval].mlevel <= level )
         {
            newitem->value[9] = mval;
            break;
         }
      }

      if( level < 20 )
         newitem->value[10] = 1;
      else if( level > 19 && level < 60 )
         newitem->value[10] = 2;
      else if( level > 59 && level < LEVEL_AVATAR )
         newitem->value[10] = 3;
      else
         newitem->value[10] = 4;

      if( level < 20 )
         value = number_range( 0, 1 );
      else if( level > 19 && level < 60 )
         value = number_range( 0, 2 );
      else
         value = number_range( 0, 3 );

      weapongen( newitem );
      SET_WEAR_FLAG( newitem, ITEM_WIELD );
      SET_OBJ_FLAG( newitem, ITEM_MAGIC );
      newitem->value[7] = value;
      if( newitem->value[7] == 1 )
         newitem->rent = 0;
      else
         newitem->rent = sysdata.minrent - 1;
      stralloc_printf( &newitem->name, "%s%s",
                       armor_materials[newitem->value[9]].name, weapon_type[newitem->value[8]].name );
      if( newitem->value[7] > 0 )
         stralloc_printf( &newitem->short_descr,
                          "Socketed %s%s", armor_materials[newitem->value[9]].name, weapon_type[newitem->value[8]].name );
      else
         stralloc_printf( &newitem->short_descr,
                          "%s%s", armor_materials[newitem->value[9]].name, weapon_type[newitem->value[8]].name );
      if( newitem->value[7] > 0 )
         stralloc_printf( &newitem->objdesc, "A socketed %s%s lies here on the ground.",
                          armor_materials[newitem->value[9]].name, weapon_type[newitem->value[8]].name );
      else
         stralloc_printf( &newitem->objdesc, "A %s%s lies here on the ground.",
                          armor_materials[newitem->value[9]].name, weapon_type[newitem->value[8]].name );
      return ( newitem );
   }
}

/*
 * make some coinage
 */
OBJ_DATA *create_money( int amount )
{
   OBJ_DATA *obj;

   if( amount <= 0 )
   {
      bug( "Create_money: zero or negative money %d.", amount );
      amount = 1;
   }

   if( amount == 1 )
   {
      if( !( obj = create_object( get_obj_index( OBJ_VNUM_MONEY_ONE ), 0 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return NULL;
      }
   }
   else
   {
      if( !( obj = create_object( get_obj_index( OBJ_VNUM_MONEY_SOME ), 0 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return NULL;
      }
      stralloc_printf( &obj->short_descr, obj->short_descr, amount );
      obj->value[0] = amount;
   }
   return obj;
}

void generate_treasure( CHAR_DATA * ch, OBJ_DATA * corpse )
{
   int tchance;
   int level = corpse->level;
   AREA_DATA *area = ch->in_room->area;

   /*
    * Rolling for the initial check to see if we should be generating anything at all 
    */
   tchance = number_range( 1, 100 );

   /*
    * 1-20% chance of zilch 
    */
   if( tchance <= area->tg_nothing )
   {
      if( !str_cmp( corpse->name, "corpse random" ) )
         log_string( "Generated nothing" );
      return;
   }

   /*
    * 21-74% of generating gold 
    */
   else if( tchance <= area->tg_gold )
   {
      int x, gold;

      if( level <= 10 )
         x = 30;
      else if( level <= 20 )
         x = 40;
      else if( level <= 30 )
         x = 60;
      else if( level <= 40 )
         x = 90;
      else if( level <= 50 )
         x = 150;
      else if( level <= 60 )
         x = 170;
      else if( level <= 70 )
         x = 200;
      else if( level <= 80 )
         x = 300;
      else if( level <= 90 )
         x = 400;
      else
         x = 500;

      gold = ( dice( level, x ) + ( dice( level, x / 10 ) + dice( get_curr_lck( ch ), x / 3 ) ) );
      gold = gold + ( gold * ( ch->pcdata->exgold / 100 ) );
      obj_to_obj( create_money( gold ), corpse );
      if( !str_cmp( corpse->name, "corpse random" ) )
         log_printf( "Generated %d gold", gold );
      return;
   }
   else if( tchance <= area->tg_item )
   {
      OBJ_DATA *item = generate_item( area, level );
      if( !item )
      {
         bug( "%s", "generate_treasure: Item object failed to create!" );
         return;
      }
      obj_to_obj( item, corpse );
      if( !str_cmp( corpse->name, "corpse random" ) )
         log_printf( "Generated %s", item->short_descr );
      return;
   }
   else if( tchance <= area->tg_gem )
   {
      int x;

      for( x = 0; x < ( ( level / 25 ) + 1 ); x++ )
      {
         OBJ_DATA *item = generate_gem( level );
         if( !item )
         {
            bug( "%s", "generate_treasure: Gem object failed to create!" );
            return;
         }
         obj_to_obj( item, corpse );
      }
      if( !str_cmp( corpse->name, "corpse random" ) )
         log_string( "Generated gems" );
      return;
   }
   else
   {
      OBJ_DATA *item = generate_rune( ch, level );
      if( !item )
      {
         bug( "%s", "generate_treasure: Rune object failed to create!" );
         return;
      }
      obj_to_obj( item, corpse );
      if( !str_cmp( corpse->name, "corpse random" ) )
         log_printf( "Generated %s", item->short_descr );
      return;
   }
}

/* Command used to test random treasure drops */
CMDF do_rttest( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *corpse;
   char arg[MIL];
   int mlvl, times, x;

   if( !IS_IMP( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( !arg || arg[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: rttest <mob level> <times>\n\r", ch );
      return;
   }
   if( !is_number( arg ) && !is_number( argument ) )
   {
      send_to_char( "Numerical arguments only.\n\r", ch );
      return;
   }

   mlvl = atoi( arg );
   times = atoi( argument );

   if( times < 1 )
   {
      send_to_char( "Um, yeah. Come on man!\n\r", ch );
      return;
   }
   if( !( corpse = create_object( get_obj_index( OBJ_VNUM_CORPSE_NPC ), mlvl ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }
   stralloc_printf( &corpse->name, "%s", "corpse random" );
   stralloc_printf( &corpse->short_descr, corpse->short_descr, "some random thing" );
   stralloc_printf( &corpse->objdesc, corpse->objdesc, "some random thing" );
   obj_to_room( corpse, ch->in_room, ch );

   for( x = 0; x < times; x++ )
      generate_treasure( ch, corpse );
}

void rword_descrips( CHAR_DATA * ch, OBJ_DATA * item, RWORD_DATA * rword )
{
   ch_printf( ch, "&YAs you attach the rune, your %s glows radiantly and becomes %s!\n\r", item->short_descr, rword->name );
   stralloc_printf( &item->name, "%s %s", item->name, rword->name );
   stralloc_printf( &item->short_descr, "%s", rword->name );
   stralloc_printf( &item->objdesc, "%s lies here on the ground.", rword->name );
   return;
}

void add_rword_affect( OBJ_DATA * item, int v1, int v2 )
{
   AFFECT_DATA *paf;

   if( v1 == 0 )
      return;

   CREATE( paf, AFFECT_DATA, 1 );
   paf->type = -1;
   paf->duration = -1;
   paf->location = v1;
   if( paf->location == APPLY_WEAPONSPELL || paf->location == APPLY_WEARSPELL
       || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN
       || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
      paf->modifier = slot_lookup( v2 );
   else
      paf->modifier = v2;
   paf->bit = 0;
   LINK( paf, item->first_affect, item->last_affect, next, prev );
   ++top_affect;
   return;
}

void check_runewords( CHAR_DATA * ch, OBJ_DATA * item )
{
   RWORD_DATA *rword;

   /*
    * Runewords must contain at least 2 runes, so if these first 2 checks fail, bail out. 
    */
   if( !item->socket[0] || !str_cmp( item->socket[0], "None" ) )
      return;

   if( !item->socket[1] || !str_cmp( item->socket[1], "None" ) )
      return;

   if( item->item_type == ITEM_ARMOR )
   {
      for( rword = first_rword; rword; rword = rword->next )
      {
         if( rword->type == 1 )
            continue;

         if( !rword->rune3 )
         {
            if( !str_cmp( rword->rune1, item->socket[0] ) && !str_cmp( rword->rune2, item->socket[1] ) )
            {
               add_rword_affect( item, rword->stat1[0], rword->stat1[1] );
               add_rword_affect( item, rword->stat2[0], rword->stat2[1] );
               add_rword_affect( item, rword->stat3[0], rword->stat3[1] );
               add_rword_affect( item, rword->stat4[0], rword->stat4[1] );
               item->value[2] = 0;
               rword_descrips( ch, item, rword );
               return;
            }
            continue;
         }

         if( !item->socket[2] || !str_cmp( item->socket[2], "None" ) )
            continue;

         if( !str_cmp( rword->rune1, item->socket[0] ) && !str_cmp( rword->rune2, item->socket[1] )
             && !str_cmp( rword->rune3, item->socket[2] ) )
         {
            add_rword_affect( item, rword->stat1[0], rword->stat1[1] );
            add_rword_affect( item, rword->stat2[0], rword->stat2[1] );
            add_rword_affect( item, rword->stat3[0], rword->stat3[1] );
            add_rword_affect( item, rword->stat4[0], rword->stat4[1] );
            item->value[2] = 0;
            rword_descrips( ch, item, rword );
            return;
         }
      }
   }

   for( rword = first_rword; rword; rword = rword->next )
   {
      if( rword->type == 0 )
         continue;

      if( !rword->rune3 )
      {
         if( !str_cmp( rword->rune1, item->socket[0] ) && !str_cmp( rword->rune2, item->socket[1] ) )
         {
            add_rword_affect( item, rword->stat1[0], rword->stat1[1] );
            add_rword_affect( item, rword->stat2[0], rword->stat2[1] );
            add_rword_affect( item, rword->stat3[0], rword->stat3[1] );
            add_rword_affect( item, rword->stat4[0], rword->stat4[1] );
            item->value[7] = 0;
            rword_descrips( ch, item, rword );
            return;
         }
         continue;
      }

      if( !item->socket[2] || !str_cmp( item->socket[2], "None" ) )
         continue;

      if( !str_cmp( rword->rune1, item->socket[0] ) && !str_cmp( rword->rune2, item->socket[1] )
          && !str_cmp( rword->rune3, item->socket[2] ) )
      {
         add_rword_affect( item, rword->stat1[0], rword->stat1[1] );
         add_rword_affect( item, rword->stat2[0], rword->stat2[1] );
         add_rword_affect( item, rword->stat3[0], rword->stat3[1] );
         add_rword_affect( item, rword->stat4[0], rword->stat4[1] );
         item->value[7] = 0;
         rword_descrips( ch, item, rword );
         return;
      }
   }
   return;
}

void add_rune_affect( CHAR_DATA * ch, OBJ_DATA * item, OBJ_DATA * rune )
{
   AFFECT_DATA *paf;

   CREATE( paf, AFFECT_DATA, 1 );
   paf->type = -1;
   paf->duration = -1;
   if( item->item_type == ITEM_WEAPON || item->item_type == ITEM_MISSILE_WEAPON )
      paf->location = rune->value[0];
   else
      paf->location = rune->value[2];
   if( paf->location == APPLY_WEAPONSPELL || paf->location == APPLY_WEARSPELL
       || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN
       || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
   {
      if( item->item_type == ITEM_WEAPON || item->item_type == ITEM_MISSILE_WEAPON )
         paf->modifier = slot_lookup( rune->value[1] );
      else
         paf->modifier = slot_lookup( rune->value[3] );
   }
   else
   {
      if( item->item_type == ITEM_WEAPON || item->item_type == ITEM_MISSILE_WEAPON )
         paf->modifier = rune->value[1];
      else
         paf->modifier = rune->value[3];
   }
   paf->bit = 0;
   LINK( paf, item->first_affect, item->last_affect, next, prev );
   separate_obj( rune );
   obj_from_char( rune );
   extract_obj( rune );
   ++top_affect;
   check_runewords( ch, item );
   return;
}

CMDF do_socket( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   OBJ_DATA *rune, *item;

   if( IS_NPC( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: socket <rune> <item>\n\r\n\r", ch );
      send_to_char( "Where <rune> is the name of the rune you wish to use.\n\r", ch );
      send_to_char( "Where <item> is the weapon or armor you wish to socket the rune into.\n\r", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      do_socket( ch, "" );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      do_socket( ch, "" );
      return;
   }

   if( ( rune = get_obj_carry( ch, arg ) ) == NULL )
   {
      ch_printf( ch, "You do not have a %s rune in your inventory!\n\r", arg );
      return;
   }

   if( ( item = get_obj_carry( ch, argument ) ) == NULL )
   {
      ch_printf( ch, "You do not have a %s in your inventory!\n\r", argument );
      return;
   }

   if( rune->item_type != ITEM_RUNE )
   {
      ch_printf( ch, "%s is not a rune and cannot be used like that!\n\r", rune->short_descr );
      return;
   }

   separate_obj( item );

   if( item->item_type == ITEM_WEAPON || item->item_type == ITEM_MISSILE_WEAPON )
   {
      if( item->value[7] < 1 )
      {
         ch_printf( ch, "%s does not have any free sockets left.\n\r", item->short_descr );
         return;
      }

      if( !item->socket[0] || !str_cmp( item->socket[0], "None" ) )
      {
         stralloc_printf( &item->socket[0], "%s", capitalize( arg ) );
         item->value[7] -= 1;
         ch_printf( ch, "%s glows brightly as the %s rune is inserted and now feels more powerful!\n\r",
                    item->short_descr, capitalize( arg ) );
         add_rune_affect( ch, item, rune );
         return;
      }

      if( !item->socket[1] || !str_cmp( item->socket[1], "None" ) )
      {
         stralloc_printf( &item->socket[1], "%s", capitalize( arg ) );
         item->value[7] -= 1;
         ch_printf( ch, "%s glows brightly as the %s rune is inserted and now feels more powerful!\n\r",
                    item->short_descr, capitalize( arg ) );
         add_rune_affect( ch, item, rune );
         return;
      }

      if( !item->socket[2] || !str_cmp( item->socket[2], "None" ) )
      {
         stralloc_printf( &item->socket[2], "%s", capitalize( arg ) );
         item->value[7] -= 1;
         ch_printf( ch, "%s glows brightly as the %s rune is inserted and now feels more powerful!\n\r",
                    item->short_descr, capitalize( arg ) );
         add_rune_affect( ch, item, rune );
         return;
      }
      bug( "do_socket: (%s) %s has open sockets, but all sockets are filled?!?", ch->name, item->short_descr );
      send_to_char( "Ooops. Something bad happened. Contact the immortals for assitance.\n\r", ch );
      return;
   }

   if( item->item_type == ITEM_ARMOR && IS_WEAR_FLAG( item, ITEM_WEAR_BODY ) )
   {
      if( item->value[2] < 1 )
      {
         ch_printf( ch, "%s does not have any free sockets left.\n\r", item->short_descr );
         return;
      }

      if( !item->socket[0] || !str_cmp( item->socket[0], "None" ) )
      {
         stralloc_printf( &item->socket[0], "%s", capitalize( arg ) );
         item->value[2] -= 1;
         ch_printf( ch, "%s glows brightly as the %s rune is inserted and now feels more powerful!\n\r",
                    item->short_descr, capitalize( arg ) );
         add_rune_affect( ch, item, rune );
         return;
      }

      if( !item->socket[1] || !str_cmp( item->socket[1], "None" ) )
      {
         stralloc_printf( &item->socket[1], "%s", capitalize( arg ) );
         item->value[2] -= 1;
         ch_printf( ch, "%s glows brightly as the %s rune is inserted and now feels more powerful!\n\r",
                    item->short_descr, capitalize( arg ) );
         add_rune_affect( ch, item, rune );
         return;
      }

      if( !item->socket[2] || !str_cmp( item->socket[2], "None" ) )
      {
         stralloc_printf( &item->socket[2], "%s", capitalize( arg ) );
         item->value[2] -= 1;
         ch_printf( ch, "%s glows brightly as the %s rune is inserted and now feels more powerful!\n\r",
                    item->short_descr, capitalize( arg ) );
         add_rune_affect( ch, item, rune );
         return;
      }
      bug( "do_socket: (%s) %s has open sockets, but all sockets are filled?!?", ch->name, item->short_descr );
      send_to_char( "Ooops. Something bad happened. Contact the immortals for assitance.\n\r", ch );
      return;
   }

   ch_printf( ch, "%s cannot be socketed. Only weapons and body armors are valid.\n\r", item->short_descr );
   return;
}

int get_ore( char *ore )
{
   if( !str_cmp( ore, "iron" ) )
      return ORE_IRON;

   if( !str_cmp( ore, "gold" ) )
      return ORE_GOLD;

   if( !str_cmp( ore, "silver" ) )
      return ORE_SILVER;

   if( !str_cmp( ore, "adamantite" ) )
      return ORE_ADAM;

   if( !str_cmp( ore, "mithril" ) )
      return ORE_MITH;

   if( !str_cmp( ore, "blackmite" ) )
      return ORE_BLACK;

   if( !str_cmp( ore, "titanium" ) )
      return ORE_TITANIUM;

   if( !str_cmp( ore, "steel" ) )
      return ORE_STEEL;

   if( !str_cmp( ore, "bronze" ) )
      return ORE_BRONZE;

   if( !str_cmp( ore, "dwarven" ) )
      return ORE_DWARVEN;

   if( !str_cmp( ore, "elven" ) )
      return ORE_ELVEN;

   return -1;
}

/* Written by Samson - 6/2/00
   Rewritten by Dwip - 12/12/02 (Happy Birthday, me!)
   Re-rewritten by Tarl 13/12/02 (Happy belated Birthday, Dwip ;)
   Forge command stuff.  Eliminates the need for forgemob. 
   Utilizes the new armorgen code, and greatly expands the types
   of things makable with forge.
*/
CMDF do_forge( CHAR_DATA * ch, char *argument )
{
   /*
    * Variable declarations here 
    */
   CHAR_DATA *smith;
   OBJ_DATA *item, *oreobj, *next_content;
   CLAN_DATA *clan;
   char arg[MIL], item_type[MIL], arg3[MIL];
   int material = 0, armor = 0, weapon = 0, ore_vnum = 0, ore_type = -1;
   int base_vnum = 11299;  /* All ore vnums must be one after this one */
   int orecount, consume, location = 0, cost = 0;
   bool msmith = FALSE;
   bool gsmith = FALSE;

   /*
    * Check to see what sort of flunky the smith is 
    */
   for( smith = ch->in_room->first_person; smith; smith = smith->next_in_room )
   {
      if( IS_ACT_FLAG( smith, ACT_SMITH ) )
      {
         msmith = TRUE; /* We have a mob flagged as a smith */
         break;
      }
      if( IS_ACT_FLAG( smith, ACT_GUILDFORGE ) )
      {
         gsmith = TRUE; /* We have a mob flagged as a guildforge */
         break;
      }
   }

   /*
    * Check for required stuff for PC forging 
    */
   if( msmith == FALSE && gsmith == FALSE )
   {
      /*
       * Check to see we're dealing with a PC, here. 
       */
      if( IS_NPC( ch ) )
      {
         send_to_char( "Sorry, NPCs cannot forge items.\n\r", ch );
         return;
      }

      /*
       * Does the PC have the metallurgy skill? 
       */
      if( ch->level < skill_table[gsn_metallurgy]->race_level[ch->race] )
      {
         send_to_char( "Better leave the metallurgy to the experienced smiths.\n\r", ch );
         return;
      }

      /*
       * Does the PC have actual training in the metallurgy skill? 
       */
      if( ch->pcdata->learned[gsn_metallurgy] < 1 )
      {
         send_to_char( "Perhaps you should seek training before attempting this on your own.\n\r", ch );
         return;
      }

      /*
       * And let's make sure they're in a forge room, too. 
       */
      if( !IS_ROOM_FLAG( ch->in_room, ROOM_FORGE ) )
      {
         send_to_char( "But you are not in a forge!\n\r", ch );
         return;
      }
   }

   /*
    * Finally, the argument funness. 
    */
   argument = one_argument( argument, arg );
   argument = one_argument( argument, item_type );
   argument = one_argument( argument, arg3 );

   /*
    * Make sure we got all the args in there 
    */
   if( arg[0] == '\0' || item_type[0] == '\0' || arg3[0] == '\0' )
   {
      send_to_char( "Usage: forge <ore type> <item type> <item>\n\r\n\r", ch );
      send_to_char( "Ore type may be one of the following:\n\r", ch );
      send_to_char( "Bronze, Iron, Steel, Silver, Gold, Adamantine, Mithril, Blackmite*, or Titanium*.\n\r\n\r", ch );
      send_to_char( "Item Type may be one of the following types:\n\r", ch );
      send_to_char( "Boots, Leggings, Cuirass, Sleeves, Gauntlets, Helmet, Shield, Weapon.\n\r\n\r", ch );
      send_to_char( "Item may be one of the following if armor: \n\r", ch );
      send_to_char( "Chain, Splint, Ring, Scale, Banded, Plate, Fieldplate, Fullplate, Buckler**,\n\r", ch );
      send_to_char( "Small**, Medium**, Body**, Longsword***, Dagger***, Mace***, Axe***, Claw***\n\r", ch );
      send_to_char( "Shortsword***, Claymore***, Maul***, Staff***, WarAxe***, Spear***, Pike***\n\r\n\r", ch );
      send_to_char( "*Blackmite and Titanium ores can only be worked by Dwarves.\n\r", ch );
      send_to_char( "**For use with Item Type shield only.\n\r", ch );
      send_to_char( "***For use with Item Type weapon only.\n\r", ch );
      return;
   }

   ore_type = get_ore( arg );
   if( ore_type == -1 )
   {
      ch_printf( ch, "%s isn't a valid ore type.\n\r", arg );
      return;
   }

   /*
    * Oh, Dwip.... you thought that get_ore had no purpose? Guess again..... 
    */
   switch ( ore_type )
   {
      case ORE_IRON:
         ore_vnum = base_vnum + 1;
         material = 1;  /* this will be value4 of the armorgen */
         break;

      case ORE_SILVER:
         ore_vnum = base_vnum + 3;
         material = 5;
         break;

      case ORE_GOLD:
         ore_vnum = base_vnum + 2;
         material = 6;
         break;

      case ORE_ADAM:
         ore_vnum = base_vnum + 4;
         material = 10;
         break;

      case ORE_MITH:
         ore_vnum = base_vnum + 5;
         material = 8;
         break;

      case ORE_BLACK:
         ore_vnum = base_vnum + 6;
         material = 11;
         break;

      case ORE_TITANIUM:
         ore_vnum = base_vnum + 7;
         material = 9;
         break;

      case ORE_STEEL:
         ore_vnum = base_vnum + 8;
         material = 2;
         break;

      case ORE_BRONZE:
         ore_vnum = base_vnum + 9;
         material = 3;
         break;

      case ORE_DWARVEN:
         ore_vnum = base_vnum + 10;
         material = 12;
         break;

      case ORE_ELVEN:
         ore_vnum = base_vnum + 11;
         material = 13;
         break;
   }

   /*
    * Check to see if forger can work the material in question. 
    */
   if( ore_type == ORE_BLACK || ore_type == ORE_TITANIUM || ore_type == ORE_DWARVEN )
   {
      if( msmith || gsmith )
      {
         if( smith->race != RACE_DWARF )
         {
            interpret( smith, "say I lack the skills to work with that ore. You will have to find someone else." );
            return;
         }
      }
      else
      {
         if( ch->race != RACE_DWARF )
         {
            send_to_char( "You lack the skills to work that ore.\n\r", ch );
            return;
         }
      }
   }

   /*
    * See how much of the specified ore the PC has 
    */
   orecount = 0;
   consume = 0;

   for( oreobj = ch->first_carrying; oreobj; oreobj = oreobj->next_content )
   {
      if( oreobj->pIndexData->vnum == ore_vnum )
         orecount += oreobj->count;
   }

   if( orecount < 1 )
   {
      ch_printf( ch, "You have no %s ore to forge an item with!\n\r", arg );
      return;
   }

   /*
    * And now we play with the second argument. 
    */
   if( !str_cmp( item_type, "boots" ) )
   {
      if( orecount < 2 )
      {
         send_to_char( "You need at least 2 chunks of ore to create boots.\n\r", ch );
         return;
      }
      consume = 2;
      location = 1;
   }

   if( !str_cmp( item_type, "leggings" ) )
   {
      if( orecount < 3 )
      {
         send_to_char( "You need at least 3 chunks of ore to create leggings.\n\r", ch );
         return;
      }
      consume = 3;
      location = 2;
   }

   if( !str_cmp( item_type, "cuirass" ) )
   {
      if( orecount < 4 )
      {
         send_to_char( "You need at least 4 chunks of ore to create a cuirass.\n\r", ch );
         return;
      }
      consume = 4;
      location = 3;
   }

   if( !str_cmp( item_type, "gauntlets" ) )
   {
      if( orecount < 2 )
      {
         send_to_char( "You need at least 2 chunks of ore to create gauntlets.\n\r", ch );
         return;
      }
      consume = 2;
      location = 4;
   }

   if( !str_cmp( item_type, "sleeves" ) )
   {
      if( orecount < 2 )
      {
         send_to_char( "You need at least 2 chunks of ore to create sleeves.\n\r", ch );
         return;
      }
      consume = 2;
      location = 5;
   }

   if( !str_cmp( item_type, "helmet" ) )
   {
      if( orecount < 3 )
      {
         send_to_char( "You need at least 3 chunks of ore to create a helmet.\n\r", ch );
         return;
      }
      consume = 3;
      location = 6;
   }

   if( !str_cmp( item_type, "shield" ) )
   {
      if( orecount < 3 )
      {
         send_to_char( "You need at least 3 chunks of ore to create a shield.\n\r", ch );
         return;
      }
      consume = 3;
      location = 7;
   }

   if( !str_cmp( item_type, "weapon" ) )
   {
      if( orecount < 2 )
      {
         send_to_char( "You need at least 2 chunks of ore to create a weapon.\n\r", ch );
         return;
      }
      consume = 2;
      location = 8;
   }

   if( consume == 0 )
   {
      ch_printf( ch, "%s is not a valid item type to forge.\n\r", item_type );
      return;
   }

   /*
    * Now to play with argument 3 a bit. 
    */
   if( !str_cmp( arg3, "chain" ) )
   {
      if( location == 7 || location == 8 )
      {
         send_to_char( "Armor cannot be of types weapon or shield.\n\r", ch );
         return;
      }
      if( location == 1 )
      {
         send_to_char( "You can't make that out of chainmail...\n\r", ch );
         return;
      }
      armor = 5;
   }

   if( !str_cmp( arg3, "splint" ) )
   {
      if( location == 7 || location == 8 )
      {
         send_to_char( "Armor cannot be of types weapon or shield.\n\r", ch );
         return;
      }
      if( location == 1 || location == 4 || location == 6 )
      {
         send_to_char( "You can't make that out of splintmail...\n\r", ch );
         return;
      }
      armor = 6;
   }

   if( !str_cmp( arg3, "ring" ) )
   {
      if( location == 7 || location == 8 )
      {
         send_to_char( "Armor cannot be of types weapon or shield.\n\r", ch );
         return;
      }
      if( location == 1 || location == 6 )
      {
         send_to_char( "You can't make that out of ringmail...\n\r", ch );
         return;
      }
      armor = 7;
   }

   if( !str_cmp( arg3, "scale" ) )
   {
      if( location == 7 || location == 8 )
      {
         send_to_char( "Armor cannot be of types weapon or shield.\n\r", ch );
         return;
      }

      if( location == 1 || location == 6 )
      {
         send_to_char( "You can't make that out of scalemail...\n\r", ch );
         return;
      }
      armor = 8;
   }

   if( !str_cmp( arg3, "banded" ) )
   {
      if( location == 7 || location == 8 )
      {
         send_to_char( "Armor cannot be of types weapon or shield.\n\r", ch );
         return;
      }
      if( location == 1 || location == 6 )
      {
         send_to_char( "You can't make that out of bandedmail...\n\r", ch );
         return;
      }
      armor = 9;
   }

   if( !str_cmp( arg3, "plate" ) )
   {
      if( location == 7 || location == 8 )
      {
         send_to_char( "Armor cannot be of types weapon or shield.\n\r", ch );
         return;
      }
      armor = 10;
   }

   if( !str_cmp( arg3, "fieldplate" ) )
   {
      if( location == 7 || location == 8 )
      {
         send_to_char( "Armor cannot be of types weapon or shield.\n\r", ch );
         return;
      }
      armor = 11;
   }

   if( !str_cmp( arg3, "fullplate" ) )
   {
      if( location == 7 || location == 8 )
      {
         send_to_char( "Armor cannot be of types weapon or shield.\n\r", ch );
         return;
      }
      armor = 12;
   }

   if( !str_cmp( arg3, "buckler" ) )
   {
      if( location != 7 )
      {
         send_to_char( "Bucklers must be of type shield.\n\r", ch );
         return;
      }
      armor = 13;
   }

   if( !str_cmp( arg3, "small" ) )
   {
      if( location != 7 )
      {
         send_to_char( "Small shields must be of type shield.\n\r", ch );
         return;
      }
      armor = 14;
   }

   if( !str_cmp( arg3, "medium" ) )
   {
      if( location != 7 )
      {
         send_to_char( "Medium shields must be of type shield.\n\r", ch );
         return;
      }
      armor = 15;
   }

   if( !str_cmp( arg3, "body" ) )
   {
      if( location != 7 )
      {
         send_to_char( "Body shields must be of type shield.\n\r", ch );
         return;
      }
      armor = 16;
   }

   if( !str_cmp( arg3, "longsword" ) )
   {
      if( location != 8 )
      {
         send_to_char( "Longswords must be of type weapon.\n\r", ch );
         return;
      }
      weapon = 4;
   }

   if( !str_cmp( arg3, "claw" ) )
   {
      if( location != 8 )
      {
         send_to_char( "Claws must be of type weapon.\n\r", ch );
         return;
      }
      weapon = 2;
   }

   if( !str_cmp( arg3, "dagger" ) )
   {
      if( location != 8 )
      {
         send_to_char( "Daggers must be of type weapon.\n\r", ch );
         return;
      }
      weapon = 1;
   }

   if( !str_cmp( arg3, "mace" ) )
   {
      if( location != 8 )
      {
         send_to_char( "Maces must be of type weapon.\n\r", ch );
         return;
      }
      weapon = 6;
   }

   if( !str_cmp( arg3, "axe" ) )
   {
      if( location != 8 )
      {
         send_to_char( "Axes must be of type weapon.\n\r", ch );
         return;
      }
      weapon = 9;
   }

   if( !str_cmp( arg3, "shortsword" ) )
   {
      if( location != 8 )
      {
         send_to_char( "Shortswords must be of type weapon.\n\r", ch );
         return;
      }
      weapon = 3;
   }

   if( !str_cmp( arg3, "claymore" ) )
   {
      if( location != 8 )
      {
         send_to_char( "Claymores must be of type weapon.\n\r", ch );
         return;
      }
      weapon = 5;
   }

   if( !str_cmp( arg3, "maul" ) )
   {
      if( location != 8 )
      {
         send_to_char( "Mauls must be of type weapon.\n\r", ch );
         return;
      }
      weapon = 7;
   }

   if( !str_cmp( arg3, "staff" ) )
   {
      if( location != 8 )
      {
         send_to_char( "Staves must be of type weapon.\n\r", ch );
         return;
      }
      weapon = 8;
   }

   if( !str_cmp( arg3, "waraxe" ) )
   {
      if( location != 8 )
      {
         send_to_char( "War Axes must be of type weapon.\n\r", ch );
         return;
      }
      weapon = 10;
   }

   if( !str_cmp( arg3, "spear" ) )
   {
      if( location != 8 )
      {
         send_to_char( "Spears must be of type weapon.\n\r", ch );
         return;
      }
      weapon = 11;
   }

   if( !str_cmp( arg3, "pike" ) )
   {
      if( location != 8 )
      {
         send_to_char( "Pikes must be of type weapon.\n\r", ch );
         return;
      }
      weapon = 12;
   }

   if( armor == 0 && weapon == 0 )
   {
      ch_printf( ch, "%s is not a valid item type to forge.\n\r", arg3 );
      return;
   }

   /*
    * Check to see if the item can be made and charge $$$ 
    */
   if( msmith || gsmith )
   {
      if( location == 8 )
         cost = ( int )( 1.3 * ( weapon_type[armor].cost * armor_materials[material].cost ) );
      else
         cost = ( int )( 1.3 * ( armor_type[armor].cost * armor_materials[material].cost ) );

      if( ch->gold < cost )
      {
         act_printf( AT_TELL, smith, NULL, ch, TO_VICT,
                     "$n tells you 'It will cost %d gold to forge this, but you cannot afford it!", cost );
         return;
      }
      else
         ch->gold -= cost;
   }

   if( gsmith )
   {
      /*
       * Guild forge mobs 
       */
      if( !ch->pcdata->clan )
      {
         for( clan = first_clan; clan; clan = clan->next )
         {
            if( clan->forge == smith->pIndexData->vnum )
            {
               clan->balance += ( int )( 0.2 * cost );
               save_clan( clan );
               break;
            }
         }
      }
   }

   /*
    * Needs rewriting to go through inventory less, but couldn't use the previous
    * code since there could be multiple groups of ore. 
    */
   /*
    * Had to be modified and such. Wasn't doing anything as a while statement - Samson 
    */
   for( item = ch->first_carrying; item; item = next_content )
   {
      next_content = item->next_content;

      if( item->pIndexData->vnum == ore_vnum && consume > 0 )
      {
         if( item->count - consume <= 0 )
         {
            consume -= item->count;
            extract_obj( item );
         }
         else
         {
            item->count -= consume;
            consume = 0;
         }
      }
   }

   /*
    * If PC, check vs skill 
    */
   if( msmith == FALSE && gsmith == FALSE )
   {
      if( number_percent(  ) > ch->pcdata->learned[gsn_metallurgy] )
      {
         send_to_char( "Your efforts result in nothing more than a molten pile of goo.\n\r", ch );
         learn_from_failure( ch, gsn_metallurgy );
         return;
      }
   }

   if( !( item = create_object( get_obj_index( OBJ_VNUM_TREASURE ), 50 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      send_to_char( "Ooops. Something happened while forging the item. Inform the immortals.\n\r", ch );
      return;
   }
   obj_to_char( item, ch );

   if( location == 8 )
   {
      item->item_type = ITEM_WEAPON;
      item->value[8] = weapon;
      item->value[9] = material;

      if( ch->level < 20 )
         item->value[10] = 1;
      else if( ch->level > 19 && ch->level < 60 )
         item->value[10] = 2;
      else if( ch->level > 59 && ch->level < LEVEL_AVATAR )
         item->value[10] = 3;
      else
      {
         item->value[10] = 4;
         item->rent = sysdata.minrent - 1;
      }

      weapongen( item );
      item->value[7] = 0;
      stralloc_printf( &item->name, "%s%s", armor_materials[item->value[9]].name, weapon_type[item->value[8]].name );
      stralloc_printf( &item->short_descr, "%s%s", armor_materials[item->value[9]].name, weapon_type[item->value[8]].name );
      stralloc_printf( &item->objdesc, "A %s%s lies here on the ground.",
                       armor_materials[item->value[9]].name, weapon_type[item->value[8]].name );
   }
   else
   {
      item->item_type = ITEM_ARMOR;
      item->value[3] = armor;
      item->value[4] = material;
      armorgen( item );
      stralloc_printf( &item->name, "%s%s %s",
                       armor_materials[item->value[4]].name, armor_type[item->value[3]].name, item_type );
      stralloc_printf( &item->short_descr, "%s%s %s",
                       armor_materials[item->value[4]].name, armor_type[item->value[3]].name, item_type );
      stralloc_printf( &item->objdesc, "A %s%s %s lies here in a heap.",
                       armor_materials[item->value[4]].name, armor_type[item->value[3]].name, item_type );
   }

   /*
    * And finally - set the damn wear flag! 
    */
   switch ( location )
   {
      default:
         break;

      case 1:
         SET_WEAR_FLAG( item, ITEM_WEAR_FEET );
         break;

      case 2:
         SET_WEAR_FLAG( item, ITEM_WEAR_LEGS );
         break;

      case 3:
         SET_WEAR_FLAG( item, ITEM_WEAR_BODY );
         break;

      case 4:
         SET_WEAR_FLAG( item, ITEM_WEAR_HANDS );
         break;

      case 5:
         SET_WEAR_FLAG( item, ITEM_WEAR_ARMS );
         break;

      case 6:
         SET_WEAR_FLAG( item, ITEM_WEAR_HEAD );
         break;

      case 7:
         SET_WEAR_FLAG( item, ITEM_WEAR_SHIELD );
         break;

      case 8:
         SET_WEAR_FLAG( item, ITEM_WIELD );
         break;
   }
   SET_WEAR_FLAG( item, ITEM_TAKE );
   if( msmith || gsmith )
      ch_printf( ch, "%s forges you %s, at a cost of %d gold.\n\r", smith->short_descr, item->short_descr, cost );
   else
      ch_printf( ch, "You've forged yourself %s!\n\r", item->short_descr );
   return;
}
