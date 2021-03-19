#
# Copyright (c) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
# See file LICENSE for terms.
#

if HAVE_UCG

SUBDIRS = core plan algo

lib_LTLIBRARIES    = libucg.la
libucg_la_CFLAGS   = $(BASE_CFLAGS)
libucg_la_CPPFLAGS = $(BASE_CPPFLAGS)
libucg_la_LDFLAGS  = -ldl -version-info $(SOVERSION)
libucg_ladir       = $(includedir)/ucg
libucg_la_SOURCES  =
libucg_la_LIBADD   = \
	../ucs/libucs.la \
	../uct/libuct.la \
	../ucp/libucp.la \
	algo/libucg_algo.la \
	plan/libucg_plan.la \
	core/libucg_core.la

nobase_dist_libucg_la_HEADERS = \
	api/ucg_def.h \
	api/ucg_dt.h \
	api/ucg_rte_mpi.h \
	api/ucg_version.h \
	api/ucg.h

endif # HAVE_UCG
