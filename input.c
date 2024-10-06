
#include "input.h"

void init_pad(void)
{
    SIO_CTRL(0) = SIO_CTRL_TX_ENABLE;
    SIO_MODE(0) = SIO_MODE_BAUD_DIV1 | SIO_MODE_DATA_8;
}

uint16_t read_pad(void)
{
    SIO_CTRL(0) = SIO_CTRL_TX_ENABLE | SIO_CTRL_DTR | SIO_CTRL_DSR_IRQ_ENABLE;
    for (int i = 0; i < 300; i++)
        asm volatile("nop");

    const uint8_t msg[5] = { 0x01, 0x42, 0x00, 0x00, 0x00 };
    uint8_t resp[5] = {0};
    for (int i = 0; i < 5; i++) {
        SIO_DATA(0) = msg[i];
        while ((SIO_STAT(0) & SIO_STAT_RX_NOT_EMPTY) == 0);
        resp[i] = SIO_DATA(0);
    }

    SIO_CTRL(0) = SIO_CTRL_TX_ENABLE | SIO_CTRL_ACKNOWLEDGE;
    return ~(*(uint16_t*)&resp[3]);
}
