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
 *                       Xorith's Connection History                        *
 ****************************************************************************/

/* ConnHistory Feature (header)
 *
 * Based loosely on Samson's Channel History functions. (basic idea)
 * Written by: Xorith 5/7/03, last updated: 9/20/03
 *
 * Stores connection data in an array so that it can be reviewed later.
 *
 */

/* Max number of connections to keep in the history.
 * Don't set this too high... */
#define MAX_CONNHISTORY 30

/* Change this for your codebase! Currently set for AFKMud */
#define CH_LVL_ADMIN LEVEL_ADMIN

/* Path to the conn.hst file */
/* default is: ../system/conn.hst */
#define CH_FILE SYSTEM_DIR "conn.hst"

/* ConnType's for Connection History
 * Be sure to add new types into the update_connhistory function! */
#define CONNTYPE_LOGIN 		0
#define CONNTYPE_QUIT 		1
#define CONNTYPE_IDLE 		2
#define CONNTYPE_LINKDEAD 	3
#define CONNTYPE_NEWPLYR	4
#define CONNTYPE_RECONN		5

/* conn history checking error codes */
#define CHK_CONN_OK 1
#define CHK_CONN_REMOVED 2

typedef struct conn_data CONN_DATA;
extern CONN_DATA *first_conn;
extern CONN_DATA *last_conn;

struct conn_data
{
   CONN_DATA *next;
   CONN_DATA *prev;
   char *user;
   char *when;
   char *host;
   int type;
   int level;
   int invis_lvl;
};
