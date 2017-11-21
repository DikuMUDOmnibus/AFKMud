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
 *                           Deity handling module                          *
 ****************************************************************************/

/* Put together by Rennard for Realms of Despair.  Brap on... */

#include "mud.h"
#include "deity.h"

bool check_pets( CHAR_DATA * ch, MOB_INDEX_DATA * pet );
ROOM_INDEX_DATA *recall_room( CHAR_DATA * ch );

DEITY_DATA *first_deity;
DEITY_DATA *last_deity;

void free_deity( DEITY_DATA * deity )
{
   UNLINK( deity, first_deity, last_deity, next, prev );
   STRFREE( deity->name );
   DISPOSE( deity->deitydesc );
   DISPOSE( deity->filename );
   DISPOSE( deity );
   return;
}

void free_deities( void )
{
   DEITY_DATA *deity, *deity_next;

   for( deity = first_deity; deity; deity = deity_next )
   {
      deity_next = deity->next;
      free_deity( deity );
   }
   return;
}

/* Get pointer to deity structure from deity name */
DEITY_DATA *get_deity( char *name )
{
   DEITY_DATA *deity;
   for( deity = first_deity; deity; deity = deity->next )
      if( !str_cmp( name, deity->name ) )
         return deity;
   return NULL;
}

void write_deity_list( void )
{
   DEITY_DATA *tdeity;
   FILE *fpout;
   char filename[256];

   snprintf( filename, 256, "%s%s", DEITY_DIR, DEITY_LIST );
   fpout = fopen( filename, "w" );
   if( !fpout )
      bug( "%s", "FATAL: cannot open deity.lst for writing!" );
   else
   {
      for( tdeity = first_deity; tdeity; tdeity = tdeity->next )
         fprintf( fpout, "%s\n", tdeity->filename );
      fprintf( fpout, "%s", "$\n" );
      FCLOSE( fpout );
   }
}

/* Save a deity's data to its data file */
/* Modified to include deity recallroom for supplication - Samson 4-13-98 */
#define DEITY_VERSION 1 /* Added for deity file compatibility. Adjani, 1-31-04 */
void save_deity( DEITY_DATA * deity )
{
   FILE *fp;
   char filename[256];

   if( !deity )
   {
      bug( "%s", "save_deity: null deity pointer!" );
      return;
   }

   if( !deity->filename || deity->filename[0] == '\0' )
   {
      bug( "save_deity: %s has no filename", deity->name );
      return;
   }

   snprintf( filename, 256, "%s%s", DEITY_DIR, deity->filename );

   if( ( fp = fopen( filename, "w" ) ) == NULL )
   {
      bug( "%s", "save_deity: fopen" );
      perror( filename );
   }
   else
   {
      fprintf( fp, "#VERSION %d\n", DEITY_VERSION );  /* Adjani, 1-31-04 */
      fprintf( fp, "%s", "#DEITY\n" );
      fprintf( fp, "Filename		%s~\n", deity->filename );
      fprintf( fp, "Name		%s~\n", deity->name );
      fprintf( fp, "Description	%s~\n", deity->deitydesc );
      fprintf( fp, "Alignment		%d\n", deity->alignment );
      fprintf( fp, "Worshippers	%d\n", deity->worshippers );
      fprintf( fp, "Flee		%d\n", deity->flee );
      fprintf( fp, "Flee_npcrace	%d\n", deity->flee_npcrace );
      fprintf( fp, "Flee_npcrace2   %d\n", deity->flee_npcrace2 );   /* Adjani, 1-24-04 */
      fprintf( fp, "Flee_npcrace3   %d\n", deity->flee_npcrace3 );   /* Adjani, 1-24-04 */
      fprintf( fp, "Flee_npcfoe	%d\n", deity->flee_npcfoe );
      fprintf( fp, "Flee_npcfoe2    %d\n", deity->flee_npcfoe2 ); /* Adjani, 1-24-04 */
      fprintf( fp, "Flee_npcfoe3    %d\n", deity->flee_npcfoe3 ); /* Adjani, 1-24-04 */
      fprintf( fp, "Kill		%d\n", deity->kill );
      fprintf( fp, "Kill_npcrace	%d\n", deity->kill_npcrace );
      fprintf( fp, "Kill_npcrace2   %d\n", deity->kill_npcrace2 );   /* Adjani, 1-24-04 */
      fprintf( fp, "Kill_npcrace3   %d\n", deity->kill_npcrace3 );   /* Adjani, 1-24-04 */
      fprintf( fp, "Kill_npcfoe	%d\n", deity->kill_npcfoe );
      fprintf( fp, "Kill_npcfoe2    %d\n", deity->kill_npcfoe2 ); /* Adjani, 1-24-04 */
      fprintf( fp, "Kill_npcfoe3    %d\n", deity->kill_npcfoe3 ); /* Adjani, 1-24-04 */
      fprintf( fp, "Kill_magic	%d\n", deity->kill_magic );
      fprintf( fp, "Sac		%d\n", deity->sac );
      fprintf( fp, "Bury_corpse	%d\n", deity->bury_corpse );
      fprintf( fp, "Aid_spell		%d\n", deity->aid_spell );
      fprintf( fp, "Aid		%d\n", deity->aid );
      fprintf( fp, "Steal		%d\n", deity->steal );
      fprintf( fp, "Backstab		%d\n", deity->backstab );
      fprintf( fp, "Die		%d\n", deity->die );
      fprintf( fp, "Die_npcrace	%d\n", deity->die_npcrace );
      fprintf( fp, "Die_npcrace2    %d\n", deity->die_npcrace2 ); /* Adjani, 1-24-04 */
      fprintf( fp, "Die_npcrace3    %d\n", deity->die_npcrace3 ); /* Adjani, 1-24-04 */
      fprintf( fp, "Die_npcfoe	%d\n", deity->die_npcfoe );
      fprintf( fp, "Die_npcfoe2     %d\n", deity->die_npcfoe2 );  /* Adjani, 1-24-04 */
      fprintf( fp, "Die_npcfoe3     %d\n", deity->die_npcfoe3 );  /* Adjani, 1-24-04 */
      fprintf( fp, "Spell_aid		%d\n", deity->spell_aid );
      fprintf( fp, "Dig_corpse	%d\n", deity->dig_corpse );
      fprintf( fp, "Scorpse		%d\n", deity->scorpse );
      fprintf( fp, "Savatar		%d\n", deity->savatar );
      fprintf( fp, "Smount		%d\n", deity->smount ); /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Sminion		%d\n", deity->sminion );   /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Sdeityobj		%d\n", deity->sdeityobj );
      fprintf( fp, "Sdeityobj2	%d\n", deity->sdeityobj2 );   /* Added by Tarl 02 Mar 02 */
      fprintf( fp, "Srecall		%d\n", deity->srecall );
      fprintf( fp, "Class           %s~\n", deity->Class == -1 ? "none" : npc_class[deity->Class] );  /* Adjani, 2-18-04 */
      fprintf( fp, "Class2          %s~\n", deity->Class2 == -1 ? "none" : npc_class[deity->Class2] );
      fprintf( fp, "Class3          %s~\n", deity->Class3 == -1 ? "none" : npc_class[deity->Class3] );
      fprintf( fp, "Class4          %s~\n", deity->Class4 == -1 ? "none" : npc_class[deity->Class4] );
      fprintf( fp, "Element		%d\n", deity->element );
      fprintf( fp, "Element2          %d\n", deity->element2 );   /* Added by Tarl 25 Feb 02 */
      fprintf( fp, "Element3          %d\n", deity->element3 );   /* Added by Tarl 25 Feb 02 */
      fprintf( fp, "Sex             %s~\n", deity->sex == -1 ? "none" : npc_sex[deity->sex] );  /* Adjani, 2-18-04 */
      fprintf( fp, "Affected        %s~\n", ext_flag_string( &deity->affected, a_flags ) );
      fprintf( fp, "Affected2       %s~\n", ext_flag_string( &deity->affected2, a_flags ) );
      fprintf( fp, "Affected3       %s~\n", ext_flag_string( &deity->affected3, a_flags ) );
      fprintf( fp, "Suscept		%d\n", deity->suscept );
      fprintf( fp, "Suscept2          %d\n", deity->suscept2 );   /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Suscept3          %d\n", deity->suscept3 );   /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Race            %s~\n", deity->race == -1 ? "none" : npc_race[deity->race] );  /* Adjani, 2-18-04 */
      fprintf( fp, "Race2           %s~\n", deity->race2 == -1 ? "none" : npc_race[deity->race2] );
      fprintf( fp, "Race3           %s~\n", deity->race3 == -1 ? "none" : npc_race[deity->race3] );
      fprintf( fp, "Race4           %s~\n", deity->race4 == -1 ? "none" : npc_race[deity->race4] );
      fprintf( fp, "Npcrace         %s~\n", deity->npcrace == -1 ? "none" : npc_race[deity->npcrace] );  /* Adjani, 2-18-04 */
      fprintf( fp, "Npcrace2        %s~\n", deity->npcrace2 == -1 ? "none" : npc_race[deity->npcrace2] );
      fprintf( fp, "Npcrace3        %s~\n", deity->npcrace3 == -1 ? "none" : npc_race[deity->npcrace3] );
      fprintf( fp, "Npcfoe          %s~\n", deity->npcfoe == -1 ? "none" : npc_race[deity->npcfoe] ); /* Adjani, 2-18-04 */
      fprintf( fp, "Npcfoe2         %s~\n", deity->npcfoe2 == -1 ? "none" : npc_race[deity->npcfoe2] );
      fprintf( fp, "Npcfoe3         %s~\n", deity->npcfoe3 == -1 ? "none" : npc_race[deity->npcfoe3] );
      fprintf( fp, "Susceptnum	%d\n", deity->susceptnum );
      fprintf( fp, "Susceptnum2       %d\n", deity->susceptnum2 );   /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Susceptnum3       %d\n", deity->susceptnum3 );   /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Elementnum        %d\n", deity->elementnum );
      fprintf( fp, "Elementnum2       %d\n", deity->elementnum2 );   /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Elementnum3       %d\n", deity->elementnum3 );   /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Affectednum       %d\n", deity->affectednum );
      fprintf( fp, "Affectednum     %d\n", deity->affectednum2 ); /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Affectednum3	%d\n", deity->affectednum3 ); /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Spell1		%d\n", deity->spell1 ); /* Added by Tarl 24 Mar 02 */
      fprintf( fp, "Spell2		%d\n", deity->spell2 ); /* Added by Tarl 24 Mar 02 */
      fprintf( fp, "Spell3		%d\n", deity->spell3 ); /* Added by Tarl 24 Mar 02 */
      fprintf( fp, "Sspell1         %d\n", deity->sspell1 );   /* Added by Tarl 24 Mar 02 */
      fprintf( fp, "Sspell2         %d\n", deity->sspell2 );   /* Added by Tarl 24 Mar 02 */
      fprintf( fp, "Sspell3         %d\n", deity->sspell3 );   /* Added by Tarl 24 Mar 02 */
      fprintf( fp, "Objstat		%d\n", deity->objstat );
      fprintf( fp, "Recallroom	%d\n", deity->recallroom );   /* Samson */
      fprintf( fp, "Avatar		%d\n", deity->avatar ); /* Restored by Samson */
      fprintf( fp, "Mount			%d\n", deity->mount );  /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Minion		%d\n", deity->minion ); /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Deityobj		%d\n", deity->deityobj );  /* Restored by Samson */
      fprintf( fp, "Deityobj2		%d\n", deity->deityobj2 ); /* Added by Tarl 02 Mar 02 */
      fprintf( fp, "%s", "End\n\n" );
      fprintf( fp, "%s", "#END\n" );
      FCLOSE( fp );
   }
   return;
}

/* Read in actual deity data */

/* Modified to include new recallroom for supplication - Samson 4-13-98 */
void fread_deity( DEITY_DATA * deity, FILE * fp, int filever ) /* Adjani, versions, with help from the ever-patient Xorith. 1-31-04 */
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

         case 'A':
            /*
             * Formatted the code to read a little easier. 
             */
            if( !str_cmp( word, "affected" ) )  /* affected, affected2, affected3 - Adjani 1-31-04 */
            {
               if( filever == 0 )
                  deity->affected = fread_bitvector( fp );
               else
               {
                  char *affectflags = fread_flagstring( fp ), flag[MSL];
                  int value;

                  affectflags = one_argument( affectflags, flag );
                  value = get_aflag( flag );
                  if( value < 0 || value > MAX_BITS )
                     bug( "%s: Unknown affectflag for %s: %s\n\r", __FUNCTION__, word, flag );
                  else
                  {
                     xCLEAR_BITS( deity->affected );
                     xSET_BIT( deity->affected, value );
                  }
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "affected2" ) )
            {
               if( filever == 0 )
                  deity->affected2 = fread_bitvector( fp );
               else
               {
                  char *affectflags = fread_flagstring( fp ), flag[MSL];
                  int value;

                  affectflags = one_argument( affectflags, flag );
                  value = get_aflag( flag );
                  if( value < 0 || value > MAX_BITS )
                     bug( "%s: Unknown affectflag for %s: %s\n\r", __FUNCTION__, word, flag );
                  else
                  {
                     xCLEAR_BITS( deity->affected2 );
                     xSET_BIT( deity->affected2, value );
                  }
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "affected3" ) )
            {
               if( filever == 0 )
                  deity->affected3 = fread_bitvector( fp );
               else
               {
                  char *affectflags = fread_flagstring( fp ), flag[MSL];
                  int value;

                  affectflags = one_argument( affectflags, flag );
                  value = get_aflag( flag );
                  if( value < 0 || value > MAX_BITS )
                     bug( "%s: Unknown affectflag for %s: %s\n\r", __FUNCTION__, word, flag );
                  else
                  {
                     xCLEAR_BITS( deity->affected3 );
                     xSET_BIT( deity->affected3, value );
                  }
               }
               fMatch = TRUE;
               break;
            }
            KEY( "Affectednum", deity->affectednum, fread_number( fp ) );
            KEY( "Affectednum2", deity->affectednum2, fread_number( fp ) );   /* Added by Tarl 24 Feb 02 */
            KEY( "Affectednum3", deity->affectednum3, fread_number( fp ) );   /* Added by Tarl 24 Feb 02 */
            KEY( "Aid", deity->aid, fread_number( fp ) );
            KEY( "Aid_spell", deity->aid_spell, fread_number( fp ) );
            KEY( "Alignment", deity->alignment, fread_number( fp ) );
            KEY( "Avatar", deity->avatar, fread_number( fp ) );   /* Restored by Samson */
            break;

         case 'B':
            KEY( "Backstab", deity->backstab, fread_number( fp ) );
            KEY( "Bury_corpse", deity->bury_corpse, fread_number( fp ) );
            break;

         case 'C':  /* Class, Class2, Class3, Class4 - Adjani 1-31-04 */
            if( !str_cmp( word, "Class" ) )
            {
               int Class;
               if( filever == 0 )
                  Class = fread_number( fp );

               else
               {

                  Class = get_npc_class( fread_flagstring( fp ) );

                  if( Class < -1 || Class >= MAX_NPC_CLASS )
                  {
                     bug( "%s: Deity %s has invalid %s! Defaulting to warrior.", __FUNCTION__, deity->name, word );
                     Class = CLASS_WARRIOR;
                  }

                  deity->Class = Class;
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Class2" ) )
            {
               int Class2;
               if( filever == 0 )
                  Class2 = fread_number( fp );

               else
               {

                  Class2 = get_npc_class( fread_flagstring( fp ) );

                  if( Class2 < -1 || Class2 >= MAX_NPC_CLASS )
                  {
                     bug( "%s: Deity %s has invalid %s! Defaulting to warrior.", __FUNCTION__, deity->name, word );
                     Class2 = CLASS_WARRIOR;
                  }

                  deity->Class2 = Class2;
               }
               fMatch = TRUE;
               break;
            }


            if( !str_cmp( word, "Class3" ) )
            {
               int Class3;
               if( filever == 0 )
                  Class3 = fread_number( fp );

               else
               {

                  Class3 = get_npc_class( fread_flagstring( fp ) );

                  if( Class3 < -1 || Class3 >= MAX_NPC_CLASS )
                  {
                     bug( "%s: Deity %s has invalid %s! Defaulting to warrior.", __FUNCTION__, deity->name, word );
                     Class3 = CLASS_WARRIOR;
                  }

                  deity->Class3 = Class3;
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "Class4" ) )
            {
               int Class4;
               if( filever == 0 )
                  Class4 = fread_number( fp );

               else
               {

                  Class4 = get_npc_class( fread_flagstring( fp ) );

                  if( Class4 < -1 || Class4 >= MAX_NPC_CLASS )
                  {
                     bug( "%s: Deity %s has invalid %s! Defaulting to warrior.", __FUNCTION__, deity->name, word );
                     Class4 = CLASS_WARRIOR;
                  }

                  deity->Class4 = Class4;
               }
               fMatch = TRUE;
               break;
            }

            break;

         case 'D':
            KEY( "Deityobj", deity->deityobj, fread_number( fp ) );  /* Restored by Samson */
            KEY( "Deityobj2", deity->deityobj2, fread_number( fp ) );   /* Added by Tarl 02 Mar 02 */
            KEY( "Description", deity->deitydesc, fread_string_nohash( fp ) );
            KEY( "Die", deity->die, fread_number( fp ) );
            KEY( "Die_npcrace", deity->die_npcrace, fread_number( fp ) );
            KEY( "Die_npcrace2", deity->die_npcrace2, fread_number( fp ) );   /* Adjani, 1-24-04 */
            KEY( "Die_npcrace3", deity->die_npcrace3, fread_number( fp ) );
            KEY( "Die_npcfoe", deity->die_npcfoe, fread_number( fp ) );
            KEY( "Die_npcfoe2", deity->die_npcfoe2, fread_number( fp ) );  /* Adjani, 1-24-04 */
            KEY( "Die_npcfoe3", deity->die_npcfoe3, fread_number( fp ) );
            KEY( "Dig_corpse", deity->dig_corpse, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) ) /* Adjani 1-31-04 */
            {
               if( filever == 0 )
               {
                  deity->die_npcrace2 = 0;
                  deity->die_npcrace3 = 0;
                  deity->die_npcfoe2 = 0;
                  deity->die_npcfoe3 = 0;
                  deity->flee_npcrace2 = 0;
                  deity->flee_npcrace3 = 0;
                  deity->flee_npcfoe2 = 0;
                  deity->flee_npcfoe3 = 0;
                  deity->kill_npcrace2 = 0;
                  deity->kill_npcrace3 = 0;
                  deity->kill_npcfoe2 = 0;
                  deity->kill_npcfoe3 = 0;
                  deity->npcrace2 = 0;
                  deity->npcrace3 = 0;
                  deity->npcfoe2 = 0;
                  deity->npcfoe3 = 0;
               }
               return;
            }

            KEY( "Element", deity->element, fread_number( fp ) );
            KEY( "Element2", deity->element2, fread_number( fp ) );  /* Added by Tarl 24 Feb 02 */
            KEY( "Element3", deity->element3, fread_number( fp ) );  /* Added by Tarl 24 Feb 02 */
            KEY( "Elementnum", deity->elementnum, fread_number( fp ) );
            KEY( "Elementnum2", deity->elementnum2, fread_number( fp ) );  /* Added by Tarl 24 Feb 02 */
            KEY( "Elementnum3", deity->elementnum3, fread_number( fp ) );  /* Added by Tarl 24 Feb 02 */
            break;

         case 'F':
            KEY( "Filename", deity->filename, fread_string_nohash( fp ) );
            KEY( "Flee", deity->flee, fread_number( fp ) );
            KEY( "Flee_npcrace", deity->flee_npcrace, fread_number( fp ) );
            KEY( "Flee_npcrace2", deity->flee_npcrace2, fread_number( fp ) ); /* Adjani, 1-24-04 */
            KEY( "Flee_npcrace3", deity->flee_npcrace3, fread_number( fp ) );
            KEY( "Flee_npcfoe", deity->flee_npcfoe, fread_number( fp ) );
            KEY( "Flee_npcfoe2", deity->flee_npcfoe2, fread_number( fp ) );   /* Adjani, 1-24-04 */
            KEY( "Flee_npcfoe3", deity->flee_npcfoe3, fread_number( fp ) );
            break;

         case 'K':
            KEY( "Kill", deity->kill, fread_number( fp ) );
            KEY( "Kill_npcrace", deity->kill_npcrace, fread_number( fp ) );
            KEY( "Kill_npcrace2", deity->kill_npcrace2, fread_number( fp ) ); /* Adjani, 1-24-04 */
            KEY( "Kill_npcrace3", deity->kill_npcrace3, fread_number( fp ) );
            KEY( "Kill_npcfoe", deity->kill_npcfoe, fread_number( fp ) );
            KEY( "Kill_npcfoe2", deity->kill_npcfoe2, fread_number( fp ) );   /* Adjani, 1-24-04 */
            KEY( "Kill_npcfoe3", deity->kill_npcfoe3, fread_number( fp ) );
            KEY( "Kill_magic", deity->kill_magic, fread_number( fp ) );
            break;

         case 'M':
            KEY( "Minion", deity->minion, fread_number( fp ) );   /* Added by Tarl 24 Feb 02 */
            KEY( "Mount", deity->mount, fread_number( fp ) );  /* Added by Tarl 24 Feb 02 */
            break;

         case 'N':
            KEY( "Name", deity->name, fread_string( fp ) );
            if( !str_cmp( word, "npcfoe" ) ) /* npcfoe, npcfoe2, npcfoe3, npcrace, npcrace2, npcrace3 - Adjani 1-31-04 */
            {
               int npcfoe;

               if( filever == 0 )
                  npcfoe = fread_number( fp );
               else
                  npcfoe = get_npc_race( fread_flagstring( fp ) );

               if( npcfoe < -1 || npcfoe >= MAX_NPC_RACE )
               {
                  bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name, word );
                  npcfoe = RACE_HUMAN;
               }
               deity->npcfoe = npcfoe;
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "npcfoe2" ) )
            {
               int npcfoe2;

               if( filever == 0 )
                  npcfoe2 = fread_number( fp );
               else
                  npcfoe2 = get_npc_race( fread_flagstring( fp ) );
               if( npcfoe2 < -1 || npcfoe2 >= MAX_NPC_RACE )
               {
                  bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name, word );
                  npcfoe2 = RACE_HUMAN;
               }
               deity->npcfoe2 = npcfoe2;
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "npcfoe3" ) )
            {
               int npcfoe3;

               if( filever == 0 )
                  npcfoe3 = fread_number( fp );
               else
                  npcfoe3 = get_npc_race( fread_flagstring( fp ) );
               if( npcfoe3 < -1 || npcfoe3 >= MAX_NPC_RACE )
               {
                  bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name, word );
                  npcfoe3 = RACE_HUMAN;
               }
               deity->npcfoe3 = npcfoe3;
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "npcrace" ) )
            {
               int npcrace;

               if( filever == 0 )
                  npcrace = fread_number( fp );
               else
                  npcrace = get_npc_race( fread_flagstring( fp ) );

               if( npcrace < -1 || npcrace >= MAX_NPC_RACE )
               {
                  bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name, word );
                  npcrace = RACE_HUMAN;
               }
               deity->npcrace = npcrace;
               fMatch = TRUE;
               break;
            }
            if( !str_cmp( word, "npcrace2" ) )
            {
               int npcrace2;

               if( filever == 0 )
                  npcrace2 = fread_number( fp );
               else
                  npcrace2 = get_npc_race( fread_flagstring( fp ) );
               if( npcrace2 < -1 || npcrace2 >= MAX_NPC_RACE )
               {
                  bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name, word );
                  npcrace2 = RACE_HUMAN;
               }
               deity->npcrace2 = npcrace2;
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "npcrace3" ) )
            {
               int npcrace3;

               if( filever == 0 )
                  npcrace3 = fread_number( fp );
               else
                  npcrace3 = get_npc_race( fread_flagstring( fp ) );
               if( npcrace3 < -1 || npcrace3 >= MAX_NPC_RACE )
               {
                  bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name, word );
                  npcrace3 = RACE_HUMAN;
               }
               deity->npcrace3 = npcrace3;
               fMatch = TRUE;
               break;
            }
            break;

         case 'O':
            KEY( "Objstat", deity->objstat, fread_number( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "race" ) )   /* race, race2, race3, race4 - Adjani 1-31-04 */
            {
               int race;
               if( filever == 0 )
                  race = fread_number( fp );
               else
               {
                  race = get_npc_race( fread_flagstring( fp ) );

                  if( race < -1 || race >= MAX_NPC_RACE )
                  {
                     bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name, word );
                     race = RACE_HUMAN;
                  }
                  deity->race = race;
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "race2" ) )
            {
               int race2;
               if( filever == 0 )
                  race2 = fread_number( fp );
               else
               {
                  race2 = get_npc_race( fread_flagstring( fp ) );

                  if( race2 < -1 || race2 >= MAX_NPC_RACE )
                  {
                     bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name, word );
                     race2 = RACE_HUMAN;
                  }
                  deity->race2 = race2;
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "race3" ) )
            {
               int race3;
               if( filever == 0 )
                  race3 = fread_number( fp );
               else
               {
                  race3 = get_npc_race( fread_flagstring( fp ) );

                  if( race3 < -1 || race3 >= MAX_NPC_RACE )
                  {
                     bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name, word );
                     race3 = RACE_HUMAN;
                  }
                  deity->race3 = race3;
               }
               fMatch = TRUE;
               break;
            }

            if( !str_cmp( word, "race4" ) )
            {
               int race4;
               if( filever == 0 )
                  race4 = fread_number( fp );
               else
               {
                  race4 = get_npc_race( fread_flagstring( fp ) );

                  if( race4 < -1 || race4 >= MAX_NPC_RACE )
                  {
                     bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name, word );
                     race4 = RACE_HUMAN;
                  }
                  deity->race4 = race4;
               }
               fMatch = TRUE;
               break;
            }

            KEY( "Recallroom", deity->recallroom, fread_number( fp ) );
            break;

         case 'S':
            KEY( "Sac", deity->sac, fread_number( fp ) );
            KEY( "Savatar", deity->savatar, fread_number( fp ) );
            KEY( "Scorpse", deity->scorpse, fread_number( fp ) );
            KEY( "Sdeityobj", deity->sdeityobj, fread_number( fp ) );
            KEY( "Sdeityobj2", deity->sdeityobj2, fread_number( fp ) ); /* Added by Tarl 02 Mar 02 */
            KEY( "Smount", deity->smount, fread_number( fp ) );   /* Added by Tarl 24 Feb 02 */
            KEY( "Sminion", deity->sminion, fread_number( fp ) ); /* Added by Tarl 24 Feb 02 */
            KEY( "Srecall", deity->srecall, fread_number( fp ) );
            if( !str_cmp( word, "Sex" ) ) /* Adjani, 2-18-04 */
            {
               int sex;
               if( filever == 0 )
                  sex = fread_number( fp );
               else
               {
                  sex = get_npc_sex( fread_flagstring( fp ) );
                  if( sex < -1 || sex >= SEX_MAX )
                  {
                     bug( "%s: Deity %s has invalid %s! Defaulting to neuter.", __FUNCTION__, deity->name, word );
                     sex = SEX_NEUTRAL;
                  }
                  deity->sex = sex;
               }
               fMatch = TRUE;
               break;
            }
            KEY( "Spell_aid", deity->spell_aid, fread_number( fp ) );
            KEY( "Spell1", deity->spell1, fread_number( fp ) );   /* Added by Tarl 24 Mar 02 */
            KEY( "Spell2", deity->spell2, fread_number( fp ) );   /* Added by Tarl 24 Mar 02 */
            KEY( "Spell3", deity->spell3, fread_number( fp ) );   /* Added by Tarl 24 Mar 02 */
            KEY( "Sspell1", deity->sspell1, fread_number( fp ) ); /* Added by Tarl 24 Mar 02 */
            KEY( "Sspell2", deity->sspell2, fread_number( fp ) ); /* Added by Tarl 24 Mar 02 */
            KEY( "Sspell3", deity->sspell3, fread_number( fp ) ); /* Added by Tarl 24 Mar 02 */

            KEY( "Steal", deity->steal, fread_number( fp ) );
            KEY( "Suscept", deity->suscept, fread_number( fp ) );
            KEY( "Suscept2", deity->suscept2, fread_number( fp ) );  /* Added by Tarl 24 Feb 02 */
            KEY( "Suscept3", deity->suscept3, fread_number( fp ) );  /* Added by Tarl 24 Feb 02 */
            KEY( "Susceptnum", deity->susceptnum, fread_number( fp ) );
            KEY( "Susceptnum2", deity->susceptnum2, fread_number( fp ) );  /* Added by Tarl 24 Feb 02 */
            KEY( "Susceptnum3", deity->susceptnum3, fread_number( fp ) );  /* Added by Tarl 24 Feb 02 */
            break;

         case 'W':
            KEY( "Worshippers", deity->worshippers, fread_number( fp ) );
            break;
      }
      if( !fMatch )
         bug( "Fread_deity: no match: %s", word );
   }
}

/* Load a deity file */
bool load_deity_file( const char *deityfile )
{
   char filename[256];
   DEITY_DATA *deity;
   FILE *fp;
   bool found;
   int filever = 0;

   found = FALSE;
   snprintf( filename, 256, "%s%s", DEITY_DIR, deityfile );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
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
            bug( "%s", "Load_deity_file: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "VERSION" ) )   /* Adjani, 1-31-04 */
         {
            filever = fread_number( fp );
            letter = fread_letter( fp );
            if( letter != '#' )
            {
               bug( "%s: # not found after reading file version %d.", __FUNCTION__, filever );
               break;
            }
            word = fread_word( fp );
         }
         if( !str_cmp( word, "DEITY" ) )
         {
            CREATE( deity, DEITY_DATA, 1 );
            fread_deity( deity, fp, filever );  /* Filever added for file versions. Whee! Adjani, 1-31-04 */
            LINK( deity, first_deity, last_deity, next, prev );
            found = TRUE;
            break;
         }
         else
         {
            bug( "Load_deity_file: bad section: %s.", word );
            break;
         }
      }
      FCLOSE( fp );
   }

   return found;
}

/* Load in all the deity files */
void load_deity( void )
{
   FILE *fpList;
   const char *filename;
   char deitylist[256];

   first_deity = NULL;
   last_deity = NULL;

   log_string( "Loading deities..." );

   snprintf( deitylist, 256, "%s%s", DEITY_DIR, DEITY_LIST );
   if( !( fpList = fopen( deitylist, "r" ) ) )
   {
      perror( deitylist );
      exit( 1 );
   }

   for( ;; )
   {
      filename = feof( fpList ) ? "$" : fread_word( fpList );
      if( filename[0] == '$' )
         break;
      if( !load_deity_file( filename ) )
         bug( "Cannot load deity file: %s", filename );
   }
   FCLOSE( fpList );
   log_string( "Done deities" );
   return;
}

/* Function modified from original form, support added for recallroom + avatar & object restored - Samson */
CMDF do_setdeity( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL], arg3[MIL];
   DEITY_DATA *deity;
   int value;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_RESTRICTED:
         send_to_char( "You cannot do this while in another command.\n\r", ch );
         return;

      case SUB_DEITYDESC:
         deity = ( DEITY_DATA * ) ch->pcdata->dest_buf;
         DISPOSE( deity->deitydesc );
         deity->deitydesc = copy_buffer_nohash( ch );
         stop_editing( ch );
         save_deity( deity );
         ch->substate = ch->tempnum;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting description.\n\r", ch );
         return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' )
   {
      send_to_char( "Usage: setdeity <deity> <field> <toggle>\n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( "filename name description alignment worshippers minion sex race race2\n\r", ch );
      send_to_char( "race3 race4 class class2 class3 class4 npcfoe npcfoe2 npcfoe3 npcrace\n\r", ch );
      send_to_char( "npcrace2 npcrace3 element element2 element3 elementnum elementnum2\n\r", ch );
      send_to_char( "elementnum3 affectednum affectednum2 affectednum3 affected affected2\n\r", ch );
      send_to_char( "affected3 susceptnum susceptnum2 susceptnum3 suscept suscept2 suscept3\n\r", ch );
      send_to_char( "recallroom avatar mount object object2 delete\n\r", ch );
      send_to_char( "\n\rFavor adjustments:\n\r", ch );
      send_to_char( "flee flee_npcrace flee_npcrace2 flee_npcrace3 flee_npcfoe flee_npcfoe2\n\r", ch );
      send_to_char( "flee_npcfoe3 kill kill_npcrace kill_npcrace2 kill_npcrace3 kill_npcfoe\n\r", ch );
      send_to_char( "kill_npcfoe2 kill_npcfoe3 kill_magic die die_npcrace die_npcrace2\n\r", ch );
      send_to_char( "die_npcrace3 die_npcfoe die_npcfoe2 die_npcfoe3 dig_corpse bury_corpse\n\r", ch );
      send_to_char( "steal backstab aid aid_spell spell_aid sac\n\r", ch );
      send_to_char( "\n\rFavor requirements for supplicate:\n\r", ch );
      send_to_char( "scorpse savatar smount sdeityobj sdeityobj2 srecall spell1 spell2\n\r", ch );
      send_to_char( "spell3 sspell1 sspell2 sspell3\n\r", ch );   /* Added by Tarl 24 Mar 02 */
      send_to_char( "Objstat - being one of:\n\r", ch );
      send_to_char( "str int wis con dex cha lck\n\r", ch );
      send_to_char( " 0 - 1 - 2 - 3 - 4 - 5 - 6\n\r", ch ); /* Entire list tinkered with by Adjani, 1-27-04 */
      return;
   }

   deity = get_deity( arg1 );
   if( !deity )
   {
      send_to_char( "No such deity.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      char filename[256];

      snprintf( filename, 256, "%s%s", DEITY_DIR, deity->filename );
      free_deity( deity );
      ch_printf( ch, "&YDeity information for %s deleted.\n\r", arg1 );

      if( !remove( filename ) )
         ch_printf( ch, "&RDeity file for %s destroyed.\n\r", arg1 );

      write_deity_list(  );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      DEITY_DATA *udeity;

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "You can't set a deity's name to nothing.\r\n", ch );
         return;
      }
      if( ( udeity = get_deity( argument ) ) )
      {
         send_to_char( "There is already another deity with that name.\r\n", ch );
         return;
      }
      STRFREE( deity->name );
      deity->name = STRALLOC( argument );
      send_to_char( "Done.\r\n", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "avatar" ) )
   {
      deity->avatar = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "mount" ) )
   {
      deity->mount = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "minion" ) )
   {
      deity->minion = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "object" ) )
   {
      deity->deityobj = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "object2" ) )
   {
      deity->deityobj2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "recallroom" ) )
   {
      deity->recallroom = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !is_valid_filename( ch, DEITY_DIR, argument ) )
         return;

      snprintf( filename, 256, "%s%s", DEITY_DIR, deity->filename );
      if( !remove( filename ) )
         send_to_char( "Old deity file deleted.\r\n", ch );
      DISPOSE( deity->filename );
      deity->filename = str_dup( argument );
      send_to_char( "Done.\r\n", ch );
      save_deity( deity );
      write_deity_list( );
      return;
   }

   if( !str_cmp( arg2, "description" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_DEITYDESC;
      ch->pcdata->dest_buf = deity;
      if( !deity->deitydesc || deity->deitydesc[0] == '\0' )
         deity->deitydesc = str_dup( "" );
      start_editing( ch, deity->deitydesc );
      editor_desc_printf( ch, "Deity description for deity '%s'.", deity->name );
      return;
   }

   if( !str_cmp( arg2, "alignment" ) )
   {
      deity->alignment = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee" ) )
   {
      deity->flee = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcrace" ) )
   {
      deity->flee_npcrace = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcrace2" ) )   /* flee_npcrace2, flee_npcrace3, flee_npcfoe2, flee_npcfoe3 - Adjani, 1-27-04 */
   {
      deity->flee_npcrace2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcrace3" ) )
   {
      deity->flee_npcrace3 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcfoe" ) )
   {
      deity->flee_npcfoe = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcfoe2" ) )
   {
      deity->flee_npcfoe2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcfoe3" ) )
   {
      deity->flee_npcfoe3 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill" ) )
   {
      deity->kill = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcrace" ) )
   {
      deity->kill_npcrace = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcrace2" ) )   /* kill_npcrace2, kill_npcrace3, kill_npcfoe2, kill_npcfoe3 - Adjani, 1-24-04 */
   {
      deity->kill_npcrace2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }
   if( !str_cmp( arg2, "kill_npcrace3" ) )
   {
      deity->kill_npcrace3 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcfoe" ) )
   {
      deity->kill_npcfoe = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcfoe2" ) )
   {
      deity->kill_npcfoe2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcfoe3" ) )
   {
      deity->kill_npcfoe3 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_magic" ) )
   {
      deity->kill_magic = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "sac" ) )
   {
      deity->sac = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "bury_corpse" ) )
   {
      deity->bury_corpse = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "aid_spell" ) )
   {
      deity->aid_spell = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "aid" ) )
   {
      deity->aid = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "steal" ) )
   {
      deity->steal = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "backstab" ) )
   {
      deity->backstab = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die" ) )
   {
      deity->die = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcrace" ) )
   {
      deity->die_npcrace = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcrace2" ) ) /* die_npcrace2, die_npcrace3, die_npcfoe2, die_npcfoe3 - Adjani, 1-24-04 */
   {
      deity->die_npcrace2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcrace3" ) )
   {
      deity->die_npcrace3 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcfoe" ) )
   {
      deity->die_npcfoe = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcfoe2" ) )
   {
      deity->die_npcfoe2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcfoe3" ) )
   {
      deity->die_npcfoe3 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "spell_aid" ) )
   {
      deity->spell_aid = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "dig_corpse" ) )
   {
      deity->dig_corpse = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "scorpse" ) )
   {
      deity->scorpse = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "savatar" ) )
   {
      deity->savatar = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "smount" ) )
   {
      deity->smount = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "sminion" ) )
   {
      deity->sminion = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "sdeityobj" ) )
   {
      deity->sdeityobj = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "sdeityobj2" ) )
   {
      deity->sdeityobj2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "objstat" ) )
   {
      deity->objstat = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "srecall" ) )
   {
      deity->srecall = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "worshippers" ) )
   {
      deity->worshippers = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "race" ) )   /* race, race2, race3, race4 - Adjani, 2-18-04 */
   {
      bool fMatch = FALSE;
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->race = -1;
         }
         else
         {
            value = get_pc_race( arg3 );
            if( value < 0 || value > MAX_PC_RACE )
               ch_printf( ch, "Unknown race: %s\n\r", arg3 );
            else
            {
               deity->race = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "race2" ) )
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->race2 = -1;
         }
         else
         {
            value = get_pc_race( arg3 );
            if( value < 0 || value > MAX_PC_RACE )
               ch_printf( ch, "Unknown race2: %s\n\r", arg3 );
            else
            {
               deity->race2 = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "race3" ) )
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->race3 = -1;
         }
         else
         {
            value = get_pc_race( arg3 );
            if( value < 0 || value > MAX_PC_RACE )
               ch_printf( ch, "Unknown race3: %s\n\r", arg3 );
            else
            {
               deity->race3 = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "race4" ) )
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->race4 = -1;
         }
         else
         {
            value = get_pc_race( arg3 );
            if( value < 0 || value > MAX_PC_RACE )
               ch_printf( ch, "Unknown race4: %s\n\r", arg3 );
            else
            {
               deity->race4 = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcrace" ) )   /* npcrace, npcrace2, npcrace3 - Adjani, 2-18-04 */
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->npcrace = -1;
         }
         else
         {
            value = get_npc_race( arg3 );
            if( value < 0 || value > MAX_NPC_RACE )
               ch_printf( ch, "Unknown npcrace: %s\n\r", arg3 );
            else
            {
               deity->npcrace = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcrace2" ) )
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->npcrace2 = -1;
         }
         else
         {
            value = get_npc_race( arg3 );
            if( value < 0 || value > MAX_NPC_RACE )
               ch_printf( ch, "Unknown npcrace2: %s\n\r", arg3 );
            else
            {
               deity->npcrace2 = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcrace3" ) )
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->npcrace3 = -1;
         }
         else
         {
            value = get_npc_race( arg3 );
            if( value < 0 || value > MAX_NPC_RACE )
               ch_printf( ch, "Unknown npcrace3: %s\n\r", arg3 );
            else
            {
               deity->npcrace3 = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcfoe" ) ) /* npcfoe, npcfoe2, npcfoe3 - Adjani, 2-18-04 */
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->npcfoe = -1;
         }
         else
         {
            value = get_npc_race( arg3 );
            if( value < 0 || value > MAX_NPC_RACE )
               ch_printf( ch, "Unknown npcfoe: %s\n\r", arg3 );
            else
            {
               deity->npcfoe = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcfoe2" ) )
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->npcfoe2 = -1;
         }
         else
         {
            value = get_npc_race( arg3 );
            if( value < 0 || value > MAX_NPC_RACE )
               ch_printf( ch, "Unknown npcfoe2: %s\n\r", arg3 );
            else
            {
               deity->npcfoe2 = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcfoe3" ) )
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->npcfoe3 = -1;
         }
         else
         {
            value = get_npc_race( arg3 );
            if( value < 0 || value > MAX_NPC_RACE )
               ch_printf( ch, "Unknown npcfoe3: %s\n\r", arg3 );
            else
            {
               deity->npcfoe3 = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "Class" ) )  /* Class, Class2, Class3, Class4 - Adjani, 2-18-04 */
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->Class = -1;
         }
         else
         {
            value = get_pc_class( arg3 );
            if( value < 0 || value > MAX_PC_CLASS )
               ch_printf( ch, "Unknown Class: %s\n\r", arg3 );
            else
            {
               deity->Class = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "Class2" ) )
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->Class2 = -1;
         }
         else
         {
            value = get_pc_class( arg3 );
            if( value < 0 || value > MAX_PC_CLASS )
               ch_printf( ch, "Unknown Class2: %s\n\r", arg3 );
            else
            {
               deity->Class2 = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "Class3" ) )
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->Class3 = -1;
         }
         else
         {
            value = get_pc_class( arg3 );
            if( value < 0 || value > MAX_PC_CLASS )
               ch_printf( ch, "Unknown Class3: %s\n\r", arg3 );
            else
            {
               deity->Class3 = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "Class4" ) )
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->Class4 = -1;
         }
         else
         {
            value = get_pc_class( arg3 );
            if( value < 0 || value > MAX_PC_CLASS )
               ch_printf( ch, "Unknown Class4: %s\n\r", arg3 );
            else
            {
               deity->Class4 = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "susceptnum" ) )
   {
      deity->susceptnum = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "susceptnum2" ) )  /* Added by Tarl 24 Feb 02 */
   {
      deity->susceptnum2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "susceptnum3" ) )  /* Added by Tarl 24 Feb 02 */
   {
      deity->susceptnum3 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }
   if( !str_cmp( arg2, "elementnum" ) )
   {
      deity->elementnum = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "elementnum2" ) )  /* Added by Tarl 24 Feb 02 */
   {
      deity->elementnum2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "elementnum3" ) )  /* Added by Tarl 24 Feb 02 */
   {
      deity->elementnum3 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "affectednum" ) )
   {
      deity->affectednum = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "affectednum2" ) ) /* Added by Tarl 24 Feb 02 */
   {
      deity->affectednum2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "affectednum3" ) ) /* Added by Tarl 24 Feb 02 */
   {
      deity->affectednum3 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "spell1" ) ) /* Added by Tarl 24 Mar 02 */
   {
      if( skill_lookup( argument ) < 0 )
      {
         send_to_char( "No skill/spell by that name.\n\r", ch );
         return;
      }
      deity->spell1 = skill_lookup( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "spell2" ) ) /* Added by Tarl 24 Mar 02 */
   {
      if( skill_lookup( argument ) < 0 )
      {
         send_to_char( "No skill/spell by that name.\n\r", ch );
         return;
      }
      deity->spell2 = skill_lookup( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "spell3" ) ) /* Added by Tarl 24 Mar 02 */
   {
      if( skill_lookup( argument ) < 0 )
      {
         send_to_char( "No skill/spell by that name.\n\r", ch );
         return;
      }
      deity->spell3 = skill_lookup( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }
   if( !str_cmp( arg2, "Sspell1" ) )   /* Added by Tarl 24 Mar 02 */
   {
      deity->sspell1 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "Sspell2" ) )   /* Added by Tarl 24 Mar 02 */
   {
      deity->sspell2 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "Sspell3" ) )   /* Added by Tarl 24 Mar 02 */
   {
      deity->sspell3 = atoi( argument );
      send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "suscept" ) )   /* suscept, suscept2, suscept3 - Adjani, 2-18-04 */
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->suscept = 0;
         }
         else
         {
            value = get_risflag( arg3 );
            if( value < 0 || value > 31 )
               ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
            else
            {
               deity->suscept = 0;
               SET_BIT( deity->suscept, 1 << value );
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "suscept2" ) )  /* Added by Tarl 24 Feb 02 */
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->suscept2 = 0;
         }
         else
         {
            value = get_risflag( arg3 );
            if( value < 0 || value > 31 )
               ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
            else
            {
               deity->suscept2 = 0;
               TOGGLE_BIT( deity->suscept2, 1 << value );
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "suscept3" ) )  /* Added by Tarl 24 Feb 02 */
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->suscept3 = 0;
         }
         else
         {
            value = get_risflag( arg3 );
            if( value < 0 || value > 31 )
               ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
            else
            {
               deity->suscept3 = 0;
               SET_BIT( deity->suscept3, 1 << value );
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "element" ) )   /* element, element2, element3 - Adjani, 2-18-04 */
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->element = 0;
         }
         else
         {
            value = get_risflag( arg3 );
            if( value < 0 || value > 31 )
               ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
            else
            {
               deity->element = 0;
               SET_BIT( deity->element, 1 << value );
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "element2" ) )  /* Added by Tarl 24 Feb 02 */
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->element2 = 0;
         }
         else
         {
            value = get_risflag( arg3 );
            if( value < 0 || value > 31 )
               ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
            else
            {
               deity->element = 0;
               SET_BIT( deity->element2, 1 << value );
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "element3" ) )  /* Added by Tarl 24 Feb 02 */
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->element3 = 0;
         }
         else
         {
            value = get_risflag( arg3 );
            if( value < 0 || value > 31 )
               ch_printf( ch, "Unknown flag: %s\n\r", arg3 );
            else
            {
               deity->element = 0;
               SET_BIT( deity->element3, 1 << value );
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "sex" ) ) /* Adjani, 2-18-04 */
   {
      bool fMatch = FALSE;

      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         if( !str_cmp( arg3, "none" ) )
         {
            fMatch = TRUE;
            deity->sex = -1;
         }
         else
         {
            value = get_npc_sex( arg3 );
            if( value < 0 || value > SEX_MAX )
               ch_printf( ch, "Unknown sex: %s\n\r", arg3 );
            else
            {
               deity->sex = value;
               fMatch = TRUE;
            }
         }
      }
      if( fMatch )
         send_to_char( "Done.\n\r", ch );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "affected" ) )  /* affected, affected2, affected3 - Adjani, 2-18-04 */
   {
      if( !str_cmp( argument, "none" ) )
      {
         xCLEAR_BITS( deity->affected );
         send_to_char( "Affected cleared.\n\r", ch );
         save_deity( deity );
         return;
      }
      value = get_aflag( argument );
      if( value < 0 || value > MAX_AFFECTED_BY )
         ch_printf( ch, "Unknown flag: %s\n\r", argument );
      else
      {
         xCLEAR_BITS( deity->affected );
         xSET_BIT( deity->affected, value );
         save_deity( deity );
         ch_printf( ch, "Affected '%s' set.\n\r", argument );
      }
      return;
   }

   if( !str_cmp( arg2, "affected2" ) )
   {
      if( !str_cmp( argument, "none" ) )
      {
         xCLEAR_BITS( deity->affected2 );
         send_to_char( "Affected2 cleared.\n\r", ch );
         save_deity( deity );
         return;
      }
      value = get_aflag( argument );
      if( value < 0 || value > MAX_AFFECTED_BY )
         ch_printf( ch, "Unknown flag: %s\n\r", argument );
      else
      {
         xCLEAR_BITS( deity->affected2 );
         xSET_BIT( deity->affected2, value );
         save_deity( deity );
         ch_printf( ch, "Affected2 '%s' set.\n\r", argument );
      }
      return;
   }
   if( !str_cmp( arg2, "affected3" ) )
   {
      if( !str_cmp( argument, "none" ) )
      {
         xCLEAR_BITS( deity->affected3 );
         send_to_char( "Affected3 cleared.\n\r", ch );
         save_deity( deity );
         return;
      }
      value = get_aflag( argument );
      if( value < 0 || value > MAX_AFFECTED_BY )
         ch_printf( ch, "Unknown flag: %s\n\r", argument );
      else
      {
         xCLEAR_BITS( deity->affected3 );
         xSET_BIT( deity->affected3, value );
         save_deity( deity );
         ch_printf( ch, "Affected3 '%s' set.\n\r", argument );
      }
      return;
   }

}

/* Modified to show recallroom, and restored to show avatar + object - Samson 8-1-98 */
/* Regrouped by Adjani 12-03-2002 and again on 1-27-04 and once more on 1-31-04. */
CMDF do_showdeity( CHAR_DATA * ch, char *argument )
{
   DEITY_DATA *deity;
   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }
   if( argument[0] == '\0' )
   {
      send_to_char( "Usage: showdeity <deity>\n\r", ch );
      return;
   }
   deity = get_deity( argument );
   if( !deity )
   {
      send_to_char( "No such deity.\n\r", ch );
      return;
   }
   ch_printf( ch, "\n\rDeity: %-11s Filename: %s \n\r", deity->name, deity->filename );
   ch_printf( ch, "Description:\n\r %s \n\r", deity->deitydesc );
   ch_printf( ch, "Alignment:     %-14d  Sex:   %-14s \n\r", deity->alignment,
              deity->sex == -1 ? "none" : npc_sex[deity->sex] );
   ch_printf( ch, "Races:         %-14s  %-14s  %-14s  %-14s\n\r",
              ( deity->race < 0
                || deity->race > MAX_PC_RACE ) ? "none" : race_table[deity->race]->race_name, ( deity->race2 < 0
                                                                                                || deity->race2 >
                                                                                                MAX_PC_RACE ) ? "none" :
              race_table[deity->race2]->race_name, ( deity->race3 < 0
                                                     || deity->race3 >
                                                     MAX_PC_RACE ) ? "none" : race_table[deity->race3]->race_name,
              ( deity->race4 < 0 || deity->race4 > MAX_PC_RACE ) ? "none" : race_table[deity->race4]->race_name );
   ch_printf( ch, "Classes:       %-14s  %-14s  %-14s  %-14s \n\r",
              ( deity->Class < 0
                || deity->Class > MAX_PC_CLASS ) ? "none" : class_table[deity->Class]->who_name, ( deity->Class2 < 0
                                                                                                   || deity->Class2 >
                                                                                                   MAX_PC_CLASS ) ? "none" :
              class_table[deity->Class2]->who_name, ( deity->Class3 < 0
                                                      || deity->Class3 >
                                                      MAX_PC_CLASS ) ? "none" : class_table[deity->Class3]->who_name,
              ( deity->Class4 < 0 || deity->Class4 > MAX_PC_CLASS ) ? "none" : class_table[deity->Class4]->who_name );
   ch_printf( ch, "Npcraces:      %-14s  %-14s  %-14s\n\r",
              ( deity->npcrace < 0
                || deity->npcrace > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcrace], ( deity->npcrace2 < 0
                                                                                          || deity->npcrace2 >
                                                                                          MAX_NPC_RACE ) ? "none" :
              npc_race[deity->npcrace2], ( deity->npcrace3 < 0
                                           || deity->npcrace3 > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcrace3] );
   ch_printf( ch, "Npcfoes:       %-14s  %-14s  %-14s\n\r",
              ( deity->npcfoe < 0 || deity->npcfoe > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcfoe], ( deity->npcfoe2 < 0
                                                                                                          || deity->npcfoe2 >
                                                                                                          MAX_NPC_RACE ) ?
              "none" : npc_race[deity->npcfoe2], ( deity->npcfoe3 < 0
                                                   || deity->npcfoe3 > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcfoe3] );
   ch_printf( ch, "Affects:       %-14s", !&deity->affected ? "(NONE)" : affect_bit_name( &deity->affected ) );
   ch_printf( ch, "  %-14s", !&deity->affected2 ? "(NONE)" : affect_bit_name( &deity->affected2 ) );
   ch_printf( ch, "  %-14s\n\r", !&deity->affected3 ? "(NONE)" : affect_bit_name( &deity->affected3 ) );
   ch_printf( ch, "Elements:      %-14s", flag_string( deity->element, ris_flags ) );
   ch_printf( ch, "  %-14s", flag_string( deity->element2, ris_flags ) );
   ch_printf( ch, "  %-14s\n\r", flag_string( deity->element3, ris_flags ) );
   ch_printf( ch, "Suscepts:      %-14s", flag_string( deity->suscept, ris_flags ) );
   ch_printf( ch, "  %-14s", flag_string( deity->suscept2, ris_flags ) );
   ch_printf( ch, "  %-14s\n\r", flag_string( deity->suscept3, ris_flags ) );
   ch_printf( ch, "Spells:        %-14s  %-14s  %-14s\n\r\n\r", skill_table[deity->spell1]->name,
              skill_table[deity->spell2]->name, skill_table[deity->spell3]->name );
   ch_printf( ch, "Affectednums:  %-5d %-5d %-7d ", deity->affectednum, deity->affectednum2, deity->affectednum3 );
   ch_printf( ch, "Elementnums:   %-5d %-5d %-5d\n\r", deity->elementnum, deity->elementnum2, deity->elementnum3 );
   ch_printf( ch, "Susceptnums:   %-5d %-5d %-7d ", deity->susceptnum, deity->susceptnum2, deity->susceptnum3 );
   ch_printf( ch, "Sspells:       %-5d %-5d %-5d\n\r", deity->sspell1, deity->sspell2, deity->sspell3 );
   ch_printf( ch, "Flee_npcraces: %-5d %-5d %-7d ", deity->flee_npcrace, deity->flee_npcrace2, deity->flee_npcrace3 );
   ch_printf( ch, "Flee_npcfoes:  %-5d %-5d %-5d \n\r", deity->flee_npcfoe, deity->flee_npcfoe2, deity->flee_npcfoe3 );
   ch_printf( ch, "Kill_npcraces: %-5d %-5d %-7d ", deity->kill_npcrace, deity->kill_npcrace2, deity->kill_npcrace3 );
   ch_printf( ch, "Kill_npcfoes:  %-5d %-5d %-5d \n\r", deity->kill_npcfoe, deity->kill_npcfoe2, deity->kill_npcfoe3 );
   ch_printf( ch, "Die_npcraces:  %-5d %-5d %-7d ", deity->die_npcrace, deity->die_npcrace2, deity->die_npcrace3 );
   ch_printf( ch, "Die_npcfoes:   %-5d %-5d %-5d \n\r\n\r", deity->die_npcfoe, deity->die_npcfoe2, deity->die_npcfoe3 );
   ch_printf( ch, "Kill_magic: %-10d Sac:        %-10d Bury_corpse: %-10d \n\r", deity->kill_magic, deity->sac,
              deity->bury_corpse );
   ch_printf( ch, "Dig_corpse: %-10d Flee:       %-10d Kill:        %-10d \n\r", deity->flee, deity->kill,
              deity->dig_corpse );
   ch_printf( ch, "Die:        %-10d Steal:      %-10d Backstab:    %-10d \n\r", deity->die, deity->steal, deity->backstab );
   ch_printf( ch, "Aid:        %-10d Aid_spell:  %-10d Spell_aid:   %-10d \n\r", deity->aid, deity->aid_spell,
              deity->spell_aid );
   ch_printf( ch, "Object:     %-10d Object2:    %-10d Avatar:      %-10d \n\r", deity->deityobj, deity->deityobj2,
              deity->avatar );
   ch_printf( ch, "Mount:      %-10d Minion:     %-10d Scorpse:     %-10d \n\r", deity->mount, deity->minion,
              deity->scorpse );
   ch_printf( ch, "Savatar:    %-10d Smount:     %-10d Sminion:     %-10d \n\r", deity->savatar, deity->smount,
              deity->sminion );
   ch_printf( ch, "Sdeityobj:  %-10d Sdeityobj2: %-10d Srecall:     %-10d \n\r", deity->sdeityobj, deity->sdeityobj,
              deity->srecall );
   ch_printf( ch, "Recallroom: %-10d Objstat:    %-10d Worshippers: %-10d \n\r", deity->recallroom, deity->objstat,
              deity->worshippers );
   return;
}

CMDF do_makedeity( CHAR_DATA * ch, char *argument )
{
   DEITY_DATA *deity;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Usage: makedeity <deity name>\r\n", ch );
      return;
   }

   smash_tilde( argument );

   if( ( deity = get_deity( argument ) ) )
   {
      send_to_char( "A deity with that name already holds weight on this world.\r\n", ch );
      return;
   }

   CREATE( deity, DEITY_DATA, 1 );
   LINK( deity, first_deity, last_deity, next, prev );
   deity->name = STRALLOC( argument );
   deity->filename = str_dup( strlower( argument ) );
   write_deity_list( );
   save_deity( deity );
   ch_printf( ch, "%s deity has been created\r\n", argument );
   return;
}

CMDF do_devote( CHAR_DATA * ch, char *argument )
{
   DEITY_DATA *deity;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Devote yourself to which deity?\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "none" ) )
   {
      AFFECT_DATA af;
      if( !ch->pcdata->deity )
      {
         send_to_char( "You have already chosen to worship no deities.\n\r", ch );
         return;
      }
      --ch->pcdata->deity->worshippers;
      if( ch->pcdata->deity->worshippers < 0 )
         ch->pcdata->deity->worshippers = 0;
      ch->pcdata->favor = -2500;
      ch->mental_state = -80;
      send_to_char( "A terrible curse afflicts you as you forsake a deity!\n\r", ch );
      xREMOVE_BITS( ch->affected_by, ch->pcdata->deity->affected );
      xREMOVE_BITS( ch->affected_by, ch->pcdata->deity->affected2 );
      xREMOVE_BITS( ch->affected_by, ch->pcdata->deity->affected3 );
      REMOVE_RESIS( ch, ch->pcdata->deity->element );
      REMOVE_RESIS( ch, ch->pcdata->deity->element2 );
      REMOVE_RESIS( ch, ch->pcdata->deity->element3 );
      REMOVE_SUSCEP( ch, ch->pcdata->deity->suscept );
      REMOVE_SUSCEP( ch, ch->pcdata->deity->suscept2 );
      REMOVE_SUSCEP( ch, ch->pcdata->deity->suscept3 );
      affect_strip( ch, gsn_blindness );
      af.type = gsn_blindness;
      af.location = APPLY_HITROLL;
      af.modifier = -4;
      af.duration = ( int )( 50 * DUR_CONV );
      af.bit = AFF_BLIND;
      affect_to_char( ch, &af );
      save_deity( ch->pcdata->deity );
      send_to_char( "You cease to worship any deity.\n\r", ch );
      ch->pcdata->deity = NULL;
      STRFREE( ch->pcdata->deity_name );
      save_char_obj( ch );
      return;
   }

   deity = get_deity( argument );
   if( !deity )
   {
      send_to_char( "No such deity holds weight on this world.\n\r", ch );
      return;
   }

   if( ch->pcdata->deity )
   {
      send_to_char( "You are already devoted to a deity.\n\r", ch );
      return;
   }

   /*
    * Edited by Tarl 25 Feb 02 
    */
   if( ( deity->Class != -1 ) && ( ( deity->Class != ch->Class ) && ( deity->Class2 != ch->Class )
                                   && ( deity->Class3 != ch->Class ) && ( deity->Class4 != ch->Class ) ) )
   {
      send_to_char( "That deity will not accept your worship due to your class.\n\r", ch );
      return;
   }

   if( ( deity->sex != -1 ) && ( deity->sex != ch->sex ) )
   {
      send_to_char( "That deity will not accept worshippers of your sex.\n\r", ch );
      return;
   }

   /*
    * Edited by Tarl 25 Feb 02 
    */
   if( ( deity->race != -1 ) && ( ( deity->race != ch->race ) && ( deity->race2 != ch->race )
                                  && ( deity->race3 != ch->race ) && ( deity->race4 != ch->race ) ) )
   {
      send_to_char( "That deity will not accept worshippers of your race.\n\r", ch );
      return;
   }

   STRFREE( ch->pcdata->deity_name );
   ch->pcdata->deity_name = QUICKLINK( deity->name );
   ch->pcdata->deity = deity;

   act( AT_MAGIC, "Body and soul, you devote yourself to $t!", ch, ch->pcdata->deity_name, NULL, TO_CHAR );
   ++ch->pcdata->deity->worshippers;
   save_deity( ch->pcdata->deity );
   save_char_obj( ch );
   return;
}

CMDF do_deities( CHAR_DATA * ch, char *argument )
{
   DEITY_DATA *deity;
   int count = 0;

   if( !argument || argument[0] == '\0' )
   {
      send_to_pager( "&gFor detailed information on a deity, try 'deities <deity>' or 'help deities'\n\r", ch );
      send_to_pager( "Deity			Worshippers\n\r", ch );
      for( deity = first_deity; deity; deity = deity->next )
      {
         pager_printf( ch, "&G%-14s	&g%19d\n\r", deity->name, deity->worshippers );
         count++;
      }
      if( !count )
      {
         send_to_pager( "&gThere are no deities on this world.\n\r", ch );
         return;
      }
      return;
   }

   deity = get_deity( argument );
   if( !deity )
   {
      send_to_pager( "&gThat deity does not exist.\n\r", ch );
      return;
   }

   pager_printf( ch, "&gDeity:        &G%s\n\r", deity->name );
   pager_printf( ch, "&gDescription:\n\r&G%s", deity->deitydesc );
   return;
}

/*
Internal function to adjust favor.
Fields are:
0 = flee		5 = sac			10 = backstab	
1 = flee_npcrace	6 = bury_corpse		11 = die
2 = kill		7 = aid_spell		12 = die_npcrace
3 = kill_npcrace	8 = aid			13 = spell_aid
4 = kill_magic		9 = steal		14 = dig_corpse
15 = die_npcfoe	       16 = flee_npcfoe         17 = kill_npcfoe
*/
void adjust_favor( CHAR_DATA * ch, int field, int mod )
{
   int oldfavor;

   if( IS_NPC( ch ) || !ch->pcdata->deity )
      return;

   oldfavor = ch->pcdata->favor;

   if( ( ch->alignment - ch->pcdata->deity->alignment > 650
         || ch->alignment - ch->pcdata->deity->alignment < -650 ) && ch->pcdata->deity->alignment != 0 )
   {
      ch->pcdata->favor -= 2;
      ch->pcdata->favor = URANGE( -2500, ch->pcdata->favor, 5000 );

      if( ch->pcdata->favor > ch->pcdata->deity->affectednum )
      {
         if( !xIS_EMPTY( ch->pcdata->deity->affected ) && !xHAS_BITS( ch->affected_by, ch->pcdata->deity->affected ) )
         {
            ch_printf( ch, "%s looks favorably upon you and bestows %s as a reward.\n\r",
                       ch->pcdata->deity_name, ext_flag_string( &ch->pcdata->deity->affected, a_flags ) );
            xSET_BITS( ch->affected_by, ch->pcdata->deity->affected );
         }
      }
      if( ch->pcdata->favor > ch->pcdata->deity->affectednum2 )
      {
         if( !xIS_EMPTY( ch->pcdata->deity->affected2 ) && !xHAS_BITS( ch->affected_by, ch->pcdata->deity->affected2 ) )
         {
            ch_printf( ch, "%s looks favorably upon you and bestows %s as a reward.\n\r",
                       ch->pcdata->deity_name, ext_flag_string( &ch->pcdata->deity->affected2, a_flags ) );
            xSET_BITS( ch->affected_by, ch->pcdata->deity->affected2 );
         }
      }
      if( ch->pcdata->favor > ch->pcdata->deity->affectednum3 )
      {
         if( !xIS_EMPTY( ch->pcdata->deity->affected3 ) && !xHAS_BITS( ch->affected_by, ch->pcdata->deity->affected3 ) )
         {
            ch_printf( ch, "%s looks favorably upon you and bestows %s as a reward.\n\r",
                       ch->pcdata->deity_name, ext_flag_string( &ch->pcdata->deity->affected3, a_flags ) );
            xSET_BITS( ch->affected_by, ch->pcdata->deity->affected3 );
         }
      }
      if( ch->pcdata->favor > ch->pcdata->deity->elementnum )
      {
         if( ch->pcdata->deity->element != 0 && !IS_RESIS( ch, ch->pcdata->deity->element ) )
         {
            SET_RESIS( ch, ch->pcdata->deity->element );
            ch_printf( ch, "%s looks favorably upon you and bestows %s resistance upon you.\n\r",
                       ch->pcdata->deity_name, flag_string( ch->pcdata->deity->element, ris_flags ) );
         }
      }
      if( ch->pcdata->favor > ch->pcdata->deity->elementnum2 )
      {
         if( ch->pcdata->deity->element2 != 0 && !IS_RESIS( ch, ch->pcdata->deity->element2 ) )
         {
            SET_RESIS( ch, ch->pcdata->deity->element2 );
            ch_printf( ch, "%s looks favorably upon you and bestows %s resistance upon you.\n\r",
                       ch->pcdata->deity_name, flag_string( ch->pcdata->deity->element2, ris_flags ) );
         }
      }
      if( ch->pcdata->favor > ch->pcdata->deity->elementnum3 )
      {
         if( ch->pcdata->deity->element3 != 0 && !IS_RESIS( ch, ch->pcdata->deity->element3 ) )
         {
            SET_RESIS( ch, ch->pcdata->deity->element3 );
            ch_printf( ch, "%s looks favorably upon you and bestows %s resistance upon you.\n\r",
                       ch->pcdata->deity_name, flag_string( ch->pcdata->deity->element3, ris_flags ) );
         }
      }
      if( ch->pcdata->favor < ch->pcdata->deity->susceptnum )
      {
         if( ch->pcdata->deity->suscept != 0 && !IS_SUSCEP( ch, ch->pcdata->deity->suscept ) )
         {
            SET_SUSCEP( ch, ch->pcdata->deity->suscept );
            ch_printf( ch, "%s looks poorly upon you and makes you more vulnerable to %s as punishment.\n\r",
                       ch->pcdata->deity_name, flag_string( ch->pcdata->deity->suscept, ris_flags ) );
         }
      }
      if( ch->pcdata->favor < ch->pcdata->deity->susceptnum2 )
      {
         if( ch->pcdata->deity->suscept2 != 0 && !IS_SUSCEP( ch, ch->pcdata->deity->suscept2 ) )
         {
            SET_SUSCEP( ch, ch->pcdata->deity->suscept2 );
            ch_printf( ch, "%s looks poorly upon you and makes you more vulnerable to %s as punishment.\n\r",
                       ch->pcdata->deity_name, flag_string( ch->pcdata->deity->suscept2, ris_flags ) );
         }
      }
      if( ch->pcdata->favor < ch->pcdata->deity->susceptnum3 )
      {
         if( ch->pcdata->deity->suscept3 != 0 && !IS_SUSCEP( ch, ch->pcdata->deity->suscept3 ) )
         {
            SET_SUSCEP( ch, ch->pcdata->deity->suscept3 );
            ch_printf( ch, "%s looks poorly upon you and makes you more vulnerable to %s as punishment.\n\r",
                       ch->pcdata->deity_name, flag_string( ch->pcdata->deity->suscept3, ris_flags ) );
         }
      }

      if( ( oldfavor > ch->pcdata->deity->affectednum && ch->pcdata->favor <= ch->pcdata->deity->affectednum )
          || ( oldfavor > ch->pcdata->deity->affectednum2 && ch->pcdata->favor <= ch->pcdata->deity->affectednum2 )
          || ( oldfavor > ch->pcdata->deity->affectednum3 && ch->pcdata->favor <= ch->pcdata->deity->affectednum3 )
          || ( oldfavor > ch->pcdata->deity->elementnum && ch->pcdata->favor <= ch->pcdata->deity->elementnum )
          || ( oldfavor > ch->pcdata->deity->elementnum2 && ch->pcdata->favor <= ch->pcdata->deity->elementnum2 )
          || ( oldfavor > ch->pcdata->deity->elementnum3 && ch->pcdata->favor <= ch->pcdata->deity->elementnum3 )
          || ( oldfavor < ch->pcdata->deity->susceptnum && ch->pcdata->favor >= ch->pcdata->deity->susceptnum )
          || ( oldfavor < ch->pcdata->deity->susceptnum2 && ch->pcdata->favor >= ch->pcdata->deity->susceptnum2 )
          || ( oldfavor < ch->pcdata->deity->susceptnum3 && ch->pcdata->favor >= ch->pcdata->deity->susceptnum3 ) )
      {
         update_aris( ch );
      }

      /*
       * Added by Tarl 24 Mar 02 
       */
      if( ( oldfavor > ch->pcdata->deity->sspell1 ) && ( ch->pcdata->favor <= ch->pcdata->deity->sspell1 ) )
      {
         if( ch->level < skill_table[ch->pcdata->deity->spell1]->skill_level[ch->Class]
             && ch->level < skill_table[ch->pcdata->deity->spell1]->race_level[ch->race] )
         {
            ch->pcdata->learned[ch->pcdata->deity->spell1] = 0;
         }
      }

      if( ( oldfavor > ch->pcdata->deity->sspell2 ) && ( ch->pcdata->favor <= ch->pcdata->deity->sspell2 ) )
      {
         if( ch->level < skill_table[ch->pcdata->deity->spell2]->skill_level[ch->Class]
             && ch->level < skill_table[ch->pcdata->deity->spell2]->race_level[ch->race] )
         {
            ch->pcdata->learned[ch->pcdata->deity->spell2] = 0;
         }
      }

      if( ( oldfavor > ch->pcdata->deity->sspell3 ) && ( ch->pcdata->favor <= ch->pcdata->deity->sspell3 ) )
      {
         if( ch->level < skill_table[ch->pcdata->deity->spell3]->skill_level[ch->Class]
             && ch->level < skill_table[ch->pcdata->deity->spell3]->race_level[ch->race] )
         {
            ch->pcdata->learned[ch->pcdata->deity->spell3] = 0;
         }
      }
      return;
   }

   if( mod < 1 )
      mod = 1;
   switch ( field )
   {
      default:
         break;

      case 0:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->flee / mod );
         break;
      case 1:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->flee_npcrace / mod );
         break;
      case 2:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->kill / mod );
         break;
      case 3:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->kill_npcrace / mod );
         break;
      case 4:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->kill_magic / mod );
         break;
      case 5:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->sac / mod );
         break;
      case 6:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->bury_corpse / mod );
         break;
      case 7:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->aid_spell / mod );
         break;
      case 8:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->aid / mod );
         break;
      case 9:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->steal / mod );
         break;
      case 10:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->backstab / mod );
         break;
      case 11:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->die / mod );
         break;
      case 12:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->die_npcrace / mod );
         break;
      case 13:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->spell_aid / mod );
         break;
      case 14:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->dig_corpse / mod );
         break;
      case 15:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->die_npcfoe / mod );
         break;
      case 16:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->flee_npcfoe / mod );
         break;
      case 17:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->kill_npcfoe / mod );
         break;
      case 18:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->flee_npcrace2 / mod );
         break;
      case 19:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->flee_npcrace3 / mod );
         break;
      case 20:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->flee_npcfoe2 / mod );
         break;
      case 21:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->flee_npcfoe3 / mod );
         break;
      case 22:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->kill_npcrace2 / mod );
         break;
      case 23:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->kill_npcrace3 / mod );
         break;
      case 24:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->kill_npcfoe2 / mod );
         break;
      case 25:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->kill_npcfoe3 / mod );
         break;
      case 26:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->die_npcrace2 / mod );
         break;
      case 27:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->die_npcrace3 / mod );
         break;
      case 28:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->die_npcfoe2 / mod );
         break;
      case 29:
         ch->pcdata->favor += number_fuzzy( ch->pcdata->deity->die_npcfoe3 / mod );
         break;


   }
   ch->pcdata->favor = URANGE( -2500, ch->pcdata->favor, 5000 );

   if( ch->pcdata->favor > ch->pcdata->deity->affectednum )
   {
      if( !xIS_EMPTY( ch->pcdata->deity->affected ) && !xHAS_BITS( ch->affected_by, ch->pcdata->deity->affected ) )
      {
         ch_printf( ch, "%s looks favorably upon you and bestows %s as a reward.\n\r",
                    ch->pcdata->deity_name, ext_flag_string( &ch->pcdata->deity->affected, a_flags ) );
         xSET_BITS( ch->affected_by, ch->pcdata->deity->affected );
      }
   }
   if( ch->pcdata->favor > ch->pcdata->deity->affectednum2 )
   {
      if( !xIS_EMPTY( ch->pcdata->deity->affected2 ) && !xHAS_BITS( ch->affected_by, ch->pcdata->deity->affected2 ) )
      {
         ch_printf( ch, "%s looks favorably upon you and bestows %s as a reward.\n\r",
                    ch->pcdata->deity_name, ext_flag_string( &ch->pcdata->deity->affected2, a_flags ) );
         xSET_BITS( ch->affected_by, ch->pcdata->deity->affected2 );
      }
   }
   if( ch->pcdata->favor > ch->pcdata->deity->affectednum3 )
   {
      if( !xIS_EMPTY( ch->pcdata->deity->affected3 ) && !xHAS_BITS( ch->affected_by, ch->pcdata->deity->affected3 ) )
      {
         ch_printf( ch, "%s looks favorably upon you and bestows %s as a reward.\n\r",
                    ch->pcdata->deity_name, ext_flag_string( &ch->pcdata->deity->affected3, a_flags ) );
         xSET_BITS( ch->affected_by, ch->pcdata->deity->affected3 );
      }
   }
   if( ch->pcdata->favor > ch->pcdata->deity->elementnum )
   {
      if( ch->pcdata->deity->element != 0 && !IS_RESIS( ch, ch->pcdata->deity->element ) )
      {
         SET_RESIS( ch, ch->pcdata->deity->element );
         ch_printf( ch, "%s looks favorably upon you and bestows %s resistance upon you.\n\r",
                    ch->pcdata->deity_name, flag_string( ch->pcdata->deity->element, ris_flags ) );
      }
   }
   if( ch->pcdata->favor > ch->pcdata->deity->elementnum2 )
   {
      if( ch->pcdata->deity->element2 != 0 && !IS_RESIS( ch, ch->pcdata->deity->element2 ) )
      {
         SET_RESIS( ch, ch->pcdata->deity->element2 );
         ch_printf( ch, "%s looks favorably upon you and bestows %s resistance upon you.\n\r",
                    ch->pcdata->deity_name, flag_string( ch->pcdata->deity->element2, ris_flags ) );
      }
   }
   if( ch->pcdata->favor > ch->pcdata->deity->elementnum3 )
   {
      if( ch->pcdata->deity->element3 != 0 && !IS_RESIS( ch, ch->pcdata->deity->element3 ) )
      {
         SET_RESIS( ch, ch->pcdata->deity->element3 );
         ch_printf( ch, "%s looks favorably upon you and bestows %s resistance upon you.\n\r",
                    ch->pcdata->deity_name, flag_string( ch->pcdata->deity->element3, ris_flags ) );
      }
   }
   if( ch->pcdata->favor < ch->pcdata->deity->susceptnum )
   {
      if( ch->pcdata->deity->suscept != 0 && !IS_SUSCEP( ch, ch->pcdata->deity->suscept ) )
      {
         SET_SUSCEP( ch, ch->pcdata->deity->suscept );
         ch_printf( ch, "%s looks poorly upon you and makes you more vulnerable to %s as punishment.\n\r",
                    ch->pcdata->deity_name, flag_string( ch->pcdata->deity->suscept, ris_flags ) );
      }
   }
   if( ch->pcdata->favor < ch->pcdata->deity->susceptnum2 )
   {
      if( ch->pcdata->deity->suscept2 != 0 && !IS_SUSCEP( ch, ch->pcdata->deity->suscept2 ) )
      {
         SET_SUSCEP( ch, ch->pcdata->deity->suscept2 );
         ch_printf( ch, "%s looks poorly upon you and makes you more vulnerable to %s as punishment.\n\r",
                    ch->pcdata->deity_name, flag_string( ch->pcdata->deity->suscept2, ris_flags ) );
      }
   }
   if( ch->pcdata->favor < ch->pcdata->deity->susceptnum3 )
   {
      if( ch->pcdata->deity->suscept3 != 0 && !IS_SUSCEP( ch, ch->pcdata->deity->suscept3 ) )
      {
         SET_SUSCEP( ch, ch->pcdata->deity->suscept3 );
         ch_printf( ch, "%s looks poorly upon you and makes you more vulnerable to %s as punishment.\n\r",
                    ch->pcdata->deity_name, flag_string( ch->pcdata->deity->suscept3, ris_flags ) );
      }
   }
   /*
    * Added by Tarl 24 Mar 02 
    */
   if( ch->pcdata->favor > ch->pcdata->deity->sspell1 )
   {
      if( ch->pcdata->deity->spell1 != 0 )
      {
         if( ch->pcdata->learned[ch->pcdata->deity->spell1] != 0 )
            return;
         ch->pcdata->learned[ch->pcdata->deity->spell1] = 95;
      }
   }

   if( ch->pcdata->favor > ch->pcdata->deity->sspell2 )
   {
      if( ch->pcdata->deity->spell2 != 0 )
      {
         if( ch->pcdata->learned[ch->pcdata->deity->spell2] != 0 )
            return;
         ch->pcdata->learned[ch->pcdata->deity->spell2] = 95;
      }
   }

   if( ch->pcdata->favor > ch->pcdata->deity->sspell3 )
   {
      if( ch->pcdata->deity->spell3 != 0 )
      {
         if( ch->pcdata->learned[ch->pcdata->deity->spell3] != 0 )
            return;
         ch->pcdata->learned[ch->pcdata->deity->spell3] = 95;
      }
   }

   /*
    * If favor crosses over the line then strip the affect 
    */
   if( ( oldfavor > ch->pcdata->deity->affectednum && ch->pcdata->favor <= ch->pcdata->deity->affectednum )
       || ( oldfavor > ch->pcdata->deity->affectednum2 && ch->pcdata->favor <= ch->pcdata->deity->affectednum2 )
       || ( oldfavor > ch->pcdata->deity->affectednum3 && ch->pcdata->favor <= ch->pcdata->deity->affectednum3 )
       || ( oldfavor > ch->pcdata->deity->elementnum && ch->pcdata->favor <= ch->pcdata->deity->elementnum )
       || ( oldfavor > ch->pcdata->deity->elementnum2 && ch->pcdata->favor <= ch->pcdata->deity->elementnum2 )
       || ( oldfavor > ch->pcdata->deity->elementnum3 && ch->pcdata->favor <= ch->pcdata->deity->elementnum3 )
       || ( oldfavor < ch->pcdata->deity->susceptnum && ch->pcdata->favor >= ch->pcdata->deity->susceptnum )
       || ( oldfavor < ch->pcdata->deity->susceptnum2 && ch->pcdata->favor >= ch->pcdata->deity->susceptnum2 )
       || ( oldfavor < ch->pcdata->deity->susceptnum3 && ch->pcdata->favor >= ch->pcdata->deity->susceptnum3 ) )
   {
      update_aris( ch );
   }

   /*
    * Added by Tarl 24 Mar 02 
    */
   if( ( oldfavor > ch->pcdata->deity->sspell1 ) && ( ch->pcdata->favor <= ch->pcdata->deity->sspell1 ) )
   {
      if( ch->level < skill_table[ch->pcdata->deity->spell1]->skill_level[ch->Class]
          && ch->level < skill_table[ch->pcdata->deity->spell1]->race_level[ch->race] )
      {
         ch->pcdata->learned[ch->pcdata->deity->spell1] = 0;
      }
   }

   if( ( oldfavor > ch->pcdata->deity->sspell2 ) && ( ch->pcdata->favor <= ch->pcdata->deity->sspell2 ) )
   {
      if( ch->level < skill_table[ch->pcdata->deity->spell2]->skill_level[ch->Class]
          && ch->level < skill_table[ch->pcdata->deity->spell2]->race_level[ch->race] )
      {
         ch->pcdata->learned[ch->pcdata->deity->spell2] = 0;
      }
   }

   if( ( oldfavor > ch->pcdata->deity->sspell3 ) && ( ch->pcdata->favor <= ch->pcdata->deity->sspell3 ) )
   {
      if( ch->level < skill_table[ch->pcdata->deity->spell3]->skill_level[ch->Class]
          && ch->level < skill_table[ch->pcdata->deity->spell3]->race_level[ch->race] )
      {
         ch->pcdata->learned[ch->pcdata->deity->spell3] = 0;
      }
   }
   return;
}

/* Modified to support new contintental/planar recall points - Samson 4-13-98 */
CMDF do_supplicate( CHAR_DATA * ch, char *argument )
{
   int oldfavor;

   if( IS_NPC( ch ) || !ch->pcdata->deity )
   {
      send_to_char( "You have no deity to supplicate to.\n\r", ch );
      return;
   }

   oldfavor = ch->pcdata->favor;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Supplicate for what?\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "corpse" ) )
   {
      char buf2[MSL], buf3[MSL];
      OBJ_DATA *obj;
      bool found;

      if( ch->pcdata->favor < ch->pcdata->deity->scorpse )
      {
         send_to_char( "You are not favored enough for a corpse retrieval.\n\r", ch );
         return;
      }

      if( IS_ROOM_FLAG( ch->in_room, ROOM_CLANSTOREROOM ) )
      {
         send_to_char( "You cannot supplicate in a storage room.\n\r", ch );
         return;
      }

      found = FALSE;
      mudstrlcpy( buf3, " ", MSL );
      snprintf( buf2, MSL, "the corpse of %s", ch->name );
      for( obj = first_object; obj; obj = obj->next )
      {
         if( obj->in_room && !str_cmp( buf2, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
         {
            found = TRUE;
            if( IS_ROOM_FLAG( obj->in_room, ROOM_NOSUPPLICATE ) )
            {
               act( AT_MAGIC, "The image of your corpse appears, but suddenly wavers away.", ch, NULL, NULL, TO_CHAR );
               return;
            }
            act( AT_MAGIC, "Your corpse appears suddenly, surrounded by a divine presence...", ch, NULL, NULL, TO_CHAR );
            act( AT_MAGIC, "$n's corpse appears suddenly, surrounded by a divine force...", ch, NULL, NULL, TO_ROOM );
            obj_from_room( obj );
            obj = obj_to_room( obj, ch->in_room, ch );
         }
      }
      if( !found )
      {
         send_to_char( "No corpse of yours litters the world...\n\r", ch );
         return;
      }
      ch->pcdata->favor -= ch->pcdata->deity->scorpse;
      adjust_favor( ch, -1, 1 );
      return;
   }

   if( !str_cmp( argument, "avatar" ) )
   {
      MOB_INDEX_DATA *pMobIndex;
      CHAR_DATA *victim;

      if( ch->pcdata->favor < ch->pcdata->deity->savatar )
      {
         send_to_char( "You are not favored enough for that.\n\r", ch );
         return;
      }

      pMobIndex = get_mob_index( ch->pcdata->deity->avatar );
      if( pMobIndex == NULL )
      {
         send_to_char( "Your deity has no avatar to pray for.\n\r", ch );
         return;
      }

      if( check_pets( ch, pMobIndex ) )
      {
         send_to_char( "You have already received an avatar of your deity!\n\r", ch );
         return;
      }

      victim = create_mobile( pMobIndex );
      if( !char_to_room( victim, ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      fix_maps( ch, victim );

      act( AT_MAGIC, "$n summons a powerful avatar!", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You summon a powerful avatar!", ch, NULL, NULL, TO_CHAR );
      bind_follower( victim, ch, gsn_charm_person, ( ch->pcdata->favor / 4 ) + 1 );
      victim->level = LEVEL_AVATAR / 2;
      victim->hit = ch->hit + ch->pcdata->favor;
      victim->alignment = ch->pcdata->deity->alignment;
      victim->max_hit = ch->hit + ch->pcdata->favor;

      ch->pcdata->favor -= ch->pcdata->deity->savatar;
      adjust_favor( ch, -1, 1 );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( argument, "mount" ) )
   {
      MOB_INDEX_DATA *pMobIndex;
      CHAR_DATA *victim;

      if( ch->pcdata->favor < ch->pcdata->deity->smount )
      {
         send_to_char( "You are not favored enough for that.\n\r", ch );
         return;
      }

      pMobIndex = get_mob_index( ch->pcdata->deity->mount );
      if( pMobIndex == NULL )
      {
         send_to_char( "Your deity has no mount to pray for.\n\r", ch );
         return;
      }

      if( check_pets( ch, pMobIndex ) )
      {
         send_to_char( "You have already received a mount of your deity!\n\r", ch );
         return;
      }

      victim = create_mobile( pMobIndex );
      if( !char_to_room( victim, ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      fix_maps( ch, victim );

      act( AT_MAGIC, "$n summons a mount!", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You summon a mount!", ch, NULL, NULL, TO_CHAR );
      bind_follower( victim, ch, gsn_charm_person, ( ch->pcdata->favor / 4 ) + 1 );
      ch->pcdata->favor -= ch->pcdata->deity->smount;
      adjust_favor( ch, -1, 1 );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( argument, "minion" ) )
   {
      MOB_INDEX_DATA *pMobIndex;
      CHAR_DATA *victim;

      if( ch->pcdata->favor < ch->pcdata->deity->sminion )
      {
         send_to_char( "You are not favored enough for that.\n\r", ch );
         return;
      }

      pMobIndex = get_mob_index( ch->pcdata->deity->minion );
      if( pMobIndex == NULL )
      {
         send_to_char( "Your deity has no minion to pray for.\n\r", ch );
         return;
      }

      if( check_pets( ch, pMobIndex ) )
      {
         send_to_char( "You have already received a minion of your deity!\n\r", ch );
         return;
      }

      victim = create_mobile( pMobIndex );
      if( !char_to_room( victim, ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      fix_maps( ch, victim );

      act( AT_MAGIC, "$n summons a minion!", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You summon a minion!", ch, NULL, NULL, TO_CHAR );
      bind_follower( victim, ch, gsn_charm_person, ( ch->pcdata->favor / 4 ) + 1 );
      victim->level = LEVEL_AVATAR / 3;
      victim->hit = ( ch->hit / 2 ) + ( ch->pcdata->favor / 4 );
      victim->alignment = ch->pcdata->deity->alignment;
      victim->max_hit = ( ch->hit / 2 ) + ( ch->pcdata->favor / 4 );

      ch->pcdata->favor -= ch->pcdata->deity->sminion;
      adjust_favor( ch, -1, 1 );
      return;
   }

   if( !str_cmp( argument, "object" ) )
   {
      OBJ_DATA *obj;
      OBJ_INDEX_DATA *pObjIndex;
      AFFECT_DATA *paf;

      if( ch->pcdata->favor < ch->pcdata->deity->sdeityobj )
      {
         send_to_char( "You are not favored enough for that.\n\r", ch );
         return;
      }

      pObjIndex = get_obj_index( ch->pcdata->deity->deityobj );
      if( pObjIndex == NULL )
      {
         send_to_char( "Your deity has no object to pray for.\n\r", ch );
         return;
      }

      if( !( obj = create_object( pObjIndex, ch->level ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      if( CAN_WEAR( obj, ITEM_TAKE ) )
         obj = obj_to_char( obj, ch );
      else
         obj = obj_to_room( obj, ch->in_room, ch );

      act( AT_MAGIC, "$n weaves $p from divine matter!", ch, obj, NULL, TO_ROOM );
      act( AT_MAGIC, "You weave $p from divine matter!", ch, obj, NULL, TO_CHAR );
      ch->pcdata->favor -= ch->pcdata->deity->sdeityobj;
      adjust_favor( ch, -1, 1 );

      CREATE( paf, AFFECT_DATA, 1 );
      paf->type = -1;
      paf->duration = -1;
      xCLEAR_BITS( paf->rismod );
      switch ( ch->pcdata->deity->objstat )
      {
         case 0:
            paf->location = APPLY_STR;
            break;
         case 1:
            paf->location = APPLY_INT;
            break;
         case 2:
            paf->location = APPLY_WIS;
            break;
         case 3:
            paf->location = APPLY_CON;
            break;
         case 4:
            paf->location = APPLY_DEX;
            break;
         case 5:
            paf->location = APPLY_CHA;
            break;
         case 6:
            paf->location = APPLY_LCK;
            break;
      }
      paf->modifier = 1;
      paf->bit = 0;
      LINK( paf, obj->first_affect, obj->last_affect, next, prev );
      return;
   }

   if( !str_cmp( argument, "object2" ) )
   {
      OBJ_DATA *obj;
      OBJ_INDEX_DATA *pObjIndex;
      AFFECT_DATA *paf;

      if( ch->pcdata->favor < ch->pcdata->deity->sdeityobj2 )
      {
         send_to_char( "You are not favored enough for that.\n\r", ch );
         return;
      }

      pObjIndex = get_obj_index( ch->pcdata->deity->deityobj2 );
      if( pObjIndex == NULL )
      {
         send_to_char( "Your deity has no object to pray for.\n\r", ch );
         return;
      }

      if( !( obj = create_object( pObjIndex, ch->level ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      if( CAN_WEAR( obj, ITEM_TAKE ) )
         obj = obj_to_char( obj, ch );
      else
         obj = obj_to_room( obj, ch->in_room, ch );

      stralloc_printf( &obj->name, "sigil %s", ch->pcdata->deity->name );

      act( AT_MAGIC, "$n weaves $p from divine matter!", ch, obj, NULL, TO_ROOM );
      act( AT_MAGIC, "You weave $p from divine matter!", ch, obj, NULL, TO_CHAR );
      ch->pcdata->favor -= ch->pcdata->deity->sdeityobj2;
      adjust_favor( ch, -1, 1 );

      CREATE( paf, AFFECT_DATA, 1 );
      paf->type = -1;
      paf->duration = -1;
      xCLEAR_BITS( paf->rismod );
      switch ( ch->pcdata->deity->objstat )
      {
         case 0:
            paf->location = APPLY_STR;
            break;
         case 1:
            paf->location = APPLY_INT;
            break;
         case 2:
            paf->location = APPLY_WIS;
            break;
         case 3:
            paf->location = APPLY_CON;
            break;
         case 4:
            paf->location = APPLY_DEX;
            break;
         case 5:
            paf->location = APPLY_CHA;
            break;
         case 6:
            paf->location = APPLY_LCK;
            break;
      }
      paf->modifier = 1;
      paf->bit = 0;
      LINK( paf, obj->first_affect, obj->last_affect, next, prev );
      return;
   }

   if( !str_cmp( argument, "recall" ) )
   {
      ROOM_INDEX_DATA *location = NULL;

      if( ch->pcdata->favor < ch->pcdata->deity->srecall )
      {
         send_to_char( "Your favor is inadequate for such a supplication.\n\r", ch );
         return;
      }

      if( IS_ROOM_FLAG( ch->in_room, ROOM_NOSUPPLICATE ) )
      {
         send_to_char( "You have been forsaken!\n\r", ch );
         return;
      }

      if( get_timer( ch, TIMER_RECENTFIGHT ) > 0 && !IS_IMMORTAL( ch ) )
      {
         send_to_char( "You cannot supplicate recall under adrenaline!\n\r", ch );
         return;
      }

      location = recall_room( ch );

      if( !location )
      {
         bug( "No room index for recall supplication. Deity: %s", ch->pcdata->deity->name );
         send_to_char( "Your deity has forsaken you!\n\r", ch );
         return;
      }

      act( AT_MAGIC, "$n disappears in a column of divine power.", ch, NULL, NULL, TO_ROOM );

      leave_map( ch, NULL, location );

      act( AT_MAGIC, "$n appears in the room from a column of divine mist.", ch, NULL, NULL, TO_ROOM );
      ch->pcdata->favor -= ch->pcdata->deity->srecall;
      adjust_favor( ch, -1, 1 );
      return;
   }
   send_to_char( "You cannot supplicate for that.\n\r", ch );
   return;
}
