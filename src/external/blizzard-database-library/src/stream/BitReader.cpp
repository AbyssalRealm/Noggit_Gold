#include <stream/BitReader.h>

namespace BlizzardDatabaseLib {
    namespace Stream {

        BitReader::BitReader(std::unique_ptr<char[]>& dataStart, unsigned int dataLength) : _dataStart(dataStart), DataLength(dataLength), Position(0), Offset(0)
        {

        }

        std::string BitReader::ReadNullTermintingString()
        {
            std::stringstream stream;
            unsigned char byte;
            while ((byte = (unsigned char)ReadUint32(8)) != '\0')
                stream << byte;

            return stream.str();
        }

        int BitReader::ReadInt32(int numberOfBits)
        {
          return static_cast<int>(ReadUint32(numberOfBits));
        }

        unsigned int BitReader::ReadUint32(int numberOfBits)
        {
            auto index = Offset + (Position >> 3);
            auto p1 = (32 - numberOfBits - (Position & 7));
            auto p2 = (32 - numberOfBits);
            constexpr size_t ptrSize = sizeof(uint32_t);

            if (index + ptrSize > DataLength) {
                size_t over = (index + ptrSize) - DataLength;
                if (over > 3) {
                    throw std::out_of_range("Read access out of bounds");
                }
            }

            unsigned int result;
            memcpy(&result, _dataStart.get() + index, ptrSize);

            if (p1 > 0)
                result = result << p1;
            result = result >> p2;

            Position += numberOfBits;
            return result;
        }

        unsigned long long BitReader::ReadUint64(int numberOfBits)
        {
            auto index = Offset + (Position >> 3);
            auto p1 = (64 - numberOfBits - (Position & 7));
            auto p2 = (64 - numberOfBits);
            unsigned long long ptrSize = sizeof(unsigned long long);

            // hackfix because it reads out of bounds if using ptrSize 8 with smaller types
            if (numberOfBits <= 32)
              ptrSize = sizeof(unsigned int);

            if (index + ptrSize > DataLength) {
                size_t over = (index + ptrSize) - DataLength;
                if (over > 3) {
                    throw std::out_of_range("Read access out of bounds");
                }
            }

            unsigned long long result;
            memcpy(&result, _dataStart.get() + index, ptrSize);

            if (p1 > 0)
                result = result << p1;
            result = result >> p2;

            Position += numberOfBits;
            return result;
        }

        Structures::Int64 BitReader::ReadValue64(int numberOfBits)
        {
            auto value = Structures::Int64();
            value.ULongLong = ReadUint64(numberOfBits);
            return value;
        }

        Structures::Int64 BitReader::ReadSignedValue64(int numberOfBits)
        {
            auto value = Structures::Int64();
            value.ULongLong = ReadUint64(numberOfBits);
            auto signedShift = 1UL << (numberOfBits - 1);
            value.LongLong = (signedShift ^ value.ULongLong) - signedShift;
            return value;
        }
    }
}