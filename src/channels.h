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
 *                          Dynamic Channel System                          *
 ****************************************************************************/

#define MAX_CHANHISTORY 20
#define CHANNEL_FILE SYSTEM_DIR "channels.dat"

typedef struct mud_channel MUD_CHANNEL;

extern MUD_CHANNEL *first_channel;
extern MUD_CHANNEL *last_channel;

typedef enum
{
   CHAN_GLOBAL, CHAN_ZONE, CHAN_GUILD, CHAN_COUNCIL, CHAN_PK, CHAN_LOG
} channel_types;

#define IS_CHANFLAG(var, bit)       IS_SET((var)->flags, (bit))
#define SET_CHANFLAG(var, bit)      SET_BIT((var)->flags, (bit))
#define REMOVE_CHANFLAG(var, bit)   REMOVE_BIT((var)->flags, (bit))

#define     CHAN_KEEPHISTORY  BV00
#define     CHAN_INTERPORT    BV01  /* Can be defined, but will only operate when multiport is enabled */

struct mud_channel
{
   MUD_CHANNEL *next;
   MUD_CHANNEL *prev;
   char *name;
   char *colorname;
   char *history[MAX_CHANHISTORY][2];  /* Not saved */
   int hlevel[MAX_CHANHISTORY];  /* Not saved */
   int hinvis[MAX_CHANHISTORY];  /* Not saved */
   time_t htime[MAX_CHANHISTORY];   /* Not saved *//* Xorith */
   int level;
   int type;
   int flags;
};
