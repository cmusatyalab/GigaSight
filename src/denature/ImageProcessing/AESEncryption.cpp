/*
 * AESEncryption.cpp
 *
 *  Created on: Nov 26, 2012
 *      Author: yuxiao
 */

#include "AESEncryption.h"
#include "boost/date_time/posix_time/posix_time.hpp"

AESEncryption::AESEncryption() {
	// TODO Auto-generated constructor stub

	key_data = NULL;
	keyFile.assign("key");
	key_data_len = 32;
	AES_BLOCK_SIZE = 16;

	m_queue = NULL;
	bEnd = false;
}

AESEncryption::~AESEncryption() {
	// TODO Auto-generated destructor stub
	keyFile.clear();
}

void AESEncryption::setBlockSize(int i){

	AES_BLOCK_SIZE = i;

}

int AESEncryption::getBlockSize(){
	return AES_BLOCK_SIZE;
}

void AESEncryption::setKey(const std::string key){

	memcpy(key_data, key.c_str(), key.length());
}


/**
 * Create an 256 bit key and IV using the supplied key_data. salt can be added for taste.
 * Fills in the encryption and decryption ctx objects and returns 0 on success
 **/
int AESEncryption::aes_init(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx,
             EVP_CIPHER_CTX *d_ctx)
{

  int i, nrounds = 5;
  unsigned char key[32], iv[32];
  memset(iv, 0, 32);
  memset(key,0, 32);

  /*
   * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
   * nrounds is the number of times the we hash the material. More rounds are more secure but
   * slower.
   */
  i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);

  if (i != 32) {
    printf("Key size is %d bits - should be 256 bits\n", i);
    return -1;
  }

  //std::cout << "key " << key << " iv " << iv << std::endl;


  EVP_CIPHER_CTX_init(e_ctx);
  EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
  EVP_CIPHER_CTX_init(d_ctx);
  EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);

  std::ofstream keyStream;
  keyStream.open("key", std::ios::out);

  if (keyStream.is_open()){
	  keyStream << iv;
	  //keyStream.write(reinterpret_cast<char*>(&iv[0]), 32);
  }

  keyStream.close();

  memset(key, 0, 32);
  memset(iv, 0, 32);

  return 0;
}


int AESEncryption::aes_init_encryption(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx, int nrounds)
{

	  int i;
	  unsigned char key[32], iv[32];

	  /*
	   * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
	   * nrounds is the number of times the we hash the material. More rounds are more secure but
	   * slower.
	   */
	  i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);

	  if (i != 32) {
		printf("Key size is %d bits - should be 256 bits\n", i);
		return -1;
	  }


	  EVP_CIPHER_CTX_init(e_ctx);
	  EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);


	  std::ofstream keyStream;
	  keyStream.open(keyFile.c_str(), std::ios::out);

	  if (keyStream.is_open()){
		  keyStream << iv;
	  }

	  std::cout << "key " << key << "iv" << iv << std::endl;
	  keyStream.close();

	  return 0;
}


int AESEncryption::aes_init_decryption(EVP_CIPHER_CTX *d_ctx, unsigned char* salt){

		unsigned char key[32], iv[32];
		memset(key, 0, 32);
		memset(iv, 0, 32);

		int nrounds = 5;
		int i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);

		if (i != 32) {
			printf("Key size is %d bits - should be 256 bits\n", i);
			return -1;
		}

		std::ifstream keyStream;
		keyStream.open(keyFile.c_str(), std::ios::in | std::ios::binary);

		if (keyStream.is_open()){
			keyStream.read((char*)iv, 32);
			//std::cout << "key " << key << "iv" << iv << std::endl;
		}

		keyStream.close();


		//EVP_CIPHER_CTX_init(&en);
		//EVP_EncryptInit_ex(&en, EVP_aes_256_cbc(), NULL, key, iv);

		EVP_CIPHER_CTX_init(d_ctx);
		EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);


		memset(key, 0, 32);
		memset(iv, 0, 32);
		return 0;

}

/*
 * Encrypt *len bytes of data
 * All data going in & out is considered binary (unsigned char[])
 */
unsigned char* AESEncryption::aes_encrypt(EVP_CIPHER_CTX *e, unsigned char *plaintext, int *len)
{
	  /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
	  int c_len = *len + AES_BLOCK_SIZE, f_len = 0;

	  unsigned char* ciphertext = (unsigned char*)malloc(c_len);
	  memset(ciphertext, 0, c_len);

	  /* allows reusing of 'e' for multiple encryption cycles */
	  if(!EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL)){
		  printf("ERROR in EVP_EncryptInit_ex \n");
		  return ciphertext;
	  }

	  /* update ciphertext, c_len is filled with the length of ciphertext generated,
	   *len is the size of plaintext in bytes */
	  if(!EVP_EncryptUpdate(e, ciphertext, &c_len, plaintext, *len)){
		  printf("ERROR in EVP_EncryptUpdate \n");
		  return ciphertext;
	  }

	  /* update ciphertext with the final remaining bytes */
	  if(!EVP_EncryptFinal_ex(e, ciphertext+c_len, &f_len)){
		  printf("ERROR in EVP_EncryptFinal_ex \n");
		  return ciphertext;
	  }

	  *len = c_len + f_len;

	  //std::cout << "plaintext" << plaintext << std::endl;
	  //std::cout << "decrypt" << ciphertext << std::endl;
	  //std::cout << "encrpyted len " << *len << std::endl;

	  return ciphertext;
}

/*
 * Decrypt *len bytes of ciphertext
 */
unsigned char* AESEncryption::aes_decrypt(EVP_CIPHER_CTX *e, unsigned char *ciphertext, int *len)
{
	  /* because we have padding ON, we must allocate an extra cipher block size of memory */
	  int p_len = *len, f_len = 0;

	  //std::cout << "p_len " << p_len << std::endl;

	  unsigned char *plaintext = (unsigned char*) malloc(p_len + AES_BLOCK_SIZE);
	  memset(plaintext, 0, p_len + AES_BLOCK_SIZE);

	  EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL);
	  EVP_DecryptUpdate(e, plaintext, &p_len, ciphertext, *len);
	  EVP_DecryptFinal_ex(e, plaintext+p_len, &f_len);

	  *len = p_len + f_len;
	  //std::cout << "plaintext" << plaintext << std::endl;
	  //std::cout << "decrypt" << ciphertext << std::endl;
	  //std::cout << "flen " << f_len << std::endl;

	  return plaintext;

}


void AESEncryption::aes_encrypt_file(const boost::filesystem3::path& path, unsigned char* key_data, const boost::filesystem3::path& outputPath){

	std::cout << "enter aes_encrypt_file " <<std::endl;
	boost::filesystem3::directory_iterator end_iter;

	if (!boost::filesystem3::exists(path)){
		std::cout << "File does not exist" << std::endl;
		return;
	}

	if (boost::filesystem3::is_directory(path)){

		for( boost::filesystem3::directory_iterator dir_iter(path) ; dir_iter != end_iter ; ++dir_iter)
		{

			aes_encrypt_file((*dir_iter).path(), key_data, outputPath);

		}

	}else{


		boost::posix_time::ptime mst1 = boost::posix_time::microsec_clock::local_time();

		/*if (!boost::filesystem3::exists(outputPath)){
			std::cout << "outputPath does not exist " << path.string() << std::endl;
			keyFile = path.string() + "_encrypted_key";
		}
		else
			keyFile = outputPath.string()+"_key";
*/

		std::cout << "encryption " << path.c_str() << outputPath.filename().string() << std::endl;
		//key_data = (unsigned char*)"PI=3.1415926...";

		if (key_data == NULL){
			std::cout << "key is missing" << std::endl;
			return;
		}


		key_data_len = strlen((char *)key_data)+1;

		std::cout << "key_data_len" << key_data_len << std::endl;
		/* 8 bytes to salt the key_data during key generation. This is an example of
			    compiled in salt. We just read the bit pattern created by these two 4 byte
			    integers on the stack as 64 bits of contigous salt material -
			    ofcourse this only works if sizeof(int) >= 4 */
		unsigned int salt[] = {12345, 54321};

		/* gen key and iv. init the cipher ctx object */
		/*if (aes_init(key_data, key_data_len, (unsigned char *)&salt, &en, &de)) {
		    std::cout << "Couldn't initialize AES cipher" << std::endl;
		    return;
		}*/
		if (aes_init(key_data, key_data_len, (unsigned char *)&salt, &en, &de)){
		    std::cout << "Couldn't initialize AES cipher" << std::endl;
		    return;
		}

		AES_BLOCK_SIZE = 16;
		int len =0;


		std::ofstream outputStream;
		std::ofstream outputLogStream;

		std::string outputFileName, outputLog, comparedVideo;

		//outputFileName = path.filename().string()+"_encrypted";

		if (!boost::filesystem3::exists(outputPath)){
			outputFileName = path.filename().string()+"_encrypted";
		}else
			outputFileName = outputPath.string();

		outputLog = path.filename().string()+"_encrypted_log";
		//comparedVideo = path.filename().string() + "_save";

		std::cout << "write encrypted file to " << outputFileName << std::endl;

		outputStream.open(outputFileName.c_str(), std::ios::out | std::ios::app);

		outputLogStream.open(outputLog.c_str(), std::ios::out | std::ios::app);

		//comparedVideoStream.open(comparedVideo.c_str(), std::ios::out | std::ios::app);

		if ((!outputStream.is_open()) || (!outputLogStream.is_open()))
			 return;


		 std::ifstream inputStream;
		 inputStream.open(path.string().c_str(), std::ios::in | std::ios::binary);

		 if (!inputStream.is_open())
			 return;

		 int buffer_size = 40;
		 int length = 0;

		 // get length of file:
		 inputStream.seekg(0, std::ios::end);
		 length = inputStream.tellg();
		 inputStream.seekg(0, std::ios::beg);

		 std::cout << ";" << length << ";";

	     while (length > 0){

	    	 unsigned char buffer[buffer_size];

	    	 memset(buffer, 0, buffer_size);

			 /* The enc/dec functions deal with binary data and not C strings. strlen() will
			 	    return length of the string without counting the '\0' string marker. We always
			 	    pass in the marker byte to the encrypt/decrypt functions so that after decryption
			 	    we end up with a legal C string */

			 inputStream.read((char *)(&buffer[0]), buffer_size);

			 //comparedVideoStream.write((char*)(&buffer[0]), buffer_size);

	    	 length -= buffer_size;

	    	 len = buffer_size;

			 //unsigned char * ciphertext = aes_encrypt(&en, buffer, &len);


			 /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
			 int c_len = len + AES_BLOCK_SIZE, f_len = 0;

			 unsigned char* ciphertext = (unsigned char*)malloc(c_len);
			 memset(ciphertext, 0, c_len);

				  /* allows reusing of 'e' for multiple encryption cycles */
				  if(!EVP_EncryptInit_ex(&en, NULL, NULL, NULL, NULL)){
					  printf("ERROR in EVP_EncryptInit_ex \n");
					  return;
				  }

				  /* update ciphertext, c_len is filled with the length of ciphertext generated,
				   *len is the size of plaintext in bytes */
				  if(!EVP_EncryptUpdate(&en, ciphertext, &c_len, buffer, len)){
					  printf("ERROR in EVP_EncryptUpdate \n");
					  return;
				  }

				  /* update ciphertext with the final remaining bytes */
				  if(!EVP_EncryptFinal_ex(&en, ciphertext+c_len, &f_len)){
					  printf("ERROR in EVP_EncryptFinal_ex \n");
					  return;
				  }

				  len = c_len + f_len;



			/* unsigned char* plaintext = aes_decrypt(&de, ciphertext, &len);

			 if (memcmp(buffer, plaintext, len) != 0)
				std::cout << "different " << std::endl;

			 free(plaintext);
			 */

			 outputLogStream << len << "\r\n";

			 outputStream.write(reinterpret_cast<char*>(&ciphertext[0]), len);

			 free(ciphertext);

			 memset(buffer, 0, buffer_size);

		 }

		 inputStream.close();
		 outputStream.close();
		 outputLogStream.close();
		 //comparedVideoStream.close();


		boost::posix_time::ptime mst2 = boost::posix_time::microsec_clock::local_time();
    	boost::posix_time::time_duration msdiff = mst2 - mst1;
	    std::cout << mst1.time_of_day() << ";" << mst2.time_of_day() << ";" << msdiff.total_milliseconds() << "ms" << std::endl;



		//keyFile.clear();

	}



}


void AESEncryption::aes_decrypt_file(const boost::filesystem3::path& path, const boost::filesystem3::path& keyPath, const boost::filesystem3::path& logPath){
	//std::cout << "enter aes_encrypt_file " <<std::endl;
		boost::filesystem3::directory_iterator end_iter;

		if (!boost::filesystem3::exists(path)){
			std::cout << "File does not exist" << std::endl;
			return;
		}

		if (boost::filesystem3::is_directory(path)){

			for( boost::filesystem3::directory_iterator dir_iter(path) ; dir_iter != end_iter ; ++dir_iter)
			{

				aes_decrypt_file((*dir_iter).path(), keyPath, logPath);

			}

		}else{


			keyFile.assign(keyPath.c_str());

			key_data = (unsigned char*)"PI=3.1415926...";

			if (key_data == NULL){
				std::cout << "key is missing" << std::endl;
				return;
			}


			key_data_len = strlen((char *)key_data)+1;


			unsigned int salt[] = {12345, 54321};

			/* gen key and iv. init the cipher ctx object */
			if (aes_init_decryption(&de, (unsigned char *)&salt)) {
			    std::cout << "Couldn't initialize AES cipher" << std::endl;
			    return;
			}

			AES_BLOCK_SIZE = 16;
			int len =0;

			//key_data_len = sizeof(key_data)/sizeof(char);

			std::ofstream outputStream;

			std::string outputFileName, inputLog;

			outputFileName = path.filename().string()+"_decrypted";

			if (!boost::filesystem::exists(logPath))
				len = 48;
			else {

				inputLog = logPath.filename().string();

				std::ifstream inputLogStream;
				inputLogStream.open(inputLog.c_str(), std::ios::in);

				if (!inputLogStream.is_open())
					len = 48;
				else {

					std::string temp = "";
					std::getline(inputLogStream, temp);

					if (boost::iequals(temp, ""))
						len = 48;
					else
						len = atoi(temp.c_str());

					temp.clear();
					inputLogStream.close();
				}

			}


			outputStream.open(outputFileName.c_str(), std::ios::out | std::ios::binary);

			if (!outputStream.is_open())
				 return;


			std::ifstream inputStream;
			inputStream.open(path.string().c_str(), std::ios::in | std::ios::binary);

			if (!inputStream.is_open())
				 return;


			inputStream.seekg(0, std::ios::end);
			int size = inputStream.tellg();
			inputStream.seekg(0, std::ios::beg);

			//std::cout << "file length " << size << ";" << sizeof(int) << std::endl;

			while(size > 0){


				size -=len;

				 /* The enc/dec functions deal with binary data and not C strings. strlen() will
				 	    return length of the string without counting the '\0' string marker. We always
				 	    pass in the marker byte to the encrypt/decrypt functions so that after decryption
				 	    we end up with a legal C string */

				unsigned char buffer[len];
				memset(buffer, 0, len);

				int size = len;
				inputStream.read((char *)(&buffer[0]), size);

				unsigned char *ciphertext = new unsigned char[40];

				memset(ciphertext, 0, 40);

				ciphertext = aes_decrypt(&de, buffer, &size);


				outputStream.write(reinterpret_cast<char*>(&ciphertext[0]), size);

				delete [] ciphertext;

			}

			 //std::cout << "written in total " << count << std::endl;
			 //std::cout << size << "bytes not processed" << std::endl;

			 if (outputStream.is_open())
				 outputStream.close();

			 if (inputStream.is_open())
				 inputStream.close();

			 //std::cout << "finished " << std::endl;

			 keyFile.clear();

		}

}



void AESEncryption::run(SharedQueue<std::pair<std::string, std::string> >* queue){

	if (queue == NULL)
		return;

	m_queue = queue;

	unsigned char key_data[32];
	strcpy((char*)key_data,  "PI=3.1415926...");

	std::pair<std::string, std::string> input;

	while (!bEnd){

		m_queue->wait_and_pop(input);

		boost::filesystem3::path p(input.first);
		boost::filesystem3::path q(input.second);

		aes_encrypt_file(p, key_data, q);

		p.clear();
		q.clear();

	}
}



void AESEncryption::stop(){

	bEnd = true;
	m_queue->push(std::pair<std::string, std::string>("", ""));
}
