# Page Table Demo

The kernel module accepts query requests from a process via ioctl, walk through its page table
and return page information to user space.

The user space demo would query the kernel module about the mapping of global variables,
stack variables and mmaped pages (fresh, read and written).

References
- mm/gup.c: follow_page_mask
- mm/pagewalk.c
