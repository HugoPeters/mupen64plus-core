#ifndef _wk_api_h_
#define _wk_api_h_

#ifdef __cplusplus
extern "C" {
#endif

void wk_sleep(int ms);

void* wk_semaphore_create(int initialCount, int maxCount);
void wk_semaphore_get(void* handle);
void wk_semaphore_put(void* handle);
void wk_semaphore_destroy(void* handle);

#ifdef __cplusplus
}
#endif

#endif // _wk_api_h_
