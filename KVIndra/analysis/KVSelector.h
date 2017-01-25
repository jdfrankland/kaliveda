// Dummy header file to switch between old or new selector class
#ifdef WITH_NEW_INDRA_SELECTOR
#include "KVINDRAEventSelector.h"
typedef KVINDRAEventSelector KVSelector;
#else
#include "KVOldINDRASelector.h"
typedef KVOldINDRASelector KVSelector;
#endif
