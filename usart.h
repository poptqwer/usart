#pragma once
#include <vector>
#include <deque>
#include <string>
#include <cstdint>
#include <windows.h>
#include <cstdio>
#include<thread>
#include<atomic>
#include <mutex> 
#include<array>


union drawData
{
    uint8_t buffer[27];

    class channel_format
    {
        uint8_t hb;
        uint8_t lb;
    public:
        void set_value(uint16_t value)
        {
            hb = (value & 0xfc0) >> 6; // Get the high byte
            lb = value & 0x3f;        // Get the low byte
        }
    };

    struct
    {
        uint8_t head;
        channel_format red;
        channel_format green;
        uint8_t spo2;
        uint8_t hear_rate;
        channel_format blue;
        channel_format black; //11
        channel_format reserved1;//13
        channel_format reserved2;//15
        channel_format reserved3;//17
        channel_format reserved4;//19
        channel_format pink;
        channel_format orange;
        channel_format earthy;
        channel_format royalBlue;
    };

    void reset()
    {
        for (auto& d : buffer)
            d = 0;
    }

    drawData()
    {
        head = 0xFE;
    }
};

#define BUFFER_SIZE 256
class USART
{
    //std::deque<uint16_t> received_data; // Buffer to store received data

	std::array<uint16_t, BUFFER_SIZE> received_data; // Buffer to store received data
    size_t buffer_write_index = 0; // Index for writing to the buffer
    size_t buffer_read_index = 0; // Index for reading from the buffer
    std::atomic<size_t> data_count = 0; // Count of valid data in the buffer
	uint16_t remain_data_unit = 0; // Temporary variable to hold combined bytes from previous read
	bool has_remain_data = false; // Flag to indicate if there's a remaining byte from the previous read

    HANDLE hSerial_TX = INVALID_HANDLE_VALUE;
    HANDLE hSerial_RX = INVALID_HANDLE_VALUE;





    //std::mutex data_mutex; // 保護資料的鎖


    std::atomic<bool> running{ false };
public:

    USART(const std::string tx_port_name,
          const std::string rx_port_name);
    ~USART();
    void close();

    void receive_event();
     
    size_t get_received_data(uint16_t& data);

    void send(void* data, DWORD data_size);
    void draw(drawData& data);
};
