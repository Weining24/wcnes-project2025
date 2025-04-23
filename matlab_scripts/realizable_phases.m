%%% Script to compute realizable phase shifts given symbol
%%% frequencies (/"clock dividers")

d0 = 20;
d1 = 18;

d = (d0 + d1)/2;

delay = 0:1:d;
phase = delay./d*360;
bar(delay, phase);
xlabel("delay [cycles]")
ylabel("phase shift [degrees]")