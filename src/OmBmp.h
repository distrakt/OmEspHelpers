#ifndef __OmBmp__
#define __OmBmp__

#include <stdint.h>
#include "OmXmlWriter.h"

// This has classes for writing BMPs directly into the OmXmlWriter.
// So you could serve a bitmap image, or embed it right in the <img> html element
// as a data attribute.

/*! Streaming .bmp file writer. Feed pixels into it with put1bit, only */
/*! exactly as many as there are, row by row, left to right. */
/*! adapted liberally from from http://paulbourke.net/dataformats/bmp/ */

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

/*! @brief class to stream a 1-bit .bmp bitmap file, as you feed it pixel by pixel */
class OmBitmap1BmpStream : public OmIByteStream
{
public:
    OmIByteStream *consumer;
    bool put(uint8_t ch) override
    {
        UNUSED(ch);
        return false; // not supported by this streamer
    }

    int width;
    int height;
    bool didPutHeader;
    int x;
    int y;
    int rowPadBytes;
    uint8_t byteInProgress;

    bool putBmpHeader(int width, int height)
    {
        int rowBytes = ((width + 7) / 8 + 3) & 0xfffffffc;
        int dataSize = rowBytes * height;
        int paletteSize = 8; // 2 colors, 4 bytes each.
        const int bmpHeaderSize = 14;
        const int bmpInfoHeaderSize = 40;
        int fileSize = bmpHeaderSize + bmpInfoHeaderSize + paletteSize + dataSize;

        bool result = true;
        // header
        result &= put8('B');
        result &= put8('M');
        result &= put32(fileSize);
        result &= put16(0); // reserved1
        result &= put16(0); // reserved2
        result &= put32(bmpHeaderSize + bmpInfoHeaderSize + paletteSize); // offset to data

        // DIB info header
        result &= put32(bmpInfoHeaderSize);
        result &= put32(this->width); // width
        result &= put32(this->height); // height
        result &= put16(1); // planes
        result &= put16(1); // bits
        result &= put32(0); // compression
        result &= put32(dataSize);
        result &= put32(1000); // xresolution px/m
        result &= put32(1000); // yresolution
        result &= put32(2); // ncolours
        result &= put32(0); // important colors

        // Write the two color palette.

        result &= put32(0x00101010); // color black. (not really black and white, for my thing this looks good)
        result &= put32(0x00c0c0c0);  // color white.
        return result;
    }

    /*! @brief Instantiate the bitmap streamer, with width and height. */
    OmBitmap1BmpStream(OmIByteStream *consumer, int width, int height)
    {
        this->consumer = consumer;
        this->didPutHeader = false;
        this->x = 0;
        this->y = 0;
        this->width = width;
        this->height = height;
        this->byteInProgress = 0;

        /*! after last pixel of a row (which triggers a put, always) */
        /*! 0-3 more pad bytes may be issued to make rowbytes multiple 4. */
        this->rowPadBytes = (4 - ((width % 32) + 7) / 8) & 3; // bytes to emit BEFORE hitting 8 pixels, when x == width
    }

    /*! @brief emit a single 1-bit pixel of our bitmap image.
     Will return true for each bit you send, until width*height have been sent.
     Then no futher bits will get streamed out, and return false. */
    bool put1Bit(int bit)
    {
        if(this->x >= this->width && this->y >= this->height)
            return false;
        bool result = true;

        if(!didPutHeader)
        {
            result &= this->putBmpHeader(this->width, this->height);
            this->didPutHeader = true;
        }
        this->byteInProgress |= bit << (7 - this->x % 8);
        this->x++;
        if(this->x == this->width || (this->x & 0x0007) == 0) // finished one byte in progress
        {
            this->consumer->put(this->byteInProgress);
            this->byteInProgress = 0;
        }

        if(this->x == this->width)
        {
            // at the end of the row, do our special dance
            for(int ix = 0; ix < this->rowPadBytes; ix++)
                result &= this->put8(0);
            this->x = 0;
            this->y++;
        }
        return result;
    }


private:
    bool putN(int n, uint8_t *data)
    {
        bool result = true;
        while(n-- > 0)
            result &= this->consumer->put(*data++);
        return result;
    }

    bool put8(uint8_t x)
    {
        return this->putN(1, &x);
    }

    bool put16(uint16_t x)
    {
        return this->putN(2, (uint8_t *)&x);
    }

    bool put32(uint32_t x)
    {
        return this->putN(4, (uint8_t *)&x);
    }

};


/*! @brief class to stream a 24-bit .bmp pixel map file, as you feed it pixel by pixel */
class OmBitmap24BmpStream : public OmIByteStream
{
public:
    OmIByteStream *consumer;
    bool didPutHeader = false;
    int width;
    int height;
    int x;
    int y;

    OmBitmap24BmpStream(OmIByteStream *consumer, int width, int height)
    {
        this->consumer = consumer;
        this->width = width;
        this->height = height;
        this->didPutHeader = false;
        this->x = 0;
        this->y = 0; // position in progress writing
    }

    bool put(uint8_t ch) override
    {
        UNUSED(ch);
        return false; // not supported by this streamer
    }

    bool putBmpHeader()
    {
        //ok, go!
        int rowBytes = (this->width * 3 + 3) & 0xffffFFFC;
        int dataSize = rowBytes * this->height;
        int paletteSize = 0; // no color palette
        const int bmpHeaderSize = 14;
        const int bmpInfoHeaderSize = 40;
        int fileSize = bmpHeaderSize + bmpInfoHeaderSize + paletteSize + dataSize;

        bool result = true;
        // header
        result &= put8('B');
        result &= put8('M');
        result &= put32(fileSize);
        result &= put16(0); // reserved1
        result &= put16(0); // reserved2
        result &= put32(bmpHeaderSize + bmpInfoHeaderSize + paletteSize); // offset to data

        // DIB info header
        result &= put32(bmpInfoHeaderSize);
        result &= put32(this->width); // width
        result &= put32(this->height); // height
        result &= put16(1); // planes
        result &= put16(24); // bits/px
        result &= put32(0); // compression
        result &= put32(dataSize);
        result &= put32(1000); // xresolution px/m
        result &= put32(1000); // yresolution
        result &= put32(2); // ncolours
        result &= put32(0); // important colors

        // And no color palette.
        return result;
    }

    /*! @brief emit a single 24-bit pixel of our pixel map image.
     Will return true for each pixel you send, until width*height have been sent.
     Then no futher bits will get streamed out, and return false. */
    bool put1Pixel(uint8_t r, uint8_t g, uint8_t b)
    {
        if(this->x >= this->width && this->y >= this->height)
            return false;
        bool result = true;

        if(!didPutHeader)
        {
            result &= this->putBmpHeader();
            this->didPutHeader = true;
        }
        result &= this->put8(b);
        result &= this->put8(g);
        result &= this->put8(r);

        this->x++;
        if(this->x >= this->width)
        {
            // Each row must be padded to a multiple of four BYTES.
            // since each pixel is 3 bytes, this can happen.
            int extras = (4 - (this->width * 3) % 4) % 4;
            while(extras-- > 0)
                this->put8(0);

            this->x = 0;
            this->y++;
            // And did we just hit the final row?
            if(this->y >= this->height)
            {
                this->consumer->done();
                this->done();
            }
        }

        return result;
    }

private:
    bool putN(int n, uint8_t *data)
    {
        bool result = true;
        while(n-- > 0)
            result &= this->consumer->put(*data++);
        return result;
    }

    bool put8(uint8_t x)
    {
        return this->putN(1, &x);
    }

    bool put16(uint16_t x)
    {
        return this->putN(2, (uint8_t *)&x);
    }

    bool put32(uint32_t x)
    {
        return this->putN(4, (uint8_t *)&x);
    }

};

extern const uint8_t base64_table[65];



/*! @brief Given a consumer, feed in bytes and put out base64 bytes.
call done() to emit the last bit of padding.
Adapted liberally from  *  http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c */
class OmBase64Stream : public OmIByteStream
{
public:
    OmIByteStream *consumer = NULL;

    OmBase64Stream(OmIByteStream *consumer)
    {
        this->consumer = consumer;
    }

    uint8_t buf[3];
    int bufSize = 0;
    bool put(uint8_t ch) override
    {
        if(this->isDone)
            return false;

        bool result = true;
        if(this->bufSize < 3)
            this->buf[this->bufSize++] = ch;

        if(this->bufSize == 3)
        {
            // when we get three chars, emit four.
            result &= this->consumer->put(base64_table[this->buf[0] >> 2]);
            result &= this->consumer->put(base64_table[((this->buf[0] & 0x03) << 4) | (this->buf[1] >> 4)]);
            result &= this->consumer->put(base64_table[((this->buf[1] & 0x0f) << 2) | (this->buf[2] >> 6)]);
            result &= this->consumer->put(base64_table[this->buf[2] & 0x3f]);
            bufSize = 0;
        }
        return result;
    }

    bool done() override
    {
        bool result = true;
        this->isDone = true;
        // close any last bits
        switch(this->bufSize)
        {
            case 0:
                break;

            case 1:
                result &= this->consumer->put(base64_table[this->buf[0] >> 2]);
                result &= this->consumer->put(base64_table[(this->buf[0] & 0x03) << 4]);
                result &= this->consumer->put('=');
                result &= this->consumer->put('=');
                break;

            case 2:
                result &= this->consumer->put(base64_table[this->buf[0] >> 2]);
                result &= this->consumer->put(base64_table[((this->buf[0] & 0x03) << 4) | (this->buf[1] >> 4)]);
                result &= this->consumer->put(base64_table[(this->buf[1] & 0x0f) << 2]);
                result &= this->consumer->put('=');
                break;
        }
        this->bufSize = 0;
        return result;
    }
};

#endif // __OmBmp__
