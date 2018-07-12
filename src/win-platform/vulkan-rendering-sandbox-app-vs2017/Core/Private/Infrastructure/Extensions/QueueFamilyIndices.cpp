#include "../../../Public/Infrastructure/Extensions/QueueFamilyIndices.hpp"

bool QueueFamilyIndices::IsComplete() const
{
	return graphicsFamily >= 0 && presentFamily >= 0;
}
