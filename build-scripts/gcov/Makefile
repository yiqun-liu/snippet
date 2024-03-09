hello-vanilla: hello.c
	gcc -o hello-vanilla hello.c

hello-gcov hello-gcov-hello.gcno: hello.c hello.h
	gcc -o $@ $< -O0 -fprofile-arcs -ftest-coverage -fkeep-inline-functions \
		-fkeep-static-functions -lgcov
	file $@-hello.gcno

# relocate to hard-wired directory
hello-gcov-hard: hello.c hello.h
	gcc -o $@ $< -O0 -fprofile-arcs -ftest-coverage -fkeep-inline-functions \
		-fkeep-static-functions -lgcov -fprofile-generate=/tmp/gcov-hard
	file $@-hello.gcno

# gcov data file
hello-gcov-hello.gcda: hello-gcov
	unset GCOV_PREFIX && unset GCOV_PREFIX_STRIP && ./hello-gcov > /dev/null

# gcov report
hello.c.gcov hello.h.gcov: hello-gcov-hello.gcda
	gcov -o hello-gcov-hello.gcda hello.c hello.h

# lcov report
hello.info: hello-gcov-hello.gcda
	lcov --directory . --capture --rc branch_coverage=1 --output-file hello.info

# lcov html
lcov-html: hello.info
	rm -rf html
	mkdir html
	genhtml -o html/ --rc branch_coverage=1 hello.info

all: hello-vanilla hello-gcov

define test_msg
	@echo -e -n '\033[0;32m[$@]:$(1)\033[0m\n'
endef

test-no-relocate: hello-gcov
	$(call test_msg, started)
	@rm -f *.gcda
	@# the .gcda file is always generated at its build path
	mkdir -p gcov-subdir && cp hello-gcov gcov-subdir
	cd gcov-subdir && unset GCOV_PREFIX && unset GCOV_PREFIX_STRIP && ./hello-gcov > /dev/null
	@# expect data file to show up in build directory
	file *.gcda
	@# expect no file in executable storage directory / working directory
	! ls gcov-subdir | grep gcda
	@# clean up
	rm -rf gcov-subdir
	$(call test_msg, passed\n)

# relocate .gcda by env variable
test-relocate-var: hello-gcov
	$(call test_msg, started)
	@rm -rf /tmp/gcov-var
	@mkdir -p /tmp/gcov-var
	@# if build path = /d1/d2/d3/gcov, strip=2 would cause .gcda to be generated at prefix/d3
	@# it is safe to set GCOV_PREFIX_STRIP to higher value, e.g. 100 -- then you got abs path
	export GCOV_PREFIX="/tmp/gcov-var" ; export GCOV_PREFIX_STRIP=100 ; ./hello-gcov > /dev/null
	file /tmp/gcov-var/*
	@# clean up
	@rm -rf /tmp/gcov-var
	$(call test_msg, passed\n)

# relocate to hard-wired directory
test-relocate-hard: hello-gcov-hard
	$(call test_msg, started)
	@rm -rf /tmp/gcov-hard
	@mkdir -p /tmp/gcov-hard
	./hello-gcov-hard > /dev/null
	file /tmp/gcov-hard/*
	@# clean up
	@rm -rf /tmp/gcov-hard
	$(call test_msg, passed\n)

test-gcov: hello.c.gcov hello.h.gcov
	$(call test_msg, started)
	file hello.c.gcov hello.h.gcov
	$(call test_msg, passed)

test-lcov:
	$(call test_msg, started)
	make --always-make lcov-html
	tree html/
	$(call test_msg, passed)

test: test-no-relocate test-relocate-var test-relocate-hard test-gcov test-lcov
	$(call test_msg, All tests passed.)

clean:
	@# remove build output
	- rm -f hello-vanilla hello-gcov*
	@# remove gcov output
	- rm -f *.gcda *.gcno *.gcov
	@# remove lcov output
	- rm -rf *.info html/

.PHONY: lcov-html all clean test test-no-relocate test-relocate-var test-relocate-hard test-gcov \
	test-lcov

.DEFAULT_GOAL = test
