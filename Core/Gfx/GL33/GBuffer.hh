#pragma once

#include <vector>
#include <flextGL.h>
#include <Core/Visual/Resolution.hh>

namespace Ares
{
namespace GL33
{

/// A G-Buffer, i.e. a FBO with some attachment textures.
class GBuffer
{
public:
    /// The description of an attachment texture to the G-Buffer.
    struct Attachment
    {
        GLenum format = GL_RGBA; ///< The format of the attachment texture.
        GLenum internalFormat = GL_RGBA8; ///< The internal format of the attachment texture.
        GLenum minFilter = GL_NEAREST; ///< The minification filter for the attachment texture.
        GLenum magFilter = GL_NEAREST; ///< The magnification filter for the attachment texture.
    };

private:
    struct AttachmentSlot
    {
        Attachment attachment;
        GLuint texture;
    };

    std::vector<AttachmentSlot> attachments_;
    GLuint fbo_;
    bool fboOk_;
    Resolution resolution_;

    /// Binds `fbo_` to GL_FRAMEBUFFER, then attachs all attachment textures to
    /// it and sets `fboOk_` to wether this succeeded or not.
    void setupFbo();

    GBuffer(const GBuffer& toCopy) = delete;
    GBuffer& operator=(const GBuffer& toCopy) = delete;

public:
    /// Creates a new, uninitialized G-Buffer.
    /// The base resolution will set to (0, 0).
    GBuffer();

    /// Initializes a new G-Buffer given a list of attachments and its base
    /// resolution.
    ///
    /// **OpenGL**: rebinds `GL_TEXTURE_2D`, `GL_FRAMEBUFFER`
    GBuffer(Resolution resolution, std::initializer_list<Attachment> attachments);

    GBuffer(GBuffer&& toMove);
    GBuffer& operator=(GBuffer&& toMove);

    /// **OpenGL**: rebinds `GL_FRAMEBUFFER`
    ~GBuffer();

    /// Returns `true` if the GBuffer is currently both initialized and in a
    /// consistent state (i.e. all textures were attached to it succesfully).
    inline operator bool() const
    {
        return fbo_ != 0 && fboOk_;
    }


    /// Returns the current base resolution of the G-Buffer.
    inline Resolution resolution() const
    {
        return resolution_;
    }

    /// Returns the OpenGL name of the underlying FBO.
    inline GLuint fbo() const
    {
        return fbo_;
    }

    /// Returns the OpenGL name of the attachment texture with the given index,
    /// or `0` if the index is out of bounds.
    inline GLuint attachmentTexture(unsigned int index) const
    {
        return index < attachments_.size() ? attachments_[index].texture : 0;
    }

    /// Returns the number of attachments the G-Buffer has.
    inline unsigned int nAttachments() const
    {
        return attachments_.size();
    }


    /// Changes the base resolution of the G-Buffer, rescaling/regenerating all
    /// attachment textures as needed.
    /// Check `operator bool()` afterwards to see if the resizing succeeded.
    ///
    /// **OpenGL**: rebinds `GL_TEXTURE_2D`, `GL_FRAMEBUFFER`
    void resize(Resolution newResolution);
};

}
}
