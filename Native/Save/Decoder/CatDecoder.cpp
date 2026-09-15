#include "../Model/CatData.h"
#include "../Decoder/CatDecoder.h"
#include "../Compression/Lz4.h"
#include "../../Logger.h"

#include <cstring>
#include <cstdint>
static uint32_t ReadU32(
    const std::vector<unsigned char> &d,
    size_t off)
{
    uint32_t v = 0;

    if (off + 4 <= d.size())
        memcpy(&v, d.data() + off, 4);

    return v;
}

static uint64_t ReadU64(
    const std::vector<unsigned char> &d,
    size_t off)
{
    uint64_t v = 0;

    if (off + 8 <= d.size())
        memcpy(&v, d.data() + off, 8);

    return v;
}
bool DecompressCatBlob(
    const std::vector<unsigned char> &input,
    std::vector<unsigned char> &output)
{
    if (input.size() < 4)
        return false;

    uint32_t uncomp;

    memcpy(
        &uncomp,
        input.data(),
        4);

    output.resize(uncomp);

    const unsigned char *stream =
        input.data() + 4;

    size_t streamSize =
        input.size() - 4;

    bool ok =
        Lz4DecompressBlock(
            stream,
            streamSize,
            output.data(),
            output.size());

    if (!ok)
    {
        Log("[CatManager] LZ4 failed");
        output.clear();
        return false;
    }

    return true;
}

static bool ContainsU64(
    const std::vector<unsigned char> &data,
    uint64_t value)
{
    for (size_t i = 0; i + 8 <= data.size(); i++)
    {
        uint64_t v = 0;

        memcpy(
            &v,
            data.data() + i,
            8);

        if (v == value)
            return true;
    }

    return false;
}

static bool FindU64(
    const std::vector<unsigned char> &data,
    uint64_t value,
    size_t &offset)
{
    for (size_t i = 0; i + 8 <= data.size(); i++)
    {
        uint64_t v = 0;

        memcpy(
            &v,
            data.data() + i,
            8);

        if (v == value)
        {
            offset = i;
            return true;
        }
    }

    return false;
}

bool FindU32(
    const std::vector<unsigned char> &data,
    uint32_t target,
    size_t &offset)
{
for (size_t i = 0; i + 4 <= data.size(); i += 4)
    {
        uint32_t value;

        memcpy(
            &value,
            data.data() + i,
            4);

        if (value == target)
        {
            offset = i;
            return true;
        }
    }

    return false;
}

CatData DecodeCat(const std::vector<uint8_t> &data)
{
    CatData cat{};

    if (data.size() < 32)
        return cat;

    cat.id = ReadU64(data, 4);

    uint32_t nameLen =
        ReadU32(data, 0x0C);

    if (nameLen > 0 && nameLen < 128)
    {
        size_t start = 0x14;

        for (uint32_t i = 0; i < nameLen; i++)
        {
            uint16_t c = 0;

            memcpy(
                &c,
                data.data() + start + i * 2,
                2);

            if (c)
                cat.name += (char)c;
        }
    }

        return cat;
    }
