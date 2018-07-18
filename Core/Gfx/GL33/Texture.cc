#include "Texture.hh"

#include "../Base/MapTree.hh"

namespace Ares
{
namespace GL33
{

static MapTree<ImageFormat::Channel, GLenum> gInternalFormatTree;
static bool gInternalFormatTreePopulated = false;

static void populateInternalFormatTree()
{
    if(gInternalFormatTreePopulated)
    {
        return;
    }

// Defines a mapping between [1..4] `ImageFormats` with 1 to 4 `aresName`
// `ImageFormat::Channel`s and `GL_[RGBA]${glName}` OpenGL formats
#define ARES_addToGlFormatTree(aresName, glName) do \
    { \
        auto it = gInternalFormatTree.at(aresName); it = GL_R ## glName; \
        it = it.at(aresName); it = GL_RG ## glName; \
        it = it.at(aresName); it = GL_RGB ## glName; \
        it = it.at(aresName); it = GL_RGBA ## glName; \
    } while(false)

    using Ch = ImageFormat::Channel;


    // Common R, RG, RGB, RGBA formats
    ARES_addToGlFormatTree(Ch::I8, 8I);
    ARES_addToGlFormatTree(Ch::U8, 8UI);
    ARES_addToGlFormatTree(Ch::UN8, 8);

    ARES_addToGlFormatTree(Ch::I16, 16I);
    ARES_addToGlFormatTree(Ch::U16, 16UI);
    ARES_addToGlFormatTree(Ch::UN16, 16);

    ARES_addToGlFormatTree(Ch::F16, 16F);
    ARES_addToGlFormatTree(Ch::F32, 32F);

    // Special formats
    gInternalFormatTree.at(Ch::UN10, Ch::UN10, Ch::UN10, Ch::UN2) = GL_RGB10_A2;
    gInternalFormatTree.at(Ch::U10, Ch::U10, Ch::U10, Ch::U2) = GL_RGB10_A2UI;
    gInternalFormatTree.at(Ch::F32Depth) = GL_DEPTH_COMPONENT32F;
}


bool textureFormats(GLenum& outFormat, GLenum& outInternalFormat,
                    ImageFormat imageFormat)
{
    populateInternalFormatTree();

    unsigned int nChannels = imageFormat.nChannelsSet();
    if(!imageFormat.isValid() || nChannels == 0)
    {
        return false;
    }

    // `format` depends on the number of channels
    static constexpr const GLenum textureFormats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };

    switch(imageFormat.channels[0])
    {
        case ImageFormat::Channel::F32Depth:
            outFormat = GL_DEPTH_COMPONENT;
        break;

        default:
            outFormat = textureFormats[nChannels];
        break;
    }

    // `internalFormat` needs to be looked up into the format tree
    // Look up each non-`None` channel in sequence in the format tree
    auto it = gInternalFormatTree.begin();
    for(unsigned int i = 0;
        i < 4 && imageFormat.channels[i] != ImageFormat::Channel::None;
        i ++)
    {
        auto nextIt = it.get(imageFormat.channels[i]);
        if(!nextIt)
        {
            // No correspondence for this combination of channels in the internal format tree
            return false;
        }
        it = nextIt;
    }

    // The internal format is the value at the last looked-up node
    outInternalFormat = it.value();

    return true;
}

}
}
