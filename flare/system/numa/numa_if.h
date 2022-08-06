
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_SYSTEM_NUMA_NUMA_IF_H_
#define FLARE_SYSTEM_NUMA_NUMA_IF_H_

namespace flare::system_internal {

    extern long get_mempolicy(int *mode, unsigned long *nmask,
                              unsigned long maxnode, void *addr, unsigned flags);
    extern long mbind(void *start, unsigned long len, int mode,
                      const unsigned long *nmask, unsigned long maxnode, unsigned flags);
    extern long set_mempolicy(int mode, const unsigned long *nmask,
                              unsigned long maxnode);
    extern long migrate_pages(int pid, unsigned long maxnode,
                              const unsigned long *frommask,
                              const unsigned long *tomask);

    extern long move_pages(int pid, unsigned long count,
                           void **pages, const int *nodes, int *status, int flags);

#define MPOL_DEFAULT     0
#define MPOL_PREFERRED   1
#define MPOL_BIND        2
#define MPOL_INTERLEAVE  3
#define MPOL_LOCAL       4
#define MPOL_MAX         5

    /* Flags for get_mem_policy */
#define MPOL_F_NODE    (1<<0)   /* return next il node or node of address */
    /* Warning: MPOL_F_NODE is unsupported and
       subject to change. Don't use. */
#define MPOL_F_ADDR     (1<<1)  /* look up vma using address */
#define MPOL_F_MEMS_ALLOWED (1<<2) /* query nodes allowed in cpuset */

/* Flags for mbind */
#define MPOL_MF_STRICT  (1<<0)  /* Verify existing pages in the mapping */
#define MPOL_MF_MOVE	(1<<1)  /* Move pages owned by this process to conform to mapping */
#define MPOL_MF_MOVE_ALL (1<<2) /* Move every page to conform to mapping */

    bool    get_numa_supported();
    int32_t get_numa_max_possible_node();
    int32_t get_numa_max_node();

}  // namespace flare::system_internal

#endif  // FLARE_SYSTEM_NUMA_NUMA_IF_H_
