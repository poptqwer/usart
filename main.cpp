#include "usart.h"

drawData data;

USART rs232("COM4", "COM5");

uint16_t received_data; // Buffer to store received data
std::thread event_thread;   

uint16_t red_value = 0;
uint16_t blue_value = 0;
uint16_t green_value = 0;
uint16_t black_value = 0;




void thread_function()
{
    while (true)
    {

		rs232.receive_event();  

        if (rs232.get_received_data(received_data))
        {
            data.red.set_value(received_data);

 
            rs232.draw(data);
        }
    }

}

int main()
{
    event_thread = std::thread(thread_function);

	event_thread.join();        


    return 0;
}


//data.red.set_value(red_value);
 //data.blue.set_value(blue_value);
 //data.green.set_value(green_value);
 //data.black.set_value(black_value);

 //red_value = (red_value + 1) % 4096; // Increment test_val and wrap around at 1024 
 //blue_value = (red_value + 50) % 4096; // Increment test_val and wrap around at 1024
 //green_value = (blue_value + 50) % 4096; // Increment test_val and wrap around at 1024
 //black_value = (green_value + 50) % 4096; // Increment test_val and wrap around at 1024


 //received_buffer.pop_front();