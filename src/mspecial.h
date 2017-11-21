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
 *                   "Special procedure" module for Mobs                    *
 ****************************************************************************/

/******************************************************
            Desolation of the Dragon MUD II
      (C) 1997, 1998  Jesse DeFer and Heath Leach
 http://dotd.mudservices.com  dotd@dotd.mudservices.com 
 ******************************************************/

/* Any spec_fun added here needs to be added to specfuns.dat as well.
 * If you don't know what that means, ask Samson to take care of it.
 */

/* spec_fun support for dlsym, so that the list can be restricted - Samson */

typedef struct specfun_list SPEC_LIST;

extern SPEC_LIST *first_specfun;
extern SPEC_LIST *last_specfun;

struct specfun_list
{
   SPEC_LIST *next;
   SPEC_LIST *prev;
   char *name;
};

/* Generic Mobs */
SPECF spec_janitor( CHAR_DATA * ch );  /* Scavenges trash */
SPECF spec_snake( CHAR_DATA * ch ); /* Poisons people with its bite */
SPECF spec_poison( CHAR_DATA * ch );   /* For area conversion compatibility - DON'T REMOVE THIS */
SPECF spec_fido( CHAR_DATA * ch );  /* Eats corpses */
SPECF spec_cast_adept( CHAR_DATA * ch );  /* For healer mobs */
SPECF spec_RustMonster( CHAR_DATA * ch ); /* Eats anything on the ground */

/* Generic Cityguards */
SPECF spec_GenericCityguard( CHAR_DATA * ch );
SPECF spec_guard( CHAR_DATA * ch );

/* Generic Citizens */
SPECF spec_GenericCitizen( CHAR_DATA * ch );

/* Class Procs */
SPECF spec_warrior( CHAR_DATA * ch );  /* Warriors */
SPECF spec_thief( CHAR_DATA * ch ); /* Rogues */
SPECF spec_cast_mage( CHAR_DATA * ch );   /* Mages */
SPECF spec_cast_cleric( CHAR_DATA * ch ); /* Clerics */
SPECF spec_cast_undead( CHAR_DATA * ch ); /* Necromancers */
SPECF spec_ranger( CHAR_DATA * ch );   /* Rangers */
SPECF spec_paladin( CHAR_DATA * ch );  /* Paladins */
SPECF spec_druid( CHAR_DATA * ch ); /* Druids */
SPECF spec_antipaladin( CHAR_DATA * ch ); /* Antipaladins */
SPECF spec_bard( CHAR_DATA * ch );  /* Bards */

/* Dragon stuff */
SPECF spec_breath_any( CHAR_DATA * ch );
SPECF spec_breath_acid( CHAR_DATA * ch );
SPECF spec_breath_fire( CHAR_DATA * ch );
SPECF spec_breath_frost( CHAR_DATA * ch );
SPECF spec_breath_gas( CHAR_DATA * ch );
SPECF spec_breath_lightning( CHAR_DATA * ch );
