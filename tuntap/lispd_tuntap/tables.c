/*
 * tables.c
 *
 * Support and storage for LISP EID maps and other tables.
 *
 *
 * Copyright (C) 2009-2012 Cisco Systems, Inc, 2012. All rights reserved.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Please send any bug reports or fixes you make to the email address(es):
 *    LISP-MN developers <devel@lispmob.org>
 *
 * Written or modified by:
 *    Chris White       <chris@logicalelegance.com>
 *    David Meyer       <dmm@cisco.com>
 *
 */

#include "tables.h"

#define SECS_PER_MIN 60
//#define DEBUG_HASHES

/*
 * Tables
 */
patricia_tree_t *AF4_eid_cache;
patricia_tree_t *AF6_eid_cache;

patricia_tree_t *AF4_eid_db;
patricia_tree_t *AF6_eid_db;

// A populated count to bit table for lsb setup.
// This is for when we assume all locators are
// available when setting up an entry. i.e.
// 3 locators --> 0000000....00000111
int lsb_table[32 + 1];
void build_lsb_table(void);
void build_rloc_table(void);

/*
 * create_tables
 */
void create_tables(void)
{
  log_msg(INFO,  " Creating mapping tables...");
  
  AF4_eid_cache = New_Patricia(sizeof(struct in_addr) * 8);
  AF6_eid_cache = New_Patricia(sizeof(struct in6_addr) * 8);
  AF4_eid_db = New_Patricia(sizeof(struct in_addr) * 8);
  AF6_eid_db = New_Patricia(sizeof(struct in6_addr) * 8);

  if (!AF4_eid_cache || !AF6_eid_cache || !AF4_eid_db || !AF6_eid_db) {
    log_msg(INFO,  "    FAILED.");
  } else {
    log_msg(INFO,  "    Success.");
  }
  // XXX Replace with mutex // XXX Replace with mutex spin_lock_init(&table_lock);

  build_lsb_table();
  build_rloc_table();
}

/*
 * build_lsb_table()
 *
 * Build a lookup table for initial lsb setup.
 */
void build_lsb_table(void)
{
    int i, j;

    lsb_table[0] = 0;
    for (i = 1; i <= 32; i++) {
        lsb_table[i] = 0;
        for (j = 0; j < i; j++) {
            lsb_table[i] |= 1 << j;
        }
    }
}

/*
 * build_rloc_table()
 *
 * Build an ifindex to rloc mapping hash table for
 * advanced connection management.
 */
void build_rloc_table()
{
    int hashes = (1 << IFINDEX_HASH_BITS);
    int i;

    for (i = 0; i < hashes; i++) {
        // XXX globals.if_to_rloc_hash_table[i] = NULL;
    }
}

/*
 * lookup_eid_cache_v4()
 *
 * Look up a given ipv4 eid in the cache, returning true and
 * filling in the entry pointer if found, or false if not found.
 */
int lookup_eid_cache_v4(int eid, lisp_map_cache_t **entry)
{
  patricia_node_t *result;
  prefix_t prefix;
  
  // XXX Replace with mutex spin_lock_bh(&table_lock);
  prefix.family = AF_INET;
  prefix.bitlen = 32;
  prefix.ref_count = 0;
  prefix.add.sin.s_addr = eid;

  result = patricia_search_best(AF4_eid_cache, &prefix);
  if (!result) {
    // XXX Replace with mutex spin_unlock_bh(&table_lock);
    return(0);
  }

  *entry = (lisp_map_cache_t *)(result->data);
  // XXX Replace with mutex spin_unlock_bh(&table_lock);
  return(1);
}

/*
 * lookup_eid_cache_v4_exact()
 *
 * Find an exact match for a prefix/prefixlen if possible
 */
int lookup_eid_cache_v4_exact(int eid, int prefixlen, lisp_map_cache_t **entry)
{
  patricia_node_t *result;
  prefix_t prefix;

  // XXX Replace with mutex spin_lock_bh(&table_lock);
  
  prefix.family = AF_INET;
  prefix.bitlen = prefixlen;
  prefix.ref_count = 0;
  prefix.add.sin.s_addr = eid;

  result = patricia_search_exact(AF4_eid_cache, &prefix);
  if (!result) {
    // XXX Replace with mutex spin_unlock_bh(&table_lock);
    *entry = NULL;
    return(0);
  }

  *entry = (lisp_map_cache_t *)(result->data);
  // XXX Replace with mutex spin_unlock_bh(&table_lock);
  return(1);
}

/*
 * lookup_eid_cache_v6_exact()
 *
 * Find an exact match for a prefix/prefixlen if possible
 */
int lookup_eid_cache_v6_exact(lisp_addr_t eid_prefix, int prefixlen, lisp_map_cache_t **entry)
{
  patricia_node_t *result;
  prefix_t prefix;
  
  // XXX Replace with mutex spin_lock_bh(&table_lock);
  prefix.family = AF_INET6;
  prefix.bitlen = prefixlen;
  prefix.ref_count = 0;
  memcpy(prefix.add.sin6.s6_addr, eid_prefix.address.ipv6.s6_addr, 
     sizeof(struct in6_addr));

  result = patricia_search_exact(AF6_eid_cache, &prefix);
  if (!result) {
    // XXX Replace with mutex spin_unlock_bh(&table_lock);
    *entry = NULL;
    return(0);
  }

  *entry = (lisp_map_cache_t *)(result->data);
  // XXX Replace with mutex spin_unlock_bh(&table_lock);
  return(1);
}

/*
 * lookup_eid_cache_v6()
 *
 * Find a longest match for a prefix if possible
 */
int lookup_eid_cache_v6(lisp_addr_t eid, lisp_map_cache_t **entry)
{
  patricia_node_t *result;
  prefix_t prefix;
  
  // XXX Replace with mutex spin_lock_bh(&table_lock);
  prefix.family = AF_INET6;
  prefix.bitlen = 128;
  prefix.ref_count = 0;
  memcpy(prefix.add.sin6.s6_addr, eid.address.ipv6.s6_addr, 
     sizeof(struct in6_addr));

  result = patricia_search_best(AF6_eid_cache, &prefix);
  if (!result) {
    // XXX Replace with mutex spin_unlock_bh(&table_lock);
    return(0);
  }

  *entry = (lisp_map_cache_t *)(result->data);
  // XXX Replace with mutex spin_unlock_bh(&table_lock);
  return(1);
}

/*
 * lookup_eid_db_v4()
 *
 * Look up a given ipv4 eid in the database, returning true and
 * filling in the entry pointer if found, or false if not found.
 */
int lookup_eid_db_v4(int eid, lisp_database_entry_t **entry)
{
  patricia_node_t *result;
  prefix_t prefix;
  
  prefix.family = AF_INET;
  prefix.bitlen = 32;
  prefix.ref_count = 0;
  prefix.add.sin.s_addr = eid;

  // XXX Replace with mutex spin_lock_bh(&table_lock);
  result = patricia_search_best(AF4_eid_db, &prefix);
  if (!result) {
    // XXX Replace with mutex spin_unlock_bh(&table_lock);
    return(0);
  }

  *entry = (lisp_database_entry_t *)(result->data);
  // XXX Replace with mutex spin_unlock_bh(&table_lock);
  return(1);
}

/*
 * lookup_eid_db_v4_exact()
 *
 * Look up a given ipv4 eid/prefixlen in the database, returning true and
 * filling in the entry pointer if found, or false if not found.
 */
int lookup_eid_db_v4_exact(int eid, int prefixlen, lisp_database_entry_t **entry)
{
  patricia_node_t *result;
  prefix_t prefix;
  
  prefix.family = AF_INET;
  prefix.bitlen = prefixlen;
  prefix.ref_count = 0;
  prefix.add.sin.s_addr = eid;
  // XXX Replace with mutex spin_lock_bh(&table_lock);
  result = patricia_search_exact(AF4_eid_db, &prefix);
  if (!result) {
    // XXX Replace with mutex spin_unlock_bh(&table_lock);
    *entry = NULL;
    return(0);
  }

  *entry = (lisp_database_entry_t *)(result->data);
  // XXX Replace with mutex spin_unlock_bh(&table_lock);
  return(1);
}

/*
 * lookup_eid_db_v6_exact()
 *
 * Find an exact match for a prefix/prefixlen if possible in the
 * database, using an ipv6 prefix.
 */
int lookup_eid_db_v6_exact(lisp_addr_t eid_prefix, int prefixlen, lisp_database_entry_t **entry)
{
  patricia_node_t *result;
  prefix_t prefix;
  
  // XXX Replace with mutex spin_lock_bh(&table_lock);
  prefix.family = AF_INET6;
  prefix.bitlen = prefixlen;
  prefix.ref_count = 0;
  memcpy(prefix.add.sin6.s6_addr, eid_prefix.address.ipv6.s6_addr, 
     sizeof(struct in6_addr));

  result = patricia_search_exact(AF6_eid_db, &prefix);
  if (!result) {
    // XXX Replace with mutex spin_unlock_bh(&table_lock);
    *entry = NULL;
    return(0);
  }

  *entry = (lisp_database_entry_t *)(result->data);
  // XXX Replace with mutex spin_unlock_bh(&table_lock);
  return(1);
}

/*
 * eid_entry_expiration()
 *
 * Called when the timer associated with an EID entry expires.
 */
int eid_entry_expiration(timer *t, void *arg)
{
  lisp_map_cache_t *entry = (lisp_map_cache_t *)arg;
  
  log_msg(INFO,  "Got expiration for EID %pi4", &entry->eid_prefix.address.ip.s_addr);
  del_eid_cache_entry(entry->eid_prefix, entry->eid_prefix_length);
  return(0);
}

/*
 * eid_entry_probe_sample()
 *
 * Called when the sampling timer expires, send a sample
 * of the EID cache entry to lispd.
 */
int eid_entry_probe_sample(timer *t, void *arg)
{
    lisp_map_cache_t *entry = (lisp_map_cache_t *)arg;

    // XXX Replace with mutex spin_lock_bh(&table_lock);
    log_msg(INFO,  "Sending sample of EID %pi4 for probe", &entry->eid_prefix.address.ip.s_addr);
    //send_cache_sample_notification(entry, ProbeSample);

    /*
     * Restart the timer
     */
    //start_timer(t, entry->sampling_interval, entry->sampling_interval, arg);
    // XXX Replace with mutex spin_unlock_bh(&table_lock);
    return(0);
}

/*
 * eid_entry_smr_sample()
 *
 * Called when the sampling timer expires, send a sample
 * of the EID cache entry to lispd.
 */
int eid_entry_smr_sample(timer *t, void *arg)
{
    lisp_map_cache_t *entry = (lisp_map_cache_t *)arg;

    // XXX Replace with mutex spin_lock_bh(&table_lock);
    if (entry->active_within_period) {
        log_msg(INFO,  "Sending sample of EID %pi4 for SMR", &entry->eid_prefix.address.ip.s_addr);
   //     send_cache_sample_notification(entry, SMRSample);
    }
    // XXX Replace with mutex spin_unlock_bh(&table_lock);
    return(0);
}

/*
 * add_db_entry_locator
 *
 * Add a new locator to a database entry. If the given locator
 * is already in the list, return the pointer to that list
 * member, otherwise allocate a new one.
 */
void add_db_entry_locator(lisp_database_entry_t *db_entry,
                          lisp_db_add_msg_loc_t *locator_list,
                          int index)
{
  lisp_database_loc_t *locator_entry;
  lisp_db_add_msg_loc_t *new_locator =
          locator_list + index;

  locator_entry = db_entry->locator_list[index];

  if (!locator_entry) {
      log_msg(INFO,  "No existing locator at index %d, creating...\n", index);

    /*
     * Allocate a new locator
     */
      locator_entry  = (lisp_database_loc_t *)malloc(sizeof(lisp_database_loc_t));

      if (!locator_entry) {
          log_msg(INFO,  "Couldn't allocate lisp_database_loctype");
          return;
      }
      db_entry->locator_list[index] = locator_entry;
  }

  /*
   * Copy in the fields
   */
  memset(locator_entry, 0, sizeof(lisp_database_loc_t));
  locator_entry->priority  = new_locator->priority;
  locator_entry->weight    = new_locator->weight;
  locator_entry->mpriority = new_locator->mpriority;
  memcpy(&locator_entry->locator, &new_locator->locator, sizeof(lisp_addr_t));

  log_msg(INFO, "   Added locator %pI4 (%pI4)", &locator_entry->locator.address.ip,
         &new_locator->locator.address.ip);
}

/*
 * add_eid_db_entry()
 *
 * Add an EID/Locator mapping to the database for ipv4
 */
void add_eid_db_entry(lisp_db_add_msg_t *entry)
{
  patricia_node_t       *node;
  lisp_database_entry_t *db_entry;
  struct timeval         timestamp;
  int afi = entry->eid_prefix.afi;
  int entry_exists = 0;
  int locator_idx;

  if (afi == AF_INET) {
      log_msg(INFO,  " Adding database mapping: 0x%x/%d\n",
             entry->eid_prefix.address.ip.s_addr,
             entry->eid_prefix_length);
      lookup_eid_db_v4_exact(entry->eid_prefix.address.ip.s_addr,
                             entry->eid_prefix_length, &db_entry);

  } else {
      log_msg(INFO,  " Adding database mapping: ipv6/%d\n",
             entry->eid_prefix_length);
      lookup_eid_db_v6_exact(entry->eid_prefix,
                             entry->eid_prefix_length, &db_entry);
  }

  log_msg(INFO, "here 2...");
  /*
   * Check for existing entry
   */
  if (!db_entry) {

      db_entry = (lisp_database_entry_t *)malloc(sizeof(lisp_database_entry_t));

      if (!db_entry) {
          log_msg(INFO,  "Couldn't allocate db_entry");
          return;               /* XXX: correct? */
      }

      memset(db_entry, 0, sizeof(lisp_database_entry_t));
      if (afi == AF_INET) {
          db_entry->eid_prefix.address.ip.s_addr = entry->eid_prefix.address.ip.s_addr;
      } else {
          memcpy(&db_entry->eid_prefix, &entry->eid_prefix, sizeof(lisp_addr_t));
      }
      db_entry->eid_prefix_length       = entry->eid_prefix_length;
      db_entry->eid_prefix.afi          = afi;
      gettimeofday(&timestamp, NULL);
      db_entry->timestamp               = timestamp.tv_sec;
      log_msg(INFO,  "  Entry not found, creating new entry");
  } else {
      log_msg(INFO,  "  Existing entry found, replacing locator list");
      entry_exists = 1;
  }

  /*
   * Add the locators
   */
  for (locator_idx = 0; locator_idx < MAX_LOCATORS; locator_idx++) {
      if (locator_idx < entry->count) {
          add_db_entry_locator(db_entry, (lisp_db_add_msg_loc_t *)entry->locators,
                               locator_idx);
      } else if (db_entry->locator_list[locator_idx]) {

          /*
           * Free old locators beyond what's current
           */
          free(db_entry->locator_list[locator_idx]);
          db_entry->locator_list[locator_idx] = NULL;
      }
  }
  db_entry->count                   = entry->count;

  /* Insert new prefix if not found */
  if (!entry_exists) {
      // XXX Replace with mutex spin_lock_bh(&table_lock);
      if (afi == AF_INET) {
          node = make_and_lookup_v4(AF4_eid_db, entry->eid_prefix.address.ip.s_addr,
                                    entry->eid_prefix_length);
      } else {
          node = make_and_lookup_v6(AF6_eid_db, entry->eid_prefix.address.ipv6,
                                    entry->eid_prefix_length);
      }
      if (!node) {
          log_msg(INFO,  " Failed to allocate EID db tree node");
          // XXX Replace with mutex spin_unlock_bh(&table_lock);
          return;
      }
      node->data = db_entry;
      // XXX Replace with mutex spin_unlock_bh(&table_lock);
  }
}

/*
 * update_locator_hash_table()
 *
 * Update the hash table used to assign packets to locators
 * based on priority and weight. Adapted from the NX/OS
 * code.
 */
void update_locator_hash_table(lisp_map_cache_t *entry)
{
    lisp_map_cache_loc_t *loc;
    int i, j, best_priority = 255;
    int percent_left = 100;
    int loc_count = 0;
    int table_index, weight;
    unsigned char used_locs[MAX_LOCATORS];

    log_msg(INFO,  "Recomputing locator hash table for %pI4/%d\n",
           &entry->eid_prefix, entry->eid_prefix_length);

    memset(used_locs, 0, sizeof(unsigned char) * MAX_LOCATORS);

    /*
     * Find the highest priority RLOCS that are up.
     */
    for (i = 0; i < MAX_LOCATORS; i++) {
        loc = entry->locator_list[i];
        if (!loc || (loc->state == 0)) {
            continue;
        }
        if (loc->priority < best_priority) {
            best_priority = loc->priority;
        }
    }

    /*
     * Build a set of locators that we are going to use.
     * This is at the priority established above, up, etc.
     * (Can these two ops be merged?)
     */
    for (i = 0; i < MAX_LOCATORS; i++) {
        loc = entry->locator_list[i];

        // Match criteria
        if (!loc || (loc->priority != best_priority) || (loc->state == 0)) {
            continue;
        }

        // Out of available weight? Then we're done.
        if ((percent_left - loc->weight) < 0) {
            break;
        }
        percent_left -= loc->weight;

        used_locs[loc_count] = i;
        loc_count++;
    }

    /*
     * Failed to find any usable? Set the buckets to invalid
     */
    if (!loc_count) {
        for (i = 0; i < LOC_HASH_SIZE; i++) {
            entry->locator_hash_table[i] = -1;
        }
        log_msg(INFO,  "   No usable locators found, hash table invalidated.");
        return;
    }

    /*
     * Divide up the remaining weight amongst the
     * locators we're using. This is in case some of
     * the best priority locators were down or the sum
     * of weights was not 100.
     */
    percent_left = percent_left / loc_count;

    /*
     * Fill in the hash table for each locator, up to the
     * weight it has been alloted.
     */
    table_index = 0;
    for (i = 0; i < loc_count; i++) {
        weight = (percent_left + entry->locator_list[used_locs[i]]->weight) >> 2;
        for (j = table_index; j < (table_index + weight); j++) {
            entry->locator_hash_table[j] = used_locs[i];
        }
        table_index += weight;
    }

    /*
     * Fill up any remaining space in the hash table
     * with the last locator.
     */
    if (table_index != LOC_HASH_SIZE) {
        for (i = table_index; i < LOC_HASH_SIZE; i++) {
            entry->locator_hash_table[i] = used_locs[loc_count - 1];
        }
    }

#ifdef DEBUG_HASHES
    char hash_str[LOC_HASH_SIZE + 1];
    for (i = 0; i < LOC_HASH_SIZE; i++) {
        hash_str[i] = (char)(entry->locator_hash_table[i]) + 0x30; // Quick convesion
    }
    hash_str[i] = '\0';
    printk("     New hash table: %s\n", hash_str);
#endif
}

/*
 * add_cache_entry_locator
 *
 * Add a new locator to a cache entry. If the given locator
 * is already in the list, return the pointer to that list
 * member, otherwise allocate a new one.
 */
void add_cache_entry_locator(lisp_map_cache_t *cache_entry,
                                              lisp_eid_map_msg_loc_t *locator_list,
                                              int index)
{
  lisp_map_cache_loc_t *locator_entry;
  lisp_eid_map_msg_loc_t *new_locator =
          locator_list + index;
  int input_count = 0, output_count = 0;

  locator_entry = cache_entry->locator_list[index];

  if (!locator_entry) {
      log_msg(INFO,  "No existing locator at index %d, creating...\n", index);

    /*
     * Allocate a new locator
     */
      locator_entry  = (lisp_map_cache_loc_t *)malloc(sizeof(lisp_map_cache_loc_t));

      if (!locator_entry) {
          log_msg(INFO,  "Couldn't allocate locator entry\n");
          return;
      }
      memset(locator_entry, 0, sizeof(lisp_map_cache_loc_t));
      cache_entry->locator_list[index] = locator_entry;
  }

  /*
   * Copy in the fields
   */
  if (!memcmp(&locator_entry->locator, &new_locator->locator, sizeof(lisp_addr_t))) {
      // Preserve the counters
      input_count = locator_entry->data_packets_in;
      output_count = locator_entry->data_packets_out;
  }

  memset(locator_entry, 0, sizeof(lisp_map_cache_loc_t));
  locator_entry->data_packets_in = input_count;
  locator_entry->data_packets_out = output_count;
  locator_entry->priority  = new_locator->priority;
  locator_entry->weight    = new_locator->weight;
  locator_entry->mpriority = new_locator->mpriority;
  locator_entry->reachability_alg = 0;  // Undef XXX
  locator_entry->state = 1;

  memcpy(&locator_entry->locator, &new_locator->locator, sizeof(lisp_addr_t));
}


/* 
 * del_eid_cache_entry() 
 *
 * Delete an EID mapping from the cache
 */
void del_eid_cache_entry(lisp_addr_t eid,
             int prefixlen)
{
  lisp_map_cache_loc_t *locator;
  lisp_map_cache_t     *entry;
  patricia_node_t      *result;
  prefix_t              prefix;
  int                   loc_idx = 0;

  prefix.family = AF_INET;
  prefix.bitlen = prefixlen;
  prefix.ref_count = 0;
  prefix.add.sin.s_addr = eid.address.ip.s_addr;
  // XXX Replace with mutex spin_lock_bh(&table_lock);
  result = patricia_search_exact(AF4_eid_cache, &prefix);
  if (!result) {
      log_msg(INFO,  "   Unable to locate cache entry %pi4 for deletion",
             &eid.address.ip.s_addr);
      // XXX Replace with mutex spin_unlock_bh(&table_lock);
      return;
  } else {
      log_msg(INFO,  "   Deleting map cache EID entry %pi4", &eid.address.ip.s_addr);
  }
  
  /*
   * Remove the entry from the trie
   */
  entry = (lisp_map_cache_t *)(result->data);
  patricia_remove(AF4_eid_cache, result);

  /*
    * Free the locators
    */
  for (loc_idx = 0; loc_idx < MAX_LOCATORS; loc_idx++) {
      locator = entry->locator_list[loc_idx];
      if (locator) {
          free(locator);
      }
  }

  /*
   * Free the entry
   */
  if (entry->how_learned) {
      // XXX del_timer(&entry->expiry_timer);
      // XXX del_timer(&entry->smr_timer);
  }
  // XXX del_timer(&entry->probe_timer);
  free(entry);
  // XXX Replace with mutex spin_unlock_bh(&table_lock);
}

/*
 * add_eid_cache_entry()
 *
 * Add an EID/locator mapping to the v4 map cache 
 */
void add_eid_cache_entry(lisp_eid_map_msg_t *entry)
{
  patricia_node_t       *node;
  lisp_map_cache_t      *map_entry;
  struct timeval         timestamp;
  int                    afi = entry->eid_prefix.afi;
  int                    locator_idx = 0;
  int                    entry_exists = 0;

  if (afi == AF_INET) {
      log_msg(INFO,  " Adding cache mapping: %pi4/%d\n",
             &entry->eid_prefix,
             entry->eid_prefix_length);
      lookup_eid_cache_v4_exact(entry->eid_prefix.address.ip.s_addr,
                                entry->eid_prefix_length, &map_entry);
  } else {
      log_msg(INFO,  " Adding cache mapping: %pi6/%d\n",
             entry->eid_prefix.address.ipv6.s6_addr,
             entry->eid_prefix_length);
      lookup_eid_cache_v6_exact(entry->eid_prefix,
                                entry->eid_prefix_length, &map_entry);
  }

  /*
   * Check for existing entry
   */
  if (!map_entry) {
      map_entry = (lisp_map_cache_t *)malloc(sizeof(lisp_map_cache_t));
      if (!map_entry) {
          log_msg(INFO,  "Couldn't allocate map_entry\n");
          return;               /* XXX: correct? */
      }

      memset(map_entry, 0, sizeof(lisp_map_cache_t));
      if (afi == AF_INET) {
          map_entry->eid_prefix.address.ip.s_addr    = entry->eid_prefix.address.ip.s_addr;
      } else {
          memcpy(&map_entry->eid_prefix, &entry->eid_prefix, sizeof(lisp_addr_t));
      }
      map_entry->eid_prefix_length               = entry->eid_prefix_length;
      map_entry->eid_prefix.afi                  = afi;
      map_entry->how_learned                     = entry->how_learned;
      map_entry->ttl                             = entry->ttl;
      map_entry->sampling_interval               = entry->sampling_interval;

      /*
       * If not static, start the expiration timers, and set up the smr timer.
       */
      if (map_entry->how_learned) {
          start_timer(&map_entry->expiry_timer, entry->ttl * SECS_PER_MIN, eid_entry_expiration, (void *)map_entry);
          // Jitter XXX
      } else {
          log_msg(INFO,  "Not starting expiration timer.");
      }

     /*
      * Start the sampling timer
      */
      if (entry->sampling_interval) {
          start_timer(&map_entry->probe_timer, entry->sampling_interval, eid_entry_probe_sample, (void *)map_entry);
          log_msg(INFO,  " Started probe sampling timer at %d second interval", entry->sampling_interval);
      }
      gettimeofday(&timestamp, NULL);
      map_entry->timestamp               = timestamp.tv_sec;

      log_msg(INFO,  "  Entry not found, creating new entry");
  } else {
    log_msg(INFO,  "  Existing entry found, replacing locator list");
    entry_exists = 1;

    // XXX NEED TO UPDATE TIMERS/TTL HERE???
  } 
  
  /* 
   * Create the locators list
   */

  // Negative cache entry??
  if (!entry->count) {
      map_entry->locators_present = 0;
      for (locator_idx = 0; locator_idx < MAX_LOCATORS; locator_idx++) {
          if (map_entry->locator_list[locator_idx]) {
              free(map_entry->locator_list[locator_idx]);
              map_entry->locator_list[locator_idx] = NULL;
          }
      }
  } else {  // Has locators
      map_entry->locators_present = 1;
      for (locator_idx = 0; locator_idx < MAX_LOCATORS; locator_idx++) {
          if (locator_idx < entry->count) {
              add_cache_entry_locator(map_entry, (lisp_eid_map_msg_loc_t *)entry->locators,
                                      locator_idx);
          } else if (map_entry->locator_list[locator_idx]) {

              /*
               * Free old locators beyond what's current
               */
              free(map_entry->locator_list[locator_idx]);
              map_entry->locator_list[locator_idx] = NULL;
          }
      }
  }
  map_entry->count                           = entry->count;

  // Update LSBs
  map_entry->lsb = lsb_table[map_entry->count];

  // Update weight table
  update_locator_hash_table(map_entry);

  /* Insert new prefix if it doesn't exist */
  if (!entry_exists) {
      // XXX Replace with mutex spin_lock_bh(&table_lock);
      if (afi == AF_INET) {
          node = make_and_lookup_v4(AF4_eid_cache, entry->eid_prefix.address.ip.s_addr,
                                    entry->eid_prefix_length);
      } else {
          node = make_and_lookup_v6(AF6_eid_cache, entry->eid_prefix.address.ipv6,
                                    entry->eid_prefix_length);
      }
      if (!node) {
          log_msg(INFO,  " Failed to allocate EID tree node");
          // XXX Replace with mutex spin_unlock_bh(&table_lock);
          return;
      }
      node->data = map_entry;
      // XXX Replace with mutex spin_unlock_bh(&table_lock);
  }
}

/*
 * update_locator_set_by_msg()
 *
 * For the EID in the returned cache sample message, go through
 * the list of locators and mark them according to the status
 * in the message. This is primarly as a result of RLOC probing,
 * but could be used by any liveness detection procedure at the
 * protocol level.
 */
void update_locator_set_by_msg(lisp_cache_sample_msg_t *msg) {
    int i, j;
    int loc_mask = 1;
    char loc_status = 0;
    lisp_map_cache_t      *map_entry = NULL;
    lisp_addr_t *loc_addr;
    lisp_map_cache_loc_t *locator = NULL;

    int status;

    /*
     * Lookup the EID and get the entry for it in the cache
     */
     switch (msg->eid.afi) {
     case AF_INET:
         status = lookup_eid_cache_v4_exact(msg->eid.address.ip.s_addr, msg->eid_prefix_length,
                                            &map_entry);
         break;
     case AF_INET6:
         status = lookup_eid_cache_v6_exact(msg->eid, msg->eid_prefix_length,
                                      &map_entry);
         break;
     default:
         log_msg(INFO,  "  Unknown EID AFI %d in update_locator_set_by_msg()",
                msg->eid.afi);
         return;
         break;
     }

     if (!map_entry || !status) {
         log_msg(INFO,  "  No such EID in map cache.");
         return;
     }

    for (i = 0; i < msg->num_locators; i++) {
        locator = NULL;
        loc_addr = &(msg->locators[i]);
        loc_status = !!(msg->status_bits & loc_mask);
        loc_mask = loc_mask << 1;

        /*
         * Match the locator to one in our entry.
         */
        for (j = 0; j < MAX_LOCATORS; j++) {
            if (map_entry->locator_list[j]) {
                if (!memcmp(&map_entry->locator_list[j]->locator,
                            loc_addr, sizeof(lisp_addr_t))) {
                    locator = map_entry->locator_list[j];
                }
            } else {
                break;
            }
        }

        if (!locator) {
            if (loc_addr->afi == AF_INET) {
                log_msg(INFO, "  Locator %pI4 not found in cache entry, skipping.\n",
                       &loc_addr->address.ip.s_addr);
            } else if (loc_addr->afi == AF_INET6) {
                log_msg(INFO, "  Locator %pI6 not found in cache entry, skipping.\n",
                       loc_addr->address.ipv6.s6_addr);
            } else {
                log_msg(INFO, "  Unknown AFI %d for locator, skipping.\n",
                       loc_addr->afi);
            }
            continue;
        }

        /*
         * Mark the locator as up or down as the message says.
         */
        if (loc_addr->afi == AF_INET) {
            log_msg(INFO, "Marking locator %pI4 as %s\n",
                   &locator->locator.address.ip.s_addr,
                   loc_status ? "up" : "down");
        } else if (loc_addr->afi == AF_INET6) {
            log_msg(INFO, "  Marking locator %pI6 as %s\n",
                   locator->locator.address.ipv6.s6_addr,
                   loc_status ? "up" : "down");
        }
        locator->state = loc_status;
    }
    update_locator_hash_table(map_entry);
}

/*
 * add_ifindex_to_rloc_mapping
 *
 * Add an rloc to our interface to rloc map
 */
#if 0
void add_ifindex_to_rloc_mapping(int ifindex, lisp_addr_t *rloc)
{
    int hash_index = ifindex & ((1 << IFINDEX_HASH_BITS) - 1);
    rloc_map_entry_t *new_rloc;
    rloc_map_entry_t *hash_head = globals.if_to_rloc_hash_table[hash_index];
    int replace = 0;

    new_rloc = hash_head;

    // Check if we're replacing an existing index
    while (new_rloc) {
        if (new_rloc->ifindex == ifindex) {
            replace = 1;
            break;
        }
        new_rloc = new_rloc->next;
    }

    if (!replace) {
        new_rloc = (rloc_map_entry_t *)malloc(sizeof(rloc_map_entry_t));

        if (!new_rloc) {
            log_msg(INFO,  "Unable to allocate new rloc table entry.");
            return;
        }
        new_rloc->ifindex = ifindex;
        memcpy(&new_rloc->addr, rloc, sizeof(lisp_addr_t));

        if (!hash_head) {
            globals.if_to_rloc_hash_table[hash_index] = new_rloc;
            new_rloc->next = NULL;
        } else {
            new_rloc->next = hash_head;
        }
    } else {

        // Just update the existing entry
        new_rloc->ifindex = ifindex;
        memcpy(&new_rloc->addr, rloc, sizeof(lisp_addr_t));
    }

    if (ifindex != 0) {

        // Non-default ifindex implies we have multiple RLOCs
        globals.multiple_rlocs = 1;
    }
}
#endif
/*
 * free_database_entry
 */
void free_database_entry(void *entry)
{
    lisp_database_entry_t *db_entry = (lisp_database_entry_t *)entry;
    int i;

    /*
     * Free locators
     */
    for (i = 0; i < MAX_LOCATORS; i++) {
        if (db_entry->locator_list[i] != 0) {
            free(db_entry->locator_list[i]);
        }
    }
    free(db_entry);
}

/*
 * free_cache_entry
 */
void free_cache_entry(void *entry)
{
    lisp_map_cache_t       *map_entry;
    int i;

    map_entry = (lisp_map_cache_t *)entry;

    /*
     * Free locators
     */
    for (i = 0; i < MAX_LOCATORS; i++) {
        if (map_entry->locator_list[i] != 0) {
            free(map_entry->locator_list[i]);
        }
    }
    if (map_entry->how_learned) {
        // XXX del_timer(&map_entry->smr_timer);
        // XXX del_timer(&map_entry->expiry_timer);
    }
    // XXX del_timer(&map_entry->probe_timer);
    free(map_entry);
}

/*
 * clear_map_cache()
 *
 * Clear the entire contents of the map cache.
 * Could be performed, for example, when a roaming event occurs.
 *
 * Arguments unused. Provided to match command table.
 */
void clear_map_cache(void)
{
    patricia_node_t  *node;
    lisp_map_cache_t *entry;

    log_msg(INFO,  "Clearing map caches....");
    // XXX Replace with mutex spin_lock_bh(&table_lock);

    PATRICIA_WALK(AF4_eid_cache->head, node) {
       patricia_remove(AF4_eid_cache, node);
       entry = (lisp_map_cache_t *)node->data;
       free_cache_entry(entry);
    } PATRICIA_WALK_END;

    PATRICIA_WALK(AF6_eid_cache->head, node) {
       patricia_remove(AF6_eid_cache, node);
       entry = (lisp_map_cache_t *)node->data;
       free_cache_entry(entry);
    } PATRICIA_WALK_END;

    // XXX Replace with mutex spin_unlock_bh(&table_lock);
    log_msg(INFO,  "Done.");
}

/*
 * teardown_trees
 *
 * Remove all allocated trees and associated kernel memory
 */
void teardown_trees(void)
{
    // XXX Replace with mutex spin_lock_bh(&table_lock);
    Destroy_Patricia(AF4_eid_cache, free_cache_entry);
    log_msg(INFO,  "Destroyed ipv4 EID trie\n");

    Destroy_Patricia(AF6_eid_cache, free_cache_entry);
    log_msg(INFO,  "Destroyed ipv6 EID trie\n");

    Destroy_Patricia(AF4_eid_db, free_database_entry);
    log_msg(INFO,  "Destroyed ipv4 EID db\n");

    Destroy_Patricia(AF6_eid_db, free_database_entry);
    log_msg(INFO,  "Destroyed ipv6 EID db\n");
    // XXX Replace with mutex spin_unlock_bh(&table_lock);

    log_msg(INFO,  "Destroyed patricia structures\n");

}

/*
 * start_traffic_monitor()
 *
 * Mark each cache entry as "no traffic" in preparation
 * for traffic monitoring. If a packet comes in or goes out
 * to the EID later then the traffic bit gets set to true.
 */
void start_traffic_monitor(void)
{
    patricia_node_t *node;
    lisp_map_cache_t *entry;

    // XXX Replace with mutex spin_lock_bh(&table_lock);

    /*
     * Set all entries in the cache to start
     * traffic monitoring.
     */
    PATRICIA_WALK(AF4_eid_cache->head, node) {
        entry = (lisp_map_cache_t *)node->data;
        entry->active_within_period = 0;
        start_timer(&entry->smr_timer, TRAFFIC_MON_PERIOD, eid_entry_smr_sample, entry);
    } PATRICIA_WALK_END;

    PATRICIA_WALK(AF6_eid_cache->head, node) {
        entry = (lisp_map_cache_t *)node->data;
        entry->active_within_period = 0;
        start_timer(&entry->smr_timer, TRAFFIC_MON_PERIOD, eid_entry_smr_sample, entry);
    } PATRICIA_WALK_END;

    log_msg(INFO,  "Marked cache entries for traffic monitoring.");
    // XXX Replace with mutex spin_unlock_bh(&table_lock);
}
