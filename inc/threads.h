#ifndef __HEADER_THREADS__
#define __HEADER_THREADS__

void threads_init(void);

void* thread_capture_audio(void* parameters);

void* thread_playback_audio(void* parameters);

void* thread_video_capture(void* parameters);

void* thread_AVstream(void* parameters);

void* thread_fdbm(void* parameters);

#endif
