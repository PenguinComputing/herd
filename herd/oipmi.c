/*
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * Copyright 2007-2009 Sun Microsystems, Inc. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the only version 2 of the GNU General
 * Public License, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included in the COPYING file that accompanied this file).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/types.h>
#include <fcntl.h>


#include "oipmi.h"
#include "options.h"
#include "mce.h"
#include "dimm.h"
#include "log.h"

static struct {

    int do_exit;
    int timer;

} self;


int oipmi_create_NB_event(struct mce* mce, ipmi_sel_t* sel);
int send_open_ipmi_cmd(uint8_t, uint8_t, int, uint8_t*, int*, uint8_t*);

void oipmi_init()
{
   memset(&self, 0, sizeof(self));
}

void oipmi_exit()
{
}

void oipmi_main_loop()
{
    if (options.singleiter_mode) {
	mce_process_log();
	return;
    }

    log_msg("herd daemon started");

    int timer = options_get_int("check_timer_secs");
    if (timer <= 0)
	timer = 30;

    while (!self.do_exit) {
	mce_process_log();
	sleep(timer);
    }
}

int ipmi_request_event(ipmi_sel_t* sel)
{
    uint8_t recbuffer[8];
    int recsz = sizeof(recbuffer);
    int ret;

    ret = send_open_ipmi_cmd(IPMI_SENSOR_EVENT, IPMI_CMD_REQUEST_EVENT,
			     sizeof(ipmi_sel_t), (uint8_t*)sel,
			     &recsz, recbuffer);

    return ret;
}

// Process an MCE from the kernel. Package it into an SEL entry and
// send it on its way to the BMC.
int oipmi_process_mce(struct mce* mce)
{
    ipmi_sel_t sel;
    int ok = 0;

    // Dispatch based on the bank reporting the MCE error. Almost all
    // MCEs will come from the NorthBridge device (bank 4).
    switch(mce->bank) {
    case 4:
	ok = oipmi_create_NB_event(mce, &sel);
	break;
    default:
	return 0;
    }

    if (!ok) return -1;

    return ipmi_request_event(&sel);
}

int oipmi_create_NB_event(struct mce* mce, ipmi_sel_t* sel)
{
    McaNbStatus st;

    assert(sizeof(st) == sizeof(mce->status));
    memcpy(&st, &(mce->status), sizeof(st));

    // First, make sure the ErrorValid and ErrorEnable bits are set.
    if (!st.ErrValid || !st.ErrEn)
	return 0;

    if (st.CorrECC || st.UnCorrECC) {
	decode_input_t dinput;
	decode_result_t result;

	dinput.address = mce->addr;
	/* See table for ErrorCodeExt: 0x08 = ECC error. */
	if (st.ErrorCodeExt == 0x08) {
	    dinput.syndrome = (st.Syndrome << 8) + st.ECC_Synd;
	} else {
	    dinput.syndrome = 0;
	}

	if (!dimm_translate_address(&dinput, &result)) {
	    log_msg("Unable to identify CPU Node,Dimm for address %012llx\n",
		    mce->addr);
	    return 0;
	}

	sel->evm_rev = 4;
	sel->sensor_type = 0x0C; // Memory Sensor Type
	sel->sensor_num = 0;
	sel->event_type = 0x6f;  // Sensor-specific discrete
	sel->event_dir = 0;

	sel->event_data[0] = 0x20 + st.UnCorrECC;

	uint8_t ndp = (result.dimm << 4) + result.node;
	sel->event_data[1] = (result.is_dimm_pair ? 0xff : ndp);
	sel->event_data[2] = ndp;

	return 1;
    }

    return 0;
}

// Send a test (i.e. fake) ECC correctable error message to the SP
// System Event Log.

int oipmi_test(uint64_t address, u32 syndrome)
{
    int ret;

    struct mce* mce = (struct mce*)malloc(sizeof(struct mce));
    memset(mce, 0, sizeof(*mce));

    mce->bank = 4;
    mce->addr = address;
    McaNbStatus st;
    memset(&st, 0, sizeof(st));
    st.ErrValid = 1;
    st.ErrEn = 1;
    st.CorrECC = 1;
    st.Syndrome = (syndrome >> 8) & 0xff;
    st.ECC_Synd = syndrome & 0xff;
    assert(sizeof(st) == sizeof(mce->status));
    memcpy(&(mce->status), &st, sizeof(st));
    ret = oipmi_process_mce(mce);
    free(mce);

    return ret;
}


int send_open_ipmi_cmd(uint8_t netfn, uint8_t cmd,
		       int  datai_len, uint8_t* datai,
		       int* datao_len, uint8_t* datao)
{
    struct ipmi_system_interface_addr bmc_addr = {
	addr_type:      IPMI_SYSTEM_INTERFACE_ADDR_TYPE,
	channel:        IPMI_BMC_CHANNEL,
    };

    int fd = 0, rv;
    fd_set rset;

    struct ipmi_addr addr;
    struct ipmi_recv recv;
    struct ipmi_req req;

    struct timeval  timeout;

    // Open ipmi device
    if ((fd = open(IPMI_DEV, O_RDWR)) == -1 ) {
	log_errno("Could not open IPMI device \"%s\"", IPMI_DEV);
	goto fail;
    }
 
    memset(&req, 0, sizeof(struct ipmi_req));
    req.addr = (unsigned char *)&bmc_addr;
    req.addr_len = sizeof(bmc_addr);
    req.msgid = MSGID;
    req.msg.netfn = netfn;
    req.msg.cmd = cmd;
    req.msg.data_len = datai_len;
    req.msg.data = datai;

    if (ioctl(fd, IPMICTL_SEND_COMMAND, &req) < 0) {
	log_errno("Could not send IPMI command");
	goto fail;
    }

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    timeout.tv_usec = 0;
    timeout.tv_sec  = 5; // Setting timeout value to 5 secs

    if(( rv = select(fd+1, &rset, NULL, NULL, &timeout)) == -1) {
	log_errno("Failed to receive response from IPMI interface");
	goto fail;
    }

    if(rv == 0) {
	log_msg("Service Processor is not responding to messages");
	goto fail;
    }

    if (datao && *datao_len)
	memset(datao, 0, *datao_len);
    recv.msg.data = datao;
    recv.msg.data_len = *datao_len;
    recv.addr = (unsigned char *) &addr;
    recv.addr_len = sizeof(addr);

    if ((rv = ioctl(fd, IPMICTL_RECEIVE_MSG_TRUNC, &recv)) == -1) {
	log_errno("Could not receive IPMI response");
	goto fail;
    }

    *datao_len = recv.msg.data_len;
    
    // Response validattion
    if (recv.msgid != MSGID || recv.recv_type != IPMI_RESPONSE_RECV_TYPE) {
	log_msg("Could not understand IPMI response from SP");
	goto fail;
    }

    close(fd);
    return 0;

 fail:
    if (fd > 0)
	close(fd);
    return -1;
}
