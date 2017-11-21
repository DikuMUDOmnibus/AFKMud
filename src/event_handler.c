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
 *                               Event Handlers                             *
 ****************************************************************************/

#include <string.h>
#include "mud.h"
#include "auction.h"
#include "event.h"

SPELLF spell_smaug( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_energy_drain( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_fire_breath( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_frost_breath( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_acid_breath( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_lightning_breath( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_gas_breath( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_spiral_blast( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_dispel_magic( int sn, int level, CHAR_DATA * ch, void *vo );
SPELLF spell_dispel_evil( int sn, int level, CHAR_DATA * ch, void *vo );
void mprog_fight_trigger( CHAR_DATA * mob, CHAR_DATA * ch );
void mprog_hitprcnt_trigger( CHAR_DATA * mob, CHAR_DATA * ch );
CMDF do_ageattack( CHAR_DATA * ch, char *argument );
void talk_auction( char *fmt, ... );
void save_aucvault( CHAR_DATA * ch, char *aucmob );
void add_sale( char *aucmob, char *seller, char *buyer, char *item, int bidamt, bool collected );

int reboot_counter;

/* Replaces violence_update */
void ev_violence( void *data )
{
   CHAR_DATA *ch, *victim, *rch, *rch_next;
   ch_ret retcode;
   int attacktype, cnt;

   ch = ( CHAR_DATA * ) data;

   if( !ch )
   {
      bug( "%s", "ev_violence: NULL ch pointer!" );
      return;
   }
   victim = who_fighting( ch );

   if( !victim || !ch->in_room || !ch->name || ( ch->in_room != victim->in_room ) )
   {
      stop_fighting( ch, TRUE );
      return;
   }

   if( char_died( ch ) )
   {
      stop_fighting( ch, TRUE );
      return;
   }

   /*
    * Let the battle begin! 
    */
   if( IS_AFFECTED( ch, AFF_PARALYSIS ) )
   {
      add_event( 2, ev_violence, ch );
      return;
   }

   retcode = rNONE;

   if( IS_ROOM_FLAG( ch->in_room, ROOM_SAFE ) )
   {
      log_printf( "ev_violence: %s fighting %s in a SAFE room.", ch->name, victim->name );
      stop_fighting( ch, TRUE );
   }
   else if( IS_AWAKE( ch ) && ch->in_room == victim->in_room )
      retcode = multi_hit( ch, victim, TYPE_UNDEFINED );
   else
      stop_fighting( ch, FALSE );

   if( char_died( ch ) )
      return;

   if( retcode == rCHAR_DIED || ( victim = who_fighting( ch ) ) == NULL )
      return;

   /*
    *  Mob triggers
    *  -- Added some victim death checks, because it IS possible.. -- Alty
    */
   rprog_rfight_trigger( ch );
   if( char_died( ch ) || char_died( victim ) )
      return;

   mprog_hitprcnt_trigger( ch, victim );
   if( char_died( ch ) || char_died( victim ) )
      return;

   mprog_fight_trigger( ch, victim );
   if( char_died( ch ) || char_died( victim ) )
      return;

   /*
    * NPC special attack flags            -Thoric
    */
   if( IS_NPC( ch ) )
   {
      if( !xIS_EMPTY( ch->attacks ) )
      {
         attacktype = -1;
         if( 30 + ( ch->level / 4 ) >= number_percent(  ) )
         {
            cnt = 0;
            for( ;; )
            {
               if( cnt++ > 10 )
               {
                  attacktype = -1;
                  break;
               }
               attacktype = number_range( 7, MAX_ATTACK_TYPE - 1 );
               if( IS_ATTACK( ch, attacktype ) )
                  break;
            }
            switch ( attacktype )
            {
               case ATCK_BASH:
                  interpret( ch, "bash" );
                  retcode = global_retcode;
                  break;
               case ATCK_STUN:
                  interpret( ch, "stun" );
                  retcode = global_retcode;
                  break;
               case ATCK_GOUGE:
                  interpret( ch, "gouge" );
                  retcode = global_retcode;
                  break;
               case ATCK_AGE:
                  do_ageattack( ch, "" );
                  retcode = global_retcode;
                  break;
               case ATCK_DRAIN:
                  retcode = spell_energy_drain( skill_lookup( "energy drain" ), ch->level, ch, victim );
                  break;
               case ATCK_FIREBREATH:
                  retcode = spell_fire_breath( skill_lookup( "fire breath" ), ch->level, ch, victim );
                  break;
               case ATCK_FROSTBREATH:
                  retcode = spell_frost_breath( skill_lookup( "frost breath" ), ch->level, ch, victim );
                  break;
               case ATCK_ACIDBREATH:
                  retcode = spell_acid_breath( skill_lookup( "acid breath" ), ch->level, ch, victim );
                  break;
               case ATCK_LIGHTNBREATH:
                  retcode = spell_lightning_breath( skill_lookup( "lightning breath" ), ch->level, ch, victim );
                  break;
               case ATCK_GASBREATH:
                  retcode = spell_gas_breath( skill_lookup( "gas breath" ), ch->level, ch, victim );
                  break;
               case ATCK_SPIRALBLAST:
                  retcode = spell_spiral_blast( skill_lookup( "spiral blast" ), ch->level, ch, victim );
                  break;
               case ATCK_POISON:
                  retcode = spell_smaug( gsn_poison, ch->level, ch, victim );
                  break;
               case ATCK_NASTYPOISON:
                  retcode = spell_smaug( gsn_poison, ch->level, ch, victim );
                  break;
               case ATCK_GAZE:
                  break;
               case ATCK_BLINDNESS:
                  retcode = spell_smaug( skill_lookup( "blindness" ), ch->level, ch, victim );
                  break;
               case ATCK_CAUSESERIOUS:
                  retcode = spell_smaug( skill_lookup( "cause serious" ), ch->level, ch, victim );
                  break;
               case ATCK_EARTHQUAKE:
                  retcode = spell_smaug( skill_lookup( "earthquake" ), ch->level, ch, victim );
                  break;
               case ATCK_CAUSECRITICAL:
                  retcode = spell_smaug( skill_lookup( "cause critical" ), ch->level, ch, victim );
                  break;
               case ATCK_CURSE:
                  retcode = spell_smaug( skill_lookup( "curse" ), ch->level, ch, victim );
                  break;
               case ATCK_FLAMESTRIKE:
                  retcode = spell_smaug( skill_lookup( "flamestrike" ), ch->level, ch, victim );
                  break;
               case ATCK_HARM:
                  retcode = spell_smaug( skill_lookup( "harm" ), ch->level, ch, victim );
                  break;
               case ATCK_FIREBALL:
                  retcode = spell_smaug( skill_lookup( "fireball" ), ch->level, ch, victim );
                  break;
               case ATCK_COLORSPRAY:
                  retcode = spell_smaug( skill_lookup( "colour spray" ), ch->level, ch, victim );
                  break;
               case ATCK_WEAKEN:
                  retcode = spell_smaug( skill_lookup( "weaken" ), ch->level, ch, victim );
                  break;
            }
            if( attacktype != -1 && ( retcode == rCHAR_DIED || char_died( ch ) ) )
               return;
         }
      }

      /*
       * NPC special defense flags           -Thoric
       */
      if( !xIS_EMPTY( ch->defenses ) )
      {
         attacktype = -1;
         if( 50 + ( ch->level / 4 ) > number_percent(  ) )
         {
            cnt = 0;
            for( ;; )
            {
               if( cnt++ > 10 )
               {
                  attacktype = -1;
                  break;
               }
               attacktype = number_range( 2, MAX_DEFENSE_TYPE - 1 );
               if( IS_DEFENSE( ch, attacktype ) )
                  break;
            }

            switch ( attacktype )
            {
               case DFND_CURELIGHT:
                  act( AT_MAGIC, "$n mutters a few incantations...and looks a little better.", ch, NULL, NULL, TO_ROOM );
                  retcode = spell_smaug( skill_lookup( "cure light" ), ch->level, ch, ch );
                  break;
               case DFND_CURESERIOUS:
                  act( AT_MAGIC, "$n mutters a few incantations...and looks a bit better.", ch, NULL, NULL, TO_ROOM );
                  retcode = spell_smaug( skill_lookup( "cure serious" ), ch->level, ch, ch );
                  break;
               case DFND_CURECRITICAL:
                  act( AT_MAGIC, "$n mutters a few incantations...and looks healthier.", ch, NULL, NULL, TO_ROOM );
                  retcode = spell_smaug( skill_lookup( "cure critical" ), ch->level, ch, ch );
                  break;
               case DFND_HEAL:
                  act( AT_MAGIC, "$n mutters a few incantations...and looks much healthier.", ch, NULL, NULL, TO_ROOM );
                  retcode = spell_smaug( skill_lookup( "heal" ), ch->level, ch, ch );
                  break;
               case DFND_DISPELMAGIC:
                  if( victim->first_affect )
                  {
                     act( AT_MAGIC, "$n utters an incantation...", ch, NULL, NULL, TO_ROOM );
                     retcode = spell_dispel_magic( skill_lookup( "dispel magic" ), ch->level, ch, victim );
                  }
                  break;
               case DFND_DISPELEVIL:
                  act( AT_MAGIC, "$n utters an incantation...", ch, NULL, NULL, TO_ROOM );
                  retcode = spell_dispel_evil( skill_lookup( "dispel evil" ), ch->level, ch, victim );
                  break;
               case DFND_SANCTUARY:
                  if( !IS_AFFECTED( ch, AFF_SANCTUARY ) )
                  {
                     act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
                     retcode = spell_smaug( skill_lookup( "sanctuary" ), ch->level, ch, ch );
                  }
                  else
                     retcode = rNONE;
                  break;
            }
            if( attacktype != -1 && ( retcode == rCHAR_DIED || char_died( ch ) ) )
               return;
         }
      }
   }

   /*
    * Fun for the whole family!
    */
   for( rch = ch->in_room->first_person; rch; rch = rch_next )
   {
      rch_next = rch->next_in_room;

      if( IS_AWAKE( rch ) && !rch->fighting )
      {
         /*
          * PC's auto-assist others in their group.
          */
         if( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) || IS_ACT_FLAG( ch, ACT_PET ) )
         {
            if( IS_NPC( rch ) && ( IS_AFFECTED( rch, AFF_CHARM ) || IS_ACT_FLAG( rch, ACT_PET ) ) )
            {
               multi_hit( rch, victim, TYPE_UNDEFINED );
               continue;
            }
            if( is_same_group( ch, rch ) && IS_PLR_FLAG( rch, PLR_AUTOASSIST ) )
            {
               multi_hit( rch, victim, TYPE_UNDEFINED );
               continue;
            }
         }

         /*
          * NPC's assist NPC's of same type or 12.5% chance regardless.
          */
         if( IS_NPC( rch ) && !IS_AFFECTED( rch, AFF_CHARM ) && !IS_ACT_FLAG( rch, ACT_NOASSIST )
             && !IS_ACT_FLAG( rch, ACT_PET ) )
         {
            if( char_died( ch ) )
               break;
            if( rch->pIndexData == ch->pIndexData || number_bits( 3 ) == 0 )
            {
               CHAR_DATA *vch;
               CHAR_DATA *target;
               int number;

               target = NULL;
               number = 0;
               for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
               {
                  if( can_see( rch, vch, FALSE ) && is_same_group( vch, victim ) && number_range( 0, number ) == 0 )
                  {
                     if( vch->mount && vch->mount == rch )
                        target = NULL;
                     else
                     {
                        target = vch;
                        number++;
                     }
                  }
               }

               if( target )
                  multi_hit( rch, target, TYPE_UNDEFINED );
            }
         }
      }
   }

   /*
    * If we are both still here lets get together and do it again some time :) 
    */
   if( ch && victim && victim->position != POS_DEAD && ch->position != POS_DEAD )
      add_event( 2, ev_violence, ch );

   return;
}

/* Replaces area_update */
void ev_area_reset( void *data )
{
   AREA_DATA *area;
   DESCRIPTOR_DATA *d;
   CHAR_DATA *ch;

   area = ( AREA_DATA * ) data;

   if( area->resetmsg && str_cmp( area->resetmsg, "" ) )
   {
      for( d = first_descriptor; d; d = d->next )
      {
         ch = d->original ? d->original : d->character;
         if( !ch )
            continue;
         if( IS_AWAKE( ch ) && ch->in_room && ch->in_room->area == area )
            act( AT_RESET, "$t", ch, area->resetmsg, NULL, TO_CHAR );
      }
   }

   reset_area( area );
   area->last_resettime = current_time;
   add_event( number_range( ( area->reset_frequency * 60 ) / 2, 3 * ( area->reset_frequency * 60 ) / 2 ),
              ev_area_reset, area );
}

/* Replaces auction_update */
void ev_auction( void *data )
{
   ROOM_INDEX_DATA *aucvault;

   if( !auction->item )
      return;

   switch ( ++auction->going )   /* increase the going state */
   {
      case 1: /* going once */
      case 2: /* going twice */
         talk_auction( "%s: going %s for %d.", auction->item->short_descr,
                       ( ( auction->going == 1 ) ? "once" : "twice" ), auction->bet );

         add_event( sysdata.auctionseconds, ev_auction, NULL );
         break;

      case 3: /* SOLD! */
         if( !auction->buyer && auction->bet )
         {
            bug( "Auction code reached SOLD, with NULL buyer, but %d gold bid", auction->bet );
            auction->bet = 0;
         }
         if( auction->bet > 0 && auction->buyer != auction->seller )
         {
            OBJ_DATA *obj = auction->item;

            aucvault = get_room_index( auction->seller->in_room->vnum + 1 );

            talk_auction( "%s sold to %s for %d gold.", auction->item->short_descr,
                          IS_NPC( auction->buyer ) ? auction->buyer->short_descr : auction->buyer->name, auction->bet );

            if( auction->item->buyer ) /* Set final buyer for item - Samson 6-23-99 */
            {
               STRFREE( auction->item->buyer );
               auction->item->buyer = STRALLOC( auction->buyer->name );
            }

            auction->item->bid = auction->bet;  /* Set final bid for item - Samson 6-23-99 */

            /*
             * Stop it from listing on house lists - Samson 11-1-99 
             */
            SET_OBJ_FLAG( auction->item, ITEM_AUCTION );

            /*
             * Reset the 1 year timer on the item - Samson 11-1-99 
             */
            auction->item->day = time_info.day;
            auction->item->month = time_info.month;
            auction->item->year = time_info.year + 1;

            obj_to_room( auction->item, aucvault, auction->seller );
            save_aucvault( auction->seller, auction->seller->short_descr );

            add_sale( auction->seller->short_descr, obj->seller, auction->buyer->name, obj->short_descr, obj->bid, FALSE );

            auction->item = NULL;   /* reset item */
            obj = NULL;
         }
         else  /* not sold */
         {
            aucvault = get_room_index( auction->seller->in_room->vnum + 1 );

            talk_auction( "No bids received for %s - removed from auction.\n\r", auction->item->short_descr );
            talk_auction( "%s has been returned to the auction house.\n\r", auction->item->short_descr );

            obj_to_room( auction->item, aucvault, auction->seller );
            save_aucvault( auction->seller, auction->seller->short_descr );
         }  /* else */
         auction->item = NULL;   /* clear auction */
   }  /* switch */
}

/* Replaces reboot_check from update.c */
void ev_reboot_count( void *data )
{
   if( reboot_counter == -5 )
   {
      echo_to_all( AT_GREEN, "Reboot countdown cancelled.", ECHOTAR_ALL );
      return;
   }

   reboot_counter--;
   if( reboot_counter < 1 )
   {
      echo_to_all( AT_YELLOW, "Reboot countdown completed. System rebooting.", ECHOTAR_ALL );
      log_string( "Rebooted by countdown" );

      mud_down = TRUE;
      return;
   }

   if( reboot_counter > 2 )
      echo_all_printf( AT_YELLOW, ECHOTAR_ALL, "Game reboot in %d minutes.", reboot_counter );
   if( reboot_counter == 2 )
      echo_to_all( AT_YELLOW, "&RGame reboot in 2 minutes.", ECHOTAR_ALL );
   if( reboot_counter == 1 )
      echo_to_all( AT_YELLOW, "}RGame reboot in 1 minute!! Please find somewhere to log off.", ECHOTAR_ALL );

   add_event( 60, ev_reboot_count, NULL );
   return;
}
