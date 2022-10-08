/* Copyright (C) 2005-2015 Holger Ruckdeschel <holger@hoicher.de>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include "common.h"

#if 0
#include "queue.h"
#include "thread.h"

static void * thread_main(void * arg)
{
	Queue<int>* q = (Queue<int>*) arg;
	while (1) {
		int i = q->get();
		printf("get %d\n", i);
		if (i == 9) {
			break;
		}
	}

	return NULL;
}

void test_queue()
{
	Queue<int> q;
	Thread t(thread_main);
	t.start((void*) &q);

	for (unsigned int i=0; i<10; i++) {
		printf("put %d\n",i );
		q.put(i);
		printf("put %d done\n",i );
	}

	t.wait();
}
#endif

