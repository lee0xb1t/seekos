#ifndef __paging_h__
#define __paging_h__

#include <lib/ktypes.h>


/**
 * @brief Format of a 4-Level PML4 Entry (PML4E) that References a Page-Directory-Pointer Table
 */
typedef union
{
  struct
  {
    /**
     * [Bit 0] Present; must be 1 to reference a page-directory-pointer table.
     */
    u64 present                                                 : 1;
#define PML4E_64_PRESENT_BIT                                         0
#define PML4E_64_PRESENT_FLAG                                        0x01
#define PML4E_64_PRESENT_MASK                                        0x01
#define PML4E_64_PRESENT(_)                                          (((_) >> 0) & 0x01)

    /**
     * [Bit 1] Read/write; if 0, writes may not be allowed to the 512-GByte region controlled by this entry.
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 write                                                   : 1;
#define PML4E_64_WRITE_BIT                                           1
#define PML4E_64_WRITE_FLAG                                          0x02
#define PML4E_64_WRITE_MASK                                          0x01
#define PML4E_64_WRITE(_)                                            (((_) >> 1) & 0x01)

    /**
     * [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 512-GByte region controlled by this entry.
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 supervisor                                              : 1;
#define PML4E_64_SUPERVISOR_BIT                                      2
#define PML4E_64_SUPERVISOR_FLAG                                     0x04
#define PML4E_64_SUPERVISOR_MASK                                     0x01
#define PML4E_64_SUPERVISOR(_)                                       (((_) >> 2) & 0x01)

    /**
     * [Bit 3] Page-level write-through; indirectly determines the memory type used to access the page-directory-pointer table
     * referenced by this entry.
     *
     * @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
     */
    u64 write_through                                   : 1;
#define PML4E_64_PAGE_LEVEL_WRITE_THROUGH_BIT                        3
#define PML4E_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                       0x08
#define PML4E_64_PAGE_LEVEL_WRITE_THROUGH_MASK                       0x01
#define PML4E_64_PAGE_LEVEL_WRITE_THROUGH(_)                         (((_) >> 3) & 0x01)

    /**
     * [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the page-directory-pointer table
     * referenced by this entry.
     *
     * @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
     */
    u64 cache_disable                                   : 1;
#define PML4E_64_PAGE_LEVEL_CACHE_DISABLE_BIT                        4
#define PML4E_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                       0x10
#define PML4E_64_PAGE_LEVEL_CACHE_DISABLE_MASK                       0x01
#define PML4E_64_PAGE_LEVEL_CACHE_DISABLE(_)                         (((_) >> 4) & 0x01)

    /**
     * [Bit 5] Accessed; indicates whether this entry has been used for linear-address translation.
     *
     * @see Vol3A[4.8(Accessed and Dirty Flags)]
     */
    u64 accessed                                                : 1;
#define PML4E_64_ACCESSED_BIT                                        5
#define PML4E_64_ACCESSED_FLAG                                       0x20
#define PML4E_64_ACCESSED_MASK                                       0x01
#define PML4E_64_ACCESSED(_)                                         (((_) >> 5) & 0x01)
    u64 reserved1                                               : 1;

    /**
     * [Bit 7] Reserved (must be 0).
     */
    u64 must_be_zero                                              : 1;
#define PML4E_64_MUST_BE_ZERO_BIT                                    7
#define PML4E_64_MUST_BE_ZERO_FLAG                                   0x80
#define PML4E_64_MUST_BE_ZERO_MASK                                   0x01
#define PML4E_64_MUST_BE_ZERO(_)                                     (((_) >> 7) & 0x01)

    /**
     * [Bits 10:8] Ignored.
     */
    u64 ignored1                                                : 3;
#define PML4E_64_IGNORED_1_BIT                                       8
#define PML4E_64_IGNORED_1_FLAG                                      0x700
#define PML4E_64_IGNORED_1_MASK                                      0x07
#define PML4E_64_IGNORED_1(_)                                        (((_) >> 8) & 0x07)

    /**
     * [Bit 11] For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with
     * ordinary paging)
     *
     * @see Vol3A[4.5.5(Restart of HLAT Paging)]
     */
    u64 restart                                                 : 1;
#define PML4E_64_RESTART_BIT                                         11
#define PML4E_64_RESTART_FLAG                                        0x800
#define PML4E_64_RESTART_MASK                                        0x01
#define PML4E_64_RESTART(_)                                          (((_) >> 11) & 0x01)

    /**
     * [Bits 47:12] Physical address of 4-KByte aligned page-directory-pointer table referenced by this entry.
     */
    u64 page_frame                                         : 36;
#define PML4E_64_PAGE_FRAME_NUMBER_BIT                               12
#define PML4E_64_PAGE_FRAME_NUMBER_FLAG                              0xFFFFFFFFF000
#define PML4E_64_PAGE_FRAME_NUMBER_MASK                              0xFFFFFFFFF
#define PML4E_64_PAGE_FRAME_NUMBER(_)                                (((_) >> 12) & 0xFFFFFFFFF)
    u64 reserved2                                               : 4;

    /**
     * [Bits 62:52] Ignored.
     */
    u64 ignored2                                                : 11;
#define PML4E_64_IGNORED_2_BIT                                       52
#define PML4E_64_IGNORED_2_FLAG                                      0x7FF0000000000000
#define PML4E_64_IGNORED_2_MASK                                      0x7FF
#define PML4E_64_IGNORED_2(_)                                        (((_) >> 52) & 0x7FF)

    /**
     * [Bit 63] If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 512-GByte region
     * controlled by this entry); otherwise, reserved (must be 0).
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 execute_disable                                          : 1;
#define PML4E_64_EXECUTE_DISABLE_BIT                                 63
#define PML4E_64_EXECUTE_DISABLE_FLAG                                0x8000000000000000
#define PML4E_64_EXECUTE_DISABLE_MASK                                0x01
#define PML4E_64_EXECUTE_DISABLE(_)                                  (((_) >> 63) & 0x01)
  };

  u64 as_u64;
} pml4e_t;


/**
 * @brief Format of a 4-Level Page-Directory-Pointer-Table Entry (PDPTE) that References a Page Directory
 */
typedef union
{
  struct
  {
    /**
     * [Bit 0] Present; must be 1 to reference a page directory.
     */
    u64 present                                                 : 1;
#define PDPTE_64_PRESENT_BIT                                         0
#define PDPTE_64_PRESENT_FLAG                                        0x01
#define PDPTE_64_PRESENT_MASK                                        0x01
#define PDPTE_64_PRESENT(_)                                          (((_) >> 0) & 0x01)

    /**
     * [Bit 1] Read/write; if 0, writes may not be allowed to the 1-GByte region controlled by this entry.
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 write                                                   : 1;
#define PDPTE_64_WRITE_BIT                                           1
#define PDPTE_64_WRITE_FLAG                                          0x02
#define PDPTE_64_WRITE_MASK                                          0x01
#define PDPTE_64_WRITE(_)                                            (((_) >> 1) & 0x01)

    /**
     * [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 1-GByte region controlled by this entry.
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 supervisor                                              : 1;
#define PDPTE_64_SUPERVISOR_BIT                                      2
#define PDPTE_64_SUPERVISOR_FLAG                                     0x04
#define PDPTE_64_SUPERVISOR_MASK                                     0x01
#define PDPTE_64_SUPERVISOR(_)                                       (((_) >> 2) & 0x01)

    /**
     * [Bit 3] Page-level write-through; indirectly determines the memory type used to access the page directory referenced by
     * this entry.
     *
     * @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
     */
    u64 write_through                                   : 1;
#define PDPTE_64_PAGE_LEVEL_WRITE_THROUGH_BIT                        3
#define PDPTE_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                       0x08
#define PDPTE_64_PAGE_LEVEL_WRITE_THROUGH_MASK                       0x01
#define PDPTE_64_PAGE_LEVEL_WRITE_THROUGH(_)                         (((_) >> 3) & 0x01)

    /**
     * [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the page directory referenced by
     * this entry.
     *
     * @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
     */
    u64 cache_disable                                   : 1;
#define PDPTE_64_PAGE_LEVEL_CACHE_DISABLE_BIT                        4
#define PDPTE_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                       0x10
#define PDPTE_64_PAGE_LEVEL_CACHE_DISABLE_MASK                       0x01
#define PDPTE_64_PAGE_LEVEL_CACHE_DISABLE(_)                         (((_) >> 4) & 0x01)

    /**
     * [Bit 5] Accessed; indicates whether this entry has been used for linear-address translation.
     *
     * @see Vol3A[4.8(Accessed and Dirty Flags)]
     */
    u64 accessed                                                : 1;
#define PDPTE_64_ACCESSED_BIT                                        5
#define PDPTE_64_ACCESSED_FLAG                                       0x20
#define PDPTE_64_ACCESSED_MASK                                       0x01
#define PDPTE_64_ACCESSED(_)                                         (((_) >> 5) & 0x01)
    u64 reserved1                                               : 1;

    /**
     * [Bit 7] Page size; must be 0 (otherwise, this entry maps a 1-GByte page).
     */
    u64 large_page                                               : 1;
#define PDPTE_64_LARGE_PAGE_BIT                                      7
#define PDPTE_64_LARGE_PAGE_FLAG                                     0x80
#define PDPTE_64_LARGE_PAGE_MASK                                     0x01
#define PDPTE_64_LARGE_PAGE(_)                                       (((_) >> 7) & 0x01)

    /**
     * [Bits 10:8] Ignored.
     */
    u64 ignored1                                                : 3;
#define PDPTE_64_IGNORED_1_BIT                                       8
#define PDPTE_64_IGNORED_1_FLAG                                      0x700
#define PDPTE_64_IGNORED_1_MASK                                      0x07
#define PDPTE_64_IGNORED_1(_)                                        (((_) >> 8) & 0x07)

    /**
     * [Bit 11] For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with
     * ordinary paging)
     *
     * @see Vol3A[4.5.5(Restart of HLAT Paging)]
     */
    u64 restart                                                 : 1;
#define PDPTE_64_RESTART_BIT                                         11
#define PDPTE_64_RESTART_FLAG                                        0x800
#define PDPTE_64_RESTART_MASK                                        0x01
#define PDPTE_64_RESTART(_)                                          (((_) >> 11) & 0x01)

    /**
     * [Bits 47:12] Physical address of 4-KByte aligned page directory referenced by this entry.
     */
    u64 page_frame                                         : 36;
#define PDPTE_64_PAGE_FRAME_NUMBER_BIT                               12
#define PDPTE_64_PAGE_FRAME_NUMBER_FLAG                              0xFFFFFFFFF000
#define PDPTE_64_PAGE_FRAME_NUMBER_MASK                              0xFFFFFFFFF
#define PDPTE_64_PAGE_FRAME_NUMBER(_)                                (((_) >> 12) & 0xFFFFFFFFF)
    u64 reserved2                                               : 4;

    /**
     * [Bits 62:52] Ignored.
     */
    u64 ignored2                                                : 11;
#define PDPTE_64_IGNORED_2_BIT                                       52
#define PDPTE_64_IGNORED_2_FLAG                                      0x7FF0000000000000
#define PDPTE_64_IGNORED_2_MASK                                      0x7FF
#define PDPTE_64_IGNORED_2(_)                                        (((_) >> 52) & 0x7FF)

    /**
     * [Bit 63] If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 1-GByte region
     * controlled by this entry); otherwise, reserved (must be 0).
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 execute_disable                                          : 1;
#define PDPTE_64_EXECUTE_DISABLE_BIT                                 63
#define PDPTE_64_EXECUTE_DISABLE_FLAG                                0x8000000000000000
#define PDPTE_64_EXECUTE_DISABLE_MASK                                0x01
#define PDPTE_64_EXECUTE_DISABLE(_)                                  (((_) >> 63) & 0x01)
  };

  u64 as_u64;
} pdpte_t;


/**
 * @brief Format of a 4-Level Page-Directory Entry that Maps a 2-MByte Page
 */
typedef union
{
  struct
  {
    /**
     * [Bit 0] Present; must be 1 to map a 2-MByte page.
     */
    u64 present                                                 : 1;
#define PDE_2MB_64_PRESENT_BIT                                       0
#define PDE_2MB_64_PRESENT_FLAG                                      0x01
#define PDE_2MB_64_PRESENT_MASK                                      0x01
#define PDE_2MB_64_PRESENT(_)                                        (((_) >> 0) & 0x01)

    /**
     * [Bit 1] Read/write; if 0, writes may not be allowed to the 2-MByte page referenced by this entry.
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 write                                                   : 1;
#define PDE_2MB_64_WRITE_BIT                                         1
#define PDE_2MB_64_WRITE_FLAG                                        0x02
#define PDE_2MB_64_WRITE_MASK                                        0x01
#define PDE_2MB_64_WRITE(_)                                          (((_) >> 1) & 0x01)

    /**
     * [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 2-MByte page referenced by this entry.
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 supervisor                                              : 1;
#define PDE_2MB_64_SUPERVISOR_BIT                                    2
#define PDE_2MB_64_SUPERVISOR_FLAG                                   0x04
#define PDE_2MB_64_SUPERVISOR_MASK                                   0x01
#define PDE_2MB_64_SUPERVISOR(_)                                     (((_) >> 2) & 0x01)

    /**
     * [Bit 3] Page-level write-through; indirectly determines the memory type used to access the 2-MByte page referenced by
     * this entry.
     *
     * @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
     */
    u64 write_through                                   : 1;
#define PDE_2MB_64_PAGE_LEVEL_WRITE_THROUGH_BIT                      3
#define PDE_2MB_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                     0x08
#define PDE_2MB_64_PAGE_LEVEL_WRITE_THROUGH_MASK                     0x01
#define PDE_2MB_64_PAGE_LEVEL_WRITE_THROUGH(_)                       (((_) >> 3) & 0x01)

    /**
     * [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the 2-MByte page referenced by
     * this entry.
     *
     * @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
     */
    u64 cache_disable                                   : 1;
#define PDE_2MB_64_PAGE_LEVEL_CACHE_DISABLE_BIT                      4
#define PDE_2MB_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                     0x10
#define PDE_2MB_64_PAGE_LEVEL_CACHE_DISABLE_MASK                     0x01
#define PDE_2MB_64_PAGE_LEVEL_CACHE_DISABLE(_)                       (((_) >> 4) & 0x01)

    /**
     * [Bit 5] Accessed; indicates whether software has accessed the 2-MByte page referenced by this entry.
     *
     * @see Vol3A[4.8(Accessed and Dirty Flags)]
     */
    u64 accessed                                                : 1;
#define PDE_2MB_64_ACCESSED_BIT                                      5
#define PDE_2MB_64_ACCESSED_FLAG                                     0x20
#define PDE_2MB_64_ACCESSED_MASK                                     0x01
#define PDE_2MB_64_ACCESSED(_)                                       (((_) >> 5) & 0x01)

    /**
     * [Bit 6] Dirty; indicates whether software has written to the 2-MByte page referenced by this entry.
     *
     * @see Vol3A[4.8(Accessed and Dirty Flags)]
     */
    u64 dirty                                                   : 1;
#define PDE_2MB_64_DIRTY_BIT                                         6
#define PDE_2MB_64_DIRTY_FLAG                                        0x40
#define PDE_2MB_64_DIRTY_MASK                                        0x01
#define PDE_2MB_64_DIRTY(_)                                          (((_) >> 6) & 0x01)

    /**
     * [Bit 7] Page size; must be 1 (otherwise, this entry references a page directory).
     */
    u64 large_page                                               : 1;
#define PDE_2MB_64_LARGE_PAGE_BIT                                    7
#define PDE_2MB_64_LARGE_PAGE_FLAG                                   0x80
#define PDE_2MB_64_LARGE_PAGE_MASK                                   0x01
#define PDE_2MB_64_LARGE_PAGE(_)                                     (((_) >> 7) & 0x01)

    /**
     * [Bit 8] Global; if CR4.PGE = 1, determines whether the translation is global; ignored otherwise.
     *
     * @see Vol3A[4.10(Caching Translation Information)]
     */
    u64 global                                                  : 1;
#define PDE_2MB_64_GLOBAL_BIT                                        8
#define PDE_2MB_64_GLOBAL_FLAG                                       0x100
#define PDE_2MB_64_GLOBAL_MASK                                       0x01
#define PDE_2MB_64_GLOBAL(_)                                         (((_) >> 8) & 0x01)

    /**
     * [Bits 10:9] Ignored.
     */
    u64 ignored1                                                : 2;
#define PDE_2MB_64_IGNORED_1_BIT                                     9
#define PDE_2MB_64_IGNORED_1_FLAG                                    0x600
#define PDE_2MB_64_IGNORED_1_MASK                                    0x03
#define PDE_2MB_64_IGNORED_1(_)                                      (((_) >> 9) & 0x03)

    /**
     * [Bit 11] For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with
     * ordinary paging)
     *
     * @see Vol3A[4.5.5(Restart of HLAT Paging)]
     */
    u64 restart                                                 : 1;
#define PDE_2MB_64_RESTART_BIT                                       11
#define PDE_2MB_64_RESTART_FLAG                                      0x800
#define PDE_2MB_64_RESTART_MASK                                      0x01
#define PDE_2MB_64_RESTART(_)                                        (((_) >> 11) & 0x01)

    /**
     * [Bit 12] Indirectly determines the memory type used to access the 2-MByte page referenced by this entry.
     *
     * @note The PAT is supported on all processors that support 4-level paging.
     * @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
     */
    u64 pat                                                     : 1;
#define PDE_2MB_64_PAT_BIT                                           12
#define PDE_2MB_64_PAT_FLAG                                          0x1000
#define PDE_2MB_64_PAT_MASK                                          0x01
#define PDE_2MB_64_PAT(_)                                            (((_) >> 12) & 0x01)
    u64 reserved1                                               : 8;

    /**
     * [Bits 47:21] Physical address of the 2-MByte page referenced by this entry.
     */
    u64 page_frame                                         : 27;
#define PDE_2MB_64_PAGE_FRAME_NUMBER_BIT                             21
#define PDE_2MB_64_PAGE_FRAME_NUMBER_FLAG                            0xFFFFFFE00000
#define PDE_2MB_64_PAGE_FRAME_NUMBER_MASK                            0x7FFFFFF
#define PDE_2MB_64_PAGE_FRAME_NUMBER(_)                              (((_) >> 21) & 0x7FFFFFF)
    u64 reserved2                                               : 4;

    /**
     * [Bits 58:52] Ignored.
     */
    u64 ignored2                                                : 7;
#define PDE_2MB_64_IGNORED_2_BIT                                     52
#define PDE_2MB_64_IGNORED_2_FLAG                                    0x7F0000000000000
#define PDE_2MB_64_IGNORED_2_MASK                                    0x7F
#define PDE_2MB_64_IGNORED_2(_)                                      (((_) >> 52) & 0x7F)

    /**
     * [Bits 62:59] Protection key; if CR4.PKE = 1, determines the protection key of the page; ignored otherwise.
     *
     * @see Vol3A[4.6.2(Protection Keys)]
     */
    u64 protection_key                                           : 4;
#define PDE_2MB_64_PROTECTION_KEY_BIT                                59
#define PDE_2MB_64_PROTECTION_KEY_FLAG                               0x7800000000000000
#define PDE_2MB_64_PROTECTION_KEY_MASK                               0x0F
#define PDE_2MB_64_PROTECTION_KEY(_)                                 (((_) >> 59) & 0x0F)

    /**
     * [Bit 63] If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 2-MByte page
     * controlled by this entry); otherwise, reserved (must be 0).
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 execute_disable                                          : 1;
#define PDE_2MB_64_EXECUTE_DISABLE_BIT                               63
#define PDE_2MB_64_EXECUTE_DISABLE_FLAG                              0x8000000000000000
#define PDE_2MB_64_EXECUTE_DISABLE_MASK                              0x01
#define PDE_2MB_64_EXECUTE_DISABLE(_)                                (((_) >> 63) & 0x01)
  };

  u64 as_u64;
} pde_2mb_t;

/**
 * @brief Format of a 4-Level Page-Directory Entry that References a Page Table
 */
typedef union
{
  struct
  {
    /**
     * [Bit 0] Present; must be 1 to reference a page table.
     */
    u64 present                                                 : 1;
#define PDE_64_PRESENT_BIT                                           0
#define PDE_64_PRESENT_FLAG                                          0x01
#define PDE_64_PRESENT_MASK                                          0x01
#define PDE_64_PRESENT(_)                                            (((_) >> 0) & 0x01)

    /**
     * [Bit 1] Read/write; if 0, writes may not be allowed to the 2-MByte region controlled by this entry.
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 write                                                   : 1;
#define PDE_64_WRITE_BIT                                             1
#define PDE_64_WRITE_FLAG                                            0x02
#define PDE_64_WRITE_MASK                                            0x01
#define PDE_64_WRITE(_)                                              (((_) >> 1) & 0x01)

    /**
     * [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 2-MByte region controlled by this entry.
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 supervisor                                              : 1;
#define PDE_64_SUPERVISOR_BIT                                        2
#define PDE_64_SUPERVISOR_FLAG                                       0x04
#define PDE_64_SUPERVISOR_MASK                                       0x01
#define PDE_64_SUPERVISOR(_)                                         (((_) >> 2) & 0x01)

    /**
     * [Bit 3] Page-level write-through; indirectly determines the memory type used to access the page table referenced by this
     * entry.
     *
     * @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
     */
    u64 write_through                                   : 1;
#define PDE_64_PAGE_LEVEL_WRITE_THROUGH_BIT                          3
#define PDE_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                         0x08
#define PDE_64_PAGE_LEVEL_WRITE_THROUGH_MASK                         0x01
#define PDE_64_PAGE_LEVEL_WRITE_THROUGH(_)                           (((_) >> 3) & 0x01)

    /**
     * [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the page table referenced by this
     * entry.
     *
     * @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
     */
    u64 cache_disable                                   : 1;
#define PDE_64_PAGE_LEVEL_CACHE_DISABLE_BIT                          4
#define PDE_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                         0x10
#define PDE_64_PAGE_LEVEL_CACHE_DISABLE_MASK                         0x01
#define PDE_64_PAGE_LEVEL_CACHE_DISABLE(_)                           (((_) >> 4) & 0x01)

    /**
     * [Bit 5] Accessed; indicates whether this entry has been used for linear-address translation.
     *
     * @see Vol3A[4.8(Accessed and Dirty Flags)]
     */
    u64 accessed                                                : 1;
#define PDE_64_ACCESSED_BIT                                          5
#define PDE_64_ACCESSED_FLAG                                         0x20
#define PDE_64_ACCESSED_MASK                                         0x01
#define PDE_64_ACCESSED(_)                                           (((_) >> 5) & 0x01)
    u64 reserved1                                               : 1;

    /**
     * [Bit 7] Page size; must be 0 (otherwise, this entry maps a 2-MByte page).
     */
    u64 large_page                                               : 1;
#define PDE_64_LARGE_PAGE_BIT                                        7
#define PDE_64_LARGE_PAGE_FLAG                                       0x80
#define PDE_64_LARGE_PAGE_MASK                                       0x01
#define PDE_64_LARGE_PAGE(_)                                         (((_) >> 7) & 0x01)

    /**
     * [Bits 10:8] Ignored.
     */
    u64 ignored1                                                : 3;
#define PDE_64_IGNORED_1_BIT                                         8
#define PDE_64_IGNORED_1_FLAG                                        0x700
#define PDE_64_IGNORED_1_MASK                                        0x07
#define PDE_64_IGNORED_1(_)                                          (((_) >> 8) & 0x07)

    /**
     * [Bit 11] For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with
     * ordinary paging)
     *
     * @see Vol3A[4.5.5(Restart of HLAT Paging)]
     */
    u64 restart                                                 : 1;
#define PDE_64_RESTART_BIT                                           11
#define PDE_64_RESTART_FLAG                                          0x800
#define PDE_64_RESTART_MASK                                          0x01
#define PDE_64_RESTART(_)                                            (((_) >> 11) & 0x01)

    /**
     * [Bits 47:12] Physical address of 4-KByte aligned page table referenced by this entry.
     */
    u64 page_frame                                         : 36;
#define PDE_64_PAGE_FRAME_NUMBER_BIT                                 12
#define PDE_64_PAGE_FRAME_NUMBER_FLAG                                0xFFFFFFFFF000
#define PDE_64_PAGE_FRAME_NUMBER_MASK                                0xFFFFFFFFF
#define PDE_64_PAGE_FRAME_NUMBER(_)                                  (((_) >> 12) & 0xFFFFFFFFF)
    u64 Reserved2                                               : 4;

    /**
     * [Bits 62:52] Ignored.
     */
    u64 ignored2                                                : 11;
#define PDE_64_IGNORED_2_BIT                                         52
#define PDE_64_IGNORED_2_FLAG                                        0x7FF0000000000000
#define PDE_64_IGNORED_2_MASK                                        0x7FF
#define PDE_64_IGNORED_2(_)                                          (((_) >> 52) & 0x7FF)

    /**
     * [Bit 63] If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 2-MByte region
     * controlled by this entry); otherwise, reserved (must be 0).
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 execute_disable                                          : 1;
#define PDE_64_EXECUTE_DISABLE_BIT                                   63
#define PDE_64_EXECUTE_DISABLE_FLAG                                  0x8000000000000000
#define PDE_64_EXECUTE_DISABLE_MASK                                  0x01
#define PDE_64_EXECUTE_DISABLE(_)                                    (((_) >> 63) & 0x01)
  };

  u64 as_u64;
} pde_t;

/**
 * @brief Format of a 4-Level Page-Table Entry that Maps a 4-KByte Page
 */
typedef union
{
  struct
  {
    /**
     * [Bit 0] Present; must be 1 to map a 4-KByte page.
     */
    u64 present                                                 : 1;
#define PTE_64_PRESENT_BIT                                           0
#define PTE_64_PRESENT_FLAG                                          0x01
#define PTE_64_PRESENT_MASK                                          0x01
#define PTE_64_PRESENT(_)                                            (((_) >> 0) & 0x01)

    /**
     * [Bit 1] Read/write; if 0, writes may not be allowed to the 4-KByte page referenced by this entry.
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 write                                                   : 1;
#define PTE_64_WRITE_BIT                                             1
#define PTE_64_WRITE_FLAG                                            0x02
#define PTE_64_WRITE_MASK                                            0x01
#define PTE_64_WRITE(_)                                              (((_) >> 1) & 0x01)

    /**
     * [Bit 2] User/supervisor; if 0, user-mode accesses are not allowed to the 4-KByte page referenced by this entry.
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 supervisor                                              : 1;
#define PTE_64_SUPERVISOR_BIT                                        2
#define PTE_64_SUPERVISOR_FLAG                                       0x04
#define PTE_64_SUPERVISOR_MASK                                       0x01
#define PTE_64_SUPERVISOR(_)                                         (((_) >> 2) & 0x01)

    /**
     * [Bit 3] Page-level write-through; indirectly determines the memory type used to access the 4-KByte page referenced by
     * this entry.
     *
     * @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
     */
    u64 write_through                                   : 1;
#define PTE_64_PAGE_LEVEL_WRITE_THROUGH_BIT                          3
#define PTE_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                         0x08
#define PTE_64_PAGE_LEVEL_WRITE_THROUGH_MASK                         0x01
#define PTE_64_PAGE_LEVEL_WRITE_THROUGH(_)                           (((_) >> 3) & 0x01)

    /**
     * [Bit 4] Page-level cache disable; indirectly determines the memory type used to access the 4-KByte page referenced by
     * this entry.
     *
     * @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
     */
    u64 cache_disable                                   : 1;
#define PTE_64_PAGE_LEVEL_CACHE_DISABLE_BIT                          4
#define PTE_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                         0x10
#define PTE_64_PAGE_LEVEL_CACHE_DISABLE_MASK                         0x01
#define PTE_64_PAGE_LEVEL_CACHE_DISABLE(_)                           (((_) >> 4) & 0x01)

    /**
     * [Bit 5] Accessed; indicates whether software has accessed the 4-KByte page referenced by this entry.
     *
     * @see Vol3A[4.8(Accessed and Dirty Flags)]
     */
    u64 accessed                                                : 1;
#define PTE_64_ACCESSED_BIT                                          5
#define PTE_64_ACCESSED_FLAG                                         0x20
#define PTE_64_ACCESSED_MASK                                         0x01
#define PTE_64_ACCESSED(_)                                           (((_) >> 5) & 0x01)

    /**
     * [Bit 6] Dirty; indicates whether software has written to the 4-KByte page referenced by this entry.
     *
     * @see Vol3A[4.8(Accessed and Dirty Flags)]
     */
    u64 dirty                                                   : 1;
#define PTE_64_DIRTY_BIT                                             6
#define PTE_64_DIRTY_FLAG                                            0x40
#define PTE_64_DIRTY_MASK                                            0x01
#define PTE_64_DIRTY(_)                                              (((_) >> 6) & 0x01)

    /**
     * [Bit 7] Indirectly determines the memory type used to access the 4-KByte page referenced by this entry.
     *
     * @see Vol3A[4.9.2(Paging and Memory Typing When the PAT is Supported (Pentium III and More Recent Processor Families))]
     */
    u64 pat                                                     : 1;
#define PTE_64_PAT_BIT                                               7
#define PTE_64_PAT_FLAG                                              0x80
#define PTE_64_PAT_MASK                                              0x01
#define PTE_64_PAT(_)                                                (((_) >> 7) & 0x01)

    /**
     * [Bit 8] Global; if CR4.PGE = 1, determines whether the translation is global; ignored otherwise.
     *
     * @see Vol3A[4.10(Caching Translation Information)]
     */
    u64 global                                                  : 1;
#define PTE_64_GLOBAL_BIT                                            8
#define PTE_64_GLOBAL_FLAG                                           0x100
#define PTE_64_GLOBAL_MASK                                           0x01
#define PTE_64_GLOBAL(_)                                             (((_) >> 8) & 0x01)

    /**
     * [Bits 10:9] Ignored.
     */
    u64 ignored1                                                : 2;
#define PTE_64_IGNORED_1_BIT                                         9
#define PTE_64_IGNORED_1_FLAG                                        0x600
#define PTE_64_IGNORED_1_MASK                                        0x03
#define PTE_64_IGNORED_1(_)                                          (((_) >> 9) & 0x03)

    /**
     * [Bit 11] For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with
     * ordinary paging)
     *
     * @see Vol3A[4.5.5(Restart of HLAT Paging)]
     */
    u64 restart                                                 : 1;
#define PTE_64_RESTART_BIT                                           11
#define PTE_64_RESTART_FLAG                                          0x800
#define PTE_64_RESTART_MASK                                          0x01
#define PTE_64_RESTART(_)                                            (((_) >> 11) & 0x01)

    /**
     * [Bits 47:12] Physical address of the 4-KByte page referenced by this entry.
     */
    u64 page_frame                                         : 36;
#define PTE_64_PAGE_FRAME_NUMBER_BIT                                 12
#define PTE_64_PAGE_FRAME_NUMBER_FLAG                                0xFFFFFFFFF000
#define PTE_64_PAGE_FRAME_NUMBER_MASK                                0xFFFFFFFFF
#define PTE_64_PAGE_FRAME_NUMBER(_)                                  (((_) >> 12) & 0xFFFFFFFFF)
    u64 reserved1                                               : 4;

    /**
     * [Bits 58:52] Ignored.
     */
    u64 ignored2                                                : 7;
#define PTE_64_IGNORED_2_BIT                                         52
#define PTE_64_IGNORED_2_FLAG                                        0x7F0000000000000
#define PTE_64_IGNORED_2_MASK                                        0x7F
#define PTE_64_IGNORED_2(_)                                          (((_) >> 52) & 0x7F)

    /**
     * [Bits 62:59] Protection key; if CR4.PKE = 1, determines the protection key of the page; ignored otherwise.
     *
     * @see Vol3A[4.6.2(Protection Keys)]
     */
    u64 ProtectionKey                                           : 4;
#define PTE_64_PROTECTION_KEY_BIT                                    59
#define PTE_64_PROTECTION_KEY_FLAG                                   0x7800000000000000
#define PTE_64_PROTECTION_KEY_MASK                                   0x0F
#define PTE_64_PROTECTION_KEY(_)                                     (((_) >> 59) & 0x0F)

    /**
     * [Bit 63] If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 1-GByte page
     * controlled by this entry); otherwise, reserved (must be 0).
     *
     * @see Vol3A[4.6(Access Rights)]
     */
    u64 execute_disable                                          : 1;
#define PTE_64_EXECUTE_DISABLE_BIT                                   63
#define PTE_64_EXECUTE_DISABLE_FLAG                                  0x8000000000000000
#define PTE_64_EXECUTE_DISABLE_MASK                                  0x01
#define PTE_64_EXECUTE_DISABLE(_)                                    (((_) >> 63) & 0x01)
  };

  u64 as_u64;
} pte_t;



/**
 * @brief Format of a common Page-Table Entry
 */
typedef union
{
  struct
  {
    u64 present                                                 : 1;
#define PT_ENTRY_64_PRESENT_BIT                                      0
#define PT_ENTRY_64_PRESENT_FLAG                                     0x01
#define PT_ENTRY_64_PRESENT_MASK                                     0x01
#define PT_ENTRY_64_PRESENT(_)                                       (((_) >> 0) & 0x01)
    u64 write                                                   : 1;
#define PT_ENTRY_64_WRITE_BIT                                        1
#define PT_ENTRY_64_WRITE_FLAG                                       0x02
#define PT_ENTRY_64_WRITE_MASK                                       0x01
#define PT_ENTRY_64_WRITE(_)                                         (((_) >> 1) & 0x01)
    u64 supervisor                                              : 1;
#define PT_ENTRY_64_SUPERVISOR_BIT                                   2
#define PT_ENTRY_64_SUPERVISOR_FLAG                                  0x04
#define PT_ENTRY_64_SUPERVISOR_MASK                                  0x01
#define PT_ENTRY_64_SUPERVISOR(_)                                    (((_) >> 2) & 0x01)
    u64 write_through                                   : 1;
#define PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH_BIT                     3
#define PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH_FLAG                    0x08
#define PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH_MASK                    0x01
#define PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH(_)                      (((_) >> 3) & 0x01)
    u64 cache_disable                                   : 1;
#define PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE_BIT                     4
#define PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE_FLAG                    0x10
#define PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE_MASK                    0x01
#define PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE(_)                      (((_) >> 4) & 0x01)
    u64 accessed                                                : 1;
#define PT_ENTRY_64_ACCESSED_BIT                                     5
#define PT_ENTRY_64_ACCESSED_FLAG                                    0x20
#define PT_ENTRY_64_ACCESSED_MASK                                    0x01
#define PT_ENTRY_64_ACCESSED(_)                                      (((_) >> 5) & 0x01)
    u64 dirty                                                   : 1;
#define PT_ENTRY_64_DIRTY_BIT                                        6
#define PT_ENTRY_64_DIRTY_FLAG                                       0x40
#define PT_ENTRY_64_DIRTY_MASK                                       0x01
#define PT_ENTRY_64_DIRTY(_)                                         (((_) >> 6) & 0x01)
    u64 large_page                                               : 1;
#define PT_ENTRY_64_LARGE_PAGE_BIT                                   7
#define PT_ENTRY_64_LARGE_PAGE_FLAG                                  0x80
#define PT_ENTRY_64_LARGE_PAGE_MASK                                  0x01
#define PT_ENTRY_64_LARGE_PAGE(_)                                    (((_) >> 7) & 0x01)
    u64 global                                                  : 1;
#define PT_ENTRY_64_GLOBAL_BIT                                       8
#define PT_ENTRY_64_GLOBAL_FLAG                                      0x100
#define PT_ENTRY_64_GLOBAL_MASK                                      0x01
#define PT_ENTRY_64_GLOBAL(_)                                        (((_) >> 8) & 0x01)

    /**
     * [Bits 10:9] Ignored.
     */
    u64 ignored1                                                : 2;
#define PT_ENTRY_64_IGNORED_1_BIT                                    9
#define PT_ENTRY_64_IGNORED_1_FLAG                                   0x600
#define PT_ENTRY_64_IGNORED_1_MASK                                   0x03
#define PT_ENTRY_64_IGNORED_1(_)                                     (((_) >> 9) & 0x03)
    u64 restart                                                 : 1;
#define PT_ENTRY_64_RESTART_BIT                                      11
#define PT_ENTRY_64_RESTART_FLAG                                     0x800
#define PT_ENTRY_64_RESTART_MASK                                     0x01
#define PT_ENTRY_64_RESTART(_)                                       (((_) >> 11) & 0x01)

    /**
     * [Bits 47:12] Physical address of the 4-KByte page referenced by this entry.
     */
    u64 page_frame                                         : 36;
#define PT_ENTRY_64_PAGE_FRAME_NUMBER_BIT                            12
#define PT_ENTRY_64_PAGE_FRAME_NUMBER_FLAG                           0xFFFFFFFFF000
#define PT_ENTRY_64_PAGE_FRAME_NUMBER_MASK                           0xFFFFFFFFF
#define PT_ENTRY_64_PAGE_FRAME_NUMBER(_)                             (((_) >> 12) & 0xFFFFFFFFF)
    u64 reserved1                                               : 4;

    /**
     * [Bits 58:52] Ignored.
     */
    u64 ignored2                                                : 7;
#define PT_ENTRY_64_IGNORED_2_BIT                                    52
#define PT_ENTRY_64_IGNORED_2_FLAG                                   0x7F0000000000000
#define PT_ENTRY_64_IGNORED_2_MASK                                   0x7F
#define PT_ENTRY_64_IGNORED_2(_)                                     (((_) >> 52) & 0x7F)
    u64 protection_key                                           : 4;
#define PT_ENTRY_64_PROTECTION_KEY_BIT                               59
#define PT_ENTRY_64_PROTECTION_KEY_FLAG                              0x7800000000000000
#define PT_ENTRY_64_PROTECTION_KEY_MASK                              0x0F
#define PT_ENTRY_64_PROTECTION_KEY(_)                                (((_) >> 59) & 0x0F)
    u64 execute_disable                                          : 1;
#define PT_ENTRY_64_EXECUTE_DISABLE_BIT                              63
#define PT_ENTRY_64_EXECUTE_DISABLE_FLAG                             0x8000000000000000
#define PT_ENTRY_64_EXECUTE_DISABLE_MASK                             0x01
#define PT_ENTRY_64_EXECUTE_DISABLE(_)                               (((_) >> 63) & 0x01)
  };

  u64 as_u64;
} pt_entry_t;



#endif //__paging_h__