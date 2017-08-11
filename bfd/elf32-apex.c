/*
 * Copyrights
 */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/apex.h"
#include "elf32-apex.h"
//#define TARGET_LITTLE_SYM apex_elf32_def_vec
//#define TARGET_LITTLE_NAME "elf32-apex"
#ifndef TARGET_BIG_SYM
#define TARGET_BIG_SYM	apex_elf32_def_vec
#define TARGET_BIG_NAME	"elf32-apex"
#endif
#define ELF_ARCH	bfd_arch_apex
#define ELF_MAXPAGESIZE 0x100000
#define ELF_COMMONPAGESIZE 0x2000
#define ELF_MACHINE_CODE	EM_APEX



static reloc_howto_type *
bfd_elf32_bfd_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
				 bfd_reloc_code_real_type code ATTRIBUTE_UNUSED)
{

  return NULL;
}
static reloc_howto_type *
bfd_elf32_bfd_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
				 const char *r_name ATTRIBUTE_UNUSED)
{
    return NULL;
}
#include "elf32-target.h"
