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
 *                     Completely Revised Boards Module                     *
 ****************************************************************************
 * Revised by:   Xorith                                                     *
 * Last Updated: 10/2/03                                                    *
 ****************************************************************************/

#define BOARD_FILE "boards.lst"  /* New board file */
#define OLD_BOARD_FILE "boards.txt" /* Old board file */
#define PROJECTS_FILE SYSTEM_DIR "projects.txt" /* For projects    */
#define MAX_REPLY 10 /* How many messages in each level? */
#define MAX_BOARD_EXPIRE 180  /* Max days notes have to live. */

#define IS_BOARD_FLAG( board, flag )      ( xIS_SET((board)->flags, (flag)) )

typedef struct board_data BOARD_DATA;
typedef struct note_data NOTE_DATA;
typedef struct project_data PROJECT_DATA;
typedef struct board_chardata BOARD_CHARDATA;

extern BOARD_DATA *first_board;
extern BOARD_DATA *last_board;
extern PROJECT_DATA *first_project;
extern PROJECT_DATA *last_project;

#define BD_IGNORE 2
#define BD_ANNOUNCE 1

/* As a safety precaution, players who are writing a note are moved here... */
#define ROOM_VNUM_BOARD ROOM_VNUM_LIMBO

typedef enum
{
   BOARD_R1, BOARD_BU_PRUNED, BOARD_PRIVATE, BOARD_ANNOUNCE, MAX_BOARD_FLAGS
} bflags;


/* Note Data */
struct note_data
{
   NOTE_DATA *next;
   NOTE_DATA *prev;
   NOTE_DATA *first_reply;
   NOTE_DATA *last_reply;
   NOTE_DATA *parent;
   char *sender;
   char *to_list;
   char *subject;
   char *text;
   short reply_count;  /* Keep a count of our replies */
   short expire; /* Global Board Use */
   time_t date_stamp;   /* Global Board Use */
};

struct board_data
{
   BOARD_DATA *next; /* Next Board */
   BOARD_DATA *prev; /* Prev Board */
   NOTE_DATA *first_note;  /* First Note on Board */
   NOTE_DATA *last_note;   /* Last Note on Board */
   int objvnum;   /* Object Vnum of a physical board */
   EXT_BV flags;  /* Board Flags */
   char *name; /* Name of Board */
   char *filename;   /* Filename for the board */
   char *desc; /* Short description of the board */
   char *readers; /* Readers */
   char *posters; /* Posers */
   char *moderators; /* Moderators of this board */
   char *group;   /* In-Game organization that 'owns' the board */
   short read_level;   /* Minimum Level to read this board */
   short post_level;   /* Minimum Level to post this board */
   short remove_level; /* Minimum Level to remove a post */
   short msg_count; /* Quick count of messages */
   short expire; /* Days until notes are archived. */
};

/* Project Data */
struct project_data
{
   PROJECT_DATA *next;  /* Next project in list       */
   PROJECT_DATA *prev;  /* Previous project in list      */
   NOTE_DATA *first_log;   /* First log on project       */
   NOTE_DATA *last_log; /* Last log  on project       */
   char *name;
   char *owner;
   char *coder;
   char *status;
   char *description;
   bool taken; /* Has someone taken project?      */
   short type;   /* Type of Project -- XORITH */
   time_t date_stamp;
};

struct board_chardata
{
   BOARD_CHARDATA *next;
   BOARD_CHARDATA *prev;
   char *board_name;
   time_t last_read;
   short alert;
};
