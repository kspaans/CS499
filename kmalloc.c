#include <inttypes.h>
#include <kmalloc.h>

#define ALIGN(val, align) (size + align - 1) & ~(align - 1);

extern char runtime_data_start;
static char *runtime_data = &runtime_data_start;

void *kmalloc(size_t size) {
	char *p = runtime_data;
	runtime_data += ALIGN(size, KMALLOC_ALIGN);
	return p;
}
