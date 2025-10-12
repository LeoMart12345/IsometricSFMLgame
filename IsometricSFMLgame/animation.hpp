//SWAP TO SPRITE SHEET SO ITS MORE EFFICIENT

#pragma once


#include <SFML/Graphics.hpp>
#include <vector>

class Animation
{
public:

    unsigned int startFrame;
    unsigned int endFrame;

    float duration;

    Animation(unsigned int startFrame, unsigned int endFrame, float duration)
    {
        this->startFrame = startFrame;
        this->endFrame = endFrame;
        this->duration = duration;
    }

    unsigned int getLength() { return endFrame - startFrame + 1; }
};
