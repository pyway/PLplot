/* $Id$
 * $Log$
 * Revision 1.2  1993/07/16 22:09:12  mjl
 * Made keypress handler a bit easier to invoke.  Eliminated low-level
 * aspect and orientation handling.  Some name changes.  Now uses standard
 * meta coordinate system.
 *
 * Revision 1.1  1993/07/02  06:58:37  mjl
 * The new TCL/TK driver!  Yes it's finally here!  YAAAAAAAAYYYYYYY!!!
 *
*/

/*	tk.c
*
*	Maurice LeBrun
*	30-Apr-93
*
*	PLPLOT TCL/TK device driver.
*
*	Passes graphics commands to renderer and certain X
*	events back to user if requested.
*/

#ifdef TK

#include "plserver.h"
#include "drivers.h"
#include "metadefs.h"
#include "pdf.h"
#include "plevent.h"

#define BUFMAX 2048

#define tk_wr(code) \
if (code) { abort_session(pls, "Unable to write to pipe"); }

/* Use vfork() if the system supports it */

#ifndef FORK
#define FORK fork
#endif

/* INDENT OFF */
/*----------------------------------------------------------------------*/
/* Struct to hold device-specific info. */

typedef struct {
    Tk_Window w;		/* Main window */
    Tcl_Interp *interp;		/* Interpreter */

    FILE  *file;		/* fifo or socket file descriptor */
    char  *filename;		/* Name of fifo or socket */
    char  *filetype;		/* Set to "fifo" or "socket" */

    char  *program;		/* Name of client main window */

    short xold, yold;		/* Coordinates of last point plotted */
    int   exit_eventloop;	/* Flag for breaking out of event loop */
    int   pass_thru;		/* Skips normal error termination when set */
    int   launched_server;	/* Keep track of who started server */
} TkDev;

/* Function prototypes */

static void  tk_start		(PLStream *);
static void  tk_stop		(PLStream *);
static void  WaitForPage	(PLStream *);
static void  HandleEvents	(PLStream *);
static void  tk_configure	(PLStream *);
static void  launch_server	(PLStream *);
static void  flush_output	(PLStream *);
static void  plwindow_init	(PLStream *);
static void  link_init		(PLStream *);

/* Tcl/TK utility commands */

static void  tk_wait		(PLStream *, char *);
static void  abort_session	(PLStream *, char *);
static void  server_cmd		(PLStream *, char *);
static void  tcl_cmd		(PLStream *, char *);
static int   tcl_eval		(PLStream *, char *);

/* These are internal TCL commands */

static int   Abort		(ClientData, Tcl_Interp *, int, char **);
static int   KeyEH		(ClientData, Tcl_Interp *, int, char **);

/* INDENT ON */
/*----------------------------------------------------------------------*\
* plD_init_tk()
*
* Initialize device.
* TK-dependent stuff done in tk_start().  You can set the display by
* calling plsfnam() with the display name as the (string) argument.
\*----------------------------------------------------------------------*/

void
plD_init_tk(PLStream *pls)
{
    U_CHAR c = (U_CHAR) INITIALIZE;
    TkDev *dev;
    int xmin = 0;
    int xmax = PIXELS_X - 1;
    int ymin = 0;
    int ymax = PIXELS_Y - 1;

    float pxlx = (double) PIXELS_X / (double) LPAGE_X;
    float pxly = (double) PIXELS_Y / (double) LPAGE_Y;

    dbug_enter("plD_init_tk");

    pls->termin = 1;		/* is an interactive terminal */
    pls->icol0 = 1;
    pls->width = 1;
    pls->bytecnt = 0;
    pls->page = 0;

    if (pls->bufmax == 0)
	pls->bufmax = BUFMAX;

/* Allocate and initialize device-specific data */

    pls->dev = calloc(1, (size_t) sizeof(TkDev));
    if (pls->dev == NULL)
	plexit("plD_init_tk: Out of memory.");

    dev = (TkDev *) pls->dev;

/* Start interpreter and spawn server process */

    tk_start(pls);

/* Get ready for plotting */

    dev->xold = UNDEFINED;
    dev->yold = UNDEFINED;

    plP_setpxl(pxlx, pxly);
    plP_setphy(xmin, xmax, ymin, ymax);

/* Send init info */

    tk_wr(pdf_wr_1byte(dev->file, c));
    pls->bytecnt++;

/* The header and version fields will be useful when the client & server */
/* reside on different machines */

    tk_wr(pdf_wr_header(dev->file, PLSERV_HEADER));
    tk_wr(pdf_wr_header(dev->file, PLSERV_VERSION));

    tk_wr(pdf_wr_header(dev->file, "xmin"));
    tk_wr(pdf_wr_2bytes(dev->file, (U_SHORT) xmin));

    tk_wr(pdf_wr_header(dev->file, "xmax"));
    tk_wr(pdf_wr_2bytes(dev->file, (U_SHORT) xmax));

    tk_wr(pdf_wr_header(dev->file, "ymin"));
    tk_wr(pdf_wr_2bytes(dev->file, (U_SHORT) ymin));

    tk_wr(pdf_wr_header(dev->file, "ymax"));
    tk_wr(pdf_wr_2bytes(dev->file, (U_SHORT) ymax));

    tk_wr(pdf_wr_header(dev->file, ""));

/* Flush pipe since number of bytes not accounted for */

    flush_output(pls);
}

/*----------------------------------------------------------------------*\
* plD_line_tk()
*
* Draw a line in the current color from (x1,y1) to (x2,y2).
\*----------------------------------------------------------------------*/

void
plD_line_tk(PLStream *pls, short x1, short y1, short x2, short y2)
{
    U_CHAR c;
    U_SHORT xy[4];
    static long count = 0, max_count = 100;
    TkDev *dev = (TkDev *) pls->dev;

    if ( (++count/max_count)*max_count == count) {
	count = 0;
	HandleEvents(pls);	/* Check for events */
    }

    if (x1 == dev->xold && y1 == dev->yold) {
	c = (U_CHAR) LINETO;
	tk_wr(pdf_wr_1byte(dev->file, c));

	xy[0] = x2;
	xy[1] = y2;
	tk_wr(pdf_wr_2nbytes(dev->file, xy, 2));

	pls->bytecnt += 5;
    }
    else {
	c = (U_CHAR) LINE;
	tk_wr(pdf_wr_1byte(dev->file, c));

	xy[0] = x1;
	xy[1] = y1;
	xy[2] = x2;
	xy[3] = y2;
	tk_wr(pdf_wr_2nbytes(dev->file, xy, 4));

	pls->bytecnt += 9;
    }
    dev->xold = x2;
    dev->yold = y2;
}

/*----------------------------------------------------------------------*\
* plD_polyline_tk()
*
* Draw a polyline in the current color from (x1,y1) to (x2,y2).
\*----------------------------------------------------------------------*/

void
plD_polyline_tk(PLStream *pls, short *xa, short *ya, PLINT npts)
{
    U_CHAR c = (U_CHAR) POLYLINE;
    static long count = 0, max_count = 100;
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("plD_polyline_tk");

    if ( (++count/max_count)*max_count == count) {
	count = 0;
	HandleEvents(pls);	/* Check for events */
    }

    tk_wr(pdf_wr_1byte(dev->file, c));
    tk_wr(pdf_wr_2bytes(dev->file, (U_SHORT) npts));
    tk_wr(pdf_wr_2nbytes(dev->file, (U_SHORT *) xa, npts));
    tk_wr(pdf_wr_2nbytes(dev->file, (U_SHORT *) ya, npts));
    pls->bytecnt += (3 + 4 * npts);

    dev->xold = xa[npts - 1];
    dev->yold = ya[npts - 1];
}

/*----------------------------------------------------------------------*\
* plD_eop_tk()
*
* End of page.  
* User must hit <RETURN> or click right mouse button to continue.
\*----------------------------------------------------------------------*/

void
plD_eop_tk(PLStream *pls)
{
    U_CHAR c = (U_CHAR) EOP;
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("plD_eop_tk");

    if (pls->nopause)
	return;

    tk_wr(pdf_wr_1byte(dev->file, c));
    pls->bytecnt += 1;
    WaitForPage(pls);
}

/*----------------------------------------------------------------------*\
* plD_bop_tk()
*
* Set up for the next page.
\*----------------------------------------------------------------------*/

void
plD_bop_tk(PLStream *pls)
{
    U_CHAR c = (U_CHAR) BOP;
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("plD_bop_tk");

    dev->xold = UNDEFINED;
    dev->yold = UNDEFINED;
    pls->page++;
    tk_wr(pdf_wr_1byte(dev->file, c));
    pls->bytecnt += 1;
}

/*----------------------------------------------------------------------*\
* plD_tidy_tk()
*
* Close graphics file
\*----------------------------------------------------------------------*/

void
plD_tidy_tk(PLStream *pls)
{
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("plD_tidy_tk");

    tk_stop(pls);
    pls->fileset = 0;
    pls->page = 0;
    free((void *) pls->dev);
}

/*----------------------------------------------------------------------*\
* plD_color_tk()
*
* Set pen color.
\*----------------------------------------------------------------------*/

void
plD_color_tk(PLStream *pls)
{
    U_CHAR c = (U_CHAR) NEW_COLOR;
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("plD_color_tk");

    tk_wr(pdf_wr_1byte(dev->file, c));
    tk_wr(pdf_wr_1byte(dev->file, (U_CHAR) pls->icol0));
    pls->bytecnt += 2;

    if (pls->icol0 == PL_RGB_COLOR) {
	tk_wr(pdf_wr_1byte(dev->file, pls->curcolor.r));
	tk_wr(pdf_wr_1byte(dev->file, pls->curcolor.g));
	tk_wr(pdf_wr_1byte(dev->file, pls->curcolor.b));
	pls->bytecnt += 3;
    }
}

/*----------------------------------------------------------------------*\
* plD_text_tk()
*
* Switch to text mode.
\*----------------------------------------------------------------------*/

void
plD_text_tk(PLStream *pls)
{
    dbug_enter("plD_text_tk");

    HandleEvents(pls);	/* Check for events */
}

/*----------------------------------------------------------------------*\
* plD_graph_tk()
*
* Switch to graphics mode.
\*----------------------------------------------------------------------*/

void
plD_graph_tk(PLStream *pls)
{
    dbug_enter("plD_graph_tk");

    HandleEvents(pls);	/* Check for events */
}

/*----------------------------------------------------------------------*\
* plD_width_tk()
*
* Set pen width.
\*----------------------------------------------------------------------*/

void
plD_width_tk(PLStream *pls)
{
    U_CHAR c = (U_CHAR) NEW_WIDTH;
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("plD_width_tk");

    tk_wr(pdf_wr_1byte(dev->file, c));
    tk_wr(pdf_wr_2bytes(dev->file, (U_SHORT) (pls->width)));
    pls->bytecnt += 3;
}

/*----------------------------------------------------------------------*\
* plD_esc_tk()
*
* Escape function.
\*----------------------------------------------------------------------*/

void
plD_esc_tk(PLStream *pls, PLINT op, void *ptr)
{
    dbug_enter("plD_esc_tk");
}

/*----------------------------------------------------------------------*\
* tk_start
*
* Create TCL interpreter and spawn off server process.
* Each stream that uses the tk driver gets its own interpreter.
\*----------------------------------------------------------------------*/

static void
tk_start(PLStream *pls)
{
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("tk_start");

/* Instantiate a TCL interpreter. */

    dev->interp = Tcl_CreateInterp();

/* Initialize top level window */
/* Request pls->program (if set) for the main window name */

    if (pls->program == NULL)
	pls->program = "plclient";

    if (tk_toplevel(&dev->w, dev->interp, pls->FileName, pls->program, NOMAP))
	abort_session(pls, "Unable to create top-level window");

/* Initialize stuff known to interpreter */

    tk_configure(pls);

/* Launch server process if necessary */

    launch_server(pls);

/* Initialize widgets */

    plwindow_init(pls);

/* Initialize data link */

    link_init(pls);

    server_cmd( pls, "update" );

    return;
}

/*----------------------------------------------------------------------*\
* tk_stop
*
* Normal termination & cleanup.
\*----------------------------------------------------------------------*/

static void
tk_stop(PLStream *pls)
{
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("tk_stop");

/* Safety check for out of control code */

    if (dev->pass_thru)
	return;

    dev->pass_thru = 1;

/* Close fifo if it exists */

    if (dev->file != NULL) {
	if (fclose(dev->file)) {
	    fprintf(stderr, "%s: Error closing fifo\n", dev->program);
	}
	dev->file = NULL;
    }

/* If we launched server, blow it away */
/* First unset its client variable so it won't try communicating */

    if (tcl_eval(pls, "winfo $plserver exists")) {
	server_cmd( pls, "$plw_end $plwindow" );
	if (dev->launched_server) {
	    server_cmd( pls, "after 1 destroy ." );
	}
    }

/* Blow away main window */

    tcl_cmd(pls, "destroy .");

/* Blow away interpreter if it exists */

    if (dev->interp != NULL) {
	Tcl_DeleteInterp(dev->interp);
	dev->interp = NULL;
    }
}

/*----------------------------------------------------------------------*\
* abort_session
*
* Terminates with an error.  
* Cleanup is done by plD_tidy_tk(), called by plexit().
\*----------------------------------------------------------------------*/

static void
abort_session(PLStream *pls, char *msg)
{
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("abort_session");

/* Safety check for out of control code */

    if (dev->pass_thru)
	return;

    dev->pass_thru = 1;

    pls->nopause = TRUE;
    plexit(msg);
}

/*----------------------------------------------------------------------*\
* tk_configure
*
* Does global variable & command initialization, mostly for interpreter.
\*----------------------------------------------------------------------*/

static void
tk_configure(PLStream *pls)
{
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("tk_configure");

/* Use main window name as program name, now that we have it */

    dev->program = Tk_Name(dev->w);
    Tcl_SetVar(dev->interp, "client", dev->program, 0);

/* Tell interpreter about commands. */

    Tcl_CreateCommand(dev->interp, "abort", Abort,
		      (ClientData) pls, (void (*)()) NULL);

    Tcl_CreateCommand(dev->interp, "keypress", KeyEH,
		      (ClientData) pls, (void (*)()) NULL);

/* Set default names for server widget procs */

    Tcl_SetVar(dev->interp, "plserver_init",   "plserver_init", 0);
    Tcl_SetVar(dev->interp, "plw_create", "plw_create", 0);
    Tcl_SetVar(dev->interp, "plw_init",   "plw_init", 0);
    Tcl_SetVar(dev->interp, "plw_flash",  "plw_flash", 0);
    Tcl_SetVar(dev->interp, "plw_end",    "plw_end", 0);

/* Eval user-specified TCL command -- can be used to modify defaults */

    if (pls->tcl_cmd != NULL)
	tcl_cmd(pls, pls->tcl_cmd);
}

/*----------------------------------------------------------------------*\
* launch_server
*
* Starts server process if necessary.
* There are three cases:
*
* pls->plserver == NULL		plserver is started
*
* pls->plserver != NULL
*   && plserver already exists	No action is taken
*
* pls->plserver != NULL
*   && plserver doesn't exist	The specified server is created
*
* In the second case there is no work to be done so we just return.
*
* In the third case, the client has specified which server to start.
* This additional flexibility may come in handy but usually the default
* server will suffice (since the user may customize its init file).
\*----------------------------------------------------------------------*/

static void
launch_server(PLStream *pls)
{
    TkDev *dev = (TkDev *) pls->dev;
    char **argv;
    int i;
    pid_t pid;

    dbug_enter("launch_server");

#ifdef DEBUG
    fprintf(stderr, "%s -- PID: %d, PGID: %d, PPID: %d\n",
	    dev->program, getpid(), getpgrp(), getppid());
#endif

/* Check for already existing server */

    if (pls->plserver != NULL) {
	Tcl_SetVar(dev->interp, "plserver", pls->plserver, 0);
	if (tcl_eval(pls, "winfo $plserver exists")) {
	    return;
	}
	Tcl_UnsetVar(dev->interp, "plserver", 0);
    }
    else {
	pls->plserver = "plserver";
    }

/* Build argument list for exec */

    i = 0;
    argv = (char **) malloc(20 * sizeof(char *));

    argv[i++] = pls->plserver;		/* Name of server */

    argv[i++] = "-client";		/* Send back notification */
    argv[i++] = dev->program;

    argv[i++] = "-child";		/* Tell plserver it's ancestry */

    argv[i++] = "-f";			/* TCL init proc for server */
    argv[i++] = Tcl_GetVar(dev->interp,
			   "plserver_init", 0);

    if (pls->auto_path != NULL) {
	argv[i++] = "-auto_path";	/* Additional directory(s) */
	argv[i++] = pls->auto_path;	/* to autoload */
    }

    if (pls->FileName != NULL) {
	argv[i++] = "-display";		/* X display */
	argv[i++] = pls->FileName;
    }

    if (pls->geometry != NULL) {
	argv[i++] = "-geometry";	/* Top level window geometry */
	argv[i++] = pls->geometry;
    }

/* Start server process */

    if ( (pid = FORK()) < 0) {
	abort_session(pls, "fork error");
    }
    else if (pid == 0) {
	argv[i++] = NULL;
	if (execvp("plserver", argv)) {
	    abort_session(pls, "execvp error");
	}
    }

/*
* Wait for server to send back notification.
*
* The true server main window name will then be given by $plserver.
* This is not always the same as the name of the application since you
* could have multiple copies running on the display (resulting in
* names of the form "plserver #2", etc).
*/

    tcl_cmd(pls, "tkwait variable plserver");

    dev->launched_server = 1;
}

/*----------------------------------------------------------------------*\
* plwindow_init
*
* Configures the widget hierarchy we are sending the data stream to.  
*
* If a widget name (identifying the actual widget or a container widget)
* hasn't been supplied already we assume it needs to be created.
*
* In order to achieve maximum flexibility, the plplot tk driver requires
* only that certain TCL procs must be defined in the server interpreter.
* These can be used to set up the desired widget configuration.  The procs
* invoked from this driver currently include:
*
*    $plw_create	Creates the widget environment
*    $plw_init		Initializes the widget(s)
*    $plw_end		Prepares for shutdown
*    $plw_flash		Invoked when waiting for page advance
*
* Since all of these are interpreter variables, they can be trivially
* changed by the user (use the -tcl_cmd option).
*
* Each of these utility procs is called with a widget name ($plwindow)
* as argument.  "plwindow" is set from the value of pls->plwindow, and
* if null is generated from the name of the client main window (to
* ensure uniqueness).  $plwindow usually indicates the container frame
* for the actual plplot widget, but can be arbitrary -- as long as the
* usage in all the TCL procs is consistent.
*
* In order that the TK driver be able to invoke the actual plplot
* widget, the proc "$plw_init" deposits the widget name in the local
* interpreter variable "plwidget".
*
* In addition, the name of the client main window is given as (2nd)
* argument to "$plw_init".  This establishes the client name used
* for communication for all child widgets that require it.
\*----------------------------------------------------------------------*/

static void
plwindow_init(PLStream *pls)
{
    TkDev *dev = (TkDev *) pls->dev;
    char str[10], **argv, *clientvar;
    int i;
    long bg;

    dbug_enter("plwindow_init");

/* If widget does not exist we must create it */

    if (pls->plwindow == NULL) {

/* Give it a name */
/* To make sure it's unique, use the main window id (replace blanks */
/* with underscores to avoid quoting problems) */

	pls->plwindow = (char *)
	    malloc(1+(strlen(dev->program)) * sizeof(char));

	sprintf(pls->plwindow, ".%s", dev->program);
	for (i = 0; i < strlen(pls->plwindow); i++) {
	    if (pls->plwindow[i] == ' ')
		pls->plwindow[i] = '_';
	}

	Tcl_SetVar(dev->interp, "plwindow", pls->plwindow, 0);

/* Create the plframe widget & anything else you want with it. */

	server_cmd( pls, "update" );
	server_cmd( pls, "$plw_create $plwindow" );
    }

/* Initialize the widget(s) */

    server_cmd( pls, "update" );
    server_cmd( pls, "[list $plw_init $plwindow $client]" );

/* Now we should have the actual plplot widget name in $plwidget */
/* Configure it if necessary. */

    if (pls->bgcolorset) {
	bg = (((pls->bgcolor.r << 8) | pls->bgcolor.g) << 8) | pls->bgcolor.b;
	sprintf(str, "#%06x", (bg & 0xFFFFFF));
	Tcl_SetVar(dev->interp, "bg", str, 0);
	server_cmd( pls, "update" );
	server_cmd( pls, "$plwidget configure -bg $bg" );
    }
}

/*----------------------------------------------------------------------*\
* link_init
*
* Initializes the link between the client and the plplot widget for
* data transfer.  Right now only fifo's are supported.
\*----------------------------------------------------------------------*/

static void
link_init(PLStream *pls)
{
    TkDev *dev = (TkDev *) pls->dev;
    int fd;

    dbug_enter("link_init");

/* Create the fifo for data transfer to the plframe widget */

    dev->filetype = "fifo";
    dev->filename = (char *) tempnam((char *) NULL, "pltk");

    if (mkfifo(dev->filename, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) < 0) {
	abort_session(pls, "mkfifo error");
    }

/* Tell plframe widget to open fifo (for reading). */

    Tcl_SetVar(dev->interp, "fifoname", dev->filename, 0);
    server_cmd( pls, "update" );
    server_cmd( pls, "$plwidget openfifo $fifoname" );

/* Open the fifo for writing */
/* The server must now have it open for reading or this will fail */

    if ((fd = open(dev->filename, O_WRONLY | O_NONBLOCK)) == -1) {
	abort_session(pls, "Error opening fifo for write");
    }
    dev->file = fdopen(fd, "wb");

/* Unlink fifo now so that it isn't left around if program crashes. */
/* This also ensures no other program can mess with it. */

    if (unlink(dev->filename) == -1) {
        abort_session(pls, "Error removing fifo");
    }
}

/*----------------------------------------------------------------------*\
* WaitForPage()
*
* Waits for a page advance.  A call to HandleEvents() is made before
* returning to ensure all pending events were dispatched.
\*----------------------------------------------------------------------*/

static void
WaitForPage(PLStream *pls)
{
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("WaitForPage");

    flush_output(pls);

    server_cmd( pls, "$plw_flash $plwindow" );
    tk_wait(pls, "[info exists advance] && ($advance == 1)" );
    server_cmd( pls, "$plw_flash $plwindow" );

    Tcl_SetVar(dev->interp, "advance", "0", 0);

    HandleEvents(pls);
}

/*----------------------------------------------------------------------*\
* HandleEvents()
*
* Just a front-end to the update command, for use when not actually
* waiting for an event but only checking the event queue.  Also we
* check to see if it's time to tell the server to read from the pipe.
\*----------------------------------------------------------------------*/

static void
HandleEvents(PLStream *pls)
{
    TkDev *dev = (TkDev *) pls->dev;
    char tmp[20];

    dbug_enter("HandleEvents");
    Tcl_Eval(dev->interp, "update", 0, (char **) NULL);

    if (pls->bytecnt > pls->bufmax)
	flush_output(pls);
}

/*----------------------------------------------------------------------*\
* flush_output()
*
* Flushes output and sends command to the server to read from the pipe.
* The "after 1" is used so that the send command returns before the data
* read is complete (i.e. each process continues independently).
\*----------------------------------------------------------------------*/

static void
flush_output(PLStream *pls)
{
    TkDev *dev = (TkDev *) pls->dev;
    char tmp[20];

    dbug_enter("flush_output");

#ifdef DEBUG
    fprintf(stderr, "%s: Flushing buffer, bytecnt = %d\n",
	    dev->program, pls->bytecnt);
    fflush(stderr);
#endif

    if (pls->bytecnt > 0) {
	fflush(dev->file);
	sprintf(tmp, "%d", pls->bytecnt);
	Tcl_SetVar(dev->interp, "nbytes", tmp, 0);
	server_cmd( pls, "after 1 $plwidget read $nbytes" );
	pls->bytecnt = 0;
    }
}

/*----------------------------------------------------------------------*\
* Abort
*
* Just a TCL front-end to abort_session().
\*----------------------------------------------------------------------*/

static int
Abort(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    PLStream *pls = (PLStream *) clientData;

    dbug_enter("Abort");

    abort_session(pls, "");
    return TCL_OK;
}

/*----------------------------------------------------------------------*\
* KeyEH()
*
* This TCL command handles keyboard events.
*
* Arguments:
*	command name
*	keysym name (textual string)
*	keysym value
*	ASCII equivalent (optional)
*
* The first argument is keysym name -- this is all that's really required 
* although it's better to send the numeric keysym value since then we
* can avoid a long lookup procedure.  Sometimes, when faking input, it
* is inconvenient to have to worry about what the numeric keysym value
* is, so in a few cases a missing keysym value is tolerated.
\*----------------------------------------------------------------------*/

static int
KeyEH(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    PLStream *pls = (PLStream *) clientData;
    TkDev *dev = (TkDev *) pls->dev;

    PLKey key;
    char *keysym, c;
    int advance = 0;

    dbug_enter("KeyEH");

    if (argc < 2) {
	plwarn("KeyEH: Insufficient arguments given");
	return;
    }
    key.code = 0;
    key.string[0] = '\0';

/* Keysym name */

    keysym = argv[1];

/* Keysym value */
/* If missing, explicitly check for a few common ones */

    if (argc > 2)
	key.code = atol(argv[2]);

    if (argc == 2 || key.code == 0) {
	c = *keysym;
	if ((c == 'B') && (strcmp(keysym, "BackSpace") == 0)) {
	    key.code = PLK_BackSpace;
	}
	else if ((c == 'D') && (strcmp(keysym, "Delete") == 0)) {
	    key.code = PLK_Delete;
	}
	else if ((c == 'L') && (strcmp(keysym, "Linefeed") == 0)) {
	    key.code = PLK_Linefeed;
	}
	else if ((c == 'R') && (strcmp(keysym, "Return") == 0)) {
	    key.code = PLK_Return;
	}
	else if ((c == 'P') && (strcmp(keysym, "Prior") == 0)) {
	    key.code = PLK_Prior;
	}
	else if ((c == 'N') && (strcmp(keysym, "Next") == 0)) {
	    key.code = PLK_Next;
	}
	else {
	    fprintf(stderr, "Unrecognized keysym %s\n", keysym);
	    return;
	}
    }

/* ASCII value */

    if (argc > 3) {
	key.string[0] = argv[3][0];
	key.string[1] = '\0';
    }

#ifdef DEBUG
    fprintf(stderr, "KeyEH: Keysym %s, hex %x, ASCII: %s\n",
	    keysym, key.code, key.string);
#endif

/* Call user event handler */
/* Since this is called first, the user can disable all plplot internal
   event handling by setting key.code to 0 and key.string to '\0' */

    if (pls->KeyEH != NULL)
	(*pls->KeyEH) (&key, pls->KeyEH_data, &advance);

/* Handle internal events */

/* Advance to next page (i.e. terminate event loop) on a <eol> */
/* Check for both <CR> and <LF> for portability, also a <Page Down> */

    if (key.code == PLK_Return ||
	key.code == PLK_Linefeed ||
	key.code == PLK_Next)
	advance = TRUE;

    if (advance)
	Tcl_SetVar(dev->interp, "advance", "1", 0);

/* Terminate on a 'Q' (not 'q', since it's too easy to hit by mistake) */

    if (key.string[0] == 'Q') {
	pls->nopause = TRUE;
	plexit("");
    }
    return TCL_OK;
}

/*----------------------------------------------------------------------*\
* tk_wait()
*
* Waits for the specified expression to evaluate to true before
* proceeding.  While we are waiting to proceed, all events (for this
* or other interpreters) are handled.  
\*----------------------------------------------------------------------*/

static void
tk_wait(PLStream *pls, char *cmd)
{
    TkDev *dev = (TkDev *) pls->dev;
    int result = 0;

    dbug_enter("tk_wait");

    while ( ! result) {
	if (Tcl_ExprBoolean(dev->interp, cmd, &result)) {
	    fprintf(stderr, "tk_wait command \"%s\" failed:\n\t %s\n",
		    cmd, dev->interp->result);
	    break;
	}
	Tk_DoOneEvent(0);
    }
}

/*----------------------------------------------------------------------*\
* server_cmd
*
* Sends specified command to server, aborting on an error.
\*----------------------------------------------------------------------*/

static void
server_cmd(PLStream *pls, char *cmd)
{
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("server_cmd");
#ifdef DEBUG_ENTER
    fprintf(stderr, "Sending command: %s\n", cmd);
#endif

    if (Tcl_VarEval(dev->interp, "send $plserver ", cmd, (char **) NULL)) {
	fprintf(stderr, "Server command \"%s\" failed:\n\t %s\n",
		cmd, dev->interp->result);
	abort_session(pls, "");
    }
}

/*----------------------------------------------------------------------*\
* tcl_cmd
*
* Evals the specified command, aborting on an error.
\*----------------------------------------------------------------------*/

static void
tcl_cmd(PLStream *pls, char *cmd)
{
    TkDev *dev = (TkDev *) pls->dev;

    dbug_enter("tcl_cmd");
#ifdef DEBUG_ENTER
    fprintf(stderr, "Evaluating command: %s\n", cmd);
#endif

    if (tcl_eval(pls, cmd)) {
	fprintf(stderr, "TCL command \"%s\" failed:\n\t %s\n",
		cmd, dev->interp->result);
	abort_session(pls, "");
    }
}

/*----------------------------------------------------------------------*\
* tcl_eval
*
* Evals the specified string, returning the result.
* A front-end to Tcl_Eval just to make it easier to use here.
\*----------------------------------------------------------------------*/

static int
tcl_eval(PLStream *pls, char *cmd)
{
    TkDev *dev = (TkDev *) pls->dev;

    return(Tcl_Eval(dev->interp, cmd, 0, (char **) NULL));
}

/*----------------------------------------------------------------------*/
#else
int
pldummy_tk()
{
    return 0;
}

#endif				/* TK */
