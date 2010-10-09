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

#ifndef OIPMI_H
#define OIPMI_H

#include <stdint.h>
#include "mce.h"

struct mce;

void oipmi_init();
void oipmi_exit();
void oipmi_main_loop();
int  oipmi_process_mce(struct mce*);
int  oipmi_test(uint64_t address, u32 syndrome);


typedef struct {
    uint8_t   evm_rev;
    uint8_t   sensor_type;
    uint8_t   sensor_num;
    uint8_t   event_type : 7;
    uint8_t   event_dir  : 1;
    uint8_t   event_data[3];
} ipmi_sel_t;

#define MSGID 0x222

#define IPMI_SENSOR_EVENT 0x04
#define IPMI_CMD_REQUEST_EVENT 0x02

#define IPMI_DEV "/dev/ipmi0"


// The following definitions are taken from linux/ipmi.h. They are
// replicated here maintly for easy of building and packaging, as
// older distributions such as RHEL4 do not ship the ipmi.h linux
// kernel header file in a location that is convenient for inclusing
// (unlike RHEL5's "kernel-headers" RPM).


// When the address is not used, the type will be set to this value.
// The channel is the BMC's channel number for the channel (usually
// 0), or IPMC_BMC_CHANNEL if communicating directly with the BMC.
#define IPMI_SYSTEM_INTERFACE_ADDR_TYPE 0x0c

#define IPMI_BMC_CHANNEL         0xf
#define IPMI_MAX_ADDR_SIZE       32
#define IPMI_RESPONSE_RECV_TYPE  1

// The magic IOCTL value for this interface.
#define IPMI_IOC_MAGIC 'i'

// A raw IPMI message without any addressing. This covers both
// commands and responses. The completion code is always the first
// byte of data in the response (as the spec shows the messages laid
// out).
 
struct ipmi_msg
{
    unsigned char  netfn;
    unsigned char  cmd;
    unsigned short data_len;
    unsigned char  *data;
};

// Messages sent to the interface are this format.

struct ipmi_req
{
    // Address to send the message to.
    unsigned char *addr;
    unsigned int  addr_len;

    // The sequence number for the message. This exact value will be
    // reported back in the response to this request if it is a
    // command. If it is a response, this will be used as the sequence
    // value for the response.
    long msgid;

    struct ipmi_msg msg;
};

// Messages received from the interface are this format.

struct ipmi_recv
{
    // Is this a command, response or an asyncronous event.
    int recv_type;

    // Address the message was from is put here.  The caller must
    // supply the memory.
    unsigned char *addr;

    /// The size of the address buffer.The caller supplies the full
    /// buffer length, this value is updated to the actual message
    /// length when the message is received.
    unsigned int  addr_len;

    // The sequence number specified in the request of this is a
    // response.  If this is a command, this will be the sequence
    // number from the command.
    long msgid;

    // The data field must point to a buffer. The data_size field must
    // be set to the size of the message buffer. The caller supplies
    // the full buffer length, this value is updated to the actual
    // message length when the message is received.
    struct ipmi_msg msg;
};

struct ipmi_addr
{
    // Try to take these from the "Channel Medium Type" table in
    // section 6.5 of the IPMI 1.5 manual.
    int   addr_type;
    short channel;
    char  data[IPMI_MAX_ADDR_SIZE];
};

struct ipmi_system_interface_addr
{
    int           addr_type;
    short         channel;
    unsigned char lun;
};



// Send a message to the interfaces.  error values are:
//    - EFAULT - an address supplied was invalid.
//    - EINVAL - The address supplied was not valid, or the command
//               was not allowed.
//    - EMSGSIZE - The message to was too large.
//    - ENOMEM - Buffers could not be allocated for the command.
 
#define IPMICTL_SEND_COMMAND _IOR(IPMI_IOC_MAGIC, 13, struct ipmi_req)


#define IPMICTL_RECEIVE_MSG_TRUNC _IOWR(IPMI_IOC_MAGIC, 11, struct ipmi_recv)

#endif
