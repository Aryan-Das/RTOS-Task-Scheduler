#include <stdint.h>
#include "uart.hpp"
#include "tcb.hpp"





int main(){
    configure_uart();
    const char* str = "Hello World!\n";
    for(const char* p = str; *p != '\0'; ++p){
        uart_putc(*p);
    }
}


