#include <ucg/plans/tree/algo/binomial_tree.h>
#include <ucg/plans/tree/algo/knomial_tree.h>
#include <ucg/plans/base/action.h>
#include <ucg/plans/base/plan.h>
#include <ucg/plans/ucg_ppool_impl.h>
#include <ucs/sys/compiler_def.h>

typedef struct ucg_plan_ktree_bcast_config {
    ucg_plan_tree_bcast_config_t super;
    uint32_t degree;
} ucg_plan_ktree_bcast_config_t;

static ucs_status_t ucg_plan_ktree_bcast_calc_tree_node(void *params,
                                                        ucg_plan_tree_node_t *tree_node)
{
    // For bcast, leftmost tree has better parallelism.
    return ucg_plan_ktree_left((ucg_plan_ktree_params_t*)params, &tree_node);
}

static ucs_config_field_t ucg_plan_ktree_bcast_config_table[] = {
    {"GROUP_SIZE", "8", 
     "Specifies the threshold for the root switch method.\n"
     "<= threshold: Swap locations of new and old root.\n"
     "> threshold : New root forward data to new root.",
     ucs_offsetof(ucg_plan_ktree_bcast_config_t, group_size), UCS_CONFIG_TYPE_UINT},
    NULL,
};

static ucg_plan_core_t g_ktree_bcast_core = {
    .type = UCG_PLAN_TYPE_BCAST,
    .id = UCG_PLAN_BCAST_ID_KTREE,
    .desc = "Non topo-aware knomial tree broadcast",
    .flags = UCG_PLAN_CAP_FLAG_SWITCH_ROOT,
    .config_entry = {
        .name = "ktree bcast", 
        .prefix = "KTREE_BCAST",
        .table = ucg_plan_ktree_bcast_config_table,
        .size = sizeof(ucg_plan_ktree_bcast_config_t),
    },
    .clone = ucg_plan_tree_bcast_clone,
    .destroy = ucg_plan_tree_bcast_destroy,
};

static ucg_plan_tree_bcast_t g_ktree_bcast = {
    .super = {
        .super = {
            .refcount = 1, /* Never release. */
            .core = &g_btree_bcast_core,
        },
    },
    .calc_tree_node = ucg_plan_btree_bcast_calc_tree_node,
};
UCG_PPOOL_REGISTER_PLAN(g_btree_bcast);