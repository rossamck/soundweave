#include "PingPongBuffer.h"


PingPongBuffer::PingPongBuffer(int capacity) : capacity_(capacity),
                                               buffer_a_(capacity),
                                               buffer_b_(capacity),
                                               current_buffer_(&buffer_a_),
                                               write_offset_(0),
                                               read_offset_(0),
                                               buffer_a_full_(false),
                                               buffer_b_full_(false),
                                               on_buffer_full_callback_(nullptr)
{
}


void PingPongBuffer::add_data(const std::vector<short> &data)
{
    // std::cout << "Adding data" << std::endl;
    int data_size = static_cast<int>(data.size());
    int remaining_capacity = capacity_ - write_offset_;
    // std::cout << "data size = " << data_size << std::endl;
    // std::cout << "remaining capacity = " << remaining_capacity << std::endl;

    if (current_buffer_ == &buffer_a_)
    {   

        if (data_size <= remaining_capacity)
        {
            
            // The entire data fits in buffer A
            std::copy(data.begin(), data.end(), current_buffer_->begin() + write_offset_);
            write_offset_ += data_size;
            
        }
        else
        {
             
            // Some of the data will spill over to buffer B
            // std::cout << "OVERLAP" << std::endl;
            // std::cout << "remaining capacity = " << remaining_capacity << std::endl;
            std::copy(data.begin(), data.begin() + remaining_capacity, current_buffer_->begin() + write_offset_);
            write_offset_ = capacity_;
            buffer_a_full_ = true;
            // std::cout << "SHOULD CALLBACK BUFFER A data" << std::endl;

            // NEW ADDITION
            trigger_callback();


            switch_buffer();
            add_data(std::vector<short>(data.begin() + remaining_capacity, data.end()));


        }


        if (write_offset_ == capacity_)
        {
            buffer_a_full_ = true;
            // std::cout << "trigger callback a!" << std::endl;
            
            trigger_callback();
            switch_buffer();
        }
    }
    else
    {
        if (data_size <= remaining_capacity)
        {
            // The entire data fits in buffer B
            std::copy(data.begin(), data.end(), current_buffer_->begin() + write_offset_);
            write_offset_ += data_size;
        }
        else
        {
            // Some of the data will spill over to buffer A
            // std::cout << "OVERLAP" << std::endl;
            // std::cout << "remaining capacity = " << remaining_capacity << std::endl;
            std::copy(data.begin(), data.begin() + remaining_capacity, current_buffer_->begin() + write_offset_);
            write_offset_ = capacity_;
            buffer_b_full_ = true;
            // std::cout << "SHOULD CALLBACK BUFFER B data" << std::endl;


            // NEW ADDITION
            trigger_callback();

            switch_buffer();
            add_data(std::vector<short>(data.begin() + remaining_capacity, data.end()));
        }

        if (write_offset_ == capacity_)
        {
            buffer_b_full_ = true;
            // std::cout << "trigger callback b" << std::endl;
            
            trigger_callback();
            switch_buffer();
        }
    }

    if (current_buffer_ == &buffer_a_)
    {
        // std::cout << "Buffer A length = " << write_offset_ << std::endl;
    }
    else
    {
        // std::cout << "Buffer B length = " << write_offset_ << std::endl;
    }

    // std::cout << "Selected buffer length = " << write_offset_ << std::endl;
}

std::vector<short> PingPongBuffer::get_current_buffer() const
{
    if (current_buffer_ == &buffer_a_)
    {
        return std::vector<short>(buffer_a_.begin(), buffer_a_.begin() + capacity_ / 2);
    }
    else
    {
        return std::vector<short>(buffer_b_.begin(), buffer_b_.begin() + capacity_ / 2);
    }
}

bool PingPongBuffer::is_full() const
{
    return buffer_a_full_ || buffer_b_full_;
}

void PingPongBuffer::trigger_callback()
{
    if (on_buffer_full_callback_)
    {
        if (current_buffer_ == &buffer_a_)
        {
            std::vector<short> full_buffer(buffer_a_.begin(), buffer_a_.end());
            on_buffer_full_callback_(full_buffer, 0); // Pass 0 for buffer A
        }
        else
        {
            std::vector<short> full_buffer(buffer_b_.begin(), buffer_b_.end());
            on_buffer_full_callback_(full_buffer, 1); // Pass 1 for buffer B
        }
    }
}

void PingPongBuffer::set_on_buffer_full_callback(FullBufferCallback callback)
{
    on_buffer_full_callback_ = callback;
}

void PingPongBuffer::switch_buffer()
{
    if (current_buffer_ == &buffer_a_)
    {
        current_buffer_ = &buffer_b_;
        std::fill(buffer_a_.begin(), buffer_a_.end(), 0);
        write_offset_ = 0;
        read_offset_ = 0;
        // std::cout << "Switching to buffer B" << std::endl;
    }
    else
    {
        current_buffer_ = &buffer_a_;
        std::fill(buffer_b_.begin(), buffer_b_.end(), 0);
        write_offset_ = 0;
        read_offset_ = 0;
        // std::cout << "Switching to buffer A" << std::endl;
    }
    buffer_a_full_ = false;
    buffer_b_full_ = false;
}