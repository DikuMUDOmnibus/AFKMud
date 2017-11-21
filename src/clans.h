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
 *                           Special clan module                            *
 ****************************************************************************/

#define CLAN_DIR "../clans/"  /* Clan data dir     */
#define COUNCIL_DIR "../councils/"  /* Council data dir  */
#define CLAN_LIST "clan.lst"  /* List of clans     */
#define COUNCIL_LIST "council.lst"  /* List of councils     */

typedef struct clan_data CLAN_DATA;
typedef struct council_data COUNCIL_DATA;

/* Clan roster additions */
typedef struct member_data MEMBER_DATA;   /* Individual member data */
typedef struct member_list MEMBER_LIST;   /* List of members in clan */
typedef struct membersort_data MS_DATA;   /* List for sorted roster list */

extern CLAN_DATA *first_clan;
extern CLAN_DATA *last_clan;
extern COUNCIL_DATA *first_council;
extern COUNCIL_DATA *last_council;
extern MEMBER_LIST *first_member_list;
extern MEMBER_LIST *last_member_list;

/* Modified from original form - Samson */
/* Eliminated clan_type GUILD 11-30-98 */
typedef enum
{
   CLAN_PLAIN, CLAN_NOKILL, CLAN_ORDER
} clan_types;

#define IS_CLANNED(ch) (!IS_NPC((ch)) && (ch)->pcdata->clan && (ch)->pcdata->clan->clan_type != CLAN_ORDER )
#define IS_GUILDED(ch) (!IS_NPC((ch)) && (ch)->pcdata->clan && (ch)->pcdata->clan->clan_type == CLAN_ORDER )
#define IS_LEADER(ch)  ( !IS_NPC((ch)) && (ch)->pcdata->clan && is_name( (ch)->name, (ch)->pcdata->clan->leader  ) )
#define IS_NUMBER1(ch) ( !IS_NPC((ch)) && (ch)->pcdata->clan && is_name( (ch)->name, (ch)->pcdata->clan->number1 ) )
#define IS_NUMBER2(ch) ( !IS_NPC((ch)) && (ch)->pcdata->clan && is_name( (ch)->name, (ch)->pcdata->clan->number2 ) )

struct clan_data
{
   CLAN_DATA *next;  /* next clan in list       */
   CLAN_DATA *prev;  /* previous clan in list      */
   char *filename;   /* Clan filename        */
   char *name; /* Clan name            */
   char *motto;   /* Clan motto           */
   char *clandesc;   /* A brief description of the clan  */
   char *deity;   /* Clan's deity            */
   char *leader;  /* Head clan leader        */
   char *number1; /* First officer        */
   char *number2; /* Second officer       */
   char *badge;   /* Clan badge on who/where/to_room      */
   char *leadrank;   /* Leader's rank        */
   char *onerank; /* Number One's rank       */
   char *tworank; /* Number Two's rank       */
   int pkills[10];   /* Number of pkills on behalf of clan  */
   int pdeaths[10];  /* Number of pkills against clan */
   int mkills; /* Number of mkills on behalf of clan  */
   int mdeaths;   /* Number of clan deaths due to mobs   */
   int illegal_pk;   /* Number of illegal pk's by clan   */
   int score;  /* Overall score        */
   short clan_type; /* See clan type defines      */
   short favour; /* Deities favour upon the clan     */
   short strikes;   /* Number of strikes against the clan  */
   short members;   /* Number of clan members     */
   short mem_limit; /* Number of clan members allowed   */
   short alignment; /* Clan's general alignment      */
   bool getleader;   /* TRUE if they need to have the code provide a new leader */
   bool getone;   /* TRUE if they need to have the code provide a new number1 */
   bool gettwo;   /* TRUE if they need to have the code provide a new number2 */
   int board;  /* Vnum of clan board         */
   int clanobj1;  /* Vnum of first clan obj     */
   int clanobj2;  /* Vnum of second clan obj    */
   int clanobj3;  /* Vnum of third clan obj     */
   int clanobj4;  /* Vnum of fourth clan obj    */
   int clanobj5;  /* Vnum of fifth clan obj     */
   int recall; /* Vnum of clan's recall room    */
   int storeroom; /* Vnum of clan's store room     */
   int guard1; /* Vnum of clan guard type 1     */
   int guard2; /* Vnum of clan guard type 2     */
   int Class;  /* For guilds           */
   int tithe;  /* Percentage of gold sent to clan account after battle */
   int balance;   /* Clan's bank account balance */
   int inn; /* Vnum for clan's inn if they own one */
   int idmob;  /* Vnum for clan's Identifying mobile if they have one */
   int shopkeeper;   /* Vnum for clan shopkeeper if they have one */
   int auction;   /* Vnum for clan auctioneer */
   int bank;   /* Vnum for clan banker */
   int repair; /* Vnum for clan repairsmith */
   int forge;  /* Vnum for clan forgesmith */
};

struct council_data
{
   COUNCIL_DATA *next;  /* next council in list       */
   COUNCIL_DATA *prev;  /* previous council in list      */
   char *filename;   /* Council filename        */
   char *name; /* Council name            */
   char *councildesc;   /* A brief description of the council  */
   char *head; /* Council head         */
   char *head2;   /* Council co-head                      */
   char *powers;  /* Council powers       */
   short members;   /* Number of council members     */
   int board;  /* Vnum of council board      */
   int meeting;   /* Vnum of council's meeting room   */
};

struct membersort_data
{
   MS_DATA *next;
   MS_DATA *prev;
   MEMBER_DATA *member;
};

struct member_data
{
   MEMBER_DATA *next;   /* Next member */
   MEMBER_DATA *prev;   /* Prev member */
   char *name; /* Name of member */
   char *since;   /* Member since */
   int Class;  /* class of member */
   int level;  /* level of member */
   int deaths; /* Pdeaths for clans, mdeaths for guilds/orders */
   int kills;  /* Pkills for clans, mkills for guilds/orders */
};

struct member_list
{
   MEMBER_DATA *first_member; /* First Member */
   MEMBER_DATA *last_member;  /* Last Member */
   MEMBER_LIST *next;   /* Next clan */
   MEMBER_LIST *prev;   /* Prev clan */
   char *name; /* Clan name */
};
