#define M64P_CORE_PROTOTYPES
#include "m64p_api.h"

void m64p_get_api(m64p_api* aOutApi)
{
    aOutApi->ConfigOpenSection = ConfigOpenSection;
    aOutApi->ConfigDeleteSection = ConfigDeleteSection;
    aOutApi->ConfigSetParameter = ConfigSetParameter;
    aOutApi->ConfigGetParameter = ConfigGetParameter;
    aOutApi->ConfigSetDefaultInt = ConfigSetDefaultInt;
    aOutApi->ConfigSetDefaultFloat = ConfigSetDefaultFloat;
    aOutApi->ConfigSetDefaultBool = ConfigSetDefaultBool;
    aOutApi->ConfigSetDefaultString = ConfigSetDefaultString;
    aOutApi->ConfigGetParamInt = ConfigGetParamInt;
    aOutApi->ConfigGetParamFloat = ConfigGetParamFloat;
    aOutApi->ConfigGetParamBool = ConfigGetParamBool;
    aOutApi->ConfigGetParamString = ConfigGetParamString;
    aOutApi->CoreDoCommand = CoreDoCommand;
    aOutApi->CoreGetAPIVersions = CoreGetAPIVersions;
}
