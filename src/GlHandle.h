#pragma once
#include <glad/glad.h>

namespace gl
{

// 派生の static gen() / del() を CRTP で呼ぶ。デストラクタからは仮想関数が派生へ届かないため
template <typename Derived> class HandleBase
{
  public:
    HandleBase() = default;
    ~HandleBase()
    {
        reset();
    }

    HandleBase(const HandleBase &) = delete;
    HandleBase &operator=(const HandleBase &) = delete;

    HandleBase(HandleBase &&other) noexcept : id_(other.id_)
    {
        other.id_ = 0;
    }
    HandleBase &operator=(HandleBase &&other) noexcept
    {
        if (this != &other)
        {
            reset();
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }

    void create()
    {
        reset(Derived::gen());
    }

    // 外部で生成された ID の所有権を引き取る
    void reset(GLuint id = 0)
    {
        if (id_ != 0)
            Derived::del(id_);
        id_ = id;
    }

    GLuint get() const
    {
        return id_;
    }

    operator GLuint() const
    {
        return id_;
    }

  private:
    GLuint id_ = 0;
};

class VertexArrayHandle : public HandleBase<VertexArrayHandle>
{
  public:
    static GLuint gen()
    {
        GLuint id = 0;
        glGenVertexArrays(1, &id);
        return id;
    }
    static void del(GLuint id)
    {
        glDeleteVertexArrays(1, &id);
    }
};

class BufferHandle : public HandleBase<BufferHandle>
{
  public:
    static GLuint gen()
    {
        GLuint id = 0;
        glGenBuffers(1, &id);
        return id;
    }
    static void del(GLuint id)
    {
        glDeleteBuffers(1, &id);
    }
};

class TextureHandle : public HandleBase<TextureHandle>
{
  public:
    static GLuint gen()
    {
        GLuint id = 0;
        glGenTextures(1, &id);
        return id;
    }
    static void del(GLuint id)
    {
        glDeleteTextures(1, &id);
    }
};

class FramebufferHandle : public HandleBase<FramebufferHandle>
{
  public:
    static GLuint gen()
    {
        GLuint id = 0;
        glGenFramebuffers(1, &id);
        return id;
    }
    static void del(GLuint id)
    {
        glDeleteFramebuffers(1, &id);
    }
};

class RenderbufferHandle : public HandleBase<RenderbufferHandle>
{
  public:
    static GLuint gen()
    {
        GLuint id = 0;
        glGenRenderbuffers(1, &id);
        return id;
    }
    static void del(GLuint id)
    {
        glDeleteRenderbuffers(1, &id);
    }
};

} // namespace gl
