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
 *                             Player ships                                 *
 *                   Brought over from Lands of Altanos                     *
 ***************************************************************************/

typedef struct ship_data SHIP_DATA;

extern SHIP_DATA *first_ship;
extern SHIP_DATA *last_ship;
extern char *const ship_flags[];

struct ship_data
{
   SHIP_DATA *next;
   SHIP_DATA *prev;
   char *name;
   char *owner;
   int fuel;
   int max_fuel;
   int hull;
   int max_hull;
   int type;
   int vnum;
   EXT_BV flags;
   short x;
   short y;
   short map;
   int room;
};

typedef enum
{
   SHIP_ANCHORED, SHIP_ONMAP, SHIP_AIRSHIP
} ship_flaggies;

typedef enum
{
   SHIP_NONE, SHIP_SKIFF, SHIP_COASTER, SHIP_CARAVEL, SHIP_GALLEON, SHIP_WARSHIP, SHIP_MAX
} ship_types;

#define IS_SHIP_FLAG(var, bit) 	xIS_SET((var)->flags, (bit))
#define SET_SHIP_FLAG(var, bit) 	xSET_BIT((var)->flags, (bit))
#define REMOVE_SHIP_FLAG(var, bit)	xREMOVE_BIT((var)->flags, (bit))

#define SHIP_FILE       SYSTEM_DIR "ships.dat"  /* For ships */
