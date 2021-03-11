#include <ucg/plans/tree/binomial_tree.h>
#include <ucg/plans/base/ucg_plan_base.h>
#include <ucg/plans/ucg_plan_impl.h>
#include <ucs/sys/compiler_def.h>

static ucg_plan_t* ucg_plan_btree_bcast_clone(ucg_plan_t *plan, 
                                              ucg_plan_params_t *params,
                                              ucg_plan_params_cmp_t cmp);

static void ucg_plan_btree_bcast_release(ucg_plan_t *plan);

static ucg_plan_core_t g_btree_bcast_core = {
    .type = UCG_PLAN_TYPE_BCAST,
    .id = UCG_PLAN_BCAST_ID_BTREE,
    .desc = "Non topo-aware binomial tree broadcast",
    .flags = UCG_PLAN_CAP_FLAG_SWITCH_ROOT,
    .config = { /* No configuration. */
        .name = NULL, 
        .prefix = NULL,
        .table = NULL,
        .size = 0,
    },
    .clone = ucg_plan_btree_bcast_clone,
    .destroy = ucg_plan_btree_bcast_destroy,
};

static ucg_plan_bcast_t g_btree_bcast = {
    .super = {
        .refcount = 1, /* Never release. */
        .core = &g_btree_bcast_core,
        .phase = NULL,
    },
};
UCG_PLAN_REGISTER(g_btree_bcast)

static ucs_status_t ucg_plan_btree_bcast_calc_tree_node(int root, int self, uint32_t size, 
                                                        ucg_plan_btree_node_t *tree_node)
{
    ucg_plan_btree_params_t btree_params = {
        .self = self,
        .root = root,
        .size = size,
    };

    return ucg_plan_btree_left(&btree_params, &tree_node);
}

typedef struct ucg_plan_bcast_phase_params {
    int root;
    int self;
    uint32_t count;
    
} ucg_plan_bcast_phase_params_t;
static ucs_status_t ucg_plan_btree_bcast_phase_init_core(ucg_plan_phase_t *phase,
                                                         ucg_plan_bcast_params_t *params)
{
    ucs_assert(phase->core != NULL);
    // Calcuate my father and children in the tree.
    int child[UCG_PLAN_PHASE_PEERS_MAX_NUM] = {0};
    ucg_plan_btree_node_t node = {
        .child = child,
        .child_cnt = UCG_PLAN_PHASE_PEERS_MAX_NUM,
    };
    
    ucs_status_t status = ucg_plan_btree_bcast_calc_tree_node(params->root,
                                                              params->super.members.self, 
                                                              params->super.members.count, 
                                                              &node);
    if (status != UCS_OK) {
        ucs_error("Fail to calculate binomial tree node, %s.", ucs_status_string(status));
        return status;
    }

    // Setup phase action
    ucg_plan_phase_action_t *actions = phase->core->actions;
    ucg_plan_phase_peers_t *peers = phase->core->peers;
    int count = 0;
    if (node.father_cnt == 1) {
        // RECV from father
        actions[count] = UCG_PLAN_PHASE_ACTION_RECV;
        peers[count].count = 1;
        peers[count].ranks[0] = node.father;
        count++;
    }
    
    int child_cnt = node.child_cnt;
    int *child = node.child;
    if (child_cnt > 0) {
        // FORWARD or SEND to child
        actions[count] = node.father_cnt == 0 ? UCG_PLAN_PHASE_ACTION_SEND : UCG_PLAN_PHASE_ACTION_FORWARD;
        peers[count].count = child_cnt;
        int *ranks = peers[count].ranks;
        for (int i = 0 ; i < child_cnt; ++i) {
            ranks[i] = child[i];
        }
        count++;
    }

    if (node.father_cnt == 1) {
        /* Let COPY after FORWARD, so that COPY and FORWARD may be in parallel.
           FORWARD use NIC, COPY use CPU. */
        actions[count] = UCG_PLAN_PHASE_ACTION_COPY;
        peers[count].count = 1;
        peers[count].ranks[0] = node.father;
        count++;
    }
    phase->core->count = count;
    return UCS_OK;
}

static void ucg_plan_btree_bcast_phase_init_buffer(ucg_plan_phase_t *phase,
                                                   ucg_plan_bcast_params_t *params)
{
    uint64_t length = ucg_dt_size(params->dtype) * params->count;
    ucg_plan_phase_buffer_vecs_t *bvectors = phase->bvectors;    
    for (int i = 0; i < UCG_PLAN_PHASE_ACTION_MAX; ++i) {
        ucg_plan_phase_action_t action = phase->core->actions[i];
        ucg_plan_phase_peers_t *peers = &phase->core->peers[i];
        uint8_t *buffer = UCG_BUFFER_MSG_HOLDER;
        switch (action) {
            case UCG_PLAN_PHASE_ACTION_RECV:
                bvectors[i].count = 1;
                bvectors[i].vec[0].buffers[0].length = length;
                break;
            case UCG_PLAN_PHASE_ACTION_SEND:
                buffer = params->buffer; // fall through
            case UCG_PLAN_PHASE_ACTION_FORWARD:
                ucg_plan_phase_buffer_vec_t *vec = bvectors[i].vec;
                int child_cnt = peers->count;
                for (int i = 0 ; i < child_cnt; ++i) {
                    vec[i].buffers[0].length = length;
                    vec[i].buffers[0].first = buffer;
                }
                break;
            case UCG_PLAN_PHASE_ACTION_COPY:
                bvectors[i].vec[0].buffers[0].length = length;
                bvectors[i].vec[0].buffers[0].first = UCG_BUFFER_MSG_HOLDER;
                bvectors[i].vec[0].buffers[0].second = params->buffer;
                break;
            case UCG_PLAN_PHASE_ACTION_NOP:
                goto finish;
            default:
                ucs_fatal("Unknow action.");
                break;
        }
    }
finish:
    return;
}

static ucs_status_t ucg_plan_btree_bcast_create_phase(ucg_plan_bcast_params_t *params, 
                                                      ucg_plan_phase_t **phase)
{
    int with_phase_core = 1;
    ucg_plan_phase_t *new_phase = ucg_plan_phase_allocate(with_phase_core);
    if (new_phase == NULL) {
        return UCS_ERR_NO_MEMORY;
    }
    // Set the members so that the core can be initialized according to the members
    ucg_plan_phase_members_t members = {
        .root = params->root,
        .self = params->super.members.self,
        .count = params->super.members.count,
        .mh = params->super.members.mh,
    };
    ucs_status_t status = ucg_plan_phase_set_member(new_phase, &members);
    if (status != UCS_OK) {
        return status;
    }

    status = ucg_plan_btree_bcast_phase_init_core(new_phase, params);
    if (status != UCS_OK) {
        return status;
    }
    ucg_plan_btree_bcast_phase_init_buffer(new_phase, params);
    
    *phase = new_phase;
    return UCS_OK;
}

static ucs_status_t ucg_plan_btree_bcast_arm_plan(ucg_plan_bcast_t *plan,
                                                  ucg_plan_phase_t *phase,
                                                  ucg_plan_bcast_params_t *params)
{
    // NOTE: dt state is delayed until it is actually used. 
    plan->super.core = ucg_plan_btree_bcast_core_obtain();
    plan->super.phase = phase;
    // save parameters
    plan->params.super.type = params->super.type;
    ucs_assert(params->super.config == NULL);
    plan->params.super.config = NULL;
    plan->params.super.members.count = params->super.members.count;
    plan->params.super.members.self = params->super.members.self;
    int alloc_size = plan->params.super.members.count * sizeof(uint64_t);
    uint64_t *mh = ucs_malloc(alloc_size, "plan members");
    if (mh == NULL) {
        return UCS_ERR_NO_MEMORY;
    }
    memcpy(plan->params.super.members.mh, params->super.members.mh, alloc_size);
    return UCS_OK;
}

static void ucg_plan_btree_bcast_cleanup(ucg_plan_t *bplan)
{
    ucg_plan_bcast_t *plan = ucs_container_of(bplan, ucg_plan_bcast_t, super);
    if (plan == NULL) {
        return;
    }

    if (plan->super.dt.pack_state != NULL) {
        ucg_dt_state_finish(plan->super.dt.pack_state);
        plan->super.dt.pack_state = NULL;
    }

    if (plan->super.dt.unpack_state != NULL) {
        ucg_dt_state_finish(plan->super.dt.unpack_state);
        plan->super.dt.unpack_state = NULL;
    }

    if (plan->params.super.members.mh != NULL) {
        ucs_free(plan->params.super.members.mh);
        plan->params.super.members.mh = NULL;
    }
    
    ucg_plan_phase_t *phase = plan->super.phase;
    if (phase != NULL) {
        ucs_assert(phase->next == NULL);
        ucg_plan_phase_release(phase, NULL);
        plan->super.phase = NULL;
    }

    return;
}

static ucg_plan_t* ucg_plan_btree_bcast_init(ucg_plan_bcast_t *plan, 
                                            ucg_plan_bcast_params_t *params)
{
    // Step 1: create phase
    ucg_plan_phase_t *phase = NULL;
    ucs_status_t status = ucg_plan_btree_bcast_create_phase(params, &phase);
    if (status != UCS_OK) {
        ucs_error("Fail to initialize phase, %s.", ucs_status_string(status));
        return status;
    }
    // Step 2: arm plan
    status = ucg_plan_btree_bcast_arm_plan(plan, params, phase);
    if (status != UCS_OK) {
        ucs_error("Fail to arm plan, %s.", ucs_status_string(status));
        return status;
    }
    
    return UCS_OK;
}

static ucs_status_t ucg_plan_btree_bcast_clone_phase(ucg_plan_bcast_t *plan,
                                                     ucg_plan_bcast_params_t *params,
                                                     ucg_plan_phase_t **phase)
{
    ucg_plan_phase_t *old_phase = plan->super.phase;
    ucs_assert(old_phase->next == NULL);
    ucs_assert(old_phase->members.count == params->super.members.count);

    int with_core = 0;
    if (params->root != plan->params.root
        || params->super.members.self != old_phase->members.self) {
        with_core = 1;
    }

    ucg_plan_phase_t *new_phase = ucg_plan_phase_allocate(with_core);
    if (new_phase == NULL) {
        return UCS_ERR_NO_MEMORY;
    }
    ucs_status_t status = UCS_OK;
    if (with_core) {
        status = ucg_plan_phase_set_member(new_phase, &params->super.members);
        if (status != UCS_OK) {
            ucg_plan_phase_release(new_phase, ucg_plan_btree_bcast_phase_cleanup);
            return status;
        }
        if (params->root != plan->params.root) {
            // switch root

        }
        ucg_plan_btree_bcast_phase_init_core(new_phase, params);
    } else {
        new_phase->core = ucg_plan_phase_obtain_core(plan->super.phase);
    }
    ucg_plan_btree_bcast_phase_init_buffer(new_phase, params);
    
    *phase = new_phase;
    return UCS_OK;
}

static ucg_plan_t* ucg_plan_btree_bcast_similar_clone(ucg_plan_bcast_t* plan,
                                                      ucg_plan_bcast_params_t *params)
{
    ucg_plan_bcast_t *new_plan = ucg_plan_allocate(sizeof(ucg_plan_bcast_t));
    if (new_plan == NULL) {
        ucs_error("Fail to allocate plan object.");
        return NULL;
    }
    ucs_status_t status = ucg_plan_btree_bcast_clone_phase(plan, new_plan, params);
    if (status != UCS_OK) {
        ucg_plan_release(new_plan, ucg_plan_btree_bcast_cleanup);
        return NULL;
    }
    return &new_plan->super;
}

static ucg_plan_t* ucg_plan_btree_bcast_unequal_clone(ucg_plan_bcast_t *plan,
                                                      ucg_plan_bcast_params_t *params)
{
    ucs_status_t status = UCS_OK;
    if (plan->super.phase == NULL) {
         // The plan is not initialized, use its space.
        status = ucg_plan_btree_bcast_init(plan, params);
        if (status != UCS_OK) {
            ucg_plan_btree_bcast_cleanup(&plan->super);
            return NULL;
        }
        return &plan->super;
    }

    ucg_plan_bcast_t *new_plan = ucg_plan_allocate(sizeof(ucg_plan_bcast_t));
    if (new_plan == NULL) {
        ucs_error("Fail to allocate plan object.");
        return NULL;
    }
    status = ucg_plan_btree_bcast_init(new_plan, params);
    if (status != UCS_OK) {
        ucg_plan_release(new_plan, ucg_plan_btree_bcast_cleanup);
        return NULL;
    }
    return &new_plan->super;
}

ucg_plan_t* ucg_plan_btree_bcast_clone(ucg_plan_t *bplan, 
                                       ucg_plan_params_t *bparams,
                                       ucg_plan_params_cmp_t cmp)
{
    /* If the caller already knows it's all the same, don't call me. */
    ucs_assert(cmp != UCG_PLAN_PARAMS_CMP_SAME);

    ucg_plan_bcast_t *plan = ucs_container_of(bplan, ucg_plan_bcast_t, super);
    ucg_plan_bcast_params_t *params = ucs_container_of(bparams, ucg_plan_bcast_params_t, super);
    if (cmp == UCG_PLAN_PARAMS_CMP_SIMILAR) {
        return ucg_plan_btree_bcast_similar_clone(plan, params);
    }

    if (cmp == UCG_PLAN_PARAMS_CMP_UNEQUAL) {
        return ucg_plan_btree_bcast_unequal_clone(plan, params);
    }
    // Should never be here.
    ucs_assert(0);
    return NULL;
}

static void ucg_plan_btree_bcast_destroy(ucg_plan_t *bplan)
{
    bplan->core->refcount--;
    ucg_plan_release(bplan, ucg_plan_btree_bcast_cleanup);
    return;
}