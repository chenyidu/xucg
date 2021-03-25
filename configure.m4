#
# Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
# See file LICENSE for terms.
#
ucg_modules=":xplan"
m4_include([src/ucg/core/configure.m4])
m4_include([src/ucg/algo/configure.m4])
m4_include([src/ucg/plan/configure.m4])
AC_DEFINE_UNQUOTED([ucg_MODULES], ["${ucg_modules}"], [UCG loadable modules])

AC_CONFIG_FILES([src/ucg/Makefile
                 src/ucg/api/ucg_version.h
                 src/ucg/core/ucg_version.c])