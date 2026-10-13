# glfwdojo {#mainpage}

LearnOpenGL で OpenGL を学習し、
それをモダンな C++ コードにして、
様々なレンダリングや 3D モデルなどの要素を付け足しています。

## 描画パイプライン

@dot
digraph pipeline {
    rankdir=LR;
    node [shape=box, style=rounded];
    shadow [label="Shadow",   URL="\ref gl::ShadowPass"];
    geom   [label="Geometry", URL="\ref gl::GeometryPass"];
    shadow -> geom;
}
@enddot

## 主要なクラス

詳しくは @ref gl::GeometryPass を参照
