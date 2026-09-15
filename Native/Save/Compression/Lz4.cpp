#include "../Decoder/CatDecoder.h"

#include <cstring>


bool Lz4DecompressBlock(
    const unsigned char* src,
    size_t srcSize,
    unsigned char* dst,
    size_t dstSize
)
{
    size_t srcPos = 0;
    size_t dstPos = 0;


    while(srcPos < srcSize && dstPos < dstSize)
    {
        unsigned char token = src[srcPos++];

        int literalLen = token >> 4;
        int matchLen = token & 0x0F;


        // literal extension
        if(literalLen == 15)
        {
            unsigned char b;

            do
            {
                if(srcPos >= srcSize)
                    return false;

                b = src[srcPos++];

                literalLen += b;

            } while(b == 255);
        }


        // copy literals

        if(srcPos + literalLen > srcSize)
            return false;


        if(dstPos + literalLen > dstSize)
            return false;


        memcpy(
            dst + dstPos,
            src + srcPos,
            literalLen
        );


        srcPos += literalLen;
        dstPos += literalLen;


        if(srcPos >= srcSize)
            break;


        // offset

        if(srcPos + 2 > srcSize)
            return false;


        int offset =
            src[srcPos] |
            (src[srcPos+1] << 8);


        srcPos += 2;


        if(offset == 0 || offset > dstPos)
            return false;


        // match length

        matchLen += 4;


        if((token & 0x0F) == 15)
        {
            unsigned char b;

            do
            {
                if(srcPos >= srcSize)
                    return false;


                b = src[srcPos++];

                matchLen += b;

            } while(b == 255);
        }


        // copy match

        for(int i=0;i<matchLen;i++)
        {
            if(dstPos >= dstSize)
                return false;


            dst[dstPos] =
                dst[dstPos-offset];


            dstPos++;
        }
    }


    return dstPos == dstSize;
}