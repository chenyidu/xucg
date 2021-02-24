
#include <ucg/base/ucg_plan_derived.h>

struct ucg_plan_ktree_bcast_config {
    ucg_config_t super;
    int refcount;
    uint32_t degree;
} ucg_plan_ktree_bcast_config_t;

static ucs_config_field_t ucg_plan_ktree_bcast_config_table[] = {
    {"DEGREE", "2", "Degree of k-nomial tree.", ucs_offsetof(ucg_plan_ktree_bcast_config_t, degree), UCS_CONFIG_TYPE_UINT},
    {NULL},
};
UCS_CONFIG_REGISTER_TABLE(ucg_plan_ktree_bcast_config_table, "K-nomial tree bcast", 
                          "KTREE_BCAST", ucg_plan_ktree_bcast_config_t);

int ucg_plan_ktree_bcast_is_available()
{

}

ucg_plan_t* ucg_plan_ktree_bcast_init(ucg_plan_infra_t *infra, ucg_plan_params_t *params)
{

}

ucg_plan_t* ucg_plan_ktree_bcast_clone(ucg_plan_t *plan, ucg_plan_params_t *params)
{

}


UCG_PLAN_INFRA_DEFINE(UCG_PLAN_ID_KTREE_BCAST, UCG_PLAN_TYPE_BCAST, 
                      "K-noimal tree based broadcast.",
                      ucg_plan_ktree_bcast_is_available, 
                      ucg_plan_ktree_bcast_query, 
                      ucg_plan_ktree_bcast_init,
                      ucg_plan_ktree_bcast_clone,
                      ucg_plan_ktree_bcast_release)