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
 *                            Ban module by Shaddai                         *
 ****************************************************************************/

#define BAN_LIST "ban.lst" /* List of bans                 */

/*
 * The watch directory contains a maximum of one file for each immortal
 * that contains output from "player watches". The name of each file
 * in this directory is the name of the immortal who requested the watch
 */
#define WATCH_DIR "../watch/" /* Imm watch files --Gorog      */
#define WATCH_LIST "watch.lst"   /* List of watches              */

typedef struct ban_data BAN_DATA;
typedef struct watch_data WATCH_DATA;

extern BAN_DATA *first_ban;
extern BAN_DATA *last_ban;
extern WATCH_DATA *first_watch;
extern WATCH_DATA *last_watch;

 /*
  * Ban Types --- Shaddai
  */
#define BAN_WARN -1

/*
 * Site ban structure.
 */
struct ban_data
{
   BAN_DATA *next;
   BAN_DATA *prev;
   char *name; /* Name of site/class/race banned */
   char *user; /* Name of user from site */
   char *note; /* Why it was banned */
   char *ban_by;  /* Who banned this site */
   char *ban_time;   /* Time it was banned */
   int unban_date;   /* When ban expires */
   short duration;  /* How long it is banned for */
   short level;  /* Level that is banned */
   bool warn;  /* Echo on warn channel */
   bool prefix;   /* Use of *site */
   bool suffix;   /* Use of site* */
};

/*
 * Player watch data structure  --Gorog
 */
struct watch_data
{
   WATCH_DATA *next;
   WATCH_DATA *prev;
   short imm_level;
   char *imm_name;   /* imm doing the watching */
   char *target_name;   /* player or command being watched   */
   char *player_site;   /* site being watched     */
};

bool check_bans( CHAR_DATA * ch );
