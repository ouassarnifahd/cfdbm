%% https://www.hark.jp/

close all
clear
clc

pkg load signal control

%% Load MIT HRTF DataBase
i = 0;
% fs = 16000;
fs = 44100;
for theta = [270:5:355 0:5:90]
    i+=1;, DeG = num2str(theta);
    nameL = ['script/elev0-MIT/L0e' repmat('0',1,3-length(DeG)) DeG 'a.wav'];
    nameR = ['script/elev0-MIT/R0e' repmat('0',1,3-length(DeG)) DeG 'a.wav'];
    [LF sampling_rate] = wavread(nameL);
    [RG sampling_rate] = wavread(nameR);
    if fs != sampling_rate
        RG = resample(RG,fs,sampling_rate);
        LF = resample(LF,fs,sampling_rate);
    end
    xL(:,i) = LF;
    xR(:,i) = RG;
end
clear DeG RG LF sampling_rate nameL nameR i

N = 512;
XL = fft(xL,N);
XR = fft(xR,N);
theta = -90:5:90;
F = [linspace(0,0.5,N/2) linspace(0.5,0,N/2)]*fs;

%% Calculate IPD and ILD DataBase
Clr = conj(XL).*XR;
Cll = conj(XL).*XL;

%% old
IPD = angle(Clr);
ILD = 20*log10(abs(Clr./Cll));

IPDnew = angle(XR./XL);
ILDnew = 20*log10(abs(XR./XL));

%% Difference between Maximum and Minimum of
%% IPD and ILD over Direction for Each Freq (step 31,25 Hz)
ILDmaxmin = abs(max(ILD')-min(ILD'))';
IPDmaxmin = abs(max(IPD')-min(IPD'))';

ILDmaxmin = ILDmaxmin(1:N/2);
IPDmaxmin = IPDmaxmin(1:N/2);
F = F(1:N/2)';

IPDmaxmin(IPDmaxmin==0) = 10^(-13);
ILDmaxmin(ILDmaxmin==0) = 10^(-13);

%save('-v7','IPDILD.mat','IPDtarget','ILDtarget','IPDmaxmin','ILDmaxmin','F')

c_header = fopen('inc/ipdild_data.h', 'w');

fprintf(c_header, '#ifndef __IPDILD_DATA__\n');
fprintf(c_header, '#define __IPDILD_DATA__\n\n');

fprintf(c_header, '#define ILDIPD_DEG_MAX %d\n', theta[1]);
fprintf(c_header, '#define ILDIPD_DEG_MIN %d\n', theta[end]);
fprintf(c_header, '#define ILDIPD_DEG_STEP %d\n', theta[2] - theta[1]);
fprintf(c_header, '#define ILDIPD_LEN %d\n\n', N/2);

fprintf(c_header, 'const float ILDtarget[][ILDIPD_LEN] = {');
for theta_=1:180/5+1
    ILDtarget = ILD(1:N/2,find(theta==-90 + 5 * (theta_ - 1)));
    fprintf(c_header, '{ %12.8f,', ILDtarget(1));
    for i=2:N/2-1
        fprintf(c_header, ' %12.8f,', ILDtarget(i));
    end

    if theta_==180/5+1
        fprintf(c_header, ' %12.8f }};\n\n', ILDtarget(N/2));
    else
        fprintf(c_header, ' %12.8f },\n', ILDtarget(N/2));
    end
end

fprintf(c_header, 'const float IPDtarget[][%d] = {', N/2);
for theta_=1:180/5+1
    IPDtarget = IPD(1:N/2,find(theta==-90 + 5 * (theta_ - 1)));
    fprintf(c_header, '{ %12.8f,', IPDtarget(1));
    for i=2:N/2-1
        fprintf(c_header, ' %12.8f,', IPDtarget(i));
    end

    if theta_==180/5+1
        fprintf(c_header, ' %12.8f }};\n\n', IPDtarget(N/2));
    else
        fprintf(c_header, ' %12.8f },\n', IPDtarget(N/2));
    end
end

fprintf(c_header, 'const float IPDmaxmin[] = {');
for i=1:N/2-1
    fprintf(c_header, ' %12.8f,', IPDmaxmin(i));
end
fprintf(c_header, ' %12.8f };\n\n', IPDmaxmin(N/2));

fprintf(c_header, 'const float ILDmaxmin[] = {');
for i=1:N/2-1
    fprintf(c_header, ' %12.8f,', ILDmaxmin(i));
end
fprintf(c_header, ' %12.8f };\n\n', ILDmaxmin(N/2));

fprintf(c_header, '#endif\n');

fclose(c_header);
