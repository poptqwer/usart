#include "usart.h"
#include <iostream>
#include <windows.h>


USART::USART(const std::string tx_port_name, const std::string rx_port_name)
{
    close();
    // 1. 開啟 COM4
    // 注意：\\\\.\\COM4 是 Windows 的特殊路徑格式
    std::wstring tx_path = L"\\\\.\\" + std::wstring(tx_port_name.begin(), tx_port_name.end());
    std::wstring rx_path = L"\\\\.\\" + std::wstring(rx_port_name.begin(), rx_port_name.end());

    hSerial_TX = CreateFile(tx_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hSerial_TX == INVALID_HANDLE_VALUE) {
        std::cout << "Error: Could not open TX COM port!" << std::endl;
        std::cout << "Windows Error Code: " << GetLastError() << std::endl;
        return;
    }

    hSerial_RX = CreateFile(rx_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hSerial_RX == INVALID_HANDLE_VALUE) {
        std::cout << "Error: Could not open RX COM port!" << std::endl;
        std::cout << "Windows Error Code: " << GetLastError() << std::endl;
        return;
    }


    // 2. 設定通訊參數 (DCB 結構體)
    DCB dcbTxSerialParams = { 0 };
    dcbTxSerialParams.DCBlength = sizeof(dcbTxSerialParams);

    if (!GetCommState(hSerial_TX, &dcbTxSerialParams)) {
        std::cout << " " << std::endl;//錯誤：無法取得當前狀態！
    }
    dcbTxSerialParams.BaudRate = CBR_115200;   // 波特率 115200
    dcbTxSerialParams.ByteSize = 8;          // 資料位元 8
    dcbTxSerialParams.StopBits = ONESTOPBIT; // 停止位 1
    dcbTxSerialParams.Parity = NOPARITY;   // 無校驗
    SetCommState(hSerial_TX, &dcbTxSerialParams);
 


    DCB dcbRxSerialParams = { 0 };
    dcbRxSerialParams.DCBlength = sizeof(dcbRxSerialParams);

    if (!GetCommState(hSerial_RX, &dcbRxSerialParams)) {
        std::cout << " " << std::endl;//錯誤：無法取得當前狀態！
    }

    dcbRxSerialParams.BaudRate = CBR_115200;   // 波特率 115200
    dcbRxSerialParams.ByteSize = 8;          // 資料位元 8
    dcbRxSerialParams.StopBits = ONESTOPBIT; // 停止位 1
    dcbRxSerialParams.Parity = NOPARITY;   // 無校驗

    COMMTIMEOUTS timeouts = { 0 };
    // MAXDWORD 配合 0 和 0，代表 ReadFile 如果緩衝區沒資料，會立刻回傳 0，不會等待
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    SetCommTimeouts(hSerial_RX, &timeouts);

	running = true;


}

USART::~USART()
{
    close();
}

void USART::close()
{
    running = false;

    //// 強制解除 WaitCommEvent 的阻塞狀態以便退出
    //SetCommMask(hSerial_RX, 0);

    //if (event_thread.joinable()) {
    //    event_thread.join();
    //}


    if (hSerial_TX != INVALID_HANDLE_VALUE) CloseHandle(hSerial_TX);
    if (hSerial_RX != INVALID_HANDLE_VALUE) CloseHandle(hSerial_RX);
}

size_t USART::get_received_data(uint16_t& data)
{
    //std::lock_guard<std::mutex> lock(data_mutex); // 1. 上鎖，禁止背景執行緒動 buffer
    // 
// 1. 先獲取當前數量
    size_t current = data_count.load(std::memory_order_acquire);
    if (current == 0) return 0;

	data = received_data[buffer_read_index]; // 2. 從 buffer 讀取資料到參數

	buffer_read_index = (buffer_read_index + 1) % BUFFER_SIZE; // 3. 移動讀取索引，環繞回起點

    // 3. 原子減少並回傳「舊的值」
    // 這樣可以保證回傳的一定是你進入函數時看到的那個數量，不受背景執行緒干擾
    return data_count.fetch_sub(1, std::memory_order_acq_rel);
}

void USART::send(void* data, DWORD data_size)
{
    // 3. 傳送資料

    DWORD bytesWritten;

    if (WriteFile(hSerial_TX, data, data_size, &bytesWritten, NULL)) {
        //std::cout << " " << bytesWritten << " " << std::endl;//  std::cout << "成功傳送 " << bytesWritten << " 字元！" << std::endl;
    }
}


void USART::draw(drawData& data)
{
    send(data.buffer, 27);
}

void USART::receive_event()
{
    DWORD bytesRead;

    uint8_t temp_buffer[256]; // Temporary buffer to read data into

        if (ReadFile(hSerial_RX, temp_buffer, sizeof(temp_buffer), &bytesRead, NULL))
        {
            size_t i = 0;
			size_t conbine_count = 0;
			uint16_t data_unit = 0; // Temporary variable to hold combined bytes
            if (has_remain_data)
            {
				data_unit = remain_data_unit; // Start with the remaining byte from the previous read
                conbine_count++;
            }

            while (i < bytesRead)
            {
                data_unit = (data_unit << 8) + temp_buffer[i]; // Shift existing data and add new byte.
				conbine_count++;
				if (conbine_count % 2 == 0) // Every two bytes, we have a complete uint16_t
                {
                    received_data[buffer_write_index] = data_unit; // Store the complete uint16_t in the buffer

                    buffer_write_index = (buffer_write_index + 1) % BUFFER_SIZE; // Move to the next index, wrap around if necessary

                    data_count.fetch_add(1, std::memory_order_release);

                    data_unit = 0; // Reset for the next data unit

                    conbine_count = 0;
                }

                i++;
            }

            has_remain_data = (conbine_count % 2 != 0); // If there's an odd byte, we have remaining data
			remain_data_unit = has_remain_data ? data_unit : 0; // Store the remaining byte if it exists    
        }
}
