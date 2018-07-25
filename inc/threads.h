#ifndef __HEADER_THREADS__
#define __HEADER_THREADS__

void threads_init(void);

void* thread_capture_audio(void* parameters);

void* thread_openCV(void* parameters);

void* thread_fdbm_pool(void* parameters);

void* thread_fdbm_worker(void* parameters);

void* thread_playback_audio(void* parameters);

void* thread_AVstream(void* parameters);

#endif
