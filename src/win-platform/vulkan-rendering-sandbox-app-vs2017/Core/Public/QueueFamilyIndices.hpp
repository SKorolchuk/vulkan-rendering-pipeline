#pragma once

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool IsComplete() const;
};
