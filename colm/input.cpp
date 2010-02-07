/*
 *  Copyright 2007, 2008 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "input.h"
#include "fsmrun.h"
#include <stdio.h>
#include <iostream>

using std::cerr;
using std::endl;

bool InputStream::isTree()
{ 
	if ( queue != 0 && queue->type == RunBuf::Token )
		return true;
	return false;
}

bool InputStream::isIgnore()
{ 
	if ( queue != 0 && queue->type == RunBuf::Ignore )
		return true;
	return false;
}


/*
 * String
 */

int InputStreamString::getData( char *dest, int length )
{ 
	int available = dlen - offset;

	if ( available < length )
		length = available;

	memcpy( dest, data+offset, length );
	offset += length;

	if ( offset == dlen )
		eof = true;

	return length;
}

void InputStreamString::pushBackBuf( RunBuf *runBuf )
{
	//char *data = runBuf->buf + runBuf->offset;
	long length = runBuf->length;

	assert( length <= offset );
	offset -= length;
}

/*
 * File
 */

int InputStreamFile::isEOF()
{
	return queue == 0 && feof( file );
}

int InputStreamFile::needFlush()
{
	return queue == 0 && feof( file );
}

int InputStreamFile::getData( char *dest, int length )
{
	/* If there is any data in queue, read from that first. */
	if ( queue != 0 ) {
		long avail = queue->length - queue->offset;
		if ( length >= avail ) {
			memcpy( dest, &queue->buf[queue->offset], avail );
			RunBuf *del = queue;
			queue = queue->next;
			delete del;
			return avail;
		}
		else {
			memcpy( dest, &queue->buf[queue->offset], length );
			queue->offset += length;
			return length;
		}
	}
	else {
		return fread( dest, 1, length, file );
	}
}

void InputStreamFile::pushBackBuf( RunBuf *runBuf )
{
	runBuf->next = queue;
	queue = runBuf;
}

/*
 * FD
 */

int InputStreamFd::isEOF()
{
	return queue == 0 && eof;
}

int InputStreamFd::needFlush()
{
	return queue == 0 && eof;
}

void InputStreamFd::pushBackBuf( RunBuf *runBuf )
{
	runBuf->next = queue;
	queue = runBuf;
}

int InputStreamFd::getData( char *dest, int length )
{
	/* If there is any data in queue, read from that first. */
	if ( queue != 0 ) {
		long avail = queue->length - queue->offset;
		if ( length >= avail ) {
			memcpy( dest, &queue->buf[queue->offset], avail );
			RunBuf *del = queue;
			queue = queue->next;
			delete del;
			return avail;
		}
		else {
			memcpy( dest, &queue->buf[queue->offset], length );
			queue->offset += length;
			return length;
		}
	}
	else {
		long got = read( fd, dest, length );
		if ( got == 0 )
			eof = true;
		return got;
	}
}

/*
 * Accum
 */

int InputStreamAccum::isEOF()
{
	return eof;
}

bool InputStreamAccum::tryAgainLater()
{
	if ( !flush && head == 0 && tail == 0 )
		return true;

	return false;
}

int InputStreamAccum::needFlush()
{
	if ( flush ) {
		flush = false;
		return true;
	}

	if ( eof )
		return true;
		
	return false;
}

int InputStreamAccum::getData( char *dest, int length )
{
	if ( head == 0 )
		return 0;

	int available = head->length - offset;

	if ( available < length )
		length = available;

	memcpy( dest, head->data + offset, length );
	offset += length;

	if ( offset == head->length ) {
		head = head->next;
		if ( head == 0 )
			tail = 0;
		offset = 0;
	}

	return length;
}

void InputStreamAccum::pushBackBuf( RunBuf *runBuf )
{
}

void InputStreamAccum::append( const char *data, long len )
{
	AccumData *ad = new AccumData;
	if ( head == 0 ) {
		head = tail = ad;
		ad->next = 0;
	}
	else {
		tail->next = ad;
		ad->next = 0;
		tail = ad;
	}

	ad->data = new char[len];
	memcpy( ad->data, data, len );
	ad->length = len;
}
