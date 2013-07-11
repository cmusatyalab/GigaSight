CPP_FLAGS = -Wall -Werror

CPP_FILES = Main.cpp \
			HTTPServer/Client.cpp \
			HTTPServer/connection.cpp \
			HTTPServer/connection_manager.cpp \
			HTTPServer/EmulateAndroidClient.cpp \
			HTTPServer/reply.cpp \
			HTTPServer/request_handler.cpp \
			HTTPServer/request_parser.cpp \
			HTTPServer/server.cpp \
			ImageProcessing/AESEncryption.cpp \
			ImageProcessing/ImageProcessing.cpp \
			ImageProcessing/VideoDecoding.cpp \
			ImageProcessing/VideoEncoding.cpp \
			ResourceManagement/ProcessingRuleContainer.cpp \
			ResourceManagement/SegmentInformation.cpp \
			ResourceManagement/StreamInformation.cpp \
			UploadServer/MP4FileUpload.cpp \
			UploadServer/SocketUtils.cpp \
			UploadServer/UploadManager.cpp \
			UploadServer/UploadThread.cpp

LD_FLAGS = -pthread \
		   -lboost_filesystem \
		   -lboost_system \
		   -lboost_thread \
		   -lcrypto \
		   -ljansson \
		   -lopencv_contrib \
		   -lopencv_core \
		   -lopencv_highgui \
		   -lopencv_imgproc \
		   -lopencv_objdetect

default: build Main.cpp

build:
	g++	$(CPP_FLAGS) \
		$(CPP_FILES) \
		$(LD_FLAGS) \
		-o VideoDenaturing

clean: VideoDenaturing
	rm VideoDenaturing
