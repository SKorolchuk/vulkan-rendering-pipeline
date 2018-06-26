#include <stdafx.h>

bool QueueFamilyIndices::IsComplete() const
{
	return graphicsFamily >= 0 && presentFamily >= 0;
}
