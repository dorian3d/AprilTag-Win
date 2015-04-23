#pragma once

#include "pch.h"
#include "pxcsensemanager.h"
#include "pxcmetadata.h"

namespace RealityCap
{
	class VideoManager
	{
	public:
		VideoManager();
		~VideoManager();
		
		bool StartVideo(); // returns true if video started successfully
		void StopVideo();
		void SetDelegate(PXCSenseManager::Handler* handler); // must be called before StartVideo()

	private:		
		PXCSenseManager* senseMan;
		PXCSenseManager::Handler* sampleHandler;
	};
}