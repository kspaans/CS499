#include <kern/ksyms.h>
#include <panic.h>
#include <types.h>

struct ksym *ksym_for_address(uint32_t code) {
  struct ksym *ret = NULL;

  for (struct ksym *ks = &ksyms_start; ks < &ksyms_end; ++ks) {
    if (ks->code > code)
      break;
    ret = ks;
  }

  if (!ret)
    panic("No symbol for address");

  return ret;
}

const char *symbol_for_address(uint32_t code) {
  struct ksym *ks = ksym_for_address(code);
  return ks->name;
}

const char *symbol_for_address_exact(uint32_t code) {
  struct ksym *ks = ksym_for_address(code);

  if (ks && ks->code == code)
    return ks->name;

  return "(none)";
}
