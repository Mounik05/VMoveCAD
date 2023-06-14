//Headers Needed For Library Users
#ifndef _DATAKIT_H_
#define _DATAKIT_H_

#include <stdio.h>
#include <stddef.h>

#if !defined(__APPLE__)
#include <malloc.h>
#endif

#include "util/BaseTypes/dtk_pnt.hpp"
#include "util/BaseTypes/dtk_dir.hpp"
#include "util/BaseTypes/dtk_status.hpp"
#include "util/BaseTypes/dtk_string.hpp"
#include "util/BaseTypes/dtk_uuid.hpp"
#include "util/BaseTypes/dtk_rgb.hpp"
#include "util/dtk_picture.hpp"
#include "util/error_dtk.hpp"

#include "def/define.h"

#include "struct/str_feat_dtk.hpp"

#include "util/util_ent_dtk.hpp"
#include "util/util_topology_dtk.hpp"
#include "util/util_geom_dtk.hpp"
#include "util/util_draw_ptr_dtk.hpp"
#include "util/util_draw_dtk.hpp"
#include "util/util_mesh_dtk.hpp"
#include "util/util_cgr_dtk.hpp" 
#include "util/util_kinematics_dtk.hpp"
#include "util/util_selection_set.hpp"
#include "util/dtk_render.hpp"
#include "util/dtk_graphical.hpp"
#include "util/dtk_global_data_set.hpp"
#include "util/dtk_global_data_set_ptr.hpp"

//New DATAKIT API
#include "util/dtk_maindoc.hpp"
#include "util/dtk_maindoc_ptr.hpp"
#include "util/dtk_reader.hpp"
#include "util/dtk_log.hpp"
#include "util/dtk_api.hpp"



#endif
