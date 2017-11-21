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
 *                         Table load/save Module                           *
 ****************************************************************************/

#include <string.h>
#include <limits.h>
#include <dlfcn.h>
#include "mud.h"
#include "language.h"

int get_logflag( char *flag );
void free_social( SOCIALTYPE * social );
void add_social( SOCIALTYPE * social );
void free_command( CMDTYPE * command );
void add_command( CMDTYPE * command );
bool load_race_file( const char *fname );
void skill_notfound( CHAR_DATA * ch, char *argument );
SPELLF spell_notfound( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_null( int sn, int level, CHAR_DATA * ch, void *vo );
bool validate_spec_fun( char *name );

/* global variables */
int top_sn;
int top_herb;
int MAX_PC_CLASS;
int MAX_PC_RACE;

SKILLTYPE *skill_table[MAX_SKILL];
CLASS_TYPE *class_table[MAX_CLASS];
RACE_TYPE *race_table[MAX_RACE];
char *title_table[MAX_CLASS][MAX_LEVEL + 1][2];
SKILLTYPE *herb_table[MAX_HERB];
SKILLTYPE *disease_table[MAX_DISEASE];

LANG_DATA *first_lang;
LANG_DATA *last_lang;

char *const skill_tname[] = { "unknown", "Spell", "Skill", "Weapon", "Tongue", "Herb", "Racial", "Disease", "Lore" };

char *const old_ris_flags[] = {
   "fire", "cold", "electricity", "energy", "blunt", "pierce", "slash", "acid",
   "poison", "drain", "sleep", "charm", "hold", "nonmagic", "plus1", "plus2",
   "plus3", "plus4", "plus5", "plus6", "magic", "paralysis", "good", "evil", "hack",
   "lash"
};

SPELL_FUN *spell_function( char *name )
{
   void *funHandle;
   const char *error;

   funHandle = dlsym( sysdata.dlHandle, name );
   if( ( error = dlerror(  ) ) != NULL )
   {
      bug( "Error locating %s in symbol table. %s", name, error );
      return ( SPELL_FUN * ) spell_notfound;
   }
   return ( SPELL_FUN * ) funHandle;
}

DO_FUN *skill_function( char *name )
{
   void *funHandle;
   const char *error;

   funHandle = dlsym( sysdata.dlHandle, name );
   if( ( error = dlerror(  ) ) != NULL )
   {
      bug( "Error locating %s in symbol table. %s", name, error );
      return ( DO_FUN * ) skill_notfound;
   }
   return ( DO_FUN * ) funHandle;
}

bool load_class_file( const char *fname )
{
   char buf[256];
   const char *word;
   bool fMatch;
   CLASS_TYPE *Class;
   int cl = -1;
   int tlev = 0;
   FILE *fp;

   snprintf( buf, 256, "%s%s", CLASS_DIR, fname );
   if( !( fp = fopen( buf, "r" ) ) )
   {
      perror( buf );
      return FALSE;
   }

   CREATE( Class, CLASS_TYPE, 1 );

   /*
    * Setup defaults for additions to Class structure 
    */
   xCLEAR_BITS( Class->affected );
   xCLEAR_BITS( Class->resist );
   xCLEAR_BITS( Class->suscept );

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

         case 'A':
            KEY( "Affected", Class->affected, fread_bitvector( fp ) );
            KEY( "Armor", Class->armor, fread_number( fp ) );
            KEY( "Armwear", Class->armwear, fread_number( fp ) ); /* Samson */
            KEY( "AttrPrime", Class->attr_prime, fread_number( fp ) );
            break;

         case 'C':
            KEY( "Class", cl, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               FCLOSE( fp );
               if( cl < 0 || cl >= MAX_CLASS )
               {
                  bug( "Load_class_file: Class (%s) bad/not found (%d)",
                       Class->who_name ? Class->who_name : "name not found", cl );
                  STRFREE( Class->who_name );
                  DISPOSE( Class );
                  return FALSE;
               }
               class_table[cl] = Class;
               return TRUE;
            }

         case 'F':
            KEY( "Footwear", Class->footwear, fread_number( fp ) );  /* Samson 1-3-99 */
            break;

         case 'H':
            KEY( "Headwear", Class->headwear, fread_number( fp ) );  /* Samson 1-3-99 */
            KEY( "Held", Class->held, fread_number( fp ) ); /* Samson 1-3-99 */
            KEY( "HpMax", Class->hp_max, fread_number( fp ) );
            KEY( "HpMin", Class->hp_min, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Legwear", Class->legwear, fread_number( fp ) ); /* Samson 1-3-99 */
            break;

         case 'M':
            KEY( "Mana", Class->fMana, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", Class->who_name, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Resist", Class->resist, fread_bitvector( fp ) );
            break;

         case 'S':
            KEY( "Shield", Class->shield, fread_number( fp ) );

            if( !str_cmp( word, "Skill" ) )
            {
               int sn, lev, adp;

               word = fread_word( fp );
               lev = fread_number( fp );
               adp = fread_number( fp );
               sn = skill_lookup( word );
               if( cl < 0 || cl >= MAX_CLASS )
                  bug( "tables.c: load_class_file: Skill %s -- Class bad/not found (%d)", word, cl );
               else if( !IS_VALID_SN( sn ) )
                  bug( "%s: Skill %s unknown. Class: %d", __FUNCTION__, word, cl );
               else
               {
                  skill_table[sn]->skill_level[cl] = lev;
                  skill_table[sn]->skill_adept[cl] = adp;
               }
               fMatch = TRUE;
               break;
            }
            KEY( "Skilladept", Class->skill_adept, fread_number( fp ) );
            KEY( "Suscept", Class->suscept, fread_bitvector( fp ) );
            break;

         case 'T':
            if( !str_cmp( word, "Title" ) )
            {
               if( cl < 0 || cl >= MAX_CLASS )
               {
                  bug( "tables.c: load_class_file: Title -- Class bad/not found (%d)", cl );
                  fread_flagstring( fp );
                  fread_flagstring( fp );
               }
               else if( tlev < MAX_LEVEL + 1 )
               {
                  title_table[cl][tlev][0] = fread_string( fp );
                  title_table[cl][tlev][1] = fread_string( fp );
                  ++tlev;
               }
               else
                  bug( "tables.c: load_class_file: Too many titles. Class: %d", cl );
               fMatch = TRUE;
               break;
            }
            KEY( "Thac0gain", Class->thac0_gain, fread_float( fp ) );
            KEY( "Thac0", Class->base_thac0, fread_number( fp ) );
            break;

         case 'W':
            KEY( "Weapon", Class->weapon, fread_number( fp ) );
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
 * Load in all the Class files.
 */
void load_classes(  )
{
   FILE *fpList;
   const char *filename;
   char classlist[256];
   int i;

   MAX_PC_CLASS = 0;

   /*
    * Pre-init the class_table with blank classes
    */
   for( i = 0; i < MAX_CLASS; i++ )
      class_table[i] = NULL;

   snprintf( classlist, 256, "%s%s", CLASS_DIR, CLASS_LIST );
   if( !( fpList = fopen( classlist, "r" ) ) )
   {
      perror( classlist );
      exit( 1 );
   }

   for( ;; )
   {
      filename = feof( fpList ) ? "$" : fread_word( fpList );
      if( filename[0] == '$' )
         break;

      if( !load_class_file( filename ) )
         bug( "tables.c: load_classes: Cannot load Class file: %s", filename );
      else
         MAX_PC_CLASS++;
   }
   FCLOSE( fpList );
   for( i = 0; i < MAX_CLASS; i++ )
   {
      if( class_table[i] == NULL )
      {
         CREATE( class_table[i], CLASS_TYPE, 1 );
         class_table[i]->who_name = STRALLOC( "No Class name" );
      }
   }
   return;
}

void write_class_file( int cl )
{
   FILE *fpout;
   char filename[256];
   struct class_type *Class = class_table[cl];
   int x, y;

   snprintf( filename, 256, "%s%s.class", CLASS_DIR, Class->who_name );
   if( !( fpout = fopen( filename, "w" ) ) )
   {
      bug( "tables.c: write_class_file: Cannot open: %s for writing", filename );
      return;
   }
   fprintf( fpout, "Name        %s~\n", Class->who_name );
   fprintf( fpout, "Class       %d\n", cl );
   fprintf( fpout, "Attrprime   %d\n", Class->attr_prime );
   fprintf( fpout, "Weapon      %d\n", Class->weapon );
   fprintf( fpout, "Armor       %d\n", Class->armor );   /* Samson */
   fprintf( fpout, "Legwear     %d\n", Class->legwear ); /* Samson 1-3-99 */
   fprintf( fpout, "Headwear    %d\n", Class->headwear );   /* Samson 1-3-99 */
   fprintf( fpout, "Armwear     %d\n", Class->armwear ); /* Samson 1-3-99 */
   fprintf( fpout, "Footwear    %d\n", Class->footwear );   /* Samson 1-3-99 */
   fprintf( fpout, "Shield      %d\n", Class->shield );  /* Samson 1-3-99 */
   fprintf( fpout, "Held        %d\n", Class->held ); /* Samson 1-3-99 */
   fprintf( fpout, "Skilladept  %d\n", Class->skill_adept );
   fprintf( fpout, "Thac0       %d\n", Class->base_thac0 );
   fprintf( fpout, "Thac0gain   %f\n", ( float )Class->thac0_gain );
   fprintf( fpout, "Hpmin       %d\n", Class->hp_min );
   fprintf( fpout, "Hpmax       %d\n", Class->hp_max );
   fprintf( fpout, "Mana        %d\n", Class->fMana );
   fprintf( fpout, "Affected    %s\n", print_bitvector( &Class->affected ) );
   fprintf( fpout, "Resist	 %s\n", print_bitvector( &Class->resist ) );
   fprintf( fpout, "Suscept	 %s\n", print_bitvector( &Class->suscept ) );
   for( x = 0; x < top_sn; x++ )
   {
      if( !skill_table[x]->name || skill_table[x]->name[0] == '\0' )
         break;
      if( ( y = skill_table[x]->skill_level[cl] ) < LEVEL_IMMORTAL )
         fprintf( fpout, "Skill '%s' %d %d\n", skill_table[x]->name, y, skill_table[x]->skill_adept[cl] );
   }
   for( x = 0; x <= MAX_LEVEL; x++ )
      fprintf( fpout, "Title\n%s~\n%s~\n", title_table[cl][x][0], title_table[cl][x][1] );

   fprintf( fpout, "%s", "End\n" );
   FCLOSE( fpout );
}

/*
 * Load in all the race files.
 */
void load_races(  )
{
   FILE *fpList;
   const char *filename;
   char racelist[256];
   int i;

   MAX_PC_RACE = 0;

   /*
    * Pre-init the race_table with blank races
    */
   for( i = 0; i < MAX_RACE; i++ )
      race_table[i] = NULL;

   snprintf( racelist, 256, "%s%s", RACE_DIR, RACE_LIST );
   if( !( fpList = fopen( racelist, "r" ) ) )
   {
      perror( racelist );
      exit( 1 );
   }

   for( ;; )
   {
      filename = feof( fpList ) ? "$" : fread_word( fpList );
      if( filename[0] == '$' )
         break;

      if( !load_race_file( filename ) )
         bug( "Cannot load race file: %s", filename );
      else
         MAX_PC_RACE++;
   }
   for( i = 0; i < MAX_RACE; i++ )
   {
      if( race_table[i] == NULL )
      {
         CREATE( race_table[i], RACE_TYPE, 1 );
         snprintf( race_table[i]->race_name, 16, "%s", "unused" );
      }
   }
   FCLOSE( fpList );
   return;
}

void write_race_file( int ra )
{
   FILE *fpout;
   char filename[256];
   struct race_type *race = race_table[ra];
   int i, x, y;

   if( !race->race_name )
   {
      bug( "Race %d has null name, not writing .race file.", ra );
      return;
   }

   snprintf( filename, 256, "%s%s.race", RACE_DIR, race->race_name );
   if( !( fpout = fopen( filename, "w" ) ) )
   {
      bug( "Cannot open: %s for writing", filename );
      return;
   }

   fprintf( fpout, "Name        %s~\n", race->race_name );
   fprintf( fpout, "Race        %d\n", ra );
   fprintf( fpout, "Classes     %d\n", race->class_restriction );
   fprintf( fpout, "Str_Plus    %d\n", race->str_plus );
   fprintf( fpout, "Dex_Plus    %d\n", race->dex_plus );
   fprintf( fpout, "Wis_Plus    %d\n", race->wis_plus );
   fprintf( fpout, "Int_Plus    %d\n", race->int_plus );
   fprintf( fpout, "Con_Plus    %d\n", race->con_plus );
   fprintf( fpout, "Cha_Plus    %d\n", race->cha_plus );
   fprintf( fpout, "Lck_Plus    %d\n", race->lck_plus );
   fprintf( fpout, "Hit         %d\n", race->hit );
   fprintf( fpout, "Mana        %d\n", race->mana );
   fprintf( fpout, "Affected    %s\n", print_bitvector( &race->affected ) );
   fprintf( fpout, "Resist      %s\n", print_bitvector( &race->resist ) );
   fprintf( fpout, "Suscept     %s\n", print_bitvector( &race->suscept ) );
   fprintf( fpout, "Language    %d\n", race->language );
   fprintf( fpout, "Align       %d\n", race->alignment );
   fprintf( fpout, "Min_Align  %d\n", race->minalign );
   fprintf( fpout, "Max_Align	%d\n", race->maxalign );
   fprintf( fpout, "AC_Plus    %d\n", race->ac_plus );
   fprintf( fpout, "Exp_Mult   %d\n", race->exp_multiplier );
   fprintf( fpout, "Bodyparts  %d\n", race->body_parts );
   fprintf( fpout, "Attacks    %s\n", print_bitvector( &race->attacks ) );
   fprintf( fpout, "Defenses   %s\n", print_bitvector( &race->defenses ) );
   fprintf( fpout, "Height     %d\n", race->height );
   fprintf( fpout, "Weight     %d\n", race->weight );
   fprintf( fpout, "Hunger_Mod  %d\n", race->hunger_mod );
   fprintf( fpout, "Thirst_mod  %d\n", race->thirst_mod );
   fprintf( fpout, "Mana_Regen  %d\n", race->mana_regen );
   fprintf( fpout, "HP_Regen    %d\n", race->hp_regen );
   for( i = 0; i < MAX_WHERE_NAME; i++ )
      fprintf( fpout, "WhereName  %s~\n", race->where_name[i] );

   for( x = 0; x < top_sn; x++ )
   {
      if( !skill_table[x]->name || skill_table[x]->name[0] == '\0' )
         break;
      if( ( y = skill_table[x]->race_level[ra] ) < LEVEL_IMMORTAL )
         fprintf( fpout, "Skill '%s' %d %d\n", skill_table[x]->name, y, skill_table[x]->race_adept[ra] );
   }
   fprintf( fpout, "%s", "End\n" );
   FCLOSE( fpout );
}

bool load_race_file( const char *fname )
{
   char buf[256];
   const char *word;
   char *race_name = NULL;
   bool fMatch;
   RACE_TYPE *race;
   int ra = -1;
   FILE *fp;
   int i, wear = 0;

   snprintf( buf, 256, "%s%s", RACE_DIR, fname );
   if( !( fp = fopen( buf, "r" ) ) )
   {
      perror( buf );
      return FALSE;
   }

   CREATE( race, RACE_TYPE, 1 );
   for( i = 0; i < MAX_WHERE_NAME; i++ )
      race->where_name[i] = STRALLOC( where_name[i] );

   race->body_parts = 0;

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

         case 'A':
            KEY( "Align", race->alignment, fread_number( fp ) );
            KEY( "AC_Plus", race->ac_plus, fread_number( fp ) );
            KEY( "Affected", race->affected, fread_bitvector( fp ) );
            KEY( "Attacks", race->attacks, fread_bitvector( fp ) );
            break;

         case 'B':
            KEY( "Bodyparts", race->body_parts, fread_number( fp ) );
            break;

         case 'C':
            KEY( "Con_Plus", race->con_plus, fread_number( fp ) );
            KEY( "Cha_Plus", race->cha_plus, fread_number( fp ) );
            KEY( "Classes", race->class_restriction, fread_number( fp ) );
            break;

         case 'D':
            KEY( "Dex_Plus", race->dex_plus, fread_number( fp ) );
            KEY( "Defenses", race->defenses, fread_bitvector( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               FCLOSE( fp );
               if( ra < 0 || ra >= MAX_RACE )
               {
                  bug( "Load_race_file: Race (%s) bad/not found (%d)",
                       race->race_name ? race->race_name : "name not found", ra );
                  STRFREE( race_name );
                  DISPOSE( race );
                  return FALSE;
               }
               race_table[ra] = race;
               STRFREE( race_name );
               return TRUE;
            }

            KEY( "Exp_Mult", race->exp_multiplier, fread_number( fp ) );
            break;


         case 'I':
            KEY( "Int_Plus", race->int_plus, fread_number( fp ) );
            break;

         case 'H':
            KEY( "Height", race->height, fread_number( fp ) );
            KEY( "Hit", race->hit, fread_number( fp ) );
            KEY( "HP_Regen", race->hp_regen, fread_number( fp ) );
            KEY( "Hunger_Mod", race->hunger_mod, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Language", race->language, fread_number( fp ) );
            KEY( "Lck_Plus", race->lck_plus, fread_number( fp ) );
            break;


         case 'M':
            KEY( "Mana", race->mana, fread_number( fp ) );
            KEY( "Mana_Regen", race->mana_regen, fread_number( fp ) );
            KEY( "Min_Align", race->minalign, fread_number( fp ) );
            race->minalign = -1000;
            KEY( "Max_Align", race->maxalign, fread_number( fp ) );
            race->maxalign = -1000;
            break;

         case 'N':
            KEY( "Name", race_name, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Race", ra, fread_number( fp ) );
            KEY( "Resist", race->resist, fread_bitvector( fp ) );
            break;

         case 'S':
            KEY( "Str_Plus", race->str_plus, fread_number( fp ) );
            KEY( "Suscept", race->suscept, fread_bitvector( fp ) );
            if( !str_cmp( word, "Skill" ) )
            {
               int sn, lev, adp;

               word = fread_word( fp );
               lev = fread_number( fp );
               adp = fread_number( fp );
               sn = skill_lookup( word );
               if( ra < 0 || ra >= MAX_RACE )
                  bug( "load_race_file: Skill %s -- race bad/not found (%d)", word, ra );
               else if( !IS_VALID_SN( sn ) )
               {
                  bug( "loading races - skill %s = SN %d", word, sn );
                  bug( "load_race_file: Skill %s unknown", word );
               }
               else
               {
                  skill_table[sn]->race_level[ra] = lev;
                  skill_table[sn]->race_adept[ra] = adp;
               }
               fMatch = TRUE;
               break;
            }
            break;

         case 'T':
            KEY( "Thirst_Mod", race->thirst_mod, fread_number( fp ) );
            break;

         case 'W':
            KEY( "Weight", race->weight, fread_number( fp ) );
            KEY( "Wis_Plus", race->wis_plus, fread_number( fp ) );
            if( !str_cmp( word, "WhereName" ) )
            {
               if( ra < 0 || ra >= MAX_RACE )
               {
                  bug( "load_race_file: Title -- race bad/not found (%d)", ra );
                  fread_flagstring( fp );
                  fread_flagstring( fp );
               }
               else if( wear < MAX_WHERE_NAME )
               {
                  STRFREE( race->where_name[wear] );
                  race->where_name[wear] = fread_string( fp );
                  ++wear;
               }
               else
                  bug( "%s", "load_race_file: Too many where_names" );
               fMatch = TRUE;
               break;
            }
            break;
      }
      if( race_name != NULL )
         snprintf( race->race_name, 16, "%s", race_name );

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
}

/*
 * Function used by qsort to sort skills
 */
int skill_comp( SKILLTYPE ** sk1, SKILLTYPE ** sk2 )
{
   SKILLTYPE *skill1 = ( *sk1 );
   SKILLTYPE *skill2 = ( *sk2 );

   if( !skill1 && skill2 )
      return 1;
   if( skill1 && !skill2 )
      return -1;
   if( !skill1 && !skill2 )
      return 0;
   if( skill1->type < skill2->type )
      return -1;
   if( skill1->type > skill2->type )
      return 1;
   return strcmp( skill1->name, skill2->name );
}

/*
 * Sort the skill table with qsort
 */
void sort_skill_table(  )
{
   log_string( "Sorting skill table..." );
   qsort( &skill_table[1], top_sn - 1, sizeof( SKILLTYPE * ), ( int ( * )( const void *, const void * ) )skill_comp );
}

/*
 * Write skill data to a file
 */
void fwrite_skill( FILE * fpout, SKILLTYPE * skill )
{
   SMAUG_AFF *aff;
   int modifier;

   fprintf( fpout, "Name         %s~\n", skill->name );
   fprintf( fpout, "Type         %s\n", skill_tname[skill->type] );
   fprintf( fpout, "Info         %d\n", skill->info );
   if( skill->author && skill->author[0] != '\0' )
      fprintf( fpout, "Author	    %s~\n", skill->author );
   fprintf( fpout, "Flags        %d\n", skill->flags );
   if( skill->target )
      fprintf( fpout, "Target       %d\n", skill->target );
   if( skill->minimum_position )
      fprintf( fpout, "Minpos       %s~\n", npc_position[skill->minimum_position] );
   if( skill->saves )
      fprintf( fpout, "Saves        %d\n", skill->saves );
   if( skill->slot )
      fprintf( fpout, "Slot         %d\n", skill->slot );
   if( skill->min_mana )
      fprintf( fpout, "Mana         %d\n", skill->min_mana );
   if( skill->beats )
      fprintf( fpout, "Rounds       %d\n", skill->beats );
   if( skill->range )
      fprintf( fpout, "Range        %d\n", skill->range );
   if( skill->guild != -1 )
      fprintf( fpout, "Guild        %d\n", skill->guild );
   if( skill->rent )
      fprintf( fpout, "Rent		%d\n", skill->rent );
   if( skill->skill_fun )
      fprintf( fpout, "Code         %s\n", skill->skill_fun_name );
   else if( skill->spell_fun )
      fprintf( fpout, "Code         %s\n", skill->spell_fun_name );
   fprintf( fpout, "Dammsg       %s~\n", skill->noun_damage );
   if( skill->msg_off && skill->msg_off[0] != '\0' )
      fprintf( fpout, "Wearoff      %s~\n", skill->msg_off );

   if( skill->hit_char && skill->hit_char[0] != '\0' )
      fprintf( fpout, "Hitchar      %s~\n", skill->hit_char );
   if( skill->hit_vict && skill->hit_vict[0] != '\0' )
      fprintf( fpout, "Hitvict      %s~\n", skill->hit_vict );
   if( skill->hit_room && skill->hit_room[0] != '\0' )
      fprintf( fpout, "Hitroom      %s~\n", skill->hit_room );
   if( skill->hit_dest && skill->hit_dest[0] != '\0' )
      fprintf( fpout, "Hitdest      %s~\n", skill->hit_dest );

   if( skill->miss_char && skill->miss_char[0] != '\0' )
      fprintf( fpout, "Misschar     %s~\n", skill->miss_char );
   if( skill->miss_vict && skill->miss_vict[0] != '\0' )
      fprintf( fpout, "Missvict     %s~\n", skill->miss_vict );
   if( skill->miss_room && skill->miss_room[0] != '\0' )
      fprintf( fpout, "Missroom     %s~\n", skill->miss_room );

   if( skill->die_char && skill->die_char[0] != '\0' )
      fprintf( fpout, "Diechar      %s~\n", skill->die_char );
   if( skill->die_vict && skill->die_vict[0] != '\0' )
      fprintf( fpout, "Dievict      %s~\n", skill->die_vict );
   if( skill->die_room && skill->die_room[0] != '\0' )
      fprintf( fpout, "Dieroom      %s~\n", skill->die_room );

   if( skill->imm_char && skill->imm_char[0] != '\0' )
      fprintf( fpout, "Immchar      %s~\n", skill->imm_char );
   if( skill->imm_vict && skill->imm_vict[0] != '\0' )
      fprintf( fpout, "Immvict      %s~\n", skill->imm_vict );
   if( skill->imm_room && skill->imm_room[0] != '\0' )
      fprintf( fpout, "Immroom      %s~\n", skill->imm_room );

   if( skill->dice && skill->dice[0] != '\0' )
      fprintf( fpout, "Dice         %s~\n", skill->dice );
   if( skill->value )
      fprintf( fpout, "Value        %d\n", skill->value );
   if( skill->difficulty )
      fprintf( fpout, "Difficulty   %d\n", skill->difficulty );
   if( skill->participants )
      fprintf( fpout, "Participants %d\n", skill->participants );
   if( skill->components && skill->components[0] != '\0' )
      fprintf( fpout, "Components   %s~\n", skill->components );
   if( skill->teachers && skill->teachers[0] != '\0' )
      fprintf( fpout, "Teachers     %s~\n", skill->teachers );
   for( aff = skill->affects; aff; aff = aff->next )
   {
      fprintf( fpout, "Affect       '%s' %d ", aff->duration, aff->location );
      modifier = atoi( aff->modifier );
      if( ( aff->location == APPLY_WEAPONSPELL
            || aff->location == APPLY_WEARSPELL
            || aff->location == APPLY_REMOVESPELL
            || aff->location == APPLY_STRIPSN || aff->location == APPLY_RECURRINGSPELL ) && IS_VALID_SN( modifier ) )
         fprintf( fpout, "'%d' ", skill_table[modifier]->slot );
      else
         fprintf( fpout, "'%s' ", aff->modifier );
      fprintf( fpout, "%d\n", aff->bitvector );
   }

   if( skill->type != SKILL_HERB )
   {
      int y, min = 1000;

      for( y = 0; y < MAX_CLASS; ++y )
         if( skill->skill_level[y] < min )
            min = skill->skill_level[y];

      fprintf( fpout, "Minlevel     %d\n", min );

      min = 1000;
      for( y = 0; y < MAX_RACE; ++y )
         if( skill->race_level[y] < min )
            min = skill->race_level[y];
   }
   fprintf( fpout, "%s", "End\n\n" );
}

/*
 * Save the skill table to disk
 */
#define SKILLVERSION 3
/* Updated to 1 for position text - Samson 4-26-00 */
/* 2 was skipped */
/* Updated to 3 for AFF_NONE insertion - Samson 7-27-04 */
void save_skill_table(  )
{
   int x;
   FILE *fpout;

   if( !( fpout = fopen( SKILL_FILE, "w" ) ) )
   {
      perror( SKILL_FILE );
      bug( "%s", "Cannot open skills.dat for writing" );
      return;
   }

   fprintf( fpout, "#VERSION	%d\n", SKILLVERSION );

   for( x = 0; x < top_sn; ++x )
   {
      if( !skill_table[x]->name || skill_table[x]->name[0] == '\0' )
         break;
      fprintf( fpout, "%s", "#SKILL\n" );
      fwrite_skill( fpout, skill_table[x] );
   }
   fprintf( fpout, "%s", "#END\n" );
   FCLOSE( fpout );
}

/*
 * Save the herb table to disk
 */
/* Uses the same format as skills, therefore the skill version applies */
void save_herb_table(  )
{
   int x;
   FILE *fpout;

   if( ( fpout = fopen( HERB_FILE, "w" ) ) == NULL )
   {
      perror( HERB_FILE );
      bug( "%s", "Cannot open herbs.dat for writing" );
      return;
   }

   fprintf( fpout, "#VERSION	%d\n", SKILLVERSION );

   for( x = 0; x < top_herb; x++ )
   {
      if( !herb_table[x]->name || herb_table[x]->name[0] == '\0' )
         break;
      fprintf( fpout, "%s", "#HERB\n" );
      fwrite_skill( fpout, herb_table[x] );
   }
   fprintf( fpout, "%s", "#END\n" );
   FCLOSE( fpout );
}

/*
 * Save the socials to disk
 */
void save_socials( void )
{
   FILE *fpout;
   SOCIALTYPE *social;
   int x;

   if( ( fpout = fopen( SOCIAL_FILE, "w" ) ) == NULL )
   {
      bug( "%s", "Cannot open socials.dat for writting" );
      perror( SOCIAL_FILE );
      return;
   }

   for( x = 0; x < 27; x++ )
   {
      for( social = social_index[x]; social; social = social->next )
      {
         if( !social->name || social->name[0] == '\0' )
         {
            bug( "Save_socials: blank social in hash bucket %d", x );
            continue;
         }
         fprintf( fpout, "%s", "#SOCIAL\n" );
         fprintf( fpout, "Name        %s~\n", social->name );
         if( social->char_no_arg )
            fprintf( fpout, "CharNoArg   %s~\n", social->char_no_arg );
         else
            bug( "Save_socials: NULL char_no_arg in hash bucket %d", x );
         if( social->others_no_arg )
            fprintf( fpout, "OthersNoArg %s~\n", social->others_no_arg );
         if( social->char_found )
            fprintf( fpout, "CharFound   %s~\n", social->char_found );
         if( social->others_found )
            fprintf( fpout, "OthersFound %s~\n", social->others_found );
         if( social->vict_found )
            fprintf( fpout, "VictFound   %s~\n", social->vict_found );
         if( social->char_auto )
            fprintf( fpout, "CharAuto    %s~\n", social->char_auto );
         if( social->others_auto )
            fprintf( fpout, "OthersAuto  %s~\n", social->others_auto );
         if( social->obj_self )
            fprintf( fpout, "ObjSelf     %s~\n", social->obj_self );
         if( social->obj_others )
            fprintf( fpout, "ObjOthers   %s~\n", social->obj_others );
         fprintf( fpout, "%s", "End\n\n" );
      }
   }
   fprintf( fpout, "%s", "#END\n" );
   FCLOSE( fpout );
}

int get_skill( char *skilltype )
{
   if( !str_cmp( skilltype, "Racial" ) )
      return SKILL_RACIAL;
   if( !str_cmp( skilltype, "Spell" ) )
      return SKILL_SPELL;
   if( !str_cmp( skilltype, "Skill" ) )
      return SKILL_SKILL;
   if( !str_cmp( skilltype, "Weapon" ) )
      return SKILL_WEAPON;
   if( !str_cmp( skilltype, "Tongue" ) )
      return SKILL_TONGUE;
   if( !str_cmp( skilltype, "Herb" ) )
      return SKILL_HERB;
   if( !str_cmp( skilltype, "Lore" ) )
      return SKILL_LORE;
   return SKILL_UNKNOWN;
}

/*
 * Save the commands to disk
 * Added flags Aug 25, 1997 --Shaddai
 */

#define CMDVERSION 3
/* Updated to 1 for command position - Samson 4-26-00 */
/* Updated to 2 for log flag - Samson 4-26-00 */
/* Updated to 3 for command flags - Samson 7-9-00 */
void save_commands( void )
{
   FILE *fpout;
   CMDTYPE *command;
   int x;

   if( ( fpout = fopen( COMMAND_FILE, "w" ) ) == NULL )
   {
      bug( "%s", "Cannot open commands.dat for writing" );
      perror( COMMAND_FILE );
      return;
   }

   fprintf( fpout, "#VERSION	%d\n", CMDVERSION );

   for( x = 0; x < 126; x++ )
   {
      for( command = command_hash[x]; command; command = command->next )
      {
         if( !command->name || command->name[0] == '\0' )
         {
            bug( "Save_commands: blank command in hash bucket %d", x );
            continue;
         }
         fprintf( fpout, "%s", "#COMMAND\n" );
         fprintf( fpout, "Name        %s~\n", command->name );
         /*
          * Modded to use new field - Trax 
          */
         fprintf( fpout, "Code        %s\n", command->fun_name ? command->fun_name : "" );
         fprintf( fpout, "Position    %s~\n", npc_position[command->position] );
         fprintf( fpout, "Level       %d\n", command->level );
         fprintf( fpout, "Log         %s~\n", log_flag[command->log] );
         if( command->flags )
            fprintf( fpout, "Flags       %s~\n", flag_string( command->flags, cmd_flags ) );
         fprintf( fpout, "%s", "End\n\n" );
      }
   }
   fprintf( fpout, "%s", "#END\n" );
   FCLOSE( fpout );
}

SKILLTYPE *fread_skill( FILE * fp, int version )
{
   const char *word;
   bool fMatch;
   bool got_info = FALSE;
   SKILLTYPE *skill;
   int x;

   CREATE( skill, SKILLTYPE, 1 );
   skill->slot = 0;
   skill->min_mana = 0;
   for( x = 0; x < MAX_CLASS; ++x )
   {
      skill->skill_level[x] = LEVEL_IMMORTAL;
      skill->skill_adept[x] = 95;
   }
   for( x = 0; x < MAX_RACE; ++x )
   {
      skill->race_level[x] = LEVEL_IMMORTAL;
      skill->race_adept[x] = 95;
   }
   skill->guild = -1;
   skill->target = 0;
   skill->rent = -2;
   skill->skill_fun = NULL;
   skill->spell_fun = NULL;

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

         case 'A':
            KEY( "Author", skill->author, fread_string( fp ) );
            if( !str_cmp( word, "Affect" ) )
            {
               SMAUG_AFF *aff;
               char mod[MIL];

               CREATE( aff, SMAUG_AFF, 1 );
               aff->duration = str_dup( fread_word( fp ) );
               aff->location = fread_number( fp );
               mudstrlcpy( mod, fread_word( fp ), MIL );

               if( version < 2 )
               {
                  if( version < 1 )
                  {
                     if( aff->location == APPLY_AFFECT || aff->location == APPLY_EXT_AFFECT )
                     {
                        int mvalue = atoi( mod );

                        mudstrlcpy( mod, a_flags[mvalue], MIL );
                     }
                     if( aff->location == APPLY_RESISTANT
                      || aff->location == APPLY_IMMUNE
                      || aff->location == APPLY_ABSORB
                      || aff->location == APPLY_SUSCEPTIBLE )
                     {
                        int mvalue = atoi( mod );

                        mudstrlcpy( mod, flag_string( mvalue, old_ris_flags ), MIL );
                     }
                  }
                  else
                  {
                     if( ( aff->location == APPLY_RESISTANT && is_number( mod ) )
                       || ( aff->location == APPLY_IMMUNE && is_number( mod ) )
                       || ( aff->location == APPLY_ABSORB && is_number( mod ) )
                       || ( aff->location == APPLY_SUSCEPTIBLE && is_number( mod ) ) )
                     {
                        int mvalue = atoi( mod );

                        mudstrlcpy( mod, flag_string( mvalue, old_ris_flags ), MIL );
                     }
                  }
               }

               if( aff->location == APPLY_AFFECT )
                  aff->location = APPLY_EXT_AFFECT;
               aff->modifier = str_dup( mod );
               aff->bitvector = fread_number( fp );
               if( version < 3 && aff->bitvector > -1 )
                  ++aff->bitvector;
               if( !got_info )
               {
                  for( x = 0; x < 32; ++x )
                  {
                     if( IS_SET( aff->bitvector, 1 << x ) )
                     {
                        aff->bitvector = x;
                        break;
                     }
                  }
                  if( x == 32 )
                     aff->bitvector = -1;
               }
               aff->next = skill->affects;
               skill->affects = aff;
               fMatch = TRUE;
               break;
            }
            break;

         case 'C':
            if( !str_cmp( word, "Class" ) )
            {
               int Class = fread_number( fp );

               skill->skill_level[Class] = fread_number( fp );
               skill->skill_adept[Class] = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "Code" ) )
            {
               SPELL_FUN *spellfun;
               DO_FUN *dofun;
               char *w = fread_word( fp );

               fMatch = TRUE;
               if( validate_spec_fun( w ) )
               {
                  bug( "ERROR: Trying to assign spec_fun to skill/spell %s", w );
                  skill->skill_fun = skill_notfound;
                  skill->spell_fun = spell_notfound;
               }
               else if( !str_prefix( "do_", w ) && ( dofun = skill_function( w ) ) != skill_notfound )
               {
                  skill->skill_fun = dofun;
                  skill->spell_fun = NULL;
                  skill->skill_fun_name = str_dup( w );
               }
               else if( str_prefix( "do_", w ) && ( spellfun = spell_function( w ) ) != spell_notfound )
               {
                  skill->spell_fun = spellfun;
                  skill->skill_fun = NULL;
                  skill->spell_fun_name = str_dup( w );
               }
               else
               {
                  bug( "fread_skill: unknown skill/spell %s", w );
                  skill->spell_fun = spell_null;
               }
               break;
            }
            KEY( "Components", skill->components, fread_string_nohash( fp ) );
            break;

         case 'D':
            KEY( "Dammsg", skill->noun_damage, fread_string_nohash( fp ) );
            KEY( "Dice", skill->dice, fread_string_nohash( fp ) );
            KEY( "Diechar", skill->die_char, fread_string_nohash( fp ) );
            KEY( "Dieroom", skill->die_room, fread_string_nohash( fp ) );
            KEY( "Dievict", skill->die_vict, fread_string_nohash( fp ) );
            KEY( "Difficulty", skill->difficulty, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( skill->saves != 0 && SPELL_SAVE( skill ) == SE_NONE )
               {
                  bug( "fread_skill(%s):  Has saving throw (%d) with no saving effect.", skill->name, skill->saves );
                  SET_SSAV( skill, SE_NEGATE );
               }
               if( !skill->author )
                  skill->author = STRALLOC( "Smaug" );
               return skill;
            }
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               skill->flags = fread_number( fp );
               /*
                * convert to new style       -Thoric
                */
               if( !got_info )
               {
                  skill->info = skill->flags & ( BV11 - 1 );
                  if( IS_SET( skill->flags, OLD_SF_SAVE_NEGATES ) )
                  {
                     if( IS_SET( skill->flags, OLD_SF_SAVE_HALF_DAMAGE ) )
                     {
                        SET_SSAV( skill, SE_QUARTERDAM );
                        REMOVE_BIT( skill->flags, OLD_SF_SAVE_HALF_DAMAGE );
                     }
                     else
                        SET_SSAV( skill, SE_NEGATE );
                     REMOVE_BIT( skill->flags, OLD_SF_SAVE_NEGATES );
                  }
                  else if( IS_SET( skill->flags, OLD_SF_SAVE_HALF_DAMAGE ) )
                  {
                     SET_SSAV( skill, SE_HALFDAM );
                     REMOVE_BIT( skill->flags, OLD_SF_SAVE_HALF_DAMAGE );
                  }
                  skill->flags >>= 11;
               }
               fMatch = TRUE;
               break;
            }
            break;

         case 'G':
            KEY( "Guild", skill->guild, fread_number( fp ) );
            break;

         case 'H':
            KEY( "Hitchar", skill->hit_char, fread_string_nohash( fp ) );
            KEY( "Hitdest", skill->hit_dest, fread_string_nohash( fp ) );
            KEY( "Hitroom", skill->hit_room, fread_string_nohash( fp ) );
            KEY( "Hitvict", skill->hit_vict, fread_string_nohash( fp ) );
            break;

         case 'I':
            KEY( "Immchar", skill->imm_char, fread_string_nohash( fp ) );
            KEY( "Immroom", skill->imm_room, fread_string_nohash( fp ) );
            KEY( "Immvict", skill->imm_vict, fread_string_nohash( fp ) );
            if( !str_cmp( word, "Info" ) )
            {
               skill->info = fread_number( fp );
               got_info = TRUE;
               fMatch = TRUE;
               break;
            }
            break;

         case 'M':
            KEY( "Mana", skill->min_mana, fread_number( fp ) );
            if( !str_cmp( word, "Minlevel" ) )
            {
               fread_to_eol( fp );
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "Minpos" ) )
            {
               if( version < 1 )
               {
                  fMatch = TRUE;
                  skill->minimum_position = fread_number( fp );
                  if( skill->minimum_position < 100 )
                  {
                     switch ( skill->minimum_position )
                     {
                        default:
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                           break;
                        case 5:
                           skill->minimum_position = 6;
                           break;
                        case 6:
                           skill->minimum_position = 8;
                           break;
                        case 7:
                           skill->minimum_position = 9;
                           break;
                        case 8:
                           skill->minimum_position = 12;
                           break;
                        case 9:
                           skill->minimum_position = 13;
                           break;
                        case 10:
                           skill->minimum_position = 14;
                           break;
                        case 11:
                           skill->minimum_position = 15;
                           break;
                     }
                  }
                  else
                     skill->minimum_position -= 100;
                  break;
               }
               else
               {
                  int position;

                  position = get_npc_position( fread_flagstring( fp ) );

                  if( position < 0 || position >= POS_MAX )
                  {
                     bug( "fread_skill: Skill %s has invalid position! Defaulting to standing.", skill->name );
                     position = POS_STANDING;
                  }
                  skill->minimum_position = position;
                  fMatch = TRUE;
                  break;
               }
            }

            KEY( "Misschar", skill->miss_char, fread_string_nohash( fp ) );
            KEY( "Missroom", skill->miss_room, fread_string_nohash( fp ) );
            KEY( "Missvict", skill->miss_vict, fread_string_nohash( fp ) );
            break;

         case 'N':
            KEY( "Name", skill->name, fread_string_nohash( fp ) );
            break;

         case 'P':
            KEY( "Participants", skill->participants, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Range", skill->range, fread_number( fp ) );
            KEY( "Rounds", skill->beats, fread_number( fp ) );
            KEY( "Rent", skill->rent, fread_number( fp ) );
            if( !str_cmp( word, "Race" ) )
            {
               int race = fread_number( fp );

               skill->race_level[race] = fread_number( fp );
               skill->race_adept[race] = fread_number( fp );
               fMatch = TRUE;
               break;
            }
            break;

         case 'S':
            KEY( "Slot", skill->slot, fread_number( fp ) );
            KEY( "Saves", skill->saves, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Target", skill->target, fread_number( fp ) );
            KEY( "Teachers", skill->teachers, fread_string_nohash( fp ) );
            KEY( "Type", skill->type, get_skill( fread_word( fp ) ) );
            break;

         case 'V':
            KEY( "Value", skill->value, fread_number( fp ) );
            break;

         case 'W':
            KEY( "Wearoff", skill->msg_off, fread_string_nohash( fp ) );
            break;
      }
      if( !fMatch )
         bug( "Fread_skill: no match: %s", word );
   }
}

void load_skill_table( void )
{
   FILE *fp;
   int version = 0;

   if( ( fp = fopen( SKILL_FILE, "r" ) ) != NULL )
   {
      top_sn = 0;
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
            bug( "%s: # not found.", __FUNCTION__ );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "VERSION" ) )
         {
            version = fread_number( fp );
            continue;
         }
         if( !str_cmp( word, "SKILL" ) )
         {
            if( top_sn >= MAX_SKILL )
            {
               bug( "%s: more skills than MAX_SKILL %d", __FUNCTION__, MAX_SKILL );
               FCLOSE( fp );
               return;
            }
            skill_table[top_sn++] = fread_skill( fp, version );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s: bad section: %s", __FUNCTION__, word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   else
   {
      perror( SKILL_FILE );
      bug( "%s", "Cannot open skills.dat" );
      exit( 1 );
   }
}

void load_herb_table(  )
{
   FILE *fp;
   int version = 0;

   if( ( fp = fopen( HERB_FILE, "r" ) ) != NULL )
   {
      top_herb = 0;
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
            bug( "%s", "Load_herb_table: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "VERSION" ) )
         {
            version = fread_number( fp );
            continue;
         }
         if( !str_cmp( word, "HERB" ) )
         {
            if( top_herb >= MAX_HERB )
            {
               bug( "load_herb_table: more herbs than MAX_HERB %d", MAX_HERB );
               FCLOSE( fp );
               return;
            }
            herb_table[top_herb++] = fread_skill( fp, version );
            if( herb_table[top_herb - 1]->slot == 0 )
               herb_table[top_herb - 1]->slot = top_herb - 1;
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "Load_herb_table: bad section: %s", word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   else
   {
      bug( "%s", "Cannot open herbs.dat" );
      exit( 1 );
   }
}

void fread_social( FILE * fp )
{
   const char *word;
   bool fMatch;
   SOCIALTYPE *social;

   CREATE( social, SOCIALTYPE, 1 );

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

         case 'C':
            KEY( "CharNoArg", social->char_no_arg, fread_string_nohash( fp ) );
            KEY( "CharFound", social->char_found, fread_string_nohash( fp ) );
            KEY( "CharAuto", social->char_auto, fread_string_nohash( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !social->name )
               {
                  bug( "%s", "Fread_social: Name not found" );
                  free_social( social );
                  return;
               }
               if( !social->char_no_arg )
               {
                  bug( "%s", "Fread_social: CharNoArg not found" );
                  free_social( social );
                  return;
               }
               add_social( social );
               return;
            }
            break;

         case 'N':
            KEY( "Name", social->name, fread_string_nohash( fp ) );
            break;

         case 'O':
            KEY( "ObjOthers", social->obj_others, fread_string_nohash( fp ) );
            KEY( "ObjSelf", social->obj_self, fread_string_nohash( fp ) );
            KEY( "OthersNoArg", social->others_no_arg, fread_string_nohash( fp ) );
            KEY( "OthersFound", social->others_found, fread_string_nohash( fp ) );
            KEY( "OthersAuto", social->others_auto, fread_string_nohash( fp ) );
            break;

         case 'V':
            KEY( "VictFound", social->vict_found, fread_string_nohash( fp ) );
            break;
      }

      if( !fMatch )
         bug( "Fread_social: no match: %s", word );
   }
}

void load_socials( void )
{
   FILE *fp;

   if( ( fp = fopen( SOCIAL_FILE, "r" ) ) != NULL )
   {
      top_sn = 0;
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
            bug( "%s", "Load_socials: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "SOCIAL" ) )
         {
            fread_social( fp );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "Load_socials: bad section: %s", word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   else
   {
      bug( "%s", "Cannot open socials.dat" );
      exit( 1 );
   }
}

/*
 *  Added the flags Aug 25, 1997 --Shaddai
 */
void fread_command( FILE * fp, int version )
{
   const char *word;
   bool fMatch;
   CMDTYPE *command;

   CREATE( command, CMDTYPE, 1 );
   command->flags = 0;  /* Default to no flags set */

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

         case 'C':
            KEY( "Code", command->fun_name, str_dup( fread_word( fp ) ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !command->name )
               {
                  bug( "%s", "Fread_command: Name not found" );
                  free_command( command );
                  return;
               }
               if( !command->fun_name )
               {
                  bug( "fread_command: No function name supplied for %s", command->name );
                  free_command( command );
                  return;
               }
               /*
                * Mods by Trax
                * Fread in code into char* and try linkage here then
                * deal in the "usual" way I suppose..
                */
               command->do_fun = skill_function( command->fun_name );
               if( command->do_fun == skill_notfound )
               {
                  bug( "Fread_command: Function %s not found for %s", command->fun_name, command->name );
                  free_command( command );
                  return;
               }
               add_command( command );
               /*
                * Automatically approve all immortal commands for use as a ghost 
                */
               if( command->level >= LEVEL_IMMORTAL )
                  SET_BIT( command->flags, CMD_GHOST );
               return;
            }
            break;

         case 'F':
            if( !str_cmp( word, "flags" ) )
            {
               if( version < 3 )
               {
                  command->flags = fread_number( fp );
                  fMatch = TRUE;
                  break;
               }
               else
               {
                  char *cmdflags = NULL;
                  char flag[MIL];
                  int value;

                  cmdflags = fread_flagstring( fp );

                  while( cmdflags[0] != '\0' )
                  {
                     cmdflags = one_argument( cmdflags, flag );
                     value = get_cmdflag( flag );
                     if( value < 0 || value > 31 )
                        bug( "Unknown command flag: %s\n\r", flag );
                     else
                        TOGGLE_BIT( command->flags, 1 << value );
                  }
                  fMatch = TRUE;
                  break;
               }
            }
            break;

         case 'L':
            KEY( "Level", command->level, fread_number( fp ) );
            if( !str_cmp( word, "Log" ) )
            {
               if( version < 2 )
               {
                  command->log = fread_number( fp );
                  fMatch = TRUE;
                  break;
               }
               else
               {
                  char *lflag = NULL;
                  int lognum;

                  lflag = fread_flagstring( fp );
                  lognum = get_logflag( lflag );

                  if( lognum < 0 || lognum > LOG_ALL )
                  {
                     bug( "fread_command: Command %s has invalid log flag! Defaulting to normal.", command->name );
                     lognum = LOG_NORMAL;
                  }
                  command->log = lognum;
                  fMatch = TRUE;
                  break;
               }
            }
            break;

         case 'N':
            KEY( "Name", command->name, fread_string_nohash( fp ) );
            break;

         case 'P':
            if( !str_cmp( word, "Position" ) )
            {
               char *tpos = NULL;
               int position;

               tpos = fread_flagstring( fp );
               position = get_npc_position( tpos );

               if( position < 0 || position >= POS_MAX )
               {
                  bug( "fread_command: Command %s has invalid position! Defaulting to standing.", command->name );
                  position = POS_STANDING;
               }
               command->position = position;
               fMatch = TRUE;
               break;
            }
            break;
      }

      if( !fMatch )
         bug( "Fread_command: no match: %s", word );
   }
}

void load_commands( void )
{
   FILE *fp;
   int version = 0;

   if( ( fp = fopen( COMMAND_FILE, "r" ) ) != NULL )
   {
      top_sn = 0;
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
            bug( "%s", "Load_commands: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "VERSION" ) )
         {
            version = fread_number( fp );
            continue;
         }
         if( !str_cmp( word, "COMMAND" ) )
         {
            fread_command( fp, version );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "Load_commands: bad section: %s", word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   else
   {
      bug( "%s", "Cannot open commands.dat" );
      exit( 1 );
   }
}

void save_classes(  )
{
   int x;

   for( x = 0; x < MAX_PC_CLASS; x++ )
      write_class_file( x );
}

void save_races(  )
{
   int x;

   for( x = 0; x < MAX_PC_RACE; x++ )
      write_race_file( x );
}

void free_tongues( void )
{
   LANG_DATA *lang, *lang_next;
   LCNV_DATA *lcnv, *lcnv_next;

   for( lang = first_lang; lang; lang = lang_next )
   {
      lang_next = lang->next;

      for( lcnv = lang->first_precnv; lcnv; lcnv = lcnv_next )
      {
         lcnv_next = lcnv->next;
         UNLINK( lcnv, lang->first_precnv, lang->last_precnv, next, prev );
         DISPOSE( lcnv->old );
         DISPOSE( lcnv->lnew );
         DISPOSE( lcnv );
      }
      for( lcnv = lang->first_cnv; lcnv; lcnv = lcnv_next )
      {
         lcnv_next = lcnv->next;
         UNLINK( lcnv, lang->first_cnv, lang->last_cnv, next, prev );
         DISPOSE( lcnv->old );
         DISPOSE( lcnv->lnew );
         DISPOSE( lcnv );
      }
      STRFREE( lang->name );
      DISPOSE( lang->alphabet );
      UNLINK( lang, first_lang, last_lang, next, prev );
      DISPOSE( lang );
   }
   return;
}

/*
 * Tongues / Languages loading/saving functions			-Altrag
 */
void fread_cnv( FILE * fp, LCNV_DATA ** first_cnv, LCNV_DATA ** last_cnv )
{
   LCNV_DATA *cnv;
   char letter;

   for( ;; )
   {
      letter = fread_letter( fp );
      if( letter == '~' || letter == EOF )
         break;
      ungetc( letter, fp );
      CREATE( cnv, LCNV_DATA, 1 );

      cnv->old = str_dup( fread_word( fp ) );
      cnv->olen = strlen( cnv->old );
      cnv->lnew = str_dup( fread_word( fp ) );
      cnv->nlen = strlen( cnv->lnew );
      fread_to_eol( fp );
      LINK( cnv, *first_cnv, *last_cnv, next, prev );
   }
}

void load_tongues(  )
{
   FILE *fp;
   LANG_DATA *lng;
   char *word;
   char letter;

   if( !( fp = fopen( TONGUE_FILE, "r" ) ) )
   {
      perror( "Load_tongues" );
      return;
   }
   for( ;; )
   {
      letter = fread_letter( fp );
      if( letter == EOF )
         break;
      else if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }
      else if( letter != '#' )
      {
         bug( "Letter '%c' not #.", letter );
         exit( 1 );
      }
      word = fread_word( fp );
      if( !str_cmp( word, "end" ) )
         break;
      fread_to_eol( fp );
      CREATE( lng, LANG_DATA, 1 );
      lng->name = STRALLOC( word );
      fread_cnv( fp, &lng->first_precnv, &lng->last_precnv );
      lng->alphabet = fread_string_nohash( fp );
      fread_cnv( fp, &lng->first_cnv, &lng->last_cnv );
      fread_to_eol( fp );
      LINK( lng, first_lang, last_lang, next, prev );
   }
   FCLOSE( fp );
   return;
}
