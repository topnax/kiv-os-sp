#include "pipe.h"


Pipe::Pipe(size_t capacity) :
        fill_count(0), empty_count(capacity), capacity(capacity) {
    this->buffer.resize(capacity);
}

/*
 * This class implements the general producer consumer concept
 */

void Pipe::Close_Out() {
    this->closed_out = true;

    // notify anyone trying to write to the pipe or read from the pipe
    this->empty_count.notify();
    this->fill_count.notify();
}

void Pipe::Close_In() {
    if (!this->closed_in) {
        // pipe is about to be closed, write EOT
        this->write_char(static_cast<char>(kiv_hal::NControl_Codes::EOT));
        this->closed_in = true;
    }
}

std::vector<char> Pipe::Read(size_t nodes_to_read, bool &nothing_left_to_read) {
    std::vector<char> read;
    read.reserve(nodes_to_read);

    for (size_t i = 0; i < nodes_to_read; i++) {
        // check whether out handle has been closed and there is nothing left to read
        if (this->closed_out && read_ptr == write_ptr) {
            nothing_left_to_read = true;
            return read;
        }
        this->fill_count.wait();

        // check whether the out handle has not been closed while we have been waiting
        if (this->closed_out) {
            return read;
        }

        char read_char = this->buffer[this->read_ptr];
        read.push_back(read_char);

        this->read_ptr++;

        // end of the buffer reached, roll over
        if (read_ptr == this->capacity) this->read_ptr = 0;

        this->empty_count.notify();

        if (read_char == static_cast<char>(kiv_hal::NControl_Codes::EOT)) {
            // EOT has been read, stop reading
            this->closed_out = true;
            return read;
        }
    }

    return read;
}

bool Pipe::write_char(char c) {
    this->empty_count.wait();

    // check whether the out handle has not been closed while we have been waiting
    if (this->closed_out) {
        // out handle closed - we can't write the char
        return false;
    }

    // receiving close signal while waiting for empty_count should result in writing EOT only
    // current implementation allows writer to write one more byte before pipe is closed

    this->buffer[this->write_ptr] = c;
    this->write_ptr++;

    // end of the buffer reached, roll over
    if (this->write_ptr == capacity) write_ptr = 0;

    this->fill_count.notify();

    return true;
}

size_t Pipe::Write(std::vector<char> data) {
    size_t nodes_to_write = data.size();
    size_t nodes_written = 0;

    for (size_t i = 0; i < nodes_to_write; i++) {
        if (!this->closed_in) {
            if (!write_char(data.at(i))) {
                // stop writing
                break;
            }
            nodes_written++;
        } else {
            break;
        }
    }
    return nodes_written;
}
