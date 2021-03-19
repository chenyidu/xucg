/*
* Copyright (C) Mellanox Technologies Ltd. 2001-2015.  ALL RIGHTS RESERVED.
* Copyright (C) Huawei Technologies Co., Ltd. 2019.  ALL RIGHTS RESERVED.
* See file LICENSE for terms.
*/


void ucg_get_version(unsigned *major_version, unsigned *minor_version,
                     unsigned *release_number)
{
    *major_version  = 1;
    *minor_version  = 10;
    *release_number = 0;
}

const char *ucg_get_version_string()
{
    return "1.10.0";
}
