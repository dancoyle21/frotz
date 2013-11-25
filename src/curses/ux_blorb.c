/*
 * ux_blorb.c - Blorb routines
 *	David Griffith <dave@661.org>
 *
 * This file is part of Frotz.
 *
 * Frotz is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Frotz is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#define __UNIX_PORT_FILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <libgen.h>
#include <math.h>

#ifdef USE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "ux_frotz.h"
#include "ux_blorb.h"

f_setup_t f_setup;
u_setup_t u_setup;


/*
 * isblorb
 *
 * Returns 1 if this file is a Blorb file, 0 if not.
 *
 * FIXME Is there a potential endian problem here?
 */
static int isblorb(FILE *fp)
{
    char mybuf[4];

    if (fp == NULL)
	return 0;

    fread(mybuf, 1, 4, fp);
    if (strncmp(mybuf, "FORM", 4))
	return 0;

    fseek(fp, 4, SEEK_CUR);
    fread(mybuf, 1, 4, fp);

    if (strncmp(mybuf, "IFRS", 4))
	return 0;

    return 1;
}


/*
 * ux_init_blorb
 *
 * Check if we're opening a Blorb file directly.  If not, check
 * to see if there's a seperate Blorb file that looks like it goes
 * along with this Zcode file.  If we have a Blorb file one way or the
 * other, make a Blorb map.  If we opened a Blorb file directly, that
 * means that our executable is in that file and therefore we will look
 * for a ZCOD chunk and record its location so os_load_story() can find it.
 *
 */
bb_err_t ux_init_blorb(char *filename)
{
    FILE *fp;
    char *p;
    char *mystring;
    int  len1;
    int  len2;

    bb_err_t blorb_err;

    if ((fp = fopen(filename, "rb")) == NULL)
	return bb_err_Read;

    printf("filename: %s\n", filename);

    /* Is this really a Blorb file?  If not, maybe we're loading a naked
     * zcode file and our resources are in a seperate blorb file.
     */
    if (isblorb(fp)) {			/* Now we know to look */
	u_setup.exec_in_blorb = 1;	/* for zcode in the blorb */
    } else {
	len1 = strlen(filename) + strlen(EXT_BLORB);
	len2 = strlen(filename) + strlen(EXT_BLORBLONG);

	mystring = malloc(len2 * sizeof(char) + 1);
        strncat(mystring, filename, len1 * sizeof(char));
	p = rindex(mystring, '.');
	*p = '\0';

        strncat(mystring, EXT_BLORB, len1 * sizeof(char));

	/* Check if foo.blb is there. */
        if ((fp = fopen(mystring, "rb")) == NULL) {
	    p = rindex(mystring, '.');
	    *p = '\0';
            strncat(mystring, EXT_BLORBLONG, len2 * sizeof(char));
	    fp = fopen(mystring, "rb");
	}
	if (fp == NULL || !isblorb(fp))	/* No matching blorbs found. */
	    return bb_err_NoBlorb;

	u_setup.exec_in_blorb = 0;	/* Using naked zcode with */
    }					/* resources in seperate blorb. */

    /* Create a Blorb map from this file.
     * This will fail if the file is not a valid Blorb file.
     * From this map, we can now pick out any resource we need.
     */
    blorb_err = bb_create_map(fp, &blorb_map);
    if (blorb_err != bb_err_None)
	return bb_err_Format;

    /* Locate the EXEC chunk within the blorb file and record its
     * location so os_load_story() can find it.
     */
    if (u_setup.exec_in_blorb) {
	blorb_err = bb_load_chunk_by_type(blorb_map, bb_method_FilePos,
		&blorb_res, bb_make_id('Z','C','O','D'), 0);
    }

    if (blorb_err == bb_err_None) {
	u_setup.exec_in_blorb = 1;
	u_setup.use_blorb = 1;
    }

    fclose(fp);
    return blorb_err;
}


char *findchunk(char *data, char *string, int length)
{
    char *mydata = data+12;
    while (TRUE) {
	if (strncmp((char*)mydata, string, 4) == 0)
		return mydata+8;
	mydata += ReadLong(mydata+4)+8;
	if ((mydata - data) >= length)
		break;
    }
    return NULL;
}


unsigned short ReadShort(const unsigned char *bytes)
{
    return (unsigned short)(
	((unsigned short)(bytes[0] & 0xFF) << 8) |
	((unsigned short)(bytes[1] & 0xFF)));
}


unsigned long ReadLong(const unsigned char *bytes)
{
    return (unsigned long)(
	((unsigned long)(bytes[0] & 0xFF) << 24) |
	((unsigned long)(bytes[1] & 0xFF) << 16) |
	((unsigned long)(bytes[2] & 0xFF) << 8) |
	((unsigned long)(bytes[3] & 0xFF)));
}


double ReadExtended(const unsigned char *bytes)
{
    double f;
    int expon;
    unsigned long hiMant, loMant;

    expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hiMant = ReadLong(bytes+2);
    loMant = ReadLong(bytes+6);

    if (expon == 0 && hiMant == 0 && loMant == 0)
	f = 0;
    else {
 	if (expon == 0x7FFF) /* Infinity or NaN */
	    f = -1;
	else {
	    expon -= 16383;
	    /* must #include <math.h> or these won't work */
	    f = ldexp(UnsignedToFloat(hiMant),expon -= 31);
	    f += ldexp(UnsignedToFloat(loMant),expon -= 32);
	}
    }

    if (bytes[0] & 0x80)
	return -f;
    return f;
}