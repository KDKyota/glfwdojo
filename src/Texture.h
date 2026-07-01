#pragma once
#include <string>

class Texture {
private:
	unsigned int id_; // OpenGLテスクチャのID
	std::string type_;
	//bool owner_; // このインスタンスがIDを所有しているか
	/*
	* owner_は所有権をコピーした時に自分がその所有権を持つのかということを明示しておくためのもの
	* これがないとコピーした側が破棄されたときに
	* コピーされた側はづ出に存在しないIDをデストラクタで解放してしまう
	*/
	std::string path_; 
	/*
	* ここでchar* にしなかった理由としては呼び出し物との文字列が破壊されていた時に
	* ダングリングポインタ（すでに解放されたり破壊されたポインタを検索し続ける）になってしまう
	*/
	bool flip_;

public:
	/*
	* Textureコントラクタ
	* 画像のパスpathと垂直反転フラグflipを受け取りロードを行う
	*/
	Texture(const char* path, const bool flip);
	/*
	* Textureデストラクタ
	* id_が0でないとき時だけglDeleteTextureを呼ぶ
	*/
	~Texture();

	// コピー禁止
	Texture(const Texture&) = delete; // コピーコントラクタ禁止
	Texture& operator=(const Texture&) = delete; // コピー演算禁止

	// ムーブ・右辺値代入は許可（shared_ptr内部で利用される）
	// 例外処理を投げない
	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&&) noexcept;

	void bind(unsigned int unit) const; // glActivateTexture + glBindTextureをまとめる
	unsigned int getID() const; // IDの取得
};
