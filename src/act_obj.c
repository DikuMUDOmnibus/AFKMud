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
 *                       Object manipulation module                         *
 ****************************************************************************/

#include "mud.h"
#include "clans.h"
#include "deity.h"

/*double sqrt( double x );*/

void write_corpses( CHAR_DATA * ch, char *name, OBJ_DATA * objrem );
void check_clan_storeroom( CHAR_DATA * ch );
void check_clan_shop( CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj );
void adjust_favor( CHAR_DATA * ch, int field, int mod );
OBJ_DATA *create_money( int amount );
void mprog_bribe_trigger( CHAR_DATA * mob, CHAR_DATA * ch, int amount );
void mprog_give_trigger( CHAR_DATA * mob, CHAR_DATA * ch, OBJ_DATA * obj );

/*
 * Turn an object into scraps.		-Thoric
 */
void make_scraps( OBJ_DATA * obj )
{
   OBJ_DATA *scraps, *tmpobj;
   CHAR_DATA *ch = NULL;

   separate_obj( obj );
   if( !( scraps = create_object( get_obj_index( OBJ_VNUM_SCRAPS ), 0 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }
   scraps->timer = number_range( 5, 15 );
   if( IS_OBJ_FLAG( obj, ITEM_ONMAP ) )
   {
      SET_OBJ_FLAG( scraps, ITEM_ONMAP );
      scraps->map = obj->map;
      scraps->x = obj->x;
      scraps->y = obj->y;
   }

   /*
    * don't make scraps of scraps of scraps of ... 
    */
   if( obj->pIndexData->vnum == OBJ_VNUM_SCRAPS )
   {
      stralloc_printf( &scraps->short_descr, "%s", "some debris" );
      stralloc_printf( &scraps->objdesc, "%s", "Bits of debris lie on the ground here." );
   }
   else
   {
      stralloc_printf( &scraps->short_descr, scraps->short_descr, obj->short_descr );
      stralloc_printf( &scraps->objdesc, scraps->objdesc, obj->short_descr );
   }

   if( obj->carried_by )
   {
      act( AT_OBJECT, "$p falls to the ground in scraps!", obj->carried_by, obj, NULL, TO_CHAR );
      if( obj == get_eq_char( obj->carried_by, WEAR_WIELD )
          && ( tmpobj = get_eq_char( obj->carried_by, WEAR_DUAL_WIELD ) ) != NULL )
         tmpobj->wear_loc = WEAR_WIELD;

      obj_to_room( scraps, obj->carried_by->in_room, obj->carried_by );
   }
   else if( obj->in_room )
   {
      if( ( ch = obj->in_room->first_person ) != NULL )
      {
         act( AT_OBJECT, "$p is reduced to little more than scraps.", ch, obj, NULL, TO_ROOM );
         act( AT_OBJECT, "$p is reduced to little more than scraps.", ch, obj, NULL, TO_CHAR );
      }
      obj_to_room( scraps, obj->in_room, NULL );
   }
   if( ( obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_KEYRING
         || obj->item_type == ITEM_QUIVER || obj->item_type == ITEM_CORPSE_PC ) && obj->first_content )
   {
      if( ch && ch->in_room )
      {
         act( AT_OBJECT, "The contents of $p fall to the ground.", ch, obj, NULL, TO_ROOM );
         act( AT_OBJECT, "The contents of $p fall to the ground.", ch, obj, NULL, TO_CHAR );
      }
      if( obj->carried_by )
         empty_obj( obj, NULL, obj->carried_by->in_room );
      else if( obj->in_room )
         empty_obj( obj, NULL, obj->in_room );
      else if( obj->in_obj )
         empty_obj( obj, obj->in_obj, NULL );
   }
   extract_obj( obj );
}

/*
 * how resistant an object is to damage				-Thoric
 */
short get_obj_resistance( OBJ_DATA * obj )
{
   short resist;

   resist = number_fuzzy( sysdata.maximpact );

   /*
    * magical items are more resistant 
    */
   if( IS_OBJ_FLAG( obj, ITEM_MAGIC ) )
      resist += number_fuzzy( 12 );
   /*
    * metal objects are definately stronger 
    */
   if( IS_OBJ_FLAG( obj, ITEM_METAL ) )
      resist += number_fuzzy( 5 );
   /*
    * organic objects are most likely weaker 
    */
   if( IS_OBJ_FLAG( obj, ITEM_ORGANIC ) )
      resist -= number_fuzzy( 5 );
   /*
    * blessed objects should have a little bonus 
    */
   if( IS_OBJ_FLAG( obj, ITEM_BLESS ) )
      resist += number_fuzzy( 5 );
   /*
    * lets make store inventory pretty tough 
    */
   if( IS_OBJ_FLAG( obj, ITEM_INVENTORY ) )
      resist += 20;

   /*
    * okay... let's add some bonus/penalty for item level... 
    */
   resist += ( obj->level / 10 ) - 2;

   /*
    * and lasty... take armor or weapon's condition into consideration 
    */
   if( obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON )
      resist += ( obj->value[0] / 2 ) - 2;

   return URANGE( 10, resist, 99 );
}

void get_obj( CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * container )
{
   int weight, amt;  /* gold per-race multipliers */

   if( !CAN_WEAR( obj, ITEM_TAKE ) && ( ch->level < sysdata.level_getobjnotake ) )
   {
      send_to_char( "You can't take that.\n\r", ch );
      return;
   }

   if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) && !can_take_proto( ch ) )
   {
      send_to_char( "A godly force prevents you from getting close to it.\n\r", ch );
      return;
   }

   if( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
   {
      act( AT_PLAIN, "$d: you can't carry that many items.", ch, NULL, obj->name, TO_CHAR );
      return;
   }

   if( IS_OBJ_FLAG( obj, ITEM_COVERING ) )
      weight = obj->weight;
   else
      weight = get_obj_weight( obj );

   if( ch->carry_weight + weight > can_carry_w( ch ) )
   {
      act( AT_PLAIN, "$d: you can't carry that much weight.", ch, NULL, obj->name, TO_CHAR );
      return;
   }

   if( container )
   {
      if( container->item_type == ITEM_KEYRING && !IS_OBJ_FLAG( container, ITEM_COVERING ) )
      {
         act( AT_ACTION, "You remove $p from $P", ch, obj, container, TO_CHAR );
         act( AT_ACTION, "$n removes $p from $P", ch, obj, container, TO_ROOM );
      }
      else
      {
         act( AT_ACTION, IS_OBJ_FLAG( container, ITEM_COVERING ) ?
              "You get $p from beneath $P." : "You get $p from $P", ch, obj, container, TO_CHAR );
         act( AT_ACTION, IS_OBJ_FLAG( container, ITEM_COVERING ) ?
              "$n gets $p from beneath $P." : "$n gets $p from $P", ch, obj, container, TO_ROOM );
      }
      obj_from_obj( obj );
   }
   else
   {
      act( AT_ACTION, "You get $p.", ch, obj, container, TO_CHAR );
      act( AT_ACTION, "$n gets $p.", ch, obj, container, TO_ROOM );
      obj_from_room( obj );
   }

   /*
    * Clan storeroom checks 
    */
   if( IS_ROOM_FLAG( ch->in_room, ROOM_CLANSTOREROOM ) && ( !container || container->carried_by == NULL ) )
      check_clan_storeroom( ch );

   if( obj->item_type != ITEM_CONTAINER )
      check_for_trap( ch, obj, TRAP_GET );

   if( char_died( ch ) )
      return;

   if( obj->item_type == ITEM_MONEY )
   {
      amt = obj->value[0];
      ch->gold += amt;
      extract_obj( obj );
      return;
   }
   else
      obj_to_char( obj, ch );

   if( char_died( ch ) || obj_extracted( obj ) )
      return;
   oprog_get_trigger( ch, obj );
   return;
}

CMDF do_get( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   OBJ_DATA *obj, *obj_next, *container;
   short number;
   bool found;

   argument = one_argument( argument, arg1 );
   if( is_number( arg1 ) )
   {
      number = atoi( arg1 );
      if( number < 1 )
      {
         send_to_char( "That was easy...\n\r", ch );
         return;
      }
      if( ( ch->carry_number + number ) > can_carry_n( ch ) )
      {
         send_to_char( "You can't carry that many.\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg1 );
   }
   else
      number = 0;

   argument = one_argument( argument, arg2 );
   /*
    * munch optional words 
    */
   if( !str_cmp( arg2, "from" ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   /*
    * Get type. 
    */
   if( arg1[0] == '\0' )
   {
      send_to_char( "Get what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( arg2[0] == '\0' )
   {
      if( number <= 1 && str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
      {
         /*
          * 'get obj' 
          */
         obj = get_obj_list( ch, arg1, ch->in_room->first_content );
         if( !obj )
         {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, arg1, TO_CHAR );
            return;
         }

         separate_obj( obj );
         get_obj( ch, obj, NULL );

         if( char_died( ch ) )
            return;
         if( IS_SAVE_FLAG( SV_GET ) )
            save_char_obj( ch );
      }
      else
      {
         short cnt = 0;
         bool fAll;
         char *chk;

         if( IS_ROOM_FLAG( ch->in_room, ROOM_DONATION ) )
         {
            send_to_char( "The gods frown upon such a display of greed!\n\r", ch );
            return;
         }
         if( !str_cmp( arg1, "all" ) )
            fAll = TRUE;
         else
            fAll = FALSE;
         if( number > 1 )
            chk = arg1;
         else
            chk = &arg1[4];
         /*
          * 'get all' or 'get all.obj' 
          */
         found = FALSE;
         for( obj = ch->in_room->last_content; obj; obj = obj_next )
         {
            obj_next = obj->prev_content;
            if( ( fAll || nifty_is_name( chk, obj->name ) ) && can_see_obj( ch, obj, FALSE ) )
            {
               if( IS_OBJ_FLAG( obj, ITEM_ONMAP ) )
               {
                  if( ch->map != obj->map || ch->x != obj->x || ch->y != obj->y )
                  {
                     found = FALSE;
                     continue;
                  }
               }

               found = TRUE;
               if( number && ( cnt + obj->count ) > number )
                  split_obj( obj, number - cnt );
               cnt += obj->count;
               get_obj( ch, obj, NULL );

               if( char_died( ch ) || ch->carry_number >= can_carry_n( ch )
                   || ch->carry_weight >= can_carry_w( ch ) || ( number && cnt >= number ) )
               {
                  if( IS_SAVE_FLAG( SV_GET ) && !char_died( ch ) )
                     save_char_obj( ch );
                  return;
               }
            }
         }

         if( !found )
         {
            if( fAll )
               send_to_char( "I see nothing here.\n\r", ch );
            else
               act( AT_PLAIN, "I see no $T here.", ch, NULL, chk, TO_CHAR );
         }
         else if( IS_SAVE_FLAG( SV_GET ) )
            save_char_obj( ch );
      }
   }
   else
   {
      /*
       * 'get ... container' 
       */
      if( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }

      if( ( container = get_obj_here( ch, arg2 ) ) == NULL )
      {
         act( AT_PLAIN, "I see no $T here.", ch, NULL, arg2, TO_CHAR );
         return;
      }

      switch ( container->item_type )
      {
         default:
            if( !IS_OBJ_FLAG( container, ITEM_COVERING ) )
            {
               send_to_char( "That's not a container.\n\r", ch );
               return;
            }
            if( ch->carry_weight + container->weight > can_carry_w( ch ) )
            {
               send_to_char( "It's too heavy for you to lift.\n\r", ch );
               return;
            }
            break;

         case ITEM_CONTAINER:
         case ITEM_CORPSE_NPC:
         case ITEM_KEYRING:
         case ITEM_QUIVER:
            break;

         case ITEM_CORPSE_PC:
         {
            char name[MIL];
            CHAR_DATA *gch;
            char *pd;

            if( IS_NPC( ch ) )
            {
               send_to_char( "You can't do that.\n\r", ch );
               return;
            }

            pd = container->short_descr;
            pd = one_argument( pd, name );
            pd = one_argument( pd, name );
            pd = one_argument( pd, name );
            pd = one_argument( pd, name );

            if( IS_OBJ_FLAG( container, ITEM_CLANCORPSE )
                && !IS_NPC( ch ) && ( get_timer( ch, TIMER_PKILLED ) > 0 ) && str_cmp( name, ch->name ) )
            {
               send_to_char( "You cannot loot from that corpse...yet.\n\r", ch );
               return;
            }

            /*
             * Killer/owner loot only if die to pkill blow --Blod 
             */
            /*
             * Added check for immortal so IMMS can get things out of
             * * corpses --Shaddai 
             */

            if( IS_OBJ_FLAG( container, ITEM_CLANCORPSE ) && !IS_NPC( ch ) && !IS_IMMORTAL( ch )
                && container->action_desc[0] != '\0' && str_cmp( name, ch->name )
                && str_cmp( container->action_desc, ch->name ) )
            {
               send_to_char( "You did not inflict the death blow upon this corpse.\n\r", ch );
               return;
            }

            if( IS_OBJ_FLAG( container, ITEM_CLANCORPSE ) && IS_PCFLAG( ch, PCFLAG_DEADLY )
                && container->value[4] - ch->level < 6 && container->value[4] - ch->level > -6 )
               break;

            if( str_cmp( name, ch->name ) && !IS_IMMORTAL( ch ) )
            {
               bool fGroup;

               fGroup = FALSE;
               for( gch = first_char; gch; gch = gch->next )
               {
                  if( !IS_NPC( gch ) && is_same_group( ch, gch ) && !str_cmp( name, gch->name ) )
                  {
                     fGroup = TRUE;
                     break;
                  }
               }

               if( !fGroup )
               {
                  send_to_char( "That's someone else's corpse.\n\r", ch );
                  return;
               }
            }
         }
      }

      if( !IS_OBJ_FLAG( container, ITEM_COVERING ) && IS_SET( container->value[1], CONT_CLOSED ) )
      {
         act( AT_PLAIN, "The $d is closed.", ch, NULL, container->name, TO_CHAR );
         return;
      }

      if( number <= 1 && str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
      {
         /*
          * 'get obj container' 
          */
         obj = get_obj_list( ch, arg1, container->first_content );
         if( !obj )
         {
            act( AT_PLAIN, IS_OBJ_FLAG( container, ITEM_COVERING ) ?
                 "I see nothing like that beneath the $T." : "I see nothing like that in the $T.", ch, NULL, arg2, TO_CHAR );
            return;
         }
         separate_obj( obj );
         get_obj( ch, obj, container );
         /*
          * Oops no wonder corpses were duping oopsie did I do that
          * * --Shaddai
          */
         if( container->item_type == ITEM_CORPSE_PC )
            write_corpses( NULL, container->short_descr + 14, NULL );
         check_for_trap( ch, container, TRAP_GET );
         if( char_died( ch ) )
            return;
         if( IS_SAVE_FLAG( SV_GET ) )
            save_char_obj( ch );
      }
      else
      {
         int cnt = 0;
         bool fAll;
         char *chk;

         /*
          * 'get all container' or 'get all.obj container' 
          */
         if( IS_OBJ_FLAG( container, ITEM_CLANCORPSE )
             && !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && str_cmp( ch->name, container->name + 7 ) )
         {
            send_to_char( "The gods frown upon such wanton greed!\n\r", ch );
            return;
         }

         if( !str_cmp( arg1, "all" ) )
            fAll = TRUE;
         else
            fAll = FALSE;
         if( number > 1 )
            chk = arg1;
         else
            chk = &arg1[4];
         found = FALSE;
         for( obj = container->first_content; obj; obj = obj_next )
         {
            obj_next = obj->next_content;
            if( ( fAll || nifty_is_name( chk, obj->name ) ) && can_see_obj( ch, obj, FALSE ) )
            {
               found = TRUE;
               if( number && ( cnt + obj->count ) > number )
                  split_obj( obj, number - cnt );
               cnt += obj->count;
               get_obj( ch, obj, container );
               if( char_died( ch )
                   || ch->carry_number >= can_carry_n( ch )
                   || ch->carry_weight >= can_carry_w( ch ) || ( number && cnt >= number ) )
               {
                  if( container->item_type == ITEM_CORPSE_PC )
                     write_corpses( NULL, container->short_descr + 14, NULL );
                  if( found && IS_SET( sysdata.save_flags, SV_GET ) )
                     save_char_obj( ch );
                  return;
               }
            }
         }

         if( !found )
         {
            if( fAll )
            {
               if( container->item_type == ITEM_KEYRING && !IS_OBJ_FLAG( container, ITEM_COVERING ) )
                  act( AT_PLAIN, "The $T holds no keys.", ch, NULL, arg2, TO_CHAR );
               else
                  act( AT_PLAIN, IS_OBJ_FLAG( container, ITEM_COVERING ) ?
                       "I see nothing beneath the $T." : "I see nothing in the $T.", ch, NULL, arg2, TO_CHAR );
            }
            else
            {
               if( container->item_type == ITEM_KEYRING && !IS_OBJ_FLAG( container, ITEM_COVERING ) )
                  act( AT_PLAIN, "The $T does not hold that key.", ch, NULL, arg2, TO_CHAR );
               else
                  act( AT_PLAIN, IS_OBJ_FLAG( container, ITEM_COVERING ) ?
                       "I see nothing like that beneath the $T." : "I see nothing like that in the $T.", ch, NULL, arg2,
                       TO_CHAR );
            }
         }
         else
            check_for_trap( ch, container, TRAP_GET );
         if( char_died( ch ) )
            return;
         /*
          * Oops no wonder corpses were duping oopsie did I do that
          * * --Shaddai
          */
         if( container->item_type == ITEM_CORPSE_PC )
         {
            write_corpses( NULL, container->short_descr + 14, NULL );
         }
         if( found && IS_SAVE_FLAG( SV_GET ) )
            save_char_obj( ch );
      }
   }
   return;
}

CMDF do_put( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   OBJ_DATA *container, *obj, *obj_next;
   short count;
   int number;
   bool save_char = FALSE;

   argument = one_argument( argument, arg1 );
   if( is_number( arg1 ) )
   {
      number = atoi( arg1 );
      if( number < 1 )
      {
         send_to_char( "That was easy...\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg1 );
   }
   else
      number = 0;
   argument = one_argument( argument, arg2 );
   /*
    * munch optional words 
    */
   if( ( !str_cmp( arg2, "into" ) || !str_cmp( arg2, "inside" )
         || !str_cmp( arg2, "in" ) || !str_cmp( arg2, "under" )
         || !str_cmp( arg2, "onto" ) || !str_cmp( arg2, "on" ) ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Put what in what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
   {
      send_to_char( "You can't do that.\n\r", ch );
      return;
   }

   if( ( container = get_obj_here( ch, arg2 ) ) == NULL )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, arg2, TO_CHAR );
      return;
   }

   if( !container->carried_by && IS_SAVE_FLAG( SV_PUT ) )
      save_char = TRUE;

   if( IS_OBJ_FLAG( container, ITEM_COVERING ) )
   {
      if( ch->carry_weight + container->weight > can_carry_w( ch ) )
      {
         send_to_char( "It's too heavy for you to lift.\n\r", ch );
         return;
      }
   }
   else
   {
      if( container->item_type != ITEM_CONTAINER
          && container->item_type != ITEM_KEYRING && container->item_type != ITEM_QUIVER )
      {
         send_to_char( "That's not a container.\n\r", ch );
         return;
      }

      if( IS_SET( container->value[1], CONT_CLOSED ) )
      {
         act( AT_PLAIN, "The $d is closed.", ch, NULL, container->name, TO_CHAR );
         return;
      }
   }

   if( number <= 1 && str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
   {
      /*
       * 'put obj container' 
       */
      if( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
      {
         send_to_char( "You do not have that item.\n\r", ch );
         return;
      }

      if( obj == container )
      {
         send_to_char( "You can't fold it into itself.\n\r", ch );
         return;
      }

      if( !can_drop_obj( ch, obj ) )
      {
         send_to_char( "You can't let go of it.\n\r", ch );
         return;
      }

      if( container->item_type == ITEM_KEYRING && obj->item_type != ITEM_KEY )
      {
         send_to_char( "That's not a key.\n\r", ch );
         return;
      }

      if( container->item_type == ITEM_QUIVER && obj->item_type != ITEM_PROJECTILE )
      {
         send_to_char( "That's not a projectile.\n\r", ch );
         return;
      }

      if( ( IS_OBJ_FLAG( container, ITEM_COVERING )
            && ( get_obj_weight( obj ) / obj->count )
            > ( ( get_obj_weight( container ) / container->count ) - container->weight ) ) )
      {
         send_to_char( "It won't fit under there.\n\r", ch );
         return;
      }

      /*
       * note use of get_real_obj_weight 
       */
      if( ( get_real_obj_weight( obj ) / obj->count )
          + ( get_real_obj_weight( container ) / container->count ) > container->value[0] )
      {
         send_to_char( "It won't fit.\n\r", ch );
         return;
      }

      separate_obj( obj );
      separate_obj( container );
      obj_from_char( obj );
      obj = obj_to_obj( obj, container );
      check_for_trap( ch, container, TRAP_PUT );
      if( char_died( ch ) )
         return;
      count = obj->count;
      obj->count = 1;
      if( container->item_type == ITEM_KEYRING && !IS_OBJ_FLAG( container, ITEM_COVERING ) )
      {
         act( AT_ACTION, "$n slips $p onto $P.", ch, obj, container, TO_ROOM );
         act( AT_ACTION, "You slip $p onto $P.", ch, obj, container, TO_CHAR );
      }
      else
      {
         act( AT_ACTION, IS_OBJ_FLAG( container, ITEM_COVERING )
              ? "$n hides $p beneath $P." : "$n puts $p in $P.", ch, obj, container, TO_ROOM );
         act( AT_ACTION, IS_OBJ_FLAG( container, ITEM_COVERING )
              ? "You hide $p beneath $P." : "You put $p in $P.", ch, obj, container, TO_CHAR );
      }
      obj->count = count;

      /*
       * Oops no wonder corpses were duping oopsie did I do that
       * * --Shaddai
       */
      if( container->item_type == ITEM_CORPSE_PC )
         write_corpses( NULL, container->short_descr + 14, NULL );
      if( save_char )
         save_char_obj( ch );
      /*
       * Clan storeroom check 
       */
      if( IS_ROOM_FLAG( ch->in_room, ROOM_CLANSTOREROOM ) && container->carried_by == NULL )
      {
         if( obj == NULL )
         {
            bug( "%s", "do_put: clanstoreroom check: Object is NULL!" );
            return;
         }
         if( obj->rent < sysdata.minrent )
            check_clan_storeroom( ch );
         else
         {
            ch_printf( ch, "%s is a rent item and therefore cannot be stored here.\n\r", obj->short_descr );
            obj_from_obj( obj );
            obj_to_char( obj, ch );
         }
      }
   }
   else
   {
      bool found = FALSE;
      int cnt = 0;
      bool fAll;
      char *chk;

      if( !str_cmp( arg1, "all" ) )
         fAll = TRUE;
      else
         fAll = FALSE;
      if( number > 1 )
         chk = arg1;
      else
         chk = &arg1[4];

      separate_obj( container );
      /*
       * 'put all container' or 'put all.obj container' 
       */
      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;

         if( ( fAll || nifty_is_name( chk, obj->name ) )
             && can_see_obj( ch, obj, FALSE )
             && obj->wear_loc == WEAR_NONE
             && obj != container
             && can_drop_obj( ch, obj )
             && ( container->item_type != ITEM_KEYRING || obj->item_type == ITEM_KEY )
             && ( container->item_type != ITEM_QUIVER || obj->item_type == ITEM_PROJECTILE )
             && get_obj_weight( obj ) + get_obj_weight( container ) <= container->value[0] )
         {
            if( number && ( cnt + obj->count ) > number )
               split_obj( obj, number - cnt );
            cnt += obj->count;
            obj_from_char( obj );
            if( container->item_type == ITEM_KEYRING )
            {
               act( AT_ACTION, "$n slips $p onto $P.", ch, obj, container, TO_ROOM );
               act( AT_ACTION, "You slip $p onto $P.", ch, obj, container, TO_CHAR );
            }
            else
            {
               act( AT_ACTION, "$n puts $p in $P.", ch, obj, container, TO_ROOM );
               act( AT_ACTION, "You put $p in $P.", ch, obj, container, TO_CHAR );
            }
            obj = obj_to_obj( obj, container );
            found = TRUE;

            check_for_trap( ch, container, TRAP_PUT );
            if( char_died( ch ) )
               return;
            if( number && cnt >= number )
               break;
         }
      }

      /*
       * Don't bother to save anything if nothing was dropped   -Thoric
       */
      if( !found )
      {
         if( fAll )
            act( AT_PLAIN, "You are not carrying anything.", ch, NULL, NULL, TO_CHAR );
         else
            act( AT_PLAIN, "You are not carrying any $T.", ch, NULL, chk, TO_CHAR );
         return;
      }

      if( save_char )
         save_char_obj( ch );
      /*
       * Oops no wonder corpses were duping oopsie did I do that
       * * --Shaddai
       */
      if( container->item_type == ITEM_CORPSE_PC )
      {
         write_corpses( NULL, container->short_descr + 14, NULL );
      }

      /*
       * Clan storeroom check 
       */
      if( IS_ROOM_FLAG( ch->in_room, ROOM_CLANSTOREROOM ) && container->carried_by == NULL )
      {
         if( obj == NULL )
         {
            bug( "%s", "do_put: clanstoreroom check: Object is NULL!" );
            return;
         }
         if( obj->rent < sysdata.minrent )
            check_clan_storeroom( ch );
         else
         {
            ch_printf( ch, "%s is a rent item and therefore cannot be stored here.\n\r", obj->short_descr );
            obj_from_obj( obj );
            obj_to_char( obj, ch );
         }
      }
   }

   return;
}

/* Function modified from original form - Whir 1-29-98 */
CMDF do_drop( CHAR_DATA * ch, char *argument )
{
   char arg[MIL];
   OBJ_DATA *obj;
   OBJ_DATA *obj_next;
   bool found;
   int number;

   argument = one_argument( argument, arg );
   if( is_number( arg ) )
   {
      number = atoi( arg );
      if( number < 1 )
      {
         send_to_char( "That was easy...\n\r", ch );
         return;
      }
      argument = one_argument( argument, arg );
   }
   else
      number = 0;

   if( arg[0] == '\0' )
   {
      send_to_char( "Drop what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !IS_NPC( ch ) && IS_PLR_FLAG( ch, PLR_LITTERBUG ) )
   {
      send_to_char( "&[immortal]A godly force prevents you from dropping anything...\n\r", ch );
      return;
   }

   if( IS_ROOM_FLAG( ch->in_room, ROOM_NODROP ) && ch != supermob )
   {
      send_to_char( "&[magic]A magical force stops you!\n\r", ch );
      send_to_char( "&[tell]Someone tells you, 'No littering here!'\n\r", ch );
      return;
   }

   if( number > 0 )
   {
      /*
       * 'drop NNNN coins' 
       */

      if( !str_cmp( arg, "coins" ) || !str_cmp( arg, "coin" ) )
      {
         if( ch->gold < number )
         {
            send_to_char( "You haven't got that many coins.\n\r", ch );
            return;
         }

         ch->gold -= number;

         for( obj = ch->in_room->first_content; obj; obj = obj_next )
         {
            obj_next = obj->next_content;

            switch ( obj->pIndexData->vnum )
            {
               case OBJ_VNUM_MONEY_ONE:
                  number += 1;
                  extract_obj( obj );
                  break;

               case OBJ_VNUM_MONEY_SOME:
                  number += obj->value[0];
                  extract_obj( obj );
                  break;
            }
         }

         act( AT_ACTION, "$n drops some gold.", ch, NULL, NULL, TO_ROOM );
         obj_to_room( create_money( number ), ch->in_room, ch );
         send_to_char( "You let the gold slip from your hand.\n\r", ch );
         if( IS_SAVE_FLAG( SV_DROP ) )
            save_char_obj( ch );
         return;
      }
   }

   if( number <= 1 && str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
   {
      /*
       * 'drop obj' 
       */
      if( ( obj = get_obj_carry( ch, arg ) ) == NULL )
      {
         send_to_char( "You do not have that item.\n\r", ch );
         return;
      }

      if( !can_drop_obj( ch, obj ) )
      {
         send_to_char( "You can't let go of it.\n\r", ch );
         return;
      }

      separate_obj( obj );
      act( AT_ACTION, "$n drops $p.", ch, obj, NULL, TO_ROOM );
      act( AT_ACTION, "You drop $p.", ch, obj, NULL, TO_CHAR );

      obj_from_char( obj );
      obj_to_room( obj, ch->in_room, ch );

      oprog_drop_trigger( ch, obj );   /* mudprogs */

      if( char_died( ch ) || obj_extracted( obj ) )
         return;

      if( IS_ROOM_FLAG( ch->in_room, ROOM_DONATION ) )
         SET_OBJ_FLAG( obj, ITEM_DONATION );

      /*
       * For when an immortal or anyone else moves a corpse, since picking it up deletes the corpse save - Samson 
       */
      if( obj->item_type == ITEM_CORPSE_PC )
         write_corpses( NULL, obj->short_descr + 14, NULL );

      /*
       * Clan storeroom saving 
       */
      if( IS_ROOM_FLAG( ch->in_room, ROOM_CLANSTOREROOM ) )
      {
         if( obj->rent < sysdata.minrent )
            check_clan_storeroom( ch );
         else
         {
            ch_printf( ch, "%s is a rent item and cannot be stored here.\n\r", obj->short_descr );
            obj_from_room( obj );
            obj_to_char( obj, ch );
         }
      }
   }
   else
   {
      int cnt = 0;
      char *chk;
      bool fAll;

      if( !str_cmp( arg, "all" ) )
         fAll = TRUE;
      else
         fAll = FALSE;
      if( number > 1 )
         chk = arg;
      else
         chk = &arg[4];
      /*
       * 'drop all' or 'drop all.obj' 
       */
      if( IS_ROOM_FLAG( ch->in_room, ROOM_NODROPALL ) || IS_ROOM_FLAG( ch->in_room, ROOM_CLANSTOREROOM ) )
      {
         send_to_char( "You can't seem to do that here...\n\r", ch );
         return;
      }
      found = FALSE;
      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;

         if( ( fAll || nifty_is_name( chk, obj->name ) )
             && can_see_obj( ch, obj, FALSE ) && obj->wear_loc == WEAR_NONE && can_drop_obj( ch, obj ) )
         {
            found = TRUE;
            if( HAS_PROG( obj->pIndexData, DROP_PROG ) && obj->count > 1 )
            {
               ++cnt;
               separate_obj( obj );
               obj_from_char( obj );
               if( !obj_next )
                  obj_next = ch->first_carrying;
            }
            else
            {
               if( number && ( cnt + obj->count ) > number )
                  split_obj( obj, number - cnt );
               cnt += obj->count;
               obj_from_char( obj );
            }
            /*
             * Edited by Whir for being too spammy (see above)- 1/29/98 
             */
            obj_to_room( obj, ch->in_room, ch );

            if( IS_ROOM_FLAG( ch->in_room, ROOM_DONATION ) )
               SET_OBJ_FLAG( obj, ITEM_DONATION );

            if( IS_ROOM_FLAG( ch->in_room, ROOM_CLANSTOREROOM ) )
            {
               if( obj->rent < sysdata.minrent )
                  check_clan_storeroom( ch );
               else
               {
                  ch_printf( ch, "%s is a rent item and cannot be stored here.\n\r", obj->short_descr );
                  obj_from_room( obj );
                  obj_to_char( obj, ch );
                  continue;
               }
            }
            oprog_drop_trigger( ch, obj );   /* mudprogs */
            if( char_died( ch ) )
               return;
            if( number && cnt >= number )
               break;
         }
      }

      if( !found )
      {
         if( fAll )
            act( AT_PLAIN, "You are not carrying anything.", ch, NULL, NULL, TO_CHAR );
         else
            act( AT_PLAIN, "You are not carrying any $T.", ch, NULL, chk, TO_CHAR );
      }
      else
      {
         if( fAll )
         {
            /*
             * Added by Whir - 1/28/98 
             */
            act( AT_ACTION, "$n drops everything $e owns.", ch, NULL, NULL, TO_ROOM );
            act( AT_ACTION, "You drop everything you own.", ch, NULL, NULL, TO_CHAR );
         }
      }
   }
   if( IS_SAVE_FLAG( SV_DROP ) )
      save_char_obj( ch ); /* duping protector */
   return;
}

CMDF do_give( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL], arg2[MIL];
   CHAR_DATA *victim;
   OBJ_DATA *obj;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !str_cmp( arg2, "to" ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Give what to whom?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( is_number( arg1 ) )
   {
      /*
       * 'give NNNN coins victim' 
       */
      int amount;

      amount = atoi( arg1 );
      if( amount <= 0 || ( str_cmp( arg2, "coins" ) && str_cmp( arg2, "coin" ) ) )
      {
         send_to_char( "Sorry, you can't do that.\n\r", ch );
         return;
      }

      argument = one_argument( argument, arg2 );
      if( !str_cmp( arg2, "to" ) && argument[0] != '\0' )
         argument = one_argument( argument, arg2 );
      if( arg2[0] == '\0' )
      {
         send_to_char( "Give what to whom?\n\r", ch );
         return;
      }

      if( ( victim = get_char_room( ch, arg2 ) ) == NULL )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }

      if( ch->gold < amount )
      {
         send_to_char( "Very generous of you, but you haven't got that much gold.\n\r", ch );
         return;
      }

      ch->gold -= amount;
      victim->gold += amount;

      act_printf( AT_ACTION, ch, NULL, victim, TO_VICT, "$n gives you %d coin%s.", amount, amount == 1 ? "" : "s" );
      act( AT_ACTION, "$n gives $N some gold.", ch, NULL, victim, TO_NOTVICT );
      act( AT_ACTION, "You give $N some gold.", ch, NULL, victim, TO_CHAR );
      mprog_bribe_trigger( victim, ch, amount );
      if( IS_SAVE_FLAG( SV_GIVE ) && !char_died( ch ) )
         save_char_obj( ch );
      if( IS_SAVE_FLAG( SV_RECEIVE ) && !char_died( victim ) )
         save_char_obj( victim );
      return;
   }

   if( !( obj = get_obj_carry( ch, arg1 ) ) )
   {
      send_to_char( "You do not have that item.\n\r", ch );
      return;
   }

   if( obj->wear_loc != WEAR_NONE )
   {
      send_to_char( "You must remove it first.\n\r", ch );
      return;
   }

   if( !( victim = get_char_room( ch, arg2 ) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( !can_drop_obj( ch, obj ) )
   {
      send_to_char( "You can't let go of it.\n\r", ch );
      return;
   }

   if( victim->carry_number + ( get_obj_number( obj ) / obj->count ) > can_carry_n( victim ) )
   {
      act( AT_PLAIN, "$N has $S hands full.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim->carry_weight + ( get_obj_weight( obj ) / obj->count ) > can_carry_w( victim ) )
   {
      act( AT_PLAIN, "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( !can_see_obj( victim, obj, FALSE ) )
   {
      act( AT_PLAIN, "$N can't see it.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( IS_OBJ_FLAG( obj, ITEM_PROTOTYPE ) && !can_take_proto( victim ) )
   {
      act( AT_PLAIN, "You cannot give that to $N!", ch, NULL, victim, TO_CHAR );
      return;
   }

   separate_obj( obj );
   obj_from_char( obj );
   act( AT_ACTION, "$n gives $p to $N.", ch, obj, victim, TO_NOTVICT );
   act( AT_ACTION, "$n gives you $p.", ch, obj, victim, TO_VICT );
   act( AT_ACTION, "You give $p to $N.", ch, obj, victim, TO_CHAR );
   obj_to_char( obj, victim );
   mprog_give_trigger( victim, ch, obj );
   /*
    * Added by Tarl 5 May 2002 to ensure pets aren't given rent items.
    * * Moved up here to prevent a minor bug with the fact the item is purged on save ;)
    */
   if( ( IS_OBJ_FLAG( obj, ITEM_PERSONAL ) || obj->rent >= sysdata.minrent )
       && ( IS_ACT_FLAG( victim, ACT_PET ) || IS_AFFECTED( victim, AFF_CHARM ) ) )
   {
      ch_printf( ch, "%s says 'I'm sorry master, but I can't carry this.'\n\r", victim->short_descr );
      ch_printf( ch, "%s hands %s back to you.\n\r", victim->short_descr, obj->short_descr );
      obj_from_char( obj );
      obj_to_char( obj, ch );
      save_char_obj( ch );
      return;
   }
   if( IS_SAVE_FLAG( SV_GIVE ) && !char_died( ch ) )
      save_char_obj( ch );
   if( IS_SAVE_FLAG( SV_RECEIVE ) && !char_died( victim ) )
      save_char_obj( victim );

   if( IS_ACT_FLAG( victim, ACT_GUILDVENDOR ) )
      check_clan_shop( ch, victim, obj );

   return;
}

/*
 * Remove an object.
 */
bool remove_obj( CHAR_DATA * ch, int iWear, bool fReplace )
{
   OBJ_DATA *obj, *tmpobj;

   if( ( obj = get_eq_char( ch, iWear ) ) == NULL )
      return TRUE;

   if( !fReplace && ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
   {
      act( AT_PLAIN, "$d: you can't carry that many items.", ch, NULL, obj->name, TO_CHAR );
      return FALSE;
   }

   if( !fReplace )
      return FALSE;

   if( IS_OBJ_FLAG( obj, ITEM_NOREMOVE ) )
   {
      act( AT_PLAIN, "You can't remove $p.", ch, obj, NULL, TO_CHAR );
      return FALSE;
   }

   if( obj == get_eq_char( ch, WEAR_WIELD ) && ( tmpobj = get_eq_char( ch, WEAR_DUAL_WIELD ) ) != NULL )
      tmpobj->wear_loc = WEAR_WIELD;

   unequip_char( ch, obj );

   act( AT_ACTION, "$n stops using $p.", ch, obj, NULL, TO_ROOM );
   act( AT_ACTION, "You stop using $p.", ch, obj, NULL, TO_CHAR );
   oprog_remove_trigger( ch, obj );
   return TRUE;
}

/*
 * See if char could be capable of dual-wielding		-Thoric
 */
bool could_dual( CHAR_DATA * ch )
{
   if( IS_NPC( ch ) || ch->pcdata->learned[gsn_dual_wield] )
      return TRUE;

   return FALSE;
}

bool can_dual( CHAR_DATA * ch )
{
   OBJ_DATA *wield = NULL;
   bool nwield = FALSE;

   if( !could_dual( ch ) )
      return FALSE;

   wield = get_eq_char( ch, WEAR_WIELD );

   /* Check for missile wield or dual wield */
   if( get_eq_char( ch, WEAR_MISSILE_WIELD ) || get_eq_char( ch, WEAR_DUAL_WIELD ) )
      nwield = TRUE;

   if( wield && nwield )
   {
      send_to_char( "You are already wielding two weapons!\n\r", ch );
      return FALSE;
   }

   if( ( wield || nwield ) && get_eq_char( ch, WEAR_SHIELD ) )
   {
      send_to_char( "You cannot dual wield, you're already holding a shield!\n\r", ch );
      return FALSE;
   }

   if( ( wield || nwield ) && get_eq_char( ch, WEAR_HOLD ) )
   {
      send_to_char( "You cannot hold another weapon, you're already holding something in that hand!\n\r", ch );
      return FALSE;
   }

   if( IS_OBJ_FLAG( wield, ITEM_TWOHAND ) )
   {
      send_to_char( "You cannot wield a second weapon while already wielding a two-handed weapon!\n\r", ch );
      return FALSE;
   }
   return TRUE;
}

/*
 * Check to see if there is room to wear another object on this location
 * (Layered clothing support)
 */
bool can_layer( CHAR_DATA * ch, OBJ_DATA * obj, short wear_loc )
{
   OBJ_DATA *otmp;
   short bitlayers = 0;
   short objlayers = obj->pIndexData->layers;

   for( otmp = ch->first_carrying; otmp; otmp = otmp->next_content )
      if( otmp->wear_loc == wear_loc )
      {
         if( !otmp->pIndexData->layers )
            return FALSE;
         else
            bitlayers |= otmp->pIndexData->layers;
      }
   if( ( bitlayers && !objlayers ) || bitlayers > objlayers )
      return FALSE;
   if( !bitlayers || ( ( bitlayers & ~objlayers ) == bitlayers ) )
      return TRUE;
   return FALSE;
}

int item_ego( OBJ_DATA * obj )
{
   int obj_ego;

   obj_ego = obj->rent;

   if( obj_ego < sysdata.minrent - 5000 )
      return ( 0 );

   if( nifty_is_name( "scroll", obj->name ) || nifty_is_name( "potion", obj->name )
       || nifty_is_name( "bag", obj->name ) || nifty_is_name( "key", obj->name ) )
   {
      return ( 0 );
   }

   if( obj_ego >= 90000 )
      obj_ego = 90;
   else
      obj_ego /= 1000;

   return ( obj_ego );
}

int char_ego( CHAR_DATA * ch )
{
   int p_ego, tmp;

   if( !IS_NPC( ch ) )
   {
      p_ego = ch->level;
      tmp = ( ( get_curr_int( ch ) + get_curr_wis( ch ) + get_curr_cha( ch ) + get_curr_lck( ch ) ) / 4 );
      tmp = tmp - 17;
      p_ego += tmp;

   }
   else
   {
      p_ego = 10000;
   }
   if( p_ego <= 0 )
      p_ego = ch->level;
   return ( p_ego );
}

int can_wear_obj( CHAR_DATA * ch, OBJ_DATA * obj )
{
#define CWO(c,s) \
    ( ch->Class == (c) && IS_OBJ_FLAG(obj, (s)) )

   if( CWO( CLASS_MAGE, ITEM_ANTI_MAGE ) )
      return FALSE;
   if( CWO( CLASS_WARRIOR, ITEM_ANTI_WARRIOR ) )
      return FALSE;
   if( CWO( CLASS_CLERIC, ITEM_ANTI_CLERIC ) )
      return FALSE;
   if( CWO( CLASS_ROGUE, ITEM_ANTI_ROGUE ) )
      return FALSE;
   if( CWO( CLASS_DRUID, ITEM_ANTI_DRUID ) )
      return FALSE;
   if( CWO( CLASS_RANGER, ITEM_ANTI_RANGER ) )
      return FALSE;
   if( CWO( CLASS_PALADIN, ITEM_ANTI_PALADIN ) )
      return FALSE;
   if( CWO( CLASS_MONK, ITEM_ANTI_MONK ) )
      return FALSE;
   if( CWO( CLASS_NECROMANCER, ITEM_ANTI_NECRO ) )
      return FALSE;
   if( CWO( CLASS_ANTIPALADIN, ITEM_ANTI_APAL ) )
      return FALSE;
   if( CWO( CLASS_BARD, ITEM_ANTI_BARD ) )
      return FALSE;
#undef CWO

#define CWC(c,s) \
      ( ch->Class != (c) && IS_OBJ_FLAG(obj, (s)) )

   if( CWC( CLASS_CLERIC, ITEM_ONLY_CLERIC ) )
      return FALSE;
   if( CWC( CLASS_MAGE, ITEM_ONLY_MAGE ) )
      return FALSE;
   if( CWC( CLASS_ROGUE, ITEM_ONLY_ROGUE ) )
      return FALSE;
   if( CWC( CLASS_WARRIOR, ITEM_ONLY_WARRIOR ) )
      return FALSE;
   if( CWC( CLASS_BARD, ITEM_ONLY_BARD ) )
      return FALSE;
   if( CWC( CLASS_DRUID, ITEM_ONLY_DRUID ) )
      return FALSE;
   if( CWC( CLASS_MONK, ITEM_ONLY_MONK ) )
      return FALSE;
   if( CWC( CLASS_RANGER, ITEM_ONLY_RANGER ) )
      return FALSE;
   if( CWC( CLASS_PALADIN, ITEM_ONLY_PALADIN ) )
      return FALSE;
   if( CWC( CLASS_NECROMANCER, ITEM_ONLY_NECRO ) )
      return FALSE;
   if( CWC( CLASS_ANTIPALADIN, ITEM_ONLY_APAL ) )
      return FALSE;
#undef CWC

#define CWS(c,s) \
	( ch->sex == (c) && IS_OBJ_FLAG(obj, (s)) )

   if( CWS( SEX_NEUTRAL, ITEM_ANTI_NEUTER ) )
      return FALSE;
   if( CWS( SEX_MALE, ITEM_ANTI_MEN ) )
      return FALSE;
   if( CWS( SEX_FEMALE, ITEM_ANTI_WOMEN ) )
      return FALSE;
   if( CWS( SEX_HERMAPHRODYTE, ITEM_ANTI_HERMA ) )
      return FALSE;
#undef CWS

   return ( TRUE );
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 *
 * Restructured a bit to allow for specifying body location	-Thoric
 * & Added support for layering on certain body locations
 */
/* Function modified from original form by Samson - 10-17-97 */
void wear_obj( CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace, int wear_bit )
{
   OBJ_DATA *tmpobj = NULL;
   int bit, tmp;

   separate_obj( obj );

   if( IS_OBJ_FLAG( obj, ITEM_PERSONAL ) && str_cmp( ch->name, obj->owner ) )
   {
      send_to_char( "That item is personalized and belongs to someone else.\n\r", ch );
      return;
   }

   if( char_ego( ch ) < item_ego( obj ) )
   {
      send_to_char( "You can't figure out how to use it.\n\r", ch );
      act( AT_ACTION, "$n tries to use $p, but can't figure it out.", ch, obj, NULL, TO_ROOM );
      return;
   }

   if( !IS_IMMORTAL( ch ) && !can_wear_obj( ch, obj ) )
   {
      act( AT_MAGIC, "You are forbidden to use that item.", ch, NULL, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to use $p, but is forbidden to do so.", ch, obj, NULL, TO_ROOM );
      return;
   }

   /*
    * Check to see if an item they're trying to equip requires that they be mounted - Samson 3-18-01 
    */
   if( !IS_IMMORTAL( ch ) && IS_OBJ_FLAG( obj, ITEM_MUSTMOUNT ) && !IS_MOUNTED( ch ) )
   {
      act( AT_ACTION, "You must be mounted to use $p.", ch, obj, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to use $p while not mounted.", ch, obj, NULL, TO_ROOM );
      return;
   }
   /*
    * Added by Tarl 8 May 2002 so that horses can't wield weapons and armor. 
    */
   if( IS_NPC( ch ) )
   {
      if( GET_RACE( ch ) == RACE_HORSE )
      {
         act( AT_ACTION, "$n snorts in surprise. Maybe they can't use that?", ch, obj, NULL, TO_ROOM );
         return;
      }
   }

   if( wear_bit > -1 )
   {
      bit = wear_bit;
      if( !CAN_WEAR( obj, 1 << bit ) )
      {
         if( fReplace )
         {
            switch ( 1 << bit )
            {
               case ITEM_HOLD:
                  send_to_char( "You cannot hold that.\n\r", ch );
                  break;
               case ITEM_WIELD:
               case ITEM_MISSILE_WIELD:
                  send_to_char( "You cannot wield that.\n\r", ch );
                  break;
               default:
                  ch_printf( ch, "You cannot wear that on your %s.\n\r", w_flags[bit] );
            }
         }
         return;
      }
   }
   else
   {
      for( bit = -1, tmp = 1; tmp < 31; tmp++ )
      {
         if( CAN_WEAR( obj, 1 << tmp ) )
         {
            bit = tmp;
            break;
         }
      }
   }

   /*
    * currently cannot have a light in non-light position 
    */
   if( obj->item_type == ITEM_LIGHT )
   {
      if( !remove_obj( ch, WEAR_LIGHT, fReplace ) )
         return;
      if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
      {
         act( AT_ACTION, "$n holds $p as a light.", ch, obj, NULL, TO_ROOM );
         act( AT_ACTION, "You hold $p as your light.", ch, obj, NULL, TO_CHAR );
      }
      equip_char( ch, obj, WEAR_LIGHT );
      oprog_wear_trigger( ch, obj );
      return;
   }

   if( bit == -1 )
   {
      if( fReplace )
         send_to_char( "You can't wear, wield, or hold that.\n\r", ch );
      return;
   }

   switch ( 1 << bit )
   {
      default:
         bug( "wear_obj: uknown/unused item_wear bit %d. Obj vnum: %d", bit, obj->pIndexData->vnum );
         if( fReplace )
            send_to_char( "You can't wear, wield, or hold that.\n\r", ch );
         return;

      case ITEM_LODGE_RIB:
         act( AT_ACTION, "$p strikes you and deeply imbeds itself in your ribs!", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$p strikes $n and deeply imbeds itself in $s ribs!", ch, obj, NULL, TO_ROOM );
         equip_char( ch, obj, WEAR_LODGE_RIB );
         break;

      case ITEM_LODGE_ARM:
         act( AT_ACTION, "$p strikes you and deeply imbeds itself in your arm!", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$p strikes $n and deeply imbeds itself in $s arm!", ch, obj, NULL, TO_ROOM );
         equip_char( ch, obj, WEAR_LODGE_ARM );
         break;

      case ITEM_LODGE_LEG:
         act( AT_ACTION, "$p strikes you and deeply imbeds itself in your leg!", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$p strikes $n and deeply imbeds itself in $s leg!", ch, obj, NULL, TO_ROOM );
         equip_char( ch, obj, WEAR_LODGE_LEG );
         break;

      case ITEM_HOLD:
         if( get_eq_char( ch, WEAR_DUAL_WIELD )
         || ( get_eq_char( ch, WEAR_WIELD )
         && ( get_eq_char( ch, WEAR_MISSILE_WIELD ) || get_eq_char( ch, WEAR_SHIELD ) ) ) )
         {
            if( get_eq_char( ch, WEAR_SHIELD ) )
               send_to_char( "You cannot hold something while using a weapon and a shield!\n\r", ch );
            else
               send_to_char( "You cannot hold something AND two weapons!\n\r", ch );
            return;
         }
         tmpobj = get_eq_char( ch, WEAR_WIELD );
         if( tmpobj && IS_OBJ_FLAG( tmpobj, ITEM_TWOHAND ) )
         {
            send_to_char( "You cannot hold something while wielding a two-handed weapon!\n\r", ch );
            return;
         }
         if( !remove_obj( ch, WEAR_HOLD, fReplace ) )
            return;
         if( obj->item_type == ITEM_WAND
             || obj->item_type == ITEM_STAFF
             || obj->item_type == ITEM_FOOD
             || obj->item_type == ITEM_COOK
             || obj->item_type == ITEM_PILL
             || obj->item_type == ITEM_POTION
             || obj->item_type == ITEM_SCROLL
             || obj->item_type == ITEM_DRINK_CON
             || obj->item_type == ITEM_BLOOD
             || obj->item_type == ITEM_PIPE
             || obj->item_type == ITEM_HERB
             || obj->item_type == ITEM_KEY || !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n holds $p in $s hands.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You hold $p in your hands.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_HOLD );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_FINGER:
         if( get_eq_char( ch, WEAR_FINGER_L )
             && get_eq_char( ch, WEAR_FINGER_R )
             && !remove_obj( ch, WEAR_FINGER_L, fReplace ) && !remove_obj( ch, WEAR_FINGER_R, fReplace ) )
            return;

         if( !get_eq_char( ch, WEAR_FINGER_L ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
            {
               act( AT_ACTION, "$n slips $s left finger into $p.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You slip your left finger into $p.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_FINGER_L );
            oprog_wear_trigger( ch, obj );
            return;
         }

         if( !get_eq_char( ch, WEAR_FINGER_R ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
            {
               act( AT_ACTION, "$n slips $s right finger into $p.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You slip your right finger into $p.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_FINGER_R );
            oprog_wear_trigger( ch, obj );
            return;
         }

         send_to_char( "You already wear something on both fingers.\n\r", ch );
         return;

      case ITEM_WEAR_NECK:
         if( get_eq_char( ch, WEAR_NECK_1 ) != NULL
             && get_eq_char( ch, WEAR_NECK_2 ) != NULL
             && !remove_obj( ch, WEAR_NECK_1, fReplace ) && !remove_obj( ch, WEAR_NECK_2, fReplace ) )
            return;

         if( !get_eq_char( ch, WEAR_NECK_1 ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_NECK_1 );
            oprog_wear_trigger( ch, obj );
            return;
         }

         if( !get_eq_char( ch, WEAR_NECK_2 ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_NECK_2 );
            oprog_wear_trigger( ch, obj );
            return;
         }

         send_to_char( "You already wear two neck items.\n\r", ch );
         return;

      case ITEM_WEAR_BODY:
         if( !can_layer( ch, obj, WEAR_BODY ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\n\r", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n fits $p on $s body.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You fit $p on your body.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_BODY );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_HEAD:
         if( !remove_obj( ch, WEAR_HEAD, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n dons $p upon $s head.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You don $p upon your head.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_HEAD );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_EYES:
         if( !remove_obj( ch, WEAR_EYES, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n places $p on $s eyes.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You place $p on your eyes.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_EYES );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_FACE:
         if( !remove_obj( ch, WEAR_FACE, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n places $p on $s face.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You place $p on your face.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_FACE );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_EARS:
         if( !remove_obj( ch, WEAR_EARS, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p on $s ears.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p on your ears.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_EARS );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_LEGS:
         if( !can_layer( ch, obj, WEAR_LEGS ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\n\r", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n slips into $p.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You slip into $p.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_LEGS );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_FEET:
         if( !can_layer( ch, obj, WEAR_FEET ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\n\r", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p on $s feet.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p on your feet.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_FEET );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_HANDS:
         if( !can_layer( ch, obj, WEAR_HANDS ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\n\r", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p on $s hands.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p on your hands.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_HANDS );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_ARMS:
         if( !can_layer( ch, obj, WEAR_ARMS ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\n\r", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p on $s arms.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p on your arms.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_ARMS );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_ABOUT:
         if( !can_layer( ch, obj, WEAR_ABOUT ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\n\r", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p about $s body.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p about your body.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_ABOUT );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_BACK:
         if( !remove_obj( ch, WEAR_BACK, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n slings $p on $s back.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You sling $p on your back.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_BACK );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_WAIST:
         if( !can_layer( ch, obj, WEAR_WAIST ) )
         {
            send_to_char( "It won't fit overtop of what you're already wearing.\n\r", ch );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p about $s waist.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p about your waist.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_WAIST );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_WRIST:
         if( get_eq_char( ch, WEAR_WRIST_L )
             && get_eq_char( ch, WEAR_WRIST_R )
             && !remove_obj( ch, WEAR_WRIST_L, fReplace ) && !remove_obj( ch, WEAR_WRIST_R, fReplace ) )
            return;

         if( !get_eq_char( ch, WEAR_WRIST_L ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
            {
               act( AT_ACTION, "$n fits $p around $s left wrist.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You fit $p around your left wrist.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_WRIST_L );
            oprog_wear_trigger( ch, obj );
            return;
         }

         if( !get_eq_char( ch, WEAR_WRIST_R ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
            {
               act( AT_ACTION, "$n fits $p around $s right wrist.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You fit $p around your right wrist.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_WRIST_R );
            oprog_wear_trigger( ch, obj );
            return;
         }

         send_to_char( "You already wear two wrist items.\n\r", ch );
         return;

      case ITEM_WEAR_ANKLE:
         if( get_eq_char( ch, WEAR_ANKLE_L )
             && get_eq_char( ch, WEAR_ANKLE_R )
             && !remove_obj( ch, WEAR_ANKLE_L, fReplace ) && !remove_obj( ch, WEAR_ANKLE_R, fReplace ) )
            return;

         if( !get_eq_char( ch, WEAR_ANKLE_L ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
            {
               act( AT_ACTION, "$n fits $p around $s left ankle.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You fit $p around your left ankle.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_ANKLE_L );
            oprog_wear_trigger( ch, obj );
            return;
         }

         if( !get_eq_char( ch, WEAR_ANKLE_R ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
            {
               act( AT_ACTION, "$n fits $p around $s right ankle.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You fit $p around your right ankle.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_ANKLE_R );
            oprog_wear_trigger( ch, obj );
            return;
         }

         send_to_char( "You already wear two ankle items.\n\r", ch );
         return;

      case ITEM_WEAR_SHIELD:
         if( get_eq_char( ch, WEAR_DUAL_WIELD )
             || ( get_eq_char( ch, WEAR_WIELD ) && get_eq_char( ch, WEAR_MISSILE_WIELD ) )
             || ( get_eq_char( ch, WEAR_WIELD ) && get_eq_char( ch, WEAR_HOLD ) ) )
         {
            if( get_eq_char( ch, WEAR_HOLD ) )
               send_to_char( "You can't use a shield while using a weapon and holding something!\n\r", ch );
            else
               send_to_char( "You can't use a shield AND two weapons!\n\r", ch );
            return;
         }
         tmpobj = get_eq_char( ch, WEAR_WIELD );
         if( IS_OBJ_FLAG( tmpobj, ITEM_TWOHAND ) )
         {
            send_to_char( "You cannot wear a shield while wielding a two-handed weapon!\n\r", ch );
            return;
         }
         if( !remove_obj( ch, WEAR_SHIELD, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n uses $p as a shield.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You use $p as a shield.", ch, obj, NULL, TO_CHAR );
         }
         equip_char( ch, obj, WEAR_SHIELD );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_MISSILE_WIELD:
      case ITEM_WIELD:
         if( !could_dual( ch ) )
         {
            if( !remove_obj( ch, WEAR_MISSILE_WIELD, fReplace ) )
               return;
            if( !remove_obj( ch, WEAR_WIELD, fReplace ) )
               return;
            tmpobj = NULL;
         }

         else
         {
            OBJ_DATA *mw, *dw, *hd, *sd;
            tmpobj = get_eq_char( ch, WEAR_WIELD );
            mw = get_eq_char( ch, WEAR_MISSILE_WIELD );
            dw = get_eq_char( ch, WEAR_DUAL_WIELD );
            hd = get_eq_char( ch, WEAR_HOLD );
            sd = get_eq_char( ch, WEAR_SHIELD );

            if( tmpobj )
            {
               if( !can_dual( ch ) )
                  return;

               if( get_obj_weight( obj ) + get_obj_weight( tmpobj ) > str_app[get_curr_str( ch )].wield )
               {
                  send_to_char( "It is too heavy for you to wield.\n\r", ch );
                  return;
               }

               if( mw || dw )
               {
                  send_to_char( "You're already wielding two weapons.\n\r", ch );
                  return;
               }

               if( hd || sd )
               {
                  send_to_char( "You're already wielding a weapon AND holding something.\n\r", ch );
                  return;
               }

               if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n dual-wields $p.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You dual-wield $p.", ch, obj, NULL, TO_CHAR );
               }
               if( 1 << bit == ITEM_MISSILE_WIELD )
                  equip_char( ch, obj, WEAR_MISSILE_WIELD );
               else
                  equip_char( ch, obj, WEAR_DUAL_WIELD );
               oprog_wear_trigger( ch, obj );
               return;
            }

            if( mw )
            {
               if( !can_dual( ch ) )
                  return;

               if( 1 << bit == ITEM_MISSILE_WIELD )
               {
                  send_to_char( "You're already wielding a missile weapon.\n\r", ch );
                  return;
               }

               if( get_obj_weight( obj ) + get_obj_weight( mw ) > str_app[get_curr_str( ch )].wield )
               {
                  send_to_char( "It is too heavy for you to wield.\n\r", ch );
                  return;
               }

               if( tmpobj || dw )
               {
                  send_to_char( "You're already wielding two weapons.\n\r", ch );
                  return;
               }

               if( hd || sd )
               {
                  send_to_char( "You're already wielding a weapon AND holding something.\n\r", ch );
                  return;
               }

               if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wields $p.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wield $p.", ch, obj, NULL, TO_CHAR );
               }
               equip_char( ch, obj, WEAR_WIELD );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }

         if( get_obj_weight( obj ) > str_app[get_curr_str( ch )].wield )
         {
            send_to_char( "It is too heavy for you to wield.\n\r", ch );
            return;
         }

         if( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wields $p.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wield $p.", ch, obj, NULL, TO_CHAR );
         }
         if( 1 << bit == ITEM_MISSILE_WIELD )
            equip_char( ch, obj, WEAR_MISSILE_WIELD );
         else
            equip_char( ch, obj, WEAR_WIELD );
         oprog_wear_trigger( ch, obj );
         return;
   }
}

CMDF do_wear( CHAR_DATA * ch, char *argument )
{
   char arg1[MIL];
   char arg2[MIL];
   OBJ_DATA *obj;
   int wear_bit;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( ( !str_cmp( arg2, "on" ) || !str_cmp( arg2, "upon" ) || !str_cmp( arg2, "around" ) ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' )
   {
      send_to_char( "Wear, wield, or hold what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( arg1, "all" ) )
   {
      OBJ_DATA *obj_next;

      for( obj = ch->first_carrying; obj; obj = obj_next )
      {
         obj_next = obj->next_content;
         if( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj, FALSE ) )
         {
            wear_obj( ch, obj, FALSE, -1 );
            if( char_died( ch ) )
               return;
         }
      }
      return;
   }
   else
   {
      if( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
      {
         send_to_char( "You do not have that item.\n\r", ch );
         return;
      }
      if( arg2[0] != '\0' )
         wear_bit = get_wflag( arg2 );
      else
         wear_bit = -1;
      wear_obj( ch, obj, TRUE, wear_bit );
   }

   return;
}

CMDF do_remove( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj, *obj_next;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Remove what?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( argument, "all" ) )   /* SB Remove all */
   {
      for( obj = ch->first_carrying; obj != NULL; obj = obj_next )
      {
         obj_next = obj->next_content;
         if( obj->wear_loc != WEAR_NONE && can_see_obj( ch, obj, FALSE ) )
            remove_obj( ch, obj->wear_loc, TRUE );
      }
      return;
   }

   if( ( obj = get_obj_wear( ch, argument ) ) == NULL )
   {
      send_to_char( "You are not using that item.\n\r", ch );
      return;
   }
   if( ( obj_next = get_eq_char( ch, obj->wear_loc ) ) != obj )
   {
      act( AT_PLAIN, "You must remove $p first.", ch, obj_next, NULL, TO_CHAR );
      return;
   }

   remove_obj( ch, obj->wear_loc, TRUE );
   return;
}

/* Function modified from original form on unknown date - Samson */
CMDF do_bury( CHAR_DATA * ch, char *argument )
{
   OBJ_DATA *obj;
   bool shovel;
   short move;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "What do you wish to bury?\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   shovel = FALSE;
   for( obj = ch->first_carrying; obj; obj = obj->next_content )
      if( obj->item_type == ITEM_SHOVEL )
      {
         shovel = TRUE;
         break;
      }

   obj = get_obj_list_rev( ch, argument, ch->in_room->last_content );
   if( !obj )
   {
      send_to_char( "You can't find it.\n\r", ch );
      return;
   }
   separate_obj( obj );
   if( !CAN_WEAR( obj, ITEM_TAKE ) )
   {
      if( !IS_OBJ_FLAG( obj, ITEM_CLANCORPSE ) || !IS_PCFLAG( ch, PCFLAG_DEADLY ) )
      {
         act( AT_PLAIN, "You cannot bury $p.", ch, obj, 0, TO_CHAR );
         return;
      }
   }

   switch ( ch->in_room->sector_type )
   {
      case SECT_CITY:
      case SECT_INDOORS:
         send_to_char( "The floor is too hard to dig through.\n\r", ch );
         return;
      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:   /* Water checks for river and no_swim added by Samson */
      case SECT_UNDERWATER:
      case SECT_RIVER:
         send_to_char( "You cannot bury something here.\n\r", ch );
         return;
      case SECT_AIR:
         send_to_char( "What?  In the air?!\n\r", ch );
         return;
   }

   if( obj->weight > ( UMAX( 5, ( can_carry_w( ch ) / 10 ) ) ) && !shovel )
   {
      send_to_char( "You'd need a shovel to bury something that big.\n\r", ch );
      return;
   }

   move = ( obj->weight * 50 * ( shovel ? 1 : 5 ) ) / UMAX( 1, can_carry_w( ch ) );
   move = URANGE( 2, move, 1000 );
   if( move > ch->move )
   {
      send_to_char( "You don't have the energy to bury something of that size.\n\r", ch );
      return;
   }
   ch->move -= move;
   if( obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC )
      adjust_favor( ch, 6, 1 );

   act( AT_ACTION, "You solemnly bury $p...", ch, obj, NULL, TO_CHAR );
   act( AT_ACTION, "$n solemnly buries $p...", ch, obj, NULL, TO_ROOM );
   SET_OBJ_FLAG( obj, ITEM_BURIED );
   WAIT_STATE( ch, URANGE( 10, move / 2, 100 ) );
   return;
}

CMDF do_sacrifice( CHAR_DATA * ch, char *argument )
{
   char name[MIL];
   OBJ_DATA *obj;

   if( !argument || argument[0] == '\0' || !str_cmp( argument, ch->name ) )
   {
      act( AT_ACTION, "$n offers $mself to $s deity, who graciously declines.", ch, NULL, NULL, TO_ROOM );
      send_to_char( "Your deity appreciates your offer and may accept it later.\n\r", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   obj = get_obj_list_rev( ch, argument, ch->in_room->last_content );
   if( !obj )
   {
      send_to_char( "You can't find it.\n\r", ch );
      return;
   }

   separate_obj( obj );
   if( !CAN_WEAR( obj, ITEM_TAKE ) )
   {
      act( AT_PLAIN, "$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR );
      return;
   }
   if( !IS_NPC( ch ) && ch->pcdata->deity && ch->pcdata->deity->name[0] != '\0' )
      mudstrlcpy( name, ch->pcdata->deity->name, MIL );

   else if( !IS_NPC( ch ) && ch->pcdata->clan && ch->pcdata->clan->deity[0] != '\0' )
      mudstrlcpy( name, ch->pcdata->clan->deity, MIL );

   else
      mudstrlcpy( name, "The Wedgy", MIL );

   if( obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC )
   {
      int xp;

      adjust_favor( ch, 5, 1 );

      if( obj->item_type == ITEM_CORPSE_NPC )
         xp = obj->level * 10;
      else
         xp = obj->value[4] * 10;

      ch->gold += xp;
      if( !str_cmp( name, "The Wedgy" ) )
         ch_printf( ch, "The Wedgy swoops down from the void to scoop up %s!\n\r", obj->short_descr );
      else
         ch_printf( ch, "%s grants you %d gold for your sacrifice.\n\r", name, xp );
   }
   else
   {
      ch->gold += 1;
      ch_printf( ch, "%s gives you one gold coin for your sacrifice.\n\r", name );
   }
   act_printf( AT_ACTION, ch, obj, NULL, TO_ROOM, "$n sacrifices $p to %s.", name );
   oprog_sac_trigger( ch, obj );
   if( obj_extracted( obj ) )
      return;
   separate_obj( obj );
   extract_obj( obj );
   return;
}

/* Function modified from original form - Samson 4-21-98 */
CMDF do_brandish( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *vch, *vch_next;
   OBJ_DATA *staff;
   ch_ret retcode;
   int sn;

   if( ( staff = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
   {
      send_to_char( "You hold nothing in your hand.\n\r", ch );
      return;
   }

   if( staff->item_type != ITEM_STAFF )
   {
      send_to_char( "You can brandish only with a staff.\n\r", ch );
      return;
   }

   if( ( sn = staff->value[3] ) < 0 || sn >= top_sn || skill_table[sn]->spell_fun == NULL )
   {
      bug( "Do_brandish: bad sn %d.", sn );
      return;
   }

   WAIT_STATE( ch, 2 * sysdata.pulseviolence );

   if( staff->value[2] > 0 )
   {
      if( !oprog_use_trigger( ch, staff, NULL, NULL, NULL ) )
      {
         act( AT_MAGIC, "$n brandishes $p.", ch, staff, NULL, TO_ROOM );
         act( AT_MAGIC, "You brandish $p.", ch, staff, NULL, TO_CHAR );
      }
      for( vch = ch->in_room->first_person; vch; vch = vch_next )
      {
         vch_next = vch->next_in_room;
         if( IS_PLR_FLAG( vch, PLR_WIZINVIS ) && vch->pcdata->wizinvis > ch->level )
            continue;
         else
            switch ( skill_table[sn]->target )
            {
               default:
                  bug( "Do_brandish: bad target for sn %d.", sn );
                  return;

               case TAR_IGNORE:
                  if( vch != ch )
                     continue;
                  break;

               case TAR_CHAR_OFFENSIVE:
                  if( IS_NPC( ch ) ? IS_NPC( vch ) : !IS_NPC( vch ) )
                     continue;
                  break;

               case TAR_CHAR_DEFENSIVE:
                  if( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) )
                     continue;
                  break;

               case TAR_CHAR_SELF:
                  if( vch != ch )
                     continue;
                  break;
            }

         retcode = obj_cast_spell( staff->value[3], staff->value[0], ch, vch, NULL );
         if( retcode == rCHAR_DIED )
         {
            bug( "%s", "do_brandish: char died" );
            return;
         }
      }
   }

   if( --staff->value[2] <= 0 )  /* Modified to prevent extraction when reaching zero */
   {
      staff->value[2] = 0; /* Added to prevent the damn things from getting negative values - Samson 4-21-98 */
      act( AT_MAGIC, "$p blazes brightly in $n's hands, but does nothing.", ch, staff, NULL, TO_ROOM );
      act( AT_MAGIC, "$p blazes brightly, but does nothing.", ch, staff, NULL, TO_CHAR );
   }
   return;
}

/* Function modified from original form - Samson 4-21-98 */
CMDF do_zap( CHAR_DATA * ch, char *argument )
{
   CHAR_DATA *victim;
   OBJ_DATA *wand, *obj;
   ch_ret retcode;

   if( ( !argument || argument[0] == '\0' ) && !ch->fighting )
   {
      send_to_char( "Zap whom or what?\n\r", ch );
      return;
   }

   if( ( wand = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
   {
      send_to_char( "You hold nothing in your hand.\n\r", ch );
      return;
   }

   if( wand->item_type != ITEM_WAND )
   {
      send_to_char( "You can zap only with a wand.\n\r", ch );
      return;
   }

   obj = NULL;
   if( !argument || argument[0] == '\0' )
   {
      if( ch->fighting )
         victim = who_fighting( ch );
      else
      {
         send_to_char( "Zap whom or what?\n\r", ch );
         return;
      }
   }
   else
   {
      if( ( victim = get_char_room( ch, argument ) ) == NULL && ( obj = get_obj_here( ch, argument ) ) == NULL )
      {
         send_to_char( "You can't find it.\n\r", ch );
         return;
      }
   }

   WAIT_STATE( ch, 2 * sysdata.pulseviolence );

   if( wand->value[2] > 0 )
   {
      if( victim )
      {
         if( !oprog_use_trigger( ch, wand, victim, NULL, NULL ) )
         {
            act( AT_MAGIC, "$n aims $p at $N.", ch, wand, victim, TO_ROOM );
            act( AT_MAGIC, "You aim $p at $N.", ch, wand, victim, TO_CHAR );
         }
      }
      else
      {
         if( !oprog_use_trigger( ch, wand, NULL, obj, NULL ) )
         {
            act( AT_MAGIC, "$n aims $p at $P.", ch, wand, obj, TO_ROOM );
            act( AT_MAGIC, "You aim $p at $P.", ch, wand, obj, TO_CHAR );
         }
      }

      retcode = obj_cast_spell( wand->value[3], wand->value[0], ch, victim, obj );
      if( retcode == rCHAR_DIED )
      {
         bug( "%s", "do_zap: char died" );
         return;
      }
   }

   if( --wand->value[2] <= 0 )   /* Modified to prevent extraction when reaching zero */
   {
      wand->value[2] = 0;  /* To prevent negative values - Samson */
      act( AT_MAGIC, "$p hums softly, but does nothing.", ch, wand, NULL, TO_ROOM );
      act( AT_MAGIC, "$p hums softly, but does nothing.", ch, wand, NULL, TO_CHAR );
   }
   return;
}

/* Make objects in rooms that are nofloor fall - Scryn 1/23/96 */
void obj_fall( OBJ_DATA * obj, bool through )
{
   EXIT_DATA *pexit;
   ROOM_INDEX_DATA *to_room;
   static int fall_count;
   static bool is_falling; /* Stop loops from the call to obj_to_room()  -- Altrag */

   if( !obj->in_room || is_falling )
      return;

   if( fall_count > 30 )
   {
      bug( "%s", "object falling in loop more than 30 times" );
      extract_obj( obj );
      fall_count = 0;
      return;
   }

   if( IS_ROOM_FLAG( obj->in_room, ROOM_NOFLOOR ) && CAN_GO( obj, DIR_DOWN ) && !IS_OBJ_FLAG( obj, ITEM_MAGIC ) )
   {
      pexit = get_exit( obj->in_room, DIR_DOWN );
      to_room = pexit->to_room;

      if( through )
         fall_count++;
      else
         fall_count = 0;

      if( obj->in_room == to_room )
      {
         bug( "Object falling into same room, room %d", to_room->vnum );
         extract_obj( obj );
         return;
      }

      if( obj->in_room->first_person )
      {
         act( AT_PLAIN, "$p falls far below...", obj->in_room->first_person, obj, NULL, TO_ROOM );
         act( AT_PLAIN, "$p falls far below...", obj->in_room->first_person, obj, NULL, TO_CHAR );
      }
      obj_from_room( obj );
      is_falling = TRUE;
      obj_to_room( obj, to_room, NULL );
      is_falling = FALSE;

      if( obj->in_room->first_person )
      {
         act( AT_PLAIN, "$p falls from above...", obj->in_room->first_person, obj, NULL, TO_ROOM );
         act( AT_PLAIN, "$p falls from above...", obj->in_room->first_person, obj, NULL, TO_CHAR );
      }

      if( !IS_ROOM_FLAG( obj->in_room, ROOM_NOFLOOR ) && through )
      {
/*		int dam = (int)9.81*sqrt(fall_count*2/9.81)*obj->weight/2;
*/ int dam = fall_count * obj->weight / 2;
         /*
          * Damage players 
          */
         if( obj->in_room->first_person && number_percent(  ) > 15 )
         {
            CHAR_DATA *rch;
            CHAR_DATA *vch = NULL;
            int chcnt = 0;

            for( rch = obj->in_room->first_person; rch; rch = rch->next_in_room, chcnt++ )
               if( number_range( 0, chcnt ) == 0 )
                  vch = rch;
            act( AT_WHITE, "$p falls on $n!", vch, obj, NULL, TO_ROOM );
            act( AT_WHITE, "$p falls on you!", vch, obj, NULL, TO_CHAR );

            if( IS_ACT_FLAG( vch, ACT_HARDHAT ) )
               act( AT_WHITE, "$p bounces harmlessly off your head!", vch, obj, NULL, TO_CHAR );
            else
               damage( vch, vch, dam * vch->level, TYPE_UNDEFINED );
         }
         /*
          * Damage objects 
          */
         switch ( obj->item_type )
         {
            case ITEM_WEAPON:
            case ITEM_MISSILE_WEAPON:
               if( ( obj->value[6] - dam ) <= 0 )
               {
                  if( obj->in_room->first_person )
                  {
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_ROOM );
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_CHAR );
                  }
                  make_scraps( obj );
               }
               else
                  obj->value[6] -= dam;
               break;
            case ITEM_PROJECTILE:
               if( ( obj->value[5] - dam ) <= 0 )
               {
                  if( obj->in_room->first_person )
                  {
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_ROOM );
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_CHAR );
                  }
                  make_scraps( obj );
               }
               else
                  obj->value[5] -= dam;
               break;
            case ITEM_ARMOR:
               if( ( obj->value[0] - dam ) <= 0 )
               {
                  if( obj->in_room->first_person )
                  {
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_ROOM );
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_CHAR );
                  }
                  make_scraps( obj );
               }
               else
                  obj->value[0] -= dam;
               break;
            default:
               if( ( dam * 15 ) > get_obj_resistance( obj ) )
               {
                  if( obj->in_room->first_person )
                  {
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_ROOM );
                     act( AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_CHAR );
                  }
                  make_scraps( obj );
               }
               break;
         }
      }
      obj_fall( obj, TRUE );
   }
   return;
}
