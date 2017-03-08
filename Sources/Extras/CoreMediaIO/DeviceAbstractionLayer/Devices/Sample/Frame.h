//
// Created by julee on 16-11-19.
//

#ifndef INSTA360_FRAME_H
#define INSTA360_FRAME_H
#include <stdint.h>
#include <string.h>
#include <memory>
#include <vector>

class Frame
{
public:
    Frame(void *data, int size, bool isStillImage)
    {
        mBuf = std::shared_ptr<std::vector<uint8_t> >(new std::vector<uint8_t>(size));
        memcpy((*mBuf).data(), data, size);
        mIsStillImage = isStillImage;
    }

    uint8_t *data()
    {
        return mBuf->data();
    }

    int size()
    {
        return (int)mBuf->size();
    }

    bool isStillImage()
    {
        return mIsStillImage;
    }

private:
    std::shared_ptr<std::vector<uint8_t> > mBuf;
    bool mIsStillImage;
};

#endif //INSTA360_FRAME_H
