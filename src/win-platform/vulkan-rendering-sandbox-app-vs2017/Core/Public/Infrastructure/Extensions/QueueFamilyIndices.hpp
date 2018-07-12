#pragma once

#ifndef _QUEUE_FAMILY_INDICES_HPP_
#define	_QUEUE_FAMILY_INDICES_HPP_

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool IsComplete() const;
};

#endif
