#include "vdp1.private.h"
#include "core.h"

vdp1_hook_fn vdp1NewCommandsFetched = NULL;
vdp1_hook_fn vdp1BeforeDrawCall = NULL;
vdp1_hook_fn vdp1FrameCompleted = NULL;
