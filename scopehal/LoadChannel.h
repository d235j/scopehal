/***********************************************************************************************************************
*                                                                                                                      *
* libscopehal v0.1                                                                                                     *
*                                                                                                                      *
* Copyright (c) 2012-2023 Andrew D. Zonenberg and contributors                                                         *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

#ifndef LoadChannel_h
#define LoadChannel_h

/**
	@brief A single channel of a power supply
 */
class LoadChannel : public InstrumentChannel
{
public:

	LoadChannel(
		const std::string& hwname,
		Load* load,
		const std::string& color = "#808080",
		size_t index = 0);

	virtual ~LoadChannel();

	/*
		Measured voltage/current

		Available but not using (can be rederived, no sense wasting bandwidth pulling from hardware):
			power
			resistance

		waveform?? need to learn more about this
	 */

	//Well defined stream IDs used by LoadChannel
	enum StreamIndexes
	{
		STREAM_VOLTAGE_MEASURED,
		STREAM_CURRENT_MEASURED,
		STREAM_SET_POINT
	};

	float GetVoltageMeasured()
	{ return GetScalarValue(STREAM_VOLTAGE_MEASURED); }

	float GetCurrentMeasured()
	{ return GetScalarValue(STREAM_CURRENT_MEASURED); }

	float GetSetPoint()
	{ return GetScalarValue(STREAM_SET_POINT); }

	virtual void Refresh(vk::raii::CommandBuffer& cmdBuf, std::shared_ptr<QueueHandle> queue) override;
	virtual bool ValidateChannel(size_t i, StreamDescriptor stream) override;

protected:
	Load* m_load;
};

#endif
