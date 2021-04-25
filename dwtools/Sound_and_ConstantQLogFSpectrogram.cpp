/* Sound_and_ConstantQLogFSpectrogram.cpp
 * 
 * Copyright (C) 2021 David Weenink
 * 
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this work. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Sound_and_ConstantQLogFSpectrogram.h"
#include "Sound_and_Spectrum.h"

void windowShape_VEC_preallocated (VEC const& target, kSound_windowShape windowShape) {
	const integer n = target.size;
	const double imid = 0.5 * (double) (n + 1);
	switch (windowShape) {
		case kSound_windowShape::RECTANGULAR: {
			target  <<=  1.0; 
		} break; case kSound_windowShape::TRIANGULAR: {  // "Bartlett"
			for (integer i = 1; i <= n; i ++) {
				const double phase = (double) (i - 0.5) / n;
				target [i] = 1.0 - fabs ((2.0 * phase - 1.0));
			} 
		} break; case kSound_windowShape::PARABOLIC: {  // "Welch"
			for (integer i = 1; i <= n; i ++) { 
				const double phase = (double) (i - 0.5) / n;
				target [i] = 1.0 - (2.0 * phase - 1.0) * (2.0 * phase - 1.0);
			}
		} break; case kSound_windowShape::HANNING: {
			for (integer i = 1; i <= n; i ++) {
				const double phase = (double) (i - 0.5) / n;
				target [i] = 0.5 * (1.0 - cos (2.0 * NUMpi * phase));
			}
		} break; case kSound_windowShape::HAMMING: {
			for (integer i = 1; i <= n; i ++) { 
				const double phase = (double) (i - 0.5) / n;
				target [i] = 0.54 - 0.46 * cos (2.0 * NUMpi * phase);
			}
		} break; case kSound_windowShape::GAUSSIAN_1: {
			const double edge = exp (-3.0), onebyedge1 = 1.0 / (1.0 - edge);   // -0.5..+0.5
			for (integer i = 1; i <= n; i ++) {
				const double phase = ((double) i - imid) / n;
				target [i] = (exp (-12.0 * phase * phase) - edge) * onebyedge1;
			}
		} break; case kSound_windowShape::GAUSSIAN_2: {
			const double edge = exp (-12.0), onebyedge1 = 1.0 / (1.0 - edge);
			for (integer i = 1; i <= n; i ++) {
				const double phase = ((double) i - imid) / n;
				target [i] = (exp (-48.0 * phase * phase) - edge) * onebyedge1;
			}
		} break; case kSound_windowShape::GAUSSIAN_3: {
			const double edge = exp (-27.0), onebyedge1 = 1.0 / (1.0 - edge);
			for (integer i = 1; i <= n; i ++) {
				const double phase = ((double) i - imid) / n;
				target [i] = (exp (-108.0 * phase * phase) - edge) * onebyedge1;
			}
		} break; case kSound_windowShape::GAUSSIAN_4: {
			const double edge = exp (-48.0), onebyedge1 = 1.0 / (1.0 - edge);
			for (integer i = 1; i <= n; i ++) { 
				const double phase = ((double) i - imid) / n;
				target [i] = (exp (-192.0 * phase * phase) - edge) * onebyedge1; 
			}
		} break; case kSound_windowShape::GAUSSIAN_5: {
			const double edge = exp (-75.0), onebyedge1 = 1.0 / (1.0 - edge);
			for (integer i = 1; i <= n; i ++) { 
				const double phase = ((double) i - imid) / n;
				target [i] = (exp (-300.0 * phase * phase) - edge) * onebyedge1;
			}
		} break; case kSound_windowShape::KAISER_1: {
			const double factor = 1.0 / NUMbessel_i0_f (2 * NUMpi);
			for (integer i = 1; i <= n; i ++) { 
				const double phase = 2.0 * ((double) i - imid) / n;   // -1..+1
				const double root = 1.0 - phase * phase;
				target [i] = root <= 0.0 ? 0.0 : factor * NUMbessel_i0_f (2.0 * NUMpi * sqrt (root));
			}
		} break; case kSound_windowShape::KAISER_2: {
			const double factor = 1.0 / NUMbessel_i0_f (2 * NUMpi * NUMpi + 0.5);
			for (integer i = 1; i <= n; i ++) { 
				const double phase = 2.0 * ((double) i - imid) / n;   // -1..+1
				const double root = 1.0 - phase * phase;
				target [i] = root <= 0.0 ? 0.0 : factor * NUMbessel_i0_f ((2.0 * NUMpi * NUMpi + 0.5) * sqrt (root)); 
			}
		} break; default: {
			target  <<=  1.0;
		}
	}
}

autoVEC windowShape_VEC (integer n, kSound_windowShape windowShape) {
	autoVEC result = raw_VEC (n);
	windowShape_VEC_preallocated (result.get(), windowShape);
	return result;
}


static void Spectrum_reuse (Spectrum me, double fmax, double nx) {
	my xmin = 0.0;
	my xmax = fmax;
	my nx = nx;
	my dx = fmax / (nx - 1);
}

autoConstantQLogFSpectrogram Sound_to_ConstantQLogFSpectrogram (Sound me, double lowestFrequency, double fmax, double qualityFactor, integer numberOfStepsPerOctave, double timeOversamplingFactor) {
	try {
		double nyquistFrequency = 0.5 / my dx;
		if (fmax <= 0.0)
			fmax = nyquistFrequency;
		Melder_require (fmax  <= nyquistFrequency,
			U"The maximum frequency should not exceed the nyquist frequency (", nyquistFrequency, U" Hz).");
		autoSpectrum spectrum;
		if (fmax < nyquistFrequency) {
			autoSound resampled = Sound_resample (me, 2.0 * fmax, 50);
			spectrum = Sound_to_Spectrum (resampled.get(), true);
			nyquistFrequency = fmax;
		} else {
			spectrum = Sound_to_Spectrum (me, true);
		}
		Melder_require (lowestFrequency < nyquistFrequency,
			U"The lowest frequency should be smaller than the maximum frequency.");
		autoConstantQLogFSpectrogram thee = ConstantQLogFSpectrogram_create (lowestFrequency, nyquistFrequency, numberOfStepsPerOctave, qualityFactor);
		if (timeOversamplingFactor < 1.0)
			timeOversamplingFactor = 4.0; // default oversampling
		autoSpectrum filter = Spectrum_create (nyquistFrequency, spectrum -> nx / 2);	
		const integer maximumNumberOfFilterSamples = filter -> nx;
		autoVEC window = raw_VEC (maximumNumberOfFilterSamples);
		for (integer ifreq = 1; ifreq <= thy nx; ifreq ++) {
			const double frequency =  thy v_myFrequencyToHertz (Sampled_indexToX (thee.get(), ifreq));
			const double bandwidth = frequency / qualityFactor;
			double flow = frequency - bandwidth, fhigh = frequency + bandwidth;
			Melder_clipLeft (0.0, & flow);
			Melder_clipRight (& fhigh, nyquistFrequency);
			integer iflow, ifhigh;
			const integer numberOfFilterSamples = Sampled_getWindowSamples (spectrum.get(), flow, fhigh, & iflow, & ifhigh);
			Melder_require (numberOfFilterSamples > 1,
				U"The number of spectral filter values should be larger than 1.");
			Spectrum_reuse (filter.get(), fhigh - flow, numberOfFilterSamples);
			filter -> z.part (1, 2, 1, numberOfFilterSamples)  <<=  spectrum -> z.part (1, 2, iflow, ifhigh);
			window.resize (numberOfFilterSamples);
			windowShape_VEC_preallocated (window.get(), kSound_windowShape :: HANNING);
			filter -> z.part (1, 2, 1, numberOfFilterSamples)  *=  window.get();
			autoSound filtered = Spectrum_to_Sound (filter.get());
			autoFrequencyBin frequencyBin = FrequencyBin_create (my xmin, my xmax, filtered -> nx, filtered -> dx, filtered -> x1);
			frequencyBin -> z.row (1)  <<=  filtered -> z.row (1);
			thy frequencyBins.addItem_move (frequencyBin.move());
		}
		Melder_assert (thy frequencyBins.size == thy nx); // maintain invariant
		return thee;
	} catch (MelderError) {
		Melder_throw (me, U": cannot create ConstantQLogFSpectrogram.");
	}
}

autoSound ConstantQLogFSpectrogram_to_Sound (ConstantQLogFSpectrogram me) {
	try {
		FrequencyBin frequencyBin = my frequencyBins.at [1];
		const double duration = frequencyBin -> xmax - frequencyBin -> xmin;
		const double nyquistFrequency = my v_myFrequencyToHertz (my xmax);
		const double samplingFrequency = 2.0 * nyquistFrequency;
		const integer numberOfSamples = duration * samplingFrequency;
		integer numberOfFFTSamples = 2;
		while (numberOfFFTSamples < numberOfSamples)
			numberOfFFTSamples *= 2;
		autoSpectrum spectrum = Spectrum_create (nyquistFrequency, numberOfFFTSamples / 2 + 1);
		autoSound reusable = Sound_createSimple (1, duration, samplingFrequency);
		for (integer ifreq = 1; ifreq <= my nx; ifreq ++) {
			frequencyBin = my frequencyBins.at [ifreq];
			reusable -> z.row (1).part (1, frequencyBin -> nx)  <<=  frequencyBin -> z.row (1);
			reusable -> dx = frequencyBin -> dx;
			reusable -> x1 = frequencyBin -> x1;
			reusable -> nx = frequencyBin -> nx;
			autoSpectrum filter = Sound_to_Spectrum (reusable.get(), false);
			const double frequency = exp2 (Sampled_indexToX (me, ifreq)), bandwidth = frequency / my qualityFactor;
			double flow = frequency - bandwidth;
			double fhigh = frequency + bandwidth;
			Melder_clipLeft (0.0, & flow);
			Melder_clipRight (& fhigh, nyquistFrequency);
			integer iflow, ifhigh;
			Sampled_getWindowSamples (spectrum.get(), flow, fhigh, & iflow, & ifhigh);
			Melder_clipLeft (1_integer, & iflow);
			Melder_clipRight (& ifhigh, spectrum -> nx);
			spectrum -> z.part (1, 2, iflow, ifhigh)  +=  filter -> z.part (1, 2, 1, ifhigh - iflow + 1);
		}
		autoSound thee = Spectrum_to_Sound (spectrum.get());
		const double synthesizedDuration = thy xmax - thy xmin;
		if (synthesizedDuration > duration) {
			/*
				The number of samples in the original was not a power of 2 and to apply an FFT the number of samples had to be extended.
			*/
			autoSound part = Sound_extractPart (thee.get(), 0.0, duration, kSound_windowShape::RECTANGULAR, 1.0, false);
			part -> xmin = frequencyBin -> xmin;
			part -> xmax = frequencyBin -> xmax;
			thee = part.move();
		} else if (synthesizedDuration == duration) {
			thy xmin = frequencyBin -> xmin;
			thy xmax = frequencyBin -> xmax;
		} else {
			Melder_throw (U"The synthesized number of samples is too low!");
		}
		return thee;
	} catch (MelderError) {
		Melder_throw (me, U": could not create Sound.");
	}
}

static autoConstantQLogFSpectrogram Sound_to_ConstantQLogFSpectrogram_old (Sound me, double lowestFrequency, double q, integer numberOfStepsPerOctave, integer numberOfSteps, double timeOversamplingFactor) {
	try {
		
		autoConstantQLogFSpectrogram thee = ConstantQLogFSpectrogram_create (lowestFrequency, numberOfStepsPerOctave, numberOfSteps, q);
		const double samplingFrequency = 1.0 / my dx, nyquistFrequency = 0.5 * samplingFrequency;
		Melder_require (thy v_myFrequencyToHertz (thy xmax) <= nyquistFrequency,
			U"The number of steps you want result in a maximum frequency which is above the Nyquist frequency of the sound. "
			"The maximum number of steps should not exceed ", Melder_iroundDown (numberOfStepsPerOctave * log2 (nyquistFrequency / lowestFrequency)), U".");
		if (timeOversamplingFactor < 1.0)
			timeOversamplingFactor = 4.0; // default oversampling
		const integer maximumNumberOfAnalysisSamples = samplingFrequency * q / lowestFrequency + 1;
		autoVEC window = raw_VEC (maximumNumberOfAnalysisSamples);
		autoMAT windowedExp = raw_MAT (2, maximumNumberOfAnalysisSamples);
		VEC windowedCos = windowedExp.row(1), windowedSin = windowedExp.row(2);
		MelderInfo_open ();
		for (integer ifreq = 1; ifreq <= numberOfSteps; ifreq ++) {
			const double frequency =  exp2 (Sampled_indexToX (thee.get(), ifreq));
			const integer halfNumberOfWindowSamples = Melder_iroundDown (0.5 * samplingFrequency * q / frequency);
			const integer numberOfWindowSamples = 2 * halfNumberOfWindowSamples + 1;
			double windowLength = numberOfWindowSamples * my dx;
			Melder_clipRight (& windowLength, my nx * my dx);
			const double dt = windowLength / timeOversamplingFactor;
			integer numberOfFrames;
			double t1;
			Sampled_shortTermAnalysis (me, windowLength, dt, & numberOfFrames, & t1);
			autoFrequencyBin frequencyBin = FrequencyBin_create (my xmin, my xmax, numberOfFrames, dt, t1);
			const integer windowCentreSampleNumber = halfNumberOfWindowSamples + 1;
			for (integer i = 1; i <= numberOfWindowSamples; i ++) {
				const double x = (i - windowCentreSampleNumber) * my dx / (0.5 * windowLength);
				const double root = sqrt (Melder_clippedLeft (0.0, 1.0 - sqr (x)));   // clipping should be rare
				window [i] = NUMbessel_i0_f ((2.0 * NUMpi * NUMpi + 0.5) * root);
			}
			window.resize (numberOfWindowSamples);
			for (integer i = 1; i <= numberOfWindowSamples; i ++) {
				windowedCos [i] =   window [i] * cos (NUM2pi * q * (i - 1) / numberOfWindowSamples);
				windowedSin [i] = - window [i] * sin (NUM2pi * q * (i - 1) / numberOfWindowSamples);
			}
			for (integer iframe = 1; iframe <= numberOfFrames; iframe ++) {
				const double midTime = Sampled_indexToX (frequencyBin.get(), iframe);
				const integer soundCentreSampleNumber = Sampled_xToNearestIndex (me, midTime);   // time accuracy is half a sampling period

				integer leftSample = soundCentreSampleNumber - halfNumberOfWindowSamples;
				integer rightSample = soundCentreSampleNumber + halfNumberOfWindowSamples;
				/*
					Catch some edge cases, which are uncommon because Sampled_shortTermAnalysis() filtered out most problems.
				*/
				Melder_clipLeft (1_integer, & leftSample);
				Melder_clipRight (& rightSample, my nx);
				Melder_require (rightSample >= leftSample,
					U"Unexpected edge case: right sample (", rightSample, U") less than left sample (", leftSample, U").");
				const integer actualNumberOfSamples = rightSample - leftSample + 1;
				const integer windowFromSoundOffset = windowCentreSampleNumber - soundCentreSampleNumber;
				const double windowSq = NUMsum2 (window.part (windowFromSoundOffset + leftSample, windowFromSoundOffset + rightSample));
				const double scaleFactor = 1.0 / (actualNumberOfSamples * sqrt (windowSq)) ;
				VEC amplitudePart = my z.row(1).part (leftSample, rightSample);
				frequencyBin -> z [1] [iframe] = NUMinner (amplitudePart, windowedCos.part (windowFromSoundOffset + leftSample,
					windowFromSoundOffset + rightSample)) * scaleFactor;
				frequencyBin -> z [2] [iframe] = NUMinner (amplitudePart, windowedSin.part (windowFromSoundOffset + leftSample, 
					windowFromSoundOffset + rightSample)) * scaleFactor;
			}
			thy frequencyBins.addItem_move (frequencyBin.move());
		}
		return thee;
	} catch (MelderError) {
		Melder_throw (me, U": cannot create ConstantQLogFSpectrogram.");
	}
}

/* End of file Sound_and_ConstantQLogFSpectrogram.cpp */
