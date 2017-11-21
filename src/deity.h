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

#define DEITY_DIR "../deity/" /* Deity data dir      */
#define DEITY_LIST "deity.lst"   /* List of deities     */

#define IS_DEVOTED(ch) ( !IS_NPC((ch)) && (ch)->pcdata->deity )

typedef struct deity_data DEITY_DATA;

extern DEITY_DATA *first_deity;
extern DEITY_DATA *last_deity;

struct deity_data
{
   DEITY_DATA *next;
   DEITY_DATA *prev;
   char *filename;
   char *name;
   char *deitydesc;
   short alignment;
   short worshippers;
   short scorpse;
   short sdeityobj;
   short sdeityobj2;
   short savatar;
   short smount; /* Added by Tarl 24 Feb 02 */
   short sminion;   /* Added by Tarl 24 Feb 02 */
   short sspell1;   /* Added by Tarl 24 Mar 02 */
   short sspell2;   /* Added by Tarl 24 Mar 02 */
   short sspell3;   /* Added by Tarl 24 Mar 02 */
   short srecall;
   short flee;
   short flee_npcrace;
   short flee_npcrace2;
   short flee_npcrace3;
   short flee_npcfoe;
   short flee_npcfoe2;
   short flee_npcfoe3;
   short kill;
   short kill_magic;
   short kill_npcrace;
   short kill_npcrace2;
   short kill_npcrace3;
   short kill_npcfoe;
   short kill_npcfoe2;
   short kill_npcfoe3;
   short sac;
   short bury_corpse;
   short aid_spell;
   short aid;
   short backstab;
   short steal;
   short die;
   short die_npcrace;
   short die_npcrace2;
   short die_npcrace3;
   short die_npcfoe;
   short die_npcfoe2;
   short die_npcfoe3;
   short spell_aid;
   short dig_corpse;
   int race;
   int race2;
   int race3;
   int race4;
   int Class;
   int Class2;
   int Class3;
   int Class4;
   int element;
   int element2;  /* Added by Tarl 24 Feb 02 */
   int element3;  /* Added by Tarl 24 Feb 02 */
   int sex;
   EXT_BV affected;
   EXT_BV affected2; /* Added by Tarl 24 Feb 02 */
   EXT_BV affected3; /* Added by Tarl 24 Feb 02 */
   int npcrace;
   int npcrace2;
   int npcrace3;
   int npcfoe;
   int npcfoe2;
   int npcfoe3;
   int suscept;
   int suscept2;  /* Added by Tarl 24 Feb 02 */
   int suscept3;  /* Added by Tarl 24 Feb 02 */
   int susceptnum;
   int susceptnum2;  /* Added by Tarl 24 Feb 02 */
   int susceptnum3;  /* Added by Tarl 24 Feb 02 */
   int elementnum;
   int elementnum2;  /* Added by Tarl 24 Feb 02 */
   int elementnum3;  /* Added by Tarl 24 Feb 02 */
   int affectednum;
   int affectednum2; /* Added by Tarl 24 Feb 02 */
   int affectednum3; /* Added by Tarl 24 Feb 02 */
   short spell1; /* Added by Tarl 24 Mar 02 */
   short spell2; /* Added by Tarl 24 Mar 02 */
   short spell3; /* Added by Tarl 24 Mar 02 */
   int objstat;
   int recallroom;   /* Samson 4-13-98 */
   int avatar; /* Restored by Samson 8-1-98 */
   int mount;  /* Added by Tarl 24 Feb 02 */
   int minion; /* Added by Tarl 24 Feb 02 */
   int deityobj;  /* Restored by Samson 8-1-98 */
   int deityobj2;
};

void save_deity( DEITY_DATA * deity );
