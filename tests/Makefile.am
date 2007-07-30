EXTRA_DIST = $(TESTSUITE_AT) testsuite.at testsuite package.m4 runtest.sh tests.in make_tests
DISTCLEANFILES       = atconfig $(check_SCRIPTS)
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE) package.m4

## ------------ ##
## package.m4.  ##
## ------------ ##

$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	{				      \
	  echo '# Signature of the current package.'; \
	  echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])'; \
	  echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])'; \
	  echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])'; \
	  echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])'; \
	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	  echo 'm4_define([AT_TESTS], [$(TESTSUITE_AT)])'; \
	} >$(srcdir)/package.m4

#

TESTSUITE = $(srcdir)/testsuite
check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) $(TESTSUITE)

clean-local:
	$(SHELL) $(TESTSUITE) --clean

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): package.m4 testsuite.at $(TESTSUITE_AT)
	$(AUTOTEST) -I $(srcdir) $@.at -o $@.tmp
	mv $@.tmp $@

TESTSUITE_AT = version.at tests.at run.at

tests.at: tests.in make_tests
	$(srcdir)/make_tests "$(srcdir)/tests.in"
