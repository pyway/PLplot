// Copyright (C) 2016 Alan W. Irwin
//
// This file is part of PLplot.
//
// PLplot is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as published
// by the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// PLplot is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with PLplot; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
//! @file
//! C source code for the plplotf95c library.  The routines in this
//! library are not meant to be called by external applications.
//! (Normally, they would be kept private by putting them in the
//! plplotf95 library and declaring them static.  However, best
//! cross-platform linker results are normally obtained by not mixing
//! different languages into the same library so that is the only
//! reason the plplotf95c C library is kept separate from the plplotf95
//! Fortran library.)  The plplotf95 Fortran library wraps the routines
//! in this plplotf95 C library and the plplot core C library
//! to implement the externally visible Fortran interface for the PLplot library.
//!
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plplot.h"
#ifndef MIN
#define MIN( a, b )    ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif

// Static function declarations.

static int
Alloc2dChar( char ***fp, unsigned nx, unsigned ny );

static int
Free2dChar( char **f, unsigned nx );

static void
convert_string_array( char **cstrings, char * fstrings, unsigned number, unsigned length );

// Static functions.

//--------------------------------------------------------------------------
// Alloc2dChar()
//
//! Allocates a block of memory for use as a 2-d grid of characters.
//! The resulting array can be indexed as f[i][j] anywhere.  Note that
//! this type of allocation is required by functions which take a 2-d
//! grid of characters as an argument, e.g., the plparseopt argument.
//!
//! Example usage:
//!
//!   char **f;
//!
//!   Alloc2dChar(&f, nx, ny);
//!
//! @param fp Location of the storage (address of a **).
//! @param nx Size of the grid in x.
//! @param ny Size of the grid in y.
//!
//--------------------------------------------------------------------------

static int
Alloc2dChar( char ***fp, unsigned nx, unsigned ny )
{
    unsigned i;

    if ( nx < 1 )
        return 1;
    if ( ny < 1 )
        return 2;
    if ( ( *fp = (char **) calloc( (size_t) nx, sizeof ( char * ) ) ) == NULL )
        return 3;

    for ( i = 0; i < nx; i++ )
    {
        if ( ( ( *fp )[i] = (char *) calloc( (size_t) ny, sizeof ( char ) ) ) == NULL )
            return 4;
    }

    return 0;
}

//--------------------------------------------------------------------------
// Free2dChar()
//
//! Frees a 2d grid of memory allocated with Alloc2dChar().
//!
//! Example usage:
//!
//!   int nx, ny;
//!   char **f;
//!
//!   Initialize nx and ny to whatever values are desired, e.g.,
//!   nx = 15;
//!   ny = 6;
//!   Then...
//!
//!   Alloc2dChar(&f, nx, ny);
//!
//!   and use f[i][j] in this routine or any
//!   called routine with an argument of f where i < nx and j < ny.
//!
//!   ...Once completely finished with using f, then ...
//!
//!   Free2dChar(f, nx);
//!
//! (Free2dChar does not require ny to free the grid of memory.)
//!
//! @param f A pointer to a pointer to the grid of memory that needs to
//! be freed.
//! @param nx Size of the grid in x.
//! @returns zero on success.
//!
//--------------------------------------------------------------------------

static int
Free2dChar( char **f, unsigned nx )
{
    unsigned i;

    if ( nx < 1 )
        return 1;
    for ( i = 0; i < nx; i++ )
        free( (void *) f[i] );

    free( (void *) f );
    return 0;
}

//! Internal utility routine to convert a Fortran character array
//! to an array of pointers to null-terminated C character
//! strings.
//!
//! @param cstrings [OUT ONLY] A 2D array which is typed as char ** and
//! with dimensions set up (with Alloc2dChar) of
//! [fnumber][length+1].  This routine fills each of the "fnumber" rows
//! with a null-terminated C character string with an individual
//! length (not including the NULL terminator) of less than or equal
//! to "length".
//! @param fstrings [IN ONLY] Pointer to a Fortran character array
//! which is declared externally in a Fortran routine as
//! character*(length) fstrings(number) which contains on entry "number" number of
//! Fortran character*(length) array elements each with
//! a uniform length of "length" padded on the right with blanks.
//! @param number [IN ONLY] Number of Fortran array elements to to copy
//! to the cstrings array of pointers to character strings.
//! @param length [IN ONLY] One less than the number of bytes in the C
//! strings which can have a NULL in any position (including the last
//! byte).  This value is also the blank-padded length of each of the
//! strings in the Fortran character array pointed to by "fstrings".
//!
static void
convert_string_array( char **cstrings, char * fstrings, unsigned number, unsigned length )
{
    size_t number_size_t = (size_t) number, length_size_t = (size_t) length;
    size_t j, i, string_length;

    for ( j = 0; j < number_size_t; j++ )
    {
        strncpy( cstrings[j], &fstrings[j * length_size_t], length_size_t );

        // NULL-terminate cstrings[j] and get rid of trailing blanks
        i = length_size_t;
        while ( i > 0 && cstrings[j][i - 1] == ' ' )
            cstrings[j][--i] = '\0';
    }
}

//! This plcolorbar wrapper is called from the Fortran level via an iso_c_binding
//! interface.  This wrapper is necessary because of the call to
//! convert_string_array to convert the Fortran array of characters arguments to the
//! array of char pointers form required by the corresponding plcolorbar arguments.
//!

PLDLLIMPEXP_F95C void
fc_plparseopts( unsigned name_length, unsigned size, char *names, PLINT mode )
{
    // Assign to NULL to quiet spurious potentially undefined warning.
    char     ** cnames   = NULL;
    unsigned actual_size = size;
    char     * save_pointer;

    // names is a pointer to a a blank-terminated Fortran
    // character*(name_length) names(size) array.


    // Make cnames the same size as the Fortran character array names
    // except the character string size is one larger to contain the
    // trailing NULL associated with C strings.
    Alloc2dChar( &cnames, actual_size, name_length + 1 );
    // gdb analysis indicates this pointer gets set to NULL below
    // in the plparseopts call so we need to save it so we
    // can free the associated memory after the plparseopts call.
    save_pointer = NULL;
    if ( actual_size > 1 )
        save_pointer = cnames[1];

    // Convert names (which refers to a blank-terminated Fortran
    // character*(name_length) names(size) array) to cnames which
    // is an array of pointers to null-terminated C strings.
    convert_string_array( cnames, names, size, name_length );

    plparseopts( &size, (const char **) cnames, mode );

    // cleanup
    // plparseopts changes size (and also the contents of cnames[1] to
    // NULL) during parsing so must free what was cnames[1] and make
    // the Free2dChar call with actual_size
    if ( actual_size > 1 )
        free( (void *) save_pointer );
    Free2dChar( cnames, actual_size );
}
