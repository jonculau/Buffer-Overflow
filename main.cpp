#include <iostream>
#include <list>
#include <algorithm>
#include <chrono>
#include <Windows.h>
#include <stdint.h>
#include <string>
#include <ctime>
#include <fstream>
#include <iterator>

bool cmdOptionExists(char **begin, char **end, const std::string &option);

char *getCmdOption(char **begin, char **end, const std::string &option);

void help();

char *RandomData(int size);

int main(int argc, char *argv[])
{

    long long TestTime;

    if (argc < 2)
    {
        std::cout << "Too few arguments" << std::endl;
        help();
        return (int)std::errc::invalid_argument;
    }
    // OPEN PORT
    std::string portName = "\\\\.\\COM" + std::to_string(std::stoi(argv[1]));
    HANDLE hcomm = CreateFile(portName.c_str(),
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              0,
                              OPEN_EXISTING,
                              0,
                              0);

    if (hcomm == INVALID_HANDLE_VALUE)
    {
        //  Handle the error.
        std::cout << "CreateFile failed with error " << GetLastError() << std::endl;
        return 1;
    }

    DCB dcb = {0};
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = 1;
    timeouts.ReadTotalTimeoutConstant = 1;
    timeouts.ReadTotalTimeoutMultiplier = 1;
    timeouts.WriteTotalTimeoutConstant = MAXWORD;
    timeouts.WriteTotalTimeoutMultiplier = MAXWORD;

    /* Configuration */
    if (cmdOptionExists(argv, argv + argc, "-BaudRate"))
    {
        dcb.BaudRate = std::stoul(getCmdOption(argv, argv + argc, "-BaudRate"));
    }
    else
    {
        dcb.BaudRate = CBR_9600;
    }

    if (cmdOptionExists(argv, argv + argc, "-ByteSize"))
    {
        dcb.ByteSize = (BYTE)getCmdOption(argv, argv + argc, "-ByteSize")[0] - 48;
    }
    else
    {
        dcb.ByteSize = (BYTE)8;
    }

    if (cmdOptionExists(argv, argv + argc, "-Parity"))
    {
        dcb.Parity = (BYTE)getCmdOption(argv, argv + argc, "-Parity")[0] - 48;
    }
    else
    {
        dcb.Parity = NOPARITY;
    }

    if (cmdOptionExists(argv, argv + argc, "-StopBits"))
    {
        dcb.StopBits = (BYTE)getCmdOption(argv, argv + argc, "-StopBits")[0] - 48;
    }
    else
    {
        dcb.StopBits = ONESTOPBIT;
    }

    if (cmdOptionExists(argv, argv + argc, "-fRtsControl"))
    {
        dcb.fRtsControl = std::stoul(getCmdOption(argv, argv + argc, "-fRtsControl"));
    }
    else
    {
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
    }

    if (cmdOptionExists(argv, argv + argc, "-fDtrControl"))
    {
        dcb.fDtrControl = std::stoul(getCmdOption(argv, argv + argc, "-fDtrControl"));
    }
    else
    {
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
    }

    std::cout << "Port Settings:" << std::endl;
    std::cout << portName << " - ";
    std::cout << "BaudRate: " << dcb.BaudRate << " - ";
    std::cout << "ByteSize: " << (int)dcb.ByteSize << " - ";
    std::cout << "Parity: " << (int)dcb.Parity << " - ";
    std::cout << "StopBits: " << (int)dcb.StopBits << " - ";
    std::cout << "ByteSize: " << (int)dcb.ByteSize << " - ";
    std::cout << "fRtsControl: " << dcb.fRtsControl << " - ";
    std::cout << "fDtrControl: " << dcb.fDtrControl << std::endl;

    std::cout << "Test Time[m]: ";

    std::cin >> TestTime;

    /* Test */

    if (!::SetCommState(hcomm, &dcb))
    {
        std::cout << "SetCommState failed with error " << GetLastError() << std::endl;
        return 2;
    }
    if (!::SetCommTimeouts(hcomm, &timeouts))
    {
        std::cout << "SetCommTimeouts failed with error " << GetLastError() << std::endl;
        return 2;
    }

    TestTime = TestTime;

    DWORD dwBytesWritten;

    std::list<std::string> frames;

    bool written = false;
    bool isTestRunning = true;

    const auto start = std::chrono::system_clock::now();
    const auto dtn = std::chrono::minutes(TestTime);
    const auto end = std::chrono::system_clock::to_time_t(start + dtn);

    time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::cout << std::endl << "Starting BufferOverflow Atack at " << ctime(&ts) << std::endl;

    while (isTestRunning)
    {
        char *data = RandomData(dcb.BaudRate / 8);
        bool written = WriteFile(hcomm,
                                 data,
                                 sizeof(data),
                                 &dwBytesWritten,
                                 NULL);
        auto now = std::chrono::system_clock::now();
        ts = std::chrono::system_clock::to_time_t(now);
        frames.push_back(data);
        isTestRunning = (end > ts);
    }

    std::cout << "Finish at " << ctime(&ts) << std::endl;

    CloseHandle(hcomm);

    std::string fileName = ctime(&ts);

    fileName += ".bin";
    
    
    for (int i = 0; i < fileName.size(); i++)
        if (fileName[i] == ':' || fileName[i] == '\n') fileName.replace(i,1,"-");
    
    std::cout << "Generating Log: " <<  fileName;

    std::ofstream out;

    out.open(fileName);

    for (auto it = frames.begin(); it != frames.end(); ++it)
    {
        out.write((*it).c_str(), sizeof(*it));
    }

    out.close();

    return 0;
}

char *RandomData(int size)
{
    char *data = new char[size];
    for (int i = 0; i < size; i++)
    {
        data[i] = (char)(rand() % 128);
    }
    return data;
}
void help()
{
    std::cout << "[Port Number]" << std::endl;
    std::cout << "-BaudRate -- 110 >> 256000" << std::endl;
    std::cout << "-Parity [0- NOPARITY] [1- ODDPARITY] [2- EVENPARITY] [3- MARKPARITY] [4- SPACEPARITY]" << std::endl;
    std::cout << "-StopBits [0- ONESTOPBIT] [1- ONE5STOPBITS] [2- TWOSTOPBITS]" << std::endl;
    std::cout << "-ByteSize 4-8" << std::endl;
    std::cout << "-fRtsControl [0- RTS_CONTROL_DISABLE] [1-RTS_CONTROL_ENABLE] [2- RTS_CONTROL_HANDSHAKE] [3- RTS_CONTROL_TOGGLE]" << std::endl;
    std::cout << "-fDtrControl [0- DTR_CONTROL_DISABLE] [1- DTR_CONTROL_ENABLE] [2- DTR_CONTROL_HANDSHAKE]" << std::endl;
}

bool cmdOptionExists(char **begin, char **end, const std::string &option)
{
    return std::find(begin, end, option) != end;
}

char *getCmdOption(char **begin, char **end, const std::string &option)
{
    char **itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}