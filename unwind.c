/*
 * Program to unwind the stack frames of a x86_64 process using libunwind
 *
 * Author: Arun Prakash Jana <engineerarun@gmail.com>
 * Copyright (C) 2014, 2015 by Arun Prakash Jana <engineerarun@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with tictactoe.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libunwind.h>
#include <libunwind-ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

int main(int argc, char **argv)
{
	unw_addr_space_t addrspace;
	struct UPT_info *uptinfo = NULL;
	unw_accessors_t accessors;
	unw_cursor_t cursor;
	unw_word_t RIP, RBP;

	pid_t PID = 1;
	int ret = 0, wait_loops = 20, wait_time = 1000, waitstatus, stopped = 0;

	if (argc !=2) {
		fprintf(stderr, "Usage: unwind PID\n");
		return -1;
	}

	if ((PID = atoi(argv[1])) <= 0) {
		fprintf(stderr, "Valid PID please!\n");
		return -1;
	}

	fprintf(stdout, "Tracing PID: %d\n", PID);

	/* Create address space for little endian */
	addrspace = unw_create_addr_space(&accessors, 0);
	if (!addrspace) {
		fprintf(stderr, "unw_create_addr_space failed\n");
		return -1;
	}

        ret = ptrace(PTRACE_ATTACH, PID, NULL, NULL);
        if (0 != ret && 0 != errno) {
                ret = errno;    
                return ret; 
        }

	while (wait_loops-- > 0) {
		ret = waitpid(PID, &waitstatus, WUNTRACED | WNOHANG);
		if (WIFSTOPPED(waitstatus)) {
			stopped = 1;
			break;
		}
		usleep(wait_time);
	}

	if (!stopped) {
		fprintf(stderr, "Traget process couldn't be stopped\n");
		goto bail;
	}

	uptinfo = (struct UPT_info *)_UPT_create(PID);
	if (!uptinfo) {
		fprintf(stderr, "_UPT_create failed\n");
		goto bail;
	}

	ret = unw_init_remote(&cursor, addrspace, uptinfo);
	if (ret < 0) {
		fprintf(stderr, "unw_init_remote failed\n");
		goto bail;
	}

	if (unw_get_reg(&cursor, UNW_X86_64_RIP, &RIP) < 0 || unw_get_reg(&cursor, UNW_X86_64_RBP, &RBP)) {
		fprintf(stderr, "unw_get_reg RIP/RBP failed\n");
		goto bail;
	}

	fprintf(stdout, "RIP: 0x%lx\n", RIP);
	fprintf(stdout, "RBP: 0x%lx\n", RBP);

bail:
	if (uptinfo)
		_UPT_destroy(uptinfo);

	unw_destroy_addr_space(addrspace);

	return 0;
}
