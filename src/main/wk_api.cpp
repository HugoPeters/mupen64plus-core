#include "wk_api.h"
#include "C_ThreadingTools.h"
#include "C_Semaphore.h"

extern "C"
{
    void wk_sleep(int ms)
    {
        C_ThreadingTools::Sleep(ms);
    }

	void* wk_semaphore_create(int initialCount, int maxCount)
	{
		return new C_Semaphore(initialCount, maxCount);
	}

	void wk_semaphore_get(void* handle)
	{
		static_cast<C_Semaphore*>(handle)->Get();
	}

	void wk_semaphore_put(void* handle)
	{
		static_cast<C_Semaphore*>(handle)->Put();
	}

	void wk_semaphore_destroy(void* handle)
	{
		delete static_cast<C_Semaphore*>(handle);
	}
}

