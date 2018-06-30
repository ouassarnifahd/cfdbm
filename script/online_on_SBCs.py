from scipy.io import loadmat
import numpy.matlib as nplib
import alsaaudio as audio
import scipy as scp
import numpy as np
import time as tm

# How long do you want to run the code?
minutes = 120

# Input & Output Settings
with_FDBM = 1
audioformat = audio.PCM_FORMAT_S16_LE
channels_inp = 2
channels_out = 2
framerate = 16000
periodsize = 256
print('BufferSize = %i samples and FrameLength = %.3f ms'% (periodsize,2*periodsize*1000/framerate))

# Input Device, ADC-USB
inp = audio.PCM(audio.PCM_CAPTURE,audio.PCM_NORMAL,device='plughw:2,0')
inp.setchannels(channels_inp)
inp.setrate(framerate)
inp.setformat(audioformat)
inp.setperiodsize(periodsize)


# Output Device, SBC or Laptop
out = audio.PCM(audio.PCM_PLAYBACK,audio.PCM_NORMAL,device='plughw:0,0')
out.setchannels(channels_out)
out.setrate(framerate)
out.setformat(audioformat)
out.setperiodsize(periodsize)

# Load ILD-and-IPD DataBase
for k,v in loadmat('IPDILD.mat').items(): exec(k+'=v[:,0]') if k[0]!='_' else None

# Frequency Domain Binaural Model (FDBM)
nfft = 512;
WinLen = nfft
Win = nplib.repmat(np.sin(np.pi*(np.arange(WinLen))/WinLen),2,1).T
FrmShf = np.int(WinLen/2)

count = 0; end_signal = np.int(np.ceil(60.0*minutes*framerate/periodsize))
x = np.zeros((WinLen,2))
xbuf = np.zeros((WinLen,2))

while True:
    l,data = inp.read()

    if count == end_signal:
        break
    if len(data) != WinLen*2:
        print("skip this data %i samples" % (len(data)))
        continue

    if with_FDBM == 1:
        xi = np.fromstring(data,dtype=np.int16)
        xi = np.reshape(np.float32(xi),(np.int16(xi.shape[0]/2),2))
        # xi = xi[:,[1,0]]
        x[FrmShf::,:] = xi

        Xbuf = scp.fft((x*Win).T,nfft)

        XIS = Xbuf[1,0:np.int(nfft/2)]/Xbuf[0,0:np.int(nfft/2)]

        IPDtest = np.angle(XIS[1:Fcut/fs*nfft])
        ILDtest = 20*np.log10(np.abs(XIS[Fcut/fs*nfft:end]))

        muIPD = np.abs(IPDtest-IPDtarget(theta))/IPDmaxmin
        muILD = np.abs(ILDtest-ILDtarget(theta))/ILDmaxmin

        # PowerL = 20*np.log10(np.abs(Xbuf[0,:]))
        # PowerR = 20*np.log10(np.abs(Xbuf[1,:]))
        # attenuationL = np.abs(PowerL.max()-PowerL.min())
        # attenuationR = np.abs(PowerR.max()-PowerR.min())
        # gamma = min([attenuationL,attenuationR])

        mu = np.concatenate((muIPD,muILD))
        mu[mu>1]=1;

        G = (1-mu)**16
        # G = 10**(-gamma*mu/20)
        G = np.concatenate((G,G[::-1]))
        # repmat jutsu
        sig_fdbm = np.real(scp.ifft(Xbuf*nplib.repmat(G,2,1))).T * Win

        xbuf[0:FrmShf,:] = xbuf[FrmShf::,:] + sig_fdbm[0:FrmShf,:]
        xbuf[FrmShf::,:] = sig_fdbm[FrmShf::,:]

        data = bytes(np.int16(xbuf[0:FrmShf,:]))
        out.write(data)

        x[0:FrmShf,:] = xi
    else:
        out.write(data)
    count += 1
