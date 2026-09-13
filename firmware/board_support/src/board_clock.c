/*==================[inclusions]=============================================*/
#include "board_clock.h"
#include "gptimer_hal.h"

/*==================[external functions definition]=============================*/
bool BoardClockInit(void)
{
    return (GpTimerInit() == HAL_GPTIMER_OK);
}

uint32_t BoardClockGetMs(void)
{
    return GpTimerGetMs();
}

/*==================[end of file]============================================*/