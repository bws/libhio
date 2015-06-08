/* -*- Mode: C; c-basic-offset:2 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2015      Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "hio_types.h"

int hio_dataset_unlink (hio_context_t ctx, const char *name, int64_t set_id, hio_unlink_mode_t mode) {
  hio_module_t *module;
  int rc = HIO_ERR_NOT_FOUND;

  if (NULL == ctx || NULL == name || 0 > set_id) {
    return HIO_ERR_BAD_PARAM;
  }

  if (0 == ctx->context_module_count) {
    /* create hio modules for each item in the specified data roots */
    rc = hioi_context_create_modules (ctx);
    if (HIO_SUCCESS != rc) {
      return rc;
    }
  }

  if (HIO_UNLINK_MODE_CURRENT == mode) {
    module = hioi_context_select_module (ctx);
    if (NULL == module) {
      hio_err_push (HIO_ERROR, ctx, NULL, "Could not select hio module");
      return HIO_ERR_NOT_FOUND;
    }

    return module->dataset_unlink (module, name, set_id);
  }


  for (int i = 0 ; i < ctx->context_module_count ; ++i) {
    module = ctx->context_modules[i];

    if (HIO_SUCCESS == module->dataset_unlink (module, name, set_id)) {
      rc = HIO_SUCCESS;
      if (HIO_UNLINK_MODE_FIRST == mode) {
        break;
      }
    }
  }

  return rc;
}
