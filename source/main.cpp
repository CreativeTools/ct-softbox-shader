#include "c4d.h"
#include <string.h>

Bool RegisterSoftbox(void);

Bool PluginStart(void)
{
	if (!RegisterSoftbox()) return FALSE;
	return TRUE;
}

void PluginEnd(void)
{
}

Bool PluginMessage(LONG id, void *data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init()) return FALSE;
			return TRUE;

		case C4DMSG_PRIORITY:
			return TRUE;

		case C4DPL_BUILDMENU:
			break;

		case C4DPL_COMMANDLINEARGS:
			break;

		case C4DPL_EDITIMAGE:
			return FALSE;
	}

	return FALSE;
}
