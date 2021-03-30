#include "ucg_group.h"
#include "ucg_util.h"
#include "ucg_request.h"

ucs_status_t ucg_group_apply_params(ucg_group_t *group, const ucg_group_params_t *params)
{
    ucs_status_t status = UCS_ERR_INVALID_PARAM;
    uint64_t field_mask = params->field_mask;

    UCG_CHECK_REQUIRED_FIELD(field_mask, UCG_GROUP_PARAMS_UCP_WORKER, err);
    group->ucp_worker = params->ucp_worker;

    UCG_CHECK_REQUIRED_FIELD(field_mask, UCG_GROUP_PARAMS_GROUP_ID, err);
    group->id = params->id;

    UCG_CHECK_REQUIRED_FIELD(field_mask, UCG_GROUP_PARAMS_GROUP_MEMBERS, err);
    status = ucg_group_members_clone(&params->members, &group->members);
    if (status != UCS_OK) {
        return status;
    }

    return UCS_OK;
err:
    return UCS_ERR_INVALID_PARAM;
}

// Release all resources allocated by ucg_group_apply_params()
void ucg_group_release_params(ucg_group_t *group)
{
    ucg_group_members_free(&group->members);
    return;
}

ucs_status_t ucg_group_create(ucg_context_h context, 
                              const ucg_group_params_t *params,
                              ucg_group_h *group)
{
    UCG_CHECK_PARAMS(context != NULL && params != NULL && group != NULL);

    ucg_group_t *grp = ucs_malloc(sizeof(ucg_group_t), "ucg group");
    if (grp == NULL) {
        return UCS_ERR_NO_MEMORY;
    }

    grp->context = context;
    ucs_status_t status = ucg_group_apply_params(grp, params);
    if (status != UCS_OK) {
        goto err_free_group;
    }

    grp->is_barrier = 0;
    grp->next_req_id = 0;
    ucs_callbackq_init(&grp->progress_q);
    
    *group = grp;
    return UCS_OK;
err_release_params:
    ucg_group_release_params(grp);
err_free_group:
    ucs_free(grp);
    return status;
}

void ucg_group_destroy(ucg_group_h group)
{
    UCG_CHECK_PARAMS_VOID(group != NULL);
    ucs_callbackq_cleanup(&group->progress_q);
    ucg_group_release_params(group);
    ucs_free(group);
    return;
}

int ucg_group_progress(ucg_group_h group)
{
    UCG_CHECK_PARAMS_ZERO(group != NULL);
    return ucs_callbackq_dispatch(&group->progress_q);
}