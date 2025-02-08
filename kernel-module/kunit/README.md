# KUnit testing

## Structure

Three demo are included in this directory, catergorized due to the following considerations:
* internal: the source code is expected to be part of the kernel source tree
  * not necessarily built-in; may be a module
  * linking with KUnit code is easy: KUnit tests can be run without building the full kernel
  * reference examples: fs/ext4, drivers/gpu/drm/ttm
* external: the source code is an external kernel module
  * linking with KUnit code must be dynamic: testing requires a running kernel with `CONFIG_KUNIT=y`
  * the test is carried out when test module is inserted
  * "external-static": the test code is linked statically with function code
    * A single binary module which includes function code and test code is made
  * "external-dynamic": the test code is linked dynamically with function code
    * Two modules are made, one for function code and one for tests
    * the function module must "export" the functions to be tested to make it accessible

## Reminders

* Make sure the kernel is built with "CONFIG_KUNIT=y". Otherwise you would get error in modpost step
  which complains kunit symbols undefined.

## TODO

* coverage
* fake / mocking
* data reuse

## References

Documentation/dev-tools/kunit/
