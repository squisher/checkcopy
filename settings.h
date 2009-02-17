/* $Id: settings.h 38 2008-07-20 18:37:20Z squisher $ */
/*
 *  Copyright (c) 2008 David Mohr <david@mcbf.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __SETINGS_H__
#define __SETINGS_H__

/* BUF_SIZE determines the size of the chunks that get read in from the
 * input file in one call to fread */
#define BUF_SIZE 8192

/* BUF_SLOTS is the number of slots the ring buffer between the copying and
 * the hashing thread has */
#define BUF_SLOTS 64

/* CONS_WAIT_TIME sets how long the hashing thread should wait for the copying
 * thread to produce data before timing out, which is non-fatal, see below */
#define CONS_WAIT_TIME (G_USEC_PER_SEC / 2)

/* MAX_CONSUMER_RETRIES is the number of times that the consumer can time out,
 * before it is considered an error, see above */
#define MAX_CONSUMER_RETRIES 4

/* PROGRESS_UPDATE_NUM defines how many updates there should be to the progress
 * bar. More updates mean a higher load on the X server. This value is used
 * to calculate after how many blocks the progress bar is updated */
#define PROGRESS_UPDATE_NUM 200

/* MIN_BLOCKS_PER_UPDATE limits the calculation based upon PROGRESS_UPDATE_NUM
 * so that there have to be at least this many blocks before the progress gets
 * updated. This should only matter for very small copying operations */
#define MIN_BLOCKS_PER_UPDATE 16

/* MAX_FILENAME_LEN specifies the longest as filename can get before it is
 * truncated in the progress dialog */
#define MAX_FILENAME_LEN 100

#endif /* __SETINGS_H__ */
