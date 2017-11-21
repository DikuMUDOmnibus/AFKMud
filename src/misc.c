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
 * Note: Most of the stuff in here would go in act_obj.c, but act_obj was   *
 *                               getting big.                               *
 ****************************************************************************/

#include "mud.h"
#include "liquids.h"

void sound_to_char( const char *fname, int volume, CHAR_DATA * ch, bool toroom );
LIQ_TABLE *get_liq_vnum( int vnum );
ch_ret check_room_for_traps( CHAR_DATA * ch, int flag );
void teleport( CHAR_DATA * ch, int room, int flags );
void raw_kill( CHAR_DATA *ch, CHAR_DATA *victim );
extern int top_exit;

/* generate an action description message */
void actiondesc( CHAR_DATA * ch, OBJ_DATA * obj, void *vo )
{
   LIQ_TABLE *liq = NULL;
   char charbuf[MSL], roombuf[MSL];
   char *srcptr = obj->action_desc;
   char *charptr = charbuf;
   char *roomptr = roombuf;
   const char *ichar = "You";
   const char *iroom = "Someone";

   while( *srcptr != '\0' )
   {
      if( *srcptr == '$' )
      {
         srcptr++;
         switch ( *srcptr )
         {
            case 'e':
               ichar = "you";
               iroom = "$e";
               break;

            case 'm':
               ichar = "you";
               iroom = "$m";
               break;

            case 'n':
               ichar = "you";
               iroom = "$n";
               break;

            case 's':
               ichar = "your";
               iroom = "$s";
               break;

               /*
                * case 'q':
                * iroom = "s";
                * break;
                */

            default:
               srcptr--;
               *charptr++ = *srcptr;
               *roomptr++ = *srcptr;
               break;
         }
      }
      else if( *srcptr == '%' && *++srcptr == 's' )
      {
         ichar = "You";
         iroom = "$n";
      }
      else
      {
         *charptr++ = *srcptr;
         *roomptr++ = *srcptr;
         srcptr++;
         continue;
      }

      while( ( *charptr = *ichar ) != '\0' )
      {
         charptr++;
         ichar++;
      }

      while( ( *roomptr = *iroom ) != '\0' )
      {
         roomptr++;
         iroom++;
      }
      srcptr++;
   }
   *charptr = '\0';
   *roomptr = '\0';

   switch ( obj->item_type )
   {
      case ITEM_BLOOD:
      case ITEM_FOUNTAIN:
         act( AT_ACTION, charbuf, ch, obj, ch, TO_CHAR );
         act( AT_ACTION, roombuf, ch, obj, ch, TO_ROOM );
         return;

      case ITEM_DRINK_CON:
         if( ( liq = get_liq_vnum( obj->value[2] ) ) == NULL )
            bug( "Do_look: bad liquid number %d.", obj->value[2] );

         act( AT_ACTION, charbuf, ch, obj, liq->shortdesc, TO_CHAR );
         act( AT_ACTION, roombuf, ch, obj, liq->shortdesc, TO_ROOM );
         return;

      case ITEM_PIPE:
         return;

      case ITEM_ARMOR:
      case ITEM_WEAPON:
      case ITEM_LIGHT:
         return;

      case ITEM_COOK:
      case ITEM_FOOD:
      case ITEM_PILL:
         act( AT_ACTION, charbuf, ch, obj, ch, TO_CHAR );
         act( AT_ACTION, roombuf, ch, obj, ch, TO_ROOM );
         return;

      default:
         return;
   }
}

CMDF do_eat( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   OBJ_DATA *obj;
   ch_ret retcode;
   int foodcond;
   bool hgflag = TRUE;
   bool immH = FALSE;   /* Immune to hunger */
   bool immT = FALSE;   /* Immune to thirst */

   if( argument[0] == '\0' )
   {
      send_to_char( "Eat what?\n\r", ch );
      return;
   }

   if( IS_NPC( ch ) || ch->pcdata->condition[COND_FULL] > 5 )
      if( ms_find_obj( ch ) )
         return;

   if( ( obj = find_obj( ch, argument, TRUE ) ) == NULL )
      return;

   if( !IS_IMMORTAL( ch ) )
   {
      if( obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL && obj->item_type != ITEM_COOK )
      {
         act( AT_ACTION, "$n starts to nibble on $p... ($e must really be hungry)", ch, obj, NULL, TO_ROOM );
         act( AT_ACTION, "You try to nibble on $p...", ch, obj, NULL, TO_CHAR );
         return;
      }

      if( !IS_NPC( ch ) && ch->pcdata->condition[COND_FULL] > sysdata.maxcondval - 8 )
      {
         send_to_char( "You are too full to eat more.\n\r", ch );
         return;
      }
   }

   if( !IS_NPC( ch ) && ch->pcdata->condition[COND_FULL] == -1 )
      immH = TRUE;

   if( !IS_NPC( ch ) && ch->pcdata->condition[COND_THIRST] == -1 )
      immT = TRUE;

   if( !IS_PKILL( ch ) || ( IS_PKILL( ch ) && !IS_PCFLAG( ch, PCFLAG_HIGHGAG ) ) )
      hgflag = FALSE;

   /*
    * required due to object grouping 
    */
   separate_obj( obj );
   if( obj->in_obj )
   {
      if( !hgflag )
         act( AT_PLAIN, "You take $p from $P.", ch, obj, obj->in_obj, TO_CHAR );
      act( AT_PLAIN, "$n takes $p from $P.", ch, obj, obj->in_obj, TO_ROOM );
   }
   if( ch->fighting && number_percent(  ) > ( get_curr_dex( ch ) * 2 + 47 ) )
   {
      snprintf( buf, MSL, "%s",
                ( ch->in_room->sector_type == SECT_UNDERWATER ||
                  ch->in_room->sector_type == SECT_WATER_SWIM ||
                  ch->in_room->sector_type == SECT_WATER_NOSWIM ||
                  ch->in_room->sector_type == SECT_RIVER ) ? "dissolves in the water" :
                ( ch->in_room->sector_type == SECT_AIR ||
                  IS_ROOM_FLAG( ch->in_room, ROOM_NOFLOOR ) ) ? "falls far below" : "is trampled underfoot" );
      act( AT_MAGIC, "$n drops $p, and it $T.", ch, obj, buf, TO_ROOM );
      if( !hgflag )
         act( AT_MAGIC, "Oops, $p slips from your hand and $T!", ch, obj, buf, TO_CHAR );
   }
   else
   {
      if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
      {
         if( !obj->action_desc || obj->action_desc[0] == '\0' )
         {
            act( AT_ACTION, "$n eats $p.", ch, obj, NULL, TO_ROOM );
            if( !hgflag )
               act( AT_ACTION, "You eat $p.", ch, obj, NULL, TO_CHAR );
            sound_to_char( "eat.wav", 100, ch, FALSE );
         }
         else
            actiondesc( ch, obj, NULL );
      }

      switch ( obj->item_type )
      {

         case ITEM_COOK:
         case ITEM_FOOD:
            WAIT_STATE( ch, sysdata.pulsepersec / 3 );
            if( obj->timer > 0 && obj->value[1] > 0 )
               foodcond = ( obj->timer * 10 ) / obj->value[1];
            else
               foodcond = 10;

            if( !IS_NPC( ch ) )
            {
               int condition;

               condition = ch->pcdata->condition[COND_FULL];
               gain_condition( ch, COND_FULL, ( obj->value[0] * foodcond ) / 10 );
               if( condition <= 1 && ch->pcdata->condition[COND_FULL] > 1 )
                  send_to_char( "You are no longer hungry.\n\r", ch );
               else if( ch->pcdata->condition[COND_FULL] > sysdata.maxcondval * 0.8 )
                  send_to_char( "You are full.\n\r", ch );
            }

            if( obj->value[3] != 0 || ( foodcond < 4 && number_range( 0, foodcond + 1 ) == 0 )
                || ( obj->item_type == ITEM_COOK && obj->value[2] == 0 ) )
            {
               /*
                * The food was poisoned! 
                */
               AFFECT_DATA af;

               /*
                * Half Trolls are not affected by bad food 
                */
               if( ch->race == RACE_HALF_TROLL )
                  ch_printf( ch, "%s was poisoned, but had no affect on you.\n\r", obj->short_descr );

               else
               {
                  if( obj->value[3] != 0 )
                  {
                     act( AT_POISON, "$n chokes and gags.", ch, NULL, NULL, TO_ROOM );
                     act( AT_POISON, "You choke and gag.", ch, NULL, NULL, TO_CHAR );
                     ch->mental_state = URANGE( 20, ch->mental_state + 5, 100 );
                  }
                  else
                  {
                     act( AT_POISON, "$n gags on $p.", ch, obj, NULL, TO_ROOM );
                     act( AT_POISON, "You gag on $p.", ch, obj, NULL, TO_CHAR );
                     ch->mental_state = URANGE( 15, ch->mental_state + 5, 100 );
                  }

                  af.type = gsn_poison;
                  af.duration = 2 * obj->value[0] * ( obj->value[3] > 0 ? obj->value[3] : 1 );
                  af.location = APPLY_NONE;
                  af.modifier = 0;
                  af.bit = AFF_POISON;
                  affect_join( ch, &af );
               }
            }
            break;

         case ITEM_PILL:
            if( who_fighting( ch ) && IS_PKILL( ch ) )
               WAIT_STATE( ch, sysdata.pulsepersec / 4 );
            else
               WAIT_STATE( ch, sysdata.pulsepersec / 3 );
            /*
             * allow pills to fill you, if so desired 
             */
            if( !IS_NPC( ch ) && obj->value[4] )
            {
               int condition;

               condition = ch->pcdata->condition[COND_FULL];
               gain_condition( ch, COND_FULL, obj->value[4] );
               if( condition <= 1 && ch->pcdata->condition[COND_FULL] > 1 )
                  send_to_char( "You are no longer hungry.\n\r", ch );
               else if( ch->pcdata->condition[COND_FULL] > sysdata.maxcondval - 8 )
                  send_to_char( "You are full.\n\r", ch );
            }
            retcode = obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
            if( retcode == rNONE )
               retcode = obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
            if( retcode == rNONE )
               retcode = obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
            break;
      }
   }
   extract_obj( obj );

   /*
    * Reset immunity for those who have it 
    */
   if( !IS_NPC( ch ) )
   {
      if( immH )
         ch->pcdata->condition[COND_FULL] = -1;
      if( immT )
         ch->pcdata->condition[COND_THIRST] = -1;
   }
   return;
}

CMDF do_quaff( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj;
   ch_ret retcode;
   bool hgflag = TRUE;

   if( argument[0] == '\0' || !str_cmp( argument, "" ) )
   {
      send_to_char( "Quaff what?\n\r", ch );
      return;
   }

   if( ( obj = find_obj( ch, argument, TRUE ) ) == NULL )
      return;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
      return;

   if( obj->item_type != ITEM_POTION )
   {
      if( obj->item_type == ITEM_DRINK_CON )
         cmdf( ch, "drink %s", obj->name );
      else
      {
         act( AT_ACTION, "$n lifts $p up to $s mouth and tries to drink from it...", ch, obj, NULL, TO_ROOM );
         act( AT_ACTION, "You bring $p up to your mouth and try to drink from it...", ch, obj, NULL, TO_CHAR );
      }
      return;
   }

   /*
    * Empty container check              -Shaddai
    */
   if( obj->value[1] == -1 && obj->value[2] == -1 && obj->value[3] == -1 )
   {
      send_to_char( "You suck in nothing but air.\n\r", ch );
      return;
   }

   if( !IS_PKILL( ch ) || ( IS_PKILL( ch ) && !IS_PCFLAG( ch, PCFLAG_HIGHGAG ) ) )
      hgflag = FALSE;

   separate_obj( obj );
   if( obj->in_obj )
   {
      if( !CAN_PKILL( ch ) )
      {
         act( AT_PLAIN, "You take $p from $P.", ch, obj, obj->in_obj, TO_CHAR );
         act( AT_PLAIN, "$n takes $p from $P.", ch, obj, obj->in_obj, TO_ROOM );
      }
   }

   /*
    * If fighting, chance of dropping potion         -Thoric
    */
   if( ch->fighting && number_percent(  ) > ( get_curr_dex( ch ) * 2 + 48 ) )
   {
      act( AT_MAGIC, "$n fumbles $p and shatters it into fragments.", ch, obj, NULL, TO_ROOM );
      if( !hgflag )
         act( AT_MAGIC, "Oops... $p is knocked from your hand and shatters!", ch, obj, NULL, TO_CHAR );
   }
   else
   {
      if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
      {
         if( !CAN_PKILL( ch ) || !obj->in_obj )
         {
            act( AT_ACTION, "$n quaffs $p.", ch, obj, NULL, TO_ROOM );
            if( !hgflag )
               act( AT_ACTION, "You quaff $p.", ch, obj, NULL, TO_CHAR );
         }
         else if( obj->in_obj )
         {
            act( AT_ACTION, "$n quaffs $p from $P.", ch, obj, obj->in_obj, TO_ROOM );
            if( !hgflag )
               act( AT_ACTION, "You quaff $p from $P.", ch, obj, obj->in_obj, TO_CHAR );
         }
      }

      if( who_fighting( ch ) && IS_PKILL( ch ) )
         WAIT_STATE( ch, sysdata.pulsepersec / 5 );
      else
         WAIT_STATE( ch, sysdata.pulsepersec / 3 );

      retcode = obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
      if( retcode == rNONE )
         retcode = obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
      if( retcode == rNONE )
         retcode = obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
   }
   extract_obj( obj );
   return;
}

CMDF do_recite( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *scroll, *obj;
   ch_ret retcode;

   argument = one_argument( argument, arg1 );

   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Recite what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( scroll = get_obj_carry( ch, arg1 ) ) )
   {
      send_to_char( "You do not have that scroll.\n\r", ch );
      return;
   }

   if( scroll->item_type != ITEM_SCROLL )
   {
      act( AT_ACTION, "$n holds up $p as if to recite something from it...", ch, scroll, NULL, TO_ROOM );
      act( AT_ACTION, "You hold up $p and stand there with your mouth open.  (Now what?)", ch, scroll, NULL, TO_CHAR );
      return;
   }

   if( IS_NPC( ch ) && ( scroll->pIndexData->vnum == OBJ_VNUM_SCROLL_SCRIBING ) )
   {
      send_to_char( "As a mob, this dialect is foreign to you.\n\r", ch );
      return;
   }

   if( ( scroll->pIndexData->vnum == OBJ_VNUM_SCROLL_SCRIBING ) && ( ch->level + 10 < scroll->value[0] ) )
   {
      send_to_char( "This scroll is too complex for you to understand.\n\r", ch );
      return;
   }

   obj = NULL;
   if( !argument || argument[0] == '\0' )
      victim = ch;
   else
   {
      if( ( victim = get_char_room( ch, argument ) ) == NULL && ( obj = get_obj_here( ch, argument ) ) == NULL )
      {
         send_to_char( "You can't find it.\n\r", ch );
         return;
      }
   }

   separate_obj( scroll );
   act( AT_MAGIC, "$n recites $p.", ch, scroll, NULL, TO_ROOM );
   act( AT_MAGIC, "You recite $p.", ch, scroll, NULL, TO_CHAR );

   if( victim != ch )
      WAIT_STATE( ch, 2 * sysdata.pulseviolence );
   else
      WAIT_STATE( ch, sysdata.pulsepersec / 2 );

   retcode = obj_cast_spell( scroll->value[1], scroll->value[0], ch, victim, obj );
   if( retcode == rNONE )
      retcode = obj_cast_spell( scroll->value[2], scroll->value[0], ch, victim, obj );
   if( retcode == rNONE )
      retcode = obj_cast_spell( scroll->value[3], scroll->value[0], ch, victim, obj );

   extract_obj( scroll );
   return;
}

/*
 * Function to handle the state changing of a triggerobject (lever)  -Thoric
 */
void pullorpush( CHAR_DATA * ch, OBJ_DATA * obj, bool pull )
{
   CHAR_DATA *rch;
   bool isup;
   ROOM_INDEX_DATA *room, *to_room;
   EXIT_DATA *pexit, *pexit_rev;
   int edir;
   char *txt;

   if( IS_SET( obj->value[0], TRIG_UP ) )
      isup = TRUE;
   else
      isup = FALSE;
   switch ( obj->item_type )
   {
      default:
         ch_printf( ch, "You can't %s that!\n\r", pull ? "pull" : "push" );
         return;
      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
         if( ( !pull && isup ) || ( pull && !isup ) )
         {
            ch_printf( ch, "It is already %s.\n\r", isup ? "up" : "down" );
            return;
         }
         break;
      case ITEM_BUTTON:
         if( ( !pull && isup ) || ( pull && !isup ) )
         {
            ch_printf( ch, "It is already %s.\n\r", isup ? "in" : "out" );
            return;
         }
         break;
   }

   if( ( pull ) && HAS_PROG( obj->pIndexData, PULL_PROG ) )
   {
      if( !IS_SET( obj->value[0], TRIG_AUTORETURN ) )
         REMOVE_BIT( obj->value[0], TRIG_UP );
      oprog_pull_trigger( ch, obj );
      return;
   }

   if( ( !pull ) && HAS_PROG( obj->pIndexData, PUSH_PROG ) )
   {
      if( !IS_SET( obj->value[0], TRIG_AUTORETURN ) )
         SET_BIT( obj->value[0], TRIG_UP );
      oprog_push_trigger( ch, obj );
      return;
   }

   if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
   {
      act_printf( AT_ACTION, ch, obj, NULL, TO_ROOM, "$n %s $p.", pull ? "pulls" : "pushes" );
      act_printf( AT_ACTION, ch, obj, NULL, TO_CHAR, "You %s $p.", pull ? "pull" : "push" );
   }

   if( !IS_SET( obj->value[0], TRIG_AUTORETURN ) )
   {
      if( pull )
         REMOVE_BIT( obj->value[0], TRIG_UP );
      else
         SET_BIT( obj->value[0], TRIG_UP );
   }

   if( IS_SET( obj->value[0], TRIG_TELEPORT )
       || IS_SET( obj->value[0], TRIG_TELEPORTALL ) || IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) )
   {
      int flags;

      if( !( room = get_room_index( obj->value[1] ) ) )
      {
         bug( "PullOrPush: obj points to invalid room %d", obj->value[1] );
         return;
      }
      flags = 0;
      if( IS_SET( obj->value[0], TRIG_SHOWROOMDESC ) )
         SET_BIT( flags, TELE_SHOWDESC );
      if( IS_SET( obj->value[0], TRIG_TELEPORTALL ) )
         SET_BIT( flags, TELE_TRANSALL );
      if( IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) )
         SET_BIT( flags, TELE_TRANSALLPLUS );

      teleport( ch, obj->value[1], flags );
      return;
   }

   if( IS_SET( obj->value[0], TRIG_RAND4 ) || IS_SET( obj->value[0], TRIG_RAND6 ) )
   {
      int maxd;

      if( !( room = get_room_index( obj->value[1] ) ) )
      {
         bug( "PullOrPush: obj points to invalid room %d", obj->value[1] );
         return;
      }

      if( IS_SET( obj->value[0], TRIG_RAND4 ) )
         maxd = 3;
      else
         maxd = 5;

      randomize_exits( room, maxd );
      for( rch = room->first_person; rch; rch = rch->next_in_room )
      {
         send_to_char( "You hear a loud rumbling sound.\n\r", rch );
         send_to_char( "Something seems different...\n\r", rch );
      }
   }

   /* Death support added by Remcon */
   if( IS_SET( obj->value[0], TRIG_DEATH ) )
   {
      /* Should we really send a message to the room? */
      act( AT_DEAD, "$n falls prey to a terrible death!", ch, NULL, NULL, TO_ROOM );
      act( AT_DEAD, "Oopsie... you're dead!\r\n", ch, NULL, NULL, TO_CHAR );
      log_printf( "%s hit a DEATH TRIGGER in room %d!", ch->name, ch->in_room->vnum );

      /* Personaly I figured if we wanted it to be a full DT we could just have it send them into a DT. */
      raw_kill( ch, ch );
      return;
   }

   /* Object loading added by Remcon */
   if( IS_SET( obj->value[0], TRIG_OLOAD ) )
   {
      OBJ_INDEX_DATA *pObjIndex;
      OBJ_DATA *tobj;

      /* value[1] for the obj vnum */
      if( !( pObjIndex = get_obj_index( obj->value[1] ) ) )
      {
         bug( "%s: obj points to invalid object vnum %d", __FUNCTION__, obj->value[1] );
         return;
      }
      /* Set room to NULL before the check */
      room = NULL;
      /* value[2] for the room vnum to put the object in if there is one, 0 for giving it to char or current room */
      if( obj->value[2] > 0 && !( room = get_room_index( obj->value[2] ) ) )
      {
         bug( "%s: obj points to invalid room vnum %d", __FUNCTION__, obj->value[2] );
         return;
      }
      /* Uses value[3] for level */
      if( !( tobj = create_object( pObjIndex, URANGE( 0, obj->value[3], MAX_LEVEL ) ) ) )
      {
         bug( "%s: obj couldnt create_obj vnum %d at level %d", __FUNCTION__, obj->value[1], obj->value[3] );
         return;
      }
      if( room )
         obj_to_room( tobj, room, ch );
      else
      {
         if( CAN_WEAR( obj, ITEM_TAKE ) )
            obj_to_char( tobj, ch );
         else
            obj_to_room( tobj, ch->in_room, ch );
      }
      return;
   }

   /* Mob loading added by Remcon */
   if( IS_SET( obj->value[0], TRIG_MLOAD ) )
   {
      MOB_INDEX_DATA *pMobIndex;
      CHAR_DATA *mob;

      /* value[1] for the obj vnum */
      if( !( pMobIndex = get_mob_index( obj->value[1] ) ) )
      {
         bug( "%s: obj points to invalid mob vnum %d", __FUNCTION__, obj->value[1] );
         return;
      }
      /* Set room to current room before the check */
      room = ch->in_room;
      /* value[2] for the room vnum to put the object in if there is one, 0 for giving it to char or current room */
      if( obj->value[2] > 0 && !( room = get_room_index( obj->value[2] ) ) )
      {
         bug( "%s: obj points to invalid room vnum %d", __FUNCTION__, obj->value[2] );
         return;
      }
      if( !( mob = create_mobile( pMobIndex ) ) )
      {
         bug( "%s: obj couldnt create_mobile vnum %d", __FUNCTION__, obj->value[1] );
         return;
      }
      char_to_room( mob, room );
      return;
   }

   /* Spell casting support added by Remcon */
   if( IS_SET( obj->value[0], TRIG_CAST ) )
   {
      if( obj->value[1] <= 0 || !IS_VALID_SN( obj->value[1] ) )
      {
         bug( "%s: obj points to invalid sn [%d]", __FUNCTION__, obj->value[1] );
         return;
      }
      obj_cast_spell( obj->value[1], URANGE( 1, ( obj->value[2] > 0 ) ? obj->value[2] : ch->level, MAX_LEVEL ), ch, ch, NULL );
      return;
   }

   /* Container support added by Remcon */
   if( IS_SET( obj->value[0], TRIG_CONTAINER ) )
   {
      OBJ_DATA *container = NULL;

      room = get_room_index( obj->value[1] );
      if( !room )
         room = obj->in_room;
      if( !room )
      {
         bug( "%s: obj points to invalid room %d", __FUNCTION__, obj->value[1] );
         return;
      }

      for( container = ch->in_room->first_content; container; container = container->next_content )
      {
         if( container->pIndexData->vnum == obj->value[2] )
            break;
      }
      if( !container )
      {
         bug( "%s: obj points to a container [%d] thats not where it should be?", __FUNCTION__, obj->value[2] );
         return;
      }
      if( container->item_type != ITEM_CONTAINER )
      {
         bug( "%s: obj points to object [%d], but it isn't a container.", __FUNCTION__, obj->value[2] );
         return;
      }
      /* Could toss in some messages. Limit how it is handled etc... I'll leave that to each one to do */
      /* Started to use TRIG_OPEN, TRIG_CLOSE, TRIG_LOCK, and TRIG_UNLOCK like TRIG_DOOR does. */
      /* It limits it alot, but it wouldn't allow for an EATKEY change */
      if( IS_SET( obj->value[3], CONT_CLOSEABLE ) )
         TOGGLE_BIT( container->value[1], CONT_CLOSEABLE );
      if( IS_SET( obj->value[3], CONT_PICKPROOF ) )
         TOGGLE_BIT( container->value[1], CONT_PICKPROOF );
      if( IS_SET( obj->value[3], CONT_CLOSED ) )
         TOGGLE_BIT( container->value[1], CONT_CLOSED );
      if( IS_SET( obj->value[3], CONT_LOCKED ) )
         TOGGLE_BIT( container->value[1], CONT_LOCKED );
      if( IS_SET( obj->value[3], CONT_EATKEY ) )
         TOGGLE_BIT( container->value[1], CONT_EATKEY );
      return;
   }

   if( IS_SET( obj->value[0], TRIG_DOOR ) )
   {
      room = get_room_index( obj->value[1] );
      if( !room )
         room = obj->in_room;
      if( !room )
      {
         bug( "PullOrPush: obj points to invalid room %d", obj->value[1] );
         return;
      }
      if( IS_SET( obj->value[0], TRIG_D_NORTH ) )
      {
         edir = DIR_NORTH;
         txt = "to the north";
      }
      else if( IS_SET( obj->value[0], TRIG_D_SOUTH ) )
      {
         edir = DIR_SOUTH;
         txt = "to the south";
      }
      else if( IS_SET( obj->value[0], TRIG_D_EAST ) )
      {
         edir = DIR_EAST;
         txt = "to the east";
      }
      else if( IS_SET( obj->value[0], TRIG_D_WEST ) )
      {
         edir = DIR_WEST;
         txt = "to the west";
      }
      else if( IS_SET( obj->value[0], TRIG_D_UP ) )
      {
         edir = DIR_UP;
         txt = "from above";
      }
      else if( IS_SET( obj->value[0], TRIG_D_DOWN ) )
      {
         edir = DIR_DOWN;
         txt = "from below";
      }
      else
      {
         bug( "%s", "PullOrPush: door: no direction flag set." );
         return;
      }
      pexit = get_exit( room, edir );
      if( !pexit )
      {
         if( !IS_SET( obj->value[0], TRIG_PASSAGE ) )
         {
            bug( "PullOrPush: obj points to non-exit %d", obj->value[1] );
            return;
         }
         to_room = get_room_index( obj->value[2] );
         if( !to_room )
         {
            bug( "PullOrPush: dest points to invalid room %d", obj->value[2] );
            return;
         }
         pexit = make_exit( room, to_room, edir );
         pexit->key = -1;
         xCLEAR_BITS( pexit->exit_info );
         top_exit++;
         act( AT_PLAIN, "A passage opens!", ch, NULL, NULL, TO_CHAR );
         act( AT_PLAIN, "A passage opens!", ch, NULL, NULL, TO_ROOM );
         return;
      }
      if( IS_SET( obj->value[0], TRIG_UNLOCK ) && IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         REMOVE_EXIT_FLAG( pexit, EX_LOCKED );
         act( AT_PLAIN, "You hear a faint click $T.", ch, NULL, txt, TO_CHAR );
         act( AT_PLAIN, "You hear a faint click $T.", ch, NULL, txt, TO_ROOM );
         if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
            REMOVE_EXIT_FLAG( pexit_rev, EX_LOCKED );
         return;
      }
      if( IS_SET( obj->value[0], TRIG_LOCK ) && !IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         SET_EXIT_FLAG( pexit, EX_LOCKED );
         act( AT_PLAIN, "You hear a faint click $T.", ch, NULL, txt, TO_CHAR );
         act( AT_PLAIN, "You hear a faint click $T.", ch, NULL, txt, TO_ROOM );
         if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
            SET_EXIT_FLAG( pexit_rev, EX_LOCKED );
         return;
      }
      if( IS_SET( obj->value[0], TRIG_OPEN ) && IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         REMOVE_EXIT_FLAG( pexit, EX_CLOSED );
         for( rch = room->first_person; rch; rch = rch->next_in_room )
            act( AT_ACTION, "The $d opens.", rch, NULL, pexit->keyword, TO_CHAR );
         if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
         {
            REMOVE_EXIT_FLAG( pexit_rev, EX_CLOSED );
            /*
             * bug here pointed out by Nick Gammon 
             */
            for( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
               act( AT_ACTION, "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR );
         }
         check_room_for_traps( ch, trap_door[edir] );
         return;
      }
      if( IS_SET( obj->value[0], TRIG_CLOSE ) && !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         SET_EXIT_FLAG( pexit, EX_CLOSED );
         for( rch = room->first_person; rch; rch = rch->next_in_room )
            act( AT_ACTION, "The $d closes.", rch, NULL, pexit->keyword, TO_CHAR );
         if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
         {
            SET_EXIT_FLAG( pexit_rev, EX_CLOSED );
            /*
             * bug here pointed out by Nick Gammon 
             */
            for( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
               act( AT_ACTION, "The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR );
         }
         check_room_for_traps( ch, trap_door[edir] );
         return;
      }
   }
}

CMDF do_pull( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Pull what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = get_obj_here( ch, argument ) ) )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
      return;
   }
   pullorpush( ch, obj, TRUE );
}

CMDF do_push( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Push what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = get_obj_here( ch, argument ) ) )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
      return;
   }
   pullorpush( ch, obj, FALSE );
}

CMDF do_rap( CHAR_DATA * ch, char *argument )
{
   EXIT_DATA *pexit;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Rap on what?\n\r", ch );
      return;
   }
   if( ch->fighting )
   {
      send_to_char( "You have better things to do with your hands right now.\n\r", ch );
      return;
   }
   if( ( pexit = find_door( ch, argument, FALSE ) ) != NULL )
   {
      ROOM_INDEX_DATA *to_room;
      EXIT_DATA *pexit_rev;
      char *keyword;

      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         send_to_char( "Why knock?  It's open.\n\r", ch );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_SECRET ) )
         keyword = "wall";
      else
         keyword = pexit->keyword;
      act( AT_ACTION, "You rap loudly on the $d.", ch, NULL, keyword, TO_CHAR );
      act( AT_ACTION, "$n raps loudly on the $d.", ch, NULL, keyword, TO_ROOM );
      if( ( to_room = pexit->to_room ) != NULL && ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
      {
         CHAR_DATA *rch;

         for( rch = to_room->first_person; rch; rch = rch->next_in_room )
            act( AT_ACTION, "Someone raps loudly from the other side of the $d.", rch, NULL, pexit_rev->keyword, TO_CHAR );
      }
   }
   else
   {
      act( AT_ACTION, "You make knocking motions through the air.", ch, NULL, NULL, TO_CHAR );
      act( AT_ACTION, "$n makes knocking motions through the air.", ch, NULL, NULL, TO_ROOM );
   }
   return;
}

/* pipe commands (light, tamp, smoke) by Thoric */
CMDF do_tamp( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *cpipe;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Tamp what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( cpipe = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You aren't carrying that.\n\r", ch );
      return;
   }

   if( cpipe->item_type != ITEM_PIPE )
   {
      send_to_char( "You can't tamp that.\n\r", ch );
      return;
   }

   if( !IS_SET( cpipe->value[3], PIPE_TAMPED ) )
   {
      act( AT_ACTION, "You gently tamp $p.", ch, cpipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n gently tamps $p.", ch, cpipe, NULL, TO_ROOM );
      SET_BIT( cpipe->value[3], PIPE_TAMPED );
      return;
   }
   send_to_char( "It doesn't need tamping.\n\r", ch );
}

CMDF do_smoke( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *cpipe;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Smoke what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( cpipe = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You aren't carrying that.\n\r", ch );
      return;
   }
   if( cpipe->item_type != ITEM_PIPE )
   {
      act( AT_ACTION, "You try to smoke $p... but it doesn't seem to work.", ch, cpipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to smoke $p... (I wonder what $e's been putting in $s pipe?)", ch, cpipe, NULL, TO_ROOM );
      return;
   }
   if( !IS_SET( cpipe->value[3], PIPE_LIT ) )
   {
      act( AT_ACTION, "You try to smoke $p, but it's not lit.", ch, cpipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to smoke $p, but it's not lit.", ch, cpipe, NULL, TO_ROOM );
      return;
   }
   if( cpipe->value[1] > 0 )
   {
      if( !oprog_use_trigger( ch, cpipe, NULL, NULL, NULL ) )
      {
         act( AT_ACTION, "You draw thoughtfully from $p.", ch, cpipe, NULL, TO_CHAR );
         act( AT_ACTION, "$n draws thoughtfully from $p.", ch, cpipe, NULL, TO_ROOM );
      }

      if( IS_VALID_HERB( cpipe->value[2] ) && cpipe->value[2] < top_herb )
      {
         int sn = cpipe->value[2] + TYPE_HERB;
         SKILLTYPE *skill = get_skilltype( sn );

         WAIT_STATE( ch, skill->beats );
         if( skill->spell_fun )
            obj_cast_spell( sn, UMIN( skill->min_level, ch->level ), ch, ch, NULL );
         if( obj_extracted( cpipe ) )
            return;
      }
      else
         bug( "do_smoke: bad herb type %d", cpipe->value[2] );

      SET_BIT( cpipe->value[3], PIPE_HOT );
      if( --cpipe->value[1] < 1 )
      {
         REMOVE_BIT( cpipe->value[3], PIPE_LIT );
         SET_BIT( cpipe->value[3], PIPE_DIRTY );
         SET_BIT( cpipe->value[3], PIPE_FULLOFASH );
      }
   }
}

CMDF do_light( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *cpipe;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Light what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( cpipe = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You aren't carrying that.\n\r", ch );
      return;
   }
   if( cpipe->item_type != ITEM_PIPE )
   {
      send_to_char( "You can't light that.\n\r", ch );
      return;
   }
   if( !IS_SET( cpipe->value[3], PIPE_LIT ) )
   {
      if( cpipe->value[1] < 1 )
      {
         act( AT_ACTION, "You try to light $p, but it's empty.", ch, cpipe, NULL, TO_CHAR );
         act( AT_ACTION, "$n tries to light $p, but it's empty.", ch, cpipe, NULL, TO_ROOM );
         return;
      }
      act( AT_ACTION, "You carefully light $p.", ch, cpipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n carefully lights $p.", ch, cpipe, NULL, TO_ROOM );
      SET_BIT( cpipe->value[3], PIPE_LIT );
      return;
   }
   send_to_char( "It's already lit.\n\r", ch );
}

/*
 * Apply a salve/ointment					-Thoric
 * Support for applying to others.  Pkill concerns dealt with elsewhere.
 */
CMDF do_apply( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *salve, *obj;
   ch_ret retcode;

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Apply what?\n\r", ch );
      return;
   }
   if( ch->fighting )
   {
      send_to_char( "You're too busy fighting ...\n\r", ch );
      return;
   }
   if( ms_find_obj( ch ) )
      return;
   if( !( salve = get_obj_carry( ch, arg1 ) ) )
   {
      send_to_char( "You do not have that.\n\r", ch );
      return;
   }

   obj = NULL;
   if( !argument || argument[0] == '\0' )
      victim = ch;
   else
   {
      if( ( victim = get_char_room( ch, argument ) ) == NULL && ( obj = get_obj_here( ch, argument ) ) == NULL )
      {
         send_to_char( "Apply it to what or who?\n\r", ch );
         return;
      }
   }

   /*
    * apply salve to another object 
    */
   if( obj )
   {
      act( AT_ACTION, "You rub $p on $o, but nothing happens. (not implemented)", ch, salve, obj, TO_CHAR );
      // send_to_char( "You can't do that... yet.\n\r", ch );
      return;
   }

   if( victim->fighting )
   {
      send_to_char( "Wouldn't work very well while they're fighting ...\n\r", ch );
      return;
   }

   if( salve->item_type != ITEM_SALVE )
   {
      if( victim == ch )
      {
         act( AT_ACTION, "$n starts to rub $p on $mself...", ch, salve, NULL, TO_ROOM );
         act( AT_ACTION, "You try to rub $p on yourself...", ch, salve, NULL, TO_CHAR );
      }
      else
      {
         act( AT_ACTION, "$n starts to rub $p on $N...", ch, salve, victim, TO_NOTVICT );
         act( AT_ACTION, "$n starts to rub $p on you...", ch, salve, victim, TO_VICT );
         act( AT_ACTION, "You try to rub $p on $N...", ch, salve, victim, TO_CHAR );
      }
      return;
   }
   separate_obj( salve );
   --salve->value[1];

   if( !oprog_use_trigger( ch, salve, NULL, NULL, NULL ) )
   {
      if( !salve->action_desc || salve->action_desc[0] == '\0' )
      {
         if( salve->value[1] < 1 )
         {
            if( victim != ch )
            {
               act( AT_ACTION, "$n rubs the last of $p onto $N.", ch, salve, victim, TO_NOTVICT );
               act( AT_ACTION, "$n rubs the last of $p onto you.", ch, salve, victim, TO_VICT );
               act( AT_ACTION, "You rub the last of $p onto $N.", ch, salve, victim, TO_CHAR );
            }
            else
            {
               act( AT_ACTION, "You rub the last of $p onto yourself.", ch, salve, NULL, TO_CHAR );
               act( AT_ACTION, "$n rubs the last of $p onto $mself.", ch, salve, NULL, TO_ROOM );
            }
         }
         else
         {
            if( victim != ch )
            {
               act( AT_ACTION, "$n rubs $p onto $N.", ch, salve, victim, TO_NOTVICT );
               act( AT_ACTION, "$n rubs $p onto you.", ch, salve, victim, TO_VICT );
               act( AT_ACTION, "You rub $p onto $N.", ch, salve, victim, TO_CHAR );
            }
            else
            {
               act( AT_ACTION, "You rub $p onto yourself.", ch, salve, NULL, TO_CHAR );
               act( AT_ACTION, "$n rubs $p onto $mself.", ch, salve, NULL, TO_ROOM );
            }
         }
      }
      else
         actiondesc( ch, salve, NULL );
   }

   WAIT_STATE( ch, salve->value[3] );
   retcode = obj_cast_spell( salve->value[4], salve->value[0], ch, victim, NULL );
   if( retcode == rNONE )
      retcode = obj_cast_spell( salve->value[5], salve->value[0], ch, victim, NULL );
   if( retcode == rCHAR_DIED )
   {
      bug( "%s", "do_apply: char died" );
      return;
   }

   if( !obj_extracted( salve ) && salve->value[1] <= 0 )
      extract_obj( salve );
   return;
}

/* The Northwind's Rune system */
CMDF do_invoke( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj;
   ROOM_INDEX_DATA *location;
   CHAR_DATA *opponent;

   location = NULL;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Invoke what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You do not have that item.\n\r", ch );
      return;
   }

   if( obj->value[2] < 1 )
   {
      send_to_char( "&RIt is out of charges!\n\r", ch );
      return;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_NO_RECALL ) || IS_AREA_FLAG( ch->in_room->area, AFLAG_NORECALL ) )
   {
      send_to_char( "&RAncient magiks prevent the rune from functioning here.\n\r", ch );
      return;
   }

   if( obj->value[1] > 0 )
   {
      location = get_room_index( obj->value[1] );

      if( ( opponent = who_fighting( ch ) ) != NULL )
      {
         send_to_char( "&RYou cannot recall during a fight!\n\r", ch );
         return;
      }

      if( !location )
      {
         send_to_char( "There is a problem with your rune. Contact the immortals.\n\r", ch );
         bug( "%s", "do_invoke: Rune with NULL room!" );
         return;
      }

      act( AT_MAGIC, "$n disappears in a swirl of smoke.", ch, NULL, NULL, TO_ROOM );
      send_to_char( "&[magic]You invoke the rune and are instantly transported away!\n\r", ch );

      leave_map( ch, NULL, location );

      obj->value[2] -= 1;
      if( obj->value[2] == 0 )
      {
         STRFREE( obj->short_descr );
         obj->short_descr = STRALLOC( "A depleted recall rune" );
         send_to_char( "The rune hums softly and is now depleted of power.\n\r", ch );
      }
      act( AT_ACTION, "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
      return;
   }
   else
   {
      send_to_char( "&RHow are you going to recall with THAT?!?\n\r", ch );
      return;
   }
}

CMDF do_mark( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj;
   char arg[MIL];

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      send_to_char( "Mark what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = get_obj_carry( ch, arg ) ) )
   {
      send_to_char( "You do not have that item.\n\r", ch );
      return;
   }

   if( obj->item_type != ITEM_RUNE )
   {
      send_to_char( "&RHow can you mark something that's not a rune?\n\r", ch );
      return;
   }
   else
   {
      if( obj->value[1] > 0 )
      {
         send_to_char( "&RThat rune is already marked!\n\r", ch );
         return;
      }

      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "Ok, but how do you want to identify this rune?\n\r", ch );
         return;
      }

      if( IS_ROOM_FLAG( ch->in_room, ROOM_NO_RECALL ) || IS_AREA_FLAG( ch->in_room->area, AFLAG_NORECALL ) )
      {
         send_to_char( "&RAncient magiks prevent the rune from taking the mark here.\n\r", ch );
         return;
      }

      if( IS_PLR_FLAG( ch, PLR_ONMAP ) || IS_ACT_FLAG( ch, ACT_ONMAP ) )
      {
         send_to_char( "You cannot mark runes on the overland.\n\r", ch );
         return;
      }

      obj->value[1] = ch->in_room->vnum;
      obj->value[2] = 5;
      stralloc_printf( &obj->name, "%s rune", argument );
      stralloc_printf( &obj->short_descr, "A recall rune to %s", capitalize( argument ) );
      STRFREE( obj->objdesc );
      obj->objdesc = STRALLOC( "A small magical rune with a mark inscribed on it lies here." );
      send_to_char( "&BYou mark the rune with your location.\n\r", ch );
      return;
   }
}

/* Connect pieces of an ITEM -- Originally from ACK! Modified for Smaug by Zarius 5/19/2000 */
CMDF do_connect( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *first_ob, *second_ob, *new_ob;
   char arg1[MSL];

   argument = one_argument( argument, arg1 );

   if( !arg1 || arg1[0] == '\0' || !argument || argument[0] == '\0' )
   {
      send_to_char( "Syntax: Connect <firstobj> <secondobj>.\n\r", ch );
      return;
   }

   if( !( first_ob = get_obj_carry( ch, arg1 ) ) )
   {
      send_to_char( "You must be holding both parts!!\n\r", ch );
      return;
   }

   if( !( second_ob = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You must be holding both parts!!\n\r", ch );
      return;
   }

   if( first_ob->item_type != ITEM_PIECE || second_ob->item_type != ITEM_PIECE )
   {
      send_to_char( "Both items must be pieces of another item!\n\r", ch );
      return;
   }

   separate_obj( first_ob );
   separate_obj( second_ob );

   /*
    * check to see if the pieces connect 
    */
   if( first_ob->value[0] == second_ob->pIndexData->vnum && second_ob->value[0] == first_ob->pIndexData->vnum )
   {
      /*
       * Good connection. Generate the final product.
       */
      if( !( new_ob = create_object( get_obj_index( first_ob->value[2] ), ch->level ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      extract_obj( first_ob );
      extract_obj( second_ob );
      obj_to_char( new_ob, ch );
      act( AT_ACTION, "$n jiggles some pieces together...\n\r...suddenly they snap in place, creating $p!", ch, new_ob, NULL,
           TO_ROOM );
      act( AT_ACTION, "You jiggle the pieces together...\n\r...suddenly they snap into place, creating $p!", ch, new_ob,
           NULL, TO_CHAR );
   }
   else
   {
      /*
       * bad connection 
       */
      act( AT_ACTION, "$n jiggles some pieces together, but can't seem to make them connect.", ch, NULL, NULL, TO_ROOM );
      act( AT_ACTION, "You try to fit them together every which way, but they just don't want to fit together.",
           ch, NULL, NULL, TO_CHAR );
      return;
   }
   return;
}

/* Junk command installed by Samson 1-13-98 */
/* Code courtesy of Stu, from the mailing list. Allows player to destroy an item in their inventory */
CMDF do_junk( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj, *obj_next;
   bool found = FALSE;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Junk what?\n\r", ch );
      return;
   }

   for( obj = ch->first_carrying; obj; obj = obj_next )
   {
      obj_next = obj->next_content;
      if( ( nifty_is_name( argument, obj->name ) ) && can_see_obj( ch, obj, FALSE ) && obj->wear_loc == WEAR_NONE )
      {
         found = TRUE;
         break;
      }
   }
   if( found )
   {
      if( !can_drop_obj( ch, obj ) && ch->level < LEVEL_IMMORTAL )
      {
         send_to_char( "You cannot junk that, it's cursed!\n\r", ch );
         return;
      }
      separate_obj( obj );
      obj_from_char( obj );
      extract_obj( obj );
      act( AT_ACTION, "$n junks $p.", ch, obj, NULL, TO_ROOM );
      act( AT_ACTION, "You junk $p.", ch, obj, NULL, TO_CHAR );
   }
   return;
}

/* Donate command installed by Samson 2-6-98 
   Coded by unknown author. Players can donate items for others to use. */
/* Slight bug corrected, objects weren't being seperated from each other - Whir 3-25-98 */
CMDF do_donate( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Donate what?\n\r", ch );
      return;
   }

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      send_to_char( "You cannot donate while fighting!\n\r", ch );
      return;
   }

   if( ( obj = get_obj_carry( ch, argument ) ) == NULL )
   {
      send_to_char( "You do not have that!\n\r", ch );
      return;
   }
   else
   {
      if( !can_drop_obj( ch, obj ) && ch->level < LEVEL_IMMORTAL )
      {
         send_to_char( "You cannot donate that, it's cursed!\n\r", ch );
         return;
      }

      if( ( obj->item_type == ITEM_CORPSE_NPC ) || ( obj->item_type == ITEM_CORPSE_PC ) )
      {
         send_to_char( "You cannot donate corpses!\n\r", ch );
         return;
      }

      if( obj->timer > 0 )
      {
         send_to_char( "You cannot donate that.\n\r", ch );
         return;
      }

      if( IS_OBJ_FLAG( obj, ITEM_PERSONAL ) )
      {
         send_to_char( "You cannot donate personal items.\n\r", ch );
         return;
      }

      separate_obj( obj );
      SET_OBJ_FLAG( obj, ITEM_DONATION );
      obj_from_char( obj );
      act( AT_ACTION, "You donate $p, how generous of you!", ch, obj, NULL, TO_CHAR );
      obj_to_room( obj, get_room_index( ROOM_VNUM_DONATION ), NULL );
      save_char_obj( ch );
      return;
   }
}
