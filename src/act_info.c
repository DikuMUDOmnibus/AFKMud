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
 *                          Informational module                            *
 ****************************************************************************/

#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include "mud.h"
#include "clans.h"
#include "fight.h"
#include "liquids.h"
#include "mxp.h"
#include "overland.h"
#include "pfiles.h"
#include "polymorph.h"

#define HISTORY_FILE SYSTEM_DIR "history.txt"   /* Used in do_history - Samson 2-12-98 */

/*
 * Keep players from defeating examine progs -Druid
 * False = do not trigger
 * True = Trigger
 */
bool EXA_prog_trigger = TRUE;

LIQ_TABLE *get_liq_vnum( int vnum );
char *mxp_obj_str( CHAR_DATA * ch, OBJ_DATA * obj );
char *mxp_obj_str_close( CHAR_DATA * ch, OBJ_DATA * obj );
char *spacetodash( char *argument );
void look_sky( CHAR_DATA * ch );
void remove_visit( CHAR_DATA * ch, ROOM_INDEX_DATA * room );
CMDF do_track( CHAR_DATA * ch, char *argument );
void display_map( CHAR_DATA * ch );
void load_socials( void );
void free_social( SOCIALTYPE * social );
void unlink_social( SOCIALTYPE * social );
void save_sysdata( SYSTEM_DATA sys );
char *sha256_crypt( const char *pwd );

char *const where_name[] = {
   "<used as light>     ",
   "<worn on finger>    ",
   "<worn on finger>    ",
   "<worn around neck>  ",
   "<worn around neck>  ", /* 5 */
   "<worn on body>      ",
   "<worn on head>      ",
   "<worn on legs>      ",
   "<worn on feet>      ",
   "<worn on hands>     ", /* 10 */
   "<worn on arms>      ",
   "<worn as shield>    ",
   "<worn about body>   ",
   "<worn about waist>  ",
   "<worn around wrist> ", /* 15 */
   "<worn around wrist> ",
   "<wielded>           ",
   "<held>              ",
   "<dual wielded>      ",
   "<worn on ears>      ", /* 20 */
   "<worn on eyes>      ",
   "<missile wielded>   ",
   "<worn on back>      ",
   "<worn on face>      ",
   "<worn on ankle>     ", /* 25 */
   "<worn on ankle>     ",
   "<lodged in a rib>   ",
   "<lodged in an arm>  ",
   "<lodged in a leg>   "
};

/*
StarMap was written by Nebseni of Clandestine MUD and ported to Smaug
by Desden, el Chaman Tibetano.
*/

#define NUM_DAYS sysdata.dayspermonth
/* Match this to the number of days per month; this is the moon cycle */
#define NUM_MONTHS sysdata.monthsperyear
/* Match this to the number of months defined in month_name[].  */
#define MAP_WIDTH 72
#define MAP_HEIGHT 8
/* Should be the string length and number of the constants below.*/

const char *star_map[] = {
   "                                               C. C.                  g*",
   "    O:       R*        G*    G.  W* W. W.          C. C.    Y* Y. Y.    ",
   "  O*.                c.          W.W.     W.            C.       Y..Y.  ",
   "O.O. O.              c.  G..G.           W:      B*                   Y.",
   "     O.    c.     c.                     W. W.                  r*    Y.",
   "     O.c.     c.      G.             P..     W.        p.      Y.   Y:  ",
   "        c.                    G*    P.  P.           p.  p:     Y.   Y. ",
   "                 b*             P.: P*                 p.p:             "
};

/****************** CONSTELLATIONS and STARS *****************************
  Cygnus     Mars        Orion      Dragon       Cassiopeia          Venus
           Ursa Ninor                           Mercurius     Pluto    
               Uranus              Leo                Crown       Raptor
*************************************************************************/

const char *sun_map[] = {
   "\\`|'/",
   "- O -",
   "/.|.\\"
};

const char *moon_map[] = {
   " @@@ ",
   "@@@@@",
   " @@@ "
};

void look_sky( CHAR_DATA * ch )
{
   char buf[MSL];
   char buf2[4];
   int starpos, sunpos, moonpos, moonphase, i, linenum, precip;

   send_to_pager( "You gaze up towards the heavens and see:\n\r", ch );

   precip = ( ch->in_room->area->weather->precip + 3 * weath_unit - 1 ) / weath_unit;
   if( precip > 1 )
   {
      send_to_char( "There are some clouds in the sky so you cannot see anything else.\n\r", ch );
      return;
   }
   sunpos = ( MAP_WIDTH * ( sysdata.hoursperday - time_info.hour ) / sysdata.hoursperday );
   moonpos = ( sunpos + time_info.day * MAP_WIDTH / NUM_DAYS ) % MAP_WIDTH;
   if( ( moonphase = ( ( ( ( MAP_WIDTH + moonpos - sunpos ) % MAP_WIDTH ) + ( MAP_WIDTH / 16 ) ) * 8 ) / MAP_WIDTH ) > 4 )
      moonphase -= 8;
   starpos = ( sunpos + MAP_WIDTH * time_info.month / NUM_MONTHS ) % MAP_WIDTH;
   /*
    * The left end of the star_map will be straight overhead at midnight during month 0 
    */

   for( linenum = 0; linenum < MAP_HEIGHT; linenum++ )
   {
      if( ( time_info.hour >= sysdata.hoursunrise && time_info.hour <= sysdata.hoursunset )
          && ( linenum < 3 || linenum >= 6 ) )
         continue;

      mudstrlcpy( buf, " ", MSL );

      /*
       * for ( i = MAP_WIDTH/4; i <= 3*MAP_WIDTH/4; i++)
       */
      for( i = 1; i <= MAP_WIDTH; i++ )
      {
         /*
          * plot moon on top of anything else...unless new moon & no eclipse 
          */
         if( ( time_info.hour >= sysdata.hoursunrise && time_info.hour <= sysdata.hoursunset )  /* daytime? */
             && ( moonpos >= MAP_WIDTH / 4 - 2 ) && ( moonpos <= 3 * MAP_WIDTH / 4 + 2 )  /* in sky? */
             && ( i >= moonpos - 2 ) && ( i <= moonpos + 2 )   /* is this pixel near moon? */
             && ( ( sunpos == moonpos && time_info.hour == sysdata.hournoon ) || moonphase != 0 )  /*no eclipse */
             && ( moon_map[linenum - 3][i + 2 - moonpos] == '@' ) )
         {
            if( ( moonphase < 0 && i - 2 - moonpos >= moonphase ) || ( moonphase > 0 && i + 2 - moonpos <= moonphase ) )
               mudstrlcat( buf, "&W@", MSL );
            else
               mudstrlcat( buf, " ", MSL );
         }
         else if( ( linenum >= 3 ) && ( linenum < 6 ) && /* nighttime */
                  ( moonpos >= MAP_WIDTH / 4 - 2 ) && ( moonpos <= 3 * MAP_WIDTH / 4 + 2 )   /* in sky? */
                  && ( i >= moonpos - 2 ) && ( i <= moonpos + 2 ) /* is this pixel near moon? */
                  && ( moon_map[linenum - 3][i + 2 - moonpos] == '@' ) )
         {
            if( ( moonphase < 0 && i - 2 - moonpos >= moonphase ) || ( moonphase > 0 && i + 2 - moonpos <= moonphase ) )
               mudstrlcat( buf, "&W@", MSL );
            else
               mudstrlcat( buf, " ", MSL );
         }
         else  /* plot sun or stars */
         {
            if( time_info.hour >= sysdata.hoursunrise && time_info.hour <= sysdata.hoursunset ) /* daytime */
            {
               if( i >= sunpos - 2 && i <= sunpos + 2 )
               {
                  snprintf( buf2, 4, "&Y%c", sun_map[linenum - 3][i + 2 - sunpos] );
                  mudstrlcat( buf, buf2, MSL );
               }
               else
                  mudstrlcat( buf, " ", MSL );
            }
            else
            {
               switch ( star_map[linenum][( MAP_WIDTH + i - starpos ) % MAP_WIDTH] )
               {
                  default:
                     mudstrlcat( buf, " ", MSL );
                     break;
                  case ':':
                     mudstrlcat( buf, ":", MSL );
                     break;
                  case '.':
                     mudstrlcat( buf, ".", MSL );
                     break;
                  case '*':
                     mudstrlcat( buf, "*", MSL );
                     break;
                  case 'G':
                     mudstrlcat( buf, "&G ", MSL );
                     break;
                  case 'g':
                     mudstrlcat( buf, "&g ", MSL );
                     break;
                  case 'R':
                     mudstrlcat( buf, "&R ", MSL );
                     break;
                  case 'r':
                     mudstrlcat( buf, "&r ", MSL );
                     break;
                  case 'C':
                     mudstrlcat( buf, "&C ", MSL );
                     break;
                  case 'O':
                     mudstrlcat( buf, "&O ", MSL );
                     break;
                  case 'B':
                     mudstrlcat( buf, "&B ", MSL );
                     break;
                  case 'P':
                     mudstrlcat( buf, "&P ", MSL );
                     break;
                  case 'W':
                     mudstrlcat( buf, "&W ", MSL );
                     break;
                  case 'b':
                     mudstrlcat( buf, "&b ", MSL );
                     break;
                  case 'p':
                     mudstrlcat( buf, "&p ", MSL );
                     break;
                  case 'Y':
                     mudstrlcat( buf, "&Y ", MSL );
                     break;
                  case 'c':
                     mudstrlcat( buf, "&c ", MSL );
                     break;
               }
            }
         }
      }
      mudstrlcat( buf, "\n\r", MSL );
      send_to_pager( buf, ch );
   }
}

char *strip_crlf( char *str )
{
   static char newstr[MSL];
   int i, j;

   if( !str || str[0] == '\0' )
      return "";

   for( i = j = 0; str[i] != '\0'; i++ )
      if( str[i] != '\r' && str[i] != '\n' )
      {
         newstr[j++] = str[i];
      }
   newstr[j] = '\0';
   return newstr;
}

char *format_obj_to_char( OBJ_DATA * obj, CHAR_DATA * ch, bool fShort )
{
   static char buf[MSL];
   bool glowsee = FALSE;

   /*
    * can see glowing invis items in the dark 
    */
   if( IS_OBJ_FLAG( obj, ITEM_GLOW ) && IS_OBJ_FLAG( obj, ITEM_INVIS )
       && !IS_AFFECTED( ch, AFF_TRUESIGHT ) && !IS_AFFECTED( ch, AFF_DETECT_INVIS ) )
      glowsee = TRUE;

   buf[0] = '\0';
   if( IS_OBJ_FLAG( obj, ITEM_INVIS ) )
      mudstrlcat( buf, "(Invis) ", MSL );
   if( ( IS_AFFECTED( ch, AFF_DETECT_EVIL ) || ch->Class == CLASS_PALADIN ) && IS_OBJ_FLAG( obj, ITEM_EVIL ) )
      mudstrlcat( buf, "(Red Aura) ", MSL );
   if( ch->Class == CLASS_PALADIN
       && ( IS_OBJ_FLAG( obj, ITEM_ANTI_EVIL ) && !IS_OBJ_FLAG( obj, ITEM_ANTI_NEUTRAL )
            && !IS_OBJ_FLAG( obj, ITEM_ANTI_GOOD ) ) )
      mudstrlcat( buf, "(Flaming Red) ", MSL );
   if( ch->Class == CLASS_PALADIN
       && ( !IS_OBJ_FLAG( obj, ITEM_ANTI_EVIL ) && IS_OBJ_FLAG( obj, ITEM_ANTI_NEUTRAL )
            && !IS_OBJ_FLAG( obj, ITEM_ANTI_GOOD ) ) )
      mudstrlcat( buf, "(Flaming Grey) ", MSL );
   if( ch->Class == CLASS_PALADIN
       && ( !IS_OBJ_FLAG( obj, ITEM_ANTI_EVIL ) && !IS_OBJ_FLAG( obj, ITEM_ANTI_NEUTRAL )
            && IS_OBJ_FLAG( obj, ITEM_ANTI_GOOD ) ) )
      mudstrlcat( buf, "(Flaming White) ", MSL );
   if( ch->Class == CLASS_PALADIN
       && ( IS_OBJ_FLAG( obj, ITEM_ANTI_EVIL ) && IS_OBJ_FLAG( obj, ITEM_ANTI_NEUTRAL )
            && !IS_OBJ_FLAG( obj, ITEM_ANTI_GOOD ) ) )
      mudstrlcat( buf, "(Smouldering Red-Grey) ", MSL );
   if( ch->Class == CLASS_PALADIN
       && ( IS_OBJ_FLAG( obj, ITEM_ANTI_EVIL ) && !IS_OBJ_FLAG( obj, ITEM_ANTI_NEUTRAL )
            && IS_OBJ_FLAG( obj, ITEM_ANTI_GOOD ) ) )
      mudstrlcat( buf, "(Smouldering Red-White) ", MSL );
   if( ch->Class == CLASS_PALADIN
       && ( !IS_OBJ_FLAG( obj, ITEM_ANTI_EVIL ) && IS_OBJ_FLAG( obj, ITEM_ANTI_NEUTRAL )
            && IS_OBJ_FLAG( obj, ITEM_ANTI_GOOD ) ) )
      mudstrlcat( buf, "(Smouldering Grey-White) ", MSL );

   if( IS_AFFECTED( ch, AFF_DETECT_MAGIC ) && IS_OBJ_FLAG( obj, ITEM_MAGIC ) )
      mudstrlcat( buf, "(Magical) ", MSL );
   if( !glowsee && IS_OBJ_FLAG( obj, ITEM_GLOW ) )
      mudstrlcat( buf, "(Glowing) ", MSL );
   if( IS_OBJ_FLAG( obj, ITEM_HUM ) )
      mudstrlcat( buf, "(Humming) ", MSL );
   if( IS_OBJ_FLAG( obj, ITEM_HIDDEN ) )
      mudstrlcat( buf, "(Hidden) ", MSL );
   if( IS_OBJ_FLAG( obj, ITEM_BURIED ) )
      mudstrlcat( buf, "(Buried) ", MSL );
   if( IS_IMMORTAL( ch ) && IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) )
      mudstrlcat( buf, "(PROTO) ", MSL );
   if( IS_AFFECTED( ch, AFF_DETECTTRAPS ) && is_trapped( obj ) )
      mudstrlcat( buf, "(Trap) ", MSL );

   mudstrlcat( buf, mxp_obj_str( ch, obj ), MSL );

   if( fShort )
   {
      if( glowsee && ( IS_NPC(ch) || !IS_PLR_FLAG( ch, PLR_HOLYLIGHT ) ) )
         mudstrlcat( buf, "the faint glow of something", MSL );
      else if( obj->short_descr )
         mudstrlcat( buf, obj->short_descr, MSL );
   }
   else
   {
      if( glowsee && ( IS_NPC(ch) || !IS_PLR_FLAG( ch, PLR_HOLYLIGHT ) ) )
         mudstrlcat( buf, "You see the faint glow of something nearby.", MSL );
      if( obj->objdesc && obj->objdesc[0] != '\0' )
         mudstrlcat( buf, obj->objdesc, MSL );
   }

   mudstrlcat( buf, color_str( AT_OBJECT, ch ), MSL );
   mudstrlcat( buf, mxp_obj_str_close( ch, obj ), MSL );

   if( obj->count > 1 )
      snprintf( buf+strlen(buf), MSL-strlen(buf), " (%d)", obj->count );
   return buf;
}

/*
 * Some increasingly freaky hallucinated objects		-Thoric
 * (Hats off to Albert Hoffman's "problem child")
 */
char *hallucinated_object( int ms, bool fShort )
{
   int sms = URANGE( 1, ( ms + 10 ) / 5, 20 );

   if( fShort )
      switch ( number_range( 6 - URANGE( 1, sms / 2, 5 ), sms ) )
      {
         case 1:
            return "a sword";
         case 2:
            return "a stick";
         case 3:
            return "something shiny";
         case 4:
            return "something";
         case 5:
            return "something interesting";
         case 6:
            return "something colorful";
         case 7:
            return "something that looks cool";
         case 8:
            return "a nifty thing";
         case 9:
            return "a cloak of flowing colors";
         case 10:
            return "a mystical flaming sword";
         case 11:
            return "a swarm of insects";
         case 12:
            return "a deathbane";
         case 13:
            return "a figment of your imagination";
         case 14:
            return "your gravestone";
         case 15:
            return "the long lost boots of Ranger Samson";
         case 16:
            return "a glowing tome of arcane knowledge";
         case 17:
            return "a long sought secret";
         case 18:
            return "the meaning of it all";
         case 19:
            return "the answer";
         case 20:
            return "the key to life, the universe and everything";
      }
   switch ( number_range( 6 - URANGE( 1, sms / 2, 5 ), sms ) )
   {
      case 1:
         return "A nice looking sword catches your eye.";
      case 2:
         return "The ground is covered in small sticks.";
      case 3:
         return "Something shiny catches your eye.";
      case 4:
         return "Something catches your attention.";
      case 5:
         return "Something interesting catches your eye.";
      case 6:
         return "Something colorful flows by.";
      case 7:
         return "Something that looks cool calls out to you.";
      case 8:
         return "A nifty thing of great importance stands here.";
      case 9:
         return "A cloak of flowing colors asks you to wear it.";
      case 10:
         return "A mystical flaming sword awaits your grasp.";
      case 11:
         return "A swarm of insects buzzes in your face!";
      case 12:
         return "The extremely rare Deathbane lies at your feet.";
      case 13:
         return "A figment of your imagination is at your command.";
      case 14:
         return "You notice a gravestone here... upon closer examination, it reads your name.";
      case 15:
         return "The long lost boots of Ranger Samson lie off to the side.";
      case 16:
         return "A glowing tome of arcane knowledge hovers in the air before you.";
      case 17:
         return "A long sought secret of all mankind is now clear to you.";
      case 18:
         return "The meaning of it all, so simple, so clear... of course!";
      case 19:
         return "The answer.  One.  It's always been One.";
      case 20:
         return "The key to life, the universe and everything awaits your hand.";
   }
   return "Whoa!!!";
}

/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char( OBJ_DATA * list, CHAR_DATA * ch, bool fShort, bool fShowNothing )
{
   char **prgpstrShow;
   int *prgnShow, *pitShow;
   char *pstrShow;
   OBJ_DATA *obj;
   int nShow, iShow, count, offcount, tmp, ms, cnt;
   bool fCombine;

   if( !ch->desc )
      return;

   /*
    * if there's no list... then don't do all this crap!  -Thoric
    */
   if( !list )
   {
      if( fShowNothing )
      {
         if( IS_NPC( ch ) || IS_PLR_FLAG( ch, PLR_COMBINE ) )
            send_to_char( "     ", ch );
         set_char_color( AT_OBJECT, ch );
         send_to_char( "Nothing.\n\r", ch );
      }
      return;
   }

   /*
    * Alloc space for output lines.
    */
   count = 0;
   for( obj = list; obj; obj = obj->next_content )
      count++;

   ms = ( ch->mental_state ? ch->mental_state : 1 )
      * ( IS_NPC( ch ) ? 1 : ( ch->pcdata->condition[COND_DRUNK] ? ( ch->pcdata->condition[COND_DRUNK] / 12 ) : 1 ) );

   /*
    * If not mentally stable...
    */
   if( abs( ms ) > 40 )
   {
      offcount = URANGE( -( count ), ( count * ms ) / 100, count * 2 );
      if( offcount < 0 )
         offcount += number_range( 0, abs( offcount ) );
      else if( offcount > 0 )
         offcount -= number_range( 0, offcount );
   }
   else
      offcount = 0;

   if( count + offcount <= 0 )
   {
      if( fShowNothing )
      {
         if( IS_NPC( ch ) || IS_PLR_FLAG( ch, PLR_COMBINE ) )
            send_to_char( "     ", ch );
         set_char_color( AT_OBJECT, ch );
         send_to_char( "Nothing.\n\r", ch );
      }
      return;
   }

   CREATE( prgpstrShow, char *, count + ( ( offcount > 0 ) ? offcount : 0 ) );
   CREATE( prgnShow, int, count + ( ( offcount > 0 ) ? offcount : 0 ) );
   CREATE( pitShow, int, count + ( ( offcount > 0 ) ? offcount : 0 ) );
   nShow = 0;
   tmp = ( offcount > 0 ) ? offcount : 0;
   cnt = 0;

   /*
    * Format the list of objects.
    */
   for( obj = list; obj; obj = obj->next_content )
   {
      if( IS_OBJ_FLAG( obj, ITEM_AUCTION ) )
         continue;

      if( offcount < 0 && ++cnt > ( count + offcount ) )
         break;
      if( tmp > 0 && number_bits( 1 ) == 0 )
      {
         prgpstrShow[nShow] = str_dup( hallucinated_object( ms, fShort ) );
         prgnShow[nShow] = 1;
         pitShow[nShow] = number_range( ITEM_LIGHT, ITEM_BOOK );
         nShow++;
         --tmp;
      }
      if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj, FALSE )
          && ( obj->item_type != ITEM_TRAP || IS_AFFECTED( ch, AFF_DETECTTRAPS ) ) )
      {
         pstrShow = format_obj_to_char( obj, ch, fShort );
         fCombine = FALSE;

         if( IS_NPC( ch ) || IS_PLR_FLAG( ch, PLR_COMBINE ) )
         {
            /*
             * Look for duplicates, case sensitive.
             * Matches tend to be near end so run loop backwords.
             */
            for( iShow = nShow - 1; iShow >= 0; iShow-- )
            {
               if( !str_cmp( prgpstrShow[iShow], pstrShow ) )
               {
                  prgnShow[iShow] += obj->count;
                  fCombine = TRUE;
                  break;
               }
            }
         }

         pitShow[nShow] = obj->item_type;
         /*
          * Couldn't combine, or didn't want to.
          */
         if( !fCombine )
         {
            prgpstrShow[nShow] = str_dup( pstrShow );
            prgnShow[nShow] = obj->count;
            nShow++;
         }
      }
   }
   if( tmp > 0 )
   {
      int x;
      for( x = 0; x < tmp; x++ )
      {
         prgpstrShow[nShow] = str_dup( hallucinated_object( ms, fShort ) );
         prgnShow[nShow] = 1;
         pitShow[nShow] = number_range( ITEM_LIGHT, ITEM_BOOK );
         nShow++;
      }
   }

   /*
    * Output the formatted list.      -Color support by Thoric
    */
   for( iShow = 0; iShow < nShow; iShow++ )
   {
      switch ( pitShow[iShow] )
      {
         default:
            set_char_color( AT_OBJECT, ch );
            break;
         case ITEM_BLOOD:
            set_char_color( AT_BLOOD, ch );
            break;
         case ITEM_MONEY:
         case ITEM_TREASURE:
            set_char_color( AT_YELLOW, ch );
            break;
         case ITEM_COOK:
         case ITEM_FOOD:
            set_char_color( AT_HUNGRY, ch );
            break;
         case ITEM_DRINK_CON:
         case ITEM_FOUNTAIN:
            set_char_color( AT_THIRSTY, ch );
            break;
         case ITEM_FIRE:
            set_char_color( AT_FIRE, ch );
            break;
         case ITEM_SCROLL:
         case ITEM_WAND:
         case ITEM_STAFF:
            set_char_color( AT_MAGIC, ch );
            break;
      }
      if( fShowNothing )
         send_to_char( "     ", ch );

      send_to_char( prgpstrShow[iShow], ch );

      if( prgnShow[iShow] != 1 )
         ch_printf( ch, " (%d)", prgnShow[iShow] );

      send_to_char( "\n\r", ch );
      DISPOSE( prgpstrShow[iShow] );
   }

   if( fShowNothing && nShow == 0 )
   {
      if( IS_NPC( ch ) || IS_PLR_FLAG( ch, PLR_COMBINE ) )
         send_to_char( "     ", ch );
      set_char_color( AT_OBJECT, ch );
      send_to_char( "Nothing.\n\r", ch );
   }

   /*
    * Clean up.
    */
   DISPOSE( prgpstrShow );
   DISPOSE( prgnShow );
   DISPOSE( pitShow );
   return;
}

/*
 * Show fancy descriptions for certain spell affects		-Thoric
 */
void show_visible_affects_to_char( CHAR_DATA * victim, CHAR_DATA * ch )
{
   char name[MSL];

   if( IS_NPC( victim ) )
      mudstrlcpy( name, victim->short_descr, MSL );
   else
      mudstrlcpy( name, victim->name, MSL );
   name[0] = toupper( name[0] );

   if( IS_AFFECTED( victim, AFF_SANCTUARY ) )
   {
      if( IS_GOOD( victim ) )
         ch_printf( ch, "&W%s glows with an aura of divine radiance.\n\r", name );
      else if( IS_EVIL( victim ) )
         ch_printf( ch, "&z%s shimmers beneath an aura of dark energy.\n\r", name );
      else
         ch_printf( ch, "&w%s is shrouded in flowing shadow and light.\n\r", name );
   }
   if( IS_AFFECTED( victim, AFF_BLADEBARRIER ) )
      ch_printf( ch, "&w%s is surrounded by a spinning barrier of sharp blades.\n\r", name );
   if( IS_AFFECTED( victim, AFF_FIRESHIELD ) )
      ch_printf( ch, "&[fire]%s is engulfed within a blaze of mystical flame.\n\r", name );
   if( IS_AFFECTED( victim, AFF_SHOCKSHIELD ) )
      ch_printf( ch, "&B%s is surrounded by cascading torrents of energy.\n\r", name );
   if( IS_AFFECTED( victim, AFF_ACIDMIST ) )
      ch_printf( ch, "&G%s is visible through a cloud of churning mist.\n\r", name );
   if( IS_AFFECTED( victim, AFF_VENOMSHIELD ) )
      ch_printf( ch, "&g%s is enshrouded in a choking cloud of gas.\n\r", name );
   if( IS_AFFECTED( victim, AFF_HASTE ) )
      ch_printf( ch, "&Y%s appears to be slightly blurred.\n\r", name );
   if( IS_AFFECTED( victim, AFF_SLOW ) )
      ch_printf( ch, "&[magic]%s appears to be moving very slowly.\n\r", name );
   /*
    * Scryn 8/13
    */
   if( IS_AFFECTED( victim, AFF_ICESHIELD ) )
      ch_printf( ch, "&C%s is ensphered by shards of glistening ice.\n\r", name );
   if( IS_AFFECTED( victim, AFF_CHARM ) )
      ch_printf( ch, "&[magic]%s follows %s around everywhere.\n\r", name,
                 victim->master == ch ? "you" : victim->master->name );
   if( !IS_NPC( victim ) && !victim->desc && victim->switched && IS_AFFECTED( victim->switched, AFF_POSSESS ) )
      ch_printf( ch, "&[magic]%s appears to be in a deep trance...\n\r", PERS( victim, ch, FALSE ) );
}

void show_condition( CHAR_DATA * ch, CHAR_DATA * victim )
{
   char buf[MSL];
   int percent;

   if( victim->max_hit > 0 )
      percent = ( 100 * victim->hit ) / victim->max_hit;
   else
      percent = -1;

   if( victim != ch )
   {
      mudstrlcpy( buf, PERS( victim, ch, FALSE ), MSL );
      if( percent >= 100 )
         mudstrlcat( buf, " is in perfect health.\n\r", MSL );
      else if( percent >= 90 )
         mudstrlcat( buf, " is slightly scratched.\n\r", MSL );
      else if( percent >= 80 )
         mudstrlcat( buf, " has a few bruises.\n\r", MSL );
      else if( percent >= 70 )
         mudstrlcat( buf, " has some cuts.\n\r", MSL );
      else if( percent >= 60 )
         mudstrlcat( buf, " has several wounds.\n\r", MSL );
      else if( percent >= 50 )
         mudstrlcat( buf, " has many nasty wounds.\n\r", MSL );
      else if( percent >= 40 )
         mudstrlcat( buf, " is bleeding freely.\n\r", MSL );
      else if( percent >= 30 )
         mudstrlcat( buf, " is covered in blood.\n\r", MSL );
      else if( percent >= 20 )
         mudstrlcat( buf, " is leaking guts.\n\r", MSL );
      else if( percent >= 10 )
         mudstrlcat( buf, " is almost dead.\n\r", MSL );
      else
         mudstrlcat( buf, " is DYING.\n\r", MSL );
   }
   else
   {
      mudstrlcpy( buf, "You", MSL );
      if( percent >= 100 )
         mudstrlcat( buf, " are in perfect health.\n\r", MSL );
      else if( percent >= 90 )
         mudstrlcat( buf, " are slightly scratched.\n\r", MSL );
      else if( percent >= 80 )
         mudstrlcat( buf, " have a few bruises.\n\r", MSL );
      else if( percent >= 70 )
         mudstrlcat( buf, " have some cuts.\n\r", MSL );
      else if( percent >= 60 )
         mudstrlcat( buf, " have several wounds.\n\r", MSL );
      else if( percent >= 50 )
         mudstrlcat( buf, " have many nasty wounds.\n\r", MSL );
      else if( percent >= 40 )
         mudstrlcat( buf, " are bleeding freely.\n\r", MSL );
      else if( percent >= 30 )
         mudstrlcat( buf, " are covered in blood.\n\r", MSL );
      else if( percent >= 20 )
         mudstrlcat( buf, " are leaking guts.\n\r", MSL );
      else if( percent >= 10 )
         mudstrlcat( buf, " are almost dead.\n\r", MSL );
      else
         mudstrlcat( buf, " are DYING.\n\r", MSL );
   }

   buf[0] = UPPER( buf[0] );
   send_to_char( buf, ch );
   return;
}

/* Function modified from original form - Whir */
/* Gave a reason buffer to PLR_AFK -Whir - 8/31/98 */
/* Eliminated clan_type GUILD - Samson 11-30-98 */
void show_char_to_char_0( CHAR_DATA * victim, CHAR_DATA * ch, int num )
{
   char buf[MSL];

   buf[0] = '\0';

   if( !can_see( ch, victim, TRUE ) )
      return;

   set_char_color( AT_PERSON, ch );
   if( !IS_NPC( victim ) && !victim->desc )
   {
      if( !victim->switched )
         mudstrlcat( buf, "[(Link Dead)] ", MSL );
      else if( !IS_AFFECTED( victim, AFF_POSSESS ) )
         mudstrlcat( buf, "(Switched) ", MSL );
   }
   if( IS_NPC( victim ) && IS_AFFECTED( victim, AFF_POSSESS ) && IS_IMMORTAL( ch ) && victim->desc )
      snprintf( buf + strlen( buf ), MSL - strlen( buf ), "(%s)", victim->desc->original->name );
   if( IS_PLR_FLAG( victim, PLR_AFK ) )
   {
      if( victim->pcdata->afkbuf )
         snprintf( buf + strlen( buf ), MSL - strlen( buf ), "[AFK %s] ", victim->pcdata->afkbuf );
      else
         mudstrlcat( buf, "[AFK] ", MSL );
   }

   if( IS_PLR_FLAG( victim, PLR_WIZINVIS ) || IS_ACT_FLAG( victim, ACT_MOBINVIS ) )
   {
      if( !IS_NPC( victim ) )
         snprintf( buf + strlen( buf ), MSL - strlen( buf ), "(Invis %d) ", victim->pcdata->wizinvis );
      else
         snprintf( buf + strlen( buf ), MSL - strlen( buf ), "(Mobinvis %d) ", victim->mobinvis );
   }

   if( !IS_NPC( victim ) && victim->pcdata->clan && victim->pcdata->clan->badge
       && ( victim->pcdata->clan->clan_type != CLAN_ORDER ) )
      ch_printf( ch, "%s ", victim->pcdata->clan->badge );
   else
      set_char_color( AT_PERSON, ch );

   if( IS_AFFECTED( victim, AFF_INVISIBLE ) )
      mudstrlcat( buf, "(Invis) ", MSL );
   if( IS_AFFECTED( victim, AFF_HIDE ) )
      mudstrlcat( buf, "(Hiding) ", MSL );
   if( IS_AFFECTED( victim, AFF_PASS_DOOR ) )
      mudstrlcat( buf, "(Translucent) ", MSL );
   if( IS_AFFECTED( victim, AFF_FAERIE_FIRE ) )
      mudstrlcat( buf, "(Pink Aura) ", MSL );
   if( IS_EVIL( victim ) && ( IS_AFFECTED( ch, AFF_DETECT_EVIL ) || ch->Class == CLASS_PALADIN ) )
      mudstrlcat( buf, "(Red Aura) ", MSL );
   if( IS_NEUTRAL( victim ) && ch->Class == CLASS_PALADIN )
      mudstrlcat( buf, "(Grey Aura) ", MSL );
   if( IS_GOOD( victim ) && ch->Class == CLASS_PALADIN )
      mudstrlcat( buf, "(White Aura) ", MSL );
   if( IS_AFFECTED( victim, AFF_BERSERK ) )
      mudstrlcat( buf, "(Wild-eyed) ", MSL );
   if( IS_PLR_FLAG( victim, PLR_LITTERBUG ) )
      mudstrlcat( buf, "(LITTERBUG) ", MSL );
   if( IS_IMMORTAL( ch ) && IS_ACT_FLAG( victim, ACT_PROTOTYPE ) )
      mudstrlcat( buf, "(PROTO) ", MSL );
   if( IS_NPC( victim ) && ch->mount && ch->mount == victim && ch->in_room == ch->mount->in_room )
      mudstrlcat( buf, "(Mount) ", MSL );
   if( victim->desc && victim->desc->connected == CON_EDITING )
      mudstrlcat( buf, "(Writing) ", MSL );

   set_char_color( AT_PERSON, ch );
   if( ( victim->position == victim->defposition && victim->long_descr && victim->long_descr[0] != '\0' )
       || ( victim->morph && victim->morph->morph && victim->morph->morph->defpos == victim->position ) )
   {
      if( victim->morph != NULL )
      {
         if( !IS_IMMORTAL( ch ) )
         {
            if( victim->morph->morph != NULL )
               mudstrlcat( buf, victim->morph->morph->long_desc, MSL );
            else
               mudstrlcat( buf, strip_crlf( victim->long_descr ), MSL );
         }
         else
         {
            mudstrlcat( buf, PERS( victim, ch, FALSE ), MSL );
            if( !IS_PLR_FLAG( ch, PLR_BRIEF ) && !IS_NPC( victim ) )
               mudstrlcat( buf, victim->pcdata->title, MSL );
            mudstrlcat( buf, ".", MSL );
         }
      }
      else
         mudstrlcat( buf, strip_crlf( victim->long_descr ), MSL );

      if( num > 1 && IS_NPC( victim ) )
         snprintf( buf + strlen( buf ), MSL - strlen( buf ), " (%d)", num );
      mudstrlcat( buf, "\n\r", MSL );
      send_to_char( buf, ch );
      show_visible_affects_to_char( victim, ch );
      return;
   }
   else
   {
      if( victim->morph != NULL && victim->morph->morph != NULL && !IS_IMMORTAL( ch ) )
         mudstrlcat( buf, MORPHPERS( victim, ch, FALSE ), MSL );
      else
         mudstrlcat( buf, PERS( victim, ch, FALSE ), MSL );
   }

   if( !IS_PLR_FLAG( ch, PLR_BRIEF ) && !IS_NPC( victim ) )
      mudstrlcat( buf, victim->pcdata->title, MSL );

   mudstrlcat( buf, color_str( AT_PERSON, ch ), MSL );

/* Furniture ideas taken from ROT. Furniture 1.01 is provided by Xerves.
   Info rewrite for sleeping/resting/standing/sitting on Objects -- Xerves
 */
   switch ( victim->position )
   {
      case POS_DEAD:
         mudstrlcat( buf, " is DEAD!!", MSL );
         break;
      case POS_MORTAL:
         mudstrlcat( buf, " is mortally wounded.", MSL );
         break;
      case POS_INCAP:
         mudstrlcat( buf, " is incapacitated.", MSL );
         break;
      case POS_STUNNED:
         mudstrlcat( buf, " is lying here stunned.", MSL );
         break;
      case POS_SLEEPING:
         if( victim->on != NULL )
         {
            if( IS_SET( victim->on->value[2], SLEEP_AT ) )
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), " is sleeping at %s.", victim->on->short_descr );
            else if( IS_SET( victim->on->value[2], SLEEP_ON ) )
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), " is sleeping on %s.", victim->on->short_descr );
            else
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), " is sleeping in %s.", victim->on->short_descr );
         }
         else
         {
            if( ch->position == POS_SITTING || ch->position == POS_RESTING )
               mudstrlcat( buf, " is sleeping nearby.", MSL );
            else
               mudstrlcat( buf, " is deep in slumber here.", MSL );
         }
         break;
      case POS_RESTING:
         if( victim->on != NULL )
         {
            if( IS_SET( victim->on->value[2], REST_AT ) )
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), " is resting at %s.", victim->on->short_descr );
            else if( IS_SET( victim->on->value[2], REST_ON ) )
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), " is resting on %s.", victim->on->short_descr );
            else
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), " is resting in %s.", victim->on->short_descr );
         }
         else
         {
            if( ch->position == POS_RESTING )
               mudstrlcat( buf, " is sprawled out alongside you.", MSL );
            else if( ch->position == POS_MOUNTED )
               mudstrlcat( buf, " is sprawled out at the foot of your mount.", MSL );
            else
               mudstrlcat( buf, " is sprawled out here.", MSL );
         }
         break;
      case POS_SITTING:
         if( victim->on != NULL )
         {
            if( IS_SET( victim->on->value[2], SIT_AT ) )
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), " is sitting at %s.", victim->on->short_descr );
            else if( IS_SET( victim->on->value[2], SIT_ON ) )
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), " is sitting on %s.", victim->on->short_descr );
            else
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), " is sitting in %s.", victim->on->short_descr );
         }
         else
            mudstrlcat( buf, " is sitting here.", MSL );
         break;
      case POS_STANDING:
         if( victim->on != NULL )
         {
            if( IS_SET( victim->on->value[2], STAND_AT ) )
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), " is standing at %s.", victim->on->short_descr );
            else if( IS_SET( victim->on->value[2], STAND_ON ) )
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), " is standing on %s.", victim->on->short_descr );
            else
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), " is standing in %s.", victim->on->short_descr );
         }
         else if( IS_IMMORTAL( victim ) )
            mudstrlcat( buf, " is here before you.", MSL );
         else if( ( victim->in_room->sector_type == SECT_UNDERWATER )
                  && !IS_AFFECTED( victim, AFF_AQUA_BREATH ) && !IS_NPC( victim ) )
            mudstrlcat( buf, " is drowning here.", MSL );
         else if( victim->in_room->sector_type == SECT_UNDERWATER )
            mudstrlcat( buf, " is here in the water.", MSL );
         else if( ( victim->in_room->sector_type == SECT_OCEANFLOOR )
                  && !IS_AFFECTED( victim, AFF_AQUA_BREATH ) && !IS_NPC( victim ) )
            mudstrlcat( buf, " is drowning here.", MSL );
         else if( victim->in_room->sector_type == SECT_OCEANFLOOR )
            mudstrlcat( buf, " is standing here in the water.", MSL );
         else if( IS_AFFECTED( victim, AFF_FLOATING ) || IS_AFFECTED( victim, AFF_FLYING ) )
            mudstrlcat( buf, " is hovering here.", MSL );
         else
            mudstrlcat( buf, " is standing here.", MSL );
         break;
      case POS_SHOVE:
         mudstrlcat( buf, " is being shoved around.", MSL );
         break;
      case POS_DRAG:
         mudstrlcat( buf, " is being dragged around.", MSL );
         break;
      case POS_MOUNTED:
         mudstrlcat( buf, " is here, upon ", MSL );
         if( !victim->mount )
            mudstrlcat( buf, "thin air???", MSL );
         else if( victim->mount == ch )
            mudstrlcat( buf, "your back.", MSL );
         else if( victim->in_room == victim->mount->in_room )
         {
            mudstrlcat( buf, PERS( victim->mount, ch, FALSE ), MSL );
            mudstrlcat( buf, ".", MSL );
         }
         else
            mudstrlcat( buf, "someone who left??", MSL );
         break;
      case POS_FIGHTING:
      case POS_EVASIVE:
      case POS_DEFENSIVE:
      case POS_AGGRESSIVE:
      case POS_BERSERK:
         mudstrlcat( buf, " is here, fighting ", MSL );
         if( !victim->fighting )
         {
            mudstrlcat( buf, "thin air???", MSL );

            /*
             * some bug somewhere.... kinda hackey fix -h 
             */
            if( !victim->mount )
               victim->position = POS_STANDING;
            else
               victim->position = POS_MOUNTED;
         }
         else if( who_fighting( victim ) == ch )
            mudstrlcat( buf, "YOU!", MSL );
         else if( victim->in_room == victim->fighting->who->in_room )
         {
            mudstrlcat( buf, PERS( victim->fighting->who, ch, FALSE ), MSL );
            mudstrlcat( buf, ".", MSL );
         }
         else
            mudstrlcat( buf, "someone who left??", MSL );
         break;
   }

   if( num > 1 && IS_NPC( victim ) )
      snprintf( buf + strlen( buf ), MSL - strlen( buf ), " (%d)", num );

   /*
    * made thusly so hidden characters are damn well hidden unless player
    * * has detect_hidden active.
    * * Sten
    */
   mudstrlcat( buf, "\n\r", MSL );
   buf[0] = UPPER( buf[0] );

   send_to_char( buf, ch );
   show_visible_affects_to_char( victim, ch );
   return;
}

/* Modified by Samson 5-15-99 */
void show_race_line( CHAR_DATA * ch, CHAR_DATA * victim )
{
   int feet, inches;

   if( !IS_NPC( victim ) && ( victim != ch ) )
   {
      feet = victim->height / 12;
      inches = victim->height % 12;
      if( IS_IMMORTAL( ch ) )
         ch_printf( ch, "%s is a level %d %s %s.\n\r", victim->name, victim->level,
                    npc_race[victim->race], npc_class[victim->Class] );

      ch_printf( ch, "%s is %d'%d\" and weighs %d pounds.\n\r", PERS( victim, ch, FALSE ), feet, inches, victim->weight );
      return;
   }
   if( !IS_NPC( victim ) && ( victim == ch ) )
   {
      feet = victim->height / 12;
      inches = victim->height % 12;
      ch_printf( ch, "You are a level %d %s %s.\n\r", victim->level, npc_race[victim->race], npc_class[victim->Class] );
      ch_printf( ch, "You are %d'%d\" and weigh %d pounds.\n\r", feet, inches, victim->weight );
      return;
   }
}

void show_char_to_char_1( CHAR_DATA * victim, CHAR_DATA * ch )
{
   OBJ_DATA *obj;
   int iWear;
   bool found;

   if( can_see( victim, ch, FALSE ) && !IS_PLR_FLAG( ch, PLR_WIZINVIS ) )
   {
      act( AT_ACTION, "$n looks at you.", ch, NULL, victim, TO_VICT );
      if( victim != ch )
         act( AT_ACTION, "$n looks at $N.", ch, NULL, victim, TO_NOTVICT );
      else
         act( AT_ACTION, "$n looks at $mself.", ch, NULL, victim, TO_NOTVICT );
   }

   if( victim->chardesc && victim->chardesc[0] != '\0' )
   {
      if( victim->morph != NULL && victim->morph->morph != NULL )
         send_to_char( victim->morph->morph->description, ch );
      else
         send_to_char( victim->chardesc, ch );
   }
   else
   {
      if( victim->morph != NULL && victim->morph->morph != NULL )
         send_to_char( victim->morph->morph->description, ch );
      else if( IS_NPC( victim ) )
         act( AT_PLAIN, "You see nothing special about $M.", ch, NULL, victim, TO_CHAR );
      else if( ch != victim )
         act( AT_PLAIN, "$E isn't much to look at...", ch, NULL, victim, TO_CHAR );
      else
         act( AT_PLAIN, "You're not much to look at...", ch, NULL, NULL, TO_CHAR );
   }

   show_race_line( ch, victim );
   show_condition( ch, victim );

   found = FALSE;
   for( iWear = 0; iWear < MAX_WEAR; iWear++ )
   {
      if( ( obj = get_eq_char( victim, iWear ) ) != NULL && can_see_obj( ch, obj, FALSE ) )
      {
         if( !found )
         {
            send_to_char( "\n\r", ch );
            if( victim != ch )
               act( AT_PLAIN, "$N is using:", ch, NULL, victim, TO_CHAR );
            else
               act( AT_PLAIN, "You are using:", ch, NULL, NULL, TO_CHAR );
            found = TRUE;
         }
         set_char_color( AT_OBJECT, ch );
         if( !IS_NPC( victim ) && victim->race > 0 && victim->race < MAX_PC_RACE )
            send_to_char( race_table[victim->race]->where_name[iWear], ch );
         else
            send_to_char( where_name[iWear], ch );

         mxpobjmenu = MXP_NONE;
         mxptail[0] = '\0';

         send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
         send_to_char( "\n\r", ch );
      }
   }

   /*
    * Crash fix here by Thoric
    */
   if( IS_NPC( ch ) || victim == ch )
      return;

   if( IS_IMMORTAL( ch ) )
   {
      if( IS_NPC( victim ) )
         ch_printf( ch, "\n\rMobile #%d '%s' ", victim->pIndexData->vnum, victim->name );
      else
         ch_printf( ch, "\n\r%s ", victim->name );

      ch_printf( ch, "is a level %d %s %s.\n\r",
                 victim->level,
                 IS_NPC( victim ) ? victim->race < MAX_NPC_RACE && victim->race >= 0 ?
                 npc_race[victim->race] : "unknown" : victim->race < MAX_PC_RACE &&
                 race_table[victim->race]->race_name &&
                 race_table[victim->race]->race_name[0] != '\0' ?
                 race_table[victim->race]->race_name : "unknown",
                 IS_NPC( victim ) ? victim->Class < MAX_NPC_CLASS && victim->Class >= 0 ?
                 npc_class[victim->Class] : "unknown" : victim->Class < MAX_PC_CLASS &&
                 class_table[victim->Class]->who_name &&
                 class_table[victim->Class]->who_name[0] != '\0' ? class_table[victim->Class]->who_name : "unknown" );
   }

   if( number_percent(  ) < LEARNED( ch, gsn_peek ) )
   {
      ch_printf( ch, "\n\rYou peek at %s inventory:\n\r",
                 victim->sex == SEX_MALE ? "his" : victim->sex == SEX_FEMALE ? "her" : "its" );

      mxpobjmenu = MXP_STEAL;
      mudstrlcpy( mxptail, spacetodash( victim->name ), MIL );

      show_list_to_char( victim->first_carrying, ch, TRUE, TRUE );
   }
   else if( ch->pcdata->learned[gsn_peek] > 0 )
      learn_from_failure( ch, gsn_peek );

   return;
}

bool is_same_mob( CHAR_DATA * i, CHAR_DATA * j )
{
   if( !IS_NPC( i ) || !IS_NPC( j ) )
      return FALSE;

   if( i->pIndexData == j->pIndexData && GET_POS( i ) == GET_POS( j ) &&
       xSAME_BITS( i->affected_by, j->affected_by ) && xSAME_BITS( i->act, j->act ) &&
       !str_cmp( GET_NAME( i ), GET_NAME( j ) ) && !str_cmp( i->short_descr, j->short_descr ) &&
       !str_cmp( i->long_descr, j->long_descr )
       && ( ( i->chardesc && j->chardesc ) && !str_cmp( i->chardesc, j->chardesc ) ) && is_same_map( i, j ) )
      return TRUE;

   return FALSE;
}

void show_char_to_char( CHAR_DATA * list, CHAR_DATA * ch )
{
   CHAR_DATA *rch, *j;
   short num = 0;

   if( !list )
      return;

   for( rch = list; rch; rch = rch->next_in_room )
   {
      if( rch == ch || ( rch == supermob && !IS_IMP( ch ) ) )
         continue;

      num = 0;
      for( j = list; j != rch; j = j->next_in_room )
         if( is_same_mob( j, rch ) )
            break;
      if( j != rch )
         continue;
      for( j = rch; j; j = j->next_in_room )
         if( is_same_mob( j, rch ) )
            num++;

      if( can_see( ch, rch, FALSE ) && !is_ignoring( rch, ch ) )
         show_char_to_char_0( rch, ch, num );
      else if( room_is_dark( ch->in_room, ch ) && IS_AFFECTED( ch, AFF_INFRARED ) )
         send_to_char( "&[blood]The red form of a living creature is here.&D\n\r", ch );
   }
   return;
}

bool check_blind( CHAR_DATA * ch )
{
   if( IS_PLR_FLAG( ch, PLR_HOLYLIGHT ) )
      return TRUE;

   if( IS_AFFECTED( ch, AFF_TRUESIGHT ) )
      return TRUE;

   if( IS_AFFECTED( ch, AFF_BLIND ) )
   {
      send_to_char( "You can't see a thing!\n\r", ch );
      return FALSE;
   }

   return TRUE;
}

/*
 * Returns classical DIKU door direction based on text in arg	-Thoric
 */
int get_door( char *arg )
{
   int door;

   if( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) )
      door = 0;
   else if( !str_cmp( arg, "e" ) || !str_cmp( arg, "east" ) )
      door = 1;
   else if( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) )
      door = 2;
   else if( !str_cmp( arg, "w" ) || !str_cmp( arg, "west" ) )
      door = 3;
   else if( !str_cmp( arg, "u" ) || !str_cmp( arg, "up" ) )
      door = 4;
   else if( !str_cmp( arg, "d" ) || !str_cmp( arg, "down" ) )
      door = 5;
   else if( !str_cmp( arg, "ne" ) || !str_cmp( arg, "northeast" ) )
      door = 6;
   else if( !str_cmp( arg, "nw" ) || !str_cmp( arg, "northwest" ) )
      door = 7;
   else if( !str_cmp( arg, "se" ) || !str_cmp( arg, "southeast" ) )
      door = 8;
   else if( !str_cmp( arg, "sw" ) || !str_cmp( arg, "southwest" ) )
      door = 9;
   else
      door = -1;
   return door;
}

/* Function modified from original form - Samson 1-25-98 */
CMDF do_exits( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   EXIT_DATA *pexit;
   bool found, fAuto;

   buf[0] = '\0';
   fAuto = !str_cmp( argument, "auto" );

   if( !check_blind( ch ) )
      return;

   if( !IS_NPC( ch )
       && !IS_PLR_FLAG( ch, PLR_HOLYLIGHT )
       && !IS_AFFECTED( ch, AFF_TRUESIGHT ) && !IS_AFFECTED( ch, AFF_INFRARED ) && room_is_dark( ch->in_room, ch ) )
   {
      set_char_color( AT_DGREY, ch );
      send_to_char( "It is pitch black ... \r\n", ch );
      return;
   }

   if( fAuto && MXP_ON( ch ) )
      send_to_char( MXP_TAG_ROOMEXIT, ch );

   set_char_color( AT_EXITS, ch );

   mudstrlcpy( buf, fAuto ? "[Exits:" : "Obvious exits:\n\r", MSL );

   found = FALSE;
   for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
   {
      if( IS_IMMORTAL( ch ) )
         /*
          * Immortals see all exits, even secret ones 
          */
      {
         if( pexit->to_room )
         {
            found = TRUE;
            if( fAuto )
            {
               mudstrlcat( buf, " ", MSL );

               if( MXP_ON( ch ) )
                  mudstrlcat( buf, "<Ex>", MSL );

               mudstrlcat( buf, capitalize( dir_name[pexit->vdir] ), MSL );

               if( MXP_ON( ch ) )
                  mudstrlcat( buf, "</Ex>", MSL );

               if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
                  mudstrlcat( buf, "->(Overland)", MSL );

               /*
                * New code added to display closed, or otherwise invisible exits to immortals 
                */
               /*
                * Installed by Samson 1-25-98 
                */
               if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
                  mudstrlcat( buf, "->(Closed)", MSL );
               if( IS_EXIT_FLAG( pexit, EX_WINDOW ) )
                  mudstrlcat( buf, "->(Window)", MSL );
               if( IS_EXIT_FLAG( pexit, EX_HIDDEN ) )
                  mudstrlcat( buf, "->(Hidden)", MSL );
            }
            else
            {
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%s - %s\n\r", capitalize( dir_name[pexit->vdir] ),
                         pexit->to_room->name );

               /*
                * More new code added to display closed, or otherwise invisible exits to immortals 
                */
               /*
                * Installed by Samson 1-25-98 
                */
               if( room_is_dark( pexit->to_room, ch ) )
                  mudstrlcat( buf, " (Dark)", MSL );
               if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
                  mudstrlcat( buf, " (Closed)", MSL );
               if( IS_EXIT_FLAG( pexit, EX_HIDDEN ) )
                  mudstrlcat( buf, " (Hidden)", MSL );
               if( IS_EXIT_FLAG( pexit, EX_WINDOW ) )
                  mudstrlcat( buf, " (Window)", MSL );
               mudstrlcat( buf, "\n\r", MSL );
            }
         }
      }
      else
      {
         if( pexit->to_room && !IS_EXIT_FLAG( pexit, EX_SECRET ) && ( !IS_EXIT_FLAG( pexit, EX_WINDOW ) || IS_EXIT_FLAG( pexit, EX_ISDOOR ) ) && !IS_EXIT_FLAG( pexit, EX_HIDDEN ) && !IS_EXIT_FLAG( pexit, EX_FORTIFIED ) /* Checks for walls, Marcus */
             && !IS_EXIT_FLAG( pexit, EX_HEAVY )
             && !IS_EXIT_FLAG( pexit, EX_MEDIUM )
             && !IS_EXIT_FLAG( pexit, EX_LIGHT ) && !IS_EXIT_FLAG( pexit, EX_CRUMBLING ) )
         {
            found = TRUE;
            if( fAuto )
            {
               mudstrlcat( buf, " ", MSL );

               if( MXP_ON( ch ) )
                  mudstrlcat( buf, "<Ex>", MSL );

               mudstrlcat( buf, capitalize( dir_name[pexit->vdir] ), MSL );

               if( MXP_ON( ch ) )
                  mudstrlcat( buf, "</Ex>", MSL );

               if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
                  mudstrlcat( buf, "->(Closed)", MSL );
            }
            else
            {
               snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%s - %s\n\r", capitalize( dir_name[pexit->vdir] ),
                         room_is_dark( pexit->to_room, ch ) ? "Too dark to tell" : pexit->to_room->name );
            }
         }
      }
   }

   if( !found )
   {
      mudstrlcat( buf, fAuto ? " none]" : "None]", MSL );
      if( MXP_ON( ch ) )
         mudstrlcat( buf, MXP_TAG_ROOMEXIT_CLOSE, MSL );
   }
   else
   {
      if( fAuto )
      {
         mudstrlcat( buf, "]", MSL );
         if( MXP_ON( ch ) )
            mudstrlcat( buf, MXP_TAG_ROOMEXIT_CLOSE, MSL );
      }
   }

   mudstrlcat( buf, "\n\r", MSL );
   send_to_char( buf, ch );
   return;
}

void print_compass( CHAR_DATA * ch )
{
   EXIT_DATA *pexit;
   int exit_info[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
   static char *const exit_colors[] = { "&w", "&Y", "&C", "&b", "&w", "&R" };
   for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
   {
      if( !pexit->to_room || IS_EXIT_FLAG( pexit, EX_HIDDEN ) ||
          ( IS_EXIT_FLAG( pexit, EX_SECRET ) && IS_EXIT_FLAG( pexit, EX_CLOSED ) ) )
         continue;
      if( IS_EXIT_FLAG( pexit, EX_WINDOW ) )
         exit_info[pexit->vdir] = 2;
      else if( IS_EXIT_FLAG( pexit, EX_SECRET ) )
         exit_info[pexit->vdir] = 3;
      else if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
         exit_info[pexit->vdir] = 4;
      else if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
         exit_info[pexit->vdir] = 5;
      else
         exit_info[pexit->vdir] = 1;
   }
   ch_printf( ch, "\n\r&[rmname]%s%-50s%s         %s%s    %s%s    %s%s\n\r", MXP_ON( ch ) ? MXP_TAG_ROOMNAME : "",
              ch->in_room->name, MXP_ON( ch ) ? MXP_TAG_ROOMNAME_CLOSE : "", exit_colors[exit_info[DIR_NORTHWEST]],
              exit_info[DIR_NORTHWEST] ? "NW" : "- ", exit_colors[exit_info[DIR_NORTH]], exit_info[DIR_NORTH] ? "N" : "-",
              exit_colors[exit_info[DIR_NORTHEAST]], exit_info[DIR_NORTHEAST] ? "NE" : " -" );
   if( IS_IMMORTAL( ch ) && IS_PLR_FLAG( ch, PLR_ROOMVNUM ) )
      ch_printf( ch, "&w-<---- &YVnum: %6d &w----------------------------->-        ", ch->in_room->vnum );
   else
      send_to_char( "&w-<----------------------------------------------->-        ", ch );
   ch_printf( ch, "%s%s&w<-%s%s&w-&W(*)&w-%s%s&w->%s%s\n\r", exit_colors[exit_info[DIR_WEST]],
              exit_info[DIR_WEST] ? "W" : "-", exit_colors[exit_info[DIR_UP]], exit_info[DIR_UP] ? "U" : "-",
              exit_colors[exit_info[DIR_DOWN]], exit_info[DIR_DOWN] ? "D" : "-", exit_colors[exit_info[DIR_EAST]],
              exit_info[DIR_EAST] ? "E" : "-" );
   ch_printf( ch, "                                                           %s%s    %s%s    %s%s\n\r\n\r",
              exit_colors[exit_info[DIR_SOUTHWEST]], exit_info[DIR_SOUTHWEST] ? "SW" : "- ",
              exit_colors[exit_info[DIR_SOUTH]], exit_info[DIR_SOUTH] ? "S" : "-", exit_colors[exit_info[DIR_SOUTHEAST]],
              exit_info[DIR_SOUTHEAST] ? "SE" : " -" );
   return;
}

/* Function modified from original form on varying dates - Samson */
CMDF do_look( CHAR_DATA * ch, char *argument )
{
   char arg[MIL], arg1[MIL], arg2[MIL];
   char *pdesc;
   EXIT_DATA *pexit;
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   ROOM_INDEX_DATA *original;
   short door;
   int number, cnt;
   AREA_DATA *tarea = ch->in_room->area;

   if( !ch->desc )
      return;

   if( ch->position < POS_SLEEPING )
   {
      send_to_char( "You can't see anything but stars!\n\r", ch );
      return;
   }

   if( ch->position == POS_SLEEPING )
   {
      send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
      return;
   }

   if( !check_blind( ch ) )
      return;

   if( !IS_PLR_FLAG( ch, PLR_HOLYLIGHT ) && !IS_AFFECTED( ch, AFF_TRUESIGHT ) && room_is_dark( ch->in_room, ch ) )
   {
      send_to_char( "&zIt is pitch black ... \n\r", ch );
      if ( !*argument || !str_cmp( argument, "auto" ) )
         show_char_to_char( ch->in_room->first_person, ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !arg1 || arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
   {
      if( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_ACT_FLAG( ch, ACT_ONMAP ) )
      {
         display_map( ch );
         if( !ch->inflight )
         {
            show_list_to_char( ch->in_room->first_content, ch, FALSE, FALSE );
            show_char_to_char( ch->in_room->first_person, ch );
         }
         return;
      }

      /*
       * 'look' or 'look auto' 
       */
      if( IS_PLR_FLAG( ch, PLR_COMPASS ) )
         print_compass( ch );
      else
      {
         if( MXP_ON( ch ) )
            send_to_char( MXP_TAG_ROOMNAME, ch );

         set_char_color( AT_RMNAME, ch );

         /*
          * Added immortal option to see vnum when looking - Samson 4-3-98 
          */
         send_to_char( ch->in_room->name, ch );
         if( IS_IMMORTAL( ch ) && IS_PLR_FLAG( ch, PLR_ROOMVNUM ) )
            ch_printf( ch, "   &YVnum: %d", ch->in_room->vnum );

         if( MXP_ON( ch ) )
            send_to_char( MXP_TAG_ROOMNAME_CLOSE, ch );

         send_to_char( "\n\r", ch );
      }

      /*
       * Moved the exits to be under the name of the room 
       */
      /*
       * Yannick 24 september 1997                        
       */
      if( IS_PLR_FLAG( ch, PLR_AUTOEXIT ) )
         do_exits( ch, "auto" );

      /*
       * Room flag display installed by Samson 12-10-97 
       */
      /*
       * Forget exactly who, but thanks to the Smaug archives :) 
       */
      if( IS_IMMORTAL( ch ) && IS_PCFLAG( ch, PCFLAG_AUTOFLAGS ) )
      {
         tarea = ch->in_room->area;

         ch_printf( ch, "&[aflags][Area Flags: %s]\n\r",
                    ( tarea->flags > 0 ) ? flag_string( tarea->flags, area_flags ) : "none" );
         ch_printf( ch, "&[rflags][Room Flags: %s]\n\r", ext_flag_string( &ch->in_room->room_flags, r_flags ) );
      }

      /*
       * Room Sector display written and installed by Samson 12-10-97 
       */
      /*
       * Added Continent/Plane flag display on 3-28-98 
       */
      if( IS_IMMORTAL( ch ) && IS_PCFLAG( ch, PCFLAG_SECTORD ) )
      {
         ch_printf( ch, "&[stype][Sector Type: %s] [Continent or Plane: %s]\n\r",
                    sect_types[ch->in_room->sector_type], continents[tarea->continent] );
      }

      /*
       * Area name and filename display installed by Samson 12-13-97 
       */
      if( IS_IMMORTAL( ch ) && IS_PCFLAG( ch, PCFLAG_ANAME ) )
      {
         ch_printf( ch, "&[aname][Area name: %s]  ", ch->in_room->area->name );
         if( ch->level >= LEVEL_CREATOR )
            ch_printf( ch, "[Area filename: %s]\n\r", ch->in_room->area->filename );
         else
            send_to_char( "\n\r", ch );
      }

      set_char_color( AT_RMDESC, ch );

      /*
       * view desc or nitedesc --  Dracones 
       */
      if( !IS_PLR_FLAG( ch, PLR_BRIEF ) )
      {
         if( MXP_ON( ch ) )
            send_to_char( MXP_TAG_ROOMDESC, ch );

         if( time_info.hour >= sysdata.hoursunrise && time_info.hour <= sysdata.hoursunset )
            send_to_char( ch->in_room->roomdesc, ch );
         else
         {
            if( ch->in_room->nitedesc && ch->in_room->nitedesc[0] != '\0' )
               send_to_char( ch->in_room->nitedesc, ch );
            else
               send_to_char( ch->in_room->roomdesc, ch );
         }
         if( MXP_ON( ch ) )
            send_to_char( MXP_TAG_ROOMDESC_CLOSE, ch );
      }
      if( !IS_NPC( ch ) && ch->hunting )
         do_track( ch, GET_NAME( ch->hunting->who ) );

      mxpobjmenu = MXP_GROUND;
      mxptail[0] = '\0';

      show_list_to_char( ch->in_room->first_content, ch, FALSE, FALSE );
      show_char_to_char( ch->in_room->first_person, ch );
      return;
   }

   if( !str_cmp( arg1, "sky" ) || !str_cmp( arg1, "stars" ) )
   {
      if( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_ACT_FLAG( ch, ACT_ONMAP ) )
      {
         look_sky( ch );
         return;
      }

      if( !IS_OUTSIDE( ch ) || INDOOR_SECTOR( ch->in_room->sector_type ) )
      {
         send_to_char( "You can't see the sky indoors.\n\r", ch );
         return;
      }
      else
      {
         look_sky( ch );
         return;
      }
   }

   if( !str_cmp( arg1, "board" ) )  /* New note interface - Samson */
   {
      if( !( obj = get_obj_here( ch, arg1 ) ) )
      {
         send_to_char( "You do not see that here.\n\r", ch );
         return;
      }
      interpret( ch, "review" );
      return;
   }

   if( !str_cmp( arg1, "mailbag" ) )   /* New mail interface - Samson 4-29-99 */
   {
      if( !( obj = get_obj_here( ch, arg1 ) ) )
      {
         send_to_char( "You do not see that here.\n\r", ch );
         return;
      }

      interpret( ch, "mail list" );
      return;
   }

   if( !str_cmp( arg1, "under" ) )
   {
      int count;

      /*
       * 'look under' 
       */
      if( !arg2 || arg2[0] == '\0' )
      {
         send_to_char( "Look beneath what?\n\r", ch );
         return;
      }

      if( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
      {
         send_to_char( "You do not see that here.\n\r", ch );
         return;
      }
      if( !CAN_WEAR( obj, ITEM_TAKE ) && ch->level < sysdata.level_getobjnotake )
      {
         send_to_char( "You can't seem to get a grip on it.\n\r", ch );
         return;
      }
      if( ch->carry_weight + obj->weight > can_carry_w( ch ) )
      {
         send_to_char( "It's too heavy for you to look under.\n\r", ch );
         return;
      }
      count = obj->count;
      obj->count = 1;
      act( AT_PLAIN, "You lift $p and look beneath it:", ch, obj, NULL, TO_CHAR );
      act( AT_PLAIN, "$n lifts $p and looks beneath it:", ch, obj, NULL, TO_ROOM );
      obj->count = count;

      mxpobjmenu = MXP_GROUND;
      snprintf( mxptail, MIL, "%s", spacetodash( obj->name ) );

      if( IS_OBJ_FLAG( obj, ITEM_COVERING ) )
         show_list_to_char( obj->first_content, ch, TRUE, TRUE );
      else
         send_to_char( "Nothing.\n\r", ch );
      if( EXA_prog_trigger )
         oprog_examine_trigger( ch, obj );
      return;
   }

   /*
    * Look in 
    */
   if( !str_cmp( arg1, "i" ) || !str_cmp( arg1, "in" ) )
   {
      int count;

      /*
       * 'look in' 
       */
      if( !arg2 || arg2[0] == '\0' )
      {
         send_to_char( "Look in what?\n\r", ch );
         return;
      }

      if( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
      {
         send_to_char( "You do not see that here.\n\r", ch );
         return;
      }

      switch ( obj->item_type )
      {
         default:
            send_to_char( "That is not a container.\n\r", ch );
            break;

         case ITEM_DRINK_CON:
            if( obj->value[1] <= 0 )
            {
               send_to_char( "It is empty.\n\r", ch );
               if( EXA_prog_trigger )
                  oprog_examine_trigger( ch, obj );
               break;
            }
            {
               LIQ_TABLE *liq = get_liq_vnum( obj->value[2] );

               ch_printf( ch, "It's %s full of a %s liquid.\n\r", ( obj->value[1] * 10 ) < ( obj->value[0] * 10 ) / 4
                          ? "less than halfway" : ( obj->value[1] * 10 ) < 2 * ( obj->value[0] * 10 ) / 4
                          ? "around halfway" : ( obj->value[1] * 10 ) < 3 * ( obj->value[0] * 10 ) / 4
                          ? "more than halfway" : obj->value[1] == obj->value[0] ? "completely" : "almost", liq->color );
            }
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            break;

         case ITEM_PORTAL:
            for( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
            {
               if( pexit->vdir == DIR_PORTAL && IS_EXIT_FLAG( pexit, EX_PORTAL ) )
               {
                  if( room_is_private( pexit->to_room ) && ch->level < sysdata.level_override_private )
                  {
                     send_to_char( "&WThe room ahead is private!&D\n\r", ch );
                     return;
                  }

                  if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
                  {
                     original = ch->in_room;
                     enter_map( ch, pexit, pexit->x, pexit->y, -1 );
                     leave_map( ch, NULL, original );
                  }
                  else
                  {
                     bool visited;

                     visited = has_visited( ch, pexit->to_room->area );
                     original = ch->in_room;
                     char_from_room( ch );
                     if( !char_to_room( ch, pexit->to_room ) )
                        log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
                     do_look( ch, "auto" );
                     char_from_room( ch );
                     if( !char_to_room( ch, original ) )
                        log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
                     if( !visited )
                        remove_visit( ch, pexit->to_room );
                  }
               }
            }
            send_to_char( "You see swirling chaos...\n\r", ch );
            break;
         case ITEM_CONTAINER:
         case ITEM_QUIVER:
         case ITEM_KEYRING:
         case ITEM_CORPSE_NPC:
         case ITEM_CORPSE_PC:
            if( IS_SET( obj->value[1], CONT_CLOSED ) )
            {
               send_to_char( "It is closed.\n\r", ch );
               break;
            }
            count = obj->count;
            obj->count = 1;
            if( obj->item_type == ITEM_KEYRING )
               act( AT_PLAIN, "$p holds:", ch, obj, NULL, TO_CHAR );
            else
               act( AT_PLAIN, "$p contains:", ch, obj, NULL, TO_CHAR );
            obj->count = count;

            mxpobjmenu = MXP_CONT;
            snprintf( mxptail, MIL, "%s", spacetodash( obj->name ) );
            show_list_to_char( obj->first_content, ch, TRUE, TRUE );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            break;
      }
      return;
   }

   /*
    * finally fixed the annoying look 2.obj desc bug -Thoric 
    */
   number = number_argument( arg1, arg );
   for( cnt = 0, obj = ch->last_carrying; obj; obj = obj->prev_content )
   {
      if( can_see_obj( ch, obj, FALSE ) )
      {
         if( ( pdesc = get_extra_descr( arg, obj->first_extradesc ) ) != NULL )
         {
            if( ( cnt += obj->count ) < number )
               continue;
            send_to_char( pdesc, ch );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            return;
         }
         if( ( pdesc = get_extra_descr( arg, obj->pIndexData->first_extradesc ) ) != NULL )
         {
            if( ( cnt += obj->count ) < number )
               continue;
            send_to_char( pdesc, ch );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            return;
         }
         if( nifty_is_name_prefix( arg, obj->name ) )
         {
            if( ( cnt += obj->count ) < number )
               continue;
            pdesc = get_extra_descr( obj->name, obj->pIndexData->first_extradesc );
            if( !pdesc )
               pdesc = get_extra_descr( obj->name, obj->first_extradesc );
            if( !pdesc )
               send_to_char( "You see nothing special.\r\n", ch );
            else
               send_to_char( pdesc, ch );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            return;
         }
      }
   }

   for( obj = ch->in_room->last_content; obj; obj = obj->prev_content )
   {
      if( can_see_obj( ch, obj, FALSE ) )
      {
         if( ( pdesc = get_extra_descr( arg, obj->first_extradesc ) ) != NULL )
         {
            if( ( cnt += obj->count ) < number )
               continue;
            send_to_char( pdesc, ch );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            return;
         }
         if( ( pdesc = get_extra_descr( arg, obj->pIndexData->first_extradesc ) ) != NULL )
         {
            if( ( cnt += obj->count ) < number )
               continue;
            send_to_char( pdesc, ch );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            return;
         }
         if( nifty_is_name_prefix( arg, obj->name ) )
         {
            if( ( cnt += obj->count ) < number )
               continue;
            pdesc = get_extra_descr( obj->name, obj->pIndexData->first_extradesc );
            if( !pdesc )
               pdesc = get_extra_descr( obj->name, obj->first_extradesc );
            if( !pdesc )
               send_to_char( "You see nothing special.\r\n", ch );
            else
               send_to_char( pdesc, ch );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            return;
         }
      }
   }

   if( ( pdesc = get_extra_descr( arg1, ch->in_room->first_extradesc ) ) != NULL )
   {
      send_to_char( pdesc, ch );
      return;
   }

   door = get_door( arg1 );
   if( ( pexit = find_door( ch, arg1, TRUE ) ) != NULL )
   {
      if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && !IS_EXIT_FLAG( pexit, EX_WINDOW ) )
      {
         if( ( IS_EXIT_FLAG( pexit, EX_SECRET ) || IS_EXIT_FLAG( pexit, EX_DIG ) ) && door != -1 )
            send_to_char( "Nothing special there.\n\r", ch );
         else
            act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_BASHED ) )
         act( AT_RED, "The $d has been bashed from its hinges!", ch, NULL, pexit->keyword, TO_CHAR );

      if( pexit->exitdesc && pexit->exitdesc[0] != '\0' )
      {
         send_to_char( pexit->exitdesc, ch );
         send_to_char( "\n\r", ch );
      }
      else
         send_to_char( "Nothing special there.\n\r", ch );

      /*
       * Ability to look into the next room        -Thoric
       */
      if( pexit->to_room
          && ( is_affected( ch, gsn_spy ) || is_affected( ch, gsn_scout ) || is_affected( ch, gsn_scry ) ||
               IS_EXIT_FLAG( pexit, EX_xLOOK ) || ch->level >= LEVEL_IMMORTAL ) )
      {
         if( !IS_EXIT_FLAG( pexit, EX_xLOOK ) && ch->level < LEVEL_IMMORTAL )
         {
            set_char_color( AT_SKILL, ch );
            /*
             * Change by Narn, Sept 96 to allow characters who don't have the
             * scry spell to benefit from objects that are affected by scry.
             */
            /*
             * Except that I agree with DOTD logic - scrying doesn't work like this.
             * * Samson - 6-20-99
             */

            if( IS_ROOM_FLAG( pexit->to_room, ROOM_NOSCRY ) || IS_AREA_FLAG( pexit->to_room->area, AFLAG_NOSCRY ) )
            {
               send_to_char( "That room is magically protected. You cannot see inside.\n\r", ch );
               return;
            }

            if( !IS_NPC( ch ) )
            {
               int skill = -1, percent;

               if( is_affected( ch, gsn_spy ) )
                  skill = gsn_spy;
               if( is_affected( ch, gsn_scry ) )
                  skill = gsn_scry;
               if( is_affected( ch, gsn_scout ) )
                  skill = gsn_scout;

               if( skill == -1 )
                  skill = gsn_spy;

               percent = ch->pcdata->learned[skill];
               if( !percent )
                  percent = 35;  /* 95 was too good -Thoric */

               if( number_percent(  ) > percent )
               {
                  send_to_char( "You cannot get a good look.\n\r", ch );
                  return;
               }
            }
         }
         if( room_is_private( pexit->to_room ) && ch->level < sysdata.level_override_private )
         {
            send_to_char( "&WThe room ahead is private!&D\n\r", ch );
            return;
         }

         if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
         {
            original = ch->in_room;
            enter_map( ch, pexit, pexit->x, pexit->y, -1 );
            char_from_room( ch );
            if( !char_to_room( ch, original ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            leave_map( ch, NULL, NULL );
         }
         else
         {
            bool visited;

            visited = has_visited( ch, pexit->to_room->area );
            original = ch->in_room;
            char_from_room( ch );
            if( !char_to_room( ch, pexit->to_room ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            do_look( ch, "auto" );
            char_from_room( ch );
            if( !char_to_room( ch, original ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            if( !visited )
               remove_visit( ch, pexit->to_room );
         }
      }
      return;
   }
   else if( door != -1 )
   {
      send_to_char( "Nothing special there.\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg1 ) ) != NULL )
   {
      if( !is_ignoring( victim, ch ) )
      {
         show_char_to_char_1( victim, ch );
         return;
      }
   }

   send_to_char( "You do not see that here.\n\r", ch );
   return;
}

/* A much simpler version of look, this function will show you only
the condition of a mob or pc, or if used without an argument, the
same you would see if you enter the room and have config +brief.
-- Narn, winter '96
*/
CMDF do_glance( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   bool brief;

   if( !ch->desc )
      return;

   if( ch->position < POS_SLEEPING )
   {
      send_to_char( "You can't see anything but stars!\n\r", ch );
      return;
   }

   if( ch->position == POS_SLEEPING )
   {
      send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
      return;
   }

   if( !check_blind( ch ) )
      return;

   set_char_color( AT_ACTION, ch );

   if( !argument || argument[0] == '\0' )
   {
      if( IS_PLR_FLAG( ch, PLR_BRIEF ) )
         brief = TRUE;
      else
         brief = FALSE;
      SET_PLR_FLAG( ch, PLR_BRIEF );
      do_look( ch, "auto" );
      if( !brief )
         REMOVE_PLR_FLAG( ch, PLR_BRIEF );
      return;
   }

   if( ( victim = get_char_room( ch, argument ) ) == NULL )
   {
      send_to_char( "They're not here.\n\r", ch );
      return;
   }
   else
   {
      if( can_see( victim, ch, FALSE ) )
      {
         act( AT_ACTION, "$n glances at you.", ch, NULL, victim, TO_VICT );
         act( AT_ACTION, "$n glances at $N.", ch, NULL, victim, TO_NOTVICT );
      }
      if( IS_IMMORTAL( ch ) && victim != ch )
      {
         if( IS_NPC( victim ) )
            ch_printf( ch, "Mobile #%d '%s' ", victim->pIndexData->vnum, victim->name );
         else
            ch_printf( ch, "%s ", victim->name );
         ch_printf( ch, "is a level %d %s %s.\n\r",
                    victim->level,
                    IS_NPC( victim ) ? victim->race < MAX_NPC_RACE && victim->race >= 0 ?
                    npc_race[victim->race] : "unknown" : victim->race < MAX_PC_RACE &&
                    race_table[victim->race]->race_name &&
                    race_table[victim->race]->race_name[0] != '\0' ?
                    race_table[victim->race]->race_name : "unknown",
                    IS_NPC( victim ) ? victim->Class < MAX_NPC_CLASS && victim->Class >= 0 ?
                    npc_class[victim->Class] : "unknown" : victim->Class < MAX_PC_CLASS &&
                    class_table[victim->Class]->who_name &&
                    class_table[victim->Class]->who_name[0] != '\0' ? class_table[victim->Class]->who_name : "unknown" );
      }
      show_condition( ch, victim );
      return;
   }

   return;
}

CMDF do_examine( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   OBJ_DATA *obj;
   short dam;

   if( !ch )
   {
      bug( "%s", "do_examine: null ch." );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Examine what?\n\r", ch );
      return;
   }

   EXA_prog_trigger = FALSE;
   do_look( ch, argument );
   EXA_prog_trigger = TRUE;

   /*
    * Support for checking equipment conditions,
    * and support for trigger positions by Thoric
    */
   if( ( obj = get_obj_here( ch, argument ) ) != NULL )
   {
      switch ( obj->item_type )
      {
         default:
            break;

         case ITEM_ARMOR:
            ch_printf( ch, "Condition: %s\n\r", condtxt( obj->value[1], obj->value[0] ) );
            if( obj->value[2] > 0 )
               ch_printf( ch, "Available sockets: %d\n\r", obj->value[2] );
            if( obj->socket[0] && str_cmp( obj->socket[0], "None" ) )
               ch_printf( ch, "Socket 1: %s Rune\n\r", obj->socket[0] );
            if( obj->socket[1] && str_cmp( obj->socket[1], "None" ) )
               ch_printf( ch, "Socket 2: %s Rune\n\r", obj->socket[1] );
            if( obj->socket[2] && str_cmp( obj->socket[2], "None" ) )
               ch_printf( ch, "Socket 3: %s Rune\n\r", obj->socket[2] );
            break;

         case ITEM_WEAPON:
         case ITEM_MISSILE_WEAPON:
            ch_printf( ch, "Condition: %s\n\r", condtxt( obj->value[6], obj->value[0] ) );
            if( obj->value[7] > 0 )
               ch_printf( ch, "Available sockets: %d\n\r", obj->value[7] );
            if( obj->socket[0] && str_cmp( obj->socket[0], "None" ) )
               ch_printf( ch, "Socket 1: %s Rune\n\r", obj->socket[0] );
            if( obj->socket[1] && str_cmp( obj->socket[1], "None" ) )
               ch_printf( ch, "Socket 2: %s Rune\n\r", obj->socket[1] );
            if( obj->socket[2] && str_cmp( obj->socket[2], "None" ) )
               ch_printf( ch, "Socket 3: %s Rune\n\r", obj->socket[2] );
            break;

         case ITEM_PROJECTILE:
            ch_printf( ch, "Condition: %s\n\r", condtxt( obj->value[5], obj->value[0] ) );
            break;

         case ITEM_COOK:
            mudstrlcpy( buf, "As you examine it carefully you notice that it ", MSL );
            dam = obj->value[2];
            if( dam >= 3 )
               mudstrlcat( buf, "is burned to a crisp.", MSL );
            else if( dam == 2 )
               mudstrlcat( buf, "is a little over cooked.", MSL );   /* Bugfix 5-18-99 */
            else if( dam == 1 )
               mudstrlcat( buf, "is perfectly roasted.", MSL );
            else
               mudstrlcat( buf, "is raw.", MSL );
            mudstrlcat( buf, "\n\r", MSL );
            send_to_char( buf, ch );
         case ITEM_FOOD:
            if( obj->timer > 0 && obj->value[1] > 0 )
               dam = ( obj->timer * 10 ) / obj->value[1];
            else
               dam = 10;
            if( obj->item_type == ITEM_FOOD )
               mudstrlcpy( buf, "As you examine it carefully you notice that it ", MSL );
            else
               mudstrlcpy( buf, "Also it ", MSL );
            if( dam >= 10 )
               mudstrlcat( buf, "is fresh.", MSL );
            else if( dam == 9 )
               mudstrlcat( buf, "is nearly fresh.", MSL );
            else if( dam == 8 )
               mudstrlcat( buf, "is perfectly fine.", MSL );
            else if( dam == 7 )
               mudstrlcat( buf, "looks good.", MSL );
            else if( dam == 6 )
               mudstrlcat( buf, "looks ok.", MSL );
            else if( dam == 5 )
               mudstrlcat( buf, "is a little stale.", MSL );
            else if( dam == 4 )
               mudstrlcat( buf, "is a bit stale.", MSL );
            else if( dam == 3 )
               mudstrlcat( buf, "smells slightly off.", MSL );
            else if( dam == 2 )
               mudstrlcat( buf, "smells quite rank.", MSL );
            else if( dam == 1 )
               mudstrlcat( buf, "smells revolting!", MSL );
            else if( dam <= 0 )
               mudstrlcat( buf, "is crawling with maggots!", MSL );
            mudstrlcat( buf, "\n\r", MSL );
            send_to_char( buf, ch );
            break;


         case ITEM_SWITCH:
         case ITEM_LEVER:
         case ITEM_PULLCHAIN:
            if( IS_SET( obj->value[0], TRIG_UP ) )
               send_to_char( "You notice that it is in the up position.\n\r", ch );
            else
               send_to_char( "You notice that it is in the down position.\n\r", ch );
            break;

         case ITEM_BUTTON:
            if( IS_SET( obj->value[0], TRIG_UP ) )
               send_to_char( "You notice that it is depressed.\n\r", ch );
            else
               send_to_char( "You notice that it is not depressed.\n\r", ch );
            break;

         case ITEM_CORPSE_PC:
         case ITEM_CORPSE_NPC:
         {
            short timerfrac = obj->timer;
            if( obj->item_type == ITEM_CORPSE_PC )
               timerfrac = ( int )obj->timer / 8 + 1;

            switch ( timerfrac )
            {
               default:
                  send_to_char( "This corpse has recently been slain.\n\r", ch );
                  break;
               case 4:
                  send_to_char( "This corpse was slain a little while ago.\n\r", ch );
                  break;
               case 3:
                  send_to_char( "A foul smell rises from the corpse, and it is covered in flies.\n\r", ch );
                  break;
               case 2:
                  send_to_char( "A writhing mass of maggots and decay, you can barely go near this corpse.\n\r", ch );
                  break;
               case 1:
               case 0:
                  send_to_char( "Little more than bones, there isn't much left of this corpse.\n\r", ch );
                  break;
            }
         }
         case ITEM_CONTAINER:
            if( IS_OBJ_FLAG( obj, ITEM_COVERING ) )
               break;
         case ITEM_DRINK_CON:
         case ITEM_QUIVER:
            send_to_char( "When you look inside, you see:\n\r", ch );
         case ITEM_KEYRING:
            EXA_prog_trigger = FALSE;
            cmdf( ch, "look in %s", argument );
            EXA_prog_trigger = TRUE;
            break;
      }
      if( IS_OBJ_FLAG( obj, ITEM_COVERING ) )
      {
         EXA_prog_trigger = FALSE;
         cmdf( ch, "look under %s", argument );
         EXA_prog_trigger = TRUE;
      }
      oprog_examine_trigger( ch, obj );
      if( char_died( ch ) || obj_extracted( obj ) )
         return;
      check_for_trap( ch, obj, TRAP_EXAMINE );
   }
   return;
}

/*
 * Produce a description of the weather based on area weather using
 * the following sentence format:
 *		<combo-phrase> and <single-phrase>.
 * Where the combo-phrase describes either the precipitation and
 * temperature or the wind and temperature. The single-phrase
 * describes either the wind or precipitation depending upon the
 * combo-phrase.
 * Last Modified: July 31, 1997
 * Fireblade - Under Construction
 * Yeah. Big comment. Wasteful function. Eliminated unneeded strings -- Xorith
 */
CMDF do_weather( CHAR_DATA * ch, char *argument )
{
   int temp, precip, wind;

   if( !IS_PLR_FLAG( ch, PLR_ONMAP ) && ( !IS_OUTSIDE( ch ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) )
   {
      send_to_char( "You can't see the sky from here.\n\r", ch );
      return;
   }

   temp = ( ch->in_room->area->weather->temp + 3 * weath_unit - 1 ) / weath_unit;
   precip = ( ch->in_room->area->weather->precip + 3 * weath_unit - 1 ) / weath_unit;
   wind = ( ch->in_room->area->weather->wind + 3 * weath_unit - 1 ) / weath_unit;

   if( precip >= 3 )
      ch_printf( ch, "&B%s and %s.\n\r", preciptemp_msg[precip][temp], wind_msg[wind] );
   else
      ch_printf( ch, "&B%s and %s.\n\r", windtemp_msg[wind][temp], precip_msg[precip] );
   return;
}

CMDF do_compare( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   OBJ_DATA *obj1, *obj2;
   int value1, value2;
   char *msg;

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Compare what to what?\n\r", ch );
      return;
   }

   if( ( obj1 = get_obj_carry( ch, arg1 ) ) == NULL )
   {
      send_to_char( "You do not have that item.\n\r", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      for( obj2 = ch->first_carrying; obj2; obj2 = obj2->next_content )
      {
         if( obj2->wear_loc != WEAR_NONE && can_see_obj( ch, obj2, FALSE )
             && obj1->item_type == obj2->item_type && ( obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE ) != 0 )
            break;
      }

      if( !obj2 )
      {
         send_to_char( "You aren't wearing anything comparable.\n\r", ch );
         return;
      }
   }
   else
   {
      if( ( obj2 = get_obj_carry( ch, argument ) ) == NULL )
      {
         send_to_char( "You do not have that item.\n\r", ch );
         return;
      }
   }

   if( obj1->item_type != obj2->item_type )
   {
      ch_printf( ch, "Why would you compare a %s to a %s?\n\r", o_types[obj1->item_type], o_types[obj2->item_type] );
      return;
   }
   msg = NULL;
   value1 = 0;
   value2 = 0;

   if( obj1 == obj2 )
      msg = "You compare $p to itself.  It looks about the same.";
   else if( obj1->item_type != obj2->item_type )
      msg = "You can't compare $p and $P.";
   else
   {
      switch ( obj1->item_type )
      {
         default:
            msg = "You can't compare $p and $P.";
            break;

         case ITEM_ARMOR:
            value1 = obj1->value[0];
            value2 = obj2->value[0];
            break;

         case ITEM_WEAPON:
            value1 = obj1->value[1] + obj1->value[2];
            value2 = obj2->value[1] + obj2->value[2];
            break;
      }
   }
   if( !msg )
   {
      if( value1 == value2 )
         msg = "$p and $P look about the same.";
      else if( value1 > value2 )
         msg = "$p looks better than $P.";
      else
         msg = "$p looks worse than $P.";
   }
   act( AT_PLAIN, msg, ch, obj1, obj2, TO_CHAR );
   return;
}

CMDF do_oldwhere( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   DESCRIPTOR_DATA *d;
   bool found;

   if( !argument || argument[0] == '\0' )
   {
      pager_printf( ch, "\n\rPlayers near you in %s:\n\r", ch->in_room->area->name );
      found = FALSE;
      for( d = first_descriptor; d; d = d->next )
         if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
             && ( victim = d->character ) != NULL && !IS_NPC( victim ) && victim->in_room
             && victim->in_room->area == ch->in_room->area && can_see( ch, victim, TRUE ) && !is_ignoring( victim, ch ) )
         {
            found = TRUE;

            pager_printf( ch, "&[people]%-13s  ", victim->name );
            if( CAN_PKILL( victim ) && victim->pcdata->clan && victim->pcdata->clan->clan_type != CLAN_ORDER )
               pager_printf( ch, "%-18s\t", victim->pcdata->clan->badge );
            else if( CAN_PKILL( victim ) )
               send_to_pager( "(&wUnclanned&[people])\t", ch );
            else
               send_to_pager( "\t\t\t", ch );
            pager_printf( ch, "&[rmname]%s\n\r", victim->in_room->name );
         }
      if( !found )
         send_to_char( "None\n\r", ch );
   }
   else
   {
      found = FALSE;
      for( victim = first_char; victim; victim = victim->next )
         if( victim->in_room && victim->in_room->area == ch->in_room->area && !IS_AFFECTED( victim, AFF_HIDE )
             && !IS_AFFECTED( victim, AFF_SNEAK ) && can_see( ch, victim, TRUE ) && is_name( argument, victim->name ) )
         {
            found = TRUE;
            pager_printf( ch, "&[people]%-28s &[rmname]%s\n\r", PERS( victim, ch, TRUE ), victim->in_room->name );
            break;
         }
      if( !found )
         act( AT_PLAIN, "You didn't find any $T.", ch, NULL, argument, TO_CHAR );
   }
   return;
}

CMDF do_consider( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   char *msg;
   int diff;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Consider killing whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, argument ) ) == NULL )
   {
      send_to_char( "They're not here.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You decide you're pretty sure you could take yourself in a fight.\n\r", ch );
      return;
   }

   diff = victim->level - ch->level;

   if( diff <= -10 )
      msg = "You are far more experienced than $N.";
   else if( diff <= -5 )
      msg = "$N is not nearly as experienced as you.";
   else if( diff <= -2 )
      msg = "You are more experienced than $N.";
   else if( diff <= 1 )
      msg = "You are just about as experienced as $N.";
   else if( diff <= 4 )
      msg = "You are not nearly as experienced as $N.";
   else if( diff <= 9 )
      msg = "$N is far more experienced than you!";
   else
      msg = "$N would make a great teacher for you!";
   act( AT_CONSIDER, msg, ch, NULL, victim, TO_CHAR );

   diff = ( int )( victim->max_hit - ch->max_hit ) / 6;

   if( diff <= -200 )
      msg = "$N looks like a feather!";
   else if( diff <= -150 )
      msg = "You could kill $N with your hands tied!";
   else if( diff <= -100 )
      msg = "Hey! Where'd $N go?";
   else if( diff <= -50 )
      msg = "$N is a wimp.";
   else if( diff <= 0 )
      msg = "$N looks weaker than you.";
   else if( diff <= 50 )
      msg = "$N looks about as strong as you.";
   else if( diff <= 100 )
      msg = "It would take a bit of luck...";
   else if( diff <= 150 )
      msg = "It would take a lot of luck, and equipment!";
   else if( diff <= 200 )
      msg = "Why don't you dig a grave for yourself first?";
   else
      msg = "$N is built like a TANK!";
   act( AT_CONSIDER, msg, ch, NULL, victim, TO_CHAR );

   return;
}

CMDF do_wimpy( CHAR_DATA * ch, char *argument )
{
   int wimpy;

   if( !str_cmp( argument, "max" ) )
   {
      if( IS_PKILL( ch ) )
         wimpy = ( int )( ch->max_hit / 2.25 );
      else
         wimpy = ( int )( ch->max_hit / 1.2 );
   }
   else if( !argument || argument[0] == '\0' )
      wimpy = ( int )ch->max_hit / 5;
   else
      wimpy = atoi( argument );

   if( wimpy < 0 )
   {
      send_to_char( "&YYour courage exceeds your wisdom.\n\r", ch );
      return;
   }
   if( IS_PKILL( ch ) && wimpy > ( int )ch->max_hit / 2.25 )
   {
      send_to_char( "&YSuch cowardice ill becomes you.\n\r", ch );
      return;
   }
   else if( wimpy > ( int )ch->max_hit / 1.2 )
   {
      send_to_char( "&YSuch cowardice ill becomes you.\n\r", ch );
      return;
   }
   ch->wimpy = wimpy;
   ch_printf( ch, "&YWimpy set to %d hit points.\n\r", wimpy );
   return;
}

/* Encryption upgraded to SHA-256 - Samson 1-22-06 */
CMDF do_password( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   char *pArg, *pwdnew;
   char cEnd;

   if( IS_NPC( ch ) )
      return;

   /*
    * Can't use one_argument here because it smashes case.
    * So we just steal all its code.  Bleagh.
    */
   pArg = arg1;
   while( isspace( *argument ) )
      argument++;

   cEnd = ' ';
   if( *argument == '\'' || *argument == '"' )
      cEnd = *argument++;

   while( *argument != '\0' )
   {
      if( *argument == cEnd )
      {
         argument++;
         break;
      }
      *pArg++ = *argument++;
   }
   *pArg = '\0';

   pArg = arg2;
   while( isspace( *argument ) )
      argument++;

   cEnd = ' ';
   if( *argument == '\'' || *argument == '"' )
      cEnd = *argument++;

   while( *argument != '\0' )
   {
      if( *argument == cEnd )
      {
         argument++;
         break;
      }
      *pArg++ = *argument++;
   }
   *pArg = '\0';

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Syntax: password <new> <again>.\n\r", ch );
      return;
   }

   /*
    * This should stop all the mistyped password problems --Shaddai 
    */
   if( str_cmp( arg1, arg2 ) )
   {
      send_to_char( "Passwords don't match try again.\n\r", ch );
      return;
   }

   if( strlen( arg2 ) < 5 )
   {
      send_to_char( "New password must be at least five characters long.\n\r", ch );
      return;
   }

   if( arg1[0] == '!' || arg2[0] == '!' )
   {
      send_to_char( "New password cannot begin with the '!' character.\n\r", ch );
      return;
   }

   pwdnew = sha256_crypt( arg2 );   /* SHA-256 Encryption */
   DISPOSE( ch->pcdata->pwd );
   ch->pcdata->pwd = str_dup( pwdnew );
   save_char_obj( ch );
   send_to_char( "Ok.\n\r", ch );
   return;
}

/* Rewritten by Xorith. Cleaner, with sorted/all and partial name matching */
CMDF do_socials( CHAR_DATA *ch, char *argument )
{
   int col = 0, iHash = 0, count = 0;
   char letter = ' ';
   SOCIALTYPE *social;
   bool sorted = false, all = false;

   /* Bools are better on performance -- X */
   if( argument[0] != '\0' && !str_cmp( argument, "sorted" ) )
      sorted = true;
   if( argument[0] != '\0' && !str_cmp( argument, "all" ) )
      all = true;

   if( argument[0] != '\0' && !( sorted || all ) )
      pager_printf( ch, "&[plain]Socials beginning with '%s':\n\r", argument );
   else if( argument[0] != '\0' && sorted )
      send_to_pager( "&[plain]Socials -- Tabbed by Letter\n\r", ch );
   else
      send_to_pager( "&[plain]Socials -- All\n\r", ch );

   for( iHash = 0; iHash < 27; iHash++ )
      for( social = social_index[iHash]; social; social = social->next )
      {
         if( ( argument[0] != '\0' && !( sorted || all ) ) && str_prefix( argument, social->name ) )
            continue;

         if( argument[0] != '\0' && sorted && letter != social->name[0] )
         {
            letter = social->name[0];
            if( col % 5 != 0 )
               send_to_pager( "\n\r", ch );
            pager_printf( ch, "&c[ &[plain]%c &c]\n\r", toupper( letter ) );
            col = 0;
         }
         pager_printf( ch, "&[plain]%-15s ", social->name );
         count++;
         if( ++col % 5 == 0 )
            send_to_pager( "\n\r", ch );
      }

   if( col % 5 != 0 )
      send_to_pager( "\n\r", ch );

   if( count )
      pager_printf( ch, "&[plain]   %d socials found.\n\r", count );
   else
      send_to_pager( "&[plain]   No socials found.\n\r", ch );

   return;
}

/* Rewritten by Xorith to be cleaner, with "sorted" and "all" support. Also supports
    partial name matching. */
CMDF do_commands( CHAR_DATA *ch, char *argument )
{
   int col = 0, hash = 0, count = 0, chTrust = 0;
   char letter = ' ';
   CMDTYPE *command;
   bool sorted = false, all = false;

   /* Bools are better on performance -- X */
   if( argument[0] != '\0' && !str_cmp( argument, "sorted" ) )
      sorted = true;
   if( argument[0] != '\0' && !str_cmp( argument, "all" ) )
      all = true;
   chTrust = get_trust( ch );

   if( argument[0] != '\0' && !( sorted || all ) )
      pager_printf( ch, "&[plain]Commands beginning with '%s':\n\r", argument );
   else if( argument[0] != '\0' && sorted )
      send_to_pager( "&[plain]Commands -- Tabbed by Letter\n\r", ch );
   else
      send_to_pager( "&[plain]Commands -- All\n\r", ch );

   for( hash = 0; hash < 126; hash++ )
      for( command = command_hash[hash]; command; command = command->next )
      {
         if( command->level > LEVEL_AVATAR || command->level > chTrust || IS_SET( command->flags, CMD_MUDPROG ) )
            continue;

         if( ( argument[0] != '\0' && !( sorted || all ) ) && str_prefix( argument, command->name ) )
            continue;

         if( argument[0] != '\0' && sorted && letter != command->name[0] )
         {
            if( command->name[0] < 97 || command->name[0] > 122 )
            {
               if( !count )
                  send_to_pager( "&c[ &[plain]Misc&c ]\n\r", ch );
            }
            else
            {
               letter = command->name[0];
               if( col % 5 != 0 )
                  send_to_pager( "\n\r", ch );
               pager_printf( ch, "&c[ &[plain]%c &c]\n\r", toupper( letter ) );
               col = 0;
            }
         }
         pager_printf( ch, "&[plain]%-11s %-3d ", command->name, command->level );
         count++;
         if( ++col % 5 == 0 )
            send_to_pager( "\n\r", ch );
      }

   if( col % 5 != 0 )
      send_to_pager( "\n\r", ch );

   if( count )
      pager_printf( ch, "&[plain]  %d commands found.\n\r", count );
   else
      send_to_pager( "&[plain]  No commands found.\n\r", ch );

   return;
}

/*
 * display WIZLIST file						-Thoric
 */
CMDF do_wizlist( CHAR_DATA * ch, char *argument )
{
   set_pager_color( AT_IMMORT, ch );
   show_file( ch, WIZLIST_FILE );
}

#define PCFYN( ch, flag ) ( IS_PCFLAG( (ch), flag ) ? " &z(&GON&z)&D" : "&z(&ROFF&z)&D" )
#define PLRYN( ch, flag ) ( IS_PLR_FLAG( (ch), flag ) ? " &z(&GON&z)&D" : "&z(&ROFF&z)&D" )

/*
 * Contributed by Grodyn.
 * Display completely overhauled, 2/97 -- Blodkai
 */
/* Function modified from original form on varying dates - Samson */
/* Almost completely redone by Zarius - 1/30/2004 */
CMDF do_config( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "\n\r&gAFKMud Configurations ", ch );
      send_to_char( "&G(use 'config +/-<keyword>' to toggle, see 'help config')&D\n\r", ch );
      send_to_char( "&G--------------------------------------------------------------------------------&D\n\r\n\r", ch );
      send_to_char( "&g&uDisplay:&d   ", ch );
      ch_printf( ch, "&wPager        &z: %3s\t", PCFYN( ch, PCFLAG_PAGERON ) );
      ch_printf( ch, "&wGag          &z: %3s\t", PCFYN( ch, PCFLAG_GAG ) );
      ch_printf( ch, "&wBrief        &z: %3s\n\r", PLRYN( ch, PLR_BRIEF ) );
      ch_printf( ch, "           &wCombine      &z: %3s\t", PLRYN( ch, PLR_COMBINE ) );
      ch_printf( ch, "&wBlank        &z: %3s\t", PLRYN( ch, PLR_BLANK ) );
      ch_printf( ch, "&wPrompt       &z: %3s\n\r", PLRYN( ch, PLR_PROMPT ) );
      ch_printf( ch, "           &wAnsi         &z: %3s\t", PLRYN( ch, PLR_ANSI ) );
      ch_printf( ch, "&wCompass      &z: %3s\n\r", PLRYN( ch, PLR_COMPASS ) );

      /*
       * Config option for Smartsac added 1-18-00 - Samson 
       */
      send_to_char( "\n\r&g&uAuto:&d      ", ch );
      ch_printf( ch, "&wAutosac      &z: %3s\t", PLRYN( ch, PLR_AUTOSAC ) );
      ch_printf( ch, "&wAutogold     &z: %3s\t", PLRYN( ch, PLR_AUTOGOLD ) );
      ch_printf( ch, "&wAutoloot     &z: %3s\n\r", PLRYN( ch, PLR_AUTOLOOT ) );
      ch_printf( ch, "           &wAutoexit     &z: %3s\t", PLRYN( ch, PLR_AUTOEXIT ) );
      ch_printf( ch, "&wSmartsac     &z: %3s\t", PLRYN( ch, PLR_SMARTSAC ) );
      ch_printf( ch, "&wGuildsplit   &z: %3s\n\r", PLRYN( ch, PLR_GUILDSPLIT ) );
      ch_printf( ch, "           &wGroupsplit   &z: %3s\t", PLRYN( ch, PLR_GROUPSPLIT ) );
      ch_printf( ch, "&wAutoassist   &z: %3s\n\r", PLRYN( ch, PLR_AUTOASSIST ) );

      /*
       * PCFLAG_NOBEEP config option added by Samson 2-15-98 
       */
      send_to_char( "\n\r&g&uSafeties:&d  ", ch );
      ch_printf( ch, "&wNoRecall     &z: %3s\t", PCFYN( ch, PCFLAG_NORECALL ) );
      ch_printf( ch, "&wNoSummon     &z: %3s\t", PCFYN( ch, PCFLAG_NOSUMMON ) );
      ch_printf( ch, "&wNoBeep       &z: %3s\n\r", PCFYN( ch, PCFLAG_NOBEEP ) );

      if( !IS_PCFLAG( ch, PCFLAG_DEADLY ) )
      {
         ch_printf( ch, "           &wNoTell       &z: %3s\t", PCFYN( ch, PCFLAG_NOTELL ) );
         ch_printf( ch, "&wDrag         &z: %3s\n\r", PLRYN( ch, PLR_SHOVEDRAG ) );
      }
      else
         ch_printf( ch, "           &wNoTell       &z: %3s\n\r", PCFYN( ch, PCFLAG_NOTELL ) );

      send_to_char( "\n\r&g&uMisc:&d      ", ch );
      ch_printf( ch, "&wTelnet_GA    &z: %3s\t", PLRYN( ch, PLR_TELNET_GA ) );
      ch_printf( ch, "&wGroupwho     &z: %3s\t", PCFYN( ch, PCFLAG_GROUPWHO ) );
      ch_printf( ch, "&wNoIntro      &z: %3s\n\r", PCFYN( ch, PCFLAG_NOINTRO ) );
      ch_printf( ch, "           &wMSP          &z: %3s\t", PLRYN( ch, PLR_MSP ) );
      ch_printf( ch, "&wMXP          &z: %3s\t", PLRYN( ch, PLR_MXP ) );
      ch_printf( ch, "&wMXPPrompt    &z: %3s\n\r", PLRYN( ch, PLR_MXPPROMPT ) );
      ch_printf( ch, "           &wCheckboard   &z: %3s\t", PCFYN( ch, PCFLAG_CHECKBOARD ) );
      ch_printf( ch, "&wNoQuote      &z: %3s\n\r", PCFYN( ch, PCFLAG_NOQUOTE ) );

      /*
       * Config option for Room Flag display added by Samson 12-10-97 
       */
      /*
       * Config option for Sector Type display added by Samson 12-10-97 
       */
      /*
       * Config option Area name and filename added by Samson 12-13-97 
       */
      /*
       * Config option Passdoor added by Samson 3-21-98 
       */

      if( IS_IMMORTAL( ch ) )
      {
         send_to_char( "\n\r&g&uImmortal :&d ", ch );
         ch_printf( ch, "&wRoomvnum     &z: %3s\t", PLRYN( ch, PLR_ROOMVNUM ) );
         ch_printf( ch, "&wRoomflags    &z: %3s\t", PCFYN( ch, PCFLAG_AUTOFLAGS ) );
         ch_printf( ch, "&wSectortypes  &z: %3s\n\r", PCFYN( ch, PCFLAG_SECTORD ) );
         ch_printf( ch, "           &wFilename     &z: %3s\t", PCFYN( ch, PCFLAG_ANAME ) );
         ch_printf( ch, "&wPassdoor     &z: %3s\n\r", PCFYN( ch, PCFLAG_PASSDOOR ) );
      }

      send_to_char( "\n\r&g&uSettings:&d  ", ch );
      ch_printf( ch, "&wPager Length &z(&Y%d&z)    &wWimpy &z(&Y%d&z)&d\n\r", ch->pcdata->pagerlen, ch->wimpy );

      /*
       * Now only shows sentences section if you have any - Zarius 
       */
      if( IS_PLR_FLAG( ch, PLR_SILENCE )
          || IS_PLR_FLAG( ch, PLR_NO_EMOTE )
          || IS_PLR_FLAG( ch, PLR_NO_TELL ) || IS_PCFLAG( ch, PCFLAG_NOTITLE ) || IS_PLR_FLAG( ch, PLR_LITTERBUG ) )
      {  //added NOTITLE - Zarius
         send_to_char( "\n\r\n\r&g&uSentences imposed on you:&d\n\r", ch );
         ch_printf( ch, "&R%s%s%s%s%s&D",
                    IS_PLR_FLAG( ch, PLR_SILENCE ) ?
                    "For your abuse of channels, you are currently silenced.\n\r" : "",
                    IS_PLR_FLAG( ch, PLR_NO_EMOTE ) ?
                    "The gods have removed your emotes.\n\r" : "",
                    IS_PLR_FLAG( ch, PLR_NO_TELL ) ?
                    "You are not permitted to send 'tells' to others.\n\r" : "",
                    IS_PCFLAG( ch, PCFLAG_NOTITLE ) ?
                    "You are not permitted to change your title.\n\r" : "",
                    IS_PLR_FLAG( ch, PLR_LITTERBUG ) ? "A convicted litterbug.  You cannot drop anything.\n\r" : "\n\r" );
      }
   }
   else
   {
      bool fSet;
      int bit = 0;

      if( argument[0] == '+' )
         fSet = TRUE;
      else if( argument[0] == '-' )
         fSet = FALSE;
      else
      {
         send_to_char( "Config -option or +option?\n\r", ch );
         return;
      }

      if( !str_prefix( argument + 1, "autoexit" ) )
         bit = PLR_AUTOEXIT;
      else if( !str_prefix( argument + 1, "autoloot" ) )
         bit = PLR_AUTOLOOT;
      else if( !str_prefix( argument + 1, "autosac" ) )
         bit = PLR_AUTOSAC;
      else if( !str_prefix( argument + 1, "smartsac" ) )
         bit = PLR_SMARTSAC;
      else if( !str_prefix( argument + 1, "autogold" ) )
         bit = PLR_AUTOGOLD;
      else if( !str_prefix( argument + 1, "guildsplit" ) )
         bit = PLR_GUILDSPLIT;
      else if( !str_prefix( argument + 1, "groupsplit" ) )
         bit = PLR_GROUPSPLIT;
      else if( !str_prefix( argument + 1, "autoassist" ) )
         bit = PLR_AUTOASSIST;
      else if( !str_prefix( argument + 1, "blank" ) )
         bit = PLR_BLANK;
      else if( !str_prefix( argument + 1, "brief" ) )
         bit = PLR_BRIEF;
      else if( !str_prefix( argument + 1, "combine" ) )
         bit = PLR_COMBINE;
      else if( !str_prefix( argument + 1, "prompt" ) )
         bit = PLR_PROMPT;
      else if( !str_prefix( argument + 1, "telnetga" ) )
         bit = PLR_TELNET_GA;
      else if( !str_prefix( argument + 1, "msp" ) )
         bit = PLR_MSP;
      else if( !str_prefix( argument + 1, "mxp" ) )
         bit = PLR_MXP;
      else if( !str_prefix( argument + 1, "ansi" ) )
         bit = PLR_ANSI;
      else if( !str_prefix( argument + 1, "compass" ) )
         bit = PLR_COMPASS;
      else if( !str_prefix( argument + 1, "mxpprompt" ) )
         bit = PLR_MXPPROMPT;
      else if( !str_prefix( argument + 1, "drag" ) )
         bit = PLR_SHOVEDRAG;
      else if( IS_IMMORTAL( ch ) && !str_prefix( argument + 1, "roomvnum" ) )
         bit = PLR_ROOMVNUM;

      /*
       * removed PLR_FLEE check here, because it should never happen since there is no toggle - Zarius
       */
      if( bit )
      {
         if( ( bit == PLR_SHOVEDRAG ) && IS_PCFLAG( ch, PCFLAG_DEADLY ) )
         {
            send_to_char( "Pkill characters can not config that option.\n\r", ch );
            return;
         }

         if( fSet )
         {
            SET_PLR_FLAG( ch, bit );
            ch_printf( ch, "&Y%s &wis now &GON\n\r", capitalize( argument + 1 ) );
         }
         else
         {
            REMOVE_PLR_FLAG( ch, bit );
            ch_printf( ch, "&Y%s &wis now &ROFF\n\r", capitalize( argument + 1 ) );
         }
         return;
      }
      else
      {
         if( !str_prefix( argument + 1, "norecall" ) )
            bit = PCFLAG_NORECALL;
         else if( !str_prefix( argument + 1, "nointro" ) )
            bit = PCFLAG_NOINTRO;
         else if( !str_prefix( argument + 1, "nosummon" ) )
            bit = PCFLAG_NOSUMMON;
         else if( !str_prefix( argument + 1, "gag" ) )
            bit = PCFLAG_GAG;
         else if( !str_prefix( argument + 1, "pager" ) )
            bit = PCFLAG_PAGERON;
         else if( !str_prefix( argument + 1, "nobeep" ) )
            bit = PCFLAG_NOBEEP;
         else if( !str_prefix( argument + 1, "passdoor" ) )
            bit = PCFLAG_PASSDOOR;
         else if( !str_prefix( argument + 1, "groupwho" ) )
            bit = PCFLAG_GROUPWHO;
         else if( !str_prefix( argument + 1, "@hgflag_" ) )
            bit = PCFLAG_HIGHGAG;
         else if( !str_prefix( argument + 1, "notell" ) )
            bit = PCFLAG_NOTELL;
         else if( !str_prefix( argument + 1, "checkboard" ) )
            bit = PCFLAG_CHECKBOARD;
         else if( !str_prefix( argument + 1, "noquote" ) )
            bit = PCFLAG_NOQUOTE;
         else if( IS_IMMORTAL( ch ) )
         {
            if( !str_prefix( argument + 1, "roomflags" ) )
               bit = PCFLAG_AUTOFLAGS;
            else if( !str_prefix( argument + 1, "sectortypes" ) )
               bit = PCFLAG_SECTORD;
            else if( !str_prefix( argument + 1, "filename" ) )
               bit = PCFLAG_ANAME;
            else
            {
               send_to_char( "Config -option or +option?\n\r", ch );
               return;
            }
         }
         else
         {
            send_to_char( "Config -option or +option?\n\r", ch );
            return;
         }

         if( fSet )
         {
            SET_PCFLAG( ch, bit );
            ch_printf( ch, "&Y%s &wis now &GON\n\r", capitalize( argument + 1 ) );
         }
         else
         {
            REMOVE_PCFLAG( ch, bit );
            ch_printf( ch, "&Y%s &wis now &ROFF\n\r", capitalize( argument + 1 ) );
         }
         return;
      }
   }

   return;
}

/* Maintained strictly for license compliance. Do not remove. */
CMDF do_credits( CHAR_DATA * ch, char *argument )
{
   interpret( ch, "help credits" );
}

/* History File viewer added by Samson 2-12-98 - At Dwip's request */
CMDF do_history( CHAR_DATA * ch, char *argument )
{
   set_pager_color( AT_SOCIAL, ch );
   show_file( ch, HISTORY_FILE );
}

CMDF do_news( CHAR_DATA * ch, char *argument )
{
   set_pager_color( AT_NOTE, ch );
   interpret( ch, "help news" );
}

/* Statistical display for the mud, added by Samson 1-31-98 */
CMDF do_world( CHAR_DATA * ch, char *argument )
{
   send_to_char( "&cBase source code: Smaug 1.4a\n\r", ch );
   ch_printf( ch, "Current source revision: %s %s\n\r", CODENAME, CODEVERSION );
   send_to_char( "The MUD first came online on: Thu Sep 4 1997\n\r", ch );
   ch_printf( ch, "The MUD last rebooted on: %s\n\r", str_boot_time );
   ch_printf( ch, "The system time is      : %s\n\r", c_time( current_time, -1 ) );
   ch_printf( ch, "Your local time is      : %s\n\r", c_time( current_time, ch->pcdata->timezone ) );
   ch_printf( ch, "\n\rTotal number of zones in the world: %d\n\r", top_area );
   ch_printf( ch, "Total number of rooms in the world: %d\n\r", top_room );
   ch_printf( ch, "\n\rNumber of distinct mobs in the world: %d\n\r", top_mob_index );
   ch_printf( ch, "Number of mobs loaded into the world: %d\n\r", nummobsloaded );
   ch_printf( ch, "\n\rNumber of distinct objects in the world: %d\n\r", top_obj_index );
   ch_printf( ch, "Number of objects loaded into the world: %d\n\r", numobjsloaded );
   ch_printf( ch, "\n\rCurrent number of registered players: %d\n\r", num_pfiles );
   ch_printf( ch, "All time high number of players on at one time: %d\n\r", sysdata.alltimemax );
}

/* Reason buffer added to AFK. See also show_char_to_char_0. -Whir - 8/31/98 */
CMDF do_afk( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
      return;

   if( IS_PLR_FLAG( ch, PLR_AFK ) )
   {
      REMOVE_PLR_FLAG( ch, PLR_AFK );
      send_to_char( "You are no longer afk.\n\r", ch );
      DISPOSE( ch->pcdata->afkbuf );
      act( AT_GREY, "$n is no longer afk.", ch, NULL, NULL, TO_ROOM );
   }
   else
   {
      SET_PLR_FLAG( ch, PLR_AFK );
      send_to_char( "You are now afk.\n\r", ch );
      DISPOSE( ch->pcdata->afkbuf );
      if( !argument || argument[0] == '\0' )
         ch->pcdata->afkbuf = str_dup( argument );
      act( AT_GREY, "$n is now afk.", ch, NULL, NULL, TO_ROOM );
      return;
   }
}

CMDF do_busy( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
      return;

   if( IS_PLR_FLAG( ch, PLR_BUSY ) )
   {
      REMOVE_PLR_FLAG( ch, PLR_BUSY );
      send_to_char( "You are no longer busy.\n\r", ch );
      act( AT_GREY, "$n is no longer busy.", ch, NULL, NULL, TO_ROOM );
   }
   else
   {
      SET_PLR_FLAG( ch, PLR_BUSY );
      send_to_char( "You are now busy.\n\r", ch );
      act( AT_GREY, "$n is now busy.", ch, NULL, NULL, TO_ROOM );
      return;
   }
}

/* A buncha different stuff added/subtracted/rearranged in do_slist to remove 
 * annoying, but harmless bugs, as well as provide better functionality - Adjani 12-05-2002 
 */
CMDF do_slist( CHAR_DATA * ch, char *argument )
{
   int sn, i, lFound;
   char skn[MIL], arg1[MIL], arg2[MIL];
   int lowlev, hilev;
   short lasttype = SKILL_SPELL;
   int cl = ch->Class;

   if( IS_NPC( ch ) )
      return;

   lowlev = 1;
   hilev = LEVEL_AVATAR;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1 && arg1[0] != '\0' && !is_number( arg1 ) )
   {
      cl = get_npc_class( arg1 );
      if( cl < 0 || cl > MAX_CLASS )
      {
         ch_printf( ch, "%s isn't a valid class!\n\r", arg1 );
         return;
      }

      if( arg2 && arg2[0] != '\0' )
      {
         if( is_number( arg2 ) )
         {
            lowlev = atoi( arg2 );
            if( lowlev < 1 || lowlev > LEVEL_AVATAR )
            {
               ch_printf( ch, "%d is out of range. Only valid between 1 and %d\n\r", lowlev, LEVEL_AVATAR );
               return;
            }
         }
      }

      if( argument && argument[0] != '\0' )
      {
         if( is_number( argument ) )
         {
            hilev = atoi( argument );
            if( hilev < 1 || hilev > LEVEL_AVATAR )
            {
               ch_printf( ch, "%d is out of range. Only valid between 1 and %d\n\r", hilev, LEVEL_AVATAR );
               return;
            }
         }
      }
   }
   else if( arg1 && arg1[0] != '\0' && is_number( arg1 ) )
   {
      lowlev = atoi( arg1 );
      if( lowlev < 1 || lowlev > LEVEL_AVATAR )
      {
         ch_printf( ch, "%d is out of range. Only valid between 1 and %d\n\r", lowlev, LEVEL_AVATAR );
         return;
      }

      if( arg2 && arg2[0] != '\0' && is_number( arg2 ) )
      {
         hilev = atoi( arg2 );
         if( hilev < 1 || hilev > LEVEL_AVATAR )
         {
            ch_printf( ch, "%d is out of range. Only valid between 1 and %d\n\r", hilev, LEVEL_AVATAR );
            return;
         }
      }
   }

   if( lowlev > hilev )
      lowlev = hilev;

   set_pager_color( AT_MAGIC, ch );
   pager_printf( ch, "%s Spell & Skill List\n\r", class_table[cl]->who_name );
   send_to_pager( "--------------------------------------\n\r", ch );

   for( i = lowlev; i <= hilev; i++ )
   {
      lFound = 0;
      mudstrlcpy( skn, "Spell", MIL );
      for( sn = 0; sn < top_sn; sn++ )
      {
         if( !skill_table[sn]->name )
            break;

         if( skill_table[sn]->type != lasttype )
         {
            lasttype = skill_table[sn]->type;
            mudstrlcpy( skn, skill_tname[lasttype], MIL );
         }

         if( ch->pcdata->learned[sn] <= 0 && SPELL_FLAG( skill_table[sn], SF_SECRETSKILL ) )
            continue;

         if( i == skill_table[sn]->skill_level[cl] || i == skill_table[sn]->race_level[ch->race] )
         {
            int xx = cl;
            if( skill_table[sn]->type == SKILL_RACIAL )
               xx = ch->race;

            if( !lFound )
            {
               lFound = 1;
               pager_printf( ch, "Level %d\n\r", i );
            }

            pager_printf( ch, "%7s: %20.20s \t Current: %-3d Max: %-3d  MinPos: %s \n\r",
                          skn, skill_table[sn]->name, ch->pcdata->learned[sn], skill_table[sn]->skill_adept[cl],
                          npc_position[skill_table[sn]->minimum_position] );
         }
      }
   }
   return;
}

CMDF do_pager( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      if( IS_PCFLAG( ch, PCFLAG_PAGERON ) )
      {
         send_to_char( "Pager disabled.\n\r", ch );
         do_config( ch, "-pager" );
      }
      else
      {
         ch_printf( ch, "Pager is now enabled at %d lines.\n\r", ch->pcdata->pagerlen );
         do_config( ch, "+pager" );
      }
      return;
   }
   if( !is_number( argument ) )
   {
      send_to_char( "Set page pausing to how many lines?\n\r", ch );
      return;
   }
   ch->pcdata->pagerlen = atoi( argument );
   if( ch->pcdata->pagerlen < 5 )
      ch->pcdata->pagerlen = 5;
   ch_printf( ch, "Page pausing set to %d lines.\n\r", ch->pcdata->pagerlen );
   return;
}

/* Command so imms can view the imotd - Samson 1-20-01 */
CMDF do_imotd( CHAR_DATA * ch, char *argument )
{
   if( exists_file( IMOTD_FILE ) )
      show_file( ch, IMOTD_FILE );
   return;
}

/* Command so people can call up the MOTDs - Samson 1-20-01 */
CMDF do_motd( CHAR_DATA * ch, char *argument )
{
   if( exists_file( MOTD_FILE ) )
      show_file( ch, MOTD_FILE );
   return;
}

/* Saves MOTDs to disk - Samson 12-31-00 */
void save_motd( char *name, char *str )
{
   FILE *fp;

   if( ( fp = fopen( name, "w" ) ) == NULL )
   {
      bug( "%s", "save_motd: fopen" );
      perror( name );
   }
   else
      fprintf( fp, "%s", str );
   FCLOSE( fp );
   return;
}

void load_motd( CHAR_DATA * ch, char *name )
{
   FILE *fp;
   char buf[MSL];
   int c;
   int num = 0;

   if( ( fp = fopen( name, "r" ) ) == NULL )
   {
      bug( "%s", "load_motd: Cannot open" );
      perror( name );
   }
   while( !feof( fp ) )
   {
      while( ( buf[num] = fgetc( fp ) ) != EOF && buf[num] != '\n' && buf[num] != '\r' && num < ( MSL - 2 ) )
         num++;
      c = fgetc( fp );
      if( ( c != '\n' && c != '\r' ) || c == buf[num] )
         ungetc( c, fp );
      buf[num++] = '\n';
      buf[num++] = '\r';
      buf[num] = '\0';
   }
   FCLOSE( fp );
   DISPOSE( ch->pcdata->motd_buf );
   ch->pcdata->motd_buf = str_dup( buf );
   return;
}

/* Handles editing the MOTDs on the server, independent of helpfiles now - Samson 12-31-00 */
CMDF do_motdedit( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];

   if( IS_NPC( ch ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_RESTRICTED:
         send_to_char( "You cannot do this while in another command.\n\r", ch );
         return;

      case SUB_EDMOTD:
         DISPOSE( ch->pcdata->motd_buf );
         ch->pcdata->motd_buf = copy_buffer_nohash( ch );
         stop_editing( ch );
         ch->substate = ch->tempnum;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         send_to_char( "Aborting MOTD message.\n\r", ch );
         return;
   }

   argument = one_argument( argument, arg1 );

   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Usage: motdedit <field>\n\r", ch );
      send_to_char( "Usage: motdedit save <field>\n\r\n\r", ch );
      send_to_char( "Field can be one of:\n\r", ch );
      send_to_char( "motd imotd\n\r", ch );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      if( ch->pcdata->motd_buf == NULL )
      {
         send_to_char( "Nothing to save.\n\r", ch );
         return;
      }
      if( !str_cmp( ch->pcdata->motd_buf, "" ) )
      {
         send_to_char( "Nothing to save.\n\r", ch );
         return;
      }
      if( !str_cmp( argument, "motd" ) )
      {
         save_motd( MOTD_FILE, ch->pcdata->motd_buf );
         send_to_char( "MOTD Message updated.\n\r", ch );
         DISPOSE( ch->pcdata->motd_buf );
         sysdata.motd = current_time;
         save_sysdata( sysdata );
         return;
      }
      if( !str_cmp( argument, "imotd" ) )
      {
         save_motd( IMOTD_FILE, ch->pcdata->motd_buf );
         send_to_char( "IMOTD Message updated.\n\r", ch );
         DISPOSE( ch->pcdata->motd_buf );
         sysdata.imotd = current_time;
         save_sysdata( sysdata );
         return;
      }
      do_motdedit( ch, "" );
      return;
   }
   if( !str_cmp( arg1, "motd" ) || !str_cmp( arg1, "imotd" ) )
   {
      if( !str_cmp( arg1, "motd" ) )
         load_motd( ch, MOTD_FILE );
      else
         load_motd( ch, IMOTD_FILE );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_EDMOTD;
      ch->pcdata->dest_buf = ch;
      if( !ch->pcdata->motd_buf || ch->pcdata->motd_buf[0] == '\0' )
         ch->pcdata->motd_buf = str_dup( "" );
      start_editing( ch, ch->pcdata->motd_buf );
      set_editor_desc( ch, "An MOTD." );
      return;
   }
   do_motdedit( ch, "" );
   return;
}

typedef struct ignore_data IGNORE_DATA;

/* Structure for link list of ignored players */
struct ignore_data
{
   IGNORE_DATA *next;
   IGNORE_DATA *prev;
   char *name;
};

void free_ignores( CHAR_DATA * ch )
{
   IGNORE_DATA *temp, *next;

   /*
    * free up memory allocated to stored ignored names 
    */
   for( temp = ch->pcdata->first_ignored; temp; temp = next )
   {
      next = temp->next;
      UNLINK( temp, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
      STRFREE( temp->name );
      DISPOSE( temp );
   }
   return;
}

void save_ignores( CHAR_DATA * ch, FILE * fp )
{
   IGNORE_DATA *temp;

   for( temp = ch->pcdata->first_ignored; temp; temp = temp->next )
      fprintf( fp, "Ignored      %s~\n", temp->name );
}

void load_ignores( CHAR_DATA * ch, FILE * fp )
{
   char *temp;
   char fname[256];
   struct stat fst;
   int ign;
   IGNORE_DATA *inode;

   /*
    * Get the name 
    */
   temp = fread_flagstring( fp );

   snprintf( fname, 256, "%s%c/%s", PLAYER_DIR, tolower( temp[0] ), capitalize( temp ) );

   /*
    * If there isn't a pfile for the name 
    */
   /*
    * then don't add it to the list       
    */
   if( stat( fname, &fst ) == -1 )
      return;

   /*
    * Count the number of names already ignored 
    */
   for( ign = 0, inode = ch->pcdata->first_ignored; inode; inode = inode->next )
      ign++;

   /*
    * Add the name unless the limit has been reached 
    */
   if( ign >= sysdata.maxign )
      bug( "%s", "load_ignores: too many ignored names" );
   else
   {
      /*
       * Add the name to the list 
       */
      CREATE( inode, IGNORE_DATA, 1 );
      inode->name = STRALLOC( temp );

      LINK( inode, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
   }
   return;
}

/*
 * The ignore command allows players to ignore up to MAX_IGN
 * other players. Players may ignore other characters whether
 * they are online or not. This is to prevent people from
 * spamming someone and then logging off quickly to evade
 * being ignored.
 * Syntax:
 *	ignore		-	lists players currently ignored
 *	ignore none	-	sets it so no players are ignored
 *	ignore <player>	-	start ignoring player if not already
 *				ignored otherwise stop ignoring player
 *	ignore reply	-	start ignoring last player to send a
 *				tell to ch, to deal with invis spammers
 * Last Modified: June 26, 1997
 * - Fireblade
 */
CMDF do_ignore( CHAR_DATA * ch, char *argument )
{
   IGNORE_DATA *temp, *next;
   char fname[256], fname2[256];
   struct stat fst;
   CHAR_DATA *victim;

   if( IS_NPC( ch ) )
      return;

   /*
    * If no arguements, then list players currently ignored 
    */
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "\n\r&[divider]----------------------------------------\n\r", ch );
      send_to_char( "&GYou are currently ignoring:\n\r", ch );
      send_to_char( "&[divider]----------------------------------------\n\r", ch );

      if( !ch->pcdata->first_ignored )
      {
         send_to_char( "&[ignore]\t    no one\n\r", ch );
         return;
      }

      send_to_char( "&[ignore]", ch );
      for( temp = ch->pcdata->first_ignored; temp; temp = temp->next )
         ch_printf( ch, "\t  - %s\n\r", temp->name );
      return;
   }

   /*
    * Clear players ignored if given arg "none" 
    */
   else if( !str_cmp( argument, "none" ) )
   {
      for( temp = ch->pcdata->first_ignored; temp; temp = next )
      {
         next = temp->next;
         UNLINK( temp, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
         STRFREE( temp->name );
         DISPOSE( temp );
      }
      send_to_char( "&[ignore]You now ignore no one.\n\r", ch );
      return;
   }

   /*
    * Prevent someone from ignoring themself... 
    */
   else if( !str_cmp( argument, "self" ) || nifty_is_name( argument, ch->name ) )
   {
      send_to_char( "&[ignore]Cannot ignore yourself.\n\r", ch );
      return;
   }

   else
   {
      int i;

      snprintf( fname, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ) );
      snprintf( fname2, 256, "%s/%s", GOD_DIR, capitalize( argument ) );

      victim = NULL;

      /*
       * get the name of the char who last sent tell to ch 
       */
      if( !str_cmp( argument, "reply" ) )
      {
         if( !ch->reply )
         {
            ch_printf( ch, "&[ignore]%s is not here.\n\r", argument );
            return;
         }
         else
            mudstrlcpy( argument, ch->reply->name, MSL );
      }

      /*
       * Loop through the linked list of ignored players, keep track of how many are being ignored 
       */
      for( temp = ch->pcdata->first_ignored, i = 0; temp; temp = temp->next, i++ )
      {
         /*
          * If the argument matches a name in list remove it 
          */
         if( !str_cmp( temp->name, capitalize( argument ) ) )
         {
            UNLINK( temp, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
            ch_printf( ch, "&[ignore]You no longer ignore %s.\n\r", temp->name );
            STRFREE( temp->name );
            DISPOSE( temp );
            return;
         }
      }

      /*
       * if there wasn't a match check to see if the name is valid.
       * * This if-statement may seem like overkill but it is intended to prevent people from doing the
       * * spam and log thing while still allowing ya to ignore new chars without pfiles yet... 
       */
      if( stat( fname, &fst ) == -1 && ( !( victim = get_char_world( ch, argument ) )
                                         || IS_NPC( victim ) || str_cmp( capitalize( argument ), victim->name ) != 0 ) )
      {
         ch_printf( ch, "&[ignore]No player exists by the name %s.\n\r", argument );
         return;
      }

      if( !check_parse_name( capitalize( argument ), FALSE ) )
      {
         send_to_char( "That's not a valid name to ignore!\n\r", ch );
         return;
      }

      if( stat( fname2, &fst ) != -1 )
      {
         send_to_char( "&[ignore]You cannot ignore an immortal.\n\r", ch );
         return;
      }

      if( victim )
         mudstrlcpy( capitalize( argument ), victim->name, MSL );

      /*
       * If its valid and the list size limit has not been reached create a node and at it to the list 
       */
      if( i < sysdata.maxign )
      {
         IGNORE_DATA *inew;
         CREATE( inew, IGNORE_DATA, 1 );
         inew->name = STRALLOC( capitalize( argument ) );
         LINK( inew, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
         ch_printf( ch, "&[ignore]You now ignore %s.\n\r", inew->name );
         return;
      }
      else
      {
         ch_printf( ch, "&[ignore]You may only ignore %d players.\n\r", sysdata.maxign );
         return;
      }
   }
}

/*
 * This function simply checks to see if ch is ignoring ign_ch.
 * Last Modified: October 10, 1997
 * - Fireblade
 */
bool is_ignoring( CHAR_DATA * ch, CHAR_DATA * ign_ch )
{
   IGNORE_DATA *temp;

   if( !ch )   /* Paranoid bug check, you never know. */
   {
      bug( "%s", "is_ignoring: NULL CH!" );
      return FALSE;
   }

   if( !ign_ch )  /* Bail out, webwho can access this and ign_ch will be NULL */
      return FALSE;

   if( IS_NPC( ch ) || IS_NPC( ign_ch ) )
      return FALSE;

   for( temp = ch->pcdata->first_ignored; temp; temp = temp->next )
   {
      if( nifty_is_name( temp->name, ign_ch->name ) )
         return TRUE;
   }
   return FALSE;
}
