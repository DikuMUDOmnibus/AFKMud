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
 *                  Internal server shell command module                    *
 ****************************************************************************/

/* Comment out this define if the child processes throw segfaults */
#define USEGLOB   /* Samson 4-16-98 - For new shell command */

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>   /* Samson 4-16-98 - For new shell command */
#include <fcntl.h>
#include <arpa/telnet.h>

#ifdef USEGLOB /* Samson 4-16-98 - For new command pipe */
#include <glob.h>
#endif

#include "mud.h"
#include "shell.h"
#include "clans.h"

void unlink_command( CMDTYPE * command );
void free_command( CMDTYPE * command );
void skill_notfound( CHAR_DATA * ch, char *argument );
int get_logflag( char *flag );
bool compressEnd( DESCRIPTOR_DATA * d );

/* Global variables - Samson */
bool compilelock = FALSE;  /* Reboot/shutdown commands locked during compiles */
SHELLCMD *first_shellcmd;
SHELLCMD *last_shellcmd;

extern char lastplayercmd[MIL * 2];
extern bool bootlock;

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

#ifndef USEGLOB
/* OLD command shell provided by Ferris - ferris@FootPrints.net Installed by Samson 4-6-98
 * For safety reasons, this is only available if the USEGLOB define is commented out.
 */
/*
 * Local functions.
 */
FILE *popen( const char *command, const char *type );
int pclose( FILE * stream );

char *fgetf( char *s, int n, register FILE * iop )
{
   register int c;
   register char *cs;

   c = '\0';
   cs = s;
   while( --n > 0 && ( c = getc( iop ) ) != EOF )
      if( ( *cs++ = c ) == '\0' )
         break;
   *cs = '\0';
   return ( ( c == EOF && cs == s ) ? NULL : s );
}

/* NOT recommended to be used as a conventional command! */
void command_pipe( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   FILE *fp;

   compressEnd( ch->desc );
   fp = popen( argument, "r" );
   fgetf( buf, MSL, fp );
   pclose( fp );
   pager_printf( ch, "&R%s\n\r", buf );
   return;
}

/* End OLD shell command code */
#endif

/* New command shell code by Thoric - Installed by Samson 4-16-98 */
CMDF do_mudexec( CHAR_DATA * ch, char *argument )
{
   int desc;
   int flags;
#ifndef S_SPLINT_S
   pid_t pid;
#endif

   if( !ch->desc )
      return;

   desc = ch->desc->descriptor;

   set_char_color( AT_PLAIN, ch );

   compressEnd( ch->desc );

   if( ( pid = fork(  ) ) == 0 )
   {
      char *p = argument;
#ifdef USEGLOB
      glob_t g;
#else
      char **argv;
      int argc = 0;
#endif
#ifdef DEBUGGLOB
      int argc = 0;
#endif

      flags = fcntl( desc, F_GETFL, 0 );
      flags &= ~FNDELAY;
      fcntl( desc, F_SETFL, flags );
      dup2( desc, STDIN_FILENO );
      dup2( desc, STDOUT_FILENO );
      dup2( desc, STDERR_FILENO );
      putenv( "TERM=vt100" );
      putenv( "COLUMNS=80" );
      putenv( "LINES=24" );

#ifdef USEGLOB
      g.gl_offs = 1;
      strtok( argument, " " );

      if( ( p = strtok( NULL, " " ) ) != NULL )
         glob( p, GLOB_DOOFFS | GLOB_NOCHECK, NULL, &g );

      if( !g.gl_pathv[g.gl_pathc - 1] )
         g.gl_pathv[g.gl_pathc - 1] = p;

      while( ( p = strtok( NULL, " " ) ) != NULL )
      {
         glob( p, GLOB_DOOFFS | GLOB_NOCHECK | GLOB_APPEND, NULL, &g );
         if( !g.gl_pathv[g.gl_pathc - 1] )
            g.gl_pathv[g.gl_pathc - 1] = p;
      }
      g.gl_pathv[0] = argument;

#ifdef DEBUGGLOB
      for( argc = 0; argc < g.gl_pathc; argc++ )
         printf( "arg %d: %s\n\r", argc, g.gl_pathv[argc] );
      fflush( stdout );
#endif

      execvp( g.gl_pathv[0], g.gl_pathv );
#else
      while( *p )
      {
         while( isspace( *p ) )
            ++p;
         if( *p == '\0' )
            break;
         ++argc;
         while( !isspace( *p ) && *p )
            ++p;
      }
      p = argument;
      argv = calloc( argc + 1, sizeof( char * ) );

      argc = 0;
      argv[argc] = strtok( argument, " " );
      while( ( argv[++argc] = strtok( NULL, " " ) ) != NULL );

      execvp( argv[0], argv );
#endif

      fprintf( stderr, "Shell process: %s failed!\n", argument );
      perror( "mudexec" );
      exit( 0 );
   }
   else if( pid < 2 )
   {
      send_to_char( "Process fork failed.\n\r", ch );
      fprintf( stderr, "%s", "Shell process: fork failed!\n" );
      return;
   }
   else
   {
      ch->desc->process = pid;
      ch->desc->connected = CON_FORKED;
   }
}

/* End NEW shell command code */

/* This function verifies filenames during copy operations - Samson 4-7-98 */
int copy_file( CHAR_DATA * ch, char *filename )
{
   FILE *fp;

   if( !( fp = fopen( filename, "r" ) ) )
   {
      ch_printf( ch, "&RThe file %s does not exist, or cannot be opened. Check your spelling.\n\r", filename );
      return 1;
   }
   FCLOSE( fp );
   return 0;
}

/* The guts of the compiler code, make any changes to the compiler options here - Samson 4-8-98 */
void compile_code( CHAR_DATA * ch, char *argument )
{
   if( !str_cmp( argument, "clean" ) )
   {
      funcf( ch, do_mudexec, "%s", "make -C ../src clean" );
      return;
   }

   if( !str_cmp( argument, "dns" ) )
   {
      funcf( ch, do_mudexec, "%s", "make -C ../src dns" );
      return;
   }
   funcf( ch, do_mudexec, "%s", "make -C ../src" );
   return;
}

/* This command compiles the code on the mud, works only on code port - Samson 4-8-98 */
CMDF do_compile( CHAR_DATA * ch, char *argument )
{
   if( port != CODEPORT )
   {
      send_to_char( "&RThe compiler can only be run on the code port.\n\r", ch );
      return;
   }

   if( bootlock )
   {
      send_to_char( "&RThe reboot timer is running, the compiler cannot be used at this time.\n\r", ch );
      return;
   }

   if( compilelock )
   {
      send_to_char( "&RThe compiler is in use, please wait for the compilation to finish.\n\r", ch );
      return;
   }

   compilelock = TRUE;
   echo_all_printf( AT_RED, ECHOTAR_IMM, "Compiler operation initiated by %s. Reboot and shutdown commands are locked.",
                    ch->name );
   compile_code( ch, argument );
   return;
}

/* This command catches the shortcut "copy" - Samson 4-8-98 */
CMDF do_copy( CHAR_DATA * ch, char *argument )
{
   send_to_char( "&YTo use a copy command, you have to spell it out!\n\r", ch );
   return;
}

/* This command copies Class files from build port to the others - Samson 9-17-98 */
CMDF do_copyclass( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   int valid = 0;
   char *fname, *fname2 = NULL;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use the copyclass command!\n\r", ch );
      return;
   }

   if( port != BUILDPORT )
   {
      send_to_char( "&RThe copyclass command may only be used from the Builders' port.\n\r", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "You must specify a file to copy.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      fname = "*.class";
      fname2 = "skills.dat";
   }
   else if( !str_cmp( argument, "skills" ) )
      fname = "skills.dat";
   else
   {
      fname = argument;
      snprintf( buf, MSL, "%s%s", BUILDCLASSDIR, fname );
      valid = copy_file( ch, buf );
   }

   if( valid != 0 )
   {
      bug( "do_copyclass: Error opening file for copy - %s!", fname );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      if( !sysdata.TESTINGMODE )
      {
         send_to_char( "&RClass and skill files updated to main port.\n\r", ch );
#ifdef USEGLOB
         funcf( ch, do_mudexec, "cp %s%s %s", BUILDCLASSDIR, fname, MAINCLASSDIR );
#else
         funcf( ch, command_pipe, "cp %s%s %s", BUILDCLASSDIR, fname, MAINCLASSDIR );
#endif
         funcf( ch, do_mudexec, "cp %s%s %s", BUILDSYSTEMDIR, fname2, MAINSYSTEMDIR );
      }
      send_to_char( "&GClass and skill files updated to code port.\n\r", ch );

#ifdef USEGLOB
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDCLASSDIR, fname, CODECLASSDIR );
#else
      funcf( ch, command_pipe, "cp %s%s %s", BUILDCLASSDIR, fname, CODECLASSDIR );
#endif
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDSYSTEMDIR, fname2, CODESYSTEMDIR );
      return;
   }

   if( !str_cmp( argument, "skills" ) )
   {
      if( !sysdata.TESTINGMODE )
      {
         send_to_char( "&RSkill file updated to main port.\n\r", ch );
         funcf( ch, do_mudexec, "cp %s%s %s", BUILDSYSTEMDIR, fname, MAINSYSTEMDIR );
      }
      send_to_char( "&GSkill file updated to code port.\n\r", ch );
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDSYSTEMDIR, fname, CODESYSTEMDIR );
      return;
   }

   if( !sysdata.TESTINGMODE )
   {
      ch_printf( ch, "&R%s: file updated to main port.\n\r", argument );
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDCLASSDIR, fname, MAINCLASSDIR );
   }
   ch_printf( ch, "&G%s: file updated to code port.\n\r", argument );
   funcf( ch, do_mudexec, "cp %s%s %s", BUILDCLASSDIR, fname, CODECLASSDIR );
   return;
}

/* This command copies zones from build port to the others - Samson 4-7-98 */
CMDF do_copyzone( CHAR_DATA * ch, char *argument )
{
   int valid = 0;
   char *fname, *fname2 = NULL;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use the copyzone command!\n\r", ch );
      return;
   }

   if( port != BUILDPORT )
   {
      send_to_char( "&RThe copyzone command may only be used from the Builders' port.\n\r", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "You must specify a file to copy.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
      fname = "*.are";
   else
   {
      fname = argument;

      if( !str_cmp( argument, "help.are" ) )
         fname2 = "help.are";
      if( !str_cmp( argument, "gods.are" ) )
         fname2 = "gods.are";
      if( !str_cmp( argument, "bywater.are" ) )
         fname2 = "bywater.are";
      if( !str_cmp( argument, "entry.are" ) )
         fname2 = "entry.are";
      if( !str_cmp( argument, "astral.are" ) )
         fname2 = "astral.are";
      if( !str_cmp( argument, "void.are" ) )
         fname2 = "void.are";
      if( !str_cmp( argument, "alsherok.are" ) )
         fname2 = "alsherok.are";
      if( !str_cmp( argument, "alatia.are" ) )
         fname2 = "alatia.are";
      if( !str_cmp( argument, "eletar.are" ) )
         fname2 = "eletar.are";
      if( !str_cmp( argument, "varsis.are" ) )
         fname2 = "varsis.are";
      if( !str_cmp( argument, "gwyn.are" ) )
         fname2 = "gwyn.are";
      if( !str_cmp( argument, "sindhae.are" ) )
         fname2 = "sindhae.are";

      valid = copy_file( ch, fname );
   }

   if( valid != 0 )
   {
      bug( "do_copyzone: Error opening file for copy - %s!", fname );
      return;
   }

   if( !sysdata.TESTINGMODE )
   {
      send_to_char( "&RArea file(s) updated to main port.\n\r", ch );
#ifdef USEGLOB
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDZONEDIR, fname, MAINZONEDIR );
#else
      funcf( ch, command_pipe, "cp %s%s %s", BUILDZONEDIR, fname, MAINZONEDIR );
#endif
   }

   if( fname2 == "help.are"
       || fname2 == "gods.are"
       || fname2 == "void.are"
       || fname2 == "astral.are"
       || fname2 == "bywater.are"
       || fname2 == "entry.are"
       || fname2 == "alsherok.are"
       || fname2 == "alatia.are"
       || fname2 == "eletar.are" || fname2 == "varsis.are" || fname2 == "gwyn.are" || fname2 == "sindhae.are" )
   {
      send_to_char( "&GArea file(s) updated to code port.\n\r", ch );
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDZONEDIR, fname2, CODEZONEDIR );
   }
   return;
}

/* This command copies maps from build port to the others - Samson 8-2-99 */
CMDF do_copymap( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   int valid = 0;
   char *fname;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use the copymap command!\n\r", ch );
      return;
   }

   if( port != BUILDPORT )
   {
      send_to_char( "&RThe copymap command may only be used from the Builders' port.\n\r", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "You must specify a file to copy.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
      fname = "*.raw";
   else
   {
      fname = argument;
      snprintf( buf, MSL, "%s%s", BUILDMAPDIR, fname );
      valid = copy_file( ch, buf );
   }

   if( valid != 0 )
   {
      bug( "do_copymap: Error opening file for copy - %s!", fname );
      return;
   }

   if( !sysdata.TESTINGMODE )
   {
      send_to_char( "&RMap file(s) updated to main port.\n\r", ch );
#ifdef USEGLOB
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDMAPDIR, fname, MAINMAPDIR );
#else
      funcf( ch, command_pipe, "cp %s%s %s", BUILDMAPDIR, fname, MAINMAPDIR );
#endif

#ifdef USEGLOB
      funcf( ch, do_mudexec, "cp %s*.dat %s", BUILDMAPDIR, MAINMAPDIR );
#else
      funcf( ch, command_pipe, "cp %s*.dat %s", BUILDMAPDIR, MAINMAPDIR );
#endif
   }

   send_to_char( "&GMap file(s) updated to code port.\n\r", ch );
#ifdef USEGLOB
   funcf( ch, do_mudexec, "cp %s%s %s", BUILDMAPDIR, fname, CODEMAPDIR );
#else
   funcf( ch, command_pipe, "cp %s%s %s", BUILDMAPDIR, fname, CODEMAPDIR );
#endif

#ifdef USEGLOB
   funcf( ch, do_mudexec, "cp %s*.dat %s", BUILDMAPDIR, CODEMAPDIR );
#else
   funcf( ch, command_pipe, "cp %s*.dat %s", BUILDMAPDIR, CODEMAPDIR );
#endif

   return;
}

CMDF do_copybits( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use the copybits command!\n\r", ch );
      return;
   }

   if( port != BUILDPORT )
   {
      send_to_char( "&RThe copybits command may only be used from the Builders' port.\n\r", ch );
      return;
   }

   if( !sysdata.TESTINGMODE )
   {
      send_to_char( "&RAbit/Qbit files updated to main port.\n\r", ch );
      funcf( ch, do_mudexec, "cp %sabit.lst %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
      funcf( ch, do_mudexec, "cp %sqbit.lst %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
   }
   send_to_char( "&GAbit/Qbit files updated to code port.\n\r", ch );
   funcf( ch, do_mudexec, "cp %sabit.lst %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
   funcf( ch, do_mudexec, "cp %sqbit.lst %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
   return;
}

/* This command copies the social file from build port to the other ports - Samson 5-2-98 */
CMDF do_copysocial( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use the copysocial command!", ch );
      return;
   }

   if( port != BUILDPORT )
   {
      send_to_char( "&RThe copysocial command may only be used from the Builders' port.\n\r", ch );
      return;
   }

   if( !sysdata.TESTINGMODE )
   {
      /*
       * Build port to Main port 
       */
      send_to_char( "&RSocial file updated to main port.\n\r", ch );
      funcf( ch, do_mudexec, "cp %ssocials.dat %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
   }

   /*
    * Build port to Code port 
    */
   send_to_char( "&GSocial file updated to code port.\n\r", ch );
   funcf( ch, do_mudexec, "cp %ssocials.dat %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
   return;
}

/* This command copies the rune file from build port to the other ports - Samson 5-2-98 */
CMDF do_copyrunes( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use the copyrunes command!", ch );
      return;
   }

   if( port != BUILDPORT )
   {
      send_to_char( "&RThe copyrunes command may only be used from the Builders' port.\n\r", ch );
      return;
   }

   if( !sysdata.TESTINGMODE )
   {
      /*
       * Build port to Main port 
       */
      send_to_char( "&RRune file updated to main port.\n\r", ch );
      funcf( ch, do_mudexec, "cp %srunes.dat %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
   }

   /*
    * Build port to Code port 
    */
   send_to_char( "&GRune file updated to code port.\n\r", ch );
   funcf( ch, do_mudexec, "cp %srunes.dat %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
   return;
}

CMDF do_copyslay( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use the copyslay command!", ch );
      return;
   }

   if( port != BUILDPORT )
   {
      send_to_char( "&RThe copyslay command may only be used from the Builders' port.\n\r", ch );
      return;
   }

   if( !sysdata.TESTINGMODE )
   {
      /*
       * Build port to Main port 
       */
      send_to_char( "&RSlay file updated to main port.\n\r", ch );
      funcf( ch, do_mudexec, "cp %sslay.dat %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
   }

   /*
    * Build port to Code port 
    */
   send_to_char( "&GSlay file updated to code port.\n\r", ch );
   funcf( ch, do_mudexec, "cp %sslay.dat %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
   return;
}

/* This command copies the morphs file from build port to the other ports - Samson 5-2-98 */
CMDF do_copymorph( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use the copymorph command!", ch );
      return;
   }

   if( port != BUILDPORT )
   {
      send_to_char( "&RThe copymorph command may only be used from the Builders' port.\n\r", ch );
      return;
   }

   if( !sysdata.TESTINGMODE )
   {
      /*
       * Build port to Main port 
       */
      send_to_char( "&RPolymorph file updated to main port.\n\r", ch );
      funcf( ch, do_mudexec, "cp %smorph.dat %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
   }

   /*
    * Build port to Code port 
    */
   send_to_char( "&GPolymorph file updated to code port.\n\r", ch );
   funcf( ch, do_mudexec, "cp %smorph.dat %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
   return;
}

/* This command copies the mud binary file from code port to main port and build port - Samson 4-7-98 */
CMDF do_copycode( CHAR_DATA * ch, char *argument )
{
   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use the copycode command!\n\r", ch );
      return;
   }

   if( port != CODEPORT )
   {
      send_to_char( "&RThe copycode command may only be used from the Code Port.\n\r", ch );
      return;
   }

   /*
    * Code port to Builders' port 
    */
   send_to_char( "&GBinary file updated to builder port.\n\r", ch );
   funcf( ch, do_mudexec, "cp -f %s%s %s%s", TESTCODEDIR, BINARYFILE, BUILDCODEDIR, BINARYFILE );

   if( !sysdata.TESTINGMODE )
   {
      send_to_char( "&RBinary file updated to main port.\n\r", ch );
      /*
       * Code port to Main port 
       */
      funcf( ch, do_mudexec, "cp -f %s%s %s%s", TESTCODEDIR, BINARYFILE, MAINCODEDIR, BINARYFILE );
   }

   /*
    * Code port to Builders' port 
    */
   send_to_char( "&GDNS Resolver file updated to builder port.\n\r", ch );
#ifdef WINDOZE
   funcf( ch, do_mudexec, "cp -f %sresolver.exe %sresolver.exe", TESTCODEDIR, BUILDCODEDIR );
#else
   funcf( ch, do_mudexec, "cp -f %sresolver %sresolver", TESTCODEDIR, BUILDCODEDIR );
#endif

   if( !sysdata.TESTINGMODE )
   {
      send_to_char( "&RDNS Resolver file updated to main port.\n\r", ch );
      /*
       * Code port to Main port 
       */
#ifdef WINDOZE
      funcf( ch, do_mudexec, "cp -f %sresolver.exe %sresolver.exe", TESTCODEDIR, MAINCODEDIR );
#else
      funcf( ch, do_mudexec, "cp -f %sresolver %sresolver", TESTCODEDIR, MAINCODEDIR );
#endif
   }
   return;
}

/* This command copies race files from build port to main port and code port - Samson 10-13-98 */
CMDF do_copyrace( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   int valid = 0;
   char *fname;
   char *fnamecheck = NULL;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use the copyrace command!\n\r", ch );
      return;
   }

   if( port != BUILDPORT )
   {
      send_to_char( "&RThe copyrace command may only be used from the Builders' Port.\n\r", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "You must specify a file to copy.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      fname = "*.race";
   }
   else
   {
      fname = argument;
      snprintf( buf, MSL, "%s%s", BUILDRACEDIR, fname );
      valid = copy_file( ch, buf );
   }

   if( valid != 0 )
   {
      bug( "do_copyrace: Error opening file for copy - %s!", fnamecheck );
      return;
   }

   /*
    * Builders' port to Code port 
    */
   send_to_char( "&GRace file(s) updated to code port.\n\r", ch );
#ifdef USEGLOB
   funcf( ch, do_mudexec, "cp %s%s %s", BUILDRACEDIR, fname, CODERACEDIR );
#else
   funcf( ch, command_pipe, "cp %s%s %s", BUILDRACEDIR, fname, CODERACEDIR );
#endif

   if( !sysdata.TESTINGMODE )
   {
      /*
       * Builders' port to Main port 
       */
      send_to_char( "&RRace file(s) updated to main port.\n\r", ch );
#ifdef USEGLOB
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDRACEDIR, fname, MAINRACEDIR );
#else
      funcf( ch, command_pipe, "cp %s%s %s", BUILDRACEDIR, fname, MAINRACEDIR );
#endif
   }
   return;
}

/* This command copies deity files from build port to main port and code port - Samson 10-13-98 */
CMDF do_copydeity( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];
   int valid = 0;
   char *fname;

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mobs cannot use the copydeity command!\n\r", ch );
      return;
   }

   if( port != BUILDPORT )
   {
      send_to_char( "&RThe copydeity command may only be used from the Builders' Port.\n\r", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "You must specify a file to copy.\n\r", ch );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      fname = "*";
   }
   else
   {
      fname = argument;
      snprintf( buf, MSL, "%s%s", BUILDDEITYDIR, fname );
      valid = copy_file( ch, buf );
   }

   if( valid != 0 )
   {
      bug( "do_copydeity: Error opening file for copy - %s!", fname );
      return;
   }

   /*
    * Builders' port to Code port 
    */
   send_to_char( "&GDeity file(s) updated to code port.\n\r", ch );
#ifdef USEGLOB
   funcf( ch, do_mudexec, "cp %s%s %s", BUILDDEITYDIR, fname, CODEDEITYDIR );
#else
   funcf( ch, command_pipe, "cp %s%s %s", BUILDDEITYDIR, fname, CODEDEITYDIR );
#endif

   if( !sysdata.TESTINGMODE )
   {
      /*
       * Builders' port to Main port 
       */
      send_to_char( "&RDeity file(s) updated to main port.\n\r", ch );
#ifdef USEGLOB
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDDEITYDIR, fname, MAINDEITYDIR );
#else
      funcf( ch, command_pipe, "cp %s%s %s", BUILDDEITYDIR, fname, MAINDEITYDIR );
#endif
   }
   return;
}

/*
====================
GREP In-Game command	-Nopey
====================
*/
/* Modified by Samson to be a bit less restrictive. So one can grep anywhere the account will allow. */
CMDF do_grep( CHAR_DATA * ch, char *argument )
{
   char buf[MSL];

   set_char_color( AT_PLAIN, ch );

   if( !argument || argument[0] == '\0' )
      mudstrlcpy( buf, "grep --help", MSL ); /* Will cause it to forward grep's help options to you */
   else
      snprintf( buf, MSL, "grep -n %s", argument );   /* Line numbers are somewhat important */

#ifdef USEGLOB
   do_mudexec( ch, buf );
#else
   command_pipe( ch, buf );
#endif
   return;
}

/* IP/DNS resolver - passes to the shell code now - Samson 4-23-00 */
CMDF do_resolve( CHAR_DATA * ch, char *argument )
{
   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "Resolve what?\n\r", ch );
      return;
   }
   funcf( ch, do_mudexec, "host %s", argument );
}

void free_shellcommand( SHELLCMD * command )
{
   DISPOSE( command->name );
   DISPOSE( command->fun_name );
   DISPOSE( command );
}

void free_shellcommands( void )
{
   SHELLCMD *scommand, *scmd_next;

   for( scommand = first_shellcmd; scommand; scommand = scmd_next )
   {
      scmd_next = scommand->next;
      free_shellcommand( scommand );
   }
   return;
}

void add_shellcommand( SHELLCMD * command )
{
   int x;
   SHELLCMD *sprev;
   CMDTYPE *cmd;

   if( !command )
   {
      bug( "%s", "Add_shellcommand: NULL command" );
      return;
   }

   if( !command->name )
   {
      bug( "%s", "Add_shellcommand: NULL command->name" );
      return;
   }

   if( !command->do_fun )
   {
      bug( "Add_shellcommand: NULL command->do_fun for command %s", command->name );
      return;
   }

   /*
    * make sure the name is all lowercase 
    */
   for( x = 0; command->name[x] != '\0'; x++ )
      command->name[x] = LOWER( command->name[x] );

   for( sprev = first_shellcmd; sprev; sprev = sprev->next )
      if( strcasecmp( sprev->name, command->name ) >= 0 )
         break;

   if( !sprev )
      LINK( command, first_shellcmd, last_shellcmd, next, prev );
   else
      INSERT( command, sprev, first_shellcmd, next, prev );

   /*
    * Kick it out of the main command table if it's there 
    */
   if( ( cmd = find_command( command->name ) ) != NULL )
   {
      log_printf( "Removing command: %s and replacing in shell command table.", cmd->name );
      unlink_command( cmd );
      free_command( cmd );
   }
   return;
}

SHELLCMD *find_shellcommand( char *command )
{
   SHELLCMD *cmd;

   for( cmd = first_shellcmd; cmd; cmd = cmd->next )
      if( !str_prefix( command, cmd->name ) )
         return cmd;

   return NULL;
}

void fread_shellcommand( FILE * fp, int version )
{
   const char *word;
   bool fMatch;
   SHELLCMD *command;

   CREATE( command, SHELLCMD, 1 );
   command->flags = 0;  /* Default to no flags set */

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = FALSE;

      switch ( UPPER( word[0] ) )
      {
         case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;

         case 'C':
            KEY( "Code", command->fun_name, str_dup( fread_word( fp ) ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !command->name )
               {
                  bug( "%s", "Fread_shellcommand: Name not found" );
                  free_shellcommand( command );
                  return;
               }
               if( !command->fun_name )
               {
                  bug( "fread_shellcommand: No function name supplied for %s", command->name );
                  free_shellcommand( command );
                  return;
               }
               /*
                * Mods by Trax
                * Fread in code into char* and try linkage here then
                * deal in the "usual" way I suppose..
                */
               command->do_fun = skill_function( command->fun_name );
               if( command->do_fun == skill_notfound )
               {
                  bug( "Fread_shellcommand: Function %s not found for %s", command->fun_name, command->name );
                  free_shellcommand( command );
                  return;
               }
               add_shellcommand( command );
               /*
                * Automatically approve all immortal commands for use as a ghost 
                */
               if( command->level >= LEVEL_IMMORTAL )
                  SET_BIT( command->flags, CMD_GHOST );
               return;
            }
            break;

         case 'F':
            if( !str_cmp( word, "flags" ) )
            {
               if( version < 3 )
               {
                  command->flags = fread_number( fp );
                  fMatch = TRUE;
                  break;
               }
               else
               {
                  char *cmdflags = NULL;
                  char flag[MIL];
                  int value;

                  cmdflags = fread_flagstring( fp );

                  while( cmdflags[0] != '\0' )
                  {
                     cmdflags = one_argument( cmdflags, flag );
                     value = get_cmdflag( flag );
                     if( value < 0 || value >= 32 )
                        bug( "Unknown command flag: %s\n\r", flag );
                     else
                        SET_BIT( command->flags, 1 << value );
                  }
                  fMatch = TRUE;
                  break;
               }
            }
            break;

         case 'L':
            KEY( "Level", command->level, fread_number( fp ) );
            if( !str_cmp( word, "Log" ) )
            {
               if( version < 2 )
               {
                  command->log = fread_number( fp );
                  fMatch = TRUE;
                  break;
               }
               else
               {
                  char *lflag = NULL;
                  int lognum;

                  lflag = fread_flagstring( fp );
                  lognum = get_logflag( lflag );

                  if( lognum < 0 || lognum > LOG_ALL )
                  {
                     bug( "fread_shellcommand: Command %s has invalid log flag! Defaulting to normal.", command->name );
                     lognum = LOG_NORMAL;
                  }
                  command->log = lognum;
                  fMatch = TRUE;
                  break;
               }
            }
            break;

         case 'N':
            KEY( "Name", command->name, fread_string_nohash( fp ) );
            break;

         case 'P':
            if( !str_cmp( word, "Position" ) )
            {
               char *tpos = NULL;
               int position;

               tpos = fread_flagstring( fp );
               position = get_npc_position( tpos );

               if( position < 0 || position >= POS_MAX )
               {
                  bug( "fread_shellcommand: Command %s has invalid position! Defaulting to standing.", command->name );
                  position = POS_STANDING;
               }
               command->position = position;
               fMatch = TRUE;
               break;
            }
            break;
      }
      if( !fMatch )
         bug( "Fread_shellcommand: no match: %s", word );
   }
}

void load_shellcommands( void )
{
   FILE *fp;
   int version = 0;

   first_shellcmd = NULL;
   last_shellcmd = NULL;

   if( ( fp = fopen( SHELL_COMMAND_FILE, "r" ) ) != NULL )
   {
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "%s", "Load_shell_commands: # not found." );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "VERSION" ) )
         {
            version = fread_number( fp );
            continue;
         }
         if( !str_cmp( word, "COMMAND" ) )
         {
            fread_shellcommand( fp, version );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "Load_shell_commands: bad section: %s", word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   else
   {
      bug( "Cannot open %s", SHELL_COMMAND_FILE );
      exit( 1 );
   }
}

#define SHELLCMDVERSION 3
/* Updated to 1 for command position - Samson 4-26-00 */
/* Updated to 2 for log flag - Samson 4-26-00 */
/* Updated to 3 for command flags - Samson 7-9-00 */
void save_shellcommands( void )
{
   FILE *fpout;
   SHELLCMD *command;

   if( !( fpout = fopen( SHELL_COMMAND_FILE, "w" ) ) )
   {
      bug( "Cannot open %s for writing", SHELL_COMMAND_FILE );
      perror( SHELL_COMMAND_FILE );
      return;
   }

   fprintf( fpout, "#VERSION	%d\n", SHELLCMDVERSION );

   for( command = first_shellcmd; command; command = command->next )
   {
      if( !command->name || command->name[0] == '\0' )
      {
         bug( "%s", "Save_shellcommands: blank command in list" );
         continue;
      }
      fprintf( fpout, "%s", "#COMMAND\n" );
      fprintf( fpout, "Name        %s~\n", command->name );
      /*
       * Modded to use new field - Trax 
       */
      fprintf( fpout, "Code        %s\n", command->fun_name ? command->fun_name : "" );
      fprintf( fpout, "Position    %s~\n", npc_position[command->position] );
      fprintf( fpout, "Level       %d\n", command->level );
      fprintf( fpout, "Log         %s~\n", log_flag[command->log] );
      if( command->flags )
         fprintf( fpout, "Flags       %s~\n", flag_string( command->flags, cmd_flags ) );
      fprintf( fpout, "%s", "End\n\n" );
   }
   fprintf( fpout, "%s", "#END\n" );
   FCLOSE( fpout );
}

void shellcommands( CHAR_DATA * ch, int curr_lvl )
{
   SHELLCMD *cmd;
   int col;

   col = 1;
   send_to_pager( "\n\r", ch );
   for( cmd = first_shellcmd; cmd; cmd = cmd->next )
      if( cmd->level == curr_lvl )
      {
         pager_printf( ch, "%-12s", cmd->name );
         if( ++col % 6 == 0 )
            send_to_pager( "\n\r", ch );
      }
   if( col % 6 != 0 )
      send_to_pager( "\n\r", ch );
   return;
}

/* Clone of do_cedit modified because shell commands don't use a hash table */
CMDF do_shelledit( CHAR_DATA * ch, char *argument )
{
   SHELLCMD *command;
   char arg1[MIL], arg2[MIL];

   set_char_color( AT_IMMORT, ch );

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !arg1 || arg1[0] == '\0' )
   {
      send_to_char( "Syntax: shelledit save\n\r", ch );
      send_to_char( "Syntax: shelledit <command> create [code]\n\r", ch );
      send_to_char( "Syntax: shelledit <command> delete\n\r", ch );
      send_to_char( "Syntax: shelledit <command> show\n\r", ch );
      send_to_char( "Syntax: shelledit <command> [field]\n\r", ch );
      send_to_char( "\n\rField being one of:\n\r", ch );
      send_to_char( "  level position log code flags\n\r", ch );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      save_shellcommands(  );
      send_to_char( "Shell commands saved.\n\r", ch );
      return;
   }

   command = find_shellcommand( arg1 );
   if( !str_cmp( arg2, "create" ) )
   {
      if( command )
      {
         send_to_char( "That command already exists!\n\r", ch );
         return;
      }
      CREATE( command, SHELLCMD, 1 );
      command->name = str_dup( arg1 );
      command->level = get_trust( ch );
      if( *argument )
         one_argument( argument, arg2 );
      else
         snprintf( arg2, MIL, "do_%s", arg1 );
      command->do_fun = skill_function( arg2 );
      command->fun_name = str_dup( arg2 );
      add_shellcommand( command );
      send_to_char( "Shell command added.\n\r", ch );
      if( command->do_fun == skill_notfound )
         ch_printf( ch, "Code %s not found. Set to no code.\n\r", arg2 );
      return;
   }

   if( !command )
   {
      send_to_char( "Shell command not found.\n\r", ch );
      return;
   }
   else if( command->level > get_trust( ch ) )
   {
      send_to_char( "You cannot touch this command.\n\r", ch );
      return;
   }

   if( arg2[0] == '\0' || !str_cmp( arg2, "show" ) )
   {
      ch_printf( ch,
                 "Command:   %s\n\rLevel:     %d\n\rPosition:  %s\n\rLog:       %s\n\rFunc Name: %s\n\rFlags:     %s\n\r",
                 command->name, command->level, npc_position[command->position], log_flag[command->log], command->fun_name,
                 flag_string( command->flags, cmd_flags ) );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      free_shellcommand( command );
      send_to_char( "Shell command deleted.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "code" ) )
   {
      DO_FUN *fun = skill_function( argument );

      if( fun == skill_notfound )
      {
         send_to_char( "Code not found.\n\r", ch );
         return;
      }
      command->do_fun = fun;
      DISPOSE( command->fun_name );
      command->fun_name = str_dup( argument );
      send_to_char( "Shell command code updated.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      int level = atoi( argument );

      if( level < 0 || level > get_trust( ch ) )
      {
         send_to_char( "Level out of range.\n\r", ch );
         return;
      }
      command->level = level;
      send_to_char( "Command level updated.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "log" ) )
   {
      int clog = get_logflag( argument );

      if( clog < 0 || clog > LOG_ALL )
      {
         send_to_char( "Log out of range.\n\r", ch );
         return;
      }
      command->log = clog;
      send_to_char( "Command log setting updated.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "position" ) )
   {
      int pos;

      pos = get_npc_position( argument );

      if( pos < 0 || pos > POS_MAX )
      {
         send_to_char( "Invalid position.\n\r", ch );
         return;
      }
      command->position = pos;
      send_to_char( "Command position updated.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      int flag;
      if( is_number( argument ) )
         flag = atoi( argument );
      else
         flag = get_cmdflag( argument );
      if( flag < 0 || flag >= 32 )
      {
         if( is_number( argument ) )
            send_to_char( "Invalid flag: range is from 0 to 31.\n\r", ch );
         else
            ch_printf( ch, "Unknown flag %s.\n", argument );
         return;
      }

      TOGGLE_BIT( command->flags, 1 << flag );
      send_to_char( "Command flags updated.\n\r", ch );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      one_argument( argument, arg1 );

      if( !arg1 || arg1[0] == '\0' )
      {
         send_to_char( "Cannot clear name field!\n\r", ch );
         return;
      }
      DISPOSE( command->name );
      command->name = str_dup( arg1 );
      send_to_char( "Done.\n\r", ch );
      return;
   }
   /*
    * display usage message 
    */
   do_shelledit( ch, "" );
}

bool shell_hook( CHAR_DATA * ch, char *command, char *argument )
{
   SHELLCMD *cmd;
   char logline[MIL];
   bool found = FALSE;
   int trust, loglvl;

   trust = get_trust( ch );

   for( cmd = first_shellcmd; cmd; cmd = cmd->next )
      if( !str_prefix( command, cmd->name )
          && ( cmd->level <= trust
               || ( !IS_NPC( ch ) && ch->pcdata->council && is_name( cmd->name, ch->pcdata->council->powers )
                    && cmd->level <= ( trust + MAX_CPD ) ) || ( !IS_NPC( ch ) && ch->pcdata->bestowments
                                                                && ch->pcdata->bestowments[0] != '\0'
                                                                && hasname( ch->pcdata->bestowments, cmd->name )
                                                                && cmd->level <= ( trust + sysdata.bestow_dif ) ) ) )
      {
         found = TRUE;
         break;
      }

   if( !found )
      return FALSE;

   snprintf( logline, MIL, "%s %s", command, argument );

   /*
    * Log and snoop.
    */
   snprintf( lastplayercmd, MIL * 2, "%s used command: %s", ch->name, logline );

   if( cmd->log == LOG_NEVER )
      mudstrlcpy( logline, "XXXXXXXX XXXXXXXX XXXXXXXX", MIL );

   loglvl = cmd->log;

   if( !IS_NPC( ch ) && IS_CMD_FLAG( cmd, CMD_MUDPROG ) )
   {
      send_to_char( "Huh?\n\r", ch );
      return FALSE;
   }

   if( IS_PLR_FLAG( ch, PLR_LOG ) || fLogAll || loglvl == LOG_BUILD || loglvl == LOG_HIGH || loglvl == LOG_ALWAYS )
   {
      char lbuf[MSL];

      /*
       * Added by Narn to show who is switched into a mob that executes
       * a logged command.  Check for descriptor in case force is used. 
       */
      if( ch->desc && ch->desc->original )
         snprintf( lbuf, MSL, "Log %s (%s): %s", ch->name, ch->desc->original->name, logline );
      else
         snprintf( lbuf, MSL, "Log %s: %s", ch->name, logline );

      /*
       * Make it so a 'log all' will send most output to the log
       * file only, and not spam the log channel to death   -Thoric
       */
      if( fLogAll && loglvl == LOG_NORMAL && IS_PLR_FLAG( ch, PLR_LOG ) )
         loglvl = LOG_ALL;
      log_string_plus( lbuf, loglvl, ch->level );
   }

   if( ch->desc && ch->desc->snoop_by )
   {
      buffer_printf( ch->desc->snoop_by, "%s", ch->name );
      write_to_buffer( ch->desc->snoop_by, "% ", 2 );
      write_to_buffer( ch->desc->snoop_by, logline, 0 );
      write_to_buffer( ch->desc->snoop_by, "\n\r", 2 );
   }

   ( *cmd->do_fun ) ( ch, argument );
   return TRUE;
}
