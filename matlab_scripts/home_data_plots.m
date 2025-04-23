%%% script to process and plot data from experiments I made at home

%%% read SmartRF studio logs

load experiments_home_data.mat

% dual_ref = readtable("experiments_home/linear_geometry_25+3+28_cm_two_antennae_reference", FileType="delimitedtext", ReadVariableNames=false);
% quad_ref = readtable("experiments_home/linear_geometry_25+3+28_cm_four_antennae_reference", FileType="delimitedtext", ReadVariableNames=false);
% quad_0_0_0_0 = readtable("experiments_home/linear_geometry_25+3+28_cm_four_antennae_0_0_0_0", FileType="delimitedtext", ReadVariableNames=false);
% quad_0_0_90_90 = readtable("experiments_home/linear_geometry_25+3+28_cm_four_antennae_0_0_90_90", FileType="delimitedtext", ReadVariableNames=false);
% quad_0_0_180_180 = readtable("experiments_home/linear_geometry_25+3+28_cm_four_antennae_0_0_180_180", FileType="delimitedtext", ReadVariableNames=false);
% quad_0_0_270_270 = readtable("experiments_home/linear_geometry_25+3+28_cm_four_antennae_0_0_270_270", FileType="delimitedtext", ReadVariableNames=false);

%%% Extract RSSI

N = 2500; % number of samples

RSSI = [dual_ref.Var20(1:N), ...
quad_ref.Var20(1:N), ...
quad_0_0_0_0.Var20(1:N), ...
quad_0_0_90_90.Var20(1:N), ...
quad_0_0_180_180.Var20(1:N), ...
quad_0_0_270_270.Var20(1:N)];

rssi_avg = mean(RSSI);

%%% compute time required to receive M packets
M = 2500;

time_to_M = [dual_ref.Var1(M) - dual_ref.Var1(1), ...
quad_ref.Var1(M) - quad_ref.Var1(1), ...
quad_0_0_0_0.Var1(M) - quad_0_0_0_0.Var1(1), ...
quad_0_0_90_90.Var1(M) - quad_0_0_90_90.Var1(1), ...
quad_0_0_180_180.Var1(M) - quad_0_0_180_180.Var1(1), ...
quad_0_0_270_270.Var1(M) - quad_0_0_270_270.Var1(1)];


%%% plot results
figure
boxplot(RSSI)
xticklabels({"two antennae, reference code", "four antennae, reference code", "four antennae, phased (0, 0, 0, 0)", "four antennae, phased (0, 0, 90, 90)", "four antennae, phased (0, 0, 180, 180)", "four antennae, phased (0, 0, 270, 270)"})
ylabel("RSSI [dBm]")
title("Measured signal strength (2500 packets)")
subtitle("Carrier-tag distance: 25 cm, tag-receiver distance: 28 cm, linear geometry")

figure
bar(time_to_M)
xticklabels({"two antennae, reference code", "four antennae, reference code", "four antennae, phased (0, 0, 0, 0)", "four antennae, phased (0, 0, 90, 90)", "four antennae, phased (0, 0, 180, 180)", "four antennae, phased (0, 0, 270, 270)"})
ylabel("Time [hh:mm:ss.ms]")
title("Measured time to receive 2500 packets")
subtitle("Carrier-tag distance: 25 cm, tag-receiver distance: 28 cm, linear geometry")