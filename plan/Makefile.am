#
# Copyright (c) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
# See file LICENSE for terms.
#
base = base/action.h \
       base/action.c \
       base/plan.h \
       base/plan.c 

bcast = bcast/bcast_tree.c 

sources = $(base) \
          $(bcast) 

noinst_LTLIBRARIES      = libucg_plan.la
libucg_plan_la_SOURCES  = $(sources)
libucg_plan_la_CFLAGS   = $(BASE_CFLAGS)
libucg_plan_la_CPPFLAGS = $(BASE_CPPFLAGS)