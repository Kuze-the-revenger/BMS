#include "Com_Debug.h"
int fputc(int ch, FILE *File)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 1000);
    return ch;
}


