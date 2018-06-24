#ifndef __HEADER_THREADS__
#define __HEADER_THREADS__

void threads_init(void);

void* thread_audio_capture(void* parameters);

void* thread_audio_playback(void* parameters);

void* thread_video_capture(void* parameters);

void* thread_AVstream(void* parameters);

void* thread_fdbm(void* parameters);

#endif
