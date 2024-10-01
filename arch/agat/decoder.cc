#include "lib/core/globals.h"
#include "lib/decoders/decoders.h"
#include "agat.h"
#include "lib/core/crc.h"
#include "lib/fluxmap.h"
#include "lib/decoders/fluxmapreader.h"
#include "lib/decoders/fluxpattern.h"
#include "lib/sector.h"
#include "lib/core/bytes.h"
#include "fmt/format.h"
#include <string.h>

// clang-format off
/*
 * data:    X  X  X  X   X  X  X  X   X  -  -  X   -  X  -  X   -  X  X  -   X  -  X  -  = 0xff956a
 * flux:   01 01 01 01  01 01 01 01  01 00 10 01  00 01 00 01  00 01 01 00  01 00 01 00  = 0x555549111444
 * 
 * data:    X  X  X  X   X  X  X  X   -  X  X  -   X  -  X  -   X  -  -  X   -  X  -  X  = 0xff6a95
 * flux:   01 01 01 01  01 01 01 01  00 01 01 00  01 00 01 00  01 00 10 01  00 01 00 01  = 0x555514444911
 *
 * Each pattern is prefixed with this one:
 *
 * data:          -  -   -  X   -  -   X  - = 0x12
 * flux:    (10) 10 10  10 01  00 10  01 00 = 0xa924
 * magic:   (10) 10 00  10 01  00 10  01 00 = 0x8924
 *                  ^
 *
 * This seems to be generated by emitting A4 in MFM and then a single 0 bit to
 * shift it out of phase, so the data bits become clock bits and vice versa.
 *
 *           X - X - - X - -  = 0xA4
 *          0100010010010010  = MFM encoded
 *           1000100100100100 = with trailing zero
 *            - - - X - - X - = effective bitstream = 0x12
 */
// clang-format on

static const FluxPattern SECTOR_PATTERN(64, SECTOR_ID);
static const FluxPattern DATA_PATTERN(64, DATA_ID);

static const FluxMatchers ALL_PATTERNS = {&SECTOR_PATTERN, &DATA_PATTERN};

class AgatDecoder : public Decoder
{
public:
    AgatDecoder(const DecoderProto& config): Decoder(config) {}

    nanoseconds_t advanceToNextRecord() override
    {
        return seekToPattern(ALL_PATTERNS);
    }

    void decodeSectorRecord() override
    {
        if (readRaw64() != SECTOR_ID)
            return;

        auto bytes = decodeFmMfm(readRawBits(64)).slice(0, 4);
        if (bytes[3] != 0x5a)
            return;

        _sector->logicalTrack = bytes[1] >> 1;
        _sector->logicalSector = bytes[2];
        _sector->logicalSide = bytes[1] & 1;
        _sector->status = Sector::DATA_MISSING; /* unintuitive but correct */
    }

    void decodeDataRecord() override
    {
        if (readRaw64() != DATA_ID)
            return;

        Bytes bytes = decodeFmMfm(readRawBits((AGAT_SECTOR_SIZE + 2) * 16))
                          .slice(0, AGAT_SECTOR_SIZE + 2);

        if (bytes[AGAT_SECTOR_SIZE + 1] != 0x5a)
            return;

        _sector->data = bytes.slice(0, AGAT_SECTOR_SIZE);
        uint8_t wantChecksum = bytes[AGAT_SECTOR_SIZE];
        uint8_t gotChecksum = agatChecksum(_sector->data);
        _sector->status =
            (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
    }
};

std::unique_ptr<Decoder> createAgatDecoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new AgatDecoder(config));
}
