#define NDEBUG

#include "ppmmatrix.h"
#include "console.h"

using isrRegistrar = IsrRegistrar<rcUsart::RxHandler, rcUsart::TxHandler, btUsart::RxHandler, btUsart::TxHandler>;

using terminalDevice = btUsart;
using terminal = std::basic_ostream<terminalDevice>;

int main() {
    btUsart::init<9600>();
    rcUsart::init<115200>();
    {
        Scoped<EnableInterrupt<>> ei;
        uint8_t counter = 0;
        while(true) {
            Util::delay(1000_ms);
            std::outl<terminal>("Test01: "_pgm, ++counter);
        }
    }
}

// BT
ISR(USART0_RX_vect) {
    isrRegistrar::isr<AVR::ISR::Usart<0>::RX>();
}
ISR(USART0_UDRE_vect){
    isrRegistrar::isr<AVR::ISR::Usart<0>::UDREmpty>();
}

// SumD
ISR(USART1_RX_vect) {
    isrRegistrar::isr<AVR::ISR::Usart<1>::RX>();
}
ISR(USART1_UDRE_vect){
    isrRegistrar::isr<AVR::ISR::Usart<1>::UDREmpty>();
}

#ifndef NDEBUG
void assertFunction(const PgmStringView& expr, const PgmStringView& file, unsigned int line) noexcept {
    std::outl<terminal>("Assertion failed: "_pgm, expr, Char{','}, file, Char{','}, line);
    while(true) {}
}
#endif
