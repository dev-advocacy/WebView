// WebView2Logger.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

int main()
{
    try
    {
        typedef logging::ipc::reliable_message_queue queue_t;

        // Create a message_queue_type object that is associated with the interprocess
        // message queue named "ipc_message_queue".
        queue_t queue
        (
            keywords::name = logging::ipc::object_name(logging::ipc::object_name::user, "WebView_ipc_message_queue"),
            keywords::open_mode = logging::open_mode::open_or_create,
            keywords::capacity = 256,
            keywords::block_size = 1024,
            keywords::overflow_policy = queue_t::block_on_overflow
        );

        std::cout << "Viewer process running..." << std::endl;

        // Keep reading log messages from the associated message queue and print them on the console.
        // queue.receive() will block if the queue is empty.
        std::string message;
        while (queue.receive(message) == queue_t::succeeded)
        {
            std::cout << message << std::endl;

            // Clear the buffer for the next message
            message.clear();
        }
    }
    catch (std::exception& e)
    {
        std::cout << "Failure: " << e.what() << std::endl;
    }

    return 0;
}
