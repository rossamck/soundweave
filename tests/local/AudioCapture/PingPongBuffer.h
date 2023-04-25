#ifndef PINGPONGBUFFER_H
#define PINGPONGBUFFER_H

#include <vector>
#include <iostream>

/**
 * @brief Special buffer calls callback when full
 *
 * Fills up one of two buffers over time through 'add_data' calls and makes call to callback
 * with the full buffer.
 */
class PingPongBuffer
{
public:
    typedef void (*FullBufferCallback)(const std::vector<short>&, int);

    /**
     * @brief Constructor for class
     *
     * Initialises all members variables.
     */
    PingPongBuffer(int capacity);

    /**
     * @brief Function for adding data to the PingPongBuffer 
     *
     * Fills the current buffer with the provided data. 
     * If buffer reaches capacity, all data contained in it is sent out via callbacks, 
     * then buffer is switched out and remaining data added to new buffer.
     */
    void add_data(const std::vector<short> &data);

    /**
     * @brief Returns half the data of the current buffer
     *
     * Returns all data contained within the first half of the current buffer
     */
    std::vector<short> get_current_buffer() const;

    /**
     * @brief Returns true if any of the buffers are full
     *
     */
    bool is_full() const;

    /**
     * @brief Calls the callback registered with the class with buffer data
     *
     * Call the callback with the current buffer and a '0' if buffer A and a '1' if buffer B.
     * Does nothing if no callback is registered.
     */
    void trigger_callback();

    /**
     * @brief Sets the registered callback to the one provided
     *
     */
    void set_on_buffer_full_callback(FullBufferCallback callback);


private:
    int capacity_;
    std::vector<short> buffer_a_;
    std::vector<short> buffer_b_;
    std::vector<short> *current_buffer_;
    int write_offset_;
    int read_offset_;
    bool buffer_a_full_;
    bool buffer_b_full_;
    FullBufferCallback on_buffer_full_callback_;

    /**
     * @brief Switches current buffer out with the other, and resets its values
     *
     * Resets all values to 0. Resets read and write offset to 0. 
     * Lastly, marks both buffers as empty.
     */
    void switch_buffer();
};

#endif // PINGPONGBUFFER_H