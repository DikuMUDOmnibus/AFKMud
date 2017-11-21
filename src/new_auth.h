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
 *                      New Name Authorization module                       *
 ****************************************************************************/

#define AUTH_FILE SYSTEM_DIR "auth.dat"
#define RESERVED_LIST SYSTEM_DIR "reserved.lst" /* List of reserved names  */

typedef struct auth_list AUTH_LIST; /* new auth -- Rantic */

/* new auth -- Rantic */
extern AUTH_LIST *first_auth_name;
extern AUTH_LIST *last_auth_name;

/* New auth stuff --Rantic */
typedef enum
{
   AUTH_ONLINE = 0, AUTH_OFFLINE, AUTH_LINK_DEAD, AUTH_CHANGE_NAME, AUTH_unused, AUTH_AUTHED
} auth_types;

/* new auth -- Rantic */
#define NOT_AUTHED(ch) ( get_auth_state((ch)) != AUTH_AUTHED && IS_PCFLAG( (ch), PCFLAG_UNAUTHED ) )
#define IS_WAITING_FOR_AUTH(ch) ( (ch)->desc && get_auth_state((ch)) == AUTH_ONLINE && IS_PCFLAG( (ch), PCFLAG_UNAUTHED ) )

struct auth_list
{
   AUTH_LIST *next;
   AUTH_LIST *prev;
   char *name; /* Name of character awaiting authorization */
   short state;  /* Current state of authed */
   char *authed_by;  /* Name of immortal who authorized the name */
   char *change_by;  /* Name of immortal requesting name change */
};
