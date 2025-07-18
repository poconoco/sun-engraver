#include <Arduino.h>

template <size_t NUM_BITS>
class BitSet {
public:
    BitSet() { 
        clear(); 
    }

    void set(size_t index, bool value = true) {
        if (index >= NUM_BITS) 
            return;
        size_t byte = index / 8;
        uint8_t mask = 1 << (index % 8);
        
        if (value) 
            bits[byte] |= mask;
        else
            bits[byte] &= ~mask;
    }

    bool get(size_t index) const {
        if (index >= NUM_BITS) 
            return false;

        size_t byte = index / 8;
        uint8_t mask = 1 << (index % 8);
        return bits[byte] & mask;
}   

    void clear() {
        for (size_t i = 0; i < numBytes(); ++i)
            bits[i] = 0;
    }

    constexpr size_t size() const { return NUM_BITS; }

private:
    static constexpr size_t numBytes() { 
        return (NUM_BITS + 7) / 8; 
    }

    uint8_t bits[numBytes()] = {};
};
