#ifdef _MSC_VER
#pragma once
#endif

#ifndef _XBOX_FILE_FORMATS
#define _XBOX_FILE_FORMATS

#pragma pack(push)

#define XBE_HEADER_MAGIC 0x48454258 // XBEH
/*
{{{ following source was downloaded from http://www.caustik.com/xbox/download/xbe.htm
Changes made to the embedded source:
replaced class xbe by namespace xbe,
removed public:
removed C-style names for struct types.
{{{{
*/

// Note: This source is compatible with MSVC 6.0 or higher, and is public domain.

// ******************************************************************
// * standard typedefs
// ******************************************************************
typedef signed int     sint;
typedef unsigned int   uint;

typedef char           int08;
typedef short          int16;
typedef long           int32;

typedef unsigned char  uint08;
typedef unsigned short uint16;
typedef unsigned long  uint32;

typedef signed char    sint08;
typedef signed short   sint16;
typedef signed long    sint32;

// ******************************************************************
// * class : xbe
// ******************************************************************
namespace xbe
{
    //public:

        #pragma pack(1)
        struct header
        {
            uint32 m_magic;                         // magic number [should be "XBEH"]
            uint08 m_digsig[256];                   // digital signature
            uint32 m_base;                          // base address
            uint32 m_sizeof_headers;                // size of headers
            uint32 m_sizeof_image;                  // size of image
            uint32 m_sizeof_image_header;           // size of image header
            uint32 m_timedate;                      // timedate stamp
            uint32 m_certificate_addr;              // certificate address
            uint32 m_sections;                      // number of sections
            uint32 m_section_headers_addr;          // section headers address

            struct init_flags
            {
                uint m_mount_utility_drive    : 1;  // mount utility drive flag
                uint m_format_utility_drive   : 1;  // format utility drive flag
                uint m_limit_64mb             : 1;  // limit development kit run time memory to 64mb flag
                uint m_dont_setup_harddisk    : 1;  // don't setup hard disk flag
                uint m_unused                 : 4;  // unused (or unknown)
                uint m_unused_b1              : 8;  // unused (or unknown)
                uint m_unused_b2              : 8;  // unused (or unknown)
                uint m_unused_b3              : 8;  // unused (or unknown)
            }m_init_flags;

            uint32 m_entry;                         // entry point address
            uint32 m_tls_addr;                      // thread local storage directory address
            uint32 m_pe_stack_commit;               // size of stack commit
            uint32 m_pe_heap_reserve;               // size of heap reserve
            uint32 m_pe_heap_commit;                // size of heap commit
            uint32 m_pe_base_addr;                  // original base address
            uint32 m_pe_sizeof_image;               // size of original image
            uint32 m_pe_checksum;                   // original checksum
            uint32 m_pe_timedate;                   // original timedate stamp
            uint32 m_debug_pathname_addr;           // debug pathname address
            uint32 m_debug_filename_addr;           // debug filename address
            uint32 m_debug_unicode_filename_addr;   // debug unicode filename address
            uint32 m_kernel_image_thunk_addr;       // kernel image thunk address
            uint32 m_nonkernel_import_dir_addr;     // non kernel import directory address
            uint32 m_library_versions;              // number of library versions
            uint32 m_library_versions_addr;         // library versions address
            uint32 m_kernel_library_version_addr;   // kernel library version address
            uint32 m_xapi_library_version_addr;     // xapi library version address
            uint32 m_logo_bitmap_addr;              // logo bitmap address
            uint32 m_logo_bitmap_size;              // logo bitmap size
        };

        struct certificate
        {
            uint32 m_size;                          // size of certificate
            uint32 m_timedate;                      // timedate stamp
            uint32 m_titleid;                       // title id
            uint16 m_title_name[40];                // title name (unicode)
            uint32 m_alt_title_id[0x10];            // alternate title ids
            uint32 m_allowed_media;                 // allowed media types
            uint32 m_game_region;                   // game region
            uint32 m_game_ratings;                  // game ratings
            uint32 m_disk_number;                   // disk number
            uint32 m_version;                       // version
            uint08 m_lan_key[16];                   // lan key
            uint08 m_sig_key[16];                   // signature key
            uint08 m_title_alt_sig_key[16][16];     // alternate signature keys
        };

        struct section_header
        {
            struct flags                            // flags
            {
                uint m_writable             : 1;    // writable flag
                uint m_preload              : 1;    // preload flag
                uint m_executable           : 1;    // executable flag
                uint m_inserted_file        : 1;    // inserted file flag
                uint m_head_page_ro         : 1;    // head page read only flag
                uint m_tail_page_ro         : 1;    // tail page read only flag
                uint m_unused_a1            : 1;    // unused (or unknown)
                uint m_unused_a2            : 1;    // unused (or unknown)
                uint m_unused_b1            : 8;    // unused (or unknown)
                uint m_unused_b2            : 8;    // unused (or unknown)
                uint m_unused_b3            : 8;    // unused (or unknown)
            }m_flags;
            uint32  m_virtual_addr;                  // virtual address
            uint32  m_virtual_size;                  // virtual size
            uint32  m_raw_addr;                      // file offset to raw data
            uint32  m_sizeof_raw;                    // size of raw data
            uint32  m_section_name_addr;             // section name addr
            uint32  m_section_reference_count;       // section reference count
            uint16 *m_head_shared_ref_count_addr;    // head shared page reference count address
            uint16 *m_tail_shared_ref_count_addr;    // tail shared page reference count address
            uint08  m_section_digest[20];            // section digest
        };

        struct library_version
        {
            char   m_name[8];                       // library name
            uint16 m_major_version;                 // major version
            uint16 m_minor_version;                 // minor version
            uint16 m_build_version;                 // build version
            struct flags                            // flags
            {
                uint16 m_qfe_version        : 13;   // QFE Version
                uint16 m_approved           : 2;    // Approved? (0:no, 1:possibly, 2:yes)
                uint16 m_debug_build        : 1;    // Is this a debug build?
            }m_flags;
        };

        struct tls                                  // thread local storage
        {
            uint32 m_data_start_addr;               // raw start address
            uint32 m_data_end_addr;                 // raw end address
            uint32 m_tls_index_addr;                // tls index  address
            uint32 m_tls_callback_addr;             // tls callback address
            uint32 m_sizeof_zero_fill;              // size of zero fill
            uint32 m_characteristics;               // characteristics
        };
};

/* }}}}} end of embedded source */

#pragma pack(pop)

#endif
