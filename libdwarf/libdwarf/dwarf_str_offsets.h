#ifndef DWARF_STR_OFFSETS_H
#define DWARF_STR_OFFSETS_H
/*
    Copyright (C) 2018-2018 David Anderson. All Rights Reserved.

    This program is free software; you can redistribute it
    and/or modify it under the terms of version 2.1 of
    the GNU Lesser General Public License
    as published by the Free Software Foundation.

    This program is distributed in the hope that it would be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    Further, this software is distributed without any warranty
    that it is free of the rightful claim of any third person
    regarding infringement or the like.
    Any license provided herein, whether implied or
    otherwise, applies only to this software file.
    Patent licenses, if any, provided herein do not
    apply to combinations of this program with
    other software, or any other product whatsoever.

    You should have received a copy of the GNU Lesser General Public
    License along with this program; if not, write the Free Software
    Foundation, Inc., 51 Franklin Street - Fifth Floor,
    Boston MA 02110-1301,
    USA.

*/


struct  Dwarf_Str_Offsets_Table_s {
    /*  pointers are to dwarf-memory valid till Dwarf_Debug
        is closed..  None are to be deallocated. */
    Dwarf_Unsigned so_magic_value;
    Dwarf_Debug  so_dbg;

    /* Section data. */
    Dwarf_Small   *so_section_start_ptr;
    Dwarf_Small   *so_section_end_ptr;
    Dwarf_Unsigned so_section_size;
    /* Overall data about wasted space in the section. */
    Dwarf_Unsigned so_wasted_section_bytes;
    /* The number of tables processed in the section. */
    Dwarf_Unsigned so_table_count;

    /*  Used to iterate through the section getting
        to each table */
    Dwarf_Unsigned so_next_table_offset;

    /*  Per table (ie, a table is a
        header and array of offsets) inside the section.  */
    Dwarf_Small *so_header_ptr;
    Dwarf_Small *so_end_cu_ptr;
    Dwarf_Small *so_array_ptr;
    Dwarf_Unsigned so_table_start_offset;
    Dwarf_Unsigned so_array_start_offset;
    Dwarf_Unsigned so_array_entry_count;
    Dwarf_Half     so_array_entry_size;

};

int _dwarf_trial_read_dwarf_five_hdr(Dwarf_Debug dbg,
    Dwarf_Small *table_start_ptr,
    Dwarf_Unsigned secsize,
    Dwarf_Small * secendptr,
    Dwarf_Unsigned *length_out,
    Dwarf_Half *local_offset_size_out,
    Dwarf_Half *local_extension_size_out,
    Dwarf_Half *version_out,
    Dwarf_Half *padding_out,
    Dwarf_Error *error);
int
_dwarf_find_all_offsets_via_fission(Dwarf_Debug dbg,
    Dwarf_CU_Context cu_context,
    Dwarf_Error *error);


#endif /* DWARF_STR_OFFSETS_H */
