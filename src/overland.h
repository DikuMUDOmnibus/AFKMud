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
 *	                     Overland ANSI Map Module                         *
 *                      Created by Samson of Alsherok                       *
 ****************************************************************************/

#define OVERLAND_ALSHEROK 50000
#define OVERLAND_ALATIA 50500
#define OVERLAND_ELETAR 51000

#define MAX_X 1000
#define MAX_Y 1000

/* Change these filenames to match yours */
#define ENTRANCE_FILE "entrances.dat"
#define LANDMARK_FILE "landmarks.dat"

typedef enum
{
   MAP_ALSHEROK, MAP_ELETAR, MAP_ALATIA, MAP_MAX
} map_types;

/* 3 maps, each of 1000x1000 rooms, starting from 0,0 as the NW corner */
extern unsigned char map_sector[MAP_MAX][MAX_X][MAX_Y];
extern char *const map_names[];
extern char *const map_name[];
extern char *const continents[];
extern char *const sect_types[];
extern const struct sect_color_type sect_show[];

typedef struct entrance_data ENTRANCE_DATA;
typedef struct landmark_data LANDMARK_DATA;

extern ENTRANCE_DATA *first_entrance;
extern ENTRANCE_DATA *last_entrance;
extern LANDMARK_DATA *first_landmark;
extern LANDMARK_DATA *last_landmark;

struct landmark_data
{
   LANDMARK_DATA *next;
   LANDMARK_DATA *prev;
   short map; /* Map the landmark is on */
   short x;   /* X coordinate of landmark */
   short y;   /* Y coordinate of landmark */
   int distance;  /* Distance the landmark is visible from */
   char *description;   /* Description of the landmark */
   bool Isdesc;   /* If TRUE is room desc. If not is landmark */
};

struct entrance_data
{
   ENTRANCE_DATA *next;
   ENTRANCE_DATA *prev;
   short herex;  /* Coordinates the entrance is at */
   short herey;
   short therex; /* Coordinates the entrance goes to, if any */
   short therey;
   short tomap;  /* Map it goes to, if any */
   short onmap;  /* Which map it's on */
   int vnum;   /* Target vnum if it goes to a regular zone */
   short prevsector;   /* Previous sector type to restore with when an exit is deleted */
   char *area; /* Area name */
};

struct sect_color_type
{
   short sector; /* Terrain sector */
   char *color;   /* Color to display as */
   char *symbol;  /* Symbol you see for the sector */
   char *desc; /* Description of sector type */
   bool canpass;  /* Impassable terrain */
   int move;   /* Movement loss */
   short graph1; /* Color numbers for graphic conversion */
   short graph2;
   short graph3;
};

short get_terrain( short map, short x, short y );
ENTRANCE_DATA *check_entrance( short map, short x, short y );
void delete_entrance( ENTRANCE_DATA * enter );
void putterr( short map, short x, short y, short terr );
ch_ret process_exit( CHAR_DATA * ch, short map, short x, short y, int dir, bool running );
double distance( short chX, short chY, short lmX, short lmY );
double calc_angle( short chX, short chY, short lmX, short lmY, double *ipDistan );
