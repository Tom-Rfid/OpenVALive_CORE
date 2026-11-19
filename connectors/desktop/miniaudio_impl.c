/*
 * OpenVALive desktop - the miniaudio implementation lives in this unit only.
 * miniaudio is public domain / MIT-0 (third_party/miniaudio).
 */
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_DECODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#include "../../third_party/miniaudio/miniaudio.h"
