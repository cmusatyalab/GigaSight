/*
 * AESEncryption.h
 *
 *  Created on: Nov 26, 2012
 *      Author: yuxiao
 */

#ifndef AESENCRYPTION_H_
#define AESENCRYPTION_H_
#include <openssl/evp.h>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "SharedQueue.h"

class AESEncryption {
public:
	AESEncryption();
	virtual ~AESEncryption();

	void run(SharedQueue<std::pair<std::string, std::string> >* queue);
	void stop();
	void setBlockSize(int i);
	int getBlockSize();
	void setKey(const std::string key);
	//void aes_encrypt_file(const boost::filesystem3::path& path);
	//void aes_encrypt_file(const boost::filesystem3::path& path, unsigned char* key_data, const std::string keyFile);
	//void aes_decrypt_file(const boost::filesystem3::path& path, const boost::filesystem3::path& keyPath);
	void aes_decrypt_file(const boost::filesystem3::path& path, const boost::filesystem3::path& keyPath, const boost::filesystem3::path& logPath);

	void aes_encrypt_file(const boost::filesystem3::path& path, unsigned char* key_data, const boost::filesystem3::path& outputPath);
private:
	/* "opaque" encryption, decryption ctx structures that libcrypto uses to record
				     status of enc/dec operations */
	EVP_CIPHER_CTX en, de;
	unsigned char *key_data;
	int key_data_len;
	int AES_BLOCK_SIZE;
	int aes_init(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx, EVP_CIPHER_CTX *d_ctx);
	unsigned char* aes_encrypt(EVP_CIPHER_CTX *e, unsigned char *plaintext, int *len);
	unsigned char* aes_decrypt(EVP_CIPHER_CTX *e, unsigned char *ciphertext, int *len);
	int aes_init_decryption(EVP_CIPHER_CTX *d_ctx, unsigned char* salt);
	int aes_init_encryption(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx, int nrounds);
	//std::string videoFileName;
	std::string keyFile;
	SharedQueue<std::pair<std::string, std::string> > *m_queue;
	bool bEnd;

};

#endif /* AESENCRYPTION_H_ */
