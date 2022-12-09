/*
 * PercivalFrameDecoderLib.cpp
 *
 *  Created on: 7 Mar 2017
 *      Author: gnx91527
 */

#include "PercivalFrameDecoder.h"
#include "ClassLoader.h"

namespace FrameReceiver
{
	/**
	 * Registration of this decoder through the ClassLoader.  This macro
	 * registers the class without needing to worry about name mangling
	 */
	REGISTER(FrameDecoder, PercivalFrameDecoder, "PercivalFrameDecoder");

} // namespace FrameReceiver




