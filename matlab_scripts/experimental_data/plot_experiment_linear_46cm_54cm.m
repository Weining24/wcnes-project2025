%%% script to process and plot data from experiments I made at home

%%% read SmartRF studio logs

load experiments_linear_46cm_54cm.mat

% dual_ref = readtable("new_experiments/first_test_lab_linear_geometry_46cm_54cm/dual_antenna_ref", FileType="delimitedtext", ReadVariableNames=false);
% quad_ref = readtable("new_experiments/first_test_lab_linear_geometry_46cm_54cm/quad_antenna_ref", FileType="delimitedtext", ReadVariableNames=false);
% quad_0_0_0_0 = readtable("new_experiments/first_test_lab_linear_geometry_46cm_54cm/quad_antenna_0_0_0_0", FileType="delimitedtext", ReadVariableNames=false);
% quad_0_0_90_90 = readtable("new_experiments/first_test_lab_linear_geometry_46cm_54cm/quad_antenna_0_0_90_90", FileType="delimitedtext", ReadVariableNames=false);
% quad_0_0_180_180 = readtable("new_experiments/first_test_lab_linear_geometry_46cm_54cm/quad_antenna_0_0_180_180", FileType="delimitedtext", ReadVariableNames=false);
% quad_0_0_270_270 = readtable("new_experiments/first_test_lab_linear_geometry_46cm_54cm/quad_antenna_0_0_270_270", FileType="delimitedtext", ReadVariableNames=false);

%%% Extract RSSI

N = 1200; % number of samples

RSSI = [dual_ref.Var20(1:N), ...
quad_ref.Var20(1:N), ...
quad_0_0_0_0.Var20(1:N), ...
quad_0_0_90_90.Var20(1:N), ...
quad_0_0_180_180.Var20(1:N), ...
quad_0_0_270_270.Var20(1:N)];

rssi_avg = mean(RSSI);

%%% compute time required to receive M packets
M = 1200;

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
title("Measured signal strength (1200 packets)")
subtitle("Carrier-tag distance: 46 cm, tag-receiver distance: 54 cm, linear geometry")

figure
bar(time_to_M)
xticklabels({"two antennae, reference code", "four antennae, reference code", "four antennae, phased (0, 0, 0, 0)", "four antennae, phased (0, 0, 90, 90)", "four antennae, phased (0, 0, 180, 180)", "four antennae, phased (0, 0, 270, 270)"})
ylabel("Time [hh:mm:ss.ms]")
title("Measured time to receive 1200 packets")
subtitle("Carrier-tag distance: 46 cm, tag-receiver distance: 54 cm, linear geometry")