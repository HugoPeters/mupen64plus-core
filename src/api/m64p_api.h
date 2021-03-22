#ifndef _m64p_internal_api_h_
#define _m64p_internal_api_h_

#include "m64p_config.h"
#include "m64p_frontend.h"
#include "m64p_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _m64p_api
{
    ptr_ConfigOpenSection       ConfigOpenSection;
    ptr_ConfigDeleteSection     ConfigDeleteSection;
    ptr_ConfigSetParameter      ConfigSetParameter;
    ptr_ConfigGetParameter      ConfigGetParameter;
    ptr_ConfigSetDefaultInt     ConfigSetDefaultInt;
    ptr_ConfigSetDefaultFloat   ConfigSetDefaultFloat;
    ptr_ConfigSetDefaultBool    ConfigSetDefaultBool;
    ptr_ConfigSetDefaultString  ConfigSetDefaultString;
    ptr_ConfigGetParamInt       ConfigGetParamInt;
    ptr_ConfigGetParamFloat     ConfigGetParamFloat;
    ptr_ConfigGetParamBool      ConfigGetParamBool;
    ptr_ConfigGetParamString    ConfigGetParamString;
    ptr_CoreDoCommand           CoreDoCommand;
    ptr_CoreGetAPIVersions      CoreGetAPIVersions;
} m64p_api;

void m64p_get_api(m64p_api* aOutApi);

//typedef struct _m64p_plugin_api
//{
//
//} m64p_plugin_api;
//
//typedef struct _m64p_rsp_plugin_api
//{
//
//};

#ifdef __cplusplus
}
#endif

#endif // _m64p_internal_api_h_
