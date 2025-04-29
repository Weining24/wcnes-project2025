%%% Script to compute optimal phasing for beamforming

load quad_array.mat
f = 2.45e9;

angle = 25; % steering direction

phases = phaseShift(quadArray, f, [angle 0])
quadArray.PhaseShift = phases;
patternAzimuth(quadArray, f)