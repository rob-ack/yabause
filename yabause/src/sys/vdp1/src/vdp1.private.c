#include "vdp1.private.h"
#include "core.h"

vdp1_hook_fn vdp1NewCommandsFetchedHook = NULL;
vdp1_hook_fn vdp1BeforeDrawCallHook = NULL;
vdp1_hook_fn vdp1DrawCompletedHook = NULL;
vdp1_hook_fn vdp1FrameCompletedHook = NULL;
