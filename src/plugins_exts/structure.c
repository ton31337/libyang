/**
 * @file structure.c
 * @author Michal Vasko <mvasko@cesnet.cz>
 * @brief libyang extension plugin - strcture (RFC 8791)
 *
 * Copyright (c) 2022 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "libyang.h"
#include "plugins_exts.h"

struct lysc_ext_instance_structure {
    struct lysc_must *musts;
    uint16_t flags;
    const char *dsc;
    const char *ref;
    struct lysp_tpdf *typedefs;
    struct lysp_node_grp *groupings;
    struct lysc_node *child;
};

/**
 * @brief Compile structure extension instances.
 *
 * Implementation of ::lyplg_ext_compile_clb callback set as lyext_plugin::compile.
 */
static LY_ERR
structure_compile(struct lysc_ctx *cctx, const struct lysp_ext_instance *p_ext, struct lysc_ext_instance *c_ext)
{
    LY_ERR ret;
    LY_ARRAY_COUNT_TYPE u;
    struct lysc_module *mod_c;
    const struct lysc_node *child;
    struct lysc_ext_instance_structure *struct_ext;
    uint32_t prev_options = *lysc_ctx_get_options(cctx);

    /* structure can appear only at the top level of a YANG module or submodule */
    if ((c_ext->parent_stmt != LY_STMT_MODULE) && (c_ext->parent_stmt != LY_STMT_SUBMODULE)) {
        lyplg_ext_log(c_ext, LY_LLERR, LY_EVALID, lysc_ctx_get_path(cctx),
                "Extension %s must not be used as a non top-level statement in \"%s\" statement.",
                p_ext->name, ly_stmt2str(c_ext->parent_stmt));
        return LY_EVALID;
    }

    mod_c = (struct lysc_module *)c_ext->parent;

    /* check identifier namespace */
    LY_ARRAY_FOR(mod_c->exts, u) {
        if ((&mod_c->exts[u] != c_ext) && (mod_c->exts[u].def == c_ext->def) && !strcmp(mod_c->exts[u].argument, c_ext->argument)) {
            /* duplication of the same structure extension in a single module */
            lyplg_ext_log(c_ext, LY_LLERR, LY_EVALID, lysc_ctx_get_path(cctx), "Extension %s is instantiated multiple times.", p_ext->name);
            return LY_EVALID;
        }
    }
    LY_LIST_FOR(mod_c->data, child) {
        if (!strcmp(child->name, c_ext->argument)) {
            /* identifier collision */
            lyplg_ext_log(c_ext, LY_LLERR, LY_EVALID, lysc_ctx_get_path(cctx), "Extension %s collides "
                    "with a %s with the same identifier.", p_ext->name, lys_nodetype2str(child->nodetype));
            return LY_EVALID;
        }
    }

    /* allocate the storage */
    struct_ext = calloc(1, sizeof *struct_ext);
    if (!struct_ext) {
        goto emem;
    }
    c_ext->data = struct_ext;

    /* compile substatements */
    LY_ARRAY_CREATE_GOTO(cctx->ctx, c_ext->substmts, 14, ret, emem);
    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[0].stmt = LY_STMT_MUST;
    c_ext->substmts[0].cardinality = LY_STMT_CARD_ANY;
    c_ext->substmts[0].storage = &struct_ext->musts;

    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[1].stmt = LY_STMT_STATUS;
    c_ext->substmts[1].cardinality = LY_STMT_CARD_OPT;
    c_ext->substmts[1].storage = &struct_ext->flags;

    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[2].stmt = LY_STMT_DESCRIPTION;
    c_ext->substmts[2].cardinality = LY_STMT_CARD_OPT;
    c_ext->substmts[2].storage = &struct_ext->dsc;

    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[3].stmt = LY_STMT_REFERENCE;
    c_ext->substmts[3].cardinality = LY_STMT_CARD_OPT;
    c_ext->substmts[3].storage = &struct_ext->ref;

    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[4].stmt = LY_STMT_TYPEDEF;
    c_ext->substmts[4].cardinality = LY_STMT_CARD_ANY;
    c_ext->substmts[4].storage = &struct_ext->typedefs;

    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[5].stmt = LY_STMT_GROUPING;
    c_ext->substmts[5].cardinality = LY_STMT_CARD_ANY;
    c_ext->substmts[5].storage = &struct_ext->groupings;

    /* data-def-stmt */
    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[6].stmt = LY_STMT_CONTAINER;
    c_ext->substmts[6].cardinality = LY_STMT_CARD_ANY;
    c_ext->substmts[6].storage = &struct_ext->child;

    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[7].stmt = LY_STMT_LEAF;
    c_ext->substmts[7].cardinality = LY_STMT_CARD_ANY;
    c_ext->substmts[7].storage = &struct_ext->child;

    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[8].stmt = LY_STMT_LEAF_LIST;
    c_ext->substmts[8].cardinality = LY_STMT_CARD_ANY;
    c_ext->substmts[8].storage = &struct_ext->child;

    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[9].stmt = LY_STMT_LIST;
    c_ext->substmts[9].cardinality = LY_STMT_CARD_ANY;
    c_ext->substmts[9].storage = &struct_ext->child;

    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[10].stmt = LY_STMT_CHOICE;
    c_ext->substmts[10].cardinality = LY_STMT_CARD_ANY;
    c_ext->substmts[10].storage = &struct_ext->child;

    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[11].stmt = LY_STMT_ANYDATA;
    c_ext->substmts[11].cardinality = LY_STMT_CARD_ANY;
    c_ext->substmts[11].storage = &struct_ext->child;

    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[12].stmt = LY_STMT_ANYXML;
    c_ext->substmts[12].cardinality = LY_STMT_CARD_ANY;
    c_ext->substmts[12].storage = &struct_ext->child;

    LY_ARRAY_INCREMENT(c_ext->substmts);
    c_ext->substmts[13].stmt = LY_STMT_USES;
    c_ext->substmts[13].cardinality = LY_STMT_CARD_ANY;
    c_ext->substmts[13].storage = &struct_ext->child;

    *lysc_ctx_get_options(cctx) |= LYS_COMPILE_NO_CONFIG | LYS_COMPILE_NO_DISABLED;
    ret = lys_compile_extension_instance(cctx, p_ext, c_ext);
    *lysc_ctx_get_options(cctx) = prev_options;
    if (ret) {
        return ret;
    }

    return LY_SUCCESS;

emem:
    lyplg_ext_log(c_ext, LY_LLERR, LY_EMEM, lysc_ctx_get_path(cctx), "Memory allocation failed (%s()).", __func__);
    return LY_EMEM;
}

/**
 * @brief INFO printer
 *
 * Implementation of ::lyplg_ext_schema_printer_clb set as ::lyext_plugin::sprinter
 */
static LY_ERR
structure_schema_printer(struct lyspr_ctx *ctx, struct lysc_ext_instance *ext, ly_bool *flag)
{
    lysc_print_extension_instance(ctx, ext, flag);
    return LY_SUCCESS;
}

/**
 * @brief Free structure extension instances' data.
 *
 * Implementation of ::lyplg_clb_free_clb callback set as lyext_plugin::free.
 */
static void
structure_free(struct ly_ctx *ctx, struct lysc_ext_instance *ext)
{
    lyplg_ext_instance_substatements_free(ctx, ext->substmts);
    free(ext->data);
}

/**
 * @brief Plugin descriptions for the structure extension
 *
 * Note that external plugins are supposed to use:
 *
 *   LYPLG_EXTENSIONS = {
 */
const struct lyplg_ext_record plugins_structure[] = {
    {
        .module = "ietf-yang-structure-ext",
        .revision = "2020-06-17",
        .name = "structure",

        .plugin.id = "libyang 2 - structure, version 1",
        .plugin.compile = structure_compile,
        .plugin.sprinter = structure_schema_printer,
        .plugin.free = structure_free,
        .plugin.node = NULL,
        .plugin.snode = NULL,
        .plugin.validate = NULL
    },
    {0}     /* terminating zeroed record */
};