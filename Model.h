/*
Copyright(c) 2021 jvde.github@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "Stream.h"
#include "DSP.h"
#include "Device.h"
#include "AIS.h"
#include "Demod.h"
#include "Utilities.h"
#include "IO.h"

namespace AIS
{
	// Abstract demodulation model
	class Model
	{
	protected:
		std::string name = "";

		Device::DeviceBase* control;
		Util::Timer<RAW> timer;
		Util::PassThrough<NMEA> output;

		bool fixedpointDS = false;

	public:

		Model(Device::DeviceBase* ctrl)
		{
			control = ctrl;
		}

		virtual void buildModel(int, bool) {}

		StreamOut<NMEA>& Output() { return output; }

		void setName(std::string s) { name = s; }
		std::string getName() { return name; }

		float getTotalTiming() { return timer.getTotalTiming(); }

		virtual void setFixedPointDownsampling(bool b) { fixedpointDS = b; }
	};


	// common downsampling model 
	class ModelFrontend : public Model
	{
	private:
		DSP::DownsampleKFilter DSK;
		DSP::Downsample2CIC5 DS2_1, DS2_2, DS2_3, DS2_4, DS2_5, DS2_6, DS2_7;
		DSP::Downsample2CIC5 DS2_a, DS2_b;
		DSP::Upsample US;
		DSP::FilterCIC5 FCIC5_a, FCIC5_b;
		DSP::FilterComplex FFIR_a, FFIR_b;
		DSP::Downsample16Fixed DS16_Fixed;

		Util::ConvertRAW convert;

	protected:
		const int nSymbolsPerSample = 48000 / 9600;

		Util::PassThrough<CFLOAT32> C_a, C_b;
		DSP::Rotate ROT;
	public:
		ModelFrontend(Device::DeviceBase* c) : Model(c) {}
		void buildModel(int, bool);
	};

	// Standard demodulation model
	class ModelStandard : public ModelFrontend
	{
		DSP::FMDemodulation FM_a, FM_b;

		DSP::Filter FR_a, FR_b;
		std::vector<AIS::Decoder> DEC_a, DEC_b;
		DSP::SamplerParallel S_a, S_b;

	public:
		ModelStandard(Device::DeviceBase* c) : ModelFrontend(c) {}
		void buildModel(int, bool);
	};


	// Base model for development purposes, simplest and fastest
	class ModelBase : public ModelFrontend
	{
		DSP::FMDemodulation FM_a, FM_b;
		DSP::Filter FR_a, FR_b;
		DSP::SamplerPLL sampler_a, sampler_b;
		AIS::Decoder DEC_a, DEC_b;

	public:
		ModelBase(Device::DeviceBase* c) : ModelFrontend(c) {}
		void buildModel(int, bool);
	};

	// simple model embedding some elements of a coherent model with local phase estimation
	class ModelCoherent : public ModelFrontend
	{
		DSP::SquareFreqOffsetCorrection CGF_a, CGF_b;
		std::vector<DSP::CoherentDemodulation> CD_a, CD_b;

		DSP::FilterComplex FC_a, FC_b;
		std::vector<AIS::Decoder> DEC_a, DEC_b;
		DSP::SamplerParallelComplex S_a, S_b;

		int nHistory = 8;
		int nDelay = 0;

	public:
		ModelCoherent(Device::DeviceBase* c, int h = 12, int d = 3) : ModelFrontend(c)
		{
			setName("AIS engine " VERSION);
			nHistory = h; nDelay = d;

		}
		void buildModel(int,bool);
	};

	// Challenger model, some small improvements to test before moving into the default engine
	class ModelChallenger : public ModelCoherent
	{
	public:
		ModelChallenger(Device::DeviceBase* c, int h = 8, int d = 0) : ModelCoherent(c, h, d) 
		{
			setName("Challenger model");
		}
	};

	// Standard demodulation model for FM demodulated files

	class ModelDiscriminator : public Model
	{
		Util::RealPart RP;
		Util::ImaginaryPart IP;

		DSP::Filter FR_a, FR_b;
		std::vector<AIS::Decoder> DEC_a, DEC_b;
		DSP::SamplerParallel S_a, S_b;

		Util::ConvertRAW convert;

	public:
		ModelDiscriminator(Device::DeviceBase* c) : Model(c) {}
		void buildModel(int,bool);
	};
}
