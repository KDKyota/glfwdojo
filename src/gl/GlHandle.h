#pragma once
#include <glad/glad.h>

namespace gl {

/**
 * @brief OpenGL オブジェクトの ID を RAII 管理する基底クラス。
 *
 * CRTP で派生の static gen()/del() を呼ぶ。
 */
template <typename Derived>
class HandleBase {
  public:
    HandleBase() = default;
    ~HandleBase() {
        reset();
    }

    HandleBase(const HandleBase &) = delete;
    HandleBase &operator=(const HandleBase &) = delete;

    HandleBase(HandleBase &&other) noexcept : id_(other.id_) {
        other.id_ = 0;
    }
    HandleBase &operator=(HandleBase &&other) noexcept {
        if (this != &other) {
            reset();
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }

    /**
     * @brief 新しい OpenGL オブジェクトを生成する。
     */
    void create() {
        reset(Derived::gen());
    }

    // 外部で生成された ID の所有権を引き取る
    void reset(GLuint id = 0) {
        if (id_ != 0)
            Derived::del(id_);
        id_ = id;
    }

    GLuint get() const {
        return id_;
    }

    operator GLuint() const {
        return id_;
    }

  private:
    GLuint id_ = 0;
};

/**
 * @brief VAO を管理する。
 */
class VertexArrayHandle : public HandleBase<VertexArrayHandle> {
  public:
    static GLuint gen() {
        GLuint id = 0;
        glGenVertexArrays(1, &id);
        return id;
    }
    static void del(GLuint id) {
        glDeleteVertexArrays(1, &id);
    }
};

/**
 * @brief VBO・EBO・UBO などのバッファを管理する。
 */
class BufferHandle : public HandleBase<BufferHandle> {
  public:
    static GLuint gen() {
        GLuint id = 0;
        glGenBuffers(1, &id);
        return id;
    }
    static void del(GLuint id) {
        glDeleteBuffers(1, &id);
    }
};

/**
 * @brief テクスチャを管理する。
 */
class TextureHandle : public HandleBase<TextureHandle> {
  public:
    static GLuint gen() {
        GLuint id = 0;
        glGenTextures(1, &id);
        return id;
    }
    static void del(GLuint id) {
        glDeleteTextures(1, &id);
    }
};

/**
 * @brief FBO を管理する。
 */
class FramebufferHandle : public HandleBase<FramebufferHandle> {
  public:
    static GLuint gen() {
        GLuint id = 0;
        glGenFramebuffers(1, &id);
        return id;
    }
    static void del(GLuint id) {
        glDeleteFramebuffers(1, &id);
    }
};

/**
 * @brief RBO を管理する。
 */
class RenderbufferHandle : public HandleBase<RenderbufferHandle> {
  public:
    static GLuint gen() {
        GLuint id = 0;
        glGenRenderbuffers(1, &id);
        return id;
    }
    static void del(GLuint id) {
        glDeleteRenderbuffers(1, &id);
    }
};

/**
 * @brief 時間計測やオクルージョンに使うクエリを管理する。
 */
class QueryHandle : public HandleBase<QueryHandle> {
  public:
    static GLuint gen() {
        GLuint id = 0;
        glGenQueries(1, &id);
        return id;
    }
    static void del(GLuint id) {
        glDeleteQueries(1, &id);
    }
};

} // namespace gl
