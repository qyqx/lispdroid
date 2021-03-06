/*
 * lisp_mod.h
 *
 * Declarations and constants for the LISP kernel module.
 *
 * Copyright 2010, Cisco Systems
 * Author: Chris White
 */

#pragma once

#include "linux/module.h"	
#include "linux/kernel.h"
#include "linux/netfilter.h"
#include "linux/netfilter_ipv4.h"
#include "linux/netlink.h"
#include "net/net_namespace.h"
#include "tables.h"
#include "lisp_ipc.h"
#include "lisp_ipc_kernel.h"
#include "lisp_input.h"
#include "lisp_output.h"
#include "lisp_slab.h"
#include "lib/patricia/patricia.h"

#define NETLINK_LISP 20  /* XXX Temporary, needs to be in /usr/include/linux/netlink.h */
#define IFINDEX_HASH_BITS 4

typedef struct {
  struct sock *nl_socket;       /* Netlink socket */
  struct nf_hook_ops netfilter_ops_in;  /* Netfilter hook definition, input */
  struct nf_hook_ops netfilter_ops_out; /* Netfilter hook definition, output */
  struct nf_hook_ops netfilter_ops_out6; /* "" For ipv6 */
  int    always_encap;         /* Always LISP encapsulate? */
  int    multiple_rlocs;       /* Use multiple RLOCs */
  ushort my_rloc_af;
  ushort udp_encap_port;
  ushort udp_control_port;
  unsigned int instance_id;
  int   use_instance_id;
  int   daemonPID; /* Process ID for lispd */
  // RLOC to if_index table, used by lisp connection manager
  rloc_map_entry_t *if_to_rloc_hash_table[1 << IFINDEX_HASH_BITS];
} lisp_globals;

