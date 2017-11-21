/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2007 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 * Registered with the United States Copyright Office: TX 5-877-286         *
 *                                                                          *
 * External contributions from Remcon, Quixadhal, Zarius, and many others.  *
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
 *                     Stock Zone reader and converter                      *
 ****************************************************************************/

/* Converts stock Smaug version 0 and version 1 areas into AFKMud format - Samson 12-21-01 */
/* Converts SmaugWiz version 1000 files into AFKMud format - Samson 4-24-03 */

#include <ctype.h>
#include "mud.h"
#include "help.h"
#include "shops.h"

void write_area_list( void );
void boot_log( const char *, ... );
void close_area( AREA_DATA * pArea );
void mprog_read_programs( FILE * fp, MOB_INDEX_DATA * pMobIndex );
void oprog_read_programs( FILE * fp, OBJ_INDEX_DATA * pObjIndex );
void rprog_read_programs( FILE * fp, ROOM_INDEX_DATA * pRoomIndex );
void load_room_reset( ROOM_INDEX_DATA * room, FILE * fp );
void renumber_put_resets( ROOM_INDEX_DATA * room );
void load_area_file( AREA_DATA * tarea, char *filename, bool isproto );
AREA_DATA *find_area( char *filename );
bool check_area_conflict( AREA_DATA *area, int low_range, int hi_range );

extern int top_affect;
extern int top_exit;
extern int top_reset;
extern int top_shop;
extern int top_repair;
extern int top_ed;
extern FILE *fpArea;

bool area_failed;
int dotdcheck;

char *const stock_act[] = {
   "npc", "sentinel", "scavenger", "r1", "r2", "aggressive", "stayarea",
   "wimpy", "pet", "train", "practice", "immortal", "deadly", "polyself",
   "meta_aggr", "guardian", "running", "nowander", "mountable", "mounted",
   "scholar", "secretive", "hardhat", "mobinvis", "noassist", "autonomous",
   "pacifist", "noattack", "annoying", "statshield", "proto", "r14"
};

char *const stock_ex_flags[] = {
   "isdoor", "closed", "locked", "secret", "swim", "pickproof", "fly", "climb",
   "dig", "eatkey", "nopassdoor", "hidden", "passage", "portal", "r1", "r2",
   "can_climb", "can_enter", "can_leave", "auto", "noflee", "searchable",
   "bashed", "bashproof", "nomob", "window", "can_look", "isbolt", "bolted"
};

char *const stock_aff[] = {
   "blind", "invisible", "detect_evil", "detect_invis", "detect_magic",
   "detect_hidden", "hold", "sanctuary", "faerie_fire", "infrared", "curse",
   "_flaming", "poison", "protect", "_paralysis", "sneak", "hide", "sleep",
   "charm", "flying", "pass_door", "floating", "truesight", "detect_traps",
   "scrying", "fireshield", "shockshield", "r1", "iceshield", "possess",
   "berserk", "aqua_breath", "recurringspell", "contagious", "acidmist",
   "venomshield"
};

char *const stock_race[] = {
   "human", "high-elf", "dwarf", "halfling", "pixie", "vampire", "half-ogre",
   "half-orc", "half-troll", "half-elf", "gith", "drow", "sea-elf",
   "iguanadon", "gnome", "r5", "r6", "r7", "r8", "troll",
   "ant", "ape", "baboon", "bat", "bear", "bee",
   "beetle", "boar", "bugbear", "cat", "dog", "dragon", "ferret", "fly",
   "gargoyle", "gelatin", "ghoul", "gnoll", "gnome", "goblin", "golem",
   "gorgon", "harpy", "hobgoblin", "kobold", "lizardman", "locust",
   "lycanthrope", "minotaur", "mold", "mule", "neanderthal", "ooze", "orc",
   "rat", "rustmonster", "shadow", "shapeshifter", "shrew", "shrieker",
   "skeleton", "slime", "snake", "spider", "stirge", "thoul", "troglodyte",
   "undead", "wight", "wolf", "worm", "zombie", "bovine", "canine", "feline",
   "porcine", "mammal", "rodent", "avis", "reptile", "amphibian", "fish",
   "crustacean", "insect", "spirit", "magical", "horse", "animal", "humanoid",
   "monster", "god"
};

char *const stock_class[] = {
   "mage", "cleric", "rogue", "warrior", "vampire", "druid", "ranger",
   "augurer", "paladin", "nephandi", "savage", "pirate", "pc12", "pc13",
   "pc14", "pc15", "pc16", "pc17", "pc18", "pc19",
   "baker", "butcher", "blacksmith", "mayor", "king", "queen"
};

char *const stock_pos[] = {
   "dead", "mortal", "incapacitated", "stunned", "sleeping", "berserk", "resting",
   "aggressive", "sitting", "fighting", "defensive", "evasive", "standing", "mounted",
   "shove", "drag"
};

char *const stock_oflags[] = {
   "glow", "hum", "dark", "loyal", "evil", "invis", "magic", "nodrop", "bless",
   "antigood", "antievil", "antineutral", "noremove", "inventory",
   "antimage", "antirogue", "antiwarrior", "anticleric", "organic", "metal",
   "donation", "clanobject", "clancorpse", "antivampire(UNUSED)", "antidruid",
   "hidden", "poisoned", "covering", "deathrot", "buried", "proto",
   "nolocate", "groundrot", "lootable"
};

char *const stock_wflags[] = {
   "take", "finger", "neck", "body", "head", "legs", "feet", "hands", "arms",
   "shield", "about", "waist", "wrist", "wield", "hold", "dual", "ears", "eyes",
   "missile", "back", "face", "ankle", "r4", "r5", "r6",
   "r7", "r8", "r9", "r10", "r11", "r12", "r13"
};

char *const stock_rflags[] = {
   "dark", "death", "nomob", "indoors", "lawful", "neutral", "chaotic",
   "nomagic", "tunnel", "private", "safe", "solitary", "petshop", "norecall",
   "donation", "nodropall", "silence", "logspeech", "nodrop", "clanstoreroom",
   "nosummon", "noastral", "teleport", "teleshowdesc", "nofloor",
   "nosupplicate", "arena", "nomissile", "r4", "r5", "proto", "dnd"
};

char *const stock_area_flags[] = {
   "nopkill", "freekill", "noteleport", "spelllimit", "r4", "r5", "r6", "r7", "r8",
   "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16", "r17",
   "r18", "r19", "r20", "r21", "r22", "r23", "r24",
   "r25", "r26", "r27", "r28", "r29", "r30", "r31"
};

char *const stock_lang_names[] = {
   "common", "elvish", "dwarven", "pixie", "ogre", "orcish", "trollese", "rodent", "insectoid",
   "mammal", "reptile", "dragon", "spiritual", "magical", "goblin", "god", "ancient",
   "halfling", "clan", "gith", "r20", "r21", "r22", "r23", "r24",
   "r25", "r26", "r27", "r28", "r29", "r30", "r31"
};

char *const stock_attack_flags[] = {
   "bite", "claws", "tail", "sting", "punch", "kick", "trip", "bash", "stun",
   "gouge", "backstab", "feed", "drain", "firebreath", "frostbreath",
   "acidbreath", "lightnbreath", "gasbreath", "poison", "nastypoison", "gaze",
   "blindness", "causeserious", "earthquake", "causecritical", "curse",
   "flamestrike", "harm", "fireball", "colorspray", "weaken", "r1"
};

char *const stock_defense_flags[] = {
   "parry", "dodge", "heal", "curelight", "cureserious", "curecritical",
   "dispelmagic", "dispelevil", "sanctuary", "fireshield", "shockshield",
   "shield", "bless", "stoneskin", "teleport", "monsum1", "monsum2", "monsum3",
   "monsum4", "disarm", "iceshield", "grip", "truesight", "r4", "r5", "r6", "r7",
   "r8", "r9", "r10", "r11", "r12", "acidmist", "venomshield"
};

char *const stock_o_types[] = {
   "none", "light", "scroll", "wand", "staff", "weapon", "_fireweapon", "_missile",
   "treasure", "armor", "potion", "_worn", "furniture", "trash", "_oldtrap",
   "container", "_note", "drinkcon", "key", "food", "money", "pen", "boat",
   "corpse", "corpse_pc", "fountain", "pill", "blood", "bloodstain",
   "scraps", "pipe", "herbcon", "herb", "incense", "fire", "book", "switch",
   "lever", "pullchain", "button", "dial", "rune", "runepouch", "match", "trap",
   "map", "portal", "paper", "tinder", "lockpick", "spike", "disease", "oil",
   "fuel", "_empty1", "_empty2", "missileweapon", "projectile", "quiver", "shovel",
   "salve", "cook", "keyring", "odor", "chance"
};

char *const stock_sect[] = {
   "indoors", "city", "field", "forest", "hills", "mountain", "water_swim",
   "water_noswim", "underwater", "air", "desert", "dunno", "oceanfloor",
   "underground", "lava", "swamp", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
   "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16"
};

// Runs the entire list, easier to call in places that have to check them all
bool check_area_conflicts( int lo, int hi )
{
   AREA_DATA *area;

   for( area = first_area; area; area = area->next )
   {
      if( check_area_conflict( area, lo, hi ) )
         return true;
   }
   return false;
}

void load_stmobiles( AREA_DATA * tarea, FILE * fp, bool manual )
{
   MOB_INDEX_DATA *pMobIndex;
   char *ln;
   int x1, x2, x3, x4, x5, x6, x7;

   if( !tarea )
   {
      bug( "%s: no #AREA seen yet.", __FUNCTION__ );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   for( ;; )
   {
      char letter;
      bool oldmob, tmpBootDb;

      letter = fread_letter( fp );
      if( letter != '#' )
      {
         bug( "%s: # not found.", __FUNCTION__ );
         if( fBootDb )
         {
            shutdown_mud( "# not found" );
            exit( 1 );
         }
         else
            return;
      }

      int vnum = fread_number( fp );
      if( vnum == 0 )
         break;

      tmpBootDb = fBootDb;
      fBootDb = false;

      AREA_DATA *area;
      for( area = first_area; area; area = area->next )
      {
         if( !str_cmp( area->filename, tarea->filename ) )
            continue;

         bool area_conflict = check_area_conflict( area, vnum, vnum );

         if( area_conflict )
         {
            log_printf( "ERROR: %s has vnum conflict with %s!",
                        tarea->filename, ( area->filename ? area->filename : "(invalid)" ) );
            log_printf( "%s occupies vnums   : %-6d - %-6d", ( area->filename ? area->filename : "(invalid)" ),
                        area->low_vnum, area->hi_vnum );
            log_printf( "%s wants to use vnum: %-6d", tarea->filename, vnum );
            if( !manual )
            {
               log_string( "This is a fatal error. Program terminated." );
               exit( 1 );
            }
            else
            {
               area_failed = true;
               log_string( "This is a fatal error. Conversion terminated." );
               close_area( tarea );
               return;
            }
         }
      }

      if( get_mob_index( vnum ) )
      {
         if( tmpBootDb )
         {
            bug( "%s: vnum %d duplicated.", __FUNCTION__, vnum );
            if( manual )
            {
               area_failed = true;
               log_string( "This is a fatal error. Conversion terminated." );
               close_area( tarea );
               return;
            }
            else
            {
               shutdown_mud( "duplicate vnum" );
               exit( 1 );
            }
         }
         else
         {
            pMobIndex = get_mob_index( vnum );
            log_printf_plus( LOG_BUILD, sysdata.build_level, "Cleaning mobile: %d", vnum );
            clean_mob( pMobIndex );
            oldmob = true;
         }
      }
      else
      {
         oldmob = false;
         CREATE( pMobIndex, MOB_INDEX_DATA, 1 );
      }
      fBootDb = tmpBootDb;

      pMobIndex->vnum = vnum;
      if( fBootDb )
      {
         if( !tarea->low_vnum )
            tarea->low_vnum = vnum;
         if( vnum > tarea->hi_vnum )
            tarea->hi_vnum = vnum;
      }
      pMobIndex->area = tarea;
      pMobIndex->player_name = fread_string( fp );
      pMobIndex->short_descr = fread_string( fp );
      pMobIndex->long_descr = fread_string( fp );
      {
         char *desc = fread_flagstring( fp );
         if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
         {
            pMobIndex->chardesc = STRALLOC( desc );
            pMobIndex->chardesc[0] = UPPER( pMobIndex->chardesc[0] );
         }
      }

      if( pMobIndex->long_descr != NULL )
         pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );
      {
         char *sact, *saff;
         char flag[MIL];
         EXT_BV temp;
         int value;

         temp = fread_bitvector( fp );
         sact = ext_flag_string( &temp, stock_act );

         while( sact[0] != '\0' )
         {
            sact = one_argument( sact, flag );
            value = get_actflag( flag );
            if( value < 0 || value >= MAX_ACT_FLAG )
               bug( "Bad act_flag: %s", flag );
            else
               SET_ACT_FLAG( pMobIndex, value );
         }
         SET_ACT_FLAG( pMobIndex, ACT_IS_NPC );

         temp = fread_bitvector( fp );
         saff = ext_flag_string( &temp, stock_aff );

         while( saff[0] != '\0' )
         {
            saff = one_argument( saff, flag );
            value = get_aflag( flag );
            if( value < 0 || value >= MAX_AFFECTED_BY )
               bug( "Bad aff_flag: %s", flag );
            else
               SET_AFFECTED( pMobIndex, value );
         }
      }

      pMobIndex->pShop = NULL;
      pMobIndex->rShop = NULL;
      pMobIndex->alignment = fread_number( fp );
      letter = fread_letter( fp );
      pMobIndex->level = fread_number( fp );

      fread_number( fp );
      pMobIndex->mobthac0 = 21;  /* Autovonvert to the autocomputation value */
      pMobIndex->ac = fread_number( fp );
      pMobIndex->hitnodice = fread_number( fp );
      /*
       * 'd'      
       */ fread_letter( fp );
      pMobIndex->hitsizedice = fread_number( fp );
      /*
       * '+'      
       */ fread_letter( fp );
      pMobIndex->hitplus = fread_number( fp );
      pMobIndex->damnodice = fread_number( fp );
      /*
       * 'd'      
       */ fread_letter( fp );
      pMobIndex->damsizedice = fread_number( fp );
      /*
       * '+'      
       */ fread_letter( fp );
      pMobIndex->damplus = fread_number( fp );

      ln = fread_line( fp );
      x1 = x2 = 0;
      sscanf( ln, "%d %d", &x1, &x2 );
      pMobIndex->gold = x1;
      pMobIndex->exp = -1; /* Convert mob to use autocalc exp */

      {
         char *spos;
         int pos = fread_number( fp );

         if( pos < 100 )
         {
            switch ( pos )
            {
               default:
               case 0:
               case 1:
               case 2:
               case 3:
               case 4:
                  break;
               case 5:
                  pos = 6;
                  break;
               case 6:
                  pos = 8;
                  break;
               case 7:
                  pos = 9;
                  break;
               case 8:
                  pos = 12;
                  break;
               case 9:
                  pos = 13;
                  break;
               case 10:
                  pos = 14;
                  break;
               case 11:
                  pos = 15;
                  break;
            }
         }
         else
            pos -= 100;

         spos = stock_pos[pos];
         pMobIndex->position = get_npc_position( spos );
      }

      {
         char *sdefpos;
         int defpos = fread_number( fp );

         if( defpos < 100 )
         {
            switch ( defpos )
            {
               default:
               case 0:
               case 1:
               case 2:
               case 3:
               case 4:
                  break;
               case 5:
                  defpos = 6;
                  break;
               case 6:
                  defpos = 8;
                  break;
               case 7:
                  defpos = 9;
                  break;
               case 8:
                  defpos = 12;
                  break;
               case 9:
                  defpos = 13;
                  break;
               case 10:
                  defpos = 14;
                  break;
               case 11:
                  defpos = 15;
                  break;
            }
         }
         else
         {
            defpos -= 100;
         }
         sdefpos = stock_pos[defpos];
         pMobIndex->defposition = get_npc_position( sdefpos );
      }
      /*
       * Back to meaningful values.
       */
      pMobIndex->sex = fread_number( fp );

      if( letter != 'S' && letter != 'C' && letter != 'D' && letter != 'Z' )
      {
         bug( "%s: vnum %d: letter '%c' not S, C, Z, or D.", __FUNCTION__, vnum, letter );
         shutdown_mud( "bad mob data" );
         exit( 1 );
      }

      if( letter == 'C' || letter == 'D' || letter == 'Z' ) /* Realms complex mob  -Thoric */
      {
         pMobIndex->perm_str = fread_number( fp );
         pMobIndex->perm_int = fread_number( fp );
         pMobIndex->perm_wis = fread_number( fp );
         pMobIndex->perm_dex = fread_number( fp );
         pMobIndex->perm_con = fread_number( fp );
         pMobIndex->perm_cha = fread_number( fp );
         pMobIndex->perm_lck = fread_number( fp );
         pMobIndex->saving_poison_death = fread_number( fp );
         pMobIndex->saving_wand = fread_number( fp );
         pMobIndex->saving_para_petri = fread_number( fp );
         pMobIndex->saving_breath = fread_number( fp );
         pMobIndex->saving_spell_staff = fread_number( fp );

         if( tarea->version < 1000 )   /* Standard Smaug version 0 or 1 */
         {
            ln = fread_line( fp );
            x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
            sscanf( ln, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );

            {
               char *srace, *sclass;

               if( x1 >= 0 && x1 < 90 )
                  srace = stock_race[x1];
               else
                  srace = "human";

               pMobIndex->race = get_npc_race( srace );

               if( pMobIndex->race < 0 || pMobIndex->race >= MAX_NPC_RACE )
               {
                  bug( "%s: vnum %d: Mob has invalid race! Defaulting to monster.", __FUNCTION__, vnum );
                  pMobIndex->race = get_npc_race( "monster" );
               }

               if( x2 >= 0 && x2 < 25 )
                  sclass = stock_class[x2];
               else
                  sclass = "warrior";

               pMobIndex->Class = get_npc_class( sclass );

               if( pMobIndex->Class < 0 || pMobIndex->Class >= MAX_NPC_CLASS )
               {
                  bug( "%s: vnum %d: Mob has invalid Class! Defaulting to warrior.", __FUNCTION__, vnum );
                  pMobIndex->Class = get_npc_class( "warrior" );
               }
            }
            pMobIndex->height = x3;
            pMobIndex->weight = x4;

            {
               char *speaks = NULL, *speaking = NULL;
               char flag[MIL];
               int value;

               speaks = flag_string( x5, stock_lang_names );

               while( speaks[0] != '\0' )
               {
                  speaks = one_argument( speaks, flag );
                  value = get_langnum( flag );
                  if( value == -1 )
                     bug( "Bad speaks: %s", flag );
                  else
                     TOGGLE_BIT( pMobIndex->speaks, 1 << value );
               }

               speaking = flag_string( x6, stock_lang_names );

               while( speaking[0] != '\0' )
               {
                  speaking = one_argument( speaking, flag );
                  value = get_langnum( flag );
                  if( value == -1 )
                     bug( "Bad speaking: %s", flag );
                  else
                     TOGGLE_BIT( pMobIndex->speaking, 1 << value );
               }
            }
            pMobIndex->numattacks = ( float )x7;
         }  /* End of standard Smaug zone */
         else  /* A SmaugWiz zone */
         {
            char *speaks = NULL;
            char *speaking = NULL;
            char flag[MIL];
            int value;

            ln = fread_line( fp );
            x1 = x2 = x3 = x4 = x5 = 0;
            sscanf( ln, "%d %d %d %d %d", &x1, &x2, &x3, &x4, &x5 );

            {
               char *srace, *sclass;

               if( x1 >= 0 && x1 < 90 )
                  srace = stock_race[x1];
               else
                  srace = "human";

               pMobIndex->race = get_npc_race( srace );

               if( pMobIndex->race < 0 || pMobIndex->race >= MAX_NPC_RACE )
               {
                  bug( "%s: vnum %d: Mob has invalid race: %s. Defaulting to monster.", __FUNCTION__, vnum, srace );
                  pMobIndex->race = get_npc_race( "monster" );
               }

               if( x2 >= 0 && x2 < 25 )
                  sclass = stock_class[x2];
               else
                  sclass = "warrior";

               pMobIndex->Class = get_npc_class( sclass );

               if( pMobIndex->Class < 0 || pMobIndex->Class >= MAX_NPC_CLASS )
               {
                  bug( "%s: vnum %d: Mob has invalid class: %s. Defaulting to warrior.", __FUNCTION__, vnum, sclass );
                  pMobIndex->Class = get_npc_class( "warrior" );
               }
            }
            pMobIndex->height = x3;
            pMobIndex->weight = x4;
            pMobIndex->numattacks = ( float )x5;

            speaks = fread_flagstring( fp );

            while( speaks[0] != '\0' )
            {
               speaks = one_argument( speaks, flag );
               value = get_langnum( flag );
               if( value == -1 )
                  bug( "Unknown speaks language: %s", flag );
               else
                  TOGGLE_BIT( pMobIndex->speaks, 1 << value );
            }

            speaking = fread_flagstring( fp );

            while( speaking[0] != '\0' )
            {
               speaking = one_argument( speaking, flag );
               value = get_langnum( flag );
               if( value == -1 )
                  bug( "Unknown speaking language: %s", flag );
               else
                  TOGGLE_BIT( pMobIndex->speaking, 1 << value );
            }
         }  /* End of SmaugWiz zone */

         if( !pMobIndex->speaks )
            pMobIndex->speaks = LANG_COMMON;
         if( !pMobIndex->speaking )
            pMobIndex->speaking = LANG_COMMON;

         if( pMobIndex->race <= MAX_RACE )   /* Convert the mob to use randatreasure according to race */
            pMobIndex->gold = -1;
         else
            pMobIndex->gold = 0;

         pMobIndex->hitroll = fread_number( fp );
         pMobIndex->damroll = fread_number( fp );
         pMobIndex->xflags = fread_number( fp );
         pMobIndex->resistant = fread_bitvector( fp );
         pMobIndex->immune = fread_bitvector( fp );
         pMobIndex->susceptible = fread_bitvector( fp );

         {
            char *attacks, *defenses;
            char flag[MIL];
            EXT_BV temp;
            int value;

            temp = fread_bitvector( fp );
            attacks = ext_flag_string( &temp, stock_attack_flags );

            while( attacks[0] != '\0' )
            {
               attacks = one_argument( attacks, flag );
               value = get_attackflag( flag );
               if( value < 0 || value >= MAX_ATTACK_TYPE )
                  bug( "Bad attack: %s", flag );
               else
                  SET_ATTACK( pMobIndex, value );
            }

            temp = fread_bitvector( fp );
            defenses = ext_flag_string( &temp, stock_defense_flags );

            while( defenses[0] != '\0' )
            {
               defenses = one_argument( defenses, flag );
               value = get_defenseflag( flag );
               if( value < 0 || value >= MAX_DEFENSE_TYPE )
                  bug( "Bad defense: %s", flag );
               else
                  SET_DEFENSE( pMobIndex, value );
            }
         }
         if( letter == 'Z' )
         {
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
         }
         if( letter == 'D' && dotdcheck )
         {
            fread_number( fp );
            fread_number( fp );
            pMobIndex->absorb = fread_bitvector( fp );
         }
      }
      else
      {
         pMobIndex->perm_str = 13;
         pMobIndex->perm_dex = 13;
         pMobIndex->perm_int = 13;
         pMobIndex->perm_wis = 13;
         pMobIndex->perm_cha = 13;
         pMobIndex->perm_con = 13;
         pMobIndex->perm_lck = 13;
         pMobIndex->race = RACE_HUMAN;
         pMobIndex->Class = CLASS_WARRIOR;
         pMobIndex->xflags = 0;
         xCLEAR_BITS( pMobIndex->resistant );
         xCLEAR_BITS( pMobIndex->immune );
         xCLEAR_BITS( pMobIndex->susceptible );
         xCLEAR_BITS( pMobIndex->absorb );
         pMobIndex->numattacks = 0;
         xCLEAR_BITS( pMobIndex->attacks );
         xCLEAR_BITS( pMobIndex->defenses );
      }

      for( ;; )
      {
         letter = fread_letter( fp );

         if( letter == 'S' || letter == '$' )
            break;

         if( letter == '#' )
         {
            ungetc( letter, fp );
            break;
         }

         if( letter == 'T' )
            fread_to_eol( fp );

         else if( letter == '>' )
         {
            ungetc( letter, fp );
            mprog_read_programs( fp, pMobIndex );
         }
         else
         {
            bug( "%s: vnum %d has unknown field '%c' after defense values", __FUNCTION__, vnum, letter );
            shutdown_mud( "Invalid mob field data" );
            exit( 1 );
         }
      }

      if( !oldmob )
      {
         int iHash = vnum % MAX_KEY_HASH;
         pMobIndex->next = mob_index_hash[iHash];
         mob_index_hash[iHash] = pMobIndex;
         top_mob_index++;
      }
   }
   return;
}

void load_stobjects( AREA_DATA * tarea, FILE * fp, bool manual )
{
   OBJ_INDEX_DATA *pObjIndex;
   char letter;
   char *ln;
   int x1, x2, x3, x4, x5, x6;

   if( !tarea )
   {
      bug( "%s: no #AREA seen yet.", __FUNCTION__ );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   for( ;; )
   {
      bool tmpBootDb, oldobj;

      letter = fread_letter( fp );
      if( letter != '#' )
      {
         bug( "%s: # not found.", __FUNCTION__ );
         if( fBootDb )
         {
            shutdown_mud( "# not found" );
            exit( 1 );
         }
         else
            return;
      }

      int vnum = fread_number( fp );
      if( vnum == 0 )
         break;

      tmpBootDb = fBootDb;
      fBootDb = false;

      AREA_DATA *area;
      for( area = first_area; area; area = area->next )
      {
         if( !str_cmp( area->filename, tarea->filename ) )
            continue;

         bool area_conflict = check_area_conflict( area, vnum, vnum );

         if( area_conflict )
         {
            log_printf( "ERROR: %s has vnum conflict with %s!",
                        tarea->filename, ( area->filename ? area->filename : "(invalid)" ) );
            log_printf( "%s occupies vnums   : %-6d - %-6d", ( area->filename ? area->filename : "(invalid)" ),
                        area->low_vnum, area->hi_vnum );
            log_printf( "%s wants to use vnum: %-6d", tarea->filename, vnum );
            if( !manual )
            {
               log_string( "This is a fatal error. Program terminated." );
               exit( 1 );
            }
            else
            {
               area_failed = true;
               log_string( "This is a fatal error. Conversion terminated." );
               close_area( tarea );
               return;
            }
         }
      }

      if( get_obj_index( vnum ) )
      {
         if( tmpBootDb )
         {
            bug( "%s: vnum %d duplicated.", __FUNCTION__, vnum );
            if( manual )
            {
               area_failed = true;
               log_string( "This is a fatal error. Conversion terminated." );
               close_area( tarea );
               return;
            }
            else
            {
               shutdown_mud( "duplicate vnum" );
               exit( 1 );
            }
         }
         else
         {
            pObjIndex = get_obj_index( vnum );
            log_printf_plus( LOG_BUILD, sysdata.build_level, "Cleaning object: %d", vnum );
            clean_obj( pObjIndex );
            oldobj = true;
         }
      }
      else
      {
         oldobj = false;
         CREATE( pObjIndex, OBJ_INDEX_DATA, 1 );
      }
      fBootDb = tmpBootDb;

      pObjIndex->vnum = vnum;
      if( fBootDb )
      {
         if( !tarea->low_vnum )
            tarea->low_vnum = vnum;
         if( vnum > tarea->hi_vnum )
            tarea->hi_vnum = vnum;
      }

      pObjIndex->area = tarea;
      pObjIndex->name = fread_string( fp );
      pObjIndex->short_descr = fread_string( fp );
      pObjIndex->objdesc = fread_string( fp );
      {
         char *desc = fread_flagstring( fp );
         if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
            pObjIndex->action_desc = STRALLOC( desc );
      }
      if( pObjIndex->objdesc != NULL )
         pObjIndex->objdesc[0] = UPPER( pObjIndex->objdesc[0] );
      {
         char *sotype, *eflags, *wflags;
         char flag[MIL];
         EXT_BV temp;
         int value;

         sotype = stock_o_types[fread_number( fp )];
         pObjIndex->item_type = get_otype( sotype );

         temp = fread_bitvector( fp );
         eflags = ext_flag_string( &temp, stock_oflags );

         while( eflags[0] != '\0' )
         {
            eflags = one_argument( eflags, flag );
            value = get_oflag( flag );
            if( value < 0 || value >= MAX_ITEM_FLAG )
               bug( "Bad extra flag: %s", flag );
            else
               SET_OBJ_FLAG( pObjIndex, value );
         }

         ln = fread_line( fp );
         x1 = x2 = 0;
         sscanf( ln, "%d %d", &x1, &x2 );

         wflags = flag_string( x1, stock_wflags );

         while( wflags[0] != '\0' )
         {
            wflags = one_argument( wflags, flag );
            value = get_wflag( flag );
            if( value < 0 || value >= MAX_WEAR_FLAG )
               bug( "Bad wear flag: %s", flag );
            else
               SET_WEAR_FLAG( pObjIndex, 1 << value );
         }
         pObjIndex->layers = x2;
      }

      if( tarea->version < 1000 )
      {
         ln = fread_line( fp );
         x1 = x2 = x3 = x4 = x5 = x6 = 0;
         sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );
         pObjIndex->value[0] = x1;
         pObjIndex->value[1] = x2;
         pObjIndex->value[2] = x3;
         pObjIndex->value[3] = x4;
         pObjIndex->value[4] = x5;
         pObjIndex->value[5] = x6;

         pObjIndex->weight = fread_number( fp );
         pObjIndex->weight = UMAX( 1, pObjIndex->weight );
         pObjIndex->cost = fread_number( fp );
         pObjIndex->rent = fread_number( fp );

         if( pObjIndex->rent >= sysdata.minrent )
            pObjIndex->limit = 1;   /* Sets new limit since stock zones won't have one */
         else
            pObjIndex->limit = 9999;   /* Default value, this should more than insure that the shit loads */

         pObjIndex->rent = -2;
      }
      else
      {
         ln = fread_line( fp );
         x1 = x2 = x3 = x4 = x5 = x6 = 0;
         sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );
         pObjIndex->value[0] = x1;
         pObjIndex->value[1] = x2;
         pObjIndex->value[2] = x3;
         pObjIndex->value[3] = x4;
         pObjIndex->value[4] = x5;
         pObjIndex->value[5] = x6;
      }

      if( tarea->version == 1 || tarea->version == 1000 )
      {
         switch ( pObjIndex->item_type )
         {
            default:
               break;

            case ITEM_PILL:
            case ITEM_POTION:
            case ITEM_SCROLL:
               pObjIndex->value[1] = skill_lookup( fread_word( fp ) );
               pObjIndex->value[2] = skill_lookup( fread_word( fp ) );
               pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
               break;

            case ITEM_STAFF:
            case ITEM_WAND:
               pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
               break;

            case ITEM_SALVE:
               pObjIndex->value[4] = skill_lookup( fread_word( fp ) );
               pObjIndex->value[5] = skill_lookup( fread_word( fp ) );
               break;
         }
      }

      if( tarea->version == 1000 )
      {
         while( !isdigit( letter = fread_letter( fp ) ) )
            fread_to_eol( fp );
         ungetc( letter, fp );

         pObjIndex->weight = fread_number( fp );
         pObjIndex->weight = UMAX( 1, pObjIndex->weight );
         pObjIndex->cost = fread_number( fp );
         pObjIndex->rent = fread_number( fp );

         if( pObjIndex->rent >= sysdata.minrent )
            pObjIndex->limit = 1;   /* Sets new limit since stock zones won't have one */
         else
            pObjIndex->limit = 9999;   /* Default value, this should more than insure that the shit loads */

         pObjIndex->rent = -2;
      }

      for( ;; )
      {
         letter = fread_letter( fp );

         if( letter == 'S' )
            break;

         if( letter == 'A' )
         {
            affect_data *paf;
            char *risa = NULL;
            char flag[MIL];
            int value;

            paf = new affect_data;
            paf->type = -1;
            paf->duration = -1;
            paf->bit = 0;
            paf->modifier = 0;
            xCLEAR_BITS( paf->rismod );

            paf->location = fread_number( fp );

            if( paf->location == APPLY_WEAPONSPELL
                || paf->location == APPLY_WEARSPELL
                || paf->location == APPLY_REMOVESPELL
                || paf->location == APPLY_STRIPSN
                || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
               paf->modifier = slot_lookup( fread_number( fp ) );
            else if( paf->location == APPLY_RESISTANT
                     || paf->location == APPLY_IMMUNE
                     || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
            {
               value = fread_number( fp );
               risa = flag_string( value, ris_flags );

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
            paf->bit = 0;
            LINK( paf, pObjIndex->first_affect, pObjIndex->last_affect, next, prev );
            ++top_affect;
         }

         else if( letter == 'D' )
         {
            fread_number( fp );
            pObjIndex->magic_flags = fread_number( fp );
         }

         else if( letter == 'E' )
         {
            EXTRA_DESCR_DATA *ed;

            CREATE( ed, EXTRA_DESCR_DATA, 1 );
            ed->keyword = fread_string( fp );
            ed->extradesc = fread_string( fp );
            LINK( ed, pObjIndex->first_extradesc, pObjIndex->last_extradesc, next, prev );
            top_ed++;
         }
         else if( letter == '>' )
         {
            ungetc( letter, fp );
            oprog_read_programs( fp, pObjIndex );
         }

         else
         {
            ungetc( letter, fp );
            break;
         }
      }

      /*
       * Translate spell "slot numbers" to internal "skill numbers."
       */
      if( tarea->version == 0 )
      {
         switch ( pObjIndex->item_type )
         {
            default:
               break;

            case ITEM_PILL:
            case ITEM_POTION:
            case ITEM_SCROLL:
               pObjIndex->value[1] = slot_lookup( pObjIndex->value[1] );
               pObjIndex->value[2] = slot_lookup( pObjIndex->value[2] );
               pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
               break;

            case ITEM_STAFF:
            case ITEM_WAND:
               pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
               break;

            case ITEM_SALVE:
               pObjIndex->value[4] = slot_lookup( pObjIndex->value[4] );
               pObjIndex->value[5] = slot_lookup( pObjIndex->value[5] );
               break;
         }
      }

      /*
       * Set stuff the object won't have in stock 
       */
      pObjIndex->socket[0] = STRALLOC( "None" );
      pObjIndex->socket[1] = STRALLOC( "None" );
      pObjIndex->socket[2] = STRALLOC( "None" );

      if( !oldobj )
      {
         int iHash = vnum % MAX_KEY_HASH;
         pObjIndex->next = obj_index_hash[iHash];
         obj_index_hash[iHash] = pObjIndex;
         top_obj_index++;
      }
   }
   return;
}

void load_strooms( AREA_DATA *tarea, FILE *fp, bool manual )
{
   ROOM_INDEX_DATA *pRoomIndex;
   char *ln;
   int count = 0;

   if( !tarea )
   {
      bug( "%s: no #AREA seen yet.", __FUNCTION__ );
      shutdown_mud( "No #AREA" );
      exit( 1 );
   }

   for( ;; )
   {
      char letter;
      int door;
      bool tmpBootDb;
      bool oldroom;
      int x1, x2, x3, x4, x5, x6, x7, x8, x9;

      letter = fread_letter( fp );
      if( letter != '#' )
      {
         bug( "%s: # not found.", __FUNCTION__ );
         if( fBootDb )
         {
            shutdown_mud( "# not found" );
            exit( 1 );
         }
         else
            return;
      }

      int vnum = fread_number( fp );
      if( vnum == 0 )
         break;

      tmpBootDb = fBootDb;
      fBootDb = false;

      AREA_DATA *area;
      for( area = first_area; area; area = area->next )
      {
         if( !str_cmp( area->filename, tarea->filename ) )
            continue;

         bool area_conflict = check_area_conflict( area, vnum, vnum );

         if( area_conflict )
         {
            log_printf( "ERROR: %s has vnum conflict with %s!",
                        tarea->filename, ( area->filename ? area->filename : "(invalid)" ) );
            log_printf( "%s occupies vnums   : %-6d - %-6d", ( area->filename ? area->filename : "(invalid)" ),
                        area->low_vnum, area->hi_vnum );
            log_printf( "%s wants to use vnum: %-6d", tarea->filename, vnum );
            if( !manual )
            {
               log_string( "This is a fatal error. Program terminated." );
               exit( 1 );
            }
            else
            {
               area_failed = true;
               log_string( "This is a fatal error. Conversion terminated." );
               close_area( tarea );
               return;
            }
         }
      }

      if( get_room_index( vnum ) != NULL )
      {
         if( tmpBootDb )
         {
            bug( "%s: vnum %d duplicated.", __FUNCTION__, vnum );
            if( manual )
            {
               area_failed = true;
               log_string( "This is a fatal error. Conversion terminated." );
               close_area( tarea );
               return;
            }
            else
            {
               shutdown_mud( "duplicate vnum" );
               exit( 1 );
            }
         }
         else
         {
            pRoomIndex = get_room_index( vnum );
            log_printf_plus( LOG_BUILD, sysdata.build_level, "Cleaning room: %d", vnum );
            clean_room( pRoomIndex );
            oldroom = true;
         }
      }
      else
      {
         oldroom = false;
         CREATE( pRoomIndex, ROOM_INDEX_DATA, 1 );
         pRoomIndex->first_person = NULL;
         pRoomIndex->last_person = NULL;
         pRoomIndex->first_content = NULL;
         pRoomIndex->last_content = NULL;
      }

      fBootDb = tmpBootDb;
      pRoomIndex->area = tarea;
      pRoomIndex->vnum = vnum;

      if( fBootDb )
      {
         if( !tarea->low_vnum )
            tarea->low_vnum = vnum;
         if( vnum > tarea->hi_vnum )
            tarea->hi_vnum = vnum;
      }
      pRoomIndex->name = fread_string( fp );
      {
         char *ndesc = fread_flagstring( fp );
         if( ndesc && ndesc[0] != '\0' && str_cmp( ndesc, "(null)" ) )
            pRoomIndex->roomdesc = str_dup( ndesc );
      }

      /*
       * Area number         fread_number( fp ); 
       */
      ln = fread_line( fp );
      x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = x9 = 0;
      sscanf( ln, "%d %d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8, &x9 );

      {
         char *roomflags, *sect;
         char flag[MIL];
         int value;

         roomflags = flag_string( x2, stock_rflags );

         while( roomflags[0] != '\0' )
         {
            roomflags = one_argument( roomflags, flag );
            value = get_rflag( flag );
            if( value < 0 || value >= ROOM_MAX )
               bug( "Bad room flag: %s", flag );
            else
               SET_ROOM_FLAG( pRoomIndex, value );
         }

         sect = stock_sect[x3];
         pRoomIndex->sector_type = get_sectypes( sect );
         pRoomIndex->winter_sector = -1;
      }
      pRoomIndex->tele_delay = x4;
      pRoomIndex->tele_vnum = x5;
      pRoomIndex->tunnel = x6;
      pRoomIndex->light = 0;

      if( pRoomIndex->sector_type < 0 || pRoomIndex->sector_type >= SECT_MAX )
      {
         bug( "%s: vnum %d has bad sector_type %d.", __FUNCTION__, vnum, pRoomIndex->sector_type );
         pRoomIndex->sector_type = 1;
      }

      for( ;; )
      {
         letter = fread_letter( fp );

         if( letter == 'S' )
            break;

         // Smaug resets, applied reset fix. We can cheat some here since we wrote that :)
         if( letter == 'R' && ( tarea->version == 0 || tarea->version == 1 ) )
            load_room_reset( pRoomIndex, fp );

         else if( letter == 'R' && tarea->version == 1000 )  /* SmaugWiz resets */
         {
            exit_data *pexit;
            char letter2;
            int extra, arg1, arg2, arg3, arg4;

            letter2 = fread_letter( fp );
            extra = fread_number( fp );
            arg1 = fread_number( fp );
            arg2 = fread_number( fp );
            arg3 = fread_number( fp );
            arg4 = ( letter2 == 'G' || letter2 == 'R' ) ? 0 : fread_number( fp );
            fread_to_eol( fp );

            ++count;

            /*
             * Validate parameters.
             * We're calling the index functions for the side effect.
             */
            switch ( letter2 )
            {
               default:
                  bug( "%s: SmaugWiz - bad command '%c'.", __FUNCTION__, letter2 );
                  if( fBootDb )
                     boot_log( "%s: %s (%d) bad command '%c'.", __FUNCTION__, tarea->filename, count, letter2 );
                  return;

               case 'M':
                  if( get_mob_index( arg2 ) == NULL && fBootDb )
                     boot_log( "%s: SmaugWiz - %s (%d) 'M': mobile %d doesn't exist.", __FUNCTION__, tarea->filename, count,
                               arg2 );
                  break;

               case 'O':
                  if( get_obj_index( arg2 ) == NULL && fBootDb )
                     boot_log( "%s: SmaugWiz - %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, tarea->filename, count,
                               letter2, arg2 );
                  break;

               case 'P':
                  if( get_obj_index( arg2 ) == NULL && fBootDb )
                     boot_log( "%s: SmaugWiz - %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, tarea->filename, count,
                               letter2, arg2 );
                  if( arg4 > 0 )
                  {
                     if( get_obj_index( arg4 ) == NULL && fBootDb )
                        boot_log( "%s: SmaugWiz - %s (%d) 'P': destination object %d doesn't exist.", __FUNCTION__,
                                  tarea->filename, count, arg4 );
                  }
                  break;

               case 'G':
               case 'E':
                  if( get_obj_index( arg2 ) == NULL && fBootDb )
                     boot_log( "%s: SmaugWiz - %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, tarea->filename, count,
                               letter2, arg2 );
                  break;

               case 'T':
                  break;

               case 'H':
                  if( arg1 > 0 )
                     if( get_obj_index( arg2 ) == NULL && fBootDb )
                        boot_log( "%s: SmaugWiz - %s (%d) 'H': object %d doesn't exist.", __FUNCTION__, tarea->filename, count,
                                  arg2 );
                  break;

               case 'D':
                  if( arg3 < 0 || arg3 > MAX_DIR + 1 || ( pexit = get_exit( pRoomIndex, arg3 ) ) == NULL
                      || !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
                  {
                     bug( "%s: SmaugWiz - 'D': exit %d not door.", __FUNCTION__, arg3 );
                     log_printf( "Reset: %c %d %d %d %d %d", letter2, extra, arg1, arg2, arg3, arg4 );
                     if( fBootDb )
                        boot_log( "%s: SmaugWiz - %s (%d) 'D': exit %d not door.", __FUNCTION__, tarea->filename, count, arg3 );
                  }
                  if( arg4 < 0 || arg4 > 2 )
                  {
                     bug( "%s: 'D': bad 'locks': %d.", __FUNCTION__, arg4 );
                     if( fBootDb )
                        boot_log( "%s: SmaugWiz - %s (%d) 'D': bad 'locks': %d.", __FUNCTION__, tarea->filename, count, arg4 );
                  }
                  break;

               case 'R':
                  if( arg3 < 0 || arg3 > 10 )
                  {
                     bug( "%s: 'R': bad exit %d.", __FUNCTION__, arg3 );
                     if( fBootDb )
                        boot_log( "%s: SmaugWiz - %s (%d) 'R': bad exit %d.", __FUNCTION__, tarea->filename, count, arg3 );
                     break;
                  }
                  break;
            }
            /*
             * Don't bother asking why arg1 isn't passed, SmaugWiz had some purpose for it, but it remains a mystery 
             */
            add_reset( pRoomIndex, letter2, extra, arg2, arg3, arg4, -1, -1, -1 );

         }  /* End SmaugWiz resets */

         else if( letter == 'D' )
         {
            exit_data *pexit;
            int locks;

            door = fread_number( fp );
            if( door < 0 || door > DIR_SOMEWHERE )
            {
               bug( "%s: vnum %d has bad door number %d.", __FUNCTION__, vnum, door );
               if( fBootDb )
                  exit( 1 );
            }
            else
            {
               pexit = make_exit( pRoomIndex, NULL, door );
               pexit->exitdesc = fread_string( fp );
               pexit->keyword = fread_string( fp );
               xCLEAR_BITS( pexit->exit_info );
               ln = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = 0;
               sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );

               locks = x1;
               pexit->key = x2;
               pexit->vnum = x3;
               pexit->vdir = door;
               pexit->x = -1;
               pexit->y = -1;
               pexit->pulltype = x5;
               pexit->pull = x6;

               switch ( locks )
               {
                  case 1:
                     SET_EXIT_FLAG( pexit, EX_ISDOOR );
                     break;
                  case 2:
                     SET_EXIT_FLAG( pexit, EX_ISDOOR );
                     SET_EXIT_FLAG( pexit, EX_PICKPROOF );
                     break;
                  default:
                  {
                     char *oldexits = NULL;
                     char flag[MIL];
                     int value;

                     oldexits = flag_string( locks, stock_ex_flags );

                     while( oldexits[0] != '\0' )
                     {
                        oldexits = one_argument( oldexits, flag );
                        value = get_exflag( flag );
                        if( value < 0 || value >= MAX_EXFLAG )
                           bug( "Bad exit flag: %s", flag );
                        else
                           SET_EXIT_FLAG( pexit, value );
                     }
                  }
               }
            }
         }
         else if( letter == 'E' )
         {
            EXTRA_DESCR_DATA *ed;

            CREATE( ed, EXTRA_DESCR_DATA, 1 );
            ed->keyword = fread_string( fp );
            ed->extradesc = fread_string( fp );
            LINK( ed, pRoomIndex->first_extradesc, pRoomIndex->last_extradesc, next, prev );
            top_ed++;
         }
         else if( letter == 'M' )   /* maps */
            fread_to_eol( fp );  /* Skip, AFKMud doesn't have these */
         else if( letter == '>' )
         {
            ungetc( letter, fp );
            rprog_read_programs( fp, pRoomIndex );
         }
         else
         {
            bug( "%s: vnum %d has flag '%c' not 'RDES'.", __FUNCTION__, vnum, letter );
            shutdown_mud( "Room flag not RDES" );
            exit( 1 );
         }
      }

      if( !oldroom )
      {
         int iHash = vnum % MAX_KEY_HASH;
         pRoomIndex->next = room_index_hash[iHash];
         room_index_hash[iHash] = pRoomIndex;
         LINK( pRoomIndex, tarea->first_room, tarea->last_room, next_aroom, prev_aroom );
         top_room++;
      }
   }
   return;
}

void load_stresets( AREA_DATA *tarea, FILE *fp )
{
   ROOM_INDEX_DATA *pRoomIndex = NULL;
   ROOM_INDEX_DATA *roomlist;
   bool not01 = false;
   int count = 0;

   if( !tarea )
   {
      bug( "%s: no #AREA seen yet.", __FUNCTION__ );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   if( !tarea->first_room )
   {
      bug( "%s: No #ROOMS section found. Cannot load resets.", __FUNCTION__ );
      if( fBootDb )
      {
         shutdown_mud( "No #ROOMS" );
         exit( 1 );
      }
      else
         return;
   }

   for( ;; )
   {
      exit_data *pexit;
      char letter;
      int extra, arg1, arg2, arg3;
      short arg4, arg5, arg6;

      if( ( letter = fread_letter( fp ) ) == 'S' )
         break;

      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }

      extra = fread_number( fp );
      if( letter == 'M' || letter == 'O' )
         extra = 0;
      arg1 = fread_number( fp );
      arg2 = fread_number( fp );
      arg3 = fread_number( fp );
      arg4 = arg5 = arg6 = -1; // Converted resets have no overland coordinates
      fread_to_eol( fp );
      ++count;

      /*
       * Validate parameters.
       * We're calling the index functions for the side effect.
       */
      switch( letter )
      {
         default:
            bug( "%s: bad command '%c'.", __FUNCTION__, letter );
            if( fBootDb )
               boot_log( "%s: %s (%d) bad command '%c'.", __FUNCTION__, tarea->filename, count, letter );
            return;

         case 'M':
            if( get_mob_index( arg1 ) == NULL && fBootDb )
               boot_log( "%s: %s (%d) 'M': mobile %d doesn't exist.", __FUNCTION__, tarea->filename, count, arg1 );

            if( ( pRoomIndex = get_room_index( arg3 ) ) == NULL && fBootDb )
               boot_log( "%s: %s (%d) 'M': room %d doesn't exist.", __FUNCTION__, tarea->filename, count, arg3 );
            else
               add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            break;

         case 'O':
            if( get_obj_index( arg1 ) == NULL && fBootDb )
               boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, tarea->filename, count, letter, arg1 );

            if( ( pRoomIndex = get_room_index( arg3 ) ) == NULL && fBootDb )
               boot_log( "%s: %s (%d) '%c': room %d doesn't exist.", __FUNCTION__, tarea->filename, count, letter, arg3 );
            else
            {
               if( !pRoomIndex )
                  bug( "%s: Unable to add object reset - room not found.", __FUNCTION__ );
               else
                  add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            }
            break;

         case 'P':
            if( get_obj_index( arg1 ) == NULL && fBootDb )
               boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, tarea->filename, count, letter, arg1 );
            if( arg3 > 0 )
            {
               if( get_obj_index( arg3 ) == NULL && fBootDb )
                  boot_log( "%s: %s (%d) 'P': destination object %d doesn't exist.", __FUNCTION__, tarea->filename, count, arg3 );
               if( extra > 1 )
                  not01 = true;
            }
            if( !pRoomIndex )
               bug( "%s: Unable to add put reset - room not found.", __FUNCTION__ );
            else
            {
               if( arg3 == 0 )
                  arg3 = OBJ_VNUM_DUMMYOBJ; // This may look stupid, but for some reason it works.
               add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            }
            break;

         case 'G':
         case 'E':
            if( get_obj_index( arg1 ) == NULL && fBootDb )
               boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, tarea->filename, count, letter, arg1 );
            if( !pRoomIndex )
               bug( "%s: Unable to add give/equip reset - room not found.", __FUNCTION__ );
            else
               add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            break;

         case 'T':
            if( IS_SET( extra, TRAP_OBJ ) )
               bug( "%s: Unable to add legacy object trap reset. Must be converted manually.", __FUNCTION__ );
            else
            {
               if( !( pRoomIndex = get_room_index( arg3 ) ) )
                  bug( "%s: Unable to add trap reset - room not found.", __FUNCTION__ );
               else
                  add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            }
            break;

         case 'H':
            bug( "%s: Unable to convert legacy hide reset. Must be converted manually.", __FUNCTION__ );
            break;

         case 'D':
            if( !( pRoomIndex = get_room_index( arg1 ) ) )
            {
               bug( "%s: 'D': room %d doesn't exist.", __FUNCTION__, arg1 );
               log_printf( "Reset: %c %d %d %d %d", letter, extra, arg1, arg2, arg3 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'D': room %d doesn't exist.", __FUNCTION__, tarea->filename, count, arg1 );
               break;
            }

            if( arg2 < 0 || arg2 > MAX_DIR + 1
                || !( pexit = get_exit( pRoomIndex, arg2 ) ) || !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
            {
               bug( "%s: 'D': exit %d not door.", __FUNCTION__, arg2 );
               log_printf( "Reset: %c %d %d %d %d", letter, extra, arg1, arg2, arg3 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'D': exit %d not door.", __FUNCTION__, tarea->filename, count, arg2 );
            }

            if( arg3 < 0 || arg3 > 2 )
            {
               bug( "%s: 'D': bad 'locks': %d.", __FUNCTION__, arg3 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'D': bad 'locks': %d.", __FUNCTION__, tarea->filename, count, arg3 );
            }
            add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            break;

         case 'R':
            if( !( pRoomIndex = get_room_index( arg1 ) ) && fBootDb )
               boot_log( "%s: %s (%d) 'R': room %d doesn't exist.", __FUNCTION__, tarea->filename, count, arg1 );
            else
               add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3, arg4, arg5, arg6 );
            if( arg2 < 0 || arg2 > 10 )
            {
               bug( "%s: 'R': bad exit %d.", __FUNCTION__, arg2 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'R': bad exit %d.", __FUNCTION__, tarea->filename, count, arg2 );
               break;
            }
            break;
      }
   }

   if( !not01 )
   {
      for( roomlist = tarea->first_room; roomlist; roomlist = roomlist->next_aroom )
         renumber_put_resets( roomlist );
   }
   return;
}

void load_stshops( AREA_DATA * tarea, FILE * fp )
{
   SHOP_DATA *pShop;

   for( ;; )
   {
      MOB_INDEX_DATA *pMobIndex;
      int iTrade;

      CREATE( pShop, SHOP_DATA, 1 );
      pShop->keeper = fread_number( fp );
      if( pShop->keeper == 0 )
      {
         DISPOSE( pShop );
         break;
      }
      for( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
         pShop->buy_type[iTrade] = fread_number( fp );
      pShop->profit_buy = fread_number( fp );
      pShop->profit_sell = fread_number( fp );
      pShop->profit_buy = URANGE( pShop->profit_sell + 5, pShop->profit_buy, 1000 );
      pShop->profit_sell = URANGE( 0, pShop->profit_sell, pShop->profit_buy - 5 );
      pShop->open_hour = fread_number( fp );
      pShop->close_hour = fread_number( fp );
      fread_to_eol( fp );
      pMobIndex = get_mob_index( pShop->keeper );
      pMobIndex->pShop = pShop;

      if( !first_shop )
         first_shop = pShop;
      else
         last_shop->next = pShop;
      pShop->next = NULL;
      pShop->prev = last_shop;
      last_shop = pShop;
      top_shop++;
   }
   return;
}

void load_strepairs( AREA_DATA * tarea, FILE * fp )
{
   REPAIR_DATA *rShop;

   for( ;; )
   {
      MOB_INDEX_DATA *pMobIndex;
      int iFix;

      CREATE( rShop, REPAIR_DATA, 1 );
      rShop->keeper = fread_number( fp );
      if( rShop->keeper == 0 )
      {
         DISPOSE( rShop );
         break;
      }
      for( iFix = 0; iFix < MAX_FIX; iFix++ )
         rShop->fix_type[iFix] = fread_number( fp );
      rShop->profit_fix = fread_number( fp );
      rShop->shop_type = fread_number( fp );
      rShop->open_hour = fread_number( fp );
      rShop->close_hour = fread_number( fp );
      fread_to_eol( fp );
      pMobIndex = get_mob_index( rShop->keeper );
      pMobIndex->rShop = rShop;

      if( !first_repair )
         first_repair = rShop;
      else
         last_repair->next = rShop;
      rShop->next = NULL;
      rShop->prev = last_repair;
      last_repair = rShop;
      top_repair++;
   }
   return;
}

void load_stock_area_file( const char *filename, bool manual )
{
   AREA_DATA *tarea = NULL;
   char *word;

   if( manual )
   {
      char fname[256];

      snprintf( fname, 256, "%s%s", AREA_CONVERT_DIR, filename );
      if( !( fpArea = fopen( fname, "r" ) ) )
      {
         perror( fname );
         bug( "%s: Error locating area file for conversion. Not present in conversion directory.", __FUNCTION__ );
         return;
      }
   }
   else if( !( fpArea = fopen( filename, "r" ) ) )
   {
      perror( filename );
      bug( "%s: error loading file (can't open) %s", __FUNCTION__, filename );
      return;
   }

   if( fread_letter( fpArea ) != '#' )
   {
      if( fBootDb )
      {
         bug( "%s: No # found at start of area file.", __FUNCTION__ );
         exit( 1 );
      }
      else
      {
         log_printf( "%s: No # found at start of area file %s", __FUNCTION__, filename );
         return;
      }
   }
   word = fread_word( fpArea );
   if( !str_cmp( word, "AREA" ) )
   {
      tarea = create_area(  );
      tarea->name = fread_string_nohash( fpArea );
      tarea->author = STRALLOC( "unknown" );
      tarea->filename = str_dup( strArea );
      tarea->version = 0;
   }
   else if( !str_cmp( word, "VERSION" ) )
   {
      int temp = fread_number( fpArea );

      if( temp >= 1000 )
      {
         word = fread_word( fpArea );
         if( !str_cmp( word, "#AREA" ) )
         {
            tarea = create_area(  );
            tarea->name = fread_string_nohash( fpArea );
            tarea->author = STRALLOC( "unknown" );
            tarea->filename = str_dup( strArea );
            tarea->version = temp;
         }
         else
         {
            area_failed = true;
            FCLOSE( fpArea );
            log_printf( "%s: Invalid header at start of area file %s", __FUNCTION__, filename );
            return;
         }
      }
   }
   else
   {
      area_failed = true;
      FCLOSE( fpArea );
      log_printf( "%s: Invalid header at start of area file %s", __FUNCTION__, filename );
      return;
   }
   dotdcheck = 0;

   for( ;; )
   {
      if( manual && area_failed )
      {
         FCLOSE( fpArea );
         return;
      }

      if( fread_letter( fpArea ) != '#' )
      {
         log_printf( "%s: # not found. %s", __FUNCTION__, tarea->filename );
         return;
      }

      word = fread_word( fpArea );

      if( word[0] == '$' )
         break;

      else if( !str_cmp( word, "AUTHOR" ) )
      {
         STRFREE( tarea->author );
         tarea->author = fread_string( fpArea );
      }
      else if( !str_cmp( word, "FLAGS" ) )
      {
         char *ln;
         char *aflags;
         char flag[MIL];
         int x1, x2, value;

         ln = fread_line( fpArea );
         x1 = x2 = 0;
         sscanf( ln, "%d %d", &x1, &x2 );

         aflags = flag_string( x1, stock_area_flags );

         while( aflags[0] != '\0' )
         {
            aflags = one_argument( aflags, flag );
            value = get_areaflag( flag );
            if( value < 0 || value > 31 )
               bug( "Bad area flag: %s", flag );
            else
               SET_BIT( tarea->flags, 1 << value );
         }
         tarea->reset_frequency = x2;
      }
      else if( !str_cmp( word, "RANGES" ) )
      {
         int x1, x2, x3, x4;
         char *ln;

         for( ;; )
         {
            ln = fread_line( fpArea );

            if( ln[0] == '$' )
               break;

            x1 = x2 = x3 = x4 = 0;
            sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );

            tarea->low_soft_range = x1;
            tarea->hi_soft_range = x2;
            tarea->low_hard_range = x3;
            tarea->hi_hard_range = x4;
         }
      }
      else if( !str_cmp( word, "ECONOMY" ) )
      {
         /*
          * Not that these values are used anymore, but hey. 
          */
         fread_number( fpArea );
         fread_number( fpArea );
      }
      else if( !str_cmp( word, "RESETMSG" ) )
      {
         DISPOSE( tarea->resetmsg );
         tarea->resetmsg = fread_string_nohash( fpArea );
      }
      /*
       * Rennard 
       */
      else if( !str_cmp( word, "HELPS" ) )
      {
         help_data *pHelp;

         for( ;; )
         {
            pHelp = new help_data;
            pHelp->level = fread_number( fpArea );
            pHelp->keyword = fread_string_nohash( fpArea );
            if( pHelp->keyword[0] == '$' )
            {
               free_help( pHelp );
               break;
            }
            pHelp->text = fread_string_nohash( fpArea );
            if( pHelp->keyword[0] == '\0' )
            {
               free_help( pHelp );
               continue;
            }
            add_help( pHelp );
         }
      }
      else if( !str_cmp( word, "MOBILES" ) )
         load_stmobiles( tarea, fpArea, manual );
      else if( !str_cmp( word, "OBJECTS" ) )
         load_stobjects( tarea, fpArea, manual );
      else if( !str_cmp( word, "RESETS" ) )
         load_stresets( tarea, fpArea );
      else if( !str_cmp( word, "ROOMS" ) )
         load_strooms( tarea, fpArea, manual );
      else if( !str_cmp( word, "SHOPS" ) )
         load_stshops( tarea, fpArea );
      else if( !str_cmp( word, "REPAIRS" ) )
         load_strepairs( tarea, fpArea );
      else if( !str_cmp( word, "SPECIALS" ) )
      {
         bool done = false;

         for( ;; )
         {
            MOB_INDEX_DATA *pMobIndex;
            char *temp;
            char letter;

            switch ( letter = fread_letter( fpArea ) )
            {
               default:
                  bug( "%s: letter '%c' not *MORS.", __FUNCTION__, letter );
                  exit( 1 );

               case 'S':
                  done = true;
                  break;

               case '*':
               case 'O':
               case 'R':
                  break;

               case 'M':
                  pMobIndex = get_mob_index( fread_number( fpArea ) );
                  temp = fread_word( fpArea );
                  if( !pMobIndex )
                  {
                     bug( "%s: 'M': Invalid mob vnum!", __FUNCTION__ );
                     break;
                  }
                  if( !( pMobIndex->spec_fun = m_spec_lookup( temp ) ) )
                  {
                     bug( "%s: 'M': vnum %d, no spec_fun called %s.", __FUNCTION__, pMobIndex->vnum, temp );
                     pMobIndex->spec_funname = NULL;
                  }
                  else
                     pMobIndex->spec_funname = STRALLOC( temp );
                  break;
            }
            if( done )
               break;
            fread_to_eol( fpArea );
         }
      }
      else if( !str_cmp( word, "CLIMATE" ) )
      {
         char *ln;
         int x1, x2, x3, x4;

         if( dotdcheck > 0 && dotdcheck < 4 )
         {
            bug( "DOTDII area encountered with invalid header format, check value %d", dotdcheck );
            shutdown_mud( "Invalid DOTDII area" );
            exit( 1 );
         }

         ln = fread_line( fpArea );
         x1 = x2 = x3 = x4 = 0;
         sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );

         tarea->weather->climate_temp = x1;
         tarea->weather->climate_precip = x2;
         tarea->weather->climate_wind = x3;
      }
      else if( !str_cmp( word, "NEIGHBOR" ) )
      {
         neighbor_data *anew;

         CREATE( anew, NEIGHBOR_DATA, 1 );
         anew->next = NULL;
         anew->prev = NULL;
         anew->address = NULL;
         anew->name = fread_string( fpArea );
         LINK( anew, tarea->weather->first_neighbor, tarea->weather->last_neighbor, next, prev );
      }
      else if( !str_cmp( word, "VERSION" ) )
      {
         int area_version = fread_number( fpArea );

         if( ( area_version < 0 || area_version > 1 ) && area_version != 1000 )
         {
            area_failed = true;
            bug( "%s: Version %d in %s is non-stock area format. Unable to process.", __FUNCTION__, area_version, filename );
            if( !manual )
            {
               shutdown_mud( "Non-standard area format" );
               exit( 1 );
            }
            close_area( tarea );
            --top_area;
            return;
         }
         tarea->version = area_version;
      }
      else if( !str_cmp( word, "SPELLLIMIT" ) )
         fread_number( fpArea ); /* Skip, AFKMud doesn't have this */
      else if( !str_cmp( word, "DERIVATIVES" ) )   /* Chronicles tag, safe to skip */
         fread_to_eol( fpArea );
      else if( !str_cmp( word, "NAMEFORMAT" ) ) /* Chronicles tag, safe to skip */
         fread_to_eol( fpArea );
      else if( !str_cmp( word, "DESCFORMAT" ) ) /* Chronicles tag, safe to skip */
         fread_to_eol( fpArea );
      else if( !str_cmp( word, "PLANE" ) )   /* DOTD 2.3.6 tag, safe to skip */
      {
         ++dotdcheck;
         fread_number( fpArea );
      }
      else if( !str_cmp( word, "CURRENCY" ) )   /* DOTD 2.3.6 tag, safe to skip */
      {
         ++dotdcheck;
         fread_to_eol( fpArea );
      }
      else if( !str_cmp( word, "HIGHECONOMY" ) )   /* DOTD 2.3.6 tag, safe to skip */
      {
         ++dotdcheck;
         fread_to_eol( fpArea );
      }
      else if( !str_cmp( word, "LOWECONOMY" ) ) /* DOTD 2.3.6 tag, safe to skip */
      {
         ++dotdcheck;
         fread_to_eol( fpArea );
      }
      else
      {
         bug( "%s: %s: bad section name.", __FUNCTION__, tarea->filename );
         if( fBootDb )
         {
            shutdown_mud( "Corrupted area file" );
            exit( 1 );
         }
         else
         {
            FCLOSE( fpArea );
            return;
         }
      }
   }
   FCLOSE( fpArea );
   if( tarea )
   {
      sort_area_name( tarea );
      sort_area_vnums( tarea );
      if( tarea->version == 0 && !dotdcheck )
         log_printf( "%-20s: Converted Smaug 1.02a Zone  Vnums: %5d - %-5d", tarea->filename, tarea->low_vnum, tarea->hi_vnum );
      else if( tarea->version == 1 && !dotdcheck )
         log_printf( "%-20s: Converted Smaug 1.4a Zone  Vnums: %5d - %-5d", tarea->filename, tarea->low_vnum, tarea->hi_vnum );
      else if( dotdcheck )
         log_printf( "%-20s: Converted DOTDII 2.3.6 Zone  Vnums: %5d - %-5d", tarea->filename, tarea->low_vnum, tarea->hi_vnum );
      else
         log_printf( "%-20s: Converted SmaugWiz Zone  Vnums: %5d - %-5d", tarea->filename, tarea->low_vnum, tarea->hi_vnum );

      if( tarea->low_vnum < 0 || tarea->hi_vnum < 0 )
         bug( "%-20s: Bad Vnum Range", tarea->filename );
      if( !tarea->author )
         tarea->author = STRALLOC( "Unknown" );
   }
   else
      log_printf( "(%s)", filename );

   return;
}

/* Use of the forceload argument with this command isn't recommended
 * unless you KNOW for sure that what you're doing will be safe.
 * Trying to force something that is broken *WILL* result in a crashed mud.
 * The argument was added as a laziness feature for the installation of new
 * zones built on a builders' port and transferred to the main port either via
 * the shell code or some other means. IT IS NOT MEANT TO DO ANYTHING ELSE!
 * Omitting it from the helpfile was deliberate.
 * You've been warned. Broken muds are not our fault.
 */
CMDF do_areaconvert( CHAR_DATA * ch, char *argument )
{
   AREA_DATA *tarea = NULL;
   int tmp;
   bool manual;
   char arg[MIL];

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      if( ch )
         send_to_char( "Convert what zone?\r\n", ch );
      else
         bug( "%s: Attempt made to convert with no filename.", __FUNCTION__ );
      return;
   }

   area_failed = false;

   mudstrlcpy( strArea, arg, MIL );

   if( ch )
   {
      fBootDb = true;
      manual = true;
      send_to_char( "&RReading in area file...\r\n", ch );
   }
   else
      manual = false;
   if( argument && !str_cmp( argument, "forceload" ) )
      load_area_file( last_area, strArea, false );
   else
      load_stock_area_file( arg, manual );
   if( ch )
      fBootDb = false;

   if( ch )
   {
      if( area_failed )
      {
         send_to_char( "&YArea conversion failed! See your logs for why.\r\n", ch );
         return;
      }

      if( ( tarea = find_area( arg ) ) )
      {
         send_to_char( "&YLinking exits...\r\n", ch );
         fix_area_exits( tarea );
         if( tarea->first_room )
         {
            tmp = tarea->nplayer;
            tarea->nplayer = 0;
            send_to_char( "&YResetting area...\r\n", ch );
            reset_area( tarea );
            tarea->nplayer = tmp;
         }
         if( tarea->version < 2 || tarea->version == 1000 )
         {
            send_to_char( "&GWriting area in AFKMud format...\r\n", ch );
            fold_area( tarea, tarea->filename, FALSE );
            send_to_char( "&YArea conversion complete.\r\n", ch );
         }
         else
            ch_printf( ch, "&GForced load on %s complete.\r\n", tarea->filename );
         write_area_list(  );
      }
   }
   return;
}
