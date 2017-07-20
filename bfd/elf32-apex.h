/*
 * Copyrights
 */


extern void
elf32_apex_setup_params (struct bfd_link_info *, bfd *, asection *,
                        bfd_boolean, bfd_boolean, bfd_boolean,
                        bfd_vma, bfd_boolean);

extern int
elf32_apex_setup_section_lists (bfd *, struct bfd_link_info *);

extern bfd_boolean
elf32_apex_size_stubs (bfd *, struct bfd_link_info *, bfd_boolean);

extern bfd_boolean
elf32_apex_build_stubs (struct bfd_link_info *);
