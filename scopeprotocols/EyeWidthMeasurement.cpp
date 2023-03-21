/***********************************************************************************************************************
*                                                                                                                      *
* libscopeprotocols                                                                                                    *
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

#include "scopeprotocols.h"
#include "EyeWidthMeasurement.h"
#include "EyePattern.h"
#include <algorithm>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

EyeWidthMeasurement::EyeWidthMeasurement(const string& color)
	: Filter(color, CAT_MEASUREMENT)
{
	m_xAxisUnit = Unit(Unit::UNIT_MILLIVOLTS);
	AddStream(Unit(Unit::UNIT_FS), "widthslice", Stream::STREAM_TYPE_ANALOG);
	AddStream(Unit(Unit::UNIT_FS), "minwidth", Stream::STREAM_TYPE_ANALOG_SCALAR);

	//Set up channels
	CreateInput("Eye");

	m_startname = "Start Voltage";
	m_parameters[m_startname] = FilterParameter(FilterParameter::TYPE_FLOAT, Unit(Unit::UNIT_VOLTS));
	m_parameters[m_startname].SetFloatVal(0);

	m_endname = "End Voltage";
	m_parameters[m_endname] = FilterParameter(FilterParameter::TYPE_FLOAT, Unit(Unit::UNIT_VOLTS));
	m_parameters[m_endname].SetFloatVal(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Factory methods

bool EyeWidthMeasurement::ValidateChannel(size_t i, StreamDescriptor stream)
{
	if(stream.m_channel == NULL)
		return false;

	if( (i == 0) && (stream.GetType() == Stream::STREAM_TYPE_EYE) )
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string EyeWidthMeasurement::GetProtocolName()
{
	return "Eye Width";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual decoder logic

void EyeWidthMeasurement::Refresh()
{
	if(!VerifyAllInputsOK(true))
	{
		SetData(nullptr, 0);
		m_streams[1].m_value = NAN;
		return;
	}

	//Get the input data
	auto din = dynamic_cast<EyeWaveform*>(GetInputWaveform(0));

	//Create the output
	auto cap = SetupEmptySparseAnalogOutputWaveform(din, 0);
	din->PrepareForCpuAccess();
	cap->PrepareForCpuAccess();
	cap->m_timescale = 1;

	//Make sure voltages are in the right order
	float vstart = m_parameters[m_startname].GetFloatVal();
	float vend = m_parameters[m_endname].GetFloatVal();
	if(vstart > vend)
	{
		float tmp = vstart;
		vstart = vend;
		vend = tmp;
	}

	//Figure out how many volts per eye bin and round everything to nearest eye bin
	float vrange = m_inputs[0].GetVoltageRange();
	float volts_per_row = vrange / din->GetHeight();
	float volts_at_bottom = din->GetCenterVoltage() - vrange/2;

	size_t start_bin = round( (vstart - volts_at_bottom) / volts_per_row);
	size_t end_bin = round( (vend - volts_at_bottom) / volts_per_row);
	start_bin = min(start_bin, din->GetHeight()-1);
	end_bin = min(end_bin, din->GetHeight()-1);
	float duration_mv = volts_per_row * 1000;
	float base_mv = volts_at_bottom * 1000;

	float* data = din->GetData();
	int64_t w = din->GetWidth();
	int64_t xcenter = w / 2;
	float ber_max = FLT_EPSILON;
	double width_fs = 2 * din->m_uiWidth;
	double fs_per_pixel = width_fs / w;
	int64_t far_left = INT64_MAX;
	int64_t far_right = INT64_MIN;
	for(size_t i=start_bin; i <= end_bin; i++)
	{
		float* row = data + i*w;

		int64_t cleft = 0;		//left side of eye opening
		int64_t cright = w-1;	//right side of eye opening

		//Find the edges of the eye in this scanline
		for(int64_t dx = 0; dx < xcenter; dx ++)
		{
			//left of center
			int64_t x = xcenter - dx;
			if(row[x] > ber_max)
				cleft = max(cleft, x);

			//right of center
			x = xcenter + dx;
			if(row[x] > ber_max)
				cright = min(cright, x);
		}

		far_left = min(far_left, cleft);
		far_right = max(far_right, cright);

		float value = fs_per_pixel * (cright - cleft);

		//Output waveform generation
		cap->m_offsets.push_back(round(i*duration_mv + base_mv));
		cap->m_durations.push_back(round(duration_mv));
		cap->m_samples.push_back(value);
	}

	m_streams[1].m_value = fs_per_pixel * (far_right - far_left);

	SetData(cap, 0);

	cap->MarkModifiedFromCpu();
}
