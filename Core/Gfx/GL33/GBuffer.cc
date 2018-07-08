#include "GBuffer.hh"

namespace Ares
{
namespace GL33
{

GBuffer::GBuffer()
    : fbo_(0), fboOk_(false), resolution_{0, 0}
{
}

GBuffer::GBuffer(Resolution resolution, std::initializer_list<Attachment> attachments)
    : GBuffer()
{
    for(auto attachment : attachments)
    {
        attachments_.emplace_back(AttachmentSlot{attachment, 0});
    }

    for(auto it = attachments_.begin(); it != attachments_.end(); it ++)
    {
        glGenTextures(1, &it->texture);
        if(!it->texture)
        {
            // Attachment texture creation error
            return;
        }
    }

    resize(resolution); // Will create and setup FBO
}

GBuffer::GBuffer(GBuffer&& toMove)
    : GBuffer()
{
    (void)operator=(std::move(toMove));
}

GBuffer& GBuffer::operator=(GBuffer&& toMove)
{
    (void)this->~GBuffer();

    // Move data over
    attachments_ = std::move(toMove.attachments_);
    fbo_ = toMove.fbo_;
    fboOk_ = toMove.fboOk_;
    resolution_ = toMove.resolution_;

    // Invalidate the moved instance
    toMove.fbo_ = 0;
    toMove.fboOk_ = false;

    return *this;
}

GBuffer::~GBuffer()
{
    if(fbo_)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        for(auto it = attachments_.begin(); it != attachments_.end(); it ++)
        {
            glDeleteTextures(1, &it->texture); it->texture = 0;
        }
    }
}


/// Returns:
/// - `GL_COLOR_ATTACHMENT0` if `format` is a color format
/// - `GL_DEPTH_ATTACHMENT` if `format` is a depth format
/// - `GL_STENCIL_ATTACHMENT` if `format` is a stencil attachment
/// - `GL_DEPTH_STENCIL_ATTACHMENT` if `format` is a depth-stencil attachment
inline static GLenum fboAttachmentType(GLenum format)
{
    switch(format)
    {
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32:
    case GL_DEPTH_COMPONENT32F:
        return GL_DEPTH_ATTACHMENT;

    case GL_DEPTH_STENCIL:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
        return GL_DEPTH_STENCIL_ATTACHMENT;

    case GL_STENCIL_INDEX:
    case GL_STENCIL_INDEX1:
    case GL_STENCIL_INDEX4:
    case GL_STENCIL_INDEX8:
    case GL_STENCIL_INDEX16:
        return GL_DEPTH_ATTACHMENT;

    default:
        return GL_COLOR_ATTACHMENT0;
    };
}

void GBuffer::setupFbo()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    unsigned int nColorAttachments = 0;
    for(unsigned int i = 0; i < attachments_.size(); i ++)
    {
        GLenum attachmentType = fboAttachmentType(attachments_[i].attachment.format);
        attachmentType = attachmentType == GL_COLOR_ATTACHMENT0
                         ? attachmentType + (nColorAttachments ++) // (new color attachment added)
                         : attachmentType;

        glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType,
                               GL_TEXTURE_2D, attachments_[i].texture, 0);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    fboOk_ = status == GL_FRAMEBUFFER_COMPLETE;
}

void GBuffer::resize(Resolution newResolution)
{
    if(newResolution == resolution_)
    {
        return;
    }
    resolution_ = newResolution;

    // Delete old FBO (if any)
    if(fbo_)
    {
        glDeleteFramebuffers(1, &fbo_); fbo_ = 0;
    }

    // Regenerate all attachment textures
    for(auto it = attachments_.begin(); it != attachments_.end(); it ++)
    {
        GLenum attachmentType = fboAttachmentType(it->attachment.format);
        GLenum inDataType = attachmentType != GL_DEPTH_STENCIL_ATTACHMENT
                            ? GL_FLOAT
                            : GL_UNSIGNED_INT_24_8;
        glBindTexture(GL_TEXTURE_2D, it->texture);
        glTexImage2D(GL_TEXTURE_2D, 0, it->attachment.internalFormat,
                     resolution_.width, resolution_.height,
                     0, it->attachment.format, inDataType, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, it->attachment.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, it->attachment.magFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    }

    // [Re]create FBO
    glGenFramebuffers(1, &fbo_);
    setupFbo();
}

}
}
